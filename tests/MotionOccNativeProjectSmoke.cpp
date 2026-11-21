#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "RigidProject.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

namespace {
void require(bool condition, const QString& error)
{
    if (!condition) throw std::runtime_error(error.toStdString());
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    using namespace MotionOcc;
    try {
        QTemporaryDir temporary;
        CadDocument cad;
        QString error;
        int first = 0, second = 0;
        require(cad.addPrimitive(Primitive::Box, 4, 6, 8, first, error), error);
        require(cad.addPrimitive(Primitive::Sphere, 2, 0, 0, second, error), error);
        NativeMotionDocument native;
        const QJsonObject origin{{"x", 0}, {"y", 0}, {"z", 0}};
        const QJsonObject body{{"id", 0}, {"name", "多个几何的刚体"}, {"mass", 3}, {"isFixed", false},
            {"isCustomMassProps", true}, {"centerOfMass", origin}, {"inertia", QJsonArray{10, 11, 12, 0, 0, 0}},
            {"geoEntityIds", QJsonArray{301, 902}}, {"customOriginalExtension", QJsonObject{{"keep", true}}}};
        const QJsonObject scenario{{"id", 0}, {"name", "完整运动方案"}, {"endTime", .02}, {"stepSize", .001}, {"gravity", origin}};
        const QJsonObject cable{{"id", 8}, {"name", "只拉绳索"}, {"bodyIId", 0}, {"bodyJId", -1},
            {"localI", origin}, {"localJ", QJsonObject{{"x", 10}, {"y", 0}, {"z", 0}}},
            {"freeLength", 10}, {"stiffness", 2}, {"damping", .1}, {"engagementWidth", .01}};
        native.root = {{"formatVersion", 1}, {"userExtension", "保持完整"}, {"motionData", QJsonObject{
            {"bodies", QJsonArray{body}}, {"scenarios", QJsonArray{scenario}}, {"tensionCables", QJsonArray{cable}}}}};
        native.bodyPartBindings.insert(0, {first, second});
        native.baseDirectory = temporary.path();
        RigidModel model;
        model.nativeDocument = std::make_shared<NativeMotionDocument>(native);
        syncNativeMotionModel(model);
        model.scenarios.front().curveRequests = {
            {1, RigidCurveMetric::NativeChannel, 0, -1, "体速度幅值", "#2176c1", true, "bodies", "Velocity"},
            {2, RigidCurveMetric::NativeChannel, 8, 0, "绳索拉力", "#d35f39", false, "tensionCables", "Tension"},
            {3, RigidCurveMetric::NativeChannel, 19, 17, "第18阶模态", "#24946b", false, "autoFlexBodies", "ModalCoordinates"}};
        const auto path = temporary.filePath("native.mxmotion");
        require(saveRigidProject(path, cad, model, error), error);
        CadDocument restored;
        RigidModel next;
        require(loadRigidProject(path, restored, next, error), error);
        require(next.nativeDocument && next.nativeDocument->root == native.root, "Native project lost an original record or extension");
        require(next.nativeDocument->bodyPartBindings == native.bodyPartBindings, "Native project changed the CAD bindings");
        require(next.nativeDocument->baseDirectory == native.baseDirectory, "Native project lost relative asset resolution");
        require(next.bodies.size() == 1 && next.bodies.front().id == 0 && next.scenarios.size() == 1
            && next.scenarios.front().id == 0 && restored.parts().size() == 2, "Native project projection lost zero IDs or multiple parts");
        const auto& requests = next.scenarios.front().curveRequests;
        require(requests.size() == 3 && requests[0].component == -1 && requests[0].sourceType == "bodies"
            && requests[1].sourceId == 8 && !requests[1].visible && requests[2].component == 17,
            "Native project lost stable curve keys, magnitude or modal component");
        // Reject a dangling binding without replacing the current complete model.
        QFile file(path); require(file.open(QIODevice::ReadOnly), file.errorString());
        auto object = QJsonDocument::fromJson(file.readAll()).object(); file.close();
        auto storedModel = object.value("model").toObject(); auto motion = storedModel.value("nativeMotion").toObject();
        motion.insert("bindings", QJsonArray{QJsonObject{{"bodyId", 0}, {"partIds", QJsonArray{999999}}}});
        storedModel.insert("nativeMotion", motion); object.insert("model", storedModel);
        require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), file.errorString()); file.write(QJsonDocument(object).toJson()); file.close();
        require(!loadRigidProject(path, restored, next, error), "Dangling CAD binding was accepted");
        require(next.nativeDocument->root == native.root && restored.parts().size() == 2, "Failed project read damaged the active document");
        QTextStream(stdout) << "Native project regression passed: lossless source, multipart bindings, zero IDs, native curves, atomic failure.\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << '\n'; return 1;
    }
}
