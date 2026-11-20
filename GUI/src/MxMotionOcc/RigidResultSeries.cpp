#include "RigidResultSeries.h"

#include <QCoreApplication>
#include <QQuaternion>

#include <algorithm>
#include <cmath>
#include <limits>

namespace MotionOcc {
namespace {

QString label(const char* text) { return QCoreApplication::translate("RigidPlot", text); }

template<class State>
const State* findState(const std::vector<State>& states, int State::* member, int id)
{
    const State* result = nullptr;
    for (const auto& state : states) {
        if (state.*member != id) continue;
        if (result) return nullptr; // Duplicate IDs are as invalid as a missing entity.
        result = &state;
    }
    return result;
}

const RigidJoint* findJoint(const RigidInput& input, int id)
{
    const auto it = std::find_if(input.joints.begin(), input.joints.end(),
                               [id](const RigidJoint& joint) { return joint.id == id; });
    return it == input.joints.end() ? nullptr : &*it;
}

const PreparedBody* findBody(const RigidInput& input, int id)
{
    const auto it = std::find_if(input.bodies.begin(), input.bodies.end(),
                               [id](const PreparedBody& body) { return body.body.id == id; });
    return it == input.bodies.end() ? nullptr : &*it;
}

RigidCurveSeries invalid(const QString& message)
{
    RigidCurveSeries series;
    series.error = message;
    return series;
}

} // namespace

bool isBodyCurve(RigidCurveMetric metric)
{
    switch (metric) {
    case RigidCurveMetric::BodyPosition:
    case RigidCurveMetric::BodyDisplacement:
    case RigidCurveMetric::BodyVelocity:
    case RigidCurveMetric::BodyRotation:
    case RigidCurveMetric::BodyAngularVelocity:
        return true;
    default:
        return false;
    }
}

QString rigidCurveQuantity(RigidCurveMetric metric, JointKind jointKind)
{
    switch (metric) {
    case RigidCurveMetric::BodyPosition: return label("质心位置");
    case RigidCurveMetric::BodyDisplacement: return label("质心位移增量");
    case RigidCurveMetric::BodyVelocity: return label("线速度");
    case RigidCurveMetric::BodyRotation: return label("欧拉角");
    case RigidCurveMetric::BodyAngularVelocity: return label("角速度");
    case RigidCurveMetric::JointCoordinate:
        return jointKind == JointKind::Prismatic ? label("相对行程") : label("相对转角");
    case RigidCurveMetric::JointRate:
        return jointKind == JointKind::Prismatic ? label("相对滑移速度") : label("相对角速度");
    }
    return {};
}

QString rigidCurveUnit(RigidCurveMetric metric, JointKind jointKind)
{
    switch (metric) {
    case RigidCurveMetric::BodyPosition:
    case RigidCurveMetric::BodyDisplacement: return QStringLiteral("mm");
    case RigidCurveMetric::BodyVelocity: return QStringLiteral("mm/s");
    case RigidCurveMetric::BodyRotation: return QStringLiteral("°");
    case RigidCurveMetric::BodyAngularVelocity: return QStringLiteral("°/s");
    case RigidCurveMetric::JointCoordinate:
        return jointKind == JointKind::Prismatic ? QStringLiteral("mm") : QStringLiteral("°");
    case RigidCurveMetric::JointRate:
        return jointKind == JointKind::Prismatic ? QStringLiteral("mm/s") : QStringLiteral("°/s");
    }
    return {};
}

QString rigidCurveTitle(const RigidInput& input, const RigidCurveRequest& request)
{
    if (!request.name.trimmed().isEmpty()) return request.name.trimmed();
    QString object;
    JointKind kind = JointKind::Revolute;
    if (isBodyCurve(request.metric)) {
        const auto* body = findBody(input, request.sourceId);
        object = body && !body->body.name.isEmpty() ? body->body.name : label("运动体 %1").arg(request.sourceId);
    } else {
        const auto* joint = findJoint(input, request.sourceId);
        if (joint) kind = joint->kind;
        object = joint && !joint->name.isEmpty() ? joint->name : label("运动副 %1").arg(request.sourceId);
    }
    QString quantity = rigidCurveQuantity(request.metric, kind);
    if (isBodyCurve(request.metric)) {
        static const char* components[] = {"X", "Y", "Z", "幅值"};
        if (request.component >= 0 && request.component < 4)
            quantity += QLatin1Char(' ') + label(components[request.component]);
    }
    return QStringLiteral("%1 · %2").arg(object, quantity);
}

RigidCurveSeries rigidCurveSeries(const RigidInput& input, const RigidResult& result,
                                 const RigidCurveRequest& request)
{
    if (result.scenarioId != input.scenario.id)
        return invalid(label("结果与求解方案不匹配。"));
    if (result.frames.empty()) return invalid(label("当前方案没有结果采样。"));
    const bool bodyMetric = isBodyCurve(request.metric);
    const auto* body = bodyMetric ? findBody(input, request.sourceId) : nullptr;
    const auto* joint = bodyMetric ? nullptr : findJoint(input, request.sourceId);
    if ((bodyMetric && !body) || (!bodyMetric && (!joint || joint->kind == JointKind::Fixed)))
        return invalid(label("所选对象没有对应的结果通道。"));
    if ((bodyMetric && (request.component < 0 || request.component > 3))
        || (request.metric == RigidCurveMetric::BodyRotation && request.component > 2)
        || (!bodyMetric && request.component != 0))
        return invalid(label("结果分量无效。"));
    const auto kind = joint ? joint->kind : JointKind::Revolute;
    RigidCurveSeries series;
    series.quantity = rigidCurveQuantity(request.metric, kind);
    series.unit = rigidCurveUnit(request.metric, kind);
    if (series.quantity.isEmpty() || series.unit.isEmpty()) return invalid(label("结果变量不受支持。"));
    const auto* initial = bodyMetric
        ? findState(result.frames.front().bodies, &BodyState::bodyId, request.sourceId) : nullptr;
    if (bodyMetric && !initial) return invalid(label("初始帧缺少所选运动体。"));
    series.times.reserve(int(result.frames.size()));
    series.values.reserve(int(result.frames.size()));
    double lastTime = -std::numeric_limits<double>::infinity();
    double lastAngle = 0.0;
    for (const auto& frame : result.frames) {
        if (!std::isfinite(frame.seconds) || frame.seconds <= lastTime)
            return invalid(label("结果时间必须有限且严格递增。"));
        lastTime = frame.seconds;
        double value = 0.0;
        if (bodyMetric) {
            const auto* state = findState(frame.bodies, &BodyState::bodyId, request.sourceId);
            if (!state) return invalid(label("结果帧缺少运动体或包含重复编号。"));
            Vector3d vector{};
            switch (request.metric) {
            case RigidCurveMetric::BodyPosition: vector = state->positionMm; break;
            case RigidCurveMetric::BodyDisplacement:
                for (std::size_t i = 0; i < vector.size(); ++i)
                    vector[i] = state->positionMm[i] - initial->positionMm[i];
                break;
            case RigidCurveMetric::BodyVelocity: vector = state->velocityMmS; break;
            case RigidCurveMetric::BodyAngularVelocity: vector = state->angularVelocityDegS; break;
            case RigidCurveMetric::BodyRotation: {
                double norm = 0.0;
                for (const double coordinate : state->rotation) {
                    if (!std::isfinite(coordinate)) return invalid(label("结果包含非有限的姿态。"));
                    norm += coordinate * coordinate;
                }
                if (!std::isfinite(norm) || norm < 1e-20) return invalid(label("结果包含无效的姿态四元数。"));
                const double scale = 1.0 / std::sqrt(norm);
                const auto angles = QQuaternion(float(state->rotation[0] * scale), float(state->rotation[1] * scale),
                    float(state->rotation[2] * scale), float(state->rotation[3] * scale)).toEulerAngles();
                vector = {angles.x(), angles.y(), angles.z()};
                break;
            }
            default: return invalid(label("运动体结果变量不受支持。"));
            }
            if (!std::all_of(vector.begin(), vector.end(), [](double x) { return std::isfinite(x); }))
                return invalid(label("结果包含非有限的采样。"));
            value = request.component < 3 ? vector[request.component] : std::hypot(vector[0], vector[1], vector[2]);
            if (request.metric == RigidCurveMetric::BodyRotation) {
                if (!series.values.isEmpty())
                    value += 360.0 * std::floor((lastAngle - value) / 360.0 + 0.5);
                lastAngle = value;
            }
        } else {
            const auto* state = findState(frame.joints, &JointState::jointId, request.sourceId);
            if (!state) return invalid(label("结果帧缺少运动副或包含重复编号。"));
            value = request.metric == RigidCurveMetric::JointCoordinate ? state->position : state->speed;
        }
        if (!std::isfinite(value)) return invalid(label("结果包含非有限的采样。"));
        series.times.append(frame.seconds);
        series.values.append(value);
    }
    return series;
}

} // namespace MotionOcc
