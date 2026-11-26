#include "MultiphysicsSchematicDialog.h"
#include "UiMetrics.h"
#include "UiPalette.h"

#include <QAbstractButton>
#include <QFileDialog>
#include <QFontMetrics>
#include <QFrame>
#include <QGraphicsItem>
#include <QGraphicsPathItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QShowEvent>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>

namespace MotionOcc {
namespace {

struct PinSpec {
    QString key;       // e.g. "node", "portA", "portB", "portP", "portT", "inlet", "outlet", "suction", "delivery", "target", "dcOut", "dcIn", "acOut", "acIn"
    QString label;
    QPointF localPos;
    bool isHydNode = false;
    bool isSupplyOut = false;
    bool isInverterDcIn = false;
    bool isInverterAcOut = false;
    bool isActuatorAcIn = false;
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

class SchematicView final : public QGraphicsView {
public:
    explicit SchematicView(QGraphicsScene* scene, QWidget* parent = nullptr) : QGraphicsView(scene, parent) {
        setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
        setDragMode(QGraphicsView::ScrollHandDrag);
        setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
        setBackgroundBrush(QColor(246, 248, 251));
    }
protected:
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
        QPen minorPen(QColor(232, 236, 242), 1.0);
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
    QString telemetry;
    int severity = 0; // 0 none, 1 warning, 2 error
    QColor accentColor{34, 118, 178};
    QVector<PinSpec> pins;
    QString activePinKey;
    std::function<void(const BlockKey&, const QPointF&)> onMovedCommit;
    std::function<void(const BlockKey&)> onDoubleClicked;
    std::function<bool(const BlockKey&, const QString& pinKey)> onPinClicked;

    SchematicBlockItem(BlockKey k, const QJsonObject& r) : key(std::move(k)), row(r) {
        setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
        setAcceptHoverEvents(true);
        configurePinsAndAppearance();
    }

    QRectF boundingRect() const override {
        return QRectF(-98, -52, 196, 110);
    }

    QPointF scenePinPos(const QString& pinKey) const {
        for (const auto& p : pins) if (p.key == pinKey) return mapToScene(p.localPos);
        return scenePos();
    }

