#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionSymbols.h"
#include "render/OccRenderShaders.h"
#include "navigation/ViewportOrientationTriad.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QQuaternion>
#include <QResizeEvent>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVector2D>
#include <QVector4D>
#include <QWheelEvent>

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepGProp.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GProp_GProps.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Lin.hxx>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <tuple>
#include <utility>
#include <vector>

namespace MotionOcc {
namespace {

using Index = std::uint32_t;
using Triangle = std::array<Index, 3>;
using Edge = std::array<Index, 2>;
using Vec = std::array<double, 3>;
constexpr Index invalidIndex = std::numeric_limits<Index>::max();
constexpr double minSpan = 1e-4; // millimetres
constexpr double maxSpan = 1e30;

Vec vec(const QVector3D& p) { return {p.x(), p.y(), p.z()}; }
Vec subtract(const Vec& a, const Vec& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
double dot(const Vec& a, const Vec& b) { return a[0]*b[0] + a[1]*b[1] + a[2]*b[2]; }
Vec cross(const Vec& a, const Vec& b)
{
    return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}

struct Bounds {
    Vec min{}, max{};
    bool valid = false;

    void add(const Vec& p)
    {
        if (!valid) {
            min = max = p;
            valid = true;
        } else {
            for (int i = 0; i < 3; ++i) {
                min[i] = std::min(min[i], p[i]);
                max[i] = std::max(max[i], p[i]);
            }
        }
    }

    void merge(const Bounds& other)
    {
        if (other.valid) { add(other.min); add(other.max); }
    }

    Vec center() const { return {(min[0]+max[0])*0.5, (min[1]+max[1])*0.5, (min[2]+max[2])*0.5}; }
    Vec radius() const { return {(max[0]-min[0])*0.5, (max[1]-min[1])*0.5, (max[2]-min[2])*0.5}; }
};

struct PartPose {
    QMatrix4x4 restToWorld;
    std::array<Vec, 3> rows, inverseRows;
    Vec translation;

    Vec relativePoint(const Vec& point, const Vec& worldOrigin) const
    {
        const Vec offset = subtract(translation, worldOrigin);
        return {dot(rows[0], point) + offset[0], dot(rows[1], point) + offset[1],
                dot(rows[2], point) + offset[2]};
    }
};

bool makePartPose(const QMatrix4x4& matrix, PartPose& pose)
{
    if (matrix.isIdentity() || !matrix.isAffine()) return false;
    constexpr double rigidTolerance = 1e-5; // allow roundoff in float rotation matrices
    for (int i = 0; i < 3; ++i) {
        pose.translation[i] = matrix(i, 3);
        if (!(std::abs(pose.translation[i]) <= maxSpan)) return false;
        for (int j = 0; j < 3; ++j) pose.rows[i][j] = matrix(i, j);
        for (int j = 0; j <= i; ++j) {
            const double expected = i == j ? 1.0 : 0.0;
            if (!(std::abs(dot(pose.rows[i], pose.rows[j]) - expected) <= rigidTolerance)) return false;
        }
    }
    const auto& r = pose.rows;
    const std::array<Vec, 3> cofactors{cross(r[1], r[2]), cross(r[2], r[0]), cross(r[0], r[1])};
    const double determinant = dot(r[0], cofactors[0]);
    if (!(determinant > 0.0)) return false;
    // Invert the submitted float coefficients in double precision for picking.
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) pose.inverseRows[i][j] = cofactors[j][i] / determinant;
    }
    pose.restToWorld = matrix;
    return true;
}

Bounds posedBounds(const Bounds& rest, const PartPose& pose)
{
    if (!rest.valid) return {};
    const Vec center = pose.relativePoint(rest.center(), {}), radius = rest.radius();
    // Transform the rest AABB conservatively, without visiting any vertices.
    Bounds bounds;
    bounds.valid = true;
    for (int i = 0; i < 3; ++i) {
        const Vec& row = pose.rows[i];
        const double extent = dot({std::abs(row[0]), std::abs(row[1]), std::abs(row[2])}, radius);
        bounds.min[i] = center[i] - extent;
        bounds.max[i] = center[i] + extent;
    }
    return bounds;
}

struct Vertex {
    float position[3];
    float normal[3]{};
    float scalar = 0;
};

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> triangles;
    std::vector<Index> edges;
    std::vector<Index> sourceIndices;
    Bounds bounds;

    Vec position(Index i) const
    {
        const auto& p = vertices[i].position;
        return {p[0], p[1], p[2]};
    }
};

struct Part {
    int id;
    Geometry cad;
    Geometry mesh;
    QVector3D origin; // shared rest-space origin for CAD and FEM rendering
    bool deformed = false;
    bool suppressed = false;

    bool usesMesh(OccViewport::DisplayMode mode) const
    {
        return (deformed || mode == OccViewport::DisplayMode::Mesh) && !mesh.triangles.empty();
    }
};

struct PickCurve {
    TopoDS_Edge edge;
    std::vector<Vec> points;
    std::vector<double> parameters;
};

struct PickGeometry {
    TopoDS_Shape shape;
    TopTools_IndexedMapOfShape faces, edges, vertices, solids;
    std::vector<int> faceSolids;
    std::vector<PickCurve> curves;
    std::vector<Vec> nodes;
    QVector<int> nodeIds;
    std::vector<Edge> meshEdges;
    std::vector<Triangle> meshFaces;
    std::vector<std::array<Index, 4>> elements;
};

void setMeshTopology(PickGeometry& geometry, const FemMesh& mesh)
{
    geometry.nodes = mesh.nodes;
    geometry.meshFaces = mesh.surface;
    geometry.elements = mesh.tetrahedra;
    std::set<Edge> edges;
    const auto add = [&](Index a, Index b) {
        if (a < mesh.nodes.size() && b < mesh.nodes.size() && a != b) edges.insert({std::min(a,b),std::max(a,b)});
    };
    for (const auto& face : mesh.surface) for (int j = 0; j < 3; ++j) add(face[j],face[(j+1)%3]);
    for (const auto& cell : mesh.tetrahedra) for (int j = 0; j < 4; ++j) for (int k = j+1; k < 4; ++k) add(cell[j],cell[k]);
    geometry.meshEdges.assign(edges.begin(),edges.end());
}

Vec vec(const gp_Pnt& p) { return {p.X(), p.Y(), p.Z()}; }

bool samePick(const PickResult& a, const PickResult& b)
{
    return a.kind == b.kind && a.partId == b.partId && a.index == b.index;
}

bool cadPick(PickKind kind)
{
    return kind==PickKind::Part || kind==PickKind::Vertex || kind==PickKind::Edge ||
           kind==PickKind::Face || kind==PickKind::Solid;
}

const TopoDS_Shape* pickTopology(const PickResult& pick,const PickGeometry& geometry)
{
    const TopTools_IndexedMapOfShape* map=nullptr;
    switch(pick.kind) {
    case PickKind::Part:return geometry.shape.IsNull()?nullptr:&geometry.shape;
    case PickKind::Vertex:map=&geometry.vertices;break;
    case PickKind::Edge:map=&geometry.edges;break;
    case PickKind::Face:map=&geometry.faces;break;
    case PickKind::Solid:map=&geometry.solids;break;
    default:return nullptr;
    }
    return pick.index>0 && pick.index<=map->Extent()?&(*map)(pick.index):nullptr;
}

bool validPickReference(const PickResult& pick,const PickGeometry& geometry)
{
    if(!pick.isValid())return false;
    const auto inRange=[&](int count) { return pick.index>0 && pick.index<=count; };
    const auto meshReference=[&](const auto& nodes) {
        if(!pick.nodeIds.isEmpty() && pick.nodeIds.size()!=int(nodes.size()))return false;
        int ordinal=0;
        for(const auto node:nodes) {
            if(node>=geometry.nodes.size())return false;
            if(!pick.nodeIds.isEmpty() && pick.nodeIds[ordinal]!=geometry.nodeIds.value(int(node),int(node+1)))return false;
            ++ordinal;
        }
        return true;
    };
    switch(pick.kind) {
    case PickKind::Part:return !geometry.shape.IsNull();
    case PickKind::Vertex:return inRange(geometry.vertices.Extent());
    case PickKind::Edge:return inRange(geometry.edges.Extent());
    case PickKind::Face:return inRange(geometry.faces.Extent());
    case PickKind::Solid:return inRange(geometry.solids.Extent());
    case PickKind::Node:
        return pick.index>=0 && pick.index<int(geometry.nodes.size()) &&
               (pick.nodeId<=0 || pick.nodeId==geometry.nodeIds.value(pick.index,pick.index+1));
    case PickKind::MeshEdge:
        return inRange(int(geometry.meshEdges.size())) && meshReference(geometry.meshEdges[pick.index-1]);
    case PickKind::MeshFace:
        return inRange(int(geometry.meshFaces.size())) && meshReference(geometry.meshFaces[pick.index-1]);
    case PickKind::Element:
        if(!geometry.elements.empty())
            return inRange(int(geometry.elements.size())) && meshReference(geometry.elements[pick.index-1]);
        return inRange(int(geometry.meshFaces.size())) && meshReference(geometry.meshFaces[pick.index-1]);
    }
    return false;
}

bool normalise(Vec& direction)
{
    const double length = std::sqrt(dot(direction, direction));
    if (!std::isfinite(length) || length < 1e-14) return false;
    for (double& value : direction) value /= length;
    return true;
}

PickGeometry pickGeometry(const CadPart& part, const FemMesh* mesh)
{
    PickGeometry result;
    result.shape = part.shape;
    TopExp::MapShapes(part.shape, TopAbs_FACE, result.faces);
    TopExp::MapShapes(part.shape, TopAbs_EDGE, result.edges);
    TopExp::MapShapes(part.shape, TopAbs_VERTEX, result.vertices);
    TopExp::MapShapes(part.shape, TopAbs_SOLID, result.solids);
    result.faceSolids.resize(std::size_t(result.faces.Extent()) + 1, 0);
    for (int solid=1; solid<=result.solids.Extent(); ++solid) {
        TopTools_IndexedMapOfShape faces; TopExp::MapShapes(result.solids(solid), TopAbs_FACE, faces);
        for (int face=1; face<=faces.Extent(); ++face) {
            const int id=result.faces.FindIndex(faces(face));
            if (id>0) {
                auto& owner=result.faceSolids[std::size_t(id)];
                owner=owner==0 ? solid : (owner==solid ? solid : -1);
            }
        }
    }
    result.curves.resize(result.edges.Extent());
    for (int i = 1; i <= result.edges.Extent(); ++i) {
        auto& out = result.curves[i - 1];
        out.edge = TopoDS::Edge(result.edges(i));
        try {
            if (BRep_Tool::Degenerated(out.edge)) continue;
            BRepAdaptor_Curve curve(out.edge);
            const double first = curve.FirstParameter(), last = curve.LastParameter();
            if (!std::isfinite(first) || !std::isfinite(last) || std::abs(first) > 1e50 || std::abs(last) > 1e50) continue;
            // Adaptive OCC tessellation is only for cursor proximity/highlight.
            // The returned point and derivative are evaluated on the real curve.
            GProp_GProps props;
            BRepGProp::LinearProperties(out.edge, props);
            const double deflection = std::max(1e-7, std::abs(props.Mass()) * 1e-5);
            GCPnts_QuasiUniformDeflection samples(curve, deflection);
            if (samples.IsDone() && samples.NbPoints() >= 2 && samples.NbPoints() < 100000) {
                for (int j = 1; j <= samples.NbPoints(); ++j) {
                    out.points.push_back(vec(samples.Value(j)));
                    out.parameters.push_back(samples.Parameter(j));
                }
            } else {
                for (int j = 0; j <= 64; ++j) {
                    const double parameter = first + (last - first) * j / 64.0;
                    out.points.push_back(vec(curve.Value(parameter)));
                    out.parameters.push_back(parameter);
                }
            }
        } catch (const Standard_Failure&) {
            // A failed edge cannot produce a made-up topology hit.
            out.points.clear(); out.parameters.clear();
        }
    }
    if (mesh) {
        setMeshTopology(result, *mesh);
        result.nodeIds.reserve(int(mesh->nodes.size()));
        for (int i = 0; i < int(mesh->nodes.size()); ++i) result.nodeIds.append(i + 1);
    }
    return result;
}

bool showsEdges(OccViewport::DisplayMode mode, const Geometry& geometry)
{
    return mode != OccViewport::DisplayMode::Shaded || geometry.triangles.empty();
}

void deriveEdges(Geometry& geometry)
{
    // The FEM overlay is the set of actual element edges, not the CAD
    // tessellation or the interior tetrahedron connectivity.
    std::vector<std::uint64_t> keys;
    keys.reserve(geometry.triangles.size());
    for (std::size_t i = 0; i < geometry.triangles.size(); i += 3) {
        for (int j = 0; j < 3; ++j) {
            const Index a = geometry.triangles[i + j], b = geometry.triangles[i + (j + 1) % 3];
            keys.push_back((std::uint64_t(std::min(a, b)) << 32) | std::max(a, b));
        }
    }
    std::sort(keys.begin(), keys.end());
    keys.erase(std::unique(keys.begin(), keys.end()), keys.end());
    geometry.edges.reserve(keys.size() * 2);
    for (const std::uint64_t key : keys) {
        geometry.edges.push_back(Index(key >> 32));
        geometry.edges.push_back(Index(key));
    }
}

template<class PositionAt>
Geometry snapshot(std::size_t count, const std::vector<Triangle>& triangles,
                  const std::vector<Edge>& edges, bool fem, PositionAt positionAt)
{
    Geometry result;
    std::vector<Index> remap(count, invalidIndex);
    const auto mapVertex = [&](Index source) -> Index {
        if (source >= count || result.vertices.size() >= invalidIndex) return invalidIndex;
        Index& mapped = remap[source];
        if (mapped != invalidIndex) return mapped;
        const Vec p = positionAt(source);
        // Check before narrowing Netgen's doubles. Leave headroom for float
        // vertex transforms and picking only primitives present in both buffers.
        for (double value : p) {
            if (!std::isfinite(value) || std::abs(value) > maxSpan) return invalidIndex;
        }
        mapped = Index(result.vertices.size());
        result.vertices.push_back({{float(p[0]), float(p[1]), float(p[2])}});
        result.sourceIndices.push_back(source);
        return mapped;
    };
    result.triangles.reserve(triangles.size() * 3);
    for (const auto& triangle : triangles) {
        const Index a = mapVertex(triangle[0]), b = mapVertex(triangle[1]), c = mapVertex(triangle[2]);
        if (a == invalidIndex || b == invalidIndex || c == invalidIndex) continue;
        const Vec pa = result.position(a), pb = result.position(b), pc = result.position(c);
        const Vec n = cross(subtract(pb, pa), subtract(pc, pa));
        const double length = std::sqrt(dot(n, n));
        if (!(length > 0.0)) continue;
        for (Index index : {a, b, c}) {
            result.triangles.push_back(index);
            // CAD tessellations share nodes within each face. FEM triangles use
            // flat lighting in the shader, preserving their element geometry.
            if (!fem) {
                for (int k = 0; k < 3; ++k) result.vertices[index].normal[k] += float(n[k] / length);
            }
        }
        result.bounds.add(pa);
        result.bounds.add(pb);
        result.bounds.add(pc);
    }
    if (!fem) {
        for (auto& v : result.vertices) {
            const Vec normal{v.normal[0], v.normal[1], v.normal[2]};
            const double length = std::sqrt(dot(normal, normal));
            if (length > 0.0) {
                for (int k = 0; k < 3; ++k) v.normal[k] = float(normal[k] / length);
            }
        }
    }
    result.edges.reserve(edges.size() * 2);
    for (const auto& edge : edges) {
        const Index a = mapVertex(edge[0]), b = mapVertex(edge[1]);
        if (a == invalidIndex || b == invalidIndex || a == b) continue;
        const Vec pa = result.position(a), pb = result.position(b);
        if (pa == pb) continue;
        result.edges.push_back(a);
        result.edges.push_back(b);
        result.bounds.add(pa);
        result.bounds.add(pb);
    }
    if (result.edges.empty()) deriveEdges(result);
    return result;
}

