#include "RigidProject.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "NativeTopologyBinding.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QTemporaryDir>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <initializer_list>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <utility>

namespace MotionOcc {
namespace {

constexpr int kLegacyVersion = 1;
constexpr int kStepVersion = 2;
constexpr int kVersion = 3;
constexpr qint64 kMaxProjectBytes = 128ll * 1024 * 1024;
constexpr int kMaxPartBytes = 32 * 1024 * 1024;
constexpr int kMaxMnfBytes = 64 * 1024 * 1024;
constexpr int kMaxEmbeddedBytes = 80 * 1024 * 1024;
constexpr int kMaxAssets = 1024;
constexpr int kMaxParts = 20000;
constexpr int kMaxName = 1024; // UTF-16 code units, as in rigid model validation.
// Bound collections before allocating. The solver's semantic checks still apply.
constexpr int kMaxBodies = 256;
constexpr int kMaxJoints = 1024;
constexpr int kMaxScenarios = 256;
constexpr int kMaxCurvesPerScenario = 256;
// More than enough for the largest valid v1 project; prevents a large flat
// array or object of tiny values from expanding out of proportion during parse.
constexpr qint64 kMaxJsonSeparators = 1000000;
constexpr int kMaxId = (std::numeric_limits<int>::max)();
constexpr std::uint64_t kExhaustedPartId = std::uint64_t(kMaxId) + 1;

struct ProjectError { QString message; };

[[noreturn]] void fail(const QString& message) { throw ProjectError{message}; }

void require(bool valid, const QString& message)
{
    if (!valid) fail(message);
}

template<class Operation>
bool run(const QString& context, QString& error, const Operation& operation)
{
    try {
        operation();
        error.clear();
        return true;
    } catch (const ProjectError& failure) {
        error = context + QStringLiteral(": ") + failure.message;
    } catch (const std::bad_alloc&) {
        error = context + QStringLiteral(": not enough memory.");
    } catch (const std::exception& failure) {
        error = context + QStringLiteral(": ") + QString::fromUtf8(failure.what());
    } catch (...) {
        error = context + QStringLiteral(": unexpected error.");
    }
    return false;
}

void checkPath(const QString& path)
{
    require(!path.isEmpty() && !path.contains(QChar::Null), QStringLiteral("Invalid file path."));
}

void checkProjectPath(const QString& path)
{
    checkPath(path);
    require(QFileInfo(path).suffix().compare(QStringLiteral("mxmotion"), Qt::CaseInsensitive) == 0,
            QStringLiteral("Expected an .mxmotion project path."));
}

void openOutput(QSaveFile& file)
{
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) fail(file.errorString());
}

void writeBytes(QSaveFile& file, const QByteArray& bytes)
{
    if (file.write(bytes) != bytes.size()) fail(file.errorString());
}

// Qt validates UTF-8 and the overall structure, but its parser collapses duplicate
// object keys and accepts some non-JSON string escapes. Count original members
// and bound nesting before asking Qt to allocate a document.
qint64 countSourceMembers(const QByteArray& bytes)
{
    qint64 members = 0;
    qint64 commas = 0;
    int depth = 0;
    bool inString = false;
    for (int i = 0; i < bytes.size(); ++i) {
        const unsigned char c = bytes[i];
        if (inString) {
            if (c == '\\') {
                require(++i < bytes.size(), QStringLiteral("Truncated JSON escape."));
                switch (bytes[i]) {
                case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't': case 'u':
                    break; // Qt checks the four hexadecimal digits of a Unicode escape.
                default: fail(QStringLiteral("Invalid JSON escape."));
                }
            } else if (c == '"') {
                inString = false;
            } else if (c < 0x20) {
                fail(QStringLiteral("Unescaped control character in a JSON string."));
            }
        } else if (c == '"') {
            inString = true;
        } else if (c == '{' || c == '[') {
            if (++depth > 32) fail(QStringLiteral("Project JSON is nested too deeply."));
        } else if (c == '}' || c == ']') {
            if (--depth < 0) fail(QStringLiteral("Unbalanced project JSON."));
        } else if (c == ':') {
            if (++members > kMaxJsonSeparators) fail(QStringLiteral("Project has too many JSON fields."));
        } else if (c == ',') {
            if (++commas > kMaxJsonSeparators) fail(QStringLiteral("Project has too many JSON values."));
        }
    }
    return members;
}

qint64 countParsedMembers(const QJsonValue& value)
{
    qint64 members = 0;
    if (value.isObject()) {
        const auto object = value.toObject();
        members = object.size();
        for (auto it = object.constBegin(); it != object.constEnd(); ++it)
            members += countParsedMembers(it.value());
    } else if (value.isArray()) {
        for (const auto& item : value.toArray()) members += countParsedMembers(item);
    }
    return members;
}

QJsonObject readProject(const QString& path)
{
    checkProjectPath(path);
    require(QFileInfo(path).isFile(), QStringLiteral("Project is not a regular file."));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) fail(file.errorString());
    require(file.size() <= kMaxProjectBytes, QStringLiteral("Project exceeds 128 MiB."));
    const QByteArray bytes = file.read(kMaxProjectBytes + 1);
    if (file.error() != QFileDevice::NoError) fail(file.errorString());
    require(bytes.size() <= kMaxProjectBytes && file.atEnd(), QStringLiteral("Project exceeds 128 MiB."));

    const auto originalMembers = countSourceMembers(bytes);
    QJsonParseError parseError{};
    const auto document = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError)
        fail(QStringLiteral("Invalid UTF-8 JSON at byte %1: %2").arg(parseError.offset).arg(parseError.errorString()));
    require(document.isObject(), QStringLiteral("Expected a project object."));
    const auto root = document.object();
    require(originalMembers == countParsedMembers(root), QStringLiteral("Duplicate JSON object key."));
    return root;
}

