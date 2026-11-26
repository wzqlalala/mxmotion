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
    f.setScenarioField("endTime", 0.35);
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

// Stage 3 Case 1: Full Hydraulic Power Unit (Pump + ReliefValve + Accumulator + Filter + ServoValve) -> Cylinder
void testMultiComponentHydraulicCircuitAndAccumulator()
{
    SliderFixture f(120.0);
    f.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 1177.2 N downward gravity load
    f.setScenarioField("hydraulicSteadyInit", true);
    f.setScenarioField("endTime", 0.35);
    f.setScenarioField("stepSize", 0.001);

    setCollection(f.document, "hydraulicNodes", {
        QJsonObject{{"id", 1}, {"name", "主油箱"}, {"enabled", true}, {"nodeType", "Tank"}, {"pressureMPa", 0.1}},
        QJsonObject{{"id", 2}, {"name", "泵出口母管"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 150.0}, {"initialPressureMPa", 18.0}},
        QJsonObject{{"id", 3}, {"name", "阀前滤后腔"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 60.0}, {"initialPressureMPa", 18.0}},
        QJsonObject{{"id", 4}, {"name", "无杆腔油口节点"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 40.0}, {"initialPressureMPa", 2.0}},
        QJsonObject{{"id", 5}, {"name", "有杆腔油口节点"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 40.0}, {"initialPressureMPa", 0.5}}
    });

    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "伺服回路举升缸"}, {"enabled", true},
        {"mountMode", "Points"},
        {"bodyIId", 0}, {"bodyJId", -1},
        {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
        {"pistonDiameter", 80.0}, {"rodDiameter", 45.0},
        {"stroke", 400.0}, {"initialStroke", 100.0},
        {"deadVolumeA", 50000.0}, {"deadVolumeB", 50000.0},
        {"viscousFriction", 2.0}, {"internalLeakage", 0.0},
        {"supplyMode", "ExternalCircuit"},
        {"portANodeId", -1}, {"portBNodeId", 5},
        {"ratedFlowLpm", 40.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", "0"},
        {"initialPressureA", 1.0}, {"initialPressureB", 0.5}
    }});

    setCollection(f.document, "hydraulicComponents", {
        QJsonObject{
            {"id", 1}, {"name", "定量液压泵"}, {"enabled", true}, {"componentType", "Pump"},
            {"suctionNodeId", 1}, {"deliveryNodeId", 2},
            {"displacementCcRev", 22.0}, {"speedExpression", "1500"},
            {"volumetricEfficiency", 0.95}, {"mechanicalEfficiency", 0.90}, {"ratedPressureMPa", 21.0}
        },
        QJsonObject{
            {"id", 2}, {"name", "安全溢流阀"}, {"enabled", true}, {"componentType", "ReliefValve"},
            {"inletNodeId", 2}, {"outletNodeId", 1},
            {"crackingPressureMPa", 20.0}, {"pressureOverrideMPa", 1.0}, {"ratedFlowLpm", 60.0}
        },
        QJsonObject{
            {"id", 3}, {"name", "稳压蓄能器"}, {"enabled", true}, {"componentType", "Accumulator"},
            {"targetNodeId", 2},
            {"prechargePressureMPa", 12.0}, {"gasVolumeL", 0.6}, {"polytropicGamma", 1.3}
        },
        QJsonObject{
            {"id", 4}, {"name", "高压过滤器"}, {"enabled", true}, {"componentType", "Filter"},
            {"inletNodeId", 2}, {"outletNodeId", 3},
            {"ratedFlowLpm", 80.0}, {"cleanPressureDropMPa", 0.15}, {"foulingFactor", 1.1}
        },
        QJsonObject{
            {"id", 5}, {"name", "电液伺服阀"}, {"enabled", true}, {"componentType", "ServoValve"},
            {"portPNodeId", 3}, {"portTNodeId", 1}, {"portANodeId", 4}, {"portBNodeId", 5},
            {"ratedFlowLpm", 45.0}, {"ratedPressureDropMPa", 3.5},
            {"centerMode", "ClosedCenter"}, {"responseTimeMs", 20.0}, {"underlap", 0.0},
            {"commandExpression", "0"},
            {"commandTableX", QJsonArray{0.0, 0.12, 0.125, 0.35}},
            {"commandTableY", QJsonArray{0.0, 0.0, 0.75, 0.75}}
        },
        QJsonObject{
            {"id", 6}, {"name", "A口端口连接"}, {"enabled", true}, {"componentType", "PortConnection"},
            {"targetNodeId", 4}, {"actuatorType", "Cylinder"}, {"actuatorId", 1},
            {"actuatorPort", "A"}, {"conductanceLpm", 240.0}
        }
    });

    const auto diag = diagnoseNativeMotion(f.cad, f.document, 0);
    require(diag.errorCount == 0 && diag.warningCount == 0,
            QStringLiteral("Clean hydraulic circuit had unexpected issues: err=%1 warn=%2")
                .arg(diag.errorCount).arg(diag.warningCount));
    require(diag.hydraulicActuators == 1 && diag.hydraulicNodes == 5 && diag.hydraulicComponents == 6,
            "Unexpected hydraulic circuit entity counts in diagnostics");

    const auto res = f.solve();
    const auto& pSupply = channel(res, "hydraulicNodes", 2, "Pressure").values;
    const auto& reliefFlow = channel(res, "hydraulicComponents", 2, "Flow").values;
    const auto& accOil = channel(res, "hydraulicComponents", 3, "OilVolume").values;
    const auto& servoSpool = channel(res, "hydraulicComponents", 5, "SpoolPosition").values;
    const auto& stroke = channel(res, "hydraulicCylinders", 1, "Stroke").values;

    // During t in [0, 0.12s] (closed servo valve), the 31.35 L/min pump charges the 0.6 L accumulator
    // past 20 MPa so ReliefValve regulates manifold pressure in 20..21 MPa and bypasses excess flow;
    // once ServoValve opens wide (t > 0.125s), Accumulator discharges stored oil to supplement pump flow!
    require(pSupply[120] > 20.0 && pSupply[120] < 21.2,
            QStringLiteral("Pump/relief manifold pressure at t=0.12s out of regulated 20..21.2 MPa band: %1 MPa").arg(pSupply[120]));
    double maxReliefFlow = *std::max_element(reliefFlow.begin(), reliefFlow.end());
    require(maxReliefFlow > 10.0, QStringLiteral("Relief valve should bypass excess pump flow when servo valve is closed: %1 L/min").arg(maxReliefFlow));
    require(accOil[120] > 0.19 && accOil.back() < accOil[120] && pSupply.back() > 12.0,
            QStringLiteral("Accumulator charge/discharge peak-shaving unexpected: oil@0.12s=%1 L, oil@end=%2 L, P@end=%3 MPa")
                .arg(accOil[120]).arg(accOil.back()).arg(pSupply.back()));

    // First-order servo spool lag at t = 0.135 s (10 ms after command step to 0.75 with tau = 20 ms)
    require(servoSpool[135] > 0.15 && servoSpool[135] < 0.65,
            QStringLiteral("Servo valve 1st-order spool lag missing at t=0.135s: %1").arg(servoSpool[135]));
    expectNear(servoSpool.back(), 0.75, 0.02, "Servo spool settles to 0.75 command");
    require(stroke.back() > 115.0, QStringLiteral("Cylinder did not extend under circuit supply: %1 mm").arg(stroke.back()));
}

