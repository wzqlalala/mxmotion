#include "RigidResultSeries.h"
#include "NativeMotionSolver.h"

#include <QCoreApplication>
#include <QMap>
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

QString rigidChannelSourceType(const QString& sourceType)
{
    // Original .motion.json files use singular PascalCase enum names. New
    // results and requests use the stable document collection names.
    static const QMap<QString, QString> collections{
        {"Body", "bodies"}, {"Joint", "joints"}, {"Marker", "markers"}, {"Drive", "drives"},
        {"Coupling", "couplings"}, {"Sensor", "sensors"}, {"Controller", "controllers"},
        {"Spring", "springs"}, {"TensionCable", "tensionCables"}, {"SoftStop", "softStops"},
        {"ScalarForce", "scalarForces"}, {"ScalarTorque", "scalarTorques"},
        {"ThreePointForce", "threePointForces"}, {"ThreePointTorque", "threePointTorques"},
        {"VectorForce", "vectorForces"}, {"AutoFlexBody", "autoFlexBodies"}};
    return collections.value(sourceType, sourceType);
}

QString rigidChannelSourceLabel(const QString& sourceType)
{
    static const QMap<QString, const char*> names{
        {"bodies", "运动体"}, {"joints", "运动副"}, {"markers", "标记点"}, {"drives", "驱动"},
        {"couplings", "耦合副"}, {"sensors", "传感器"}, {"controllers", "控制器"},
        {"springs", "弹簧"}, {"tensionCables", "拉索"}, {"softStops", "柔性限位"},
        {"scalarForces", "标量力"}, {"scalarTorques", "标量力矩"},
        {"threePointForces", "三点力"}, {"threePointTorques", "三点力矩"},
        {"vectorForces", "矢量载荷"}, {"autoFlexBodies", "柔性体"}};
    const auto it = names.constFind(rigidChannelSourceType(sourceType));
    return it == names.cend() ? sourceType : label(it.value());
}

QString rigidChannelObjectName(const RigidInput& input, const QString& sourceType, int sourceId)
{
    const auto collection = rigidChannelSourceType(sourceType);
    if (input.nativeInput) {
        // Reading the immutable Qt JSON snapshot keeps curve queries independent
        // of OCCT and of the solver runtime, including the standalone plot tests.
        const auto objects = input.nativeInput->document.root.value(QStringLiteral("motionData"))
            .toObject().value(collection).toArray();
        for (const auto& value : objects) {
            const auto object = value.toObject();
            if (object.value(QStringLiteral("id")).toInt(-1) == sourceId)
                return object.value(QStringLiteral("name")).toString();
        }
    }
    if (collection == QStringLiteral("bodies")) {
        const auto* body = findBody(input, sourceId);
        if (body) return body->body.name;
    }
    return {};
}

QString rigidChannelVariableLabel(const QString& variable)
{
    static const QMap<QString, const char*> names{
        {"Position", "位置"}, {"Displacement", "位移增量"}, {"Velocity", "速度"},
        {"Acceleration", "加速度"}, {"Rotation", "欧拉角"}, {"AngularVelocity", "角速度"},
        {"AngularAcceleration", "角加速度"}, {"PotentialEnergy", "势能"}, {"KineticEnergy", "动能"},
        {"ReactionForce", "约束反力"}, {"ReactionTorque", "约束反力矩"},
        {"ActuationForce", "驱动力"}, {"ActuationTorque", "驱动力矩"},
        {"ImposedValue", "驱动位移"}, {"ImposedSpeed", "驱动速度"}, {"ImposedAcc", "驱动加速度"},
        {"SensorValue", "传感器值"}, {"PidTarget", "控制目标"}, {"PidFeedback", "控制反馈"},
        {"PidError", "控制误差"}, {"PidOutput", "控制输出"}, {"Magnitude", "幅值"},
        {"SignedForce", "有向力"}, {"Coordinate", "坐标"}, {"Deformation", "变形量"},
        {"Speed", "速度"}, {"Rate", "相对速度"}, {"Engaged", "作用状态"}, {"Power", "功率"}, {"ElasticEnergy", "弹性能"},
        {"ForceI", "I 端力"}, {"ForceJ", "J 端力"}, {"TorqueI", "I 端力矩"}, {"TorqueJ", "J 端力矩"},
        {"PointI", "I 端位置"}, {"PointJ", "J 端位置"}, {"ModalCoordinates", "模态坐标"}};
    const auto it = names.constFind(variable);
    if (it != names.cend()) return label(it.value());
    if (variable.endsWith(QLatin1Char('J'))) {
        const auto original = names.constFind(variable.chopped(1));
        if (original != names.cend()) return label("%1（J 端）").arg(label(original.value()));
    }
    return variable;
}

