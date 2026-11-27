#include "MultiphysicsSchematicDialog.h"
#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"

#include <QAbstractButton>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLabel>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <tuple>

namespace MotionOcc {
namespace {

struct PinSpec {
    QString key; // "node", "portANodeId", "portBNodeId", "portPNodeId", "portTNodeId", "suctionNodeId", "deliveryNodeId", "targetNodeId", "actuatorEndpoint", "inletNodeId", "outletNodeId", "dcOut", "supplyId", "acOut", "inverterId"
    QString label;
    QPointF localPos;
    bool isHydNode = false;
    bool isSupplyOut = false;
    bool isInverterDcIn = false;
    bool isInverterAcOut = false;
    bool isActuatorAcIn = false;
    bool isPortConnActuatorEnd = false;
};

struct BlockKey {
    QString collection;
    int id = -1;
    bool operator<(const BlockKey& o) const {
        return collection != o.collection ? collection < o.collection : id < o.id;
    }
    bool operator==(const BlockKey& o) const {
        return collection == o.collection && id == o.id;
    }
    bool operator!=(const BlockKey& o) const {
        return !(*this == o);
    }
};

QString blockTypeTitle(const QString& col, const QJsonObject& row)
{
    if (col == "hydraulicNodes") {
        const QString t = row.value("nodeType").toString("Chamber");
        return t == "Tank" ? QStringLiteral("油箱") : t == "PressureSource" ? QStringLiteral("压力源") : QStringLiteral("液压容腔");
    }
    if (col == "hydraulicComponents") {
        static const QMap<QString, QString> m{
            {"Pump", QStringLiteral("定量泵")}, {"FlowSource", QStringLiteral("流量源")},
            {"DirectionalValve", QStringLiteral("换向阀")}, {"ServoValve", QStringLiteral("电液伺服阀")},
            {"ReliefValve", QStringLiteral("溢流阀")}, {"PressureReducingValve", QStringLiteral("减压阀")},
            {"SequenceValve", QStringLiteral("顺序阀")}, {"CheckValve", QStringLiteral("单向阀")},
            {"ThrottleValve", QStringLiteral("节流阀")}, {"Pipe", QStringLiteral("液压管路")},
            {"Filter", QStringLiteral("过滤器")}, {"Accumulator", QStringLiteral("蓄能器")},
            {"PortConnection", QStringLiteral("端口连接")}};
        return m.value(row.value("componentType").toString(), QStringLiteral("液压元件"));
    }
    if (col == "hydraulicCylinders") return QStringLiteral("液压缸");
    if (col == "hydraulicMotors") return QStringLiteral("液压马达");
    if (col == "powerSupplies") return QStringLiteral("直流电源");
    if (col == "inverters") return QStringLiteral("伺服逆变器");
    if (col == "electricMotors") return QStringLiteral("伺服电机");
    if (col == "ehaActuators") return QStringLiteral("EHA作动器");
    if (col == "emaActuators") return QStringLiteral("EMA作动器");
    return col;
}

QString formatPortConnDetail(const NativeMotionDocument& doc, const QJsonObject& row)
{
    const int nid = row.value("targetNodeId").toInt(-1);
    const QString nodeName = nid >= 0
        ? doc.entity("hydraulicNodes", nid).value("name").toString(QStringLiteral("节点#%1").arg(nid))
        : QStringLiteral("待选节点");
    const QString actType = row.value("actuatorType").toString("Cylinder");
    const QString actCol = actType == "Motor" ? QStringLiteral("hydraulicMotors") : QStringLiteral("hydraulicCylinders");
    const int aid = row.value("actuatorId").toInt(-1);
    const QString port = row.value("actuatorPort").toString("A");
    const QString actName = aid >= 0
        ? doc.entity(actCol, aid).value("name").toString(QStringLiteral("执行器#%1").arg(aid)) + QStringLiteral(".") + port
        : QStringLiteral("待选油口");
    return QStringLiteral("%1 ⇄ %2").arg(nodeName, actName);
}

double sampleChannel(const RigidResult* res, const QString& col, int id, const QString& var, int frameIdx, bool* ok = nullptr)
{
    if (ok) *ok = false;
    if (!res || frameIdx < 0 || std::size_t(frameIdx) >= res->frames.size()) return 0.0;
    for (const auto& ch : res->channels) {
        if (ch.sourceType == col && ch.sourceId == id && ch.variable == var && ch.components == 1
            && std::size_t(frameIdx) < ch.values.size()) {
            if (ok) *ok = true;
            return ch.values[std::size_t(frameIdx)];
        }
    }
    return 0.0;
}

QString formatTelemetryBadge(const RigidResult* res, const QString& col, int id, int frameIdx)
{
    if (!res || res->frames.empty()) return {};
    bool ok1 = false, ok2 = false;
    if (col == "hydraulicNodes") {
        const double p = sampleChannel(res, col, id, QStringLiteral("Pressure"), frameIdx, &ok1);
        const double q = sampleChannel(res, col, id, QStringLiteral("NetFlow"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 MPa | %2 L/m").arg(p, 0, 'f', 2).arg(ok2 ? q : 0.0, 0, 'f', 1);
    } else if (col == "hydraulicComponents") {
        const double q = sampleChannel(res, col, id, QStringLiteral("Flow"), frameIdx, &ok1);
        const double gp = sampleChannel(res, col, id, QStringLiteral("GasPressure"), frameIdx, &ok2);
        if (ok2) return QStringLiteral("%1 MPa | %2 L/m").arg(gp, 0, 'f', 2).arg(q, 0, 'f', 1);
        const double dp = sampleChannel(res, col, id, QStringLiteral("DeltaPressure"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 L/m | Δ%2 MPa").arg(q, 0, 'f', 1).arg(dp, 0, 'f', 2);
    } else if (col == "hydraulicCylinders") {
        const double s = sampleChannel(res, col, id, QStringLiteral("Stroke"), frameIdx, &ok1);
        const double f = sampleChannel(res, col, id, QStringLiteral("Force"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 mm | %2 N").arg(s, 0, 'f', 1).arg(f, 0, 'f', 0);
    } else if (col == "hydraulicMotors") {
        const double n = sampleChannel(res, col, id, QStringLiteral("Speed"), frameIdx, &ok1);
        const double t = sampleChannel(res, col, id, QStringLiteral("Torque"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 rpm | %2 N·m").arg(n, 0, 'f', 1).arg(t * 1e-3, 0, 'f', 1);
    } else if (col == "powerSupplies") {
        const double v = sampleChannel(res, col, id, QStringLiteral("BusVoltage"), frameIdx, &ok1);
        const double i = sampleChannel(res, col, id, QStringLiteral("SourceCurrent"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 V | %2 A").arg(v, 0, 'f', 1).arg(i, 0, 'f', 1);
    } else if (col == "inverters") {
        const double v = sampleChannel(res, col, id, QStringLiteral("OutputVoltage"), frameIdx, &ok1);
        const double i = sampleChannel(res, col, id, QStringLiteral("PhaseCurrent"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 V | %2 A").arg(v, 0, 'f', 1).arg(i, 0, 'f', 1);
    } else if (col == "electricMotors") {
        const double n = sampleChannel(res, col, id, QStringLiteral("Speed"), frameIdx, &ok1);
        const double i = sampleChannel(res, col, id, QStringLiteral("Current"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 rpm | %2 A").arg(n, 0, 'f', 1).arg(i, 0, 'f', 1);
    } else if (col == "ehaActuators" || col == "emaActuators") {
        const double s = sampleChannel(res, col, id, QStringLiteral("Stroke"), frameIdx, &ok1);
        const double i = sampleChannel(res, col, id, QStringLiteral("Current"), frameIdx, &ok2);
        if (ok1) return QStringLiteral("%1 mm | %2 A").arg(s, 0, 'f', 1).arg(i, 0, 'f', 1);
    }
    return {};
}

QFont makeSchematicFont(int pixelSize, bool bold = false)
{
    QFont font(MxFunctions::UiMetrics::uiFontFamily());
    font.setPixelSize(pixelSize);
    font.setWeight(bold ? QFont::Bold : QFont::Normal);
    font.setStyleStrategy(static_cast<QFont::StyleStrategy>(QFont::PreferAntialias | QFont::PreferQuality));
    font.setHintingPreference(QFont::PreferNoHinting);
    return font;
}

QPainterPath makeCubicWirePath(const QPointF& p1, const QPointF& p2)
{
    QPainterPath path(p1);
    const double dx = std::max(36.0, std::abs(p2.x() - p1.x()) * 0.45);
    path.cubicTo(QPointF(p1.x() + (p2.x() >= p1.x() ? dx : -dx), p1.y()),
                 QPointF(p2.x() - (p2.x() >= p1.x() ? dx : -dx), p2.y()),
                 p2);
    return path;
}

// 将正交折线点序列转换为带圆角过渡的平滑工程原理图路径
QPainterPath buildRoundedOrthogonalPath(const QVector<QPointF>& rawPts, double cornerRadius = 8.0)
{
    QVector<QPointF> pts;
    pts.reserve(rawPts.size());
    for (const QPointF& p : rawPts) {
        if (pts.isEmpty() || QLineF(pts.back(), p).length() > 0.8) pts.push_back(p);
    }
    // 剔除共线冗余中间点
    for (int i = 1; i + 1 < pts.size();) {
        const QPointF d1 = pts[i] - pts[i - 1];
        const QPointF d2 = pts[i + 1] - pts[i];
        const bool sameHoriz = std::abs(d1.y()) < 0.8 && std::abs(d2.y()) < 0.8 && (d1.x() * d2.x() >= 0.0);
        const bool sameVert = std::abs(d1.x()) < 0.8 && std::abs(d2.x()) < 0.8 && (d1.y() * d2.y() >= 0.0);
        if (sameHoriz || sameVert) pts.removeAt(i);
        else ++i;
    }
    QPainterPath path;
    if (pts.isEmpty()) return path;
    path.moveTo(pts.front());
    if (pts.size() == 2) {
        path.lineTo(pts.back());
        return path;
    }
    for (int i = 1; i + 1 < pts.size(); ++i) {
        const QPointF prev = pts[i - 1];
        const QPointF curr = pts[i];
        const QPointF next = pts[i + 1];
        const QLineF inSeg(curr, prev);
        const QLineF outSeg(curr, next);
        const double r = std::min({cornerRadius, inSeg.length() * 0.45, outSeg.length() * 0.45});
        if (r < 1.2) {
            path.lineTo(curr);
            continue;
        }
        const QPointF enterPt = curr + (prev - curr) * (r / inSeg.length());
        const QPointF leavePt = curr + (next - curr) * (r / outSeg.length());
        path.lineTo(enterPt);
        path.quadTo(curr, leavePt);
    }
    path.lineTo(pts.back());
    return path;
}

enum class WireRouteKind {
    Standard,        // 常规前向或支路正交连接
    TankReturnBottom,// 回油母线（经下方水平通道直入油箱底边）
    TankReturnLeft,  // 跨层回油母线（经最底部回油总管与左侧垂直走廊汇入油箱左边）
    TierWrap         // 跨行级联（上一行末端经行间水平走廊接入下一行首端）
};

QPainterPath makeOrthogonalWirePath(const QPointF& p1, const QPointF& dir1,
                                    const QPointF& p2, const QPointF& dir2,
                                    WireRouteKind kind = WireRouteKind::Standard,
                                    double channelY = 0.0,
                                    double auxX = 0.0,
                                    double busFraction = 0.5)
{
    constexpr double stub = 18.0;
    QVector<QPointF> pts;
    pts.push_back(p1);

    if (kind == WireRouteKind::TankReturnBottom) {
        const QPointF s1 = p1 + dir1 * stub;
        pts.push_back(s1);
        pts.push_back(QPointF(s1.x(), channelY));
        pts.push_back(QPointF(p2.x(), channelY));
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts, 8.0);
    }

    if (kind == WireRouteKind::TankReturnLeft) {
        const QPointF s1 = p1 + dir1 * stub;
        pts.push_back(s1);
        pts.push_back(QPointF(s1.x(), channelY));
        pts.push_back(QPointF(auxX, channelY));
        pts.push_back(QPointF(auxX, p2.y()));
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts, 8.0);
    }

    if (kind == WireRouteKind::TierWrap) {
        const QPointF s1 = p1 + dir1 * 24.0;
        const QPointF s2 = p2 + dir2 * 24.0;
        pts.push_back(s1);
        pts.push_back(QPointF(s1.x(), channelY));
        pts.push_back(QPointF(s2.x(), channelY));
        pts.push_back(s2);
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts, 9.0);
    }

    // 垂直支路（如蓄能器底部直连母管顶部）
    if (std::abs(dir1.y()) > 0.5 && std::abs(dir2.y()) > 0.5) {
        if (std::abs(p1.x() - p2.x()) < 1.5 && (p2.y() - p1.y()) * dir1.y() > 0.0) {
            pts.push_back(p2);
            return buildRoundedOrthogonalPath(pts);
        }
        const double midY = 0.5 * (p1.y() + p2.y());
        pts.push_back(QPointF(p1.x(), midY));
        pts.push_back(QPointF(p2.x(), midY));
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts);
    }

    // 一端水平、另一端垂直（如溢流阀左入油口接上方高压母管底部，或油箱底部接支路）
    if (std::abs(dir1.x()) > 0.5 && std::abs(dir2.y()) > 0.5) {
        const QPointF s1 = p1 + dir1 * stub;
        const QPointF s2 = p2 + dir2 * stub;
        if ((p2.x() - p1.x()) * dir1.x() > stub && (p1.y() - p2.y()) * dir2.y() > stub) {
            pts.push_back(QPointF(p2.x(), p1.y()));
        } else {
            pts.push_back(s1);
            pts.push_back(QPointF(s1.x(), s2.y()));
            pts.push_back(s2);
        }
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts);
    }
    if (std::abs(dir1.y()) > 0.5 && std::abs(dir2.x()) > 0.5) {
        const QPointF s1 = p1 + dir1 * stub;
        const QPointF s2 = p2 + dir2 * stub;
        if ((p1.x() - p2.x()) * dir2.x() > stub && (p2.y() - p1.y()) * dir1.y() > stub) {
            pts.push_back(QPointF(p1.x(), p2.y()));
        } else {
            pts.push_back(s1);
            pts.push_back(QPointF(s2.x(), s1.y()));
            pts.push_back(s2);
        }
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts);
    }

    // 标准水平对水平端口连接
    if (dir1.x() > 0.5 && dir2.x() < -0.5) {
        if (p2.x() >= p1.x() + 14.0) {
            if (std::abs(p1.y() - p2.y()) < 1.5) {
                pts.push_back(p2);
            } else {
                const double midX = p1.x() + std::clamp(busFraction, 0.25, 0.75) * (p2.x() - p1.x());
                pts.push_back(QPointF(midX, p1.y()));
                pts.push_back(QPointF(midX, p2.y()));
                pts.push_back(p2);
            }
            return buildRoundedOrthogonalPath(pts);
        }
        // 目标位于源左侧时自动上下绕行避开自身卡片
        const QPointF s1 = p1 + dir1 * stub;
        const QPointF s2 = p2 + dir2 * stub;
        const double detourY = (p2.y() >= p1.y()) ? (std::max(p1.y(), p2.y()) + 62.0)
                                                  : (std::min(p1.y(), p2.y()) - 62.0);
        pts.push_back(s1);
        pts.push_back(QPointF(s1.x(), detourY));
        pts.push_back(QPointF(s2.x(), detourY));
        pts.push_back(s2);
        pts.push_back(p2);
        return buildRoundedOrthogonalPath(pts);
    }

    // 反向水平连接（右→左）或同侧端口连接
    if (dir1.x() < -0.5 && dir2.x() > 0.5 && p2.x() <= p1.x() - 14.0) {
        if (std::abs(p1.y() - p2.y()) < 1.5) {
            pts.push_back(p2);
        } else {
            const double midX = 0.5 * (p1.x() + p2.x());
            pts.push_back(QPointF(midX, p1.y()));
            pts.push_back(QPointF(midX, p2.y()));
            pts.push_back(p2);
        }
        return buildRoundedOrthogonalPath(pts);
    }

    const QPointF s1 = p1 + dir1 * stub;
    const QPointF s2 = p2 + dir2 * stub;
    const double midY = 0.5 * (s1.y() + s2.y());
    pts.push_back(s1);
    pts.push_back(QPointF(s1.x(), midY));
    pts.push_back(QPointF(s2.x(), midY));
    pts.push_back(s2);
    pts.push_back(p2);
    return buildRoundedOrthogonalPath(pts);
}

class SchematicView final : public QGraphicsView {
public:
    std::function<void(const QPointF& scenePos)> onSceneMouseMove;
    std::function<bool()> onRightClickCancel;

    explicit SchematicView(QGraphicsScene* scene, QWidget* parent = nullptr) : QGraphicsView(scene, parent) {
        setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setMouseTracking(true);
        viewport()->setMouseTracking(true);
        const bool dark = MxFunctions::UiPalette::theme() == MxFunctions::UiPalette::Theme::Dark;
        setBackgroundBrush(dark ? MxFunctions::UiPalette::workspace() : QColor(246, 248, 251));
    }
protected:
    void mouseMoveEvent(QMouseEvent* event) override {
        if (onSceneMouseMove) onSceneMouseMove(mapToScene(event->pos()));
        QGraphicsView::mouseMoveEvent(event);
    }
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::RightButton && onRightClickCancel && onRightClickCancel()) {
            event->accept();
            return;
        }
        QGraphicsView::mousePressEvent(event);
    }
    void wheelEvent(QWheelEvent* event) override {
        const double factor = event->angleDelta().y() > 0 ? 1.14 : (1.0 / 1.14);
        const double curScale = transform().m11();
        if ((factor > 1.0 && curScale < 3.5) || (factor < 1.0 && curScale > 0.25)) {
            scale(factor, factor);
        }
        event->accept();
    }
    void drawBackground(QPainter* painter, const QRectF& rect) override {
        QGraphicsView::drawBackground(painter, rect);
        painter->save();
        const bool dark = MxFunctions::UiPalette::theme() == MxFunctions::UiPalette::Theme::Dark;
        QPen minorPen(dark ? MxFunctions::UiPalette::border().darker(120) : QColor(232, 236, 242), 1.0);
        painter->setPen(minorPen);
        const double step = 24.0;
        const double left = std::floor(rect.left() / step) * step;
        const double top = std::floor(rect.top() / step) * step;
        for (double x = left; x < rect.right(); x += step) painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
        for (double y = top; y < rect.bottom(); y += step) painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
        painter->restore();
    }
};

class SchematicBlockItem final : public QGraphicsObject {
public:
    BlockKey key;
    QJsonObject row;
    QString title;
    QString subtitle;
    QString extraHint;
    QString telemetry;
    int severity = 0; // 0 none, 1 warning, 2 error
    bool treeHighlighted = false;
    bool groupHighlighted = false;
    bool dimmed = false;
    bool nodePickCandidate = false;
    QColor accentColor{34, 118, 178};
    QVector<PinSpec> pins;
    QString activePinKey;
    QString hoveredPinKey;
    QSet<QString> candidatePinKeys;

    std::function<void(const BlockKey&, const QPointF&)> onMovedCommit;
    std::function<void(const BlockKey&)> onClicked;
    std::function<void(const BlockKey&)> onDoubleClicked;
    std::function<bool(const BlockKey&, const QString& pinKey)> onPinClicked;

    SchematicBlockItem(BlockKey k, const QJsonObject& r, const NativeMotionDocument& doc)
        : key(std::move(k)), row(r) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setAcceptHoverEvents(true);
        configurePinsAndAppearance(doc);
    }

    QRectF boundingRect() const override {
        return QRectF(-118, -58, 236, 126);
    }

    QRectF cardBodyRect() const {
        if (key.collection == "hydraulicNodes") {
            const QString nt = row.value("nodeType").toString("Chamber");
            return (nt == "Tank" || nt == "PressureSource") ? QRectF(-74, -26, 148, 52) : QRectF(-72, -25, 144, 50);
        }
        return QRectF(-96, -38, 192, 76);
    }

    QPointF scenePinPos(const QString& pinKey) const {
        for (const auto& p : pins) if (p.key == pinKey) return mapToScene(p.localPos);
        return scenePos();
    }

    QPointF pinOutwardDir(const QString& pinKey) const {
        for (const auto& p : pins) {
            if (p.key != pinKey) continue;
            if (p.localPos.x() < -20.0) return QPointF(-1.0, 0.0);
            if (p.localPos.x() > 20.0) return QPointF(1.0, 0.0);
            if (p.localPos.y() > 15.0) return QPointF(0.0, 1.0);
            if (p.localPos.y() < -15.0) return QPointF(0.0, -1.0);
        }
        return QPointF(1.0, 0.0);
    }

    // forcedSide: 0 自动按相对方位选择外缘, 1 底边, -1 顶边, -2 左边, 2 右边
    std::pair<QPointF, QPointF> sceneHydNodeEdgeAnchor(const QPointF& otherPos,
                                                       int forcedSide = 0,
                                                       double slotOffset = 0.0) const {
        const QRectF r = cardBodyRect();
        const QPointF c = scenePos();
        if (forcedSide == 1) {
            const double xOff = std::clamp(slotOffset, -r.width() * 0.34, r.width() * 0.34);
            return {c + QPointF(xOff, r.bottom()), QPointF(0.0, 1.0)};
        }
        if (forcedSide == -1) {
            const double xOff = std::clamp(slotOffset, -r.width() * 0.34, r.width() * 0.34);
            return {c + QPointF(xOff, r.top()), QPointF(0.0, -1.0)};
        }
        if (forcedSide == -2) {
            const double yOff = std::clamp(slotOffset, -r.height() * 0.30, r.height() * 0.30);
            return {c + QPointF(r.left(), yOff), QPointF(-1.0, 0.0)};
        }
        if (forcedSide == 2) {
            const double yOff = std::clamp(slotOffset, -r.height() * 0.30, r.height() * 0.30);
            return {c + QPointF(r.right(), yOff), QPointF(1.0, 0.0)};
        }
        const double dx = otherPos.x() - c.x();
        const double dy = otherPos.y() - c.y();
        if (std::abs(dy) > 64.0 && std::abs(dx) < 92.0) {
            const double xOff = std::clamp(slotOffset, -r.width() * 0.30, r.width() * 0.30);
            if (dy < 0.0) return {c + QPointF(xOff, r.top()), QPointF(0.0, -1.0)};
            return {c + QPointF(xOff, r.bottom()), QPointF(0.0, 1.0)};
        }
        const double yOff = std::clamp(slotOffset, -r.height() * 0.28, r.height() * 0.28);
        if (dx < 0.0) return {c + QPointF(r.left(), yOff), QPointF(-1.0, 0.0)};
        return {c + QPointF(r.right(), yOff), QPointF(1.0, 0.0)};
    }

    void configurePinsAndAppearance(const NativeMotionDocument& doc) {
        title = row.value("name").toString();
        subtitle = blockTypeTitle(key.collection, row);
        extraHint.clear();
        pins.clear();
        setToolTip(title.isEmpty() ? subtitle : QStringLiteral("%1（%2）").arg(title, subtitle));

        if (key.collection == "hydraulicNodes") {
            const QString nt = row.value("nodeType").toString("Chamber");
            accentColor = nt == "Tank" ? QColor(43, 123, 186) : nt == "PressureSource" ? QColor(210, 68, 68) : QColor(28, 138, 168);
            PinSpec p{"node", nt == "Tank" ? QStringLiteral("T") : QStringLiteral("P"), QPointF(0, 0), true};
            pins.push_back(p);
        } else if (key.collection == "hydraulicComponents") {
            accentColor = QColor(34, 118, 178);
            const QString ct = row.value("componentType").toString();
            if (ct == "Pump") {
                pins.push_back({"suctionNodeId", QStringLiteral("吸"), QPointF(-96, 4)});
                pins.push_back({"deliveryNodeId", QStringLiteral("压"), QPointF(96, 4)});
            } else if (ct == "DirectionalValve" || ct == "ServoValve") {
                pins.push_back({"portPNodeId", QStringLiteral("P"), QPointF(-96, -11)});
                pins.push_back({"portTNodeId", QStringLiteral("T"), QPointF(-96, 18)});
                pins.push_back({"portANodeId", QStringLiteral("A"), QPointF(96, -11)});
                pins.push_back({"portBNodeId", QStringLiteral("B"), QPointF(96, 18)});
            } else if (ct == "Accumulator") {
                pins.push_back({"targetNodeId", QStringLiteral("口"), QPointF(0, 38)});
            } else if (ct == "PortConnection") {
                accentColor = QColor(210, 118, 32);
                const QString actPort = row.value("actuatorPort").toString("A") == "B"
                    ? QStringLiteral("油口B") : QStringLiteral("油口A");
                pins.push_back({"targetNodeId", QStringLiteral("节点"), QPointF(-96, 0)});
                PinSpec pAct{"actuatorEndpoint", actPort, QPointF(96, 0)};
                pAct.isPortConnActuatorEnd = true;
                pins.push_back(pAct);
                extraHint = formatPortConnDetail(doc, row);
            } else {
                pins.push_back({"inletNodeId", QStringLiteral("入"), QPointF(-96, 4)});
                pins.push_back({"outletNodeId", QStringLiteral("出"), QPointF(96, 4)});
            }
        } else if (key.collection == "hydraulicCylinders" || key.collection == "hydraulicMotors") {
            accentColor = QColor(18, 112, 166);
            pins.push_back({"portANodeId", QStringLiteral("A"), QPointF(-96, -11)});
            pins.push_back({"portBNodeId", QStringLiteral("B"), QPointF(-96, 18)});
        } else if (key.collection == "powerSupplies") {
            accentColor = QColor(132, 82, 198);
            PinSpec p{"dcOut", QStringLiteral("DC+"), QPointF(96, 4)};
            p.isSupplyOut = true;
            pins.push_back(p);
        } else if (key.collection == "inverters") {
            accentColor = QColor(42, 152, 102);
            PinSpec pIn{"supplyId", QStringLiteral("DC"), QPointF(-96, 4)};
            pIn.isInverterDcIn = true;
            PinSpec pOut{"acOut", QStringLiteral("UVW"), QPointF(96, 4)};
            pOut.isInverterAcOut = true;
            pins.push_back(pIn);
            pins.push_back(pOut);
        } else if (key.collection == "electricMotors" || key.collection == "ehaActuators" || key.collection == "emaActuators") {
            accentColor = key.collection == "electricMotors" ? QColor(38, 156, 98) : QColor(142, 92, 205);
            PinSpec pIn{"inverterId", QStringLiteral("电"), QPointF(-96, 4)};
            pIn.isActuatorAcIn = true;
            pins.push_back(pIn);
        }
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->save();
        painter->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
        if (dimmed) painter->setOpacity(0.38);

        const int bodyPx = MxFunctions::UiMetrics::bodyFontSize();
        const int helperPx = MxFunctions::UiMetrics::helperFontSize();
        const bool isNode = key.collection == "hydraulicNodes";
        const QRectF bodyRect = cardBodyRect();
        const bool selectedLook = isSelected() || treeHighlighted;

        if (nodePickCandidate) {
            painter->setPen(QPen(QColor(16, 185, 129), 2.6, Qt::DashLine));
            painter->setBrush(QColor(16, 185, 129, 36));
            const qreal r = isNode ? 26.0 : 12.0;
            painter->drawRoundedRect(bodyRect.adjusted(-6, -6, 6, 6), r, r);
        } else if (selectedLook || groupHighlighted) {
            const QColor glow = selectedLook ? QColor(24, 144, 255, 72) : QColor(24, 144, 255, 42);
            painter->setPen(QPen(QColor(24, 144, 255), selectedLook ? 2.4 : 1.6));
            painter->setBrush(glow);
            const qreal r = isNode ? 26.0 : 12.0;
            painter->drawRoundedRect(bodyRect.adjusted(-5, -5, 5, 5), r, r);
        } else if (severity > 0) {
            const QColor halo = severity == 2 ? QColor(220, 53, 69, 85) : QColor(240, 158, 36, 90);
            painter->setPen(QPen(halo.darker(115), 2.2));
            painter->setBrush(halo);
            const qreal r = isNode ? 25.0 : 11.0;
            painter->drawRoundedRect(bodyRect.adjusted(-5, -5, 5, 5), r, r);
        }

        painter->setPen(QPen(selectedLook ? QColor(22, 96, 212) : accentColor.darker(115), selectedLook ? 2.5 : 1.5));
        painter->setBrush(QColor(255, 255, 255));
        painter->drawRoundedRect(bodyRect, isNode ? 22 : 8, isNode ? 22 : 8);

        if (!isNode) {
            QPainterPath header;
            header.addRoundedRect(QRectF(bodyRect.left(), bodyRect.top(), bodyRect.width(), 22), 8, 8);
            painter->fillPath(header, accentColor);
            painter->fillRect(QRectF(bodyRect.left(), bodyRect.top() + 14, bodyRect.width(), 8), accentColor);
            painter->setPen(Qt::white);
            painter->setFont(makeSchematicFont(bodyPx, true));
            painter->drawText(QRectF(bodyRect.left() + 8, bodyRect.top() + 1, bodyRect.width() - 16, 20),
                              Qt::AlignCenter, subtitle);
        }

        painter->setPen(QColor(30, 41, 59));
        const QFont pinFont = makeSchematicFont(helperPx, true);
        const QFontMetrics pinFm(pinFont);
        double leftPad = 12.0;
        double rightPad = 12.0;
        double bottomPad = 6.0;
        for (const auto& p : pins) {
            if (p.isHydNode) continue;
            const double w = pinFm.horizontalAdvance(p.label) + 16.0;
            if (p.localPos.x() < -20.0) leftPad = std::max(leftPad, w);
            else if (p.localPos.x() > 20.0) rightPad = std::max(rightPad, w);
            else if (p.localPos.y() > 20.0) bottomPad = std::max(bottomPad, 20.0);
        }

        painter->setFont(makeSchematicFont(bodyPx, isNode || selectedLook));
        const QFontMetrics fm = painter->fontMetrics();
        if (isNode) {
            const QRectF nameRect(bodyRect.left() + 7, bodyRect.top() + 5, bodyRect.width() - 14, 20);
            painter->drawText(nameRect, Qt::AlignCenter,
                              fm.elidedText(title, Qt::ElideRight, int(nameRect.width())));
        } else if (!extraHint.isEmpty()) {
            const QRectF nameRect(bodyRect.left() + leftPad, bodyRect.top() + 24,
                                  bodyRect.width() - leftPad - rightPad, 22);
            painter->drawText(nameRect, Qt::AlignCenter,
                              fm.elidedText(title, Qt::ElideRight, int(nameRect.width())));
        } else {
            const QRectF nameRect(bodyRect.left() + leftPad, bodyRect.top() + 24,
                                  bodyRect.width() - leftPad - rightPad, 52.0 - bottomPad);
            if (fm.horizontalAdvance(title) <= nameRect.width()) {
                painter->drawText(nameRect, Qt::AlignCenter, title);
            } else {
                // 优先在下划线处断行，避免把 _OD1 / _EHA 等后缀劈开成两行
                QString displayTitle = title;
                const int usIdx = displayTitle.lastIndexOf(QLatin1Char('_'));
                if (usIdx > 2 && usIdx + 1 < displayTitle.size()
                    && fm.horizontalAdvance(displayTitle.left(usIdx)) <= nameRect.width()
                    && fm.horizontalAdvance(displayTitle.mid(usIdx)) <= nameRect.width()) {
                    displayTitle = displayTitle.left(usIdx) + QLatin1Char('\n') + displayTitle.mid(usIdx);
                    painter->drawText(nameRect, Qt::AlignCenter, displayTitle);
                } else {
                    const QString twoLine = fm.elidedText(title, Qt::ElideRight, int(nameRect.width() * 2.0 - 8.0));
                    painter->drawText(nameRect, Qt::AlignCenter | Qt::TextWordWrap | Qt::TextWrapAnywhere, twoLine);
                }
            }
        }

        if (isNode) {
            painter->setFont(makeSchematicFont(helperPx, false));
            painter->setPen(nodePickCandidate ? QColor(5, 150, 105) : accentColor.darker(120));
            painter->drawText(QRectF(bodyRect.left() + 8, bodyRect.top() + 25, bodyRect.width() - 16, 17), Qt::AlignCenter,
                              nodePickCandidate ? QStringLiteral("点击选择此节点") : subtitle);
        } else if (!extraHint.isEmpty()) {
            painter->setFont(makeSchematicFont(helperPx - 1, false));
            const bool incomplete = extraHint.contains(QStringLiteral("待选"));
            painter->setPen(incomplete ? QColor(180, 83, 9) : QColor(71, 85, 105));
            const QRectF hintRect(bodyRect.left() + 6, bodyRect.top() + 52, bodyRect.width() - 12, 18);
            painter->drawText(hintRect, Qt::AlignCenter,
                              painter->fontMetrics().elidedText(extraHint, Qt::ElideMiddle, int(hintRect.width())));
        }

        if (!telemetry.isEmpty()) {
            painter->setFont(makeSchematicFont(helperPx, true));
            const QRectF badge(-76, bodyRect.bottom() + 4, 152, 18);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(25, 35, 48, 225));
            painter->drawRoundedRect(badge, 4, 4);
            painter->setPen(QColor(138, 232, 184));
            painter->drawText(badge, Qt::AlignCenter, telemetry);
        }

        // 绘制连接端口及可拾取状态光圈
        painter->setFont(makeSchematicFont(helperPx, true));
        for (const auto& p : pins) {
            if (p.isHydNode) continue;
            const bool activePin = (p.key == activePinKey);
            const bool candidatePin = candidatePinKeys.contains(p.key);
            const bool hoveredPin = (p.key == hoveredPinKey);

            if (activePin || candidatePin || hoveredPin) {
                const QColor ringCol = activePin ? QColor(245, 158, 11)
                                     : candidatePin ? QColor(16, 185, 129)
                                                    : QColor(37, 99, 235);
                painter->setPen(QPen(ringCol, 2.0));
                painter->setBrush(QColor(ringCol.red(), ringCol.green(), ringCol.blue(), 48));
                painter->drawEllipse(p.localPos, 10.0, 10.0);
            }

            const qreal pinRadius = (activePin || candidatePin || hoveredPin) ? 7.2 : 6.0;
            painter->setPen(QPen(QColor(30, 40, 52), 1.3));
            painter->setBrush(activePin ? QColor(255, 193, 7)
                            : candidatePin ? QColor(16, 185, 129)
                            : hoveredPin ? QColor(59, 130, 246)
                                         : accentColor.lighter(135));
            painter->drawEllipse(p.localPos, pinRadius, pinRadius);

            const QRectF lblRect(p.localPos.x() < 0 ? p.localPos.x() + 10
                                                    : (p.localPos.x() > 0 ? p.localPos.x() - 46 : -18),
                                 p.localPos.x() == 0 ? p.localPos.y() - 22 : p.localPos.y() - 8,
                                 36, 16);
            painter->setPen(candidatePin ? QColor(5, 150, 105) : QColor(51, 65, 85));
            const Qt::Alignment align = p.localPos.x() < 0 ? (Qt::AlignLeft | Qt::AlignVCenter)
                                      : p.localPos.x() > 0 ? (Qt::AlignRight | Qt::AlignVCenter)
                                                           : Qt::AlignCenter;
            painter->drawText(lblRect, align, p.label);
        }
        painter->restore();
    }

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override {
        QString hitPin;
        for (const auto& p : pins) {
            if (p.isHydNode) continue;
            if (QLineF(event->pos(), p.localPos).length() <= 12.0) {
                hitPin = p.key;
                break;
            }
        }
        if (hitPin != hoveredPinKey) {
            hoveredPinKey = hitPin;
            setCursor(!hitPin.isEmpty() || nodePickCandidate ? Qt::PointingHandCursor : Qt::OpenHandCursor);
            update();
        }
        QGraphicsObject::hoverMoveEvent(event);
    }

    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override {
        if (!hoveredPinKey.isEmpty()) {
            hoveredPinKey.clear();
            unsetCursor();
            update();
        }
        QGraphicsObject::hoverLeaveEvent(event);
    }

    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && onPinClicked) {
            for (const auto& p : pins) {
                const double r = p.isHydNode ? 34.0 : 13.0;
                if (QLineF(event->pos(), p.localPos).length() <= r) {
                    if (onPinClicked(key, p.key)) {
                        event->accept();
                        return;
                    }
                }
            }
        }
        m_pressPos = pos();
        m_pressScenePos = event->scenePos();
        QGraphicsObject::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        QGraphicsObject::mouseReleaseEvent(event);
        const bool moved = QLineF(m_pressPos, pos()).length() > 1.5;
        if (moved && onMovedCommit) {
            onMovedCommit(key, pos());
        } else if (!moved && event->button() == Qt::LeftButton
                   && QLineF(m_pressScenePos, event->scenePos()).length() <= 4.0 && onClicked) {
            onClicked(key);
        }
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        if (onDoubleClicked) onDoubleClicked(key);
        event->accept();
    }

private:
    QPointF m_pressPos;
    QPointF m_pressScenePos;
};

struct WireRecord {
    QGraphicsPathItem* item = nullptr;
    QGraphicsPathItem* halo = nullptr;
    BlockKey endpointA;
    BlockKey endpointB;
};

} // namespace

struct MultiphysicsSchematicDialog::Impl {
    NativeMotionDocument doc;
    const RigidResult* solvedResult = nullptr;
    int frameIndex = 0;
    Callbacks callbacks;

    QGraphicsScene* scene = nullptr;
    SchematicView* view = nullptr;
    QPushButton* wireModeBtn = nullptr;
    QFrame* pickBanner = nullptr;
    QLabel* pickBannerLabel = nullptr;
    QPushButton* cancelPickBtn = nullptr;
    QLabel* statusLabel = nullptr;
    QSlider* frameSlider = nullptr;
    QLabel* frameTimeLabel = nullptr;
    bool initialFitDone = false;
    bool initialLayoutChecked = false;
    std::set<BlockKey> verifiedEntitySet;

    std::map<BlockKey, SchematicBlockItem*> blockItems;
    std::vector<WireRecord> wireItems;
    QGraphicsPathItem* rubberBandItem = nullptr;
    QPointF lastMouseScenePos{0.0, 0.0};
    MotionDiagnosticReport lastReport;

    // 当前树/画布选中态
    QString selectedCollection;
    int selectedId = -1;

    // 端口直接连线态
    bool hasPendingPin = false;
    BlockKey pendingBlock;
    QString pendingPin;

    // 参数面板发起的可视化拾取态
    PickRequest activePick;

    void putEntity(const QString& col, const QJsonObject& updated) {
        auto data = doc.root.value("motionData").toObject();
        QJsonArray arr = data.value(col).toArray();
        const int id = updated.value("id").toInt(-1);
        bool found = false;
        for (int i = 0; i < arr.size(); ++i) {
            if (arr[i].toObject().value("id").toInt(-1) == id) {
                arr[i] = updated;
                found = true;
                break;
            }
        }
        if (!found) arr.append(updated);
        data.insert(col, arr);
        doc.root.insert("motionData", data);
    }

    int allocateId(const QString& col) const {
        int maxId = 0;
        for (const auto& v : doc.collection(col))
            maxId = std::max(maxId, v.toObject().value("id").toInt(0));
        return maxId + 1;
    }

    QString pinDisplayName(const BlockKey& bk, const QString& pinKey) const {
        if (blockItems.count(bk)) {
            for (const auto& p : blockItems.at(bk)->pins)
                if (p.key == pinKey) return p.label;
        }
        return pinKey;
    }

    bool isCompatibleWithPendingWire(const BlockKey& targetBlock, const QString& targetPin) const {
        if (!hasPendingPin) return false;
        if (targetBlock == pendingBlock && targetPin == pendingPin) return false;

        const auto isElecActuator = [](const QString& c) {
            return c == "electricMotors" || c == "ehaActuators" || c == "emaActuators";
        };
        const auto isHydActuator = [](const QString& c) {
            return c == "hydraulicCylinders" || c == "hydraulicMotors";
        };
        const auto isHydPortOwner = [&](const QString& c) {
            return c == "hydraulicComponents" || isHydActuator(c);
        };

        // 电源 <-> 逆变器
        if (pendingBlock.collection == "powerSupplies")
            return targetBlock.collection == "inverters" && targetPin == "supplyId";
        if (pendingBlock.collection == "inverters" && pendingPin == "supplyId")
            return targetBlock.collection == "powerSupplies";

        // 逆变器 <-> 电机 / EHA / EMA
        if (pendingBlock.collection == "inverters" && pendingPin == "acOut")
            return isElecActuator(targetBlock.collection) && targetPin == "inverterId";
        if (isElecActuator(pendingBlock.collection))
            return targetBlock.collection == "inverters" && targetPin == "acOut";

        // 端口连接右端 (actuatorEndpoint) <-> 液压缸 / 液压马达 A/B 口
        const bool firstIsPortConnAct = (pendingBlock.collection == "hydraulicComponents" && pendingPin == "actuatorEndpoint");
        const bool secondIsPortConnAct = (targetBlock.collection == "hydraulicComponents" && targetPin == "actuatorEndpoint");
        if (firstIsPortConnAct)
            return isHydActuator(targetBlock.collection) && (targetPin == "portANodeId" || targetPin == "portBNodeId");
        if (secondIsPortConnAct)
            return isHydActuator(pendingBlock.collection) && (pendingPin == "portANodeId" || pendingPin == "portBNodeId");

        // 液压节点 <-> 任意液压油口
        if (pendingBlock.collection == "hydraulicNodes")
            return isHydPortOwner(targetBlock.collection) && targetPin != "actuatorEndpoint";
        if (isHydPortOwner(pendingBlock.collection))
            return targetBlock.collection == "hydraulicNodes"
                   || (isHydPortOwner(targetBlock.collection) && targetBlock != pendingBlock && targetPin != "actuatorEndpoint");
        return false;
    }

    bool isCompatibleWithPickRequest(const BlockKey& targetBlock, const QString& targetPin) const {
        switch (activePick.target) {
        case PickTarget::HydraulicNode:
            return targetBlock.collection == "hydraulicNodes";
        case PickTarget::HydraulicActuatorPort:
            return (targetBlock.collection == "hydraulicCylinders" || targetBlock.collection == "hydraulicMotors")
                   && (targetPin == "portANodeId" || targetPin == "portBNodeId");
        case PickTarget::PowerSupply:
            return targetBlock.collection == "powerSupplies";
        case PickTarget::Inverter:
            return targetBlock.collection == "inverters";
        case PickTarget::None:
        default:
            return false;
        }
    }

    void updateBannerAndVisualStates() {
        const bool picking = (activePick.target != PickTarget::None);
        if (pickBanner) {
            if (picking) {
                QString targetDesc;
                switch (activePick.target) {
                case PickTarget::HydraulicNode:
                    targetDesc = QStringLiteral("请直接点击图中绿色高亮的【油箱 / 压力源 / 液压容腔】节点");
                    break;
                case PickTarget::HydraulicActuatorPort:
                    targetDesc = QStringLiteral("请直接点击液压缸或液压马达卡片左侧绿色高亮的【A】或【B】油口圆点");
                    break;
                case PickTarget::PowerSupply:
                    targetDesc = QStringLiteral("请直接点击图中绿色高亮的【直流电源】");
                    break;
                case PickTarget::Inverter:
                    targetDesc = QStringLiteral("请直接点击图中绿色高亮的【伺服逆变器】");
                    break;
                default:
                    break;
                }
                pickBannerLabel->setText(QStringLiteral("正在为【%1】选择目标 — %2（右键或 Esc 取消）")
                                             .arg(activePick.promptTitle, targetDesc));
                pickBanner->show();
            } else if (hasPendingPin) {
                const QString ownerName = doc.entity(pendingBlock.collection, pendingBlock.id).value("name").toString();
                pickBannerLabel->setText(
                    QStringLiteral("正在从【%1 · %2】引出连接 — 请点击图中绿色高亮的目标端口或节点完成连接（右键或 Esc 取消）")
                        .arg(ownerName, pinDisplayName(pendingBlock, pendingPin)));
                pickBanner->show();
            } else {
                pickBanner->hide();
            }
        }

        const bool groupSel = !selectedCollection.isEmpty() && selectedId < 0;
        for (auto& [bk, item] : blockItems) {
            item->activePinKey.clear();
            item->candidatePinKeys.clear();
            item->nodePickCandidate = false;
            item->treeHighlighted = (!selectedCollection.isEmpty() && selectedId >= 0
                                     && bk.collection == selectedCollection && bk.id == selectedId);
            item->groupHighlighted = (groupSel && bk.collection == selectedCollection);
            item->dimmed = false;

            if (picking) {
                const bool isOwner = (bk.collection == activePick.ownerCollection && bk.id == activePick.ownerId);
                if (isOwner) {
                    item->treeHighlighted = true;
                    item->activePinKey = activePick.ownerPinKey;
                }
                bool anyMatch = false;
                if (activePick.target == PickTarget::HydraulicNode && bk.collection == "hydraulicNodes") {
                    item->nodePickCandidate = true;
                    anyMatch = true;
                } else if (activePick.target == PickTarget::PowerSupply && bk.collection == "powerSupplies") {
                    item->nodePickCandidate = true;
                    item->candidatePinKeys.insert("dcOut");
                    anyMatch = true;
                } else if (activePick.target == PickTarget::Inverter && bk.collection == "inverters") {
                    item->nodePickCandidate = true;
                    item->candidatePinKeys.insert("acOut");
                    anyMatch = true;
                } else {
                    for (const auto& p : item->pins) {
                        if (isCompatibleWithPickRequest(bk, p.key)) {
                            item->candidatePinKeys.insert(p.key);
                            anyMatch = true;
                        }
                    }
                }
                item->dimmed = (!anyMatch && !isOwner);
            } else if (hasPendingPin) {
                if (bk == pendingBlock) {
                    item->activePinKey = pendingPin;
                    item->treeHighlighted = true;
                } else {
                    bool anyMatch = false;
                    for (const auto& p : item->pins) {
                        if (isCompatibleWithPendingWire(bk, p.key)) {
                            if (p.isHydNode) item->nodePickCandidate = true;
                            else item->candidatePinKeys.insert(p.key);
                            anyMatch = true;
                        }
                    }
                    item->dimmed = !anyMatch;
                }
            } else if (groupSel) {
                item->dimmed = !item->groupHighlighted;
            }
            item->setSelected(item->treeHighlighted);
            item->update();
        }

        // 更新连接线高亮态
        for (const auto& w : wireItems) {
            const bool endpointSelected = (!selectedCollection.isEmpty() && selectedId >= 0)
                && ((w.endpointA.collection == selectedCollection && w.endpointA.id == selectedId)
                    || (w.endpointB.collection == selectedCollection && w.endpointB.id == selectedId));
            if (w.halo) w.halo->setVisible(endpointSelected);
        }

        updateRubberBand(lastMouseScenePos);
    }

    void updateRubberBand(const QPointF& cursorScenePos) {
        lastMouseScenePos = cursorScenePos;
        if (!scene) return;

        bool showBand = false;
        QPointF startPos;
        QColor bandColor(24, 144, 255);

        if (hasPendingPin && blockItems.count(pendingBlock)) {
            startPos = blockItems.at(pendingBlock)->scenePinPos(pendingPin);
            showBand = true;
            bandColor = QColor(16, 185, 129);
        } else if (activePick.target != PickTarget::None && !activePick.ownerCollection.isEmpty() && activePick.ownerId >= 0) {
            const BlockKey ownerKey{activePick.ownerCollection, activePick.ownerId};
            if (blockItems.count(ownerKey)) {
                startPos = activePick.ownerPinKey.isEmpty()
                    ? blockItems.at(ownerKey)->scenePos()
                    : blockItems.at(ownerKey)->scenePinPos(activePick.ownerPinKey);
                showBand = true;
                bandColor = QColor(24, 144, 255);
            }
        }

        if (!showBand) {
            if (rubberBandItem) rubberBandItem->setVisible(false);
            return;
        }

        // 靠近兼容端口时自动磁吸预览终点
        QPointF endPos = cursorScenePos;
        double bestDist = 22.0;
        for (const auto& [bk, item] : blockItems) {
            if (item->nodePickCandidate) {
                const double d = QLineF(cursorScenePos, item->scenePos()).length();
                if (d < 36.0 && d < bestDist + 14.0) {
                    endPos = item->scenePos();
                    bestDist = d;
                }
            }
            for (const auto& p : item->pins) {
                if (!item->candidatePinKeys.contains(p.key)) continue;
                const QPointF pinScene = item->scenePinPos(p.key);
                const double d = QLineF(cursorScenePos, pinScene).length();
                if (d < bestDist) {
                    bestDist = d;
                    endPos = pinScene;
                }
            }
        }

        if (!rubberBandItem) {
            rubberBandItem = scene->addPath(QPainterPath(), QPen(bandColor, 2.4, Qt::DashLine, Qt::RoundCap));
            rubberBandItem->setZValue(20.0);
        }
        rubberBandItem->setPen(QPen(bandColor, 2.4, Qt::DashLine, Qt::RoundCap));
        rubberBandItem->setPath(makeCubicWirePath(startPos, endPos));
        rubberBandItem->setVisible(true);
    }

    void commitAndRefresh(const QString& statusMsg = {}) {
        if (callbacks.commitDocument) {
            NativeMotionDocument copy = doc;
            callbacks.commitDocument(std::move(copy));
        }
        if (callbacks.runDiagnostics) {
            lastReport = callbacks.runDiagnostics(doc);
        }
        rebuildScene(false);
        if (!statusMsg.isEmpty() && statusLabel) statusLabel->setText(statusMsg);
    }

    static QRectF staticCardRect(const QString& col, const QJsonObject& row) {
        if (col == "hydraulicNodes") {
            const QString nt = row.value("nodeType").toString("Chamber");
            return (nt == "Tank" || nt == "PressureSource") ? QRectF(-74, -26, 148, 52) : QRectF(-72, -25, 144, 50);
        }
        return QRectF(-96, -38, 192, 76);
    }

    static QRectF staticPaddedBox(const QString& col, const QJsonObject& row, double cx, double cy,
                                  double padX = 12.0, double padY = 10.0) {
        return staticCardRect(col, row).translated(cx, cy).adjusted(-padX, -padY, padX, padY + 6.0);
    }

    std::set<BlockKey> currentEnabledEntitySet() const {
        std::set<BlockKey> res;
        const QStringList allCols{
            "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
            "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
        };
        for (const auto& col : allCols) {
            for (const auto& v : doc.collection(col)) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                res.insert(BlockKey{col, o.value("id").toInt(-1)});
            }
        }
        return res;
    }

    QPointF findNonOverlappingSpot(const QString& col, const QJsonObject& row,
                                   const QPointF& preferred, const BlockKey& ignoreKey = {}) const {
        const QStringList allCols{
            "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
            "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
        };
        QVector<QRectF> occupied;
        for (const auto& c : allCols) {
            for (const auto& v : doc.collection(c)) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                const BlockKey bk{c, o.value("id").toInt(-1)};
                if (bk == ignoreKey || !o.contains("schemX") || !o.contains("schemY")) continue;
                occupied.push_back(staticPaddedBox(c, o, o.value("schemX").toDouble(), o.value("schemY").toDouble(), 16.0, 14.0));
            }
        }
        const auto isFree = [&](double x, double y) {
            const QRectF cand = staticPaddedBox(col, row, x, y, 16.0, 14.0);
            for (const QRectF& occ : occupied) {
                if (cand.intersects(occ)) return false;
            }
            return true;
        };
        const double gx = std::round(preferred.x() / 24.0) * 24.0;
        const double gy = std::round(preferred.y() / 24.0) * 24.0;
        if (isFree(gx, gy)) return QPointF(gx, gy);
        for (int ring = 1; ring <= 18; ++ring) {
            for (int dy = -ring; dy <= ring; ++dy) {
                for (int dx = -ring; dx <= ring; ++dx) {
                    if (std::abs(dx) != ring && std::abs(dy) != ring) continue;
                    const double cx = gx + dx * 108.0;
                    const double cy = gy + dy * 96.0;
                    if (isFree(cx, cy)) return QPointF(cx, cy);
                }
            }
        }
        return QPointF(gx, gy + 220.0);
    }

