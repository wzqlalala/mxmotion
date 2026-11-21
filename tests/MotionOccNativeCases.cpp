#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeTopologyBinding.h"
#ifdef MX_NATIVE_SOLVE_CASES
#include "NativeMotionSolver.h"
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Builder.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Quaternion.hxx>
#include <gp_Trsf.hxx>
#endif

#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <GProp_PrincipalProps.hxx>
#include <gp_Mat.hxx>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>

namespace {
using namespace MotionOcc;

QJsonArray vector(double x, double y, double z) { return {x, y, z}; }

bool saveReport(const QString& path, const QJsonArray& cases, QString& error)
{
    QJsonObject report{{"format", "MxMotionNativeCaseReport"}, {"cases", cases}};
    const auto bytes = QJsonDocument(report).toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = file.errorString(); return false;
    }
    return true;
}

QJsonArray geometryReport(const CadDocument& cad)
{
    QJsonArray parts;
    for (const auto& part : cad.parts()) {
        QJsonObject item{{"id", part.id}, {"name", part.name}, {"sourceName", part.sourceName}, {"instancePath", part.instancePath}};
        GProp_GProps props;
        BRepGProp::VolumeProperties(part.shape, props);
        const auto point = props.CentreOfMass();
        item["volumeMm3"] = props.Mass();
        item["centerMm"] = vector(point.X(), point.Y(), point.Z());
        const auto inertia = props.MatrixOfInertia();
        item["inertiaPerVolume"] = vector(inertia.Value(1, 1) / props.Mass(), inertia.Value(2, 2) / props.Mass(), inertia.Value(3, 3) / props.Mass());
        item["inertiaTensorPerVolume"] = QJsonArray{inertia.Value(1, 1) / props.Mass(), inertia.Value(2, 2) / props.Mass(),
            inertia.Value(3, 3) / props.Mass(), inertia.Value(1, 2) / props.Mass(), inertia.Value(1, 3) / props.Mass(),
            inertia.Value(2, 3) / props.Mass()};
        double first = 0, second = 0, third = 0;
        props.PrincipalProperties().Moments(first, second, third);
        item["principalInertiaPerVolume"] = vector(first / props.Mass(), second / props.Mass(), third / props.Mass());
        parts.append(item);
    }
    return parts;
}