QString rigidChannelComponentLabel(int components, int component)
{
    if (component == -1) return label("幅值");
    if (components == 1) return {};
    if (components == 3 && component >= 0 && component < 3) return QString(QChar('X' + component));
    return label("分量 %1").arg(component + 1);
}

const ResultChannel* rigidCurveChannel(const RigidResult& result, const RigidCurveRequest& request)
{
    if (request.metric != RigidCurveMetric::NativeChannel || request.sourceId < 0
        || request.sourceType.isEmpty() || request.variable.isEmpty()) return nullptr;
    const QString source = rigidChannelSourceType(request.sourceType);
    const ResultChannel* found = nullptr;
    for (const auto& channel : result.channels) {
        if (rigidChannelSourceType(channel.sourceType) != source || channel.sourceId != request.sourceId
            || channel.variable != request.variable) continue;
        if (found) return nullptr; // Duplicate keys must not choose an arbitrary result.
        found = &channel;
    }
    return found;
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
    case RigidCurveMetric::NativeChannel: return label("结果通道");
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
    case RigidCurveMetric::NativeChannel: return {};
    }
    return {};
}

QString rigidCurveTitle(const RigidInput& input, const RigidCurveRequest& request, const RigidResult* result)
{
    if (!request.name.trimmed().isEmpty()) return request.name.trimmed();
    if (request.metric == RigidCurveMetric::NativeChannel) {
        QString object = rigidChannelObjectName(input, request.sourceType, request.sourceId);
        if (object.isEmpty()) object = QStringLiteral("%1 %2").arg(rigidChannelSourceLabel(request.sourceType)).arg(request.sourceId);
        const auto* channel = result ? rigidCurveChannel(*result, request) : nullptr;
        QString quantity = rigidChannelVariableLabel(request.variable);
        const auto component = rigidChannelComponentLabel(channel ? channel->components : 3, request.component);
        if (!component.isEmpty()) quantity += QLatin1Char(' ') + component;
        return QStringLiteral("%1 · %2").arg(object, quantity);
    }
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
    if (request.metric == RigidCurveMetric::NativeChannel) {
        const auto* channel = rigidCurveChannel(result, request);
        if (!channel) return invalid(label("结果通道缺失或包含重复来源。"));
        if (channel->components <= 0 || channel->components > 1024
            || result.frames.size() > std::size_t(std::numeric_limits<int>::max())
            || channel->values.size() / std::size_t(channel->components) != result.frames.size()
            || channel->values.size() % std::size_t(channel->components) != 0)
            return invalid(label("结果通道采样数量与帧数不一致。"));
        const bool magnitude = request.component == -1;
        const bool euler = request.variable == QStringLiteral("Rotation") || request.variable == QStringLiteral("RotationJ");
        if ((magnitude && (channel->components != 3 || euler))
            || (!magnitude && (request.component < 0 || request.component >= channel->components)))
            return invalid(label("结果分量无效。"));
        RigidCurveSeries series;
        series.quantity = rigidChannelVariableLabel(channel->variable);
        series.unit = channel->unit; // Empty is a valid dimensionless result.
        series.times.reserve(int(result.frames.size()));
        series.values.reserve(int(result.frames.size()));
        double lastTime = -std::numeric_limits<double>::infinity();
        double lastAngle = 0.0;
        for (std::size_t i = 0; i < result.frames.size(); ++i) {
            const double time = result.frames[i].seconds;
            if (!std::isfinite(time) || time <= lastTime)
                return invalid(label("结果时间必须有限且严格递增。"));
            lastTime = time;
            const auto begin = channel->values.begin() + i * std::size_t(channel->components);
            if (!std::all_of(begin, begin + channel->components, [](double v) { return std::isfinite(v); }))
                return invalid(label("结果包含非有限的采样。"));
            double value = magnitude ? std::hypot(begin[0], begin[1], begin[2]) : begin[request.component];
            if (euler && channel->components == 3) {
                if (!series.values.isEmpty()) value += 360.0 * std::floor((lastAngle - value) / 360.0 + 0.5);
                lastAngle = value;
            }
            if (!std::isfinite(value)) return invalid(label("结果包含非有限的采样。"));
            series.times.append(time);
            series.values.append(value);
        }
        return series;
    }
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