QJsonObject record(const QJsonValue& value, std::initializer_list<const char*> fields, const char* label)
{
    const auto object = value.toObject();
    bool valid = value.isObject() && object.size() == static_cast<int>(fields.size());
    for (const auto* field : fields) valid = valid && object.contains(QLatin1String(field));
    require(valid, QStringLiteral("Missing or unexpected fields in %1.").arg(QLatin1String(label)));
    return object;
}

QJsonArray array(const QJsonValue& value, int max, const char* field)
{
    const auto items = value.toArray();
    require(value.isArray() && items.size() <= max,
            QStringLiteral("Expected an array of at most %1 items for %2.").arg(max).arg(QLatin1String(field)));
    return items;
}

double number(const QJsonValue& value, const char* field)
{
    const double result = value.toDouble();
    require(value.isDouble() && std::isfinite(result),
            QStringLiteral("Expected a finite JSON number for %1.").arg(QLatin1String(field)));
    return result;
}

std::uint64_t integer(const QJsonValue& value, int min, std::uint64_t max, const char* field)
{
    const double result = number(value, field);
    require(result >= min && result <= static_cast<double>(max) && std::trunc(result) == result,
            QStringLiteral("Invalid integer for %1.").arg(QLatin1String(field)));
    return static_cast<std::uint64_t>(result);
}

int id(const QJsonObject& object, const char* field = "id", int min = 1)
{
    return static_cast<int>(integer(object.value(QLatin1String(field)), min, kMaxId, field));
}

bool boolean(const QJsonValue& value, const char* field)
{
    require(value.isBool(), QStringLiteral("Expected a JSON boolean for %1.").arg(QLatin1String(field)));
    return value.toBool();
}

QString checkedName(const QString& name)
{
    if (name.size() > kMaxName || name.contains(QChar::Null) || !name.isValidUtf16())
        fail(QStringLiteral("Invalid name (maximum 1024 characters, valid Unicode, no NUL)."));
    return name;
}

QString name(const QJsonObject& object)
{
    const auto value = object.value(QStringLiteral("name"));
    require(value.isString(), QStringLiteral("Expected a JSON string for name."));
    return checkedName(value.toString());
}

Vector3d readVector(const QJsonValue& value, const char* field)
{
    const auto items = array(value, 3, field);
    require(items.size() == 3, QStringLiteral("Expected three coordinates for %1.").arg(QLatin1String(field)));
    return {number(items[0], field), number(items[1], field), number(items[2], field)};
}

QJsonArray writeVector(const Vector3d& v) { return {v[0], v[1], v[2]}; }

const char* jointKind(JointKind kind)
{
    switch (kind) {
    case JointKind::Revolute: return "revolute";
    case JointKind::Prismatic: return "prismatic";
    case JointKind::Fixed: return "fixed";
    }
    fail(QStringLiteral("Unknown joint kind."));
}

const char* driveKind(DriveKind kind)
{
    switch (kind) {
    case DriveKind::Speed: return "speed";
    case DriveKind::Displacement: return "displacement";
    }
    fail(QStringLiteral("Unknown drive kind."));
}

JointKind readJointKind(const QJsonValue& value)
{
    const auto kind = value.toString();
    for (const auto type : {JointKind::Revolute, JointKind::Prismatic, JointKind::Fixed})
        if (kind == QLatin1String(jointKind(type))) return type;
    fail(QStringLiteral("Invalid joint kind."));
}

DriveKind readDriveKind(const QJsonValue& value)
{
    const auto kind = value.toString();
    for (const auto type : {DriveKind::Speed, DriveKind::Displacement})
        if (kind == QLatin1String(driveKind(type))) return type;
    fail(QStringLiteral("Invalid drive kind."));
}

const char* curveMetric(RigidCurveMetric metric)
{
    switch (metric) {
    case RigidCurveMetric::BodyPosition: return "body_position";
    case RigidCurveMetric::BodyDisplacement: return "body_displacement";
    case RigidCurveMetric::BodyVelocity: return "body_velocity";
    case RigidCurveMetric::BodyRotation: return "body_rotation";
    case RigidCurveMetric::BodyAngularVelocity: return "body_angular_velocity";
    case RigidCurveMetric::JointCoordinate: return "joint_coordinate";
    case RigidCurveMetric::JointRate: return "joint_rate";
    case RigidCurveMetric::NativeChannel: return "native_channel";
    }
    fail(QStringLiteral("Unknown curve metric."));
}

RigidCurveMetric readCurveMetric(const QJsonValue& value)
{
    const auto metric = value.toString();
    for (const auto type : {RigidCurveMetric::BodyPosition, RigidCurveMetric::BodyDisplacement,
            RigidCurveMetric::BodyVelocity, RigidCurveMetric::BodyRotation,
            RigidCurveMetric::BodyAngularVelocity, RigidCurveMetric::JointCoordinate,
            RigidCurveMetric::JointRate, RigidCurveMetric::NativeChannel})
        if (metric == QLatin1String(curveMetric(type))) return type;
    fail(QStringLiteral("Invalid curve metric."));
}

QJsonArray writeCurveRequests(const std::vector<RigidCurveRequest>& requests)
{
    QJsonArray array;
    for (const auto& request : requests) {
        QJsonObject row{
            {QStringLiteral("id"), request.id},
            {QStringLiteral("metric"), QString::fromLatin1(curveMetric(request.metric))},
            {QStringLiteral("sourceId"), request.sourceId}, {QStringLiteral("component"), request.component},
            {QStringLiteral("name"), checkedName(request.name)}, {QStringLiteral("color"), request.color},
            {QStringLiteral("visible"), request.visible}};
        if (request.metric == RigidCurveMetric::NativeChannel) {
            row.insert(QStringLiteral("sourceType"), request.sourceType);
            row.insert(QStringLiteral("variable"), request.variable);
        }
        array.append(row);
    }
    return array;
}