    void configurePinsAndAppearance() {
        title = row.value("name").toString();
        subtitle = blockTypeTitle(key.collection, row);
        pins.clear();

        if (key.collection == "hydraulicNodes") {
            const QString nt = row.value("nodeType").toString("Chamber");
            accentColor = nt == "Tank" ? QColor(43, 123, 186) : nt == "PressureSource" ? QColor(210, 68, 68) : QColor(28, 138, 168);
            PinSpec p{"node", nt == "Tank" ? QStringLiteral("T") : QStringLiteral("P"), QPointF(0, 0), true};
            pins.push_back(p);
        } else if (key.collection == "hydraulicComponents") {
            accentColor = QColor(34, 118, 178);
            const QString ct = row.value("componentType").toString();
            if (ct == "Pump") {
                pins.push_back({"suctionNodeId", QStringLiteral("吸"), QPointF(-82, 4)});
                pins.push_back({"deliveryNodeId", QStringLiteral("压"), QPointF(82, 4)});
            } else if (ct == "DirectionalValve" || ct == "ServoValve") {
                pins.push_back({"portPNodeId", QStringLiteral("P"), QPointF(-82, -12)});
                pins.push_back({"portTNodeId", QStringLiteral("T"), QPointF(-82, 18)});
                pins.push_back({"portANodeId", QStringLiteral("A"), QPointF(82, -12)});
                pins.push_back({"portBNodeId", QStringLiteral("B"), QPointF(82, 18)});
            } else if (ct == "Accumulator") {
                pins.push_back({"targetNodeId", QStringLiteral("口"), QPointF(0, 38)});
            } else if (ct == "PortConnection") {
                pins.push_back({"targetNodeId", QStringLiteral("节"), QPointF(-82, 4)});
            } else {
                pins.push_back({"inletNodeId", QStringLiteral("入"), QPointF(-82, 4)});
                pins.push_back({"outletNodeId", QStringLiteral("出"), QPointF(82, 4)});
            }
        } else if (key.collection == "hydraulicCylinders" || key.collection == "hydraulicMotors") {
            accentColor = QColor(18, 112, 166);
            pins.push_back({"portANodeId", QStringLiteral("A"), QPointF(-82, -12)});
            pins.push_back({"portBNodeId", QStringLiteral("B"), QPointF(-82, 18)});
        } else if (key.collection == "powerSupplies") {
            accentColor = QColor(132, 82, 198);
            PinSpec p{"dcOut", QStringLiteral("DC+"), QPointF(82, 4)};
            p.isSupplyOut = true;
            pins.push_back(p);
        } else if (key.collection == "inverters") {
            accentColor = QColor(42, 152, 102);
            PinSpec pIn{"supplyId", QStringLiteral("DC"), QPointF(-82, 4)};
            pIn.isInverterDcIn = true;
            PinSpec pOut{"acOut", QStringLiteral("UVW"), QPointF(82, 4)};
            pOut.isInverterAcOut = true;
            pins.push_back(pIn);
            pins.push_back(pOut);
        } else if (key.collection == "electricMotors" || key.collection == "ehaActuators" || key.collection == "emaActuators") {
            accentColor = key.collection == "electricMotors" ? QColor(38, 156, 98) : QColor(142, 92, 205);
            PinSpec pIn{"inverterId", QStringLiteral("电"), QPointF(-82, 4)};
            pIn.isActuatorAcIn = true;
            pins.push_back(pIn);
        }
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) override {
        painter->save();
        const QString fontFam = MxFunctions::UiMetrics::uiFontFamily();
        const QRectF card(-82, -38, 164, 76);
        const bool isNode = key.collection == "hydraulicNodes";
        const QRectF bodyRect = isNode ? QRectF(-64, -27, 128, 54) : card;
        if (severity > 0) {
            const QColor halo = severity == 2 ? QColor(220, 53, 69, 85) : QColor(240, 158, 36, 90);
            painter->setPen(QPen(halo.darker(115), 2.2));
            painter->setBrush(halo);
            const qreal r = isNode ? 25.0 : 11.0;
            painter->drawRoundedRect(bodyRect.adjusted(-5, -5, 5, 5), r, r);
        }
        painter->setPen(QPen(isSelected() ? QColor(22, 96, 212) : accentColor.darker(115), isSelected() ? 2.4 : 1.5));
        painter->setBrush(QColor(255, 255, 255));
        painter->drawRoundedRect(bodyRect, isNode ? 22 : 8, isNode ? 22 : 8);

        // Top color header strip
        if (!isNode) {
            QPainterPath header;
            header.addRoundedRect(QRectF(bodyRect.left(), bodyRect.top(), bodyRect.width(), 22), 8, 8);
            painter->fillPath(header, accentColor);
            painter->fillRect(QRectF(bodyRect.left(), bodyRect.top() + 14, bodyRect.width(), 8), accentColor);
            painter->setPen(Qt::white);
            QFont hf(fontFam);
            hf.setPixelSize(12);
            hf.setBold(true);
            painter->setFont(hf);
            painter->drawText(QRectF(bodyRect.left() + 8, bodyRect.top() + 1, bodyRect.width() - 16, 20),
                              Qt::AlignCenter, subtitle);
        }

        // ISO/IEEE schematic glyph + entity name
        painter->setPen(QColor(30, 41, 59));
        QFont nf(fontFam);
        nf.setPixelSize(13);
        nf.setWeight(isNode ? QFont::Bold : QFont::DemiBold);
        painter->setFont(nf);
        const QRectF nameRect = isNode ? QRectF(-56, -21, 112, 22)
                                       : QRectF(bodyRect.left() + 18, bodyRect.top() + 26, bodyRect.width() - 36, 24);
        painter->drawText(nameRect, Qt::AlignCenter,
                          painter->fontMetrics().elidedText(title, Qt::ElideRight, int(nameRect.width())));

        if (isNode) {
            QFont sf(fontFam);
            sf.setPixelSize(11);
            sf.setBold(false);
            painter->setFont(sf);
            painter->setPen(accentColor.darker(120));
            painter->drawText(QRectF(-56, 2, 112, 18), Qt::AlignCenter, subtitle);
        }

        if (!telemetry.isEmpty()) {
            QFont tf(fontFam);
            tf.setPixelSize(11);
            tf.setBold(true);
            painter->setFont(tf);
            const QRectF badge(-76, bodyRect.bottom() + 4, 152, 18);
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(25, 35, 48, 225));
            painter->drawRoundedRect(badge, 4, 4);
            painter->setPen(QColor(138, 232, 184));
            painter->drawText(badge, Qt::AlignCenter, telemetry);
        }

