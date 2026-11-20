#include "CadDocument.h"
#include "RigidMotion.h"
#include "RigidProject.h"

#include <BRepTools.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QTemporaryDir>

#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace {
using namespace MotionOcc;

constexpr Vector3d kSizeMm{120.0, 80.0, 40.0};
constexpr Vector3d kCenterMm{60.0, 40.0, 20.0};
constexpr Vector3d kHingeOriginMm{17.0, -13.0, 11.0};
constexpr Vector3d kHingeAxis{2.0 / 7.0, -3.0 / 7.0, 6.0 / 7.0};
constexpr Vector3d kSliderAxis{0.0, 0.6, 0.8};
constexpr double kRadiansPerDegree = 3.14159265358979323846 / 180.0;
constexpr double kHingeRate = 105.0;
constexpr double kSliderTravel = -37.3;
constexpr double kSliderRate = -100.0;

void require(bool ok, const QString& message)
{
    if (!ok) throw std::runtime_error(message.toStdString());
}

void checked(bool ok, const QString& context, const QString& error)
{
    if (!ok) require(false, context + QStringLiteral(": ") + error);
}

void expectNear(double actual, double expected, double tolerance, const QString& context)
{
    if (!std::isfinite(actual) || std::abs(actual - expected) > tolerance) {
        require(false, QStringLiteral("%1: got %2, expected %3 (tolerance %4)")
                        .arg(context).arg(actual, 0, 'g', 16).arg(expected, 0, 'g', 16).arg(tolerance));
    }
}

template <std::size_t N>
void expectNear(const std::array<double, N>& actual, const std::array<double, N>& expected,
          double tolerance, const QString& context)
{
    for (std::size_t i = 0; i < N; ++i)
        expectNear(actual[i], expected[i], tolerance, QStringLiteral("%1[%2]").arg(context).arg(int(i)));
}

template <std::size_t N>
double dot(const std::array<double, N>& a, const std::array<double, N>& b)
{
    double sum = 0.0;
    for (std::size_t i = 0; i < N; ++i) sum += a[i] * b[i];
    return sum;
}

