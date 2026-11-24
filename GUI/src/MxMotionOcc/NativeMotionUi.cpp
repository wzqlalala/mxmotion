#include "MotionWindow.h"
#include "MotionTreeIcons.h"
#include "OccViewport.h"
#include "NativeMotionExchange.h"
#include "NativeMotionPresentation.h"
#include "NativeTopologyBinding.h"
#include "NativeMotionPickWidgets.h"
#include "NativeBodyMass.h"
#include "NativeFunctionEditor.h"
#include "NativeMotionTypedPanels.h"
#include "mFoldSection.h"
#include "compat/model/LegacyMotionContext.h"
#include "mUiMenu.h"
#include "ThemeManager.h"
#include "mFramelessMessageBox.h"
#include "mMotionScenario.h"
#include "mMotionScenarioPanel.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSet>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace MotionOcc {
namespace {

QString label(const QString& key)
{
    static const QMap<QString, QString> names{
        {"bodies", "运动体"}, {"joints", "运动副"}, {"couplings", "耦合副"}, {"drives", "驱动"},
        {"bodyDrives", "运动体驱动"}, {"bodyDriveMode", "驱动类型"},
        {"frameAngles", "初始坐标系内禀 XYZ 角 (°)"}, {"guide", "约束其余五个自由度"},
        {"springs", "弹簧"}, {"vectorForces", "力 / 力矩 / 衬套"}, {"scalarForces", "标量力"},
        {"scalarTorques", "标量力矩"}, {"threePointForces", "三点力"}, {"threePointTorques", "三点力矩"},
        {"tensionCables", "只拉绳索"}, {"softStops", "止挡"}, {"contacts", "接触"}, {"markers", "标记点"},
        {"sensors", "传感器"}, {"controllers", "控制器"}, {"controlInputs", "控制输入"},
        {"autoFlexBodies", "自动柔性体"}, {"importedMnfBodies", "导入柔性体"}, {"scenarios", "求解方案"},
        {"electroImports", "机电导入"}, {"electroExports", "机电导出"},
        {"hydraulics", "液压协同"}, {"rlTasks", "强化学习"},
        {"name", "名称"}, {"id", "编号"}, {"enabled", "启用"}, {"isFixed", "固定参考体"},
        {"bodyIId", "连接体 I"}, {"bodyJId", "连接体 J"}, {"bodyId", "运动体"},
        {"actionBodyId", "动作体"}, {"baseBodyId", "基本体"}, {"targetBodyId", "目标体"},
        {"referenceBodyId", "参考体"}, {"originBodyId", "方向原点体"},
        {"axisBodyId", "轴参考体"}, {"bodyAId", "测量体 A"}, {"bodyBId", "测量体 B"},
        {"localI", "作用点 I (初始全局 mm)"}, {"localJ", "作用点 J (初始全局 mm)"},
        {"localTarget", "方向目标点 (mm)"}, {"localOrigin", "方向原点 (mm)"},
        {"localPosition", "标记点位置 (mm)"}, {"localRotation", "标记点姿态 (四元数)"},
        {"localRotI", "连接坐标系 I"}, {"localRotJ", "连接坐标系 J"},
        {"centerOfMass", "质心 (mm)"}, {"mass", "质量 (kg)"}, {"inertia", "惯量 (kg·mm²)"},
        {"inertiaConvention", "惯量积约定"},
        {"isCustomMassProps", "指定质量属性"}, {"initialVelocity", "初始速度 (mm/s)"},
        {"densityKgMm3", "质量密度 (kg/mm³)"},
        {"initialAngularVelocity", "初始角速度 (°/s)"},
        {"jointType", "运动副类型"}, {"origin", "原点 (mm)"}, {"axis", "轴方向"},
        {"lead", "螺距 (mm/转)"}, {"actionSpin", "动作体转轴"}, {"actionPin", "动作体销轴"},
        {"baseSpin", "基本体转轴"}, {"basePin", "基本体销轴"},
        {"actionVector", "动作体方向"}, {"baseVector", "基本体方向"},
        {"allowSliding", "允许滑动"}, {"friction", "摩擦"}, {"limit", "运动副限位"},
        {"staticCoefficient", "静摩擦系数"}, {"kineticCoefficient", "动摩擦系数"},
        {"viscousCoefficient", "黏性系数"}, {"effectiveRadius", "有效半径 (mm)"},
        {"lowerEnabled", "启用下限"}, {"upperEnabled", "启用上限"}, {"lowerValue", "下限 (mm / °)"},
        {"upperValue", "上限 (mm / °)"}, {"penaltyOnly", "仅软限位"},
        {"lowerCushion", "下限缓冲区"}, {"upperCushion", "上限缓冲区"},
        {"lowerStiffness", "下限刚度"}, {"upperStiffness", "上限刚度"},
        {"lowerDamping", "下限阻尼"}, {"upperDamping", "上限阻尼"},
        {"stiffness", "刚度"}, {"damping", "阻尼"}, {"freeLength", "自由长度 (mm)"},
        {"engagementWidth", "接合过渡宽度 (mm)"}, {"direction", "方向"}, {"directionMode", "方向参考"},
        {"side", "止挡侧"}, {"stopPosition", "止挡坐标 (mm)"}, {"exponent", "力指数"},
        {"dampingDepth", "阻尼过渡深度 (mm)"}, {"hasTable", "使用力—压入量表"},
        {"tableX", "位移 / 压入量表 (mm，扭簧为 °)"}, {"tableY", "力表 (N，扭簧为 N·mm)"},
        {"springType", "弹簧类型"}, {"jointId", "运动副"}, {"freeValue", "自由值"},
        {"installValue", "安装值"}, {"preload", "预载"}, {"dampingEnabled", "启用阻尼"},
        {"springDirectionMode", "弹簧作用方向"}, {"dampingDirectionMode", "阻尼作用方向"},
        {"forceFunction", "力函数"}, {"dampingFunction", "阻尼函数"}, {"torqueFunction", "力矩函数"},
        {"controlMode", "驱动模式"}, {"targetJointId", "目标运动副"}, {"targetDriveId", "目标驱动"},
        {"motionType", "运动类型"}, {"expression", "函数表达式 (时间 t，秒)"},
        {"effortLimit", "额定力 / 力矩"}, {"powerLimit", "功率限制 (W)"}, {"noLoadSpeed", "空载速度"},
        {"driveTableX", "驱动时间表 (s)"}, {"driveTableY", "驱动数值表"},
        {"couplingType", "耦合类型"}, {"firstJointId", "第一运动副"}, {"secondJointId", "第二运动副"},
        {"gearRatio", "传动比"}, {"ratio", "传动比"}, {"radius", "节圆半径 (mm)"},
        {"pitchRadius1", "第一节圆半径 (mm)"}, {"pitchRadius2", "第二节圆半径 (mm)"},
        {"toothCount1", "第一齿数"}, {"toothCount2", "第二齿数"},
        {"loadType", "载荷类型"}, {"actionPoint", "作用点 (初始全局 mm)"},
        {"magnitude", "幅值 (N / N·mm)"}, {"magnitudeMode", "幅值来源"},
        {"fixedDirection", "固定方向"}, {"fixedAxis", "固定轴"}, {"axisMode", "力矩方向参考"},
        {"actionOnly", "仅作用于 I 体"}, {"offset", "偏置 (mm)"},
        {"preloadForce", "预载力 (N)"}, {"preloadTorque", "预载力矩 (N·mm)"},
        {"contactType", "接触类型"}, {"materialDamping", "接触阻尼"},
        {"forceExponent", "接触力指数"}, {"penetrationDepth", "接触阻尼全值深度 (mm)"},
        {"frictionEnabled", "启用摩擦"}, {"staticFriction", "静摩擦系数"}, {"dynamicFriction", "动摩擦系数"},
        {"useConvexHullAction", "动作体使用凸包"}, {"useConvexHullBase", "基本体使用凸包"},
        {"sensorType", "传感器类型"}, {"coordSystem", "坐标系"}, {"componentMode", "输出分量"},
        {"referencePoint", "测量点 (mm)"}, {"variable", "测量变量"}, {"pointA", "端点 A"}, {"pointB", "端点 B"},
        {"localPos", "位置 (mm)"}, {"axisDirection", "测量轴"},
        {"controllerType", "控制器类型"}, {"feedbackType", "反馈变量"}, {"feedbackSource", "反馈来源"},
        {"feedbackSensorId", "反馈传感器"}, {"feedbackSign", "反馈符号"},
        {"targetInputId", "目标输入"}, {"targetValue", "固定目标值"}, {"outputLimit", "输出限幅"},
        {"kp", "比例增益 Kp"}, {"ki", "积分增益 Ki"}, {"kd", "微分增益 Kd"},
        {"inputType", "输入类型"}, {"timeStart", "起始时间 (s)"}, {"timeEnd", "终止时间 (s)"},
        {"polyCoeffs", "多项式系数 (常数项在前)"}, {"curvePoints", "时间—输入值"},
        {"endTime", "终止时间 (s)"}, {"stepSize", "步长 (s)"}, {"gravity", "重力 (mm/s²)"},
        {"analysisType", "分析类型"}, {"staticMaxIterations", "静力最大迭代数"},
        {"staticTolerance", "相对平衡残差容差"}, {"staticConstraintTolerance", "约束残差容差 (mm / rad)"},
        {"staticStepDamping", "Newton 步长系数 (0 < 值 ≤ 1)"},
        {"usePreset", "使用求解预设"}, {"presetMode", "求解预设"}, {"solverType", "线性求解器"},
        {"solverTolerance", "求解容差"}, {"solverMaxIters", "最大求解迭代数"},
        {"integratorType", "积分器"}, {"integratorTol", "积分容差"}, {"integratorMaxIters", "最大积分迭代数"},
        {"integratorAlpha", "积分参数 α"}, {"integratorBeta", "积分参数 β"}, {"trackedMarkerIds", "跟踪标记点编号"},
        {"material", "材料"}, {"youngModulus", "弹性模量 (MPa)"}, {"poissonRatio", "泊松比"},
        {"density", "密度 (kg/mm³)"}, {"elementSize", "单元尺寸 (mm)"}, {"modeCount", "模态数量"},
        {"materialId", "材料"}, {"freqCutoff", "截止频率 (Hz，0 为不截断)"},
        {"connectionRadius", "连接半径 (mm，0 为自动)"}, {"dampingType", "阻尼类型"},
        {"modalDampingRatio", "模态阻尼比"}, {"rayleighMass", "瑞利质量系数"},
        {"rayleighStiffness", "瑞利刚度系数"},
        {"meshFile", "四面体网格文件"}, {"attachPoints", "连接点编号"},
        {"interfaceMode", "接口耦合方式"}, {"interfaceJointGroups", "独立接口的运动副分组"},
        {"occSurfaceFaces", "曲面绑定 (部件 / 面序号)"}, {"occCurveEdges", "曲线绑定 (部件 / 边序号)"},
        {"occCurveIEdges", "曲线 I 绑定"}, {"occCurveJEdges", "曲线 J 绑定"},
        {"partId", "部件编号"}, {"index", "面 / 边序号 (从 1 开始)"}
    };
    return names.value(key, key);
}

const QStringList jointTypes{"Revolute", "Translational", "Cylindrical", "Spherical", "Fixed", "Universal",
    "PointPlane", "Parallel", "Perpendicular", "Direction", "ConstantVelocity", "Hooke", "Screw", "Planar",
    "PointLine", "PointOnCurve", "PointOnSurface", "CurveOnCurve"};

QString enumLabel(const QString& value)
{
    static const QMap<QString, QString> names{{"Revolute", "旋转副"}, {"Translational", "移动"},
        {"Cylindrical", "圆柱副"}, {"Spherical", "球副"}, {"Fixed", "固定副"}, {"Universal", "万向副"},
        {"PointPlane", "点面约束"}, {"Parallel", "平行约束"}, {"Perpendicular", "垂直约束"},
        {"Direction", "方向约束"}, {"ConstantVelocity", "等速副"}, {"Hooke", "胡克副"}, {"Screw", "螺旋副"},
        {"Planar", "平面副"}, {"Coupling", "耦合约束"}, {"Gear", "齿轮"}, {"PointLine", "点线约束"},
        {"PointOnCurve", "点在曲线上"}, {"PointOnSurface", "点在曲面上"}, {"CurveOnCurve", "曲线在曲线上"},
        {"GearRack", "齿轮齿条"}, {"Cable", "双向线缆耦合"}, {"BodyPoint", "体点弹簧"}, {"Torsional", "扭转弹簧"},
        {"Position", "位置 / 位移"}, {"Velocity", "速度"}, {"Acceleration", "加速度"},
        {"LinearPosition", "平移位移"}, {"LinearSpeed", "线速度"}, {"RotationAngle", "转角"},
        {"RotationSpeed", "角速度"}, {"Pose", "六自由度位姿"},
        {"Dynamic", "动力学"}, {"Static", "静力平衡"}, {"PreloadedDynamics", "静力预载后动力学"},
        {"FAST", "FAST · 快速"}, {"CONTACT", "CONTACT · 接触"}, {"ROBUST", "ROBUST · 稳健"}, {"HIGH_PRECISION", "HIGH_PRECISION · 高精度"},
        {"SharedBody", "共用参考体 · 模态缩减"}, {"Independent", "独立接口 · 共回转全有限元"},
        {"Rotation", "转动"}, {"Translation", "平动"}, {"GlobalFixed", "全局固定"}, {"BodyFixed", "随体"},
        {"Constant", "常量"}, {"Expression", "函数"}, {"Force", "力"}, {"Torque", "力矩"}, {"Bushing", "衬套"},
        {"Bidirectional", "双向"}, {"PositiveOnly", "正向"}, {"NegativeOnly", "负向"},
        {"BodyDisplacement", "体位移"}, {"BodyVelocity", "体速度"}, {"BodyAcceleration", "体加速度"},
        {"JointReaction", "运动副反力"}, {"DriveActuation", "驱动力"}, {"Distance", "距离"}, {"Angle", "角度"},
        {"Magnitude", "幅值"}, {"All", "所有分量"}, {"DriveDof", "驱动自由度"}, {"Sensor", "传感器"},
        {"auto", "按原文档与几何核对"}, {"tensor", "惯量矩阵项"}, {"products", "正号惯量积"},
        {"Linear", "一次多项式"}, {"Quadratic", "二次多项式"}, {"Cubic", "三次多项式"}, {"CustomCurve", "自定义曲线"}};
    return names.value(value, value);
}

QJsonObject vec(double x = 0, double y = 0, double z = 0) { return {{"x", x}, {"y", y}, {"z", z}}; }
QJsonObject quat() { return {{"w", 1}, {"x", 0}, {"y", 0}, {"z", 0}}; }

// Original sensor widgets display world CAD coordinates while the sensor
// data model stores points relative to the body's initial COM. Other motion
// fields named localI/localJ/localPosition intentionally remain global.
void convertSensorPoints(QJsonObject& sensor, const NativeMotionDocument& document, bool toWorld)
{
    const auto convert=[&](const QJsonValue& point,int bodyId) -> QJsonValue {
        if (bodyId<0) return point;
        const auto center=document.entity("bodies",bodyId).value("centerOfMass");
        const auto component=[](const QJsonValue& p,int i,const QString& axis) { return p.isArray() && p.toArray().size()==3 ? p.toArray()[i].toDouble() : p.toObject().value(axis).toDouble(); };
        QJsonObject result; QJsonArray list;
        for (int i=0;i<3;++i) { const auto axis=QString(QChar('x'+i)); const double value=component(point,i,axis)+(toWorld?1:-1)*component(center,i,axis); result.insert(axis,value); list.append(value); }
        return point.isArray() ? QJsonValue(list) : QJsonValue(result);
    };
    if (sensor.contains("referencePoint")) sensor.insert("referencePoint",convert(sensor.value("referencePoint"),sensor.value("targetBodyId").toInt(-1)));
    for (const auto& key:{"pointA","pointB"}) if (sensor.value(key).isObject()) {
        auto point=sensor.value(key).toObject(); point.insert("localPos",convert(point.value("localPos"),point.value("bodyId").toInt(-1))); sensor.insert(key,point);
    }
}

int firstId(const NativeMotionDocument& document, const QString& collection, int fallback = -1)
{
    const auto rows = document.collection(collection);
    return rows.isEmpty() ? fallback : rows.first().toObject().value("id").toInt();
}

int nextId(const NativeMotionDocument& document, const QString& collection)
{
    QStringList collections{collection};
    const QStringList forces{"vectorForces", "scalarForces", "scalarTorques", "threePointForces", "threePointTorques"};
    if (forces.contains(collection)) collections = forces;
    QSet<int> used;
    for (const auto& kind : collections) for (const auto& item : document.collection(kind)) used.insert(item.toObject().value("id").toInt());
    int id = 1;
    while (used.contains(id)) ++id;
    return id;
}

void put(NativeMotionDocument& document, const QString& collection, const QJsonObject& item)
{
    auto data = document.root.value("motionData").toObject();
    auto rows = data.value(collection).toArray();
    int index = -1;
    for (int i = 0; i < rows.size(); ++i) if (rows[i].toObject().value("id") == item.value("id")) { index = i; break; }
    if (index < 0) rows.append(item); else rows[index] = item;
    data.insert(collection, rows);
    document.root.insert("motionData", data);
}

bool isTopologyField(const QString& key)
{
    return key == "occCurveEdges" || key == "occCurveIEdges" || key == "occCurveJEdges" || key == "occSurfaceFaces";
}

QString topologyBodyField(const QString& key)
{
    return key == "occCurveIEdges" ? QStringLiteral("actionBodyId") : QStringLiteral("baseBodyId");
}

QString topologySummary(const QJsonArray& selection, bool faces)
{
    QStringList items;
    for (const auto& value : selection) {
        const auto ref = value.toObject();
        items.append(QStringLiteral("部件 %1 / %2 %3").arg(ref.value("partId").toInt())
            .arg(faces ? QStringLiteral("面") : QStringLiteral("边")).arg(ref.value("index").toInt()));
    }
    return items.isEmpty() ? QStringLiteral("未指定几何") : items.join(QStringLiteral("；"));
}

bool selectConstraintTopology(QWidget* parent, const CadDocument& cad, const NativeMotionDocument& document,
                              const QJsonObject& joint, const QString& field, QJsonArray& selection,
                              const QString& reason = {})
{
    const bool faces = field == "occSurfaceFaces";
    const int bodyId = joint.value(topologyBodyField(field)).toInt(-1);
    const auto body = document.entity("bodies", bodyId);
    QDialog dialog(parent); dialog.setWindowTitle(QStringLiteral("选择运动副的约束%1").arg(faces ? "面" : "边")); dialog.resize(850, 570);
    auto* layout = new QVBoxLayout(&dialog);
    auto* help = new QLabel(QStringLiteral("%1 [%2] · %3 [%4]\n%5\n%6 下列坐标为初始全局坐标，单位 mm。")
        .arg(joint.value("name").toString()).arg(joint.value("id").toInt())
        .arg(body.value("name").toString()).arg(bodyId).arg(reason)
        .arg(faces ? QStringLiteral("可多选该运动体的完整约束面。")
                   : QStringLiteral("可选择实体边或独立构造曲线；曲线以所示运动体为参考，随该体运动。")));
    help->setWordWrap(true); layout->addWidget(help);
    auto* filter = new QLineEdit; filter->setPlaceholderText(QStringLiteral("筛选部件名称、边面类型或编号")); layout->addWidget(filter);
    auto* list = new QListWidget; list->setSelectionMode(QAbstractItemView::ExtendedSelection); layout->addWidget(list, 1);
    auto* problem = new QLabel; problem->setWordWrap(true); layout->addWidget(problem);
    try {
        QVector<int> candidates = document.bodyPartBindings.value(bodyId);
        if (!faces) { candidates.clear(); for (const auto& part : cad.parts()) candidates.append(part.id); }
        for (int partId : candidates) {
            const auto* part = cad.part(partId); if (!part) continue;
            TopTools_IndexedMapOfShape shapes; TopExp::MapShapes(part->shape, faces ? TopAbs_FACE : TopAbs_EDGE, shapes);
            for (int index = 1; index <= shapes.Extent(); ++index) {
                const auto& shape = shapes(index);
                QString kind;
                if (faces) {
                    switch (BRepAdaptor_Surface(TopoDS::Face(shape)).GetType()) {
                    case GeomAbs_Plane: kind = QStringLiteral("平面"); break;
                    case GeomAbs_Cylinder: kind = QStringLiteral("圆柱面"); break;
                    case GeomAbs_Sphere: kind = QStringLiteral("球面"); break;
                    case GeomAbs_Cone: kind = QStringLiteral("圆锥面"); break;
                    case GeomAbs_Torus: kind = QStringLiteral("环面"); break;
                    default: kind = QStringLiteral("曲面"); break;
                    }
                } else {
                    switch (BRepAdaptor_Curve(TopoDS::Edge(shape)).GetType()) {
                    case GeomAbs_Line: kind = QStringLiteral("直线"); break;
                    case GeomAbs_Circle: kind = QStringLiteral("圆弧 / 圆"); break;
                    case GeomAbs_Ellipse: kind = QStringLiteral("椭圆"); break;
                    default: kind = QStringLiteral("曲线"); break;
                    }
                }
                Bnd_Box bounds; BRepBndLib::AddOptimal(shape, bounds, false, false);
                if (bounds.IsVoid() || bounds.IsOpen()) continue;
                double x0, y0, z0, x1, y1, z1; bounds.Get(x0, y0, z0, x1, y1, z1);
                const QString text = QStringLiteral("%1 [%2] · %3 %4 · %5\n  范围 (%6, %7, %8) → (%9, %10, %11)")
                    .arg(part->name).arg(partId).arg(faces ? QStringLiteral("面") : QStringLiteral("边")).arg(index).arg(kind)
                    .arg(x0, 0, 'g', 9).arg(y0, 0, 'g', 9).arg(z0, 0, 'g', 9)
                    .arg(x1, 0, 'g', 9).arg(y1, 0, 'g', 9).arg(z1, 0, 'g', 9);
                auto* item = new QListWidgetItem(text, list); item->setData(Qt::UserRole, partId); item->setData(Qt::UserRole + 1, index);
                for (const auto& value : selection) {
                    const auto ref = value.toObject();
                    if (ref.value("partId").toInt() == partId && ref.value("index").toInt() == index) item->setSelected(true);
                }
            }
        }
    } catch (const Standard_Failure& failure) {
        problem->setText(QStringLiteral("无法读取约束几何：%1").arg(QString::fromUtf8(failure.GetMessageString())));
        return false;
    }
    if (list->count() == 0) problem->setText(QStringLiteral("所选运动体没有可用的约束几何。请先将该体绑定到相应的 CAD 部件；大地本身不包含边或面。"));
    QObject::connect(filter, &QLineEdit::textChanged, list, [list](const QString& text) {
        for (int i = 0; i < list->count(); ++i) list->item(i)->setHidden(!list->item(i)->text().contains(text, Qt::CaseInsensitive));
    });
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    buttons->button(QDialogButtonBox::Ok)->setEnabled(list->count() > 0);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (list->selectedItems().isEmpty()) { problem->setText(QStringLiteral("请明确选择至少一项约束几何。")); return; }
        QJsonArray selected;
        for (const auto* item : list->selectedItems()) selected.append(nativeTopologyReference(cad,
            item->data(Qt::UserRole).toInt(), item->data(Qt::UserRole + 1).toInt(), faces ? TopAbs_FACE : TopAbs_EDGE));
        selection = selected; dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}

QJsonObject bodyRecord(const CadDocument& cad, const RigidBody& body, QString& error, bool automatic = false)
{
    BodyProperties properties;
    if (!rigidBodyProperties(cad, body, properties, error)) return {};
    QJsonArray inertia;
    for (double v : properties.inertiaKgM2) inertia.append(v * 1e6);
    const auto* part = cad.part(body.partId);
    const auto& c = properties.centerMm;
    QJsonObject row{{"id", body.id}, {"name", body.name}, {"geoEntityIds", QJsonArray{body.partId}},
        {"geoEntityNames", QJsonArray{part ? (part->sourceName.isEmpty() ? part->name : part->sourceName) : QString{}}},
        {"isFixed", body.fixed}, {"isCustomMassProps", true}, {"mass", properties.massKg},
        {"centerOfMass", vec(c[0], c[1], c[2])}, {"inertia", inertia}, {"inertiaConvention", "tensor"},
        {"initialVelocity", vec()}, {"initialAngularVelocity", vec()}};
    if (automatic) {
        row.insert("isCustomMassProps",false);
        if (!updateNativeBodyMass(cad,std::vector<int>{body.partId},row,kDefaultNativeBodyDensityKgMm3,error)) return {};
    }
    return row;
}

QJsonObject templateRecord(const NativeMotionDocument& doc, const QString& collection, const QString& type)
{
    const int key = nextId(doc, collection), body = firstId(doc, "bodies"), joint = firstId(doc, "joints");
    QJsonObject row{{"id", key}, {"name", label(collection) + QStringLiteral("-%1").arg(key)}};
    auto merge = [&](const QJsonObject& values) { for (auto it = values.begin(); it != values.end(); ++it) row.insert(it.key(), it.value()); };
    const QJsonObject pair{{"bodyIId", body}, {"bodyJId", -1}, {"localI", vec()}, {"localJ", vec(0, 0, -100)}};
    if (collection == "joints") {
        merge({{"jointType", type.isEmpty() ? "Revolute" : type}, {"actionBodyId", body}, {"baseBodyId", -1},
            {"origin", vec()}, {"axis", vec(0, 0, 1)}});
        if (type == "Screw") row.insert("lead", 10.0);
        if (type == "Universal") merge({{"actionSpin", vec(0, 0, 1)}, {"actionPin", vec(1, 0, 0)},
            {"baseSpin", vec(0, 1, 0)}, {"basePin", vec(1, 0, 0)}});
        if (type == "Perpendicular" || type == "ConstantVelocity") merge({{"actionVector", vec(1, 0, 0)}, {"baseVector", vec(0, 1, 0)}});
        if (type == "CurveOnCurve") merge({{"occCurveIEdges", QJsonArray{}}, {"occCurveJEdges", QJsonArray{}}, {"allowSliding", false}});
        if (type == "PointOnCurve") row.insert("occCurveEdges", QJsonArray{});
        if (type == "PointOnSurface") row.insert("occSurfaceFaces", QJsonArray{});
        if (type == "Revolute" || type == "Translational") merge({
            {"friction", QJsonObject{{"enabled", false}, {"staticCoefficient", 0.3}, {"kineticCoefficient", 0.1},
                {"viscousCoefficient", 0.0}, {"effectiveRadius", 0.0}}},
            {"limit", QJsonObject{{"enabled", false}, {"lowerEnabled", false}, {"upperEnabled", false},
                {"lowerValue", -10.0}, {"upperValue", 10.0}, {"penaltyOnly", true},
                {"lowerCushion", 0.1}, {"upperCushion", 0.1}, {"lowerStiffness", 100.0}, {"upperStiffness", 100.0},
                {"lowerDamping", 1.0}, {"upperDamping", 1.0}}}});
    } else if (collection == "bodyDrives") {
        const QString mode = type.isEmpty() ? QStringLiteral("LinearPosition") : type;
        int movable = -1;
        for (const auto& item : doc.collection("bodies")) {
            const auto candidate = item.toObject();
            if (!candidate.value("isFixed").toBool()) {
                movable = candidate.value("id").toInt(-1);
                break;
            }
        }
        merge({{"bodyDriveMode", mode}, {"targetBodyId", movable}, {"referenceBodyId", -1},
            {"origin", vec()}, {"enabled", true}});
        const bool pose = mode == "Pose";
        if (pose) row.insert("frameAngles", vec());
        else {
            row.insert("guide", true);
            row.insert("axis", mode.startsWith("Rotation") ? vec(0, 0, 1) : vec(1, 0, 0));
        }
        QJsonObject channels;
        for (const auto axis : (pose ? QStringLiteral("XYZABC") : QStringLiteral("X")))
            channels.insert(QString(axis), QJsonObject{{"expression", "0"}, {"driveTableX", QJsonArray{}}, {"driveTableY", QJsonArray{}}});
        row.insert("channels", channels);
    } else if (collection == "drives") {
        const bool rotation = doc.entity("joints", joint).value("jointType").toString() != "Translational";
        merge({{"driveType", "JointDrive"}, {"targetType", "Joint"}, {"targetJointId", joint},
            {"motionType", rotation ? "Rotation" : "Translation"}, {"controlMode", type == "Velocity" ? "Velocity" : "Position"},
            {"expression", "0"}, {"driveTableX", QJsonArray{}}, {"driveTableY", QJsonArray{}}});
    } else if (collection == "couplings") {
        merge({{"couplingType", type.isEmpty() ? "Gear" : type}, {"firstJointId", joint}, {"secondJointId", joint}});
        if (type == "Cable") row.insert("ratio", 1.0);
        else if (type == "GearRack") row.insert("radius", 10.0);
        else merge({{"gearRatio", -1.0}, {"pitchRadius1", 10.0}, {"pitchRadius2", 10.0}, {"toothCount1", 20}, {"toothCount2", 20}});
    } else if (collection == "tensionCables") {
        merge(pair); merge({{"enabled", true}, {"freeLength", 100.0}, {"stiffness", 10.0}, {"damping", 0.1}, {"engagementWidth", 0.1}});
    } else if (collection == "softStops") {
        merge(pair); merge({{"enabled", true}, {"direction", vec(0, 0, 1)}, {"directionMode", 1}, {"side", 1},
            {"stopPosition", 110.0}, {"stiffness", 100.0}, {"exponent", 1.0}, {"damping", 1.0},
            {"dampingDepth", 0.1}, {"hasTable", false}, {"tableX", QJsonArray{}}, {"tableY", QJsonArray{}}});
    } else if (collection == "springs") {
        merge(pair); merge({{"springType", type.isEmpty() ? "BodyPoint" : type}, {"jointId", joint},
            {"localRotI", quat()}, {"localRotJ", quat()}, {"stiffness", 10.0}, {"damping", 0.1}, {"dampingEnabled", true},
            {"freeValue", 100.0}, {"installValue", 100.0}, {"preload", 0.0},
            {"springDirectionMode", "Bidirectional"}, {"dampingDirectionMode", "Bidirectional"},
            {"forceFunction", ""}, {"dampingFunction", ""}, {"tableX", QJsonArray{}}, {"tableY", QJsonArray{}}});
    } else if (collection == "vectorForces") {
        merge({{"bodyId", body}, {"loadType", type.isEmpty() ? "Force" : type}, {"actionPoint", vec()},
            {"direction", vec(0, 0, 1)}, {"directionMode", "GlobalFixed"}, {"magnitude", 1.0},
            {"magnitudeMode", "Constant"}, {"expression", ""}});
        if (type == "Bushing") merge({{"baseBodyId", -1}, {"offset", vec()},
            {"stiffness", QJsonArray{1, 1, 1, 1, 1, 1}}, {"damping", QJsonArray{0, 0, 0, 0, 0, 0}},
            {"preloadForce", QJsonArray{0, 0, 0}}, {"preloadTorque", QJsonArray{0, 0, 0}}});
    } else if (collection == "scalarForces" || collection == "scalarTorques") {
        merge(pair); merge({{"magnitude", 1.0}, {"actionOnly", false}});
        if (collection == "scalarForces") merge({{"directionMode", 0}, {"fixedDirection", vec(0, 0, 1)}, {"forceFunction", ""}});
        else merge({{"axisMode", 1}, {"fixedAxis", vec(0, 0, 1)}, {"torqueFunction", ""}});
    } else if (collection == "threePointForces" || collection == "threePointTorques") {
        merge({{"bodyIId", body}, {"targetBodyId", -1}, {"originBodyId", -1}, {"localI", vec()},
            {"localTarget", vec(0, 0, 100)}, {"localOrigin", vec()}, {"magnitude", 1.0}});
        row.insert(collection == "threePointForces" ? QStringLiteral("forceFunction") : QStringLiteral("torqueFunction"), "");
    } else if (collection == "contacts") {
        merge({{"contactType", "CADContact"}, {"actionBodyId", body}, {"baseBodyId", body},
            {"stiffness", 100.0}, {"forceExponent", 1.5}, {"materialDamping", 10.0}, {"penetrationDepth", 0.0254},
            {"frictionEnabled", false}, {"staticFriction", 0.3}, {"dynamicFriction", 0.1},
            {"useConvexHullAction", false}, {"useConvexHullBase", false}});
    } else if (collection == "markers") {
        merge({{"bodyId", body}, {"localPosition", vec()}, {"localRotation", quat()}, {"size", 5.0}, {"visible", true}});
    } else if (collection == "sensors") {
        const QString sensor = type.isEmpty() ? QStringLiteral("BodyDisplacement") : type;
        merge({{"sensorType", sensor}, {"coordSystem", "GlobalFixed"}, {"referenceBodyId", -1},
            {"componentMode", "Z"}, {"enabled", true}});
        if (sensor.startsWith("Body")) merge({{"targetBodyId", body}, {"referencePoint", vec()},
            {"variable", sensor == "BodyVelocity" ? "Velocity" : sensor == "BodyAcceleration" ? "Acceleration" : "Position"}});
        else if (sensor == "JointReaction") merge({{"targetJointId", joint}, {"variable", "ReactionForce"}});
        else if (sensor == "DriveActuation") merge({{"targetDriveId", firstId(doc, "drives")}, {"variable", "ActuationForce"}});
        else if (sensor == "Distance") merge({{"pointA", QJsonObject{{"bodyId", body}, {"localPos", vec()}}},
            {"pointB", QJsonObject{{"bodyId", -1}, {"localPos", vec()}}}, {"variable", "Distance"}});
        else merge({{"axisBodyId", -1}, {"bodyAId", body}, {"bodyBId", -1}, {"axisDirection", vec(0, 0, 1)},
            {"directionA", vec(1, 0, 0)}, {"directionB", vec(0, 1, 0)}});
    } else if (collection == "controllers") {
        merge({{"controllerType", "PID"}, {"targetDriveId", firstId(doc, "drives")}, {"targetInputId", -1},
            {"feedbackSensorId", -1}, {"feedbackType", "Position"}, {"feedbackSource", "DriveDof"},
            {"targetValue", 0.0}, {"feedbackSign", 1.0}, {"kp", 1.0}, {"ki", 0.0}, {"kd", 0.0}, {"outputLimit", 0.0}, {"enabled", true}});
    } else if (collection == "controlInputs") {
        merge({{"inputType", type.isEmpty() ? "Constant" : type}, {"timeStart", 0.0}, {"timeEnd", 1.0},
            {"polyCoeffs", QJsonArray{0, 0, 0, 0}}, {"curvePoints", QJsonArray{QJsonArray{0, 0}, QJsonArray{1, 1}}}});
    } else if (collection == "autoFlexBodies") {
        merge({{"targetBodyId", body}, {"materialId", -1}, {"material", QJsonObject{{"youngModulus", 210000.0},
            {"poissonRatio", 0.3}, {"density", 7.85e-6}}}, {"elementSize", 5.0}, {"modeCount", 10},
            {"meshFile", ""}, {"attachPoints", QJsonArray{}}, {"enabled", true}, {"connectionRadius", 0.0},
            {"freqCutoff", 0.0}, {"dampingType", 2}, {"modalDampingRatio", 0.02},
            {"rayleighMass", 10.0}, {"rayleighStiffness", 2e-4}});
    } else if (collection == "scenarios") {
        merge({{"endTime", 1.0}, {"stepSize", 0.001}, {"gravity", vec(0, 0, -9806.65)},
            {"usePreset", true}, {"presetMode", "FAST"}, {"solverType", "SPARSE_QR"},
            {"solverTolerance", 1e-10}, {"solverMaxIters", 300}, {"integratorType", "EULER_IMPLICIT"},
            {"integratorTol", 1e-8}, {"integratorMaxIters", 30}, {"integratorAlpha", -0.2}, {"integratorBeta", 0.25},
            {"trackedMarkerIds", QJsonArray{}}, {"curveRequests", QJsonArray{}}});
    }
    return row;
}

using ReadValue = std::function<QJsonValue()>;

QStringList choices(const QString& key)
{
    if (key == "jointType") return jointTypes;
    static const QMap<QString, QStringList> values{
        {"couplingType", {"Gear", "GearRack", "Cable"}}, {"springType", {"BodyPoint", "Translational", "Torsional"}},
        {"inertiaConvention", {"auto", "tensor", "products"}},
        {"controlMode", {"Position", "Velocity", "Force", "Torque"}}, {"motionType", {"Rotation", "Translation"}},
        {"bodyDriveMode", {"LinearPosition", "LinearSpeed", "RotationAngle", "RotationSpeed", "Pose"}},
        {"analysisType", {"Dynamic", "Static", "PreloadedDynamics"}},
        {"interfaceMode", {"SharedBody", "Independent"}},
        {"loadType", {"Force", "Torque", "Bushing"}}, {"magnitudeMode", {"Constant", "Expression"}},
        {"directionMode", {"GlobalFixed", "BodyFixed"}}, {"coordSystem", {"GlobalFixed", "BodyFixed"}},
        {"springDirectionMode", {"Bidirectional", "PositiveOnly", "NegativeOnly"}},
        {"dampingDirectionMode", {"Bidirectional", "PositiveOnly", "NegativeOnly"}},
        {"sensorType", {"BodyDisplacement", "BodyVelocity", "BodyAcceleration", "JointReaction", "DriveActuation", "Distance", "Angle"}},
        {"componentMode", {"X", "Y", "Z", "Magnitude", "All"}}, {"feedbackSource", {"DriveDof", "Sensor"}},
        {"feedbackType", {"Position", "Velocity"}}, {"inputType", {"Constant", "Linear", "Quadratic", "Cubic", "CustomCurve"}},
        {"presetMode", {"FAST", "CONTACT", "ROBUST", "HIGH_PRECISION"}},
        {"solverType", {"SPARSE_QR", "SPARSE_LU", "PSOR", "PSSOR", "PJACOBI", "PMINRES", "APGD", "GMRES", "MINRES"}},
        {"integratorType", {"EULER_IMPLICIT", "EULER_IMPLICIT_LINEARIZED", "EULER_IMPLICIT_PROJECTED", "TRAPEZOIDAL", "TRAPEZOIDAL_LINEARIZED", "HHT", "HEUN", "RUNGEKUTTA45", "EULER_EXPLICIT", "LEAPFROG", "NEWMARK"}}
    };
    return values.value(key);
}

ReadValue addField(QFormLayout* form, const QString& key, const QJsonValue& value,
                   const NativeMotionDocument& doc, const QString& collection, int depth = 0,
                   NativePickSession* picker = nullptr, const QString& parentPath = {}, const QString& fieldLabel = {})
{
    const QString text = fieldLabel.isEmpty() ? label(key) : fieldLabel;
    const auto addLabeledRow = [&](QWidget* editor) {
        if (collection == "bodyDrives" && depth == 0) form->addRow(new PanelLabel(text), editor);
        else form->addRow(text, editor);
    };
    const QString path = parentPath.isEmpty() ? key : parentPath + "." + key;
    const QStringList vectorKeys{"origin","frameAngles","actionPoint","centerOfMass","localI","localJ","localTarget","localOrigin","localPosition",
        "referencePoint","localPos","position","axis","direction","fixedDirection","fixedAxis","actionSpin","actionPin","baseSpin","basePin",
        "actionVector","baseVector","axisDirection","gravity","initialVelocity","initialAngularVelocity"};
    if (value.isArray() && value.toArray().size()==3 && vectorKeys.contains(key)) {
        const auto values=value.toArray(); const auto read=addField(form,key,vec(values[0].toDouble(),values[1].toDouble(),values[2].toDouble()),doc,collection,depth,picker,parentPath);
        return [read] { const auto v=read().toObject(); return QJsonArray{v.value("x"),v.value("y"),v.value("z")}; };
    }
    if (value.isObject() && depth < 8) {
        auto* box = new QGroupBox(text);
        auto* nested = new QFormLayout(box);
        nested->setSizeConstraint(QLayout::SetMinimumSize);
        const auto original = value.toObject();
        QMap<QString, ReadValue> fields;
        for (auto it = original.begin(); it != original.end(); ++it) fields.insert(it.key(), addField(nested, it.key(), it.value(), doc, collection, depth + 1, picker, path));
        QWidget* pickWidget = nullptr;
        if (picker && original.size()==3 && original.contains("x") && original.contains("y") && original.contains("z"))
            pickWidget = picker->decorateVector(nested,box,key,path);
        if (collection == "bodyDrives" && depth == 0 && pickWidget) {
            // 直接使用原拾取控件自己的边框。隐藏容器只保留序列化使用的坐标字段。
            nested->removeWidget(pickWidget);
            form->addRow(pickWidget);
            box->setParent(form->parentWidget());
            box->hide();
        } else {
            form->addRow(box);
        }
        return [fields, original] { auto out = original; for (auto it = fields.begin(); it != fields.end(); ++it) out.insert(it.key(), it.value()()); return out; };
    }
    if (value.isBool()) {
        auto* check = new QCheckBox(text); check->setChecked(value.toBool()); check->setObjectName("native_" + key);
        form->addRow(check); return [check] { return check->isChecked(); };
    }
    QString references;
    if (key.endsWith("BodyId") || key == "bodyId" || key == "bodyIId" || key == "bodyJId" || key == "bodyAId" || key == "bodyBId") references = "bodies";
    if (key.endsWith("JointId") || key == "jointId") references = "joints";
    if (key == "targetDriveId") references = "drives";
    if (key == "targetInputId") references = "controlInputs";
    if (key == "feedbackSensorId") references = "sensors";
    if (!references.isEmpty() && value.isDouble()) {
        auto* combo = new QComboBox; combo->setObjectName("native_" + key);
        combo->addItem(references == "bodies" ? QStringLiteral("大地 (-1)") : QStringLiteral("无 (-1)"), -1);
        for (const auto& item : doc.collection(references)) { const auto row = item.toObject();
            combo->addItem(QStringLiteral("%1 [%2]").arg(row.value("name").toString()).arg(row.value("id").toInt()), row.value("id").toInt()); }
        int index = combo->findData(value.toInt());
        if (index < 0) { combo->addItem(QStringLiteral("缺失引用 [%1]").arg(value.toInt()), value.toInt()); index = combo->count() - 1; }
        combo->setCurrentIndex(index); addLabeledRow(combo); return [combo] { return combo->currentData().toInt(); };
    }
    if ((key == "directionMode" || key == "axisMode" || key == "side") && value.isDouble()) {
        auto* combo = new QComboBox; combo->setObjectName("native_" + key);
        if (key == "side") { combo->addItem(QStringLiteral("上止挡 (+1)"), 1); combo->addItem(QStringLiteral("下止挡 (-1)"), -1); }
        else {
            if (collection != "softStops") combo->addItem(QStringLiteral("连接线方向"), 0);
            combo->addItem(QStringLiteral("全局固定"), 1); combo->addItem(QStringLiteral("随 I 体转动"), 2); combo->addItem(QStringLiteral("随 J 体转动"), 3);
        }
        combo->setCurrentIndex(combo->findData(value.toInt())); form->addRow(text, combo);
        return [combo] { return combo->currentData().toInt(); };
    }
    const auto options = choices(key);
    if (value.isString() && !options.isEmpty()) {
        auto* combo = new QComboBox; combo->setObjectName("native_" + key);
        for (const auto& option : options) combo->addItem(enumLabel(option), option);
        int index = combo->findData(value.toString());
        if (index < 0) { combo->addItem(value.toString(), value.toString()); index = combo->count() - 1; }
        combo->setCurrentIndex(index);
        if (QStringList{"jointType", "couplingType", "springType", "sensorType", "loadType", "bodyDriveMode"}.contains(key)) combo->setEnabled(false);
        addLabeledRow(combo); return [combo] { return combo->currentData().toString(); };
    }
    if (value.isString() || value.isDouble()) {
        const QString original = value.isDouble() ? QString::number(value.toDouble(), 'g', 17) : value.toString();
        auto* edit = new QLineEdit(original); edit->setObjectName("native_" + key); edit->setMinimumWidth(70);
        const bool law=value.isString()&&collection!="drives"
            &&QStringList{"expression","forceFunction","torqueFunction","dampingFunction"}.contains(key)
            &&QStringList{"springs","vectorForces","scalarForces","scalarTorques","threePointForces","threePointTorques"}.contains(collection);
        if (law) {
            auto* container=new QWidget;auto* layout=new QVBoxLayout(container);layout->setContentsMargins(0,0,0,0);
            layout->addWidget(edit);
            auto* builder=new QPushButton(QStringLiteral("函数构造与预览…"));builder->setObjectName("native_functionBuilder_"+key);
            layout->addWidget(builder);form->addRow(text,container);
            const bool stateVariables=collection!="vectorForces";
            auto* hint=new QLabel(collection=="vectorForces" ? QStringLiteral("t 为时间 (s)；函数覆盖常数幅值。")
                : collection.endsWith("Torques") ? QStringLiteral("t：s；x：相对转角 (rad)；v：相对角速度 (rad/s)。")
                : collection=="springs" ? QStringLiteral("t：s；x/v：变形及其速度。平动为 mm、mm/s，扭转为 °、°/s。")
                : QStringLiteral("t：s；x：沿力方向的相对位移 (mm)；v：相对速度 (mm/s)。"));
            hint->setWordWrap(true);layout->addWidget(hint);
            QObject::connect(builder,&QPushButton::clicked,container,[edit,collection,key,doc,stateVariables] {
                QWidget* editor=edit;
                while(editor->parentWidget()&&!editor->objectName().startsWith("nativeEditor_"))editor=editor->parentWidget();
                bool torque=collection.endsWith("Torques");
                if(auto* type=editor->findChild<QComboBox*>("native_springType"))torque=type->currentData().toString()=="Torsional";
                if(auto* type=editor->findChild<QComboBox*>("native_loadType"))torque=type->currentData().toString()=="Torque";
                const auto scenarios=doc.collection("scenarios");
                const double duration=scenarios.isEmpty()?10:scenarios.first().toObject().value("endTime").toDouble(10);
                auto expression=edit->text();QJsonArray x,y;
                if(!editNativeFunction(editor,expression,x,y,torque?QStringLiteral("N·mm"):QStringLiteral("N"),duration,stateVariables,false))return;
                edit->setText(expression);
                if(collection=="vectorForces")if(auto* mode=editor->findChild<QComboBox*>("native_magnitudeMode"))mode->setCurrentIndex(mode->findData("Expression"));
                // The spring solver gives a nonempty displacement table priority
                // over forceFunction. Applying a force formula switches that
                // law explicitly, while a damping formula keeps the force law.
                if(collection=="springs"&&key=="forceFunction")for(const auto& field:{"native_tableX","native_tableY"})
                    if(auto* table=editor->findChild<QTableWidget*>(field))table->setRowCount(0);
                if(collection=="springs"&&key=="dampingFunction")
                    if(auto* enabled=editor->findChild<QCheckBox*>("native_dampingEnabled"))enabled->setChecked(true);
            });
        }else addLabeledRow(edit);
        return [edit, value, original, text] {
            if (edit->text() == original) return value;
            if (value.isString()) return QJsonValue(edit->text());
            bool ok = false; const double number = edit->text().toDouble(&ok);
            if (!ok || !std::isfinite(number)) throw std::runtime_error((text + QStringLiteral("：请输入有限数值")).toStdString());
            return QJsonValue(number);
        };
    }
    if (value.isArray() && value.toArray().size() <= 100000) {
        const auto original = value.toArray();
        const bool topology = key.startsWith("occ");
        const bool pairs = key == "curvePoints";
        const bool strings = !original.isEmpty() && original.first().isString();
        const bool simple = std::all_of(original.begin(), original.end(), [&](const auto& item) {
            return item.isDouble() || item.isString() || (pairs && item.isArray()) || (topology && item.isObject()); });
        if (simple) {
            auto* box = new QGroupBox(text); auto* layout = new QVBoxLayout(box);
            layout->setSizeConstraint(QLayout::SetMinimumSize);
            auto* table = new QTableWidget(original.size(), (pairs || topology) ? 2 : 1); table->setObjectName("native_" + key);
            table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            table->setHorizontalHeaderLabels(topology ? QStringList{QStringLiteral("部件 ID"), QStringLiteral("面 / 边序号")}
                : pairs ? QStringList{QStringLiteral("时间 (s)"), QStringLiteral("数值")} : QStringList{QStringLiteral("数值")});
            table->setMinimumHeight(95); table->setMaximumHeight(200);
            for (int r = 0; r < original.size(); ++r) for (int c = 0; c < table->columnCount(); ++c) {
                const auto item = topology ? original[r].toObject().value(c == 0 ? "partId" : "index")
                    : pairs ? original[r].toArray().at(c) : original[r];
                table->setItem(r, c, new QTableWidgetItem(item.isString() ? item.toString() : QString::number(item.toDouble(), 'g', 17)));
            }
            auto* buttons = new QWidget; auto* actions = new QHBoxLayout(buttons); actions->setContentsMargins(0, 0, 0, 0);
            auto* add = new QPushButton(QStringLiteral("添加行")); auto* remove = new QPushButton(QStringLiteral("删除行"));
            actions->addWidget(add); actions->addWidget(remove); layout->addWidget(table); layout->addWidget(buttons); form->addRow(box);
            QObject::connect(add, &QPushButton::clicked, table, [table] { if (table->rowCount() < 100000) {
                const int row = table->rowCount(); table->insertRow(row);
                for (int c = 0; c < table->columnCount(); ++c) table->setItem(row, c, new QTableWidgetItem("0")); } });
            QObject::connect(remove, &QPushButton::clicked, table, [table] { if (table->currentRow() >= 0) table->removeRow(table->currentRow()); });
            return [table, topology, pairs, strings, text] {
                QJsonArray out;
                for (int r = 0; r < table->rowCount(); ++r) {
                    QJsonArray values;
                    for (int c = 0; c < table->columnCount(); ++c) {
                        const auto* item = table->item(r, c); const QString input = item ? item->text() : QString{};
                        if (strings) values.append(input);
                        else { bool ok = false; const double n = input.toDouble(&ok);
                            if (!ok || !std::isfinite(n)) throw std::runtime_error((text + QStringLiteral("：表格包含无效数值")).toStdString()); values.append(n); }
                    }
                    if (topology) out.append(QJsonObject{{"partId", values[0]}, {"index", values[1]}});
                    else if (pairs) out.append(values); else out.append(values[0]);
                }
                return out;
            };
        }
    }
    form->addRow(text, new QLabel(value.isArray() ? QStringLiteral("%1 项，保留原始数据").arg(value.toArray().size()) : QStringLiteral("保留原始数据")));
    return [value] { return value; };
}

QListWidget* partList(const CadDocument& cad, const NativeMotionDocument& document, int bodyId)
{
    auto* parts = new QListWidget;
    parts->setObjectName(QStringLiteral("nativeBodyParts"));
    parts->setMinimumHeight(120); parts->setMaximumHeight(280);
    const auto selected = document.bodyPartBindings.value(bodyId);
    for (const auto& part : cad.parts()) {
        bool occupied = false;
        for (auto it = document.bodyPartBindings.cbegin(); it != document.bodyPartBindings.cend(); ++it)
            if (it.key() != bodyId && it.value().contains(part.id)) occupied = true;
        auto* item = new QListWidgetItem(QStringLiteral("%1 [%2]%3").arg(part.name).arg(part.id).arg(occupied ? QStringLiteral(" · 已绑定") : QString{}), parts);
        item->setData(Qt::UserRole, part.id); item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(selected.contains(part.id) ? Qt::Checked : Qt::Unchecked);
        if (occupied) item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
    }
    return parts;
}

QVector<int> selectedParts(const QListWidget* parts)
{
    QVector<int> result;
    for (int i = 0; i < parts->count(); ++i) if (parts->item(i)->checkState() == Qt::Checked) result.append(parts->item(i)->data(Qt::UserRole).toInt());
    return result;
}

QJsonObject collectFlexMesh(const QVector<int>& partIds, const std::map<int, FemMesh>& meshes)
{
    if (partIds.isEmpty()) return {};
    FemMesh merged;
    QJsonArray ranges;
    for (int partId : partIds) {
        const auto found=meshes.find(partId);
        if (found==meshes.end() || found->second.tetrahedra.empty()) return {};
        const auto& mesh=found->second;
        const auto offset=std::uint32_t(merged.nodes.size());
        ranges.append(QJsonObject{{"partId", partId}, {"firstNode", int(offset)}, {"nodeCount", int(mesh.nodes.size())},
            {"firstTet", int(merged.tetrahedra.size())}, {"tetCount", int(mesh.tetrahedra.size())},
            {"firstTriangle", int(merged.surface.size())}, {"triangleCount", int(mesh.surface.size())}});
        merged.nodes.insert(merged.nodes.end(), mesh.nodes.begin(), mesh.nodes.end());
        for (auto cell : mesh.tetrahedra) { for (auto& node : cell) node+=offset; merged.tetrahedra.push_back(cell); }
        for (auto cell : mesh.surface) { for (auto& node : cell) node+=offset; merged.surface.push_back(cell); }
    }
    auto object=femMeshJson(merged);
    object.insert("partRanges", ranges);
    return object;
}

bool restoreFlexMesh(const QJsonObject& object, const QVector<int>& partIds, std::map<int, FemMesh>& meshes)
{
    FemMesh parsed; QString error;
    if (!readFemMeshJson(object, parsed, error)) return false;
    const auto ranges=object.value("partRanges").toArray();
    if (ranges.isEmpty()) {
        if (partIds.size()!=1) return false;
        meshes[partIds.front()]=std::move(parsed); return true;
    }
    std::map<int, FemMesh> parts;
    for (const auto& value : ranges) {
        const auto range=value.toObject(); const int partId=range.value("partId").toInt(-1);
        if (!partIds.contains(partId) || parts.count(partId)) return false;
        const int first=range.value("firstNode").toInt(-1), count=range.value("nodeCount").toInt(-1);
        if (first<0 || count<=0 || std::size_t(first)+std::size_t(count)>parsed.nodes.size()) return false;
        FemMesh part; part.nodes.assign(parsed.nodes.begin()+first, parsed.nodes.begin()+first+count);
        const auto extract = [&](const auto& source, auto& destination, const char* firstKey, const char* countKey) {
            const int begin=range.value(firstKey).toInt(-1), size=range.value(countKey).toInt(-1);
            if (begin<0 || size<0 || std::size_t(begin)+std::size_t(size)>source.size()) return false;
            for (int i=0; i<size; ++i) {
                auto cell=source[begin+i];
                for (auto& node : cell) {
                    if (node<std::uint32_t(first) || node>=std::uint32_t(first+count)) return false;
                    node-=first;
                }
                destination.push_back(cell);
            }
            return true;
        };
        if (!extract(parsed.tetrahedra, part.tetrahedra, "firstTet", "tetCount")
            || !extract(parsed.surface, part.surface, "firstTriangle", "triangleCount") || part.tetrahedra.empty()) return false;
        parts.emplace(partId, std::move(part));
    }
    if (parts.size()!=std::size_t(partIds.size())) return false;
    for (auto& part : parts) meshes[part.first]=std::move(part.second);
    return true;
}

QString flexMeshSummary(const QJsonObject& mesh)
{
    const int nodes=mesh.value("nodes").toArray().size(), tets=mesh.value("tetrahedra").toArray().size();
    return nodes && tets ? QStringLiteral("%1 节点 / %2 四面体；保存与求解复用此体网格。").arg(nodes).arg(tets)
        : QStringLiteral("尚无体网格。保存后按单元尺寸自动划分并预览；已有体网格会直接复用。");
}

bool selectFlexNodes(QWidget* parent, QJsonObject& mesh, QJsonArray& selected)
{
    FemMesh parsed; QString error;
    if (!readFemMeshJson(mesh, parsed, error)) return false;
    auto nodeIds=mesh.value("nodeIds").toArray();
    const bool mapped=nodeIds.size()==int(parsed.nodes.size());
    if (!mapped) { nodeIds={}; for (int i=0; i<int(parsed.nodes.size()); ++i) nodeIds.append(i+1); }
    QDialog dialog(parent); dialog.setWindowTitle(QStringLiteral("选择真实体网格的附着节点")); dialog.resize(470, 540);
    auto* layout=new QVBoxLayout(&dialog);
    auto* help=new QLabel(mapped || selected.isEmpty()
        ? QStringLiteral("勾选节点作为附着接口；相邻节点自动组成同一接口。节点坐标为初始全局 mm。")
        : QStringLiteral("原附着节点编号没有对应坐标。请在当前真实体网格上重新选择；不会按相同数字猜测旧节点。"));
    help->setWordWrap(true); layout->addWidget(help);
    auto* filter=new QLineEdit; filter->setPlaceholderText(QStringLiteral("筛选节点编号或坐标")); layout->addWidget(filter);
    auto* list=new QListWidget; list->setObjectName("nativeFlexNodeList"); layout->addWidget(list, 1);
    QSet<int> current; if (mapped) for (const auto& id : selected) current.insert(id.toInt());
    for (int i=0; i<int(parsed.nodes.size()); ++i) {
        const auto& p=parsed.nodes[i]; const int id=nodeIds[i].toInt();
        auto* item=new QListWidgetItem(QStringLiteral("%1   (%2, %3, %4) mm").arg(id)
            .arg(p[0],0,'g',9).arg(p[1],0,'g',9).arg(p[2],0,'g',9), list);
        item->setData(Qt::UserRole, id); item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(current.contains(id) ? Qt::Checked : Qt::Unchecked);
    }
    QObject::connect(filter, &QLineEdit::textChanged, list, [list](const QString& text) {
        for (int i=0; i<list->count(); ++i) list->item(i)->setHidden(!list->item(i)->text().contains(text, Qt::CaseInsensitive));
    });
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec()!=QDialog::Accepted) return false;
    selected={};
    for (int i=0; i<list->count(); ++i) if (list->item(i)->checkState()==Qt::Checked) selected.append(list->item(i)->data(Qt::UserRole).toInt());
    mesh.insert("nodeIds", nodeIds); return true;
}

