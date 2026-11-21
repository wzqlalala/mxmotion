#include "NativeMotionTypedPanels.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QTableWidget>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>

namespace MotionOcc {
namespace {

// QFormLayout can give a spanning group its unwrapped size hint even when its
// child labels and rows need more height at the dock's current width. Explicit
// minimum heights keep those children inside their groups; the scroll area then
// scrolls the full form. Recompute after text, visibility, font or width changes.
class NativeFormWidthFitter final : public QObject {
public:
    explicit NativeFormWidthFitter(QWidget* panel) : QObject(panel), m_panel(panel)
    {
        panel->layout()->setSizeConstraint(QLayout::SetNoConstraint);
        panel->installEventFilter(this);
        for (auto* group : panel->findChildren<QGroupBox*>()) {
            if (group->layout()) group->layout()->setSizeConstraint(QLayout::SetNoConstraint);
            group->installEventFilter(this);
        }
        schedule();
    }
protected:
    bool eventFilter(QObject*, QEvent* event) override
    {
        switch (event->type()) {
        case QEvent::Resize: case QEvent::LayoutRequest: case QEvent::Show:
        case QEvent::Hide: case QEvent::FontChange: case QEvent::StyleChange:
            schedule(); break;
        default: break;
        }
        return false;
    }
private:
    void schedule()
    {
        if (m_queued) return;
        m_queued = true;
        QTimer::singleShot(0, this, [this] {
            m_queued = false;
            const auto fit = [](QWidget* widget) {
                auto* layout = widget->layout();
                if (!layout) return;
                layout->activate();
                const int height = std::max(layout->totalMinimumSize().height(),
                    layout->totalHeightForWidth(widget->width()));
                if (widget->minimumHeight() != height) widget->setMinimumHeight(height);
            };
            const auto groups = m_panel->findChildren<QGroupBox*>();
            // Fit inner vector groups before the sections that contain them.
            for (auto it = groups.crbegin(); it != groups.crend(); ++it) fit(*it);
            fit(m_panel);
        });
    }
    QWidget* m_panel;
    bool m_queued = false;
};

QJsonObject vector(double x = 0, double y = 0, double z = 0)
{ return {{"x", x}, {"y", y}, {"z", z}}; }

int firstId(const NativeMotionDocument& document, const QString& collection)
{
    const auto rows = document.collection(collection);
    return rows.isEmpty() ? -1 : rows.first().toObject().value("id").toInt(-1);
}

bool effortDrive(const QJsonObject& drive)
{ const auto mode = drive.value("controlMode").toString(); return mode == "Force" || mode == "Torque"; }

QString variableLabel(const QString& key)
{
    static const QMap<QString, QString> labels{
        {"Position", "测量点位置"}, {"Rotation", "体姿态角"}, {"Velocity", "测量点速度"},
        {"AngularVelocity", "角速度"}, {"Acceleration", "测量点加速度"}, {"AngularAcceleration", "角加速度"},
        {"ReactionForce", "约束反力"}, {"ReactionTorque", "约束反力矩"},
        {"ActuationForce", "驱动力"}, {"ActuationTorque", "驱动力矩"},
        {"ImposedValue", "给定位移"}, {"ImposedSpeed", "给定速度"}, {"ImposedAcc", "给定加速度"},
        {"Coordinate", "实际位移 / 转角"}, {"Rate", "实际速度"}, {"CoordinateAcceleration", "实际加速度"},
        {"Distance", "两点距离"}, {"RelVelocity", "距离变化速度"}, {"RelAcceleration", "距离变化加速度"},
        {"Angle", "绕测量轴的相对转角"}};
    return labels.value(key, key);
}

struct FieldRow {
    QWidget* label = nullptr;
    QWidget* field = nullptr;
    void visible(bool value) const { if (label) label->setVisible(value); if (field) field->setVisible(value); }
    void enabled(bool value) const { if (label) label->setEnabled(value); if (field) field->setEnabled(value); }
    void title(const QString& text) const { if (auto* l = qobject_cast<QLabel*>(label)) l->setText(text); }
};

struct FormBuilder {
    QWidget* panel;
    QFormLayout* outer;
    const QJsonObject& row;
    const NativeFieldFactory& factory;
    QMap<QString, NativeFieldReader>& fields;
    QSet<QString> handled;
    QMap<QString, FieldRow> rows;

    QFormLayout* group(const QString& name, const QString& title)
    {
        auto* box = new QGroupBox(title, panel); box->setObjectName(name);
        auto* form = new QFormLayout(box); form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setSizeConstraint(QLayout::SetMinimumSize); outer->addRow(box); return form;
    }
    void add(QFormLayout* form, const QString& key)
    {
        handled.insert(key);
        const int r = form->rowCount(); fields.insert(key, factory(form, key, row.value(key)));
        auto* label = form->itemAt(r, QFormLayout::LabelRole);
        auto* field = form->itemAt(r, QFormLayout::FieldRole);
        if (!field) field = form->itemAt(r, QFormLayout::SpanningRole);
        rows.insert(key, {label ? label->widget() : nullptr, field ? field->widget() : nullptr});
    }
    void add(QFormLayout* form, const QStringList& keys) { for (const auto& key : keys) add(form, key); }
};

QLabel* message(QFormLayout* form, const QString& name)
{
    auto* text = new QLabel; text->setObjectName(name); text->setWordWrap(true);
    text->setTextInteractionFlags(Qt::TextSelectableByMouse); form->addRow(text); return text;
}

void watch(QWidget* panel, const std::function<void()>& changed)
{
    for (auto* combo : panel->findChildren<QComboBox*>())
        QObject::connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), panel, [changed] { changed(); });
    for (auto* edit : panel->findChildren<QLineEdit*>())
        QObject::connect(edit, &QLineEdit::textChanged, panel, [changed] { changed(); });
    for (auto* check : panel->findChildren<QCheckBox*>())
        QObject::connect(check, &QCheckBox::toggled, panel, [changed] { changed(); });
}