    bool hasSchematicLayoutIssues() const {
        const QStringList allCols{
            "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
            "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
        };
        struct Entry { BlockKey key; QJsonObject row; QRectF box; double x = 0.0; double y = 0.0; };
        QVector<Entry> entries;
        std::map<BlockKey, QPointF> posMap;
        for (const auto& col : allCols) {
            for (const auto& v : doc.collection(col)) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                if (!o.contains("schemX") || !o.contains("schemY")) return true;
                const double x = o.value("schemX").toDouble();
                const double y = o.value("schemY").toDouble();
                const BlockKey bk{col, o.value("id").toInt(-1)};
                posMap[bk] = QPointF(x, y);
                entries.push_back({bk, o, staticPaddedBox(col, o, x, y, 6.0, 6.0), x, y});
            }
        }
        // 检测任意两个卡片是否发生重叠
        for (int i = 0; i < entries.size(); ++i) {
            for (int j = i + 1; j < entries.size(); ++j) {
                if (entries[i].box.intersects(entries[j].box)) return true;
            }
        }
        // 检测蓄能器是否错误排在母管下方（蓄能器油口位于底边，排在下方会导致连线纵向穿透自身卡片）
        for (const auto& e : entries) {
            if (e.key.collection != "hydraulicComponents") continue;
            const QString ct = e.row.value("componentType").toString();
            if (ct == "Accumulator") {
                const BlockKey nk{"hydraulicNodes", e.row.value("targetNodeId").toInt(-1)};
                if (posMap.count(nk) && e.y > posMap.at(nk).y() - 36.0) return true;
            }
        }
        return false;
    }