bool selectFlexInterfaceGroups(QWidget* parent, const NativeMotionDocument& document,
                               int bodyId, QJsonArray& selected)
{
    QDialog dialog(parent); dialog.setWindowTitle(QStringLiteral("独立接口的运动副分组")); dialog.resize(760, 520);
    auto* layout = new QVBoxLayout(&dialog);
    auto* help = new QLabel(QStringLiteral("同一组的运动副连接同一个刚性接口片；不同组由柔性实体传力。每个副最多属于一组。选择自动时，按该目标体各运动副的作用点位置建立接口。"));
    help->setWordWrap(true); layout->addWidget(help);
    auto* automatic = new QCheckBox(QStringLiteral("按运动副位置自动分组")); automatic->setObjectName("nativeInterfaceGroupsAutomatic");
    automatic->setChecked(selected.isEmpty()); layout->addWidget(automatic);
    auto* editor = new QWidget; auto* horizontal = new QHBoxLayout(editor); horizontal->setContentsMargins(0,0,0,0);
    auto* groups = new QListWidget; groups->setObjectName("nativeInterfaceGroups"); horizontal->addWidget(groups, 1);
    auto* joints = new QListWidget; joints->setObjectName("nativeInterfaceGroupJoints"); horizontal->addWidget(joints, 2);
    layout->addWidget(editor, 1);
    auto values = std::make_shared<QVector<QSet<int>>>();
    for (const auto& group : selected) { QSet<int> ids; for (const auto& id : group.toArray()) ids.insert(id.toInt()); values->append(ids); }
    auto* buttonsRow = new QHBoxLayout;
    auto* add = new QPushButton(QStringLiteral("新增接口组")); add->setObjectName("nativeInterfaceGroupAdd");
    auto* remove = new QPushButton(QStringLiteral("删除接口组")); remove->setObjectName("nativeInterfaceGroupRemove");
    buttonsRow->addWidget(add); buttonsRow->addWidget(remove); layout->addLayout(buttonsRow);
    auto* status = new QLabel; status->setWordWrap(true); layout->addWidget(status);
    QSet<int> available;
    for (const auto& value : document.collection("joints")) {
        const auto joint = value.toObject();
        if (joint.value("actionBodyId").toInt(-1) != bodyId && joint.value("baseBodyId").toInt(-1) != bodyId) continue;
        const int id = joint.value("id").toInt(); available.insert(id);
        auto* item = new QListWidgetItem(QStringLiteral("%1 [%2] · %3").arg(joint.value("name").toString()).arg(id).arg(enumLabel(joint.value("jointType").toString())),joints);
        item->setData(Qt::UserRole,id); item->setFlags(item->flags() | Qt::ItemIsUserCheckable); item->setCheckState(Qt::Unchecked);
    }
    const auto populate = [=] {
        const QSignalBlocker block(joints);
        const int current = groups->currentRow();
        for (int i = 0; i < joints->count(); ++i) {
            auto* item = joints->item(i); const int id = item->data(Qt::UserRole).toInt();
            bool used = false;
            for (int g = 0; g < values->size(); ++g) if (g != current && values->at(g).contains(id)) used = true;
            item->setFlags(used || current < 0 ? item->flags() & ~Qt::ItemIsEnabled : item->flags() | Qt::ItemIsEnabled);
            item->setCheckState(current >= 0 && current < values->size() && values->at(current).contains(id) ? Qt::Checked : Qt::Unchecked);
        }
    };
    const auto refresh = [=](int selectedRow) {
        const QSignalBlocker block(groups); groups->clear();
        for (int g = 0; g < values->size(); ++g) groups->addItem(QStringLiteral("接口 %1 · %2 个副").arg(g+1).arg(values->at(g).size()));
        groups->setCurrentRow(std::clamp(selectedRow, -1, values->size()-1)); populate();
    };
    QObject::connect(groups, &QListWidget::currentRowChanged, &dialog, [populate] { populate(); });
    QObject::connect(joints, &QListWidget::itemChanged, &dialog, [=](QListWidgetItem* item) {
        const int current = groups->currentRow(); if (current < 0 || current >= values->size()) return;
        const int id = item->data(Qt::UserRole).toInt();
        if (item->checkState() == Qt::Checked) (*values)[current].insert(id); else (*values)[current].remove(id);
        refresh(current);
    });
    QObject::connect(add, &QPushButton::clicked, &dialog, [=] { values->append(QSet<int>{}); refresh(values->size()-1); });
    QObject::connect(remove, &QPushButton::clicked, &dialog, [=] { const int current = groups->currentRow(); if (current >= 0) values->removeAt(current); refresh(current-1); });
    const auto mode = [=] { editor->setEnabled(!automatic->isChecked()); add->setEnabled(!automatic->isChecked()); remove->setEnabled(!automatic->isChecked()); };
    QObject::connect(automatic, &QCheckBox::toggled, &dialog, [mode] { mode(); });
    refresh(values->isEmpty() ? -1 : 0); mode();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [=, &dialog] {
        if (!automatic->isChecked()) {
            if (values->size() < 2) { status->setText(QStringLiteral("独立接口需要至少两个组；也可选择自动分组。")); return; }
            QSet<int> used;
            for (const auto& ids : *values) {
                if (ids.isEmpty()) { status->setText(QStringLiteral("每个显式接口组至少选择一个运动副。")); return; }
                for (int id : ids) {
                    if (!available.contains(id) || used.contains(id)) { status->setText(QStringLiteral("接口组包含缺失、未连接目标体或重复分组的运动副，请重新选择。")); return; }
                    used.insert(id);
                }
            }
            if (used != available) { status->setText(QStringLiteral("请把连接该柔性体的所有运动副都分配到接口组，避免遗漏连接。")); return; }
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return false;
    selected = {};
    if (!automatic->isChecked()) for (const auto& ids : *values) {
        auto sorted = ids.values(); std::sort(sorted.begin(), sorted.end()); QJsonArray group;
        for (int id : sorted) group.append(id); selected.append(group);
    }
    return true;
}

} // namespace

void MotionWindow::snapshotNativeFlexMeshes()
{
    if (!m_model.nativeDocument) return;
    auto document=*m_model.nativeDocument; bool changed=false;
    for (const auto& value : document.collection("autoFlexBodies")) {
        auto flex=value.toObject();
        if (!flex.value("meshFile").toString().isEmpty() || !flex.value("mesh").toObject().isEmpty()) continue;
        const auto mesh=collectFlexMesh(document.bodyPartBindings.value(flex.value("targetBodyId").toInt(-1)), m_meshes);
        if (mesh.isEmpty()) continue;
        flex.insert("mesh", mesh); put(document, "autoFlexBodies", flex); changed=true;
    }
    if (changed) {
        m_model.nativeDocument=std::make_shared<NativeMotionDocument>(std::move(document));
        setProjectModified(true);
    }
}

void MotionWindow::restoreNativeFlexMeshes()
{
    if (!m_model.nativeDocument) return;
    const auto& document=*m_model.nativeDocument;
    for (const auto& value : document.collection("autoFlexBodies")) {
        const auto flex=value.toObject();
        restoreFlexMesh(flex.value("mesh").toObject(), document.bodyPartBindings.value(flex.value("targetBodyId").toInt(-1)), m_meshes);
    }
}

void MotionWindow::updateNativeFlexMeshForPart(int partId)
{
    if (!m_model.nativeDocument) return;
    auto document=*m_model.nativeDocument; bool changed=false;
    for (const auto& value : document.collection("autoFlexBodies")) {
        auto flex=value.toObject();
        const auto parts=document.bodyPartBindings.value(flex.value("targetBodyId").toInt(-1));
        if (!parts.contains(partId) || !flex.value("meshFile").toString().isEmpty()) continue;
        const auto old=flex.value("mesh").toObject();
        auto mesh=collectFlexMesh(parts,m_meshes);
        if (old.value("nodes")==mesh.value("nodes") && old.value("tetrahedra")==mesh.value("tetrahedra")) continue;
        if (!flex.value("attachGroups").toArray().isEmpty() || !flex.value("attachPoints").toArray().isEmpty())
            flex.insert("attachmentMeshChanged",true);
        if (mesh.isEmpty()) flex.remove("mesh"); else flex.insert("mesh",mesh);
        flex.remove("meshSourceFile");
        put(document,"autoFlexBodies",flex); changed=true;
    }
    if (changed) {
        m_model.nativeDocument=std::make_shared<NativeMotionDocument>(std::move(document)); setProjectModified(true);
        clearResults(); updateTree();
    }
}

void MotionWindow::ensureNativeFlexMesh(int flexId)
{
    if (!m_model.nativeDocument || m_jobPart>=0 || motionBusy()) return;
    auto document=*m_model.nativeDocument;
    auto flex=document.entity("autoFlexBodies", flexId);
    if (flex.isEmpty()) return;
    const auto partIds=document.bodyPartBindings.value(flex.value("targetBodyId").toInt(-1));
    auto mesh=flex.value("mesh").toObject();
    if (!flex.value("meshFile").toString().isEmpty()) {
        const auto previousMesh=mesh;
        FemMesh imported; QString error;
        const auto path=QDir(document.baseDirectory).absoluteFilePath(flex.value("meshFile").toString());
        if (!readNetgenMesh(path, MeshKind::Volume, imported, error)) { setNotice(error, true); return; }
        mesh=femMeshJson(imported);
        const bool changed=!previousMesh.isEmpty() && (previousMesh.value("nodes")!=mesh.value("nodes")
            || previousMesh.value("tetrahedra")!=mesh.value("tetrahedra"));
        if (changed && (!flex.value("attachGroups").toArray().isEmpty() || !flex.value("attachPoints").toArray().isEmpty()))
            flex.insert("attachmentMeshChanged",true);
        // Identical nodes/connectivity retain their known global-ID mapping.
        // A first external mesh has no previous numbering to invalidate.
        if (!changed && previousMesh.contains("nodeIds")) mesh.insert("nodeIds",previousMesh.value("nodeIds"));
        // A successful import becomes self-contained, as does an automatically
        // generated mesh. Keep the original filename as provenance only.
        flex.insert("meshSourceFile", path); flex.insert("meshFile", "");
    }
    if (mesh.isEmpty()) {
        if (partIds.isEmpty()) { setNotice(tr("目标体未绑定 CAD 实体，无法生成体网格。"), true); return; }
        for (int partId : partIds) {
            const auto existing=m_meshes.find(partId);
            if (existing!=m_meshes.end() && !existing->second.tetrahedra.empty()) continue;
            m_meshCompletion=[this, flexId](bool success) { if (success) ensureNativeFlexMesh(flexId); };
            if (!beginPartMesh(partId, flex.value("elementSize").toDouble(5.0), MeshKind::Volume)) m_meshCompletion={};
            return;
        }
        mesh=collectFlexMesh(partIds, m_meshes);
    }
    FemMesh checked; QString error;
    if (!readFemMeshJson(mesh, checked, error)) { setNotice(error, true); return; }
    flex.insert("mesh", mesh); put(document, "autoFlexBodies", flex);
    if (!commitNativeDocument(std::move(document), {ModelNode::Native, flexId, "autoFlexBodies"})) return;
    restoreNativeFlexMeshes();
    command(Command::MeshDisplay)->setEnabled(true); command(Command::MeshDisplay)->trigger();
    syncViewport();
    if (m_projectActive) editNativeEntity("autoFlexBodies",flexId);
    else m_restoreProjectEditor = true;
    setNotice(tr("自动柔性体已保存：%1").arg(flexMeshSummary(mesh)));
}

void MotionWindow::createNativeCommands()
{
    auto add = [this](Command command, const QString& text, const char* icon, const std::function<void()>& action) {
        auto* item = new QAction(QIcon(QStringLiteral(":/mMainWidgets/Icon/Motion/") + QLatin1String(icon)), text, this);
        m_commands.emplace(command, item); connect(item, &QAction::triggered, this, action);
    };
    add(Command::ImportMotionJson, tr("导入运动 JSON"), "movingBody.png", [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("导入运动数据"), {}, tr("Motion 运动数据 (*.motion.json *.json)"));
        if (!path.isEmpty()) startMotionJsonImport(path);
    });
    add(Command::ExportMotionJson, tr("导出运动 JSON"), "movingBody.png", [this] {
        if (!ensureNativeModel()) return;
        if(m_original)m_original->flush();
        snapshotNativeFlexMeshes();
        if(!m_original)for (const auto& scenario : m_model.scenarios) syncNativeScenarioCurves(m_model, scenario.id);
        auto path = QFileDialog::getSaveFileName(this, tr("导出运动数据"), "model.motion.json", tr("Motion 运动数据 (*.motion.json)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".motion.json";
        QString error;
        if (writeNativeMotionExchange(path, m_document, *m_model.nativeDocument, error))
            setNotice(tr("已导出运动数据及配套几何：%1").arg(QFileInfo(path).fileName()));
        else setNotice(error, true);
    });
    struct Entry { Command command; const char* collection; const char* icon; };
    for (const auto& entry : {Entry{Command::AddNativeJoint, "joints", "joint_universal.png"},
            {Command::AddCoupling, "couplings", "joint_coupling.png"}, {Command::AddSpring, "springs", "force_spring.png"},
            {Command::AddVectorForce, "vectorForces", "force_vector.png"}, {Command::AddScalarForce, "scalarForces", "force_vector.png"},
            {Command::AddScalarTorque, "scalarTorques", "force_torque.png"}, {Command::AddThreePointForce, "threePointForces", "force_vector.png"},
            {Command::AddThreePointTorque, "threePointTorques", "force_torque.png"}, {Command::AddContact, "contacts", "contact.png"},
            {Command::AddMarker, "markers", "movingBody.png"}, {Command::AddSensor, "sensors", "sensor_displacement.png"},
            {Command::AddController, "controllers", "controlCalculation.png"}, {Command::AddControlInput, "controlInputs", "controlInput.png"},
            {Command::AddAutoFlex, "autoFlexBodies", "mnf.png"}, {Command::AddSoftStop, "softStops", "force_spring.png"},
            {Command::AddTensionCable, "tensionCables", "force_spring.png"}}) {
        const QString collection = QLatin1String(entry.collection);
        add(entry.command, entry.command == Command::AddNativeJoint ? tr("更多运动副") : label(collection), entry.icon,
            [this, collection] { if (ensureNativeModel()) editNativeEntity(collection); });
    }
}

bool MotionWindow::ensureNativeModel()
{
    if (m_model.nativeDocument) return true;
    NativeMotionDocument document;
    document.root = {{"formatVersion", 1}, {"exportTime", QDateTime::currentDateTime().toString(Qt::ISODate)}, {"motionData", QJsonObject{}}};
    QString error;
    for (const auto& body : m_model.bodies) {
        const auto row = bodyRecord(m_document, body, error);
        if (row.isEmpty()) { setNotice(error, true); return false; }
        put(document, "bodies", row); document.bodyPartBindings.insert(body.id, {body.partId});
    }
    for (const auto& joint : m_model.joints) {
        const auto& p = joint.originMm; const auto& a = joint.axis;
        put(document, "joints", {{"id", joint.id}, {"name", joint.name},
            {"jointType", joint.kind == JointKind::Revolute ? "Revolute" : joint.kind == JointKind::Prismatic ? "Translational" : "Fixed"},
            {"actionBodyId", joint.movingBodyId}, {"baseBodyId", joint.baseBodyId == 0 ? -1 : joint.baseBodyId},
            {"origin", vec(p[0], p[1], p[2])}, {"axis", vec(a[0], a[1], a[2])}});
    }
    for (const auto& drive : m_model.drives) {
        const auto joint = document.entity("joints", drive.jointId);
        QJsonObject row{{"id", drive.id}, {"name", drive.name}, {"driveType", "JointDrive"}, {"targetType", "Joint"},
            {"targetJointId", drive.jointId}, {"motionType", joint.value("jointType").toString() == "Translational" ? "Translation" : "Rotation"},
            {"controlMode", drive.kind == DriveKind::Speed ? "Velocity" : "Position"},
            {"expression", QString::number(drive.value, 'g', 17)}};
        if (drive.kind == DriveKind::Displacement) {
            const double end = m_model.scenarios.empty() ? 1.0 : m_model.scenarios.front().duration;
            row.insert("legacyEndValue", drive.value);
            row.insert("expression", QStringLiteral("(%1)*t/(%2)").arg(drive.value, 0, 'g', 17).arg(end, 0, 'g', 17));
        }
        put(document, "drives", row);
    }
    for (const auto& scenario : m_model.scenarios) {
        auto row = templateRecord(document, "scenarios", {}); const auto& g = scenario.gravityMS2;
        row.insert("id", scenario.id); row.insert("name", scenario.name); row.insert("endTime", scenario.duration); row.insert("stepSize", scenario.step);
        row.insert("gravity", vec(g[0] * 1000.0, g[1] * 1000.0, g[2] * 1000.0)); put(document, "scenarios", row);
    }
    return commitNativeDocument(std::move(document), {});
}

namespace {
MxFunctions::PhysicalFieldType originalTreeType(const QString& collection) {
    using namespace MxFunctions;
    static const QMap<QString,PhysicalFieldType> types{
        {"bodies",Physical_MotionBody},{"joints",Physical_MotionJoint},{"couplings",Physical_MotionCoupling},
        {"drives",Physical_MotionDrive},{"springs",Physical_MotionSpring},{"forceElements",Physical_MotionVectorForce},
        {"vectorForces",Physical_MotionVectorForce},{"scalarForces",Physical_MotionVectorForce},
        {"scalarTorques",Physical_MotionVectorForce},{"threePointForces",Physical_MotionVectorForce},
        {"threePointTorques",Physical_MotionVectorForce},{"contacts",Physical_MotionContact},
        {"markers",Physical_MotionMarker},{"sensors",Physical_MotionSensor},{"controllers",Physical_MotionController},
        {"controlInputs",Physical_MotionControlInput},{"scenarios",Physical_MotionScenario},{"autoFlexBodies",Physical_MotionAutoFlex},
        {"electroImports",Physical_MotionElectroImport},{"electroExports",Physical_MotionElectroExport},
        {"hydraulics",Physical_MotionHydraulic},{"rlTasks",Physical_MotionRL}};
    return types.value(collection,Physical_None);
}
QStringList originalTreeCollections(const QString& group) {
    if(group=="forceElements")return {"vectorForces","scalarForces","scalarTorques","threePointForces","threePointTorques","tensionCables","softStops"};
    if(group=="__motionRoot")return {"bodies","joints","couplings","drives","bodyDrives","springs","vectorForces","scalarForces","scalarTorques",
        "threePointForces","threePointTorques","tensionCables","softStops","contacts","markers","sensors","controllers",
        "controlInputs","scenarios","autoFlexBodies","electroImports","electroExports","hydraulics","rlTasks"};
    return {group};
}
QString originalCurveCollection(const QString& source) {
    static const QMap<QString,QString> aliases{{"Body","bodies"},{"Joint","joints"},{"Marker","markers"},
        {"Drive","drives"},{"BodyDrive","bodyDrives"},{"Coupling","couplings"},{"Sensor","sensors"},{"Controller","controllers"},
        {"Spring","springs"},{"TensionCable","tensionCables"},{"SoftStop","softStops"},
        {"VectorForce","vectorForces"},{"ScalarForce","scalarForces"},{"ScalarTorque","scalarTorques"},
        {"ThreePointForce","threePointForces"},{"ThreePointTorque","threePointTorques"},{"AutoFlexBody","autoFlexBodies"}};
    return aliases.value(source,source);
}
struct OriginalTreeCreation { const char* name;const char* icon;const char* action;const char* type=""; };
QVector<OriginalTreeCreation> originalTreeCreations(const QString& collection) {
    if(collection=="joints")return {
        {"旋转副","Motion/joint_revolute.png","createMotionJoint","Revolute"},
        {"移动副","Motion/joint_prismatic.png","createMotionJoint","Translational"},
        {"柱面副","Motion/joint_cylindrical.png","createMotionJoint","Cylindrical"},
        {"球面副","Motion/joint_spherical.png","createMotionJoint","Spherical"},
        {"固定副","Motion/joint_fixed.png","createMotionJoint","Fixed"},
        {"万向节","Motion/joint_universal.png","createMotionJoint","Universal"},
        {"点面副","Motion/joint_pointPlane.png","createMotionJoint","PointPlane"},
        {"平行约束","Motion/joint_parallel.png","createMotionJoint","Parallel"},
        {"垂直约束","Motion/joint_perpendicular.png","createMotionJoint","Perpendicular"},
        {"方向约束","Motion/joint_direction.png","createMotionJoint","Direction"},
        {"恒速副","Motion/joint_cv.png","createMotionJoint","ConstantVelocity"},
        {"螺纹副","Motion/joint_screw.png","createMotionJoint","Screw"},
        {"平面副","Motion/joint_planar.png","createMotionJoint","Planar"},
        {"点线副","Motion/joint_pointLine.png","createMotionJoint","PointLine"},
        {"点在线上副","Motion/joint_pointOnCurve.png","createMotionJoint","PointOnCurve"},
        {"点在面上副","Motion/joint_pointOnSurface.png","createMotionJoint","PointOnSurface"},
        {"线在线上副","Motion/joint_curveOnCurve.png","createMotionJoint","CurveOnCurve"}};
    if(collection=="couplings")return {
        {"齿轮齿条副","Motion/joint_gear.png","createMotionCoupling","GearRack"},
        {"齿轮副","Motion/joint_gear.png","createMotionCoupling","Gear"},
        {"线缆副","Motion/coupling_cable.png","createMotionCoupling","Cable"}};
    if(collection=="sensors")return {
        {"体位移传感器","Motion/sensor_displacement.png","createMotionSensor","BodyDisplacement"},
        {"体速度传感器","Motion/sensor_velocity.png","createMotionSensor","BodyVelocity"},
        {"体加速度传感器","Motion/sensor_acceleration.png","createMotionSensor","BodyAcceleration"},
        {"关节反力传感器","Motion/sensor_jointReaction.png","createMotionSensor","JointReaction"},
        {"驱动输出传感器","Motion/sensor_driveActuation.png","createMotionSensor","DriveActuation"},
        {"距离传感器","Motion/sensor_distance.png","createMotionSensor","Distance"},
        {"角度传感器","Motion/sensor_angle.png","createMotionSensor","Angle"}};
    if(collection=="springs")return {
        {"拉压弹簧","Motion/tcSpring.png","createMotionSpring","Translational"},
        {"扭转弹簧","Motion/torsionSpring.png","createMotionSpring","Torsional"},
        {"体点弹簧","Motion/force_spring.png","createMotionSpring","BodyPoint"}};
    if(collection=="forceElements")return {
        {"矢量力","Motion/force_vector.png","createMotionVectorForce","Force"},
        {"矢量力矩","Motion/force_torque.png","createMotionVectorForce","Torque"},
        {"三点力","Motion/force_tri.png","createMotionVectorForce","TriForce"},
        {"三点力矩","Motion/force_tri_torque.png","createMotionVectorForce","TriTorque"},
        {"标量力","Motion/force.png","createMotionVectorForce","ScalarForce"},
        {"标量力矩","Motion/moment.png","createMotionVectorForce","ScalarTorque"},
        {"衬套","Motion/bush.png","createMotionVectorForce","Bushing"}};
    if(collection=="drives")return {{"运动副驱动","Motion/rotaryJoint.png","createMotionDrive","Velocity"}};
#if defined(MX_HAS_BODY_DRIVES)
    if(collection=="bodyDrives")return {
        {"平移位移","Motion/drive.png","createBodyDrive","LinearPosition"},
        {"线速度","Motion/drive.png","createBodyDrive","LinearSpeed"},
        {"转角","Motion/drive.png","createBodyDrive","RotationAngle"},
        {"角速度","Motion/drive.png","createBodyDrive","RotationSpeed"},
        {"六自由度位姿","Motion/drive.png","createBodyDrive","Pose"}};
#endif
    static const QMap<QString,OriginalTreeCreation> simple{
        {"bodies",{"新建运动体","BrowserTree/xinjian-23x.png","createMotionBody"}},
        {"markers",{"新建标记点","caotu/dian.png","createMotionMarker"}},
        {"controllers",{"新建控制器","BrowserTree/xinjian-23x.png","createMotionController"}},
        {"controlInputs",{"新建控制输入","BrowserTree/xinjian-23x.png","createControlInput"}},
        {"contacts",{"新建 3D 接触","","createMotionContact"}},
        {"autoFlexBodies",{"新建自动柔性体","BrowserTree/xinjian-23x.png","createMotionAutoFlex"}},
        {"scenarios",{"新建求解方案","BrowserTree/motion_solveParentItem.png","createMotionScenario"}},
        {"electroImports",{"创建机电导入","BrowserTree/xinjian-23x.png","createElectroImport"}},
        {"electroExports",{"创建机电导出","BrowserTree/xinjian-23x.png","createElectroExport"}},
        {"hydraulics",{"创建液压协同","BrowserTree/xinjian-23x.png","createHydraulic"}},
        {"rlTasks",{"创建强化学习","BrowserTree/xinjian-23x.png","createRLPanel"}}};
    return simple.contains(collection)?QVector<OriginalTreeCreation>{simple.value(collection)}:QVector<OriginalTreeCreation>{};
}
}
bool MotionWindow::renameOriginalDynamicTreeItem(QTreeWidgetItem* item) {
    if(!item || !item->data(0,Qt::UserRole+4).toBool())return false;
    const auto collection=item->data(0,Qt::UserRole+2).toString();const auto type=originalTreeType(collection);
    if(type==MxFunctions::Physical_None)return false;
    const int id=item->data(0,Qt::UserRole).toInt();const QString proposed=item->text(0).trimmed();
    ensureOriginalMotionContext();if(!m_original)return true;
    const auto previous=m_original->model()->getNameByPhyID(type,id);
    if(previous==proposed)return true;
    if(motionBusy()||m_jobPart>=0||proposed.isEmpty()||m_original->model()->isExistName(type,proposed)) {
        const QSignalBlocker block(m_tree);item->setText(0,previous);
        if(proposed.isEmpty()||m_original->model()->isExistName(type,proposed))setNotice(tr("名称不能为空或与现有对象重名。"),true);
        return true;
    }
    closeEditor();m_original->model()->resetName(type,previous,proposed);m_original->flush();return true;
}
void MotionWindow::deleteOriginalTreeEntities(const QString& collection,int id) {
    if(motionBusy()||m_jobPart>=0||!m_model.nativeDocument)return;
    closeEditor();ensureOriginalMotionContext();if(!m_original)return;
    const auto before=m_original->document();
    QMap<QString,QSet<int>> removed;
    for(const auto& name:originalTreeCollections(collection))
        for(const auto& value:before.collection(name)) {
            const int candidate=value.toObject().value("id").toInt(-1);
            if(id<0||candidate==id)removed[name].insert(candidate);
        }
    if(removed.isEmpty())return;
    // Check the exact removed records before touching typed managers. Removing
    // an incoming relationship must not silently turn a formerly moving body
    // into the ground body (-1). Existing incomplete drafts may still be deleted.
    auto proposedModel=m_model;
    for(auto it=removed.begin();it!=removed.end();++it)
        for(int value:it.value())removeNativeEntityCurves(proposedModel,it.key(),value);
    auto proposed=*proposedModel.nativeDocument;auto data=proposed.root.value("motionData").toObject();
    for(auto it=removed.begin();it!=removed.end();++it) {
        QJsonArray rows;for(const auto& value:proposed.collection(it.key()))
            if(!it.value().contains(value.toObject().value("id").toInt(-1)))rows.append(value);
        data.insert(it.key(),rows);
        if(it.key()=="bodies")for(int value:it.value())proposed.bodyPartBindings.remove(value);
    }
    auto referencesRemoved=[&](const QJsonObject& request) {
        return removed.value(originalCurveCollection(request.value("sourceType").toString()))
            .contains(request.value("sourceId").toInt(-1));
    };
    auto scenarios=data.value("scenarios").toArray();
    for(int i=0;i<scenarios.size();++i) {
        auto row=scenarios[i].toObject();
        if(referencesRemoved(row.value("currentCurveRequest").toObject()))row.insert("currentCurveRequest",QJsonObject{});
        if(row.contains("trackedMarkerIds")) {
            QJsonArray markers;for(const auto& value:row.value("trackedMarkerIds").toArray())
                if(!removed.value("markers").contains(value.toInt(-1)))markers.append(value);
            row.insert("trackedMarkerIds",markers);
        }
        scenarios[i]=row;
    }
    if(data.contains("scenarios"))data.insert("scenarios",scenarios);
    proposed.root.insert("motionData",data);
    QStringList oldIssues,newIssues;validateNativeMotionDocument(before,oldIssues);validateNativeMotionDocument(proposed,newIssues);
    for(const auto& issue:newIssues)if(!oldIssues.contains(issue)) { setNotice(tr("无法删除：%1").arg(issue),true);return; }
    auto* model=m_original->model();
    auto& scenarioManager=model->getManager<MechMotionData::mMotionScenarioManager>(MxFunctions::Physical_MotionScenario);
    for(const auto& value:scenarios) {
        const auto row=value.toObject();const int scenarioId=row.value("id").toInt();auto* scenario=scenarioManager.getData(scenarioId);
        if(!scenario)continue;
        QSet<int> keep;for(const auto& request:row.value("curveRequests").toArray())keep.insert(request.toObject().value("requestId").toInt(-1));
        const auto requests=scenario->getCurveRequests();
        for(const auto& request:requests)if(!keep.contains(request.m_requestId))scenario->removeCurveRequest(request.m_requestId);
        if(referencesRemoved(before.entity("scenarios",scenarioId).value("currentCurveRequest").toObject()))scenario->clearCurrentCurveRequest();
        for(int marker:removed.value("markers"))scenario->removeTrackedMarkerId(marker);
    }
    for(auto it=removed.begin();it!=removed.end();++it) {
        const auto type=originalTreeType(it.key());
        if(type!=MxFunctions::Physical_None)for(int value:it.value())model->deleteByPhysicalType(type,value);
    }
    m_selectionNode={};m_original->flush();
    // Native-only force elements share the original force tree but deliberately
    // have no fake old data class. Apply only their actual collection deletions.
    auto result=m_original->document();auto resultData=result.root.value("motionData").toObject();
    for(auto it=removed.begin();it!=removed.end();++it)
        if(originalTreeType(it.key())==MxFunctions::Physical_None)resultData.insert(it.key(),data.value(it.key()));
    for(int i=0;i<scenarios.size();++i) {
        const auto clean=scenarios[i].toObject();auto rows=resultData.value("scenarios").toArray();
        for(int j=0;j<rows.size();++j)if(rows[j].toObject().value("id")==clean.value("id")) {
            auto row=rows[j].toObject();
            for(const auto* field:{"curveRequests","currentCurveRequest","trackedMarkerIds"})
                if(clean.contains(field))row.insert(field,clean.value(field));
            rows[j]=row;
        }
        resultData.insert("scenarios",rows);
    }
    result.root.insert("motionData",resultData);m_original->adoptExternalDocument(result);commitOriginalMotionDocument(result);
}
bool MotionWindow::showOriginalDynamicTreeMenu(QTreeWidgetItem* item,const QPoint& position) {
    if(!item)return false;const auto collection=item->data(0,Qt::UserRole+2).toString();
    const auto type=originalTreeType(collection);const bool root=collection=="__motionRoot";
    const bool group=item->data(0,Qt::UserRole+3).toBool();const int id=item->data(0,Qt::UserRole).toInt();
    if(type==MxFunctions::Physical_None&&!root&&!(group&&collection=="bodyDrives"))return false;
    const bool idle=!motionBusy()&&m_jobPart<0;
    mUiMenu menu(this);menu.setObjectName("originalMotionTreeMenu");
    const auto action=[&](QMenu* target,const char* artwork,const QString& text,const std::function<void()>& callback) {
        auto* entry=target->addAction(*artwork?TreeIcons::resource(artwork):QIcon{},text);entry->setEnabled(idle);
        connect(entry,&QAction::triggered,this,callback);return entry;
    };
    const auto create=[&](QMenu* target,const OriginalTreeCreation& entry,const QString& prefix=QString{}) {
        auto* result=action(target,entry.icon,prefix+QString::fromUtf8(entry.name),[this,entry] {
            dispatchOriginalMotionAction(QString::fromLatin1(entry.action),QString::fromLatin1(entry.type));
        });
        result->setProperty("mxOriginalAction",entry.action);result->setProperty("mxOriginalArgument",entry.type);
    };
    const auto submenu=[&](const QString& name,const QString& text,const QIcon& icon,bool rootMenu=false) {
        auto* nested=menu.addMenu(icon,text);nested->setEnabled(idle);nested->setObjectName("originalMotionTreeCreate_"+name);
        auto entries=originalTreeCreations(name);
        if(rootMenu&&name=="couplings") { entries.resize(2);entries[0].icon="Motion/coupling_gearRack.png"; }
        if(rootMenu&&name=="forceElements") { const auto bush=entries.takeLast();entries.insert(2,bush); }
        for(const auto& entry:entries) {
            if(rootMenu&&name=="forceElements"&&QString::fromLatin1(entry.type)=="ScalarForce")nested->addSeparator();
            create(nested,entry);
        }
        MechMainWindow::ThemeManager::applyMenuStyle(nested);
    };
    if(root) {
        create(&menu,{"新建运动体","BrowserTree/motion_movingBodyParentItem.png","createMotionBody"});
        submenu("joints",tr("新建运动副"),TreeIcons::item("joints"),true);
        submenu("couplings",tr("新建耦合副"),TreeIcons::item("couplings"),true);
        create(&menu,{"新建驱动","BrowserTree/motion_driveParentItem.png","createMotionDrive","Velocity"});
#if defined(MX_HAS_BODY_DRIVES)
        submenu("bodyDrives",tr("新建运动体驱动"),TreeIcons::item("bodyDrives"),true);
#endif
        create(&menu,originalTreeCreations("markers").first());
        submenu("sensors",tr("新建传感器"),TreeIcons::item("sensors"),true);
        create(&menu,{"新建控制器","Motion/controlCalculation.png","createMotionController"});
        submenu("forceElements",tr("新建力元"),TreeIcons::item("forceElements"),true);
        submenu("springs",tr("新建弹簧"),TreeIcons::item("springs"),true);
        create(&menu,originalTreeCreations("scenarios").first());menu.addSeparator();
        action(&menu,"BrowserTree/shanchu.png",tr("删除运动数据"),[this] {
            const int answer=MechMainWindow::mFramelessMessageBox::warning(this,tr("警告"),
                tr("将删除多体动力学下的全部运动数据（含求解方案及其结果），部件/几何/网格保留。\n是否继续？"),tr("是"),tr("否"));
            if(answer==0)deleteOriginalTreeEntities("__motionRoot",-1);
        });
    } else if(group) {
        if(!originalTreeCreations(collection).isEmpty()
            && QStringList{"joints","couplings","drives","sensors","bodyDrives"}.contains(collection))
            submenu(collection,tr("新建%1").arg(label(collection)),TreeIcons::item(collection));
        else {
            const bool multiple=collection=="springs"||collection=="forceElements";
            const auto entries=originalTreeCreations(collection);
            for(int i=0;i<entries.size();++i) {
                if(collection=="forceElements"&&i==2)menu.addSeparator();
                create(&menu,entries[i],multiple?tr("新建"):QString{});
            }
            if(multiple||collection=="contacts")menu.addSeparator();
        }
        const auto groupName=collection=="forceElements"?tr("力元"):label(collection);
        action(&menu,"BrowserTree/shanchu.png",tr("删除所有%1").arg(groupName),[this,collection] { deleteOriginalTreeEntities(collection,-1); });
    } else {
        action(&menu,"BrowserTree/shanchu.png",tr("删除"),[this,collection,id] { deleteOriginalTreeEntities(collection,id); });
        action(&menu,"BrowserTree/chongminmin.png",tr("重命名"),[this,item] { m_tree->editItem(item,0); });
        if(collection=="scenarios")action(&menu,"BrowserTree/fuzhi.png",tr("求解"),[this,id] {
            editOriginalMotionEntity("scenarios",id,{});
            if(m_editor)if(auto* panel=m_editor->findChild<MechMainWindow::mMotionScenarioPanel*>())panel->onStartSolve(id);
        });
        else action(&menu,"BrowserTree/fuzhi.png",tr("复制"),[this,type,collection,id] {
            closeEditor();ensureOriginalMotionContext();if(!m_original)return;
            auto extensions=m_original->document().entity(collection,id);if(extensions.isEmpty())return;
            extensions.remove("id");extensions.remove("name");
            const int copy=m_original->model()->copyPhysicalField(type,id);if(copy<0)return;
            m_selectionNode={ModelNode::Native,copy,collection};m_original->flush();
            m_original->setExtensionFields(collection,copy,extensions);editOriginalMotionEntity(collection,copy,{});
        });
    }
    MechMainWindow::ThemeManager::applyMenuStyle(&menu);menu.exec(position);return true;
}

bool MotionWindow::commitNativeDocument(NativeMotionDocument document, ModelNode selection)
{
    QString error;
    if (!validateNativeMotionDocument(document, error) || !validateNativeMotionBindings(m_document, document, error)) {
        setNotice(error, true); return false;
    }
    auto model = m_model;
    model.nativeDocument = std::make_shared<NativeMotionDocument>(std::move(document));
    syncNativeMotionModel(model);
    if (!m_model.nativeDocument) for (auto& scenario : model.scenarios) for (auto& request : scenario.curveRequests) {
        if (request.metric == RigidCurveMetric::JointCoordinate || request.metric == RigidCurveMetric::JointRate) {
            request.variable = request.metric == RigidCurveMetric::JointCoordinate ? QStringLiteral("Coordinate") : QStringLiteral("Rate");
            request.sourceType = QStringLiteral("joints"); request.metric = RigidCurveMetric::NativeChannel;
        }
    }
    commitRigid(std::move(model), selection); return true;
}

bool MotionWindow::resolveNativeBindings(const CadDocument& cad, NativeMotionDocument& document, bool interactive)
{
    QVector<NativeMotionBindingIssue> issues;
    autoBindNativeMotionBodies(cad, document, issues);
    if (!issues.isEmpty() && !interactive) {
        QStringList messages; for (const auto& issue : issues) messages.append(issue.message);
        setNotice(messages.join(QLatin1Char('\n')), true); return false;
    }
    for (const auto& issue : issues) {
        QDialog dialog(this); dialog.setWindowTitle(tr("核对运动体的几何绑定")); dialog.resize(650, 520);
        auto* layout = new QVBoxLayout(&dialog);
        const auto body = document.entity("bodies", issue.bodyId);
        auto* help = new QLabel(tr("%1 [%2]\n%3\n请选择该运动体的全部几何零件。")
            .arg(body.value("name").toString()).arg(issue.bodyId).arg(issue.message)); help->setWordWrap(true); layout->addWidget(help);
        auto* filter = new QLineEdit; filter->setPlaceholderText(tr("搜索几何名称或编号")); layout->addWidget(filter);
        auto* parts = partList(cad, document, issue.bodyId); parts->setMaximumHeight(100000); layout->addWidget(parts, 1);
        connect(filter, &QLineEdit::textChanged, parts, [parts](const QString& text) {
            for (int i = 0; i < parts->count(); ++i) parts->item(i)->setHidden(!parts->item(i)->text().contains(text, Qt::CaseInsensitive)); });
        auto* problem = new QLabel; problem->setWordWrap(true); layout->addWidget(problem);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
            QString error;
            if (setNativeMotionBodyBinding(cad, document, issue.bodyId, selectedParts(parts), error)) dialog.accept();
            else problem->setText(error);
        });
        if (dialog.exec() != QDialog::Accepted) return false;
    }
    QString error;
    if (!validateNativeMotionBindings(cad, document, error)) { setNotice(error, true); return false; }
    QVector<NativeTopologyBindingIssue> topologyIssues; QStringList topologyEvidence;
    autoBindNativeMotionTopology(cad, document, topologyIssues, &topologyEvidence);
    if (!topologyIssues.isEmpty() && !interactive) {
        QStringList messages; for (const auto& issue : topologyIssues) messages.append(issue.message);
        setNotice(messages.join(QLatin1Char('\n')), true); return false;
    }
    for (const auto& issue : topologyIssues) {
        auto joint = document.entity("joints", issue.jointId); auto selection = joint.value(issue.selectorField).toArray();
        if (!selectConstraintTopology(this, cad, document, joint, issue.selectorField, selection, issue.message)) return false;
        joint.insert(issue.selectorField, selection); put(document, "joints", joint);
    }
    if (!validateNativeMotionDocument(document, error)) { setNotice(error, true); return false; }
    return true;
}