QJsonArray bindingReport(const NativeMotionDocument& doc, const QVector<NativeMotionBindingIssue>& issues,
                         const QJsonArray& geometry, const QMap<int, QString>& evidence)
{
    QJsonArray bodies;
    for (const auto& value : doc.collection("bodies")) {
        const auto body = value.toObject();
        const int id = body.value("id").toInt();
        QJsonArray bound;
        for (int part : doc.bodyPartBindings.value(id)) bound.append(part);
        QJsonObject item{{"id", id}, {"name", body.value("name")}, {"sourceGeometryIds", body.value("geoEntityIds")},
            {"sourceGeometryNames", body.value("geoEntityNames")}, {"sourceCenterOfMass", body.value("centerOfMass")},
            {"sourceMass", body.value("mass")}, {"sourceInertia", body.value("inertia")}, {"boundParts", bound}};
        if (evidence.contains(id)) item["bindingEvidence"] = evidence.value(id);
        for (const auto& issue : issues) if (issue.bodyId == id || issue.bodyId < 0) {
            item["issue"] = issue.message;
            QJsonArray candidates;
            for (int part : issue.candidatePartIds) candidates.append(part);
            item["candidateParts"] = candidates;
            const auto sourceCenter = body.value("centerOfMass").toObject();
            std::vector<std::pair<double, QJsonObject>> nearest;
            for (const auto& value : geometry) {
                const auto part = value.toObject();
                const auto center = part.value("centerMm").toArray();
                if (center.size() != 3) continue;
                const double dx = center[0].toDouble() - sourceCenter.value("x").toDouble();
                const double dy = center[1].toDouble() - sourceCenter.value("y").toDouble();
                const double dz = center[2].toDouble() - sourceCenter.value("z").toDouble();
                const double distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                nearest.emplace_back(distance, QJsonObject{{"partId", part.value("id")}, {"name", part.value("name")},
                    {"centerDeltaMm", vector(dx, dy, dz)}, {"centerDistanceMm", distance}});
            }
            std::sort(nearest.begin(), nearest.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            QJsonArray closest;
            for (std::size_t i = 0; i < std::min<std::size_t>(3, nearest.size()); ++i) closest.append(nearest[i].second);
            item["nearestPartCenters"] = closest;
        }
        bodies.append(item);
    }
    return bodies;
}

#ifdef MX_NATIVE_SOLVE_CASES
using Vec = std::array<double, 3>;
Vec sub(const Vec& a, const Vec& b) { return {a[0]-b[0], a[1]-b[1], a[2]-b[2]}; }
Vec add(const Vec& a, const Vec& b) { return {a[0]+b[0], a[1]+b[1], a[2]+b[2]}; }
double dot(const Vec& a, const Vec& b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
double norm(const Vec& a) { return std::sqrt(dot(a,a)); }
Vec scaled(const Vec& a, double scale) { return {a[0]*scale,a[1]*scale,a[2]*scale}; }
Vec cross(const Vec& a, const Vec& b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
Vec rotate(const Quaterniond& q, const Vec& v)
{
    const Vec xyz{q[1],q[2],q[3]}, t = scaled(cross(xyz,v),2);
    return add(v,add(scaled(t,q[0]),cross(xyz,t)));
}
Vec fromJson(const QJsonValue& value)
{
    const auto object = value.toObject();
    return {object.value("x").toDouble(),object.value("y").toDouble(),object.value("z").toDouble()};
}

// The solver uses sampled curves/triangles. This check deliberately measures
// accepted poses against the selected, trimmed CAD topology instead. Its 0.03 mm
// limit allows the 0.02 mm geometry discretization plus 0.01 mm numerical margin;
// the looser source-assembly capture tolerances are not result tolerances.
constexpr double exactGeometryToleranceMm = 0.03;

struct ExactJointGeometry {
    int id = -1;
    int actionBodyId = -1;
    int baseBodyId = -1;
    QString type;
    gp_Pnt origin;
    TopoDS_Compound shape;
    QJsonArray selectedTopology;
    std::size_t checkedFrames = 0;
    std::size_t worstFrame = 0;
    double maxDistance = -1;
    double worstTime = 0;
    double initialDistance = 0;
    double lastDistance = 0;
    double maxDistanceBeforeLast = 0;
    std::size_t violatingFrames = 0;
    int firstViolationFrame = -1;
    double firstViolationTime = -1;
    double firstViolationDistance = 0;
    gp_Pnt worstPoint;
    gp_Pnt nearestPoint;
};

bool exactJointGeometry(const CadDocument& cad, const NativeMotionDocument& document,
                        std::vector<ExactJointGeometry>& checks, QString& error)
{
    try {
        BRep_Builder builder;
        for (const auto& value : document.collection("joints")) {
            const auto joint = value.toObject();
            const auto type = joint.value("jointType").toString();
            if (type != "PointOnCurve" && type != "PointOnSurface") continue;
            ExactJointGeometry check;
            check.id = joint.value("id").toInt();
            check.actionBodyId = joint.value("actionBodyId").toInt(-1);
            check.baseBodyId = joint.value("baseBodyId").toInt(-1);
            check.type = type;
            const auto origin = fromJson(joint.value("origin"));
            check.origin = gp_Pnt(origin[0], origin[1], origin[2]);
            builder.MakeCompound(check.shape);
            const bool surface = type == "PointOnSurface";
            const auto kind = surface ? TopAbs_FACE : TopAbs_EDGE;
            const auto field = surface ? "occSurfaceFaces" : "occCurveEdges";
            auto references = joint.value(field).toArray();
            // An empty original surface selection means all trimmed faces of
            // the base body. Never add its solid: an interior point has zero
            // distance to a solid but need not lie on any of its surfaces.
            if (surface && references.isEmpty() && joint.value("surfaceFaceIds").toArray().isEmpty()) {
                for (int partId : document.bodyPartBindings.value(check.baseBodyId)) {
                    const auto* part = cad.part(partId);
                    if (!part) { error = QStringLiteral("Exact geometry check cannot find base CAD part %1").arg(partId); return false; }
                    TopTools_IndexedMapOfShape faces;
                    TopExp::MapShapes(part->shape, TopAbs_FACE, faces);
                    for (int index = 1; index <= faces.Extent(); ++index)
                        references.append(QJsonObject{{"partId", partId}, {"index", index}});
                }
            }
            for (const auto& reference : references) {
                const auto ref = reference.toObject();
                const int partId = ref.value("partId").toInt(-1), index = ref.value("index").toInt(-1);
                const auto* part = cad.part(partId);
                // Original construction curves are global references and may
                // be independent of the base body's physical solid.
                if (!part || (surface && !document.bodyPartBindings.value(check.baseBodyId).contains(partId))) {
                    error = QStringLiteral("Joint %1 exact geometry check has an invalid %2 CAD reference").arg(check.id).arg(field);
                    return false;
                }
                TopTools_IndexedMapOfShape shapes;
                TopExp::MapShapes(part->shape, kind, shapes);
                if (index < 1 || index > shapes.Extent()) {
                    error = QStringLiteral("Joint %1 exact geometry check has invalid %2 index %3").arg(check.id).arg(field).arg(index);
                    return false;
                }
                builder.Add(check.shape, shapes(index));
                check.selectedTopology.append(QJsonObject{{"partId", partId}, {"index", index}});
            }
            if (check.selectedTopology.isEmpty()) {
                error = QStringLiteral("Joint %1 has no real CAD geometry for the exact result check").arg(check.id);
                return false;
            }
            checks.push_back(std::move(check));
        }
        return true;
    } catch (const Standard_Failure& failure) {
        error = QStringLiteral("Cannot prepare exact CAD result check: %1").arg(QString::fromUtf8(failure.GetMessageString()));
        return false;
    }
}

gp_Trsf bodyPose(const BodyState& state, const Vec& initialCenter)
{
    gp_Quaternion rotation(state.rotation[1], state.rotation[2], state.rotation[3], state.rotation[0]);
    rotation.Normalize();
    gp_Trsf pose;
    pose.SetRotation(rotation);
    const auto rotatedCenter = gp_Pnt(initialCenter[0], initialCenter[1], initialCenter[2]).Transformed(pose);
    pose.SetTranslationPart(gp_Vec(rotatedCenter, gp_Pnt(state.positionMm[0], state.positionMm[1], state.positionMm[2])));
    return pose;
}

bool checkExactGeometryFrame(std::vector<ExactJointGeometry>& checks, const std::map<int, const BodyState*>& states,
                             const std::map<int, Vec>& centers, std::size_t frameIndex, double seconds, QString& error)
{
    try {
        for (auto& check : checks) {
            gp_Pnt point = check.origin;
            if (check.actionBodyId >= 0)
                point.Transform(bodyPose(*states.at(check.actionBodyId), centers.at(check.actionBodyId)));
            // Undo the animated base pose to compare in the initial placed CAD
            // world frame, including moving/rotating base bodies.
            if (check.baseBodyId >= 0)
                point.Transform(bodyPose(*states.at(check.baseBodyId), centers.at(check.baseBodyId)).Inverted());
            BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(point).Shape(), check.shape);
            if (!distance.IsDone() || distance.NbSolution() < 1 || !std::isfinite(distance.Value()) || distance.Value() < 0) {
                error = QStringLiteral("Joint %1 exact CAD distance failed at t=%2").arg(check.id).arg(seconds, 0, 'g', 17);
                return false;
            }
            const double value = distance.Value();
            if (check.checkedFrames == 0) check.initialDistance = value;
            else check.maxDistanceBeforeLast = std::max(check.maxDistanceBeforeLast, check.lastDistance);
            check.lastDistance = value;
            ++check.checkedFrames;
            if (value > exactGeometryToleranceMm) {
                if (check.violatingFrames == 0) {
                    check.firstViolationFrame = int(frameIndex);
                    check.firstViolationTime = seconds;
                    check.firstViolationDistance = value;
                }
                ++check.violatingFrames;
            }
            if (value > check.maxDistance) {
                check.maxDistance = value;
                check.worstTime = seconds;
                check.worstFrame = frameIndex;
                check.worstPoint = point;
                check.nearestPoint = distance.PointOnShape2(1);
            }
        }
        return true;
    } catch (const Standard_Failure& failure) {
        error = QStringLiteral("Exact CAD distance failed at t=%1: %2").arg(seconds, 0, 'g', 17).arg(QString::fromUtf8(failure.GetMessageString()));
        return false;
    }
}

bool checkResult(const CadDocument& cad, const NativeMotionInput& input, const RigidResult& result, QJsonObject& report, QString& error)
{
    const auto fail = [&](const QString& message) { error = message; return false; };
    const bool boundary = result.termination == ResultTermination::GeometryBoundary;
    report["termination"] = boundary ? "GeometryBoundary" : "EndTime";
    report["diagnostics"] = QJsonArray::fromStringList(result.diagnostics);
    if (result.scenarioId != input.scenario.id || result.frames.empty() || (!boundary && result.frames.size() < 2))
        return fail("Missing/mismatched result frames");
    report["actualLastTime"] = result.frames.back().seconds;
    const double timeTolerance = std::max(1e-8,input.scenario.step*1e-6);
    if (std::abs(result.frames.front().seconds) > 1e-9 || result.frames.back().seconds > input.scenario.duration + timeTolerance
        || (!boundary && std::abs(result.frames.back().seconds-input.scenario.duration) > timeTolerance))
        return fail("Result does not include the original initial and final times");
    if (boundary && result.diagnostics.isEmpty()) return fail("A geometry-boundary stop did not retain its termination reason");
    std::map<int, Vec> centers;
    std::map<int, bool> fixed;
    double scale = 1;
    for (const auto& body : input.bodies) {
        centers[body.body.id] = body.properties.centerMm;
        fixed[body.body.id] = body.body.fixed;
        scale = std::max(scale,norm(body.properties.centerMm));
    }
    std::vector<ExactJointGeometry> exactChecks;
    if (!exactJointGeometry(cad, input.document, exactChecks, error)) return false;
    double maxClosure = 0, maxFixedMotion = 0, maxDriveRateError = 0, closureTime = 0;
    int closureJointId = -1;
    int checkedDriveRates = 0;
    double previous = -1;
    for (std::size_t frameIndex=0;frameIndex<result.frames.size();++frameIndex) {
        const auto& frame = result.frames[frameIndex];
        if (!std::isfinite(frame.seconds) || frame.seconds <= previous) return fail("Nonfinite/nonincreasing physical frame times");
        previous=frame.seconds;
        std::map<int,const BodyState*> states;
        for (const auto& state : frame.bodies) {
            if (!centers.count(state.bodyId) || !states.emplace(state.bodyId,&state).second) return fail("Unexpected/duplicate body result ID");
            for (const auto& values : {state.positionMm,state.velocityMmS,state.angularVelocityDegS})
                for(double x:values) if(!std::isfinite(x)) return fail("Nonfinite body kinematics");
            double length=0;for(double x:state.rotation){if(!std::isfinite(x))return fail("Nonfinite body rotation");length+=x*x;}
            if(std::abs(length-1)>1e-5)return fail("Body quaternion is not normalized");
            if(fixed[state.bodyId])maxFixedMotion=std::max(maxFixedMotion,norm(sub(state.positionMm,centers[state.bodyId])));
        }
        if(states.size()!=centers.size())return fail("A result frame omits a prepared body");
        // result.frames contains only accepted states, including the final
        // accepted prefix when the next trial step reaches a geometry boundary.
        if (!checkExactGeometryFrame(exactChecks, states, centers, frameIndex, frame.seconds, error)) return false;
        const auto point = [&](int id,const Vec& origin){
            if(id<0)return origin;
            const auto* state=states.at(id);return add(state->positionMm,rotate(state->rotation,sub(origin,centers.at(id))));
        };
        const auto axis = [&](int id,const Vec& value){return id<0?value:rotate(states.at(id)->rotation,value);};
        for(const auto& value:input.document.collection("joints")){
            const auto joint=value.toObject();const auto type=joint.value("jointType").toString();
            const int i=joint.value("actionBodyId").toInt(-1),j=joint.value("baseBodyId").toInt(-1);
            const Vec origin=fromJson(joint.value("origin")),difference=sub(point(i,origin),point(j,origin));
            double residual = 0;
            if(type=="Revolute"||type=="Spherical"||type=="Fixed"||type=="Universal"||type=="Hooke")residual=norm(difference);
            else if(type=="Translational"||type=="Cylindrical"){
                Vec direction=axis(j,fromJson(joint.value("axis")));const double length=norm(direction);
                if(length>0){direction=scaled(direction,1/length);residual=norm(sub(difference,scaled(direction,dot(difference,direction))));}
            }
            if (residual > maxClosure) { maxClosure = residual; closureTime = frame.seconds; closureJointId = joint.value("id").toInt(); }
        }
        if(frameIndex<2)continue;
        for(const auto& value:input.document.collection("drives")){
            const auto drive=value.toObject();
            if(drive.value("controlMode").toString()!="Velocity"||!drive.value("driveTableX").toArray().isEmpty())continue;
            bool constant=false;const double commanded=drive.value("expression").toString().toDouble(&constant);if(!constant)continue;
            bool controlled=false;for(const auto& c:input.document.collection("controllers"))if(c.toObject().value("enabled").toBool(true)&&c.toObject().value("targetDriveId").toInt(-1)==drive.value("id").toInt())controlled=true;
            if(controlled)continue;
            const auto joint=input.document.entity("joints",drive.value("targetJointId").toInt(-1));
            const int i=joint.value("actionBodyId").toInt(-1),j=joint.value("baseBodyId").toInt(-1);
            Vec direction=axis(j,fromJson(joint.value("axis")));const double length=norm(direction);if(length<=0)continue;direction=scaled(direction,1/length);
            const bool rotation=drive.value("motionType").toString()=="Rotation";
            Vec vi{},vj{};
            if(rotation){if(i>=0)vi=states.at(i)->angularVelocityDegS;if(j>=0)vj=states.at(j)->angularVelocityDegS;}
            else{
                const Vec origin=fromJson(joint.value("origin"));
                const auto velocity=[&](int id){if(id<0)return Vec{};const auto* state=states.at(id);return add(state->velocityMmS,cross(scaled(state->angularVelocityDegS,3.14159265358979323846/180),sub(point(id,origin),state->positionMm)));};
                vi=velocity(i);vj=velocity(j);
            }
            const double errorRate=std::abs(dot(sub(vi,vj),direction)-commanded);
            maxDriveRateError=std::max(maxDriveRateError,errorRate);++checkedDriveRates;
            if(errorRate>std::max(0.2,std::abs(commanded)*0.02))return fail(QStringLiteral("Constant drive actual velocity differs from command: %1 vs %2").arg(dot(sub(vi,vj),direction)).arg(commanded));
        }
    }
    for(const auto& channel:result.channels){
        if(channel.components<1||channel.values.size()!=result.frames.size()*std::size_t(channel.components))return fail("Result channel length is inconsistent with frames");
        for(double value:channel.values)if(!std::isfinite(value))return fail("Nonfinite result/sensor/control channel");
    }
    report["maxJointClosureErrorMm"]=maxClosure;report["maxFixedBodyMotionMm"]=maxFixedMotion;
    report["maxJointClosureJointId"] = closureJointId;report["maxJointClosureTime"] = closureTime;
    report["maxConstantDriveRateError"]=maxDriveRateError;report["checkedDriveRates"]=checkedDriveRates;
    report["frames"]=int(result.frames.size());report["resultChannels"]=int(result.channels.size());report["lastTime"]=result.frames.back().seconds;
    QJsonArray exactReports;
    const ExactJointGeometry* worstGeometry = nullptr;
    std::size_t checkedExactFrames = 0;
    for (const auto& check : exactChecks) {
        if (!worstGeometry || check.maxDistance > worstGeometry->maxDistance) worstGeometry = &check;
        checkedExactFrames += check.checkedFrames;
        exactReports.append(QJsonObject{{"jointId", check.id}, {"jointType", check.type}, {"selectedTopology", check.selectedTopology},
            {"checkedFrames", int(check.checkedFrames)}, {"availableAcceptedFrames", int(result.frames.size())},
            {"maxExactGeometryDistanceMm", check.maxDistance}, {"worstTime", check.worstTime}, {"worstFrameIndex", int(check.worstFrame)},
            {"initialDistanceMm", check.initialDistance}, {"lastDistanceMm", check.lastDistance},
            {"maxDistanceBeforeLastMm", check.maxDistanceBeforeLast}, {"violatingFrames", int(check.violatingFrames)},
            {"firstViolationFrameIndex", check.firstViolationFrame}, {"firstViolationTime", check.firstViolationTime},
            {"firstViolationDistanceMm", check.firstViolationDistance},
            {"worstPointInInitialCadWorldMm", vector(check.worstPoint.X(), check.worstPoint.Y(), check.worstPoint.Z())},
            {"nearestPointOnCadMm", vector(check.nearestPoint.X(), check.nearestPoint.Y(), check.nearestPoint.Z())}});
    }
    report["exactGeometryChecks"] = exactReports;
    report["exactGeometryDistanceToleranceMm"] = exactGeometryToleranceMm;
    report["checkedExactGeometryFrames"] = double(checkedExactFrames);
    report["checkedAllExactGeometryFrames"] = true;
    report["maxExactGeometryDistanceMm"] = worstGeometry ? worstGeometry->maxDistance : 0;
    report["maxExactGeometryJointId"] = worstGeometry ? worstGeometry->id : -1;
    report["maxExactGeometryTime"] = worstGeometry ? worstGeometry->worstTime : 0;
    report["maxExactGeometryFrameIndex"] = worstGeometry ? int(worstGeometry->worstFrame) : -1;
    if (worstGeometry && worstGeometry->maxDistance > exactGeometryToleranceMm)
        return fail(QStringLiteral("Joint %1 accepted frame %2 at t=%3 is %4 mm from its actual trimmed CAD %5 (limit %6 mm)")
            .arg(worstGeometry->id).arg(worstGeometry->worstFrame).arg(worstGeometry->worstTime, 0, 'g', 17)
            .arg(worstGeometry->maxDistance, 0, 'g', 17).arg(worstGeometry->type == "PointOnCurve" ? "curve" : "surface")
            .arg(exactGeometryToleranceMm));
    if(maxClosure>std::max(0.05,scale*1e-4))return fail(QStringLiteral("Joint %1 anchor/radial closure residual %2 mm at t=%3 exceeds tolerance").arg(closureJointId).arg(maxClosure).arg(closureTime));
    if(maxFixedMotion>std::max(1e-5,scale*1e-7))return fail("A fixed body changed position");
    return true;
}
#endif

} // namespace

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Original CAD + .motion.json regression runner; source files are never modified.");
    parser.addHelpOption();
    parser.addOption({"cases","Directory containing supplied CAD/JSON cases.","directory"});
    parser.addOption({"report","Write a detailed JSON report after every completed case.","file"});
    parser.addOption({"filter","Only JSON paths containing this substring.","substring"});
    parser.addOption({"cad","Use this explicitly selected CAD file for one unique JSON case (e.g. a newer CAD containing construction geometry).","file"});
    parser.addOption({"bind-only","Import and bind geometry without solving."});
    parser.process(app);
    QTextStream output(stdout),errors(stderr);
    if(!parser.isSet("cases")||!parser.isSet("report")){errors<<"--cases and --report are required\n";return 2;}
    const QDir root(parser.value("cases"));
    QMap<QByteArray,QString> unique;
    QDirIterator scan(root.absolutePath(),{"*.motion.json"},QDir::Files,QDirIterator::Subdirectories);
    while(scan.hasNext()){
        const auto path=scan.next();if(parser.isSet("filter")&&!path.contains(parser.value("filter")))continue;
        QFile file(path);if(!file.open(QIODevice::ReadOnly)){errors<<path<<": "<<file.errorString()<<'\n';return 2;}
        const auto hash=QCryptographicHash::hash(file.readAll(),QCryptographicHash::Sha256);
        if(!unique.contains(hash)||path.size()<unique.value(hash).size())unique[hash]=path;
    }
    QStringList paths=unique.values();std::sort(paths.begin(),paths.end());
    if(paths.isEmpty()){errors<<"No case JSON found\n";return 2;}
    if(parser.isSet("cad") && (paths.size()!=1 || !QFileInfo::exists(parser.value("cad")))) {
        errors<<"--cad requires exactly one unique JSON case and an existing CAD file\n";return 2;
    }
    QJsonArray reports;
    QString currentCad;
    QString currentCadError;
    std::unique_ptr<CadDocument> cad;
    QJsonArray currentGeometry;
    int failed=0,boundaryStopped=0;
    for(const auto& path:paths){
        QElapsedTimer timer;timer.start();
        QJsonObject report{{"json",root.relativeFilePath(path)}};
        QString error;
        NativeMotionDocument document;
        output<<"CASE "<<root.relativeFilePath(path)<<Qt::endl;
        if(!readNativeMotionDocument(path,document,error)){report["status"]="import-failed";report["error"]=error;}
        else{
            QJsonObject counts;const auto data=document.root.value("motionData").toObject();
            for(auto it=data.begin();it!=data.end();++it)if(it.value().isArray())counts[it.key()]=it.value().toArray().size();report["collections"]=counts;
            const QDir directory=QFileInfo(path).absoluteDir();
            const auto candidates=directory.entryList({"*.stp","*.step","*.STP","*.STEP","*.brep","*.iges","*.igs"},QDir::Files,QDir::Name);
            QString selected;
            const QString expected=QFileInfo(path).fileName().chopped(QStringLiteral(".motion.json").size());
            for(const auto& file:candidates)if(QFileInfo(file).completeBaseName()==expected){selected=file;break;}
            if(selected.isEmpty()&&candidates.size()==1)selected=candidates.front();
            if(parser.isSet("cad"))selected=QFileInfo(parser.value("cad")).absoluteFilePath();
            if(selected.isEmpty()){report["status"]="cad-pair-unresolved";report["error"]="No unique same-directory CAD match";report["cadCandidates"]=QJsonArray::fromStringList(candidates);}
            else{
                const auto cadPath=QFileInfo(selected).isAbsolute()?selected:directory.absoluteFilePath(selected);report["cad"]=root.relativeFilePath(cadPath);
                if(parser.isSet("cad"))report["cadPairEvidence"]="Explicit CAD input; source JSON unchanged";
                if(currentCad!=cadPath){
                    cad=std::make_unique<CadDocument>();std::vector<int> imported;
                    output<<"  CAD "<<root.relativeFilePath(cadPath)<<Qt::endl;
                    currentCad = cadPath;
                    if(cad->importFile(cadPath,imported,error)){
                        currentCadError.clear();currentGeometry=geometryReport(*cad);
                        for (const auto& note : cad->importNotes()) output << "  CAD: " << note << Qt::endl;
                    }
                    else{cad.reset();currentCadError=error;currentGeometry={};}
                }
                if(!cad){report["status"]="cad-import-failed";report["error"]=currentCadError;}
                else{
                    report["geometry"]=currentGeometry;
                    report["cadImportNotes"] = QJsonArray::fromStringList(cad->importNotes());
                    QVector<NativeMotionBindingIssue> issues;
                    QMap<int, QString> evidence;
                    const bool bound=autoBindNativeMotionBodies(*cad,document,issues,&evidence);
                    report["bodies"]=bindingReport(document,issues,currentGeometry,evidence);
                    report["status"]=bound?"bound":"binding-unresolved";
                    output<<"  "<<document.bodyPartBindings.size()<<"/"<<document.collection("bodies").size()<<" bodies bound, "<<issues.size()<<" unresolved"<<Qt::endl;
                    if (bound) {
                        QVector<NativeTopologyBindingIssue> topologyIssues;
                        QStringList topologyEvidence;
                        const bool topologyBound = autoBindNativeMotionTopology(*cad, document, topologyIssues, &topologyEvidence);
                        report["topologyBindingEvidence"] = QJsonArray::fromStringList(topologyEvidence);
                        QJsonArray issueRows;
                        for (const auto& issue : topologyIssues) issueRows.append(QJsonObject{{"jointId", issue.jointId},
                            {"field", issue.selectorField}, {"message", issue.message}});
                        report["topologyBindingIssues"] = issueRows;
                        if (!topologyBound) report["status"] = "topology-unresolved";
                    }
#ifdef MX_NATIVE_SOLVE_CASES
                    if(bound&&!parser.isSet("bind-only")){
                        QJsonArray scenarios;bool allPassed=true,anyBoundary=false;
                        for(const auto& value:document.collection("scenarios")){
                            const auto definition=value.toObject();const int id=definition.value("id").toInt();
                            QJsonObject scenario{{"id",id},{"name",definition.value("name")},{"requestedEndTime",definition.value("endTime")},{"requestedStep",definition.value("stepSize")}};
                            NativeMotionInput input;RigidResult result;QElapsedTimer solveTimer;solveTimer.start();
                            if(!prepareNativeMotion(*cad,document,id,input,error)){scenario["status"]="prepare-failed";scenario["error"]=error;allPassed=false;}
                            else{
                                output<<"  SOLVE "<<id<<" to "<<input.scenario.duration<<" s, step "<<input.scenario.step<<Qt::endl;
                                int lastProgress = -1;
                                SolverCallbacks callbacks;
                                callbacks.progress = [&](double fraction) {
                                    const int progress = std::clamp(int(fraction * 10), 0, 10);
                                    if (progress == lastProgress) return;
                                    lastProgress = progress;
                                    output << "    " << progress * 10 << "% (" << solveTimer.elapsed() / 1000.0 << " s wall)" << Qt::endl;
                                };
                                const auto status=solveNativeMotion(input,result,error,callbacks);
                                if(status!=SolveStatus::Completed){scenario["status"]="solve-failed";scenario["error"]=error;allPassed=false;}
                                else if(!checkResult(*cad,input,result,scenario,error)){scenario["status"]="result-check-failed";scenario["error"]=error;allPassed=false;}
                                else if (result.termination == ResultTermination::GeometryBoundary) {
                                    scenario["status"] = "boundary-stopped"; anyBoundary = true;
                                    output << "  Boundary stop: " << result.diagnostics.join("; ") << Qt::endl;
                                } else scenario["status"]="passed";
                            }
                            scenario["wallSeconds"]=solveTimer.elapsed()/1000.0;scenarios.append(scenario);
                            output<<"  "<<scenario.value("status").toString()<<" "<<scenario.value("error").toString()<<Qt::endl;
                        }
                        report["scenarios"]=scenarios;report["status"]=!allPassed?"failed":anyBoundary?"boundary-stopped":"passed";
                    }
#endif
                }
            }
        }
        report["wallSeconds"]=timer.elapsed()/1000.0;
        const QString status=report.value("status").toString();
        if(status=="boundary-stopped")++boundaryStopped;
        else if(status!="bound"&&status!="passed")++failed;
        reports.append(report);
        if(!saveReport(parser.value("report"),reports,error)){errors<<"Cannot save report: "<<error<<'\n';return 2;}
        output<<"  "<<status<<" in "<<timer.elapsed()/1000.0<<" s"<<Qt::endl;
    }
    output<<paths.size()<<" unique JSON cases checked; "<<failed<<" unresolved/failed; "<<boundaryStopped
        <<" geometry-boundary stops (valid prefixes, not full-duration passes). Report: "<<parser.value("report")<<Qt::endl;
    return failed?1:0;
}
