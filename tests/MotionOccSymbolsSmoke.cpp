#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSymbols.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>

#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool condition, const QString& message) {
    if (!condition) throw std::runtime_error(message.toStdString());
}
void expectNear(double value, double expected, double tolerance, const QString& name) {
    require(std::isfinite(value) && std::abs(value - expected) <= tolerance,
            QStringLiteral("%1: %2 != %3").arg(name).arg(value, 0, 'g', 17).arg(expected, 0, 'g', 17));
}
QJsonObject xyz(double x, double y, double z) {
    return {{"x", x}, {"y", y}, {"z", z}};
}
const MotionSymbol& find(const QVector<MotionSymbol>& symbols, const QString& collection, int id) {
    for (const auto& symbol : symbols)
        if (symbol.collection == collection && symbol.id == id) return symbol;
    throw std::runtime_error(QStringLiteral("symbol missing: %1:%2").arg(collection).arg(id).toStdString());
}
bool contains(const QVector<MotionSymbol>& symbols, const QString& collection, int id = -1) {
    for (const auto& symbol : symbols)
        if (symbol.collection == collection && (id < 0 || symbol.id == id)) return true;
    return false;
}
} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    try {
        CadDocument cad;
        int part = 0;
        QString error;
        require(cad.addPrimitive(Primitive::Box, 10, 20, 30, part, error), error);

        NativeMotionDocument doc;
        const QJsonObject body{{"id", 7}, {"name", "body"}, {"centerOfMass", xyz(5, 10, 15)}};
        const QJsonObject joint{{"id", 4}, {"name", "revolute"}, {"jointType", "Revolute"},
            {"actionBodyId", 7}, {"baseBodyId", -1}, {"origin", xyz(2, 3, 4)}, {"axis", xyz(0, 0, 1)}};
        auto secondJoint = joint;
        secondJoint["id"] = 10;
        const QJsonObject spring{{"id", 2}, {"name", "spring"}, {"springType", "BodyPoint"},
            {"bodyIId", 7}, {"bodyJId", -1}, {"localI", xyz(5, 10, 15)}, {"localJ", xyz(50, 0, 0)}};
        const QJsonObject cable{{"id", 3}, {"name", "cable"}, {"bodyIId", 7}, {"bodyJId", -1},
            {"localI", xyz(1, 2, 3)}, {"localJ", xyz(20, 30, 40)}};
        const QJsonObject force{{"id", 5}, {"name", "force"}, {"loadType", "Force"}, {"bodyId", 7},
            {"actionPoint", xyz(1, 2, 3)}, {"direction", xyz(1, 0, 0)}, {"directionMode", "BodyFixed"}, {"magnitude", 1}};
        auto fixedForce = force;
        fixedForce["id"] = 6;
        fixedForce["directionMode"] = "GlobalFixed";
        auto stop = cable;
        stop["id"] = 9;
        stop["direction"] = xyz(0, 0, 1);
        stop["directionMode"] = 1;
        stop["stopPosition"] = 10;
        stop["side"] = 1;
        const QJsonObject hiddenMarker{{"id", 8}, {"visible", false}, {"bodyId", 7}, {"localPosition", xyz(9, 9, 9)}};
        const QJsonObject cylinder{{"id", 20}, {"name", "boomCyl"}, {"mountMode", "Points"},
            {"bodyIId", 7}, {"bodyJId", -1}, {"localI", xyz(2, 4, 6)}, {"localJ", xyz(0, 0, 0)},
            {"supplyMode", "ExternalCircuit"}, {"portANodeId", 30}};
        const QJsonObject hydNode{{"id", 30}, {"name", "chamberA"}, {"nodeType", "Chamber"}};
        const QJsonObject portConn{{"id", 21}, {"name", "portConn"}, {"componentType", "PortConnection"},
            {"targetNodeId", 30}, {"actuatorType", "Cylinder"}, {"actuatorId", 20}, {"actuatorPort", "A"}};

        const QJsonObject data{
            {"bodies", QJsonArray{body}},
            {"joints", QJsonArray{joint, secondJoint}},
            {"springs", QJsonArray{spring}},
            {"tensionCables", QJsonArray{cable}},
            {"softStops", QJsonArray{stop}},
            {"vectorForces", QJsonArray{force, fixedForce}},
            {"markers", QJsonArray{hiddenMarker}},
            {"hydraulicCylinders", QJsonArray{cylinder}},
            {"hydraulicNodes", QJsonArray{hydNode}},
            {"hydraulicComponents", QJsonArray{portConn}},
            {"sensors", QJsonArray{QJsonObject{{"id", 11}, {"sensorType", "BodyDisplacement"},
                {"targetBodyId", 7}, {"referencePoint", xyz(1, 2, 3)}}}}
        };
        doc.root = {{"motionData", data}};
        doc.bodyPartBindings.insert(7, {part});

        // 1. 默认未选中模型树运动对象时，视口不渲染任何运动符号
        require(nativeMotionSymbols(cad, doc).isEmpty(),
                "default unselected state must not clutter viewport with motion symbols");

        // 2. 点击单个运动副子节点：只显示该子节点，不显示兄弟运动副或无关力元/弹簧/液压缸
        const auto rest = nativeMotionSymbols(cad, doc, {}, "joints", 4);
        const auto& j = find(rest, "joints", 4);
        require(j.emphasized && !j.linePoints.empty(), "selected joint has no highlighted glyph");
        require(rest.size() == 1, "selecting a single joint leaked unrelated symbols");
        expectNear(j.origin[0], 2, 1e-12, "joint global origin");

        // 3. 点击分类父节点：预览该组全部符号且不单点高亮，不泄漏其他组
        const auto jointGroup = nativeMotionSymbols(cad, doc, {}, "joints");
        require(jointGroup.size() == 2 && !find(jointGroup, "joints", 4).emphasized
                && !find(jointGroup, "joints", 10).emphasized,
                "joint group must show all joints without marking them as individually selected");
        const auto bodyGroup = nativeMotionSymbols(cad, doc, {}, "bodies");
        expectNear(find(bodyGroup, "bodies", 7).origin[0], 5, 1e-12, "body global COM");
        const auto forceGroup = nativeMotionSymbols(cad, doc, {}, "forceElements");
        require(contains(forceGroup, "tensionCables", 3) && contains(forceGroup, "vectorForces", 5)
                && contains(forceGroup, "vectorForces", 6) && contains(forceGroup, "softStops", 9)
                && !contains(forceGroup, "joints"),
                "forceElements group must preview all force subtypes only");

        // 4. 点击端口连接或液压节点时，在三维视口中关联显示对应执行器符号作为空间上下文
        const auto portConnCtx = nativeMotionSymbols(cad, doc, {}, "hydraulicComponents", 21);
        require(contains(portConnCtx, "hydraulicCylinders", 20) && !find(portConnCtx, "hydraulicCylinders", 20).emphasized,
                "selecting PortConnection must display its connected hydraulic cylinder as context");
        const auto nodeCtx = nativeMotionSymbols(cad, doc, {}, "hydraulicNodes", 30);
        require(contains(nodeCtx, "hydraulicCylinders", 20),
                "selecting HydraulicNode must display connected hydraulic actuators as context");

        // 5. 全局总览模式 ("*")：显示全部启用且可见的符号，隐藏 visible=false 的标记点
        const auto overview = nativeMotionSymbols(cad, doc, {}, "*");
        require(contains(overview, "bodies", 7) && contains(overview, "joints", 4)
                && contains(overview, "springs", 2) && contains(overview, "hydraulicCylinders", 20)
                && contains(overview, "sensors", 11),
                "global overview (*) missed enabled motion symbols");
        for (const auto& s : overview) {
            require(s.collection != "markers", "hidden marker was displayed");
            require(s.linePoints.size() % 2 == 0, "segments must be paired");
        }

        // 6. 动画变换下按选中子节点分别校验绳索、随体力、全局力、软止挡与传感器坐标
        QMatrix4x4 pose;
        pose.translate(100, 20, 30);
        pose.rotate(90, 0, 0, 1);

        const auto animatedCable = nativeMotionSymbols(cad, doc, {{part, pose}}, "tensionCables", 3);
        require(!contains(animatedCable, "joints") && !contains(animatedCable, "vectorForces"),
                "selecting tensionCable displayed unrelated symbols");
        const auto& c = find(animatedCable, "tensionCables", 3);
        require(c.emphasized && c.partId == -1, "animated glyph would be transformed twice");
        expectNear(c.linePoints[0][0], 98, 1e-5, "animated cable endpoint x");
        expectNear(c.linePoints[0][1], 21, 1e-5, "animated cable endpoint y");
        expectNear(c.linePoints[0][2], 33, 1e-5, "animated cable endpoint z");
        expectNear(c.linePoints[1][0], 20, 1e-12, "ground cable endpoint x");
        expectNear(c.linePoints[1][1], 30, 1e-12, "ground cable endpoint y");

        const auto animatedBodyForce = nativeMotionSymbols(cad, doc, {{part, pose}}, "vectorForces", 5);
        const auto& f = find(animatedBodyForce, "vectorForces", 5);
        require(f.emphasized && f.linePoints.size() >= 2 && !contains(animatedBodyForce, "vectorForces", 6),
                "body-fixed force missing or leaked sibling force");
        expectNear(f.linePoints[1][0] - f.linePoints[0][0], 0, 1e-5, "body-fixed direction x");
        require(f.linePoints[1][1] > f.linePoints[0][1], "body-fixed force did not rotate");

        const auto animatedGlobalForce = nativeMotionSymbols(cad, doc, {{part, pose}}, "vectorForces", 6);
        const auto& fixed = find(animatedGlobalForce, "vectorForces", 6);
        expectNear(fixed.linePoints[1][1] - fixed.linePoints[0][1], 0, 1e-5, "global-fixed direction y");
        require(fixed.linePoints[1][0] > fixed.linePoints[0][0], "global-fixed force rotated with body");

        const auto animatedStop = nativeMotionSymbols(cad, doc, {{part, pose}}, "softStops", 9);
        const auto& limit = find(animatedStop, "softStops", 9);
        expectNear(limit.linePoints[1][0], 20, 1e-12, "stop reference plane x");
        expectNear(limit.linePoints[1][1], 30, 1e-12, "stop reference plane y");
        expectNear(limit.linePoints[1][2], 50, 1e-12, "stop position measured from reference endpoint");

        const auto animatedSensor = nativeMotionSymbols(cad, doc, {{part, pose}}, "sensors", 11);
        const auto& sensor = find(animatedSensor, "sensors", 11);
        expectNear(sensor.origin[0], 88, 1e-5, "sensor COM-local point transformed x");
        expectNear(sensor.origin[1], 26, 1e-5, "sensor COM-local point transformed y");
        expectNear(sensor.origin[2], 48, 1e-5, "sensor COM-local point transformed z");

        const auto source = doc.root;
        require(nativeMotionSymbols(cad, doc, {{part, pose}}).isEmpty(),
                "animation without tree selection or overview toggle displayed symbols");
        require(doc.root == source, "display helper changed solver data");

        // 7. 独立柔性接口体无直接 CAD 部件绑定时，仍跟随 bodyPoses 运动
        auto portDoc = doc;
        portDoc.bodyPartBindings.clear();
        const auto independentJoint = nativeMotionSymbols(cad, portDoc, {}, "joints", 4, {{7, pose}});
        const auto& portJoint = find(independentJoint, "joints", 4);
        expectNear(portJoint.origin[0], 97, 1e-5, "independent port joint x stayed at rest");
        expectNear(portJoint.origin[1], 22, 1e-5, "independent port joint y stayed at rest");
        const auto independentSensor = nativeMotionSymbols(cad, portDoc, {}, "sensors", 11, {{7, pose}});
        expectNear(find(independentSensor, "sensors", 11).origin[2], 48, 1e-5,
                   "independent port sensor did not follow its actual body pose");

        qInfo() << "Native symbols: selection-scoped visibility, group preview, multiphysics context, overview (*), and animated transforms passed";
        return 0;
    } catch (const std::exception& failure) {
        qCritical() << failure.what();
        return 1;
    }
}
