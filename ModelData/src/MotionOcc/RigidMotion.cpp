#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "RigidMotion.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "CadDocument.h"

#include <BRepBndLib.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Shell.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

#include <QColor>

#include "motion/functions/Function_Const.h"
#include "motion/functions/Function_Ramp.h"
#include "motion/physics/Body.h"
#include "motion/physics/LinkLock.h"
#include "motion/physics/LinkMotorLinearPosition.h"
#include "motion/physics/LinkMotorLinearSpeed.h"
#include "motion/physics/LinkMotorRotationAngle.h"
#include "motion/physics/LinkMotorRotationSpeed.h"
#include "motion/physics/Marker.h"
#include "motion/physics/SystemNSC.h"

#include <Eigen/Eigenvalues>
#include <Eigen/SparseQR>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace MotionOcc {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kMmToM = 1e-3;
constexpr double kMToMm = 1e3;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kRadToDeg = 180.0 / kPi;
constexpr double kMm3ToM3 = kMmToM * kMmToM * kMmToM;
constexpr double kMm2ToM2 = kMmToM * kMmToM;

// Bounds are for an interactive, in-memory rigid run, including the histories
// that MotionSystem::StartSolver keeps internally in addition to our frames.
constexpr std::size_t kMaxBodies = 256;
constexpr std::size_t kMaxJoints = 1024;
constexpr std::size_t kMaxScenarios = 256;
constexpr std::size_t kMaxSteps = 100000;
constexpr std::size_t kMaxHistoryEntries = 500000;
constexpr double kMinTime = 1e-8;
constexpr double kMaxTime = 1e5;
constexpr double kMaxCoordinateMm = 1e9;
constexpr double kMinMassKg = 1e-12;
constexpr double kMaxMassKg = 1e12;
constexpr double kMaxDensityKgM3 = 1e9;
constexpr double kMaxGravityMS2 = 1e5;
constexpr double kMaxDriveMagnitude = 1e9;
constexpr double kMinVolumeMm3 = 1e-15;
constexpr double kConstraintTolerance = 1e-7; // metres or quaternion residual components

struct RigidError { QString message; };

[[noreturn]] void fail(const QString& message)
{
    throw RigidError{message};
}

template <class Operation>
bool runPreparation(QString& error, const Operation& operation)
{
    try {
        operation();
        error.clear();
        return true;
    } catch (const RigidError& failure) {
        error = failure.message;
    } catch (const Standard_Failure& failure) {
        error = QStringLiteral("OCCT rigid body properties failed: %1")
                    .arg(QString::fromUtf8(failure.GetMessageString()));
    } catch (const std::bad_alloc&) {
        error = QStringLiteral("Not enough memory to prepare the rigid model.");
    } catch (const std::exception& failure) {
        error = QStringLiteral("Rigid model preparation failed: %1").arg(QString::fromUtf8(failure.what()));
    } catch (...) {
        error = QStringLiteral("Rigid model preparation failed unexpectedly.");
    }
    return false;
}

template <std::size_t N>
bool bounded(const std::array<double, N>& values, double limit)
{
    return std::all_of(values.begin(), values.end(), [limit](double x) {
        return std::isfinite(x) && std::abs(x) <= limit;
    });
}

template <std::size_t N>
bool finite(const std::array<double, N>& values)
{
    return std::all_of(values.begin(), values.end(), [](double x) { return std::isfinite(x); });
}

void checkName(const QString& name, const QString& context)
{
    if (name.size() > 1024 || name.contains(QChar::Null))
        fail(context + QStringLiteral(" has an invalid name."));
}

void checkBodyFields(const RigidBody& body)
{
    const QString context = QStringLiteral("Rigid body %1").arg(body.id);
    if (body.id <= 0 || body.partId <= 0)
        fail(context + QStringLiteral(" needs a positive body and CAD part ID."));
    checkName(body.name, context);
    if (!std::isfinite(body.densityKgM3) || body.densityKgM3 < 0.0 || body.densityKgM3 > kMaxDensityKgM3)
        fail(context + QStringLiteral(" has an invalid density (kg/m³)."));
    if (!std::isfinite(body.massKg) || body.massKg < 0.0 || body.massKg > kMaxMassKg ||
        (body.densityKgM3 == 0.0 && body.massKg < kMinMassKg))
        fail(context + QStringLiteral(" has an invalid mass (kg)."));
}

Vector3d unitAxis(const RigidJoint& joint)
{
    const auto& axis = joint.axis;
    if (!finite(axis))
        fail(QStringLiteral("Joint %1 has a non-finite axis.").arg(joint.id));
    const double scale = std::max({std::abs(axis[0]), std::abs(axis[1]), std::abs(axis[2])});
    if (scale == 0.0)
        fail(QStringLiteral("Joint %1 needs a nonzero axis.").arg(joint.id));
    const Vector3d scaled{axis[0] / scale, axis[1] / scale, axis[2] / scale};
    const double length = std::hypot(scaled[0], scaled[1], scaled[2]);
    return {scaled[0] / length, scaled[1] / length, scaled[2] / length};
}

struct Topology {
    std::size_t movable = 0;
    std::size_t constraints = 0;
    std::unordered_map<int, JointKind> jointKinds;
};

const RigidBody& bodyDefinition(const RigidBody& body) { return body; }
const RigidBody& bodyDefinition(const PreparedBody& body) { return body.body; }