bool MotionWindow::importMotionJson(const QString& path, bool interactiveBinding)
{
    if (motionBusy() || m_jobPart >= 0) return false;
    NativeMotionDocument document; QString error;
    if (!readNativeMotionDocument(path, document, error)) { setNotice(error, true); return false; }
    CadDocument exchangeCad;
    bool hasGeometry = false;
    if (!readNativeMotionExchange(path, document, exchangeCad, hasGeometry, error)) {
        setNotice(error, true);
        return false;
    }
    const bool bindingsChecked = hasGeometry;
    if (!hasGeometry) {
        if (m_document.parts().empty()) {
            const auto directory = QFileInfo(path).absoluteDir();
            const auto companions = directory.entryList({"*.stp", "*.step", "*.STP", "*.STEP"}, QDir::Files);
            QString stepPath;
            if (companions.size() == 1) stepPath = directory.filePath(companions.front());
            else if (interactiveBinding)
                stepPath = QFileDialog::getOpenFileName(this, tr("选择与旧版运动 JSON 配套的 STEP"), directory.absolutePath(),
                    tr("STEP 几何 (*.stp *.step)"));
            if (stepPath.isEmpty()) {
                setNotice(tr("这份运动 JSON 没有几何路径，请指定配套 STEP。"), true);
                return false;
            }
            std::vector<int> imported;
            if (!exchangeCad.importFile(stepPath, imported, error)) { setNotice(error, true); return false; }
            hasGeometry = true;
        }
    }
    return installMotionJson(path, std::move(document), std::move(exchangeCad), hasGeometry,
                             bindingsChecked, interactiveBinding);
}

