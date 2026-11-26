#include "NativeMotionDiagnostics.h"
#include "CadDocument.h"

#include <Eigen/Eigenvalues>

#include <QJsonArray>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <cmath>

namespace MotionOcc {
namespace {

bool active(const QJsonObject& row)
{
    return row.value("enabled").toBool(true);
}

void addItem(MotionDiagnosticReport& report,
             MotionDiagnosticItem::Severity severity,
             const QString& category,
             const QString& collection,
             int entityId,
             const QString& entityName,
             const QString& message)
{
    if (severity == MotionDiagnosticItem::Severity::Error) ++report.errorCount;
    else if (severity == MotionDiagnosticItem::Severity::Warning) ++report.warningCount;
    report.items.push_back({severity, category, collection, entityId, entityName, message});
}

bool checkBodyInertia(const QJsonObject& body, QString& detail)
{
    if (body.value("isFixed").toBool(false)) return true;
    const double mass = body.value("mass").toDouble(0.0);
    if (!std::isfinite(mass) || mass <= 0.0) {
        detail = QStringLiteral("活动运动体质量无效（mass = %1 kg），必须为正有限值。").arg(mass);
        return false;
    }
    const auto arr = body.value("inertia").toArray();
    if (arr.size() != 6) {
        detail = QStringLiteral("惯量张量分量数应为 6（Ixx, Iyy, Izz, Ixy, Ixz, Iyz），当前为 %1。").arg(arr.size());
        return false;
    }
    double vals[6]{};
    for (int i = 0; i < 6; ++i) {
        vals[i] = arr[i].toDouble(NAN);
        if (!std::isfinite(vals[i])) {
            detail = QStringLiteral("惯量分量包含非有限数值。");
            return false;
        }
    }
    const QString conv = body.value("inertiaConvention").toString("tensor");
    const double sign = conv == QLatin1String("products") ? -1.0 : 1.0;
    Eigen::Matrix3d J;
    J << vals[0], sign * vals[3], sign * vals[4],
         sign * vals[3], vals[1], sign * vals[5],
         sign * vals[4], sign * vals[5], vals[2];

    Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(J);
    if (eig.info() != Eigen::Success) {
        detail = QStringLiteral("无法求解主惯量特征值。");
        return false;
    }
    const auto p = eig.eigenvalues();
    const double scale = std::max(1e-12, p(2));
    if (p(0) <= 1e-12 * scale) {
        detail = QStringLiteral("惯量张量非正定（最小主惯量 = %1 kg·mm²，约定=%2）。请检查交叉项符号约定。")
                     .arg(p(0), 0, 'g', 6).arg(conv);
        return false;
    }
    if (p(0) + p(1) + 1e-6 * scale < p(2)) {
        detail = QStringLiteral("主惯量违反三角不等式（I1 + I2 = %1 < I3 = %2 kg·mm²，约定=%3）。")
                     .arg(p(0) + p(1), 0, 'g', 6).arg(p(2), 0, 'g', 6).arg(conv);
        return false;
    }
    return true;
}

void checkHydraulicCircuit(const NativeMotionDocument& document, MotionDiagnosticReport& report)
{
    const auto fluid = document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double defaultSupplyMPa = fluid.value("supplyPressure").toDouble(21.0);

    QMap<int, QJsonObject> activeNodes;
    bool hasReferenceBoundary = false;
    for (const auto& v : document.collection("hydraulicNodes")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        const int id = o.value("id").toInt(-1);
        activeNodes.insert(id, o);
        ++report.hydraulicNodes;
        const QString ntype = o.value("nodeType").toString();
        if (ntype == QLatin1String("Tank") || ntype == QLatin1String("PressureSource"))
            hasReferenceBoundary = true;
    }

    QSet<int> touchedNodes;
    QSet<int> reliefProtectedNodes;
    QMap<QString, int> boundPorts; // key: "C:1:A" or "M:2:B" -> nodeId

    const auto markNode = [&](int nid) {
        if (nid >= 0) touchedNodes.insert(nid);
    };

    for (const auto& v : document.collection("hydraulicCylinders")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.hydraulicActuators;
        const int id = o.value("id").toInt(-1);
        if (o.value("supplyMode").toString() == QLatin1String("ExternalCircuit")) {
            const int na = o.value("portANodeId").toInt(-1);
            const int nb = o.value("portBNodeId").toInt(-1);
            if (na >= 0) { boundPorts.insert(QStringLiteral("C:%1:A").arg(id), na); markNode(na); }
            if (nb >= 0) { boundPorts.insert(QStringLiteral("C:%1:B").arg(id), nb); markNode(nb); }
        }
    }
    for (const auto& v : document.collection("hydraulicMotors")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.hydraulicActuators;
        const int id = o.value("id").toInt(-1);
        if (o.value("supplyMode").toString() == QLatin1String("ExternalCircuit")) {
            const int na = o.value("portANodeId").toInt(-1);
            const int nb = o.value("portBNodeId").toInt(-1);
            if (na >= 0) { boundPorts.insert(QStringLiteral("M:%1:A").arg(id), na); markNode(na); }
            if (nb >= 0) { boundPorts.insert(QStringLiteral("M:%1:B").arg(id), nb); markNode(nb); }
        }
    }

    int hydIssuesBefore = report.errorCount + report.warningCount;
    double maxReliefMPa = defaultSupplyMPa;

    for (const auto& v : document.collection("hydraulicComponents")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.hydraulicComponents;
        const int cid = o.value("id").toInt(-1);
        const QString cname = o.value("name").toString();
        const QString ctype = o.value("componentType").toString();

        const auto checkPortNode = [&](const char* key, const QString& portTitle) -> int {
            const int nid = o.value(QLatin1String(key)).toInt(-1);
            if (nid < 0 || !activeNodes.contains(nid)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("%1 未关联已启用的液压节点。").arg(portTitle));
                return -1;
            }
            markNode(nid);
            return nid;
        };

        if (ctype == QLatin1String("PortConnection")) {
            const int nid = checkPortNode("targetNodeId", QStringLiteral("回路节点"));
            const bool isMotor = o.value("actuatorType").toString("Cylinder") == QLatin1String("Motor");
            const int actId = o.value("actuatorId").toInt(-1);
            const QString port = o.value("actuatorPort").toString("A");
            const auto actObj = document.entity(isMotor ? QStringLiteral("hydraulicMotors") : QStringLiteral("hydraulicCylinders"), actId);
            if (actId < 0 || actObj.isEmpty() || !active(actObj)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("端口连接未指定已启用的液压缸或液压马达。"));
            } else if (nid >= 0) {
                const QString pKey = QStringLiteral("%1:%2:%3").arg(isMotor ? QStringLiteral("M") : QStringLiteral("C")).arg(actId).arg(port);
                if (boundPorts.contains(pKey) && boundPorts.value(pKey) != nid) {
                    addItem(report, MotionDiagnosticItem::Severity::Error,
                            QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                            cid, cname, QStringLiteral("执行器同一油口被重复绑定到不同的液压节点。"));
                }
                boundPorts.insert(pKey, nid);
            }
        } else if (ctype == QLatin1String("Accumulator")) {
            const int nid = checkPortNode("targetNodeId", QStringLiteral("挂载节点"));
            if (nid >= 0 && activeNodes.value(nid).value("nodeType").toString() != QLatin1String("Chamber")
                && !boundPorts.values().contains(nid)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("蓄能器需挂载在容腔节点或执行器油口节点上，不能直接挂在恒压边界节点。"));
            }
        } else if (ctype == QLatin1String("Pump")) {
            const int suc = checkPortNode("suctionNodeId", QStringLiteral("吸油口"));
            const int del = checkPortNode("deliveryNodeId", QStringLiteral("压油口"));
            if (suc >= 0 && suc == del) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("液压泵吸油口与压油口不能连接到同一节点。"));
            }
        } else if (ctype == QLatin1String("DirectionalValve") || ctype == QLatin1String("ServoValve")) {
            const int np = checkPortNode("portPNodeId", QStringLiteral("P口"));
            const int nt = checkPortNode("portTNodeId", QStringLiteral("T口"));
            const int na = checkPortNode("portANodeId", QStringLiteral("A口"));
            const int nb = checkPortNode("portBNodeId", QStringLiteral("B口"));
            if (np >= 0 && nt >= 0 && np == nt) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("四通阀供油口 P 与回油口 T 不能短接到同一节点。"));
            }
            if (na >= 0 && nb >= 0 && na == nb) {
                addItem(report, MotionDiagnosticItem::Severity::Warning,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("四通阀工作口 A 与 B 连接到同一节点，形成差动或旁路短接。"));
            }
        } else {
            const int n1 = checkPortNode("inletNodeId", QStringLiteral("入口节点"));
            const int n2 = checkPortNode("outletNodeId", QStringLiteral("出口节点"));
            if (n1 >= 0 && n1 == n2) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname, QStringLiteral("支路元件入口与出口不能连接到同一节点。"));
            }
            if (ctype == QLatin1String("ReliefValve") && n1 >= 0) {
                reliefProtectedNodes.insert(n1);
                maxReliefMPa = std::max(maxReliefMPa, o.value("crackingPressureMPa").toDouble(21.0));
            }
        }
    }

    // Check ExternalCircuit actuators for unconnected ports
    for (const auto& v : document.collection("hydraulicCylinders")) {
        const auto o = v.toObject();
        if (!active(o) || o.value("supplyMode").toString() != QLatin1String("ExternalCircuit")) continue;
        const int id = o.value("id").toInt(-1);
        const bool hasA = boundPorts.contains(QStringLiteral("C:%1:A").arg(id));
        const bool hasB = boundPorts.contains(QStringLiteral("C:%1:B").arg(id));
        if (!hasA && !hasB) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("液压回路"), QStringLiteral("hydraulicCylinders"),
                    id, o.value("name").toString(),
                    QStringLiteral("已设为“外部液压回路”模式，但 A 口与 B 口均未连接任何回路节点。"));
        } else if (!hasA || !hasB) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("hydraulicCylinders"),
                    id, o.value("name").toString(),
                    QStringLiteral("%1 口未连接外部回路节点，求解时该腔按封闭容腔处理。").arg(!hasA ? QStringLiteral("A") : QStringLiteral("B")));
        }
    }
    for (const auto& v : document.collection("hydraulicMotors")) {
        const auto o = v.toObject();
        if (!active(o) || o.value("supplyMode").toString() != QLatin1String("ExternalCircuit")) continue;
        const int id = o.value("id").toInt(-1);
        const bool hasA = boundPorts.contains(QStringLiteral("M:%1:A").arg(id));
        const bool hasB = boundPorts.contains(QStringLiteral("M:%1:B").arg(id));
        if (!hasA && !hasB) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("液压回路"), QStringLiteral("hydraulicMotors"),
                    id, o.value("name").toString(),
                    QStringLiteral("已设为“外部液压回路”模式，但 A 口与 B 口均未连接任何回路节点。"));
        } else if (!hasA || !hasB) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("hydraulicMotors"),
                    id, o.value("name").toString(),
                    QStringLiteral("%1 口未连接外部回路节点，求解时该口按封闭口处理。").arg(!hasA ? QStringLiteral("A") : QStringLiteral("B")));
        }
    }

    // Engineering topology warnings: Pump relief protection, isolated nodes, missing tank/reference, accumulator precharge
    for (const auto& v : document.collection("hydraulicComponents")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        const int cid = o.value("id").toInt(-1);
        const QString cname = o.value("name").toString();
        const QString ctype = o.value("componentType").toString();
        if (ctype == QLatin1String("Pump")) {
            const int del = o.value("deliveryNodeId").toInt(-1);
            if (del >= 0 && activeNodes.contains(del)
                && activeNodes.value(del).value("nodeType").toString() == QLatin1String("Chamber")
                && !reliefProtectedNodes.contains(del)) {
                addItem(report, MotionDiagnosticItem::Severity::Warning,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname,
                        QStringLiteral("容积泵压油节点未配置溢流阀安全保护，换向阀中位封闭或行程到底时可能导致异常憋压。"));
            }
        } else if (ctype == QLatin1String("Accumulator")) {
            const double p0 = o.value("prechargePressureMPa").toDouble(10.0);
            if (p0 >= maxReliefMPa) {
                addItem(report, MotionDiagnosticItem::Severity::Warning,
                        QStringLiteral("液压回路"), QStringLiteral("hydraulicComponents"),
                        cid, cname,
                        QStringLiteral("蓄能器预充气压（%1 MPa）高于或等于系统供油/溢流压力（%2 MPa），油液可能无法充入蓄能器。")
                            .arg(p0, 0, 'g', 5).arg(maxReliefMPa, 0, 'g', 5));
            }
        }
    }

    if ((report.hydraulicNodes > 0 || report.hydraulicComponents > 0) && !hasReferenceBoundary) {
        addItem(report, MotionDiagnosticItem::Severity::Warning,
                QStringLiteral("液压回路"), {}, -1, QStringLiteral("回路压力基准"),
                QStringLiteral("当前液压回路未配置油箱或恒压源边界节点，建议添加油箱节点提供回油压力基准。"));
    }

    for (auto it = activeNodes.cbegin(); it != activeNodes.cend(); ++it) {
        if (!touchedNodes.contains(it.key())) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("hydraulicNodes"),
                    it.key(), it.value().value("name").toString(),
                    QStringLiteral("孤立液压节点：未连接任何启用的液压元件或执行器端口。"));
        }
    }

    if ((report.hydraulicActuators > 0 || report.hydraulicNodes > 0 || report.hydraulicComponents > 0)
        && (report.errorCount + report.warningCount == hydIssuesBefore)) {
        addItem(report, MotionDiagnosticItem::Severity::Pass,
                QStringLiteral("液压回路"), {}, -1, QStringLiteral("液压拓扑"),
                QStringLiteral("液压回路拓扑与元件参数检查通过（执行器 %1，节点 %2，回路元件 %3）。")
                    .arg(report.hydraulicActuators).arg(report.hydraulicNodes).arg(report.hydraulicComponents));
    }

    // Electrical Drive & Integrated EHA/EMA Diagnostics (also included in 回路检查)
    const int elecIssuesBefore = report.errorCount + report.warningCount;
    QMap<int, QJsonObject> activeSupplies;
    QSet<int> touchedSupplies;
    for (const auto& v : document.collection("powerSupplies")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.electricPowerDevices;
        const int sid = o.value("id").toInt(-1);
        activeSupplies.insert(sid, o);
        const double nomV = o.value("nominalVoltage").toDouble(270.0);
        const double chopV = o.value("chopperVoltage").toDouble(0.0);
        if (chopV > 0.0 && chopV <= nomV) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("powerSupplies"),
                    sid, o.value("name").toString(),
                    QStringLiteral("制动斩波开启电压（%1 V）不高于额定母线电压（%2 V），稳态时将持续泄放功率。")
                        .arg(chopV, 0, 'g', 5).arg(nomV, 0, 'g', 5));
        }
    }

    QMap<int, QJsonObject> activeInverters;
    QMap<int, QString> inverterOwners;
    for (const auto& v : document.collection("inverters")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.electricPowerDevices;
        const int iid = o.value("id").toInt(-1);
        activeInverters.insert(iid, o);
        const int sid = o.value("supplyId").toInt(-1);
        if (sid >= 0) {
            if (!activeSupplies.contains(sid)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QStringLiteral("inverters"),
                        iid, o.value("name").toString(),
                        QStringLiteral("逆变器关联的直流电源（ID=%1）未启用或不存在。").arg(sid));
            } else {
                touchedSupplies.insert(sid);
            }
        }
    }

    const auto checkInverterBinding = [&](const char* col, const QJsonObject& o, const QString& typeTitle) {
        if (!active(o)) return;
        ++report.electricActuators;
        const int aid = o.value("id").toInt(-1);
        const QString aname = o.value("name").toString();
        if (o.value("controlMode").toString() == QLatin1String("ExternalInverter")) {
            const int iid = o.value("inverterId").toInt(-1);
            if (iid < 0 || !activeInverters.contains(iid)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QLatin1String(col),
                        aid, aname,
                        QStringLiteral("%1 设为“外部逆变器”模式，但未绑定已启用的伺服逆变器。").arg(typeTitle));
            } else if (inverterOwners.contains(iid)) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("液压回路"), QLatin1String(col),
                        aid, aname,
                        QStringLiteral("伺服逆变器（ID=%1）已被“%2”占用，单台逆变器不能并联驱动多个伺服执行器。")
                            .arg(iid).arg(inverterOwners.value(iid)));
            } else {
                inverterOwners.insert(iid, aname);
            }
        }
    };

    for (const auto& v : document.collection("electricMotors"))
        checkInverterBinding("electricMotors", v.toObject(), QStringLiteral("伺服电机"));
    for (const auto& v : document.collection("ehaActuators"))
        checkInverterBinding("ehaActuators", v.toObject(), QStringLiteral("EHA 作动器"));
    for (const auto& v : document.collection("emaActuators"))
        checkInverterBinding("emaActuators", v.toObject(), QStringLiteral("EMA 作动器"));

    for (auto it = activeInverters.cbegin(); it != activeInverters.cend(); ++it) {
        if (!inverterOwners.contains(it.key())) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("inverters"),
                    it.key(), it.value().value("name").toString(),
                    QStringLiteral("伺服逆变器尚未绑定任何电机、EHA 或 EMA 作动器。"));
        }
    }
    for (auto it = activeSupplies.cbegin(); it != activeSupplies.cend(); ++it) {
        if (!touchedSupplies.contains(it.key())) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("液压回路"), QStringLiteral("powerSupplies"),
                    it.key(), it.value().value("name").toString(),
                    QStringLiteral("直流电源未向任何启用的伺服逆变器供电。"));
        }
    }

    if ((report.electricActuators > 0 || report.electricPowerDevices > 0)
        && (report.errorCount + report.warningCount == elecIssuesBefore)) {
        addItem(report, MotionDiagnosticItem::Severity::Pass,
                QStringLiteral("液压回路"), {}, -1, QStringLiteral("电气与集成作动拓扑"),
                QStringLiteral("电驱与 EHA/EMA 作动拓扑检查通过（电驱执行器 %1，电源/逆变单元 %2）。")
                    .arg(report.electricActuators).arg(report.electricPowerDevices));
    }
}

} // namespace