        // Draw connection pins
        QFont pf(fontFam);
        pf.setPixelSize(11);
        pf.setBold(true);
        painter->setFont(pf);
        for (const auto& p : pins) {
            if (p.isHydNode) continue;
            const bool activePin = (p.key == activePinKey);
            painter->setPen(QPen(QColor(30, 40, 52), 1.3));
            painter->setBrush(activePin ? QColor(255, 193, 7) : accentColor.lighter(135));
            painter->drawEllipse(p.localPos, 6.0, 6.0);
            const QRectF lblRect(p.localPos.x() < 0 ? p.localPos.x() + 9
                                                    : (p.localPos.x() > 0 ? p.localPos.x() - 41 : -16),
                                 p.localPos.x() == 0 ? p.localPos.y() - 22 : p.localPos.y() - 8,
                                 32, 16);
            painter->setPen(QColor(51, 65, 85));
            const Qt::Alignment align = p.localPos.x() < 0 ? (Qt::AlignLeft | Qt::AlignVCenter)
                                      : p.localPos.x() > 0 ? (Qt::AlignRight | Qt::AlignVCenter)
                                                           : Qt::AlignCenter;
            painter->drawText(lblRect, align, p.label);
        }
        painter->restore();
    }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && onPinClicked) {
            for (const auto& p : pins) {
                const double r = p.isHydNode ? 28.0 : 12.0;
                if (QLineF(event->pos(), p.localPos).length() <= r) {
                    if (onPinClicked(key, p.key)) {
                        event->accept();
                        return;
                    }
                }
            }
        }
        m_pressPos = pos();
        QGraphicsObject::mousePressEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override {
        QGraphicsObject::mouseReleaseEvent(event);
        if (QLineF(m_pressPos, pos()).length() > 1.5 && onMovedCommit) {
            onMovedCommit(key, pos());
        }
    }

    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override {
        if (onDoubleClicked) onDoubleClicked(key);
        event->accept();
    }

private:
    QPointF m_pressPos;
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
    QLabel* statusLabel = nullptr;
    QSlider* frameSlider = nullptr;
    QLabel* frameTimeLabel = nullptr;
    bool initialFitDone = false;

    std::map<BlockKey, SchematicBlockItem*> blockItems;
    std::vector<QGraphicsPathItem*> wireItems;
    MotionDiagnosticReport lastReport;

    bool hasPendingPin = false;
    BlockKey pendingBlock;
    QString pendingPin;

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

    void commitAndRefresh(const QString& statusMsg = {}) {
        if (callbacks.commitDocument) {
            NativeMotionDocument copy = doc;
            callbacks.commitDocument(std::move(copy));
        }
        if (callbacks.runDiagnostics) {
            lastReport = callbacks.runDiagnostics(doc);
        }
        rebuildScene(false);
        if (!statusMsg.isEmpty()) statusLabel->setText(statusMsg);
    }