bool MotionWindow::startMotionJsonImport(const QString& path, std::function<void(bool)> finished)
{
    return startMotionJsonImport(path, {}, std::move(finished));
}

bool MotionWindow::startMotionJsonImport(const QString& path, const QString& legacyStep, std::function<void(bool)> finished)
{
    struct Import {
        NativeMotionDocument document;
        CadDocument cad;
        std::shared_ptr<const PreparedCadScene> scene;
        QString error;
        bool hasGeometry = false;
        bool bindingsChecked = false;
        bool needsStep = false;
    };
    auto result = std::make_shared<Import>();
    const bool reuseCad = !m_document.parts().empty();
    return runGeometryImport(path, [path, legacyStep, reuseCad, result] {
        auto& [document, cad, scene, error, hasGeometry, bindingsChecked, needsStep] = *result;
        if (!readNativeMotionDocument(path, document, error) ||
            !readNativeMotionExchange(path, document, cad, hasGeometry, error)) return;
        bindingsChecked = hasGeometry;
        if (!hasGeometry && (!reuseCad || !legacyStep.isEmpty())) {
            const auto directory = QFileInfo(path).absoluteDir();
            const auto companions = directory.entryList({"*.stp", "*.step", "*.STP", "*.STEP"}, QDir::Files);
            const auto step = !legacyStep.isEmpty() ? legacyStep :
                (companions.size() == 1 ? directory.filePath(companions.front()) : QString{});
            if (step.isEmpty()) { needsStep = true; return; }
            std::vector<int> ids;
            if (!cad.importFile(step, ids, error)) return;
            hasGeometry = true;
            auto bound = document;
            QVector<NativeMotionBindingIssue> bodyIssues;
            QVector<NativeTopologyBindingIssue> topologyIssues;
            if (autoBindNativeMotionBodies(cad, bound, bodyIssues) &&
                autoBindNativeMotionTopology(cad, bound, topologyIssues)) {
                document = std::move(bound);
                bindingsChecked = true;
            }
        }
        if (hasGeometry) scene = OccViewport::prepareCadScene(cad);
    }, [this, path, result, finished = std::move(finished)](const QString& failure) {
        const auto error = failure.isEmpty() ? result->error : failure;
        bool success = false;
        if (!error.isEmpty()) setNotice(error, true);
        else if (result->needsStep) {
            const auto pathToStep = QFileDialog::getOpenFileName(this, tr("选择与旧版运动 JSON 配套的 STEP"),
                QFileInfo(path).absolutePath(), tr("STEP 几何 (*.stp *.step)"));
            if (!pathToStep.isEmpty() && startMotionJsonImport(path, pathToStep, finished)) return;
            if (pathToStep.isEmpty()) setNotice(tr("未导入运动 JSON，当前工程保留。"));
        } else {
            success = installMotionJson(path, std::move(result->document), std::move(result->cad),
                result->hasGeometry, result->bindingsChecked, true, result->scene);
        }
        if (finished) finished(success);
        emit geometryImportFinished(success);
    });
}