    void resolveRemainingOverlaps() {
        const QStringList allCols{
            "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
            "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
        };
        struct ItemPos { BlockKey key; QJsonObject row; double x = 0.0; double y = 0.0; };
        QVector<ItemPos> items;
        for (const auto& col : allCols) {
            for (const auto& v : doc.collection(col)) {
                auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                items.push_back({BlockKey{col, o.value("id").toInt(-1)}, o,
                                 o.value("schemX").toDouble(0.0), o.value("schemY").toDouble(0.0)});
            }
        }
        bool anyAdjusted = false;
        for (int iter = 0; iter < 12; ++iter) {
            bool iterMoved = false;
            for (int i = 0; i < items.size(); ++i) {
                for (int j = i + 1; j < items.size(); ++j) {
                    const QRectF bi = staticPaddedBox(items[i].key.collection, items[i].row, items[i].x, items[i].y, 10.0, 8.0);
                    const QRectF bj = staticPaddedBox(items[j].key.collection, items[j].row, items[j].x, items[j].y, 10.0, 8.0);
                    if (!bi.intersects(bj)) continue;
                    const QRectF inter = bi.intersected(bj);
                    if (inter.width() < inter.height()) {
                        const double shift = 0.5 * inter.width() + 6.0;
                        if (items[i].x <= items[j].x) { items[i].x -= shift; items[j].x += shift; }
                        else { items[i].x += shift; items[j].x -= shift; }
                    } else {
                        const double shift = 0.5 * inter.height() + 6.0;
                        if (items[i].y <= items[j].y) { items[i].y -= shift; items[j].y += shift; }
                        else { items[i].y += shift; items[j].y -= shift; }
                    }
                    iterMoved = true;
                    anyAdjusted = true;
                }
            }
            if (!iterMoved) break;
        }
        if (anyAdjusted) {
            for (auto& it : items) {
                it.row.insert("schemX", std::round(it.x));
                it.row.insert("schemY", std::round(it.y));
                putEntity(it.key.collection, it.row);
            }
        }
    }

