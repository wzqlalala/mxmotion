#pragma once

#include "NativeMotionGeometry.h"
#include "NativeMotionActuators.h"
#include "motion/physics/SystemSMC.h"
#include "motion/physics/LinkMotor.h"
#include "motion/functions/Function_Setpoint.h"
#include "motion/modal/FlexBodyBuilder.h"

#include <functional>
#include <cstdint>
#include <set>

namespace motion { class MotionFlexBodyAuxRef; }

namespace MotionOcc {

// The native engine records a completed dynamics step before checking its
// geometric boundaries. The adapter uses its pre-record end-of-step hook to
// reject that trial state without recording it as an accepted result.
struct NativeGeometryBoundary {
    QString jointId;
    QString kind;
    double attemptedTime = 0;
    double distanceMm = 0;
};
constexpr double NativeGeometryBoundaryToleranceMm = 0.03;

// A rejected trial state. It is thrown before the engine records a frame, so
// rigid channels, flexible meshes and modal coordinates retain one time axis.
struct NativeFlexDivergence {
    int flexId = -1;
    double attemptedTime = 0;
    QString reason;
};

struct NativePidSample { double time, target, feedback, error, output; };
struct NativePidRuntime {
    int id=-1;
    int driveId=-1;
    QJsonObject definition;
    QJsonObject trajectory;
    QJsonObject sensor;
    std::shared_ptr<motion::MotionLinkMotor> motor;
    std::shared_ptr<motion::MotionFunction_Setpoint> setpoint;
    std::shared_ptr<NativeEffortRuntime> effort;
    QString feedbackUnit,outputUnit;
    double integral=0, lastFeedback=0, lastTime=0, unwrappedAngle=0;
    bool hasSample=false, hasAngle=false;
    std::vector<NativePidSample> history;
};

class NativeMotionSystem : public motion::MotionSystemSMC {
public:
    std::vector<NativePidRuntime> controllers;
    std::map<int, std::shared_ptr<motion::MotionBody>> nativeBodies;
    std::map<int, std::shared_ptr<motion::MotionLinkMotor>> nativeMotors;
    std::map<int, std::shared_ptr<NativeEffortRuntime>> effortMotors;
    std::map<std::string, std::shared_ptr<motion::MotionLinkBase>> nativeJoints;
    std::set<int> importedMnfBodyIds;
    double acceptedStepSize=0, acceptedEndTime=0;
    std::uint64_t acceptedStepIndex=0;
    const NativeMotionInput* sourceInput=nullptr;
    bool projectPositions=false;
    std::function<void(double)> validateFlexState;
    std::function<void(double)> captureFlexState;
    int positionIterations=50;
    std::vector<motion::modal::MotionModalAssembly*> rigidInterfaceFlex;
    void configureControllers(const NativeMotionInput& input);
    void prepareStaticDescriptor(){DescriptorPrepareInject(*GetSystemDescriptor());}
    void projectConstraintVelocity(motion::MotionState& position, motion::MotionStateDelta& velocity, double& time);
    void projectAcceptedPositions();
    void CustomEndOfStep() override;
};

void validateNativeAcceptedConstraints(const NativeMotionSystem& system);
void rejectNativeGeometryBoundary(const NativeMotionSystem& system);

struct NativeFlexRuntime {
    int id=-1, bodyId=-1;
    motion::modal::MotionFlexBodyResult result;
    std::shared_ptr<motion::MotionFlexBodyAuxRef> importedBody;
    std::vector<int> importedNodeIds;
    motion::MotionFrame<> importedReferencePose;
    std::vector<double> times;
    std::vector<std::vector<double>> modalCoordinates;
    std::vector<std::vector<double>> nodePositions;
    std::vector<int> tetrahedra;
    double densityKgMm3=0;
    std::vector<std::vector<double>> interfacePoses;
    std::vector<double> referenceInterfacePoses;
    bool reducedDynamics=true;
    FlexModalBasis modalBasis;
};

NativeFlexRuntime buildImportedMnfFlex(NativeMotionSystem& system, const PreparedBody& body,
                                       const QJsonObject& definition, const QString& path,
                                       const motion::MotionFrame<>& referencePose,
                                       const motion::MotionVector<>& initialVelocity,
                                       const motion::MotionVector<>& initialAngularVelocity);
std::vector<NativeFlexRuntime> buildNativeFlex(NativeMotionSystem& system, const NativeMotionInput& input,
                                             const SolverCallbacks& callbacks, bool inspectionOnly=false);
void captureNativeFlex(std::vector<NativeFlexRuntime>& flex, double time);
void validateNativeFlex(const std::vector<NativeFlexRuntime>& flex, double time);
void extractNativeResults(const NativeMotionInput& input, const NativeMotionSystem& system,
                          const std::vector<NativeFlexRuntime>& flex, RigidResult& result);

// Sensor data is evaluated from the same accepted body state for runtime PID
// and post-processing; components are XYZ or a scalar value in element zero.
std::vector<double> evaluateNativeSensor(const QJsonObject& definition,
                                       const std::map<int,motion::state_kinematic>& bodies,
                                       const std::map<int,motion::joint_result>& joints = {},
                                       const std::map<int,motion::motion_result>& drives = {});

} // namespace MotionOcc