template <class Body>
Topology checkTopology(const std::vector<Body>& bodies, const std::vector<RigidJoint>& joints,
                       const std::vector<RigidDrive>& drives)
{
    if (bodies.size() > kMaxBodies || joints.size() > kMaxJoints || drives.size() > kMaxJoints)
        fail(QStringLiteral("Too many rigid bodies, joints or drives; simplify the model."));

    Topology topology;
    std::unordered_map<int, bool> fixed{{0, true}};
    std::unordered_set<int> parts;
    for (const auto& item : bodies) {
        const auto& body = bodyDefinition(item);
        checkBodyFields(body);
        if (!fixed.emplace(body.id, body.fixed).second)
            fail(QStringLiteral("Duplicate rigid body ID %1.").arg(body.id));
        if (!parts.insert(body.partId).second)
            fail(QStringLiteral("CAD part %1 is owned by more than one rigid body.").arg(body.partId));
        if (!body.fixed)
            ++topology.movable;
    }

    for (const auto& joint : joints) {
        const QString context = QStringLiteral("Joint %1").arg(joint.id);
        if (joint.id <= 0 || !topology.jointKinds.emplace(joint.id, joint.kind).second)
            fail(context + QStringLiteral(" needs a unique positive ID."));
        checkName(joint.name, context);
        const auto moving = fixed.find(joint.movingBodyId);
        const auto base = fixed.find(joint.baseBodyId);
        if (joint.movingBodyId <= 0 || moving == fixed.end() || base == fixed.end() || moving == base)
            fail(context + QStringLiteral(" needs two distinct body references (ground is a base only)."));
        if (moving->second && base->second)
            fail(context + QStringLiteral(" cannot connect two fixed bodies."));
        if (!bounded(joint.originMm, kMaxCoordinateMm))
            fail(context + QStringLiteral(" has an invalid origin (mm)."));
        unitAxis(joint);
        switch (joint.kind) {
        case JointKind::Revolute:
        case JointKind::Prismatic:
            topology.constraints += 5;
            break;
        case JointKind::Fixed:
            topology.constraints += 6;
            break;
        default:
            fail(context + QStringLiteral(" has an unknown joint type."));
        }
    }

    std::unordered_set<int> driveIds;
    std::unordered_set<int> drivenJoints;
    for (const auto& drive : drives) {
        const QString context = QStringLiteral("Drive %1").arg(drive.id);
        if (drive.id <= 0 || !driveIds.insert(drive.id).second)
            fail(context + QStringLiteral(" needs a unique positive ID."));
        checkName(drive.name, context);
        const auto target = topology.jointKinds.find(drive.jointId);
        if (target == topology.jointKinds.end() || target->second == JointKind::Fixed)
            fail(context + QStringLiteral(" needs a revolute or prismatic joint."));
        if (!drivenJoints.insert(drive.jointId).second)
            fail(QStringLiteral("Joint %1 has more than one drive.").arg(drive.jointId));
        if (drive.kind != DriveKind::Speed && drive.kind != DriveKind::Displacement)
            fail(context + QStringLiteral(" has an unknown drive type."));
        if (!std::isfinite(drive.value) || std::abs(drive.value) > kMaxDriveMagnitude)
            fail(context + QStringLiteral(" has an invalid speed or displacement."));
        ++topology.constraints;
    }
    return topology;
}

std::size_t checkScenario(const RigidScenario& scenario, std::size_t bodies,
                          std::size_t joints, std::size_t drives)
{
    const QString context = QStringLiteral("Scenario %1").arg(scenario.id);
    if (scenario.id <= 0)
        fail(context + QStringLiteral(" needs a positive ID."));
    checkName(scenario.name, context);
    if (!std::isfinite(scenario.duration) || !std::isfinite(scenario.step) ||
        scenario.duration < kMinTime || scenario.duration > kMaxTime ||
        scenario.step < kMinTime || scenario.step > kMaxTime)
        fail(context + QStringLiteral(" needs a duration and step between 1e-8 and 1e5 seconds."));
    if (!bounded(scenario.gravityMS2, kMaxGravityMS2))
        fail(context + QStringLiteral(" has an invalid gravity vector (m/s²)."));

    const double steps = std::ceil(scenario.duration / scenario.step);
    if (steps > kMaxSteps)
        fail(context + QStringLiteral(" exceeds the 100000-step limit."));
    // Ground, frame, and two native markers per passive joint. Allow the
    // initial frame and one extra terminal interval due to accumulated roundoff.
    const auto frames = static_cast<std::size_t>(steps) + 2;
    if (frames > kMaxHistoryEntries / (2 + bodies + 3 * joints + drives))
        fail(context + QStringLiteral(" exceeds the rigid history budget; increase the step or shorten the run."));
    return frames;
}

