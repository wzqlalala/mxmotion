#include "NativeMnfBindingView.h"

#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionSymbols.h"
#include "OccViewport.h"
#include "RigidMotion.h"

#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <utility>

namespace MotionOcc {
namespace {

void addPointGlyphs(FemMesh& mesh, double size)
{
    const auto points = std::move(mesh.nodes);
    mesh.nodes.clear();
    mesh.nodes.reserve(points.size() * 4);
    mesh.surface.reserve(points.size() * 4);
    for (const auto& [x, y, z] : points) {
        const auto i = static_cast<std::uint32_t>(mesh.nodes.size());
        mesh.nodes.push_back({x + size, y + size, z + size});
        mesh.nodes.push_back({x - size, y - size, z + size});
        mesh.nodes.push_back({x - size, y + size, z - size});
        mesh.nodes.push_back({x + size, y - size, z - size});
        mesh.surface.push_back({i, i + 2, i + 1});
        mesh.surface.push_back({i, i + 1, i + 3});
        mesh.surface.push_back({i, i + 3, i + 2});
        mesh.surface.push_back({i + 1, i + 2, i + 3});
    }
}

FemMesh restMesh(const RigidFlexResult& preview)
{
    FemMesh mesh;
    mesh.nodes.reserve(preview.referenceNodes.size() / 3);
    mesh.surface.reserve(preview.surfaceTriangles.size() / 3);
    for (std::size_t i = 0; i + 2 < preview.referenceNodes.size(); i += 3)
        mesh.nodes.push_back({preview.referenceNodes[i], preview.referenceNodes[i + 1], preview.referenceNodes[i + 2]});
    for (std::size_t i = 0; i + 2 < preview.surfaceTriangles.size(); i += 3) {
        const auto a = static_cast<std::uint32_t>(preview.surfaceTriangles[i]);
        const auto b = static_cast<std::uint32_t>(preview.surfaceTriangles[i + 1]);
        const auto c = static_cast<std::uint32_t>(preview.surfaceTriangles[i + 2]);
        if (a < mesh.nodes.size() && b < mesh.nodes.size() && c < mesh.nodes.size() && a != b && b != c && a != c)
            mesh.surface.push_back({a, b, c});
    }
    return mesh;
}

double meshSpan(const FemMesh& mesh)
{
    auto lo = mesh.nodes.front(), hi = lo;
    for (const auto& node : mesh.nodes) {
        for (int axis = 0; axis < 3; ++axis) {
            lo[axis] = std::min(lo[axis], node[axis]);
            hi[axis] = std::max(hi[axis], node[axis]);
        }
    }
    return std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
}

} // namespace

NativeMnfBindingView::NativeMnfBindingView(const RigidFlexResult& preview, QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("mnfBindingView"));
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto mesh = restMesh(preview);
    CadDocument cad;
    QString error;
    if (mesh.nodes.empty() || !cad.addPrimitive(Primitive::Box, 1, 1, 1, m_partId, error)) {
        auto* label = new QLabel(error.isEmpty() ? tr("MNF 中没有可预览的节点。") : error, this);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignCenter);
        layout->addWidget(label);
        return;
    }
    const double span = meshSpan(mesh);
    m_markerSize = std::max(0.01, span * .019);
    for (const auto& group : preview.modalBasis.attachmentGroups) {
        for (int index : group) {
            const auto node = static_cast<std::size_t>(index);
            if (node < mesh.nodes.size() && node < preview.sourceNodeIds.size())
                m_interfaceNodes.emplace(preview.sourceNodeIds[node], mesh.nodes[node]);
        }
    }
    if (mesh.surface.empty())
        addPointGlyphs(mesh, std::max(1e-6, span * .0017));

    m_viewport = new OccViewport(this);
    m_viewport->setObjectName(QStringLiteral("mnfBindingViewport"));
    m_viewport->setMinimumSize(300, 230);
    m_viewport->setPickEnabled(false);
    m_viewport->setToolTip(tr("单击蓝色接口节点。紫色十字标出运动副原点，橙色标出当前节点。"));
    m_viewport->installEventFilter(this);
    layout->addWidget(m_viewport, 1);
    auto* legend = new QLabel(tr("蓝色 接口节点　　紫色 运动副原点　　橙色 当前节点"), this);
    layout->addWidget(legend);
    // This synthetic slot belongs only to the preview's local scene.
    m_viewport->setScene(cad, {});
    m_restMeshes.emplace(m_partId, std::move(mesh));
    m_viewport->setDeformedMeshes(m_restMeshes, {});
    m_viewport->viewIsometric();
    m_viewport->fitAll();
    updateSymbols();
}

