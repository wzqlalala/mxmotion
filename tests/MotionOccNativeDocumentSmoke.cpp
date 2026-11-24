#include "NativeMotionDocument.h"
#include "NativeMotionExchange.h"
#include "NativeTopologyBinding.h"
#include "CadDocument.h"

#include <BRepGProp.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <TopoDS_Compound.hxx>
#include <TopLoc_Location.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <gp_Mat.hxx>
#include <gp_Ax1.hxx>
#include <gp_Trsf.hxx>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTextStream>

#include <functional>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

QJsonObject vector(double x, double y, double z) { return {{"x", x}, {"y", y}, {"z", z}}; }

QJsonObject body(int id, const QJsonArray& sourceIds, const CadPart& part)
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(part.shape, props);
    const auto center = props.CentreOfMass();
    const auto tensor = props.MatrixOfInertia();
    const double mass = 2.0;
    const double scale = mass / props.Mass();
    return {{"id", id}, {"name", QStringLiteral("原始运动体 %1 é").arg(id)}, {"geoEntityIds", sourceIds},
        {"mass", mass}, {"isCustomMassProps", true}, {"isFixed", false},
        {"centerOfMass", vector(center.X(), center.Y(), center.Z())},
        {"inertia", QJsonArray{tensor.Value(1, 1) * scale, tensor.Value(2, 2) * scale, tensor.Value(3, 3) * scale,
            tensor.Value(1, 2) * scale, tensor.Value(1, 3) * scale, tensor.Value(2, 3) * scale}},
        {"initialVelocity", vector(1, 2, 3)}, {"initialAngularVelocity", vector(0, 90, 0)},
        {"futureExtension", QJsonObject{{"keep", QStringLiteral("完整保留")}}}};
}

NativeMotionDocument fixture(const CadDocument& cad)
{
    QJsonObject data;
    data["bodies"] = QJsonArray{body(0, {91}, cad.parts()[0]), body(7, {42}, cad.parts()[1])};
    data["joints"] = QJsonArray{QJsonObject{{"id", 100}, {"name", "hinge"}, {"jointType", "Revolute"},
        {"actionBodyId", 0}, {"baseBodyId", -1}, {"origin", vector(2, 3, 4)}, {"axis", vector(0, 0, 1)},
        {"friction", QJsonObject{{"enabled", true}, {"staticCoefficient", 0.2}, {"effectiveRadius", 3}}},
        {"limit", QJsonObject{{"enabled", true}, {"lowerEnabled", true}, {"upperEnabled", true}, {"lowerValue", -45}, {"upperValue", 45}}}}};
    data["drives"] = QJsonArray{QJsonObject{{"id", 3}, {"name", "motor"}, {"driveType", "JointDrive"}, {"targetType", "Joint"},
        {"targetJointId", 100}, {"motionType", "Rotation"}, {"controlMode", "Velocity"}, {"expression", "90*sin(pi*t)"}}};
    data["markers"] = QJsonArray{QJsonObject{{"id", 12}, {"name", "point"}, {"bodyId", 0}, {"localPosition", vector(4, 5, 6)},
        {"localRotation", QJsonObject{{"w", 1}, {"x", 0}, {"y", 0}, {"z", 0}}}}};
    data["springs"] = QJsonArray{QJsonObject{{"id", 1}, {"name", "spring"}, {"springType", "BodyPoint"},
        {"bodyIId", 0}, {"bodyJId", 7}, {"localI", vector(1, 2, 3)}, {"localJ", vector(4, 5, 6)},
        {"stiffness", 4}, {"damping", 0.1}, {"freeValue", 5}, {"springDirectionMode", "PositiveOnly"}}};
    data["sensors"] = QJsonArray{QJsonObject{{"id", 5}, {"name", "sensor"}, {"sensorType", "BodyVelocity"},
        {"targetBodyId", 0}, {"referenceBodyId", -1}, {"referencePoint", vector(3, 4, 5)},
        {"coordSystem", "GlobalFixed"}, {"componentMode", "X"}, {"variable", "Velocity"}}};
    data["controlInputs"] = QJsonArray{QJsonObject{{"id", 2}, {"inputType", "Cubic"}, {"polyCoeffs", QJsonArray{1, 2, 3, 4}}, {"timeStart", 0}, {"timeEnd", 2}}};
    data["controllers"] = QJsonArray{QJsonObject{{"id", 3}, {"name", "controller"}, {"controllerType", "PID"},
        {"targetDriveId", 3}, {"feedbackSource", "Sensor"}, {"feedbackSensorId", 5}, {"targetInputId", 2},
        {"kp", 1.2}, {"ki", 0.2}, {"kd", 0.01}, {"targetValue", 10}, {"outputLimit", 30}}};
    data["autoFlexBodies"] = QJsonArray{QJsonObject{{"id", 8}, {"name", "optional flexible body"}, {"targetBodyId", 7}, {"enabled", false},
        {"materialId", 23}, {"attachPoints", QJsonArray{17, 19}}, {"modeCount", 12}, {"meshType", 0}, {"outputStressStrain", true}}};
    data["softStops"] = QJsonArray{QJsonObject{{"id", 1}, {"name", "stop"}, {"bodyIId", 0}, {"bodyJId", -1},
        {"localI", vector(1, 2, 3)}, {"localJ", vector(1, 2, 4)}, {"direction", vector(0, 0, 1)}, {"directionMode", 1},
        {"side", 1}, {"stopPosition", 10}, {"stiffness", 5}, {"exponent", 1.5}, {"damping", 0.2}, {"dampingDepth", 0.3}}};
    data["tensionCables"] = QJsonArray{QJsonObject{{"id", 1}, {"name", "cable"}, {"bodyIId", 0}, {"bodyJId", 7},
        {"localI", vector(1, 2, 3)}, {"localJ", vector(4, 5, 6)}, {"freeLength", 5}, {"stiffness", 5}, {"damping", 0.2}, {"engagementWidth", 0.1}}};
    data["scenarios"] = QJsonArray{QJsonObject{{"id", 9}, {"name", "native"}, {"endTime", 2}, {"stepSize", 0.01},
        {"gravity", vector(0, 0, -9810)}, {"presetMode", "FAST"}, {"trackedMarkerIds", QJsonArray{12}},
        {"curveRequests", QJsonArray{QJsonObject{{"requestId", 1}, {"requestName", "velocity"}, {"sourceType", "Body"},
            {"sourceId", 0}, {"variable", "Velocity"}, {"component", "X"}, {"visible", false},
            {"color", QJsonObject{{"r", 2}, {"g", 30}, {"b", 55}, {"a", 128}}}}}}}};
    NativeMotionDocument out;
    out.root = {{"formatVersion", 1}, {"exportTime", "2026-01-02T03:04:05"}, {"motionData", data},
        {"vendorExtension", QJsonObject{{"nested", QJsonArray{1, "unchanged", QJsonObject{{"yes", true}}}}}}};
    return out;
}