const RigidScenario* checkScenarios(const RigidModel& model, int selectedId)
{
    if (model.scenarios.size() > kMaxScenarios)
        fail(QStringLiteral("Too many rigid scenarios."));
    const RigidScenario* selected = nullptr;
    std::unordered_set<int> ids;
    std::unordered_set<int> bodyIds;
    std::unordered_map<int, JointKind> jointKinds;
    for (const auto& body : model.bodies) bodyIds.insert(body.id);
    for (const auto& joint : model.joints) jointKinds.emplace(joint.id, joint.kind);
    std::size_t totalRequests = 0;
    for (const auto& scenario : model.scenarios) {
        checkScenario(scenario, model.bodies.size(), model.joints.size(), model.drives.size());
        if (!ids.insert(scenario.id).second)
            fail(QStringLiteral("Duplicate scenario ID %1.").arg(scenario.id));
        totalRequests += scenario.curveRequests.size();
        if (scenario.curveRequests.size() > 256 || totalRequests > 20000)
            fail(QStringLiteral("Too many curve requests (256 per scenario, 20000 per project)."));
        std::unordered_set<int> requestIds;
        for (const auto& request : scenario.curveRequests) {
            const auto context = QStringLiteral("Curve %1 in scenario %2").arg(request.id).arg(scenario.id);
            if (request.id <= 0 || !requestIds.insert(request.id).second)
                fail(context + QStringLiteral(" needs a unique positive ID."));
            checkName(request.name, context);
            if (!request.color.isEmpty() && (request.color.size() > 64 || !QColor(request.color).isValid()))
                fail(context + QStringLiteral(" has an invalid color."));
            const bool jointMetric = request.metric == RigidCurveMetric::JointCoordinate ||
                                     request.metric == RigidCurveMetric::JointRate;
            int maxComponent = 3;
            switch (request.metric) {
            case RigidCurveMetric::BodyPosition:
            case RigidCurveMetric::BodyDisplacement:
            case RigidCurveMetric::BodyVelocity:
            case RigidCurveMetric::BodyAngularVelocity:
                break;
            case RigidCurveMetric::BodyRotation: maxComponent = 2; break;
            case RigidCurveMetric::JointCoordinate:
            case RigidCurveMetric::JointRate: maxComponent = 0; break;
            default: fail(context + QStringLiteral(" has an unknown metric."));
            }
            if (request.component < 0 || request.component > maxComponent)
                fail(context + QStringLiteral(" has an invalid component."));
            if (jointMetric) {
                const auto joint = jointKinds.find(request.sourceId);
                if (joint == jointKinds.end() || joint->second == JointKind::Fixed)
                    fail(context + QStringLiteral(" needs an existing revolute or prismatic joint."));
            } else if (bodyIds.count(request.sourceId) == 0) {
                fail(context + QStringLiteral(" refers to a missing motion body."));
            }
        }
        if (scenario.id == selectedId)
            selected = &scenario;
    }
    return selected;
}

double driveRateSI(const RigidDrive& drive, JointKind jointKind, const RigidScenario& scenario)
{
    const bool speed = drive.kind == DriveKind::Speed;
    const double rate = speed ? drive.value : drive.value / scenario.duration;
    const double travel = speed ? rate * scenario.duration : drive.value;
    if (!std::isfinite(rate) || !std::isfinite(travel) ||
        std::abs(rate) > kMaxDriveMagnitude || std::abs(travel) > kMaxDriveMagnitude)
        fail(QStringLiteral("Drive %1 exceeds the speed or travel range in scenario %2.").arg(drive.id).arg(scenario.id));
    if (jointKind == JointKind::Prismatic)
        return rate * kMmToM;
    const double radiansPerSecond = rate * kDegToRad;
    if (std::abs(radiansPerSecond) * std::min(scenario.step, scenario.duration) >= kPi)
        fail(QStringLiteral("Drive %1 turns at least 180 degrees in a step; reduce the scenario step.").arg(drive.id));
    return radiansPerSecond;
}

void checkSolvable(const Topology& topology)
{
    if (!topology.movable)
        fail(QStringLiteral("The rigid model needs at least one movable body."));
    if (topology.constraints > 6 * topology.movable)
        fail(QStringLiteral("The rigid model has more constraints than physical degrees of freedom."));
}

motion::MotionMatrix33<> inertiaMatrix(const std::array<double, 6>& i)
{
    motion::MotionMatrix33<> matrix;
    matrix << i[0], i[3], i[4], i[3], i[1], i[5], i[4], i[5], i[2];
    return matrix;
}

void checkProperties(int bodyId, const BodyProperties& properties)
{
    const auto& i = properties.inertiaKgM2;
    const QString context = QStringLiteral("Rigid body %1").arg(bodyId);
    if (!std::isfinite(properties.massKg) || properties.massKg < kMinMassKg || properties.massKg > kMaxMassKg ||
        !bounded(properties.centerMm, kMaxCoordinateMm) || !bounded(i, 1e24))
        fail(context + QStringLiteral(" has invalid mass, centroid or inertia."));
    const double scale = std::max({i[0], i[1], i[2]});
    if (scale < 1e-30)
        fail(context + QStringLiteral(" has a degenerate inertia tensor."));
    const Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> principal(inertiaMatrix(i) / scale, Eigen::EigenvaluesOnly);
    const auto& moments = principal.eigenvalues();
    if (principal.info() != Eigen::Success || !moments.allFinite() || moments[0] <= moments[2] * 1e-12 ||
        moments[2] > moments[0] + moments[1] + 1e-9 * moments[2])
        fail(context + QStringLiteral(" has a nonphysical or ill-conditioned inertia tensor."));
}

