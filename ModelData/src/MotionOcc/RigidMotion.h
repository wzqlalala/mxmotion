#pragma once

#include <QString>
#include <QStringList>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace MotionOcc {

class CadDocument;
struct NativeMotionDocument;
struct NativeMotionInput;
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

// Presentation requests belong to the scenario, as in the original Motion
// curve manager. They do not change the dynamics or invalidate solved samples.
enum class RigidCurveMetric {
    BodyPosition, BodyDisplacement, BodyVelocity, BodyRotation,
    BodyAngularVelocity, JointCoordinate, JointRate, NativeChannel
};

struct RigidCurveRequest {
    int id = 0;
    RigidCurveMetric metric = RigidCurveMetric::BodyPosition;
    int sourceId = 0;
    int component = 0; // X/Y/Z = 0/1/2, vector magnitude = 3; scalar joint = 0.
    QString name;
    QString color;
    bool visible = true;
    QString sourceType; // Stable native result collection; not a UI list index.
    QString variable;
};

struct RigidScenario {
    int id = 0;
    QString name;
    double duration = 1.0;
    double step = 0.01;
    Vector3d gravityMS2{0, 0, -9.80665};
    std::vector<RigidCurveRequest> curveRequests;
};

struct RigidModel {
    std::vector<RigidBody> bodies;
    std::vector<RigidJoint> joints;
    std::vector<RigidDrive> drives;
    std::vector<RigidScenario> scenarios;
    std::shared_ptr<NativeMotionDocument> nativeDocument;
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
    std::shared_ptr<NativeMotionInput> nativeInput;
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

struct ResultChannel {
    QString sourceType;
    int sourceId = 0;
    QString variable;
    QString unit;
    int components = 1;
    std::vector<double> values; // Frame-major, components values per result frame.
};

// A real eigensolution, separate from time-history/modal-coordinate data. The
// visualization may normalize a column, but the stored solver basis is never
// rescaled. A fixed-interface basis is not the assembled mechanism's modes.
struct FlexModalBasis {
    int requestedCount = 0;
    QString kind; // "FixedInterface", "FreeBody" or "MNF".
    double cutoffHz = 0;
    std::vector<double> frequenciesHz;
    std::vector<double> shapes; // Row-major 3N x frequenciesHz.size().
    std::vector<std::vector<int>> attachmentGroups; // Zero-based mesh indices.
    std::vector<int> interfaceBodyIds;
    QStringList diagnostics;
    std::vector<int> sourceModeIds; // External modal IDs, in column order.
};

struct RigidFlexResult {
    int flexId = 0;
    int bodyId = 0;
    std::vector<double> referenceNodes;
    std::vector<int> surfaceTriangles;
    std::vector<std::vector<double>> nodePositions; // Global mm, one node vector per accepted frame.
    // Original modal-result payload. Indices refer to referenceNodes, and every
    // frame array follows RigidResult::frames. All poses are [x,y,z,w,qx,qy,qz].
    std::vector<int> tetrahedra;
    int modeCount = 0;
    int interfaceCount = 0; // Unique interface bodies, not attachment patches.
    double densityKgMm3 = 0;
    std::vector<double> psiInternal; // Row-major 3N x (6*interfaceCount + modeCount).
    std::vector<double> initialInterfacePoses;
    std::vector<std::vector<double>> interfacePoses;
    std::vector<std::vector<double>> modalCoordinates;
    bool reducedDynamics = true; // false: real co-rotational tetra4 dynamics.
    FlexModalBasis modalBasis;
    std::vector<int> sourceNodeIds; // External node IDs, in referenceNodes row order.
    std::vector<double> contourMassFractions; // Normalized lumped masses for AutoFlex.
};

struct FlexModeSample {
    std::vector<double> positions; // Preview only, reference + normalized shape.
    std::vector<double> magnitudes; // Dimensionless, maximum unphased norm = 1.
    double frequencyHz = 0;
    double rawMaximum = 0; // Original eigenvector maximum norm, before preview scaling.
};

bool validateFlexModes(const RigidFlexResult& result, QString& error);
// amplitudeMm is the maximum displayed displacement at phase=1, not a solved
// physical amplitude. phase is signed [-1,1], shared by coordinates and colors.
bool sampleFlexMode(const RigidFlexResult& result, int mode, double amplitudeMm,
                    double phase, FlexModeSample& sample, QString& error);
bool exportFlexModesCsv(const QString& path, const RigidFlexResult& result, QString& error);

// The legacy free-mode option is a mass-weighted cosine projection used by the
// original UI; it is not an exact free-free eigensolution or an NX/MNF result.
enum class FlexContourKind { ElasticDisplacement, TotalDisplacement, LegacyFreeModeProjection };
void prepareAutoFlexContour(RigidFlexResult& result);
bool flexNodeScalars(const RigidFlexResult& result, std::size_t frame, FlexContourKind kind,
                     std::vector<double>& values, QString& error);
// Interpolate vectors/modal coordinates first, then compute magnitudes; a
// signed mode crossing zero must not retain the endpoint's nonzero colour.
bool flexNodeScalarsAt(const RigidFlexResult& result, std::size_t left, std::size_t right,
                       double fraction, FlexContourKind kind, std::vector<double>& values, QString& error);

enum class ResultTermination { EndTime, GeometryBoundary, FlexDivergence, StaticEquilibrium };

struct RigidResult {
    int scenarioId = 0;
    std::vector<RigidFrame> frames;
    std::vector<ResultChannel> channels;
    std::vector<RigidFlexResult> flexBodies;
    ResultTermination termination = ResultTermination::EndTime;
    QStringList diagnostics;
};

struct SolverCallbacks {
    std::function<bool()> stopRequested;
    std::function<void(double)> progress; // fraction [0,1], called from the worker thread.
};

enum class SolveStatus { Completed, Stopped, Failed };

bool rigidBodyProperties(const CadDocument& cad, const RigidBody& body, BodyProperties& out, QString& error);
// Refresh lightweight UI summaries without losing any native source records.
// Scenario curve requests are retained by stable scenario ID.
void syncNativeMotionModel(RigidModel& model);
bool validateRigidModel(const CadDocument& cad, const RigidModel& model, QString& error);
bool prepareRigidModel(const CadDocument& cad, const RigidModel& model, int scenarioId, RigidInput& out, QString& error);

// No OCCT or GUI objects are touched by the solver. The output is replaced only on success.
SolveStatus solveRigid(const RigidInput& input, RigidResult& result, QString& error,
                       const SolverCallbacks& callbacks = {});

} // namespace MotionOcc