QJsonObject readRecord(const QJsonObject& original, const QMap<QString, NativeFieldReader>& fields)
{
    auto next = original;
    for (auto it = fields.cbegin(); it != fields.cend(); ++it) next.insert(it.key(), it.value()());
    return next;
}

QStringList sensorVariablesFor(const QString& type, const QJsonObject& row, const NativeMotionDocument& document)
{
    auto options = nativeSensorVariables(type);
    if (type == "DriveActuation" && effortDrive(document.entity("drives", row.value("targetDriveId").toInt(-1)))) {
        options.removeAll("ImposedValue"); options.removeAll("ImposedSpeed"); options.removeAll("ImposedAcc");
        options.append({"Coordinate", "Rate", "CoordinateAcceleration"});
    }
    return options;
}

QString controllerFeedbackUnit(const QJsonObject& row, const NativeMotionDocument& document)
{
    if (row.value("feedbackSource").toString() == "Sensor")
        return nativeSensorQuantityUnit(document.entity("sensors", row.value("feedbackSensorId").toInt(-1)), document);
    const auto drive = document.entity("drives", row.value("targetDriveId").toInt(-1));
    const bool angular = drive.value("motionType").toString() == "Rotation";
    const QString position = angular ? (effortDrive(drive) ? QStringLiteral("°") : QStringLiteral("rad")) : QStringLiteral("mm");
    return position + (row.value("feedbackType").toString() == "Velocity" ? QStringLiteral("/s") : QString{});
}

QString controllerOutputUnit(const QJsonObject& row, const NativeMotionDocument& document)
{
    const auto drive = document.entity("drives", row.value("targetDriveId").toInt(-1));
    if (effortDrive(drive)) return nativeDriveCommandUnit(drive);
    return drive.value("motionType").toString() == "Rotation" ? QStringLiteral("rad/s") : QStringLiteral("mm/s");
}

void setProblem(QLabel* text, const QString& issue)
{
    text->setText(issue); text->setVisible(!issue.isEmpty());
    text->setToolTip(issue);
    text->setStyleSheet(issue.isEmpty() ? QString{} : QStringLiteral("color:#b44632"));
}

} // namespace

void fitNativeFormToWidth(QWidget* panel)
{
    if (panel && panel->layout()) new NativeFormWidthFitter(panel);
}

QStringList nativeSensorVariables(const QString& type)
{
    if (type == "BodyDisplacement") return {"Position", "Rotation"};
    if (type == "BodyVelocity") return {"Velocity", "AngularVelocity"};
    if (type == "BodyAcceleration") return {"Acceleration", "AngularAcceleration"};
    if (type == "JointReaction") return {"ReactionForce", "ReactionTorque"};
    if (type == "DriveActuation") return {"ActuationForce", "ActuationTorque", "ImposedValue", "ImposedSpeed", "ImposedAcc", "ReactionForce", "ReactionTorque"};
    if (type == "Distance") return {"Distance", "RelVelocity", "RelAcceleration"};
    if (type == "Angle") return {"Angle"};
    return {};
}

QString nativeSensorQuantityUnit(const QJsonObject& sensor, const NativeMotionDocument& document)
{
    const auto type = sensor.value("sensorType").toString(), variable = sensor.value("variable").toString();
    if (type == "Angle" || variable == "Rotation") return QStringLiteral("°");
    if (variable.endsWith("Torque")) return QStringLiteral("N·mm");
    if (variable.endsWith("Force")) return QStringLiteral("N");
    if (variable == "AngularVelocity") return QStringLiteral("°/s");
    if (variable == "AngularAcceleration") return QStringLiteral("°/s²");
    if (variable == "Velocity" || variable == "RelVelocity") return QStringLiteral("mm/s");
    if (variable == "Acceleration" || variable == "RelAcceleration") return QStringLiteral("mm/s²");
    if (variable.startsWith("Imposed") || variable == "Coordinate" || variable == "Rate" || variable == "CoordinateAcceleration") {
        const auto drive = document.entity("drives", sensor.value("targetDriveId").toInt(-1));
        const auto unit = drive.value("motionType").toString() == "Rotation" ? QStringLiteral("°") : QStringLiteral("mm");
        return unit + (variable == "ImposedSpeed" || variable == "Rate" ? QStringLiteral("/s")
            : variable == "ImposedAcc" || variable == "CoordinateAcceleration" ? QStringLiteral("/s²") : QString{});
    }
    if (variable == "Position" || variable == "Distance" || type == "Distance") return QStringLiteral("mm");
    return QStringLiteral("未确定");
}

QString nativeDriveCommandUnit(const QJsonObject& drive)
{
    const auto mode = drive.value("controlMode").toString();
    if (mode == "Force") return QStringLiteral("N");
    if (mode == "Torque") return QStringLiteral("N·mm");
    const auto unit = drive.value("motionType").toString() == "Rotation" ? QStringLiteral("°") : QStringLiteral("mm");
    return unit + (mode == "Velocity" ? QStringLiteral("/s") : mode == "Acceleration" ? QStringLiteral("/s²") : QString{});
}