BodyProperties computeProperties(const CadDocument& cad, const RigidBody& body)
{
    const QString context = QStringLiteral("CAD part %1 for rigid body %2").arg(body.partId).arg(body.id);
    const auto* part = cad.part(body.partId);
    if (!part)
        fail(context + QStringLiteral(" does not exist."));
    const auto& shape = part->shape;
    if (shape.IsNull() || (shape.ShapeType() != TopAbs_SOLID && shape.ShapeType() != TopAbs_COMPSOLID) ||
        !BRepCheck_Analyzer(shape).IsValid())
        fail(context + QStringLiteral(" must be a valid solid, not a shell, surface or wire."));

    Bnd_Box bounds;
    BRepBndLib::AddOptimal(shape, bounds, false, false);
    if (bounds.IsVoid() || bounds.IsOpen())
        fail(context + QStringLiteral(" has invalid bounds."));
    double x0, y0, z0, x1, y1, z1;
    bounds.Get(x0, y0, z0, x1, y1, z1);
    if (!bounded(std::array<double, 6>{x0, y0, z0, x1, y1, z1}, kMaxCoordinateMm) ||
        x0 >= x1 || y0 >= y1 || z0 >= z1)
        fail(context + QStringLiteral(" has degenerate or out-of-range bounds (mm)."));

    GProp_GProps volume(gp_Pnt((x0 + x1) * 0.5, (y0 + y1) * 0.5, (z0 + z1) * 0.5));
    for (TopExp_Explorer it(shape, TopAbs_SOLID); it.More(); it.Next()) {
        const auto& solid = it.Current();
        TopoDS_Iterator shell(solid);
        if (!shell.More())
            fail(context + QStringLiteral(" has no closed boundary."));
        for (; shell.More(); shell.Next()) {
            if (shell.Value().ShapeType() != TopAbs_SHELL ||
                BRepCheck_Shell(TopoDS::Shell(shell.Value())).Closed() != BRepCheck_NoError)
                fail(context + QStringLiteral(" contains an open or invalid shell."));
        }
        GProp_GProps component;
        // Integrate exact BRep surfaces, not the tessellation. Test the signed
        // volume per solid so inverted/degenerate solids cannot cancel out.
        const double integrationError = BRepGProp::VolumeProperties(solid, component, 1e-6, true, false);
        if (!std::isfinite(integrationError) || !std::isfinite(component.Mass()) || component.Mass() <= kMinVolumeMm3)
            fail(context + QStringLiteral(" contains a solid without positive finite volume."));
        volume.Add(component);
    }
    const double volumeMm3 = volume.Mass();
    if (!std::isfinite(volumeMm3) || volumeMm3 <= kMinVolumeMm3)
        fail(context + QStringLiteral(" has no usable solid volume."));

    BodyProperties properties;
    properties.massKg = body.densityKgM3 > 0.0 ? body.densityKgM3 * (volumeMm3 * kMm3ToM3) : body.massKg;
    const gp_Pnt center = volume.CentreOfMass();
    properties.centerMm = {center.X(), center.Y(), center.Z()};
    const gp_Mat i = volume.MatrixOfInertia(); // Unit-density tensor about COM, in mm^5.
    const double scale = (properties.massKg / volumeMm3) * kMm2ToM2;
    properties.inertiaKgM2 = {i(1, 1) * scale, i(2, 2) * scale, i(3, 3) * scale,
                              i(1, 2) * scale, i(1, 3) * scale, i(2, 3) * scale};
    checkProperties(body.id, properties);
    return properties;
}

using NativeVector = motion::MotionVector<>;
using NativeFrame = motion::MotionFrame<>;
using NativeBody = std::shared_ptr<motion::MotionBody>;
using NativeLock = std::shared_ptr<motion::MotionLinkLock>;

NativeVector vectorSI(const Vector3d& v, double scale = 1.0)
{
    return NativeVector(v[0] * scale, v[1] * scale, v[2] * scale);
}

Vector3d components(const NativeVector& v, double scale = 1.0)
{
    return {v.x() * scale, v.y() * scale, v.z() * scale};
}

NativeFrame jointFrame(const RigidJoint& joint, bool useMotorX)
{
    const auto axis = vectorSI(unitAxis(joint));
    const NativeVector seed = std::abs(axis.x()) < 0.8 ? NativeVector(1, 0, 0) : NativeVector(0, 1, 0);
    const auto transverse = seed.Cross(axis).GetNormalized();
    motion::MotionMatrix33<> orientation;
    if (useMotorX)
        orientation.Set_A_axis(axis, transverse, axis.Cross(transverse));
    else
        orientation.Set_A_axis(transverse, axis.Cross(transverse), axis);
    return NativeFrame(vectorSI(joint.originMm, kMmToM), orientation.Get_A_quaternion());
}

class RigidSystem final : public motion::MotionSystemNSC {
public:
    void initialize(std::size_t movable, std::size_t constraintCount, std::size_t speedMotorCount)
    {
        SetupInitial();
        Setup();
        Update(0.0, false);
        const int coordinates = static_cast<int>(6 * movable + speedMotorCount);
        const int rows = static_cast<int>(constraintCount);
        if (GetDOF_w() != coordinates || GetDOC_c() != rows || GetDOC_d() != 0)
            fail(QStringLiteral("Unexpected native body or constraint DOFs in the rigid assembly."));
        if (!rows)
            return;

        DescriptorPrepareInject(*GetSystemDescriptor());
        motion::MotionSparseMatrix jacobian;
        GetConstraintJacobianMatrix(&jacobian);
        if (jacobian.rows() != rows || jacobian.cols() != coordinates)
            fail(QStringLiteral("The native rigid constraint Jacobian has unexpected dimensions."));
        // Check independence on the geometric Jacobian rather than a KKT
        // matrix whose rank threshold would also depend on masses/inertias.
        for (int row = 0; row < rows; ++row) {
            const double norm = jacobian.row(row).norm();
            if (!std::isfinite(norm) || norm == 0.0)
                fail(QStringLiteral("The initial rigid assembly has a degenerate constraint."));
            for (motion::MotionSparseMatrix::InnerIterator entry(jacobian, row); entry; ++entry)
                entry.valueRef() /= norm;
        }
        Eigen::SparseMatrix<double> matrix = jacobian;
        matrix.makeCompressed();
        Eigen::SparseQR<Eigen::SparseMatrix<double>, Eigen::COLAMDOrdering<int>> qr;
        qr.setPivotThreshold(1e-10);
        qr.compute(matrix);
        if (qr.info() != Eigen::Success || qr.rank() != rows)
            fail(QStringLiteral("The initial rigid assembly is overconstrained or at a singular configuration."));
    }