    void assignAutoLayout(bool overwriteExisting) {
        const auto placeLane = [&](const QString& col, double x, double yStart, double yStep) {
            QJsonArray arr = doc.collection(col);
            for (int i = 0; i < arr.size(); ++i) {
                auto o = arr[i].toObject();
                if (overwriteExisting || !o.contains("schemX") || !o.contains("schemY")) {
                    o.insert("schemX", x);
                    o.insert("schemY", yStart + i * yStep);
                    putEntity(col, o);
                }
            }
        };

        // Partition hydraulicNodes into sources (Tank/PressureSource) vs Chambers
        int srcIdx = 0, chmIdx = 0;
        for (const auto& v : doc.collection("hydraulicNodes")) {
            auto o = v.toObject();
            const QString nt = o.value("nodeType").toString("Chamber");
            if (overwriteExisting || !o.contains("schemX") || !o.contains("schemY")) {
                if (nt == "Tank" || nt == "PressureSource") {
                    o.insert("schemX", -410.0);
                    o.insert("schemY", -140.0 + (srcIdx++) * 108.0);
                } else {
                    o.insert("schemX", 30.0 + (chmIdx % 2) * 235.0);
                    o.insert("schemY", -140.0 + (chmIdx / 2) * 108.0);
                    ++chmIdx;
                }
                putEntity("hydraulicNodes", o);
            }
        }
        int pumpIdx = 0, valveIdx = 0;
        for (const auto& v : doc.collection("hydraulicComponents")) {
            auto o = v.toObject();
            const QString ct = o.value("componentType").toString();
            if (overwriteExisting || !o.contains("schemX") || !o.contains("schemY")) {
                if (ct == "Pump" || ct == "FlowSource") {
                    o.insert("schemX", -195.0);
                    o.insert("schemY", -140.0 + (pumpIdx++) * 112.0);
                } else {
                    o.insert("schemX", 240.0);
                    o.insert("schemY", -140.0 + (valveIdx++) * 112.0);
                }
                putEntity("hydraulicComponents", o);
            }
        }
        int hydActIdx = 0;
        for (const char* col : {"hydraulicCylinders", "hydraulicMotors"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                auto o = v.toObject();
                if (overwriteExisting || !o.contains("schemX") || !o.contains("schemY")) {
                    o.insert("schemX", 465.0);
                    o.insert("schemY", -140.0 + (hydActIdx++) * 114.0);
                    putEntity(QLatin1String(col), o);
                }
            }
        }

        // Electrical & Integrated Swimlane (compact lower section)
        placeLane("powerSupplies", -380.0, 115.0, 112.0);
        placeLane("inverters", -110.0, 115.0, 112.0);
        int elecActIdx = 0;
        for (const char* col : {"electricMotors", "ehaActuators", "emaActuators"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                auto o = v.toObject();
                if (overwriteExisting || !o.contains("schemX") || !o.contains("schemY")) {
                    o.insert("schemX", 200.0);
                    o.insert("schemY", 95.0 + (elecActIdx++) * 108.0);
                    putEntity(QLatin1String(col), o);
                }
            }
        }
    }

