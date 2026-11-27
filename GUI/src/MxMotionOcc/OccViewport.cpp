#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionSymbols.h"
#include "UiMetrics.h"
#include "render/OccRenderShaders.h"
#include "navigation/ViewportOrientationTriad.h"

#include <QApplication>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFocusEvent>
#include <QImage>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QPainter>
#include <QPixmap>
#include <QQuaternion>
#include <QResizeEvent>
#include <QSet>
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
#include <mutex>
#include <numeric>
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

struct BvhNode {
    Bounds box;
    Index left = 0;  // 内部节点：左子节点下标（右子节点为 left + 1）；叶节点：triOrder 起始偏移
    Index count = 0; // 0 表示内部节点，> 0 表示叶节点包含的三角形数量
};

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> triangles;
    std::vector<Index> edges;
    std::vector<Index> sourceIndices;
    std::vector<Index> bvhTriOrder;
    std::vector<BvhNode> bvh;
    Bounds bounds;

    Vec position(Index i) const
    {
        const auto& p = vertices[i].position;
        return {p[0], p[1], p[2]};
    }
};

void buildGeometryBvh(Geometry& g)
{
    const std::size_t triCount = g.triangles.size() / 3;
    g.bvh.clear();
    g.bvhTriOrder.clear();
    if (triCount < 8) return;
    g.bvhTriOrder.resize(triCount);
    std::iota(g.bvhTriOrder.begin(), g.bvhTriOrder.end(), Index(0));
    std::vector<Bounds> triBounds(triCount);
    std::vector<Vec> triCenters(triCount);
    for (std::size_t t = 0; t < triCount; ++t) {
        Bounds b;
        b.add(g.position(g.triangles[3 * t]));
        b.add(g.position(g.triangles[3 * t + 1]));
        b.add(g.position(g.triangles[3 * t + 2]));
        triBounds[t] = b;
        triCenters[t] = b.center();
    }
    g.bvh.reserve(triCount * 2);
    g.bvh.push_back({});
    const auto build = [&](auto& self, Index nodeIdx, std::size_t begin, std::size_t end) -> void {
        Bounds box;
        Bounds centroidBox;
        for (std::size_t i = begin; i < end; ++i) {
            const Index t = g.bvhTriOrder[i];
            box.merge(triBounds[t]);
            centroidBox.add(triCenters[t]);
        }
        g.bvh[nodeIdx].box = box;
        const std::size_t count = end - begin;
        if (count <= 8) {
            g.bvh[nodeIdx].left = Index(begin);
            g.bvh[nodeIdx].count = Index(count);
            return;
        }
        const Vec ext = centroidBox.radius();
        int axis = 0;
        if (ext[1] > ext[0] && ext[1] >= ext[2]) axis = 1;
        else if (ext[2] > ext[0] && ext[2] > ext[1]) axis = 2;
        std::size_t mid = begin + count / 2;
        if (ext[axis] > 1e-9) {
            const double split = centroidBox.center()[axis];
            auto it = std::partition(g.bvhTriOrder.begin() + std::ptrdiff_t(begin),
                                     g.bvhTriOrder.begin() + std::ptrdiff_t(end),
                                     [&](Index t) { return triCenters[t][axis] < split; });
            const std::size_t partIdx = std::size_t(it - g.bvhTriOrder.begin());
            if (partIdx > begin && partIdx < end) mid = partIdx;
            else std::nth_element(g.bvhTriOrder.begin() + std::ptrdiff_t(begin),
                                  g.bvhTriOrder.begin() + std::ptrdiff_t(mid),
                                  g.bvhTriOrder.begin() + std::ptrdiff_t(end),
                                  [&](Index a, Index b) { return triCenters[a][axis] < triCenters[b][axis]; });
        }
        const Index leftChild = Index(g.bvh.size());
        g.bvh.push_back({});
        g.bvh.push_back({});
        g.bvh[nodeIdx].left = leftChild;
        g.bvh[nodeIdx].count = 0;
        self(self, leftChild, begin, mid);
        self(self, leftChild + 1, mid, end);
    };
    build(build, 0, 0, triCount);
}

struct Part {
    int id;
    std::shared_ptr<const Geometry> cad;
    Geometry mesh;
    QVector3D origin; // shared rest-space origin for CAD and FEM rendering
    bool deformed = false;
    bool suppressed = false;
    int transparencyPercent = 0;
    QColor displayColor;

    bool usesMesh(OccViewport::DisplayMode mode) const
    {
        return (deformed || mode == OccViewport::DisplayMode::Mesh) && !mesh.triangles.empty();
    }

    float opacity(OccViewport::DisplayMode mode) const
    {
        return usesMesh(mode) ? 1.0f : (100 - transparencyPercent) / 100.0f;
    }
};

struct PickCurve {
    TopoDS_Edge edge;
    std::vector<Vec> points;
    std::vector<double> parameters;
    Bounds bounds;
};

struct CadPickGeometry {
    TopoDS_Shape shape;
    bool tessellatedDisplayOnly = false;
    std::shared_ptr<const Geometry> display;

    mutable std::mutex cacheMutex;
    mutable bool shapesReady = false;
    mutable bool verticesReady = false;
    mutable bool curvesReady = false;
    mutable bool faceMeshReady = false;

    mutable TopTools_IndexedMapOfShape faces, edges, vertices, solids;
    mutable std::vector<int> faceSolids;
    mutable std::vector<Vec> vertexPoints;
    mutable std::vector<PickCurve> curves;
    mutable Geometry faceTriMesh;
    mutable std::vector<int> triFaceIds;

    mutable std::optional<Vec> cachedPartCenter;
    mutable std::vector<std::optional<Vec>> cachedFaceCenters;
    mutable std::vector<std::optional<Vec>> cachedSolidCenters;
    mutable std::vector<std::unique_ptr<IntCurvesFace_ShapeIntersector>> cachedFaceIntersectors;

    void ensureShapes() const;
    void ensureVertices() const;
    void ensureCurves() const;
    void ensureFaceTriMesh() const;
    Vec partCenter() const;
    Vec faceCenter(int faceId) const;
    Vec solidCenter(int solidId) const;
    IntCurvesFace_ShapeIntersector* faceIntersector(int faceId) const;
};

struct PickGeometry {
    std::shared_ptr<const CadPickGeometry> cad;
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
    geometry.meshEdges.clear();
    geometry.meshEdges.reserve(mesh.surface.size() * 3 + mesh.tetrahedra.size() * 6);
    const auto add = [&](Index a, Index b) {
        if (a < mesh.nodes.size() && b < mesh.nodes.size() && a != b)
            geometry.meshEdges.push_back({std::min(a, b), std::max(a, b)});
    };
    for (const auto& face : mesh.surface) for (int j = 0; j < 3; ++j) add(face[j], face[(j + 1) % 3]);
    for (const auto& cell : mesh.tetrahedra) for (int j = 0; j < 4; ++j) for (int k = j + 1; k < 4; ++k) add(cell[j], cell[k]);
    std::sort(geometry.meshEdges.begin(), geometry.meshEdges.end());
    geometry.meshEdges.erase(std::unique(geometry.meshEdges.begin(), geometry.meshEdges.end()), geometry.meshEdges.end());
}

Vec vec(const gp_Pnt& p) { return {p.X(), p.Y(), p.Z()}; }

void CadPickGeometry::ensureShapes() const
{
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (shapesReady) return;
    shapesReady = true;
    if (shape.IsNull()) return;
    TopExp::MapShapes(shape, TopAbs_FACE, faces);
    TopExp::MapShapes(shape, TopAbs_EDGE, edges);
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertices);
    TopExp::MapShapes(shape, TopAbs_SOLID, solids);
    faceSolids.assign(std::size_t(faces.Extent()) + 1, 0);
    for (int solid = 1; solid <= solids.Extent(); ++solid) {
        TopTools_IndexedMapOfShape solidFaces;
        TopExp::MapShapes(solids(solid), TopAbs_FACE, solidFaces);
        for (int f = 1; f <= solidFaces.Extent(); ++f) {
            const int id = faces.FindIndex(solidFaces(f));
            if (id > 0) {
                auto& owner = faceSolids[std::size_t(id)];
                owner = owner == 0 ? solid : (owner == solid ? solid : -1);
            }
        }
    }
    cachedFaceCenters.resize(std::size_t(faces.Extent()) + 1);
    cachedSolidCenters.resize(std::size_t(solids.Extent()) + 1);
    cachedFaceIntersectors.resize(std::size_t(faces.Extent()) + 1);
}

void CadPickGeometry::ensureVertices() const
{
    ensureShapes();
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (verticesReady) return;
    verticesReady = true;
    vertexPoints.resize(std::size_t(vertices.Extent()));
    for (int i = 1; i <= vertices.Extent(); ++i) {
        try {
            vertexPoints[std::size_t(i - 1)] = vec(BRep_Tool::Pnt(TopoDS::Vertex(vertices(i))));
        } catch (const Standard_Failure&) {
            vertexPoints[std::size_t(i - 1)] = {std::numeric_limits<double>::quiet_NaN(), 0.0, 0.0};
        }
    }
}

void CadPickGeometry::ensureCurves() const
{
    ensureShapes();
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (curvesReady) return;
    curvesReady = true;
    curves.resize(std::size_t(edges.Extent()));
    for (int i = 1; i <= edges.Extent(); ++i) {
        auto& out = curves[std::size_t(i - 1)];
        out.edge = TopoDS::Edge(edges(i));
        try {
            if (BRep_Tool::Degenerated(out.edge)) continue;
            BRepAdaptor_Curve curve(out.edge);
            const double first = curve.FirstParameter(), last = curve.LastParameter();
            if (!std::isfinite(first) || !std::isfinite(last) || std::abs(first) > 1e50 || std::abs(last) > 1e50) continue;
            if (curve.GetType() == GeomAbs_Line) {
                for (double param : {first, last}) {
                    const Vec pt = vec(curve.Value(param));
                    out.points.push_back(pt);
                    out.parameters.push_back(param);
                    out.bounds.add(pt);
                }
                continue;
            }
            const Vec p0 = vec(curve.Value(first)), p1 = vec(curve.Value(last)), pm = vec(curve.Value((first + last) * 0.5));
            const double chordEst = std::sqrt(dot(subtract(pm, p0), subtract(pm, p0)))
                                  + std::sqrt(dot(subtract(p1, pm), subtract(p1, pm)));
            const double deflection = std::max(1e-3, chordEst * 2e-3);
            GCPnts_QuasiUniformDeflection samples(curve, deflection);
            if (samples.IsDone() && samples.NbPoints() >= 2 && samples.NbPoints() <= 512) {
                out.points.reserve(std::size_t(samples.NbPoints()));
                out.parameters.reserve(std::size_t(samples.NbPoints()));
                for (int j = 1; j <= samples.NbPoints(); ++j) {
                    const Vec pt = vec(samples.Value(j));
                    out.points.push_back(pt);
                    out.parameters.push_back(samples.Parameter(j));
                    out.bounds.add(pt);
                }
            } else {
                constexpr int steps = 48;
                out.points.reserve(steps + 1);
                out.parameters.reserve(steps + 1);
                for (int j = 0; j <= steps; ++j) {
                    const double parameter = first + (last - first) * j / double(steps);
                    const Vec pt = vec(curve.Value(parameter));
                    out.points.push_back(pt);
                    out.parameters.push_back(parameter);
                    out.bounds.add(pt);
                }
            }
        } catch (const Standard_Failure&) {
            out.points.clear();
            out.parameters.clear();
            out.bounds = {};
        }
    }
}

void CadPickGeometry::ensureFaceTriMesh() const
{
    ensureShapes();
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (faceMeshReady) return;
    faceMeshReady = true;
    for (int faceId = 1; faceId <= faces.Extent(); ++faceId) {
        const auto face = TopoDS::Face(faces(faceId));
        TopLoc_Location location;
        const auto mesh = BRep_Tool::Triangulation(face, location);
        if (mesh.IsNull() || mesh->NbNodes() < 3 || mesh->NbTriangles() < 1) continue;
        const gp_Trsf trsf = location.Transformation();
        const bool reverse = (face.Orientation() == TopAbs_REVERSED) != trsf.IsNegative();
        const Index base = Index(faceMeshReady ? faceTriMesh.vertices.size() : 0);
        for (int n = 1; n <= mesh->NbNodes(); ++n) {
            const Vec p = vec(mesh->Node(n).Transformed(trsf));
            faceTriMesh.vertices.push_back({{float(p[0]), float(p[1]), float(p[2])}});
            faceTriMesh.bounds.add(p);
        }
        for (int t = 1; t <= mesh->NbTriangles(); ++t) {
            int a = 0, b = 0, c = 0;
            mesh->Triangle(t).Get(a, b, c);
            if (a < 1 || b < 1 || c < 1 || a > mesh->NbNodes() || b > mesh->NbNodes() || c > mesh->NbNodes()) continue;
            if (reverse) std::swap(b, c);
            faceTriMesh.triangles.insert(faceTriMesh.triangles.end(),
                {base + Index(a - 1), base + Index(b - 1), base + Index(c - 1)});
            triFaceIds.push_back(faceId);
        }
    }
    buildGeometryBvh(faceTriMesh);
}

Vec CadPickGeometry::partCenter() const
{
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (cachedPartCenter) return *cachedPartCenter;
    }
    Vec center = display && display->bounds.valid ? display->bounds.center() : Vec{0.0, 0.0, 0.0};
    if (tessellatedDisplayOnly && display && display->triangles.size() >= 3) {
        double areaSum = 0.0;
        Vec weighted{};
        for (std::size_t i = 0; i + 2 < display->triangles.size(); i += 3) {
            const Vec a = display->position(display->triangles[i]);
            const Vec b = display->position(display->triangles[i + 1]);
            const Vec c = display->position(display->triangles[i + 2]);
            const Vec cr = cross(subtract(b, a), subtract(c, a));
            const double area2 = std::sqrt(dot(cr, cr));
            if (!(area2 > 0.0)) continue;
            areaSum += area2;
            for (int k = 0; k < 3; ++k) weighted[k] += area2 * (a[k] + b[k] + c[k]) / 3.0;
        }
        if (areaSum > 0.0) center = {weighted[0] / areaSum, weighted[1] / areaSum, weighted[2] / areaSum};
    } else if (!shape.IsNull()) {
        try {
            GProp_GProps props;
            BRepGProp::VolumeProperties(shape, props);
            if (std::abs(props.Mass()) < 1e-14) BRepGProp::SurfaceProperties(shape, props);
            if (std::abs(props.Mass()) < 1e-14) BRepGProp::LinearProperties(shape, props);
            if (std::abs(props.Mass()) > 1e-14) center = vec(props.CentreOfMass());
        } catch (const Standard_Failure&) {}
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    cachedPartCenter = center;
    return center;
}

Vec CadPickGeometry::faceCenter(int faceId) const
{
    ensureShapes();
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (faceId < 1 || faceId > faces.Extent()) return {};
        if (cachedFaceCenters[std::size_t(faceId)]) return *cachedFaceCenters[std::size_t(faceId)];
    }
    Vec center{};
    const auto face = TopoDS::Face(faces(faceId));
    bool ok = false;
    if (!tessellatedDisplayOnly) {
        try {
            GProp_GProps props;
            BRepGProp::SurfaceProperties(face, props);
            if (std::abs(props.Mass()) > 1e-14) {
                center = vec(props.CentreOfMass());
                ok = true;
            }
        } catch (const Standard_Failure&) {}
    }
    if (!ok) {
        TopLoc_Location loc;
        const auto mesh = BRep_Tool::Triangulation(face, loc);
        if (!mesh.IsNull() && mesh->NbTriangles() >= 1) {
            const gp_Trsf trsf = loc.Transformation();
            double areaSum = 0.0;
            Vec weighted{};
            for (int t = 1; t <= mesh->NbTriangles(); ++t) {
                int ia = 0, ib = 0, ic = 0;
                mesh->Triangle(t).Get(ia, ib, ic);
                const Vec a = vec(mesh->Node(ia).Transformed(trsf));
                const Vec b = vec(mesh->Node(ib).Transformed(trsf));
                const Vec c = vec(mesh->Node(ic).Transformed(trsf));
                const Vec cr = cross(subtract(b, a), subtract(c, a));
                const double area2 = std::sqrt(dot(cr, cr));
                if (!(area2 > 0.0)) continue;
                areaSum += area2;
                for (int k = 0; k < 3; ++k) weighted[k] += area2 * (a[k] + b[k] + c[k]) / 3.0;
            }
            if (areaSum > 0.0) center = {weighted[0] / areaSum, weighted[1] / areaSum, weighted[2] / areaSum};
        }
    }
    std::lock_guard<std::mutex> lock(cacheMutex);
    cachedFaceCenters[std::size_t(faceId)] = center;
    return center;
}

