#include "MotionWindow.h"
#include "OccViewport.h"
#include "NativeMotionPresentation.h"
#include "NativeTopologyBinding.h"
#include "mFoldSection.h"

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
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QTableWidget>
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
        {"springs", "弹簧"}, {"vectorForces", "力 / 力矩 / 衬套"}, {"scalarForces", "标量力"},
        {"scalarTorques", "标量力矩"}, {"threePointForces", "三点力"}, {"threePointTorques", "三点力矩"},
        {"tensionCables", "只拉绳索"}, {"softStops", "止挡"}, {"contacts", "接触"}, {"markers", "标记点"},
        {"sensors", "传感器"}, {"controllers", "控制器"}, {"controlInputs", "控制输入"},
        {"autoFlexBodies", "自动柔性体"}, {"scenarios", "求解方案"},
        {"name", "名称"}, {"id", "编号"}, {"enabled", "启用"}, {"isFixed", "固定参考体"},
        {"bodyIId", "连接体 I"}, {"bodyJId", "连接体 J"}, {"bodyId", "运动体"},
        {"actionBodyId", "动作体"}, {"baseBodyId", "基本体"}, {"targetBodyId", "目标体"},
        {"referenceBodyId", "参考体"}, {"originBodyId", "方向原点体"},
        {"localI", "作用点 I (初始全局 mm)"}, {"localJ", "作用点 J (初始全局 mm)"},
        {"localTarget", "方向目标点 (mm)"}, {"localOrigin", "方向原点 (mm)"},
        {"localPosition", "标记点位置 (mm)"}, {"localRotation", "标记点姿态 (四元数)"},
        {"localRotI", "连接坐标系 I"}, {"localRotJ", "连接坐标系 J"},
        {"centerOfMass", "质心 (mm)"}, {"mass", "质量 (kg)"}, {"inertia", "惯量 (kg·mm²)"},
        {"inertiaConvention", "惯量积约定"},
        {"isCustomMassProps", "指定质量属性"}, {"initialVelocity", "初始速度 (mm/s)"},
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
        {"forceExponent", "接触力指数"}, {"penetrationDepth", "穿透深度 (mm)"},
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
        {"usePreset", "使用求解预设"}, {"presetMode", "求解预设"}, {"solverType", "线性求解器"},
        {"solverTolerance", "求解容差"}, {"solverMaxIters", "最大求解迭代数"},
        {"integratorType", "积分器"}, {"integratorTol", "积分容差"}, {"integratorMaxIters", "最大积分迭代数"},
        {"integratorAlpha", "积分参数 α"}, {"integratorBeta", "积分参数 β"}, {"trackedMarkerIds", "跟踪标记点编号"},
        {"material", "材料"}, {"youngModulus", "弹性模量 (MPa)"}, {"poissonRatio", "泊松比"},
        {"density", "密度 (kg/mm³)"}, {"elementSize", "单元尺寸 (mm)"}, {"modeCount", "模态数量"},
        {"meshFile", "四面体网格文件"}, {"attachPoints", "连接点编号"},
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
    auto* help = new QLabel(QStringLiteral("%1 [%2] · %3 [%4]\n%5\n可多选同一运动体的完整约束几何。下列坐标为初始全局坐标，单位 mm。")
        .arg(joint.value("name").toString()).arg(joint.value("id").toInt())
        .arg(body.value("name").toString()).arg(bodyId).arg(reason));
    help->setWordWrap(true); layout->addWidget(help);
    auto* filter = new QLineEdit; filter->setPlaceholderText(QStringLiteral("筛选部件名称、边面类型或编号")); layout->addWidget(filter);
    auto* list = new QListWidget; list->setSelectionMode(QAbstractItemView::ExtendedSelection); layout->addWidget(list, 1);
    auto* problem = new QLabel; problem->setWordWrap(true); layout->addWidget(problem);
    try {
        for (int partId : document.bodyPartBindings.value(bodyId)) {
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
        for (const auto* item : list->selectedItems()) selected.append(QJsonObject{
            {"partId", item->data(Qt::UserRole).toInt()}, {"index", item->data(Qt::UserRole + 1).toInt()}});
        selection = selected; dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted;
}

QJsonObject bodyRecord(const CadDocument& cad, const RigidBody& body, QString& error)
{
    BodyProperties properties;
    if (!rigidBodyProperties(cad, body, properties, error)) return {};
    QJsonArray inertia;
    for (double v : properties.inertiaKgM2) inertia.append(v * 1e6);
    const auto* part = cad.part(body.partId);
    const auto& c = properties.centerMm;
    return {{"id", body.id}, {"name", body.name}, {"geoEntityIds", QJsonArray{body.partId}},
        {"geoEntityNames", QJsonArray{part ? (part->sourceName.isEmpty() ? part->name : part->sourceName) : QString{}}},
        {"isFixed", body.fixed}, {"isCustomMassProps", true}, {"mass", properties.massKg},
        {"centerOfMass", vec(c[0], c[1], c[2])}, {"inertia", inertia}, {"inertiaConvention", "tensor"},
        {"initialVelocity", vec()}, {"initialAngularVelocity", vec()}};
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
            {"meshFile", ""}, {"attachPoints", QJsonArray{}}});
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
        {"controlMode", {"Position", "Velocity"}}, {"motionType", {"Rotation", "Translation"}},
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
                   const NativeMotionDocument& doc, const QString& collection, int depth = 0)
{
    const QString text = label(key);
    if (value.isObject() && depth < 8) {
        auto* box = new QGroupBox(text);
        auto* nested = new QFormLayout(box);
        nested->setSizeConstraint(QLayout::SetMinimumSize);
        const auto original = value.toObject();
        QMap<QString, ReadValue> fields;
        for (auto it = original.begin(); it != original.end(); ++it) fields.insert(it.key(), addField(nested, it.key(), it.value(), doc, collection, depth + 1));
        form->addRow(box);
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
        combo->setCurrentIndex(index); form->addRow(text, combo); return [combo] { return combo->currentData().toInt(); };
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
        if (QStringList{"jointType", "couplingType", "springType", "sensorType", "loadType"}.contains(key)) combo->setEnabled(false);
        form->addRow(text, combo); return [combo] { return combo->currentData().toString(); };
    }
    if (value.isString() || value.isDouble()) {
        const QString original = value.isDouble() ? QString::number(value.toDouble(), 'g', 17) : value.toString();
        auto* edit = new QLineEdit(original); edit->setObjectName("native_" + key); edit->setMinimumWidth(70);
        form->addRow(text, edit);
        return [edit, value, original, text] {
            if (edit->text() == original) return value;
            if (value.isString()) return QJsonValue(edit->text());
            bool ok = false; const double number = edit->text().toDouble(&ok);
            if (!ok || !std::isfinite(number)) throw std::runtime_error((text + QStringLiteral("：请输入有限数值")).toStdString());
            return QJsonValue(number);
        };
    }
    if (value.isArray() && value.toArray().size() <= 1000) {
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
            QObject::connect(add, &QPushButton::clicked, table, [table] { if (table->rowCount() < 1000) {
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

} // namespace

void MotionWindow::createNativeCommands()
{
    auto add = [this](Command command, const QString& text, const char* icon, const std::function<void()>& action) {
        auto* item = new QAction(QIcon(QStringLiteral(":/mMainWidgets/Icon/Motion/") + QLatin1String(icon)), text, this);
        m_commands.emplace(command, item); connect(item, &QAction::triggered, this, action);
    };
    add(Command::ImportMotionJson, tr("导入运动 JSON"), "movingBody.png", [this] {
        const auto path = QFileDialog::getOpenFileName(this, tr("导入运动数据"), {}, tr("Motion 运动数据 (*.motion.json *.json)"));
        if (!path.isEmpty()) importMotionJson(path);
    });
    add(Command::ExportMotionJson, tr("导出运动 JSON"), "movingBody.png", [this] {
        if (!ensureNativeModel()) return;
        for (const auto& scenario : m_model.scenarios) syncNativeScenarioCurves(m_model, scenario.id);
        auto path = QFileDialog::getSaveFileName(this, tr("导出运动数据"), "model.motion.json", tr("Motion 运动数据 (*.motion.json)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".json", Qt::CaseInsensitive)) path += ".motion.json";
        QString error;
        if (writeNativeMotionDocument(path, *m_model.nativeDocument, error)) setNotice(tr("已导出运动数据：%1").arg(QFileInfo(path).fileName()));
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

bool MotionWindow::resolveNativeBindings(NativeMotionDocument& document, bool interactive)
{
    QVector<NativeMotionBindingIssue> issues;
    autoBindNativeMotionBodies(m_document, document, issues);
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
        auto* parts = partList(m_document, document, issue.bodyId); parts->setMaximumHeight(100000); layout->addWidget(parts, 1);
        connect(filter, &QLineEdit::textChanged, parts, [parts](const QString& text) {
            for (int i = 0; i < parts->count(); ++i) parts->item(i)->setHidden(!parts->item(i)->text().contains(text, Qt::CaseInsensitive)); });
        auto* problem = new QLabel; problem->setWordWrap(true); layout->addWidget(problem);
        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
            QString error;
            if (setNativeMotionBodyBinding(m_document, document, issue.bodyId, selectedParts(parts), error)) dialog.accept();
            else problem->setText(error);
        });
        if (dialog.exec() != QDialog::Accepted) return false;
    }
    QString error;
    if (!validateNativeMotionBindings(m_document, document, error)) { setNotice(error, true); return false; }
    QVector<NativeTopologyBindingIssue> topologyIssues; QStringList topologyEvidence;
    autoBindNativeMotionTopology(m_document, document, topologyIssues, &topologyEvidence);
    if (!topologyIssues.isEmpty() && !interactive) {
        QStringList messages; for (const auto& issue : topologyIssues) messages.append(issue.message);
        setNotice(messages.join(QLatin1Char('\n')), true); return false;
    }
    for (const auto& issue : topologyIssues) {
        auto joint = document.entity("joints", issue.jointId); auto selection = joint.value(issue.selectorField).toArray();
        if (!selectConstraintTopology(this, m_document, document, joint, issue.selectorField, selection, issue.message)) return false;
        joint.insert(issue.selectorField, selection); put(document, "joints", joint);
    }
    if (!validateNativeMotionDocument(document, error)) { setNotice(error, true); return false; }
    return true;
}

bool MotionWindow::importMotionJson(const QString& path, bool interactiveBinding)
{
    if (motionBusy() || m_jobPart >= 0) return false;
    if (m_document.parts().empty()) { setNotice(tr("请先导入与运动数据配套的几何文件。"), true); return false; }
    NativeMotionDocument document; QString error;
    if (!readNativeMotionDocument(path, document, error)) { setNotice(error, true); return false; }
    if (!resolveNativeBindings(document, interactiveBinding)) return false;
    const int bodies = document.collection("bodies").size(), joints = document.collection("joints").size();
    RigidModel imported;
    imported.nativeDocument = std::make_shared<NativeMotionDocument>(std::move(document));
    syncNativeMotionModel(imported); // A new source must not inherit old scenario requests with the same ID.
    commitRigid(std::move(imported), {});
    setNotice(tr("已导入 %1：%2 个运动体、%3 个运动副。几何绑定已核对，可选择求解方案运行。")
        .arg(QFileInfo(path).fileName()).arg(bodies).arg(joints)); return true;
}

void MotionWindow::appendNativeNodes(QTreeWidgetItem* root, const std::map<QString, QTreeWidgetItem*>& groups)
{
    const QMap<QString, QString> groupNames{{"bodies", tr("运动体")}, {"autoFlexBodies", tr("柔性体")},
        {"joints", tr("运动副")}, {"couplings", tr("耦合副")}, {"drives", tr("驱动")}, {"springs", tr("弹簧")},
        {"vectorForces", tr("力元")}, {"scalarForces", tr("力元")}, {"scalarTorques", tr("力元")},
        {"threePointForces", tr("力元")}, {"threePointTorques", tr("力元")}, {"tensionCables", tr("力元")}, {"softStops", tr("力元")},
        {"sensors", tr("传感器")}, {"controllers", tr("控制器")}, {"markers", tr("标记点")}, {"contacts", tr("接触")}, {"controlInputs", tr("控制输入")}};
    for (auto it = groupNames.begin(); it != groupNames.end(); ++it) {
        auto* group = groups.at(it.value());
        for (const auto& value : m_model.nativeDocument->collection(it.key())) {
            const auto row = value.toObject(); const int id = row.value("id").toInt();
            auto* item = new QTreeWidgetItem(group, {row.value("name").toString()});
            item->setToolTip(0, label(it.key()) + QStringLiteral(" [%1]").arg(id));
            item->setData(0, Qt::UserRole, id); item->setData(0, Qt::UserRole + 1, int(ModelNode::Native)); item->setData(0, Qt::UserRole + 2, it.key());
            if (m_selectionNode.kind == ModelNode::Native && m_selectionNode.id == id && m_selectionNode.collection == it.key()) m_tree->setCurrentItem(item);
        }
        if (group->childCount()) group->setExpanded(true);
    }
    for (const auto& scenario : m_model.scenarios) {
        auto* item = new QTreeWidgetItem(root, {scenario.name}); item->setData(0, Qt::UserRole, scenario.id);
        item->setData(0, Qt::UserRole + 1, int(ModelNode::Scenario)); item->setExpanded(true);
        if (m_selectionNode.kind == ModelNode::Scenario && m_selectionNode.id == scenario.id) m_tree->setCurrentItem(item);
        for (const auto& child : {std::pair{tr("动画"), ModelNode::Animation}, {tr("结果导出"), ModelNode::Export}, {tr("运动曲线"), ModelNode::Curves}}) {
            auto* leaf = new QTreeWidgetItem(item, {child.first}); leaf->setData(0, Qt::UserRole, scenario.id);
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
        const auto row = bodyRecord(m_document, {id, part.name, part.id, false, 1.0, 0.0}, error);
        if (row.isEmpty()) continue;
        put(document, "bodies", row); document.bodyPartBindings.insert(id, {part.id}); ++added;
    }
    if (added) commitNativeDocument(std::move(document), {});
    setNotice(tr("已生成 %1 个运动体。").arg(added));
}

void MotionWindow::editNativeEntity(const QString& collection, int id, const QString& requestedType)
{
    if (motionBusy() || m_jobPart >= 0 || !m_model.nativeDocument) return;
    auto document = *m_model.nativeDocument;
    QJsonObject row = document.entity(collection, id);
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
            QString error; row = bodyRecord(m_document, {nextId(document, collection), part->name, part->id, false, 1, 0}, error);
            if (row.isEmpty()) { setNotice(error, true); return; }
            document.bodyPartBindings.insert(row.value("id").toInt(), {part->id});
        }
    }
    id = row.value("id").toInt();
    // Add optional defaults when the older document omitted editable fields.
    const auto defaults = templateRecord(document, collection, row.value(collection == "joints" ? "jointType" : collection == "springs" ? "springType" : "").toString());
    for (const auto& key : {QStringLiteral("friction"), QStringLiteral("limit"), QStringLiteral("occSurfaceFaces"),
            QStringLiteral("occCurveEdges"), QStringLiteral("occCurveIEdges"), QStringLiteral("occCurveJEdges")})
        if (!row.contains(key) && defaults.contains(key)) row.insert(key, defaults.value(key));
    const bool missingMaterial = collection == "autoFlexBodies" && !row.contains("material");
    if (missingMaterial) row.insert("material", defaults.value("material"));
    if (collection == "bodies" && !row.contains("inertiaConvention")) row.insert("inertiaConvention", "auto");
    auto* panel = new QWidget; panel->setObjectName("nativeEditor_" + collection);
    auto* form = new QFormLayout(panel); form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setSizeConstraint(QLayout::SetMinimumSize);
    auto* title = new QLabel(label(collection)); title->setStyleSheet(QStringLiteral("font-weight:600")); form->addRow(title);
    if (collection == "bodies") {
        auto* help = new QLabel(tr("惯量顺序为 Ixx、Iyy、Izz、Ixy、Ixz、Iyz。矩阵项与正号惯量积的后三项符号相反；自动模式依据原文档的明确公式或配套几何确认约定。"));
        help->setWordWrap(true); form->addRow(help);
    }
    if (collection == "autoFlexBodies") {
        auto* help = new QLabel(missingMaterial
            ? tr("原运动数据未包含材料参数。下面预填标准钢参数，请按实际材料核对后保存；留空网格文件时会由几何生成四面体网格。")
            : tr("模态求解使用下列材料参数。留空网格文件时会由几何生成四面体网格。"));
        help->setWordWrap(true); form->addRow(help);
    }
    if (collection == "softStops" || collection == "tensionCables") {
        auto* help = new QLabel(collection == "tensionCables"
            ? tr("绳索松弛时力为零，仅产生拉力。作用点使用初始全局坐标；长度单位 mm，刚度 N/mm，阻尼 N·s/mm。")
            : tr("沿方向投影计算止挡间隙，仅在越过止挡时受力。刚度单位 N/mm^指数，阻尼 N·s/mm；表格两列分别为压入量 mm 和力 N。"));
        help->setWordWrap(true); form->addRow(help);
    }
    QListWidget* parts = nullptr;
    if (collection == "bodies") { parts = partList(m_document, document, id); form->addRow(tr("绑定几何 (可多选)"), parts); }
    QMap<QString, ReadValue> fields;
    const QStringList hidden{"id", "isUserRename", "geoEntityIds", "geoEntityNames", "currentCurveRequest", "curveNode", "curveRequests",
        "legacyEndValue", "driveType", "targetType", "surfaceFaceIds", "curveEdgeIds", "curveIEdgeIds", "curveJEdgeIds"};
    if (row.contains("name")) fields.insert("name", addField(form, "name", row.value("name"), document, collection));
    for (auto it = row.begin(); it != row.end(); ++it) {
        if (it.key() == "name" || hidden.contains(it.key())) continue;
        if (collection == "joints" && isTopologyField(it.key())) {
            const auto field = it.key(); const bool faces = field == "occSurfaceFaces";
            auto selection = std::make_shared<QJsonArray>(it.value().toArray());
            auto* selected = new QLabel(topologySummary(*selection, faces)); selected->setWordWrap(true); form->addRow(label(field), selected);
            auto* choose = new QPushButton(tr("选择约束%1…").arg(faces ? tr("面") : tr("边")));
            choose->setObjectName("native_select_" + field); form->addRow(choose);
            connect(choose, &QPushButton::clicked, panel, [this, panel, selected, selection, document, row, field, faces] {
                auto joint = row; const auto bodyField = topologyBodyField(field);
                if (auto* combo = panel->findChild<QComboBox*>("native_" + bodyField)) joint.insert(bodyField, combo->currentData().toInt());
                if (selectConstraintTopology(this, m_document, document, joint, field, *selection)) selected->setText(topologySummary(*selection, faces));
            });
            fields.insert(field, [selection] { return *selection; });
        } else fields.insert(it.key(), addField(form, it.key(), it.value(), document, collection));
    }
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); form->addRow(buttons);
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("motionApplyButton"));
    connect(buttons, &QDialogButtonBox::rejected, this, [this] { closeEditor(); updateParameters(); });
    connect(buttons, &QDialogButtonBox::accepted, this, [this, document, row, fields, parts, collection, id]() mutable {
        try {
            auto next = row;
            for (auto it = fields.begin(); it != fields.end(); ++it) next.insert(it.key(), it.value()());
            if (next.value("expression") != row.value("expression")) next.remove("legacyEndValue");
            put(document, collection, next);
            QString error;
            if (parts && !setNativeMotionBodyBinding(m_document, document, id, selectedParts(parts), error)) { setNotice(error, true); return; }
            commitNativeDocument(document, collection == "scenarios" ? ModelNode{ModelNode::Scenario, id}
                : ModelNode{ModelNode::Native, id, collection});
        } catch (const std::exception& error) { setNotice(QString::fromUtf8(error.what()), true); }
    });
    // Long native records must scroll at their natural row height. Otherwise
    // the dock squeezes nested vector/table groups down to their title bars,
    // hiding the actual parameter controls even though they still hold values.
    auto* scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("nativeParameterScroll"));
    scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame);
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
