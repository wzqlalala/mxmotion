#include "NativeMotionDiagnostics.h"
#include "CadDocument.h"

#include <Eigen/Eigenvalues>

#include <QJsonArray>
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
    const auto p = eig.eigenvalues(); // sorted ascending: p(0) <= p(1) <= p(2)
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
                            "threePointTorques", "contacts", "hydraulicCylinders"}) {
        for (const auto& v : document.collection(QLatin1String(col))) {
            const auto o = v.toObject();
            if (!active(o)) continue;
            ++report.activeForces;
            for (const auto* k : {"bodyIId", "bodyJId", "bodyId", "baseBodyId", "actionBodyId", "targetBodyId"})
                if (o.contains(QLatin1String(k))) markBody(o.value(QLatin1String(k)).toInt(-1));
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

    // 3. Numerical Constraint Jacobian Rank, Redundant Constraints & Assembly Violation
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
                QStringLiteral("模型质量惯量、几何绑定与约束雅可比检查全部通过。"));
    }
    return report;
}

} // namespace MotionOcc