    bool StateSolveCorrection(motion::MotionStateDelta& dv, motion::MotionVectorDynamic<>& multipliers,
                              const motion::MotionVectorDynamic<>& residual, const motion::MotionVectorDynamic<>& constraints,
                              double mass, double damping, double stiffness, const motion::MotionState& x,
                              const motion::MotionStateDelta& v, double time, bool scatter, bool update, bool setup) override
    {
        // The pinned assembly and Euler integrator do not propagate a false
        // return from StateSolveCorrection. Don't accept an unsolved step.
        if (!residual.allFinite() || !constraints.allFinite() ||
            !motion::MotionSystemNSC::StateSolveCorrection(dv, multipliers, residual, constraints, mass, damping,
                                                            stiffness, x, v, time, scatter, update, setup) ||
            !dv.allFinite() || !multipliers.allFinite())
            fail(QStringLiteral("Native rigid constraint solve failed at %1 s; check the model and step.").arg(time, 0, 'g', 12));
        return true;
    }
};

NativeLock addJoint(RigidSystem& system, const RigidJoint& joint, const NativeBody& moving, const NativeBody& base)
{
    NativeLock link;
    switch (joint.kind) {
    case JointKind::Revolute: link = motion_types::make_shared<motion::MotionLinkLockRevolute>(); break;
    case JointKind::Prismatic: link = motion_types::make_shared<motion::MotionLinkLockPrismatic>(); break;
    case JointKind::Fixed: link = motion_types::make_shared<motion::MotionLinkLockLock>(); break;
    }
    // Both passive joint types use local Z, and both anchors initially coincide
    // in world space, regardless of where either body's CAD COM lies.
    link->Initialize(moving, base, jointFrame(joint, false).GetCoord());
    const auto key = "joint:" + std::to_string(joint.id);
    link->SetNameString(key);
    link->GetMarker1()->SetNameString(key + ":moving");
    link->GetMarker2()->SetNameString(key + ":base");
    system.AddLink(link);
    return link;
}

// Return the speed motor, if one exists, so the auxiliary integrated travel
// can be reset after initial velocity assembly without modifying body velocities.
std::shared_ptr<motion::MotionLinkMotor> addMotor(RigidSystem& system, const RigidJoint& joint,
                                                 const RigidDrive& drive, const NativeBody& moving,
                                                 const NativeBody& base, double rateSI)
{
    const bool linear = joint.kind == JointKind::Prismatic;
    const bool speed = drive.kind == DriveKind::Speed;
    std::shared_ptr<motion::MotionLinkMotor> motor;
    if (linear) {
        auto linearMotor = speed ? std::static_pointer_cast<motion::MotionLinkMotorLinear>(
                                       motion_types::make_shared<motion::MotionLinkMotorLinearSpeed>())
                                 : std::static_pointer_cast<motion::MotionLinkMotorLinear>(
                                       motion_types::make_shared<motion::MotionLinkMotorLinearPosition>());
        linearMotor->SetGuideConstraint(motion::MotionLinkMotorLinear::GuideConstraint::FREE);
        motor = linearMotor;
    } else {
        auto rotaryMotor = speed ? std::static_pointer_cast<motion::MotionLinkMotorRotation>(
                                       motion_types::make_shared<motion::MotionLinkMotorRotationSpeed>())
                                 : std::static_pointer_cast<motion::MotionLinkMotorRotation>(
                                       motion_types::make_shared<motion::MotionLinkMotorRotationAngle>());
        rotaryMotor->SetSpindleConstraint(motion::MotionLinkMotorRotation::SpindleConstraint::FREE);
        motor = rotaryMotor;
    }
    motor->Initialize(moving, base, jointFrame(joint, linear));
    motor->SetNameString("drive:" + std::to_string(drive.id));
    if (speed)
        motor->SetMotorFunction(motion_types::make_shared<motion::MotionFunction_Const>(rateSI));
    else
        motor->SetMotorFunction(motion_types::make_shared<motion::MotionFunction_Ramp>(0.0, rateSI));
    system.AddLink(motor);
    return speed ? motor : nullptr;
}

void resetMotorTravel(RigidSystem& system, const std::vector<std::shared_ptr<motion::MotionLinkMotor>>& speedMotors)
{
    if (speedMotors.empty())
        return;
    motion::MotionState x;
    motion::MotionStateDelta v, a;
    double time;
    system.StateSetup(x, v, a);
    system.StateGather(x, v, time);
    for (const auto& motor : speedMotors)
        v(motor->GetOffset_w()) = 0.0;
    system.StateScatter(x, v, time, true);
}

double constraintResidual(RigidSystem& system)
{
    motion::MotionVectorDynamic<> residual = motion::MotionVectorDynamic<>::Zero(system.GetDOC());
    system.LoadConstraint_C(residual, 1.0);
    if (!residual.allFinite())
        fail(QStringLiteral("The rigid solve produced a non-finite constraint residual."));
    return residual.size() ? residual.cwiseAbs().maxCoeff() : 0.0;
}

struct JointSample {
    const RigidJoint* definition;
    NativeLock link;
    double lastCoordinate = 0.0;
    double lastSpeed = 0.0;
    double continuous = 0.0;
};