QQuaternion viewRotation(const QVector3D& back, const QVector3D& up)
{
    // Standard directions from E:/codex/Motion/src/ui/navigation/ViewportCamera.cpp:122–138.
    QMatrix4x4 view;
    view.lookAt(back, QVector3D(), up);
    return QQuaternion::fromRotationMatrix(view.normalMatrix()).normalized();
}

QQuaternion isometricRotation() { return viewRotation({1, -1, 1}, {0, 0, 1}); }

struct Frame {
    QMatrix4x4 view;
    QMatrix4x4 projection;
    QMatrix3x3 normal;
    std::array<Vec, 3> rows;
    double nearZ, farZ;
};

struct Camera {
    QQuaternion rotation = isometricRotation(); // world to eye
    QVector3D pivot;
    QPointF pan;
    double span = 100.0; // orthographic half-span of the shorter viewport side, in mm

    double mmPerPixel(const QSize& size) const
    {
        return 2.0 * span / std::max(1, std::min(size.width(), size.height()));
    }

    void zoomAt(double logarithmicScale, const QPointF& position, const QSize& size)
    {
        if (!std::isfinite(logarithmicScale) || !std::isfinite(position.x()) || !std::isfinite(position.y())) return;
        const double before = span;
        span = std::clamp(before * std::exp(std::clamp(logarithmicScale, -50.0, 50.0)), minSpan, maxSpan);
        const QPointF offset(position.x() - size.width()*0.5, size.height()*0.5 - position.y());
        pan += offset * (mmPerPixel(size) * (1.0 - before/span));
    }

    void setPivot(const Vec& point)
    {
        const QVector3D next{float(point[0]), float(point[1]), float(point[2])};
        if (!std::isfinite(next.x()) || !std::isfinite(next.y()) || !std::isfinite(next.z())) return;
        const Vec delta = subtract(vec(next), vec(pivot));
        QMatrix4x4 matrix; matrix.rotate(rotation);
        // This camera stores world-to-eye rotation and a positive eye offset.
        // Compensate with the actual stored pivot so the displayed scene cannot jump.
        pan += QPointF(matrix(0,0)*delta[0] + matrix(0,1)*delta[1] + matrix(0,2)*delta[2],
                       matrix(1,0)*delta[0] + matrix(1,1)*delta[1] + matrix(1,2)*delta[2]);
        pivot = next;
    }

    Frame frame(const QSize& size, const Bounds& bounds) const
    {
        const double scale = mmPerPixel(size) * 0.5;
        const float halfWidth = float(scale * std::max(1, size.width()));
        const float halfHeight = float(scale * std::max(1, size.height()));
        Frame out{};
        out.view.rotate(rotation);
        out.normal = out.view.normalMatrix();
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) out.rows[i][j] = out.view(i, j);
        }
        out.view(0, 3) = float(pan.x());
        out.view(1, 3) = float(pan.y());
        out.projection.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);

        // Normalize the depth interval independently of orthographic framing.
        // Scene updates (including hide/show) never change pivot, pan or scale.
        const Vec& back = out.rows[2];
        const Vec center = subtract(bounds.center(), vec(pivot));
        const Vec radius = bounds.radius();
        const double z = bounds.valid ? dot(back, center) : 0.0;
        const double depth = bounds.valid ? dot({std::abs(back[0]), std::abs(back[1]), std::abs(back[2])}, radius) : span;
        const double precision = (std::abs(z) + depth + span) * (32.0 * std::numeric_limits<float>::epsilon());
        const double halfDepth = depth + std::max({depth * 0.04, precision, 1e-6});
        out.nearZ = z + halfDepth;
        out.farZ = z - halfDepth;
        for (int i = 0; i < 3; ++i) out.view(2, i) = float(back[i] / halfDepth);
        out.view(2, 3) = float(-z / halfDepth);
        return out;
    }

    void fit(const Bounds& bounds, const QSize& size, bool shown)
    {
        const Vec c = bounds.center(), r = bounds.radius();
        pivot = {float(c[0]), float(c[1]), float(c[2])};
        pan = {};
        // Before layout/show, a sphere is independent of the eventual aspect.
        double halfSpan = std::sqrt(dot(r, r));
        if (shown && !size.isEmpty()) {
            const auto ex = vec(rotation.rotatedVector({1, 0, 0}));
            const auto ey = vec(rotation.rotatedVector({0, 1, 0}));
            const auto ez = vec(rotation.rotatedVector({0, 0, 1}));
            const double halfWidth = r[0]*std::abs(ex[0]) + r[1]*std::abs(ey[0]) + r[2]*std::abs(ez[0]);
            const double halfHeight = r[0]*std::abs(ex[1]) + r[1]*std::abs(ey[1]) + r[2]*std::abs(ez[1]);
            const double shortSide = std::min(size.width(), size.height());
            halfSpan = std::max(halfWidth * shortSide / size.width(), halfHeight * shortSide / size.height());
        }
        span = std::clamp(halfSpan * 1.15, minSpan, maxSpan);
    }
};

// Adapted from the donor trackball and cursor-anchored zoom (ViewportCamera.cpp:208–252).
QVector3D trackball(const QPointF& position, const QPointF& center, double radius)
{
    const double x = (position.x() - center.x()) / radius;
    const double y = (center.y() - position.y()) / radius;
    const double r = std::hypot(x, y);
    const double z = r <= std::sqrt(0.5) ? std::sqrt(1.0-r*r) : 0.5/r;
    const double length = std::hypot(r, z);
    return {float(x/length), float(y/length), float(z/length)};
}

struct Ray { Vec origin, direction; };

Ray restRay(const Ray& ray, const PartPose* pose)
{
    if (!pose) return ray;
    const Vec origin = subtract(ray.origin, pose->translation);
    const auto& r = pose->inverseRows;
    // Do not renormalize: the ray parameter is shared by all parts in world space.
    return {{dot(r[0], origin), dot(r[1], origin), dot(r[2], origin)},
            {dot(r[0], ray.direction), dot(r[1], ray.direction), dot(r[2], ray.direction)}};
}

// Slab broad phase and double-sided triangle intersection adapted from
// E:/codex/Motion/src/ui/cad/CadPicking.cpp:30–61. No OCC/Chrono picking calls.
bool intersectsBounds(const Ray& ray, const Bounds& bounds, double limit, double padding)
{
    if (!bounds.valid) return false;
    double enter = 0.0, leave = limit;
    for (int i = 0; i < 3; ++i) {
        const double lo = bounds.min[i] - padding, hi = bounds.max[i] + padding;
        if (std::abs(ray.direction[i]) < 1e-15) {
            if (ray.origin[i] < lo || ray.origin[i] > hi) return false;
        } else {
            double a = (lo-ray.origin[i]) / ray.direction[i];
            double b = (hi-ray.origin[i]) / ray.direction[i];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a);
            leave = std::min(leave, b);
            if (enter > leave) return false;
        }
    }
    return true;
}

bool intersectsTriangle(const Ray& ray, const Vec& a, const Vec& b, const Vec& c, double& t)
{
    const Vec e1 = subtract(b, a), e2 = subtract(c, a), p = cross(ray.direction, e2);
    const double det = dot(e1, p);
    const double scale = std::sqrt(dot(e1, e1)*dot(e2, e2));
    if (scale <= 0.0 || std::abs(det) <= scale*1e-12) return false;
    const Vec s = subtract(ray.origin, a);
    const double u = dot(s, p) / det;
    if (u < -1e-10 || u > 1.0+1e-10) return false;
    const Vec q = cross(s, e1);
    const double v = dot(ray.direction, q) / det;
    if (v < -1e-10 || u+v > 1.0+1e-10) return false;
    t = dot(e2, q) / det;
    return t >= 0.0 && std::isfinite(t);
}

struct Buffers {
    GLuint vao = 0, vertices = 0, triangles = 0, edges = 0;
    GLsizei triangleCount = 0, edgeCount = 0;
};

struct PartBuffers { Buffers cad, mesh; };

using PickBufferKey = std::tuple<int, PickKind, int>;

// Motion b636535: ScreenStrokeRenderer.h. Two depth-buffer quanta resolve
// coplanar raster ties without pulling hidden CAD edges through front faces.
float coplanarDepthBias(QOpenGLFunctions_3_3_Core& gl)
{
    GLint framebuffer = 0, type = GL_NONE, bits = 0;
    gl.glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    const GLenum attachment = framebuffer ? GL_DEPTH_ATTACHMENT : GL_DEPTH;
    gl.glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment,
                                              GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);
    if (type != GL_NONE)
        gl.glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, attachment,
                                                  GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE, &bits);
    return bits > 0 ? float(4.0 / (std::ldexp(1.0, std::min(bits, 24)) - 1.0)) : 0.0f;
}

Geometry cadPickGeometry(const PickGeometry& source, const PickResult& pick)
{
    Geometry result;
    const auto point = [&](const Vec& p) -> Index {
        for (double v : p) if (!std::isfinite(v) || std::abs(v) > maxSpan) return invalidIndex;
        if (result.vertices.size() >= invalidIndex) return invalidIndex;
        const Index id = Index(result.vertices.size());
        result.vertices.push_back({{float(p[0]), float(p[1]), float(p[2])}});
        result.bounds.add(p);
        return id;
    };
    const auto curve = [&](const PickCurve& edge) {
        Index previous = invalidIndex;
        for (const auto& p : edge.points) {
            const Index next = point(p);
            if (previous != invalidIndex && next != invalidIndex)
                result.edges.insert(result.edges.end(), {previous, next});
            previous = next;
        }
    };
    if (pick.kind == PickKind::Edge && pick.index > 0 && pick.index <= int(source.curves.size())) {
        curve(source.curves[pick.index - 1]);
    } else if ((pick.kind == PickKind::Face && pick.index > 0 && pick.index <= source.faces.Extent()) ||
               (pick.kind == PickKind::Solid && pick.index > 0 && pick.index <= source.solids.Extent())) {
      TopTools_IndexedMapOfShape faces;
      if (pick.kind==PickKind::Face) faces.Add(source.faces(pick.index));
      else TopExp::MapShapes(source.solids(pick.index), TopAbs_FACE, faces);
      for (int faceId=1; faceId<=faces.Extent(); ++faceId) {
        const auto face = TopoDS::Face(faces(faceId));
        TopLoc_Location location;
        const auto mesh = BRep_Tool::Triangulation(face, location);
        if (!mesh.IsNull()) {
            std::vector<Index> indices(std::size_t(mesh->NbNodes()) + 1, invalidIndex);
            for (int i = 1; i <= mesh->NbNodes(); ++i)
                indices[std::size_t(i)] = point(vec(mesh->Node(i).Transformed(location.Transformation())));
            for (int i = 1; i <= mesh->NbTriangles(); ++i) {
                int a, b, c; mesh->Triangle(i).Get(a, b, c);
                if (indices[a] != invalidIndex && indices[b] != invalidIndex && indices[c] != invalidIndex)
                    result.triangles.insert(result.triangles.end(), {indices[a], indices[b], indices[c]});
            }
        }
        for (TopExp_Explorer edge(face, TopAbs_EDGE); edge.More(); edge.Next()) {
            const int index = source.edges.FindIndex(edge.Current());
            if (index > 0) curve(source.curves[index - 1]);
        }
      }
    }
    return result;
}

void deleteBuffers(QOpenGLFunctions_3_3_Core& gl, const Buffers& b)
{
    if (b.vao) gl.glDeleteVertexArrays(1, &b.vao);
    const GLuint names[] = {b.vertices, b.triangles, b.edges};
    gl.glDeleteBuffers(3, names);
}

bool bufferGeometry(QOpenGLFunctions_3_3_Core& gl, const Geometry& source, Buffers& b)
{
    if (!source.bounds.valid) return true;
    constexpr auto maxCount = std::size_t(std::numeric_limits<GLsizei>::max());
    constexpr auto maxBytes = std::size_t(std::numeric_limits<GLsizeiptr>::max());
    if (source.triangles.size() > std::min(maxCount, maxBytes / sizeof(Index)) ||
        source.edges.size() > std::min(maxCount, maxBytes / sizeof(Index)) ||
        source.vertices.size() > maxBytes / sizeof(Vertex)) return false;
    gl.glGenVertexArrays(1, &b.vao);
    gl.glGenBuffers(1, &b.vertices);
    if (!b.vao || !b.vertices) return false;
    gl.glBindVertexArray(b.vao);
    gl.glBindBuffer(GL_ARRAY_BUFFER, b.vertices);
    gl.glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(source.vertices.size() * sizeof(Vertex)),
                    source.vertices.data(), GL_STATIC_DRAW);
    gl.glEnableVertexAttribArray(0);
    gl.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
    gl.glEnableVertexAttribArray(1);
    gl.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             reinterpret_cast<const void*>(offsetof(Vertex, normal)));
    gl.glEnableVertexAttribArray(2);
    gl.glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                             reinterpret_cast<const void*>(offsetof(Vertex, scalar)));
    const auto indices = [&](const std::vector<Index>& data, GLuint& name, GLsizei& count) {
        if (data.empty()) return true;
        gl.glGenBuffers(1, &name);
        if (!name) return false;
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, name);
        gl.glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizeiptr(data.size() * sizeof(Index)), data.data(), GL_STATIC_DRAW);
        count = GLsizei(data.size());
        return true;
    };
    return indices(source.triangles, b.triangles, b.triangleCount) && indices(source.edges, b.edges, b.edgeCount);
}

bool linkProgram(QOpenGLShaderProgram& program, const char* vertex, const char* fragment,
                 const char* geometry = nullptr)
{
    return program.addShaderFromSourceCode(QOpenGLShader::Vertex, vertex) &&
           (!geometry || program.addShaderFromSourceCode(QOpenGLShader::Geometry, geometry)) &&
           program.addShaderFromSourceCode(QOpenGLShader::Fragment, fragment) && program.link();
}

QVector3D rgb(int r, int g, int b) { return {r/255.0f, g/255.0f, b/255.0f}; }
// A muted version of the #96c7ed to #e1eefa MxMotion viewport gradient
// (GUI/src/MainWindow/mMenuWidget.cpp), paired with slate CAD parts.
const QVector3D backdropTop = rgb(207, 225, 241);
const QVector3D backdropBottom = rgb(242, 247, 252);

} // namespace

struct OccViewport::State {
    std::vector<Part> parts;
    std::map<int, PickGeometry> pickGeometry;
    std::set<int> partIds; // includes hidden document parts
    std::map<int, PartPose> poses; // only non-identity poses
    std::map<int, Geometry> restMeshes;
    std::set<int> dirtyMeshParts;
    Bounds bounds;
    Camera camera;
    std::vector<Camera> cameraHistory;
    int cameraHistoryPosition = -1;
    QTimer* autoRotationTimer = nullptr;
    QTimer* triadHistoryTimer = nullptr;
    QElapsedTimer autoRotationClock;
    QVector3D autoRotationAxis{1, 0, 0};
    double autoRotationSign = 1.0;
    OrientationTriadCamera triadCamera;
    std::unique_ptr<ViewportOrientationTriad> triad;
    DisplayMode mode = DisplayMode::ShadedEdges;
    int selected = -1;
    bool pickEnabled = true;
    std::optional<PickKind> selectionFilter = PickKind::Part;
    QVector<PickResult> viewportSelection;
    bool pickActive = false, pickMultiple = false, pickFeedbackVisible = true;
    PickKind pickKind = PickKind::Part;
    QColor pickColor{232,130,28};
    std::set<int> pickParts;
    QVector<PickResult> picks;
    struct PickLayer { QVector<PickResult> picks; QColor color; };
    struct PointLayer { QVector<QVector3D> points; QColor color; float size=10.f; };
    std::map<QString, PickLayer> auxiliaryPicks;
    std::map<QString, PointLayer> auxiliaryPoints;
    PickResult hover;
    bool boxSelecting = false;
    QRectF pickRectangle;
    std::set<int> scalarParts;
    double scalarMinimum = 0, scalarMaximum = 1;
    QString scalarTitle;
    QVector<MotionSymbol> symbols;
    std::map<QString, QVector<MotionSymbol>> auxiliarySymbols;

    Qt::MouseButton button = Qt::NoButton;
    enum class Navigation { None, Select, Orbit, Pan, Zoom };
    MouseNavigationMode navigationMode = MouseNavigationMode::Classic;
    Navigation navigation = Navigation::None;
    bool dragging = false;
    QPointF press, last, ballCenter, zoomAnchor;
    QQuaternion pressRotation;
    QVector3D ballStart;
    double ballRadius = 1.0;