MotionDiagnosticReport diagnoseNativeMotion(const CadDocument& cad,
                                            const NativeMotionDocument& document,
                                            int scenarioId)
{
    MotionDiagnosticReport report;

    // 1. Validate schema & document rules
    QStringList docIssues;
    validateNativeMotionDocument(document, docIssues);
    for (const auto& issue : docIssues) {
        addItem(report, MotionDiagnosticItem::Severity::Error,
                QStringLiteral("模型结构"), {}, -1, QStringLiteral("文档契约"), issue);
    }

    QString bindError;
    if (!validateNativeMotionBindings(cad, document, bindError) && !bindError.isEmpty()) {
        addItem(report, MotionDiagnosticItem::Severity::Warning,
                QStringLiteral("几何绑定"), {}, -1, QStringLiteral("几何引用"), bindError);
    }

    // 2. Bodies, Mass/Inertia & CAD Topology Checks
    QSet<int> connectedBodies;
    const auto markBody = [&](int id) { if (id >= 0) connectedBodies.insert(id); };

    for (const auto& v : document.collection("joints")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.activeJoints;
        const int a = o.value("actionBodyId").toInt(-1);
        const int b = o.value("baseBodyId").toInt(-1);
        markBody(a);
        markBody(b);
        if (a == b) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("约束与自由度"), QStringLiteral("joints"),
                    o.value("id").toInt(-1), o.value("name").toString(),
                    QStringLiteral("动作体与基本体相同，形成自连接无效约束。"));
        }
    }
    for (const auto& v : document.collection("drives")) {
        if (active(v.toObject())) ++report.activeDrives;
    }
    for (const auto& v : document.collection("bodyDrives")) {
        const auto o = v.toObject();
        if (!active(o)) continue;
        ++report.activeDrives;
        markBody(o.value("targetBodyId").toInt(-1));
        markBody(o.value("referenceBodyId").toInt(-1));
    }
    for (const auto* col : {"springs", "tensionCables", "softStops", "vectorForces",
                            "scalarForces", "scalarTorques", "threePointForces",
                            "threePointTorques", "contacts", "hydraulicCylinders", "hydraulicMotors",
                            "electricMotors", "ehaActuators", "emaActuators"}) {
        for (const auto& v : document.collection(QLatin1String(col))) {
            const auto o = v.toObject();
            if (!active(o)) continue;
            ++report.activeForces;
            for (const auto* k : {"bodyIId", "bodyJId", "bodyId", "baseBodyId", "actionBodyId", "targetBodyId"})
                if (o.contains(QLatin1String(k))) markBody(o.value(QLatin1String(k)).toInt(-1));
            if (o.contains("targetJointId")) {
                const auto jObj = document.entity("joints", o.value("targetJointId").toInt(-1));
                markBody(jObj.value("actionBodyId").toInt(-1));
                markBody(jObj.value("baseBodyId").toInt(-1));
            }
        }
    }

    report.flexBodies = document.collection("autoFlexBodies").size()
                      + document.collection("importedMnfBodies").size();

    for (const auto& v : document.collection("bodies")) {
        const auto b = v.toObject();
        const int id = b.value("id").toInt(-1);
        const QString name = b.value("name").toString();
        ++report.totalBodies;
        if (b.value("isFixed").toBool(false)) ++report.fixedBodies;
        else ++report.movingBodies;

        QString inertiaIssue;
        if (!checkBodyInertia(b, inertiaIssue)) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("质量惯量"), QStringLiteral("bodies"), id, name, inertiaIssue);
        }

        const auto parts = document.bodyPartBindings.value(id);
        if (parts.isEmpty() && !b.value("isCustomMassProps").toBool(false)) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("几何绑定"), QStringLiteral("bodies"), id, name,
                    QStringLiteral("未绑定任何 CAD 部件，且未开启自定义质量属性。"));
        }
        for (int partId : parts) {
            if (const auto* part = cad.part(partId); part && !part->metreDisplaySource.IsNull()) {
                addItem(report, MotionDiagnosticItem::Severity::Warning,
                        QStringLiteral("几何拓扑"), QStringLiteral("bodies"), id, name,
                        QStringLiteral("关联部件“%1”毫米拓扑未通过校验，当前使用米制几何缩放显示；刚体求解可用，划分有限元体网格前需修复拓扑。")
                            .arg(part->name));
            }
        }
        if (!b.value("isFixed").toBool(false) && !connectedBodies.contains(id)) {
            addItem(report, MotionDiagnosticItem::Severity::Warning,
                    QStringLiteral("约束与自由度"), QStringLiteral("bodies"), id, name,
                    QStringLiteral("活动运动体未连接任何运动副、驱动或力元，在重力场中将做自由落体运动。"));
        }
    }

    // 3. Hydraulic Circuit & Actuator Topology Checks
    checkHydraulicCircuit(document, report);

    // 4. Numerical Constraint Jacobian Rank, Redundant Constraints & Assembly Violation
    int chosenScenario = scenarioId;
    if (chosenScenario < 0 && !document.collection("scenarios").isEmpty()) {
        chosenScenario = document.collection("scenarios").first().toObject().value("id").toInt(-1);
    }
    if (chosenScenario >= 0 && report.errorCount == 0 && report.totalBodies > 0) {
        NativeMotionInput input;
        QString prepError;
        if (prepareNativeMotion(cad, document, chosenScenario, input, prepError)) {
            NativeConstraintSummary summary;
            QString evalError;
            if (evaluateNativeConstraintSystem(input, summary, evalError)) {
                report.hasJacobianAnalysis = true;
                report.velocityDofs = summary.velocityDofs;
                report.constraintEquations = summary.constraintEquations;
                report.jacobianRank = summary.jacobianRank;
                report.topologicalDofs = summary.velocityDofs - summary.constraintEquations;
                report.effectiveDofs = summary.velocityDofs - summary.jacobianRank;
                report.redundantEquations = summary.redundantEquations;
                report.maxInitialViolation = summary.maxPositionViolation;

                if (summary.redundantEquations > 0) {
                    addItem(report, MotionDiagnosticItem::Severity::Warning,
                            QStringLiteral("约束与自由度"), {}, -1, QStringLiteral("约束雅可比"),
                            QStringLiteral("检测到 %1 个冗余约束方程（总方程 %2，雅可比数值秩 %3，真实有效自由度 %4）。求解器可自动采用平衡缩放稀疏 QR 求解运动，但冗余方向的约束反力具有静不定性。")
                                .arg(summary.redundantEquations)
                                .arg(summary.constraintEquations)
                                .arg(summary.jacobianRank)
                                .arg(report.effectiveDofs));
                } else {
                    addItem(report, MotionDiagnosticItem::Severity::Pass,
                            QStringLiteral("约束与自由度"), {}, -1, QStringLiteral("约束雅可比"),
                            QStringLiteral("约束方程满秩无冗余（方程数 %1，秩 %2，有效自由度 %3）。")
                                .arg(summary.constraintEquations)
                                .arg(summary.jacobianRank)
                                .arg(report.effectiveDofs));
                }

                if (report.effectiveDofs == 0 && report.activeDrives == 0) {
                    addItem(report, MotionDiagnosticItem::Severity::Warning,
                            QStringLiteral("约束与自由度"), {}, -1, QStringLiteral("机构自由度"),
                            QStringLiteral("系统有效自由度为 0 且没有启用运动驱动，机构处于完全锁死状态。"));
                }

                for (const auto& jv : summary.jointViolations) {
                    if (jv.violation > 1e-2) {
                        const auto jObj = document.entity("joints", jv.jointId);
                        addItem(report,
                                jv.violation > 1.0 ? MotionDiagnosticItem::Severity::Error
                                                   : MotionDiagnosticItem::Severity::Warning,
                                QStringLiteral("初始装配"), QStringLiteral("joints"),
                                jv.jointId, jObj.value("name").toString(),
                                QStringLiteral("初始装配约束残差偏大（最大分量 %1 mm/rad），请核对作用点或轴线对齐。")
                                    .arg(jv.violation, 0, 'g', 4));
                    }
                }
            } else if (!evalError.isEmpty()) {
                addItem(report, MotionDiagnosticItem::Severity::Error,
                        QStringLiteral("约束与自由度"), {}, -1, QStringLiteral("求解器组装"), evalError);
            }
        } else if (!prepError.isEmpty()) {
            addItem(report, MotionDiagnosticItem::Severity::Error,
                    QStringLiteral("模型准备"), {}, -1, QStringLiteral("几何/拓扑预处理"), prepError);
        }
    }

    if (report.errorCount == 0 && report.warningCount == 0) {
        addItem(report, MotionDiagnosticItem::Severity::Pass,
                QStringLiteral("总结论"), {}, -1, QStringLiteral("全部检查项"),
                QStringLiteral("模型质量惯量、几何绑定、机电液回路与约束雅可比检查全部通过。"));
    }
    return report;
}

} // namespace MotionOcc