std::vector<RigidCurveRequest> readCurveRequests(const QJsonValue& value)
{
    std::vector<RigidCurveRequest> requests;
    for (const auto& item : array(value, kMaxCurvesPerScenario, "curveRequests")) {
        auto data = item.toObject();
        const auto sourceType = data.take(QStringLiteral("sourceType"));
        const auto variable = data.take(QStringLiteral("variable"));
        const auto row = record(item.isObject() ? QJsonValue(data) : item,
            {"id", "metric", "sourceId", "component", "name", "color", "visible"}, "curve request");
        const auto metric = readCurveMetric(row.value(QStringLiteral("metric")));
        if (metric == RigidCurveMetric::NativeChannel)
            require(sourceType.isString() && !sourceType.toString().isEmpty()
                && variable.isString() && !variable.toString().isEmpty(), QStringLiteral("Missing native curve source."));
        else require(sourceType.isUndefined() && variable.isUndefined(), QStringLiteral("Unexpected native curve source."));
        const double component = number(row.value(QStringLiteral("component")), "component");
        require(std::trunc(component) == component && component >= (metric == RigidCurveMetric::NativeChannel ? -1 : 0)
            && component <= (metric == RigidCurveMetric::NativeChannel ? 1023 : 3), QStringLiteral("Invalid curve component."));
        require(row.value(QStringLiteral("color")).isString(), QStringLiteral("Expected a curve color string."));
        requests.push_back({id(row), metric,
            id(row, "sourceId", 0), static_cast<int>(component),
            name(row), row.value(QStringLiteral("color")).toString(),
            boolean(row.value(QStringLiteral("visible")), "visible"), sourceType.toString(), variable.toString()});
    }
    return requests;
}

QJsonObject writeModel(const RigidModel& model)
{
    if (model.nativeDocument) {
        QJsonArray bindings, curves;
        for (auto it = model.nativeDocument->bodyPartBindings.cbegin(); it != model.nativeDocument->bodyPartBindings.cend(); ++it) {
            QJsonArray ids;
            for (int part : it.value()) ids.append(part);
            bindings.append(QJsonObject{{QStringLiteral("bodyId"), it.key()}, {QStringLiteral("partIds"), ids}});
        }
        for (const auto& scenario : model.scenarios)
            curves.append(QJsonObject{{QStringLiteral("scenarioId"), scenario.id},
                {QStringLiteral("requests"), writeCurveRequests(scenario.curveRequests)}});
        return {{QStringLiteral("bodies"), QJsonArray{}}, {QStringLiteral("joints"), QJsonArray{}},
            {QStringLiteral("drives"), QJsonArray{}}, {QStringLiteral("scenarios"), QJsonArray{}},
            {QStringLiteral("nativeMotion"), QJsonObject{{QStringLiteral("document"), model.nativeDocument->root},
                {QStringLiteral("bindings"), bindings}, {QStringLiteral("curveRequests"), curves}}}};
    }
    QJsonArray bodies, joints, drives, scenarios;
    for (const auto& body : model.bodies) {
        bodies.append(QJsonObject{
            {QStringLiteral("id"), body.id}, {QStringLiteral("name"), checkedName(body.name)},
            {QStringLiteral("partId"), body.partId}, {QStringLiteral("fixed"), body.fixed},
            {QStringLiteral("massKg"), body.massKg}, {QStringLiteral("densityKgM3"), body.densityKgM3}});
    }
    for (const auto& joint : model.joints) {
        joints.append(QJsonObject{
            {QStringLiteral("id"), joint.id}, {QStringLiteral("name"), checkedName(joint.name)},
            {QStringLiteral("kind"), QString::fromLatin1(jointKind(joint.kind))},
            {QStringLiteral("movingBodyId"), joint.movingBodyId}, {QStringLiteral("baseBodyId"), joint.baseBodyId},
            {QStringLiteral("originMm"), writeVector(joint.originMm)}, {QStringLiteral("axis"), writeVector(joint.axis)}});
    }
    for (const auto& drive : model.drives) {
        drives.append(QJsonObject{
            {QStringLiteral("id"), drive.id}, {QStringLiteral("name"), checkedName(drive.name)},
            {QStringLiteral("jointId"), drive.jointId}, {QStringLiteral("kind"), QString::fromLatin1(driveKind(drive.kind))},
            {QStringLiteral("value"), drive.value}});
    }
    for (const auto& scenario : model.scenarios) {
        QJsonObject row{
            {QStringLiteral("id"), scenario.id}, {QStringLiteral("name"), checkedName(scenario.name)},
            {QStringLiteral("duration"), scenario.duration}, {QStringLiteral("step"), scenario.step},
            {QStringLiteral("gravityMS2"), writeVector(scenario.gravityMS2)}};
        if (!scenario.curveRequests.empty())
            row.insert(QStringLiteral("curveRequests"), writeCurveRequests(scenario.curveRequests));
        scenarios.append(row);
    }
    return {{QStringLiteral("bodies"), bodies}, {QStringLiteral("joints"), joints},
            {QStringLiteral("drives"), drives}, {QStringLiteral("scenarios"), scenarios}};
}

