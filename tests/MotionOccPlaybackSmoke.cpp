#include "RigidPlayback.h"
#include "NativeMotionPresentation.h"

#include <QCoreApplication>
#include <QTextStream>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
void expectNear(const QVector3D& actual, const QVector3D& expected, const char* message)
{
    require((actual - expected).length() < 1e-3f, message);
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    using namespace MotionOcc;
    try {
        RigidInput input;
        input.scenario.id = 5;
        PreparedBody first;
        first.body.id = 31;
        first.body.partId = 101;
        first.properties.centerMm = {1000, 2000, 3000};
        PreparedBody second;
        second.body.id = 7;
        second.body.partId = 42;
        second.properties.centerMm = {-20, 40, 10};
        input.bodies = {first, second};
        const double half = std::sqrt(0.5);
        BodyState a0, a1, b;
        a0.bodyId = 31; a0.positionMm = {1000, 2000, 3000};
        a1.bodyId = 31; a1.positionMm = {1010, 2020, 3000}; a1.rotation = {half, 0, 0, half};
        b.bodyId = 7; b.positionMm = {-20, 40, 10};
        // Nonzero initial time, nonuniform output frames, unrelated entity IDs,
        // and deliberately different state ordering in adjacent frames.
        RigidResult result;
        result.scenarioId = 5;
        result.frames = {{2.0, {b, a0}, {}}, {2.1, {a1, b}, {}}, {2.75, {b, a1}, {}}};
        RigidPlaybackSample sample;
        QString error;
        require(validateRigidPlayback(input, result, error), "Valid physical timeline was rejected");
        require(sampleRigidPlayback(input, result, 0.0, sample, error), "Clamp before first frame failed");
        require(sample.seconds == 2.0 && sample.frameIndex == 0, "Must use the first physical time, not zero");
        expectNear(sample.partPoses.at(101).map(QVector3D(1005, 2000, 3000)), {1005, 2000, 3000},
             "Rest CAD transform must be identity at the original global COM");
        require(sampleRigidPlayback(input, result, 2.05, sample, error), "Interpolation failed");
        expectNear(sample.partPoses.at(101).map(QVector3D(1010, 2000, 3000)),
             {float(1005 + 10 * half), float(2010 + 10 * half), 3000},
             "COM lerp and quaternion slerp must rotate global CAD about the initial COM");
        expectNear(sample.partPoses.at(42).map(QVector3D(-20, 40, 10)), {-20, 40, 10},
             "Result state order must not swap the bodies");
        require(sample.frameIndex == 1, "Interpolated physical time must select the upper frame index");
        require(sampleRigidPlayback(input, result, 9.0, sample, error)
                && sample.seconds == 2.75 && sample.frameIndex == 2, "Clamp after last frame failed");
        expectNear(sample.partPoses.at(101).map(QVector3D(1010, 2000, 3000)), {1010, 2030, 3000},
             "Exact last frame must use its full pose");
        const auto valid = sample.partPoses;
        require(!sampleRigidPlayback(input, result, std::numeric_limits<double>::quiet_NaN(), sample, error)
                && !error.isEmpty() && sample.partPoses == valid, "NaN request must fail without replacing the last valid pose");
        result.frames[1].bodies.pop_back();
        require(!sampleRigidPlayback(input, result, 2.05, sample, error), "Missing body must not be indexed out of bounds");
        require(sampleRigidPlayback(input, result, 2.75, sample, error), "Exact frame must not depend on incomplete neighbour");
        result.frames = {result.frames.front()};
        require(sampleRigidPlayback(input, result, 99, sample, error) && sample.seconds == 2,
                "Single-frame results must remain displayable");
        result.scenarioId = 8;
        require(!validateRigidPlayback(input, result, error)
                && !sampleRigidPlayback(input, result, 2, sample, error), "Foreign scenario results must not animate this model");
        result.scenarioId = 5;
        result.frames = {{2, {a0, b}, {}}, {1, {a1, b}, {}}, {3, {a1, b}, {}}};
        require(!validateRigidPlayback(input, result, error), "Unsorted interior times must be rejected before binary search");
        result.frames[1].seconds = 2;
        require(!validateRigidPlayback(input, result, error), "Duplicate frame times must be rejected");
        a0.positionMm[0] = 1e300;
        result.frames = {{2, {a0, b}, {}}};
        require(!sampleRigidPlayback(input, result, 2, sample, error), "Finite doubles overflowing render floats must be rejected");
        // Native IDs may be zero and one body can own several placed CAD
        // instances. All instances must receive the same physical transform.
        input.scenario.id = 0;
        input.nativeInput = std::make_shared<NativeMotionInput>();
        first.body.id = 0; input.bodies = {first, second};
        input.nativeInput->document.bodyPartBindings.insert(0, {101, 109});
        input.nativeInput->document.bodyPartBindings.insert(7, {42});
        a0.bodyId = a1.bodyId = 0; a0.positionMm = {1000, 2000, 3000};
        result.scenarioId = 0;
        result.frames = {{0, {a0, b}, {}}, {2, {b, a1}, {}}};
        require(validateRigidPlayback(input, result, error), "Native scenario zero was rejected");
        require(sampleRigidPlayback(input, result, 1, sample, error), "Native multi-part interpolation failed");
        require(sample.partPoses.size() == 3 && sample.partPoses.at(101) == sample.partPoses.at(109),
                "A multi-part body lost an instance or received different transforms");
        const auto edge=[](int part){return QJsonArray{QJsonObject{{"partId",part},{"index",1}}};};
        QJsonArray curveJoints{
            QJsonObject{{"id",1},{"jointType","PointOnCurve"},{"baseBodyId",0},{"occCurveEdges",edge(700)}},
            QJsonObject{{"id",2},{"jointType","CurveOnCurve"},{"actionBodyId",0},{"baseBodyId",7},
                {"occCurveIEdges",edge(701)},{"occCurveJEdges",edge(702)}},
            QJsonObject{{"id",3},{"jointType","PointOnCurve"},{"baseBodyId",-1},{"occCurveEdges",edge(703)}},
            QJsonObject{{"id",4},{"jointType","PointOnCurve"},{"baseBodyId",0},{"occCurveEdges",edge(42)}},
            QJsonObject{{"id",5},{"jointType","PointOnCurve"},{"baseBodyId",0},{"occCurveEdges",edge(700)}}};
        input.nativeInput->document.root={{"motionData",QJsonObject{{"joints",curveJoints}}}};
        require(validateRigidPlayback(input,result,error) && sampleRigidPlayback(input,result,1,sample,error),
                "Unique standalone curve references were rejected");
        require(sample.partPoses.size()==7 && sample.partPoses.at(700)==sample.partPoses.at(101)
                && sample.partPoses.at(701)==sample.partPoses.at(101) && sample.partPoses.at(702)==sample.partPoses.at(42),
                "Standalone point/curve-pair references did not follow their base/action body poses");
        expectNear(sample.partPoses.at(700).map(QVector3D(1010,2000,3000)),
             {float(1005+10*half),float(2010+10*half),3000},
             "A global construction curve did not use the solver's COM-relative rotating frame");
        expectNear(sample.partPoses.at(42).map(QVector3D(-20,40,10)),{-20,40,10},
             "A curve reference overwrote a CAD part's existing physical body ownership");
        expectNear(sample.partPoses.at(703).map(QVector3D(8,9,10)),{8,9,10},
             "A ground-referenced construction curve must remain in world coordinates");
        const auto acceptedCurvePoses=sample.partPoses;
        curveJoints.append(QJsonObject{{"id",6},{"jointType","PointOnCurve"},{"baseBodyId",7},{"occCurveEdges",edge(700)}});
        input.nativeInput->document.root={{"motionData",QJsonObject{{"joints",curveJoints}}}};
        require(!validateRigidPlayback(input,result,error) && error.contains("唯一动画姿态")
                && !sampleRigidPlayback(input,result,1,sample,error) && sample.partPoses==acceptedCurvePoses,
                "An ambiguously shared standalone curve silently chose one body's animation pose");
        curveJoints.removeLast();
        curveJoints.append(QJsonObject{{"id",6},{"jointType","PointOnCurve"},{"baseBodyId",-1},{"occCurveEdges",edge(700)}});
        input.nativeInput->document.root={{"motionData",QJsonObject{{"joints",curveJoints}}}};
        require(!validateRigidPlayback(input,result,error),"A shared ground/moving curve reference was treated as unambiguous");
        PreparedBody virtualBody; virtualBody.body.id = 55; input.bodies.push_back(virtualBody);
        BodyState virtualState; virtualState.bodyId = 55;
        for (auto& frame : result.frames) frame.bodies.push_back(virtualState);
        input.nativeInput->document.root = {{"motionData", QJsonObject{{"bodies", QJsonArray{
            QJsonObject{{"id", 55}, {"geoEntityIds", QJsonArray{}}}}}}}};
        require(sampleRigidPlayback(input, result, 1, sample, error) && sample.partPoses.size() == 3,
                "A virtual body with no CAD must not prevent other bodies from animating");
        auto virtualData=input.nativeInput->document.root.value("motionData").toObject();
        virtualData.insert("joints",QJsonArray{QJsonObject{{"id",8},{"jointType","PointOnCurve"},{"baseBodyId",55},{"occCurveEdges",edge(704)}}});
        input.nativeInput->document.root.insert("motionData",virtualData);
        for(auto& frame:result.frames)frame.bodies.back().positionMm={9,8,7};
        require(sampleRigidPlayback(input,result,1,sample,error) && sample.partPoses.size()==4,
                "A standalone curve could not follow a virtual reference body");
        expectNear(sample.partPoses.at(704).map(QVector3D(1,2,3)),{10,10,10},
             "Virtual reference body pose was discarded before updating its independent curve");
        input.nativeInput->document.root = {{"motionData", QJsonObject{{"bodies", QJsonArray{
            QJsonObject{{"id", 55}, {"geoEntityIds", QJsonArray{901}}}}}}}};
        require(!sampleRigidPlayback(input, result, 1, sample, error),
                "A real geometry reference with a missing binding was mistaken for a virtual body");
        input.bodies.pop_back(); for (auto& frame : result.frames) frame.bodies.pop_back();
        input.nativeInput->document.root = {};
        RigidFlexResult flex;
        flex.flexId = 23; flex.bodyId = 0;
        flex.referenceNodes = {0, 0, 0, 10, 0, 0, 0, 10, 0};
        flex.surfaceTriangles = {0, 1, 2};
        flex.nodePositions = {flex.referenceNodes, {1, 0, 0, 11, 0, 2, 1, 10, 4}};
        result.flexBodies = {flex};
        require(validateRigidPlayback(input, result, error), "Complete modal results were rejected");
        require(sampleRigidPlayback(input, result, 1, sample, error), "Modal node interpolation failed");
        require(sample.partPoses.count(101) == 0 && sample.partPoses.count(109) == 0
                && sample.flexMeshes.count(101) == 1 && sample.flexHiddenParts.count(109),
                "Modal mesh must replace all bound CAD without applying a second rigid transform");
        const auto& mesh = sample.flexMeshes.at(101);
        require(mesh.nodes.size() == 3 && mesh.surface.size() == 1 && mesh.nodes[2][0] == .5 && mesh.nodes[2][2] == 2,
                "Modal surface must interpolate actual engine node positions");
        result.flexBodies.front().surfaceTriangles[2] = 9;
        require(!validateRigidPlayback(input, result, error), "Invalid modal surface topology was accepted");
        result.flexBodies.front() = flex;
        result.flexBodies.front().nodePositions[1][0] = std::numeric_limits<double>::quiet_NaN();
        require(!validateRigidPlayback(input, result, error), "Nonfinite modal node result was accepted");
        RigidModel presentation; presentation.nativeDocument = std::make_shared<NativeMotionDocument>();
        RigidScenario presentationScenario; presentationScenario.id = 0;
        RigidCurveRequest forceCurve; forceCurve.id = 4; forceCurve.sourceId = 9;
        forceCurve.metric = RigidCurveMetric::NativeChannel; forceCurve.sourceType = "softStops"; forceCurve.variable = "Force";
        RigidCurveRequest bodyCurve = forceCurve; bodyCurve.id = 7; bodyCurve.sourceType = "bodies"; bodyCurve.variable = "Position";
        presentationScenario.curveRequests = {forceCurve, bodyCurve}; presentation.scenarios = {presentationScenario};
        const QJsonObject originalBodyCurve{{"requestId", 7}, {"sourceType", "Body"}, {"sourceId", 9}, {"extra", "preserved"}};
        presentation.nativeDocument->root = {{"motionData", QJsonObject{{"scenarios", QJsonArray{QJsonObject{
            {"id", 0}, {"curveRequests", QJsonArray{QJsonObject{{"requestId", 4}, {"sourceType", "SoftStop"}, {"sourceId", 9}}, originalBodyCurve}}}}}}}};
        const auto originalPresentation = presentation;
        removeNativeEntityCurves(presentation, "softStops", 9);
        require(presentation.scenarios.front().curveRequests.size() == 1
                && presentation.scenarios.front().curveRequests.front().sourceType == "bodies"
                && presentation.nativeDocument->entity("scenarios", 0).value("curveRequests").toArray() == QJsonArray{originalBodyCurve},
                "Deleting a source must remove only its display requests, including original enum aliases");
        require(originalPresentation.scenarios.front().curveRequests.size() == 2
                && originalPresentation.nativeDocument->entity("scenarios", 0).value("curveRequests").toArray().size() == 2,
                "Preparing deletion mutated the original model before physical dependency validation");
        QTextStream(stdout) << "Playback regression passed: physical time, ID mapping, interpolation, independent curve frames, invalid data.\n";
        return 0;
    } catch (const std::exception& exception) {
        QTextStream(stderr) << exception.what() << '\n';
        return 1;
    }
}