    bool handlePinClick(const BlockKey& clickedKey, const QString& pinKey) {
        if (!wireModeBtn || !wireModeBtn->isChecked()) return false;
        if (!hasPendingPin) {
            hasPendingPin = true;
            pendingBlock = clickedKey;
            pendingPin = pinKey;
            if (blockItems.count(clickedKey)) {
                blockItems[clickedKey]->activePinKey = pinKey;
                blockItems[clickedKey]->update();
            }
            statusLabel->setText(QStringLiteral("连线起点已选：%1 (%2) — 请点击目标节点或端口完成连接。")
                                     .arg(doc.entity(clickedKey.collection, clickedKey.id).value("name").toString(), pinKey));
            return true;
        }

        const BlockKey firstBlock = pendingBlock;
        const QString firstPin = pendingPin;
        hasPendingPin = false;
        if (blockItems.count(firstBlock)) {
            blockItems[firstBlock]->activePinKey.clear();
            blockItems[firstBlock]->update();
        }
        if (firstBlock == clickedKey && firstPin == pinKey) {
            statusLabel->setText(QStringLiteral("已取消当前连线选择。"));
            return true;
        }

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
            if (portOwner.collection == "hydraulicNodes" || portField == "node") return false;
            auto ownerRow = doc.entity(portOwner.collection, portOwner.id);
            if (ownerRow.isEmpty()) return false;
            ownerRow.insert(portField, nodeId);
            if (portOwner.collection == "hydraulicCylinders" || portOwner.collection == "hydraulicMotors") {
                ownerRow.insert("supplyMode", QStringLiteral("ExternalCircuit"));
            }
            putEntity(portOwner.collection, ownerRow);
            commitAndRefresh(QStringLiteral("已连接 %1.%2 → 液压节点 [%3]。")
                                 .arg(ownerRow.value("name").toString(), portField).arg(nodeId));
            return true;
        };
        if (firstBlock.collection == "hydraulicNodes" && clickedKey.collection != "hydraulicNodes") {
            if (wireHydPortToNode(clickedKey, pinKey, firstBlock.id)) return true;
        }
        if (clickedKey.collection == "hydraulicNodes" && firstBlock.collection != "hydraulicNodes") {
            if (wireHydPortToNode(firstBlock, firstPin, clickedKey.id)) return true;
        }

        // Case 4: Two hydraulic component/actuator ports clicked directly -> auto-create intermediate Chamber node!
        const auto isHydPortOwner = [](const QString& c) {
            return c == "hydraulicComponents" || c == "hydraulicCylinders" || c == "hydraulicMotors";
        };
        if (isHydPortOwner(firstBlock.collection) && isHydPortOwner(clickedKey.collection)) {
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
                {"initialPressureMPa", 1.0},
                {"schemX", mid.x()},
                {"schemY", mid.y()}
            };
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

        statusLabel->setText(QStringLiteral("所选两端端口物理域不匹配（请连接液压端口↔液压节点，或电源↔逆变器↔电机/EHA/EMA）。"));
        return true;
    }