void change(NativeMotionDocument& doc, const QString& collection, const std::function<void(QJsonObject&)>& edit)
{
    auto data = doc.root.value("motionData").toObject();
    auto records = data.value(collection).toArray();
    auto record = records.first().toObject();
    edit(record);
    records[0] = record;
    data[collection] = records;
    doc.root["motionData"] = data;
}

void geometryExchange(const QTemporaryDir& temp)
{
    CadDocument source;
    QString error;
    int removed, first, second;
    require(source.addPrimitive(Primitive::Box, 2, 2, 2, removed, error), error);
    require(source.addPrimitive(Primitive::Box, 10, 20, 30, first, error), error);
    require(source.addPrimitive(Primitive::Box, 10, 20, 30, second, error) && source.remove(removed), error);
    // The two remaining parts coincide. Their explicit bindings must survive
    // export even though geometry comparison alone cannot distinguish them.
    NativeMotionDocument original;
    const QJsonObject curve{{"id", 71}, {"name", "Curve"}, {"jointType", "PointOnCurve"}, {"actionBodyId", 0},
        {"baseBodyId", 7}, {"origin", vector(0, 0, 0)}, {"curveEdgeIds", QJsonArray{4221}},
        {"occCurveEdges", QJsonArray{nativeTopologyReference(source, first, 1, TopAbs_EDGE)}}};
    const QJsonObject surface{{"id", 72}, {"name", "Surface"}, {"jointType", "PointOnSurface"}, {"actionBodyId", -1},
        {"baseBodyId", 0}, {"origin", vector(0, 0, 0)}, {"surfaceFaceIds", QJsonArray{788}},
        {"occSurfaceFaces", QJsonArray{nativeTopologyReference(source, second, 1, TopAbs_FACE)}}};
    original.root = {{"formatVersion", 1}, {"motionData", QJsonObject{
        {"bodies", QJsonArray{body(0, {91}, source.parts()[1]), body(7, {42}, source.parts()[0])}},
        {"joints", QJsonArray{curve, surface}}}}};
    original.bodyPartBindings = {{0, {second}}, {7, {first}}};
    original.ownsTopologyCad = true;

    const auto path = temp.filePath(QStringLiteral("相对几何.motion.json"));
    require(writeNativeMotionExchange(path, source, original, error), error);
    NativeMotionDocument packed;
    require(readNativeMotionDocument(path, packed, error), error);
    auto exchange = packed.root.value("geometryExchange").toObject();
    require(exchange.value("version").toInt() == 1 && exchange.value("parts").toArray().size() == 2,
        "Geometry exchange did not declare a version and two named occurrences");
    const auto relative = exchange.value("stepFile").toString();
    const auto looseStep = temp.filePath(relative);
    QFile step(looseStep);
    require(!relative.isEmpty() && QDir::isRelativePath(relative) && step.open(QIODevice::ReadOnly),
        "Written geometry exchange did not reference a relative STEP companion");
    const auto stepBytes = step.readAll();
    step.close();
    const auto hash = QString::fromLatin1(QCryptographicHash::hash(stepBytes, QCryptographicHash::Sha256).toHex());
    require(exchange.value("sha256").toString() == hash && !packed.entity("joints", 71).contains("occCurveEdges")
        && packed.entity("joints", 71).value("curveGeometry").toArray().size() == 1,
        "Geometry exchange did not save the STEP hash and a geometry-based topology selection");

    const QDir bundle(temp.path());
    require(bundle.mkpath("relocated/motion") && bundle.mkpath("relocated/geometry"), "Could not create relocated bundle");
    const auto movedStep = temp.filePath("relocated/geometry/model.step");
    require(QFile::rename(looseStep, movedStep), "Could not relocate bundle geometry");
    const QString movedRelative = "../geometry/model.step";
    const auto movedPath = temp.filePath("relocated/motion/relative.motion.json");
    exchange["stepFile"] = movedRelative;
    packed.root["geometryExchange"] = exchange;
    require(writeNativeMotionDocument(movedPath, packed, error), error);
    require(readNativeMotionDocument(movedPath, packed, error), error);
    CadDocument restored;
    bool present = false;
    require(readNativeMotionExchange(movedPath, packed, restored, present, error) && present, error);
    require(packed.bodyPartBindings == original.bodyPartBindings && !restored.part(removed)
        && restored.part(first) && restored.part(second) && restored.part(first)->name == source.part(first)->name
        && restored.part(second)->name == source.part(second)->name
        && packed.entity("bodies", 0).value("geoEntityIds").toArray() == QJsonArray{91},
        "Relocated STEP changed logical part IDs, display names, or original geometry IDs");
    require(packed.entity("joints", 71).value("occCurveEdges").toArray().first().toObject().value("partId").toInt() == first
        && packed.entity("joints", 72).value("occSurfaceFaces").toArray().first().toObject().value("partId").toInt() == second,
        "Relocated STEP lost the proven edge or face identity");

    NativeMotionDocument legacy;
    auto known = body(0, {91}, source.parts()[1]);
    known["geoEntityNames"] = QJsonArray{restored.part(second)->sourceName};
    legacy.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{known}}}},
        {"generatorNotes", QJsonObject{{"geometryBinding", QJsonObject{{"stepFile", movedRelative}, {"stepSHA256", hash}}}}}};
    const auto legacyPath = temp.filePath("relocated/motion/legacy.motion.json");
    require(writeNativeMotionDocument(legacyPath, legacy, error), error);
    CadDocument restoredLegacy;
    require(readNativeMotionDocument(legacyPath, legacy, error) &&
        readNativeMotionExchange(legacyPath, legacy, restoredLegacy, present, error) && present, error);
    require(legacy.bodyPartBindings.value(0).size() == 1 &&
        restoredLegacy.part(legacy.bodyPartBindings.value(0).front())->sourceName == restored.part(second)->sourceName,
        "Legacy path+hash did not use verified source part names for binding");

    QFile modified(movedStep);
    require(modified.open(QIODevice::Append) && modified.write("\n") == 1, "Could not tamper with the STEP bytes");
    modified.close();
    NativeMotionDocument broken;
    require(readNativeMotionDocument(movedPath, broken, error), error);
    const auto before = broken.root;
    const auto priorShape = restored.part(first)->shape;
    require(!readNativeMotionExchange(movedPath, broken, restored, present, error) && error.contains("SHA-256")
        && broken.root == before && broken.bodyPartBindings.isEmpty() && restored.part(first)->shape.IsEqual(priorShape),
        "Checksum mismatch was accepted or replaced the live CAD or document");
}