    void assignAutoLayout(bool overwriteExisting) {
        const std::set<BlockKey> curSet = currentEnabledEntitySet();
        const bool docSetChanged = (curSet != verifiedEntitySet && !initialLayoutChecked);
        bool forceFullLayout = overwriteExisting;
        if (!forceFullLayout && (!initialLayoutChecked || docSetChanged)) {
            if (hasSchematicLayoutIssues()) forceFullLayout = true;
        }
        initialLayoutChecked = true;
        verifiedEntitySet = curSet;

        if (!forceFullLayout) {
            // 仅为缺少坐标的新增图元分配不重叠的空位
            const QStringList allCols{
                "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
                "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
            };
            for (const auto& col : allCols) {
                for (const auto& v : doc.collection(col)) {
                    auto o = v.toObject();
                    if (!o.value("enabled").toBool(true)) continue;
                    if (!o.contains("schemX") || !o.contains("schemY")) {
                        const BlockKey bk{col, o.value("id").toInt(-1)};
                        const QPointF spot = findNonOverlappingSpot(col, o, QPointF(0.0, 0.0), bk);
                        o.insert("schemX", spot.x());
                        o.insert("schemY", spot.y());
                        putEntity(col, o);
                    }
                }
            }
            resolveRemainingOverlaps();
            return;
        }

        // =====================================================================
        // 工程级机电液拓扑分层与支路排布算法：
        // 1. 液压主回路按前向功率流（油箱/压力源 → 泵 → 母管 → 过滤/减压/管路 → 阀 → A/B腔 → 缸/马达）分层；
        // 2. 旁路元件按 ISO-1219 标准挂接：蓄能器排在目标母管正上方，安全溢流阀排在母管正下方；
        // 3. 超长串联链（>= 10 级）自动折叠为上下双层对齐泳道，保障视口缩放比与文字清晰度；
        // 4. 电气驱动域（直流电源 → 伺服逆变器 → 电机/EHA/EMA）独立成下方泳道，按驱动轴一一对齐。
        // =====================================================================
        std::map<BlockKey, QJsonObject> hydRows;
        std::set<int> tankNodeIds;
        std::set<int> sourceNodeIds;
        for (const auto& v : doc.collection("hydraulicNodes")) {
            auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            const int id = o.value("id").toInt(-1);
            hydRows[BlockKey{"hydraulicNodes", id}] = o;
            const QString nt = o.value("nodeType").toString("Chamber");
            if (nt == "Tank") tankNodeIds.insert(id);
            else if (nt == "PressureSource") sourceNodeIds.insert(id);
        }
        for (const auto& v : doc.collection("hydraulicComponents")) {
            auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            hydRows[BlockKey{"hydraulicComponents", o.value("id").toInt(-1)}] = o;
        }
        for (const char* col : {"hydraulicCylinders", "hydraulicMotors"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                hydRows[BlockKey{QLatin1String(col), o.value("id").toInt(-1)}] = o;
            }
        }

        std::map<int, QVector<BlockKey>> shuntAbove; // 挂在节点正上方（如蓄能器）
        std::map<int, QVector<BlockKey>> shuntBelow; // 挂在节点正下方（如回油箱的安全溢流阀）
        std::set<BlockKey> isShunt;

        for (const auto& [bk, o] : hydRows) {
            if (bk.collection != "hydraulicComponents") continue;
            const QString ct = o.value("componentType").toString();
            if (ct == "Accumulator") {
                const int nid = o.value("targetNodeId").toInt(-1);
                if (hydRows.count(BlockKey{"hydraulicNodes", nid})) {
                    shuntAbove[nid].push_back(bk);
                    isShunt.insert(bk);
                }
            } else if (ct == "ReliefValve") {
                const int inId = o.value("inletNodeId").toInt(-1);
                const int outId = o.value("outletNodeId").toInt(-1);
                if (hydRows.count(BlockKey{"hydraulicNodes", inId}) && !tankNodeIds.count(inId)
                    && (outId < 0 || tankNodeIds.count(outId))) {
                    shuntBelow[inId].push_back(bk);
                    isShunt.insert(bk);
                }
            }
        }

        struct FwdEdge { BlockKey from; BlockKey to; double portBias = 0.0; };
        QVector<FwdEdge> edges;
        const auto addFwdEdge = [&](const BlockKey& u, const BlockKey& v, double bias = 0.0) {
            if (!hydRows.count(u) || !hydRows.count(v) || isShunt.count(u) || isShunt.count(v) || u == v) return;
            // 回油箱仅作为源头列（Stage 0），不把排油回箱边计入前向级数
            if (v.collection == "hydraulicNodes" && tankNodeIds.count(v.id)) return;
            edges.push_back({u, v, bias});
        };

        for (const auto& [bk, o] : hydRows) {
            if (isShunt.count(bk)) continue;
            if (bk.collection == "hydraulicComponents") {
                const QString ct = o.value("componentType").toString();
                if (ct == "Pump" || ct == "FlowSource") {
                    const int sId = o.value("suctionNodeId").toInt(-1);
                    const int dId = o.value("deliveryNodeId").toInt(-1);
                    if (sId >= 0) addFwdEdge(BlockKey{"hydraulicNodes", sId}, bk);
                    if (dId >= 0) addFwdEdge(bk, BlockKey{"hydraulicNodes", dId});
                } else if (ct == "DirectionalValve" || ct == "ServoValve") {
                    const int pId = o.value("portPNodeId").toInt(-1);
                    const int aId = o.value("portANodeId").toInt(-1);
                    const int bId = o.value("portBNodeId").toInt(-1);
                    if (pId >= 0) addFwdEdge(BlockKey{"hydraulicNodes", pId}, bk);
                    if (aId >= 0) addFwdEdge(bk, BlockKey{"hydraulicNodes", aId}, -0.35);
                    if (bId >= 0) addFwdEdge(bk, BlockKey{"hydraulicNodes", bId}, 0.35);
                } else if (ct == "PortConnection") {
                    const int nid = o.value("targetNodeId").toInt(-1);
                    const QString actCol = o.value("actuatorType").toString("Cylinder") == "Motor"
                                               ? QStringLiteral("hydraulicMotors")
                                               : QStringLiteral("hydraulicCylinders");
                    const int aid = o.value("actuatorId").toInt(-1);
                    const double bias = o.value("actuatorPort").toString("A") == "B" ? 0.35 : -0.35;
                    if (nid >= 0) addFwdEdge(BlockKey{"hydraulicNodes", nid}, bk, bias);
                    if (aid >= 0) addFwdEdge(bk, BlockKey{actCol, aid}, bias);
                } else {
                    const int inId = o.value("inletNodeId").toInt(-1);
                    const int outId = o.value("outletNodeId").toInt(-1);
                    if (inId >= 0) addFwdEdge(BlockKey{"hydraulicNodes", inId}, bk);
                    if (outId >= 0) addFwdEdge(bk, BlockKey{"hydraulicNodes", outId});
                }
            } else if (bk.collection == "hydraulicCylinders" || bk.collection == "hydraulicMotors") {
                if (o.value("supplyMode").toString() == "ExternalCircuit") {
                    const int aId = o.value("portANodeId").toInt(-1);
                    const int bId = o.value("portBNodeId").toInt(-1);
                    if (aId >= 0) addFwdEdge(BlockKey{"hydraulicNodes", aId}, bk, -0.35);
                    if (bId >= 0) addFwdEdge(BlockKey{"hydraulicNodes", bId}, bk, 0.35);
                }
            }
        }

        std::map<BlockKey, int> stage;
        std::set<BlockKey> hasIncoming, hasOutgoing;
        for (const auto& [bk, o] : hydRows) {
            if (!isShunt.count(bk)) stage[bk] = 0;
        }
        for (const auto& e : edges) {
            hasOutgoing.insert(e.from);
            hasIncoming.insert(e.to);
        }
        const int maxRelax = int(stage.size()) + 2;
        for (int iter = 0; iter < maxRelax; ++iter) {
            bool changed = false;
            for (const auto& e : edges) {
                const int cand = std::min(maxRelax, stage[e.from] + 1);
                if (cand > stage[e.to]) {
                    stage[e.to] = cand;
                    changed = true;
                }
            }
            if (!changed) break;
        }

        int maxConnectedStage = 0;
        for (const auto& [bk, s] : stage) {
            if (hasIncoming.count(bk) || hasOutgoing.count(bk)) maxConnectedStage = std::max(maxConnectedStage, s);
        }
        // 为尚未连线的游离液压元件分配合理默认列，避免与油箱列堆叠
        for (auto& [bk, s] : stage) {
            if (hasIncoming.count(bk) || hasOutgoing.count(bk)) continue;
            if (bk.collection == "hydraulicNodes") {
                if (!tankNodeIds.count(bk.id) && !sourceNodeIds.count(bk.id)) s = std::min(2, maxConnectedStage + 1);
            } else if (bk.collection == "hydraulicComponents") {
                const QString ct = hydRows[bk].value("componentType").toString();
                if (ct == "Pump" || ct == "FlowSource") s = 1;
                else if (ct == "PortConnection") s = std::max(2, maxConnectedStage);
                else s = std::max(2, std::min(3, maxConnectedStage + 1));
            } else {
                s = std::max(3, maxConnectedStage + 1);
            }
        }

        int maxStage = 0;
        std::map<int, QVector<BlockKey>> stageBins;
        for (const auto& [bk, s] : stage) {
            stageBins[s].push_back(bk);
            maxStage = std::max(maxStage, s);
        }

        // 重心排序消除同级分支（如前/后伺服阀、A/B油腔、双液压缸）交叉
        std::map<BlockKey, double> orderScore;
        for (int s = 0; s <= maxStage; ++s) {
            auto& vec = stageBins[s];
            std::sort(vec.begin(), vec.end());
            for (int i = 0; i < vec.size(); ++i) orderScore[vec[i]] = double(i);
        }
        for (int pass = 0; pass < 4; ++pass) {
            for (int s = 1; s <= maxStage; ++s) {
                auto& vec = stageBins[s];
                std::map<BlockKey, double> bary;
                for (const auto& bk : vec) {
                    double sum = 0.0;
                    int cnt = 0;
                    for (const auto& e : edges) {
                        if (e.to == bk) {
                            sum += orderScore[e.from] + e.portBias;
                            ++cnt;
                        }
                    }
                    bary[bk] = cnt > 0 ? (sum / cnt) : (orderScore[bk] + 10.0);
                }
                std::stable_sort(vec.begin(), vec.end(), [&](const BlockKey& a, const BlockKey& b) {
                    if (std::abs(bary[a] - bary[b]) > 1e-4) return bary[a] < bary[b];
                    return a < b;
                });
                for (int i = 0; i < vec.size(); ++i) orderScore[vec[i]] = double(i);
            }
        }

        std::map<BlockKey, QPointF> placedHyd;
        double maxHydBottomY = -200.0;
        const bool hasHyd = !hydRows.empty();
        const bool hasElec = !doc.collection("powerSupplies").isEmpty()
                          || !doc.collection("inverters").isEmpty()
                          || !doc.collection("electricMotors").isEmpty()
                          || !doc.collection("ehaActuators").isEmpty()
                          || !doc.collection("emaActuators").isEmpty();

        if (hasHyd) {
            const bool twoTier = (maxStage >= 9);
            int splitStage = maxStage + 1;
            if (twoTier) {
                const int mid = (maxStage + 1) / 2;
                splitStage = mid;
                for (int off : {0, 1, -1, 2, -2}) {
                    const int cand = mid + off;
                    if (cand <= 1 || cand >= maxStage) continue;
                    bool allNodes = !stageBins[cand].isEmpty();
                    for (const auto& bk : stageBins[cand]) {
                        if (bk.collection != "hydraulicNodes") { allNodes = false; break; }
                    }
                    if (allNodes) { splitStage = cand; break; }
                }
            }

            bool tier0HasShuntBelow = false;
            for (int s = 0; s < splitStage; ++s) {
                for (const auto& bk : stageBins[s]) {
                    if (bk.collection == "hydraulicNodes" && !shuntBelow[bk.id].isEmpty()) tier0HasShuntBelow = true;
                }
            }

            const int numTiers = twoTier ? 2 : 1;
            const double tierBaseY[2] = {
                twoTier ? (tier0HasShuntBelow ? -170.0 : -125.0) : (hasElec ? -195.0 : 0.0),
                145.0
            };

            for (int t = 0; t < numTiers; ++t) {
                const int sBegin = (t == 0) ? 0 : splitStage;
                const int sEnd = (t == 0) ? (twoTier ? splitStage - 1 : maxStage) : maxStage;
                QVector<int> activeStages;
                QVector<double> colWidths;
                constexpr double hGap = 54.0;
                double totalW = 0.0;

                for (int s = sBegin; s <= sEnd; ++s) {
                    if (stageBins[s].isEmpty()) continue;
                    double cw = 128.0;
                    for (const auto& bk : stageBins[s]) {
                        cw = std::max(cw, staticCardRect(bk.collection, hydRows[bk]).width());
                        if (bk.collection == "hydraulicNodes") {
                            for (const auto& sa : shuntAbove[bk.id]) cw = std::max(cw, staticCardRect(sa.collection, hydRows[sa]).width());
                            for (const auto& sb : shuntBelow[bk.id]) cw = std::max(cw, staticCardRect(sb.collection, hydRows[sb]).width());
                        }
                    }
                    activeStages.push_back(s);
                    colWidths.push_back(cw);
                    totalW += cw;
                }
                if (!activeStages.isEmpty()) totalW += hGap * (activeStages.size() - 1);

                double curLeft = -0.5 * totalW;
                std::map<int, double> colCenterX;
                for (int i = 0; i < activeStages.size(); ++i) {
                    colCenterX[activeStages[i]] = curLeft + 0.5 * colWidths[i];
                    curLeft += colWidths[i] + hGap;
                }

                // 先计算每列的纵向坐标：成对通道（如双阀→四腔→双作动筒）保持对称水平直连
                const double baseY = tierBaseY[t];
                for (int s : activeStages) {
                    const auto& vec = stageBins[s];
                    const double cx = colCenterX[s];
                    const int n = vec.size();
                    if (n == 1) {
                        placedHyd[vec[0]] = QPointF(cx, baseY);
                    } else if (n == 2 && vec[0].collection == "hydraulicNodes" && vec[1].collection == "hydraulicNodes") {
                        placedHyd[vec[0]] = QPointF(cx, baseY - 46.0);
                        placedHyd[vec[1]] = QPointF(cx, baseY + 46.0);
                    } else if (n == 4 && vec[0].collection == "hydraulicNodes" && vec[3].collection == "hydraulicNodes") {
                        placedHyd[vec[0]] = QPointF(cx, baseY - 148.0);
                        placedHyd[vec[1]] = QPointF(cx, baseY - 56.0);
                        placedHyd[vec[2]] = QPointF(cx, baseY + 56.0);
                        placedHyd[vec[3]] = QPointF(cx, baseY + 148.0);
                    } else {
                        const double stepY = (vec[0].collection == "hydraulicNodes") ? 90.0 : 204.0;
                        const double startY = baseY - 0.5 * (n - 1) * stepY;
                        for (int i = 0; i < n; ++i) {
                            placedHyd[vec[i]] = QPointF(cx, startY + i * stepY);
                        }
                    }
                }

                // 二次纵向微调：使多支路执行器（液压缸/马达）与其连接的 A/B 腔均值中心精准对齐
                for (int s : activeStages) {
                    for (const auto& bk : stageBins[s]) {
                        if (bk.collection != "hydraulicCylinders" && bk.collection != "hydraulicMotors") continue;
                        double sumY = 0.0;
                        int cnt = 0;
                        for (const auto& e : edges) {
                            if (e.to == bk && placedHyd.count(e.from)) { sumY += placedHyd[e.from].y(); ++cnt; }
                        }
                        if (cnt >= 2) placedHyd[bk].setY(sumY / cnt);
                    }
                }

                // 放置旁路元件：蓄能器在目标母管正上方，安全溢流阀在目标母管正下方
                for (int s : activeStages) {
                    for (const auto& bk : stageBins[s]) {
                        if (bk.collection != "hydraulicNodes") continue;
                        const QPointF np = placedHyd[bk];
                        const auto& aboveVec = shuntAbove[bk.id];
                        for (int i = 0; i < aboveVec.size(); ++i) {
                            placedHyd[aboveVec[i]] = QPointF(np.x() + i * 204.0, np.y() - 128.0);
                        }
                        const auto& belowVec = shuntBelow[bk.id];
                        for (int i = 0; i < belowVec.size(); ++i) {
                            placedHyd[belowVec[i]] = QPointF(np.x() + i * 204.0, np.y() + 132.0);
                        }
                    }
                }
            }

            for (const auto& [bk, pt] : placedHyd) {
                auto o = hydRows[bk];
                o.insert("schemX", std::round(pt.x()));
                o.insert("schemY", std::round(pt.y()));
                putEntity(bk.collection, o);
                const QRectF r = staticCardRect(bk.collection, o).translated(pt);
                maxHydBottomY = std::max(maxHydBottomY, r.bottom());
            }
        }

        // =====================================================================
        // 电气驱动泳道：直流母线电源（左）→ 伺服逆变器（中）→ 电机 / EHA / EMA（右）
        // =====================================================================
        if (hasElec) {
            QVector<QJsonObject> supplies, invs, acts;
            QVector<QString> actCols;
            for (const auto& v : doc.collection("powerSupplies")) {
                auto o = v.toObject();
                if (o.value("enabled").toBool(true)) supplies.push_back(o);
            }
            for (const auto& v : doc.collection("inverters")) {
                auto o = v.toObject();
                if (o.value("enabled").toBool(true)) invs.push_back(o);
            }
            for (const char* col : {"electricMotors", "ehaActuators", "emaActuators"}) {
                for (const auto& v : doc.collection(QLatin1String(col))) {
                    auto o = v.toObject();
                    if (o.value("enabled").toBool(true)) {
                        acts.push_back(o);
                        actCols.push_back(QLatin1String(col));
                    }
                }
            }

            constexpr double rowPitch = 108.0;
            const int numRows = std::max({1, invs.size(), acts.size(), supplies.size()});
            const double elecTopY = hasHyd
                ? (maxHydBottomY + 118.0)
                : (-0.5 * (numRows - 1) * rowPitch);

            std::map<int, double> invRowY;
            for (int i = 0; i < invs.size(); ++i) {
                const double ry = elecTopY + i * rowPitch;
                invs[i].insert("schemX", 0.0);
                invs[i].insert("schemY", std::round(ry));
                invRowY[invs[i].value("id").toInt(-1)] = ry;
                putEntity("inverters", invs[i]);
            }

            std::set<int> usedActRows;
            int fallbackRow = 0;
            for (int i = 0; i < acts.size(); ++i) {
                const int invId = acts[i].value("inverterId").toInt(-1);
                double ry = elecTopY + fallbackRow * rowPitch;
                if (acts[i].value("controlMode").toString() == "ExternalInverter" && invRowY.count(invId)) {
                    ry = invRowY[invId];
                } else {
                    while (usedActRows.count(fallbackRow)) ++fallbackRow;
                    ry = elecTopY + fallbackRow * rowPitch;
                    usedActRows.insert(fallbackRow++);
                }
                acts[i].insert("schemX", 320.0);
                acts[i].insert("schemY", std::round(ry));
                putEntity(actCols[i], acts[i]);
            }

            for (int i = 0; i < supplies.size(); ++i) {
                const int sid = supplies[i].value("id").toInt(-1);
                double sumY = 0.0;
                int cnt = 0;
                for (const auto& inv : invs) {
                    if (inv.value("supplyId").toInt(-1) == sid) {
                        sumY += inv.value("schemY").toDouble();
                        ++cnt;
                    }
                }
                const double sy = cnt > 0 ? (sumY / cnt)
                                          : (elecTopY + (supplies.size() == 1 ? 0.5 * (numRows - 1) * rowPitch : i * rowPitch));
                supplies[i].insert("schemX", -320.0);
                supplies[i].insert("schemY", std::round(sy));
                putEntity("powerSupplies", supplies[i]);
            }
        }

        resolveRemainingOverlaps();
    }

