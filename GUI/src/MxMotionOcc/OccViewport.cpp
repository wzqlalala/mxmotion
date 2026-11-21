#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionSymbols.h"
#include "render/OccRenderShaders.h"

#include <QApplication>
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
#include <limits>
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
    TopTools_IndexedMapOfShape faces, edges, vertices;
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
    DisplayMode mode = DisplayMode::ShadedEdges;
    int selected = -1;
    bool pickEnabled = true;
    bool pickActive = false, pickMultiple = false;
    PickKind pickKind = PickKind::Part;
    std::set<int> pickParts;
    QVector<PickResult> picks;
    PickResult hover;
    bool boxSelecting = false;
    QRectF pickRectangle;
    std::set<int> scalarParts;
    double scalarMinimum = 0, scalarMaximum = 1;
    QString scalarTitle;
    QVector<MotionSymbol> symbols;

    Qt::MouseButton button = Qt::NoButton;
    bool dragging = false;
    QPointF press, last, ballCenter;
    QQuaternion pressRotation;
    QVector3D ballStart;
    double ballRadius = 1.0;

    QOpenGLFunctions_3_3_Core* gl = nullptr; // owned by the current widget context
    QMetaObject::Connection destruction;
    std::unique_ptr<QOpenGLShaderProgram> backdrop, surface, edge;
    GLuint emptyVao = 0;
    std::vector<PartBuffers> buffers;
    bool ready = false, sceneDirty = true, uploadFailed = false;
    QString lastError;

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

    void dragTo(const QPointF& position, const QSize& size)
    {
        if (button == Qt::NoButton) return;
        if (!dragging) {
            const auto delta = position - press;
            if (std::abs(delta.x()) + std::abs(delta.y()) < QApplication::startDragDistance()) return;
            dragging = true;
        }
        if (button == Qt::LeftButton) {
            const auto delta = QQuaternion::rotationTo(ballStart, trackball(position, ballCenter, ballRadius));
            camera.rotation = (delta * pressRotation).normalized();
        } else {
            const QPointF delta = position - last;
            camera.pan += QPointF(delta.x(), -delta.y()) * camera.mmPerPixel(size);
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
    qRegisterMetaType<QVector<MotionOcc::PickResult>>();
}

OccViewport::~OccViewport() { cleanupGL(); }

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
}

void OccViewport::setPartPoses(const std::map<int, QMatrix4x4>& poses)
{
    auto& s = *m_state;
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

void OccViewport::setPickEnabled(bool enabled) { m_state->pickEnabled = enabled; }

void OccViewport::beginPick(PickKind kind, bool multiple, const std::set<int>& allowedParts,
                            const QVector<PickResult>& initial)
{
    auto& s = *m_state;
    s.pickActive = true; s.pickKind = kind; s.pickMultiple = multiple; s.pickParts = allowedParts;
    s.hover = {}; s.button = Qt::NoButton; s.boxSelecting = false;
    setPickSelection(initial);
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::CrossCursor);
    update();
}

void OccViewport::endPick()
{
    auto& s = *m_state;
    s.pickActive = false; s.picks.clear(); s.hover = {}; s.pickParts.clear();
    s.button = Qt::NoButton; s.boxSelecting = false; s.pickRectangle = {};
    unsetCursor();
    update();
}

bool OccViewport::isPicking() const { return m_state->pickActive; }
QVector<PickResult> OccViewport::pickSelection() const { return m_state->picks; }