void sourceFrameBindings(const QTemporaryDir& temp)
{
    CadDocument definitions;
    QString error;
    int id;
    require(definitions.addPrimitive(Primitive::Box, 10, 20, 30, id, error), error);
    require(definitions.addPrimitive(Primitive::Box, 7, 11, 13, id, error), error);
    require(definitions.addPrimitive(Primitive::Box, 3, 9, 14, id, error), error);
    const auto importPlaced = [&](const QString& name, const std::vector<int>& order, CadDocument& placed) {
        Handle(TDocStd_Document) doc = new TDocStd_Document("BinXCAF");
        const auto tool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
        BRep_Builder builder;
        TopoDS_Compound empty;
        builder.MakeCompound(empty);
        const auto assembly = tool->NewShape();
        tool->SetShape(assembly, empty);
        std::vector<TDF_Label> labels;
        for (const auto& part : definitions.parts()) labels.push_back(tool->AddShape(part.shape, Standard_False));
        for (std::size_t i = 0; i < order.size(); ++i) {
            gp_Trsf placement;
            placement.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0.37 + i * 0.21);
            placement.SetTranslationPart(gp_Vec(80.0 + i * 90.0, -170.0 - i * 30.0, 25.0 + i * 11.0));
            const auto instance = tool->AddComponent(assembly, labels.at(order[i]), TopLoc_Location(placement));
            TDataStd_Name::Set(instance, QStringLiteral("Occurrence %1").arg(i).toUtf8().constData());
        }
        // Avoid exporting unused free definitions in the duplicate-instance case.
        for (std::size_t i = 0; i < labels.size(); ++i)
            if (std::find(order.begin(), order.end(), int(i)) == order.end()) tool->RemoveShape(labels[i]);
        tool->UpdateAssemblies();
        STEPCAFControl_Writer writer;
        const auto path = temp.filePath(name + ".step");
        require(writer.Transfer(doc, STEPControl_AsIs) && writer.Write(path.toUtf8().constData()) == IFSelect_RetDone,
            "Cannot write placed STEP binding fixture");
        std::vector<int> imported;
        const bool importedOk = placed.importFile(path, imported, error);
        require(importedOk, "Cannot import placed STEP binding fixture: " + error);
        QStringList importedNames;
        for (const auto& part : placed.parts()) importedNames.append(part.sourceName);
        require(imported.size() == order.size(), QStringLiteral("Placed STEP fixture imported %1 parts, expected %2: %3")
            .arg(imported.size()).arg(order.size()).arg(importedNames.join(", ")));
    };

    CadDocument placed;
    importPlaced("local-mass-frames", {2, 0, 1}, placed);
    NativeMotionDocument source;
    QJsonArray records;
    for (int i = 0; i < 3; ++i) {
        auto record = body(60 + i, QJsonArray{900 - i * 103}, definitions.parts()[i]);
        record["isCustomMassProps"] = false;
        GProp_GProps props;
        BRepGProp::VolumeProperties(definitions.parts()[i].shape, props);
        double p1, p2, p3;
        props.PrincipalProperties().Moments(p1, p2, p3);
        const double scale = record.value("mass").toDouble() / props.Mass();
        record["inertia"] = QJsonArray{p1 * scale, p2 * scale, p3 * scale, 0, 0, 0};
        records.append(record);
    }
    source.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", records}}}};
    const auto originalRoot = source.root;
    QVector<NativeMotionBindingIssue> issues;
    QMap<int, QString> evidence;
    require(autoBindNativeMotionBodies(placed, source, issues, &evidence) && source.root == originalRoot,
        "STEP definition-local automatic mass properties could not bind without changing the source");
    const int originalDefinition[] = {2, 0, 1};
    for (int i = 0; i < 3; ++i) {
        const CadPart* part = nullptr;
        for (const auto& candidate : placed.parts())
            if (candidate.sourceName == QStringLiteral("Occurrence %1").arg(i)) part = &candidate;
        require(part && source.bodyPartBindings.value(60 + originalDefinition[i]) == QVector<int>{part->id},
            "Local-frame binding inferred identity from source IDs or CAD enumeration");
        require(evidence.value(60 + originalDefinition[i]).contains(QStringLiteral("原始局部质心"))
            && evidence.value(60 + originalDefinition[i]).contains(QStringLiteral("偏差")),
            "Local-frame binding did not disclose its CAD/world centroid difference");
    }
    const auto sourcePath = temp.filePath("local-mass-frames.motion.json");
    require(writeNativeMotionDocument(sourcePath, source, error), error);
    NativeMotionDocument reread;
    require(readNativeMotionDocument(sourcePath, reread, error) && reread.root == originalRoot,
        "Binding silently transformed saved local COM or inertia values");

    const auto rejectLocal = [&](const std::function<void(QJsonObject&)>& edit, const QString& why) {
        NativeMotionDocument invalid;
        invalid.root = originalRoot;
        change(invalid, "bodies", edit);
        require(!autoBindNativeMotionBodies(placed, invalid, issues) && !invalid.bodyPartBindings.contains(60), why);
    };
    rejectLocal([](auto& record) { record["isCustomMassProps"] = true; },
        "A deliberately custom COM was reinterpreted as an automatic local frame");
    rejectLocal([](auto& record) { record["centerOfMass"] = vector(5.1, 10, 15); },
        "A nonmatching local COM was bound from inertia alone");
    rejectLocal([](auto& record) {
        auto values = record.value("inertia").toArray(); values[1] = values[1].toDouble() * 1.01; record["inertia"] = values;
    }, "Incorrect principal inertia was accepted by the local-frame fallback");
    rejectLocal([](auto& record) { record["inertia"] = QJsonArray{0, 0, 0, 0, 0, 0}; },
        "Local-frame binding accepted an absent inertia fingerprint");

    NativeMotionDocument world;
    auto worldBody = body(80, {771}, placed.parts()[0]);
    worldBody["isCustomMassProps"] = false;
    world.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{worldBody}}}}};
    require(autoBindNativeMotionBodies(placed, world, issues, &evidence)
        && world.bodyPartBindings.value(80) == QVector<int>{placed.parts()[0].id}
        && !evidence.value(80).contains(QStringLiteral("原始局部质心")),
        "A valid world-frame COM lost precedence to local-frame matching");

    CadDocument repeated;
    importPlaced("repeated-local-mass-frames", {0, 0}, repeated);
    NativeMotionDocument ambiguous;
    ambiguous.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{records.first()}}}}};
    require(!autoBindNativeMotionBodies(repeated, ambiguous, issues) && ambiguous.bodyPartBindings.isEmpty()
        && issues.size() == 1 && issues.front().candidatePartIds.size() == 2,
        "Repeated STEP definitions with identical local signatures were bound by occurrence order");

    // The body pass may precede topology re-identification after CAD reimport.
    auto withStaleTopology = source;
    auto data = withStaleTopology.root.value("motionData").toObject();
    data["joints"] = QJsonArray{QJsonObject{{"id", 5}, {"jointType", "PointOnCurve"}, {"baseBodyId", 60},
        {"occCurveEdges", QJsonArray{QJsonObject{{"partId", 99999}, {"index", 88888}}}}}};
    withStaleTopology.root["motionData"] = data;
    require(autoBindNativeMotionBodies(placed, withStaleTopology, issues),
        "Body binding prematurely rejected topology references awaiting their separate rebind pass");
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream output(stdout), errors(stderr);
    try {
        QString error;
        CadDocument cad;
        int first = -1, second = -1;
        require(cad.addPrimitive(Primitive::Box, 10, 20, 30, first, error), error);
        require(cad.addPrimitive(Primitive::Box, 20, 30, 40, second, error), error);
        auto original = fixture(cad);
        require(validateNativeMotionDocument(original, error), error);
        NativeMotionDocument parsed;
        require(parseNativeMotionDocument(QJsonDocument(original.root).toJson(), parsed, error, "/fixture"), error);
        require(parsed.root == original.root && parsed.baseDirectory == "/fixture", "Import discarded source fields or source directory");
        require(parsed.entity("bodies", 0).value("id").toInt(-1) == 0, "Source body ID zero was confused with ground");

        QTemporaryDir temp;
        require(temp.isValid(), "No temporary directory");
        const auto path = temp.filePath(QStringLiteral("完整设置.motion.json"));
        require(writeNativeMotionDocument(path, parsed, error), error);
        NativeMotionDocument roundtrip;
        require(readNativeMotionDocument(path, roundtrip, error), error);
        require(roundtrip.root == original.root, "Round trip changed expressions, units, rich entities or unknown extension fields");

        auto channelDocument = original;
        auto channelData = channelDocument.root.value("motionData").toObject();
        const QMap<QString, QString> forceSources{{"VectorForce", "vectorForces"}, {"ScalarForce", "scalarForces"},
            {"ScalarTorque", "scalarTorques"}, {"ThreePointForce", "threePointForces"}, {"ThreePointTorque", "threePointTorques"}};
        QJsonArray nativeRequests;
        int forceId = 500;
        for (auto it = forceSources.cbegin(); it != forceSources.cend(); ++it, ++forceId) {
            channelData[it.value()] = QJsonArray{QJsonObject{{"id", forceId}, {"bodyId", 0},
                {"bodyIId", 0}, {"bodyJId", -1}, {"localI", vector(0, 0, 0)}, {"localJ", vector(1, 0, 0)},
                {"targetBodyId", 0}, {"originBodyId", -1}, {"localTarget", vector(1, 0, 0)}, {"localOrigin", vector(0, 0, 0)}}};
            for (const auto& source : {it.key(), it.value()}) nativeRequests.append(QJsonObject{{"requestId", nativeRequests.size() + 1},
                {"sourceType", source}, {"sourceId", forceId}, {"variable", "ForceI"}, {"component", -1}, {"visible", false}});
        }
        for (const auto& source : {QString("AutoFlexBody"), QString("autoFlexBodies")})
            nativeRequests.append(QJsonObject{{"requestId", nativeRequests.size() + 1}, {"sourceType", source},
                {"sourceId", 8}, {"variable", "ModalCoordinates"}, {"component", 1023}});
        for (const auto& source : {QString("softStops"), QString("tensionCables"), QString("springs")})
            nativeRequests.append(QJsonObject{{"requestId", nativeRequests.size() + 1}, {"sourceType", source},
                {"sourceId", 1}, {"variable", "Magnitude"}, {"component", 0}});
        channelDocument.root["motionData"] = channelData;
        change(channelDocument, "scenarios", [&](auto& record) { record["curveRequests"] = nativeRequests; });
        const auto nativeCurvePath = temp.filePath("native-channels.motion.json");
        require(writeNativeMotionDocument(nativeCurvePath, channelDocument, error), error);
        NativeMotionDocument channelRoundtrip;
        require(readNativeMotionDocument(nativeCurvePath, channelRoundtrip, error) && channelRoundtrip.root == channelDocument.root,
                "Plural native force/flexible curve requests did not survive exact JSON round trip: " + error);
        for (double invalidComponent : {-2.0, 0.5, 1024.0}) {
            auto invalid = channelDocument;
            change(invalid, "scenarios", [&](auto& record) {
                auto requests = record.value("curveRequests").toArray(); auto request = requests[0].toObject();
                request["component"] = invalidComponent; requests[0] = request; record["curveRequests"] = requests;
            });
            require(!validateNativeMotionDocument(invalid, error), "Invalid native curve component was silently accepted");
        }
        auto danglingCurve = channelDocument;
        change(danglingCurve, "scenarios", [&](auto& record) {
            auto requests = record.value("curveRequests").toArray(); auto request = requests[0].toObject();
            request["sourceId"] = 98765; requests[0] = request; record["curveRequests"] = requests;
        });
        require(!validateNativeMotionDocument(danglingCurve, error), "A plural native curve source bypassed reference validation");

        const auto rejected = [&](const QString& collection, const std::function<void(QJsonObject&)>& edit, const QString& why) {
            auto invalid = original;
            change(invalid, collection, edit);
            require(!parseNativeMotionDocument(QJsonDocument(invalid.root).toJson(), parsed, error), why);
            require(parsed.root == original.root, "Failed parse partially overwrote the prior document");
        };
        rejected("joints", [](auto& r) { r["jointType"] = "TypoRevolute"; }, "Unknown joint silently became a revolute");
        rejected("joints", [](auto& r) { r["actionBodyId"] = 99999; }, "Dangling body reference accepted");
        rejected("bodies", [](auto& r) { r["mass"] = "2 kilograms"; }, "Non-numeric mass was silently coerced");
        rejected("bodies", [](auto& r) { r["id"] = 0.5; }, "Fractional source ID accepted");
        rejected("bodies", [](auto& r) { r["inertiaConvention"] = "guess"; }, "Unknown inertia convention accepted");
        for (const auto* convention : {"auto", "tensor", "products"}) {
            auto inertiaDocument = original;
            change(inertiaDocument, "bodies", [&](auto& record) { record["inertiaConvention"] = convention; });
            NativeMotionDocument reread;
            require(parseNativeMotionDocument(QJsonDocument(inertiaDocument.root).toJson(), reread, error)
                && reread.root == inertiaDocument.root, "Explicit inertia convention was rejected or rewritten: " + error);
        }
        rejected("sensors", [](auto& r) { r["targetBodyId"] = -1; }, "Body sensor accepted ground instead of an actual body");
        rejected("controllers", [](auto& r) { r["feedbackSensorId"] = 20; }, "Dangling PID feedback sensor accepted");
        rejected("tensionCables", [](auto& r) { r["freeLength"] = 0; }, "Zero cable free length accepted");
        rejected("tensionCables", [](auto& r) { r["bodyJId"] = 0; }, "Cable attached twice to the same body");
        rejected("softStops", [](auto& r) { r["exponent"] = 0.5; }, "Singular stop exponent accepted");
        rejected("softStops", [](auto& r) { r["directionMode"] = 4; }, "Unknown stop axis frame accepted");
        rejected("softStops", [](auto& r) { r["tableX"] = QJsonArray{0, 2, 1}; r["tableY"] = QJsonArray{0, 2, 3}; }, "Nonmonotonic restoring table accepted");
        rejected("softStops", [](auto& r) { r["hasTable"] = true; r["tableX"] = QJsonArray{0, 1}; r["tableY"] = QJsonArray{1, 2}; }, "Nonzero onset force table accepted");
        auto tableStop = original;
        change(tableStop, "softStops", [](auto& r) { r["hasTable"] = true; r["tableX"] = QJsonArray{0, 1, 2}; r["tableY"] = QJsonArray{0, 5, 7}; r.remove("stiffness"); r.remove("exponent"); });
        require(validateNativeMotionDocument(tableStop, error), "Table stop incorrectly requires inactive power-law parameters: " + error);

        QVector<NativeMotionBindingIssue> issues;
        require(autoBindNativeMotionBodies(cad, parsed, issues), issues.isEmpty() ? "Binding failed" : issues.front().message);
        require(parsed.bodyPartBindings.value(0) == QVector<int>{first} && parsed.bodyPartBindings.value(7) == QVector<int>{second},
                "Geometry matched body/source IDs instead of the saved physical geometry");
        require(validateNativeMotionBindings(cad, parsed, error), error);
        const auto bindings = parsed.bodyPartBindings;
        require(!setNativeMotionBodyBinding(cad, parsed, 7, {first}, error) && parsed.bodyPartBindings == bindings,
                "Explicit duplicate part assignment replaced a valid binding");
        auto topology = parsed;
        change(topology, "joints", [&](auto& record) {
            record["jointType"] = "PointOnCurve"; record["baseBodyId"] = 7;
            record["occCurveEdges"] = QJsonArray{QJsonObject{{"partId", second}, {"index", 1}}};
        });
        require(validateNativeMotionBindings(cad, topology, error), "Valid explicit OCC owner/index rejected: " + error);
        auto globalCurve = topology;
        change(globalCurve, "joints", [](auto& record) { record["baseBodyId"] = 0; });
        require(validateNativeMotionBindings(cad, globalCurve, error), "Original global GeoEdge reference was restricted to physical body parts");
        auto wrongIndex = topology;
        change(wrongIndex, "joints", [&](auto& record) {
            record["occCurveEdges"] = QJsonArray{QJsonObject{{"partId", second}, {"index", 999999}}};
        });
        require(!validateNativeMotionBindings(cad, wrongIndex, error), "Out-of-range OCC topology was allowed into a saved project");
        auto surfaceSelection = topology;
        change(surfaceSelection, "joints", [&](auto& record) {
            record.remove("occCurveEdges"); record["jointType"] = "PointOnSurface";
            record["occSurfaceFaces"] = QJsonArray{QJsonObject{{"partId", second}, {"index", 1}}};
        });
        require(validateNativeMotionBindings(cad, surfaceSelection, error), "Valid explicit OCC face rejected: " + error);
        change(surfaceSelection, "joints", [](auto& record) { record["baseBodyId"] = 0; });
        require(!validateNativeMotionBindings(cad, surfaceSelection, error), "Selected surface escaped its original base-body face set");

        // Legacy files stored sorted principal moments, not the global tensor
        // diagonal. Geometry matching must be invariant to that representation.
        auto principalDocument = fixture(cad);
        change(principalDocument, "bodies", [&](auto& record) {
            GProp_GProps props;
            BRepGProp::VolumeProperties(cad.parts()[0].shape, props);
            double firstMoment, secondMoment, thirdMoment;
            props.PrincipalProperties().Moments(firstMoment, secondMoment, thirdMoment);
            const double factor = record.value("mass").toDouble() / props.Mass();
            record["inertia"] = QJsonArray{firstMoment * factor, secondMoment * factor, thirdMoment * factor, 0, 0, 0};
        });
        require(autoBindNativeMotionBodies(cad, principalDocument, issues)
            && principalDocument.bodyPartBindings.value(0) == QVector<int>{first},
                "Legacy principal moments were mistaken for global-axis inertia");

        // Some native exporters kept the global diagonal but zeroed products.
        // A rotated asymmetric box makes those diagonals differ materially
        // from its principal moments, so this guards both supported formats.
        gp_Trsf rotation;
        rotation.SetRotation(gp_Ax1(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)), 0.41);
        const auto rotatedShape = BRepBuilderAPI_Transform(cad.parts()[0].shape, rotation, Standard_True).Shape();
        const auto rotatedPath = temp.filePath("rotated-inertia.brep");
        require(BRepTools::Write(rotatedShape, rotatedPath.toUtf8().constData()), "Cannot write rotated inertia fixture");
        CadDocument rotatedCad;
        std::vector<int> rotatedIds;
        require(rotatedCad.importFile(rotatedPath, rotatedIds, error), error);
        auto diagonalBody = body(70, {5001}, rotatedCad.parts()[0]);
        auto diagonalInertia = diagonalBody.value("inertia").toArray();
        require(std::abs(diagonalInertia[3].toDouble()) > 1, "Rotated fixture did not produce nonzero inertia products");
        diagonalInertia[3] = 0; diagonalInertia[4] = 0; diagonalInertia[5] = 0;
        diagonalBody["inertia"] = diagonalInertia;
        NativeMotionDocument diagonalDocument;
        diagonalDocument.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{diagonalBody}}}}};
        const auto diagonalRoot = diagonalDocument.root;
        require(autoBindNativeMotionBodies(rotatedCad, diagonalDocument, issues)
            && diagonalDocument.bodyPartBindings.value(70) == QVector<int>{rotatedIds[0]}
            && diagonalDocument.root == diagonalRoot, "World-axis diagonal-only inertia was rejected or modified");
        diagonalDocument.bodyPartBindings.clear();
        change(diagonalDocument, "bodies", [](auto& record) {
            auto inertia = record.value("inertia").toArray(); inertia[0] = inertia[0].toDouble() * 1.2; record["inertia"] = inertia;
        });
        require(!autoBindNativeMotionBodies(rotatedCad, diagonalDocument, issues), "Incorrect global-axis inertia bypassed geometric verification");

        BRep_Builder compoundBuilder;
        TopoDS_Compound compound;
        compoundBuilder.MakeCompound(compound);
        for (const auto& part : cad.parts()) compoundBuilder.Add(compound, part.shape);
        CadPart aggregate;
        aggregate.shape = compound;
        NativeMotionDocument grouped;
        grouped.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{body(19, {501, 999}, aggregate)}}}}};
        require(autoBindNativeMotionBodies(cad, grouped, issues)
            && grouped.bodyPartBindings.value(19) == QVector<int>{first, second},
                "A multi-part body's aggregate mass geometry was not matched independently of source ID spacing");

        // A source CAD part can be a compound that displays as several OCC
        // solids. Preserve its occurrence identity across actual STEP IO and
        // bind all its displayed pieces to the one original geometry ID.
        Handle(TDocStd_Document) stepDocument = new TDocStd_Document("BinXCAF");
        const auto shapeTool = XCAFDoc_DocumentTool::ShapeTool(stepDocument->Main());
        const auto compoundLabel = shapeTool->AddShape(compound, Standard_False);
        TDataStd_Name::Set(compoundLabel, "Explicit source compound");
        STEPCAFControl_Writer stepWriter;
        const auto compoundPath = temp.filePath("one-occurrence.step");
        require(stepWriter.Transfer(stepDocument, STEPControl_AsIs)
            && stepWriter.Write(compoundPath.toUtf8().constData()) == IFSelect_RetDone, "Cannot write compound STEP fixture");
        CadDocument occurrenceCad;
        std::vector<int> occurrenceIds;
        require(occurrenceCad.importFile(compoundPath, occurrenceIds, error), error);
        require(occurrenceIds.size() == 2, "Compound STEP fixture did not expose two display parts");
        NativeMotionDocument singleSource;
        singleSource.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{body(20, {879}, aggregate)}}}}};
        require(autoBindNativeMotionBodies(occurrenceCad, singleSource, issues)
            && singleSource.bodyPartBindings.value(20).size() == 2,
                "One original compound part was not bound to all pieces of its STEP occurrence");
        singleSource.bodyPartBindings.clear();
        change(singleSource, "bodies", [](auto& record) {
            record["geoEntityNames"] = QJsonArray{"Explicit source compound"};
            record["centerOfMass"] = vector(100, 200, 300);
            record["mass"] = 0.0001;
            record["inertia"] = QJsonArray{1, 1, 1, 0, 0, 0};
        });
        QMap<int, QString> bindingEvidence;
        require(autoBindNativeMotionBodies(occurrenceCad, singleSource, issues, &bindingEvidence)
            && singleSource.bodyPartBindings.value(20).size() == 2 && bindingEvidence.value(20).contains("自定义"),
                "An exact source occurrence name incorrectly rejected intentionally custom mass properties");

        // A background body can contain all remaining repeated occurrences.
        // Compare the complete name multiset, not old _2/new (2) ordinals; one
        // original occurrence may split into several display solids.
        Handle(TDocStd_Document) remainderStep = new TDocStd_Document("BinXCAF");
        const auto remainderTool = XCAFDoc_DocumentTool::ShapeTool(remainderStep->Main());
        TDataStd_Name::Set(remainderTool->AddShape(rotatedShape, Standard_False), "Known independent part");
        TDataStd_Name::Set(remainderTool->AddShape(compound, Standard_False), "Repeated background");
        gp_Trsf translation;
        translation.SetTranslation(gp_Vec(100, 200, 300));
        const auto shiftedCompound = BRepBuilderAPI_Transform(compound, translation, Standard_True).Shape();
        TDataStd_Name::Set(remainderTool->AddShape(shiftedCompound, Standard_False), "Repeated background");
        STEPCAFControl_Writer remainderWriter;
        const auto remainderPath = temp.filePath("remaining-occurrences.step");
        require(remainderWriter.Transfer(remainderStep, STEPControl_AsIs)
            && remainderWriter.Write(remainderPath.toUtf8().constData()) == IFSelect_RetDone,
            "Cannot write remaining-occurrence fixture");
        CadDocument remainderCad;
        std::vector<int> remainderIds;
        require(remainderCad.importFile(remainderPath, remainderIds, error), error);
        require(remainderIds.size() == 5, "Remaining-occurrence fixture did not contain five display solids");
        const CadPart* knownPart = nullptr;
        for (const auto& part : remainderCad.parts()) if (part.sourceName == "Known independent part") knownPart = &part;
        require(knownPart != nullptr, "Known occurrence identity was lost");
        auto knownBody = body(10, {101}, *knownPart);
        knownBody["geoEntityNames"] = QJsonArray{"Known independent part"};
        auto backgroundBody = body(11, {3001, 9001}, aggregate);
        backgroundBody["geoEntityNames"] = QJsonArray{"Repeated background", "Repeated background_2"};
        backgroundBody["centerOfMass"] = vector(1000, 2000, 3000); // Deliberately custom.
        NativeMotionDocument remainderDocument;
        remainderDocument.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{backgroundBody, knownBody}}}}};
        const auto remainderRoot = remainderDocument.root;
        require(autoBindNativeMotionBodies(remainderCad, remainderDocument, issues, &bindingEvidence)
            && remainderDocument.bodyPartBindings.value(11).size() == 4
            && bindingEvidence.value(11).contains("多重集合") && remainderDocument.root == remainderRoot,
            "A uniquely identified complete remaining occurrence set rejected custom mass properties");
        const auto rejectRemainder = [&](const std::function<void(NativeMotionDocument&)>& edit, const QString& why) {
            NativeMotionDocument invalid;
            invalid.root = remainderRoot;
            edit(invalid);
            require(!autoBindNativeMotionBodies(remainderCad, invalid, issues) && !invalid.bodyPartBindings.contains(11), why);
        };
        rejectRemainder([](auto& doc) { change(doc, "bodies", [](auto& record) {
            record["geoEntityIds"] = QJsonArray{3001}; record["geoEntityNames"] = QJsonArray{"Repeated background"};
        }); }, "A missing source occurrence was silently included in the background binding");
        rejectRemainder([](auto& doc) { change(doc, "bodies", [](auto& record) {
            record["geoEntityIds"] = QJsonArray{3001, 9001, 9002};
            record["geoEntityNames"] = QJsonArray{"Repeated background", "Repeated background_2", "Repeated background_3"};
        }); }, "An extra source occurrence was silently discarded from the background binding");
        rejectRemainder([](auto& doc) { change(doc, "bodies", [](auto& record) { record["isCustomMassProps"] = false; }); },
            "Non-custom background bypassed its inconsistent aggregate mass signature");
        rejectRemainder([](auto& doc) {
            auto data = doc.root.value("motionData").toObject(); auto bodies = data.value("bodies").toArray();
            auto extra = bodies.first().toObject(); extra["id"] = 12; extra["geoEntityNames"] = QJsonArray{"Unknown", "Unknown_2"};
            bodies.append(extra); data["bodies"] = bodies; doc.root["motionData"] = data;
        }, "Multiple unresolved bodies were assigned the remaining geometry by iteration order");

        CadDocument ambiguousCad;
        int duplicateA, duplicateB;
        require(ambiguousCad.addPrimitive(Primitive::Box, 10, 20, 30, duplicateA, error), error);
        require(ambiguousCad.addPrimitive(Primitive::Box, 10, 20, 30, duplicateB, error), error);
        NativeMotionDocument ambiguous;
        ambiguous.root = {{"formatVersion", 1}, {"motionData", QJsonObject{{"bodies", QJsonArray{body(100, {1}, ambiguousCad.parts()[0])}}}}};
        require(!autoBindNativeMotionBodies(ambiguousCad, ambiguous, issues) && ambiguous.bodyPartBindings.isEmpty()
            && issues.size() == 1 && issues.front().candidatePartIds.size() == 2,
                "Ambiguous identical geometry was silently bound by numerical ID");
        require(setNativeMotionBodyBinding(ambiguousCad, ambiguous, 100, {duplicateB}, error), error);
        require(autoBindNativeMotionBodies(ambiguousCad, ambiguous, issues) && ambiguous.bodyPartBindings.value(100) == QVector<int>{duplicateB},
                "Automatic binding overwrote the user's explicit part selection");

        sourceFrameBindings(temp);
        geometryExchange(temp);

        int imported = 0;
        if (app.arguments().size() > 1) {
            QDirIterator files(app.arguments()[1], {QStringLiteral("*.motion.json")}, QDir::Files, QDirIterator::Subdirectories);
            while (files.hasNext()) {
                const auto casePath = files.next();
                NativeMotionDocument example;
                require(readNativeMotionDocument(casePath, example, error), casePath + QLatin1Char('\n') + error);
                QFile file(casePath);
                require(file.open(QIODevice::ReadOnly), casePath);
                require(example.root == QJsonDocument::fromJson(file.readAll()).object(), "Case import changed source JSON: " + casePath);
                ++imported;
            }
            require(imported > 0, "No native motion cases were discovered");
        }
        output << "Native Motion JSON: lossless round trip, source IDs/ground, rich references, unilateral parameters, safe geometry binding and portable STEP exchange passed";
        if (imported) output << "; " << imported << " supplied case documents imported without field loss";
        output << '\n';
        return 0;
    } catch (const std::exception& failure) {
        errors << failure.what() << '\n';
        return 1;
    }
}
