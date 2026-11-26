#include "CadDocument.h"
#include "NativeMotionDiagnostics.h"
#include "NativeMotionSolver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;

namespace {

constexpr double kPi = 3.14159265358979323846;

void require(bool value, const QString& message)
{
    if (!value) throw std::runtime_error(message.toStdString());
}

void expectNear(double value, double expected, double tolerance, const QString& message)
{
    require(std::isfinite(value) && std::abs(value - expected) <= tolerance,
            QStringLiteral("%1: actual=%2 vs expected=%3 (tol=%4)")
                .arg(message)
                .arg(value, 0, 'g', 15)
                .arg(expected, 0, 'g', 15)
                .arg(tolerance));
}

QJsonObject xyz(double x, double y, double z)
{
    return {{"x", x}, {"y", y}, {"z", z}};
}

void setCollection(NativeMotionDocument& document, const QString& name, const QJsonArray& values)
{
    auto data = document.root.value("motionData").toObject();
    data[name] = values;
    document.root["motionData"] = data;
}

void setHydraulicFluid(NativeMotionDocument& document, const QJsonObject& fluid)
{
    auto data = document.root.value("motionData").toObject();
    data["hydraulicFluid"] = fluid;
    document.root["motionData"] = data;
}

const ResultChannel& channel(const RigidResult& result, const QString& source, int id, const QString& variable)
{
    for (const auto& ch : result.channels)
        if (ch.sourceType == source && ch.sourceId == id && ch.variable == variable) return ch;
    throw std::runtime_error(QStringLiteral("missing channel %1/%2/%3").arg(source).arg(id).arg(variable).toStdString());
}

struct SliderFixture {
    CadDocument cad;
    NativeMotionDocument document;

    explicit SliderFixture(double massKg = 50.0)
    {
        QString error;
        int part = 0;
        require(cad.addPrimitive(Primitive::Box, 40, 40, 40, part, error), error);
        document.root = {{"formatVersion", 1}, {"motionData", QJsonObject{}}};
        document.bodyPartBindings.insert(0, {part});
        setCollection(document, "bodies", {QJsonObject{
            {"id", 0}, {"name", "slider"}, {"geoEntityIds", QJsonArray{part}},
            {"mass", massKg}, {"isCustomMassProps", true},
            {"centerOfMass", xyz(0, 0, 200)},
            {"inertia", QJsonArray{1000.0, 1000.0, 1000.0, 0, 0, 0}},
            {"initialVelocity", xyz(0, 0, 0)},
            {"initialAngularVelocity", xyz(0, 0, 0)}
        }});
        setCollection(document, "joints", {QJsonObject{
            {"id", 0}, {"name", "vertical_guide"}, {"jointType", "Translational"},
            {"actionBodyId", 0}, {"baseBodyId", -1},
            {"origin", xyz(0, 0, 0)}, {"axis", xyz(0, 0, 1)}
        }});
        setCollection(document, "scenarios", {QJsonObject{
            {"id", 0}, {"name", "hydraulic_case"},
            {"endTime", 0.25}, {"stepSize", 0.001},
            {"gravity", xyz(0, 0, 0)},
            {"usePreset", true}, {"presetMode", "FAST"},
            {"analysisType", "Dynamic"}
        }});
        setHydraulicFluid(document, {
            {"bulkModulus", 1400.0}, {"density", 860.0},
            {"kinematicViscosity", 46.0},
            {"supplyPressure", 21.0}, {"tankPressure", 0.1}
        });
    }

    void setScenarioField(const QString& key, const QJsonValue& value)
    {
        auto sc = document.entity("scenarios", 0);
        sc[key] = value;
        setCollection(document, "scenarios", {sc});
    }