    bool tryCompletePickRequest(const BlockKey& clickedKey, const QString& pinKey) {
        if (activePick.target == PickTarget::None) return false;
        QString resolvedPin = pinKey;
        if (activePick.target == PickTarget::HydraulicNode && clickedKey.collection == "hydraulicNodes") {
            resolvedPin = QStringLiteral("node");
        } else if (activePick.target == PickTarget::PowerSupply && clickedKey.collection == "powerSupplies") {
            resolvedPin = QStringLiteral("dcOut");
        } else if (activePick.target == PickTarget::Inverter && clickedKey.collection == "inverters") {
            resolvedPin = QStringLiteral("acOut");
        } else if (activePick.target == PickTarget::HydraulicActuatorPort
                   && (clickedKey.collection == "hydraulicCylinders" || clickedKey.collection == "hydraulicMotors")) {
            if (pinKey == "portANodeId" || pinKey == "A") resolvedPin = QStringLiteral("A");
            else if (pinKey == "portBNodeId" || pinKey == "B") resolvedPin = QStringLiteral("B");
            else {
                if (statusLabel)
                    statusLabel->setText(QStringLiteral("请直接点击执行器卡片左侧高亮的【A】或【B】端口圆点。"));
                return true;
            }
        } else {
            if (statusLabel)
                statusLabel->setText(QStringLiteral("所点击对象与当前拾取目标类型不匹配，请点击绿色高亮的目标项（按 Esc 取消）。"));
            return true;
        }

        auto cb = std::move(activePick.onPicked);
        const QString prompt = activePick.promptTitle;
        activePick = {};
        updateBannerAndVisualStates();
        if (cb) cb(clickedKey.collection, clickedKey.id, resolvedPin);
        if (statusLabel) {
            const QString pickedName = doc.entity(clickedKey.collection, clickedKey.id).value("name").toString();
            statusLabel->setText(QStringLiteral("已完成【%1】拾取：%2%3。")
                                     .arg(prompt, pickedName,
                                          (resolvedPin == "A" || resolvedPin == "B") ? QStringLiteral(" · %1口").arg(resolvedPin) : QString{}));
        }
        return true;
    }

    bool handlePinClick(const BlockKey& clickedKey, const QString& pinKey) {
        // 若当前处于参数面板发起的拾取模式，优先响应拾取
        if (activePick.target != PickTarget::None) {
            return tryCompletePickRequest(clickedKey, pinKey);
        }

        // 对于液压节点本体，仅在已开启端口连线模式或已有连线起点时作为端口点击；否则允许正常单击选中与拖拽移动
        if (pinKey == "node" && !hasPendingPin && (!wireModeBtn || !wireModeBtn->isChecked())) {
            return false;
        }

        if (!hasPendingPin) {
            hasPendingPin = true;
            pendingBlock = clickedKey;
            pendingPin = pinKey;
            updateBannerAndVisualStates();
            if (statusLabel) {
                statusLabel->setText(QStringLiteral("连线起点已选：%1 (%2) — 请点击图中绿色高亮的目标节点或端口完成连接。")
                                         .arg(doc.entity(clickedKey.collection, clickedKey.id).value("name").toString(),
                                              pinDisplayName(clickedKey, pinKey)));
            }
            return true;
        }

        const BlockKey firstBlock = pendingBlock;
        const QString firstPin = pendingPin;
        hasPendingPin = false;

        if (firstBlock == clickedKey && firstPin == pinKey) {
            updateBannerAndVisualStates();
            if (statusLabel) statusLabel->setText(QStringLiteral("已取消当前连线选择。"));
            return true;
        }

        // Case 0: 端口连接右端 (actuatorEndpoint) <-> 液压缸 / 液压马达 A/B 口
        const auto wirePortConnectionActuator = [&](const BlockKey& pcKey, const BlockKey& actKey, const QString& actPin) -> bool {
            if (pcKey.collection != "hydraulicComponents") return false;
            if (actKey.collection != "hydraulicCylinders" && actKey.collection != "hydraulicMotors") return false;
            if (actPin != "portANodeId" && actPin != "portBNodeId") return false;
            auto pcRow = doc.entity("hydraulicComponents", pcKey.id);
            if (pcRow.value("componentType").toString() != "PortConnection") return false;
            const QString portName = (actPin == "portBNodeId") ? QStringLiteral("B") : QStringLiteral("A");
            pcRow.insert("actuatorType", actKey.collection == "hydraulicMotors" ? QStringLiteral("Motor") : QStringLiteral("Cylinder"));
            pcRow.insert("actuatorId", actKey.id);
            pcRow.insert("actuatorPort", portName);
            putEntity("hydraulicComponents", pcRow);
            commitAndRefresh(QStringLiteral("已将“%1”连接到 %2 · %3口。")
                                 .arg(pcRow.value("name").toString(),
                                      doc.entity(actKey.collection, actKey.id).value("name").toString(),
                                      portName));
            if (callbacks.onEntitySelected) callbacks.onEntitySelected(pcKey.collection, pcKey.id);
            return true;
        };
        if (firstPin == "actuatorEndpoint" && wirePortConnectionActuator(firstBlock, clickedKey, pinKey)) return true;
        if (pinKey == "actuatorEndpoint" && wirePortConnectionActuator(clickedKey, firstBlock, firstPin)) return true;

        // Case 1: Electrical Supply <-> Inverter
        if ((firstBlock.collection == "powerSupplies" && clickedKey.collection == "inverters") ||
            (clickedKey.collection == "powerSupplies" && firstBlock.collection == "inverters")) {
            const int supId = firstBlock.collection == "powerSupplies" ? firstBlock.id : clickedKey.id;
            const int invId = firstBlock.collection == "inverters" ? firstBlock.id : clickedKey.id;
            auto invRow = doc.entity("inverters", invId);
            invRow.insert("supplyId", supId);
            putEntity("inverters", invRow);
            commitAndRefresh(QStringLiteral("已连接直流电源 [%1] → 伺服逆变器 [%2]。").arg(supId).arg(invId));
            return true;
        }

        // Case 2: Electrical Inverter <-> ElectricMotor / EHA / EMA
        const auto isElecActuator = [](const QString& c) {
            return c == "electricMotors" || c == "ehaActuators" || c == "emaActuators";
        };
        if ((firstBlock.collection == "inverters" && isElecActuator(clickedKey.collection)) ||
            (clickedKey.collection == "inverters" && isElecActuator(firstBlock.collection))) {
            const int invId = firstBlock.collection == "inverters" ? firstBlock.id : clickedKey.id;
            const BlockKey actKey = isElecActuator(firstBlock.collection) ? firstBlock : clickedKey;
            auto actRow = doc.entity(actKey.collection, actKey.id);
            actRow.insert("controlMode", QStringLiteral("ExternalInverter"));
            actRow.insert("inverterId", invId);
            putEntity(actKey.collection, actRow);
            commitAndRefresh(QStringLiteral("已绑定伺服逆变器 [%1] → %2。").arg(invId).arg(actRow.value("name").toString()));
            return true;
        }

        // Case 3: Hydraulic port <-> HydraulicNode
        const auto wireHydPortToNode = [&](const BlockKey& portOwner, const QString& portField, int nodeId) -> bool {
            if (portOwner.collection == "hydraulicNodes" || portField == "node" || portField == "actuatorEndpoint") return false;
            auto ownerRow = doc.entity(portOwner.collection, portOwner.id);
            if (ownerRow.isEmpty()) return false;
            ownerRow.insert(portField, nodeId);
            if (portOwner.collection == "hydraulicCylinders" || portOwner.collection == "hydraulicMotors") {
                ownerRow.insert("supplyMode", QStringLiteral("ExternalCircuit"));
            }
            putEntity(portOwner.collection, ownerRow);
            commitAndRefresh(QStringLiteral("已连接 %1.%2 → 液压节点“%3”。")
                                 .arg(ownerRow.value("name").toString(),
                                      pinDisplayName(portOwner, portField),
                                      doc.entity("hydraulicNodes", nodeId).value("name").toString()));
            if (callbacks.onEntitySelected && selectedCollection == portOwner.collection && selectedId == portOwner.id)
                callbacks.onEntitySelected(portOwner.collection, portOwner.id);
            return true;
        };
        if (firstBlock.collection == "hydraulicNodes" && clickedKey.collection != "hydraulicNodes") {
            if (wireHydPortToNode(clickedKey, pinKey, firstBlock.id)) return true;
        }
        if (clickedKey.collection == "hydraulicNodes" && firstBlock.collection != "hydraulicNodes") {
            if (wireHydPortToNode(firstBlock, firstPin, clickedKey.id)) return true;
        }

        // Case 4: Two hydraulic component/actuator ports clicked directly -> auto-create intermediate Chamber node
        const auto isHydPortOwner = [](const QString& c) {
            return c == "hydraulicComponents" || c == "hydraulicCylinders" || c == "hydraulicMotors";
        };
        if (isHydPortOwner(firstBlock.collection) && isHydPortOwner(clickedKey.collection)
            && firstPin != "actuatorEndpoint" && pinKey != "actuatorEndpoint") {
            const int newNodeId = allocateId("hydraulicNodes");
            QPointF pA = blockItems.count(firstBlock) ? blockItems[firstBlock]->scenePinPos(firstPin) : QPointF(0, 0);
            QPointF pB = blockItems.count(clickedKey) ? blockItems[clickedKey]->scenePinPos(pinKey) : QPointF(100, 0);
            const QPointF mid = 0.5 * (pA + pB);
            QJsonObject nodeRow{
                {"id", newNodeId},
                {"name", QStringLiteral("连接容腔-%1").arg(newNodeId)},
                {"enabled", true},
                {"nodeType", QStringLiteral("Chamber")},
                {"volumeMl", 80.0},
                {"initialPressureMPa", 1.0}
            };
            const QPointF cleanSpot = findNonOverlappingSpot("hydraulicNodes", nodeRow, mid);
            nodeRow.insert("schemX", cleanSpot.x());
            nodeRow.insert("schemY", cleanSpot.y());
            putEntity("hydraulicNodes", nodeRow);
            auto r1 = doc.entity(firstBlock.collection, firstBlock.id);
            r1.insert(firstPin, newNodeId);
            if (firstBlock.collection != "hydraulicComponents") r1.insert("supplyMode", QStringLiteral("ExternalCircuit"));
            putEntity(firstBlock.collection, r1);

            auto r2 = doc.entity(clickedKey.collection, clickedKey.id);
            r2.insert(pinKey, newNodeId);
            if (clickedKey.collection != "hydraulicComponents") r2.insert("supplyMode", QStringLiteral("ExternalCircuit"));
            putEntity(clickedKey.collection, r2);

            commitAndRefresh(QStringLiteral("已自动创建容腔节点“%1”并连接两端油口。").arg(nodeRow.value("name").toString()));
            return true;
        }

        updateBannerAndVisualStates();
        if (statusLabel) {
            statusLabel->setText(QStringLiteral("所选两端端口物理域不匹配（请连接液压油口↔液压节点、端口连接↔执行器油口，或电源↔逆变器↔电机/EHA/EMA）。"));
        }
        return true;
    }