void completeNativeTypedRecord(QJsonObject& row, const QString& collection, const NativeMotionDocument& document)
{
    QJsonObject defaults;
    if (collection == "sensors") {
        defaults = {{"sensorType", "BodyDisplacement"}, {"enabled", true}, {"coordSystem", "GlobalFixed"},
            {"referenceBodyId", -1}, {"componentMode", "Z"}, {"targetBodyId", firstId(document, "bodies")},
            {"targetJointId", firstId(document, "joints")}, {"targetDriveId", firstId(document, "drives")},
            {"referencePoint", vector()}, {"pointA", QJsonObject{{"bodyId", firstId(document, "bodies")}, {"localPos", vector()}}},
            {"pointB", QJsonObject{{"bodyId", -1}, {"localPos", vector()}}}, {"axisBodyId", -1},
            {"bodyAId", firstId(document, "bodies")}, {"bodyBId", -1}, {"axisDirection", vector(0,0,1)}};
        const auto variables = nativeSensorVariables(row.value("sensorType").toString("BodyDisplacement"));
        defaults.insert("variable", variables.isEmpty() ? QString{} : variables.first());
    } else if (collection == "scenarios") {
        defaults = {{"endTime", 1.0}, {"stepSize", .001}, {"gravity", vector(0,0,-9806.65)},
            {"analysisType", "Dynamic"}, {"staticMaxIterations", 100}, {"staticTolerance", 1e-8},
            {"staticConstraintTolerance", 1e-7}, {"staticStepDamping", 1.0},
            {"usePreset", true}, {"presetMode", "FAST"}, {"solverType", "SPARSE_QR"},
            {"solverTolerance", 1e-8}, {"solverMaxIters", 100}, {"integratorType", "EULER_IMPLICIT_LINEARIZED"},
            {"integratorTol", 1e-8}, {"integratorMaxIters", 50}, {"integratorAlpha", -.2}, {"integratorBeta", .25},
            {"trackedMarkerIds", QJsonArray{}}};
    } else if (collection == "controllers") {
        defaults = {{"controllerType", "PID"}, {"targetDriveId", firstId(document, "drives")}, {"targetInputId", -1},
            {"feedbackSensorId", -1}, {"feedbackType", "Position"}, {"feedbackSource", "DriveDof"},
            {"targetValue", 0.0}, {"feedbackSign", 1.0}, {"kp", 1.0}, {"ki", 0.0}, {"kd", 0.0},
            {"outputLimit", 0.0}, {"enabled", true}};
    } else if (collection == "drives") {
        defaults = {{"effortLimit", 0.0}, {"powerLimit", 0.0}, {"noLoadSpeed", 0.0}};
    }
    for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        if (!row.contains(it.key())) row.insert(it.key(), it.value());
    // Angle always measures the relative angle about its axis. Old exports can
    // retain a variable from a previous sensor type, which the solver ignores.
    if (collection == "sensors" && row.value("sensorType").toString() == "Angle")
        row.insert("variable", "Angle");
}