// Stage 3 Case 2: Rotary Hydraulic Motor + PressureReducingValve + Pipe + CheckValve driving rotary load
void testRotaryHydraulicMotorAndReducingPipeBranch()
{
    CadDocument cad;
    NativeMotionDocument doc;
    QString error;
    int part = 0;
    require(cad.addPrimitive(Primitive::Cylinder, 50, 40, 40, part, error), error);
    doc.root = {{"formatVersion", 1}, {"motionData", QJsonObject{}}};
    doc.bodyPartBindings.insert(0, {part});
    setCollection(doc, "bodies", {QJsonObject{
        {"id", 0}, {"name", "rotor"}, {"geoEntityIds", QJsonArray{part}},
        {"mass", 15.0}, {"isCustomMassProps", true},
        {"centerOfMass", xyz(0, 0, 0)},
        {"inertia", QJsonArray{2.0e5, 2.0e5, 4.0e5, 0, 0, 0}}, // Izz = 0.4 kg*m^2
        {"initialVelocity", xyz(0, 0, 0)}, {"initialAngularVelocity", xyz(0, 0, 0)}
    }});
    setCollection(doc, "joints", {QJsonObject{
        {"id", 0}, {"name", "main_shaft"}, {"jointType", "Revolute"},
        {"actionBodyId", 0}, {"baseBodyId", -1},
        {"origin", xyz(0, 0, 0)}, {"axis", xyz(0, 0, 1)}
    }});
    // Opposing load torque of -50 N*m = -50,000 N*mm
    setCollection(doc, "vectorForces", {QJsonObject{
        {"id", 1}, {"name", "load_torque"}, {"enabled", true},
        {"loadType", "Torque"}, {"bodyId", 0},
        {"actionPoint", xyz(0, 0, 0)}, {"direction", xyz(0, 0, 1)},
        {"directionMode", "GlobalFixed"}, {"magnitudeMode", "Constant"},
        {"magnitude", -50000.0}
    }});
    setCollection(doc, "scenarios", {QJsonObject{
        {"id", 0}, {"name", "motor_case"},
        {"endTime", 0.30}, {"stepSize", 0.001},
        {"gravity", xyz(0, 0, 0)},
        {"usePreset", true}, {"presetMode", "ROBUST"},
        {"analysisType", "Dynamic"},
        {"hydraulicSteadyInit", true}
    }});
    setHydraulicFluid(doc, {
        {"bulkModulus", 1400.0}, {"density", 860.0},
        {"kinematicViscosity", 46.0}, {"supplyPressure", 25.0}, {"tankPressure", 0.1}
    });

    setCollection(doc, "hydraulicNodes", {
        QJsonObject{{"id", 1}, {"name", "高压恒压源"}, {"enabled", true}, {"nodeType", "PressureSource"}, {"pressureMPa", 25.0}, {"pressureExpression", "25"}},
        QJsonObject{{"id", 2}, {"name", "回油箱"}, {"enabled", true}, {"nodeType", "Tank"}, {"pressureMPa", 0.1}},
        QJsonObject{{"id", 3}, {"name", "减压后腔"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 80.0}, {"initialPressureMPa", 12.0}},
        QJsonObject{{"id", 4}, {"name", "管路末端腔"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 60.0}, {"initialPressureMPa", 12.0}},
        QJsonObject{{"id", 5}, {"name", "马达A腔连接点"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 30.0}, {"initialPressureMPa", 5.0}},
        QJsonObject{{"id", 6}, {"name", "马达B腔连接点"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 30.0}, {"initialPressureMPa", 0.5}}
    });

    // Displacement = 62.831853 mL/r => d_m = 10,000 mm^3/rad; at 12 MPa drop, max torque ~ 0.95 * 10000 * 11.9 ~ 113,000 N*mm > 50,000 N*mm!
    setCollection(doc, "hydraulicMotors", {QJsonObject{
        {"id", 1}, {"name", "回转液压马达"}, {"enabled", true},
        {"targetJointId", 0},
        {"displacementCcRev", 62.83185307179586},
        {"mechanicalEfficiency", 0.95},
        {"chamberVolumeA", 40000.0}, {"chamberVolumeB", 40000.0},
        {"viscousDamping", 50.0}, {"internalLeakage", 0.01},
        {"supplyMode", "ExternalCircuit"},
        {"portANodeId", 5}, {"portBNodeId", 6},
        {"ratedFlowLpm", 50.0}, {"ratedPressureDrop", 3.5},
        {"commandExpression", "0"},
        {"initialPressureA", 1.0}, {"initialPressureB", 0.5}
    }});

    setCollection(doc, "hydraulicComponents", {
        QJsonObject{
            {"id", 1}, {"name", "定值减压阀"}, {"enabled", true}, {"componentType", "PressureReducingValve"},
            {"inletNodeId", 1}, {"outletNodeId", 3},
            {"regulatedPressureMPa", 12.0}, {"pressureBandMPa", 0.6},
            {"ratedFlowLpm", 60.0}, {"ratedPressureDropMPa", 1.0}
        },
        QJsonObject{
            {"id", 2}, {"name", "输油管路"}, {"enabled", true}, {"componentType", "Pipe"},
            {"inletNodeId", 3}, {"outletNodeId", 4},
            {"lengthMm", 1500.0}, {"diameterMm", 14.0}, {"localLossCoeff", 0.5}
        },
        QJsonObject{
            {"id", 3}, {"name", "四通比例换向阀"}, {"enabled", true}, {"componentType", "DirectionalValve"},
            {"portPNodeId", 4}, {"portTNodeId", 2}, {"portANodeId", 5}, {"portBNodeId", 6},
            {"ratedFlowLpm", 50.0}, {"ratedPressureDropMPa", 3.0},
            {"centerMode", "ClosedCenter"}, {"responseTimeMs", 0.0}, {"underlap", 0.0},
            {"commandExpression", "0.85"}
        }
    });

    require(validateNativeMotionDocument(doc, error), "motor doc invalid: " + error);
    NativeMotionInput input;
    require(prepareNativeMotion(cad, doc, 0, input, error), "motor prepare failed: " + error);
    RigidResult res;
    require(solveNativeMotion(input, res, error) == SolveStatus::Completed, "motor solve failed: " + error);

    const auto& pReduced = channel(res, "hydraulicNodes", 3, "Pressure").values;
    const auto& mTorque = channel(res, "hydraulicMotors", 1, "Torque").values;
    const auto& mSpeed = channel(res, "hydraulicMotors", 1, "Speed").values;
    const auto& mAngle = channel(res, "hydraulicMotors", 1, "Angle").values;

    // Steady init balances 50,000 N*mm opposing load at t=0
    expectNear(mTorque.front(), 50000.0, 50.0, "Motor t=0 steady-state load torque balance");
    // Reducing valve keeps downstream below ~12.5 MPa despite 25 MPa upstream source
    for (double p : pReduced) {
        require(p < 13.0, QStringLiteral("Pressure reducing valve failed to cap pressure: %1 MPa").arg(p));
    }
    require(mSpeed.back() > 80.0 && mAngle.back() > 35.0,
            QStringLiteral("Hydraulic motor did not accelerate rotor: speed=%1 rpm, angle=%2 deg")
                .arg(mSpeed.back()).arg(mAngle.back()));
}