void NativeMnfBindingView::setPlacement(const QMatrix4x4& mnfLocalToWorld)
{
    if (!m_viewport || m_localToWorld == mnfLocalToWorld) return;
    m_localToWorld = mnfLocalToWorld;
    m_viewport->setPartPoses({{m_partId, mnfLocalToWorld}});
    // The viewport clears display overrides when poses return to identity.
    if (mnfLocalToWorld.isIdentity()) m_viewport->setDeformedMeshes(m_restMeshes, {});
    m_viewport->fitAllIncluding(m_pivot);
    updateSymbols();
}

void NativeMnfBindingView::setActiveJoint(const QString& name, std::optional<Point> worldPivot, int selectedNodeId)
{
    if (m_jointName == name && m_pivot == worldPivot && m_selectedNodeId == selectedNodeId) return;
    if (m_viewport && worldPivot && worldPivot != m_pivot &&
        !m_viewport->rect().adjusted(16, 16, -16, -16).contains(m_viewport->projectPoint(*worldPivot).toPoint()))
        m_viewport->fitAllIncluding(worldPivot);
    m_jointName = name;
    m_pivot = worldPivot;
    m_selectedNodeId = selectedNodeId;
    updateSymbols();
}

void NativeMnfBindingView::setNodePickedCallback(std::function<void(int)> callback)
{
    m_nodePicked = std::move(callback);
}

NativeMnfBindingView::Point NativeMnfBindingView::worldPoint(const Point& local) const
{
    Point point{};
    for (int i = 0; i < 3; ++i) {
        point[i] = m_localToWorld(i, 3);
        for (int j = 0; j < 3; ++j)
            point[i] += m_localToWorld(i, j) * local[j];
    }
    return point;
}

void NativeMnfBindingView::updateSymbols()
{
    if (!m_viewport) return;
    QVector<MotionSymbol> symbols;
    const QColor available(13, 125, 202);
    const QColor selected(211, 91, 13);
    const auto current = m_interfaceNodes.find(m_selectedNodeId);
    for (const auto& [id, point] : m_interfaceNodes) {
        if (id == m_selectedNodeId) continue;
        MotionSymbol node;
        node.origin = point;
        node.partId = m_partId;
        node.color = available;
        symbols.append(std::move(node));
    }
    if (m_pivot) {
        MotionSymbol joint;
        joint.origin = *m_pivot;
        joint.color = QColor(142, 73, 177);
        joint.emphasized = true;
        joint.drawOrigin = false;
        if (current == m_interfaceNodes.end())
            joint.label = m_jointName;
        for (int axis = 0; axis < 3; ++axis) {
            auto first = joint.origin, last = joint.origin;
            first[axis] -= m_markerSize;
            last[axis] += m_markerSize;
            joint.linePoints.push_back(first);
            joint.linePoints.push_back(last);
        }
        symbols.append(std::move(joint));
    }
    if (current != m_interfaceNodes.end()) {
        MotionSymbol node;
        node.origin = worldPoint(current->second);
        node.emphasized = true;
        node.color = selected;
        node.label = tr("节点 %1").arg(current->first);
        if (m_pivot) node.linePoints = {*m_pivot, node.origin};
        symbols.append(std::move(node));
    }
    m_viewport->setMotionSymbols(symbols);
}

bool NativeMnfBindingView::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_viewport) return QWidget::eventFilter(watched, event);
    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        const auto* mouse = static_cast<QMouseEvent*>(event);
        m_clickStart.reset();
        if (mouse->button() == Qt::LeftButton && mouse->buttons() == Qt::LeftButton &&
            mouse->modifiers() == Qt::NoModifier)
            m_clickStart = mouse->localPos();
        break;
    }
    case QEvent::MouseMove: {
        const auto* mouse = static_cast<QMouseEvent*>(event);
        if (m_clickStart && (mouse->localPos() - *m_clickStart).manhattanLength() >= QApplication::startDragDistance())
            m_clickStart.reset();
        break;
    }
    case QEvent::MouseButtonRelease: {
        const auto* mouse = static_cast<QMouseEvent*>(event);
        const auto start = std::exchange(m_clickStart, std::nullopt);
        if (start && mouse->button() == Qt::LeftButton && mouse->modifiers() == Qt::NoModifier &&
            (mouse->localPos() - *start).manhattanLength() < QApplication::startDragDistance())
            pickNode(mouse->localPos());
        break;
    }
    case QEvent::MouseButtonDblClick:
    case QEvent::UngrabMouse:
    case QEvent::FocusOut:
        m_clickStart.reset();
        break;
    default:
        break;
    }
    return QWidget::eventFilter(watched, event);
}

void NativeMnfBindingView::pickNode(const QPointF& point)
{
    if (!m_nodePicked || !m_pivot) return;
    int best = -1;
    double closest = 9.0;
    for (const auto& [id, local] : m_interfaceNodes) {
        const QPointF delta = point - m_viewport->projectPoint(local, m_partId);
        const double distance = std::hypot(delta.x(), delta.y());
        if (distance < closest) {
            best = id;
            closest = distance;
        }
    }
    if (best >= 0) m_nodePicked(best);
}

} // namespace MotionOcc