    void drawWire(const QPointF& p1, const QPointF& p2, const QColor& color, bool dashed = false) {
        QPainterPath path(p1);
        const double dx = std::max(36.0, std::abs(p2.x() - p1.x()) * 0.45);
        path.cubicTo(QPointF(p1.x() + (p2.x() >= p1.x() ? dx : -dx), p1.y()),
                     QPointF(p2.x() - (p2.x() >= p1.x() ? dx : -dx), p2.y()),
                     p2);
        auto* item = scene->addPath(path, QPen(color, 2.3, dashed ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        item->setZValue(-10.0);
        wireItems.push_back(item);
    }

    void rebuildWires() {
        for (auto* w : wireItems) {
            scene->removeItem(w);
            delete w;
        }
        wireItems.clear();

        const auto nodeColor = [&](int nid, const QColor& fallback) {
            const auto n = doc.entity("hydraulicNodes", nid);
            const QString nt = n.value("nodeType").toString();
            if (nt == "Tank") return QColor(43, 123, 186);
            if (nt == "PressureSource") return QColor(214, 69, 69);
            return fallback;
        };

        const auto connectToHydNode = [&](const BlockKey& owner, const QString& pinKey, int nid, const QColor& color) {
            if (nid < 0) return;
            const BlockKey nk{"hydraulicNodes", nid};
            if (!blockItems.count(owner) || !blockItems.count(nk)) return;
            drawWire(blockItems[owner]->scenePinPos(pinKey),
                     blockItems[nk]->scenePinPos("node"),
                     nodeColor(nid, color));
        };

        for (const auto& v : doc.collection("hydraulicComponents")) {
            const auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            const int cid = o.value("id").toInt();
            const BlockKey bk{"hydraulicComponents", cid};
            const QString ct = o.value("componentType").toString();
            if (ct == "Pump") {
                connectToHydNode(bk, "suctionNodeId", o.value("suctionNodeId").toInt(-1), QColor(43, 123, 186));
                connectToHydNode(bk, "deliveryNodeId", o.value("deliveryNodeId").toInt(-1), QColor(214, 69, 69));
            } else if (ct == "DirectionalValve" || ct == "ServoValve") {
                connectToHydNode(bk, "portPNodeId", o.value("portPNodeId").toInt(-1), QColor(214, 69, 69));
                connectToHydNode(bk, "portTNodeId", o.value("portTNodeId").toInt(-1), QColor(43, 123, 186));
                connectToHydNode(bk, "portANodeId", o.value("portANodeId").toInt(-1), QColor(217, 130, 43));
                connectToHydNode(bk, "portBNodeId", o.value("portBNodeId").toInt(-1), QColor(217, 130, 43));
            } else if (ct == "Accumulator") {
                connectToHydNode(bk, "targetNodeId", o.value("targetNodeId").toInt(-1), QColor(28, 138, 168));
            } else if (ct == "PortConnection") {
                const int nid = o.value("targetNodeId").toInt(-1);
                connectToHydNode(bk, "targetNodeId", nid, QColor(217, 130, 43));
                const QString actCol = o.value("actuatorType").toString("Cylinder") == "Motor"
                                           ? QStringLiteral("hydraulicMotors")
                                           : QStringLiteral("hydraulicCylinders");
                const BlockKey ak{actCol, o.value("actuatorId").toInt(-1)};
                const QString portPin = o.value("actuatorPort").toString("A") == "B" ? QStringLiteral("portBNodeId") : QStringLiteral("portANodeId");
                if (blockItems.count(bk) && blockItems.count(ak)) {
                    drawWire(blockItems[bk]->scenePos(), blockItems[ak]->scenePinPos(portPin), QColor(217, 130, 43), true);
                }
            } else {
                connectToHydNode(bk, "inletNodeId", o.value("inletNodeId").toInt(-1), QColor(214, 69, 69));
                connectToHydNode(bk, "outletNodeId", o.value("outletNodeId").toInt(-1), QColor(43, 123, 186));
            }
        }

        for (const char* col : {"hydraulicCylinders", "hydraulicMotors"}) {
            for (const auto& v : doc.collection(QLatin1String(col))) {
                const auto o = v.toObject();
                if (!o.value("enabled").toBool(true) || o.value("supplyMode").toString() != "ExternalCircuit") continue;
                const BlockKey bk{QLatin1String(col), o.value("id").toInt()};
                connectToHydNode(bk, "portANodeId", o.value("portANodeId").toInt(-1), QColor(217, 130, 43));
                connectToHydNode(bk, "portBNodeId", o.value("portBNodeId").toInt(-1), QColor(217, 130, 43));
            }
        }

        for (const auto& v : doc.collection("inverters")) {
            const auto o = v.toObject();
            if (!o.value("enabled").toBool(true)) continue;
            const int iid = o.value("id").toInt();
            const int sid = o.value("supplyId").toInt(-1);
            const BlockKey ibk{"inverters", iid};
            const BlockKey sbk{"powerSupplies", sid};
            if (sid >= 0 && blockItems.count(ibk) && blockItems.count(sbk)) {
                drawWire(sbk == ibk ? QPointF() : blockItems[sbk]->scenePinPos("dcOut"),
                         blockItems[ibk]->scenePinPos("supplyId"),
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
                    drawWire(blockItems[ibk]->scenePinPos("acOut"),
                             blockItems[abk]->scenePinPos("inverterId"),
                             QColor(42, 152, 102));
                }
            }
        }
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
                auto* blk = new SchematicBlockItem(k, o);
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
                blk->onDoubleClicked = [this](const BlockKey& key) {
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
        const QRectF fitBounds = scene->itemsBoundingRect().adjusted(-48, -38, 48, 38);
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
    : QDialog(parent), m_impl(std::make_unique<Impl>())
{
    setObjectName(QStringLiteral("multiphysicsSchematicDialog"));
    setWindowTitle(tr("机电液回路原理图与拓扑工作台"));
    resize(1240, 740);
    setMinimumSize(940, 560);

    m_impl->doc = initialDocument;
    m_impl->solvedResult = solvedResult;
    m_impl->frameIndex = std::max(0, initialFrameIndex);
    m_impl->callbacks = std::move(callbacks);
    if (m_impl->callbacks.runDiagnostics) {
        m_impl->lastReport = m_impl->callbacks.runDiagnostics(m_impl->doc);
    }

    const QString fontFam = MxFunctions::UiMetrics::uiFontFamily();
    const int btnFontPx = std::max(13, MxFunctions::UiMetrics::bodyFontSize() + 1);
    const int btnOuterH = std::max(32, MxFunctions::UiMetrics::controlHeight() + 6);
    const int btnInnerH = btnOuterH - 2;

    QFont btnFont(fontFam);
    btnFont.setPixelSize(btnFontPx);
    btnFont.setWeight(QFont::Medium);
    const QFontMetrics btnFm(btnFont);

    QFont sectionFont(fontFam);
    sectionFont.setPixelSize(std::max(12, btnFontPx - 1));
    sectionFont.setWeight(QFont::DemiBold);

    const auto styleActionBtn = [&](QPushButton* btn, bool accentToggle = false) {
        btn->setProperty("mxBtnNoTheme", true);
        btn->setFont(btnFont);
        btn->setCursor(Qt::PointingHandCursor);
        const int contentW = std::max(68, btnFm.horizontalAdvance(btn->text()) + 28);
        const int outerW = contentW + 2;
        btn->setMinimumSize(outerW, btnOuterH);
        btn->setFixedHeight(btnOuterH);
        btn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);

        const QString qss = accentToggle
            ? QStringLiteral(
                  "QPushButton { font-family:'%1'; font-size:%2px; font-weight:600;"
                  " width:%3px; min-width:%3px; height:%4px; min-height:%4px; padding:0px;"
                  " color:%5; background-color:%6; border:1px solid %7; border-radius:5px; }"
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
            : QStringLiteral(
                  "QPushButton { font-family:'%1'; font-size:%2px; font-weight:500;"
                  " width:%3px; min-width:%3px; height:%4px; min-height:%4px; padding:0px;"
                  " color:%5; background-color:%6; border:1px solid %7; border-radius:5px; }"
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
                       MxFunctions::UiPalette::selectedTextHex());
        btn->setStyleSheet(qss);
    };

    const auto makeSectionTag = [&](const QString& text, QWidget* host) {
        auto* lbl = new QLabel(text, host);
        lbl->setFont(sectionFont);
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

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 10, 12, 10);
    rootLayout->setSpacing(8);

    auto* toolbar = new QFrame(this);
    toolbar->setObjectName(QStringLiteral("schematicToolbar"));
    toolbar->setStyleSheet(QStringLiteral(
        "QFrame#schematicToolbar { background-color:%1; border:1px solid %2; border-radius:6px; }")
        .arg(MxFunctions::UiPalette::panelHex(), MxFunctions::UiPalette::borderHex()));
    auto* toolbarRows = new QVBoxLayout(toolbar);
    toolbarRows->setContentsMargins(10, 8, 10, 8);
    toolbarRows->setSpacing(6);

    auto* createRow = new QHBoxLayout;
    createRow->setContentsMargins(0, 0, 0, 0);
    createRow->setSpacing(6);

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
            created.insert("schemX", center.x());
            created.insert("schemY", center.y());
            m_impl->putEntity(col, created);
            m_impl->commitAndRefresh(tr("已添加 %1“%2”。").arg(blockTypeTitle(col, created), created.value("name").toString()));
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
        m_impl->statusLabel->setText(on ? tr("连线模式开启：依次点击元件端口圆点与目标节点（或两个油口自动建腔）即可完成拓扑连接。")
                                        : tr("拖拽浏览模式：可拖动方块调整排布，双击方块打开参数编辑面板。"));
        m_impl->rebuildScene(false);
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

    m_impl->scene = new QGraphicsScene(this);
    m_impl->view = new SchematicView(m_impl->scene, this);
    m_impl->view->setObjectName(QStringLiteral("multiphysicsSchematicView"));
    m_impl->view->setFrameShape(QFrame::StyledPanel);
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

    if (solvedResult && !solvedResult->frames.empty()) {
        auto* scrubRow = new QHBoxLayout;
        scrubRow->setSpacing(8);
        auto* scrubTitle = new QLabel(tr("结果帧动态监测:"), this);
        scrubTitle->setFont(sectionFont);
        m_impl->frameSlider = new QSlider(Qt::Horizontal, this);
        m_impl->frameSlider->setObjectName(QStringLiteral("schemTimelineSlider"));
        m_impl->frameSlider->setRange(0, int(solvedResult->frames.size()) - 1);
        m_impl->frameSlider->setValue(std::min(m_impl->frameIndex, int(solvedResult->frames.size()) - 1));
        m_impl->frameTimeLabel = new QLabel(this);
        m_impl->frameTimeLabel->setFont(btnFont);
        scrubRow->addWidget(scrubTitle);
        scrubRow->addWidget(m_impl->frameSlider, 1);
        scrubRow->addWidget(m_impl->frameTimeLabel);
        rootLayout->addLayout(scrubRow);
        connect(m_impl->frameSlider, &QSlider::valueChanged, this, [this](int idx) {
            m_impl->frameIndex = idx;
            m_impl->updateTelemetryLabels();
        });
    }

    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(12);
    m_impl->statusLabel = new QLabel(tr("提示：拖拽元件调整布局；开启“端口连线模式”点击端口连接管路/母线；双击任意元件编辑参数。"), this);
    m_impl->statusLabel->setObjectName(QStringLiteral("schemStatusLabel"));
    m_impl->statusLabel->setFont(btnFont);
    m_impl->statusLabel->setWordWrap(true);
    auto* closeBtn = new QPushButton(tr("关闭"), this);
    closeBtn->setObjectName(QStringLiteral("schemCloseButton"));
    styleActionBtn(closeBtn, false);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottomRow->addWidget(m_impl->statusLabel, 1);
    bottomRow->addWidget(closeBtn);
    rootLayout->addLayout(bottomRow);

    m_impl->rebuildScene(false);
    m_impl->updateTelemetryLabels();
}

void MultiphysicsSchematicDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    if (!m_impl->initialFitDone) {
        m_impl->initialFitDone = true;
        if (layout()) layout()->activate();
        m_impl->fitSceneComfortably();
        QTimer::singleShot(0, this, [this] {
            if (isVisible()) m_impl->fitSceneComfortably();
        });
    }
}

MultiphysicsSchematicDialog::~MultiphysicsSchematicDialog() = default;

const NativeMotionDocument& MultiphysicsSchematicDialog::currentDocument() const
{
    return m_impl->doc;
}

bool MultiphysicsSchematicDialog::exportPng(const QString& filePath) const
{
    const QRectF bounds = m_impl->scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    const QSize imgSize(std::max(640, int(std::ceil(bounds.width() * 1.5))),
                        std::max(400, int(std::ceil(bounds.height() * 1.5))));
    QImage image(imgSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor(248, 250, 252));
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    m_impl->scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(imgSize)), bounds);
    painter.end();
    return image.save(filePath);
}

} // namespace MotionOcc