bool MotionWindow::installMotionJson(const QString& path, NativeMotionDocument&& document, CadDocument&& exchangeCad,
                                     bool hasGeometry, bool bindingsChecked, bool interactiveBinding,
                                     const std::shared_ptr<const PreparedCadScene>& prepared)
{
    if (!bindingsChecked && !resolveNativeBindings(hasGeometry ? exchangeCad : m_document, document, interactiveBinding)) return false;
    QString error;
    const int bodies = document.collection("bodies").size(), joints = document.collection("joints").size();
    RigidModel imported;
    imported.nativeDocument = std::make_shared<NativeMotionDocument>(std::move(document));
    syncNativeMotionModel(imported); // A new source must not inherit old scenario requests with the same ID.
    if (hasGeometry) {
        if (!validateRigidModel(exchangeCad, imported, error)) { setNotice(error, true); return false; }
        if (!interactiveBinding && m_modified) {
            setNotice(tr("当前工程有未保存更改，无法自动替换。"), true);
            return false;
        }
        if (!confirmReplace()) return false;
        closeEditor();
        clearResults();
        m_original.reset();
        m_document = std::move(exchangeCad);
        m_projectPath.clear();
        m_meshes.clear();
        m_hidden.clear();
        m_meshForm->hide();
        command(Command::ShadedEdges)->trigger();
    }
    commitRigid(std::move(imported), {});
    restoreNativeFlexMeshes();
    syncViewport(hasGeometry, prepared);
    if (hasGeometry) for (const auto& note : m_document.importNotes()) m_output->appendPlainText(note);
    setNotice(tr("已导入 %1：%2 个运动体、%3 个运动副。几何绑定已核对，可选择求解方案运行。")
        .arg(QFileInfo(path).fileName()).arg(bodies).arg(joints)); return true;
}