JointState sampleJoint(JointSample& sample, double dt)
{
    const auto& joint = *sample.definition;
    JointState state;
    state.jointId = joint.id;
    if (joint.kind == JointKind::Fixed)
        return state;

    motion::MotionFrameMoving<> relative;
    sample.link->GetMarker2()->GetAbsFrame().TransformParentToLocal(sample.link->GetMarker1()->GetAbsFrame(), relative);
    double coordinate, speed;
    if (joint.kind == JointKind::Prismatic) {
        coordinate = relative.GetPos().z();
        speed = relative.GetPos_dt().z(); // Includes base-frame rotation and the anchor offsets from the COMs.
    } else {
        const auto& q = relative.GetRot();
        coordinate = std::remainder(2.0 * std::atan2(q.e3(), q.e0()), 2.0 * kPi);
        speed = relative.GetWvel_par().z();
    }
    if (!std::isfinite(coordinate) || !std::isfinite(speed))
        fail(QStringLiteral("Joint %1 produced non-finite motion.").arg(joint.id));

    if (dt == 0.0) {
        sample.lastCoordinate = coordinate; // Initial datum for both types of joint.
    } else if (joint.kind == JointKind::Prismatic) {
        sample.continuous = coordinate - sample.lastCoordinate;
    } else {
        const double delta = std::remainder(coordinate - sample.lastCoordinate, 2.0 * kPi);
        if (std::max(std::abs(sample.lastSpeed), std::abs(speed)) * dt >= kPi || std::abs(delta) >= kPi - 1e-9)
            fail(QStringLiteral("Joint %1 rotates too far per step to track its angle; reduce the step.").arg(joint.id));
        sample.continuous += delta;
        sample.lastCoordinate = coordinate;
    }
    sample.lastSpeed = speed;
    const double scale = joint.kind == JointKind::Revolute ? kRadToDeg : kMToMm;
    state.position = sample.continuous * scale;
    state.speed = speed * scale;
    if (!std::isfinite(state.position) || !std::isfinite(state.speed))
        fail(QStringLiteral("Joint %1 motion is out of range.").arg(joint.id));
    return state;
}

BodyState sampleBody(int id, const motion::MotionBody& body)
{
    BodyState state;
    state.bodyId = id;
    state.positionMm = components(body.GetPos(), kMToMm);
    state.velocityMmS = components(body.GetPos_dt(), kMToMm);
    state.angularVelocityDegS = components(body.GetWvel_par(), kRadToDeg);
    const auto& q = body.GetRot();
    const double length = q.Length();
    if (!finite(state.positionMm) || !finite(state.velocityMmS) || !finite(state.angularVelocityDegS) ||
        !std::isfinite(length) || std::abs(length - 1.0) > 1e-6)
        fail(QStringLiteral("Rigid body %1 produced an invalid pose or velocity.").arg(id));
    state.rotation = {q.e0() / length, q.e1() / length, q.e2() / length, q.e3() / length};
    return state;
}

} // namespace

bool rigidBodyProperties(const CadDocument& cad, const RigidBody& body, BodyProperties& out, QString& error)
{
    return runPreparation(error, [&] {
        checkBodyFields(body);
        out = computeProperties(cad, body);
    });
}

void syncNativeMotionModel(RigidModel& model)
{
    if (!model.nativeDocument) return;
    const auto oldScenarios = std::move(model.scenarios);
    model.bodies.clear();
    model.joints.clear();
    model.drives.clear();
    model.scenarios.clear();
    for (const auto& value : model.nativeDocument->collection(QStringLiteral("bodies"))) {
        const auto row = value.toObject();
        const int id = row.value(QStringLiteral("id")).toInt();
        const auto parts = model.nativeDocument->bodyPartBindings.value(id);
        model.bodies.push_back({id, row.value(QStringLiteral("name")).toString(),
            parts.isEmpty() ? 0 : parts.front(), row.value(QStringLiteral("isFixed")).toBool(),
            row.value(QStringLiteral("mass")).toDouble(1.0), 0.0});
    }
    for (const auto& value : model.nativeDocument->collection(QStringLiteral("scenarios"))) {
        const auto row = value.toObject();
        const auto gravity = row.value(QStringLiteral("gravity")).toObject();
        RigidScenario scenario;
        scenario.id = row.value(QStringLiteral("id")).toInt();
        scenario.name = row.value(QStringLiteral("name")).toString();
        scenario.duration = row.value(QStringLiteral("endTime")).toDouble(1.0);
        scenario.step = row.value(QStringLiteral("stepSize")).toDouble(0.01);
        scenario.gravityMS2 = {gravity.value(QStringLiteral("x")).toDouble() * 0.001,
            gravity.value(QStringLiteral("y")).toDouble() * 0.001, gravity.value(QStringLiteral("z")).toDouble(-9810.0) * 0.001};
        const auto old = std::find_if(oldScenarios.begin(), oldScenarios.end(),
            [&](const RigidScenario& value) { return value.id == scenario.id; });
        if (old != oldScenarios.end()) {
            // An intentionally cleared list must remain empty after model edits.
            scenario.curveRequests = old->curveRequests;
        } else {
            const QMap<QString, QString> sources{
                {"Body", "bodies"}, {"Joint", "joints"}, {"Marker", "markers"}, {"Drive", "drives"},
                {"Coupling", "couplings"}, {"Sensor", "sensors"}, {"Controller", "controllers"},
                {"Spring", "springs"}, {"TensionCable", "tensionCables"}, {"SoftStop", "softStops"},
                {"ScalarForce", "scalarForces"}, {"ScalarTorque", "scalarTorques"},
                {"ThreePointForce", "threePointForces"}, {"ThreePointTorque", "threePointTorques"},
                {"VectorForce", "vectorForces"}, {"AutoFlexBody", "autoFlexBodies"}};
            for (const auto& value : row.value(QStringLiteral("curveRequests")).toArray()) {
                const auto curve = value.toObject();
                RigidCurveRequest request;
                request.id = curve.value(QStringLiteral("requestId")).toInt();
                const auto used = [&](int id) {
                    return std::any_of(scenario.curveRequests.begin(), scenario.curveRequests.end(),
                        [id](const RigidCurveRequest& existing) { return existing.id == id; });
                };
                if (request.id <= 0 || used(request.id)) {
                    request.id = 1;
                    while (used(request.id)) ++request.id;
                }
                request.metric = RigidCurveMetric::NativeChannel;
                const auto source = curve.value(QStringLiteral("sourceType")).toString();
                request.sourceType = sources.value(source, source);
                request.sourceId = curve.value(QStringLiteral("sourceId")).toInt(-1);
                request.variable = curve.value(QStringLiteral("variable")).toString();
                const auto componentValue = curve.value(QStringLiteral("component"));
                const auto component = componentValue.toString();
                request.component = componentValue.isDouble() ? componentValue.toInt()
                    : component == QStringLiteral("Magnitude") ? -1
                    : component == QStringLiteral("Y") ? 1 : component == QStringLiteral("Z") ? 2 : 0;
                request.name = curve.value(QStringLiteral("requestName")).toString();
                request.visible = curve.value(QStringLiteral("visible")).toBool(true);
                const auto color = curve.value(QStringLiteral("color"));
                QColor selected;
                if (color.isString()) selected = QColor(color.toString());
                else if (color.isObject()) {
                    const auto rgba = color.toObject();
                    const int r = rgba.value(QStringLiteral("r")).toInt(-1);
                    const int g = rgba.value(QStringLiteral("g")).toInt(-1);
                    const int b = rgba.value(QStringLiteral("b")).toInt(-1);
                    if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255)
                        selected = QColor(r, g, b);
                }
                if (selected.isValid()) request.color = selected.name();
                scenario.curveRequests.push_back(std::move(request));
            }
        }
        model.scenarios.push_back(std::move(scenario));
    }
}