QByteArray readEmbeddedPayload(const QJsonValue& value, int maxBytes, int& remaining)
{
    require(value.isString(), QStringLiteral("Expected an embedded Base64 payload."));
    const auto text = value.toString();
    require(!text.isEmpty() && text.size() <= ((maxBytes + 2) / 3) * 4 && text.size() % 4 == 0,
            QStringLiteral("Invalid or oversized embedded payload."));
    const int size = (text.size() / 4) * 3 - (text.endsWith(QLatin1Char('=')) ? 1 : 0)
        - (text.endsWith(QLatin1String("==")) ? 1 : 0);
    require(size > 0 && size <= maxBytes && size <= remaining, QStringLiteral("Embedded payload exceeds project limits."));
    const QByteArray encoded = text.toLatin1(); // Non-Latin-1 converts to '?' and is rejected by the decoder.
    auto decoded = QByteArray::fromBase64Encoding(encoded, QByteArray::AbortOnBase64DecodingErrors);
    require(decoded && decoded.decoded.size() == size && decoded.decoded.toBase64() == encoded,
            QStringLiteral("Invalid or noncanonical embedded Base64."));
    remaining -= size;
    return std::move(decoded.decoded);
}

QJsonArray writeProjectAssets(const NativeMotionDocument& document, QJsonObject& root, int& remaining)
{
    auto motion = root.value("motionData").toObject();
    auto imports = motion.value("importedMnfBodies").toArray();
    QJsonArray assets;
    QMap<QString, QString> bySource;
    for (int row = 0; row < imports.size(); ++row) {
        auto definition = imports.at(row).toObject();
        const auto source = document.assetPath(definition.value("mnfFile").toString());
        const QFileInfo info(source);
        require(info.isFile(), QStringLiteral("找不到 MNF 文件 %1，无法将其打包到工程。").arg(source));
        const auto canonical = info.canonicalFilePath();
        const auto sourceKey = canonical.isEmpty() ? info.absoluteFilePath() : canonical;
        auto name = bySource.value(sourceKey);
        if (name.isEmpty()) {
            require(assets.size() < kMaxAssets, QStringLiteral("工程包含过多 MNF 文件。"));
            const int maximum = (std::min)(remaining, kMaxMnfBytes);
            require(info.size() > 0 && info.size() <= maximum,
                QStringLiteral("MNF 文件 %1 为空或超过工程大小上限。").arg(info.fileName()));
            QFile file(source);
            if (!file.open(QIODevice::ReadOnly))
                fail(QStringLiteral("无法读取 MNF 文件 %1：%2").arg(info.fileName(), file.errorString()));
            const auto payload = file.read(maximum + 1);
            require(file.error() == QFile::NoError && payload.size() == info.size() && file.atEnd(),
                QStringLiteral("MNF 文件 %1 未完整读取或在保存期间发生变化。").arg(info.fileName()));
            remaining -= payload.size();
            name = QStringLiteral("mnf-%1.mnf").arg(assets.size() + 1);
            bySource.insert(sourceKey, name);
            assets.append(QJsonObject{{"name", name}, {"data", QString::fromLatin1(payload.toBase64())}});
        }
        definition["mnfFile"] = name;
        imports[row] = definition;
    }
    if (!imports.isEmpty()) motion["importedMnfBodies"] = imports;

    auto flexBodies = motion.value("autoFlexBodies").toArray();
    for (int row = 0; row < flexBodies.size(); ++row) {
        auto definition = flexBodies.at(row).toObject();
        const auto source = definition.value("meshFile").toString();
        if (!source.isEmpty()) {
            FemMesh mesh;
            QString detail;
            if (!readNetgenMesh(document.assetPath(source), MeshKind::Volume, mesh, detail)) fail(detail);
            auto embedded = definition.value("mesh").toObject();
            const auto normalized = femMeshJson(mesh);
            for (auto it = normalized.constBegin(); it != normalized.constEnd(); ++it)
                embedded.insert(it.key(), it.value());
            definition["mesh"] = embedded;
            definition["meshFile"] = QString{};
            definition["meshSourceFile"] = source;
        }
        auto provenance = definition.value("meshSourceFile").toString();
        if (!provenance.isEmpty()) {
            provenance.replace('\\', '/');
            definition["meshSourceFile"] = provenance.section('/', -1);
        }
        flexBodies[row] = definition;
    }
    if (!flexBodies.isEmpty()) motion["autoFlexBodies"] = flexBodies;
    root["motionData"] = motion;
    return assets;
}

void readProjectAssets(const QJsonValue& value, NativeMotionDocument& document, int& remaining)
{
    const auto items = array(value, kMaxAssets, "project assets");
    QSet<QString> required;
    for (const auto& row : document.collection("importedMnfBodies"))
        required.insert(row.toObject().value("mnfFile").toString());
    for (const auto& row : document.collection("autoFlexBodies"))
        require(row.toObject().value("meshFile").toString().isEmpty(),
            QStringLiteral("Portable project has an external finite-element mesh."));
    require(items.isEmpty() == required.isEmpty(), QStringLiteral("Project MNF attachments do not match the model."));
    if (items.isEmpty()) return;
    auto directory = std::make_shared<QTemporaryDir>();
    if (!directory->isValid())
        fail(QStringLiteral("无法创建 MNF 附件缓存：%1").arg(directory->errorString()));
    static const QRegularExpression pattern(QStringLiteral("\\Amnf-[1-9][0-9]{0,5}\\.mnf\\z"));
    for (const auto& item : items) {
        const auto row = record(item, {"name", "data"}, "project asset");
        const auto filename = row.value("name").toString();
        require(pattern.match(filename).hasMatch() && required.remove(filename),
            QStringLiteral("Invalid, duplicate or unreferenced project MNF attachment."));
        const auto bytes = readEmbeddedPayload(row.value("data"), kMaxMnfBytes, remaining);
        QSaveFile asset(directory->filePath(filename));
        openOutput(asset);
        writeBytes(asset, bytes);
        if (!asset.commit()) fail(asset.errorString());
        document.projectAssetNames.insert(filename);
    }
    require(required.isEmpty(), QStringLiteral("Project is missing an embedded MNF attachment."));
    document.projectAssetDirectory = std::move(directory);
}

