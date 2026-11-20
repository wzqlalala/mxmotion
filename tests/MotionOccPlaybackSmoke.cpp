#include "RigidPlayback.h"

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
void near(const QVector3D& actual, const QVector3D& expected, const char* message)
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
        near(sample.partPoses.at(101).map(QVector3D(1005, 2000, 3000)), {1005, 2000, 3000},
             "Rest CAD transform must be identity at the original global COM");
        require(sampleRigidPlayback(input, result, 2.05, sample, error), "Interpolation failed");
        near(sample.partPoses.at(101).map(QVector3D(1010, 2000, 3000)),
             {float(1005 + 10 * half), float(2010 + 10 * half), 3000},
             "COM lerp and quaternion slerp must rotate global CAD about the initial COM");
        near(sample.partPoses.at(42).map(QVector3D(-20, 40, 10)), {-20, 40, 10},
             "Result state order must not swap the bodies");
        require(sample.frameIndex == 1, "Interpolated physical time must select the upper frame index");
        require(sampleRigidPlayback(input, result, 9.0, sample, error)
                && sample.seconds == 2.75 && sample.frameIndex == 2, "Clamp after last frame failed");
        near(sample.partPoses.at(101).map(QVector3D(1010, 2000, 3000)), {1010, 2030, 3000},
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
        QTextStream(stdout) << "Playback regression passed: physical time, ID mapping, interpolation, invalid data.\n";
        return 0;
    } catch (const std::exception& exception) {
        QTextStream(stderr) << exception.what() << '\n';
        return 1;
    }
}