void OccViewport::setPickSelection(const QVector<PickResult>& selection)
{
    auto& s = *m_state;
    s.picks.clear();
    for (auto pick : selection) {
        if (!pick.isValid() || pick.kind != s.pickKind || !s.pickGeometry.count(pick.partId) ||
            (!s.pickParts.empty() && !s.pickParts.count(pick.partId))) continue;
        const auto& geometry = s.pickGeometry.at(pick.partId);
        if ((pick.kind == PickKind::Vertex && (pick.index < 1 || pick.index > geometry.vertices.Extent())) ||
            (pick.kind == PickKind::Edge && (pick.index < 1 || pick.index > geometry.edges.Extent())) ||
            (pick.kind == PickKind::Face && (pick.index < 1 || pick.index > geometry.faces.Extent())) ||
            (pick.kind == PickKind::MeshEdge && (pick.index < 1 || pick.index > int(geometry.meshEdges.size()))) ||
            (pick.kind == PickKind::MeshFace && (pick.index < 1 || pick.index > int(geometry.meshFaces.size()))) ||
            (pick.kind == PickKind::Element && (pick.index < 1 || pick.index > int(geometry.elements.empty() ? geometry.meshFaces.size() : geometry.elements.size()))) ||
            (pick.kind == PickKind::Node && (pick.index < 0 || pick.index >= int(geometry.nodes.size()) ||
                (pick.nodeId > 0 && pick.nodeId != geometry.nodeIds.value(pick.index))))) continue;
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
            } else if (pick.kind == PickKind::Face || pick.kind == PickKind::Part) {
                GProp_GProps props;
                if (pick.kind == PickKind::Face) BRepGProp::SurfaceProperties(geometry.faces(pick.index),props);
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
        if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
        const auto part = std::find_if(s.parts.cbegin(), s.parts.cend(), [partId](const auto& p) { return p.id == partId; });
        if (part == s.parts.cend() || part->suppressed || part->deformed) continue;
        if (s.pickKind < PickKind::Node && !intersectsBounds(restRay(world, s.poseFor(partId)),
                part->cad.bounds, visibleDepth + 8 * unit, 8 * unit)) continue;
        PickResult candidate; candidate.kind = s.pickKind; candidate.partId = partId;
        try {
            if (s.pickKind == PickKind::Vertex) {
                for (int index = 1; index <= geometry.vertices.Extent(); ++index) {
                    candidate.index = index;
                    pointHit(vec(BRep_Tool::Pnt(TopoDS::Vertex(geometry.vertices(index)))), candidate);
                }
            } else if (s.pickKind == PickKind::Node) {
                for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                    candidate.index = index; candidate.nodeId = geometry.nodeIds.value(index, index + 1);
                    pointHit(geometry.nodes[index], candidate);
                }
            } else if (s.pickKind == PickKind::MeshEdge) {
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
            } else if (s.pickKind == PickKind::MeshFace || s.pickKind == PickKind::Element) {
                const auto ray = restRay(world,s.poseFor(partId));
                const bool volume = s.pickKind == PickKind::Element && !geometry.elements.empty();
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
                    candidate.hasDirection = s.pickKind == PickKind::MeshFace && normalise(candidate.direction);
                    best = candidate; bestDepth = depth;
                }
            } else if (s.pickKind == PickKind::Edge) {
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
                    candidate.index = s.pickKind == PickKind::Face ? geometry.faces.FindIndex(face) : -1;
                    if (s.pickKind == PickKind::Face && candidate.index < 1) continue;
                    candidate.hitPosition = vec(intersector.Pnt(hit));
                    GProp_GProps props;
                    if (s.pickKind == PickKind::Part) {
                        BRepGProp::VolumeProperties(geometry.shape, props);
                        if (std::abs(props.Mass()) < 1e-14) BRepGProp::SurfaceProperties(geometry.shape, props);
                    } else BRepGProp::SurfaceProperties(face, props);
                    candidate.position = std::abs(props.Mass()) > 1e-14 ? vec(props.CentreOfMass()) : candidate.hitPosition;
                    if (s.pickKind == PickKind::Face) {
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
    if (s.pickKind == PickKind::Part && !best.isValid()) {
        // Wire/edge-only imported construction geometry has no face intersector.
        const int partId = pickPart(position);
        if (partId > 0 && (s.pickParts.empty() || s.pickParts.count(partId))) {
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
    m_state->button = Qt::NoButton;
    m_state->camera.fit(m_state->bounds, size(), isVisible());
    update();
}

void OccViewport::viewIsometric()
{
    m_state->button = Qt::NoButton;
    m_state->camera.rotation = isometricRotation();
    update();
}

void OccViewport::viewFront()
{
    m_state->button = Qt::NoButton;
    m_state->camera.rotation = viewRotation({0, -1, 0}, {0, 0, 1});
    update();
}

void OccViewport::viewTop()
{
    m_state->button = Qt::NoButton;
    m_state->camera.rotation = viewRotation({0, 0, 1}, {0, 1, 0});
    update();
}

void OccViewport::viewRight()
{
    m_state->button = Qt::NoButton;
    m_state->camera.rotation = viewRotation({1, 0, 0}, {0, 0, 1});
    update();
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
    }
    s.emptyVao = 0;
    s.edge.reset();
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
        gl.glEnable(GL_POLYGON_OFFSET_FILL);
        gl.glPolygonOffset(1.0f, 1.0f);
        for (std::size_t i = 0; i < s.buffers.size(); ++i) {
            const auto& part = s.parts[i];
            if (part.suppressed) continue;
            const bool meshed = part.usesMesh(s.mode);
            const auto& gpu = meshed ? s.buffers[i].mesh : s.buffers[i].cad;
            if (!gpu.triangleCount) continue;
            const auto color = part.id == s.selected ? rgb(63, 137, 216) :
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
        s.edge->setUniformValue("uDepthBiasNdc", wireframe ? 0.0f : 2e-5f);
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
    gl.glBindVertexArray(0);
    gl.glDepthMask(GL_TRUE);
    gl.glDisable(GL_BLEND);
    gl.glDisable(GL_DEPTH_TEST);
    drawAxes();
    drawMotionSymbols();
    drawScalarLegend();
    drawPickOverlay();
}

void OccViewport::drawAxes()
{
    if (width() < 82 || height() < 82) return;
    const QPointF origin(39.0, height() - 39.0);
    const auto& rotation = m_state->camera.rotation;
    struct Axis { QVector3D direction; QColor color; const char* name; };
    std::array<Axis, 3> axes{{
        {rotation.rotatedVector({1, 0, 0}), QColor(183, 67, 63), "X"},
        {rotation.rotatedVector({0, 1, 0}), QColor(44, 123, 75), "Y"},
        {rotation.rotatedVector({0, 0, 1}), QColor(46, 103, 168), "Z"}
    }};
    std::sort(axes.begin(), axes.end(), [](const Axis& a, const Axis& b) { return a.direction.z() < b.direction.z(); });
    QPainter painter(this); // overlay only; no CAD or mesh rendering uses QPainter
    painter.setRenderHint(QPainter::Antialiasing);
    auto font = painter.font();
    font.setPixelSize(10);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(253, 254, 255, 178));
    painter.drawEllipse(origin, 29.0, 29.0);
    painter.setBrush(Qt::NoBrush);
    for (const auto& axis : axes) {
        const QPointF delta(axis.direction.x() * 21.0, -axis.direction.y() * 21.0);
        const double extent = std::hypot(delta.x(), delta.y());
        painter.setPen(QPen(axis.color, 1.65, Qt::SolidLine, Qt::RoundCap));
        if (extent < 2.0) {
            painter.drawEllipse(origin, 2.8, 2.8);
            painter.drawText(origin + QPointF(5, 11), QString::fromLatin1(axis.name));
        } else {
            const QPointF tip = origin + delta;
            painter.drawLine(origin, tip);
            const QPointF label = tip + delta * (5.0 / extent);
            painter.drawText(QRectF(label - QPointF(6, 6), QSizeF(12, 12)), Qt::AlignCenter, QString::fromLatin1(axis.name));
        }
    }
}




void OccViewport::drawMotionSymbols()
{
    const auto& symbols = m_state->symbols;
    if (symbols.isEmpty()) return;
    QPainter painter(this); painter.setRenderHint(QPainter::Antialiasing);
    for (const auto& symbol : symbols) {
        QColor color = symbol.color.isValid() ? symbol.color : QColor(150, 61, 179);
        if (!symbol.emphasized) color.setAlpha(200);
        painter.setPen(QPen(color, symbol.emphasized ? 2.7 : 1.65, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        for (std::size_t i = 0; i + 1 < symbol.linePoints.size(); i += 2)
            painter.drawLine(projectPoint(symbol.linePoints[i], symbol.partId), projectPoint(symbol.linePoints[i + 1], symbol.partId));
        const auto origin = projectPoint(symbol.origin, symbol.partId);
        painter.setBrush(color); painter.setPen(QPen(Qt::white, 1));
        const double radius = symbol.emphasized ? 4.5 : 2.7;
        painter.drawEllipse(origin, radius, radius);
        if (symbol.emphasized && !symbol.label.isEmpty()) {
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
    if (!s.pickActive) return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (s.pickKind == PickKind::Node) {
        painter.setPen(QPen(QColor(37, 116, 174, 190), 1));
        painter.setBrush(QColor(226, 245, 255, 220));
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (const auto& point : geometry.nodes) {
                const auto screen = projectPoint(point, partId);
                if (QRectF(rect()).contains(screen)) painter.drawEllipse(screen, 2.5, 2.5);
            }
        }
    } else if (s.pickKind == PickKind::MeshEdge || s.pickKind == PickKind::MeshFace || s.pickKind == PickKind::Element) {
        painter.setPen(QPen(QColor(35,108,151,150),1));
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (const auto& edge : geometry.meshEdges)
                painter.drawLine(projectPoint(geometry.nodes[edge[0]],partId),projectPoint(geometry.nodes[edge[1]],partId));
        }
    }
    const auto highlight = [&](const PickResult& pick, const QColor& color) {
        const auto found = s.pickGeometry.find(pick.partId);
        if (found == s.pickGeometry.end()) return;
        const auto& geometry = found->second;
        painter.setPen(QPen(color, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        const auto drawCurve = [&](const PickCurve& curve) {
            QPolygonF line;
            for (const auto& point : curve.points) line.append(projectPoint(point, pick.partId));
            if (line.size() >= 2) painter.drawPolyline(line);
        };
        if (pick.kind == PickKind::Part) {
            for (const auto& curve : geometry.curves) drawCurve(curve);
        } else if (pick.kind == PickKind::Edge && pick.index > 0 && pick.index <= int(geometry.curves.size())) {
            drawCurve(geometry.curves[pick.index - 1]);
        } else if (pick.kind == PickKind::Face && pick.index > 0 && pick.index <= geometry.faces.Extent()) {
            const auto face = TopoDS::Face(geometry.faces(pick.index));
            TopLoc_Location location;
            const auto mesh = BRep_Tool::Triangulation(face, location);
            if (!mesh.IsNull()) {
                QColor tint = color; tint.setAlpha(45);
                painter.setPen(Qt::NoPen); painter.setBrush(tint);
                for (int i = 1; i <= mesh->NbTriangles(); ++i) {
                    int a, b, c; mesh->Triangle(i).Get(a, b, c);
                    QPolygonF triangle;
                    for (int node : {a, b, c}) triangle.append(projectPoint(vec(mesh->Node(node).Transformed(location.Transformation())), pick.partId));
                    painter.drawPolygon(triangle);
                }
                painter.setPen(QPen(color, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
                painter.setBrush(Qt::NoBrush);
            }
            for (TopExp_Explorer edge(face, TopAbs_EDGE); edge.More(); edge.Next()) {
                const int index = geometry.edges.FindIndex(edge.Current());
                if (index > 0) drawCurve(geometry.curves[index - 1]);
            }
        } else if (pick.kind == PickKind::MeshEdge && pick.index > 0 && pick.index <= int(geometry.meshEdges.size())) {
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
        if (pick.kind == PickKind::Vertex && pick.index > 0 && pick.index <= geometry.vertices.Extent()) point = vec(BRep_Tool::Pnt(TopoDS::Vertex(geometry.vertices(pick.index))));
        const auto screen = projectPoint(point, pick.partId);
        painter.setBrush(color); painter.setPen(QPen(Qt::white, 1.4)); painter.drawEllipse(screen, 5.0, 5.0);
    };
    for (const auto& pick : s.picks) highlight(pick, QColor(232, 130, 28));
    if (s.hover.isValid()) highlight(s.hover, QColor(199, 46, 153));
    if (s.boxSelecting) {
        painter.setPen(QPen(QColor(25, 114, 195), 1, Qt::DashLine));
        painter.setBrush(QColor(60, 160, 230, 35)); painter.drawRect(s.pickRectangle);
    }
    const QStringList names{tr("部件"), tr("顶点"), tr("边"), tr("面"), tr("节点"), tr("网格边"), tr("网格面"), tr("单元")};
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
    m_state->button = Qt::NoButton;
    QOpenGLWidget::resizeEvent(event);
    // If Qt cannot create a context it never invokes initializeGL().
    if (isVisible() && !event->size().isEmpty() && !isValid())
        reportGraphicsError(tr("Qt could not create a desktop OpenGL context for the CAD viewport."));
}

void OccViewport::mousePressEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    // Left drag orbits (a click picks); middle or right drag pans.
    if (event->button() != Qt::LeftButton && event->button() != Qt::MiddleButton &&
        event->button() != Qt::RightButton) {
        QOpenGLWidget::mousePressEvent(event);
        return;
    }
    if (s.button == Qt::NoButton || !(event->buttons() & s.button) || event->button() == s.button) {
        s.button = event->button();
        s.dragging = false;
        s.press = s.last = event->localPos();
        const double unit = s.camera.mmPerPixel(size());
        s.ballCenter = QPointF(width()*0.5 + s.camera.pan.x()/unit, height()*0.5 - s.camera.pan.y()/unit);
        s.ballRadius = std::max(1.0, 0.5 * std::min(width(), height()));
        s.ballStart = trackball(s.press, s.ballCenter, s.ballRadius);
        s.pressRotation = s.camera.rotation;
        s.boxSelecting = s.pickActive && s.pickMultiple && s.pickKind == PickKind::Node &&
                         s.button == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier);
        s.pickRectangle = QRectF(s.press, s.press);
    }
    event->accept();
}

void OccViewport::mouseMoveEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    if (s.button == Qt::NoButton || !(event->buttons() & s.button)) {
        s.button = Qt::NoButton;
        if (s.pickActive) {
            s.hover = pickAt(event->localPos());
            update(); event->accept(); return;
        }
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }
    if (s.boxSelecting) {
        s.pickRectangle = QRectF(s.press, event->localPos()).normalized();
        s.dragging = true; update(); event->accept(); return;
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
    if (s.boxSelecting) {
        s.pickRectangle = QRectF(s.press, event->localPos()).normalized();
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                if (!s.pickRectangle.contains(projectPoint(geometry.nodes[index], partId))) continue;
                PickResult pick; pick.kind = PickKind::Node; pick.partId = partId;
                pick.index = index; pick.nodeId = geometry.nodeIds.value(index, index + 1);
                pick.position = pick.hitPosition = geometry.nodes[index];
                if (std::none_of(s.picks.cbegin(), s.picks.cend(), [&](const auto& p) { return samePick(p, pick); })) s.picks.append(pick);
            }
        }
        s.boxSelecting = false; s.dragging = false; s.button = Qt::NoButton; s.pickRectangle = {};
        emit pickSelectionChanged(s.picks); update(); event->accept(); return;
    }
    s.dragTo(event->localPos(), size());
    const bool click = s.button == Qt::LeftButton && !s.dragging && s.pickEnabled;
    const bool subtract = s.button == Qt::RightButton && !s.dragging && s.pickEnabled && s.pickActive;
    s.button = Qt::NoButton;
    if (s.dragging) update();
    event->accept();
    if (click) {
        if (s.pickActive) {
            const auto hit = pickAt(event->localPos());
            if (hit.isValid()) {
                if (!s.pickMultiple) s.picks.clear();
                const auto found = std::find_if(s.picks.begin(), s.picks.end(), [&](const auto& p) { return samePick(p, hit); });
                if (found == s.picks.end()) s.picks.append(hit);
                else s.picks.erase(found);
                s.hover = hit;
                emit pickSelectionChanged(s.picks);
            }
            update(); return;
        }
        const int id = pickPart(event->localPos());
        selectPart(id);
        emit partPicked(id);
    } else if (subtract) {
        if (!s.picks.isEmpty()) { s.picks.removeLast(); emit pickSelectionChanged(s.picks); }
        update();
    }
}

void OccViewport::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_state->pickActive) {
        endPick(); emit pickCancelled(); event->accept(); return;
    }
    QOpenGLWidget::keyPressEvent(event);
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        const auto position = event->position();
#else
        const auto position = event->posF();
#endif
        const double before = s.camera.span;
        s.camera.span = std::clamp(before * std::exp(std::clamp(-steps*0.16, -50.0, 50.0)), minSpan, maxSpan);
        const QPointF offset(position.x() - width()*0.5, height()*0.5 - position.y());
        s.camera.pan += offset * (s.camera.mmPerPixel(size()) * (1.0 - before/s.camera.span));
        update();
    }
    event->accept();
}

} // namespace MotionOcc