    void handleBlockClicked(const BlockKey& clickedKey) {
        if (activePick.target != PickTarget::None) {
            if (tryCompletePickRequest(clickedKey, {})) return;
        }
        selectedCollection = clickedKey.collection;
        selectedId = clickedKey.id;
        updateBannerAndVisualStates();
        if (callbacks.onEntitySelected) {
            callbacks.onEntitySelected(clickedKey.collection, clickedKey.id);
        }
    }

    void drawWirePath(const BlockKey& a, const BlockKey& b, const QPainterPath& path,
                      const QColor& color, bool dashed = false) {
        auto* halo = scene->addPath(path, QPen(QColor(24, 144, 255, 95), 6.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        halo->setZValue(-11.0);
        const bool endpointSelected = (!selectedCollection.isEmpty() && selectedId >= 0)
            && ((a.collection == selectedCollection && a.id == selectedId)
                || (b.collection == selectedCollection && b.id == selectedId));
        halo->setVisible(endpointSelected);

        auto* item = scene->addPath(path, QPen(color, 2.3, dashed ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        item->setZValue(-10.0);
        wireItems.push_back({item, halo, a, b});
    }

    void rebuildWires() {
        for (const auto& w : wireItems) {
            if (w.halo) { scene->removeItem(w.halo); delete w.halo; }
            if (w.item) { scene->removeItem(w.item); delete w.item; }
        }
        wireItems.clear();

        // 统计液压区几何包络，计算底部回油总管与左侧跨层回油走廊坐标
        double minHydLeftX = 1e9;
        double maxHydBottomY = -1e9;
        for (const auto& [bk, item] : blockItems) {
            if (bk.collection != "hydraulicNodes" && bk.collection != "hydraulicComponents"
                && bk.collection != "hydraulicCylinders" && bk.collection != "hydraulicMotors") continue;
            const QRectF r = item->cardBodyRect().translated(item->scenePos());
            minHydLeftX = std::min(minHydLeftX, r.left());
            maxHydBottomY = std::max(maxHydBottomY, r.bottom());
        }
        if (minHydLeftX > 1e8) { minHydLeftX = -500.0; maxHydBottomY = 120.0; }
        const double hydReturnBusBaseY = maxHydBottomY + 34.0;
        const double hydLeftCorridorBaseX = minHydLeftX - 34.0;

        int returnWireIdx = 0;
        int tierWrapIdx = 0;
        std::map<std::pair<int, int>, int> nodeSideSlotCount;

        const auto hasHydObstacleBelowTank = [&](int tankNid, double targetY) {
            const BlockKey tk{"hydraulicNodes", tankNid};
            if (!blockItems.count(tk)) return false;
            const auto* tankItem = blockItems.at(tk);
            const double tx = tankItem->scenePos().x();
            const double tankBottom = tankItem->scenePos().y() + tankItem->cardBodyRect().bottom();
            for (const auto& [bk, item] : blockItems) {
                if (bk == tk) continue;
                if (bk.collection != "hydraulicNodes" && bk.collection != "hydraulicComponents"
                    && bk.collection != "hydraulicCylinders" && bk.collection != "hydraulicMotors") continue;
                const QRectF r = item->cardBodyRect().translated(item->scenePos()).adjusted(-14, -8, 14, 10);
                if (r.top() > tankBottom + 2.0 && r.top() < targetY + 8.0 && tx >= r.left() && tx <= r.right()) {
                    return true;
                }
            }
            return false;
        };

        const auto computeInterTierCorridorY = [&](double upperY, double lowerY) {
            const double splitMid = 0.5 * (upperY + lowerY);
            double upperMaxBot = upperY + 38.0;
            double lowerMinTop = lowerY - 38.0;
            for (const auto& [bk, item] : blockItems) {
                if (bk.collection != "hydraulicNodes" && bk.collection != "hydraulicComponents"
                    && bk.collection != "hydraulicCylinders" && bk.collection != "hydraulicMotors") continue;
                const QRectF r = item->cardBodyRect().translated(item->scenePos());
                if (item->scenePos().y() < splitMid) upperMaxBot = std::max(upperMaxBot, r.bottom());
                else lowerMinTop = std::min(lowerMinTop, r.top());
            }
            return 0.5 * (upperMaxBot + lowerMinTop);
        };

        const auto nodeColor = [&](int nid, const QColor& fallback) {
            const auto n = doc.entity("hydraulicNodes", nid);
            const QString nt = n.value("nodeType").toString();
            if (nt == "Tank") return QColor(43, 123, 186);
            if (nt == "PressureSource") return QColor(214, 69, 69);
            return fallback;
        };

        const auto connectToHydNode = [&](const BlockKey& owner, const QString& pinKey, int nid,
                                          const QColor& color, bool isReturnLine = false) {
            if (nid < 0) return;
            const BlockKey nk{"hydraulicNodes", nid};
            if (!blockItems.count(owner) || !blockItems.count(nk)) return;
            auto* ownerItem = blockItems.at(owner);
            auto* nodeItem = blockItems.at(nk);
            const QPointF p1 = ownerItem->scenePinPos(pinKey);
            const QPointF d1 = ownerItem->pinOutwardDir(pinKey);
            const QString nt = doc.entity("hydraulicNodes", nid).value("nodeType").toString("Chamber");

            // 回油管路（阀 T 口 / 溢流阀出口 -> 油箱）：走专用底部回油总管或同层局部回油道，绝不斜穿中间元件
            if (isReturnLine && nt == "Tank") {
                const double localReturnY = ownerItem->scenePos().y() + ownerItem->cardBodyRect().bottom() + 26.0;
                const bool canRiseDirectlyAtTank = (localReturnY > nodeItem->scenePos().y() + nodeItem->cardBodyRect().bottom() + 14.0)
                                                && !hasHydObstacleBelowTank(nid, localReturnY);
                if (canRiseDirectlyAtTank) {
                    const int slot = nodeSideSlotCount[{nid, 1}]++;
                    const double slotOff = (slot == 0) ? 0.0 : ((slot % 2 == 1 ? 1 : -1) * ((slot + 1) / 2) * 14.0);
                    const auto [p2, d2] = nodeItem->sceneHydNodeEdgeAnchor(p1, 1, slotOff);
                    drawWirePath(owner, nk,
                                 makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::TankReturnBottom,
                                                        localReturnY + slot * 9.0),
                                 nodeColor(nid, color));
                } else {
                    const int rIdx = returnWireIdx++;
                    const double busY = hydReturnBusBaseY + rIdx * 10.0;
                    if (!hasHydObstacleBelowTank(nid, busY)) {
                        const int slot = nodeSideSlotCount[{nid, 1}]++;
                        const double slotOff = (slot == 0) ? 0.0 : ((slot % 2 == 1 ? 1 : -1) * ((slot + 1) / 2) * 14.0);
                        const auto [p2, d2] = nodeItem->sceneHydNodeEdgeAnchor(p1, 1, slotOff);
                        drawWirePath(owner, nk,
                                     makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::TankReturnBottom, busY),
                                     nodeColor(nid, color));
                    } else {
                        const int slot = nodeSideSlotCount[{nid, -2}]++;
                        const double slotOff = (slot == 0) ? 0.0 : ((slot % 2 == 1 ? 1 : -1) * ((slot + 1) / 2) * 10.0);
                        const auto [p2, d2] = nodeItem->sceneHydNodeEdgeAnchor(p1, -2, slotOff);
                        drawWirePath(owner, nk,
                                     makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::TankReturnLeft,
                                                            busY, hydLeftCorridorBaseX - rIdx * 10.0),
                                     nodeColor(nid, color));
                    }
                }
                return;
            }

            // 跨层串联折叠连接（上层右端元件出口 -> 下层左端容腔入口）
            if (d1.x() > 0.5 && nodeItem->scenePos().y() - ownerItem->scenePos().y() > 165.0
                && nodeItem->scenePos().x() < p1.x() - 120.0) {
                const double corrY = computeInterTierCorridorY(ownerItem->scenePos().y(), nodeItem->scenePos().y())
                                   + (tierWrapIdx++) * 10.0;
                const auto [p2, d2] = nodeItem->sceneHydNodeEdgeAnchor(p1, -2, 0.0);
                drawWirePath(owner, nk,
                             makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::TierWrap, corrY),
                             nodeColor(nid, color));
                return;
            }

            const auto [p2, d2] = nodeItem->sceneHydNodeEdgeAnchor(ownerItem->scenePos(), 0, 0.0);
            // 当节点在元件左侧（前向供油：节点右缘 -> 元件左口）时从节点引出，确保垂直分流母线比例一致
            const double busFrac = (pinKey == "portPNodeId") ? 0.36 : 0.50;
            if (d2.x() > 0.5 && d1.x() < -0.5) {
                drawWirePath(nk, owner,
                             makeOrthogonalWirePath(p2, d2, p1, d1, WireRouteKind::Standard, 0.0, 0.0, busFrac),
                             nodeColor(nid, color));
            } else {
                drawWirePath(owner, nk,
                             makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::Standard, 0.0, 0.0, busFrac),
                             nodeColor(nid, color));
            }
        };

        for (const auto& v : doc.collection("hydraulicComponents")) {
            const auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            const int cid = o.value("id").toInt();
            const BlockKey bk{"hydraulicComponents", cid};
            const QString ct = o.value("componentType").toString();
            if (ct == "Pump") {
                connectToHydNode(bk, "suctionNodeId", o.value("suctionNodeId").toInt(-1), QColor(43, 123, 186), false);
                connectToHydNode(bk, "deliveryNodeId", o.value("deliveryNodeId").toInt(-1), QColor(214, 69, 69), false);
            } else if (ct == "DirectionalValve" || ct == "ServoValve") {
                connectToHydNode(bk, "portPNodeId", o.value("portPNodeId").toInt(-1), QColor(214, 69, 69), false);
                connectToHydNode(bk, "portTNodeId", o.value("portTNodeId").toInt(-1), QColor(43, 123, 186), true);
                connectToHydNode(bk, "portANodeId", o.value("portANodeId").toInt(-1), QColor(217, 130, 43), false);
                connectToHydNode(bk, "portBNodeId", o.value("portBNodeId").toInt(-1), QColor(217, 130, 43), false);
            } else if (ct == "Accumulator") {
                connectToHydNode(bk, "targetNodeId", o.value("targetNodeId").toInt(-1), QColor(28, 138, 168), false);
            } else if (ct == "PortConnection") {
                const int nid = o.value("targetNodeId").toInt(-1);
                connectToHydNode(bk, "targetNodeId", nid, QColor(210, 118, 32), false);
                const QString actCol = o.value("actuatorType").toString("Cylinder") == "Motor"
                                           ? QStringLiteral("hydraulicMotors")
                                           : QStringLiteral("hydraulicCylinders");
                const BlockKey ak{actCol, o.value("actuatorId").toInt(-1)};
                const QString portPin = o.value("actuatorPort").toString("A") == "B" ? QStringLiteral("portBNodeId") : QStringLiteral("portANodeId");
                if (blockItems.count(bk) && blockItems.count(ak)) {
                    const QPointF p1 = blockItems[bk]->scenePinPos("actuatorEndpoint");
                    const QPointF d1 = blockItems[bk]->pinOutwardDir("actuatorEndpoint");
                    const QPointF p2 = blockItems[ak]->scenePinPos(portPin);
                    const QPointF d2 = blockItems[ak]->pinOutwardDir(portPin);
                    drawWirePath(bk, ak,
                                 makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::Standard),
                                 QColor(210, 118, 32), true);
                }
            } else {
                const int inId = o.value("inletNodeId").toInt(-1);
                const int outId = o.value("outletNodeId").toInt(-1);
                const bool outIsTankReturn = (ct == "ReliefValve")
                    || (outId >= 0 && doc.entity("hydraulicNodes", outId).value("nodeType").toString() == "Tank");
                connectToHydNode(bk, "inletNodeId", inId, QColor(214, 69, 69), false);
                connectToHydNode(bk, "outletNodeId", outId, QColor(43, 123, 186), outIsTankReturn);
            }
        }

        for (const char* col : {"hydraulicCylinders", "hydraulicMotors"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true) || o.value("supplyMode").toString() != "ExternalCircuit") continue;
                const BlockKey bk{QLatin1String(col), o.value("id").toInt()};
                connectToHydNode(bk, "portANodeId", o.value("portANodeId").toInt(-1), QColor(217, 130, 43), false);
                connectToHydNode(bk, "portBNodeId", o.value("portBNodeId").toInt(-1), QColor(217, 130, 43), false);
            }
        }

        for (const auto& v : doc.collection("inverters")) {
            const auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            const int iid = o.value("id").toInt();
            const int sid = o.value("supplyId").toInt(-1);
            const BlockKey ibk{"inverters", iid};
            const BlockKey sbk{"powerSupplies", sid};
            if (sid >= 0 && blockItems.count(ibk) && blockItems.count(sbk) && sbk != ibk) {
                const QPointF p1 = blockItems[sbk]->scenePinPos("dcOut");
                const QPointF d1 = blockItems[sbk]->pinOutwardDir("dcOut");
                const QPointF p2 = blockItems[ibk]->scenePinPos("supplyId");
                const QPointF d2 = blockItems[ibk]->pinOutwardDir("supplyId");
                drawWirePath(sbk, ibk,
                             makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::Standard, 0.0, 0.0, 0.44),
                             QColor(132, 82, 198));
            }
        }

        for (const char* col : {"electricMotors", "ehaActuators", "emaActuators"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true) || o.value("controlMode").toString() != "ExternalInverter") continue;
                const int iid = o.value("inverterId").toInt(-1);
                const BlockKey abk{QLatin1String(col), o.value("id").toInt()};
                const BlockKey ibk{"inverters", iid};
                if (iid >= 0 && blockItems.count(abk) && blockItems.count(ibk)) {
                    const QPointF p1 = blockItems[ibk]->scenePinPos("acOut");
                    const QPointF d1 = blockItems[ibk]->pinOutwardDir("acOut");
                    const QPointF p2 = blockItems[abk]->scenePinPos("inverterId");
                    const QPointF d2 = blockItems[abk]->pinOutwardDir("inverterId");
                    drawWirePath(ibk, abk,
                                 makeOrthogonalWirePath(p1, d1, p2, d2, WireRouteKind::Standard, 0.0, 0.0, 0.50),
                                 QColor(42, 152, 102));
                }
            }
        }
        updateRubberBand(lastMouseScenePos);
    }

    void updateTelemetryLabels() {
        for (auto& kv : blockItems) {
            kv.second->telemetry = formatTelemetryBadge(solvedResult, kv.first.collection, kv.first.id, frameIndex);
            kv.second->update();
        }
        if (frameTimeLabel && solvedResult && frameIndex >= 0 && std::size_t(frameIndex) < solvedResult->frames.size()) {
            frameTimeLabel->setText(QStringLiteral("t = %1 s").arg(solvedResult->frames[std::size_t(frameIndex)].seconds, 0, 'f', 3));
        }
    }

    void rebuildScene(bool fitView) {
        assignAutoLayout(false);
        rubberBandItem = nullptr;
        scene->clear();
        blockItems.clear();
        wireItems.clear();

        std::map<BlockKey, int> diagSeverity;
        for (const auto& item : lastReport.items) {
            if (item.collection.isEmpty() || item.entityId < 0) continue;
            const int sev = item.severity == MotionDiagnosticItem::Severity::Error ? 2
                          : item.severity == MotionDiagnosticItem::Severity::Warning ? 1 : 0;
            if (sev > 0) {
                BlockKey k{item.collection, item.entityId};
                diagSeverity[k] = std::max(diagSeverity[k], sev);
            }
        }

        const QStringList allCols{
            "hydraulicNodes", "hydraulicComponents", "hydraulicCylinders", "hydraulicMotors",
            "powerSupplies", "inverters", "electricMotors", "ehaActuators", "emaActuators"
        };

        for (const auto& col : allCols) {
            for (const auto& v : doc.collection(col)) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true)) continue;
                BlockKey k{col, o.value("id").toInt()};
                auto* blk = new SchematicBlockItem(k, o, doc);
                blk->severity = diagSeverity.count(k) ? diagSeverity[k] : 0;
                blk->telemetry = formatTelemetryBadge(solvedResult, col, k.id, frameIndex);
                blk->setPos(o.value("schemX").toDouble(0.0), o.value("schemY").toDouble(0.0));
                blk->onMovedCommit = [this](const BlockKey& key, const QPointF& newPos) {
                    auto row = doc.entity(key.collection, key.id);
                    if (row.isEmpty()) return;
                    row.insert("schemX", newPos.x());
                    row.insert("schemY", newPos.y());
                    putEntity(key.collection, row);
                    rebuildWires();
                    if (callbacks.commitDocument) {
                        NativeMotionDocument copy = doc;
                        callbacks.commitDocument(std::move(copy));
                    }
                };
                blk->onClicked = [this](const BlockKey& key) {
                    handleBlockClicked(key);
                };
                blk->onDoubleClicked = [this](const BlockKey& key) {
                    handleBlockClicked(key);
                    if (callbacks.openEntityEditor) callbacks.openEntityEditor(key.collection, key.id);
                };
                blk->onPinClicked = [this](const BlockKey& key, const QString& pinKey) {
                    return handlePinClick(key, pinKey);
                };
                QObject::connect(blk, &QGraphicsObject::xChanged, scene, [this] { rebuildWires(); });
                QObject::connect(blk, &QGraphicsObject::yChanged, scene, [this] { rebuildWires(); });
                scene->addItem(blk);
                blockItems[k] = blk;
            }
        }

        rebuildWires();
        updateBannerAndVisualStates();
        const QRectF rawBounds = blockItems.empty() ? QRectF(-320, -200, 640, 400) : scene->itemsBoundingRect();
        scene->setSceneRect(rawBounds.adjusted(-90, -70, 90, 70));
        if (fitView && !blockItems.empty()) {
            fitSceneComfortably();
        }
    }

    void fitSceneComfortably() {
        if (!view || !scene || blockItems.empty()) {
            if (view) view->resetTransform();
            return;
        }
        const bool hadBand = rubberBandItem && rubberBandItem->isVisible();
        if (rubberBandItem) rubberBandItem->setVisible(false);
        const QRectF fitBounds = scene->itemsBoundingRect().adjusted(-48, -38, 48, 38);
        if (rubberBandItem) rubberBandItem->setVisible(hadBand);
        scene->setSceneRect(fitBounds.adjusted(-70, -50, 70, 50));
        view->resetTransform();
        if (view->viewport()->width() >= 160 && view->viewport()->height() >= 120) {
            view->fitInView(fitBounds, Qt::KeepAspectRatio);
            const double curScale = view->transform().m11();
            if (curScale > 1.2) {
                view->resetTransform();
                view->scale(1.2, 1.2);
                view->centerOn(fitBounds.center());
            }
        }
    }
};