QString nativeTypedRecordIssue(const QJsonObject& row, const QString& collection,
                               const NativeMotionDocument& document)
{
    if (collection == "sensors") {
        const auto type = row.value("sensorType").toString();
        const auto variable = row.value("variable").toString(type == "Angle" || type == "Distance" ? type : QString{});
        const bool legacyBodyVariable = type.startsWith("Body") && QStringList{"Position", "Velocity", "Acceleration", "Rotation", "AngularVelocity", "AngularAcceleration"}.contains(variable);
        if (type != "Angle" && !legacyBodyVariable && !sensorVariablesFor(type, row, document).contains(variable))
            return QStringLiteral("当前传感器类型不支持所选变量。请选择列表中的测量变量；力 / 力矩执行器没有给定位移、速度或加速度。旧字段会保留，不能作为有效测量量保存。");
    } else if (collection == "controllers" && row.value("enabled").toBool(true)) {
        const auto drive = document.entity("drives", row.value("targetDriveId").toInt(-1));
        const auto mode = drive.value("controlMode").toString();
        if (drive.isEmpty() || (mode != "Velocity" && mode != "Force" && mode != "Torque"))
            return QStringLiteral("PID 目标必须是速度驱动，或力 / 力矩执行器。请先修改目标驱动的控制模式。");
        for (const auto& value : document.collection("controllers")) {
            const auto other = value.toObject();
            if (other.value("id") != row.value("id") && other.value("enabled").toBool(true)
                && other.value("targetDriveId") == row.value("targetDriveId"))
                return QStringLiteral("目标驱动已被控制器 %1 接管；同一驱动不能同时启用两个 PID。").arg(other.value("id").toInt());
        }
        if (row.value("feedbackSource").toString() == "Sensor") {
            const auto sensor = document.entity("sensors", row.value("feedbackSensorId").toInt(-1));
            if (sensor.isEmpty() || !sensor.value("enabled").toBool(true))
                return QStringLiteral("请选择一个已启用的反馈传感器。");
            const auto type = sensor.value("sensorType").toString();
            if (type == "JointReaction" || type == "DriveActuation")
                return QStringLiteral("关节反力 / 驱动传感器暂未接入 PID 的同步采样，可用于求解后的曲线。控制反馈请选择体、距离或角度传感器。");
            if (sensor.value("componentMode").toString() == "All" && type != "Distance" && type != "Angle")
                return QStringLiteral("PID 需要一个标量反馈。请把该传感器的输出分量设为 X、Y、Z 或幅值，不能选“所有分量”。");
            const auto issue = nativeTypedRecordIssue(sensor, "sensors", document);
            if (!issue.isEmpty()) return issue;
        }
        if (row.value("targetInputId").toInt(-1) >= 0 && document.entity("controlInputs", row.value("targetInputId").toInt()).isEmpty())
            return QStringLiteral("所选目标控制输入不存在，请重新选择。");
        if (row.value("outputLimit").toDouble() < 0) return QStringLiteral("输出限幅必须大于或等于 0；0 表示不设置控制器限幅。");
    } else if (collection == "drives") {
        const auto mode = row.value("controlMode").toString();
        const bool angular = row.value("motionType").toString() == "Rotation";
        const auto targetType = document.entity("joints",row.value("targetJointId").toInt(-1)).value("jointType").toString();
        if ((targetType == "Revolute" && !angular) || (targetType == "Translational" && angular))
            return QStringLiteral("目标运动副的自由度与驱动类型不一致：旋转副需转动驱动，移动副需平动驱动。");
        if ((mode == "Force" && angular) || (mode == "Torque" && !angular))
            return QStringLiteral("平动执行器请选择“力”，转动执行器请选择“力矩”。");
        if (mode == "Acceleration") return QStringLiteral("当前原生驱动未接通加速度给定，请选择位移、速度、力或力矩模式。");
        if (effortDrive(row)) {
            if (row.value("effortLimit").toDouble() < 0 || row.value("powerLimit").toDouble() < 0 || row.value("noLoadSpeed").toDouble() < 0)
                return QStringLiteral("执行器的额定力 / 力矩、功率与空载速度不得为负；0 表示不启用该限制。");
            if (row.value("noLoadSpeed").toDouble() > 0 && row.value("effortLimit").toDouble() <= 0)
                return QStringLiteral("使用力—速度 / 力矩—转速包络时，额定力 / 力矩必须为正。");
        }
    } else if (collection == "scenarios") {
        if (row.value("analysisType").toString() == "Static")
            for (const auto& value : document.collection("controllers"))
                if (value.toObject().value("enabled").toBool(true))
                    return QStringLiteral("静力平衡不能运行离散 PID。请停用控制器，或选择“静力预载后动力学”以在平衡后启动控制器。");
        if (row.value("usePreset").toBool(true) && row.value("presetMode").toString("FAST") == "CONTACT") {
            const auto flex = document.collection("autoFlexBodies");
            const bool hasFlex = std::any_of(flex.begin(),flex.end(),[](const QJsonValue& v){return v.toObject().value("enabled").toBool(true);});
            if (!hasFlex) for (const auto& value : document.collection("joints"))
                if (value.toObject().value("jointType").toString() == "CurveOnCurve")
                    return QStringLiteral("CONTACT 预设不支持曲线在曲线上约束，请选 ROBUST 或自定义直接求解器。");
        }
        if (row.value("analysisType").toString("Dynamic") != "Dynamic") {
            const double iterations=row.value("staticMaxIterations").toDouble(100), damping=row.value("staticStepDamping").toDouble(1);
            if (iterations<1 || iterations>1000 || std::trunc(iterations)!=iterations)
                return QStringLiteral("静力最大迭代数须为 1…1000 的整数。");
            if (!(damping>0 && damping<=1)) return QStringLiteral("Newton 步长系数须满足 0 < 值 ≤ 1。");
            if (!(row.value("staticTolerance").toDouble(1e-8)>0) || !(row.value("staticConstraintTolerance").toDouble(1e-7)>0))
                return QStringLiteral("静力平衡残差和约束残差的容差必须为正。");
        }
    }
    return {};
}