Vector3d sum(const Vector3d& a, const Vector3d& b)
{
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

Vector3d scale(const Vector3d& a, double s)
{
    return {a[0] * s, a[1] * s, a[2] * s};
}

Vector3d cross(const Vector3d& a, const Vector3d& b)
{
    return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}

int addBox(CadDocument& cad)
{
    int id = 0;
    QString error;
    checked(cad.addPrimitive(Primitive::Box, kSizeMm[0], kSizeMm[1], kSizeMm[2], id, error),
            QStringLiteral("Create analytic box"), error);
    const auto* part = cad.part(id);
    require(id > 0 && part && !part->shape.IsNull() && part->shape.ShapeType() == TopAbs_SOLID,
            QStringLiteral("Analytic box is not a CAD solid"));
    return id;
}

RigidModel hinge(int partId)
{
    return {{{41, QStringLiteral("转子 é"), partId, false, 2.5, 2500.0}},
            {{73, QStringLiteral("斜轴铰链"), JointKind::Revolute, 41, 0, kHingeOriginMm, {2, -3, 6}}},
            {{109, QStringLiteral("恒速驱动"), 73, DriveKind::Speed, kHingeRate}},
            {{151, QStringLiteral("偏置旋转"), 2.003, 0.005, {0.4, 0.1, -9.8}}}};
}

void checkMass(const BodyProperties& p, double mass)
{
    expectNear(p.massKg, mass, 1e-9, QStringLiteral("Mass in kg"));
    expectNear(p.centerMm, kCenterMm, 1e-7, QStringLiteral("Centroid in mm"));
    const double x = kSizeMm[0] / 1000.0, y = kSizeMm[1] / 1000.0, z = kSizeMm[2] / 1000.0;
    const std::array<double, 6> inertia{mass * (y*y + z*z) / 12.0, mass * (x*x + z*z) / 12.0,
                                        mass * (x*x + y*y) / 12.0, 0.0, 0.0, 0.0};
    expectNear(p.inertiaKgM2, inertia, 1e-10, QStringLiteral("Centroidal inertia tensor in kg m^2"));
}

RigidInput prepare(const CadDocument& cad, const RigidModel& model)
{
    QString error;
    RigidInput input;
    checked(prepareRigidModel(cad, model, model.scenarios.front().id, input, error),
            QStringLiteral("Prepare %1").arg(model.scenarios.front().name), error);
    return input;
}

RigidResult run(const RigidInput& input)
{
    QString error;
    RigidResult result;
    checked(solveRigid(input, result, error) == SolveStatus::Completed,
            QStringLiteral("Solve %1").arg(input.scenario.name), error);
    const auto& scenario = input.scenario;
    const auto steps = std::size_t(std::ceil(scenario.duration / scenario.step));
    require(result.scenarioId == scenario.id && result.frames.size() == steps + 1,
            QStringLiteral("Unexpected result scenario or number of samples"));
    require(result.frames.front().seconds == 0.0 && result.frames.back().seconds == scenario.duration,
            QStringLiteral("Result must start at zero and end exactly at the requested time"));
    for (std::size_t i = 0; i < result.frames.size(); ++i) {
        const auto& frame = result.frames[i];
        const QString context = QStringLiteral("%1 frame %2").arg(scenario.name).arg(qulonglong(i));
        expectNear(frame.seconds, i == steps ? scenario.duration : double(i) * scenario.step, 1e-12,
             context + QStringLiteral(" timestamp in seconds"));
        require(i == 0 || frame.seconds > result.frames[i - 1].seconds, context + QStringLiteral(" not advancing"));
        require(frame.bodies.size() == 1 && frame.bodies[0].bodyId == input.bodies[0].body.id &&
                frame.joints.size() == 1 && frame.joints[0].jointId == input.joints[0].id,
                context + QStringLiteral(" missing body or joint sample"));
        const auto& b = frame.bodies[0];
        const auto& j = frame.joints[0];
        require(std::isfinite(dot(b.positionMm, b.positionMm)) && std::isfinite(dot(b.velocityMmS, b.velocityMmS)) &&
                std::isfinite(dot(b.angularVelocityDegS, b.angularVelocityDegS)) &&
                std::isfinite(j.position) && std::isfinite(j.speed), context + QStringLiteral(" has non-finite samples"));
        expectNear(dot(b.rotation, b.rotation), 1.0, 1e-8, context + QStringLiteral(" quaternion norm squared"));
    }
    const auto& first = result.frames.front();
    expectNear(first.bodies[0].positionMm, kCenterMm, 1e-7, QStringLiteral("Initial COM"));
    expectNear(std::abs(first.bodies[0].rotation[0]), 1.0, 1e-8, QStringLiteral("Initial CAD orientation"));
    expectNear(first.joints[0].position, 0.0, 1e-8, QStringLiteral("Initial joint coordinate"));
    return result;
}

void checkHinge(const RigidFrame& frame)
{
    // Rodrigues' formula about the world-space anchor (not about the CAD COM).
    const double angle = frame.seconds * kHingeRate * kRadiansPerDegree;
    const Vector3d arm = sum(kCenterMm, scale(kHingeOriginMm, -1.0));
    const Vector3d rotated = sum(sum(scale(arm, std::cos(angle)), scale(cross(kHingeAxis, arm), std::sin(angle))),
                                 scale(kHingeAxis, dot(kHingeAxis, arm) * (1.0 - std::cos(angle))));
    const auto& b = frame.bodies[0];
    const auto& j = frame.joints[0];
    const auto halfAxis = scale(kHingeAxis, std::sin(angle / 2.0));
    const Quaterniond rotation{std::cos(angle / 2.0), halfAxis[0], halfAxis[1], halfAxis[2]};
    // Native time integration and constraint projection need not be algebraically exact.
    expectNear(j.position, frame.seconds * kHingeRate, 0.15, QStringLiteral("Continuous hinge angle in degrees"));
    expectNear(j.speed, kHingeRate, 0.4, QStringLiteral("Hinge speed in deg/s"));
    expectNear(b.positionMm, sum(kHingeOriginMm, rotated), 0.25, QStringLiteral("Orbiting world COM in mm"));
    expectNear(b.velocityMmS, scale(cross(kHingeAxis, rotated), kHingeRate * kRadiansPerDegree), 2.0,
         QStringLiteral("Tangential COM velocity in mm/s"));
    expectNear(b.angularVelocityDegS, scale(kHingeAxis, kHingeRate), 0.4, QStringLiteral("World angular velocity in deg/s"));
    expectNear(std::abs(dot(b.rotation, rotation)), 1.0, 2e-6, QStringLiteral("World orientation about oblique axis"));
}

void unchanged(const RigidResult& actual, const RigidResult& before)
{
    require(actual.scenarioId == before.scenarioId && actual.frames.size() == before.frames.size(),
            QStringLiteral("Interrupted or failed solve replaced the previous result"));
    for (std::size_t i = 0; i < before.frames.size(); ++i) {
        const auto& a = actual.frames[i];
        const auto& b = before.frames[i];
        require(a.seconds == b.seconds && a.bodies.size() == 1 && a.joints.size() == 1 &&
                a.bodies[0].bodyId == b.bodies[0].bodyId && a.bodies[0].positionMm == b.bodies[0].positionMm &&
                a.bodies[0].rotation == b.bodies[0].rotation && a.bodies[0].velocityMmS == b.bodies[0].velocityMmS &&
                a.bodies[0].angularVelocityDegS == b.bodies[0].angularVelocityDegS &&
                a.joints[0].jointId == b.joints[0].jointId && a.joints[0].position == b.joints[0].position &&
                a.joints[0].speed == b.joints[0].speed, QStringLiteral("An existing solution frame was changed"));
    }
}

void checkFailures(const CadDocument& cad, const RigidModel& model, const RigidInput& input,
                   const RigidResult& previous, const QTemporaryDir& temporary)
{
    bool stop = false;
    SolverCallbacks callbacks;
    callbacks.progress = [&](double fraction) { stop = fraction >= 0.1 && fraction < 1.0; };
    callbacks.stopRequested = [&] { return stop; };
    QString error;
    auto keep = previous; // An unrelated, already-completed scenario.
    require(solveRigid(input, keep, error, callbacks) == SolveStatus::Stopped && stop,
            QStringLiteral("Expected cancellation after simulation began"));
    unchanged(keep, previous);

    auto invalid = input;
    invalid.scenario.step = 0.0;
    error.clear();
    require(solveRigid(invalid, keep, error) == SolveStatus::Failed && !error.isEmpty(),
            QStringLiteral("Solver accepted a zero time step or failed without a diagnostic"));
    unchanged(keep, previous);

    const auto reject = [&](const RigidModel& bad, const QString& why) {
        QString reason;
        require(!validateRigidModel(cad, bad, reason) && !reason.isEmpty(), QStringLiteral("Accepted ") + why);
    };
    auto bad = model;
    bad.joints[0].axis = {};
    reject(bad, QStringLiteral("a zero joint axis"));
    bad = model;
    bad.drives[0].jointId = 999;
    reject(bad, QStringLiteral("a drive referencing a missing joint"));
    bad = model;
    bad.bodies[0].densityKgM3 = std::numeric_limits<double>::quiet_NaN();
    reject(bad, QStringLiteral("non-finite density"));
    bad = model;
    bad.bodies[0].partId = 999;
    reject(bad, QStringLiteral("a body referencing a missing part"));

    // Import a real face: acceptable CAD, but not a volume from which to compute inertia.
    TopExp_Explorer faces(cad.part(model.bodies[0].partId)->shape, TopAbs_FACE);
    require(faces.More(), QStringLiteral("Box has no face for the non-solid fixture"));
    std::ostringstream brep;
    BRepTools::Write(faces.Current(), brep, false, false, TopTools_FormatVersion_CURRENT);
    const auto bytes = QByteArray::fromStdString(brep.str());
    QFile file(temporary.filePath(QStringLiteral("开放曲面.brep")));
    require(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.flush(),
            QStringLiteral("Could not write a face BREP"));
    file.close();
    CadDocument sheet;
    std::vector<int> ids;
    checked(sheet.importFile(file.fileName(), ids, error), QStringLiteral("Import non-solid CAD face"), error);
    require(ids.size() == 1, QStringLiteral("Expected one face part"));
    auto body = model.bodies[0];
    body.partId = ids[0];
    BodyProperties properties = input.bodies[0].properties;
    error.clear();
    require(!rigidBodyProperties(sheet, body, properties, error) && !error.isEmpty() &&
            properties.massKg == input.bodies[0].properties.massKg && properties.centerMm == input.bodies[0].properties.centerMm &&
            properties.inertiaKgM2 == input.bodies[0].properties.inertiaKgM2,
            QStringLiteral("Non-solid geometry must be rejected without replacing the existing mass properties"));
}

void sameModel(const RigidModel& a, const RigidModel& b)
{
    require(a.bodies.size() == 1 && a.joints.size() == 1 && a.drives.size() == 1 && a.scenarios.size() == 1,
            QStringLiteral("Project changed the rigid model counts"));
    const auto& x = a.bodies[0]; const auto& y = b.bodies[0];
    const auto& j = a.joints[0]; const auto& k = b.joints[0];
    const auto& d = a.drives[0]; const auto& e = b.drives[0];
    const auto& s = a.scenarios[0]; const auto& t = b.scenarios[0];
    require(x.id == y.id && x.name == y.name && x.partId == y.partId && x.fixed == y.fixed &&
            x.massKg == y.massKg && x.densityKgM3 == y.densityKgM3, QStringLiteral("Project changed the body definition"));
    require(j.id == k.id && j.name == k.name && j.kind == k.kind && j.movingBodyId == k.movingBodyId &&
            j.baseBodyId == k.baseBodyId && j.originMm == k.originMm && j.axis == k.axis,
            QStringLiteral("Project changed the joint definition"));
    require(d.id == e.id && d.name == e.name && d.jointId == e.jointId && d.kind == e.kind && d.value == e.value,
            QStringLiteral("Project changed the drive definition"));
    require(s.id == t.id && s.name == t.name && s.duration == t.duration && s.step == t.step &&
            s.gravityMS2 == t.gravityMS2, QStringLiteral("Project changed the scenario definition"));
    require(s.curveRequests.size() == t.curveRequests.size(), QStringLiteral("Project lost curve requests"));
    for (std::size_t i = 0; i < s.curveRequests.size(); ++i) {
        const auto& c = s.curveRequests[i];
        const auto& d = t.curveRequests[i];
        require(c.id == d.id && c.metric == d.metric && c.sourceId == d.sourceId && c.component == d.component &&
            c.name == d.name && c.color == d.color && c.visible == d.visible,
            QStringLiteral("Project changed a scenario's curve request"));
    }
}

QByteArray readFile(const QString& path)
{
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), QStringLiteral("Could not read %1: %2").arg(path, file.errorString()));
    return file.readAll();
}