Vec CadPickGeometry::solidCenter(int solidId) const
{
    ensureShapes();
    {
        std::lock_guard<std::mutex> lock(cacheMutex);
        if (solidId < 1 || solidId > solids.Extent()) return {};
        if (cachedSolidCenters[std::size_t(solidId)]) return *cachedSolidCenters[std::size_t(solidId)];
    }
    Vec center{};
    try {
        GProp_GProps props;
        BRepGProp::VolumeProperties(solids(solidId), props);
        if (std::abs(props.Mass()) > 1e-14) center = vec(props.CentreOfMass());
    } catch (const Standard_Failure&) {}
    std::lock_guard<std::mutex> lock(cacheMutex);
    cachedSolidCenters[std::size_t(solidId)] = center;
    return center;
}

IntCurvesFace_ShapeIntersector* CadPickGeometry::faceIntersector(int faceId) const
{
    ensureShapes();
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (faceId < 1 || faceId > faces.Extent()) return nullptr;
    auto& slot = cachedFaceIntersectors[std::size_t(faceId)];
    if (!slot) {
        slot = std::make_unique<IntCurvesFace_ShapeIntersector>();
        slot->Load(faces(faceId), 1e-7);
    }
    return slot.get();
}

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
    const auto& cad = *geometry.cad;
    if (pick.kind == PickKind::Part) return cad.shape.IsNull() ? nullptr : &cad.shape;
    cad.ensureShapes();
    const TopTools_IndexedMapOfShape* map=nullptr;
    switch(pick.kind) {
    case PickKind::Vertex:map=&cad.vertices;break;
    case PickKind::Edge:map=&cad.edges;break;
    case PickKind::Face:map=&cad.faces;break;
    case PickKind::Solid:map=&cad.solids;break;
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
    case PickKind::Part:return !geometry.cad->shape.IsNull();
    case PickKind::Vertex:geometry.cad->ensureShapes();return inRange(geometry.cad->vertices.Extent());
    case PickKind::Edge:geometry.cad->ensureShapes();return inRange(geometry.cad->edges.Extent());
    case PickKind::Face:geometry.cad->ensureShapes();return inRange(geometry.cad->faces.Extent());
    case PickKind::Solid:geometry.cad->ensureShapes();return inRange(geometry.cad->solids.Extent());
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

std::shared_ptr<CadPickGeometry> makeCadPickGeometry(const CadPart& part, const std::shared_ptr<const Geometry>& display)
{
    auto result = std::make_shared<CadPickGeometry>();
    result->shape = part.shape;
    result->tessellatedDisplayOnly = part.tessellatedDisplayOnly;
    result->display = display;
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
    if (fem && result.edges.empty()) deriveEdges(result);
    buildGeometryBvh(result);
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
bool intersectsBoundsRange(const Ray& ray, const Bounds& bounds, double limit, double padding, double& enterOut)
{
    if (!bounds.valid) return false;
    double enter = 0.0, leave = limit;
    for (int i = 0; i < 3; ++i) {
        const double lo = bounds.min[i] - padding, hi = bounds.max[i] + padding;
        if (std::abs(ray.direction[i]) < 1e-15) {
            if (ray.origin[i] < lo || ray.origin[i] > hi) return false;
        } else {
            double a = (lo - ray.origin[i]) / ray.direction[i];
            double b = (hi - ray.origin[i]) / ray.direction[i];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a);
            leave = std::min(leave, b);
            if (enter > leave) return false;
        }
    }
    enterOut = enter;
    return true;
}

bool intersectsBounds(const Ray& ray, const Bounds& bounds, double limit, double padding)
{
    double enter = 0.0;
    return intersectsBoundsRange(ray, bounds, limit, padding, enter);
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

bool raycastGeometry(const Ray& ray, const Geometry& geometry, double& maxDist, double padding,
                     Index* outTriIndex = nullptr)
{
    double enterRoot = 0.0;
    if (geometry.triangles.size() < 3 || !intersectsBoundsRange(ray, geometry.bounds, maxDist, padding, enterRoot))
        return false;
    bool hitAny = false;
    if (geometry.bvh.empty()) {
        const std::size_t triCount = geometry.triangles.size() / 3;
        for (std::size_t t = 0; t < triCount; ++t) {
            double dist = 0.0;
            if (intersectsTriangle(ray, geometry.position(geometry.triangles[3 * t]),
                                   geometry.position(geometry.triangles[3 * t + 1]),
                                   geometry.position(geometry.triangles[3 * t + 2]), dist) && dist < maxDist) {
                maxDist = dist;
                hitAny = true;
                if (outTriIndex) *outTriIndex = Index(t);
            }
        }
        return hitAny;
    }
    Index stack[64];
    int sp = 0;
    stack[sp++] = 0;
    while (sp > 0) {
        const BvhNode& node = geometry.bvh[stack[--sp]];
        double nodeEnter = 0.0;
        if (!intersectsBoundsRange(ray, node.box, maxDist, padding, nodeEnter)) continue;
        if (node.count > 0) {
            for (Index k = 0; k < node.count; ++k) {
                const Index t = geometry.bvhTriOrder[node.left + k];
                double dist = 0.0;
                if (intersectsTriangle(ray, geometry.position(geometry.triangles[3 * t]),
                                       geometry.position(geometry.triangles[3 * t + 1]),
                                       geometry.position(geometry.triangles[3 * t + 2]), dist) && dist < maxDist) {
                    maxDist = dist;
                    hitAny = true;
                    if (outTriIndex) *outTriIndex = t;
                }
            }
        } else {
            const Index lc = node.left, rc = node.left + 1;
            double enterL = 0.0, enterR = 0.0;
            const bool hitL = intersectsBoundsRange(ray, geometry.bvh[lc].box, maxDist, padding, enterL);
            const bool hitR = intersectsBoundsRange(ray, geometry.bvh[rc].box, maxDist, padding, enterR);
            if (hitL && hitR && sp + 2 <= 64) {
                // 先压入较远子节点，保证先弹出并访问较近子节点以尽早收缩 maxDist
                if (enterL <= enterR) { stack[sp++] = rc; stack[sp++] = lc; }
                else { stack[sp++] = lc; stack[sp++] = rc; }
            } else if (hitL && sp < 64) {
                stack[sp++] = lc;
            } else if (hitR && sp < 64) {
                stack[sp++] = rc;
            }
        }
    }
    return hitAny;
}

struct FastProjector {
    Frame frame;
    Vec pivot;
    double unit = 1.0;
    double invUnit = 1.0;
    double halfW = 0.0, halfH = 0.0;
    double panX = 0.0, panY = 0.0;

    FastProjector(const Camera& camera, const QSize& size, const Bounds& bounds)
        : frame(camera.frame(size, bounds)), pivot(vec(camera.pivot)), unit(camera.mmPerPixel(size)),
          invUnit(1.0 / std::max(1e-30, unit)),
          halfW(size.width() * 0.5), halfH(size.height() * 0.5),
          panX(camera.pan.x()), panY(camera.pan.y())
    {}

    struct PartView {
        Vec sx{}, sy{}, dz{};
        double cx = 0.0, cy = 0.0, cz = 0.0;

        QPointF project(const Vec& p) const
        {
            return {cx + dot(sx, p), cy - dot(sy, p)};
        }

        double depth(const Vec& p) const
        {
            return cz - dot(dz, p);
        }

        QRectF screenBounds(const Bounds& b) const
        {
            if (!b.valid) return {};
            const Vec c = b.center(), r = b.radius();
            const double px = cx + dot(sx, c);
            const double py = cy - dot(sy, c);
            const double ex = std::abs(sx[0])*r[0] + std::abs(sx[1])*r[1] + std::abs(sx[2])*r[2];
            const double ey = std::abs(sy[0])*r[0] + std::abs(sy[1])*r[1] + std::abs(sy[2])*r[2];
            return QRectF(px - ex, py - ey, 2.0 * ex, 2.0 * ey);
        }
    };

    PartView forPart(const PartPose* pose) const
    {
        PartView out{};
        if (!pose) {
            for (int k = 0; k < 3; ++k) {
                out.sx[k] = frame.rows[0][k] * invUnit;
                out.sy[k] = frame.rows[1][k] * invUnit;
                out.dz[k] = frame.rows[2][k];
            }
            out.cx = halfW + (panX - dot(frame.rows[0], pivot)) * invUnit;
            out.cy = halfH - (panY - dot(frame.rows[1], pivot)) * invUnit;
            out.cz = frame.nearZ + dot(frame.rows[2], pivot);
            return out;
        }
        const Vec offset = subtract(pose->translation, pivot);
        for (int j = 0; j < 3; ++j) {
            const Vec col{pose->rows[0][j], pose->rows[1][j], pose->rows[2][j]};
            out.sx[j] = dot(frame.rows[0], col) * invUnit;
            out.sy[j] = dot(frame.rows[1], col) * invUnit;
            out.dz[j] = dot(frame.rows[2], col);
        }
        out.cx = halfW + (panX + dot(frame.rows[0], offset)) * invUnit;
        out.cy = halfH - (panY + dot(frame.rows[1], offset)) * invUnit;
        out.cz = frame.nearZ - dot(frame.rows[2], offset);
        return out;
    }
};

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

Geometry cadPickGeometry(const CadPickGeometry& source, const PickResult& pick)
{
    source.ensureShapes();
    if (pick.kind == PickKind::Edge || pick.kind == PickKind::Face || pick.kind == PickKind::Solid)
        source.ensureCurves();
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
const QVector3D viewportBackground = rgb(0xE5, 0xEC, 0xF0);

} // namespace

class PreparedCadScene {
public:
    struct PartData {
        std::shared_ptr<const Geometry> display;
        std::shared_ptr<const CadPickGeometry> picks;
        int transparencyPercent;
        QColor displayColor;
    };

    std::vector<int> partIds;
    std::map<int, PartData> parts;
};

struct OccViewport::State {
    std::vector<Part> parts;
    std::map<int, PickGeometry> pickGeometry;
    std::shared_ptr<const PreparedCadScene> cadScene;
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

    bool editsNodeSelection() const { return pickActive && pickMultiple && pickKind == PickKind::Node; }

    std::set<int> scalarParts;
    double scalarMinimum = 0, scalarMaximum = 1;
    QString scalarTitle;
    QVector<MotionSymbol> symbols;
    std::map<QString, QVector<MotionSymbol>> auxiliarySymbols;

    Qt::MouseButton button = Qt::NoButton;
    enum class Navigation { None, Select, Orbit, Roll, Pan, Zoom };
    MouseNavigationMode navigationMode = MouseNavigationMode::Classic;
    LeftButtonMode leftButtonMode = LeftButtonMode::OrbitSpace;
    RotationCenterMode rotationCenterMode = RotationCenterMode::ModelCenter;
    bool pickingRotationCenter = false;
    bool hoverPivotValid = false;
    Vec hoverPivotPoint{};
    QTimer* pivotPulseTimer = nullptr;
    Navigation navigation = Navigation::None;
    bool dragging = false;
    QPointF press, last, ballCenter, zoomAnchor;
    QQuaternion pressRotation;
    QVector3D ballStart;
    double ballRadius = 1.0;

    QOpenGLFunctions_3_3_Core* gl = nullptr; // owned by the current widget context
    QMetaObject::Connection destruction;
    std::unique_ptr<QOpenGLShaderProgram> surface, edge, point;
    GLuint emptyVao = 0, pointBatchVao = 0, pointBatchVbo = 0;
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
            if (!part.deformed) bounds.merge(pose ? posedBounds(part.cad->bounds, *pose) : part.cad->bounds);
            bounds.merge(pose ? posedBounds(part.mesh.bounds, *pose) : part.mesh.bounds);
        }
        // 默认旋转中心始终跟随整个可见模型的包围盒中心，并通过 setPivot 补偿平移使画面零跳变
        if (rotationCenterMode == RotationCenterMode::ModelCenter && bounds.valid) {
            camera.setPivot(bounds.center());
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
        } else if (navigation == Navigation::Roll) {
            const double dx0 = last.x() - ballCenter.x();
            const double dy0 = ballCenter.y() - last.y();
            const double dx1 = position.x() - ballCenter.x();
            const double dy1 = ballCenter.y() - position.y();
            double degrees = 0.0;
            if (std::hypot(dx0, dy0) >= 6.0 && std::hypot(dx1, dy1) >= 6.0) {
                constexpr double radToDeg = 180.0 / 3.14159265358979323846;
                degrees = std::remainder((std::atan2(dy1, dx1) - std::atan2(dy0, dx0)) * radToDeg, 360.0);
            } else {
                degrees = (position.x() - last.x()) * 0.4;
            }
            const auto delta = QQuaternion::fromAxisAndAngle({0, 0, 1}, float(degrees));
            camera.rotation = (delta * camera.rotation).normalized();
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
    m_state->pivotPulseTimer = new QTimer(this);
    m_state->pivotPulseTimer->setSingleShot(true);
    connect(m_state->pivotPulseTimer, &QTimer::timeout, this, QOverload<>::of(&OccViewport::update));
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
        updateCamera();
    });
    auto& triadCamera = m_state->triadCamera;
    triadCamera.orientation = [this] { return m_state->camera.rotation.conjugated().normalized(); };
    triadCamera.applyOrientation = [this](const QQuaternion& orientation) {
        if (!m_state->triadHistoryTimer->isActive()) m_state->rememberCamera();
        m_state->camera.rotation = orientation.conjugated().normalized();
        m_state->triadHistoryTimer->start(150);
        m_state->hover = {};
        updateCamera();
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

void OccViewport::setPresentationPeers(const QList<OccViewport*>& viewports)
{
    m_peers.clear();
    for (auto* viewport : viewports) if (viewport && viewport != this) m_peers.append(viewport);
}

void OccViewport::copyCameraFrom(const OccViewport& source)
{
    m_state->camera = source.m_state->camera;
    m_state->rotationCenterMode = source.m_state->rotationCenterMode;
    m_state->hover = {};
    update();
}

void OccViewport::updateCamera()
{
    emit cameraChanged();
    update();
}

void OccViewport::copyPresentationFrom(const OccViewport& source)
{
    auto& next = *m_state;
    const auto& previous = *source.m_state;
    next.parts = previous.parts;
    next.pickGeometry = previous.pickGeometry;
    next.cadScene = previous.cadScene;
    next.partIds = previous.partIds;
    next.poses = previous.poses;
    next.restMeshes = previous.restMeshes;
    next.bounds = previous.bounds;
    next.mode = previous.mode;
    next.selected = previous.selected;
    next.selectionFilter = previous.selectionFilter;
    next.auxiliaryPicks = previous.auxiliaryPicks;
    next.auxiliaryPoints = previous.auxiliaryPoints;
    next.scalarParts = previous.scalarParts;
    next.scalarMinimum = previous.scalarMinimum;
    next.scalarMaximum = previous.scalarMaximum;
    next.scalarTitle = previous.scalarTitle;
    next.symbols = previous.symbols;
    next.auxiliarySymbols = previous.auxiliarySymbols;
    next.rotationCenterMode = previous.rotationCenterMode;
    setLeftButtonMode(previous.leftButtonMode);
    setMouseNavigationMode(previous.navigationMode);
    next.sceneDirty = true;
    update();
}

namespace {

void deriveFeatureEdges(Geometry& geometry, double creaseDegrees)
{
    if (geometry.triangles.size() < 3) return;
    struct PosKey {
        std::int64_t x, y, z;
        bool operator<(const PosKey& o) const
        {
            if (x != o.x) return x < o.x;
            if (y != o.y) return y < o.y;
            return z < o.z;
        }
    };
    std::map<PosKey, Index> uniquePositions;
    std::vector<Index> posId(geometry.vertices.size(), invalidIndex);
    Index nextPos = 0;
    for (std::size_t i = 0; i < geometry.vertices.size(); ++i) {
        const auto& p = geometry.vertices[i].position;
        const PosKey key{
            std::llround(double(p[0]) * 10000.0),
            std::llround(double(p[1]) * 10000.0),
            std::llround(double(p[2]) * 10000.0),
        };
        const auto [it, inserted] = uniquePositions.emplace(key, nextPos);
        if (inserted) ++nextPos;
        posId[i] = it->second;
    }

    const std::size_t triCount = geometry.triangles.size() / 3;
    std::vector<Vec> faceNormals(triCount, {0.0, 0.0, 0.0});
    struct HalfEdge {
        std::uint64_t key;
        std::size_t tri;
        Index va, vb;
        bool operator<(const HalfEdge& o) const
        {
            if (key != o.key) return key < o.key;
            return tri < o.tri;
        }
    };
    std::vector<HalfEdge> halfEdges;
    halfEdges.reserve(geometry.triangles.size());

    for (std::size_t t = 0; t < triCount; ++t) {
        const Index a = geometry.triangles[3 * t];
        const Index b = geometry.triangles[3 * t + 1];
        const Index c = geometry.triangles[3 * t + 2];
        Vec n = cross(subtract(geometry.position(b), geometry.position(a)),
                      subtract(geometry.position(c), geometry.position(a)));
        normalise(n);
        faceNormals[t] = n;
        const Index triVerts[3] = {a, b, c};
        for (int e = 0; e < 3; ++e) {
            const Index u = triVerts[e];
            const Index v = triVerts[(e + 1) % 3];
            const Index pu = posId[u], pv = posId[v];
            if (pu == pv) continue;
            const std::uint64_t lo = std::min(pu, pv);
            const std::uint64_t hi = std::max(pu, pv);
            halfEdges.push_back({(hi << 32) | lo, t, u, v});
        }
    }
    std::sort(halfEdges.begin(), halfEdges.end());

    const double cosCrease = std::cos(creaseDegrees * 3.14159265358979323846 / 180.0);
    for (std::size_t i = 0; i < halfEdges.size();) {
        std::size_t j = i + 1;
        while (j < halfEdges.size() && halfEdges[j].key == halfEdges[i].key) ++j;
        bool sharp = false;
        if (j - i == 1) {
            sharp = true; // 开放边界边
        } else {
            for (std::size_t p = i; p < j && !sharp; ++p) {
                for (std::size_t q = p + 1; q < j; ++q) {
                    if (dot(faceNormals[halfEdges[p].tri], faceNormals[halfEdges[q].tri]) <= cosCrease) {
                        sharp = true;
                        break;
                    }
                }
            }
        }
        if (sharp) {
            geometry.edges.push_back(halfEdges[i].va);
            geometry.edges.push_back(halfEdges[i].vb);
        }
        i = j;
    }
}

PreparedCadScene::PartData prepareCadPart(const CadPart& part)
{
    const auto& cad = part.display;
    auto render = snapshot(cad.vertices.size(), cad.triangles, cad.edges, false,
                           [&](Index i) { return vec(cad.vertices[i]); });
    if (part.tessellatedDisplayOnly && render.edges.empty()) {
        // AP242 纯显示网格不含精确 B-Rep 拓扑边：仅提取折痕角 >= 42° 的真实几何特征棱边与开放边界，
        // 避免把平面与光滑圆柱面内部的离散三角面片边当作 CAD 线框绘制出来。
        deriveFeatureEdges(render, 42.0);
    }
    // CAD 的法线按 OCC 面求取，保留尖锐接缝。有限元仍使用单元平面法线。
    for (std::size_t i = 0; i < render.vertices.size(); ++i) {
        const Index sourceIndex = render.sourceIndices[i];
        if (sourceIndex >= cad.normals.size()) continue;
        Vec normal = vec(cad.normals[sourceIndex]);
        if (!normalise(normal)) continue;
        for (int axis = 0; axis < 3; ++axis) render.vertices[i].normal[axis] = float(normal[axis]);
    }
    auto displayPtr = std::make_shared<Geometry>(std::move(render));
    return {displayPtr, makeCadPickGeometry(part, displayPtr), part.transparencyPercent, part.displayColor};
}

const PreparedCadScene::PartData* findPreparedCadPart(const PreparedCadScene* scene, int id,
                                                      const TopoDS_Shape& shape)
{
    if (!scene) return nullptr;
    const auto found = scene->parts.find(id);
    return found != scene->parts.end() && found->second.picks->shape.IsEqual(shape) ? &found->second : nullptr;
}

} // namespace

std::shared_ptr<const PreparedCadScene> OccViewport::prepareCadScene(const CadDocument& document)
{
    auto scene = std::make_shared<PreparedCadScene>();
    scene->partIds.reserve(document.parts().size());
    for (const auto& part : document.parts()) {
        scene->partIds.push_back(part.id);
        scene->parts.emplace(part.id, prepareCadPart(part));
    }
    return scene;
}

void OccViewport::setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                           const std::set<int>& hidden, const std::shared_ptr<const PreparedCadScene>& prepared,
                           const std::vector<int>& preparedPartIds)
{
    std::map<int, int> originalIds;
    if (prepared && preparedPartIds.size() == prepared->partIds.size()) {
        for (std::size_t i = 0; i < preparedPartIds.size(); ++i)
            originalIds.emplace(preparedPartIds[i], prepared->partIds[i]);
    }
    auto scene = std::make_shared<PreparedCadScene>();
    scene->partIds.reserve(document.parts().size());
    for (const auto& part : document.parts()) {
        scene->partIds.push_back(part.id);
        const auto original = originalIds.find(part.id);
        const auto* data = findPreparedCadPart(prepared.get(), original == originalIds.end() ? part.id : original->second,
                                               part.shape);
        if (!data) data = findPreparedCadPart(m_state->cadScene.get(), part.id, part.shape);
        if (data) scene->parts.emplace(part.id, PreparedCadScene::PartData{
            data->display, data->picks, part.transparencyPercent, part.displayColor});
        else if (!hidden.count(part.id)) scene->parts.emplace(part.id, prepareCadPart(part));
    }
    applyPreparedScene(scene, meshes, hidden);
}

void OccViewport::setPartTransparency(int partId, int percent)
{
    updatePeers([&](OccViewport& peer) { peer.setPartTransparency(partId, percent); });
    const auto part = std::find_if(m_state->parts.begin(), m_state->parts.end(),
        [partId](const auto& item) { return item.id == partId; });
    if (part == m_state->parts.end() || part->transparencyPercent == percent) return;
    part->transparencyPercent = percent;
    update();
}

void OccViewport::applyPreparedScene(const std::shared_ptr<const PreparedCadScene>& scene,
                                     const std::map<int, FemMesh>& meshes, const std::set<int>& hidden)
{
    updatePeers([&](OccViewport& peer) { peer.applyPreparedScene(scene, meshes, hidden); });
    auto& s = *m_state;
    s.cadScene = scene;
    std::vector<Part> parts;
    std::map<int, PickGeometry> pickGeometry;
    parts.reserve(scene->parts.size());
    for (int id : scene->partIds) {
        if (hidden.count(id)) continue;
        const auto& prepared = scene->parts.at(id);
        Part copy{id, prepared.display, {}, {}};
        copy.transparencyPercent = prepared.transparencyPercent;
        copy.displayColor = prepared.displayColor;
        auto& topology = pickGeometry[id];
        topology.cad = prepared.picks;
        const auto found = meshes.find(id);
        if (found != meshes.end()) {
            const auto& fem = found->second;
            copy.mesh = snapshot(fem.nodes.size(), fem.surface, {}, true,
                                 [&](Index i) { return fem.nodes[i]; });
            setMeshTopology(topology, fem);
            topology.nodeIds.reserve(int(fem.nodes.size()));
            for (int i = 0; i < int(fem.nodes.size()); ++i) topology.nodeIds.append(i + 1);
        }
        Bounds rest = copy.cad->bounds;
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
        if(previous==s.pickGeometry.end() || !previous->second.cad->shape.IsEqual(next.cad->shape))continue;
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
    s.partIds = std::set<int>(scene->partIds.cbegin(), scene->partIds.cend());
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
    updatePeers([&](OccViewport& peer) { peer.setPartPoses(poses); });
    auto& s = *m_state;
    s.hover = {}; // preselection is recomputed at the next pointer position
    std::map<int, PartPose> next;
    for (const auto& [id, matrix] : poses) {
        if (!s.partIds.count(id)) continue;
        PartPose pose{};
        if (makePartPose(matrix, pose)) next.emplace_hint(next.end(), id, std::move(pose));
    }
    if (next.empty()) {
        const QScopedValueRollback<bool> updating(m_applyingPeerUpdate, true);
        clearPartPoses();
        return;
    }
    s.poses = std::move(next);
    s.updateBounds();
    update();
}

void OccViewport::clearPartPoses()
{
    updatePeers([](OccViewport& peer) { peer.clearPartPoses(); });
    const QScopedValueRollback<bool> updating(m_applyingPeerUpdate, true);
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
    updatePeers([&](OccViewport& peer) { peer.setDeformedMeshes(meshes, suppressed); });
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
    updatePeers([&](OccViewport& peer) { peer.selectPart(id); });
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

void OccViewport::setLeftButtonMode(LeftButtonMode mode)
{
    if (mode != LeftButtonMode::OrbitSpace && mode != LeftButtonMode::OrbitPlane && mode != LeftButtonMode::Pan) return;
    cancelPickRotationCenter();
    if (m_state->leftButtonMode == mode) return;
    interruptPointerOperation();
    m_state->leftButtonMode = mode;
    setToolTip(mouseNavigationHint());
    update();
}

OccViewport::LeftButtonMode OccViewport::leftButtonMode() const { return m_state->leftButtonMode; }

QString OccViewport::mouseNavigationHint() const
{
    if (m_state->navigationMode == MouseNavigationMode::Nx)
        return tr("NX 导航：中键拖动旋转；Shift+中键平移；Ctrl+中键拖动或滚轮按光标缩放；左键点选。"
                  "双击表面设旋转中心，双击空白或 Ctrl+双击适合全部。多选节点时左键加选、右键减选，拖动贯穿框选。");
    QString leftText = tr("左键拖动空间旋转");
    if (m_state->leftButtonMode == LeftButtonMode::OrbitPlane) leftText = tr("左键拖动平面旋转");
    else if (m_state->leftButtonMode == LeftButtonMode::Pan) leftText = tr("左键拖动平移");
    return tr("经典导航：%1；中键拖动平移；滚轮按光标缩放；左键单击单选。"
              "Ctrl+左键单选加、Ctrl+右键单选减；Shift+左键拖动框选加、Shift+右键拖动框选减。"
              "默认以模型包围盒中心为旋转中心，支持视图旋转中心、模型旋转中心与自定义旋转中心（或双击表面设定，双击空白适合全部）。")
        .arg(leftText);
}

OccViewport::RotationCenterMode OccViewport::rotationCenterMode() const { return m_state->rotationCenterMode; }

std::array<double, 3> OccViewport::rotationCenter() const { return vec(m_state->camera.pivot); }

void OccViewport::pulsePivotIndicator()
{
    if (m_state->pivotPulseTimer) m_state->pivotPulseTimer->start(900);
}

void OccViewport::setRotationCenterToModel()
{
    auto& s = *m_state;
    cancelPickRotationCenter();
    interruptPointerOperation();
    s.rotationCenterMode = RotationCenterMode::ModelCenter;
    if (s.bounds.valid) {
        s.rememberCamera();
        s.camera.setPivot(s.bounds.center());
        s.rememberCamera();
    }
    pulsePivotIndicator();
    updateCamera();
    emit rotationCenterChanged(rotationCenter());
}

void OccViewport::setRotationCenterToView()
{
    auto& s = *m_state;
    cancelPickRotationCenter();
    interruptPointerOperation();
    const QPointF centerPos(width() * 0.5, height() * 0.5);
    Vec target{};
    bool hitSurface = false;
    if (s.bounds.valid && QRectF(rect()).contains(centerPos)) {
        const auto frame = s.camera.frame(size(), s.bounds);
        const double unit = s.camera.mmPerPixel(size());
        const QPointF eye(-s.camera.pan.x(), -s.camera.pan.y());
        const Vec pivot = vec(s.camera.pivot);
        Ray world{};
        for (int k = 0; k < 3; ++k) {
            world.origin[k] = pivot[k] + frame.rows[0][k]*eye.x() + frame.rows[1][k]*eye.y() + frame.rows[2][k]*frame.nearZ;
            world.direction[k] = -frame.rows[2][k];
        }
        double nearest = frame.nearZ - frame.farZ;
        if (s.mode != DisplayMode::Wireframe) {
            for (const auto& part : s.parts) {
                if (part.suppressed) continue;
                const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
                const auto ray = restRay(world, s.poseFor(part.id));
                if (!intersectsBounds(ray, geometry.bounds, nearest, unit)) continue;
                for (std::size_t i = 0; i + 2 < geometry.triangles.size(); i += 3) {
                    double distance = 0.0;
                    if (intersectsTriangle(ray, geometry.position(geometry.triangles[i]),
                            geometry.position(geometry.triangles[i+1]), geometry.position(geometry.triangles[i+2]), distance)
                        && distance < nearest) {
                        nearest = distance;
                        hitSurface = true;
                    }
                }
            }
        }
        if (hitSurface) {
            for (int k = 0; k < 3; ++k) target[k] = world.origin[k] + world.direction[k] * nearest;
        } else {
            const double modelDepth = dot(frame.rows[2], subtract(s.bounds.center(), pivot));
            for (int k = 0; k < 3; ++k) {
                target[k] = pivot[k] + frame.rows[0][k]*eye.x() + frame.rows[1][k]*eye.y() + frame.rows[2][k]*modelDepth;
            }
            hitSurface = true;
        }
    }
    if (!hitSurface) {
        const auto eyeToWorld = s.camera.rotation.conjugated().normalized();
        const QVector3D worldCenter = s.camera.pivot + eyeToWorld.rotatedVector(
            QVector3D(float(-s.camera.pan.x()), float(-s.camera.pan.y()), 0.0f));
        target = vec(worldCenter);
    }
    s.rememberCamera();
    s.camera.setPivot(target);
    s.rotationCenterMode = RotationCenterMode::ViewCenter;
    s.rememberCamera();
    pulsePivotIndicator();
    updateCamera();
    emit rotationCenterChanged(rotationCenter());
}

void OccViewport::setRotationCenter(const std::array<double, 3>& worldPoint)
{
    if (!std::isfinite(worldPoint[0]) || !std::isfinite(worldPoint[1]) || !std::isfinite(worldPoint[2])) return;
    auto& s = *m_state;
    cancelPickRotationCenter();
    interruptPointerOperation();
    s.rememberCamera();
    s.camera.setPivot(worldPoint);
    s.rotationCenterMode = RotationCenterMode::CustomPoint;
    s.rememberCamera();
    pulsePivotIndicator();
    updateCamera();
    emit rotationCenterChanged(rotationCenter());
}

void OccViewport::beginPickRotationCenter()
{
    auto& s = *m_state;
    if (s.pickingRotationCenter) {
        setFocus(Qt::OtherFocusReason);
        setCursor(Qt::CrossCursor);
        update();
        return;
    }
    interruptPointerOperation();
    s.pickingRotationCenter = true;
    s.hoverPivotValid = false;
    setFocus(Qt::OtherFocusReason);
    setCursor(Qt::CrossCursor);
    update();
    emit rotationCenterPickModeChanged(true);
}

void OccViewport::cancelPickRotationCenter()
{
    auto& s = *m_state;
    if (!s.pickingRotationCenter) return;
    s.pickingRotationCenter = false;
    s.hoverPivotValid = false;
    if (s.pickActive) setCursor(Qt::CrossCursor);
    else unsetCursor();
    update();
    emit rotationCenterPickModeChanged(false);
}

bool OccViewport::isPickingRotationCenter() const { return m_state->pickingRotationCenter; }

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

std::optional<PickKind> OccViewport::selectionFilter() const { return m_state->selectionFilter; }

void OccViewport::clearViewportSelection()
{
    if (m_state->viewportSelection.isEmpty()) return;
    m_state->viewportSelection.clear();
    emit viewportSelectionChanged();
    update();
}

void OccViewport::beginPick(PickKind kind, bool multiple, const std::set<int>& allowedParts,
                            const QVector<PickResult>& initial, PartPickResolution partResolution)
{
    auto& s = *m_state;
    s.pickActive = true; s.pickKind = kind; s.pickMultiple = multiple; s.pickParts = allowedParts;
    interruptPointerOperation();
    s.hover = {};
    setPickSelection(initial, partResolution);
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
    updatePeers([&](OccViewport& peer) { peer.setAuxiliaryPickHighlights(key, picks, color); });
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
    updatePeers([&](OccViewport& peer) { peer.clearAuxiliaryPickHighlights(key); });
    m_state->auxiliaryPicks.erase(key); update();
}
void OccViewport::setAuxiliaryPoints(const QString& key, const QVector<QVector3D>& points, const QColor& color,float pointSize)
{
    if(!std::isfinite(pointSize)||pointSize<=0) {clearAuxiliaryPoints(key);return;}
    updatePeers([&](OccViewport& peer) { peer.setAuxiliaryPoints(key, points, color, pointSize); });
    QVector<QVector3D> valid;
    for (const auto& point : points)
        if (std::isfinite(point.x()) && std::isfinite(point.y()) && std::isfinite(point.z())) valid.append(point);
    if (valid.isEmpty()) m_state->auxiliaryPoints.erase(key);
    else m_state->auxiliaryPoints[key] = {valid, color.isValid() ? color : QColor(24,144,255),std::clamp(pointSize,1.f,256.f)};
    update();
}
void OccViewport::clearAuxiliaryPoints(const QString& key)
{
    updatePeers([&](OccViewport& peer) { peer.clearAuxiliaryPoints(key); });
    m_state->auxiliaryPoints.erase(key); update();
}

void OccViewport::setPickSelection(const QVector<PickResult>& selection, PartPickResolution partResolution)
{
    auto& s = *m_state;
    s.picks.clear();
    std::set<std::tuple<int, int, int>> seenPicks;
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
                geometry.cad->ensureVertices();
                pick.position = pick.hitPosition = geometry.cad->vertexPoints[std::size_t(pick.index - 1)];
            } else if (pick.kind == PickKind::Edge) {
                geometry.cad->ensureShapes();
                BRepAdaptor_Curve curve(TopoDS::Edge(geometry.cad->edges(pick.index)));
                pick.position = vec(curve.Value((curve.FirstParameter()+curve.LastParameter())*.5));
                if (curve.GetType()==GeomAbs_Circle) pick.position=vec(curve.Circle().Location());
                else if (curve.GetType()==GeomAbs_Ellipse) pick.position=vec(curve.Ellipse().Location());
            } else if (pick.kind == PickKind::Face) {
                pick.position = geometry.cad->faceCenter(pick.index);
            } else if (pick.kind == PickKind::Solid) {
                pick.position = geometry.cad->solidCenter(pick.index);
            } else if (pick.kind == PickKind::Part && partResolution == PartPickResolution::ExactCenter) {
                pick.position = geometry.cad->partCenter();
            } else if (pick.kind != PickKind::Part) {
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
        if (!seenPicks.emplace(int(pick.kind), pick.partId, pick.index).second) continue;
        s.picks.append(pick);
        if (!s.pickMultiple) break;
    }
    update();
}

void OccViewport::setPickNodes(int partId, const FemMesh& mesh, const QVector<int>& nodeIds)
{
    updatePeers([&](OccViewport& peer) { peer.setPickNodes(partId, mesh, nodeIds); });
    const auto found = m_state->pickGeometry.find(partId);
    if (found == m_state->pickGeometry.end()) return;
    auto& target = found->second;
    const bool sameTopology = target.nodes.size() == mesh.nodes.size()
        && target.meshFaces.size() == mesh.surface.size()
        && target.elements.size() == mesh.tetrahedra.size()
        && target.nodes == mesh.nodes
        && target.meshFaces == mesh.surface
        && target.elements == mesh.tetrahedra;
    if (sameTopology && !nodeIds.isEmpty() && target.nodeIds == nodeIds) return;
    if (sameTopology && nodeIds.isEmpty() && target.nodeIds.size() == int(target.nodes.size())) {
        bool defaultIds = true;
        for (int i = 0; i < target.nodeIds.size(); ++i)
            if (target.nodeIds[i] != i + 1) { defaultIds = false; break; }
        if (defaultIds) return;
    }
    if (!nodeIds.isEmpty()) {
        if (nodeIds.size() != int(mesh.nodes.size())) return;
        std::vector<int> sorted(nodeIds.cbegin(), nodeIds.cend());
        std::sort(sorted.begin(), sorted.end());
        if (sorted.front() <= 0 || std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) return;
    }
    if (!sameTopology) setMeshTopology(target, mesh);
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
    updatePeers([&](OccViewport& peer) { peer.setMeshScalars(values, minimum, maximum, title); });
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

std::optional<std::pair<double, double>> OccViewport::meshScalarRange() const
{
    const auto& s = *m_state;
    if (s.scalarParts.empty()) return std::nullopt;
    return std::make_pair(s.scalarMinimum, s.scalarMaximum);
}

void OccViewport::clearMeshScalars()
{
    updatePeers([](OccViewport& peer) { peer.clearMeshScalars(); });
    m_state->scalarParts.clear(); m_state->scalarTitle.clear(); update();
}

void OccViewport::setMotionSymbols(const QVector<MotionSymbol>& symbols)
{
    updatePeers([&](OccViewport& peer) { peer.setMotionSymbols(symbols); });
    m_state->symbols = symbols;
    update();
}

void OccViewport::setAuxiliaryMotionSymbols(const QString& key,const QVector<MotionSymbol>& symbols)
{
    updatePeers([&](OccViewport& peer) { peer.setAuxiliaryMotionSymbols(key, symbols); });
    if(symbols.isEmpty())m_state->auxiliarySymbols.erase(key);
    else m_state->auxiliarySymbols[key]=symbols;
    update();
}
void OccViewport::clearAuxiliaryMotionSymbols(const QString& key)
{
    updatePeers([&](OccViewport& peer) { peer.clearAuxiliaryMotionSymbols(key); });
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
    const FastProjector proj(s.camera, size(), s.bounds);
    const auto& frame = proj.frame;
    const double unit = proj.unit;
    const QPointF eye((position.x() - proj.halfW) * unit - proj.panX,
                      (proj.halfH - position.y()) * unit - proj.panY);
    const Vec& pivot = proj.pivot;
    Ray world;
    for (int i = 0; i < 3; ++i) {
        world.origin[i] = pivot[i] + frame.rows[0][i] * eye.x() + frame.rows[1][i] * eye.y() + frame.rows[2][i] * frame.nearZ;
        world.direction[i] = -frame.rows[2][i];
    }
    double visibleDepth = frame.nearZ - frame.farZ;
    if (s.mode != DisplayMode::Wireframe) {
        for (const auto& part : s.parts) {
            if (part.suppressed) continue;
            const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
            const auto ray = restRay(world, s.poseFor(part.id));
            raycastGeometry(ray, geometry, visibleDepth, unit * 0.01);
        }
    }
    double bestOffset = 8.0 * 8.0, bestDepth = frame.nearZ - frame.farZ;
    std::map<int, const Part*> partLookup;
    for (const auto& p : s.parts) partLookup.emplace(p.id, &p);

    for (const auto& [partId, geometry] : s.pickGeometry) {
        if (!allowedParts.empty() && !allowedParts.count(partId)) continue;
        const auto partIt = partLookup.find(partId);
        if (partIt == partLookup.end()) continue;
        const Part& part = *partIt->second;
        if (part.suppressed || part.deformed) continue;
        const auto* pose = s.poseFor(partId);
        const Ray localRay = restRay(world, pose);
        if (kind < PickKind::Node && !intersectsBounds(localRay, part.cad->bounds, visibleDepth + 8.0 * unit, 8.0 * unit))
            continue;
        const auto pv = proj.forPart(pose);
        const auto pointHit = [&](const Vec& point, PickResult candidate) {
            if (!std::isfinite(point[0])) return;
            const double depth = pv.depth(point);
            if (depth < 0.0 || depth > visibleDepth + 8.0 * unit) return;
            const QPointF offset = position - pv.project(point);
            const double offset2 = QPointF::dotProduct(offset, offset);
            if (offset2 > bestOffset || (std::abs(offset2 - bestOffset) < 1e-8 && depth >= bestDepth)) return;
            candidate.position = candidate.hitPosition = point;
            best = candidate;
            bestOffset = offset2;
            bestDepth = depth;
        };
        PickResult candidate;
        candidate.kind = kind;
        candidate.partId = partId;
        try {
            if (kind == PickKind::Vertex) {
                geometry.cad->ensureVertices();
                for (int index = 1; index <= int(geometry.cad->vertexPoints.size()); ++index) {
                    candidate.index = index;
                    pointHit(geometry.cad->vertexPoints[std::size_t(index - 1)], candidate);
                }
            } else if (kind == PickKind::Node) {
                for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                    candidate.index = index;
                    candidate.nodeId = geometry.nodeIds.value(index, index + 1);
                    pointHit(geometry.nodes[std::size_t(index)], candidate);
                }
            } else if (kind == PickKind::MeshEdge) {
                for (std::size_t index = 0; index < geometry.meshEdges.size(); ++index) {
                    const auto& edge = geometry.meshEdges[index];
                    const auto& pa = geometry.nodes[edge[0]];
                    const auto& pb = geometry.nodes[edge[1]];
                    const QPointF a = pv.project(pa), b = pv.project(pb), delta = b - a;
                    const double length2 = QPointF::dotProduct(delta, delta);
                    if (length2 < 1e-20) continue;
                    const double t = std::clamp(QPointF::dotProduct(position - a, delta) / length2, 0.0, 1.0);
                    const QPointF offset = position - a - delta * t;
                    const double offset2 = QPointF::dotProduct(offset, offset);
                    if (offset2 > bestOffset) continue;
                    const Vec point{pa[0]*(1.0 - t) + pb[0]*t, pa[1]*(1.0 - t) + pb[1]*t, pa[2]*(1.0 - t) + pb[2]*t};
                    const double depth = pv.depth(point);
                    if (depth < 0.0 || depth > visibleDepth + 8.0 * unit ||
                        (std::abs(offset2 - bestOffset) < 1e-8 && depth >= bestDepth)) continue;
                    candidate.index = int(index + 1);
                    candidate.hitPosition = point;
                    for (int k = 0; k < 3; ++k) candidate.position[k] = (pa[k] + pb[k]) * 0.5;
                    candidate.direction = subtract(pb, pa);
                    candidate.hasDirection = normalise(candidate.direction);
                    candidate.nodeIds = {geometry.nodeIds.value(edge[0], int(edge[0] + 1)),
                                         geometry.nodeIds.value(edge[1], int(edge[1] + 1))};
                    best = candidate;
                    bestOffset = offset2;
                    bestDepth = depth;
                }
            } else if (kind == PickKind::MeshFace || kind == PickKind::Element) {
                const bool volume = kind == PickKind::Element && !geometry.elements.empty();
                const std::size_t count = volume ? geometry.elements.size() : geometry.meshFaces.size();
                constexpr int tetFaces[4][3] = {{0,2,1},{0,1,3},{0,3,2},{1,2,3}};
                for (std::size_t index = 0; index < count; ++index) {
                    Bounds bounds;
                    if (volume) {
                        const auto& cell = geometry.elements[index];
                        if (std::any_of(cell.begin(), cell.end(), [&](Index n) { return n >= geometry.nodes.size(); })) continue;
                        for (auto n : cell) bounds.add(geometry.nodes[n]);
                    } else {
                        const auto& f = geometry.meshFaces[index];
                        if (std::any_of(f.begin(), f.end(), [&](Index n) { return n >= geometry.nodes.size(); })) continue;
                        for (auto n : f) bounds.add(geometry.nodes[n]);
                    }
                    if (!intersectsBounds(localRay, bounds, std::min(bestDepth, visibleDepth + 8.0 * unit), unit * 0.01)) continue;
                    double depth = bestDepth;
                    bool found = false;
                    Vec normal{};
                    for (int face = 0; face < (volume ? 4 : 1); ++face) {
                        const Index ia = volume ? geometry.elements[index][tetFaces[face][0]] : geometry.meshFaces[index][0];
                        const Index ib = volume ? geometry.elements[index][tetFaces[face][1]] : geometry.meshFaces[index][1];
                        const Index ic = volume ? geometry.elements[index][tetFaces[face][2]] : geometry.meshFaces[index][2];
                        const auto& a = geometry.nodes[ia];
                        const auto& b = geometry.nodes[ib];
                        const auto& c = geometry.nodes[ic];
                        double hit = 0.0;
                        if (intersectsTriangle(localRay, a, b, c, hit) && hit < depth && hit <= visibleDepth + 8.0 * unit) {
                            depth = hit;
                            found = true;
                            normal = cross(subtract(b, a), subtract(c, a));
                        }
                    }
                    if (!found) continue;
                    candidate.index = int(index + 1);
                    candidate.position = {};
                    candidate.nodeIds.clear();
                    const int nCount = volume ? 4 : 3;
                    for (int kNode = 0; kNode < nCount; ++kNode) {
                        const Index node = volume ? geometry.elements[index][kNode] : geometry.meshFaces[index][kNode];
                        for (int k = 0; k < 3; ++k) candidate.position[k] += geometry.nodes[node][k] / double(nCount);
                        candidate.nodeIds.append(geometry.nodeIds.value(node, int(node + 1)));
                    }
                    for (int k = 0; k < 3; ++k) candidate.hitPosition[k] = localRay.origin[k] + localRay.direction[k] * depth;
                    candidate.direction = normal;
                    candidate.hasDirection = kind == PickKind::MeshFace && normalise(candidate.direction);
                    best = candidate;
                    bestDepth = depth;
                }
            } else if (kind == PickKind::Edge) {
                geometry.cad->ensureCurves();
                const int curveCount = int(geometry.cad->curves.size());
                int hitCurveIndex = -1;
                double hitParam = 0.0;
                for (int index = 0; index < curveCount; ++index) {
                    const auto& curve = geometry.cad->curves[std::size_t(index)];
                    if (curve.points.size() < 2 || !intersectsBounds(localRay, curve.bounds, visibleDepth + 8.0 * unit, 8.0 * unit))
                        continue;
                    QPointF prevScreen = pv.project(curve.points[0]);
                    for (std::size_t j = 1; j < curve.points.size(); ++j) {
                        const QPointF curScreen = pv.project(curve.points[j]);
                        const QPointF delta = curScreen - prevScreen;
                        const double length2 = QPointF::dotProduct(delta, delta);
                        if (length2 >= 1e-20) {
                            const double t = std::clamp(QPointF::dotProduct(position - prevScreen, delta) / length2, 0.0, 1.0);
                            const QPointF offset = position - prevScreen - delta * t;
                            const double offset2 = QPointF::dotProduct(offset, offset);
                            if (offset2 <= bestOffset) {
                                const Vec approx{
                                    curve.points[j - 1][0] * (1.0 - t) + curve.points[j][0] * t,
                                    curve.points[j - 1][1] * (1.0 - t) + curve.points[j][1] * t,
                                    curve.points[j - 1][2] * (1.0 - t) + curve.points[j][2] * t,
                                };
                                const double depth = pv.depth(approx);
                                if (depth >= 0.0 && depth <= visibleDepth + 8.0 * unit &&
                                    !(std::abs(offset2 - bestOffset) < 1e-8 && depth >= bestDepth)) {
                                    bestOffset = offset2;
                                    bestDepth = depth;
                                    hitCurveIndex = index;
                                    hitParam = curve.parameters[j - 1] * (1.0 - t) + curve.parameters[j] * t;
                                }
                            }
                        }
                        prevScreen = curScreen;
                    }
                }
                if (hitCurveIndex >= 0) {
                    const auto& curve = geometry.cad->curves[std::size_t(hitCurveIndex)];
                    BRepAdaptor_Curve exact(curve.edge);
                    gp_Pnt hit;
                    gp_Vec tangent;
                    exact.D1(hitParam, hit, tangent);
                    candidate.index = hitCurveIndex + 1;
                    candidate.hitPosition = vec(hit);
                    candidate.position = vec(exact.Value((exact.FirstParameter() + exact.LastParameter()) * 0.5));
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
                    best = candidate;
                }
            } else if (kind == PickKind::Part) {
                double hitDist = std::min(bestDepth, visibleDepth + 8.0 * unit);
                if (raycastGeometry(localRay, *part.cad, hitDist, unit * 0.01) && hitDist < bestDepth) {
                    candidate.index = -1;
                    candidate.hitPosition = {
                        localRay.origin[0] + localRay.direction[0] * hitDist,
                        localRay.origin[1] + localRay.direction[1] * hitDist,
                        localRay.origin[2] + localRay.direction[2] * hitDist,
                    };
                    candidate.position = geometry.cad->partCenter();
                    best = candidate;
                    bestDepth = hitDist;
                }
            } else {
                // Face 或 Solid：先用部件三角面 BVH 在微秒级定位命中的具体 Face，仅对该单面做精确解析求交
                geometry.cad->ensureFaceTriMesh();
                double triDist = std::min(bestDepth, visibleDepth + 8.0 * unit);
                Index hitTri = invalidIndex;
                if (!raycastGeometry(localRay, geometry.cad->faceTriMesh, triDist, unit * 0.01, &hitTri) ||
                    hitTri >= geometry.cad->triFaceIds.size())
                    continue;
                const int faceId = geometry.cad->triFaceIds[hitTri];
                if (faceId < 1 || faceId > geometry.cad->faces.Extent()) continue;
                if (kind == PickKind::Solid) {
                    const int solidId = geometry.cad->faceSolids[std::size_t(faceId)];
                    if (solidId < 1) continue;
                    candidate.index = solidId;
                    candidate.hitPosition = {
                        localRay.origin[0] + localRay.direction[0] * triDist,
                        localRay.origin[1] + localRay.direction[1] * triDist,
                        localRay.origin[2] + localRay.direction[2] * triDist,
                    };
                    candidate.position = geometry.cad->solidCenter(solidId);
                    best = candidate;
                    bestDepth = triDist;
                    continue;
                }
                candidate.index = faceId;
                candidate.position = geometry.cad->faceCenter(faceId);
                candidate.hitPosition = {
                    localRay.origin[0] + localRay.direction[0] * triDist,
                    localRay.origin[1] + localRay.direction[1] * triDist,
                    localRay.origin[2] + localRay.direction[2] * triDist,
                };
                double exactDepth = triDist;
                const auto face = TopoDS::Face(geometry.cad->faces(faceId));
                if (!geometry.cad->tessellatedDisplayOnly && !BRep_Tool::Surface(face).IsNull()) {
                    BRepAdaptor_Surface surface(face);
                    bool haveExactNormal = false;
                    if (surface.GetType() == GeomAbs_Plane) {
                        const gp_Pln pln = surface.Plane();
                        gp_Vec normal(pln.Axis().Direction());
                        if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();
                        candidate.direction = {normal.X(), normal.Y(), normal.Z()};
                        haveExactNormal = normalise(candidate.direction);
                    } else if (auto* faceInt = geometry.cad->faceIntersector(faceId)) {
                        faceInt->Perform(gp_Lin(gp_Pnt(localRay.origin[0], localRay.origin[1], localRay.origin[2]),
                                                gp_Dir(localRay.direction[0], localRay.direction[1], localRay.direction[2])),
                                         0.0, frame.nearZ - frame.farZ);
                        if (faceInt->IsDone()) {
                            int bestHitIdx = -1;
                            double bestHitW = frame.nearZ - frame.farZ;
                            for (int h = 1; h <= faceInt->NbPnt(); ++h) {
                                const double w = faceInt->WParameter(h);
                                if (w >= 0.0 && w <= visibleDepth + 8.0 * unit && w < bestHitW) {
                                    bestHitW = w;
                                    bestHitIdx = h;
                                }
                            }
                            if (bestHitIdx > 0) {
                                exactDepth = bestHitW;
                                candidate.hitPosition = vec(faceInt->Pnt(bestHitIdx));
                                gp_Pnt pt;
                                gp_Vec du, dv;
                                surface.D1(faceInt->UParameter(bestHitIdx), faceInt->VParameter(bestHitIdx), pt, du, dv);
                                gp_Vec normal = du.Crossed(dv);
                                if (face.Orientation() == TopAbs_REVERSED) normal.Reverse();
                                candidate.direction = {normal.X(), normal.Y(), normal.Z()};
                                const auto useAxis = [&](const gp_Dir& dir) {
                                    candidate.direction = {dir.X(), dir.Y(), dir.Z()};
                                };
                                switch (surface.GetType()) {
                                case GeomAbs_Cylinder: useAxis(surface.Cylinder().Axis().Direction()); break;
                                case GeomAbs_Cone: useAxis(surface.Cone().Axis().Direction()); break;
                                case GeomAbs_Torus: useAxis(surface.Torus().Axis().Direction()); break;
                                case GeomAbs_Sphere: {
                                    const auto radial = gp_Vec(surface.Sphere().Location(), faceInt->Pnt(bestHitIdx));
                                    candidate.direction = {radial.X(), radial.Y(), radial.Z()};
                                    if (face.Orientation() == TopAbs_REVERSED)
                                        for (auto& value : candidate.direction) value = -value;
                                    break;
                                }
                                default: break;
                                }
                                haveExactNormal = normalise(candidate.direction);
                            }
                        }
                    }
                    if (!haveExactNormal) {
                        const auto& triMesh = geometry.cad->faceTriMesh;
                        const Vec a = triMesh.position(triMesh.triangles[3 * hitTri]);
                        const Vec b = triMesh.position(triMesh.triangles[3 * hitTri + 1]);
                        const Vec c = triMesh.position(triMesh.triangles[3 * hitTri + 2]);
                        candidate.direction = cross(subtract(b, a), subtract(c, a));
                        candidate.hasDirection = normalise(candidate.direction);
                    } else {
                        candidate.hasDirection = true;
                    }
                } else {
                    const auto& triMesh = geometry.cad->faceTriMesh;
                    const Vec a = triMesh.position(triMesh.triangles[3 * hitTri]);
                    const Vec b = triMesh.position(triMesh.triangles[3 * hitTri + 1]);
                    const Vec c = triMesh.position(triMesh.triangles[3 * hitTri + 2]);
                    candidate.direction = cross(subtract(b, a), subtract(c, a));
                    candidate.hasDirection = normalise(candidate.direction);
                }
                if (exactDepth < bestDepth) {
                    best = candidate;
                    bestDepth = exactDepth;
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
                best.kind = PickKind::Part;
                best.partId = partId;
                best.position = best.hitPosition = found->second.cad->partCenter();
            }
        }
    }
    return best;
}

void OccViewport::fitAll()
{
    fitAllIncluding(std::nullopt);
}

void OccViewport::fitAllIncluding(const std::optional<Vec>& point)
{
    auto bounds = m_state->bounds;
    if (point) bounds.add(*point);
    if (!bounds.valid) return;
    cancelPickRotationCenter();
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.fit(bounds, size(), isVisible());
    m_state->rotationCenterMode = point ? RotationCenterMode::CustomPoint : RotationCenterMode::ModelCenter;
    m_state->rememberCamera();
    updateCamera();
}

void OccViewport::viewIsometric()
{
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.rotation = isometricRotation();
    m_state->rememberCamera();
    updateCamera();
}

void OccViewport::setViewOrientation(const QVector3D& direction, const QVector3D& up)
{
    interruptPointerOperation();
    m_state->rememberCamera();
    m_state->camera.rotation = viewRotation(direction, up);
    m_state->rememberCamera();
    updateCamera();
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
    updateCamera();
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
    updateCamera();
}

void OccViewport::zoomView(bool zoomIn, double factor)
{
    if (!std::isfinite(factor) || factor <= 0.0) return;
    auto& state = *m_state;
    state.rememberCamera();
    state.camera.zoomAt((zoomIn ? -1.0 : 1.0) * std::log(factor), rect().center(), size());
    state.rememberCamera();
    state.hover = {};
    updateCamera();
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
    updateCamera();
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
    if (!init(s.surface, SurfaceVertex, SurfaceFragment)) {
        failure = tr("The CAD surface shader failed: %1").arg(s.surface->log());
    } else if (!init(s.edge, EdgeVertex, EdgeFragment, EdgeGeometry)) {
        failure = tr("The CAD edge shader failed: %1").arg(s.edge->log());
    } else if (!init(s.point, PointVertex, PointFragment, PointGeometry)) {
        failure = tr("The CAD selection marker shader failed: %1").arg(s.point->log());
    } else {
        s.gl->glGenVertexArrays(1, &s.emptyVao);
        s.gl->glGenVertexArrays(1, &s.pointBatchVao);
        s.gl->glGenBuffers(1, &s.pointBatchVbo);
        if (!s.emptyVao || !s.pointBatchVao || !s.pointBatchVbo || s.gl->glGetError() != GL_NO_ERROR) {
            failure = tr("Could not allocate the CAD viewport's OpenGL vertex array.");
        } else {
            s.gl->glBindVertexArray(s.pointBatchVao);
            s.gl->glBindBuffer(GL_ARRAY_BUFFER, s.pointBatchVbo);
            s.gl->glEnableVertexAttribArray(0);
            s.gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), nullptr);
            s.gl->glBindVertexArray(0);
            s.gl->glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
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
        if (s.pointBatchVao) s.gl->glDeleteVertexArrays(1, &s.pointBatchVao);
        if (s.pointBatchVbo) s.gl->glDeleteBuffers(1, &s.pointBatchVbo);
    } else {
        // No raw GL calls if the context was already lost. Qt's guarded shader
        // resources and the context/share group handle deferred destruction.
        s.buffers.clear();
        s.pickBuffers.clear();
    }
    s.emptyVao = s.pointBatchVao = s.pointBatchVbo = 0;
    s.edge.reset();
    s.point.reset();
    s.surface.reset();
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
        ok = bufferGeometry(gl, *s.parts[i].cad, s.buffers[i].cad) &&
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
            gl->glClearColor(viewportBackground.x(), viewportBackground.y(), viewportBackground.z(), 1.0f);
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
    gl.glClearColor(viewportBackground.x(), viewportBackground.y(), viewportBackground.z(), 1.0f);
    gl.glClearDepth(1.0);
    gl.glClearStencil(0);
    gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

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
    std::vector<std::pair<double, std::size_t>> transparent;
    for (std::size_t i = 0; i < s.buffers.size(); ++i) {
        const auto& part = s.parts[i];
        const float opacity = part.opacity(s.mode);
        if (part.suppressed || opacity <= 0.0f || opacity >= 1.0f) continue;
        const auto* pose = s.poseFor(part.id);
        const auto center = pose ? pose->relativePoint(vec(part.origin), pivot) : subtract(vec(part.origin), pivot);
        const double depth = frame.view(2, 0) * center[0] + frame.view(2, 1) * center[1] + frame.view(2, 2) * center[2];
        transparent.emplace_back(depth, i);
    }
    std::sort(transparent.begin(), transparent.end());

    std::set<int> multiSelectedParts;
    if (!s.pickActive && s.selectionFilter == PickKind::Part) {
        for (const auto& pick : s.viewportSelection)
            if (pick.kind == PickKind::Part && pick.partId > 0) multiSelectedParts.insert(pick.partId);
    }
    const auto isPartSelected = [&](int id) {
        return id == s.selected || multiSelectedParts.count(id) != 0;
    };
    const auto setupSurface = [&] {
        setup(*s.surface);
        s.surface->setUniformValue("uNormal", frame.normal);
        s.surface->setUniformValue("uOutline", false);
        s.surface->setUniformValue("uHighlight", false);
    };
    const auto drawSurface = [&](std::size_t i, float opacity) {
        const auto& part = s.parts[i];
        const bool meshed = part.usesMesh(s.mode);
        const auto& gpu = meshed ? s.buffers[i].mesh : s.buffers[i].cad;
        if (!gpu.triangleCount) return;
        const bool hovered = !s.pickActive && s.selectionFilter == PickKind::Part &&
                             s.pickEnabled && s.hover.partId == part.id;
        const auto defaultPartColor = [&](int partId, std::size_t index) {
            static const std::array<QVector3D, 12> kDefaultPartPalette{
                rgb(104, 136, 168), // 钢蓝灰
                rgb(198, 134,  76), // 暖铜橙
                rgb( 86, 158, 152), // 青钛绿
                rgb(188, 110,  94), // 陶土红
                rgb(122, 164, 116), // 鼠尾草绿
                rgb(196, 164,  84), // 工业黄铜
                rgb(130, 136, 184), // 靛蓝灰
                rgb(176, 114, 136), // 玫瑰灰
                rgb( 92, 152, 182), // 天青钢
                rgb(156, 148,  90), // 橄榄青铜
                rgb(142, 126, 162), // 雾紫灰
                rgb(154, 132, 112), // 暖钛褐
            };
            const std::size_t slot = partId > 0 ? std::size_t(partId - 1) : index;
            return kDefaultPartPalette[slot % kDefaultPartPalette.size()];
        };
        const auto cadColor = part.displayColor.isValid()
            ? QVector3D(float(part.displayColor.redF()), float(part.displayColor.greenF()), float(part.displayColor.blueF()))
            : defaultPartColor(part.id, i);
        const auto color = isPartSelected(part.id) ? rgb(63, 137, 216) :
                           hovered ? rgb(156, 115, 166) :
                           meshed ? rgb(138, 166, 189) : cadColor;
        setPose(*s.surface, part);
        s.surface->setUniformValue("uFlat", meshed);
        s.surface->setUniformValue("uScalars", meshed && s.scalarParts.count(part.id) != 0);
        s.surface->setUniformValue("uColor", color);
        s.surface->setUniformValue("uOpacity", opacity);
        gl.glBindVertexArray(gpu.vao);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.triangles);
        gl.glDrawElements(GL_TRIANGLES, gpu.triangleCount, GL_UNSIGNED_INT, nullptr);
    };
    const auto drawEdges = [&](std::size_t i, float opacity) {
        const auto& part = s.parts[i];
        const bool meshed = part.usesMesh(s.mode), selected = isPartSelected(part.id);
        const auto& geometry = meshed ? part.mesh : *part.cad;
        const auto& gpu = meshed ? s.buffers[i].mesh : s.buffers[i].cad;
        if (!gpu.edgeCount || !showsEdges(s.mode, geometry)) return;
        const auto color = selected ? rgb(15, 78, 150) : (meshed ? rgb(31, 67, 95) : rgb(42, 57, 75));
        setPose(*s.edge, part);
        s.edge->setUniformValue("uColor", QVector4D(color, opacity * (meshed ? 0.8f : 0.94f)));
        s.edge->setUniformValue("uWidthPixels", (selected ? 1.75f : (meshed ? 1.0f : 1.25f)) * dpr);
        gl.glBindVertexArray(gpu.vao);
        gl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.edges);
        gl.glDrawElements(GL_LINES, gpu.edgeCount, GL_UNSIGNED_INT, nullptr);
    };

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
        setupSurface();
        gl.glEnable(GL_POLYGON_OFFSET_FILL);
        gl.glPolygonOffset(1.0f, 1.0f);
        for (std::size_t i = 0; i < s.buffers.size(); ++i) {
            const auto& part = s.parts[i];
            if (!part.suppressed && part.opacity(s.mode) == 1.0f) drawSurface(i, 1.0f);
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
                if (part.suppressed || part.opacity(s.mode) != 1.0f) continue;
                const auto& gpu = part.usesMesh(s.mode) ? s.buffers[i].mesh : s.buffers[i].cad;
                if (!gpu.triangleCount) continue;
                setPose(*s.surface, part);
                s.surface->setUniformValue("uColor", isPartSelected(part.id) ? rgb(21, 94, 170) : rgb(57, 72, 89));
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
            if (!part.suppressed && part.opacity(s.mode) == 1.0f) drawEdges(i, 1.0f);
        }
        s.edge->release();
    }
    if (!transparent.empty()) {
        // 保留不透明物体的深度，透明件从远到近叠加，不遮挡背后的实体和云图。
        gl.glDepthMask(GL_FALSE);
        if (!wireframe) {
            gl.glDepthFunc(GL_LESS);
            gl.glEnable(GL_POLYGON_OFFSET_FILL);
            gl.glEnable(GL_CULL_FACE);
            setupSurface();
            for (const auto& entry : transparent) {
                const auto i = entry.second;
                const float opacity = s.parts[i].opacity(s.mode);
                gl.glCullFace(GL_FRONT);
                drawSurface(i, opacity);
                gl.glCullFace(GL_BACK);
                drawSurface(i, opacity);
            }
            s.surface->release();
            gl.glDisable(GL_CULL_FACE);
            gl.glDisable(GL_POLYGON_OFFSET_FILL);
        }
        gl.glDepthFunc(GL_LEQUAL);
        setup(*s.edge);
        for (const auto& entry : transparent) drawEdges(entry.second, s.parts[entry.second].opacity(s.mode));
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
    drawRotationCenterOverlay();
    drawViewportLogo();
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
        if (s.pickActive || s.selectionFilter != PickKind::Part) append(s.hover, rgb(199, 46, 153));
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
    struct PointBatch {
        std::size_t partIndex = 0;
        QVector3D color;
        std::vector<QVector3D> points;
    };
    std::vector<PointBatch> pointBatches;
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
                const auto geometry = cadPickGeometry(*s.pickGeometry.at(pick.partId).cad, pick);
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
        // 将同一部件与同色拾取点合并为单次 VBO 批量绘制，避免大量选点时数千次状态切换与单点 draw call
        const Vec markerPos = (pick.kind == PickKind::Part && pick.position == Vec{0.0, 0.0, 0.0})
            ? vec(part.origin) : pick.position;
        if (std::all_of(markerPos.begin(),markerPos.end(),[](double v){return std::isfinite(v)&&std::abs(v)<=maxSpan;})) {
            const QVector3D markerVec{static_cast<float>(markerPos[0]),
                                      static_cast<float>(markerPos[1]),
                                      static_cast<float>(markerPos[2])};
            if (!pointBatches.empty() && pointBatches.back().partIndex == entry.partIndex
                && pointBatches.back().color == entry.color) {
                pointBatches.back().points.push_back(markerVec);
            } else {
                PointBatch batch;
                batch.partIndex = entry.partIndex;
                batch.color = entry.color;
                batch.points.push_back(markerVec);
                pointBatches.push_back(std::move(batch));
            }
        }
    }
    if (!pointBatches.empty() && s.pointBatchVao && s.pointBatchVbo) {
        gl.glBindVertexArray(s.pointBatchVao);
        gl.glBindBuffer(GL_ARRAY_BUFFER, s.pointBatchVbo);
        for (const auto& batch : pointBatches) {
            if (batch.points.empty() || batch.partIndex >= s.parts.size()) continue;
            setup(*s.point, s.parts[batch.partIndex]);
            s.point->setUniformValue("uUseAttribute", true);
            s.point->setUniformValue("uDepthBiasNdc", bias);
            s.point->setUniformValue("uDiameterPixels", 10.0f * dpr);
            s.point->setUniformValue("uBorderPixels", 1.4f * dpr);
            s.point->setUniformValue("uColor", QVector4D(batch.color, 1));
            gl.glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(batch.points.size() * sizeof(QVector3D)),
                            batch.points.data(), GL_STREAM_DRAW);
            gl.glDrawArrays(GL_POINTS, 0, GLsizei(batch.points.size()));
            s.point->release();
        }
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl.glBindVertexArray(0);
    }
    for (const auto& item : s.auxiliaryPoints) {
        const auto& layer = item.second;
        if (layer.points.isEmpty() || !s.pointBatchVao || !s.pointBatchVbo) continue;
        s.point->bind();
        s.point->setUniformValue("uUseAttribute", true);
        s.point->setUniformValue("uView", frame.view);
        s.point->setUniformValue("uProjection", frame.projection);
        s.point->setUniformValue("uPivot", s.camera.pivot);
        s.point->setUniformValue("uViewportPixels", pixels);
        s.point->setUniformValue("uPosed", false);
        s.point->setUniformValue("uDepthBiasNdc", bias);
        s.point->setUniformValue("uDiameterPixels", layer.size * dpr);
        s.point->setUniformValue("uBorderPixels", std::min(1.4f,layer.size*.18f) * dpr);
        s.point->setUniformValue("uColor", QVector4D(layer.color.redF(),layer.color.greenF(),layer.color.blueF(),layer.color.alphaF()));
        gl.glBindVertexArray(s.pointBatchVao);
        gl.glBindBuffer(GL_ARRAY_BUFFER, s.pointBatchVbo);
        gl.glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(layer.points.size() * sizeof(QVector3D)),
                        layer.points.constData(), GL_STREAM_DRAW);
        gl.glDrawArrays(GL_POINTS, 0, GLsizei(layer.points.size()));
        gl.glBindBuffer(GL_ARRAY_BUFFER, 0);
        gl.glBindVertexArray(0);
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
    QSet<int> visibleJoints;
    for (const auto& symbol : s.symbols)
        if (symbol.collection == "joints") visibleJoints.insert(symbol.id);
    // 原编辑器会缓存绘制过的符号。辅助层采用同一组可见运动副，避免切换节点后残留。
    for (const auto& layer : s.auxiliarySymbols) for (const auto& symbol : layer.second) {
        if (symbol.collection == "joints" && !visibleJoints.contains(symbol.id)) continue;
        if (symbol.id >= 0 && (!symbol.linePoints.empty() || !symbol.trianglePoints.empty()))
            originalEntities.emplace(symbol.collection, symbol.id);
        symbols.push_back(&symbol);
    }
    const auto firstAuxiliary = symbols.size();
    for(const auto& symbol:s.symbols)
        if(!originalEntities.count({symbol.collection,symbol.id}))symbols.push_back(&symbol);
    std::rotate(symbols.begin(), symbols.begin() + firstAuxiliary, symbols.end());
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
    if (s.scalarParts.empty() || width() < 260 || height() < 200) return;
    QPainter painter(this);
    const int barHeight = std::min(220, height() - 100);
    const QRectF panel(width() - 177, 38, 165, barHeight + 65);
    painter.setPen(Qt::NoPen); painter.setBrush(QColor(255, 255, 255, 225)); painter.drawRoundedRect(panel, 5, 5);
    painter.setPen(QColor(34, 51, 69));
    painter.drawText(panel.adjusted(9, 5, -8, -barHeight - 30), Qt::AlignTop | Qt::TextWordWrap, s.scalarTitle);
    const QRectF bar(panel.left() + 12, panel.top() + 40, 18, barHeight);
    const bool uniform = s.scalarMinimum == s.scalarMaximum;
    if (uniform) {
        painter.fillRect(bar, QColor::fromRgbF(0, 0, 0.5));
    } else {
        QLinearGradient gradient(bar.bottomLeft(), bar.topLeft());
        for (int i = 0; i <= 16; ++i) {
            const double t = i / 16.0;
            const auto channel = [t](double center) { return std::clamp(1.5 - std::abs(4*t - center), 0.0, 1.0); };
            gradient.setColorAt(t, QColor::fromRgbF(channel(3), channel(2), channel(1)));
        }
        painter.fillRect(bar, gradient);
    }
    painter.setPen(QColor(53, 69, 87)); painter.setBrush(Qt::NoBrush); painter.drawRect(bar);
    const int tickCount = uniform ? 1 : 5;
    for (int i = 0; i < tickCount; ++i) {
        const double t = uniform ? .5 : i / 4.0;
        const double value = s.scalarMaximum * (1-t) + s.scalarMinimum * t;
        const double y = bar.top() + bar.height() * t;
        painter.drawLine(QPointF(bar.right(), y), QPointF(bar.right() + 5, y));
        painter.drawText(QRectF(bar.right() + 9, y - 10, 112, 20), Qt::AlignVCenter, QString::number(value, 'g', 6));
    }
}