void MotionWindow::appendNativeNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups)
{
    const QMap<QString, QString> groupNames{{"bodies", tr("运动体")}, {"autoFlexBodies", tr("柔性体")},
        {"importedMnfBodies", tr("柔性体")},
        {"joints", tr("运动副")}, {"couplings", tr("耦合副")}, {"drives", tr("驱动")}, {"bodyDrives", tr("运动体驱动")}, {"springs", tr("弹簧")},
        {"vectorForces", tr("力元")}, {"scalarForces", tr("力元")}, {"scalarTorques", tr("力元")},
        {"threePointForces", tr("力元")}, {"threePointTorques", tr("力元")}, {"tensionCables", tr("力元")}, {"softStops", tr("力元")},
        {"sensors", tr("传感器")}, {"controllers", tr("控制器")}, {"markers", tr("标记点")}, {"contacts", tr("接触")}, {"controlInputs", tr("控制输入")}};
    for (auto it = groupNames.begin(); it != groupNames.end(); ++it) {
        const auto rows = m_model.nativeDocument->collection(it.key());
        if (rows.isEmpty()) continue;
        auto* group = groups.at(it.value());
        for (const auto& value : rows) {
            const auto row = value.toObject(); const int id = row.value("id").toInt();
            auto* item = new QTreeWidgetItem(group, {row.value("name").toString()});
            item->setIcon(0, TreeIcons::entity(it.key(), row));
            item->setToolTip(0, label(it.key()) + QStringLiteral(" [%1]").arg(id));
            item->setData(0, Qt::UserRole, id); item->setData(0, Qt::UserRole + 1, int(ModelNode::Native)); item->setData(0, Qt::UserRole + 2, it.key());
            if(originalTreeType(it.key())!=MxFunctions::Physical_None) {
                item->setFlags(item->flags()|Qt::ItemIsEditable);
                item->setData(0,Qt::UserRole+4,true);
            }
            if (m_selectionNode.kind == ModelNode::Native && m_selectionNode.id == id && m_selectionNode.collection == it.key()) m_tree->setCurrentItem(item);
        }
        if (group->childCount()) group->setExpanded(true);
    }
    // These four groups are deliberately lazy in the uploaded mTreeModel /
    // mTreeView. They disappear when their last typed object is deleted.
    for(const auto* collection:{"electroImports","electroExports","hydraulics","rlTasks"}) {
        const auto rows=m_model.nativeDocument->collection(collection);if(rows.isEmpty())continue;
        auto* group=new QTreeWidgetItem(root,{label(collection)});group->setIcon(0,TreeIcons::item(collection));
        group->setData(0,Qt::UserRole,-1);group->setData(0,Qt::UserRole+1,int(ModelNode::None));
        group->setData(0,Qt::UserRole+2,QString::fromLatin1(collection));group->setData(0,Qt::UserRole+3,true);
        for(const auto& value:rows) {
            const auto row=value.toObject();const int id=row.value("id").toInt(-1);
            auto* item=new QTreeWidgetItem(group,{row.value("name").toString()});item->setIcon(0,TreeIcons::entity(collection,row));
            item->setFlags(item->flags()|Qt::ItemIsEditable);item->setToolTip(0,label(collection)+QStringLiteral(" [%1]").arg(id));
            item->setData(0,Qt::UserRole,id);item->setData(0,Qt::UserRole+1,int(ModelNode::Native));
            item->setData(0,Qt::UserRole+2,QString::fromLatin1(collection));item->setData(0,Qt::UserRole+4,true);
            if(m_selectionNode.kind==ModelNode::Native&&m_selectionNode.collection==collection&&m_selectionNode.id==id)m_tree->setCurrentItem(item);
        }
        group->setExpanded(true);
    }
    for (const auto& scenario : m_model.scenarios) {
        auto* item = new QTreeWidgetItem(root, {scenario.name}); item->setData(0, Qt::UserRole, scenario.id);
        item->setIcon(0, TreeIcons::item("scenarios"));
        item->setData(0, Qt::UserRole + 1, int(ModelNode::Scenario)); item->setExpanded(true);
        item->setData(0,Qt::UserRole+2,QStringLiteral("scenarios"));item->setData(0,Qt::UserRole+4,true);
        item->setFlags(item->flags()|Qt::ItemIsEditable);
        if (m_selectionNode.kind == ModelNode::Scenario && m_selectionNode.id == scenario.id) m_tree->setCurrentItem(item);
        for (const auto& child : {std::pair{tr("动画"), ModelNode::Animation}, {tr("结果导出"), ModelNode::Export}, {tr("运动曲线"), ModelNode::Curves}}) {
            auto* leaf = new QTreeWidgetItem(item, {child.first}); leaf->setData(0, Qt::UserRole, scenario.id);
            leaf->setIcon(0, TreeIcons::item(child.second == ModelNode::Animation ? "animation" :
                child.second == ModelNode::Curves ? "curves" : "export"));
            leaf->setData(0, Qt::UserRole + 1, int(child.second)); leaf->setDisabled(!m_results.count(scenario.id));
        }
    }
}

void MotionWindow::batchNativeBodies()
{
    if (motionBusy() || m_jobPart >= 0) return;
    auto document = *m_model.nativeDocument; QString error; int added = 0;
    for (const auto& part : m_document.parts()) {
        if (!canRemovePart(part.id)) continue;
        const int id = nextId(document, "bodies");
        const auto row = bodyRecord(m_document, {id, part.name, part.id, false, 1.0, 0.0}, error, true);
        if (row.isEmpty()) continue;
        put(document, "bodies", row); document.bodyPartBindings.insert(id, {part.id}); ++added;
    }
    if (added) commitNativeDocument(std::move(document), {});
    setNotice(tr("已生成 %1 个运动体。").arg(added));
}