void checkProject(const CadDocument& cad, const RigidModel& model, int lastPartId, const QTemporaryDir& temporary)
{
    const QString path = temporary.filePath(QStringLiteral("刚体_épreuve.mxmotion"));
    QString error;
    checked(saveRigidProject(path, cad, model, error), QStringLiteral("Save Unicode project"), error);
    const QByteArray bytes = readFile(path);
    const auto header = QJsonDocument::fromJson(bytes).object();
    require(header.value(QStringLiteral("format")).toString() == QStringLiteral("mxmotion") &&
            header.value(QStringLiteral("version")).toInt() == 1, QStringLiteral("Missing project format or version"));

    CadDocument loaded;
    RigidModel restored;
    checked(loadRigidProject(path, loaded, restored, error), QStringLiteral("Load Unicode project"), error);
    sameModel(restored, model);
    const int partId = model.bodies[0].partId;
    const auto* part = loaded.part(partId);
    require(loaded.parts().size() == 1 && part && part->id == partId && part->name == cad.part(partId)->name &&
            !part->display.triangles.empty(), QStringLiteral("Project lost a CAD ID, name or display geometry"));
    checkMass(prepare(loaded, restored).bodies[0].properties, 0.96);
    const CadPart before = *part;

    // The BREP and JSON are valid, but the body refers to a missing CAD part.
    // Change the CAD name too so a partial commit would be observable.
    auto corruptProject = header;
    auto parts = header.value(QStringLiteral("parts")).toArray();
    auto differentPart = parts.at(0).toObject();
    differentPart.insert(QStringLiteral("name"), QStringLiteral("Must not replace the loaded part"));
    parts.replace(0, differentPart);
    corruptProject.insert(QStringLiteral("parts"), parts);
    auto corruptModel = header.value(QStringLiteral("model")).toObject();
    auto bodies = corruptModel.value(QStringLiteral("bodies")).toArray();
    auto danglingBody = bodies.at(0).toObject();
    danglingBody.insert(QStringLiteral("partId"), 999);
    bodies.replace(0, danglingBody);
    corruptModel.insert(QStringLiteral("bodies"), bodies);
    corruptProject.insert(QStringLiteral("model"), corruptModel);
    const QString corruptPath = temporary.filePath(QStringLiteral("损坏.mxmotion"));
    QFile corrupt(corruptPath);
    const auto invalid = QJsonDocument(corruptProject).toJson();
    require(corrupt.open(QIODevice::WriteOnly) && corrupt.write(invalid) == invalid.size() && corrupt.flush(),
            QStringLiteral("Could not create corrupt project fixture"));
    corrupt.close();
    error.clear();
    require(!loadRigidProject(corruptPath, loaded, restored, error) && !error.isEmpty(),
            QStringLiteral("Loaded a corrupt project or failed without a diagnostic"));
    sameModel(restored, model);
    part = loaded.part(partId);
    require(loaded.parts().size() == 1 && part && part->id == before.id && part->name == before.name &&
            part->shape.IsEqual(before.shape) && part->display.vertices == before.display.vertices &&
            part->display.triangles == before.display.triangles && part->display.edges == before.display.edges,
            QStringLiteral("Loading corrupt data changed existing CAD"));
    require(addBox(loaded) == lastPartId + 1, QStringLiteral("Project did not preserve the next CAD part ID"));
}