bool validateRigidModel(const CadDocument& cad, const RigidModel& model, QString& error)
{
    if (model.nativeDocument)
        return validateNativeMotionDocument(*model.nativeDocument, error)
            && validateNativeMotionBindings(cad, *model.nativeDocument, error);
    return runPreparation(error, [&] {
        checkTopology(model.bodies, model.joints, model.drives);
        checkScenarios(model, 0);
        for (const auto& body : model.bodies)
            computeProperties(cad, body);
    });
}

bool prepareRigidModel(const CadDocument& cad, const RigidModel& model, int scenarioId, RigidInput& out, QString& error)
{
    if (model.nativeDocument) {
        auto document = *model.nativeDocument;
        auto data = document.root.value(QStringLiteral("motionData")).toObject();
        auto drives = data.value(QStringLiteral("drives")).toArray();
        const double duration = document.entity(QStringLiteral("scenarios"), scenarioId)
            .value(QStringLiteral("endTime")).toDouble(1.0);
        for (int i = 0; i < drives.size(); ++i) {
            auto drive = drives[i].toObject();
            if (!drive.contains(QStringLiteral("legacyEndValue"))) continue;
            drive.insert(QStringLiteral("expression"), QStringLiteral("(%1)*t/(%2)")
                .arg(drive.value(QStringLiteral("legacyEndValue")).toDouble(), 0, 'g', 17).arg(duration, 0, 'g', 17));
            drives[i] = drive;
        }
        data.insert(QStringLiteral("drives"), drives);
        document.root.insert(QStringLiteral("motionData"), data);
        auto native = std::make_shared<NativeMotionInput>();
        if (!prepareNativeMotion(cad, document, scenarioId, *native, error)) return false;
        RigidInput prepared;
        prepared.bodies = native->bodies;
        prepared.scenario = native->scenario;
        for (const auto& scenario : model.scenarios)
            if (scenario.id == scenarioId) prepared.scenario.curveRequests = scenario.curveRequests;
        prepared.nativeInput = std::move(native);
        out = std::move(prepared);
        return true;
    }
    return runPreparation(error, [&] {
        const auto topology = checkTopology(model.bodies, model.joints, model.drives);
        const auto* scenario = checkScenarios(model, scenarioId);
        if (!scenario)
            fail(QStringLiteral("Rigid scenario %1 does not exist.").arg(scenarioId));
        checkSolvable(topology);
        for (const auto& drive : model.drives)
            driveRateSI(drive, topology.jointKinds.at(drive.jointId), *scenario);

        RigidInput prepared;
        prepared.scenario = *scenario;
        prepared.joints = model.joints;
        for (auto& joint : prepared.joints)
            joint.axis = unitAxis(joint);
        prepared.drives = model.drives;
        prepared.bodies.reserve(model.bodies.size());
        for (const auto& body : model.bodies)
            prepared.bodies.push_back({body, computeProperties(cad, body)});
        out = std::move(prepared);
    });
}