    QOpenGLFunctions_3_3_Core* gl = nullptr; // owned by the current widget context
    QMetaObject::Connection destruction;
    std::unique_ptr<QOpenGLShaderProgram> backdrop, surface, edge, point;
    GLuint emptyVao = 0;
    std::vector<PartBuffers> buffers;
    std::map<PickBufferKey, Buffers> pickBuffers;
    bool ready = false, sceneDirty = true, uploadFailed = false;
    QString lastError;

    void rememberCamera()
    {
        if (cameraHistoryPosition >= 0) {
            const auto& previous = cameraHistory[std::size_t(cameraHistoryPosition)];
            const double norm=std::sqrt(double(previous.rotation.lengthSquared())*double(camera.rotation.lengthSquared()));
            const double orientation = norm>0 ? std::abs(double(QQuaternion::dotProduct(previous.rotation, camera.rotation)))/norm : 0;
            if (orientation > 1.0 - 1e-7 && previous.pivot == camera.pivot
                && previous.pan == camera.pan && previous.span == camera.span) return;
        }
        cameraHistory.resize(std::size_t(cameraHistoryPosition + 1));
        cameraHistory.push_back(camera);
        constexpr std::size_t maximumViews = 100;
        if (cameraHistory.size() > maximumViews) cameraHistory.erase(cameraHistory.begin());
        cameraHistoryPosition = int(cameraHistory.size()) - 1;
    }

    const PartPose* poseFor(int id) const
    {
        const auto found = poses.find(id);
        return found == poses.end() ? nullptr : &found->second;
    }

    void updateBounds()
    {
        bounds = {};
        for (const auto& part : parts) {
            if (part.suppressed) continue;
            const auto* pose = poseFor(part.id);
            if (!part.deformed) bounds.merge(pose ? posedBounds(part.cad.bounds, *pose) : part.cad.bounds);
            bounds.merge(pose ? posedBounds(part.mesh.bounds, *pose) : part.mesh.bounds);
        }
    }

    void beginOrbit(const QPointF& position, const QSize& size)
    {
        const double unit = camera.mmPerPixel(size);
        ballCenter = QPointF(size.width()*0.5 + camera.pan.x()/unit, size.height()*0.5 - camera.pan.y()/unit);
        ballRadius = std::max(1.0, 0.5 * std::min(size.width(), size.height()));
        ballStart = trackball(position, ballCenter, ballRadius);
        pressRotation = camera.rotation;
    }

    // Adapted from Motion b636535 SceneViewport::UpdateNxNavigation.
    // Re-anchor modifier transitions; the transition event itself never moves the view.
    bool updateNxNavigation(Qt::KeyboardModifiers modifiers, const QPointF& position, const QSize& size)
    {
        const auto next = modifiers.testFlag(Qt::ControlModifier) ? Navigation::Zoom :
                          modifiers.testFlag(Qt::ShiftModifier) ? Navigation::Pan : Navigation::Orbit;
        if (navigation == next) return false;
        navigation = next;
        last = zoomAnchor = position;
        if (next == Navigation::Orbit) beginOrbit(position, size);
        return true;
    }

    void dragTo(const QPointF& position, const QSize& size)
    {
        if (button == Qt::NoButton) return;
        if (!dragging) {
            const auto delta = position - press;
            if (std::abs(delta.x()) + std::abs(delta.y()) < QApplication::startDragDistance()) return;
            dragging = true;
        }
        if (navigation == Navigation::Orbit) {
            const auto delta = QQuaternion::rotationTo(ballStart, trackball(position, ballCenter, ballRadius));
            camera.rotation = (delta * pressRotation).normalized();
        } else if (navigation == Navigation::Pan) {
            const QPointF delta = position - last;
            camera.pan += QPointF(delta.x(), -delta.y()) * camera.mmPerPixel(size);
        } else if (navigation == Navigation::Zoom) {
            camera.zoomAt((position.y() - last.y()) * 0.006, zoomAnchor, size);
        }
        last = position;
    }
};

OccViewport::OccViewport(QWidget* parent) : QOpenGLWidget(parent), m_state(std::make_unique<State>())
{
    auto format = QSurfaceFormat::defaultFormat();
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    setFormat(format);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setToolTip(mouseNavigationHint());
    qRegisterMetaType<QVector<MotionOcc::PickResult>>();
    m_state->rememberCamera();
    m_state->triadHistoryTimer = new QTimer(this);
    m_state->triadHistoryTimer->setSingleShot(true);
    connect(m_state->triadHistoryTimer, &QTimer::timeout, this, [this] { m_state->rememberCamera(); });
    m_state->autoRotationTimer = new QTimer(this);
    m_state->autoRotationTimer->setInterval(16);
    connect(m_state->autoRotationTimer, &QTimer::timeout, this, [this] {
        auto& state = *m_state;
        const double seconds = std::min(0.1, state.autoRotationClock.restart() / 1000.0);
        if (!isVisible() || state.button != Qt::NoButton) return;
        const auto delta = QQuaternion::fromAxisAndAngle(state.autoRotationAxis,
            float(state.autoRotationSign * 30.0 * seconds));
        state.camera.rotation = (delta * state.camera.rotation).normalized();
        state.hover = {};
        update();
    });
    auto& triadCamera = m_state->triadCamera;
    triadCamera.orientation = [this] { return m_state->camera.rotation.conjugated().normalized(); };
    triadCamera.applyOrientation = [this](const QQuaternion& orientation) {
        if (!m_state->triadHistoryTimer->isActive()) m_state->rememberCamera();
        m_state->camera.rotation = orientation.conjugated().normalized();
        m_state->triadHistoryTimer->start(150);
        m_state->hover = {};
        update();
    };
    triadCamera.isOrbiting = [this] { return m_state->button != Qt::NoButton; };
    triadCamera.isometric = [this] { viewIsometric(); };
    m_state->triad = std::make_unique<ViewportOrientationTriad>(triadCamera, this, [this] {
        return !isEnabled() || m_state->button != Qt::NoButton || m_state->boxSelecting ?
            TriadNavigationAvailability::Busy : TriadNavigationAvailability::Available;
    });
}

OccViewport::~OccViewport()
{
    // The HUD's event filter and sibling angle editor reference this camera.
    // Retire them while the viewport and state still exist.
    m_state->triad.reset();
    cleanupGL();
}

void OccViewport::setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                           const std::set<int>& hidden)
{
    auto& s = *m_state;
    std::vector<Part> parts;
    std::map<int, PickGeometry> pickGeometry;
    std::set<int> partIds;
    const auto& source = document.parts();
    parts.reserve(source.size());
    for (const auto& part : source) {
        partIds.insert(part.id);
        if (hidden.find(part.id) != hidden.end()) continue;
        Part copy{part.id, {}, {}, {}};
        const auto& cad = part.display;
        copy.cad = snapshot(cad.vertices.size(), cad.triangles, cad.edges, false,
                            [&](Index i) { return vec(cad.vertices[i]); });
        // CAD-only normals are calculated on each OCC face, preserving sharp
        // seams. FEM keeps its element-flat normals and unchanged topology.
        for (std::size_t i = 0; i < copy.cad.vertices.size(); ++i) {
            const Index sourceIndex = copy.cad.sourceIndices[i];
            if (sourceIndex >= cad.normals.size()) continue;
            Vec normal = vec(cad.normals[sourceIndex]);
            if (!normalise(normal)) continue;
            for (int axis = 0; axis < 3; ++axis) copy.cad.vertices[i].normal[axis] = float(normal[axis]);
        }
        const auto found = meshes.find(part.id);
        if (found != meshes.end()) {
            const auto& fem = found->second;
            copy.mesh = snapshot(fem.nodes.size(), fem.surface, {}, true,
                                 [&](Index i) { return fem.nodes[i]; });
        }
        pickGeometry.emplace(part.id, MotionOcc::pickGeometry(part, found == meshes.end() ? nullptr : &found->second));
        Bounds rest = copy.cad.bounds;
        rest.merge(copy.mesh.bounds);
        const Vec origin = rest.center();
        copy.origin = QVector3D(float(origin[0]), float(origin[1]), float(origin[2]));
        parts.push_back(std::move(copy));
    }
    // An ordinal is not an identity: a reimport may reuse a part ID and even
    // the same edge/face indices for entirely different shapes. Keep feedback
    // only when its actual OCC shape (or FEM topology) is unchanged.
    std::set<int> unchangedCad,unchangedMesh;
    for(auto& [id,next]:pickGeometry) {
        const auto previous=s.pickGeometry.find(id);
        if(previous==s.pickGeometry.end() || !previous->second.shape.IsEqual(next.shape))continue;
        unchangedCad.insert(id);
        const auto& before=previous->second;
        if(before.nodes==next.nodes && before.meshEdges==next.meshEdges &&
           before.meshFaces==next.meshFaces && before.elements==next.elements) {
            // Actual model node IDs also survive a display-only scene rebuild.
            next.nodeIds=before.nodeIds;
            unchangedMesh.insert(id);
        }
    }
    const auto stale=[&](const PickResult& pick) {
        const auto next=pickGeometry.find(pick.partId);
        if(next==pickGeometry.end() || !validPickReference(pick,next->second))return true;
        if(!cadPick(pick.kind))return !unchangedMesh.count(pick.partId);
        if(!unchangedCad.count(pick.partId))return true;
        const auto* before=pickTopology(pick,s.pickGeometry.at(pick.partId));
        const auto* after=pickTopology(pick,next->second);
        return !before || !after || !before->IsEqual(*after);
    };
    s.picks.erase(std::remove_if(s.picks.begin(),s.picks.end(),stale),s.picks.end());
    const int previousSelectionSize = s.viewportSelection.size();
    auto& selection = s.viewportSelection;
    selection.erase(std::remove_if(selection.begin(), selection.end(), stale), selection.end());
    for(auto layer=s.auxiliaryPicks.begin();layer!=s.auxiliaryPicks.end();) {
        auto& picks=layer->second.picks;
        picks.erase(std::remove_if(picks.begin(),picks.end(),stale),picks.end());
        if(picks.isEmpty())layer=s.auxiliaryPicks.erase(layer);else ++layer;
    }
    s.parts = std::move(parts);
    s.pickGeometry = std::move(pickGeometry);
    s.hover = {};
    s.scalarParts.clear();
    s.restMeshes.clear();
    s.dirtyMeshParts.clear();
    s.partIds = std::move(partIds);
    for (auto it = s.poses.begin(); it != s.poses.end();) {
        if (s.partIds.count(it->first)) ++it;
        else it = s.poses.erase(it);
    }
    s.updateBounds();
    s.sceneDirty = true;
    update();
    if (selection.size() != previousSelectionSize) emit viewportSelectionChanged();
}

void OccViewport::setPartPoses(const std::map<int, QMatrix4x4>& poses)
{
    auto& s = *m_state;
    s.hover = {}; // preselection is recomputed at the next pointer position
    std::map<int, PartPose> next;
    for (const auto& [id, matrix] : poses) {
        if (!s.partIds.count(id)) continue;
        PartPose pose{};
        if (makePartPose(matrix, pose)) next.emplace_hint(next.end(), id, std::move(pose));
    }
    if (next.empty()) {
        clearPartPoses();
        return;
    }
    s.poses = std::move(next);
    s.updateBounds();
    update();
}

void OccViewport::clearPartPoses()
{
    auto& s = *m_state;
    clearMeshScalars();
    setDeformedMeshes({}, {});
    if (s.poses.empty()) return;
    s.poses.clear();
    s.updateBounds();
    update();
}

void OccViewport::setDeformedMeshes(const std::map<int, FemMesh>& meshes, const std::set<int>& suppressed)
{
    auto& s = *m_state;
    if (s.restMeshes.empty() && meshes.empty()
        && std::none_of(s.parts.begin(), s.parts.end(), [](const auto& part) { return part.suppressed; })) return;
    for (auto& part : s.parts) {
        const auto rest = s.restMeshes.find(part.id);
        if (rest != s.restMeshes.end()) {
            part.mesh = std::move(rest->second);
            s.restMeshes.erase(rest);
            s.dirtyMeshParts.insert(part.id);
        }
        part.deformed = false;
        part.suppressed = suppressed.count(part.id) != 0;
        const auto found = meshes.find(part.id);
        if (found == meshes.end()) continue;
        const auto& mesh = found->second;
        s.restMeshes.emplace(part.id, std::move(part.mesh));
        part.mesh = snapshot(mesh.nodes.size(), mesh.surface, {}, true, [&](Index i) { return mesh.nodes[i]; });
        part.deformed = true;
        s.poses.erase(part.id); // Deformed node positions are already in world coordinates.
        s.dirtyMeshParts.insert(part.id);
    }
    s.updateBounds();
    update();
}

void OccViewport::selectPart(int id)
{
    if (m_state->selected == id) return;
    m_state->selected = id;
    update();
}

void OccViewport::setDisplayMode(DisplayMode mode)
{
    if (m_state->mode == mode) return;
    m_state->mode = mode;
    update();
}

OccViewport::DisplayMode OccViewport::displayMode() const { return m_state->mode; }

void OccViewport::setMouseNavigationMode(MouseNavigationMode mode)
{
    if (mode != MouseNavigationMode::Classic && mode != MouseNavigationMode::Nx) return;
    if (m_state->navigationMode == mode) return;
    interruptPointerOperation();
    m_state->navigationMode = mode;
    setToolTip(mouseNavigationHint());
    update();
}

OccViewport::MouseNavigationMode OccViewport::mouseNavigationMode() const { return m_state->navigationMode; }

QString OccViewport::mouseNavigationHint() const
{
    if (m_state->navigationMode == MouseNavigationMode::Nx)
        return tr("NX 导航：中键拖动旋转；Shift+中键平移；Ctrl+中键拖动或滚轮按光标缩放；左键点选。"
                  "双击表面设旋转中心，双击空白或 Ctrl+双击适合全部。拾取中右键撤回，Esc 取消；多选节点 Ctrl+左拖贯穿框选。");
    return tr("经典导航：左键拖动旋转；中键或右键拖动平移；滚轮按光标缩放；左键点选。"
              "拾取中右键撤回，Esc 取消；多选节点 Ctrl+左拖贯穿框选。");
}

void OccViewport::setPickEnabled(bool enabled)
{
    m_state->pickEnabled = enabled;
    if (!enabled) interruptPointerOperation();
}

void OccViewport::setSelectionFilter(std::optional<PickKind> kind)
{
    if (m_state->selectionFilter == kind) return;
    m_state->selectionFilter = kind;
    m_state->hover = {};
    interruptPointerOperation();
    clearViewportSelection();
    update();
}

const QVector<PickResult>& OccViewport::viewportSelection() const { return m_state->viewportSelection; }

void OccViewport::clearViewportSelection()
{
    if (m_state->viewportSelection.isEmpty()) return;
    m_state->viewportSelection.clear();
    emit viewportSelectionChanged();
    update();
}

void OccViewport::beginPick(PickKind kind, bool multiple, const std::set<int>& allowedParts,
                            const QVector<PickResult>& initial)
{
    auto& s = *m_state;
    s.pickActive = true; s.pickKind = kind; s.pickMultiple = multiple; s.pickParts = allowedParts;
    interruptPointerOperation();
    s.hover = {};
    setPickSelection(initial);
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::CrossCursor);
    update();
    emit pickSessionChanged(true);
}

void OccViewport::endPick()
{
    auto& s = *m_state;
    const bool wasPicking = s.pickActive;
    s.pickActive = false; s.picks.clear(); s.hover = {}; s.pickParts.clear();
    s.pickFeedbackVisible = true;
    interruptPointerOperation();
    unsetCursor();
    update();
    if (wasPicking) emit pickSessionChanged(false);
}

bool OccViewport::isPicking() const { return m_state->pickActive; }
QVector<PickResult> OccViewport::pickSelection() const { return m_state->picks; }
void OccViewport::setPickHighlightColor(const QColor& color)
{
    m_state->pickColor=color.isValid()?color:QColor(232,130,28); update();
}
void OccViewport::setPickFeedbackVisible(bool visible)
{
    if(m_state->pickFeedbackVisible==visible)return;
    m_state->pickFeedbackVisible=visible;update();
}

