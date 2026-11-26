#pragma once

#include <QJsonObject>
#include <QString>
#include <map>
#include <memory>
#include <set>
#include <vector>

namespace motion {
class MotionFunction;
class MotionLinkMotor;
class MotionHydraulicCylinder;
class MotionHydraulicMotor;
class MotionHydraulicCircuit;
class MotionElectricMotor;
class MotionElectroMechanicalActuator;
class MotionElectroHydraulicActuator;
class MotionElectricDriveNetwork;
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

struct NativeHydraulicMotorSample {
    double time = 0;
    double pressureA = 0;
    double pressureB = 0;
    double torqueNmm = 0;
    double angleDeg = 0;
    double speedRpm = 0;
    double flowALpm = 0;
    double flowBLpm = 0;
    double command = 0;
    double mechPowerW = 0;
    double fluidPowerW = 0;
};
struct NativeHydraulicMotorRuntime {
    int motorId = -1;
    QJsonObject definition;
    std::shared_ptr<motion::MotionHydraulicMotor> motor;
    std::vector<NativeHydraulicMotorSample> history;
};

struct NativeHydraulicNodeSample {
    double time = 0;
    double pressureMPa = 0;
    double netFlowLpm = 0;
};
struct NativeHydraulicComponentSample {
    double time = 0;
    double flowLpm = 0;
    double flowALpm = 0;
    double flowBLpm = 0;
    double deltaPressureMPa = 0;
    double spoolPosition = 0;
    double command = 0;
    double powerW = 0;
    double gasPressureMPa = 0;
    double gasVolumeL = 0;
    double oilVolumeL = 0;
};
struct NativeHydraulicCircuitRuntime {
    std::shared_ptr<motion::MotionHydraulicCircuit> circuit;
    std::map<int, std::vector<NativeHydraulicNodeSample>> nodeHistories;
    std::map<int, std::vector<NativeHydraulicComponentSample>> componentHistories;
    std::map<int, QString> componentTypes;
    std::set<int> accumulatorIds;
    std::set<int> fourWayIds;
    std::map<int, int> portConnectionBranchIds; // componentId -> internal throttle branch ID (or 0 if directly fused)
};

struct NativeElectricMotorSample {
    double time = 0;
    double currentA = 0;
    double voltageV = 0;
    double backEmfV = 0;
    double torqueNmm = 0;
    double angleDeg = 0;
    double speedRpm = 0;
    double command = 0;
    double elecPowerW = 0;
    double mechPowerW = 0;
    double copperLossW = 0;
};
struct NativeElectricMotorRuntime {
    int motorId = -1;
    QJsonObject definition;
    std::shared_ptr<motion::MotionElectricMotor> motor;
    std::vector<NativeElectricMotorSample> history;
};

struct NativeEhaSample {
    double time = 0;
    double pressureA = 0;
    double pressureB = 0;
    double pumpSpeedRpm = 0;
    double currentA = 0;
    double voltageV = 0;
    double forceN = 0;
    double strokeMm = 0;
    double velocityMmS = 0;
    double command = 0;
    double elecPowerW = 0;
    double mechPowerW = 0;
};
struct NativeEhaRuntime {
    int actuatorId = -1;
    QJsonObject definition;
    std::shared_ptr<motion::MotionElectroHydraulicActuator> actuator;
    std::vector<NativeEhaSample> history;
};

struct NativeEmaSample {
    double time = 0;
    double strokeMm = 0;
    double velocityMmS = 0;
    double motorSpeedRpm = 0;
    double currentA = 0;
    double voltageV = 0;
    double forceN = 0;
    double command = 0;
    double elecPowerW = 0;
    double mechPowerW = 0;
};
struct NativeEmaRuntime {
    int actuatorId = -1;
    QJsonObject definition;
    std::shared_ptr<motion::MotionElectroMechanicalActuator> actuator;
    std::vector<NativeEmaSample> history;
};

struct NativePowerSupplySample {
    double time = 0;
    double busVoltageV = 0;
    double sourceCurrentA = 0;
    double loadCurrentA = 0;
    double chopperCurrentA = 0;
    double powerW = 0;
    double storedEnergyJ = 0;
};
struct NativeInverterSample {
    double time = 0;
    double busVoltageV = 0;
    double outputVoltageV = 0;
    double phaseCurrentA = 0;
    double dcCurrentA = 0;
    double modulationIndex = 0;
    double outputPowerW = 0;
    double lossW = 0;
    double command = 0;
};
struct NativeElectricDriveRuntime {
    std::shared_ptr<motion::MotionElectricDriveNetwork> network;
    std::map<int, std::vector<NativePowerSupplySample>> supplyHistories;
    std::map<int, std::vector<NativeInverterSample>> inverterHistories;
};

void createNativeEffortMotors(NativeMotionSystem& system,const NativeMotionInput& input);
double nativeEffortSpeed(const NativeEffortRuntime& runtime);
double limitNativeEffort(const NativeEffortRuntime& runtime,double requested);
void recordNativeEffortMotors(NativeMotionSystem& system);

void createNativeHydraulicCylinders(NativeMotionSystem& system, const NativeMotionInput& input);
void createNativeHydraulicMotors(NativeMotionSystem& system, const NativeMotionInput& input);
void createNativeHydraulicCircuit(NativeMotionSystem& system, const NativeMotionInput& input);
void createNativeElectricAndIntegratedActuators(NativeMotionSystem& system, const NativeMotionInput& input);
QString initializeSteadyNativeHydraulics(NativeMotionSystem& system, const QJsonObject& scenario);
void recordNativeHydraulicCylinders(NativeMotionSystem& system);
void recordNativeHydraulics(NativeMotionSystem& system);
}
