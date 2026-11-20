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
        qInfo() << "Curve queries: physical timestamps, object IDs, position/displacement, units, rotation unwrap and invalid-data checks passed";
        return 0;
    } catch (const std::exception& e) {
        qCritical() << e.what();
        return 1;
    }
}