RigidModel readModel(const QJsonValue& value, bool portable, const QString& projectDirectory, int& remaining)
{
    auto data = value.toObject();
    const auto native = data.take(QStringLiteral("nativeMotion"));
    const auto object = record(value.isObject() ? QJsonValue(data) : value, {"bodies", "joints", "drives", "scenarios"}, "model");
    RigidModel model;
    if (!native.isUndefined()) {
        for (const auto* key : {"bodies", "joints", "drives", "scenarios"})
            require(array(object.value(QLatin1String(key)), 0, key).isEmpty(), QStringLiteral("Native project has duplicate model records."));
        const auto row = portable
            ? record(native, {"document", "bindings", "curveRequests", "assets"}, "native motion")
            : record(native, {"document", "bindings", "curveRequests", "baseDirectory"}, "native motion");
        auto document = std::make_shared<NativeMotionDocument>();
        require(row.value("document").isObject() && (portable || row.value("baseDirectory").isString()),
            QStringLiteral("Invalid native document."));
        document->root = row.value(QStringLiteral("document")).toObject();
        document->baseDirectory = portable ? projectDirectory : row.value("baseDirectory").toString();
        document->ownsTopologyCad = true;
        QString detail;
        if (!validateNativeMotionDocument(*document, detail)) fail(detail);
        if (portable) readProjectAssets(row.value("assets"), *document, remaining);
        for (const auto& binding : array(row.value(QStringLiteral("bindings")), kMaxParts, "bindings")) {
            const auto item = record(binding, {"bodyId", "partIds"}, "native binding");
            const int body = id(item, "bodyId", 0);
            require(!document->bodyPartBindings.contains(body), QStringLiteral("Duplicate body binding."));
            QVector<int> ids;
            for (const auto& part : array(item.value(QStringLiteral("partIds")), kMaxParts, "partIds"))
                ids.append(static_cast<int>(integer(part, 1, kMaxId, "partId")));
            document->bodyPartBindings.insert(body, ids);
        }
        model.nativeDocument = std::move(document);
        syncNativeMotionModel(model);
        std::set<int> seen;
        for (const auto& curves : array(row.value(QStringLiteral("curveRequests")), kMaxScenarios, "curveRequests")) {
            const auto item = record(curves, {"scenarioId", "requests"}, "native scenario curves");
            const int scenarioId = id(item, "scenarioId", 0);
            require(seen.insert(scenarioId).second, QStringLiteral("Duplicate scenario curves."));
            const auto scenario = std::find_if(model.scenarios.begin(), model.scenarios.end(),
                [=](const auto& candidate) { return candidate.id == scenarioId; });
            require(scenario != model.scenarios.end(), QStringLiteral("Curve request refers to an unknown scenario."));
            scenario->curveRequests = readCurveRequests(item.value(QStringLiteral("requests")));
        }
        return model;
    }
    for (const auto& item : array(object.value(QStringLiteral("bodies")), kMaxBodies, "bodies")) {
        const auto row = record(item, {"id", "name", "partId", "fixed", "massKg", "densityKgM3"}, "body");
        model.bodies.push_back({id(row), name(row), id(row, "partId"),
            boolean(row.value(QStringLiteral("fixed")), "fixed"), number(row.value(QStringLiteral("massKg")), "massKg"),
            number(row.value(QStringLiteral("densityKgM3")), "densityKgM3")});
    }
    for (const auto& item : array(object.value(QStringLiteral("joints")), kMaxJoints, "joints")) {
        const auto row = record(item, {"id", "name", "kind", "movingBodyId", "baseBodyId", "originMm", "axis"}, "joint");
        model.joints.push_back({id(row), name(row), readJointKind(row.value(QStringLiteral("kind"))),
            id(row, "movingBodyId"), id(row, "baseBodyId", 0),
            readVector(row.value(QStringLiteral("originMm")), "originMm"), readVector(row.value(QStringLiteral("axis")), "axis")});
    }
    for (const auto& item : array(object.value(QStringLiteral("drives")), kMaxJoints, "drives")) {
        const auto row = record(item, {"id", "name", "jointId", "kind", "value"}, "drive");
        model.drives.push_back({id(row), name(row), id(row, "jointId"), readDriveKind(row.value(QStringLiteral("kind"))),
            number(row.value(QStringLiteral("value")), "value")});
    }
    for (const auto& item : array(object.value(QStringLiteral("scenarios")), kMaxScenarios, "scenarios")) {
        auto data = item.toObject();
        const bool hasRequests = data.contains(QStringLiteral("curveRequests"));
        const auto requests = data.take(QStringLiteral("curveRequests"));
        const auto row = record(item.isObject() ? QJsonValue(data) : item,
            {"id", "name", "duration", "step", "gravityMS2"}, "scenario");
        RigidScenario scenario{id(row), name(row), number(row.value(QStringLiteral("duration")), "duration"),
            number(row.value(QStringLiteral("step")), "step"), readVector(row.value(QStringLiteral("gravityMS2")), "gravityMS2")};
        if (hasRequests) scenario.curveRequests = readCurveRequests(requests);
        model.scenarios.push_back(std::move(scenario));
    }
    return model;
}