void checkCurvePersistence(const CadDocument& cad, const RigidModel& model, const QTemporaryDir& temporary)
{
    auto configured = model;
    configured.scenarios[0].curveRequests = {
        {3, RigidCurveMetric::BodyPosition, model.bodies[0].id, 0, QStringLiteral("质心绝对坐标 X"), QStringLiteral("#1769aa"), true},
        {17, RigidCurveMetric::JointCoordinate, model.joints[0].id, 0, QStringLiteral("旋转副相对转角"), QStringLiteral("#d84a39"), false}
    };
    auto second = configured.scenarios[0];
    second.id += 1;
    second.name = QStringLiteral("独立曲线的第二方案");
    second.curveRequests = {
        {3, RigidCurveMetric::BodyDisplacement, model.bodies[0].id, 3, QStringLiteral("位移模"), {}, true}
    };
    configured.scenarios.push_back(second);
    const auto path = temporary.filePath(QStringLiteral("curve-requests.mxmotion"));
    QString error;
    checked(saveRigidProject(path, cad, configured, error), QStringLiteral("Save scenario curve requests"), error);
    const auto validBytes = readFile(path);
    CadDocument loaded;
    RigidModel restored;
    checked(loadRigidProject(path, loaded, restored, error), QStringLiteral("Restore scenario curve requests"), error);
    require(restored.scenarios.size() == 2 && restored.scenarios[0].curveRequests.size() == 2 &&
        restored.scenarios[1].curveRequests.size() == 1, QStringLiteral("Curve requests crossed scenario boundaries"));
    auto firstOnly = restored;
    firstOnly.scenarios.resize(1);
    auto expected = configured;
    expected.scenarios.resize(1);
    sameModel(firstOnly, expected);
    const auto& restoredSecond = restored.scenarios[1].curveRequests[0];
    require(restoredSecond.id == 3 && restoredSecond.metric == RigidCurveMetric::BodyDisplacement &&
        restoredSecond.component == 3 && restoredSecond.color.isEmpty(), QStringLiteral("Second scenario curve changed"));

    auto invalid = configured;
    invalid.scenarios[0].curveRequests[0].sourceId = 999999;
    require(!saveRigidProject(path, cad, invalid, error) && !error.isEmpty() && readFile(path) == validBytes,
        QStringLiteral("Dangling curve source overwrote a valid project"));
    invalid = configured;
    invalid.scenarios[0].curveRequests.push_back(invalid.scenarios[0].curveRequests[0]);
    require(!validateRigidModel(cad, invalid, error), QStringLiteral("Duplicate scenario-local curve IDs accepted"));
    invalid = configured;
    invalid.scenarios[0].curveRequests[1].component = 3;
    require(!validateRigidModel(cad, invalid, error), QStringLiteral("Scalar joint curve accepted a vector component"));
    invalid = configured;
    invalid.scenarios[0].curveRequests[0].color = QStringLiteral("not-a-color");
    require(!validateRigidModel(cad, invalid, error), QStringLiteral("Invalid curve color accepted"));
}