// Stage 3 Case 3: Circuit Check Diagnostics (unprotected pump, isolated node, unconnected external ports)
void testHydraulicCircuitDiagnostics()
{
    SliderFixture f(20.0);
    setCollection(f.document, "hydraulicNodes", {
        QJsonObject{{"id", 1}, {"name", "油箱"}, {"enabled", true}, {"nodeType", "Tank"}, {"pressureMPa", 0.1}},
        QJsonObject{{"id", 2}, {"name", "无溢流保护腔"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 100.0}, {"initialPressureMPa", 1.0}},
        QJsonObject{{"id", 3}, {"name", "孤立腔"}, {"enabled", true}, {"nodeType", "Chamber"}, {"volumeMl", 50.0}, {"initialPressureMPa", 1.0}}
    });
    setCollection(f.document, "hydraulicComponents", {QJsonObject{
        {"id", 1}, {"name", "裸泵"}, {"enabled", true}, {"componentType", "Pump"},
        {"suctionNodeId", 1}, {"deliveryNodeId", 2},
        {"displacementCcRev", 25.0}, {"speedExpression", "1500"},
        {"volumetricEfficiency", 0.95}, {"mechanicalEfficiency", 0.90}, {"ratedPressureMPa", 21.0}
    }});
    setCollection(f.document, "hydraulicCylinders", {QJsonObject{
        {"id", 1}, {"name", "悬空外接缸"}, {"enabled", true},
        {"mountMode", "Joint"}, {"targetJointId", 0},
        {"pistonDiameter", 60.0}, {"rodDiameter", 30.0}, {"stroke", 200.0}, {"initialStroke", 100.0},
        {"supplyMode", "ExternalCircuit"}, {"portANodeId", -1}, {"portBNodeId", -1},
        {"commandExpression", "0"}
    }});

    const auto report = diagnoseNativeMotion(f.cad, f.document, 0);
    require(report.errorCount >= 1, "Unconnected ExternalCircuit cylinder must report an Error");
    require(report.warningCount >= 2, "Unprotected pump and isolated node must report Warnings");
}

