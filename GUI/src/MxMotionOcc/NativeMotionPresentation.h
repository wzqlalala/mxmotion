#pragma once

#include "NativeMotionDocument.h"
#include "RigidMotion.h"
#include <algorithm>

namespace MotionOcc {

// Display requests may be removed with their source without deleting any
// physical dependencies. Work on a copied model so failed physical validation
// leaves both the document and its presentation unchanged.
inline void removeNativeEntityCurves(RigidModel& model, const QString& collection, int id)
{
    if (!model.nativeDocument) return;
    const QMap<QString, QString> aliases{{"Body", "bodies"}, {"Joint", "joints"}, {"Marker", "markers"},
        {"Drive", "drives"}, {"Coupling", "couplings"}, {"Sensor", "sensors"}, {"Controller", "controllers"},
        {"Spring", "springs"}, {"TensionCable", "tensionCables"}, {"SoftStop", "softStops"},
        {"VectorForce", "vectorForces"}, {"ScalarForce", "scalarForces"}, {"ScalarTorque", "scalarTorques"},
        {"ThreePointForce", "threePointForces"}, {"ThreePointTorque", "threePointTorques"}, {"AutoFlexBody", "autoFlexBodies"},
        {"HydraulicCylinder", "hydraulicCylinders"}, {"HydraulicMotor", "hydraulicMotors"},
        {"HydraulicNode", "hydraulicNodes"}, {"HydraulicComponent", "hydraulicComponents"}};
    const auto canonical = [&](const QString& source) { return aliases.value(source, source); };
    for (auto& scenario : model.scenarios) {
        auto& requests = scenario.curveRequests;
        requests.erase(std::remove_if(requests.begin(), requests.end(), [&](const RigidCurveRequest& request) {
            const auto source = request.metric == RigidCurveMetric::NativeChannel ? canonical(request.sourceType)
                : request.metric == RigidCurveMetric::JointCoordinate || request.metric == RigidCurveMetric::JointRate
                    ? QStringLiteral("joints") : QStringLiteral("bodies");
            return source == collection && request.sourceId == id;
        }), requests.end());
    }
    auto document = std::make_shared<NativeMotionDocument>(*model.nativeDocument);
    auto data = document->root.value(QStringLiteral("motionData")).toObject();
    auto scenarios = data.value(QStringLiteral("scenarios")).toArray();
    for (int i = 0; i < scenarios.size(); ++i) {
        auto scenario = scenarios[i].toObject(); QJsonArray requests;
        for (const auto& value : scenario.value(QStringLiteral("curveRequests")).toArray()) {
            const auto request = value.toObject();
            if (canonical(request.value(QStringLiteral("sourceType")).toString()) == collection
                && request.value(QStringLiteral("sourceId")).toInt(-1) == id) continue;
            requests.append(value);
        }
        if (scenario.contains(QStringLiteral("curveRequests"))) scenario.insert(QStringLiteral("curveRequests"), requests);
        scenarios[i] = scenario;
    }
    if (data.contains(QStringLiteral("scenarios"))) data.insert(QStringLiteral("scenarios"), scenarios);
    document->root.insert(QStringLiteral("motionData"), data); model.nativeDocument = std::move(document);
}

// Persist presentation changes in the original motion JSON too. Unknown
// per-request extension fields survive when the stable request ID is retained.
inline void syncNativeScenarioCurves(RigidModel& model, int scenarioId)
{
    if (!model.nativeDocument) return;
    const auto scenario = std::find_if(model.scenarios.begin(), model.scenarios.end(),
        [=](const auto& item) { return item.id == scenarioId; });
    if (scenario == model.scenarios.end()) return;
    auto document = std::make_shared<NativeMotionDocument>(*model.nativeDocument);
    auto data = document->root.value(QStringLiteral("motionData")).toObject();
    auto scenarios = data.value(QStringLiteral("scenarios")).toArray();
    for (int i = 0; i < scenarios.size(); ++i) {
        auto row = scenarios[i].toObject();
        if (row.value(QStringLiteral("id")).toInt(-1) != scenarioId) continue;
        const auto previous = row.value(QStringLiteral("curveRequests")).toArray();
        QJsonArray requests;
        for (const auto& request : scenario->curveRequests) {
            QJsonObject curve;
            for (const auto& old : previous)
                if (old.toObject().value(QStringLiteral("requestId")).toInt(-1) == request.id) { curve = old.toObject(); break; }
            QString source = request.sourceType, variable = request.variable;
            int component = request.component;
            if (request.metric != RigidCurveMetric::NativeChannel) {
                source = QStringLiteral("bodies");
                switch (request.metric) {
                case RigidCurveMetric::BodyPosition: variable = QStringLiteral("Position"); break;
                case RigidCurveMetric::BodyDisplacement: variable = QStringLiteral("Displacement"); break;
                case RigidCurveMetric::BodyVelocity: variable = QStringLiteral("Velocity"); break;
                case RigidCurveMetric::BodyRotation: variable = QStringLiteral("Rotation"); break;
                case RigidCurveMetric::BodyAngularVelocity: variable = QStringLiteral("AngularVelocity"); break;
                case RigidCurveMetric::JointCoordinate: source = QStringLiteral("joints"); variable = QStringLiteral("Coordinate"); break;
                case RigidCurveMetric::JointRate: source = QStringLiteral("joints"); variable = QStringLiteral("Rate"); break;
                default: break;
                }
                if (component == 3) component = -1;
            }
            curve.insert(QStringLiteral("requestId"), request.id);
            curve.insert(QStringLiteral("requestName"), request.name);
            curve.insert(QStringLiteral("sourceType"), source);
            curve.insert(QStringLiteral("sourceId"), request.sourceId);
            curve.insert(QStringLiteral("variable"), variable);
            curve.insert(QStringLiteral("component"), component);
            curve.insert(QStringLiteral("color"), request.color);
            curve.insert(QStringLiteral("visible"), request.visible);
            requests.append(curve);
        }
        row.insert(QStringLiteral("curveRequests"), requests);
        scenarios[i] = row;
        break;
    }
    data.insert(QStringLiteral("scenarios"), scenarios);
    document->root.insert(QStringLiteral("motionData"), data);
    model.nativeDocument = std::move(document);
}

} // namespace MotionOcc