void OccViewport::setAuxiliaryPickHighlights(const QString& key, const QVector<PickResult>& picks, const QColor& color)
{
    QVector<PickResult> valid;
    for(const auto& pick:picks) {
        const auto geometry=m_state->pickGeometry.find(pick.partId);
        if(geometry!=m_state->pickGeometry.end() && validPickReference(pick,geometry->second))valid.append(pick);
    }
    if (valid.isEmpty()) m_state->auxiliaryPicks.erase(key);
    else m_state->auxiliaryPicks[key] = {valid, color.isValid() ? color : QColor(232,130,28)};
    update();
}
void OccViewport::clearAuxiliaryPickHighlights(const QString& key)
{
    m_state->auxiliaryPicks.erase(key); update();
}
void OccViewport::setAuxiliaryPoints(const QString& key, const QVector<QVector3D>& points, const QColor& color,float pointSize)
{
    if(!std::isfinite(pointSize)||pointSize<=0) {clearAuxiliaryPoints(key);return;}
    QVector<QVector3D> valid;
    for (const auto& point : points)
        if (std::isfinite(point.x()) && std::isfinite(point.y()) && std::isfinite(point.z())) valid.append(point);
    if (valid.isEmpty()) m_state->auxiliaryPoints.erase(key);
    else m_state->auxiliaryPoints[key] = {valid, color.isValid() ? color : QColor(24,144,255),std::clamp(pointSize,1.f,256.f)};
    update();
}
void OccViewport::clearAuxiliaryPoints(const QString& key)
{
    m_state->auxiliaryPoints.erase(key); update();
}

void OccViewport::setPickSelection(const QVector<PickResult>& selection)
{
    auto& s = *m_state;
    s.picks.clear();
    for (auto pick : selection) {
        if (!pick.isValid() || pick.kind != s.pickKind || !s.pickGeometry.count(pick.partId) ||
            (!s.pickParts.empty() && !s.pickParts.count(pick.partId))) continue;
        const auto& geometry = s.pickGeometry.at(pick.partId);
        if(!validPickReference(pick,geometry))continue;
        try {
            if (pick.kind == PickKind::Node) {
                pick.position = pick.hitPosition = geometry.nodes[pick.index];
                pick.nodeId = geometry.nodeIds.value(pick.index,pick.index+1);
            } else if (pick.kind == PickKind::Vertex) {
                pick.position = pick.hitPosition = vec(BRep_Tool::Pnt(TopoDS::Vertex(geometry.vertices(pick.index))));
            } else if (pick.kind == PickKind::Edge) {
                BRepAdaptor_Curve curve(TopoDS::Edge(geometry.edges(pick.index)));
                pick.position = vec(curve.Value((curve.FirstParameter()+curve.LastParameter())*.5));
                if (curve.GetType()==GeomAbs_Circle) pick.position=vec(curve.Circle().Location());
                else if (curve.GetType()==GeomAbs_Ellipse) pick.position=vec(curve.Ellipse().Location());
            } else if (pick.kind == PickKind::Face || pick.kind == PickKind::Part || pick.kind == PickKind::Solid) {
                GProp_GProps props;
                if (pick.kind == PickKind::Face) BRepGProp::SurfaceProperties(geometry.faces(pick.index),props);
                else if (pick.kind == PickKind::Solid) BRepGProp::VolumeProperties(geometry.solids(pick.index),props);
                else {
                    BRepGProp::VolumeProperties(geometry.shape,props);
                    if (std::abs(props.Mass())<1e-14) BRepGProp::SurfaceProperties(geometry.shape,props);
                    if (std::abs(props.Mass())<1e-14) BRepGProp::LinearProperties(geometry.shape,props);
                }
                if (std::abs(props.Mass())>1e-14) pick.position=vec(props.CentreOfMass());
            } else {
                std::vector<Index> nodes;
                if (pick.kind==PickKind::MeshEdge) nodes.assign(geometry.meshEdges[pick.index-1].begin(),geometry.meshEdges[pick.index-1].end());
                else if (pick.kind==PickKind::Element && !geometry.elements.empty()) nodes.assign(geometry.elements[pick.index-1].begin(),geometry.elements[pick.index-1].end());
                else nodes.assign(geometry.meshFaces[pick.index-1].begin(),geometry.meshFaces[pick.index-1].end());
                if (std::any_of(nodes.begin(),nodes.end(),[&](Index n) { return n>=geometry.nodes.size(); })) continue;
                QVector<int> ids; pick.position={};
                for (auto n:nodes) {
                    ids.append(geometry.nodeIds.value(n,int(n+1)));
                    for (int k=0;k<3;++k) pick.position[k]+=geometry.nodes[n][k]/nodes.size();
                }
                if (!pick.nodeIds.isEmpty() && pick.nodeIds!=ids) continue;
                pick.nodeIds=ids;
            }
        } catch (const Standard_Failure&) { continue; }
        if (std::any_of(s.picks.cbegin(), s.picks.cend(), [&](const auto& p) { return samePick(p, pick); })) continue;
        s.picks.append(pick);
        if (!s.pickMultiple) break;
    }
    update();
}

void OccViewport::setPickNodes(int partId, const FemMesh& mesh, const QVector<int>& nodeIds)
{
    const auto found = m_state->pickGeometry.find(partId);
    if (found == m_state->pickGeometry.end()) return;
    auto& target = found->second;
    if (!nodeIds.isEmpty()) {
        if (nodeIds.size() != int(mesh.nodes.size())) return;
        std::set<int> unique;
        for (const int id : nodeIds) if (id <= 0 || !unique.insert(id).second) return;
    }
    setMeshTopology(target, mesh);
    target.nodeIds = nodeIds;
    if (target.nodeIds.size() != int(target.nodes.size())) {
        target.nodeIds.clear(); target.nodeIds.reserve(int(target.nodes.size()));
        for (int i = 0; i < int(target.nodes.size()); ++i) target.nodeIds.append(i + 1);
    }
    update();
}


void OccViewport::setMeshScalars(const std::map<int, std::vector<double>>& values,
                                 double minimum, double maximum, const QString& title)
{
    auto& s = *m_state;
    s.scalarParts.clear();
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || maximum < minimum) { update(); return; }
    s.scalarMinimum = minimum; s.scalarMaximum = maximum; s.scalarTitle = title;
    const double span = maximum - minimum;
    for (auto& part : s.parts) {
        const auto found = values.find(part.id);
        if (found == values.end() || found->second.empty()) continue;
        const auto& input = found->second;
        if (std::any_of(input.cbegin(), input.cend(), [](double v) { return !std::isfinite(v); })) continue;
        auto& geometry = part.mesh;
        if (geometry.sourceIndices.size() != geometry.vertices.size() || geometry.vertices.empty() ||
            *std::max_element(geometry.sourceIndices.begin(), geometry.sourceIndices.end()) >= input.size()) continue;
        for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
            const double value = input[geometry.sourceIndices[i]];
            geometry.vertices[i].scalar = float(span > 0 ? std::clamp((value - minimum) / span, 0.0, 1.0) : 0.0);
        }
        s.scalarParts.insert(part.id); s.dirtyMeshParts.insert(part.id);
    }
    update();
}

void OccViewport::clearMeshScalars()
{
    m_state->scalarParts.clear(); m_state->scalarTitle.clear(); update();
}

void OccViewport::setMotionSymbols(const QVector<MotionSymbol>& symbols)
{
    m_state->symbols = symbols;
    update();
}

void OccViewport::setAuxiliaryMotionSymbols(const QString& key,const QVector<MotionSymbol>& symbols)
{
    if(symbols.isEmpty())m_state->auxiliarySymbols.erase(key);
    else m_state->auxiliarySymbols[key]=symbols;
    update();
}
void OccViewport::clearAuxiliaryMotionSymbols(const QString& key)
{
    m_state->auxiliarySymbols.erase(key);update();
}

QPointF OccViewport::projectPoint(const Vec& position, int partId) const
{
    const auto& s = *m_state;
    const auto frame = s.camera.frame(size(), s.bounds);
    const auto* pose = s.poseFor(partId);
    const Vec point = pose ? pose->relativePoint(position, vec(s.camera.pivot)) : subtract(position, vec(s.camera.pivot));
    const double unit = s.camera.mmPerPixel(size());
    return {width() * 0.5 + (dot(frame.rows[0], point) + s.camera.pan.x()) / unit,
            height() * 0.5 - (dot(frame.rows[1], point) + s.camera.pan.y()) / unit};
}

PickResult OccViewport::pickAt(const QPointF& position) const
{
    const auto& s = *m_state;
    if (s.pickActive) return pickAt(position, s.pickKind, s.pickParts);
    if (s.selectionFilter) return pickAt(position, *s.selectionFilter, {});
    for (const auto kind : {PickKind::Vertex, PickKind::Edge, PickKind::Face}) {
        const auto hit = pickAt(position, kind, {});
        if (hit.isValid()) return hit;
    }
    return {};
}

PickResult OccViewport::pickAt(const QPointF& position, PickKind kind, const std::set<int>& allowedParts) const
{
    const auto& s = *m_state;
    PickResult best;
    if (!s.pickEnabled || !s.bounds.valid || !QRectF(rect()).contains(position)) return best;
    const auto frame = s.camera.frame(size(), s.bounds);
    const double unit = s.camera.mmPerPixel(size());
    const QPointF eye((position.x() - width() * 0.5) * unit - s.camera.pan.x(),
                      (height() * 0.5 - position.y()) * unit - s.camera.pan.y());
    const Vec pivot = vec(s.camera.pivot);
    Ray world;
    for (int i = 0; i < 3; ++i) {
        world.origin[i] = pivot[i] + frame.rows[0][i] * eye.x() + frame.rows[1][i] * eye.y() + frame.rows[2][i] * frame.nearZ;
        world.direction[i] = -frame.rows[2][i];
    }
    double visibleDepth = frame.nearZ - frame.farZ;
    if (s.mode != DisplayMode::Wireframe) {
        for (const auto& part : s.parts) {
            if (part.suppressed) continue;
            const auto& geometry = part.usesMesh(s.mode) ? part.mesh : part.cad;
            const auto ray = restRay(world, s.poseFor(part.id));
            if (!intersectsBounds(ray, geometry.bounds, visibleDepth, unit)) continue;
            for (std::size_t i = 0; i + 2 < geometry.triangles.size(); i += 3) {
                double distance;
                if (intersectsTriangle(ray, geometry.position(geometry.triangles[i]),
                    geometry.position(geometry.triangles[i+1]), geometry.position(geometry.triangles[i+2]), distance))
                    visibleDepth = std::min(visibleDepth, distance);
            }
        }
    }
    double bestOffset = 8.0 * 8.0, bestDepth = frame.nearZ - frame.farZ;
    const auto depthAt = [&](const Vec& point, int partId) {
        const auto* pose = s.poseFor(partId);
        const auto relative = pose ? pose->relativePoint(point, pivot) : subtract(point, pivot);
        return frame.nearZ - dot(frame.rows[2], relative);
    };
    const auto pointHit = [&](const Vec& point, PickResult candidate) {
        const auto offset = position - projectPoint(point, candidate.partId);
        const double offset2 = QPointF::dotProduct(offset, offset), depth = depthAt(point, candidate.partId);
        if (depth < 0 || depth > visibleDepth + 8 * unit || offset2 > bestOffset ||
            (std::abs(offset2 - bestOffset) < 1e-8 && depth >= bestDepth)) return;
        candidate.position = candidate.hitPosition = point;
        best = candidate; bestOffset = offset2; bestDepth = depth;
    };
    for (const auto& [partId, geometry] : s.pickGeometry) {
        if (!allowedParts.empty() && !allowedParts.count(partId)) continue;
        const auto part = std::find_if(s.parts.cbegin(), s.parts.cend(), [partId](const auto& p) { return p.id == partId; });
        if (part == s.parts.cend() || part->suppressed || part->deformed) continue;
        if (kind < PickKind::Node && !intersectsBounds(restRay(world, s.poseFor(partId)),
                part->cad.bounds, visibleDepth + 8 * unit, 8 * unit)) continue;
        PickResult candidate; candidate.kind = kind; candidate.partId = partId;
        try {
            if (kind == PickKind::Vertex) {
                for (int index = 1; index <= geometry.vertices.Extent(); ++index) {
                    candidate.index = index;
                    pointHit(vec(BRep_Tool::Pnt(TopoDS::Vertex(geometry.vertices(index)))), candidate);
                }
            } else if (kind == PickKind::Node) {
                for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                    candidate.index = index; candidate.nodeId = geometry.nodeIds.value(index, index + 1);
                    pointHit(geometry.nodes[index], candidate);
                }
            } else if (kind == PickKind::MeshEdge) {
                for (std::size_t index = 0; index < geometry.meshEdges.size(); ++index) {
                    const auto& edge = geometry.meshEdges[index];
                    const auto& pa = geometry.nodes[edge[0]]; const auto& pb = geometry.nodes[edge[1]];
                    const auto a = projectPoint(pa,partId), b = projectPoint(pb,partId), delta = b-a;
                    const double length2 = QPointF::dotProduct(delta,delta);
                    if (length2 < 1e-20) continue;
                    const double t = std::clamp(QPointF::dotProduct(position-a,delta)/length2,0.0,1.0);
                    const auto offset = position-a-delta*t; const double offset2 = QPointF::dotProduct(offset,offset);
                    Vec point; for (int k = 0; k < 3; ++k) point[k] = pa[k]*(1-t)+pb[k]*t;
                    const double depth = depthAt(point,partId);
                    if (offset2 > bestOffset || depth < 0 || depth > visibleDepth+8*unit ||
                        (std::abs(offset2-bestOffset) < 1e-8 && depth >= bestDepth)) continue;
                    candidate.index = int(index+1); candidate.hitPosition = point;
                    for (int k = 0; k < 3; ++k) candidate.position[k] = (pa[k]+pb[k])*0.5;
                    candidate.direction = subtract(pb,pa); candidate.hasDirection = normalise(candidate.direction);
                    candidate.nodeIds = {geometry.nodeIds.value(edge[0],int(edge[0]+1)),geometry.nodeIds.value(edge[1],int(edge[1]+1))};
                    best = candidate; bestOffset = offset2; bestDepth = depth;
                }
            } else if (kind == PickKind::MeshFace || kind == PickKind::Element) {
                const auto ray = restRay(world,s.poseFor(partId));
                const bool volume = kind == PickKind::Element && !geometry.elements.empty();
                const std::size_t count = volume ? geometry.elements.size() : geometry.meshFaces.size();
                constexpr int tetFaces[4][3] = {{0,2,1},{0,1,3},{0,3,2},{1,2,3}};
                for (std::size_t index = 0; index < count; ++index) {
                    std::vector<Index> nodes;
                    if (volume) nodes.assign(geometry.elements[index].begin(),geometry.elements[index].end());
                    else nodes.assign(geometry.meshFaces[index].begin(),geometry.meshFaces[index].end());
                    if (std::any_of(nodes.begin(),nodes.end(),[&](Index n) { return n >= geometry.nodes.size(); })) continue;
                    Bounds bounds; for (auto n : nodes) bounds.add(geometry.nodes[n]);
                    if (!intersectsBounds(ray,bounds,std::min(bestDepth,visibleDepth+8*unit),unit*.01)) continue;
                    double depth = bestDepth; bool found = false; Vec normal{};
                    for (int face = 0; face < (volume ? 4 : 1); ++face) {
                        const auto& a = geometry.nodes[nodes[volume ? tetFaces[face][0] : 0]];
                        const auto& b = geometry.nodes[nodes[volume ? tetFaces[face][1] : 1]];
                        const auto& c = geometry.nodes[nodes[volume ? tetFaces[face][2] : 2]];
                        double hit;
                        if (intersectsTriangle(ray,a,b,c,hit) && hit < depth && hit <= visibleDepth+8*unit) {
                            depth = hit; found = true; normal = cross(subtract(b,a),subtract(c,a));
                        }
                    }
                    if (!found) continue;
                    candidate.index = int(index+1); candidate.position = {}; candidate.nodeIds.clear();
                    for (auto node : nodes) {
                        for (int k = 0; k < 3; ++k) candidate.position[k] += geometry.nodes[node][k]/nodes.size();
                        candidate.nodeIds.append(geometry.nodeIds.value(node,int(node+1)));
                    }
                    for (int k = 0; k < 3; ++k) candidate.hitPosition[k] = ray.origin[k]+ray.direction[k]*depth;
                    candidate.direction = normal;
                    candidate.hasDirection = kind == PickKind::MeshFace && normalise(candidate.direction);
                    best = candidate; bestDepth = depth;
                }
            } else if (kind == PickKind::Edge) {
                for (std::size_t index = 0; index < geometry.curves.size(); ++index) {
                    const auto& curve = geometry.curves[index];
                    for (std::size_t j = 1; j < curve.points.size(); ++j) {
                        const QPointF a = projectPoint(curve.points[j-1], partId), b = projectPoint(curve.points[j], partId);
                        const auto delta = b - a; const double length2 = QPointF::dotProduct(delta, delta);
                        if (length2 < 1e-20) continue;
                        const double t = std::clamp(QPointF::dotProduct(position - a, delta) / length2, 0.0, 1.0);
                        const auto offset = position - a - delta * t;
                        const double offset2 = QPointF::dotProduct(offset, offset);
                        if (offset2 > bestOffset) continue;
                        const double parameter = curve.parameters[j-1] * (1-t) + curve.parameters[j] * t;
                        BRepAdaptor_Curve exact(curve.edge);
                        gp_Pnt hit; gp_Vec tangent; exact.D1(parameter, hit, tangent);
                        const double depth = depthAt(vec(hit), partId);
                        if (depth < 0 || depth > visibleDepth + 8 * unit ||
                            (std::abs(offset2 - bestOffset) < 1e-8 && depth >= bestDepth)) continue;
                        candidate.index = int(index + 1); candidate.hitPosition = vec(hit);
                        candidate.position = vec(exact.Value((exact.FirstParameter() + exact.LastParameter()) * .5));
                        candidate.direction = {tangent.X(), tangent.Y(), tangent.Z()};
                        if (exact.GetType() == GeomAbs_Circle) {
                            candidate.position = vec(exact.Circle().Location());
                            const auto axis = exact.Circle().Axis().Direction();
                            candidate.direction = {axis.X(), axis.Y(), axis.Z()};
                        } else if (exact.GetType() == GeomAbs_Ellipse) {
                            candidate.position = vec(exact.Ellipse().Location());
                            const auto axis = exact.Ellipse().Axis().Direction();
                            candidate.direction = {axis.X(), axis.Y(), axis.Z()};
                        }
                        candidate.hasDirection = normalise(candidate.direction);
                        best = candidate; bestOffset = offset2; bestDepth = depth;
                    }
                }
            } else {
                const auto ray = restRay(world, s.poseFor(partId));
                IntCurvesFace_ShapeIntersector intersector;
                intersector.Load(geometry.shape, 1e-7);
                intersector.Perform(gp_Lin(gp_Pnt(ray.origin[0], ray.origin[1], ray.origin[2]),
                    gp_Dir(ray.direction[0], ray.direction[1], ray.direction[2])), 0, frame.nearZ - frame.farZ);
                if (!intersector.IsDone()) continue;
                for (int hit = 1; hit <= intersector.NbPnt(); ++hit) {
                    const double depth = intersector.WParameter(hit);
                    if (depth < 0 || depth >= bestDepth || depth > visibleDepth + 8 * unit) continue;
                    const auto face = intersector.Face(hit);
                    candidate.index = kind == PickKind::Face ? geometry.faces.FindIndex(face) : -1;
                    if (kind == PickKind::Face && candidate.index < 1) continue;
                    if (kind == PickKind::Solid) {
                        const int faceId=geometry.faces.FindIndex(face);
                        candidate.index=faceId>0 ? geometry.faceSolids[std::size_t(faceId)] : -1;
                        if (candidate.index<1) continue; // shared/ambiguous solid boundary
                    }
                    candidate.hitPosition = vec(intersector.Pnt(hit));
                    GProp_GProps props;
                    if (kind == PickKind::Part) {
                        BRepGProp::VolumeProperties(geometry.shape, props);
                        if (std::abs(props.Mass()) < 1e-14) BRepGProp::SurfaceProperties(geometry.shape, props);
                    } else if (kind==PickKind::Solid) BRepGProp::VolumeProperties(geometry.solids(candidate.index),props);
                    else BRepGProp::SurfaceProperties(face, props);
                    candidate.position = std::abs(props.Mass()) > 1e-14 ? vec(props.CentreOfMass()) : candidate.hitPosition;
                    if (kind == PickKind::Face) {
                        BRepAdaptor_Surface surface(face);
                        gp_Pnt point; gp_Vec du, dv;
                        surface.D1(intersector.UParameter(hit), intersector.VParameter(hit), point, du, dv);
                        gp_Vec normal = du.Crossed(dv);
                        if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();
                        candidate.direction = {normal.X(), normal.Y(), normal.Z()};
                        const auto useAxis = [&](const gp_Dir& direction) { candidate.direction = {direction.X(), direction.Y(), direction.Z()}; };
                        switch (surface.GetType()) {
                        case GeomAbs_Cylinder: useAxis(surface.Cylinder().Axis().Direction()); break;
                        case GeomAbs_Cone: useAxis(surface.Cone().Axis().Direction()); break;
                        case GeomAbs_Torus: useAxis(surface.Torus().Axis().Direction()); break;
                        case GeomAbs_Sphere: {
                            const auto radial = gp_Vec(surface.Sphere().Location(), intersector.Pnt(hit));
                            candidate.direction = {radial.X(), radial.Y(), radial.Z()};
                            if (face.Orientation() == TopAbs_REVERSED) for (auto& value : candidate.direction) value = -value;
                            break;
                        }
                        default: break;
                        }
                        candidate.hasDirection = normalise(candidate.direction);
                    }
                    best = candidate; bestDepth = depth;
                }
            }
        } catch (const Standard_Failure&) { /* Ignore an unevaluable entity, never invent a hit. */ }
    }
    if (kind == PickKind::Part && !best.isValid()) {
        // Wire/edge-only imported construction geometry has no face intersector.
        const int partId = pickPart(position);
        if (partId > 0 && (allowedParts.empty() || allowedParts.count(partId))) {
            const auto found = s.pickGeometry.find(partId);
            if (found != s.pickGeometry.end()) {
                GProp_GProps props; BRepGProp::LinearProperties(found->second.shape, props);
                best.kind = PickKind::Part; best.partId = partId;
                best.position = best.hitPosition = vec(props.CentreOfMass());
            }
        }
    }
    return best;
}

