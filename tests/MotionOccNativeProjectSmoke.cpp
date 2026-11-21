#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "RigidProject.h"

#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <TopLoc_Location.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

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

        // The legacy automatic mass exporter used definition-local COMs for
        // some placed STEP parts. Persist the proven identity and native placed
        // geometry while leaving those original dynamics values untouched.
        Handle(TDocStd_Document) step = new TDocStd_Document("BinXCAF");
        const auto tool = XCAFDoc_DocumentTool::ShapeTool(step->Main());
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        const auto assembly = tool->NewShape();
        tool->SetShape(assembly, compound);
        const auto definition = tool->AddShape(cad.parts().front().shape, Standard_False);
        gp_Trsf placement;
        placement.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0.43);
        placement.SetTranslationPart(gp_Vec(100, 200, 300));
        const auto instance = tool->AddComponent(assembly, definition, TopLoc_Location(placement));
        TDataStd_Name::Set(instance, "Placed automatic body");
        tool->UpdateAssemblies();
        STEPCAFControl_Writer writer;
        const auto stepPath = temporary.filePath("local-com.step");
        require(writer.Transfer(step, STEPControl_AsIs) && writer.Write(stepPath.toUtf8().constData()) == IFSelect_RetDone,
            "Could not write the local COM project fixture");
        CadDocument localCad;
        std::vector<int> imported;
        require(localCad.importFile(stepPath, imported, error), error);
        require(imported.size() == 1, QStringLiteral("Local COM STEP fixture imported %1 parts instead of one").arg(imported.size()));
        auto localNative = native;
        localNative.bodyPartBindings.clear();
        auto localBody = body;
        localBody["name"] = "局部质心自动运动体";
        localBody["isCustomMassProps"] = false;
        localBody["geoEntityIds"] = QJsonArray{301};
        localBody["centerOfMass"] = QJsonObject{{"x", 2}, {"y", 3}, {"z", 4}};
        localBody["inertia"] = QJsonArray{25, 20, 13, 0, 0, 0}; // mass 3, box 4 x 6 x 8.
        auto localData = localNative.root.value("motionData").toObject();
        localData["bodies"] = QJsonArray{localBody};
        localNative.root["motionData"] = localData;
        const auto localRoot = localNative.root;
        QVector<NativeMotionBindingIssue> bindingIssues;
        require(autoBindNativeMotionBodies(localCad, localNative, bindingIssues)
            && localNative.bodyPartBindings.value(0) == QVector<int>{imported.front()} && localNative.root == localRoot,
            "Local COM project fixture did not establish a lossless STEP binding");
        RigidModel localModel;
        localModel.nativeDocument = std::make_shared<NativeMotionDocument>(localNative);
        syncNativeMotionModel(localModel);
        const auto localPath = temporary.filePath("local-com.mxmotion");
        require(saveRigidProject(localPath, localCad, localModel, error), error);
        CadDocument localRestored;
        RigidModel localNext;
        require(loadRigidProject(localPath, localRestored, localNext, error), error);
        require(localNext.nativeDocument && localNext.nativeDocument->root == localRoot
            && localNext.nativeDocument->bodyPartBindings == localNative.bodyPartBindings,
            "Project persistence changed local COM, inertia, mass or their proven geometry binding");
        GProp_GProps restoredProperties;
        BRepGProp::VolumeProperties(localRestored.parts().front().shape, restoredProperties);
        require(restoredProperties.CentreOfMass().Distance(gp_Pnt(2, 3, 4).Transformed(placement)) < 1e-8,
            "Saving a local-COM binding silently moved the CAD assembly");
        localNext.nativeDocument->bodyPartBindings.clear();
        QMap<int, QString> bindingEvidence;
        require(autoBindNativeMotionBodies(localRestored, *localNext.nativeDocument, bindingIssues, &bindingEvidence)
            && localNext.nativeDocument->root == localRoot && bindingEvidence.value(0).contains(QStringLiteral("原始局部质心")),
            "Project BREP lost the native STEP placement required to re-identify a local COM");
        QTextStream(stdout) << "Native project regression passed: lossless source, multipart bindings, zero IDs, native curves, atomic failure.\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << '\n'; return 1;
    }
}
