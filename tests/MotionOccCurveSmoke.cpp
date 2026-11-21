#include "RigidResultSeries.h"

#include <QCoreApplication>
#include <QDebug>
#include <QQuaternion>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace {
using namespace MotionOcc;
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void near(double actual, double expected, const char* message)
{
    require(std::isfinite(actual) && std::abs(actual - expected) < 2e-4, message);
}
RigidCurveRequest request(RigidCurveMetric metric, int id, int component = 0)
{
    RigidCurveRequest curve;
    curve.id = 1;
    curve.metric = metric;
    curve.sourceId = id;
    curve.component = component;
    return curve;
}
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    using namespace MotionOcc;
    try {
        RigidInput input;
        input.scenario.id = 7;
        PreparedBody main;
        main.body.id = 42;
        main.body.name = QStringLiteral("偏置体");
        input.bodies.push_back(main);
        PreparedBody other;
        other.body.id = 3;
        input.bodies.push_back(other);
        RigidJoint hinge;
        hinge.id = 9;
        hinge.kind = JointKind::Revolute;
        input.joints.push_back(hinge);
        RigidJoint slider;
        slider.id = 20;
        slider.kind = JointKind::Prismatic;
        input.joints.push_back(slider);
        RigidResult result;
        result.scenarioId = 7;
        const double times[] = {0.25, 0.35, 0.8, 1.3};
        const double angles[] = {170.0, 190.0, 270.0, 370.0};
        for (int i = 0; i < 4; ++i) {
            RigidFrame frame;
            frame.seconds = times[i];
            BodyState state;
            state.bodyId = 42;
            state.positionMm = {100.0 + i * 3, 200.0 - i * 4, 300.0};
            state.velocityMmS = {3.0, 4.0, 0.0};
            state.angularVelocityDegS = {0.0, 0.0, 90.0};
            const auto q = QQuaternion::fromAxisAndAngle(0, 0, 1, float(angles[i]));
            state.rotation = {q.scalar(), q.x(), q.y(), q.z()};
            BodyState another;
            another.bodyId = 3;
            another.positionMm = {-999, -999, -999};
            frame.bodies = i % 2 ? std::vector<BodyState>{state, another} : std::vector<BodyState>{another, state};
            frame.joints = {{20, -3.0 + i, -2.0}, {9, 180.0 + 30.0 * i, 60.0}};
            result.frames.push_back(frame);
        }
        auto series = rigidCurveSeries(input, result, request(RigidCurveMetric::BodyPosition, 42));
        require(series.isValid() && series.times.size() == 4, "absolute position series missing");
        near(series.values.front(), 100, "position was silently changed to displacement");
        near(series.values.back(), 109, "body state was looked up by array position instead of ID");
        require(series.times == QVector<double>({0.25, 0.35, 0.8, 1.3}), "physical timestamps were replaced with frame indices");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::BodyDisplacement, 42, 3));
        near(series.values.front(), 0, "initial displacement is not zero");
        near(series.values.back(), 15, "displacement norm is not relative to initial COM");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::BodyVelocity, 42, 3));
        near(series.values.back(), 5, "velocity magnitude is wrong");
        require(series.unit == QStringLiteral("mm/s"), "linear velocity unit is wrong");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::BodyAngularVelocity, 42, 2));
        near(series.values.back(), 90, "angular velocity was converted twice");
        require(series.unit == QStringLiteral("°/s"), "angular velocity unit is wrong");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::BodyRotation, 42, 2));
        for (int i = 0; i < 4; ++i) near(series.values[i], angles[i], "Euler curve was not unwrapped across 180 or 360 degrees");
        require(!rigidCurveSeries(input, result, request(RigidCurveMetric::BodyRotation, 42, 3)).isValid(), "Euler magnitude was accepted as physical angle");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::JointCoordinate, 9));
        near(series.values.back(), 270, "continuous joint coordinate was confused with global node position");
        require(series.quantity.contains(QStringLiteral("相对转角")) && series.unit == QStringLiteral("°"), "revolute coordinate mislabeled");
        series = rigidCurveSeries(input, result, request(RigidCurveMetric::JointRate, 20));
        near(series.values.back(), -2, "slider rate wrong");
        require(series.unit == QStringLiteral("mm/s"), "slider units confused with revolute units");
        auto invalid = result;
        invalid.frames[2].bodies.erase(invalid.frames[2].bodies.begin() + 1);
        require(!rigidCurveSeries(input, invalid, request(RigidCurveMetric::BodyPosition, 42)).isValid(), "missing physical sample was silently bridged");
        invalid = result;
        invalid.frames[1].bodies.push_back(invalid.frames[1].bodies.front());
        require(!rigidCurveSeries(input, invalid, request(RigidCurveMetric::BodyPosition, 42)).isValid(), "duplicate body ID was silently accepted");
        invalid = result;
        invalid.frames[2].seconds = invalid.frames[1].seconds;
        require(!rigidCurveSeries(input, invalid, request(RigidCurveMetric::BodyPosition, 42)).isValid(), "duplicate time was silently accepted");
        invalid = result;
        invalid.frames[1].bodies.front().velocityMmS[0] = std::numeric_limits<double>::quiet_NaN();
        require(!rigidCurveSeries(input, invalid, request(RigidCurveMetric::BodyVelocity, 42)).isValid(), "non-finite sample was silently skipped");
        invalid = result;
        invalid.scenarioId = 99;
        require(!rigidCurveSeries(input, invalid, request(RigidCurveMetric::BodyPosition, 42)).isValid(), "another scenario's result was shown");
        {
            RigidInput nativeInput;
            nativeInput.scenario.id = 0; // Original documents use nonnegative IDs.
            RigidResult native;
            native.scenarioId = 0;
            native.frames = result.frames;
            native.channels = {
                {"sensors", 9, "SensorValue", "N", 3, {-99,-99,-99, -99,-99,-99, -99,-99,-99, -99,-99,-99}},
                {"drives", 0, "ReactionForce", "N", 6, {1,2,3,4,5,6, 7,8,9,10,11,12, 13,14,15,16,17,18, 19,20,21,22,23,24}},
                {"sensors", 0, "SensorValue", "N", 3, {3,4,0, 0,0,7, 1,2,2, 0,12,5}},
                {"controllers", 0, "PidError", {}, 1, {2,1,0,-1}}
            };
            auto query = request(RigidCurveMetric::NativeChannel, 0, 5);
            query.sourceType = QStringLiteral("Drive"); // Legacy enum spelling.
            query.variable = QStringLiteral("ReactionForce");
            series = rigidCurveSeries(nativeInput, native, query);
            require(series.isValid() && series.unit == QStringLiteral("N"), "six-component native drive result missing");
            near(series.values.front(), 6, "native component six did not read its frame-major column");
            near(series.values.back(), 24, "native object ID zero was confused with an array index");
            require(series.times == QVector<double>({0.25,0.35,0.8,1.3}), "native result timestamps changed");
            query.sourceType = QStringLiteral("sensors");
            query.variable = QStringLiteral("SensorValue");
            query.component = -1;
            series = rigidCurveSeries(nativeInput, native, query);
            require(series.isValid(), "three-component native magnitude rejected");
            near(series.values.front(), 5, "native magnitude did not use selected stable source ID");
            near(series.values.back(), 13, "native magnitude was not computed for each physical frame");
            query.sourceType = QStringLiteral("controllers");
            query.variable = QStringLiteral("PidError");
            query.component = 0;
            series = rigidCurveSeries(nativeInput, native, query);
            require(series.isValid() && series.unit.isEmpty(), "dimensionless native scalar was rejected or relabeled");
            near(series.values.back(), -1, "signed controller value was changed");
            auto broken = native;
            broken.channels.push_back(broken.channels.back());
            require(!rigidCurveSeries(nativeInput, broken, query).isValid(), "duplicate native result key was accepted");
            broken = native;
            broken.channels.back().values.pop_back();
            require(!rigidCurveSeries(nativeInput, broken, query).isValid(), "missing native sample was bridged");
            broken = native;
            broken.channels.back().values[1] = std::numeric_limits<double>::infinity();
            require(!rigidCurveSeries(nativeInput, broken, query).isValid(), "nonfinite native sample was accepted");
            broken = native;
            broken.frames[1].seconds = broken.frames[0].seconds;
            require(!rigidCurveSeries(nativeInput, broken, query).isValid(), "duplicate native sample time was accepted");
            query.sourceId = -1;
            require(!rigidCurveSeries(nativeInput, native, query).isValid(), "negative native entity ID was accepted");
            query.sourceId = 0;
            query.component = 1;
            require(!rigidCurveSeries(nativeInput, native, query).isValid(), "scalar channel accepted a second component");
            query.sourceType = QStringLiteral("drives");
            query.variable = QStringLiteral("ReactionForce");
            query.component = -1;
            require(!rigidCurveSeries(nativeInput, native, query).isValid(), "mixed six-component result accepted an undefined magnitude");
            ResultChannel modal{"autoFlexBodies", 0, "ModalCoordinates", "mm", 18, std::vector<double>(72,0)};
            modal.values[17] = 9.0;
            native.channels.push_back(modal);
            query.sourceType = modal.sourceType;
            query.variable = modal.variable;
            query.component = 17;
            series = rigidCurveSeries(nativeInput, native, query);
            require(series.isValid(), "modal result was incorrectly limited to six components");
            near(series.values.front(), 9, "modal component index was truncated");
            native.channels.push_back({"bodies",0,"Rotation","°",3,{0,0,170, 0,0,-170, 0,0,-90, 0,0,10}});
            query.sourceType = QStringLiteral("bodies");
            query.variable = QStringLiteral("Rotation");
            query.component = 2;
            series = rigidCurveSeries(nativeInput, native, query);
            require(series.isValid(), "native Euler curve missing");
            for (int i = 0; i < 4; ++i) near(series.values[i], angles[i], "native Euler curve did not preserve original continuous-angle display");
            query.component = -1;
            require(!rigidCurveSeries(nativeInput, native, query).isValid(), "native Euler magnitude was accepted as a physical angle");
        }
        qInfo() << "Curve queries: physical timestamps, stable IDs, legacy and native channel components, units and invalid-data checks passed";
        return 0;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return 1;
    }
}