MultiphysicsSchematicDialog::MultiphysicsSchematicDialog(const NativeMotionDocument& initialDocument,
                                                         const RigidResult* solvedResult,
                                                         int initialFrameIndex,
                                                         Callbacks callbacks,
                                                         QWidget* parent)
    : MechMainWindow::mFramelessDialog(parent), m_impl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("multiphysicsSchematicDialog"));
    setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);
    setWindowTitle(tr("机电液回路原理图与拓扑工作台"));
    resize(1080, 720);
    setMinimumSize(980, 540);

    m_impl->doc = initialDocument;
    m_impl->solvedResult = solvedResult;
    m_impl->frameIndex = std::max(0, initialFrameIndex);
    m_impl->callbacks = std::move(callbacks);
    if (m_impl->callbacks.runDiagnostics) {
        m_impl->lastReport = m_impl->callbacks.runDiagnostics(m_impl->doc);
    }

    const QString fontFam = MxFunctions::UiMetrics::uiFontFamily();
    const int bodyFontPx = MxFunctions::UiMetrics::bodyFontSize();
    const int btnFontPx = bodyFontPx;
    const int btnOuterH = std::max(28, MxFunctions::UiMetrics::controlHeight() + 2);
    const int btnInnerH = btnOuterH - 2;
    const int radiusPx = MxFunctions::UiMetrics::zoneRadius();

    QFont bodyFont(fontFam);
    bodyFont.setPixelSize(bodyFontPx);
    bodyFont.setWeight(QFont::Normal);
    setFont(bodyFont);

    QFont btnFont(fontFam);
    btnFont.setPixelSize(btnFontPx);
    btnFont.setWeight(QFont::Normal);

    QFont boldBtnFont = btnFont;
    boldBtnFont.setWeight(QFont::Bold);
    const QFontMetrics btnFm(boldBtnFont);

    QFont sectionFont(fontFam);
    sectionFont.setPixelSize(bodyFontPx);
    sectionFont.setWeight(QFont::Bold);

    auto* content = new QWidget(this);
    content->setObjectName(QStringLiteral("multiphysicsSchematicContent"));
    content->setFont(bodyFont);
    content->setStyleSheet(QStringLiteral(
        "QWidget#multiphysicsSchematicContent { background-color:%1; color:%2; font-family:'%3'; font-size:%4px; }"
        "QLabel { font-family:'%3'; font-size:%4px; color:%2; background:transparent; border:none; }")
        .arg(MxFunctions::UiPalette::surfaceHex(),
             MxFunctions::UiPalette::textHex(),
             fontFam)
        .arg(bodyFontPx));
    setContentWidget(content);

    const auto styleActionBtn = [&](QPushButton* btn, bool accentToggle = false) {
        btn->setProperty("mxBtnNoTheme", true);
        btn->setFont(accentToggle ? boldBtnFont : btnFont);
        btn->setCursor(Qt::PointingHandCursor);
        const int contentW = std::max(54, btnFm.horizontalAdvance(btn->text()) + 18);
        const int outerW = contentW + 2;
        btn->setMinimumSize(outerW, btnOuterH);
        btn->setFixedHeight(btnOuterH);
        btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

        const QString qss = accentToggle
            ? QStringLiteral(
                  "QPushButton { font-family:'%1'; font-size:%2px; font-weight:bold;"
                  " width:%3px; min-width:%3px; height:%4px; min-height:%4px; padding:0px;"
                  " color:%5; background-color:%6; border:1px solid %7; border-radius:%10px; }"
                  "QPushButton:hover { background-color:%8; border-color:%5; }"
                  "QPushButton:checked { color:#ffffff; background-color:%5; border:1px solid %5; }"
                  "QPushButton:checked:hover { background-color:%9; }")
                  .arg(fontFam)
                  .arg(btnFontPx)
                  .arg(contentW)
                  .arg(btnInnerH)
                  .arg(MxFunctions::UiPalette::primaryHex(),
                       MxFunctions::UiPalette::selectedHex(),
                       MxFunctions::UiPalette::primaryHex(),
                       MxFunctions::UiPalette::hoverHex(),
                       MxFunctions::UiPalette::actionHex())
                  .arg(radiusPx)
            : QStringLiteral(
                  "QPushButton { font-family:'%1'; font-size:%2px; font-weight:normal;"
                  " width:%3px; min-width:%3px; height:%4px; min-height:%4px; padding:0px;"
                  " color:%5; background-color:%6; border:1px solid %7; border-radius:%12px; }"
                  "QPushButton:hover { background-color:%8; border-color:%9; }"
                  "QPushButton:pressed { background-color:%10; color:%11; border-color:%9; }")
                  .arg(fontFam)
                  .arg(btnFontPx)
                  .arg(contentW)
                  .arg(btnInnerH)
                  .arg(MxFunctions::UiPalette::textHex(),
                       MxFunctions::UiPalette::surfaceHex(),
                       MxFunctions::UiPalette::borderHex(),
                       MxFunctions::UiPalette::hoverHex(),
                       MxFunctions::UiPalette::primaryHex(),
                       MxFunctions::UiPalette::selectedHex(),
                       MxFunctions::UiPalette::selectedTextHex())
                  .arg(radiusPx);
        btn->setStyleSheet(qss);
    };

    const QString sectionLabelQss = QStringLiteral(
        "QLabel { font-family:'%1'; font-size:%2px; font-weight:bold; color:%3; background:transparent; border:none; }")
        .arg(fontFam)
        .arg(bodyFontPx)
        .arg(MxFunctions::UiPalette::labelHex());

    const auto makeSectionTag = [&](const QString& text, QWidget* host) {
        auto* lbl = new QLabel(text, host);
        lbl->setProperty("mxLblNoTheme", true);
        lbl->setFont(sectionFont);
        lbl->setStyleSheet(sectionLabelQss);
        return lbl;
    };

    const auto makeDivider = [&](QWidget* host) {
        auto* sep = new QFrame(host);
        sep->setFrameShape(QFrame::VLine);
        sep->setFixedWidth(1);
        sep->setFixedHeight(btnOuterH - 8);
        sep->setStyleSheet(QStringLiteral("background-color:%1; border:none;").arg(MxFunctions::UiPalette::borderHex()));
        return sep;
    };

    auto* rootLayout = new QVBoxLayout(content);
    rootLayout->setContentsMargins(12, 10, 12, 10);
    rootLayout->setSpacing(8);

    auto* toolbar = new QFrame(content);
    toolbar->setObjectName(QStringLiteral("schematicToolbar"));
    toolbar->setStyleSheet(QStringLiteral(
        "QFrame#schematicToolbar { background-color:%1; border:1px solid %2; border-radius:%3px; }")
        .arg(MxFunctions::UiPalette::panelHex(), MxFunctions::UiPalette::borderHex())
        .arg(radiusPx));
    auto* toolbarRows = new QVBoxLayout(toolbar);
    toolbarRows->setContentsMargins(10, 8, 10, 8);
    toolbarRows->setSpacing(6);

    auto* createRow = new QHBoxLayout;
    createRow->setContentsMargins(0, 0, 0, 0);
    createRow->setSpacing(5);

    const auto addQuickCreate = [&](QHBoxLayout* row, const QString& objName, const QString& text,
                                    const QString& col, const QString& subType = {}) {
        auto* btn = new QPushButton(text, toolbar);
        btn->setObjectName(objName);
        styleActionBtn(btn, false);
        row->addWidget(btn);
        connect(btn, &QPushButton::clicked, this, [this, col, subType] {
            if (!m_impl->callbacks.createDefaultEntity) return;
            auto created = m_impl->callbacks.createDefaultEntity(m_impl->doc, col, subType);
            if (created.isEmpty()) return;
            const QPointF center = m_impl->view->mapToScene(m_impl->view->viewport()->rect().center());
            const int createdId = created.value("id").toInt(-1);
            const QPointF cleanSpot = m_impl->findNonOverlappingSpot(col, created, center, BlockKey{col, createdId});
            created.insert("schemX", cleanSpot.x());
            created.insert("schemY", cleanSpot.y());
            m_impl->putEntity(col, created);
            m_impl->verifiedEntitySet.insert(BlockKey{col, createdId});
            m_impl->selectedCollection = col;
            m_impl->selectedId = createdId;
            m_impl->commitAndRefresh(tr("已添加 %1“%2”。").arg(blockTypeTitle(col, created), created.value("name").toString()));
            if (m_impl->callbacks.onEntitySelected && createdId >= 0)
                m_impl->callbacks.onEntitySelected(col, createdId);
        });
        return btn;
    };

    createRow->addWidget(makeSectionTag(tr("液压元件:"), toolbar));
    addQuickCreate(createRow, QStringLiteral("schemAddTankBtn"), tr("+ 油箱"), QStringLiteral("hydraulicNodes"), QStringLiteral("Tank"));
    addQuickCreate(createRow, QStringLiteral("schemAddChamberBtn"), tr("+ 容腔"), QStringLiteral("hydraulicNodes"), QStringLiteral("Chamber"));
    addQuickCreate(createRow, QStringLiteral("schemAddPumpBtn"), tr("+ 液压泵"), QStringLiteral("hydraulicComponents"), QStringLiteral("Pump"));
    addQuickCreate(createRow, QStringLiteral("schemAddDirValveBtn"), tr("+ 换向阀"), QStringLiteral("hydraulicComponents"), QStringLiteral("DirectionalValve"));
    addQuickCreate(createRow, QStringLiteral("schemAddServoValveBtn"), tr("+ 伺服阀"), QStringLiteral("hydraulicComponents"), QStringLiteral("ServoValve"));
    addQuickCreate(createRow, QStringLiteral("schemAddReliefBtn"), tr("+ 溢流阀"), QStringLiteral("hydraulicComponents"), QStringLiteral("ReliefValve"));
    addQuickCreate(createRow, QStringLiteral("schemAddAccBtn"), tr("+ 蓄能器"), QStringLiteral("hydraulicComponents"), QStringLiteral("Accumulator"));
    addQuickCreate(createRow, QStringLiteral("schemAddPortConnBtn"), tr("+ 端口连接"), QStringLiteral("hydraulicComponents"), QStringLiteral("PortConnection"));
    createRow->addSpacing(4);
    createRow->addWidget(makeDivider(toolbar));
    createRow->addSpacing(4);
    createRow->addWidget(makeSectionTag(tr("电气驱动:"), toolbar));
    addQuickCreate(createRow, QStringLiteral("schemAddSupplyBtn"), tr("+ 直流电源"), QStringLiteral("powerSupplies"));
    addQuickCreate(createRow, QStringLiteral("schemAddInverterBtn"), tr("+ 伺服逆变器"), QStringLiteral("inverters"));
    createRow->addStretch(1);
    toolbarRows->addLayout(createRow);

    auto* toolRow = new QHBoxLayout;
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(6);
    toolRow->addWidget(makeSectionTag(tr("拓扑与诊断:"), toolbar));

    m_impl->wireModeBtn = new QPushButton(tr("端口连线模式"), toolbar);
    m_impl->wireModeBtn->setObjectName(QStringLiteral("schemWireModeButton"));
    m_impl->wireModeBtn->setCheckable(true);
    styleActionBtn(m_impl->wireModeBtn, true);
    toolRow->addWidget(m_impl->wireModeBtn);
    connect(m_impl->wireModeBtn, &QPushButton::toggled, this, [this](bool on) {
        m_impl->hasPendingPin = false;
        m_impl->view->setDragMode(on ? QGraphicsView::NoDrag : QGraphicsView::ScrollHandDrag);
        m_impl->statusLabel->setText(on ? tr("连线模式开启：依次点击元件端口圆点与绿色高亮的目标节点/端口即可完成连接。")
                                        : tr("浏览与选配模式：单击图元联动模型树与参数栏，直接点击端口圆点也可快速连线。"));
        m_impl->updateBannerAndVisualStates();
    });

    auto* checkBtn = new QPushButton(tr("回路检查"), toolbar);
    checkBtn->setObjectName(QStringLiteral("schemCheckButton"));
    styleActionBtn(checkBtn, false);
    toolRow->addWidget(checkBtn);
    connect(checkBtn, &QPushButton::clicked, this, [this] {
        if (m_impl->callbacks.runDiagnostics) {
            m_impl->lastReport = m_impl->callbacks.runDiagnostics(m_impl->doc);
            m_impl->rebuildScene(false);
            m_impl->statusLabel->setText(
                tr("回路检查完成：错误 %1 项，警告 %2 项（异常元件已在图中高亮标出）。")
                    .arg(m_impl->lastReport.errorCount)
                    .arg(m_impl->lastReport.warningCount));
        }
    });

    toolRow->addSpacing(4);
    toolRow->addWidget(makeDivider(toolbar));
    toolRow->addSpacing(4);
    toolRow->addWidget(makeSectionTag(tr("视图与导出:"), toolbar));

    auto* autoLayoutBtn = new QPushButton(tr("自动排布"), toolbar);
    autoLayoutBtn->setObjectName(QStringLiteral("schemAutoLayoutButton"));
    styleActionBtn(autoLayoutBtn, false);
    toolRow->addWidget(autoLayoutBtn);
    connect(autoLayoutBtn, &QPushButton::clicked, this, [this] {
        m_impl->assignAutoLayout(true);
        m_impl->commitAndRefresh(tr("已按液压/电驱双泳道重新自动排布原理图。"));
        m_impl->fitSceneComfortably();
    });

    auto* fitBtn = new QPushButton(tr("自适应缩放"), toolbar);
    fitBtn->setObjectName(QStringLiteral("schemFitViewButton"));
    styleActionBtn(fitBtn, false);
    toolRow->addWidget(fitBtn);

    auto* zoomInBtn = new QPushButton(tr("放大 +"), toolbar);
    zoomInBtn->setObjectName(QStringLiteral("schemZoomInButton"));
    styleActionBtn(zoomInBtn, false);
    toolRow->addWidget(zoomInBtn);

    auto* zoomOutBtn = new QPushButton(tr("缩小 -"), toolbar);
    zoomOutBtn->setObjectName(QStringLiteral("schemZoomOutButton"));
    styleActionBtn(zoomOutBtn, false);
    toolRow->addWidget(zoomOutBtn);

    auto* resetZoomBtn = new QPushButton(tr("100% 视图"), toolbar);
    resetZoomBtn->setObjectName(QStringLiteral("schemResetZoomButton"));
    styleActionBtn(resetZoomBtn, false);
    toolRow->addWidget(resetZoomBtn);

    auto* exportBtn = new QPushButton(tr("导出 PNG"), toolbar);
    exportBtn->setObjectName(QStringLiteral("schemExportPngButton"));
    styleActionBtn(exportBtn, false);
    toolRow->addWidget(exportBtn);
    connect(exportBtn, &QPushButton::clicked, this, [this] {
        const QString path = QFileDialog::getSaveFileName(this, tr("导出机电液原理图"), QStringLiteral("circuit-schematic.png"), tr("PNG 图像 (*.png)"));
        if (!path.isEmpty()) exportPng(path);
    });

    toolRow->addStretch(1);
    toolbarRows->addLayout(toolRow);
    rootLayout->addWidget(toolbar);

    // 端口选择与连线动态引导横幅
    m_impl->pickBanner = new QFrame(content);
    m_impl->pickBanner->setObjectName(QStringLiteral("schematicPickBanner"));
    m_impl->pickBanner->setStyleSheet(QStringLiteral(
        "QFrame#schematicPickBanner { background-color:%1; border:1px solid %2; border-radius:%3px; }")
        .arg(MxFunctions::UiPalette::selectedHex(), MxFunctions::UiPalette::primaryHex())
        .arg(radiusPx));
    auto* bannerRow = new QHBoxLayout(m_impl->pickBanner);
    bannerRow->setContentsMargins(10, 5, 10, 5);
    bannerRow->setSpacing(8);
    m_impl->pickBannerLabel = new QLabel(m_impl->pickBanner);
    m_impl->pickBannerLabel->setObjectName(QStringLiteral("schematicPickBannerLabel"));
    m_impl->pickBannerLabel->setProperty("mxLblNoTheme", true);
    m_impl->pickBannerLabel->setFont(sectionFont);
    m_impl->pickBannerLabel->setStyleSheet(QStringLiteral(
        "QLabel { font-family:'%1'; font-size:%2px; font-weight:bold; color:%3; background:transparent; border:none; }")
        .arg(fontFam).arg(bodyFontPx).arg(MxFunctions::UiPalette::primaryHex()));
    m_impl->pickBannerLabel->setWordWrap(true);
    m_impl->cancelPickBtn = new QPushButton(tr("取消选择"), m_impl->pickBanner);
    m_impl->cancelPickBtn->setObjectName(QStringLiteral("schemCancelPickButton"));
    styleActionBtn(m_impl->cancelPickBtn, false);
    connect(m_impl->cancelPickBtn, &QPushButton::clicked, this, [this] {
        if (m_impl->activePick.target != PickTarget::None) {
            cancelPortPick(true);
        } else if (m_impl->hasPendingPin) {
            m_impl->hasPendingPin = false;
            m_impl->updateBannerAndVisualStates();
            m_impl->statusLabel->setText(tr("已取消端口连线。"));
        }
    });
    bannerRow->addWidget(m_impl->pickBannerLabel, 1);
    bannerRow->addWidget(m_impl->cancelPickBtn);
    m_impl->pickBanner->hide();
    rootLayout->addWidget(m_impl->pickBanner);

    m_impl->scene = new QGraphicsScene(content);
    m_impl->scene->setFont(bodyFont);
    m_impl->view = new SchematicView(m_impl->scene, content);
    m_impl->view->setObjectName(QStringLiteral("multiphysicsSchematicView"));
    m_impl->view->setFont(bodyFont);
    m_impl->view->setFrameShape(QFrame::StyledPanel);
    m_impl->view->setStyleSheet(QStringLiteral(
        "QGraphicsView#multiphysicsSchematicView { border:1px solid %1; border-radius:%2px; }")
        .arg(MxFunctions::UiPalette::borderHex())
        .arg(radiusPx));
    m_impl->view->onSceneMouseMove = [this](const QPointF& scenePos) {
        if (m_impl->hasPendingPin || m_impl->activePick.target != PickTarget::None)
            m_impl->updateRubberBand(scenePos);
    };
    m_impl->view->onRightClickCancel = [this] {
        if (m_impl->activePick.target != PickTarget::None) {
            cancelPortPick(true);
            return true;
        }
        if (m_impl->hasPendingPin) {
            m_impl->hasPendingPin = false;
            m_impl->updateBannerAndVisualStates();
            m_impl->statusLabel->setText(tr("已取消端口连线。"));
            return true;
        }
        return false;
    };
    rootLayout->addWidget(m_impl->view, 1);

    connect(fitBtn, &QPushButton::clicked, this, [this] {
        m_impl->fitSceneComfortably();
    });
    connect(zoomInBtn, &QPushButton::clicked, this, [this] {
        if (m_impl->view->transform().m11() < 3.5) m_impl->view->scale(1.18, 1.18);
    });
    connect(zoomOutBtn, &QPushButton::clicked, this, [this] {
        if (m_impl->view->transform().m11() > 0.25) m_impl->view->scale(1.0 / 1.18, 1.0 / 1.18);
    });
    connect(resetZoomBtn, &QPushButton::clicked, this, [this] {
        m_impl->view->resetTransform();
        if (!m_impl->blockItems.empty()) m_impl->view->centerOn(m_impl->scene->itemsBoundingRect().center());
    });

    const QString infoLabelQss = QStringLiteral(
        "QLabel { font-family:'%1'; font-size:%2px; font-weight:normal; color:%3; background:transparent; border:none; }")
        .arg(fontFam)
        .arg(bodyFontPx)
        .arg(MxFunctions::UiPalette::textHex());

    auto* scrubWidget = new QWidget(content);
    scrubWidget->setObjectName(QStringLiteral("schemScrubRow"));
    auto* scrubRow = new QHBoxLayout(scrubWidget);
    scrubRow->setContentsMargins(0, 0, 0, 0);
    scrubRow->setSpacing(8);
    auto* scrubTitle = makeSectionTag(tr("结果帧动态监测:"), scrubWidget);
    m_impl->frameSlider = new QSlider(Qt::Horizontal, scrubWidget);
    m_impl->frameSlider->setObjectName(QStringLiteral("schemTimelineSlider"));
    m_impl->frameTimeLabel = new QLabel(scrubWidget);
    m_impl->frameTimeLabel->setProperty("mxLblNoTheme", true);
    m_impl->frameTimeLabel->setFont(bodyFont);
    m_impl->frameTimeLabel->setStyleSheet(infoLabelQss);
    scrubRow->addWidget(scrubTitle);
    scrubRow->addWidget(m_impl->frameSlider, 1);
    scrubRow->addWidget(m_impl->frameTimeLabel);
    rootLayout->addWidget(scrubWidget);
    const auto refreshScrubVisibility = [this, scrubWidget] {
        const bool hasFrames = m_impl->solvedResult && !m_impl->solvedResult->frames.empty();
        scrubWidget->setVisible(hasFrames);
        if (hasFrames) {
            const QSignalBlocker block(m_impl->frameSlider);
            m_impl->frameSlider->setRange(0, int(m_impl->solvedResult->frames.size()) - 1);
            m_impl->frameSlider->setValue(std::clamp(m_impl->frameIndex, 0, int(m_impl->solvedResult->frames.size()) - 1));
        }
    };
    refreshScrubVisibility();
    connect(m_impl->frameSlider, &QSlider::valueChanged, this, [this](int idx) {
        m_impl->frameIndex = idx;
        m_impl->updateTelemetryLabels();
        if (m_impl->callbacks.onFrameScrubbed) m_impl->callbacks.onFrameScrubbed(idx);
    });

    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(12);
    m_impl->statusLabel = new QLabel(tr("提示：单击元件与模型树/参数栏双向联动；直接点击任意端口圆点即可引出连线。"), content);
    m_impl->statusLabel->setObjectName(QStringLiteral("schemStatusLabel"));
    m_impl->statusLabel->setProperty("mxLblNoTheme", true);
    m_impl->statusLabel->setFont(bodyFont);
    m_impl->statusLabel->setStyleSheet(infoLabelQss);
    m_impl->statusLabel->setWordWrap(true);
    auto* closeBtn = new QPushButton(tr("关闭"), content);
    closeBtn->setObjectName(QStringLiteral("schemCloseButton"));
    styleActionBtn(closeBtn, false);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    bottomRow->addWidget(m_impl->statusLabel, 1);
    bottomRow->addWidget(closeBtn);
    rootLayout->addLayout(bottomRow);

    m_impl->rebuildScene(false);
    m_impl->updateTelemetryLabels();
}