void MotionWindow::editNativeEntity(const QString& collection, int id, const QString& requestedType)
{
    if (!m_projectActive) return;
    if (editOriginalMotionEntity(collection,id,requestedType)) return;
    if (motionBusy() || m_jobPart >= 0 || !m_model.nativeDocument) return;
    auto document = *m_model.nativeDocument;
    QJsonObject row = document.entity(collection, id);
    if (collection == "importedMnfBodies") {
        if (!row.isEmpty())
            importMnf(document.assetPath(row.value("mnfFile").toString()), id);
        return;
    }
    const bool creating=row.isEmpty();
    QString type = requestedType;
    if (row.isEmpty()) {
        QStringList types;
        if (collection == "joints") types = jointTypes;
        else if (collection == "couplings") types = choices("couplingType");
        else if (collection == "springs") types = choices("springType");
        else if (collection == "vectorForces") types = choices("loadType");
        else if (collection == "sensors") types = choices("sensorType");
        else if (collection == "controlInputs") types = choices("inputType");
        if (type.isEmpty() && !types.isEmpty()) {
            QStringList shown; for (const auto& t : types) shown.append(enumLabel(t)); bool accepted = false;
            const auto selected = QInputDialog::getItem(this, tr("新建%1").arg(label(collection)), tr("类型"), shown, 0, false, &accepted);
            if (!accepted) return; type = types.value(shown.indexOf(selected));
        }
        row = templateRecord(document, collection, type);
        if (collection == "bodies") {
            const auto part = std::find_if(m_document.parts().begin(), m_document.parts().end(), [this](const auto& p) { return canRemovePart(p.id); });
            if (part == m_document.parts().end()) { setNotice(tr("没有可分配的几何实体。"), true); return; }
            QString error; row = bodyRecord(m_document, {nextId(document, collection), part->name, part->id, false, 1, 0}, error, true);
            if (row.isEmpty()) { setNotice(error, true); return; }
            document.bodyPartBindings.insert(row.value("id").toInt(), {part->id});
        }
    }
    id = row.value("id").toInt();
    // Add optional defaults when the older document omitted editable fields.
    const auto defaults = templateRecord(document, collection, row.value(collection == "joints" ? "jointType" : collection == "springs" ? "springType" : "").toString());
    if (collection=="drives") for (const auto& key:{"driveTableX","driveTableY","motionType","controlMode","expression"})
        if (!row.contains(key)) row.insert(key,defaults.value(key));
    if (QStringList{"springs","vectorForces","scalarForces","scalarTorques","threePointForces","threePointTorques"}.contains(collection))
        for (const auto& key:{"expression","forceFunction","torqueFunction","dampingFunction"})
            if (!row.contains(key)&&defaults.contains(key)) row.insert(key,defaults.value(key));
    if (collection=="vectorForces"&&!row.contains("magnitudeMode"))row.insert("magnitudeMode",defaults.value("magnitudeMode"));
    if (collection=="springs")for(const auto& key:{"tableX","tableY","dampingEnabled"})
        if(!row.contains(key))row.insert(key,defaults.value(key));
    for (const auto& key : {QStringLiteral("friction"), QStringLiteral("limit"), QStringLiteral("occSurfaceFaces"),
            QStringLiteral("occCurveEdges"), QStringLiteral("occCurveIEdges"), QStringLiteral("occCurveJEdges")})
        if (!row.contains(key) && defaults.contains(key)) row.insert(key, defaults.value(key));
    const bool autoFlex=collection=="autoFlexBodies";
    completeNativeTypedRecord(row, collection, document);
    QStringList missingFlexMaterialProperties;
    const bool missingMaterial=autoFlex && row.value("material").toObject().isEmpty()
        && document.entity("materials", row.value("materialId").toInt(-1)).isEmpty();
    if (autoFlex) {
        for (const auto& key : {"enabled", "elementSize", "modeCount", "meshFile", "attachPoints", "connectionRadius",
                "freqCutoff", "dampingType", "modalDampingRatio", "rayleighMass", "rayleighStiffness"})
            if (!row.contains(key)) row.insert(key, defaults.value(key));
        if (!row.contains("interfaceMode")) row.insert("interfaceMode", "SharedBody");
        if (!row.contains("interfaceJointGroups")) row.insert("interfaceJointGroups", QJsonArray{});
        auto material=row.value("material").toObject();
        int materialId=row.value("materialId").toInt(-1);
        const auto referenced=document.entity("materials", materialId);
        if (creating && !document.collection("materials").isEmpty()) material={};
        if (material.isEmpty()) material=referenced;
        if (material.isEmpty() && !document.collection("materials").isEmpty()) {
            material=document.collection("materials").first().toObject(); materialId=material.value("id").toInt();
        }
        if (material.isEmpty()) material=defaults.value("material").toObject();
        if (document.entity("materials", materialId).isEmpty()) {
            materialId=nextId(document, "materials"); material.insert("id", materialId);
            if (!material.contains("name")) material.insert("name", missingMaterial || creating ? tr("默认钢（请核对）") : tr("本体材料（可编辑）"));
            put(document, "materials", material);
        }
        row.insert("materialId", materialId);
        QJsonObject properties;
        for (const auto& key : {"youngModulus", "poissonRatio", "density"}) {
            if (!material.value(key).isDouble()) missingFlexMaterialProperties.append(key);
            properties.insert(key,material.value(key).toDouble());
        }
        row.insert("material", properties);
    }
    if (collection == "bodies" && !row.contains("inertiaConvention")) row.insert("inertiaConvention", "auto");
    if (collection == "bodies" && !row.contains("isCustomMassProps")) row.insert("isCustomMassProps",false);
    if (collection == "sensors") convertSensorPoints(row,document,true);
    if (collection == "bodies" && !row.contains("densityKgMm3")) row.insert("densityKgMm3", kDefaultNativeBodyDensityKgMm3);
    const bool bodyDrive = collection == "bodyDrives";
    const auto bodyDriveMode = row.value("bodyDriveMode").toString();
    const bool poseDrive = bodyDriveMode == "Pose";
    auto* panel = new QWidget; panel->setObjectName("nativeEditor_" + collection);
    auto* picker = new NativePickSession(m_viewport, m_document, row.value("occPickReferences").toObject(), panel);
    auto* form = new QFormLayout(panel); form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setSizeConstraint(QLayout::SetMinimumSize);
    if (bodyDrive) {
        form->setContentsMargins(0, 0, 0, 0);
        form->setVerticalSpacing(MxFunctions::UiMetrics::rowGap());
    } else {
        auto* title = new QLabel(label(collection)); title->setStyleSheet(QStringLiteral("font-weight:600")); form->addRow(title);
    }
    const auto addBodyDriveSection = [form](const QString& title, const QString& name) {
        auto* section = new MechMainWindow::FoldSection(title);
        section->setObjectName(name);
        form->addRow(section);
        return section;
    };
    if (collection == "bodies") {
        auto* help = new QLabel(tr("取消“指定质量属性”后，按绑定几何和密度计算质量、质心及惯量。导入数据先保留原值；重新计算或更改绑定、密度时更新。惯量顺序为 Ixx、Iyy、Izz、Ixy、Ixz、Iyz，后三项区分矩阵项与正号惯量积。"));
        help->setWordWrap(true); form->addRow(help);
    }
    if (collection == "autoFlexBodies") {
        auto* help = new QLabel(missingMaterial
            ? tr("原运动数据未包含所引用的材料参数。请核对下面选中的材料与 E / ν / ρ 后保存；没有材料库时显示默认钢参数。保存后生成并显示真实体网格，已有体网格直接复用。")
            : tr("材料参数写回所选材料。保存后生成并显示真实体网格，已有体网格直接复用；附着节点留空时按连接此体的运动副自动分组。"));
        help->setWordWrap(true); form->addRow(help);
    }
    if (collection == "softStops" || collection == "tensionCables") {
        auto* help = new QLabel(collection == "tensionCables"
            ? tr("绳索松弛时力为零，仅产生拉力。作用点使用初始全局坐标；长度单位 mm，刚度 N/mm，阻尼 N·s/mm。")
            : tr("沿方向投影计算止挡间隙，仅在越过止挡时受力。刚度单位 N/mm^指数，阻尼 N·s/mm；表格两列分别为压入量 mm 和力 N。"));
        help->setWordWrap(true); form->addRow(help);
    }
    QListWidget* parts = nullptr;
    if (collection == "bodies") {
        parts = partList(m_document, document, id); form->addRow(tr("绑定几何 (可多选)"), parts);
        auto* select = new QPushButton(tr("在视口选择绑定部件")); select->setObjectName("native_pick_bodyParts"); form->addRow(select);
        connect(select, &QPushButton::clicked, panel, [picker,select,parts] {
            std::set<int> allowed; QVector<PickResult> initial;
            for (int i=0;i<parts->count();++i) {
                const auto* item=parts->item(i); const int partId=item->data(Qt::UserRole).toInt();
                if (!(item->flags() & Qt::ItemIsEnabled)) continue;
                allowed.insert(partId);
                if (item->checkState()==Qt::Checked) { PickResult p; p.kind=PickKind::Part; p.partId=partId; initial.append(p); }
            }
            picker->begin("bodyParts",select,PickKind::Part,true,allowed,initial,[parts](const auto& picks) {
                QSet<int> ids; for (const auto& p:picks) ids.insert(p.partId);
                for (int i=0;i<parts->count();++i) if (parts->item(i)->flags() & Qt::ItemIsEnabled)
                    parts->item(i)->setCheckState(ids.contains(parts->item(i)->data(Qt::UserRole).toInt()) ? Qt::Checked : Qt::Unchecked);
            });
        });
    }
    QMap<QString, ReadValue> fields;
    fields.insert("occPickReferences", [picker] { return picker->references(); });
    QComboBox* materialChoice=nullptr;
    auto flexMesh=std::make_shared<QJsonObject>(row.value("mesh").toObject());
    auto flexGroups=std::make_shared<QJsonArray>(row.value("attachGroups").toArray());
    auto flexPositions=std::make_shared<QJsonArray>(row.value("attachPointPositions").toArray());
    auto attachmentMeshChanged=std::make_shared<bool>(row.value("attachmentMeshChanged").toBool());
    if (autoFlex) {
        if (flexMesh->isEmpty() && row.value("meshFile").toString().isEmpty())
            *flexMesh=collectFlexMesh(document.bodyPartBindings.value(row.value("targetBodyId").toInt(-1)), m_meshes);
        materialChoice=new QComboBox; materialChoice->setObjectName("native_materialId");
        for (const auto& value : document.collection("materials")) {
            const auto material=value.toObject(); materialChoice->addItem(material.value("name").toString()+QStringLiteral(" [%1]").arg(material.value("id").toInt()), material.value("id").toInt());
        }
        materialChoice->setCurrentIndex(materialChoice->findData(row.value("materialId").toInt()));
        form->addRow(tr("材料"), materialChoice);
        fields.insert("materialId", [materialChoice] { return materialChoice->currentData().toInt(); });
        fields.insert("mesh", [flexMesh] { return *flexMesh; });
        fields.insert("attachGroups", [flexGroups] { return *flexGroups; });
        fields.insert("attachPointPositions", [flexPositions] { return *flexPositions; });
        fields.insert("attachmentMeshChanged", [attachmentMeshChanged] { return *attachmentMeshChanged; });
        auto* meshState=new QLabel(flexMeshSummary(*flexMesh)); meshState->setObjectName("nativeFlexMeshStatus"); meshState->setWordWrap(true);
        meshState->setMinimumWidth(160); // Let WrapLongRows put the summary below its label in a narrow dock.
        form->addRow(tr("体网格"), meshState);
    }
    const QStringList hidden{"id", "isUserRename", "geoEntityIds", "geoEntityNames", "currentCurveRequest", "curveNode", "curveRequests",
        "legacyEndValue", "driveType", "targetType", "surfaceFaceIds", "curveEdgeIds", "curveIEdgeIds", "curveJEdgeIds", "occPickReferences",
        "channels"};
    if (!bodyDrive && row.contains("name")) fields.insert("name", addField(form, "name", row.value("name"), document, collection));
    auto specialisedFields = addNativeTypedFields(panel, form, row, collection, document,
        [&, picker](QFormLayout* target, const QString& field, const QJsonValue& value) {
            return addField(target, field, value, document, collection, 0, picker);
        }, fields);
    if (bodyDrive) {
        const auto bind = [&](MechMainWindow::FoldSection* section, const QString& key) {
            if (!row.contains(key)) return;
            fields.insert(key, addField(section->layout(), key, row.value(key), document, collection, 0, picker));
            specialisedFields.insert(key);
        };
        auto* target = addBodyDriveSection(tr("驱动目标"), "bodyDriveTargetSection");
        for (const auto* key : {"name", "bodyDriveMode", "targetBodyId", "referenceBodyId", "enabled"})
            bind(target, QString::fromLatin1(key));

        auto* coordinates = addBodyDriveSection(tr("坐标定义"), "bodyDriveCoordinateSection");
        bind(coordinates, "origin");
        if (poseDrive && row.contains("frameAngles")) {
            const auto source = row.value("frameAngles");
            const auto angles = source.toArray();
            const auto object = source.toObject();
            QMap<QString, ReadValue> readers;
            const QString axes = QStringLiteral("xyz");
            const QString components = QStringLiteral("ABC");
            auto* layout = coordinates->layout();
            for (int i = 0; i < axes.size(); ++i) {
                const QString key(axes[i]);
                const QJsonValue value = angles.size() == 3 ? angles.at(i) : object.value(key);
                readers.insert(key, addField(layout, key, QJsonValue(value.toDouble()), document, collection,
                                             0, nullptr, {}, tr("初始 %1 (°)").arg(components[i])));
                layout->itemAt(layout->rowCount() - 1, QFormLayout::LabelRole)->widget()
                    ->setToolTip(tr("驱动坐标系的初始内禀 X、Y、Z 转角。"));
            }
            fields.insert("frameAngles", [source, object, readers] {
                if (source.isArray()) return QJsonValue(QJsonArray{readers.value("x")(), readers.value("y")(), readers.value("z")()});
                auto updated = object;
                for (auto it = readers.cbegin(); it != readers.cend(); ++it) updated.insert(it.key(), it.value()());
                return QJsonValue(updated);
            });
            specialisedFields.insert("frameAngles");
        } else if (!poseDrive) {
            bind(coordinates, "axis");
            bind(coordinates, "guide");
            if (auto* axis = panel->findChild<QWidget*>("originalDirectionPicker_axis"))
                axis->setToolTip(tr("轴用初始全局坐标定义，随参考体运动。"));
        }
    }
    if (collection == "sensors") for (const auto& key : {"sensorType", "variable"})
        if (auto* selector = panel->findChild<QComboBox*>(QStringLiteral("native_") + key))
            connect(selector, qOverload<int>(&QComboBox::currentIndexChanged), panel, [picker] { picker->finish(); });
    for (auto it = row.begin(); it != row.end(); ++it) {
        if (it.key() == "name" || hidden.contains(it.key()) || specialisedFields.contains(it.key())) continue;
        if (collection=="vectorForces" && row.value("loadType").toString()=="Bushing"
            && QStringList{"expression","magnitude","magnitudeMode","direction","directionMode"}.contains(it.key())) continue;
        if (autoFlex && QStringList{"mesh", "materialId", "meshSourceFile", "attachGroups", "attachPointPositions", "attachmentMeshChanged", "interfaceJointGroups", "meshType", "minElementSize", "curvatureRefinement",
                "solverType", "freqLower", "freqUpper", "rigidBodyModeFilter", "connectionType", "interfaceDofs",
                "outputModeShapes", "outputStressStrain", "outputFormat"}.contains(it.key())) continue;
        if (autoFlex && it.key()=="dampingType") {
            auto* damping=new QComboBox; damping->setObjectName("native_dampingType");
            damping->addItem(tr("无阻尼"), 0); damping->addItem(tr("模态阻尼比"), 1); damping->addItem(tr("瑞利阻尼"), 2);
            damping->setCurrentIndex(damping->findData(it.value().toInt(2))); form->addRow(label(it.key()), damping);
            fields.insert(it.key(), [damping] { return damping->currentData().toInt(); }); continue;
        }
        if (collection == "joints" && isTopologyField(it.key())) {
            const auto field = it.key(); const bool faces = field == "occSurfaceFaces";
            auto selection = std::make_shared<QJsonArray>(it.value().toArray());
            auto* selected = new QLabel(topologySummary(*selection, faces)); selected->setWordWrap(true); form->addRow(label(field), selected);
            auto* choose = new QPushButton(tr("选择约束%1…").arg(faces ? tr("面") : tr("边")));
            choose->setObjectName("native_select_" + field); form->addRow(choose);
            auto* pick = new QPushButton(tr("在视口拾取约束%1").arg(faces ? tr("面") : tr("边")));
            pick->setObjectName("native_pick_"+field); form->addRow(pick);
            connect(pick,&QPushButton::clicked,panel,[this,picker,pick,panel,selected,selection,document,row,field,faces] {
                const auto bodyField=topologyBodyField(field); int bodyId=row.value(bodyField).toInt(-1);
                if (auto* combo=panel->findChild<QComboBox*>("native_"+bodyField)) bodyId=combo->currentData().toInt();
                const auto bound=document.bodyPartBindings.value(bodyId);
                std::set<int> allowed(bound.begin(),bound.end());
                if (!faces) { allowed.clear(); for (const auto& part:m_document.parts()) allowed.insert(part.id); }
                if (faces && allowed.empty()) { setNotice(tr("该运动体尚未绑定几何面。请先绑定实体或选择有几何的参考体。"),true); return; }
                QVector<PickResult> initial;
                for (const auto& value:*selection) { const auto ref=value.toObject(); PickResult p;
                    p.kind=faces ? PickKind::Face : PickKind::Edge; p.partId=ref.value("partId").toInt(-1);
                    p.index=ref.value("index").toInt(-1); if (allowed.empty() || allowed.count(p.partId)) initial.append(p); }
                picker->begin(field,pick,faces ? PickKind::Face : PickKind::Edge,true,allowed,initial,
                    [this,selection,selected,faces](const auto& picks) {
                        QJsonArray refs; QString error;
                        for (const auto& p:picks) {
                            const auto ref=nativeTopologyReference(m_document,p.partId,p.index,faces ? TopAbs_FACE : TopAbs_EDGE);
                            if (ref.isEmpty()) { setNotice(tr("所选拓扑已失效，请重新拾取。"),true); return; }
                            refs.append(ref);
                        }
                        *selection=refs; selected->setText(topologySummary(refs,faces));
                    });
            });
            connect(choose, &QPushButton::clicked, panel, [this, picker, panel, selected, selection, document, row, field, faces] {
                picker->finish();
                auto joint = row; const auto bodyField = topologyBodyField(field);
                if (auto* combo = panel->findChild<QComboBox*>("native_" + bodyField)) joint.insert(bodyField, combo->currentData().toInt());
                if (selectConstraintTopology(this, m_document, document, joint, field, *selection)) selected->setText(topologySummary(*selection, faces));
            });
            fields.insert(field, [selection] { return *selection; });
        } else fields.insert(it.key(), addField(form, it.key(), it.value(), document, collection, 0, picker));
    }
    if (collection == "bodies") {
        auto* massStatus=new QLabel; massStatus->setObjectName("nativeMassStatus"); massStatus->setWordWrap(true); form->addRow(massStatus);
        auto* calculate=new QPushButton(tr("按绑定几何重新计算质量")); calculate->setObjectName("nativeRecalculateMass"); form->addRow(calculate);
        auto* custom=panel->findChild<QCheckBox*>("native_isCustomMassProps");
        auto* density=panel->findChild<QLineEdit*>("native_densityKgMm3");
        const auto recalculate=[this,panel,parts,custom,density,massStatus,row] {
            if (!custom || custom->isChecked()) return;
            auto computed=row; computed.insert("isCustomMassProps",false); QString error;
            bool valid=false; const double rho=density->text().toDouble(&valid);
            if (!valid || !updateNativeBodyMass(m_document,selectedParts(parts),computed,rho,error)) {
                massStatus->setText(valid ? error : tr("请输入有效密度。")); massStatus->setProperty("massError",true); return;
            }
            panel->findChild<QLineEdit*>("native_mass")->setText(QString::number(computed.value("mass").toDouble(),'g',17));
            for (auto* box:panel->findChildren<QGroupBox*>()) if (box->title()==label("centerOfMass")) {
                const auto c=computed.value("centerOfMass").toObject();
                for (const auto& key:{"x","y","z"}) box->findChild<QLineEdit*>(QStringLiteral("native_")+key)->setText(QString::number(c.value(key).toDouble(),'g',17));
            }
            auto* tensor=panel->findChild<QTableWidget*>("native_inertia"); const auto values=computed.value("inertia").toArray();
            tensor->setRowCount(values.size());
            for (int r=0;r<values.size();++r) tensor->setItem(r,0,new QTableWidgetItem(QString::number(values[r].toDouble(),'g',17)));
            auto* convention=panel->findChild<QComboBox*>("native_inertiaConvention"); convention->setCurrentIndex(convention->findData("tensor"));
            massStatus->setProperty("massError",false);
            massStatus->setText(tr("已按 %1 个实体、密度 %2 kg/mm³ 计算质量、全局质心和质心惯量。").arg(selectedParts(parts).size()).arg(rho,0,'g',8));
        };
        connect(calculate,&QPushButton::clicked,panel,[custom,recalculate] { if (custom->isChecked()) custom->setChecked(false); else recalculate(); });
        connect(custom,&QCheckBox::toggled,panel,[recalculate,massStatus](bool manual) {
            if (!manual) recalculate(); else { massStatus->setProperty("massError",false); massStatus->setText(QStringLiteral("使用指定质量属性。")); }
        });
        const auto lockMass=[panel,custom,density] {
            const bool manual=custom->isChecked(); density->setEnabled(!manual);
            panel->findChild<QLineEdit*>("native_mass")->setReadOnly(!manual);
            panel->findChild<QTableWidget*>("native_inertia")->setEnabled(manual);
            panel->findChild<QComboBox*>("native_inertiaConvention")->setEnabled(manual);
            for (auto* box:panel->findChildren<QGroupBox*>()) if (box->title()==label("centerOfMass")) box->setEnabled(manual);
        };
        connect(custom,&QCheckBox::toggled,panel,[lockMass](bool) { lockMass(); }); lockMass();
        connect(density,&QLineEdit::editingFinished,panel,recalculate);
        connect(parts,&QListWidget::itemChanged,panel,[recalculate](QListWidgetItem*) { recalculate(); });
    }
    if (autoFlex) {
        auto interfaces = std::make_shared<QJsonArray>(row.value("interfaceJointGroups").toArray());
        fields.insert("interfaceJointGroups", [interfaces] { return *interfaces; });
        auto* interfaceInfo = new QLabel; interfaceInfo->setObjectName("nativeFlexInterfaceInfo"); interfaceInfo->setWordWrap(true); form->addRow(interfaceInfo);
        auto* groupState = new QLabel; groupState->setObjectName("nativeFlexInterfaceGroupsState"); groupState->setWordWrap(true); form->addRow(groupState);
        auto* selectGroups = new QPushButton(tr("设置独立接口的运动副分组…")); selectGroups->setObjectName("nativeFlexInterfaceGroups"); form->addRow(selectGroups);
        auto* mode = panel->findChild<QComboBox*>("native_interfaceMode");
        auto* damping = panel->findChild<QComboBox*>("native_dampingType");
        const auto updateInterface = [=] {
            const bool independent = mode->currentData().toString() == "Independent";
            selectGroups->setVisible(independent); groupState->setVisible(independent);
            groupState->setText(interfaces->isEmpty() ? QStringLiteral("接口分组：按运动副作用点自动建立独立接口。")
                : QStringLiteral("接口分组：已指定 %1 组。更换目标体后请重新分组。").arg(interfaces->size()));
            interfaceInfo->setText(independent
                ? damping->currentData().toInt() == 1
                    ? QStringLiteral("独立接口采用共回转全有限元，至少两个互不重叠、各含 3 个不共线节点的接口片。当前模态阻尼比不能用于全有限元，请选择无阻尼或瑞利阻尼。")
                    : QStringLiteral("独立接口采用共回转全有限元：各运动副端点可相对变形，节点与接口真实参与求解。至少两个接口片；每片至少 3 个不共线节点。自动附着按最近接口划分。模态数量与截止频率仅用于检查器，动力学保留全部节点自由度。")
                : QStringLiteral("共用参考体采用原模态缩减流程：全部附着组刚性夹持到同一参考体，合并去重后至少需要 3 个不共线节点；目前不支持单点或共线节点作为铰接接口。多个片之间不提供独立端点运动。无附着自由体可单独检查自由模态。"));
            interfaceInfo->setToolTip(interfaceInfo->text());
            for (const auto& key : {"modalDampingRatio"})
                if (auto* field = panel->findChild<QLineEdit*>(QStringLiteral("native_") + key)) field->setEnabled(!independent);
        };
        connect(mode, qOverload<int>(&QComboBox::currentIndexChanged), panel, [=] { picker->finish(); updateInterface(); });
        connect(damping, qOverload<int>(&QComboBox::currentIndexChanged), panel, [updateInterface] { updateInterface(); });
        connect(selectGroups, &QPushButton::clicked, panel, [=] {
            picker->finish();
            const int body = panel->findChild<QComboBox*>("native_targetBodyId")->currentData().toInt();
            if (selectFlexInterfaceGroups(this, document, body, *interfaces)) updateInterface();
        });
        updateInterface();
        auto* inspect = new QPushButton(tr("检查已保存柔性体的真实模态…")); inspect->setObjectName("nativeInspectModes");
        inspect->setEnabled(!creating); inspect->setToolTip(tr("使用已保存模型。请先保存此面板的修改；新建柔性体保存并生成网格后可检查。")); form->addRow(inspect);
        connect(inspect, &QPushButton::clicked, panel, [this, picker, id] { picker->finish(); inspectFlexModes(id); });
        for (const auto& key : missingFlexMaterialProperties)
            if (auto* input=panel->findChild<QLineEdit*>("native_"+key)) { input->clear(); input->setPlaceholderText(tr("必填材料参数")); }
        auto* attachState=new QLabel(*attachmentMeshChanged ? tr("体网格或目标几何已经改变。旧节点编号尚未重新映射，请重新选择附着节点或明确改为自动附着后求解。")
            : !flexGroups->isEmpty() ? tr("已读取 %1 个显式附着组；重新选择节点将替换这些组。").arg(flexGroups->size())
            : !flexPositions->isEmpty() ? tr("已读取 %1 个附着位置；重新选择节点将替换这些位置。").arg(flexPositions->size())
            : tr("附着节点留空时自动按运动副收组；原网格编号须有对应节点坐标。"));
        attachState->setWordWrap(true); form->addRow(attachState);
        auto* selectNodes=new QPushButton(tr("选择附着节点…")); selectNodes->setObjectName("nativeFlexSelectNodes"); form->addRow(selectNodes);
        auto* pickNodes=new QPushButton(tr("在视口拾取附着节点")); pickNodes->setObjectName("nativeFlexPickNodes"); form->addRow(pickNodes);
        auto* automatic=new QPushButton(tr("按运动副自动附着")); automatic->setObjectName("nativeFlexAutoAttach");
        automatic->setToolTip(tr("清空手动附着节点、显式节点组和位置，改为按连接目标体的运动副自动收组。")); form->addRow(automatic);
        const auto clearAttachments=[panel, flexGroups, flexPositions, attachState, attachmentMeshChanged] {
            if (auto* table=panel->findChild<QTableWidget*>("native_attachPoints")) table->setRowCount(0);
            *flexGroups={}; *flexPositions={};
            *attachmentMeshChanged=false;
            attachState->setText(QStringLiteral("附着节点留空时自动按运动副收组。"));
        };
        connect(automatic, &QPushButton::clicked, panel, [picker,clearAttachments] {
            picker->finish();
            clearAttachments();
        });
        connect(pickNodes,&QPushButton::clicked,panel,[this,picker,pickNodes,panel,document,flexMesh,flexGroups,flexPositions,attachmentMeshChanged,attachState,clearAttachments] {
            FemMesh mesh; QString error;
            if (!readFemMeshJson(*flexMesh,mesh,error)) { setNotice(tr("请先保存并生成真实体网格，再拾取附着节点。 ")+error,true); return; }
            auto* body=panel->findChild<QComboBox*>("native_targetBodyId");
            const auto parts=document.bodyPartBindings.value(body->currentData().toInt());
            if (parts.isEmpty()) { setNotice(tr("目标体没有绑定几何。"),true); return; }
            auto nodeIds=flexMesh->value("nodeIds").toArray(); const bool mapped=nodeIds.size()==int(mesh.nodes.size());
            if (!mapped) { nodeIds={}; for (int i=0;i<int(mesh.nodes.size());++i) nodeIds.append(i+1); }
            QVector<int> ids; for (const auto& id:nodeIds) ids.append(id.toInt());
            m_viewport->setPickNodes(parts.front(),mesh,ids);
            auto* table=panel->findChild<QTableWidget*>("native_attachPoints"); QSet<int> selected;
            QJsonArray originalPoints; for (int r=0;r<table->rowCount();++r) if (auto* item=table->item(r,0)) originalPoints.append(item->text().toInt());
            if (mapped) for (const auto& id:originalPoints) selected.insert(id.toInt());
            QVector<PickResult> initial;
            for (int i=0;i<ids.size();++i) if (selected.contains(ids[i])) {
                PickResult p; p.kind=PickKind::Node; p.partId=parts.front(); p.index=i; p.nodeId=ids[i]; p.position=mesh.nodes[i]; initial.append(p);
            }
            const auto groups=*flexGroups, positions=*flexPositions; const bool changed=*attachmentMeshChanged;
            const auto oldText=attachState->text(); const auto oldMesh=*flexMesh;
            picker->begin("attachPoints",pickNodes,PickKind::Node,true,{parts.front()},initial,
                [=](const auto& picks) {
                    clearAttachments(); flexMesh->insert("nodeIds",nodeIds); table->setRowCount(picks.size());
                    for (int r=0;r<picks.size();++r) table->setItem(r,0,new QTableWidgetItem(QString::number(picks[r].nodeId)));
                    attachState->setText(tr("已拾取 %1 个网格节点。左键加选，右键减选，点击和拖框均可用。").arg(picks.size()));
                },[=] {
                    *flexGroups=groups; *flexPositions=positions; *attachmentMeshChanged=changed; *flexMesh=oldMesh;
                    table->setRowCount(originalPoints.size());
                    for (int r=0;r<originalPoints.size();++r) table->setItem(r,0,new QTableWidgetItem(QString::number(originalPoints[r].toInt())));
                    attachState->setText(oldText);
                });
        });
        connect(selectNodes, &QPushButton::clicked, panel, [this, picker, panel, flexMesh, attachState, clearAttachments] {
            picker->finish();
            if (flexMesh->isEmpty()) { setNotice(tr("请先保存生成体网格，再编辑此自动柔性体选择节点；留空时按运动副自动附着。"), true); return; }
            auto* table=panel->findChild<QTableWidget*>("native_attachPoints"); if (!table) return;
            QJsonArray points;
            for (int r=0; r<table->rowCount(); ++r) if (const auto* item=table->item(r,0)) points.append(item->text().toInt());
            if (!selectFlexNodes(this, *flexMesh, points)) return;
            clearAttachments();
            table->setRowCount(points.size());
            for (int r=0; r<points.size(); ++r) table->setItem(r,0,new QTableWidgetItem(QString::number(points[r].toInt())));
            attachState->setText(tr("已选择 %1 个真实网格节点作为附着接口。").arg(points.size()));
        });
        connect(materialChoice, qOverload<int>(&QComboBox::currentIndexChanged), panel, [panel, document, materialChoice] {
            const auto material=document.entity("materials", materialChoice->currentData().toInt());
            for (const auto& key : {"youngModulus", "poissonRatio", "density"})
                if (auto* input=panel->findChild<QLineEdit*>(QStringLiteral("native_")+key))
                    input->setText(material.value(key).isDouble() ? QString::number(material.value(key).toDouble(), 'g', 17) : QString{});
        });
        if (auto* body=panel->findChild<QComboBox*>("native_targetBodyId"))
            connect(body, qOverload<int>(&QComboBox::currentIndexChanged), panel, [panel, picker, document, body, flexMesh, attachState, clearAttachments, interfaces, updateInterface, this] {
                // The active session captures node IDs and positions from the old mesh.
                // Detach it before replacing that mesh, even when node counts coincide.
                picker->finish();
                *flexMesh=collectFlexMesh(document.bodyPartBindings.value(body->currentData().toInt()), m_meshes);
                panel->findChild<QLabel*>("nativeFlexMeshStatus")->setText(flexMeshSummary(*flexMesh));
                if (auto* file=panel->findChild<QLineEdit*>("native_meshFile")) file->clear();
                clearAttachments();
                *interfaces={}; updateInterface();
                attachState->setText(tr("目标体已变更，请重新选择附着节点；留空将按新目标体的运动副自动附着。"));
            });
    }
    if (bodyDrive) {
        const bool speed = bodyDriveMode.endsWith("Speed");
        const auto stored = row.value("channels").toObject();
        const auto scenarios = document.collection("scenarios");
        const double duration = scenarios.isEmpty() ? 10 : scenarios.first().toObject().value("endTime").toDouble(10);
        auto* excitation = poseDrive ? addBodyDriveSection(tr("平移激励"), "bodyDriveTranslationSection")
                                     : addBodyDriveSection(tr("驱动激励"), "bodyDriveExcitationSection");
        auto* rotation = poseDrive ? addBodyDriveSection(tr("转动激励"), "bodyDriveRotationSection") : nullptr;
        QMap<QString, ReadValue> channelReaders;
        const QString axes = poseDrive ? QStringLiteral("XYZABC") : QStringLiteral("X");
        for (int i=0; i<axes.size(); ++i) {
            const QString key(axes[i]);
            const bool angular = bodyDriveMode.startsWith("Rotation") || (poseDrive && i >= 3);
            const QString unit = (angular ? QStringLiteral("°") : QStringLiteral("mm")) +
                                 (speed ? QStringLiteral("/s") : QString{});
            auto channel = std::make_shared<QJsonObject>(stored.value(key).toObject());
            auto* controls = new QWidget;
            auto* layout = new QHBoxLayout(controls);
            layout->setContentsMargins(0,0,0,0);
            layout->setSpacing(MxFunctions::UiMetrics::choiceGap());
            auto* expression = new QLineEdit(channel->value("expression").toString("0"), controls);
            expression->setObjectName(QStringLiteral("native_bodyDriveChannel")+key);
            auto* editor = new QToolButton(controls);
            editor->setObjectName(QStringLiteral("native_bodyDriveFunction") + key);
            editor->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/homepage/hanshu3x.png")));
            editor->setIconSize(MxFunctions::UiMetrics::quickIconSize());
            const auto editorLabel = tr("编辑函数与时间表…");
            editor->setToolTip(editorLabel);
            editor->setAccessibleName(editorLabel);
            const int controlHeight = MxFunctions::UiMetrics::controlHeight();
            editor->setFixedSize(controlHeight, controlHeight);
            layout->addWidget(expression, 1);
            layout->addWidget(editor);
            auto* section = poseDrive && i >= 3 ? rotation : excitation;
            section->addRow(QStringLiteral("%1 (%2)").arg(poseDrive ? key : enumLabel(bodyDriveMode), unit), controls);
            const auto sync = [expression, channel] {
                const auto samples = channel->value("driveTableX").toArray().size();
                expression->setReadOnly(samples > 0);
                expression->setToolTip(samples > 0 ? QObject::tr("当前使用时间表，共 %1 个采样点。").arg(samples) : QObject::tr("输入常数或时间函数，t 的单位为秒。"));
            };
            sync();
            connect(editor, &QToolButton::clicked, panel, [this, expression, channel, unit, duration, sync] {
                auto value = expression->text();
                auto times = channel->value("driveTableX").toArray();
                auto values = channel->value("driveTableY").toArray();
                if (!editNativeFunction(this, value, times, values, unit, duration)) return;
                expression->setText(value);
                channel->insert("expression", value);
                channel->insert("driveTableX", times);
                channel->insert("driveTableY", values);
                sync();
            });
            channelReaders.insert(key, [expression, channel] {
                auto out = *channel;
                out.insert("expression", expression->text());
                return out;
            });
        }
        fields.insert("channels", [channelReaders] {
            QJsonObject out;
            for (auto channel = channelReaders.begin(); channel != channelReaders.end(); ++channel)
                out.insert(channel.key(), channel.value()());
            return out;
        });
    }
    if (collection=="drives") {
        auto* function=new QPushButton(tr("函数编辑、表格与预览…")); function->setObjectName("nativeFunctionBuilder"); form->addRow(function);
        connect(function,&QPushButton::clicked,panel,[this,panel,document] {
            auto* expression=panel->findChild<QLineEdit*>("native_expression");
            auto* x=panel->findChild<QTableWidget*>("native_driveTableX"); auto* y=panel->findChild<QTableWidget*>("native_driveTableY");
            if (!expression || !x || !y) return;
            const auto read=[](QTableWidget* table) { QJsonArray values; for (int r=0;r<table->rowCount();++r) {
                bool ok=false; const double v=table->item(r,0) ? table->item(r,0)->text().toDouble(&ok) : 0;
                if (!ok || !std::isfinite(v)) throw std::runtime_error("函数表中包含无效数值。"); values.append(v);
            } return values; };
            try {
                auto text=expression->text(); auto tx=read(x), ty=read(y);
                const QJsonObject drive{{"motionType", panel->findChild<QComboBox*>("native_motionType")->currentData().toString()},
                    {"controlMode", panel->findChild<QComboBox*>("native_controlMode")->currentData().toString()}};
                const auto scenarios=document.collection("scenarios"); const double duration=scenarios.isEmpty() ? 10 : scenarios.first().toObject().value("endTime").toDouble(10);
                if (!editNativeFunction(this,text,tx,ty,nativeDriveCommandUnit(drive),duration)) return;
                expression->setText(text);
                const auto write=[](QTableWidget* table,const QJsonArray& values) { table->setRowCount(values.size());
                    for (int r=0;r<values.size();++r) table->setItem(r,0,new QTableWidgetItem(QString::number(values[r].toDouble(),'g',17))); };
                write(x,tx); write(y,ty);
            } catch (const std::exception& error) { setNotice(QString::fromUtf8(error.what()),true); }
        });
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); form->addRow(buttons);
    if (bodyDrive) {
        buttons->button(QDialogButtonBox::Ok)->setText(tr("保存"));
        buttons->button(QDialogButtonBox::Cancel)->setText(tr("取消"));
    }
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("motionApplyButton"));
    connect(buttons, &QDialogButtonBox::rejected, this, [this] { closeEditor(); updateParameters(); });
    connect(buttons, &QDialogButtonBox::accepted, this, [this, document, row, fields, parts, collection, id, panel, picker]() mutable {
        try {
            picker->finish();
            if (auto* status=panel->findChild<QLabel*>("nativeMassStatus"); status && status->property("massError").toBool()) {
                setNotice(status->text(),true); return;
            }
            auto next = row;
            for (auto it = fields.begin(); it != fields.end(); ++it) next.insert(it.key(), it.value()());
            const auto typedIssue = nativeTypedRecordIssue(next, collection, document);
            if (!typedIssue.isEmpty()) {
                setNotice(typedIssue, true); // Full reason remains in the output log and tooltip.
                m_notice->setText(tr("上次保存未通过检查；悬停查看具体原因。"));
                return;
            }
            if (collection == "controllers") {
                const auto* targetSource = panel->findChild<QComboBox*>("native_targetSource");
                if (targetSource && targetSource->currentData().toString() == "Input" && next.value("targetInputId").toInt(-1) < 0) {
                    setNotice(tr("目标来源为控制输入时，必须选择一条有效轨迹。"), true); return;
                }
            }
            if (collection=="sensors") convertSensorPoints(next,document,false);
            if (parts && !next.value("isCustomMassProps").toBool()
                && (next.value("densityKgMm3")!=row.value("densityKgMm3") || row.value("isCustomMassProps").toBool()
                    || selectedParts(parts)!=document.bodyPartBindings.value(id))) {
                QString error;
                if (!updateNativeBodyMass(m_document,selectedParts(parts),next,next.value("densityKgMm3").toDouble(),error)) { setNotice(error,true); return; }
            }
            if (collection=="autoFlexBodies") {
                if (next.value("interfaceMode").toString() == "Independent" && next.value("dampingType").toInt() == 1) {
                    setNotice(tr("独立接口的全有限元求解不能使用模态阻尼比，请选择无阻尼或瑞利阻尼。"), true); return;
                }
                if (next.value("targetBodyId")!=row.value("targetBodyId")) next.remove("meshSourceFile");
                const auto properties=next.value("material").toObject();
                const double e=properties.value("youngModulus").toDouble(NAN), nu=properties.value("poissonRatio").toDouble(NAN), rho=properties.value("density").toDouble(NAN);
                if (!std::isfinite(e) || !std::isfinite(nu) || !std::isfinite(rho) || e<=0 || rho<=0 || nu<=-1 || nu>=0.5) {
                    setNotice(tr("材料必须满足 E > 0、ρ > 0、-1 < ν < 0.5；单位为 MPa、kg/mm³。"), true); return;
                }
                const double size=next.value("elementSize").toDouble(), modes=next.value("modeCount").toDouble();
                if (!(size>0) || modes<1 || modes!=std::trunc(modes)) { setNotice(tr("请填写正的网格尺寸与整数模态数量。"), true); return; }
                auto material=document.entity("materials", next.value("materialId").toInt(-1));
                for (auto it=properties.begin(); it!=properties.end(); ++it) material.insert(it.key(), it.value());
                put(document, "materials", material);
                next.remove("material"); // Share the actual material object, as the original panel did.
            }
            if (next.value("expression") != row.value("expression")) next.remove("legacyEndValue");
            put(document, collection, next);
            QString error;
            const bool rebound=parts && selectedParts(parts)!=document.bodyPartBindings.value(id);
            if (parts && !setNativeMotionBodyBinding(m_document, document, id, selectedParts(parts), error)) { setNotice(error, true); return; }
            if (rebound) for (const auto& value : document.collection("autoFlexBodies")) {
                auto flex=value.toObject(); if (flex.value("targetBodyId").toInt(-1)!=id) continue;
                flex.remove("mesh"); flex.insert("meshFile", "");
                flex.remove("meshSourceFile");
                if (!flex.value("attachGroups").toArray().isEmpty() || !flex.value("attachPoints").toArray().isEmpty()
                    || !flex.value("attachPointPositions").toArray().isEmpty()) flex.insert("attachmentMeshChanged",true);
                put(document,"autoFlexBodies",flex);
            }
            if (commitNativeDocument(document, collection == "scenarios" ? ModelNode{ModelNode::Scenario, id}
                : ModelNode{ModelNode::Native, id, collection})) {
                if (collection=="autoFlexBodies") ensureNativeFlexMesh(id);
                else if (rebound) setNotice(tr("已更换目标体的几何绑定；关联自动柔性体将使用新几何的体网格，手动附着节点需重新确认。"));
            }
        } catch (const std::exception& error) { setNotice(QString::fromUtf8(error.what()), true); }
    });
    // Long native records must scroll at their natural row height. Otherwise
    // the dock squeezes nested vector/table groups down to their title bars,
    // hiding the actual parameter controls even though they still hold values.
    auto* scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("nativeParameterScroll"));
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
    if (bodyDrive) {
        scroll->setProperty("mxFitParameterDock", true);
        for (auto* surface : {static_cast<QWidget*>(scroll), scroll->viewport(), panel}) {
            surface->setProperty("mxSurface", true);
            surface->setAttribute(Qt::WA_StyledBackground);
        }
        for (auto* combo : panel->findChildren<QComboBox*>()) {
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(8);
            combo->setMinimumWidth(0);
            combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        }
    } else if (autoFlex || !specialisedFields.isEmpty()) {
        // Keep the ordinary form-label layout, wrapping only rows that do not
        // fit. Material/body names must not widen the complete parameter dock.
        for (auto* layout : panel->findChildren<QFormLayout*>()) layout->setRowWrapPolicy(QFormLayout::WrapLongRows);
        for (auto* combo : panel->findChildren<QComboBox*>()) {
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setMinimumContentsLength(8); combo->setMinimumWidth(0);
            combo->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        }
        for (auto* text : panel->findChildren<QLabel*>()) {
            text->setWordWrap(true);
        }
        fitNativeFormToWidth(panel);
    }
    scroll->setWidget(panel); showEditor(scroll);
}

void MotionWindow::removeNativeEntity(const QString& collection, int id)
{
    if (collection.isEmpty() || !m_model.nativeDocument) return;
    auto model = m_model;
    removeNativeEntityCurves(model, collection, id);
    auto document = *model.nativeDocument; auto data = document.root.value("motionData").toObject(); QJsonArray rows;
    for (const auto& row : document.collection(collection)) if (row.toObject().value("id").toInt() != id) rows.append(row);
    data.insert(collection, rows); document.root.insert("motionData", data);
    if (collection == "bodies") document.bodyPartBindings.remove(id);
    // Only presentation references are removed above. Physical references still
    // block deletion, with the original model left completely unchanged.
    QString error;
    if (!validateNativeMotionDocument(document, error) || !validateNativeMotionBindings(m_document, document, error)) {
        setNotice(error, true); return;
    }
    model.nativeDocument = std::make_shared<NativeMotionDocument>(std::move(document));
    syncNativeMotionModel(model); commitRigid(std::move(model), {});
}

} // namespace MotionOcc