std::array<double, 12> readPlacement(const QJsonValue& value)
{
    const auto entries = array(value, 12, "part placement");
    require(entries.size() == 12, QStringLiteral("Expected a 3×4 rigid part placement."));
    std::array<double, 12> placement;
    for (int i = 0; i < entries.size(); ++i) placement[i] = number(entries[i], "part placement");
    return placement;
}

void checkModel(const CadDocument& cad, const RigidModel& model)
{
    QString error;
    if (!validateRigidModel(cad, model, error))
        fail(error.isEmpty() ? QStringLiteral("Invalid rigid model.") : error);
}

constexpr char kCsvHeader[] = "time_s,entity_type,entity_id,name,joint_kind,"
    "position_x_mm,position_y_mm,position_z_mm,displacement_x_mm,displacement_y_mm,displacement_z_mm,"
    "velocity_x_mm_s,velocity_y_mm_s,velocity_z_mm_s,speed_mm_s,"
    "angular_velocity_x_deg_s,angular_velocity_y_deg_s,angular_velocity_z_deg_s,angular_speed_deg_s,"
    "joint_coordinate_mm,joint_rate_mm_s,joint_coordinate_deg,joint_rate_deg_s\r\n";

QByteArray quoteCsvName(const QString& name)
{
    auto bytes = checkedName(name).toUtf8();
    const QString trimmed = name.trimmed();
    // Keep names as text even when a spreadsheet opens an untrusted project's CSV.
    if (!trimmed.isEmpty() && QStringLiteral("=+-@＝＋－＠").contains(trimmed.front())) bytes.prepend('\'');
    return QByteArray(1, '"') + bytes.replace("\"", "\"\"") + '"';
}

QByteArray csvNumber(double value)
{
    if (!std::isfinite(value)) fail(QStringLiteral("Result contains a non-finite value."));
    return QByteArray::number(value, 'g', std::numeric_limits<double>::max_digits10);
}

void appendNumber(QByteArray& row, double value) { row += ','; row += csvNumber(value); }

void appendVector(QByteArray& row, const Vector3d& vector)
{
    for (const auto coordinate : vector) appendNumber(row, coordinate);
}

QByteArray rowPrefix(const QByteArray& time, const char* type, int id, const QByteArray& name)
{
    return time + ',' + type + ',' + QByteArray::number(id) + ',' + name + ',';
}

template<class State>
std::map<int, const State*> indexedStates(const std::vector<State>& states, int State::*stateId, std::size_t expected)
{
    require(states.size() == expected, QStringLiteral("Frame entity count does not match the rigid input."));
    std::map<int, const State*> indexed;
    for (const auto& state : states)
        if (!indexed.emplace(state.*stateId, &state).second) fail(QStringLiteral("Frame contains duplicate entity IDs."));
    return indexed;
}

} // namespace

bool saveRigidProject(const QString& path, const CadDocument& cad, const RigidModel& model, QString& error)
{
    return run(QStringLiteral("Could not save rigid project"), error, [&] {
        checkProjectPath(path);
        checkModel(cad, model);
        int remaining = kMaxEmbeddedBytes;
        auto modelObject = writeModel(model);
        QJsonObject projectDocument;
        QSet<int> referencedParts;
        if (model.nativeDocument) {
            auto native = modelObject.value("nativeMotion").toObject();
            projectDocument = model.nativeDocument->root;
            native["assets"] = writeProjectAssets(*model.nativeDocument, projectDocument, remaining);
            QString detail;
            if (!encodeNativeProjectTopology(cad, projectDocument, detail, &referencedParts)) fail(detail);
            native["document"] = projectDocument;
            modelObject["nativeMotion"] = native;
        }
        require(cad.parts().size() <= kMaxParts, QStringLiteral("Too many CAD parts."));
        QJsonArray parts;
        std::vector<CadDocument::ProjectPart> referenceShapes;
        referenceShapes.reserve(referencedParts.size());
        for (const auto& part : cad.parts()) {
            const auto partName = checkedName(part.name);
            QByteArray step;
            std::array<double, 12> placement;
            QString detail;
            if (!CadDocument::encodeProjectStep(part, step, placement, (std::min)(remaining, kMaxPartBytes), detail))
                fail(QStringLiteral("Could not store STEP for part %1: %2").arg(partName, detail));
            remaining -= step.size();
            QJsonArray placementJson;
            for (double value : placement) placementJson.append(value);
            QJsonObject row{{QStringLiteral("id"), part.id}, {QStringLiteral("name"), partName},
                {"geometry", QJsonObject{{"format", "step"}, {"encoding", "base64"},
                                         {"data", QString::fromLatin1(step.toBase64())}}},
                {"placement", placementJson}};
            if (!part.sourceName.isEmpty()) row.insert(QStringLiteral("sourceName"), part.sourceName);
            if (!part.instancePath.isEmpty()) row.insert(QStringLiteral("instancePath"), part.instancePath);
            if (part.transparencyPercent != 0) row.insert(QStringLiteral("transparencyPercent"), part.transparencyPercent);
            if (part.displayColor.isValid()) {
                const auto color = part.displayColor.rgba64();
                row.insert(QStringLiteral("displayColor"), QStringLiteral("#%1%2%3")
                    .arg(color.red(), 4, 16, QLatin1Char('0')).arg(color.green(), 4, 16, QLatin1Char('0'))
                    .arg(color.blue(), 4, 16, QLatin1Char('0')));
            }
            if (part.tessellatedDisplayOnly) row.insert(QStringLiteral("tessellatedDisplayOnly"), true);
            parts.append(row);
            if (referencedParts.contains(part.id))
                referenceShapes.push_back({part.id, partName, std::move(step), part.sourceName,
                    part.instancePath, CadDocument::ProjectPartEncoding::Step, placement,
                    part.transparencyPercent, part.displayColor, part.tessellatedDisplayOnly});
        }
        if (!referenceShapes.empty()) {
            // Verify selections still resolve after STEP translation before replacing an existing file.
            CadDocument restoredReferences;
            QString detail;
            if (!restoredReferences.restoreProjectParts(referenceShapes, cad.m_nextId, detail, false) ||
                !decodeNativeProjectTopology(restoredReferences, projectDocument, detail))
                fail(detail);
        }
        const QJsonObject root{{QStringLiteral("format"), QStringLiteral("mxmotion")}, {QStringLiteral("version"), kVersion},
            {"lengthUnit", "mm"}, {QStringLiteral("nextPartId"), static_cast<double>(cad.m_nextId)},
            {QStringLiteral("parts"), parts}, {QStringLiteral("model"), modelObject}};
        const auto bytes = QJsonDocument(root).toJson(QJsonDocument::Compact);
        require(bytes.size() <= kMaxProjectBytes, QStringLiteral("Project exceeds 128 MiB."));
        countSourceMembers(bytes);
        QSaveFile file(path);
        openOutput(file);
        writeBytes(file, bytes);
        if (!file.commit()) fail(file.errorString());
    });
}