    RigidResult solve()
    {
        QString error;
        require(validateNativeMotionDocument(document, error), "document invalid: " + error);
        NativeMotionInput input;
        require(prepareNativeMotion(cad, document, 0, input, error), "prepare failed: " + error);
        RigidResult result;
        const auto status = solveNativeMotion(input, result, error);
        require(status == SolveStatus::Completed, "solve failed: " + error);
        for (const auto& ch : result.channels) {
            require(ch.values.size() == result.frames.size() * std::size_t(ch.components), "channel frame mismatch");
            for (double v : ch.values) require(std::isfinite(v), "nonfinite channel value");
        }
        return result;
    }
};

// Stage 1: Model Check & Numerical Constraint Jacobian Rank Diagnostics
void testModelDiagnosticsAndConstraintRank()
{
    SliderFixture f(20.0);
    const auto clean = diagnoseNativeMotion(f.cad, f.document, 0);
    require(clean.errorCount == 0, "clean slider should have zero errors");
    require(clean.hasJacobianAnalysis, "clean slider missing Jacobian analysis");
    require(clean.velocityDofs == 6 && clean.constraintEquations == 5 && clean.jacobianRank == 5,
            QStringLiteral("unexpected translational Jacobian dimensions: n=%1 m=%2 r=%3")
                .arg(clean.velocityDofs).arg(clean.constraintEquations).arg(clean.jacobianRank));
    require(clean.effectiveDofs == 1 && clean.redundantEquations == 0, "clean slider should have 1 effective DOF and 0 redundant equations");
    require(clean.maxInitialViolation < 1e-10, "clean slider initial violation should be zero");

    // Add a duplicate parallel translational joint -> 5 redundant constraint equations
    auto joints = f.document.collection("joints");
    joints.append(QJsonObject{
        {"id", 1}, {"name", "duplicate_guide"}, {"jointType", "Translational"},
        {"actionBodyId", 0}, {"baseBodyId", -1},
        {"origin", xyz(0, 0, 50)}, {"axis", xyz(0, 0, 1)}
    });
    setCollection(f.document, "joints", joints);
    const auto redundant = diagnoseNativeMotion(f.cad, f.document, 0);
    require(redundant.hasJacobianAnalysis && redundant.constraintEquations == 10
            && redundant.jacobianRank == 5 && redundant.redundantEquations == 5
            && redundant.effectiveDofs == 1 && redundant.warningCount >= 1,
            "duplicate translational joint should report 5 redundant equations and 1 effective DOF");

    // Non-physical inertia violating triangle inequality (I1 + I2 < I3)
    SliderFixture badInertia(10.0);
    auto body = badInertia.document.entity("bodies", 0);
    body["inertia"] = QJsonArray{10.0, 10.0, 50.0, 0.0, 0.0, 0.0};
    setCollection(badInertia.document, "bodies", {body});
    const auto badReport = diagnoseNativeMotion(badInertia.cad, badInertia.document, 0);
    require(badReport.errorCount >= 1, "triangle-inequality inertia violation must report an error");
}

// Stage 2 Case 1: Constant volumetric flow -> steady piston speed v = Q_A / A_A & opposing load balance
void testConstantFlowSpeedAndForceDirection()
{
    SliderFixture f(40.0);
    f.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 40 kg * 9.81 = 392.4 N downward opposing load
    f.setScenarioField("hydraulicSteadyInit", true);
    f.setScenarioField("endTime", 0.30);
    f.setScenarioField("stepSize", 0.001);

    const double dp = 80.0, dr = 40.0;
    const double areaA = 0.25 * kPi * dp * dp; // 5026.548245743669 mm^2
    const double qLpm = 15.0;                  // 250,000 mm^3/s
    const double expectedSpeedMmS = (qLpm * 1.0e6 / 60.0) / areaA; // ~49.7359197 mm/s

    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "恒流驱动缸"}, {"enabled", true},
        {"mountMode", "Points"},
        {"bodyIId", 0}, {"bodyJId", -1},
        {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
        {"pistonDiameter", dp}, {"rodDiameter", dr},
        {"stroke", 400.0}, {"initialStroke", 100.0},
        {"deadVolumeA", 50000.0}, {"deadVolumeB", 50000.0},
        {"viscousFriction", 2.0}, {"internalLeakage", 0.0},
        {"supplyMode", "FlowCommand"},
        {"ratedFlowLpm", 40.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", QString::number(qLpm, 'g', 12)},
        {"initialPressureA", 1.0}, {"initialPressureB", 0.5}
    }});

    const auto res = f.solve();
    const auto& vel = channel(res, "hydraulicCylinders", 1, "Velocity").values;
    const auto& stroke = channel(res, "hydraulicCylinders", 1, "Stroke").values;
    const auto& force = channel(res, "hydraulicCylinders", 1, "Force").values;
    const auto& flowA = channel(res, "hydraulicCylinders", 1, "FlowA").values;

    expectNear(flowA.back(), qLpm, 1e-9, "FlowA command");
    expectNear(vel.back(), expectedSpeedMmS, 0.25, "Constant-flow steady speed Q_A / A_A");
    require(stroke.back() > stroke.front() + 12.0, "Cylinder must extend under +Q_A");
    expectNear(force.back(), 40.0 * 9.81, 5.0, "Steady hydraulic force balances gravity weight");
}