void checkCsv(const RigidInput& input, const RigidResult& result, const QString& path, bool revolute)
{
    QString error;
    checked(exportRigidCsv(path, input, result, error), QStringLiteral("Export rigid CSV"), error);
    auto bytes = readFile(path);
    if (bytes.startsWith("\xEF\xBB\xBF")) bytes.remove(0, 3);
    auto lines = bytes.trimmed().split('\n');
    require(lines.size() == 1 + 2 * int(result.frames.size()), QStringLiteral("CSV must contain one row per body/joint per frame"));
    const auto headers = lines.takeFirst().trimmed().split(',');
    const auto column = [&](const QByteArray& name) {
        const int index = headers.indexOf(name);
        require(index >= 0 && headers.count(name) == 1, QStringLiteral("Missing or duplicated CSV column: %1").arg(QString::fromLatin1(name)));
        return index;
    };
    const int time = column("time_s"), type = column("entity_type"), id = column("entity_id");
    const int px = column("position_x_mm"), dy = column("displacement_y_mm"), vz = column("velocity_z_mm_s");
    const int angular = column("angular_speed_deg_s"), kind = column("joint_kind");
    const int linearPosition = column("joint_coordinate_mm"), linearRate = column("joint_rate_mm_s");
    const int angle = column("joint_coordinate_deg"), angularRate = column("joint_rate_deg_s");
    const auto number = [&](const QList<QByteArray>& row, int index) {
        bool valid = false;
        const double value = row[index].toDouble(&valid);
        require(valid && std::isfinite(value), QStringLiteral("Invalid CSV number in %1").arg(QString::fromLatin1(headers[index])));
        return value;
    };
    for (int i = 0; i < int(result.frames.size()); ++i) {
        // Fixture names contain neither commas nor line breaks.
        const auto bodyRow = lines[2*i].trimmed().split(',');
        const auto jointRow = lines[2*i + 1].trimmed().split(',');
        require(bodyRow.size() == headers.size() && jointRow.size() == headers.size() &&
                bodyRow[type] == "body" && jointRow[type] == "joint" &&
                jointRow[kind] == (revolute ? "revolute" : "prismatic"), QStringLiteral("Malformed CSV entity rows"));
        const auto& frame = result.frames[std::size_t(i)];
        const auto& b = frame.bodies[0];
        const auto& j = frame.joints[0];
        require(number(bodyRow, id) == b.bodyId && number(jointRow, id) == j.jointId,
                QStringLiteral("CSV lost body/joint IDs"));
        expectNear(number(bodyRow, time), frame.seconds, 1e-12, QStringLiteral("CSV body timestamp"));
        expectNear(number(jointRow, time), frame.seconds, 1e-12, QStringLiteral("CSV joint timestamp"));
        expectNear(number(bodyRow, px), b.positionMm[0], 1e-8, QStringLiteral("CSV body X position (mm)"));
        expectNear(number(bodyRow, dy), b.positionMm[1] - result.frames.front().bodies[0].positionMm[1], 1e-8,
             QStringLiteral("CSV body Y displacement from the first frame (mm)"));
        expectNear(number(bodyRow, vz), b.velocityMmS[2], 1e-8, QStringLiteral("CSV body Z velocity (mm/s)"));
        expectNear(number(bodyRow, angular), std::sqrt(dot(b.angularVelocityDegS, b.angularVelocityDegS)), 1e-8,
             QStringLiteral("CSV angular speed magnitude (deg/s)"));
        expectNear(number(jointRow, revolute ? angle : linearPosition), j.position, 1e-8, QStringLiteral("CSV joint coordinate"));
        expectNear(number(jointRow, revolute ? angularRate : linearRate), j.speed, 1e-8, QStringLiteral("CSV joint rate"));
        require(jointRow[revolute ? linearPosition : angle].isEmpty() && jointRow[revolute ? linearRate : angularRate].isEmpty(),
                QStringLiteral("CSV populated incompatible joint units"));
    }
}