// Track A: Newly wired MotionEngine capabilities (3D Helical Gear axial thrust, Nonlinear Spring Table, Viscous Joint Friction)
void testNewlyWiredEngineCapabilities()
{
    // 1. 3D Helical Gear (helixAngleDeg = 25 deg, pressureAngleDeg = 20 deg) generates real axial Z reaction thrust
    auto runGear = [](double helixDeg) {
        CadDocument cad;
        NativeMotionDocument doc;
        QString error;
        int p1 = 0, p2 = 0;
        require(cad.addPrimitive(Primitive::Cylinder, 50, 20, 20, p1, error), error);
        require(cad.addPrimitive(Primitive::Cylinder, 50, 20, 20, p2, error), error);
        doc.root = {{"formatVersion", 1}, {"motionData", QJsonObject{}}};
        doc.bodyPartBindings.insert(1, {p1});
        doc.bodyPartBindings.insert(2, {p2});
        setCollection(doc, "bodies", {
            QJsonObject{{"id", 1}, {"name", "gear1"}, {"geoEntityIds", QJsonArray{p1}},
                        {"mass", 5.0}, {"isCustomMassProps", true}, {"centerOfMass", xyz(0, 0, 0)},
                        {"inertia", QJsonArray{5000.0, 5000.0, 8000.0, 0, 0, 0}},
                        {"initialVelocity", xyz(0,0,0)}, {"initialAngularVelocity", xyz(0,0,0)}},
            QJsonObject{{"id", 2}, {"name", "gear2"}, {"geoEntityIds", QJsonArray{p2}},
                        {"mass", 5.0}, {"isCustomMassProps", true}, {"centerOfMass", xyz(100, 0, 0)},
                        {"inertia", QJsonArray{5000.0, 5000.0, 8000.0, 0, 0, 0}},
                        {"initialVelocity", xyz(0,0,0)}, {"initialAngularVelocity", xyz(0,0,0)}}
        });
        setCollection(doc, "joints", {
            QJsonObject{{"id", 1}, {"name", "rev1"}, {"jointType", "Revolute"}, {"actionBodyId", 1}, {"baseBodyId", -1},
                        {"origin", xyz(0, 0, 0)}, {"axis", xyz(0, 0, 1)},
                        {"friction", QJsonObject{{"enabled", true}, {"staticCoefficient", 0.0}, {"kineticCoefficient", 0.0}, {"viscousCoefficient", 200.0}, {"effectiveRadius", 10.0}}}},
            QJsonObject{{"id", 2}, {"name", "rev2"}, {"jointType", "Revolute"}, {"actionBodyId", 2}, {"baseBodyId", -1},
                        {"origin", xyz(100, 0, 0)}, {"axis", xyz(0, 0, 1)}}
        });
        setCollection(doc, "couplings", {QJsonObject{
            {"id", 1}, {"name", "helical_gear"}, {"couplingType", "Gear"},
            {"firstJointId", 1}, {"secondJointId", 2},
            {"gearRatio", 1.0}, {"pitchRadius1", 50.0}, {"pitchRadius2", 50.0}, {"toothCount1", 25}, {"toothCount2", 25},
            {"pressureAngleDeg", 20.0}, {"helixAngleDeg", helixDeg}, {"phaseDeg", 0.0}, {"checkPhase", true}
        }});
        setCollection(doc, "drives", {QJsonObject{
            {"id", 1}, {"name", "speed_drive"}, {"driveType", "JointDrive"}, {"targetType", "Joint"},
            {"targetJointId", 1}, {"motionType", "Rotation"}, {"controlMode", "Velocity"}, {"expression", "180"}
        }});
        setCollection(doc, "vectorForces", {QJsonObject{
            {"id", 1}, {"name", "load_trq"}, {"enabled", true}, {"loadType", "Torque"}, {"bodyId", 2},
            {"actionPoint", xyz(100, 0, 0)}, {"direction", xyz(0, 0, 1)}, {"directionMode", "GlobalFixed"},
            {"magnitudeMode", "Constant"}, {"magnitude", -20000.0}
        }});
        setCollection(doc, "scenarios", {QJsonObject{
            {"id", 0}, {"name", "gear_case"}, {"endTime", 0.05}, {"stepSize", 0.001},
            {"gravity", xyz(0, 0, 0)}, {"usePreset", true}, {"presetMode", "ROBUST"}, {"analysisType", "Dynamic"}
        }});
        require(validateNativeMotionDocument(doc, error), "gear doc invalid: " + error);
        NativeMotionInput input;
        require(prepareNativeMotion(cad, doc, 0, input, error), "gear prepare failed: " + error);
        RigidResult res;
        require(solveNativeMotion(input, res, error) == SolveStatus::Completed, "gear solve failed: " + error);
        // ReactionForce on rev2: 3 components [Fx, Fy, Fz] at final frame
        const auto& rf = channel(res, "joints", 2, "ReactionForce").values;
        return std::abs(rf.back()); // Fz component of last frame
    };

    const double spurFz = runGear(0.0);
    const double helicalFz = runGear(25.0);
    expectNear(spurFz, 0.0, 1e-4, "Spur gear (beta=0) should produce zero axial thrust");
    require(helicalFz > 50.0, QStringLiteral("Helical gear (beta=25 deg) must generate significant axial Z thrust: Fz=%1 N").arg(helicalFz));

    // 2. Nonlinear Spring Lookup Table (tableX, tableY)
    SliderFixture sf(10.0);
    sf.setScenarioField("analysisType", "Static");
    sf.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 98.1 N downward weight
    setCollection(sf.document, "springs", {QJsonObject{
        {"id", 1}, {"name", "nonlinear_table_spring"}, {"enabled", true},
        {"springType", "BodyPoint"},
        {"bodyIId", 0}, {"bodyJId", -1},
        {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
        {"stiffness", 0.0}, {"damping", 1.0}, {"dampingEnabled", true},
        {"freeValue", 200.0}, {"installValue", 200.0}, {"preload", 0.0},
        {"springDirectionMode", "Bidirectional"}, {"dampingDirectionMode", "Bidirectional"},
        {"tableX", QJsonArray{-50.0, -10.0, 0.0, 50.0}},
        {"tableY", QJsonArray{981.0, 98.1, 0.0, -981.0}}
    }});
    const auto sRes = sf.solve();
    const auto& zPos = channel(sRes, "bodies", 0, "Position").values;
    // At compression x = -10 mm (length 190 mm), tableY = +98.1 N restoring upward force balancing 98.1 N weight!
    expectNear(zPos.back(), 190.0, 0.05, "Nonlinear spring lookup table static equilibrium position");
}

// Stage 4 Case 1: Full Electrical Drive Chain (DC PowerSupply + Bus Capacitor + PWM Servo Inverter -> PMSM ElectricMotor)
void testElectricDriveBusInverterAndServoMotor()
{
    CadDocument cad;
    NativeMotionDocument doc;
    QString error;
    int part = 0;
    require(cad.addPrimitive(Primitive::Cylinder, 40, 20, 20, part, error), error);
    doc.root = {{"formatVersion", 1}, {"motionData", QJsonObject{}}};
    doc.bodyPartBindings.insert(0, {part});
    setCollection(doc, "bodies", {QJsonObject{
        {"id", 0}, {"name", "servo_rotor"}, {"geoEntityIds", QJsonArray{part}},
        {"mass", 8.0}, {"isCustomMassProps", true}, {"centerOfMass", xyz(0, 0, 0)},
        {"inertia", QJsonArray{5.0e4, 5.0e4, 8.0e4, 0, 0, 0}}, // Izz = 0.08 kg*m^2
        {"initialVelocity", xyz(0, 0, 0)}, {"initialAngularVelocity", xyz(0, 0, 0)}
    }});
    setCollection(doc, "joints", {QJsonObject{
        {"id", 0}, {"name", "motor_shaft"}, {"jointType", "Revolute"},
        {"actionBodyId", 0}, {"baseBodyId", -1},
        {"origin", xyz(0, 0, 0)}, {"axis", xyz(0, 0, 1)}
    }});
    // Opposing load torque of -12 N*m = -12,000 N*mm
    setCollection(doc, "vectorForces", {QJsonObject{
        {"id", 1}, {"name", "shaft_load"}, {"enabled", true},
        {"loadType", "Torque"}, {"bodyId", 0},
        {"actionPoint", xyz(0, 0, 0)}, {"direction", xyz(0, 0, 1)},
        {"directionMode", "GlobalFixed"}, {"magnitudeMode", "Constant"},
        {"magnitude", -12000.0}
    }});
    setCollection(doc, "scenarios", {QJsonObject{
        {"id", 0}, {"name", "electric_drive_case"},
        {"endTime", 0.25}, {"stepSize", 0.001},
        {"gravity", xyz(0, 0, 0)}, {"usePreset", true}, {"presetMode", "ROBUST"},
        {"analysisType", "Dynamic"}, {"hydraulicSteadyInit", true}
    }});

    setCollection(doc, "powerSupplies", {QJsonObject{
        {"id", 1}, {"name", "高压直流电源"}, {"enabled", true},
        {"nominalVoltage", 120.0}, {"voltageExpression", "120"},
        {"internalResistance", 0.15}, {"capacitanceMf", 2.2},
        {"maxCurrent", 100.0}, {"chopperVoltage", 145.0}, {"chopperResistance", 6.0}
    }});
    setCollection(doc, "inverters", {QJsonObject{
        {"id", 1}, {"name", "伺服逆变器"}, {"enabled", true},
        {"supplyId", 1}, {"nominalBusVoltage", 120.0},
        {"controlMode", "SpeedClosedLoop"},
        {"efficiency", 0.96}, {"maxCurrent", 60.0},
        {"kpPosition", 8.0}, {"kpSpeed", 0.45}, {"kiSpeed", 6.0}, {"kpCurrent", 6.0},
        {"commandExpression", "300"} // Target 300 rpm
    }});
    setCollection(doc, "electricMotors", {QJsonObject{
        {"id", 1}, {"name", "永磁同步电机"}, {"enabled", true},
        {"targetJointId", 0},
        {"resistance", 0.35}, {"inductanceMh", 2.0},
        {"torqueConstant", 0.90}, {"backEmfConstant", 0.90},
        {"gearRatio", 2.0}, {"gearEfficiency", 0.95}, {"viscousDamping", 5.0},
        {"busVoltage", 120.0}, {"maxCurrent", 60.0},
        {"controlMode", "ExternalInverter"}, {"inverterId", 1},
        {"commandExpression", "0"}
    }});

    const auto diag = diagnoseNativeMotion(cad, doc, 0);
    require(diag.errorCount == 0 && diag.warningCount == 0, "Clean electric drive chain reported unexpected diagnostic issues");
    require(diag.electricActuators == 1 && diag.electricPowerDevices == 2, "Unexpected electric device counts in diagnostics");

    require(validateNativeMotionDocument(doc, error), "electric drive doc invalid: " + error);
    NativeMotionInput input;
    require(prepareNativeMotion(cad, doc, 0, input, error), "electric drive prepare failed: " + error);
    RigidResult res;
    require(solveNativeMotion(input, res, error) == SolveStatus::Completed, "electric drive solve failed: " + error);

    const auto& speed = channel(res, "electricMotors", 1, "Speed").values;
    const auto& torque = channel(res, "electricMotors", 1, "Torque").values;
    const auto& busV = channel(res, "powerSupplies", 1, "BusVoltage").values;
    const auto& srcI = channel(res, "powerSupplies", 1, "SourceCurrent").values;

    // t=0 steady load torque balance against 12,000 N*mm
    expectNear(torque.front(), 12000.0, 25.0, "Electric motor t=0 steady load torque balance");
    // Closed-loop speed settles near 300 rpm
    expectNear(speed.back(), 300.0, 8.0, "Closed-loop inverter+motor speed tracking to 300 rpm");
    // Bus voltage sags slightly below 120 V under load current draw I * R_s
    const double minBusV = *std::min_element(busV.begin(), busV.end());
    require(minBusV < 119.8 && minBusV > 105.0 && *std::max_element(srcI.begin(), srcI.end()) > 2.0,
            QStringLiteral("Expected realistic DC bus voltage droop and positive supply current: minBusV=%1 V").arg(minBusV));
}

// Stage 4 Case 2: Integrated EHA and EMA Position Servo Actuators under heavy gravity load
void testIntegratedEhaAndEmaServoActuators()
{
    // 1. EHA Position Servo: hold at s=150 mm (t in [0, 0.05]), then step target stroke to 175 mm
    {
        SliderFixture f(80.0);
        f.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 784.8 N downward load
        f.setScenarioField("hydraulicSteadyInit", true);
        f.setScenarioField("endTime", 0.50);
        f.setScenarioField("stepSize", 0.001);

        setCollection(f.document, "ehaActuators", {QJsonObject{
            {"id", 1}, {"name", "机翼主飞控EHA"}, {"enabled", true},
            {"mountMode", "Points"}, {"bodyIId", 0}, {"bodyJId", -1},
            {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
            {"pistonDiameter", 70.0}, {"rodDiameter", 40.0},
            {"stroke", 300.0}, {"initialStroke", 150.0}, {"deadVolume", 40000.0},
            {"pumpDisplacementCcRev", 12.0}, {"boostPressureMPa", 1.5}, {"maxReliefPressureMPa", 28.0},
            {"internalLeakage", 0.0}, {"pumpEfficiency", 0.94},
            {"resistance", 0.30}, {"torqueConstant", 0.65}, {"backEmfConstant", 0.50},
            {"rotorInertia", 400.0}, {"busVoltage", 270.0}, {"maxCurrent", 65.0},
            {"viscousFriction", 4.0},
            {"controlMode", "PositionServo"}, {"inverterId", -1},
            {"kpPosition", 160.0}, {"kpSpeed", 0.35}, {"kiSpeed", 4.0}, {"kpCurrent", 5.0},
            {"commandExpression", "150"},
            {"commandTableX", QJsonArray{0.0, 0.05, 0.08, 0.50}},
            {"commandTableY", QJsonArray{150.0, 150.0, 175.0, 175.0}}
        }});

        const auto res = f.solve();
        const auto& stroke = channel(res, "ehaActuators", 1, "Stroke").values;
        const auto& force = channel(res, "ehaActuators", 1, "Force").values;
        const auto& pumpRpm = channel(res, "ehaActuators", 1, "PumpSpeed").values;

        expectNear(force.front(), 80.0 * 9.81, 1e-2, "EHA t=0 steady force balance");
        expectNear(stroke[45], 150.0, 0.08, "EHA holds initial stroke without sag under 80 kg load");
        require(*std::max_element(pumpRpm.begin(), pumpRpm.end()) > 300.0, "EHA micro-pump must spin up to pump oil into cap chamber");
        expectNear(stroke.back(), 175.0, 1.5, "EHA closed-loop stroke tracking to 175 mm");
    }

    // 2. EMA Position Servo: ball-screw actuator lifting 60 kg load from 120 mm to 145 mm
    {
        SliderFixture f(60.0);
        f.setScenarioField("gravity", xyz(0, 0, -9810.0)); // 588.6 N downward load
        f.setScenarioField("hydraulicSteadyInit", true);
        f.setScenarioField("endTime", 0.30);
        f.setScenarioField("stepSize", 0.001);

        setCollection(f.document, "emaActuators", {QJsonObject{
            {"id", 1}, {"name", "起落架舱门EMA"}, {"enabled", true},
            {"mountMode", "Points"}, {"bodyIId", 0}, {"bodyJId", -1},
            {"localI", xyz(0, 0, 200)}, {"localJ", xyz(0, 0, 0)},
            {"screwLead", 10.0}, {"gearRatio", 2.5},
            {"stroke", 300.0}, {"initialStroke", 120.0},
            {"mechanicalEfficiency", 0.90}, {"viscousFriction", 3.0},
            {"resistance", 0.35}, {"inductanceMh", 2.0},
            {"torqueConstant", 0.70}, {"backEmfConstant", 0.70},
            {"busVoltage", 270.0}, {"maxCurrent", 50.0},
            {"controlMode", "PositionServo"}, {"inverterId", -1},
            {"kpPosition", 18.0}, {"kpSpeed", 0.50}, {"kiSpeed", 4.0}, {"kpCurrent", 6.0},
            {"commandExpression", "120"},
            {"commandTableX", QJsonArray{0.0, 0.04, 0.07, 0.30}},
            {"commandTableY", QJsonArray{120.0, 120.0, 145.0, 145.0}}
        }});

        const auto res = f.solve();
        const auto& stroke = channel(res, "emaActuators", 1, "Stroke").values;
        const auto& force = channel(res, "emaActuators", 1, "Force").values;
        const auto& motRpm = channel(res, "emaActuators", 1, "MotorSpeed").values;

        expectNear(force.front(), 60.0 * 9.81, 0.5, "EMA t=0 steady holding thrust balance");
        expectNear(stroke[35], 120.0, 0.08, "EMA holds initial stroke without sag");
        require(*std::max_element(motRpm.begin(), motRpm.end()) > 200.0, "EMA motor shaft spins up to drive ball screw");
        expectNear(stroke.back(), 145.0, 1.0, "EMA closed-loop stroke tracking to 145 mm");
    }

    // 3. Stage 4 Electrical & Integrated Diagnostics check
    {
        SliderFixture f(20.0);
        setCollection(f.document, "powerSupplies", {QJsonObject{
            {"id", 1}, {"name", "异常斩波电源"}, {"enabled", true},
            {"nominalVoltage", 270.0}, {"internalResistance", 0.05}, {"capacitanceMf", 4.7},
            {"maxCurrent", 100.0}, {"chopperVoltage", 250.0}, {"chopperResistance", 8.0} // chopper <= nominal -> Warning!
        }});
        setCollection(f.document, "emaActuators", {QJsonObject{
            {"id", 1}, {"name", "悬空外接EMA"}, {"enabled", true},
            {"mountMode", "Joint"}, {"targetJointId", 0},
            {"screwLead", 10.0}, {"gearRatio", 2.0}, {"stroke", 200.0}, {"initialStroke", 100.0},
            {"resistance", 0.4}, {"torqueConstant", 0.6}, {"backEmfConstant", 0.6},
            {"controlMode", "ExternalInverter"}, {"inverterId", -1} // Unbound ExternalInverter -> Error!
        }});
        const auto rep = diagnoseNativeMotion(f.cad, f.document, 0);
        require(rep.errorCount >= 1 && rep.warningCount >= 2,
                "Expected unbound ExternalInverter Error and chopper/unused-supply Warnings");
    }
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
        testMultiComponentHydraulicCircuitAndAccumulator();
        testRotaryHydraulicMotorAndReducingPipeBranch();
        testHydraulicCircuitDiagnostics();
        testNewlyWiredEngineCapabilities();
        testElectricDriveBusInverterAndServoMotor();
        testIntegratedEhaAndEmaServoActuators();
        qInfo().noquote() << "PASS MotionOccHydraulicSmoke (Stage 1-4 multiphysics: hydraulics + electric drives + EHA/EMA + engine extensions)";
        return 0;
    } catch (const std::exception& e) {
        qCritical().noquote() << "FAIL MotionOccHydraulicSmoke:" << e.what();
        return 1;
    }
}