SolveStatus solveRigid(const RigidInput& input, RigidResult& result, QString& error, const SolverCallbacks& callbacks)
{
    if (input.nativeInput) return solveNativeMotion(*input.nativeInput, result, error, callbacks);
    error.clear();
    const auto stopRequested = [&] { return callbacks.stopRequested && callbacks.stopRequested(); };
    try {
        if (stopRequested())
            return SolveStatus::Stopped;
        const auto topology = checkTopology(input.bodies, input.joints, input.drives);
        const auto& scenario = input.scenario;
        const auto maxFrames = checkScenario(scenario, input.bodies.size(), input.joints.size(), input.drives.size());
        checkSolvable(topology);
        for (const auto& body : input.bodies)
            checkProperties(body.body.id, body.properties);

        RigidSystem system;
        std::unordered_map<int, NativeBody> bodies;
        auto ground = motion_types::make_shared<motion::MotionBody>();
        ground->SetNameString("ground");
        ground->SetIdentifier(0);
        ground->SetBodyFixed(true);
        system.AddBody(ground);
        bodies.emplace(0, ground);
        for (const auto& item : input.bodies) {
            const auto& data = item.body;
            const auto& properties = item.properties;
            auto body = motion_types::make_shared<motion::MotionBody>();
            body->SetNameString("body:" + std::to_string(data.id));
            body->SetIdentifier(data.id);
            body->SetPos(vectorSI(properties.centerMm, kMmToM));
            body->SetRot(motion::QUNIT);
            body->SetMass(properties.massKg);
            body->SetInertia(inertiaMatrix(properties.inertiaKgM2));
            body->SetBodyFixed(data.fixed);
            body->SetCollide(false);
            system.AddBody(body);
            bodies.emplace(data.id, std::move(body));
        }

        std::unordered_map<int, const RigidJoint*> jointDefinitions;
        std::vector<JointSample> joints;
        joints.reserve(input.joints.size());
        for (const auto& joint : input.joints) {
            jointDefinitions.emplace(joint.id, &joint);
            joints.push_back({&joint, addJoint(system, joint, bodies.at(joint.movingBodyId), bodies.at(joint.baseBodyId))});
        }
        std::vector<std::shared_ptr<motion::MotionLinkMotor>> speedMotors;
        for (const auto& drive : input.drives) {
            const auto& joint = *jointDefinitions.at(drive.jointId);
            const double rate = driveRateSI(drive, joint.kind, scenario);
            if (auto motor = addMotor(system, joint, drive, bodies.at(joint.movingBodyId), bodies.at(joint.baseBodyId), rate))
                speedMotors.push_back(std::move(motor));
        }
        if (stopRequested())
            return SolveStatus::Stopped;

        motion::SimulationConfig config;
        config.step_size = scenario.step;
        config.total_time = scenario.duration;
        config.use_preset = false;
        // These links are bilateral, with no contact/complementarity rows. A
        // direct solve avoids iterative tolerance issues with small SI inertias.
        config.solver.type = motion::MotionSolver::Type::SPARSE_LU;
        config.integrator.type = motion::MotionTimestepper::Type::EULER_IMPLICIT_LINEARIZED;
        system.InitSolver(config);
        system.Set_G_acc(motion::VNULL);
        system.initialize(topology.movable, topology.constraints, speedMotors.size());
        // Assemble compatible prescribed rates at t=0 without introducing a
        // spurious gravity impulse from the native assembly's internal dt.
        if (!joints.empty()) {
            system.DoAssembly(motion::AssemblyLevel::POSITION | motion::AssemblyLevel::VELOCITY);
            resetMotorTravel(system, speedMotors);
        }
        system.Set_G_acc(vectorSI(scenario.gravityMS2));
        system.Update(0.0, false);
        if (stopRequested())
            return SolveStatus::Stopped;

        RigidResult pending;
        pending.scenarioId = scenario.id;
        pending.frames.reserve(maxFrames);
        const auto record = [&](double time) {
            const bool initial = pending.frames.empty();
            const double dt = initial ? 0.0 : time - pending.frames.back().seconds;
            if (!std::isfinite(time) || (initial ? time != 0.0 : dt <= 0.0) ||
                time > scenario.duration || pending.frames.size() >= maxFrames)
                fail(QStringLiteral("MotionEngine returned an invalid rigid frame time or count."));
            double residual = constraintResidual(system);
            if (!initial && residual > kConstraintTolerance) {
                // The linearized step solves the velocity-level equations.
                // Project any larger position drift on the native constraint manifold.
                system.DoAssembly(motion::AssemblyLevel::POSITION);
                residual = constraintResidual(system);
            }
            if (residual > kConstraintTolerance)
                fail(QStringLiteral("Joint or drive constraints did not converge at %1 s; reduce the step or check the mechanism.")
                         .arg(time, 0, 'g', 12));

            RigidFrame frame;
            frame.seconds = time;
            frame.bodies.reserve(input.bodies.size());
            for (const auto& item : input.bodies)
                frame.bodies.push_back(sampleBody(item.body.id, *bodies.at(item.body.id)));
            frame.joints.reserve(joints.size());
            for (auto& joint : joints)
                frame.joints.push_back(sampleJoint(joint, dt));
            pending.frames.push_back(std::move(frame));
        };

        record(0.0);
        if (callbacks.progress)
            callbacks.progress(0.0);
        if (stopRequested())
            return SolveStatus::Stopped;
        // StartSolver invokes this after each accepted dynamics step, including
        // its shortened terminal interval. Use the live bodies and markers,
        // never the commanded values from the engine's motor history.
        system.SetProgressCallback([&](double time, double, double fraction) {
            if (stopRequested()) {
                system.RequestStopSolve();
                return;
            }
            record(time);
            if (callbacks.progress)
                callbacks.progress(std::clamp(fraction, 0.0, 1.0));
            if (stopRequested())
                system.RequestStopSolve();
        });
        system.StartSolver(config);
        if (system.IsStopSolveRequested() || stopRequested())
            return SolveStatus::Stopped;
        if (pending.frames.size() < 2 || pending.frames.back().seconds != scenario.duration ||
            system.GetMotionTime() != scenario.duration)
            fail(QStringLiteral("MotionEngine did not produce the terminal rigid frame."));

        result = std::move(pending);
        return SolveStatus::Completed;
    } catch (const RigidError& failure) {
        error = failure.message;
    } catch (const std::bad_alloc&) {
        error = QStringLiteral("Not enough memory to solve the rigid model.");
    } catch (const std::exception& failure) {
        error = QStringLiteral("MotionEngine rigid solve failed: %1").arg(QString::fromUtf8(failure.what()));
    } catch (...) {
        error = QStringLiteral("MotionEngine rigid solve failed unexpectedly.");
    }
    return SolveStatus::Failed;
}

} // namespace MotionOcc