void MultiphysicsSchematicDialog::syncDocument(const NativeMotionDocument& document,
                                               const RigidResult* solvedResult,
                                               int frameIndex)
{
    m_impl->doc = document;
    const auto newSet = m_impl->currentEnabledEntitySet();
    if (newSet != m_impl->verifiedEntitySet) {
        m_impl->initialLayoutChecked = false;
    }
    m_impl->solvedResult = solvedResult;
    m_impl->frameIndex = std::max(0, frameIndex);
    if (m_impl->callbacks.runDiagnostics) {
        m_impl->lastReport = m_impl->callbacks.runDiagnostics(m_impl->doc);
    }
    if (auto* scrubWidget = findChild<QWidget*>(QStringLiteral("schemScrubRow"))) {
        const bool hasFrames = solvedResult && !solvedResult->frames.empty();
        scrubWidget->setVisible(hasFrames);
        if (hasFrames && m_impl->frameSlider) {
            const QSignalBlocker block(m_impl->frameSlider);
            m_impl->frameSlider->setRange(0, int(solvedResult->frames.size()) - 1);
            m_impl->frameSlider->setValue(std::clamp(m_impl->frameIndex, 0, int(solvedResult->frames.size()) - 1));
        }
    }
    m_impl->rebuildScene(false);
    m_impl->updateTelemetryLabels();
}

void MultiphysicsSchematicDialog::syncSelection(const QString& collection, int id, bool centerInView)
{
    m_impl->selectedCollection = collection;
    m_impl->selectedId = id;
    m_impl->updateBannerAndVisualStates();

    if (!centerInView || !m_impl->view || collection.isEmpty()) return;
    if (id >= 0) {
        const BlockKey k{collection, id};
        auto it = m_impl->blockItems.find(k);
        if (it != m_impl->blockItems.end() && it->second) {
            m_impl->view->ensureVisible(it->second, 90, 70);
            const auto row = m_impl->doc.entity(collection, id);
            if (!row.isEmpty() && m_impl->statusLabel && m_impl->activePick.target == PickTarget::None && !m_impl->hasPendingPin) {
                m_impl->statusLabel->setText(
                    tr("已定位到【%1 · %2】（相连管路与母线已在图中高亮）。")
                        .arg(blockTypeTitle(collection, row), row.value("name").toString()));
            }
        }
    } else {
        QRectF groupBounds;
        int count = 0;
        for (const auto& [bk, item] : m_impl->blockItems) {
            if (bk.collection != collection) continue;
            const QRectF r = item->mapToScene(item->boundingRect()).boundingRect();
            groupBounds = count == 0 ? r : groupBounds.united(r);
            ++count;
        }
        if (count > 0) {
            m_impl->view->ensureVisible(groupBounds.adjusted(-60, -50, 60, 50), 40, 40);
            if (m_impl->statusLabel && m_impl->activePick.target == PickTarget::None && !m_impl->hasPendingPin) {
                m_impl->statusLabel->setText(tr("已在原理图中高亮当前分类共 %1 个元件。").arg(count));
            }
        }
    }
}

void MultiphysicsSchematicDialog::setPlaybackFrame(int frameIndex)
{
    if (!m_impl->solvedResult || m_impl->solvedResult->frames.empty()) return;
    m_impl->frameIndex = std::clamp(frameIndex, 0, int(m_impl->solvedResult->frames.size()) - 1);
    if (m_impl->frameSlider) {
        const QSignalBlocker block(m_impl->frameSlider);
        m_impl->frameSlider->setValue(m_impl->frameIndex);
    }
    m_impl->updateTelemetryLabels();
}

void MultiphysicsSchematicDialog::beginPortPick(PickRequest request)
{
    if (m_impl->activePick.target != PickTarget::None && m_impl->activePick.onCancelled) {
        auto prevCancel = std::move(m_impl->activePick.onCancelled);
        m_impl->activePick = {};
        prevCancel();
    }
    m_impl->hasPendingPin = false;
    m_impl->activePick = std::move(request);
    if (!m_impl->activePick.ownerCollection.isEmpty() && m_impl->activePick.ownerId >= 0) {
        m_impl->selectedCollection = m_impl->activePick.ownerCollection;
        m_impl->selectedId = m_impl->activePick.ownerId;
    }
    m_impl->updateBannerAndVisualStates();
}

void MultiphysicsSchematicDialog::cancelPortPick(bool notifyCancelled)
{
    if (m_impl->activePick.target == PickTarget::None) return;
    auto cancelCb = std::move(m_impl->activePick.onCancelled);
    m_impl->activePick = {};
    m_impl->updateBannerAndVisualStates();
    if (notifyCancelled && cancelCb) cancelCb();
    if (m_impl->statusLabel) m_impl->statusLabel->setText(tr("已退出端口/节点图上拾取模式。"));
}

bool MultiphysicsSchematicDialog::isPortPicking() const
{
    return m_impl->activePick.target != PickTarget::None;
}

QString MultiphysicsSchematicDialog::selectedCollection() const
{
    return m_impl->selectedCollection;
}

int MultiphysicsSchematicDialog::selectedEntityId() const
{
    return m_impl->selectedId;
}

bool MultiphysicsSchematicDialog::activateEntityOrPin(const QString& collection, int id, const QString& pinKey)
{
    const BlockKey key{collection, id};
    if (!m_impl->blockItems.count(key)) return false;
    if (!pinKey.isEmpty()) return m_impl->handlePinClick(key, pinKey);
    m_impl->handleBlockClicked(key);
    return true;
}

void MultiphysicsSchematicDialog::showEvent(QShowEvent* event)
{
    MechMainWindow::mFramelessDialog::showEvent(event);
    if (!m_impl->initialFitDone) {
        m_impl->initialFitDone = true;
        if (layout()) layout()->activate();
        m_impl->fitSceneComfortably();
        QTimer::singleShot(0, this, [this] {
            if (isVisible()) m_impl->fitSceneComfortably();
        });
    }
}

void MultiphysicsSchematicDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        if (m_impl->activePick.target != PickTarget::None) {
            cancelPortPick(true);
            event->accept();
            return;
        }
        if (m_impl->hasPendingPin) {
            m_impl->hasPendingPin = false;
            m_impl->updateBannerAndVisualStates();
            event->accept();
            return;
        }
    }
    MechMainWindow::mFramelessDialog::keyPressEvent(event);
}

MultiphysicsSchematicDialog::~MultiphysicsSchematicDialog()
{
    cancelPortPick(true);
}

const NativeMotionDocument& MultiphysicsSchematicDialog::currentDocument() const
{
    return m_impl->doc;
}

bool MultiphysicsSchematicDialog::exportPng(const QString& filePath) const
{
    const bool hadBand = m_impl->rubberBandItem && m_impl->rubberBandItem->isVisible();
    if (m_impl->rubberBandItem) m_impl->rubberBandItem->setVisible(false);
    const QRectF bounds = m_impl->scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    const QSize imgSize(std::max(640, int(std::ceil(bounds.width() * 1.5))),
                        std::max(400, int(std::ceil(bounds.height() * 1.5))));
    QImage image(imgSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(248, 250, 252));
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    m_impl->scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(imgSize)), bounds);
    painter.end();
    if (m_impl->rubberBandItem) m_impl->rubberBandItem->setVisible(hadBand);
    return image.save(filePath);
}

} // namespace MotionOcc