void OccViewport::fitAll()
{
    if (!m_state->bounds.valid) return;
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.fit(m_state->bounds, size(), isVisible());
    m_state->rememberCamera();
    update();
}

void OccViewport::viewIsometric()
{
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.rotation = isometricRotation();
    m_state->rememberCamera();
    update();
}

void OccViewport::setViewOrientation(const QVector3D& direction, const QVector3D& up)
{
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.rotation = viewRotation(direction, up);
    m_state->rememberCamera();
    update();
}

void OccViewport::viewFront() { setViewOrientation({0, -1, 0}, {0, 0, 1}); }
void OccViewport::viewBack() { setViewOrientation({0, 1, 0}, {0, 0, 1}); }
void OccViewport::viewTop() { setViewOrientation({0, 0, 1}, {0, 1, 0}); }
void OccViewport::viewBottom() { setViewOrientation({0, 0, -1}, {0, 1, 0}); }
void OccViewport::viewRight() { setViewOrientation({1, 0, 0}, {0, 0, 1}); }
void OccViewport::viewLeft() { setViewOrientation({-1, 0, 0}, {0, 0, 1}); }

namespace {
QVector3D screenRotationAxis(const QString& axis)
{
    if (axis.endsWith(QLatin1String("Sx"), Qt::CaseInsensitive)) return {1, 0, 0};
    if (axis.endsWith(QLatin1String("Sy"), Qt::CaseInsensitive)) return {0, 1, 0};
    if (axis.endsWith(QLatin1String("Sz"), Qt::CaseInsensitive)) return {0, 0, 1};
    return {};
}
}

void OccViewport::rotateView(const QString& screenAxis, double degrees)
{
    const auto axis = screenRotationAxis(screenAxis);
    if (axis.isNull() || !std::isfinite(degrees)) return;
    auto& state = *m_state;
    state.rememberCamera();
    const auto delta = QQuaternion::fromAxisAndAngle(axis, float(std::remainder(degrees, 360.0)));
    state.camera.rotation = (delta * state.camera.rotation).normalized();
    state.rememberCamera();
    state.hover = {};
    update();
}

void OccViewport::panView(const QString& direction, double percent)
{
    if (!std::isfinite(percent)) return;
    QPointF offset;
    if (direction == QLatin1String("up")) offset.setY(height());
    else if (direction == QLatin1String("down")) offset.setY(-height());
    else if (direction == QLatin1String("left")) offset.setX(-width());
    else if (direction == QLatin1String("right")) offset.setX(width());
    else return;
    auto& state = *m_state;
    state.rememberCamera();
    // Match the original mDisplayMenuFunction::slot_directionPan input:
    // a value of 100 moves by half the viewport dimension (value / 200).
    state.camera.pan += offset * (state.camera.mmPerPixel(size()) * percent / 200.0);
    state.rememberCamera();
    state.hover = {};
    update();
}

void OccViewport::zoomView(bool zoomIn, double factor)
{
    if (!std::isfinite(factor) || factor <= 0.0) return;
    auto& state = *m_state;
    state.rememberCamera();
    state.camera.zoomAt((zoomIn ? -1.0 : 1.0) * std::log(factor), rect().center(), size());
    state.rememberCamera();
    state.hover = {};
    update();
}

void OccViewport::stepViewHistory(bool forward)
{
    auto& state = *m_state;
    state.autoRotationTimer->stop();
    state.triadHistoryTimer->stop();
    if (state.triad) state.triad->CancelNavigation();
    interruptPointerOperation();
    state.rememberCamera();
    const int next = state.cameraHistoryPosition + (forward ? 1 : -1);
    if (next < 0 || next >= int(state.cameraHistory.size())) return;
    state.cameraHistoryPosition = next;
    state.camera = state.cameraHistory[std::size_t(next)];
    update();
}

void OccViewport::setAutoRotation(const QString& screenAxis, bool enabled)
{
    auto& state = *m_state;
    if (!enabled) {
        state.autoRotationTimer->stop();
        state.rememberCamera();
        return;
    }
    const auto axis = screenRotationAxis(screenAxis);
    if (axis.isNull()) return;
    state.rememberCamera();
    state.autoRotationAxis = axis;
    state.autoRotationSign = screenAxis.contains(QLatin1Char('-')) ? -1.0 : 1.0;
    state.autoRotationClock.start();
    state.autoRotationTimer->start();
}

bool OccViewport::queryVisibleBounds(QVector3D& minimum,QVector3D& maximum) const
{
    const auto& bounds=m_state->bounds;
    if(!bounds.valid)return false;
    minimum=QVector3D(bounds.min[0],bounds.min[1],bounds.min[2]);
    maximum=QVector3D(bounds.max[0],bounds.max[1],bounds.max[2]);
    return true;
}

OccViewport::ViewCameraState OccViewport::queryCameraState() const
{
    const auto& state = *m_state;
    const auto frame = state.camera.frame(size(), state.bounds);
    const auto eyeToWorld = state.camera.rotation.conjugated().normalized();
    ViewCameraState result;
    result.target = state.camera.pivot + eyeToWorld.rotatedVector(
        QVector3D(float(-state.camera.pan.x()), float(-state.camera.pan.y()), 0));
    const double eyeDistance = std::max(2.0 * state.camera.span,
        std::abs(frame.nearZ) + 2.0 * state.camera.span);
    result.eye = result.target + eyeToWorld.rotatedVector(QVector3D(0, 0, float(eyeDistance)));
    result.up = eyeToWorld.rotatedVector(QVector3D(0, 1, 0));
    result.ortho_zoom = state.camera.mmPerPixel(size()) * std::max(1, height());
    return result;
}

bool OccViewport::glReady() const { return m_state->ready; }

void OccViewport::reportGraphicsError(const QString& message)
{
    if (m_state->lastError == message) return;
    m_state->lastError = message;
    // Let the widget finish its Qt/GL callback before UI slots can act on it.
    QTimer::singleShot(0, this, [this, message] { emit graphicsError(message); });
}

void OccViewport::initializeGL()
{
    auto& s = *m_state;
    auto* ctx = context();
    const auto actual = ctx->format();
    if (ctx->isOpenGLES() || actual.profile() != QSurfaceFormat::CoreProfile ||
        actual.version() < qMakePair(3, 3) ||
        !(s.gl = ctx->versionFunctions<QOpenGLFunctions_3_3_Core>()) || !s.gl->initializeOpenGLFunctions()) {
        s.gl = nullptr;
        reportGraphicsError(tr("The CAD viewport requires desktop OpenGL 3.3 Core (received %1.%2%3).")
                            .arg(actual.majorVersion()).arg(actual.minorVersion())
                            .arg(ctx->isOpenGLES() ? QStringLiteral(" ES") : QString()));
        return;
    }
    s.destruction = connect(ctx, &QOpenGLContext::aboutToBeDestroyed,
                            this, &OccViewport::cleanupGL, Qt::DirectConnection);

    const auto init = [](std::unique_ptr<QOpenGLShaderProgram>& program, const char* vertex,
                         const char* fragment, const char* geometry = nullptr) {
        program = std::make_unique<QOpenGLShaderProgram>();
        return linkProgram(*program, vertex, fragment, geometry);
    };
    using namespace RenderShaders;
    QString failure;
    if (!init(s.backdrop, BackdropVertex, BackdropFragment)) {
        failure = tr("The viewport background shader failed: %1").arg(s.backdrop->log());
    } else if (!init(s.surface, SurfaceVertex, SurfaceFragment)) {
        failure = tr("The CAD surface shader failed: %1").arg(s.surface->log());
    } else if (!init(s.edge, EdgeVertex, EdgeFragment, EdgeGeometry)) {
        failure = tr("The CAD edge shader failed: %1").arg(s.edge->log());
    } else if (!init(s.point, PointVertex, PointFragment, PointGeometry)) {
        failure = tr("The CAD selection marker shader failed: %1").arg(s.point->log());
    } else {
        s.gl->glGenVertexArrays(1, &s.emptyVao);
        if (!s.emptyVao || s.gl->glGetError() != GL_NO_ERROR)
            failure = tr("Could not allocate the CAD viewport's OpenGL vertex array.");
    }
    if (!failure.isEmpty()) {
        cleanupGL();
        reportGraphicsError(failure);
        return;
    }
    s.ready = true;
    s.sceneDirty = true;
    s.lastError.clear();
}

void OccViewport::releaseSceneBuffers()
{
    for (const auto& part : m_state->buffers) {
        deleteBuffers(*m_state->gl, part.cad);
        deleteBuffers(*m_state->gl, part.mesh);
    }
    m_state->buffers.clear();
    for (const auto& entry : m_state->pickBuffers) deleteBuffers(*m_state->gl, entry.second);
    m_state->pickBuffers.clear();
}

void OccViewport::cleanupGL()
{
    auto& s = *m_state;
    QObject::disconnect(s.destruction);
    s.ready = false;
    s.sceneDirty = true;
    if (!s.gl) return;
    auto* ctx = context();
    const bool switchContext = ctx && QOpenGLContext::currentContext() != ctx;
    if (switchContext && ctx->isValid()) makeCurrent();
    const bool current = ctx && QOpenGLContext::currentContext() == ctx;
    if (current) {
        releaseSceneBuffers();
        if (s.emptyVao) s.gl->glDeleteVertexArrays(1, &s.emptyVao);
    } else {
        // No raw GL calls if the context was already lost. Qt's guarded shader
        // resources and the context/share group handle deferred destruction.
        s.buffers.clear();
        s.pickBuffers.clear();
    }
    s.emptyVao = 0;
    s.edge.reset();
    s.point.reset();
    s.surface.reset();
    s.backdrop.reset();
    s.gl = nullptr;
    if (switchContext && current) doneCurrent();
}

