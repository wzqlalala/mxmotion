#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "render/OccRenderShaders.h"

#include <QApplication>
#include <QMouseEvent>
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
};

struct Geometry {
    std::vector<Vertex> vertices;
    std::vector<Index> triangles;
    std::vector<Index> edges;
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
    std::set<int> partIds; // includes hidden document parts
    std::map<int, PartPose> poses; // only non-identity poses
    std::map<int, Geometry> restMeshes;
    std::set<int> dirtyMeshParts;
    Bounds bounds;
    Camera camera;
    DisplayMode mode = DisplayMode::ShadedEdges;
    int selected = -1;
    bool pickEnabled = true;

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
}

OccViewport::~OccViewport() { cleanupGL(); }

void OccViewport::setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                           const std::set<int>& hidden)
{
    auto& s = *m_state;
    std::vector<Part> parts;
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
        Bounds rest = copy.cad.bounds;
        rest.merge(copy.mesh.bounds);
        const Vec origin = rest.center();
        copy.origin = QVector3D(float(origin[0]), float(origin[1]), float(origin[2]));
        parts.push_back(std::move(copy));
    }
    s.parts = std::move(parts);
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
    }
    event->accept();
}

void OccViewport::mouseMoveEvent(QMouseEvent* event)
{
    auto& s = *m_state;
    if (s.button == Qt::NoButton || !(event->buttons() & s.button)) {
        s.button = Qt::NoButton;
        QOpenGLWidget::mouseMoveEvent(event);
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
    s.dragTo(event->localPos(), size());
    const bool click = s.button == Qt::LeftButton && !s.dragging && s.pickEnabled;
    s.button = Qt::NoButton;
    if (s.dragging) update();
    event->accept();
    if (click) {
        const int id = pickPart(event->localPos());
        selectPart(id);
        emit partPicked(id);
    }
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
