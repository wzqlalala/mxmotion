#pragma once

#include <QJsonObject>
#include <QString>
#include <memory>
#include <vector>

namespace motion {
class MotionFunction;
class MotionLinkMotor;
class MotionHydraulicCylinder;
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

struct NativeHydraulicSample {
    double time = 0;
    double pressureA = 0;
    double pressureB = 0;
    double forceN = 0;
    double strokeMm = 0;
    double velocityMmS = 0;
    double flowALpm = 0;
    double flowBLpm = 0;
    double command = 0;
    double mechPowerW = 0;
    double fluidPowerW = 0;
};
struct NativeHydraulicRuntime {
    int cylinderId = -1;
    QJsonObject definition;
    std::shared_ptr<motion::MotionHydraulicCylinder> cylinder;
    std::vector<NativeHydraulicSample> history;
};

void createNativeEffortMotors(NativeMotionSystem& system,const NativeMotionInput& input);
double nativeEffortSpeed(const NativeEffortRuntime& runtime);
double limitNativeEffort(const NativeEffortRuntime& runtime,double requested);
void recordNativeEffortMotors(NativeMotionSystem& system);

void createNativeHydraulicCylinders(NativeMotionSystem& system, const NativeMotionInput& input);
QString initializeSteadyNativeHydraulics(NativeMotionSystem& system, const QJsonObject& scenario);
void recordNativeHydraulicCylinders(NativeMotionSystem& system);
}