bool loadRigidProject(const QString& path, CadDocument& cad, RigidModel& model, QString& error)
{
    return run(QStringLiteral("Could not load rigid project"), error, [&] {
        RigidModel restoredModel;
        std::uint64_t nextId;
        bool legacy = false;
        std::vector<CadDocument::ProjectPart> parts;
        {
            const auto root = readProject(path);
            require(root.value(QStringLiteral("format")).toString() == QStringLiteral("mxmotion"),
                    QStringLiteral("Not an MxMotion project."));
            const auto version = number(root.value(QStringLiteral("version")), "version");
            legacy = version == kLegacyVersion;
            require(legacy || version == kStepVersion || version == kVersion,
                    QStringLiteral("Unsupported MxMotion project version."));
            if (legacy) {
                record(root, {"format", "version", "nextPartId", "parts", "model"}, "project");
            } else {
                record(root, {"format", "version", "lengthUnit", "nextPartId", "parts", "model"}, "project");
                require(root.value("lengthUnit").toString() == "mm", QStringLiteral("Unsupported project length unit."));
            }
            nextId = integer(root.value(QStringLiteral("nextPartId")), 1, kExhaustedPartId, "nextPartId");
            int remaining = kMaxEmbeddedBytes;
            restoredModel = readModel(root.value("model"), version == kVersion, QFileInfo(path).absolutePath(), remaining);
            const auto items = array(root.value(QStringLiteral("parts")), kMaxParts, "parts");
            parts.reserve(static_cast<std::size_t>(items.size()));
            for (const auto& item : items) {
                auto data = item.toObject();
                const auto sourceName = data.take(QStringLiteral("sourceName"));
                const auto instancePath = data.take(QStringLiteral("instancePath"));
                const auto transparencyValue = data.take(QStringLiteral("transparencyPercent"));
                const auto colorValue = data.take(QStringLiteral("displayColor"));
                const auto tessellatedValue = data.take(QStringLiteral("tessellatedDisplayOnly"));
                const int transparency = transparencyValue.isUndefined() ? 0 :
                    static_cast<int>(integer(transparencyValue, 0, 100, "transparencyPercent"));
                const QColor displayColor(colorValue.toString());
                require(colorValue.isUndefined() || (colorValue.isString() && displayColor.isValid()),
                    QStringLiteral("部件显示颜色无效。"));
                require(tessellatedValue.isUndefined() || (tessellatedValue.isBool() && !legacy),
                    QStringLiteral("部件的三角显示标记无效。"));
                require((sourceName.isUndefined() || sourceName.isString()) && (instancePath.isUndefined() || instancePath.isString()),
                    QStringLiteral("Invalid CAD instance metadata."));
                const auto value = item.isObject() ? QJsonValue(data) : item;
                if (legacy) {
                    const auto row = record(value, {"id", "name", "brep"}, "CAD part");
                    parts.push_back({id(row), name(row), readEmbeddedPayload(row.value("brep"), kMaxPartBytes, remaining),
                        checkedName(sourceName.toString()), instancePath.toString(), CadDocument::ProjectPartEncoding::LegacyBrep,
                        {}, transparency, displayColor});
                } else {
                    const auto row = record(value, {"id", "name", "geometry", "placement"}, "CAD part");
                    const auto geometry = record(row.value("geometry"), {"format", "encoding", "data"}, "CAD geometry");
                    require(geometry.value("format").toString() == "step" &&
                            geometry.value("encoding").toString() == "base64",
                            QStringLiteral("Unsupported embedded CAD format or encoding."));
                    parts.push_back({id(row), name(row), readEmbeddedPayload(geometry.value("data"), kMaxPartBytes, remaining),
                        checkedName(sourceName.toString()), instancePath.toString(), CadDocument::ProjectPartEncoding::Step,
                        readPlacement(row.value("placement")), transparency, displayColor, tessellatedValue.toBool()});
                }
            }
        }
        CadDocument restoredCad;
        QString detail;
        if (!restoredCad.restoreProjectParts(parts, nextId, detail)) fail(detail);
        if (!legacy && restoredModel.nativeDocument &&
            !decodeNativeProjectTopology(restoredCad, restoredModel.nativeDocument->root, detail)) fail(detail);
        checkModel(restoredCad, restoredModel);
        // Both assignments move standard-allocator vectors and cannot allocate.
        cad = std::move(restoredCad);
        model = std::move(restoredModel);
    });
}

