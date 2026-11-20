#pragma once

#include <QString>

#include <array>
#include <functional>
#include <vector>

namespace MotionOcc {

class CadDocument;
using Vector3d = std::array<double, 3>;
using Quaterniond = std::array<double, 4>; // w, x, y, z

// Entity IDs are positive and local to each collection. Body 0 is ground.
// Geometry, joint origins and translational inputs are in mm; rotational
// inputs are in degrees. Mass/density and gravity keep their SI units.
struct RigidBody {
    int id = 0;
    QString name;
    int partId = 0;
    bool fixed = false;
    double massKg = 1.0;
    double densityKgM3 = 0.0; // Positive: use density instead of massKg.
};

enum class JointKind { Revolute, Prismatic, Fixed };
struct RigidJoint {
    int id = 0;
    QString name;
    JointKind kind = JointKind::Revolute;
    int movingBodyId = 0;
    int baseBodyId = 0;
    Vector3d originMm{};
    Vector3d axis{0, 0, 1};
};

enum class DriveKind { Speed, Displacement };
struct RigidDrive {
    int id = 0;
    QString name;
    int jointId = 0;
    DriveKind kind = DriveKind::Speed;
    // mm/s or deg/s for Speed. For Displacement, mm or degrees at the
    // scenario end, interpolated linearly from zero at t=0.
    double value = 0.0;
};

struct RigidScenario {
    int id = 0;
    QString name;
    double duration = 1.0;
    double step = 0.01;
    Vector3d gravityMS2{0, 0, -9.80665};
};

struct RigidModel {
    std::vector<RigidBody> bodies;
    std::vector<RigidJoint> joints;
    std::vector<RigidDrive> drives;
    std::vector<RigidScenario> scenarios;
};

struct BodyProperties {
    double massKg = 0.0;
    Vector3d centerMm{};
    std::array<double, 6> inertiaKgM2{}; // Ixx, Iyy, Izz, Ixy, Ixz, Iyz (tensor entries at the centroid).
};

struct PreparedBody {
    RigidBody body;
    BodyProperties properties;
};

struct RigidInput {
    std::vector<PreparedBody> bodies;
    std::vector<RigidJoint> joints;
    std::vector<RigidDrive> drives;
    RigidScenario scenario;
};

struct BodyState {
    int bodyId = 0;
    Vector3d positionMm{}; // World-space COM; rest orientation is the CAD world frame.
    Quaterniond rotation{1, 0, 0, 0};
    Vector3d velocityMmS{};
    Vector3d angularVelocityDegS{};
};

struct JointState {
    int jointId = 0;
    double position = 0.0; // mm for linear, degrees for angular; continuous from the initial frame.
    double speed = 0.0; // mm/s or degrees/s.
};

struct RigidFrame {
    double seconds = 0.0;
    std::vector<BodyState> bodies;
    std::vector<JointState> joints;
};

struct RigidResult {
    int scenarioId = 0;
    std::vector<RigidFrame> frames;
};

struct SolverCallbacks {
    std::function<bool()> stopRequested;
    std::function<void(double)> progress; // fraction [0,1], called from the worker thread.
};

enum class SolveStatus { Completed, Stopped, Failed };

bool rigidBodyProperties(const CadDocument& cad, const RigidBody& body, BodyProperties& out, QString& error);
bool validateRigidModel(const CadDocument& cad, const RigidModel& model, QString& error);
bool prepareRigidModel(const CadDocument& cad, const RigidModel& model, int scenarioId, RigidInput& out, QString& error);

// No OCCT or GUI objects are touched by the solver. The output is replaced only on success.
SolveStatus solveRigid(const RigidInput& input, RigidResult& result, QString& error,
                       const SolverCallbacks& callbacks = {});

} // namespace MotionOcc