bool OccViewport::uploadScene()
{
    auto& s = *m_state;
    auto& gl = *s.gl;
    releaseSceneBuffers();
    s.dirtyMeshParts.clear();
    s.buffers.resize(s.parts.size());
    bool ok = true;
    for (std::size_t i = 0; i < s.parts.size() && ok; ++i) {
        ok = bufferGeometry(gl, s.parts[i].cad, s.buffers[i].cad) &&
             bufferGeometry(gl, s.parts[i].mesh, s.buffers[i].mesh);
    }
    gl.glBindVertexArray(0);
    gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
    const GLenum error = gl.glGetError();
    if (ok && error == GL_NO_ERROR) return true;
    releaseSceneBuffers();
    if (error == GL_NO_ERROR) {
        reportGraphicsError(tr("The CAD geometry exceeds the viewport's GPU buffer limits."));
    } else {
        reportGraphicsError(tr("Could not upload CAD geometry to OpenGL (error 0x%1).")
                            .arg(static_cast<uint>(error), 0, 16));
    }
    return false;
}

bool OccViewport::uploadChangedMeshes()
{
    auto& s = *m_state;
    if (s.buffers.size() != s.parts.size()) return uploadScene();
    bool ok = true;
    for (std::size_t i = 0; i < s.parts.size(); ++i) {
        if (!s.dirtyMeshParts.count(s.parts[i].id)) continue;
        deleteBuffers(*s.gl, s.buffers[i].mesh);
        s.buffers[i].mesh = {};
        ok = bufferGeometry(*s.gl, s.parts[i].mesh, s.buffers[i].mesh) && ok;
    }
    s.dirtyMeshParts.clear();
    s.gl->glBindVertexArray(0);
    s.gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (s.gl->glGetError() != GL_NO_ERROR) ok = false;
    if (!ok) reportGraphicsError(tr("无法更新柔性体动画网格。"));
    return ok;
}

void OccViewport::paintGL()
{
    auto& s = *m_state;
    if (!s.ready) {
        if (auto* ctx = QOpenGLContext::currentContext()) {
            auto* gl = ctx->functions();
            gl->glClearColor(backdropBottom.x(), backdropBottom.y(), backdropBottom.z(), 1.0f);
            gl->glClear(GL_COLOR_BUFFER_BIT);
        }
        return;
    }
    auto& gl = *s.gl;
    if (s.sceneDirty) {
        s.sceneDirty = false;
        s.uploadFailed = !uploadScene();
        if (!s.uploadFailed) s.lastError.clear();
    } else if (!s.dirtyMeshParts.empty()) {
        s.uploadFailed = !uploadChangedMeshes();
    }

    // Qt supplies the current widget FBO (not framebuffer 0) and a device-pixel viewport.
    GLint viewport[4]{};
    gl.glGetIntegerv(GL_VIEWPORT, viewport);
    const QVector2D pixels(float(std::max(1, viewport[2])), float(std::max(1, viewport[3])));
    const float dpr = float(devicePixelRatioF());
    const float depthBias = coplanarDepthBias(gl);
    const auto frame = s.camera.frame(size(), s.bounds);
    gl.glDisable(GL_SCISSOR_TEST);
    gl.glDisable(GL_CULL_FACE);
    gl.glDisable(GL_STENCIL_TEST);
    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
    gl.glEnable(GL_MULTISAMPLE);
    gl.glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    gl.glDepthMask(GL_TRUE);
    gl.glStencilMask(0xff);
    gl.glClearColor(backdropBottom.x(), backdropBottom.y(), backdropBottom.z(), 1.0f);
    gl.glClearDepth(1.0);
    gl.glClearStencil(0);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    s.backdrop->bind();
    s.backdrop->setUniformValue("uBottom", backdropBottom);
    s.backdrop->setUniformValue("uTop", backdropTop);
    gl.glBindVertexArray(s.emptyVao);
    gl.glDrawArrays(GL_TRIANGLES, 0, 3);
    s.backdrop->release();

    const auto setup = [&](QOpenGLShaderProgram& program) {
        program.bind();
        program.setUniformValue("uView", frame.view);
        program.setUniformValue("uProjection", frame.projection);
        program.setUniformValue("uPivot", s.camera.pivot);
        program.setUniformValue("uViewportPixels", pixels);
    };
    const Vec pivot = vec(s.camera.pivot);
    const auto setPose = [&](QOpenGLShaderProgram& program, const Part& part) {
        const auto* pose = s.poseFor(part.id);
        program.setUniformValue("uPosed", pose != nullptr);
        if (!pose) return;
        // Rebase around the part's rest origin and the camera pivot before
        // narrowing the translation, so the shader avoids subtracting large world positions.
        const Vec offset = pose->relativePoint(vec(part.origin), pivot);
        QMatrix4x4 toPivot = pose->restToWorld;
        toPivot.setColumn(3, QVector4D(float(offset[0]), float(offset[1]), float(offset[2]), 1.0f));
        program.setUniformValue("uPartOrigin", part.origin);
        program.setUniformValue("uPartToPivot", toPivot);
    };
    const bool wireframe = s.mode == DisplayMode::Wireframe;
    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthFunc(GL_LESS);
    if (!wireframe && !s.buffers.empty()) {
        // GL_STENCIL_BITS is not a valid core-profile glGet query. Test the
        // actual widget FBO attachment (CadMeshRenderer.cpp:903–910).
        GLint stencil = GL_NONE;
        if (s.mode == DisplayMode::ShadedEdges) {
            gl.glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, GL_STENCIL_ATTACHMENT,
                                                      GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &stencil);
        }
        const bool silhouette = stencil != GL_NONE;
        if (silhouette) {
            gl.glEnable(GL_STENCIL_TEST);
            gl.glStencilMask(1);
            gl.glStencilFunc(GL_ALWAYS, 1, 1);
            gl.glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
        }
        setup(*s.surface);
        s.surface->setUniformValue("uNormal", frame.normal);
        s.surface->setUniformValue("uOutline", false);
        s.surface->setUniformValue("uHighlight", false);
        gl.glEnable(GL_POLYGON_OFFSET_FILL);
        gl.glPolygonOffset(1.0f, 1.0f);
        for (std::size_t i = 0; i < s.buffers.size(); ++i) {
            const auto& part = s.parts[i];
            if (part.suppressed) continue;
            const bool meshed = part.usesMesh(s.mode);
            const auto& gpu = meshed ? s.buffers[i].mesh : s.buffers[i].cad;
            if (!gpu.triangleCount) continue;
            const bool hovered = !s.pickActive && s.selectionFilter == PickKind::Part &&
                                 s.pickEnabled && s.hover.partId == part.id;
            const auto color = part.id == s.selected ? rgb(63, 137, 216) :
                               hovered ? rgb(156, 115, 166) :
                               meshed ? rgb(138, 166, 189) : rgb(110, 129, 151);
            setPose(*s.surface, part);
            s.surface->setUniformValue("uFlat", meshed);
            s.surface->setUniformValue("uScalars", meshed && s.scalarParts.count(part.id) != 0);
            s.surface->setUniformValue("uColor", color);
            gl.glBindVertexArray(gpu.vao);
            gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.triangles);
            gl.glDrawElements(GL_TRIANGLES, gpu.triangleCount, GL_UNSIGNED_INT, nullptr);
        }
        gl.glDisable(GL_POLYGON_OFFSET_FILL);
        if (silhouette) {
            // A screen-space hull pass marks smooth silhouettes. The stencil
            // excludes every visible surface, so the hull cannot tint faces.
            gl.glStencilMask(0);
            gl.glStencilFunc(GL_EQUAL, 0, 1);
            gl.glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            gl.glDepthMask(GL_FALSE);
            s.surface->setUniformValue("uOutline", true);
            s.surface->setUniformValue("uOutlinePixels", 1.05f * dpr);
            for (std::size_t i = 0; i < s.buffers.size(); ++i) {
                const auto& part = s.parts[i];
                if (part.suppressed) continue;
                const auto& gpu = part.usesMesh(s.mode) ? s.buffers[i].mesh : s.buffers[i].cad;
                if (!gpu.triangleCount) continue;
                setPose(*s.surface, part);
                s.surface->setUniformValue("uColor", part.id == s.selected ? rgb(21, 94, 170) : rgb(57, 72, 89));
                gl.glBindVertexArray(gpu.vao);
                gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.triangles);
                gl.glDrawElements(GL_TRIANGLES, gpu.triangleCount, GL_UNSIGNED_INT, nullptr);
            }
            gl.glDisable(GL_STENCIL_TEST);
            gl.glStencilMask(0xff);
        }
        s.surface->release();
    }

    if (!s.buffers.empty()) {
        setup(*s.edge);
        gl.glDepthFunc(GL_LEQUAL);
        gl.glDepthMask(wireframe ? GL_TRUE : GL_FALSE);
        gl.glEnable(GL_BLEND);
        gl.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        s.edge->setUniformValue("uDepthBiasNdc", wireframe ? 0.0f : depthBias);
        for (std::size_t i = 0; i < s.buffers.size(); ++i) {
            const auto& part = s.parts[i];
            if (part.suppressed) continue;
            const bool meshed = part.usesMesh(s.mode), selected = part.id == s.selected;
            const auto& geometry = meshed ? part.mesh : part.cad;
            const auto& gpu = meshed ? s.buffers[i].mesh : s.buffers[i].cad;
            if (!gpu.edgeCount || !showsEdges(s.mode, geometry)) continue;
            const auto color = selected ? rgb(15, 78, 150) : (meshed ? rgb(31, 67, 95) : rgb(42, 57, 75));
            setPose(*s.edge, part);
            s.edge->setUniformValue("uColor", QVector4D(color, meshed ? 0.8f : 0.94f));
            s.edge->setUniformValue("uWidthPixels", (selected ? 1.75f : (meshed ? 1.0f : 1.25f)) * dpr);
            gl.glBindVertexArray(gpu.vao);
            gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.edges);
            gl.glDrawElements(GL_LINES, gpu.edgeCount, GL_UNSIGNED_INT, nullptr);
        }
        s.edge->release();
    }
    drawCadPickFeedback();
    gl.glBindVertexArray(0);
    gl.glDepthMask(GL_TRUE);
    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
    drawMotionSymbols();
    drawScalarLegend();
    drawPickOverlay();
}

void OccViewport::drawCadPickFeedback() try
{
    auto& s = *m_state;
    auto& gl = *s.gl;
    struct Entry { PickResult pick; const Part* part; std::size_t partIndex; QVector3D color; };
    std::vector<Entry> entries;
    std::set<PickBufferKey> needed;
    const auto append = [&](const PickResult& pick, const QVector3D& color) {
        if (!cadPick(pick.kind)) return;
        const auto part = std::find_if(s.parts.begin(), s.parts.end(), [&](const Part& p) { return p.id == pick.partId; });
        // Rest CAD must never reappear on top of a deformed or suppressed body.
        const auto geometry=s.pickGeometry.find(pick.partId);
        if (part == s.parts.end() || part->suppressed || part->deformed || geometry==s.pickGeometry.end() ||
            !validPickReference(pick,geometry->second)) return;
        const PickBufferKey key{pick.partId, pick.kind, pick.index};
        if (!needed.insert(key).second) return; // selected styling wins over hover
        entries.push_back({pick, &*part, std::size_t(part - s.parts.begin()), color});
    };
    if (s.pickFeedbackVisible && (s.pickActive || s.selectionFilter != PickKind::Part)) {
        const auto& selection = s.pickActive ? s.picks : s.viewportSelection;
        for (const auto& pick : selection) append(pick, QVector3D(s.pickColor.redF(),s.pickColor.greenF(),s.pickColor.blueF()));
        append(s.hover, rgb(199, 46, 153));
    }
    for (const auto& layer : s.auxiliaryPicks)
        for (const auto& pick : layer.second.picks)
            append(pick, QVector3D(layer.second.color.redF(),layer.second.color.greenF(),layer.second.color.blueF()));
    for (auto it = s.pickBuffers.begin(); it != s.pickBuffers.end();) {
        if (needed.count(it->first)) { ++it; continue; }
        deleteBuffers(gl, it->second);
        it = s.pickBuffers.erase(it);
    }
    if (entries.empty() && s.auxiliaryPoints.empty()) return;

    const auto frame = s.camera.frame(size(), s.bounds);
    GLint viewport[4]{};
    gl.glGetIntegerv(GL_VIEWPORT, viewport);
    const QVector2D pixels(float(std::max(1, viewport[2])), float(std::max(1, viewport[3])));
    const float dpr = float(devicePixelRatioF()), bias = coplanarDepthBias(gl);
    const auto setup = [&](QOpenGLShaderProgram& program, const Part& part) {
        program.bind();
        program.setUniformValue("uView", frame.view);
        program.setUniformValue("uProjection", frame.projection);
        program.setUniformValue("uPivot", s.camera.pivot);
        program.setUniformValue("uViewportPixels", pixels);
        const auto* pose = s.poseFor(part.id);
        program.setUniformValue("uPosed", pose != nullptr);
        if (pose) {
            const Vec offset = pose->relativePoint(vec(part.origin), vec(s.camera.pivot));
            QMatrix4x4 transform = pose->restToWorld;
            transform.setColumn(3, QVector4D(float(offset[0]), float(offset[1]), float(offset[2]), 1));
            program.setUniformValue("uPartOrigin", part.origin);
            program.setUniformValue("uPartToPivot", transform);
        }
    };
    // Follow Motion's depth-tested CAD feedback. Screen overlays below remain
    // reserved for HUD and explicitly through-selected FEM nodes/connectivity.
    gl.glEnable(GL_DEPTH_TEST);
    gl.glDepthFunc(GL_LEQUAL);
    gl.glDepthMask(GL_FALSE);
    gl.glDisable(GL_STENCIL_TEST);
    gl.glEnable(GL_BLEND);
    gl.glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    for (const auto& entry : entries) {
        const auto& pick = entry.pick;
        const PickBufferKey key{pick.partId, pick.kind, pick.index};
        const Buffers* gpu = nullptr;
        if (pick.kind == PickKind::Part && entry.partIndex < s.buffers.size()) {
            // Whole-part feedback reuses display buffers rather than copying
            // the more finely sampled curves used for analytical CAD picking.
            gpu = entry.part->usesMesh(s.mode) ? &s.buffers[entry.partIndex].mesh : &s.buffers[entry.partIndex].cad;
        } else if (pick.kind == PickKind::Edge || pick.kind == PickKind::Face || pick.kind == PickKind::Solid) {
          auto found = s.pickBuffers.find(key);
          if (found == s.pickBuffers.end()) {
            Buffers buffer;
            bool ok = false;
            GLenum uploadError = GL_NO_ERROR;
            try {
                const auto geometry = cadPickGeometry(s.pickGeometry.at(pick.partId), pick);
                ok = bufferGeometry(gl, geometry, buffer);
                uploadError = gl.glGetError();
                ok = ok && uploadError == GL_NO_ERROR;
                if (ok) {
                    found = s.pickBuffers.emplace(key, buffer).first;
                    buffer = {}; // ownership transferred only after insertion
                }
            } catch (const Standard_Failure&) { ok = false; }
              catch (const std::exception&) { ok = false; }
            if (!ok) {
                deleteBuffers(gl, buffer);
                reportGraphicsError(uploadError == GL_NO_ERROR ? tr("无法创建 CAD 拾取高亮。") :
                    tr("无法上传 CAD 拾取高亮（OpenGL 0x%1）。").arg(uint(uploadError),0,16));
                continue;
            }
          }
          gpu = &found->second;
        }
        const auto& part = *entry.part;
        if (gpu && gpu->triangleCount && s.mode != DisplayMode::Wireframe) {
            setup(*s.surface, part);
            s.surface->setUniformValue("uNormal", frame.normal);
            s.surface->setUniformValue("uOutline", false);
            s.surface->setUniformValue("uHighlight", true);
            s.surface->setUniformValue("uHighlightDepthBiasNdc", bias);
            s.surface->setUniformValue("uHighlightAlpha", 0.22f);
            s.surface->setUniformValue("uColor", entry.color);
            gl.glBindVertexArray(gpu->vao);
            gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->triangles);
            gl.glDrawElements(GL_TRIANGLES, gpu->triangleCount, GL_UNSIGNED_INT, nullptr);
            s.surface->setUniformValue("uHighlight", false);
            s.surface->release();
        }
        if (gpu && gpu->edgeCount) {
            setup(*s.edge, part);
            s.edge->setUniformValue("uDepthBiasNdc", bias);
            s.edge->setUniformValue("uWidthPixels", 3.0f * dpr);
            s.edge->setUniformValue("uColor", QVector4D(entry.color, 1));
            gl.glBindVertexArray(gpu->vao);
            gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu->edges);
            gl.glDrawElements(GL_LINES, gpu->edgeCount, GL_UNSIGNED_INT, nullptr);
            s.edge->release();
        }
        // Preserve the actual reference position supplied to the picking
        // control (e.g. a circular edge's centre), with the same scene depth.
        if (std::all_of(pick.position.begin(),pick.position.end(),[](double v){return std::isfinite(v)&&std::abs(v)<=maxSpan;})) {
            setup(*s.point, part);
            s.point->setUniformValue("uPointPosition", QVector3D(float(pick.position[0]),float(pick.position[1]),float(pick.position[2])));
            s.point->setUniformValue("uDepthBiasNdc", bias);
            s.point->setUniformValue("uDiameterPixels", 10.0f * dpr);
            s.point->setUniformValue("uBorderPixels", 1.4f * dpr);
            s.point->setUniformValue("uColor", QVector4D(entry.color, 1));
            gl.glBindVertexArray(s.emptyVao);
            gl.glDrawArrays(GL_POINTS, 0, 1);
            s.point->release();
        }
    }
    for (const auto& item : s.auxiliaryPoints) {
        const auto& layer = item.second;
        s.point->bind();
        s.point->setUniformValue("uView", frame.view);
        s.point->setUniformValue("uProjection", frame.projection);
        s.point->setUniformValue("uPivot", s.camera.pivot);
        s.point->setUniformValue("uViewportPixels", pixels);
        s.point->setUniformValue("uPosed", false);
        s.point->setUniformValue("uDepthBiasNdc", bias);
        s.point->setUniformValue("uDiameterPixels", layer.size * dpr);
        s.point->setUniformValue("uBorderPixels", std::min(1.4f,layer.size*.18f) * dpr);
        s.point->setUniformValue("uColor", QVector4D(layer.color.redF(),layer.color.greenF(),layer.color.blueF(),layer.color.alphaF()));
        gl.glBindVertexArray(s.emptyVao);
        for (const auto& point : layer.points) {
            s.point->setUniformValue("uPointPosition", point);
            gl.glDrawArrays(GL_POINTS, 0, 1);
        }
        s.point->release();
    }
}
catch (const std::exception& failure) {
    reportGraphicsError(tr("无法准备 CAD 拾取高亮：%1").arg(QString::fromUtf8(failure.what())));
}