bool exportRigidCsv(const QString& path, const RigidInput& input, const RigidResult& result, QString& error)
{
    return run(QStringLiteral("Could not export rigid results"), error, [&] {
        checkPath(path);
        require(result.scenarioId == input.scenario.id && !result.frames.empty(),
                QStringLiteral("No results for the input scenario."));
        require(!input.bodies.empty() || !input.joints.empty(), QStringLiteral("No entities to export."));

        if (input.nativeInput) {
            QSaveFile file(path);
            openOutput(file);
            writeBytes(file, "time_s,entity_type,entity_id,name,variable,component,unit,value\r\n");
            std::set<QString> channelKeys;
            for (const auto& channel : result.channels) {
                const QString key = channel.sourceType + QLatin1Char(':') + QString::number(channel.sourceId)
                    + QLatin1Char(':') + channel.variable;
                require(channelKeys.insert(key).second && channel.components > 0 && channel.components <= 1024
                    && channel.values.size() == result.frames.size() * std::size_t(channel.components),
                    QStringLiteral("Invalid or duplicate native result channel: %1").arg(key));
                QString label = input.nativeInput->document.entity(channel.sourceType, channel.sourceId)
                    .value(QStringLiteral("name")).toString();
                if (label.isEmpty()) label = channel.sourceType + QString::number(channel.sourceId);
                double previous = -1.0;
                for (std::size_t frame = 0; frame < result.frames.size(); ++frame) {
                    const double time = result.frames[frame].seconds;
                    require(std::isfinite(time) && time >= 0 && time > previous, QStringLiteral("Invalid result frame times."));
                    previous = time;
                    for (int component = 0; component < channel.components; ++component) {
                        QByteArray row = csvNumber(time) + ',' + quoteCsvName(channel.sourceType) + ','
                            + QByteArray::number(channel.sourceId) + ',' + quoteCsvName(label) + ','
                            + quoteCsvName(channel.variable) + ',' + QByteArray::number(component) + ','
                            + quoteCsvName(channel.unit) + ','
                            + csvNumber(channel.values[frame * std::size_t(channel.components) + std::size_t(component)]) + "\r\n";
                        writeBytes(file, row);
                    }
                }
            }
            require(!result.channels.empty(), QStringLiteral("Native result has no channels to export."));
            if (!file.commit()) fail(file.errorString());
            return;
        }

        std::map<int, QByteArray> bodyNames;
        for (const auto& item : input.bodies)
            require(item.body.id > 0 && bodyNames.emplace(item.body.id, quoteCsvName(item.body.name)).second,
                    QStringLiteral("Invalid or duplicate body ID in the rigid input."));
        struct JointInfo { JointKind kind; QByteArray name; };
        std::map<int, JointInfo> jointInfo;
        for (const auto& joint : input.joints)
            require(joint.id > 0 && jointInfo.emplace(joint.id, JointInfo{joint.kind, quoteCsvName(joint.name)}).second,
                    QStringLiteral("Invalid or duplicate joint ID in the rigid input."));
        const auto initial = indexedStates(result.frames.front().bodies, &BodyState::bodyId, bodyNames.size());

        QSaveFile file(path);
        openOutput(file);
        writeBytes(file, QByteArray::fromRawData(kCsvHeader, sizeof(kCsvHeader) - 1));
        double previous = -1.0;
        for (const auto& frame : result.frames) {
            require(std::isfinite(frame.seconds) && frame.seconds >= 0.0 && frame.seconds > previous,
                    QStringLiteral("Frame times must be finite and strictly increasing."));
            previous = frame.seconds;
            const auto time = csvNumber(frame.seconds);
            const auto bodies = indexedStates(frame.bodies, &BodyState::bodyId, bodyNames.size());
            const auto joints = indexedStates(frame.joints, &JointState::jointId, jointInfo.size());
            for (const auto& entry : bodies) {
                const auto label = bodyNames.find(entry.first);
                const auto start = initial.find(entry.first);
                require(label != bodyNames.end() && start != initial.end(), QStringLiteral("Frame refers to an unknown body."));
                const auto& state = *entry.second;
                Vector3d displacement{};
                for (std::size_t i = 0; i < displacement.size(); ++i)
                    displacement[i] = state.positionMm[i] - start->second->positionMm[i];
                auto row = rowPrefix(time, "body", entry.first, label->second);
                appendVector(row, state.positionMm);
                appendVector(row, displacement);
                appendVector(row, state.velocityMmS);
                const auto& v = state.velocityMmS;
                appendNumber(row, std::hypot(v[0], v[1], v[2]));
                appendVector(row, state.angularVelocityDegS);
                const auto& w = state.angularVelocityDegS;
                appendNumber(row, std::hypot(w[0], w[1], w[2]));
                row += ",,,,\r\n";
                writeBytes(file, row);
            }
            for (const auto& entry : joints) {
                const auto label = jointInfo.find(entry.first);
                require(label != jointInfo.end(), QStringLiteral("Frame refers to an unknown joint."));
                const auto kind = label->second.kind;
                auto row = rowPrefix(time, "joint", entry.first, label->second.name);
                row += jointKind(kind);
                row.append(14, ','); // Empty body columns, from position_x_mm through angular_speed_deg_s.
                if (kind == JointKind::Prismatic) {
                    appendNumber(row, entry.second->position);
                    appendNumber(row, entry.second->speed);
                    row += ",,";
                } else if (kind == JointKind::Revolute) {
                    row += ",,";
                    appendNumber(row, entry.second->position);
                    appendNumber(row, entry.second->speed);
                } else {
                    row += ",,,,";
                }
                row += "\r\n";
                writeBytes(file, row);
            }
        }
        if (!file.commit()) fail(file.errorString());
    });
}

} // namespace MotionOcc