// Stage 2 Case 2: Closed-chamber compressible oil spring-mass free oscillation & energy conservation
void testClosedChamberFreeOscillationEnergy()
{
    const double massKg = 100.0;
    SliderFixture f(massKg);
    auto body = f.document.entity("bodies", 0);
    body["initialVelocity"] = xyz(0, 0, 60.0); // 60 mm/s initial upward velocity
    setCollection(f.document, "bodies", {body});

    // Use 2nd-order energy-conserving Trapezoidal integrator
    f.setScenarioField("usePreset", false);
    f.setScenarioField("integratorType", "TRAPEZOIDAL");
    f.setScenarioField("solverType", "SPARSE_QR");
    f.setScenarioField("endTime", 0.12);
    f.setScenarioField("stepSize", 0.0002);

    // Zero net force at t=0: P_A0 * A_A == P_B0 * A_B (with Dp=80, Dr=40 => A_B/A_A = 0.75 => P_A0=7.5, P_B0=10.0)
    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "封闭油弹簧缸"}, {"enabled", true},
        {"mountMode", "Joint"}, {"targetJointId", 0},
        {"pistonDiameter", 80.0}, {"rodDiameter", 40.0},
        {"stroke", 400.0}, {"initialStroke", 200.0},
        {"deadVolumeA", 80000.0}, {"deadVolumeB", 80000.0},
        {"viscousFriction", 0.0}, {"internalLeakage", 0.0},
        {"supplyMode", "ClosedPort"},
        {"ratedFlowLpm", 40.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", "0"},
        {"initialPressureA", 7.5}, {"initialPressureB", 10.0}
    }});

    const auto res = f.solve();
    const auto& vel = channel(res, "hydraulicCylinders", 1, "Velocity").values;
    const auto& ke = channel(res, "bodies", 0, "KineticEnergy").values;
    const double initialKeJ = 0.5 * massKg * 0.06 * 0.06; // 0.18 J
    expectNear(ke.front(), initialKeJ, 1e-6, "Initial kinetic energy");

    // Verify oscillation reverses velocity multiple times and peak KE at velocity extrema stays conserved
    int signChanges = 0;
    double peakKeAfterReversal = 0.0;
    for (std::size_t i = 1; i < vel.size(); ++i) {
        if ((vel[i - 1] > 0 && vel[i] <= 0) || (vel[i - 1] < 0 && vel[i] >= 0)) ++signChanges;
        if (signChanges >= 2) peakKeAfterReversal = std::max(peakKeAfterReversal, ke[i]);
    }
    require(signChanges >= 3, QStringLiteral("Closed-port oil spring did not oscillate (signChanges=%1)").arg(signChanges));
    expectNear(peakKeAfterReversal, initialKeJ, initialKeJ * 0.03, "Closed-port oil spring energy conservation");
}

// Stage 2 Case 3: Prescribed kinematic motion driving cylinder & measuring chamber pressures / load
void testPrescribedMotionLoadResponse()
{
    SliderFixture f(20.0);
    f.setScenarioField("endTime", 0.20);
    f.setScenarioField("stepSize", 0.001);

    setCollection(f.document, "drives", {QJsonObject{
        {"id", 0}, {"name", "正弦位移驱动"}, {"driveType", "JointDrive"},
        {"targetType", "Joint"}, {"targetJointId", 0},
        {"motionType", "Translation"}, {"controlMode", "Position"},
        {"expression", "15*sin(20*t)"}
    }});
    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "被动测力液压缸"}, {"enabled", true},
        {"mountMode", "Joint"}, {"targetJointId", 0},
        {"pistonDiameter", 60.0}, {"rodDiameter", 30.0},
        {"stroke", 300.0}, {"initialStroke", 150.0},
        {"deadVolumeA", 60000.0}, {"deadVolumeB", 60000.0},
        {"viscousFriction", 1.5}, {"internalLeakage", 0.02},
        {"supplyMode", "DirectionalValve"},
        {"ratedFlowLpm", 30.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", "0.2"},
        {"initialPressureA", 5.0}, {"initialPressureB", 5.0}
    }});

    const auto res = f.solve();
    const auto& stroke = channel(res, "hydraulicCylinders", 1, "Stroke").values;
    const auto& pa = channel(res, "hydraulicCylinders", 1, "PressureA").values;
    const auto& pb = channel(res, "hydraulicCylinders", 1, "PressureB").values;
    const auto& driveForce = channel(res, "drives", 0, "ActuationForce").values;

    for (std::size_t i = 0; i < res.frames.size(); ++i) {
        const double t = res.frames[i].seconds;
        expectNear(stroke[i], 150.0 + 15.0 * std::sin(20.0 * t), 1e-3, "Prescribed stroke tracking");
    }
    const auto [minPa, maxPa] = std::minmax_element(pa.begin(), pa.end());
    const auto [minPb, maxPb] = std::minmax_element(pb.begin(), pb.end());
    require(*maxPa - *minPa > 0.5 && *maxPb - *minPb > 0.5, "Chamber pressures must respond dynamically to prescribed motion");
    double maxDriveForce = 0.0;
    for (double v : driveForce) maxDriveForce = std::max(maxDriveForce, std::abs(v));
    require(maxDriveForce > 100.0, "Prescribed motion drive must sense hydraulic load reaction");
}

