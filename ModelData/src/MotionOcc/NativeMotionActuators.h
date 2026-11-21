#pragma once

#include <QJsonObject>
#include <memory>
#include <vector>

namespace motion {
class MotionFunction;
class MotionLinkMotor;
}

namespace MotionOcc {
class NativeMotionSystem;
struct NativeMotionInput;

// Public engineering units: N, N mm, mm/s or deg/s, W. The wrapped engine
// function converts effort to kg-mm-s units only at its final boundary.
struct NativeEffortSample {
    double time=0, requested=0, applied=0, speed=0, power=0;
    bool limited=false;
};
struct NativeEffortRuntime {
    int driveId=-1;
    bool angular=false;
    QJsonObject definition;
    std::weak_ptr<motion::MotionLinkMotor> motor;
    std::shared_ptr<motion::MotionFunction> command;
    std::vector<NativeEffortSample> history;
};

void createNativeEffortMotors(NativeMotionSystem& system,const NativeMotionInput& input);
double nativeEffortSpeed(const NativeEffortRuntime& runtime);
double limitNativeEffort(const NativeEffortRuntime& runtime,double requested);
void recordNativeEffortMotors(NativeMotionSystem& system);
}