void OccViewport::drawMotionSymbols()
{
    auto& s=*m_state;
    if (s.symbols.isEmpty() && s.auxiliarySymbols.empty()) return;
    std::vector<const MotionSymbol*> symbols;
    std::set<std::pair<QString,int>> originalEntities;
    for(const auto& layer:s.auxiliarySymbols)for(const auto& symbol:layer.second)
        if(symbol.id>=0&&(!symbol.linePoints.empty()||!symbol.trianglePoints.empty()))
            originalEntities.emplace(symbol.collection,symbol.id);
    for(const auto& symbol:s.symbols)
        if(!originalEntities.count({symbol.collection,symbol.id}))symbols.push_back(&symbol);
    for(const auto& layer:s.auxiliarySymbols)for(const auto& symbol:layer.second)symbols.push_back(&symbol);
    // Original Motion symbols use depth=false and world coordinates. Preserve
    // that overlay path; explicit depth=true symbols use the scene's GL depth.
    const auto frame=s.camera.frame(size(),s.bounds);
    auto& gl=*s.gl;
    for(const auto* symbol:symbols) {
        if(!symbol->lineDepthTest&&!symbol->faceDepthTest)continue;
        std::vector<Vec> vertices;
        std::vector<Triangle> triangles;
        std::vector<Edge> edges;
        if(symbol->faceDepthTest)for(std::size_t i=0;i+2<symbol->trianglePoints.size();i+=3) {
            const auto first=Index(vertices.size());
            vertices.insert(vertices.end(),symbol->trianglePoints.begin()+i,symbol->trianglePoints.begin()+i+3);
            triangles.push_back({first,first+1,first+2});
        }
        if(symbol->lineDepthTest)for(std::size_t i=0;i+1<symbol->linePoints.size();i+=2) {
            const auto first=Index(vertices.size());
            vertices.insert(vertices.end(),symbol->linePoints.begin()+i,symbol->linePoints.begin()+i+2);
            edges.push_back({first,first+1});
        }
        if(vertices.empty())continue;
        Buffers buffer;
        try {
            const auto geometry=snapshot(vertices.size(),triangles,edges,true,[&](Index i){return vertices[i];});
            if(!bufferGeometry(gl,geometry,buffer)||gl.glGetError()!=GL_NO_ERROR) {deleteBuffers(gl,buffer);continue;}
            GLint viewport[4]{};gl.glGetIntegerv(GL_VIEWPORT,viewport);
            const auto setup=[&](QOpenGLShaderProgram& program) {
                program.bind();program.setUniformValue("uView",frame.view);program.setUniformValue("uProjection",frame.projection);
                program.setUniformValue("uPivot",s.camera.pivot);
                program.setUniformValue("uViewportPixels",QVector2D(std::max(1,viewport[2]),std::max(1,viewport[3])));
                const auto* pose=s.poseFor(symbol->partId);program.setUniformValue("uPosed",pose!=nullptr);
                if(pose) {
                    const Vec offset=pose->relativePoint(vec(s.camera.pivot),vec(s.camera.pivot));
                    QMatrix4x4 transform=pose->restToWorld;
                    transform.setColumn(3,QVector4D(offset[0],offset[1],offset[2],1));
                    program.setUniformValue("uPartOrigin",s.camera.pivot);program.setUniformValue("uPartToPivot",transform);
                }
            };
            gl.glEnable(GL_DEPTH_TEST);gl.glDepthFunc(GL_LEQUAL);gl.glDepthMask(GL_FALSE);
            gl.glEnable(GL_BLEND);gl.glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
            const float bias=coplanarDepthBias(gl);
            gl.glBindVertexArray(buffer.vao);
            if(buffer.triangleCount&&symbol->faceDepthTest) {
                const auto color=symbol->faceColor.isValid()?symbol->faceColor:symbol->color;
                setup(*s.surface);s.surface->setUniformValue("uNormal",frame.normal);
                s.surface->setUniformValue("uOutline",false);s.surface->setUniformValue("uHighlight",true);
                s.surface->setUniformValue("uHighlightDepthBiasNdc",bias);s.surface->setUniformValue("uHighlightAlpha",float(color.alphaF()));
                s.surface->setUniformValue("uColor",QVector3D(color.redF(),color.greenF(),color.blueF()));
                gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer.triangles);gl.glDrawElements(GL_TRIANGLES,buffer.triangleCount,GL_UNSIGNED_INT,nullptr);
                s.surface->setUniformValue("uHighlight",false);s.surface->release();
            }
            if(buffer.edgeCount&&symbol->lineDepthTest) {
                const auto color=symbol->color;
                setup(*s.edge);s.edge->setUniformValue("uDepthBiasNdc",bias);
                s.edge->setUniformValue("uWidthPixels",float((symbol->emphasized?2.7:1.65)*devicePixelRatioF()));
                s.edge->setUniformValue("uColor",QVector4D(color.redF(),color.greenF(),color.blueF(),color.alphaF()));
                gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,buffer.edges);gl.glDrawElements(GL_LINES,buffer.edgeCount,GL_UNSIGNED_INT,nullptr);s.edge->release();
            }
            deleteBuffers(gl,buffer);
        } catch(const std::exception&) {deleteBuffers(gl,buffer);}
    }
    gl.glBindVertexArray(0);gl.glDepthMask(GL_TRUE);gl.glDisable(GL_BLEND);gl.glDisable(GL_DEPTH_TEST);
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    for (const auto* pointer : symbols) {
        const auto& symbol=*pointer;
        QColor color = symbol.color.isValid() ? symbol.color : QColor(150, 61, 179);
        if (!symbol.emphasized&&!symbol.retainOpacity) color.setAlpha(200);
        if(!symbol.faceDepthTest) {
            painter.setPen(Qt::NoPen);painter.setBrush(symbol.faceColor.isValid()?symbol.faceColor:color);
            for(std::size_t i=0;i+2<symbol.trianglePoints.size();i+=3) {
                QPolygonF triangle;
                for(std::size_t j=0;j<3;++j)triangle.append(projectPoint(symbol.trianglePoints[i+j],symbol.partId));
                painter.drawPolygon(triangle);
            }
        }
        painter.setPen(QPen(color, symbol.emphasized ? 2.7 : 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        if(!symbol.lineDepthTest)
            for (std::size_t i = 0; i + 1 < symbol.linePoints.size(); i += 2)
                painter.drawLine(projectPoint(symbol.linePoints[i], symbol.partId), projectPoint(symbol.linePoints[i + 1], symbol.partId));
        const auto origin = projectPoint(symbol.origin, symbol.partId);
        if(symbol.drawOrigin) {
            painter.setBrush(color); painter.setPen(QPen(Qt::white, 1));
            const double radius = symbol.emphasized ? 4.5 : 2.7;
            painter.drawEllipse(origin, radius, radius);
        }
        painter.setPen(symbol.textColor.isValid()?symbol.textColor:color);
        for(const auto& text:symbol.texts) {
            const auto position=projectPoint(text.second,symbol.partId);
            const auto bounds=painter.fontMetrics().boundingRect(text.first);
            painter.drawText(QRectF(position-QPointF(bounds.width()*.5,bounds.height()*.5),bounds.size()),Qt::AlignCenter,text.first);
        }
        if (symbol.emphasized && !symbol.label.isEmpty() && symbol.texts.empty()) {
            const auto bounds = painter.fontMetrics().boundingRect(symbol.label).adjusted(-5,-3,5,3);
            const QRectF textBox(origin + QPointF(9,-bounds.height()-2), QSizeF(bounds.size()));
            painter.setPen(Qt::NoPen); painter.setBrush(QColor(255,255,255,225)); painter.drawRoundedRect(textBox,3,3);
            painter.setPen(color.darker(140)); painter.drawText(textBox,Qt::AlignCenter,symbol.label);
        }
    }
}

void OccViewport::drawScalarLegend()
{
    const auto& s = *m_state;
    if (s.scalarParts.empty() || width() < 260 || height() < 180) return;
    QPainter painter(this);
    const int barHeight = std::min(220, height() - 100);
    const QRectF panel(width() - 177, 38, 165, barHeight + 65);
    painter.setPen(Qt::NoPen); painter.setBrush(QColor(255, 255, 255, 225)); painter.drawRoundedRect(panel, 5, 5);
    painter.setPen(QColor(34, 51, 69));
    painter.drawText(panel.adjusted(9, 5, -8, -barHeight - 30), Qt::AlignTop | Qt::TextWordWrap, s.scalarTitle);
    const QRectF bar(panel.left() + 12, panel.top() + 40, 18, barHeight);
    QLinearGradient gradient(bar.bottomLeft(), bar.topLeft());
    for (int i = 0; i <= 16; ++i) {
        const double t = i / 16.0;
        const auto channel = [t](double center) { return std::clamp(1.5 - std::abs(4*t - center), 0.0, 1.0); };
        gradient.setColorAt(t, QColor::fromRgbF(channel(3), channel(2), channel(1)));
    }
    painter.fillRect(bar, gradient);
    painter.setPen(QColor(53, 69, 87)); painter.setBrush(Qt::NoBrush); painter.drawRect(bar);
    for (int i = 0; i <= 4; ++i) {
        const double t = i / 4.0, value = s.scalarMaximum * (1-t) + s.scalarMinimum * t;
        const double y = bar.top() + bar.height() * t;
        painter.drawLine(QPointF(bar.right(), y), QPointF(bar.right() + 5, y));
        painter.drawText(QRectF(bar.right() + 9, y - 10, 112, 20), Qt::AlignVCenter, QString::number(value, 'g', 6));
    }
}

void OccViewport::drawPickOverlay()
{
    const auto& s = *m_state;
    const bool showActive = s.pickFeedbackVisible && (s.pickActive || s.selectionFilter != PickKind::Part);
    const auto kind = s.pickActive ? s.pickKind : s.selectionFilter.value_or(PickKind::Part);
    if (!showActive && s.auxiliaryPicks.empty()) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (showActive && kind == PickKind::Node) {
        painter.setPen(QPen(QColor(37, 116, 174, 190), 1));
        painter.setBrush(QColor(226, 245, 255, 220));
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (const auto& point : geometry.nodes) {
                const auto screen = projectPoint(point, partId);
                if (QRectF(rect()).contains(screen)) painter.drawEllipse(screen, 2.5, 2.5);
            }
        }
    } else if (showActive && (kind == PickKind::MeshEdge || kind == PickKind::MeshFace || kind == PickKind::Element)) {
        painter.setPen(QPen(QColor(35,108,151,150),1));
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (const auto& edge : geometry.meshEdges)
                painter.drawLine(projectPoint(geometry.nodes[edge[0]],partId),projectPoint(geometry.nodes[edge[1]],partId));
        }
    }
    const auto highlight = [&](const PickResult& pick, const QColor& color) {
        if (cadPick(pick.kind)) return; // rendered with real depth above
        const auto found = s.pickGeometry.find(pick.partId);
        if (found == s.pickGeometry.end()) return;
        const auto& geometry = found->second;
        if(!validPickReference(pick,geometry))return;
        painter.setPen(QPen(color, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        if (pick.kind == PickKind::MeshEdge && pick.index > 0 && pick.index <= int(geometry.meshEdges.size())) {
            const auto& edge = geometry.meshEdges[pick.index-1];
            painter.drawLine(projectPoint(geometry.nodes[edge[0]],pick.partId),projectPoint(geometry.nodes[edge[1]],pick.partId));
        } else if ((pick.kind == PickKind::MeshFace || pick.kind == PickKind::Element) && pick.index > 0) {
            std::vector<Index> nodes;
            if (pick.kind == PickKind::Element && !geometry.elements.empty() && pick.index <= int(geometry.elements.size()))
                nodes.assign(geometry.elements[pick.index-1].begin(),geometry.elements[pick.index-1].end());
            else if (pick.index <= int(geometry.meshFaces.size()))
                nodes.assign(geometry.meshFaces[pick.index-1].begin(),geometry.meshFaces[pick.index-1].end());
            if (std::all_of(nodes.begin(),nodes.end(),[&](Index n) { return n < geometry.nodes.size(); })) {
                if (nodes.size() == 3) {
                    QPolygonF face; for (auto node : nodes) face.append(projectPoint(geometry.nodes[node],pick.partId));
                    QColor tint = color; tint.setAlpha(50); painter.setBrush(tint); painter.drawPolygon(face); painter.setBrush(Qt::NoBrush);
                } else for (std::size_t i=0; i<nodes.size(); ++i) for (std::size_t j=i+1; j<nodes.size(); ++j)
                    painter.drawLine(projectPoint(geometry.nodes[nodes[i]],pick.partId),projectPoint(geometry.nodes[nodes[j]],pick.partId));
            }
        }
        Vec point = pick.position;
        if (pick.kind == PickKind::Node && pick.index >= 0 && pick.index < int(geometry.nodes.size())) point = geometry.nodes[pick.index];
        const auto screen = projectPoint(point, pick.partId);
        painter.setBrush(color); painter.setPen(QPen(Qt::white, 1.4)); painter.drawEllipse(screen, 5.0, 5.0);
    };
    for (const auto& layer : s.auxiliaryPicks)
        for (const auto& pick : layer.second.picks) highlight(pick, layer.second.color);
    if (!showActive) return;
    const auto& selection = s.pickActive ? s.picks : s.viewportSelection;
    for (const auto& pick : selection) highlight(pick, s.pickColor);
    if (s.hover.isValid()) highlight(s.hover, QColor(199, 46, 153));
    if (s.boxSelecting) {
        painter.setPen(QPen(QColor(25, 114, 195), 1, Qt::DashLine));
        painter.setBrush(QColor(60, 160, 230, 35)); painter.drawRect(s.pickRectangle);
    }
    if (!s.pickActive) return;
    const QStringList names{tr("部件"), tr("顶点"), tr("边"), tr("面"), tr("节点"), tr("网格边"), tr("网格面"), tr("单元"), tr("实体")};
    const QString label = tr("拾取%1 · 已选 %2 · 右键撤回 · Esc 结束%3")
        .arg(names.value(int(s.pickKind))).arg(s.picks.size())
        .arg(s.pickKind == PickKind::Node && s.pickMultiple ? tr(" · Ctrl + 左键贯穿框选") : QString());
    const QFontMetrics metrics(painter.font());
    const QRectF box(12, 12, std::min(width() - 24, metrics.horizontalAdvance(label) + 20), metrics.height() + 16);
    painter.setPen(Qt::NoPen); painter.setBrush(QColor(255, 255, 255, 225)); painter.drawRoundedRect(box, 4, 4);
    painter.setPen(QColor(38, 55, 72)); painter.drawText(box.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
}

int OccViewport::pickPart(const QPointF& position) const
{
    const auto& s = *m_state;
    if (!s.ready || s.uploadFailed || !s.bounds.valid || !QRectF(rect()).contains(position)) return -1;
    const auto frame = s.camera.frame(size(), s.bounds);
    const double unit = s.camera.mmPerPixel(size());
    const QPointF eye((position.x() - width()*0.5)*unit - float(s.camera.pan.x()),
                      (height()*0.5 - position.y())*unit - float(s.camera.pan.y()));
    const Vec pivot = vec(s.camera.pivot);
    // An orthographic ray is the transpose of the frame rotation applied to
    // (mouse x, mouse y, near z); no fragile float matrix inversion required.
    Ray ray{};
    for (int i = 0; i < 3; ++i) {
        ray.origin[i] = pivot[i] + frame.rows[0][i]*eye.x() + frame.rows[1][i]*eye.y() + frame.rows[2][i]*frame.nearZ;
        ray.direction[i] = -frame.rows[2][i];
    }
    double nearest = frame.nearZ - frame.farZ;
    int best = -1;
    const bool wireframe = s.mode == DisplayMode::Wireframe;
    if (!wireframe) {
        for (const auto& part : s.parts) {
            if (part.suppressed) continue;
            const auto& geometry = part.usesMesh(s.mode) ? part.mesh : part.cad;
            const Ray localRay = restRay(ray, s.poseFor(part.id));
            if (!intersectsBounds(localRay, geometry.bounds, nearest, unit * 0.01)) continue;
            const auto& triangles = geometry.triangles;
            for (std::size_t i = 0; i < triangles.size(); i += 3) {
                double distance = 0;
                if (intersectsTriangle(localRay, geometry.position(triangles[i]), geometry.position(triangles[i+1]),
                                       geometry.position(triangles[i+2]), distance) && distance < nearest) {
                    nearest = distance;
                    best = part.id;
                }
            }
        }
    }

    const double tolerance = unit * 5.0; // logical pixels, independent of DPI and zoom
    double bestOffset = tolerance * tolerance;
    double bestLineDepth = nearest;
    const bool hitFace = best != -1;
    for (const auto& part : s.parts) {
        if (part.suppressed) continue;
        const auto& geometry = part.usesMesh(s.mode) ? part.mesh : part.cad;
        if (!showsEdges(s.mode, geometry) || (hitFace && !geometry.triangles.empty())) continue;
        const auto* pose = s.poseFor(part.id);
        if (!intersectsBounds(restRay(ray, pose), geometry.bounds, nearest, tolerance)) continue;
        const auto relativePoint = [&](Index i) {
            const Vec point = geometry.position(i);
            return pose ? pose->relativePoint(point, pivot) : subtract(point, pivot);
        };
        for (std::size_t i = 0; i < geometry.edges.size(); i += 2) {
            const Vec a = relativePoint(geometry.edges[i]);
            const Vec b = relativePoint(geometry.edges[i+1]);
            const QPointF p0(dot(frame.rows[0], a), dot(frame.rows[1], a));
            const QPointF p1(dot(frame.rows[0], b), dot(frame.rows[1], b));
            const auto delta = p1 - p0;
            const double length2 = QPointF::dotProduct(delta, delta);
            if (length2 <= 0.0) continue;
            const double t = std::clamp(QPointF::dotProduct(eye - p0, delta) / length2, 0.0, 1.0);
            const auto offset = eye - (p0 + t * delta);
            const double offset2 = QPointF::dotProduct(offset, offset);
            const double depth = frame.nearZ - dot(frame.rows[2], a)*(1.0-t) - dot(frame.rows[2], b)*t;
            if (depth < 0.0 || depth > nearest || offset2 > bestOffset ||
                (offset2 == bestOffset && depth >= bestLineDepth)) continue;
            bestOffset = offset2;
            bestLineDepth = depth;
            best = part.id;
        }
    }
    return best;
}

void OccViewport::resizeEvent(QResizeEvent* event)
{
    interruptPointerOperation();
    QOpenGLWidget::resizeEvent(event);
    // If Qt cannot create a context it never invokes initializeGL().
    if (isVisible() && !event->size().isEmpty() && !isValid())
        reportGraphicsError(tr("Qt could not create a desktop OpenGL context for the CAD viewport."));
}

void OccViewport::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton &&
        event->button() != Qt::RightButton) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    auto& s = *m_state;
    s.rememberCamera();
    // An additional button never replaces the active operation or its camera anchor.
    if (s.button != Qt::NoButton && event->button() != s.button && event->buttons().testFlag(s.button)) {
        event->accept();
        return;
    }
    interruptPointerOperation();
    s.button = event->button();
    s.press = s.last = s.zoomAnchor = event->localPos();
    if (s.navigationMode == MouseNavigationMode::Nx) {
        if (s.button == Qt::MiddleButton) s.updateNxNavigation(event->modifiers(), s.press, size());
        else if (s.button == Qt::LeftButton) s.navigation = State::Navigation::Select;
    } else {
        s.navigation = s.button == Qt::LeftButton ? State::Navigation::Orbit : State::Navigation::Pan;
    }
    if (s.navigation == State::Navigation::Orbit) s.beginOrbit(s.press, size());
    const bool selectingNodes = s.pickActive ? s.pickMultiple && s.pickKind == PickKind::Node
                                            : s.selectionFilter == PickKind::Node;
    s.boxSelecting = s.pickEnabled && selectingNodes &&
                     s.button == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier);
    s.pickRectangle = QRectF(s.press, s.press);
    event->accept();
}

