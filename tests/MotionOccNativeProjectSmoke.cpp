#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "NativeTopologyBinding.h"
#include "RigidProject.h"

#include <BRepGProp.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <TopExp.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

namespace {
void require(bool condition, const QString& error)
{
    if (!condition) throw std::runtime_error(error.toStdString());
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), QStringLiteral("Could not read %1: %2").arg(path, file.errorString()));
    return file.readAll();
}

void writeFile(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush(),
        QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
}

void checkPortableAssets(const MotionOcc::CadDocument& cad, MotionOcc::NativeMotionDocument native,
                         const QTemporaryDir& temporary)
{
    using namespace MotionOcc;
    const QDir root(temporary.path());
    require(root.mkpath("source/inputs") && root.mkpath("relocated") && root.mkpath("save-as"),
        "Could not create portable project fixture directories");
    const QDir source(root.filePath("source"));
    const QDir relocated(root.filePath("relocated"));
    const QDir saveAs(root.filePath("save-as"));
    const QString mnfReference = QStringLiteral("inputs/梁.mnf");
    const auto originalMnf = source.filePath(mnfReference);
    const auto originalVol = source.filePath("inputs/mesh.vol");

    // The MNF is disabled in the model. This fixture verifies byte-exact
    // persistence independently of the MNF dynamics parser.
    const auto mnfBytes = QByteArray::fromHex("1e00000000a9ff0d0a00010203");
    writeFile(originalMnf, mnfBytes);
    writeFile(originalVol, "mesh3d\ndimension\n3\npoints\n4\n0 0 0\n1 0 0\n0 1 0\n0 0 1\n"
              "surfaceelements\n4\n1 1 1 0 3 1 2 3\n1 1 1 0 3 1 4 2\n"
              "1 1 1 0 3 1 3 4\n1 1 1 0 3 2 4 3\nvolumeelements\n1\n1 4 1 2 3 4\n"
              "endmesh\nOCC data appended by a mesh generator\n");
    const QJsonObject mnf{{"id", 7}, {"name", "梁.mnf"}, {"targetBodyId", 0}, {"enabled", false},
        {"mnfFile", mnfReference}, {"jointBindings", QJsonObject{}}, {"referenceTranslationMm", QJsonArray{0, 0, 0}},
        {"referenceEuler321Deg", QJsonArray{0, 0, 0}}};
    const QJsonArray attachments{QJsonValue(QJsonArray{0, 1, 2})};
    const QJsonObject autoFlex{{"id", 8}, {"name", "External tetrahedral mesh"}, {"targetBodyId", 0},
        {"enabled", false}, {"meshFile", originalVol}, {"attachGroups", attachments}};
    auto data = native.root.value("motionData").toObject();
    data["importedMnfBodies"] = QJsonArray{mnf};
    data["autoFlexBodies"] = QJsonArray{autoFlex};
    native.root["motionData"] = data;
    native.baseDirectory = source.absolutePath();
    RigidModel authored;
    authored.nativeDocument = std::make_shared<NativeMotionDocument>(native);
    syncNativeMotionModel(authored);

    QString error;
    const auto originalProject = source.filePath("author.mxmotion");
    require(saveRigidProject(originalProject, cad, authored, error), error);
    const auto projectBytes = readFile(originalProject);
    auto project = QJsonDocument::fromJson(projectBytes).object();
    auto modelRecord = project.value("model").toObject();
    auto nativeRecord = modelRecord.value("nativeMotion").toObject();
    const auto assets = nativeRecord.value("assets").toArray();
    require(assets.size() == 1, "Saved project did not include exactly one embedded MNF");
    const auto packedMnf = assets.first().toObject();
    const auto packedName = packedMnf.value("name").toString();
    require(project.value("version").toInt() == 3 && packedName == "mnf-1.mnf"
        && QByteArray::fromBase64(packedMnf.value("data").toString().toLatin1()) == mnfBytes
        && !nativeRecord.contains("baseDirectory") && !projectBytes.contains(source.absolutePath().toUtf8()),
        "Saved project does not contain a self-contained V3 MNF asset");

    // A V2 file must still resolve both relative and absolute references from
    // its existing native base directory before migration.
    auto legacyProject = project;
    auto legacyModel = modelRecord;
    auto legacyNative = nativeRecord;
    legacyNative.remove("assets");
    legacyNative["baseDirectory"] = native.baseDirectory;
    legacyNative["document"] = native.root;
    legacyModel["nativeMotion"] = legacyNative;
    legacyProject["model"] = legacyModel;
    legacyProject["version"] = 2;
    const auto legacyPath = relocated.filePath("legacy-v2.mxmotion");
    writeFile(legacyPath, QJsonDocument(legacyProject).toJson());
    CadDocument legacyCad;
    RigidModel legacy;
    require(loadRigidProject(legacyPath, legacyCad, legacy, error), error);
    require(legacy.nativeDocument && legacy.nativeDocument->baseDirectory == native.baseDirectory
        && legacy.nativeDocument->assetPath(mnfReference) == originalMnf
        && legacy.nativeDocument->assetPath(originalVol) == originalVol
        && readFile(legacy.nativeDocument->assetPath(mnfReference)) == mnfBytes,
        "V2 project did not retain external asset resolution");
    const auto migratedPath = relocated.filePath("migrated-v2.mxmotion");
    require(saveRigidProject(migratedPath, legacyCad, legacy, error), error);

    const auto movedProject = relocated.filePath("移动后.mxmotion");
    require(QFile::rename(originalProject, movedProject) && QFile::remove(originalMnf) && QFile::remove(originalVol),
        "Could not relocate the project and remove both original assets");
    require(!QFileInfo::exists(relocated.filePath(packedName)), "Fixture unexpectedly has a loose MNF file");

    CadDocument loaded;
    RigidModel model;
    require(loadRigidProject(movedProject, loaded, model, error), error);
    require(model.nativeDocument && model.nativeDocument->baseDirectory == relocated.absolutePath(),
        "Relocated project kept the author's directory instead of its containing directory");
    const auto copied = *model.nativeDocument;
    model.nativeDocument.reset();
    const auto reference = copied.entity("importedMnfBodies", 7).value("mnfFile").toString();
    require(reference == packedName && readFile(copied.assetPath(reference)) == mnfBytes,
        "A native document copy did not keep its embedded MNF readable");
    const auto flex = copied.entity("autoFlexBodies", 8);
    const auto mesh = flex.value("mesh").toObject();
    const auto expectedNodes = QJsonArray{QJsonArray{0, 0, 0}, QJsonArray{1, 0, 0},
        QJsonArray{0, 1, 0}, QJsonArray{0, 0, 1}};
    require(flex.value("meshFile").toString().isEmpty() && flex.value("meshSourceFile").toString() == "mesh.vol"
        && flex.value("attachGroups").toArray() == attachments && mesh.value("nodes").toArray() == expectedNodes
        && mesh.value("tetrahedra").toArray() == QJsonArray{QJsonValue(QJsonArray{0, 1, 2, 3})},
        "External mesh coordinates, connectivity or attachment numbering were not retained inline");
    const auto standalonePath = saveAs.filePath("exported.motion.json");
    require(writeNativeMotionDocument(standalonePath, copied, error), error);
    NativeMotionDocument standalone;
    require(readNativeMotionDocument(standalonePath, standalone, error), error);
    const auto exportedMnf = standalone.entity("importedMnfBodies", 7).value("mnfFile").toString();
    require(exportedMnf != reference && QDir::isRelativePath(exportedMnf)
        && readFile(standalone.assetPath(exportedMnf)) == mnfBytes,
        "Standalone Motion JSON exported a dangling reference to a packaged MNF");
    model.nativeDocument = std::make_shared<NativeMotionDocument>(copied);
    const auto savedAgain = saveAs.filePath("saved-as.mxmotion");
    require(saveRigidProject(savedAgain, loaded, model, error), error);
    for (const auto& path : {savedAgain, migratedPath}) {
        CadDocument roundtrippedCad;
        RigidModel roundtripped;
        require(loadRigidProject(path, roundtrippedCad, roundtripped, error), error);
        require(roundtripped.nativeDocument && roundtripped.nativeDocument->entity("autoFlexBodies", 8) == flex
            && readFile(roundtripped.nativeDocument->assetPath(reference)) == mnfBytes,
            "Save As or V2 migration lost the relocated assets");
    }

    const auto priorDocument = model.nativeDocument;
    const auto priorShape = loaded.parts().front().shape;
    const auto badPath = relocated.filePath("bad-assets.mxmotion");
    auto assetWithUnsafeName = packedMnf;
    assetWithUnsafeName["name"] = "../external.mnf";
    for (const QJsonArray& badAssets : {QJsonArray{}, QJsonArray{packedMnf, packedMnf}, QJsonArray{assetWithUnsafeName}}) {
        nativeRecord["assets"] = badAssets;
        modelRecord["nativeMotion"] = nativeRecord;
        project["model"] = modelRecord;
        writeFile(badPath, QJsonDocument(project).toJson());
        require(!loadRigidProject(badPath, loaded, model, error) && !error.isEmpty()
            && model.nativeDocument == priorDocument && loaded.parts().front().shape.IsEqual(priorShape)
            && readFile(model.nativeDocument->assetPath(reference)) == mnfBytes,
            "Invalid V3 assets were accepted or damaged the active project");
    }
    require(!saveRigidProject(movedProject, cad, authored, error) && !error.isEmpty()
        && readFile(movedProject) == projectBytes, "Missing source assets overwrote a valid project");
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

        checkPortableAssets(cad, native, temporary);

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
            "The project lost the native STEP placement required to re-identify a local COM");

        // Edge/face selection identities have to survive a STEP translation of
        // the placed geometry without storing kernel-specific indices.
        const auto partId = imported.front();
        auto curve = nativeTopologyReference(localCad, partId, 1, TopAbs_EDGE);
        auto surface = nativeTopologyReference(localCad, partId, 1, TopAbs_FACE);
        require(!curve.isEmpty() && !surface.isEmpty(), "Missing topology fixture");
        curve["vendorExtension"] = "keep";
        QJsonObject pointJoint{{"id", 9}, {"name", "Point on curve"}, {"jointType", "PointOnCurve"},
            {"actionBodyId", -1}, {"baseBodyId", 0}, {"origin", origin}, {"curveEdgeIds", QJsonArray{3001}},
            {"occCurveEdges", QJsonArray{curve}}};
        QJsonObject surfaceJoint{{"id", 10}, {"name", "Point on surface"}, {"jointType", "PointOnSurface"},
            {"actionBodyId", -1}, {"baseBodyId", 0}, {"origin", origin}, {"surfaceFaceIds", QJsonArray{3002}},
            {"occSurfaceFaces", QJsonArray{surface}}};
        localData["joints"] = QJsonArray{pointJoint, surfaceJoint};
        localModel.nativeDocument->root["motionData"] = localData;
        syncNativeMotionModel(localModel);
        const auto topologyPath = temporary.filePath("topology.mxmotion");
        require(saveRigidProject(topologyPath, localCad, localModel, error), error);
        QFile topologyFile(topologyPath);
        require(topologyFile.open(QIODevice::ReadOnly), topologyFile.errorString());
        auto topologyProject = QJsonDocument::fromJson(topologyFile.readAll()).object();
        topologyFile.close();
        auto topologyModel = topologyProject.value("model").toObject();
        auto storedNative = topologyModel.value("nativeMotion").toObject();
        auto storedDocument = storedNative.value("document").toObject();
        auto storedData = storedDocument.value("motionData").toObject();
        auto storedJoints = storedData.value("joints").toArray();
        auto storedCurve = storedJoints[0].toObject();
        auto storedCurveRef = storedCurve.value("curveGeometry").toArray().first().toObject();
        const auto storedSurface = storedJoints[1].toObject();
        require(!storedCurve.contains("occCurveEdges") && !storedCurveRef.contains("index")
            && storedCurveRef.value("partId").toInt() == partId && !storedSurface.contains("occSurfaceFaces")
            && !storedSurface.value("surfaceGeometry").toArray().first().toObject().contains("index"),
            "Project published an OCC enumeration instead of geometry");
        require(loadRigidProject(topologyPath, localRestored, localNext, error), error);
        const auto restoredCurveJoint = localNext.nativeDocument->entity("joints", 9);
        const auto restoredSurfaceJoint = localNext.nativeDocument->entity("joints", 10);
        const auto restoredCurve = restoredCurveJoint.value("occCurveEdges").toArray().first().toObject();
        const auto restoredSurface = restoredSurfaceJoint.value("occSurfaceFaces").toArray().first().toObject();
        require(restoredCurveJoint.value("curveEdgeIds") == pointJoint.value("curveEdgeIds")
            && restoredSurfaceJoint.value("surfaceFaceIds") == surfaceJoint.value("surfaceFaceIds")
            && restoredCurve.value("vendorExtension").toString() == "keep", "Topology conversion lost source data");
        const auto checkSelection = [&](const QJsonObject& original, const QJsonObject& loaded, TopAbs_ShapeEnum kind) {
            TopTools_IndexedMapOfShape before;
            TopTools_IndexedMapOfShape after;
            TopExp::MapShapes(localCad.part(partId)->shape, kind, before);
            TopExp::MapShapes(localRestored.part(partId)->shape, kind, after);
            const int index = loaded.value("index").toInt();
            require(index > 0 && index <= after.Extent() && loaded.value("partId").toInt() == partId,
                "Rebound project topology was invalid");
            GProp_GProps expected, actual;
            const auto properties = [kind](const TopoDS_Shape& shape, GProp_GProps& result) {
                if (kind == TopAbs_EDGE) BRepGProp::LinearProperties(shape, result);
                else BRepGProp::SurfaceProperties(shape, result);
            };
            properties(before(original.value("index").toInt()), expected);
            properties(after(index), actual);
            require(expected.CentreOfMass().Distance(actual.CentreOfMass()) < 1e-8
                && qAbs(expected.Mass() - actual.Mass()) < 1e-8, "STEP changed the selected edge or face");
        };
        checkSelection(curve, restoredCurve, TopAbs_EDGE);
        checkSelection(surface, restoredSurface, TopAbs_FACE);

        auto signature = storedCurveRef.value("geometry").toObject();
        signature["measure"] = 1234567.;
        storedCurveRef["geometry"] = signature;
        storedCurve["curveGeometry"] = QJsonArray{storedCurveRef};
        storedJoints[0] = storedCurve;
        storedData["joints"] = storedJoints;
        storedDocument["motionData"] = storedData;
        storedNative["document"] = storedDocument;
        topologyModel["nativeMotion"] = storedNative;
        topologyProject["model"] = topologyModel;
        require(topologyFile.open(QIODevice::WriteOnly | QIODevice::Truncate), topologyFile.errorString());
        topologyFile.write(QJsonDocument(topologyProject).toJson());
        topologyFile.close();
        require(!loadRigidProject(topologyPath, localRestored, localNext, error)
            && localNext.nativeDocument->entity("joints", 9) == restoredCurveJoint
            && localRestored.parts().size() == 1, "An unmatched topology signature damaged the active document");
        QTextStream(stdout) << "Native project regression passed: lossless source, multipart bindings, zero IDs, native curves, topology roundtrip, portable assets, atomic failure.\n";
        return 0;
    } catch (const std::exception& error) {
        QTextStream(stderr) << error.what() << '\n'; return 1;
    }
}