void OccViewport::drawPickOverlay()
{
    const auto& s = *m_state;
    const bool showActive = s.pickFeedbackVisible &&
        (s.pickActive || s.selectionFilter != PickKind::Part || s.boxSelecting);
    const auto kind = s.pickActive ? s.pickKind : s.selectionFilter.value_or(PickKind::Part);
    if (!showActive && s.auxiliaryPicks.empty()) return;
    const FastProjector proj(s.camera, size(), s.bounds);
    const QRectF viewRect(rect());
    std::map<int, FastProjector::PartView> partViews;
    const auto viewForPart = [&](int partId) -> const FastProjector::PartView& {
        auto it = partViews.find(partId);
        if (it == partViews.end()) it = partViews.emplace(partId, proj.forPart(s.poseFor(partId))).first;
        return it->second;
    };
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    if (showActive && kind == PickKind::Node) {
        painter.setPen(QPen(QColor(37, 116, 174, 190), 1));
        painter.setBrush(QColor(226, 245, 255, 220));
        std::size_t totalNodes = 0;
        for (const auto& [partId, geometry] : s.pickGeometry) {
            if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
            totalNodes += geometry.nodes.size();
        }
        if (totalNodes <= 400) {
            for (const auto& [partId, geometry] : s.pickGeometry) {
                if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
                const auto& pv = viewForPart(partId);
                for (const auto& point : geometry.nodes) {
                    const auto screen = pv.project(point);
                    if (viewRect.contains(screen)) painter.drawEllipse(screen, 2.5, 2.5);
                }
            }
        } else {
            // 超过 400 个候选节点时统一采用单批 drawPoints 渲染，消除大量候选点光栅化卡顿
            painter.setPen(QPen(QColor(37, 116, 174, 215), 4.2, Qt::SolidLine, Qt::RoundCap));
            QVector<QPointF> batch;
            batch.reserve(int(std::min<std::size_t>(totalNodes, 65536)));
            for (const auto& [partId, geometry] : s.pickGeometry) {
                if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
                const auto& pv = viewForPart(partId);
                for (const auto& point : geometry.nodes) {
                    const auto screen = pv.project(point);
                    if (viewRect.contains(screen)) batch.append(screen);
                }
            }
            if (!batch.isEmpty()) painter.drawPoints(batch.constData(), batch.size());
        }
    } else if (showActive && (kind == PickKind::MeshEdge || kind == PickKind::MeshFace || kind == PickKind::Element)) {
        // 如果当前已在网格显示模式，GPU 已绘制全部单元边，无需再用 CPU QPainter 重复叠加整网线框
        if (s.mode != DisplayMode::Mesh) {
            painter.setPen(QPen(QColor(35, 108, 151, 150), 1));
            QVector<QLineF> lines;
            for (const auto& [partId, geometry] : s.pickGeometry) {
                if (!s.pickParts.empty() && !s.pickParts.count(partId)) continue;
                const auto& pv = viewForPart(partId);
                lines.reserve(lines.size() + int(geometry.meshEdges.size()));
                for (const auto& edge : geometry.meshEdges) {
                    const QPointF a = pv.project(geometry.nodes[edge[0]]);
                    const QPointF b = pv.project(geometry.nodes[edge[1]]);
                    if (viewRect.contains(a) || viewRect.contains(b)) lines.append(QLineF(a, b));
                }
            }
            if (!lines.isEmpty()) painter.drawLines(lines);
        }
    }
    const auto highlight = [&](const PickResult& pick, const QColor& color) {
        if (cadPick(pick.kind)) return; // rendered with real depth above
        const auto found = s.pickGeometry.find(pick.partId);
        if (found == s.pickGeometry.end()) return;
        const auto& geometry = found->second;
        if(!validPickReference(pick,geometry))return;
        const auto& pv = viewForPart(pick.partId);
        painter.setPen(QPen(color, 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        if (pick.kind == PickKind::MeshEdge && pick.index > 0 && pick.index <= int(geometry.meshEdges.size())) {
            const auto& edge = geometry.meshEdges[pick.index-1];
            painter.drawLine(pv.project(geometry.nodes[edge[0]]), pv.project(geometry.nodes[edge[1]]));
        } else if ((pick.kind == PickKind::MeshFace || pick.kind == PickKind::Element) && pick.index > 0) {
            std::vector<Index> nodes;
            if (pick.kind == PickKind::Element && !geometry.elements.empty() && pick.index <= int(geometry.elements.size()))
                nodes.assign(geometry.elements[pick.index-1].begin(),geometry.elements[pick.index-1].end());
            else if (pick.index <= int(geometry.meshFaces.size()))
                nodes.assign(geometry.meshFaces[pick.index-1].begin(),geometry.meshFaces[pick.index-1].end());
            if (std::all_of(nodes.begin(),nodes.end(),[&](Index n) { return n < geometry.nodes.size(); })) {
                if (nodes.size() == 3) {
                    QPolygonF face; for (auto node : nodes) face.append(pv.project(geometry.nodes[node]));
                    QColor tint = color; tint.setAlpha(50); painter.setBrush(tint); painter.drawPolygon(face); painter.setBrush(Qt::NoBrush);
                } else for (std::size_t i=0; i<nodes.size(); ++i) for (std::size_t j=i+1; j<nodes.size(); ++j)
                    painter.drawLine(pv.project(geometry.nodes[nodes[i]]), pv.project(geometry.nodes[nodes[j]]));
            }
        }
        Vec point = pick.position;
        if (pick.kind == PickKind::Node && pick.index >= 0 && pick.index < int(geometry.nodes.size())) point = geometry.nodes[pick.index];
        const auto screen = pv.project(point);
        painter.setBrush(color); painter.setPen(QPen(Qt::white, 1.4)); painter.drawEllipse(screen, 5.0, 5.0);
    };
    const auto highlightBatch = [&](const QVector<PickResult>& list, const QColor& color) {
        if (list.size() <= 300) {
            for (const auto& pick : list) highlight(pick, color);
            return;
        }
        QVector<QPointF> nodeScreens;
        nodeScreens.reserve(list.size());
        const QRectF expanded = viewRect.adjusted(-8.0, -8.0, 8.0, 8.0);
        for (const auto& pick : list) {
            if (pick.kind != PickKind::Node) {
                highlight(pick, color);
                continue;
            }
            const auto found = s.pickGeometry.find(pick.partId);
            if (found == s.pickGeometry.end() || !validPickReference(pick, found->second)) continue;
            const auto& pv = viewForPart(pick.partId);
            const Vec pt = (pick.index >= 0 && pick.index < int(found->second.nodes.size()))
                ? found->second.nodes[std::size_t(pick.index)] : pick.position;
            const QPointF sc = pv.project(pt);
            if (expanded.contains(sc)) nodeScreens.append(sc);
        }
        if (!nodeScreens.isEmpty()) {
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::white, 11.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawPoints(nodeScreens.constData(), nodeScreens.size());
            painter.setPen(QPen(color, 8.6, Qt::SolidLine, Qt::RoundCap));
            painter.drawPoints(nodeScreens.constData(), nodeScreens.size());
        }
    };
    for (const auto& layer : s.auxiliaryPicks)
        highlightBatch(layer.second.picks, layer.second.color);
    if (!showActive) return;
    const auto& selection = s.pickActive ? s.picks : s.viewportSelection;
    highlightBatch(selection, s.pickColor);
    if (s.hover.isValid() && (s.pickActive || s.selectionFilter != PickKind::Part))
        highlight(s.hover, QColor(199, 46, 153));
    if (s.boxSelecting) {
        const bool subtractBox = s.button == Qt::RightButton;
        painter.setPen(QPen(subtractBox ? QColor(217, 83, 79) : QColor(25, 114, 195), 1.2, Qt::DashLine));
        painter.setBrush(subtractBox ? QColor(235, 106, 90, 36) : QColor(60, 160, 230, 35));
        painter.drawRect(s.pickRectangle);
    }
    if (!s.pickActive) return;
    const QStringList names{tr("部件"), tr("顶点"), tr("边"), tr("面"), tr("节点"), tr("网格边"), tr("网格面"), tr("单元"), tr("实体")};
    const QString label = tr("拾取%1 · 已选 %2 · %3 · Esc 结束")
        .arg(names.value(int(s.pickKind))).arg(s.picks.size())
        .arg(s.pickMultiple ? tr("左键/Ctrl+左键加选 · Ctrl+右键减选 · Shift+左/右键框选加减") : tr("左键选择 · Ctrl+右键撤回"));
    const QFontMetrics metrics(painter.font());
    const QRectF box(12, 12, std::min(width() - 24, metrics.horizontalAdvance(label) + 20), metrics.height() + 16);
    painter.setPen(Qt::NoPen); painter.setBrush(QColor(255, 255, 255, 225)); painter.drawRoundedRect(box, 4, 4);
    painter.setPen(QColor(38, 55, 72)); painter.drawText(box.adjusted(10, 0, -10, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
}

void OccViewport::drawRotationCenterOverlay()
{
    const auto& s = *m_state;
    const bool activeRotateDrag = s.dragging
        && (s.navigation == State::Navigation::Orbit || s.navigation == State::Navigation::Roll);
    const bool pulseActive = s.pivotPulseTimer && s.pivotPulseTimer->isActive();
    if (!s.pickingRotationCenter && !activeRotateDrag && !pulseActive) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto frame = s.camera.frame(size(), s.bounds);
    const Vec pivot = vec(s.camera.pivot);
    const QPointF pivotScreen = projectPoint(pivot);
    const QRectF expandedView = QRectF(rect()).adjusted(-32.0, -32.0, 32.0, 32.0);
    if (expandedView.contains(pivotScreen)) {
        // 绘制旋转中心三维坐标微轴与靶心环，使空间/平面旋转支点一目了然
        painter.setPen(QPen(QColor(255, 255, 255, 215), 3.4, Qt::SolidLine, Qt::RoundCap));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(pivotScreen, 7.5, 7.5);

        const QColor axisColors[3] = {QColor(224, 72, 72, 235), QColor(47, 168, 95, 235), QColor(47, 125, 225, 235)};
        for (int axis = 0; axis < 3; ++axis) {
            QPointF dir(frame.rows[0][axis], -frame.rows[1][axis]);
            const double len = std::hypot(dir.x(), dir.y());
            if (len < 1e-4) continue;
            const QPointF tip = pivotScreen + dir * (13.5 / len);
            const QPointF start = pivotScreen + dir * (3.2 / len);
            painter.setPen(QPen(QColor(255, 255, 255, 195), 3.0, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(start, tip);
            painter.setPen(QPen(axisColors[axis], 1.75, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(start, tip);
        }

        const QColor ringColor = s.rotationCenterMode == RotationCenterMode::ModelCenter
            ? QColor(36, 123, 214, 235)
            : QColor(232, 96, 36, 240);
        painter.setPen(QPen(ringColor, 1.8));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(pivotScreen, 7.5, 7.5);
        painter.setPen(QPen(Qt::white, 1.1));
        painter.setBrush(ringColor);
        painter.drawEllipse(pivotScreen, 2.5, 2.5);
    }

    if (!s.pickingRotationCenter) return;

    if (s.hoverPivotValid) {
        const QPointF hoverScreen = projectPoint(s.hoverPivotPoint);
        if (expandedView.contains(hoverScreen)) {
            painter.setPen(QPen(QColor(255, 255, 255, 225), 3.4, Qt::SolidLine, Qt::RoundCap));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(hoverScreen, 6.5, 6.5);
            painter.drawLine(hoverScreen + QPointF(-11.0, 0.0), hoverScreen + QPointF(-3.5, 0.0));
            painter.drawLine(hoverScreen + QPointF(3.5, 0.0), hoverScreen + QPointF(11.0, 0.0));
            painter.drawLine(hoverScreen + QPointF(0.0, -11.0), hoverScreen + QPointF(0.0, -3.5));
            painter.drawLine(hoverScreen + QPointF(0.0, 3.5), hoverScreen + QPointF(0.0, 11.0));

            const QColor snapColor(22, 138, 236, 245);
            painter.setPen(QPen(snapColor, 1.8, Qt::SolidLine, Qt::RoundCap));
            painter.drawEllipse(hoverScreen, 6.5, 6.5);
            painter.drawLine(hoverScreen + QPointF(-11.0, 0.0), hoverScreen + QPointF(-3.5, 0.0));
            painter.drawLine(hoverScreen + QPointF(3.5, 0.0), hoverScreen + QPointF(11.0, 0.0));
            painter.drawLine(hoverScreen + QPointF(0.0, -11.0), hoverScreen + QPointF(0.0, -3.5));
            painter.drawLine(hoverScreen + QPointF(0.0, 3.5), hoverScreen + QPointF(0.0, 11.0));
            painter.setPen(QPen(Qt::white, 1.0));
            painter.setBrush(snapColor);
            painter.drawEllipse(hoverScreen, 2.4, 2.4);
        }
    }

    QFont hudFont(MxFunctions::UiMetrics::uiFontFamily());
    hudFont.setPixelSize(std::max(12, MxFunctions::UiMetrics::bodyFontSize()));
    painter.setFont(hudFont);
    const QString prompt = tr("自定义旋转中心 · 点击模型顶点/棱边/表面设定 · 右键或 Esc 取消");
    const QFontMetrics metrics(hudFont);
    const double topY = s.pickActive ? 48.0 : 12.0;
    const QRectF box(12.0, topY, std::min<double>(width() - 24, metrics.horizontalAdvance(prompt) + 24),
                     metrics.height() + 16);
    painter.setPen(QPen(QColor(22, 138, 236, 150), 1.0));
    painter.setBrush(QColor(255, 255, 255, 235));
    painter.drawRoundedRect(box, 4.0, 4.0);
    painter.setPen(QColor(28, 49, 70));
    painter.drawText(box.adjusted(12.0, 0.0, -12.0, 0.0), Qt::AlignVCenter | Qt::AlignLeft, prompt);
}

void OccViewport::drawViewportLogo()
{
    if (width() < 120 || height() < 72) return;
    static const QImage sourceLogo = [] {
        QImage img(QStringLiteral(":/mMainWidgets/Icon/viewport_mxsim_logo.png"));
        if (img.isNull())
            img.load(QCoreApplication::applicationDirPath() + QStringLiteral("/viewport_mxsim_logo.png"));
        return img;
    }();
    if (sourceLogo.isNull() || sourceLogo.height() <= 0) return;

    const double dpr = std::max(1.0, devicePixelRatioF());
    const double logicalH = std::clamp(width() * 0.058, 44.0, 68.0);
    const double logicalW = logicalH * (double(sourceLogo.width()) / double(sourceLogo.height()));
    const QSize targetDeviceSize(std::max(1, qRound(logicalW * dpr)),
                                 std::max(1, qRound(logicalH * dpr)));

    static QPixmap cachedPixmap;
    static QSize cachedDeviceSize;
    static double cachedDpr = 0.0;
    if (cachedPixmap.isNull() || cachedDeviceSize != targetDeviceSize || std::abs(cachedDpr - dpr) > 1e-4) {
        QImage scaled = sourceLogo.scaled(targetDeviceSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        scaled.setDevicePixelRatio(dpr);
        cachedPixmap = QPixmap::fromImage(scaled);
        cachedDeviceSize = targetDeviceSize;
        cachedDpr = dpr;
    }

    const double actualLogicalW = cachedPixmap.width() / dpr;
    const double rightMargin = m_state->scalarParts.empty() ? 18.0 : 192.0;
    const double topMargin = 14.0;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(QPointF(width() - rightMargin - actualLogicalW, topMargin), cachedPixmap);
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
            const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
            const Ray localRay = restRay(ray, s.poseFor(part.id));
            if (raycastGeometry(localRay, geometry, nearest, unit * 0.01)) {
                best = part.id;
            }
        }
    }

    const double tolerance = unit * 5.0; // logical pixels, independent of DPI and zoom
    double bestOffset = tolerance * tolerance;
    double bestLineDepth = nearest;
    const bool hitFace = best != -1;
    for (const auto& part : s.parts) {
        if (part.suppressed) continue;
        const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
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
    const auto mods = event->modifiers();
    const bool shiftOnly = mods.testFlag(Qt::ShiftModifier) && !mods.testFlag(Qt::ControlModifier);
    const bool ctrlHeld = mods.testFlag(Qt::ControlModifier);
    const bool canMultiSelect = !s.pickActive || s.pickMultiple;
    // Shift + 左键拖动 = 框选加；Shift + 右键拖动 = 框选减
    s.boxSelecting = !s.pickingRotationCenter && s.pickEnabled && shiftOnly &&
                     ((s.button == Qt::LeftButton && canMultiSelect) || s.button == Qt::RightButton);
    if (s.boxSelecting) {
        s.navigation = State::Navigation::Select;
    } else if (!s.pickingRotationCenter && s.pickEnabled && ctrlHeld &&
               (s.button == Qt::LeftButton || s.button == Qt::RightButton)) {
        // Ctrl + 左键单选加 / Ctrl + 右键单选减：按住 Ctrl 点选时不触发相机旋转或平移
        s.navigation = State::Navigation::Select;
    } else if (s.navigationMode == MouseNavigationMode::Nx) {
        if (s.button == Qt::MiddleButton) s.updateNxNavigation(mods, s.press, size());
        else if (s.button == Qt::LeftButton) s.navigation = State::Navigation::Select;
    } else {
        if (s.button == Qt::LeftButton) {
            if (s.leftButtonMode == LeftButtonMode::OrbitPlane) s.navigation = State::Navigation::Roll;
            else if (s.leftButtonMode == LeftButtonMode::Pan) s.navigation = State::Navigation::Pan;
            else s.navigation = State::Navigation::Orbit;
        } else {
            s.navigation = State::Navigation::Pan;
        }
    }
    if (s.navigation == State::Navigation::Orbit || s.navigation == State::Navigation::Roll)
        s.beginOrbit(s.press, size());
    s.pickRectangle = QRectF(s.press, s.press);
    event->accept();
}

void OccViewport::mouseMoveEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    if (s.button == Qt::NoButton || !(event->buttons() & s.button)) {
        if (s.button != Qt::NoButton) interruptPointerOperation();
        if (s.pickingRotationCenter) {
            Vec candidate{};
            const bool found = findRotationCenterPoint(event->localPos(), candidate);
            if (found != s.hoverPivotValid || (found && candidate != s.hoverPivotPoint)) {
                s.hoverPivotValid = found;
                s.hoverPivotPoint = candidate;
                update();
            }
            event->accept();
            return;
        }
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
    if (s.dragging) updateCamera();
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
    const bool editsNodes = s.editsNodeSelection();
    const auto seedSelectedPartIfNeeded = [&] {
        if (!s.pickActive && s.selectionFilter == PickKind::Part && s.viewportSelection.isEmpty() && s.selected > 0) {
            const auto found = s.pickGeometry.find(s.selected);
            if (found != s.pickGeometry.end() && !found->second.cad->shape.IsNull()) {
                PickResult seeded;
                seeded.kind = PickKind::Part;
                seeded.partId = s.selected;
                seeded.position = seeded.hitPosition = found->second.cad->partCenter();
                s.viewportSelection.append(seeded);
            }
        }
    };

    if (s.boxSelecting && rectangle.width() + rectangle.height() >= QApplication::startDragDistance()) {
        const bool isSubtract = (s.button == Qt::RightButton);
        const PickKind boxKind = s.pickActive ? s.pickKind : s.selectionFilter.value_or(PickKind::Face);
        if (!s.pickActive && boxKind == PickKind::Part) seedSelectedPartIfNeeded();
        auto& selection = s.pickActive ? s.picks : s.viewportSelection;
        const int previousCount = selection.size();
        const FastProjector proj(s.camera, size(), s.bounds);
        std::map<int, const Part*> partLookup;
        for (const auto& p : s.parts) if (!p.suppressed) partLookup.emplace(p.id, &p);

        if (isSubtract) {
            selection.erase(std::remove_if(selection.begin(), selection.end(), [&](const PickResult& pick) {
                const auto geometry = s.pickGeometry.find(pick.partId);
                if (geometry == s.pickGeometry.end() || !validPickReference(pick, geometry->second)) return false;
                const auto partIt = partLookup.find(pick.partId);
                if (partIt == partLookup.end()) return false;
                const auto pv = proj.forPart(s.poseFor(pick.partId));
                if (pick.kind == PickKind::Node) {
                    return rectangle.contains(pv.project(geometry->second.nodes[std::size_t(pick.index)]));
                }
                if (pick.kind == PickKind::Vertex) {
                    geometry->second.cad->ensureVertices();
                    return rectangle.contains(pv.project(geometry->second.cad->vertexPoints[std::size_t(pick.index - 1)]));
                }
                if (pick.kind == PickKind::Part) {
                    const QRectF partRect = pv.screenBounds(partIt->second->cad->bounds);
                    return rectangle.contains(partRect) || rectangle.contains(pv.project(geometry->second.cad->partCenter()));
                }
                return rectangle.contains(pv.project(pick.position)) || rectangle.contains(pv.project(pick.hitPosition));
            }), selection.end());
        } else {
            std::set<std::tuple<int, int, int>> existingKeys;
            for (const auto& existing : selection)
                existingKeys.emplace(int(existing.kind), existing.partId, existing.index);
            const auto appendUniquePick = [&](const PickResult& pick) {
                if (existingKeys.emplace(int(pick.kind), pick.partId, pick.index).second)
                    selection.append(pick);
            };
            for (const auto& [partId, geometry] : s.pickGeometry) {
                if (s.pickActive && !s.pickParts.empty() && !s.pickParts.count(partId)) continue;
                const auto partIt = partLookup.find(partId);
                if (partIt == partLookup.end()) continue;
                const Part& part = *partIt->second;
                if (part.deformed && boxKind < PickKind::Node) continue;
                const auto pv = proj.forPart(s.poseFor(partId));
                const Bounds& activeBounds = boxKind >= PickKind::Node && !part.mesh.bounds.valid
                    ? part.cad->bounds : (boxKind >= PickKind::Node ? part.mesh.bounds : part.cad->bounds);
                const QRectF partScreenRect = pv.screenBounds(activeBounds);
                if (partScreenRect.isValid() && !rectangle.intersects(partScreenRect)) continue;

                if (boxKind == PickKind::Part) {
                    if (rectangle.contains(partScreenRect) || rectangle.contains(pv.project(geometry.cad->partCenter()))) {
                        PickResult pick;
                        pick.kind = PickKind::Part;
                        pick.partId = partId;
                        pick.position = pick.hitPosition = geometry.cad->partCenter();
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::Node) {
                    for (int index = 0; index < int(geometry.nodes.size()); ++index) {
                        if (!rectangle.contains(pv.project(geometry.nodes[std::size_t(index)]))) continue;
                        PickResult pick;
                        pick.kind = PickKind::Node;
                        pick.partId = partId;
                        pick.index = index;
                        pick.nodeId = geometry.nodeIds.value(index, index + 1);
                        pick.position = pick.hitPosition = geometry.nodes[std::size_t(index)];
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::Vertex) {
                    geometry.cad->ensureVertices();
                    for (int index = 1; index <= int(geometry.cad->vertexPoints.size()); ++index) {
                        const Vec& pt = geometry.cad->vertexPoints[std::size_t(index - 1)];
                        if (!std::isfinite(pt[0]) || !rectangle.contains(pv.project(pt))) continue;
                        PickResult pick;
                        pick.kind = PickKind::Vertex;
                        pick.partId = partId;
                        pick.index = index;
                        pick.position = pick.hitPosition = pt;
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::MeshEdge) {
                    for (std::size_t index = 0; index < geometry.meshEdges.size(); ++index) {
                        const auto& edge = geometry.meshEdges[index];
                        const Vec& pa = geometry.nodes[edge[0]];
                        const Vec& pb = geometry.nodes[edge[1]];
                        const Vec mid{(pa[0] + pb[0]) * 0.5, (pa[1] + pb[1]) * 0.5, (pa[2] + pb[2]) * 0.5};
                        if (!rectangle.contains(pv.project(mid)) &&
                            !(rectangle.contains(pv.project(pa)) && rectangle.contains(pv.project(pb)))) continue;
                        PickResult pick;
                        pick.kind = PickKind::MeshEdge;
                        pick.partId = partId;
                        pick.index = int(index + 1);
                        pick.position = pick.hitPosition = mid;
                        pick.direction = subtract(pb, pa);
                        pick.hasDirection = normalise(pick.direction);
                        pick.nodeIds = {geometry.nodeIds.value(edge[0], int(edge[0] + 1)),
                                        geometry.nodeIds.value(edge[1], int(edge[1] + 1))};
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::MeshFace || boxKind == PickKind::Element) {
                    const bool volume = boxKind == PickKind::Element && !geometry.elements.empty();
                    const std::size_t count = volume ? geometry.elements.size() : geometry.meshFaces.size();
                    for (std::size_t index = 0; index < count; ++index) {
                        const int nCount = volume ? 4 : 3;
                        Vec center{};
                        QVector<int> ids;
                        bool valid = true;
                        for (int kNode = 0; kNode < nCount; ++kNode) {
                            const Index n = volume ? geometry.elements[index][kNode] : geometry.meshFaces[index][kNode];
                            if (n >= geometry.nodes.size()) { valid = false; break; }
                            for (int k = 0; k < 3; ++k) center[k] += geometry.nodes[n][k] / double(nCount);
                            ids.append(geometry.nodeIds.value(n, int(n + 1)));
                        }
                        if (!valid || !rectangle.contains(pv.project(center))) continue;
                        PickResult pick;
                        pick.kind = boxKind;
                        pick.partId = partId;
                        pick.index = int(index + 1);
                        pick.position = pick.hitPosition = center;
                        pick.nodeIds = ids;
                        if (boxKind == PickKind::MeshFace) {
                            const auto& f = geometry.meshFaces[index];
                            pick.direction = cross(subtract(geometry.nodes[f[1]], geometry.nodes[f[0]]),
                                                   subtract(geometry.nodes[f[2]], geometry.nodes[f[0]]));
                            pick.hasDirection = normalise(pick.direction);
                        }
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::Edge) {
                    geometry.cad->ensureCurves();
                    for (int index = 0; index < int(geometry.cad->curves.size()); ++index) {
                        const auto& curve = geometry.cad->curves[std::size_t(index)];
                        if (curve.points.empty() || !rectangle.intersects(pv.screenBounds(curve.bounds))) continue;
                        const Vec& midSample = curve.points[curve.points.size() / 2];
                        if (!rectangle.contains(pv.project(midSample))) continue;
                        try {
                            BRepAdaptor_Curve exact(curve.edge);
                            const double midP = (exact.FirstParameter() + exact.LastParameter()) * 0.5;
                            gp_Pnt p; gp_Vec tan;
                            exact.D1(midP, p, tan);
                            PickResult pick;
                            pick.kind = PickKind::Edge;
                            pick.partId = partId;
                            pick.index = index + 1;
                            pick.position = pick.hitPosition = vec(p);
                            pick.direction = {tan.X(), tan.Y(), tan.Z()};
                            if (exact.GetType() == GeomAbs_Circle) {
                                pick.position = vec(exact.Circle().Location());
                                const auto axis = exact.Circle().Axis().Direction();
                                pick.direction = {axis.X(), axis.Y(), axis.Z()};
                            } else if (exact.GetType() == GeomAbs_Ellipse) {
                                pick.position = vec(exact.Ellipse().Location());
                                const auto axis = exact.Ellipse().Axis().Direction();
                                pick.direction = {axis.X(), axis.Y(), axis.Z()};
                            }
                            pick.hasDirection = normalise(pick.direction);
                            appendUniquePick(pick);
                        } catch (const Standard_Failure&) {}
                    }
                } else if (boxKind == PickKind::Face) {
                    geometry.cad->ensureShapes();
                    for (int faceId = 1; faceId <= geometry.cad->faces.Extent(); ++faceId) {
                        const Vec center = geometry.cad->faceCenter(faceId);
                        if (!rectangle.contains(pv.project(center))) continue;
                        PickResult pick;
                        pick.kind = PickKind::Face;
                        pick.partId = partId;
                        pick.index = faceId;
                        pick.position = pick.hitPosition = center;
                        appendUniquePick(pick);
                    }
                } else if (boxKind == PickKind::Solid) {
                    geometry.cad->ensureShapes();
                    for (int solidId = 1; solidId <= geometry.cad->solids.Extent(); ++solidId) {
                        const Vec center = geometry.cad->solidCenter(solidId);
                        if (!rectangle.contains(pv.project(center))) continue;
                        PickResult pick;
                        pick.kind = PickKind::Solid;
                        pick.partId = partId;
                        pick.index = solidId;
                        pick.position = pick.hitPosition = center;
                        appendUniquePick(pick);
                    }
                }
            }
        }
        interruptPointerOperation();
        if (!s.pickActive) {
            if (boxKind == PickKind::Part) {
                const int nextSelected = selection.isEmpty() ? -1 : selection.back().partId;
                selectPart(nextSelected);
                if (selection.isEmpty() && previousCount > 0) emit partPicked(-1);
            }
            emit viewportSelectionChanged();
        } else if (selection.size() != previousCount) {
            emit pickSelectionChanged(selection);
        }
        update();
        event->accept();
        return;
    }

    const bool changedMode = s.navigationMode == MouseNavigationMode::Nx && s.button == Qt::MiddleButton &&
                            s.updateNxNavigation(event->modifiers(), event->localPos(), size());
    if (!changedMode) s.dragTo(event->localPos(), size());
    if (s.pickingRotationCenter && !s.dragging) {
        const auto releasedButton = s.button;
        const QPointF releasePos = event->localPos();
        interruptPointerOperation();
        event->accept();
        if (releasedButton == Qt::LeftButton) {
            if (setRotationCenterAt(releasePos)) {
                cancelPickRotationCenter();
            }
        } else if (releasedButton == Qt::RightButton) {
            cancelPickRotationCenter();
        }
        return;
    }
    const bool leftClick = s.button == Qt::LeftButton && !s.dragging && s.pickEnabled;
    const bool rightClick = s.button == Qt::RightButton && !s.dragging && s.pickEnabled;
    const bool ctrlHeld = event->modifiers().testFlag(Qt::ControlModifier);
    const bool typedSelection = s.pickActive || s.selectionFilter != PickKind::Part;
    const QPointF releasePos = event->localPos();
    s.rememberCamera();
    interruptPointerOperation();
    event->accept();

    if (leftClick) {
        if (!typedSelection) {
            // 拾取部件模式：Ctrl+左键单选加；普通左键单选替换
            const int id = pickPart(releasePos);
            if (ctrlHeld) {
                if (id > 0) {
                    seedSelectedPartIfNeeded();
                    const auto foundGeom = s.pickGeometry.find(id);
                    PickResult hit;
                    hit.kind = PickKind::Part;
                    hit.partId = id;
                    if (foundGeom != s.pickGeometry.end())
                        hit.position = hit.hitPosition = foundGeom->second.cad->partCenter();
                    if (std::none_of(s.viewportSelection.cbegin(), s.viewportSelection.cend(),
                                     [&](const auto& p) { return samePick(p, hit); })) {
                        s.viewportSelection.append(hit);
                    }
                    selectPart(id);
                    emit viewportSelectionChanged();
                }
                update();
                return;
            }
            if (id >= 0) {
                clearViewportSelection();
                selectPart(id);
                emit partPicked(id);
            } else {
                clearViewportSelection();
            }
            update();
            return;
        }

        const auto hit = pickAt(releasePos);
        auto& selection = s.pickActive ? s.picks : s.viewportSelection;
        const bool appendMode = s.pickActive ? s.pickMultiple : ctrlHeld;
        if (hit.isValid()) {
            const int previousCount = selection.size();
            if (!appendMode) selection.clear();
            const auto found = std::find_if(selection.begin(), selection.end(), [&](const auto& p) { return samePick(p, hit); });
            const bool added = found == selection.end();
            if (added) selection.append(hit);
            s.hover = hit;
            if (!s.pickActive) emit viewportSelectionChanged();
            else if (added || !s.pickMultiple || selection.size() != previousCount) emit pickSelectionChanged(selection);
        } else if (!s.pickActive && !ctrlHeld) {
            clearViewportSelection();
        }
        update();
        return;
    }

    if (rightClick) {
        if (ctrlHeld) {
            // Ctrl + 右键单选减：从当前选择集中移除光标命中的目标对象
            if (!typedSelection) {
                const int id = pickPart(releasePos);
                if (id > 0) {
                    if (s.viewportSelection.isEmpty() && s.selected == id) {
                        selectPart(-1);
                        emit partPicked(-1);
                    } else if (!s.viewportSelection.isEmpty()) {
                        const int beforeCount = s.viewportSelection.size();
                        s.viewportSelection.erase(std::remove_if(s.viewportSelection.begin(), s.viewportSelection.end(),
                            [id](const PickResult& p) { return p.kind == PickKind::Part && p.partId == id; }),
                            s.viewportSelection.end());
                        if (s.viewportSelection.size() != beforeCount) {
                            const int nextSelected = s.viewportSelection.isEmpty() ? -1 : s.viewportSelection.back().partId;
                            selectPart(nextSelected);
                            if (s.viewportSelection.isEmpty()) emit partPicked(-1);
                            else emit viewportSelectionChanged();
                        }
                    }
                }
                update();
                return;
            }
            auto& selection = s.pickActive ? s.picks : s.viewportSelection;
            if (!selection.isEmpty()) {
                const int previousCount = selection.size();
                const auto hit = pickAt(releasePos);
                if (hit.isValid()) {
                    selection.erase(std::remove_if(selection.begin(), selection.end(),
                        [&](const auto& p) { return samePick(p, hit); }), selection.end());
                }
                if (selection.size() != previousCount) {
                    if (s.pickActive) emit pickSelectionChanged(selection);
                    else emit viewportSelectionChanged();
                }
            }
            update();
            return;
        }
        if (s.pickActive) {
            // 参数拾取会话内保留原界面右键单步撤回/减选兼容行为
            auto& selection = s.picks;
            const int previousCount = selection.size();
            if (editsNodes && !selection.isEmpty()) {
                const auto hit = pickAt(releasePos);
                const auto found = std::find_if(selection.begin(), selection.end(), [&](const auto& p) { return samePick(p, hit); });
                if (found != selection.end()) selection.erase(found);
            } else if (!selection.isEmpty()) {
                const auto hit = pickAt(releasePos);
                const auto found = hit.isValid()
                    ? std::find_if(selection.begin(), selection.end(), [&](const auto& p) { return samePick(p, hit); })
                    : selection.end();
                if (found != selection.end()) selection.erase(found);
                else selection.removeLast();
            }
            if (selection.size() != previousCount) emit pickSelectionChanged(selection);
            update();
        }
    }
}

bool OccViewport::findRotationCenterPoint(const QPointF& position, Vec& worldPoint) const
{
    const auto& s = *m_state;
    if (!s.bounds.valid || !QRectF(rect()).contains(position)) return false;
    const auto frame = s.camera.frame(size(), s.bounds);
    const double unit = s.camera.mmPerPixel(size());
    const QPointF eye((position.x() - width()*0.5)*unit - s.camera.pan.x(),
                      (height()*0.5 - position.y())*unit - s.camera.pan.y());
    const Vec pivot = vec(s.camera.pivot);
    Ray world{};
    for (int k = 0; k < 3; ++k) {
        world.origin[k] = pivot[k] + frame.rows[0][k]*eye.x() + frame.rows[1][k]*eye.y() + frame.rows[2][k]*frame.nearZ;
        world.direction[k] = -frame.rows[2][k];
    }

    double nearestSurface = frame.nearZ - frame.farZ;
    bool hasSurfaceHit = false;
    if (s.mode != DisplayMode::Wireframe) {
        for (const auto& part : s.parts) {
            if (part.suppressed) continue;
            const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
            const auto ray = restRay(world, s.poseFor(part.id));
            if (raycastGeometry(ray, geometry, nearestSurface, unit * 0.01)) {
                hasSurfaceHit = true;
            }
        }
    }

    const FastProjector proj(s.camera, size(), s.bounds);
    const double maxDepth = hasSurfaceHit ? (nearestSurface + 6.0 * unit) : (frame.nearZ - frame.farZ);
    const auto depthOfWorld = [&](const Vec& w) {
        return frame.nearZ - dot(frame.rows[2], subtract(w, pivot));
    };

    // 1. 优先吸附光标附近 8px 内的可见 CAD 顶点 / 有限元节点
    double bestVertexOffset2 = 8.0 * 8.0;
    double bestVertexDepth = maxDepth;
    bool foundVertex = false;
    Vec bestVertex{};
    for (const auto& part : s.parts) {
        if (part.suppressed) continue;
        const auto* pose = s.poseFor(part.id);
        const auto& displayGeom = part.usesMesh(s.mode) ? part.mesh : *part.cad;
        if (!intersectsBounds(restRay(world, pose), displayGeom.bounds, maxDepth, 8.0 * unit)) continue;
        const auto pv = proj.forPart(pose);
        const auto considerRestPoint = [&](const Vec& restPt) {
            if (!std::isfinite(restPt[0])) return;
            const double depth = pv.depth(restPt);
            if (depth < 0.0 || depth > maxDepth) return;
            const QPointF offset = position - pv.project(restPt);
            const double offset2 = QPointF::dotProduct(offset, offset);
            if (offset2 > bestVertexOffset2 || (std::abs(offset2 - bestVertexOffset2) < 1e-8 && depth >= bestVertexDepth))
                return;
            bestVertexOffset2 = offset2;
            bestVertexDepth = depth;
            bestVertex = pose ? pose->relativePoint(restPt, {}) : restPt;
            foundVertex = true;
        };
        const auto pickIt = s.pickGeometry.find(part.id);
        if (!part.deformed && pickIt != s.pickGeometry.end()) {
            if (part.usesMesh(s.mode) && !pickIt->second.nodes.empty()) {
                for (const auto& node : pickIt->second.nodes) considerRestPoint(node);
            } else if (pickIt->second.cad) {
                pickIt->second.cad->ensureVertices();
                for (const auto& vp : pickIt->second.cad->vertexPoints) considerRestPoint(vp);
            }
        } else if (part.deformed) {
            for (std::size_t i = 0; i < part.mesh.vertices.size(); ++i)
                considerRestPoint(part.mesh.position(Index(i)));
        }
    }
    if (foundVertex) {
        worldPoint = bestVertex;
        return true;
    }

    // 2. 其次吸附光标附近 6px 内的可见特征边 / 线框边（靠近中点自动吸附中点）
    double bestEdgeOffset2 = 6.0 * 6.0;
    double bestEdgeDepth = maxDepth;
    bool foundEdge = false;
    Vec bestEdge{};
    for (const auto& part : s.parts) {
        if (part.suppressed) continue;
        const auto& geometry = part.usesMesh(s.mode) ? part.mesh : *part.cad;
        if (geometry.edges.empty() || (!showsEdges(s.mode, geometry) && s.mode != DisplayMode::Wireframe)) continue;
        const auto* pose = s.poseFor(part.id);
        if (!intersectsBounds(restRay(world, pose), geometry.bounds, maxDepth, 6.0 * unit)) continue;
        for (std::size_t i = 0; i + 1 < geometry.edges.size(); i += 2) {
            const Vec ra = geometry.position(geometry.edges[i]);
            const Vec rb = geometry.position(geometry.edges[i + 1]);
            const Vec wa = pose ? pose->relativePoint(ra, {}) : ra;
            const Vec wb = pose ? pose->relativePoint(rb, {}) : rb;
            const QPointF sa = projectPoint(wa);
            const QPointF sb = projectPoint(wb);
            const QPointF delta = sb - sa;
            const double len2 = QPointF::dotProduct(delta, delta);
            if (len2 < 1e-12) continue;
            double t = std::clamp(QPointF::dotProduct(position - sa, delta) / len2, 0.0, 1.0);
            const QPointF offset = position - (sa + delta * t);
            const double offset2 = QPointF::dotProduct(offset, offset);
            if (offset2 > bestEdgeOffset2) continue;
            if (std::abs(t - 0.5) * std::sqrt(len2) <= 5.0) t = 0.5;
            const Vec w{wa[0]*(1.0 - t) + wb[0]*t, wa[1]*(1.0 - t) + wb[1]*t, wa[2]*(1.0 - t) + wb[2]*t};
            const double depth = depthOfWorld(w);
            if (depth < 0.0 || depth > maxDepth ||
                (std::abs(offset2 - bestEdgeOffset2) < 1e-8 && depth >= bestEdgeDepth)) continue;
            bestEdgeOffset2 = offset2;
            bestEdgeDepth = depth;
            bestEdge = w;
            foundEdge = true;
        }
    }
    if (foundEdge) {
        worldPoint = bestEdge;
        return true;
    }

    // 3. 表面三角网格射线精确求交点
    if (!hasSurfaceHit) return false;
    for (int k = 0; k < 3; ++k) worldPoint[k] = world.origin[k] + world.direction[k] * nearestSurface;
    return true;
}

bool OccViewport::setRotationCenterAt(const QPointF& position)
{
    auto& s = *m_state;
    Vec point{};
    if (!findRotationCenterPoint(position, point)) return false;
    s.rememberCamera();
    s.camera.setPivot(point);
    s.rotationCenterMode = RotationCenterMode::CustomPoint;
    s.rememberCamera();
    pulsePivotIndicator();
    updateCamera();
    emit rotationCenterChanged(rotationCenter());
    return true;
}

void OccViewport::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // Do not double-commit a collector or replace its geometry reference.
        if (!m_state->pickActive && !m_state->pickingRotationCenter &&
            (m_state->button == Qt::NoButton || m_state->button == Qt::LeftButton)) {
            interruptPointerOperation();
            // 双击模型表面/边/顶点设置旋转中心且画面不跳变；双击空白或 Ctrl+双击自适应全屏并恢复包围盒中心
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
    if (event->key() == Qt::Key_Escape && s.pickingRotationCenter) {
        cancelPickRotationCenter(); event->accept(); return;
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
        (m_state->button != Qt::NoButton || m_state->pickingRotationCenter || m_state->pickActive)) {
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
    m_state->hover = {};
    m_state->hoverPivotValid = false;
    update();
    QOpenGLWidget::leaveEvent(event);
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
        if (s.pickingRotationCenter) {
            s.hoverPivotValid = findRotationCenterPoint(position, s.hoverPivotPoint);
        } else {
            s.hover = s.pickActive ? pickAt(position) : PickResult{};
        }
        updateCamera();
    }
    event->accept();
}

} // namespace MotionOcc