// Stage 2 Case 4: Steady-state initialization under heavy gravity load (zero initial drop) + valve lift
void testGravitySteadyInitAndValveLift()
{
    const double massKg = 200.0;
    SliderFixture f(massKg);
    f.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 1962 N downward weight
    f.setScenarioField("hydraulicSteadyInit", true);
    f.setScenarioField("endTime", 0.35);
    f.setScenarioField("stepSize", 0.001);

    // Hold valve closed (u=0) for t in [0, 0.12], then open proportional valve (u=0.6) to lift load
    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "举升液压缸"}, {"enabled", true},
        {"mountMode", "Points"},
        {"bodyIId", 0}, {"bodyJId", -1},
        {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
        {"pistonDiameter", 80.0}, {"rodDiameter", 45.0},
        {"stroke", 400.0}, {"initialStroke", 120.0},
        {"deadVolumeA", 50000.0}, {"deadVolumeB", 50000.0},
        {"viscousFriction", 2.0}, {"internalLeakage", 0.0},
        {"supplyMode", "DirectionalValve"},
        {"ratedFlowLpm", 40.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", "if(t - 0.12, 0.6, 0.0)"},
        {"initialPressureA", 1.0}, {"initialPressureB", 1.0}
    }});

    // Note: check Expr syntax supported by MotionFunction_Expr: use smooth step or piecewise table!
    // Using commandTableX / commandTableY tests table-driven proportional valve control cleanly too!
    auto cyl = f.document.entity("hydraulicCylinders", 1);
    cyl["commandExpression"] = "0";
    cyl["commandTableX"] = QJsonArray{0.0, 0.12, 0.14, 0.35};
    cyl["commandTableY"] = QJsonArray{0.0, 0.0, 0.6, 0.6};
    setCollection(f.document, "hydraulicCylinders", {cyl});

    const auto res = f.solve();
    require(std::any_of(res.diagnostics.begin(), res.diagnostics.end(), [](const QString& d) {
        return d.contains(QStringLiteral("液压稳态初始化已完成"));
    }), "Steady hydraulic init diagnostic missing");

    const auto& stroke = channel(res, "hydraulicCylinders", 1, "Stroke").values;
    const auto& force = channel(res, "hydraulicCylinders", 1, "Force").values;
    const auto& flowA = channel(res, "hydraulicCylinders", 1, "FlowA").values;
    const auto& power = channel(res, "hydraulicCylinders", 1, "Power").values;

    // 1. Exact force balance at t = 0
    expectNear(force.front(), massKg * 9.81, 1e-3, "t=0 steady-state hydraulic force == mg");

    // 2. Zero drop while valve is closed during t in [0, 0.12]
    for (std::size_t i = 0; i < res.frames.size(); ++i) {
        if (res.frames[i].seconds <= 0.12) {
            expectNear(stroke[i], 120.0, 1e-4, "Zero initial sag/drop during closed-valve hold");
        }
    }

    // 3. Smooth upward lift after valve opens (t > 0.14)
    require(stroke.back() > 130.0, QStringLiteral("200 kg load did not lift after valve opened: final stroke=%1").arg(stroke.back()));
    require(flowA.back() > 1.0 && power.back() > 50.0, "Positive port A flow and positive lifting mechanical power expected");
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    try {
        testModelDiagnosticsAndConstraintRank();
        testConstantFlowSpeedAndForceDirection();
        testClosedChamberFreeOscillationEnergy();
        testPrescribedMotionLoadResponse();
        testGravitySteadyInitAndValveLift();
        qInfo().noquote() << "PASS MotionOccHydraulicSmoke (Stage 1 diagnostics + Stage 2 4-case hydromechanical coupling)";
        return 0;
    } catch (const std::exception& e) {
        qCritical().noquote() << "FAIL MotionOccHydraulicSmoke:" << e.what();
        return 1;
    }
}