void OccViewport::mouseMoveEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    if (s.button == Qt::NoButton || !(event->buttons() & s.button)) {
        if (s.button != Qt::NoButton) interruptPointerOperation();
        if (s.pickActive || s.selectionFilter != PickKind::Part) {
            s.hover = pickAt(event->localPos());
            update(); event->accept(); return;
        }
        PickResult hover;
        if (s.pickEnabled) { hover.kind = PickKind::Part; hover.partId = pickPart(event->localPos()); }
        if (hover.partId != s.hover.partId || hover.kind != s.hover.kind) { s.hover = hover; update(); }
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }
    if (s.boxSelecting) {
        s.pickRectangle = QRectF(s.press, event->localPos()).normalized();
        s.dragging = s.pickRectangle.width() + s.pickRectangle.height() >= QApplication::startDragDistance();
        update(); event->accept(); return;
    }
    if (s.navigationMode == MouseNavigationMode::Nx && s.button == Qt::MiddleButton &&
        s.updateNxNavigation(event->modifiers(), event->localPos(), size())) {
        event->accept();
        return;
    }
    s.dragTo(event->localPos(), size());
    if (s.dragging) update();
    event->accept();
}

void OccViewport::mouseReleaseEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    if (s.button == Qt::NoButton || event->button() != s.button) {
        QOpenGLWidget::mouseReleaseEvent(event);
        return;
    }
    const auto rectangle = QRectF(s.press, event->localPos()).normalized();
    if (s.boxSelecting && rectangle.width() + rectangle.height() >= QApplication::startDragDistance()) {
        auto& selection = s.pickActive ? s.picks : s.viewportSelection;
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                if (!rectangle.contains(projectPoint(geometry.nodes[index], partId))) continue;
                PickResult pick; pick.kind = PickKind::Node; pick.partId = partId;
                pick.index = index; pick.nodeId = geometry.nodeIds.value(index, index + 1);
                pick.position = pick.hitPosition = geometry.nodes[index];
                if (std::none_of(selection.cbegin(), selection.cend(), [&](const auto& p) { return samePick(p, pick); }))
                    selection.append(pick);
            }
        }
        interruptPointerOperation();
        if (s.pickActive) emit pickSelectionChanged(selection);
        else emit viewportSelectionChanged();
        update(); event->accept(); return;
    }
    const bool changedMode = s.navigationMode == MouseNavigationMode::Nx && s.button == Qt::MiddleButton &&
                            s.updateNxNavigation(event->modifiers(), event->localPos(), size());
    if (!changedMode) s.dragTo(event->localPos(), size());
    const bool click = s.button == Qt::LeftButton && !s.dragging && s.pickEnabled;
    const bool typedSelection = s.pickActive || s.selectionFilter != PickKind::Part;
    const bool subtract = s.button == Qt::RightButton && !s.dragging && s.pickEnabled && typedSelection;
    s.rememberCamera();
    interruptPointerOperation();
    event->accept();
    if (click) {
        if (typedSelection) {
            const auto hit = pickAt(event->localPos());
            auto& selection = s.pickActive ? s.picks : s.viewportSelection;
            const bool multiple = s.pickActive ? s.pickMultiple : event->modifiers().testFlag(Qt::ControlModifier);
            if (hit.isValid()) {
                if (!multiple) selection.clear();
                const auto found = std::find_if(selection.begin(), selection.end(), [&](const auto& p) { return samePick(p, hit); });
                if (found == selection.end()) selection.append(hit);
                else selection.erase(found);
                s.hover = hit;
                if (s.pickActive) emit pickSelectionChanged(selection);
                else emit viewportSelectionChanged();
            } else if (!s.pickActive && !multiple) {
                clearViewportSelection();
            }
            update(); return;
        }
        const int id = pickPart(event->localPos());
        selectPart(id);
        emit partPicked(id);
    } else if (subtract) {
        auto& selection = s.pickActive ? s.picks : s.viewportSelection;
        if (!selection.isEmpty()) {
            selection.removeLast();
            if (s.pickActive) emit pickSelectionChanged(selection);
            else emit viewportSelectionChanged();
        }
        update();
    }
}

bool OccViewport::setRotationCenterAt(const QPointF& position)
{
    auto& s = *m_state;
    if (!s.bounds.valid || s.mode == DisplayMode::Wireframe || !QRectF(rect()).contains(position)) return false;
    const auto frame = s.camera.frame(size(), s.bounds);
    const double unit = s.camera.mmPerPixel(size());
    const QPointF eye((position.x() - width()*0.5)*unit - s.camera.pan.x(),
                      (height()*0.5 - position.y())*unit - s.camera.pan.y());
    const Vec pivot = vec(s.camera.pivot);
    Ray world;
    for (int k = 0; k < 3; ++k) {
        world.origin[k] = pivot[k] + frame.rows[0][k]*eye.x() + frame.rows[1][k]*eye.y() + frame.rows[2][k]*frame.nearZ;
        world.direction[k] = -frame.rows[2][k];
    }
    double nearest = frame.nearZ - frame.farZ;
    bool found = false;
    for (const auto& part : s.parts) {
        if (part.suppressed) continue;
        const auto& geometry = part.usesMesh(s.mode) ? part.mesh : part.cad;
        const auto ray = restRay(world, s.poseFor(part.id));
        if (!intersectsBounds(ray, geometry.bounds, nearest, unit)) continue;
        for (std::size_t i = 0; i + 2 < geometry.triangles.size(); i += 3) {
            double distance;
            if (intersectsTriangle(ray, geometry.position(geometry.triangles[i]),
                    geometry.position(geometry.triangles[i+1]), geometry.position(geometry.triangles[i+2]), distance) &&
                distance < nearest) {
                nearest = distance;
                found = true;
            }
        }
    }
    if (!found) return false;
    Vec point;
    for (int k = 0; k < 3; ++k) point[k] = world.origin[k] + world.direction[k]*nearest;
    s.camera.setPivot(point);
    update();
    return true;
}

void OccViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (m_state->navigationMode == MouseNavigationMode::Nx && event->button() == Qt::LeftButton) {
        // Do not double-commit a collector or replace its geometry reference.
        if (!m_state->pickActive &&
            (m_state->button == Qt::NoButton || m_state->button == Qt::LeftButton)) {
            interruptPointerOperation();
            // Motion's blank-space fit is retained. Surface double-click is a
            // view-only extension of its screen-preserving rotation-reference API.
            if (event->modifiers().testFlag(Qt::ControlModifier) || !setRotationCenterAt(event->localPos())) fitAll();
        }
        event->accept();
        return;
    }
    QOpenGLWidget::mouseDoubleClickEvent(event);
}

void OccViewport::keyPressEvent(QKeyEvent* event)
{
    auto& s = *m_state;
    if (event->key() == Qt::Key_Escape && s.button != Qt::NoButton) {
        interruptPointerOperation(); event->accept(); return;
    }
    if (event->key() == Qt::Key_Escape && m_state->pickActive) {
        endPick(); emit pickCancelled(); event->accept(); return;
    }
    if (s.navigationMode == MouseNavigationMode::Nx && s.button == Qt::MiddleButton &&
        (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control)) {
        const auto modifier = event->key() == Qt::Key_Control ? Qt::ControlModifier : Qt::ShiftModifier;
        s.updateNxNavigation(event->modifiers() | modifier, s.last, size());
        event->accept(); return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

void OccViewport::keyReleaseEvent(QKeyEvent* event)
{
    auto& s = *m_state;
    if (s.navigationMode == MouseNavigationMode::Nx && s.button == Qt::MiddleButton &&
        (event->key() == Qt::Key_Shift || event->key() == Qt::Key_Control)) {
        const auto modifier = event->key() == Qt::Key_Control ? Qt::ControlModifier : Qt::ShiftModifier;
        s.updateNxNavigation(event->modifiers() & ~Qt::KeyboardModifiers(modifier), s.last, size());
        event->accept(); return;
    }
    QOpenGLWidget::keyReleaseEvent(event);
}

void OccViewport::interruptPointerOperation()
{
    auto& s = *m_state;
    if (s.triad) s.triad->InterruptPointerOperation();
    s.button = Qt::NoButton;
    s.navigation = State::Navigation::None;
    s.dragging = false;
    s.boxSelecting = false;
    s.pickRectangle = {};
    s.hover = {};
    // A focus/grab transition retires the pointer gesture, not the accepted
    // values or callbacks of NativePickSession.
    update();
}

bool OccViewport::event(QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride &&
        static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape &&
        (m_state->button != Qt::NoButton || m_state->pickActive)) {
        event->accept(); return true;
    }
    if (event->type() == QEvent::UngrabMouse || event->type() == QEvent::WindowDeactivate ||
        event->type() == QEvent::Hide || (event->type() == QEvent::EnabledChange && !isEnabled()))
        interruptPointerOperation();
    return QOpenGLWidget::event(event);
}

void OccViewport::focusOutEvent(QFocusEvent* event)
{
    interruptPointerOperation();
    QOpenGLWidget::focusOutEvent(event);
}

void OccViewport::leaveEvent(QEvent* event)
{
    m_state->hover = {}; update(); QOpenGLWidget::leaveEvent(event);
}

void OccViewport::wheelEvent(QWheelEvent* event)
{
    auto& s = *m_state;
    const int delta = event->angleDelta().y();
    const double steps = delta != 0 ? delta / 120.0 : event->pixelDelta().y() / 48.0;
    if (steps == 0.0) {
        QOpenGLWidget::wheelEvent(event);
        return;
    }
    if (s.button == Qt::NoButton) {
        s.rememberCamera();
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const auto position = event->position();
#else
        const auto position = event->posF();
#endif
        s.camera.zoomAt(-steps*0.16, position, size());
        s.rememberCamera();
        s.hover = s.pickActive ? pickAt(position) : PickResult{};
        update();
    }
    event->accept();
}

} // namespace MotionOcc