QSet<QString> addNativeTypedFields(QWidget* panel, QFormLayout* outer,
                                  const QJsonObject& original, const QString& collection,
                                  const NativeMotionDocument& document,
                                  const NativeFieldFactory& createField,
                                  QMap<QString, NativeFieldReader>& fields)
{
    FormBuilder b{panel, outer, original, createField, fields, {}, {}};
    if (collection == "scenarios") {
        auto* analysis = b.group("nativeScenarioAnalysis", QStringLiteral("分析方式"));
        b.add(analysis, "analysisType");
        auto* analysisInfo = message(analysis, "nativeScenarioAnalysisHelp");
        auto* time = b.group("nativeScenarioTime", QStringLiteral("时间控制"));
        b.add(time, QStringList{"endTime", "stepSize"});
        auto* frames = message(time, "nativeScenarioFrameCount");
        auto* environment = b.group("nativeScenarioEnvironment", QStringLiteral("物理环境"));
        b.add(environment, "gravity");
        auto* equilibrium = b.group("nativeScenarioStatic", QStringLiteral("静力平衡与预载"));
        b.add(equilibrium, QStringList{"staticMaxIterations", "staticTolerance", "staticConstraintTolerance", "staticStepDamping"});
        auto* preset = b.group("nativeScenarioPreset", QStringLiteral("求解预设"));
        b.add(preset, QStringList{"usePreset", "presetMode"});
        auto* effective = message(preset, "nativeScenarioEffectiveSettings");
        auto* advanced = b.group("nativeScenarioAdvanced", QStringLiteral("高级参数"));
        b.add(advanced, QStringList{"solverType", "solverTolerance", "solverMaxIters", "integratorType", "integratorTol", "integratorMaxIters", "integratorAlpha", "integratorBeta"});
        auto* output = b.group("nativeScenarioOutput", QStringLiteral("输出与跟踪"));
        b.add(output, "trackedMarkerIds");
        auto* issue = message(outer,"nativeTypedIssue");
        const auto rows = b.rows;
        const auto readers = fields;
        auto* mode = panel->findChild<QComboBox*>("native_analysisType");
        auto* use = panel->findChild<QCheckBox*>("native_usePreset");
        auto* presetChoice = panel->findChild<QComboBox*>("native_presetMode");
        auto* integration = panel->findChild<QComboBox*>("native_integratorType");
        auto* solver = panel->findChild<QComboBox*>("native_solverType");
        auto* duration = panel->findChild<QLineEdit*>("native_endTime");
        auto* step = panel->findChild<QLineEdit*>("native_stepSize");
        const auto changed = [=] {
            const auto analysisType = mode->currentData().toString();
            const bool dynamic = analysisType != "Static", preloaded = analysisType == "PreloadedDynamics";
            time->parentWidget()->setEnabled(dynamic); equilibrium->parentWidget()->setVisible(analysisType != "Dynamic");
            preset->parentWidget()->setEnabled(dynamic); advanced->parentWidget()->setEnabled(dynamic);
            analysisInfo->setText(preloaded ? QStringLiteral("先在 t=0 平衡重力和外载，再从含预应变的平衡状态开始动力学；静力阶段使用零速度，PID 在平衡后初始化。")
                : dynamic ? QStringLiteral("从装配后的初始状态开始时间积分。步长同时决定当前结果输出间隔。")
                : QStringLiteral("载荷固定取 t=0 的值，求得静力平衡并输出一个平衡帧；采用直接矩阵求解。运动驱动在 t=0 的给定速度必须为零；静力方案不能启用 PID。"));
            bool endOk = false, stepOk = false; const double end = duration->text().toDouble(&endOk), dt = step->text().toDouble(&stepOk);
            const double steps = endOk && stepOk && dt > 0 ? end / dt : std::numeric_limits<double>::infinity();
            if (!dynamic) frames->setText(QStringLiteral("预计输出帧数：1 帧（平衡状态）"));
            else if (endOk && stepOk && std::isfinite(steps) && end > 0 && dt > 0 && steps < 1e15) {
                const double count = std::ceil(steps - 32 * std::numeric_limits<double>::epsilon() * std::max(1.0, steps)) + 1;
                frames->setText(QStringLiteral("预计输出帧数：%1 帧（含初始帧与终止帧）%2").arg(count,0,'f',0)
                    .arg(preloaded ? QStringLiteral("；初始帧为预载平衡状态") : QString{}));
            } else frames->setText(QStringLiteral("请输入正的终止时间和步长以估计输出帧数。"));
            const bool automatic = use->isChecked(); const auto p = presetChoice->currentData().toString();
            const auto flexDefinitions = document.collection("autoFlexBodies");
            const bool flex = std::any_of(flexDefinitions.begin(), flexDefinitions.end(), [](const QJsonValue& v) { return v.toObject().value("enabled").toBool(true); });
            const bool independent = std::any_of(flexDefinitions.begin(),flexDefinitions.end(),[](const QJsonValue& v) {
                const auto body=v.toObject();return body.value("enabled").toBool(true)&&body.value("interfaceMode").toString()=="Independent";
            });
            const auto anyActive = [&](const QString& collection) {
                const auto values=document.collection(collection);return std::any_of(values.begin(),values.end(),[](const QJsonValue& v){return v.toObject().value("enabled").toBool(true);});
            };
            const bool stiff=anyActive("contacts")||anyActive("softStops")||anyActive("tensionCables");
            const auto jointDefinitions=document.collection("joints");
            const bool curve=std::any_of(jointDefinitions.begin(),jointDefinitions.end(),[](const QJsonValue& v){return v.toObject().value("jointType").toString()=="CurveOnCurve";});
            QString actualSolver=solver->currentData().toString(),actualIntegrator=integration->currentData().toString();
            if(automatic){
                if(p=="FAST"){actualSolver=stiff||curve?"SPARSE_QR":"PSOR";actualIntegrator="EULER_IMPLICIT";}
                else if(p=="CONTACT"){actualSolver=stiff?"SPARSE_QR":"APGD";actualIntegrator=stiff?"EULER_IMPLICIT":"EULER_IMPLICIT_PROJECTED";}
                else if(p=="ROBUST"){actualSolver="SPARSE_QR";actualIntegrator="HHT";}
                else{actualSolver="SPARSE_LU";actualIntegrator="NEWMARK";}
            }
            if(curve&&actualSolver!="SPARSE_LU"&&actualSolver!="SPARSE_QR")actualSolver="SPARSE_QR";
            if(flex){actualSolver=independent?"SPARSE_QR":"MINRES";actualIntegrator="EULER_IMPLICIT_LINEARIZED";}
            const bool iterativeSolver=actualSolver!="SPARSE_QR"&&actualSolver!="SPARSE_LU";
            const bool iterativeIntegration=QStringList{"EULER_IMPLICIT","TRAPEZOIDAL","TRAPEZOIDAL_LINEARIZED","HHT","NEWMARK"}.contains(actualIntegrator);
            // Avoid claiming the stored advanced values are active when the
            // solver's model-specific preset replaces those values.
            rows.value("presetMode").enabled(automatic);
            rows.value("solverType").enabled(!automatic && !flex);
            rows.value("integratorType").enabled(!automatic && !flex);
            rows.value("solverTolerance").enabled(iterativeSolver && (flex || !automatic || p != "FAST"));
            rows.value("solverMaxIters").enabled(iterativeSolver && (flex || !automatic || p != "FAST"));
            rows.value("integratorTol").enabled(iterativeIntegration && !flex && (!automatic || (p != "FAST" && p != "ROBUST")));
            rows.value("integratorMaxIters").enabled(iterativeIntegration && !flex && (!automatic || (p != "FAST" && p != "ROBUST" && !(p=="CONTACT"&&stiff))));
            rows.value("integratorAlpha").enabled(!automatic && !flex && integration->currentData().toString() == "HHT");
            rows.value("integratorBeta").enabled(false);
            rows.value("integratorBeta").title(QStringLiteral("β（仅保留原数据）"));
            QString text;
            if (flex) text=independent
                ? QStringLiteral("独立接口全有限元：SPARSE_QR + 线性化隐式 Euler，保留全部节点自由度并逐步检查约束。直接求解器不使用线性迭代次数 / 容差；该积分器不使用非线性迭代参数。")
                : QStringLiteral("共用参考体模态缩减：MINRES + 线性化隐式 Euler，线性迭代上限至少 500，线性容差生效并逐步检查约束。非线性积分迭代参数不适用。");
            else if (!automatic) text=QStringLiteral("实际采用 %1 + %2。%3%4%5").arg(actualSolver,actualIntegrator,
                iterativeSolver?QString{}:QStringLiteral("直接求解器不使用线性迭代次数 / 容差。"),
                iterativeIntegration?QString{}:QStringLiteral("当前积分器不使用非线性迭代参数。"),
                actualIntegrator=="NEWMARK"?QStringLiteral("Newmark 使用 γ=0.6、β=0.3；原 β 输入仅保留。"):
                    QStringLiteral("α 仅用于 HHT；原 β 输入仅保留。"));
            else if(p=="FAST")text=QStringLiteral("FAST 实际采用 %1 + 隐式 Euler，非线性 %2 次 / 相对容差 1e-4、绝对容差 1e-8。%3灰色参数被预设覆盖或不适用。")
                .arg(actualSolver).arg(curve?30:stiff?20:4).arg(iterativeSolver?QStringLiteral("PSOR 固定 50 次，不设置收敛阈值。"):
                    QStringLiteral("当前接触 / 单边力元 / 曲线约束使用直接矩阵求解。"));
            else if(p=="ROBUST")text=QStringLiteral("ROBUST：SPARSE_QR + HHT，α=-0.2，非线性 50 次 / 相对容差 1e-4、绝对容差 1e-8。直接求解器不使用线性迭代次数 / 容差。");
            else if(p=="CONTACT")text=stiff?QStringLiteral("CONTACT：当前接触 / 单边力元采用 SPARSE_QR + 隐式 Euler，非线性 20 次，积分容差有效。直接求解器不使用线性迭代次数 / 容差。")
                :QStringLiteral("CONTACT：APGD + 投影隐式 Euler；线性容差和迭代次数有效，非线性积分迭代参数不适用。曲线在曲线上约束不支持此预设。");
            else text=QStringLiteral("HIGH_PRECISION：SPARSE_LU + Newmark；非线性容差与迭代设置有效。直接求解器不使用线性迭代参数。Newmark 使用 γ=0.6、β=0.3；原 β 输入仅保留。");
            if(!dynamic)text=QStringLiteral("静力阶段使用实际力与约束残差检查、Newton 迭代和直接矩阵求解；只使用上方静力参数。动力学预设和积分参数保留，但本次静力不使用。");
            effective->setText(text);
            try { auto problem=nativeTypedRecordIssue(readRecord(original,readers),"scenarios",document);
                if(problem.isEmpty()&&dynamic&&!flex&&automatic&&p=="CONTACT"&&curve)
                    problem=QStringLiteral("CONTACT 预设不支持曲线在曲线上约束，请选 ROBUST 或自定义直接求解器。");
                setProblem(issue,problem);
            } catch(const std::exception& error){setProblem(issue,QString::fromUtf8(error.what()));}
        };
        watch(panel, changed); changed();
    } else if (collection == "sensors") {
        auto* kind = b.group("nativeSensorDefinition", QStringLiteral("测量类型"));
        b.add(kind, QStringList{"enabled", "sensorType"});
        auto* type = panel->findChild<QComboBox*>("native_sensorType"); type->setEnabled(true);
        auto* variable = new QComboBox; variable->setObjectName("native_variable");
        kind->addRow(QStringLiteral("测量变量"), variable); b.handled.insert("variable");
        fields.insert("variable", [variable] { return variable->currentData().toString(); });
        auto* quantity = message(kind, "nativeSensorQuantity");
        auto* body = b.group("nativeSensorBody", QStringLiteral("目标体与测量点"));
        b.add(body, QStringList{"targetBodyId", "referencePoint"});
        auto* joint = b.group("nativeSensorJoint", QStringLiteral("目标运动副")); b.add(joint, "targetJointId");
        auto* drive = b.group("nativeSensorDrive", QStringLiteral("目标驱动")); b.add(drive, "targetDriveId");
        auto* distance = b.group("nativeSensorDistance", QStringLiteral("两点距离")); b.add(distance, QStringList{"pointA", "pointB"});
        auto* angle = b.group("nativeSensorAngle", QStringLiteral("相对角度与参考轴")); b.add(angle, QStringList{"bodyAId", "bodyBId", "axisBodyId", "axisDirection"});
        auto* coordinates = b.group("nativeSensorCoordinates", QStringLiteral("分量与坐标系"));
        b.add(coordinates, QStringList{"componentMode", "coordSystem", "referenceBodyId"});
        auto* info = message(coordinates, "nativeSensorCoordinateHelp");
        auto* issue = message(outer, "nativeTypedIssue");
        // These old display vectors are not inputs to the original angle
        // evaluator. Preserve them without exposing inert editable controls.
        b.handled.insert("directionA"); b.handled.insert("directionB");
        const auto rows = b.rows; const auto readers = fields;
        auto lastType = std::make_shared<QString>(type->currentData().toString());
        auto cachedVariable = std::make_shared<QMap<QString,QString>>();
        cachedVariable->insert(*lastType, original.value("variable").toString());
        const auto changed = [=] {
            const QString currentType = type->currentData().toString();
            const bool typeChanged = currentType != *lastType;
            if (!variable->currentData().toString().isEmpty()) cachedVariable->insert(*lastType, variable->currentData().toString());
            QString selected = typeChanged ? cachedVariable->value(currentType) : cachedVariable->value(*lastType);
            if (currentType == "Angle") selected = QStringLiteral("Angle");
            QJsonObject context = original; context.insert("targetDriveId", panel->findChild<QComboBox*>("native_targetDriveId")->currentData().toInt());
            const auto options = sensorVariablesFor(currentType, context, document);
            if (selected.isEmpty() && !options.isEmpty()) selected = options.first();
            { const QSignalBlocker blocker(variable); variable->clear();
                for (const auto& option : options) variable->addItem(variableLabel(option), option);
                if (!selected.isEmpty() && variable->findData(selected) < 0) variable->addItem(QStringLiteral("旧值（当前类型不支持）：%1").arg(selected), selected);
                variable->setCurrentIndex(variable->findData(selected)); }
            *lastType = currentType;
            cachedVariable->insert(currentType, selected);
            const bool bodyType = currentType.startsWith("Body");
            const bool scalar = currentType == "Distance" || currentType == "Angle" || selected.startsWith("Imposed")
                || selected == "Coordinate" || selected == "Rate" || selected == "CoordinateAcceleration";
            body->parentWidget()->setVisible(bodyType); joint->parentWidget()->setVisible(currentType == "JointReaction");
            drive->parentWidget()->setVisible(currentType == "DriveActuation"); distance->parentWidget()->setVisible(currentType == "Distance");
            angle->parentWidget()->setVisible(currentType == "Angle"); coordinates->parentWidget()->setVisible(!scalar);
            const bool rotational = selected == "Rotation" || selected.startsWith("Angular");
            rows.value("referencePoint").visible(bodyType && !rotational);
            rows.value("coordSystem").visible(selected != "Rotation");
            const bool movingFrame = panel->findChild<QComboBox*>("native_coordSystem")->currentData().toString() == "BodyFixed";
            rows.value("referenceBodyId").visible(movingFrame && selected != "Rotation");
            info->setText(selected == "Rotation" ? QStringLiteral("姿态角按全局体姿态输出，单位 °；不使用测量点或参考体坐标变换。")
                : QStringLiteral("测量点拾取 / 输入为初始全局 mm。位置是绝对位置；随体坐标仅旋转分量，不减去参考体的位置或速度。"));
            try { const auto record = readRecord(original, readers);
                quantity->setText(QStringLiteral("输出：%1 · 单位 %2%3").arg(variableLabel(selected), nativeSensorQuantityUnit(record,document), scalar ? QStringLiteral(" · 标量") : QString{}));
                setProblem(issue, nativeTypedRecordIssue(record, "sensors", document));
            } catch (const std::exception& error) { setProblem(issue, QString::fromUtf8(error.what())); }
        };
        watch(panel, changed); changed();
    } else if (collection == "controllers") {
        auto* target = b.group("nativePidActuator", QStringLiteral("执行器"));
        b.add(target, QStringList{"enabled", "targetDriveId"});
        b.handled.insert("controllerType");
        auto* feedback = b.group("nativePidFeedback", QStringLiteral("反馈"));
        b.add(feedback, QStringList{"feedbackSource", "feedbackType", "feedbackSensorId", "feedbackSign"});
        auto* desired = b.group("nativePidTarget", QStringLiteral("目标"));
        auto* source = new QComboBox; source->setObjectName("native_targetSource");
        source->addItem(QStringLiteral("固定目标值"), "Constant"); source->addItem(QStringLiteral("控制输入轨迹"), "Input");
        source->setCurrentIndex(original.value("targetInputId").toInt(-1) >= 0 ? 1 : 0); desired->addRow(QStringLiteral("目标来源"),source);
        b.add(desired, QStringList{"targetValue", "targetInputId"});
        const auto inputReader = fields.value("targetInputId"), valueReader = fields.value("targetValue");
        fields.insert("targetInputId", [source,inputReader] { return source->currentData().toString() == "Input" ? inputReader() : QJsonValue(-1); });
        fields.insert("targetValue", [source,valueReader,original] { return source->currentData().toString() == "Input" ? original.value("targetValue") : valueReader(); });
        auto* gains = b.group("nativePidGains", QStringLiteral("增益与输出限幅"));
        b.add(gains, QStringList{"kp", "ki", "kd", "outputLimit"});
        auto* units = message(gains, "nativePidUnits"); auto* semantics = message(gains, "nativePidSemantics");
        auto* issue = message(outer, "nativeTypedIssue");
        const auto rows = b.rows;
        const auto readers = fields;
        const auto changed = [=] {
            const bool sensor = panel->findChild<QComboBox*>("native_feedbackSource")->currentData().toString() == "Sensor";
            rows.value("feedbackType").visible(!sensor); rows.value("feedbackSensorId").visible(sensor);
            const bool input = source->currentData().toString() == "Input";
            rows.value("targetValue").enabled(!input); rows.value("targetInputId").enabled(input);
            try {
                const auto record = readRecord(original, readers);
                const auto feedbackUnit = controllerFeedbackUnit(record, document), outputUnit = controllerOutputUnit(record, document);
                const auto drive = document.entity("drives", record.value("targetDriveId").toInt(-1));
                QString gainUnit = feedbackUnit;
                if (!effortDrive(drive) && sensor && gainUnit.startsWith(QStringLiteral("°"))) gainUnit.replace(0, 1, QStringLiteral("rad"));
                rows.value("targetValue").title(QStringLiteral("固定目标值 (%1)").arg(feedbackUnit));
                rows.value("outputLimit").title(QStringLiteral("输出限幅 (%1，0 为不限)").arg(outputUnit));
                rows.value("kp").title(QStringLiteral("Kp (%1 / %2)").arg(outputUnit,gainUnit));
                rows.value("ki").title(QStringLiteral("Ki (%1 / (%2·s))").arg(outputUnit,gainUnit));
                rows.value("kd").title(QStringLiteral("Kd (%1·s / %2)").arg(outputUnit,gainUnit));
                units->setText(QStringLiteral("目标与反馈：%1；执行器输出：%2。控制输入轨迹的数值使用同一目标单位。").arg(feedbackUnit,outputUnit));
                semantics->setText(effortDrive(drive)
                    ? QStringLiteral("力 / 力矩 PID：P + I − Kd × 反馈变化率；积分带抗饱和。控制器输出限幅与执行器力 / 力矩、功率、空载速度限制共同生效。")
                    : QStringLiteral("原速度 PID 保持原有合同：输出是速度给定，不是电机的力 / 力矩上限。旋转 DriveDof 目标为 rad；角度传感器目标为 °，内部换为 rad，输出为 rad/s。"));
                auto error = nativeTypedRecordIssue(record, "controllers", document);
                if (input && record.value("targetInputId").toInt(-1) < 0) error = QStringLiteral("目标来源为控制输入时，必须选择一条有效轨迹。");
                setProblem(issue,error);
            } catch (const std::exception& error) { setProblem(issue,QString::fromUtf8(error.what())); }
        };
        watch(panel,changed); changed();
    } else if (collection == "drives") {
        auto* target = b.group("nativeDriveTarget", QStringLiteral("目标与控制模式"));
        b.add(target,QStringList{"targetJointId", "motionType", "controlMode"});
        auto* law = b.group("nativeDriveLaw", QStringLiteral("给定函数"));
        b.add(law,QStringList{"expression", "driveTableX", "driveTableY"});
        auto* unit = message(law,"nativeDriveCommandUnits");
        auto* limits = b.group("nativeDriveLimits",QStringLiteral("执行器限制"));
        b.add(limits,QStringList{"effortLimit", "powerLimit", "noLoadSpeed"});
        auto* limitHelp = message(limits,"nativeDriveLimitHelp");
        auto* issue = message(outer,"nativeTypedIssue");
        const auto rows = b.rows;
        const auto readers = fields;
        const auto changed = [=] {
            try {
                const auto record = readRecord(original,readers); const bool effort = effortDrive(record);
                const auto commandUnit = nativeDriveCommandUnit(record);
                limits->parentWidget()->setVisible(effort);
                rows.value("effortLimit").title(QStringLiteral("额定力 / 力矩 (%1，0 为不限)").arg(commandUnit));
                rows.value("powerLimit").title(QStringLiteral("功率限制 (W，0 为不限)"));
                rows.value("noLoadSpeed").title(record.value("motionType").toString() == "Rotation"
                    ? QStringLiteral("空载转速 (°/s，0 为不启用)") : QStringLiteral("空载速度 (mm/s，0 为不启用)"));
                unit->setText(QStringLiteral("函数与表格纵轴单位：%1；横轴为时间 s。%2").arg(commandUnit,
                    effort ? QStringLiteral("施加真实力 / 力矩，运动由动力学求解；不会约束为给定位移。") : QStringLiteral("给定运动由约束驱动实施。")));
                if (auto* table = panel->findChild<QTableWidget*>("native_driveTableX")) table->setHorizontalHeaderLabels({QStringLiteral("时间 (s)")});
                if (auto* table = panel->findChild<QTableWidget*>("native_driveTableY")) table->setHorizontalHeaderLabels({QStringLiteral("给定值 (%1)").arg(commandUnit)});
                limitHelp->setText(QStringLiteral("额定值限制力 / 力矩幅值；功率按实际速度计算。空载速度给出驱动方向上的线性力—速度包络，需设置正的额定值；制动仍受额定值与功率限制。0 表示不启用对应限制。"));
                setProblem(issue,nativeTypedRecordIssue(record,"drives",document));
            } catch (const std::exception& error) { setProblem(issue,QString::fromUtf8(error.what())); }
        };
        watch(panel,changed); changed();
    }
    return b.handled;
}

} // namespace MotionOcc