void writeExample(const QString& path, const CadDocument& cad, const RigidModel& model)
{
    const QFileInfo output(path);
    require(output.suffix().compare(QStringLiteral("mxmotion"), Qt::CaseInsensitive) == 0,
            QStringLiteral("Expected an .mxmotion example path: %1").arg(path));
    require(!output.exists() && !output.isSymLink(), QStringLiteral("Won't overwrite %1").arg(path));
    // saveRigidProject can replace existing files. Publish from the same directory
    // with QFile::rename instead, which refuses an existing destination.
    QTemporaryDir stage(output.absoluteDir().filePath(QStringLiteral(".motionocc-example-XXXXXX")));
    require(stage.isValid(), QStringLiteral("Could not stage example near %1: %2").arg(path, stage.errorString()));
    QFile file(stage.filePath(QStringLiteral("hinge.mxmotion")));
    QString error;
    checked(saveRigidProject(file.fileName(), cad, model, error), QStringLiteral("Save hinge example"), error);
    if (!file.rename(output.absoluteFilePath()))
        require(false, QStringLiteral("Could not publish %1: %2").arg(path, file.errorString()));
    qInfo().noquote() << "Wrote hinge example to" << output.absoluteFilePath();
}

void smoke(const QTemporaryDir& temporary, const QString& example)
{
    CadDocument cad;
    require(cad.remove(addBox(cad)), QStringLiteral("Could not create a gap in the CAD part IDs"));
    const auto hingeModel = hinge(addBox(cad));
    const int lastPartId = addBox(cad);
    require(cad.remove(lastPartId), QStringLiteral("Could not remove the last allocated CAD part"));
    QString error;
    checked(validateRigidModel(cad, hingeModel, error), QStringLiteral("Validate hinge model"), error);
    const auto hingeInput = prepare(cad, hingeModel);
    checkMass(hingeInput.bodies[0].properties, 0.96); // 2500 kg/m^3 * 0.12 m * 0.08 m * 0.04 m
    auto explicitMass = hingeModel.bodies[0];
    explicitMass.densityKgM3 = 0.0;
    BodyProperties properties;
    checked(rigidBodyProperties(cad, explicitMass, properties, error), QStringLiteral("Evaluate explicit box mass"), error);
    checkMass(properties, 2.5);
    const auto hingeResult = run(hingeInput);
    checkHinge(hingeResult.frames[hingeResult.frames.size() / 2]);
    checkHinge(hingeResult.frames.back()); // Over 180 degrees: the joint coordinate must not wrap.

    auto slider = hingeModel;
    slider.joints[0] = {82, QStringLiteral("斜向滑块"), JointKind::Prismatic, slider.bodies[0].id, 0,
                        {-11, 27, 8}, {0, 3, 4}};
    slider.drives[0] = {118, QStringLiteral("行程驱动"), 82, DriveKind::Displacement, kSliderTravel};
    slider.scenarios[0] = {160, QStringLiteral("直线行程"), 0.373, 0.02, {0.3, 0.2, -9.8}};
    const auto sliderInput = prepare(cad, slider);
    const auto slideResult = run(sliderInput);
    const auto& end = slideResult.frames.back();
    expectNear(end.seconds - slideResult.frames[slideResult.frames.size() - 2].seconds, 0.013, 1e-12,
         QStringLiteral("Shortened final slider step (s)"));
    for (std::size_t i = 1; i < slideResult.frames.size(); ++i) {
        const auto& frame = slideResult.frames[i];
        expectNear(frame.joints[0].position, frame.seconds * kSliderRate, 0.02, QStringLiteral("Linear displacement ramp (mm)"));
        expectNear(frame.joints[0].speed, kSliderRate, 0.1, QStringLiteral("Slider joint speed (mm/s)"));
        expectNear(frame.bodies[0].positionMm, sum(kCenterMm, scale(kSliderAxis, frame.seconds * kSliderRate)), 0.02,
             QStringLiteral("Slider world COM (mm)"));
        expectNear(frame.bodies[0].velocityMmS, scale(kSliderAxis, kSliderRate), 0.1, QStringLiteral("Slider world velocity (mm/s)"));
    }
    expectNear(end.joints[0].position, kSliderTravel, 0.02, QStringLiteral("Terminal slider displacement (mm)"));

    auto fixed = hingeModel;
    fixed.joints[0].kind = JointKind::Fixed;
    fixed.drives.clear();
    fixed.scenarios[0] = {171, QStringLiteral("固定约束"), 0.207, 0.02, {1.0, -2.0, -9.8}};
    const auto fixedResult = run(prepare(cad, fixed));
    for (const auto& frame : fixedResult.frames) {
        const auto& b = frame.bodies[0];
        expectNear(b.positionMm, kCenterMm, 1e-3, QStringLiteral("Fixed body COM under gravity"));
        expectNear(std::abs(b.rotation[0]), 1.0, 1e-8, QStringLiteral("Fixed body orientation"));
        expectNear(b.velocityMmS, {}, 1e-3, QStringLiteral("Fixed body linear velocity"));
        expectNear(b.angularVelocityDegS, {}, 1e-3, QStringLiteral("Fixed body angular velocity"));
        expectNear(frame.joints[0].position, 0.0, 1e-8, QStringLiteral("Fixed joint coordinate"));
        expectNear(frame.joints[0].speed, 0.0, 1e-8, QStringLiteral("Fixed joint rate"));
    }

    checkFailures(cad, hingeModel, hingeInput, slideResult, temporary);
    checkProject(cad, hingeModel, lastPartId, temporary);
    checkCurvePersistence(cad, hingeModel, temporary);
    checkCsv(hingeInput, hingeResult, temporary.filePath(QStringLiteral("角度.csv")), true);
    checkCsv(sliderInput, slideResult, temporary.filePath(QStringLiteral("行程.csv")), false);
    if (!example.isEmpty()) writeExample(example, cad, hingeModel);
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 1 && (args.size() != 3 || args[1] != QStringLiteral("--write-example") || args[2].isEmpty())) {
        qCritical().noquote() << "Usage: MotionOccRigidSmoke [--write-example <new path.mxmotion>]";
        return 2;
    }
    try {
        QTemporaryDir temporary;
        require(temporary.isValid(), QStringLiteral("Could not create test directory: %1").arg(temporary.errorString()));
        smoke(temporary, args.value(2));
        qInfo().noquote() << "MotionOcc rigid smoke passed: inertia, hinge, slider, fixed, cancellation, project and CSV";
        return 0;
    } catch (const Standard_Failure& failure) {
        qCritical() << "MotionOcc rigid smoke OCCT exception:" << failure.GetMessageString();
    } catch (const std::exception& failure) {
        qCritical().noquote() << "MotionOcc rigid smoke failed:" << QString::fromUtf8(failure.what());
    }
    return 1;
}
