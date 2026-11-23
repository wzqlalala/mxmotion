#include "mMotionResultQueryService.h"
#include "mMotionRefMap.h"

#include "mMotionSensorEvaluator.h"
#include "mMotionBodySensor.h"
#include "mMotionJointSensor.h"
#include "mMotionDriveSensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"

#include <QQuaternion>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
    bool finiteVector(const QVector3D& value)
    {
        return std::isfinite(value.x()) && std::isfinite(value.y()) && std::isfinite(value.z());
    }

    bool finiteRotation(const QQuaternion& value)
    {
        return std::isfinite(value.scalar()) && std::isfinite(value.x()) &&
            std::isfinite(value.y()) && std::isfinite(value.z()) && value.lengthSquared() > 1e-20f;
    }

    double unavailableValue()
    {
        return std::numeric_limits<double>::quiet_NaN();
    }

    // Use the same accepted-frame point kinematics as the solver. Differencing
    // output samples changes velocity/acceleration when the output interval
    // changes and does not reproduce the signal used by a controller.
    bool distanceValue(const MechMotionData::mMotionDistanceSensor& sensor,
        const MechMotionData::mMotionFrame& frame, double& value)
    {
        using namespace MechMotionData;
        using MechMainWindow::mMotionSensorEvaluator;
        const auto& a = sensor.getPointA();
        const auto& b = sensor.getPointB();
        const auto* bodyA = mMotionSensorEvaluator::bodyFrameAt(frame, a.bodyId);
        const auto* bodyB = mMotionSensorEvaluator::bodyFrameAt(frame, b.bodyId);
        if ((a.bodyId >= 0 && !bodyA) || (b.bodyId >= 0 && !bodyB))return false;
        const QVector3D pointA = bodyA ? mMotionSensorEvaluator::pointPosition(*bodyA, a.localPos) : a.localPos;
        const QVector3D pointB = bodyB ? mMotionSensorEvaluator::pointPosition(*bodyB, b.localPos) : b.localPos;
        if (!finiteVector(pointA) || !finiteVector(pointB))return false;
        const QVector3D separation = pointB - pointA;
        const double length = separation.length();
        if (sensor.getVariable() == DistanceVariable::Distance) { value = length;return true; }
        const QVector3D velocityA = bodyA ? mMotionSensorEvaluator::pointVelocity(*bodyA, a.localPos) : QVector3D();
        const QVector3D velocityB = bodyB ? mMotionSensorEvaluator::pointVelocity(*bodyB, b.localPos) : QVector3D();
        if (!finiteVector(velocityA) || !finiteVector(velocityB))return false;
        const QVector3D velocity = velocityB - velocityA;
        const double rate = length > 1e-12 ? QVector3D::dotProduct(separation, velocity) / length : 0.;
        if (sensor.getVariable() == DistanceVariable::RelVelocity) { value = rate;return true; }
        if (sensor.getVariable() != DistanceVariable::RelAcceleration)return false;
        const QVector3D accelerationA = bodyA ? mMotionSensorEvaluator::pointAcceleration(*bodyA, a.localPos) : QVector3D();
        const QVector3D accelerationB = bodyB ? mMotionSensorEvaluator::pointAcceleration(*bodyB, b.localPos) : QVector3D();
        if (!finiteVector(accelerationA) || !finiteVector(accelerationB))return false;
        value = length > 1e-12 ? (double(velocity.lengthSquared()) +
            QVector3D::dotProduct(separation, accelerationB - accelerationA) - rate * rate) / length : 0.;
        return std::isfinite(value);
    }
}

namespace MechMainWindow
{
    mMotionResultQueryService::FrameSpan mMotionResultQueryService::locateTime(
        const MechMotionData::mMotionResultData* result,
        double time,
        bool clampToRange)
    {
        FrameSpan span;
        if (!result || result->m_frames.empty() || !std::isfinite(time))
            return span;

        const auto& frames = result->m_frames;
        if (clampToRange) {
            time = std::max(frames.front().m_time, std::min(time, frames.back().m_time));
        }
        else if (time < frames.front().m_time || time > frames.back().m_time) {
            return span;
        }

        const auto upper = std::lower_bound(frames.begin(), frames.end(), time,
            [](const MechMotionData::mMotionFrame& frame, double value) {
                return frame.m_time < value;
            });

        span.secondIndex = static_cast<int>(std::distance(frames.begin(), upper));
        if (upper == frames.end())
            span.secondIndex = static_cast<int>(frames.size()) - 1;
        span.firstIndex = span.secondIndex > 0 ? span.secondIndex - 1 : 0;

        const double firstTime = frames[span.firstIndex].m_time;
        const double secondTime = frames[span.secondIndex].m_time;
        span.ratio = secondTime > firstTime
            ? (time - firstTime) / (secondTime - firstTime)
            : 0.0;
        span.ratio = std::max(0.0, std::min(span.ratio, 1.0));
        return span;
    }

    bool mMotionResultQueryService::getMarkerPosition(
        const MechMotionData::mMotionResultData* result,
        int markerId,
        double time,
        MarkerPosition& position)
    {
        position = MarkerPosition();
        const FrameSpan span = locateTime(result, time);
        if (!span.isValid())
            return false;

        const auto& first = result->m_frames[span.firstIndex].m_markerFrames.find(markerId);
        const auto& second = result->m_frames[span.secondIndex].m_markerFrames.find(markerId);
        if (first == result->m_frames[span.firstIndex].m_markerFrames.end() ||
            second == result->m_frames[span.secondIndex].m_markerFrames.end())
            return false;
        if (!finiteVector(first->second.m_worldPosition) || !finiteVector(second->second.m_worldPosition) ||
            !finiteRotation(first->second.m_worldRotation) || !finiteRotation(second->second.m_worldRotation))
            return false;

        const auto ratio = static_cast<float>(span.ratio);
        position.position = first->second.m_worldPosition +
            (second->second.m_worldPosition - first->second.m_worldPosition) * ratio;
        position.rotation = QQuaternion::slerp(
            first->second.m_worldRotation,
            second->second.m_worldRotation,
            ratio);
        position.isValid = true;
        return true;
    }

    bool mMotionResultQueryService::getMarkerPositions(
        const MechMotionData::mMotionResultData* result,
        int markerId,
        QVector<QVector3D>& positions)
    {
        positions.clear();
        if (!result)
            return false;

        for (const auto& frame : result->m_frames) {
            const auto marker = frame.m_markerFrames.find(markerId);
            if (marker == frame.m_markerFrames.end() || !finiteVector(marker->second.m_worldPosition)) {
                positions.clear();
                return false;
            }
            positions.push_back(marker->second.m_worldPosition);
        }
        return !positions.isEmpty();
    }

    bool mMotionResultQueryService::getBodyMatrix(
        const MechMotionData::mMotionResultData* result,
        int bodyId,
        const QVector3D& initialCenterOfMass,
        double time,
        QMatrix4x4& matrix)
    {
        const FrameSpan span = locateTime(result, time);
        if (!span.isValid())
            return false;

        const auto& firstFrames = result->m_frames[span.firstIndex].m_bodyTransforms;
        const auto& secondFrames = result->m_frames[span.secondIndex].m_bodyTransforms;
        const auto first = firstFrames.find(bodyId);
        const auto second = secondFrames.find(bodyId);
        if (first == firstFrames.end() || second == secondFrames.end())
            return false;

        // 命中物理帧（或无限接近）时直接返回求解器原始矩阵，与拖时间条严格一致
        if (span.ratio <= 0.001) {
            matrix = first->second;
            return true;
        }
        if (span.ratio >= 0.999) {
            matrix = second->second;
            return true;
        }

        // 帧间插值沿用生成侧参数化（mMotionSolverThread 构造矩阵用的就是这组量）：
        // 质心位移 lerp + 姿态四元数 slerp，再按 T(dpos)*T(cm)*R*T(-cm) 重组
        const auto& firstBodyFrames = result->m_frames[span.firstIndex].m_bodyFrames;
        const auto& secondBodyFrames = result->m_frames[span.secondIndex].m_bodyFrames;
        const auto bodyFirst = firstBodyFrames.find(bodyId);
        const auto bodySecond = secondBodyFrames.find(bodyId);
        if (bodyFirst != firstBodyFrames.end() && bodySecond != secondBodyFrames.end())
        {
            if (!finiteVector(bodyFirst->second.m_displacement) || !finiteVector(bodySecond->second.m_displacement) ||
                !finiteRotation(bodyFirst->second.m_rotation) || !finiteRotation(bodySecond->second.m_rotation))
                return false;
            const auto blend = static_cast<float>(span.ratio);
            const QVector3D displacement = bodyFirst->second.m_displacement +
                (bodySecond->second.m_displacement - bodyFirst->second.m_displacement) * blend;
            const QQuaternion rotation = QQuaternion::slerp(
                bodyFirst->second.m_rotation,
                bodySecond->second.m_rotation,
                blend);

            matrix.setToIdentity();
            matrix.translate(displacement);
            matrix.translate(initialCenterOfMass);
            matrix.rotate(rotation);
            matrix.translate(-initialCenterOfMass);
            return true;
        }

        // 兜底：帧内无体运动学数据（旧数据/演示路径），退回矩阵分量插值
        interpolateBodyMatrix(first->second, second->second, span.ratio, matrix);
        return true;
    }

    mMotionResultQueryService::ResultSeries mMotionResultQueryService::getResultSeries(
        const MechMotionData::mMotionResultData* result,
        const MechMotionData::mMotionCurveRequest& request)
    {
        ResultSeries series;
        if (!result || request.m_sourceId < 0)
            return series;

        series.times.reserve(result->getFrameCount());
        series.values.reserve(result->getFrameCount());

        // Rotation 曲线为欧拉角（度）主值，范围 (-180,180]，连续转动会在 ±180°
        // 处跳变 360°——按序列展开为连续转角（与角度传感器同规则，度制 360° 版）
        const bool unwrapRotation = request.m_variable == MechMotionData::CurveVariable::Rotation &&
            request.m_component != MechMotionData::CurveComponent::Magnitude;
        double lastUnwrapped = 0.0;
        bool hasLast = false;

        for (const auto& frame : result->m_frames) {
            double value = 0.0;
            // Never silently remove an accepted output frame or bridge a
            // missing channel with a fabricated zero-valued sample.
            if (!appendFrameValue(frame, request, value) || !std::isfinite(value) || !std::isfinite(frame.m_time))
                return ResultSeries();

            if (unwrapRotation) {
                if (hasLast)
                    value += 360.0 * std::floor((lastUnwrapped - value) / 360.0 + 0.5);
                lastUnwrapped = value;
                hasLast = true;
            }

            series.times.push_back(frame.m_time);
            series.values.push_back(value);
        }
        return series;
    }

    mMotionResultQueryService::LoadSeries mMotionResultQueryService::getLoadSeries(
        const MechMotionData::mMotionResultData* result,
        MechMotionData::CurveSourceType sourceType,
        int sourceId)
    {
        LoadSeries series;
        if (!result || sourceId < 0)
            return series;

        const int frameCount = result->getFrameCount();
        series.times.reserve(frameCount);
        series.forces.reserve(frameCount);
        series.torques.reserve(frameCount);

        for (const auto& frame : result->m_frames) {
            const MechMotionData::mMotionJointFrame* jointFrame = nullptr;
            const MechMotionData::mMotionDriveFrame* driveFrame = nullptr;

            if (sourceType == MechMotionData::CurveSourceType::Joint) {
                const auto item = frame.m_jointFrames.find(sourceId);
                if (item != frame.m_jointFrames.end())
                    jointFrame = &item->second;
            }
            else if (sourceType == MechMotionData::CurveSourceType::Coupling) {
                const auto item = frame.m_couplingFrames.find(sourceId);
                if (item != frame.m_couplingFrames.end())
                    jointFrame = &item->second;
            }
            else if (sourceType == MechMotionData::CurveSourceType::Drive) {
                const auto item = frame.m_driveFrames.find(sourceId);
                if (item != frame.m_driveFrames.end())
                    driveFrame = &item->second;
            }
            else {
                return series;
            }

            if (!jointFrame && !driveFrame)
                return LoadSeries();

            const auto& force = jointFrame ? jointFrame->m_reactionForce : driveFrame->m_reactionForce;
            const auto& torque = jointFrame ? jointFrame->m_reactionTorque : driveFrame->m_reactionTorque;
            if (!finiteVector(force) || !finiteVector(torque) || !std::isfinite(frame.m_time))
                return LoadSeries();

            series.times.push_back(frame.m_time);
            series.forces.push_back(jointFrame
                ? jointFrame->m_reactionForce
                : driveFrame->m_reactionForce);
            series.torques.push_back(jointFrame
                ? jointFrame->m_reactionTorque
                : driveFrame->m_reactionTorque);
        }
        return series;
    }

    mMotionResultQueryService::ResultSeries mMotionResultQueryService::getControllerSeries(
        const MechMotionData::mMotionResultData* result,
        int controllerId,
        MechMotionData::CurveVariable variable)
    {
        using namespace MechMotionData;
        ResultSeries series;
        if (!result)
            return series;

        auto it = result->m_pidHistories.find(controllerId);
        if (it == result->m_pidHistories.end())
            return series;

        const int n = static_cast<int>(it->second.size());
        series.times.reserve(n);
        series.values.reserve(n);
        for (const auto& sample : it->second) {
            double v = 0.0;
            switch (variable) {
            case CurveVariable::PidTarget:   v = sample.m_target;   break;
            case CurveVariable::PidFeedback: v = sample.m_feedback; break;
            case CurveVariable::PidError:    v = sample.m_error;    break;
            case CurveVariable::PidOutput:   v = sample.m_output;   break;
            default: continue;
            }
            if (!std::isfinite(v) || !std::isfinite(sample.m_time))return ResultSeries();
            series.times.append(sample.m_time);
            series.values.append(v);
        }
        return series;
    }

    double mMotionResultQueryService::sensorValueAtFrame(
        const MechMotionData::mMotionSensorBase* sensor,
        const MechMotionData::mMotionFrame& frame,
        MechMotionData::CurveComponent component,
        bool& ok)
    {
        using namespace MechMotionData;
        ok = false;
        if (!sensor)
            return 0.0;

        auto vectorValue = [component, &ok](const QVector3D& v) -> double {
            if (!finiteVector(v)) { ok = false;return 0.; }
            switch (component) {
            case CurveComponent::X:  return v.x();
            case CurveComponent::Y:  return v.y();
            case CurveComponent::Z:  return v.z();
            default:                 return v.length();
            }
        };
        auto coordVec = [&sensor](const QVector3D& worldVec, const mMotionFrame& f) {
            const int reference = mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody);
            if (sensor->getCoordSystem() == CoordSystem::BodyFixed && reference >= 0 &&
                !mMotionSensorEvaluator::bodyFrameAt(f, reference)) {
                const float missing = float(unavailableValue());
                return QVector3D(missing, missing, missing);
            }
            return mMotionSensorEvaluator::toCoordSystem(
                worldVec, f, sensor->getCoordSystem(), reference);
        };

        switch (sensor->getType()) {
        case SensorType::BodyDisplacement:
        case SensorType::BodyVelocity:
        case SensorType::BodyAcceleration: {
            const auto* s = static_cast<const mMotionBodySensor*>(sensor);
            const auto* bf = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody));
            if (!bf)
                return 0.0;
            const bool useRefPoint = !s->getReferencePoint().isNull();
            switch (s->getVariable()) {
            case BodySensorVariable::Position:
                ok = true;
                return vectorValue(coordVec(useRefPoint
                    ? mMotionSensorEvaluator::pointPosition(*bf, s->getReferencePoint())
                    : bf->m_position, frame));
            case BodySensorVariable::Velocity:
                ok = true;
                return vectorValue(coordVec(useRefPoint
                    ? mMotionSensorEvaluator::pointVelocity(*bf, s->getReferencePoint())
                    : bf->m_velocity, frame));
            case BodySensorVariable::Acceleration:
                ok = true;
                return vectorValue(coordVec(useRefPoint
                    ? mMotionSensorEvaluator::pointAcceleration(*bf, s->getReferencePoint())
                    : bf->m_acceleration, frame));
            case BodySensorVariable::AngularVelocity:
                ok = true;
                // rad/s → °/s（与 Rotation 欧拉角(度)统一）
                return vectorValue(coordVec(bf->m_angularVelocity * 57.29577951308232, frame));
            case BodySensorVariable::AngularAcceleration:
                ok = true;
                return vectorValue(coordVec(bf->m_angularAcceleration * 57.29577951308232, frame));
            case BodySensorVariable::Rotation:
                // 与现有 Body 源曲线一致：欧拉角（度），不做坐标系变换
                if (!finiteRotation(bf->m_rotation))return 0.;
                ok = true;
                return vectorValue(bf->m_rotation.toEulerAngles());
            default:
                return 0.0;
            }
        }

        case SensorType::Distance: {
            const auto* s = static_cast<const mMotionDistanceSensor*>(sensor);
            double value = 0.;
            ok = distanceValue(*s, frame, value);
            return ok ? value : 0.;
        }

        case SensorType::Angle: {
            const auto* s = static_cast<const mMotionAngleSensor*>(sensor);
            const auto* bfA = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyA));
            const auto* bfB = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyB));
            if ((mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyA) >= 0 && !bfA) || (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyB) >= 0 && !bfB))
                return 0.0;
            // 大地体（id<0）按单位旋转处理，与 getSensorSeries 一致
            const QQuaternion qA = bfA ? bfA->m_rotation : QQuaternion();
            const QQuaternion qB = bfB ? bfB->m_rotation : QQuaternion();
            if (!finiteRotation(qA) || !finiteRotation(qB))return 0.;
            // Both relative rotation and the configured axis are in the
            // world frame, matching the solver's angle sensor.
            const QQuaternion qRel = qA * qB.conjugated();
            QVector3D axis = s->getAxisDirection();
            if (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody) >= 0) {
                const auto* axisFrame = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
                if (!axisFrame)
                    return 0.0;
                axis = axisFrame->m_rotation.rotatedVector(axis);
            }
            if (!finiteVector(axis) || axis.lengthSquared() < 1e-20f)return 0.;
            const double value = qRadiansToDegrees(mMotionSensorEvaluator::signedAngleAboutAxis(qRel, axis));
            ok = std::isfinite(value);
            return ok ? value : 0.;
        }

        default:
            return 0.0;
        }
    }

    mMotionResultQueryService::ResultSeries mMotionResultQueryService::getSensorSeries(
        const MechMotionData::mMotionResultData* result,
        const MechMotionData::mMotionSensorBase* sensor,
        MechMotionData::CurveComponent component)
    {
        ResultSeries series;
        if (!result || !sensor)
            return series;

        // 矢量取分量（幅值=模）；标量变量不走此路径
        auto vectorValue = [component](const QVector3D& v) -> double {
            if (!finiteVector(v))return unavailableValue();
            switch (component) {
            case MechMotionData::CurveComponent::X:  return v.x();
            case MechMotionData::CurveComponent::Y:  return v.y();
            case MechMotionData::CurveComponent::Z:  return v.z();
            default:                                  return v.length();
            }
        };
        // 坐标系变换仅作用于矢量量
        auto coordVec = [&sensor](const QVector3D& worldVec, const MechMotionData::mMotionFrame& frame) {
            const int reference = mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody);
            if (sensor->getCoordSystem() == MechMotionData::CoordSystem::BodyFixed && reference >= 0 &&
                !mMotionSensorEvaluator::bodyFrameAt(frame, reference)) {
                const float missing = float(unavailableValue());
                return QVector3D(missing, missing, missing);
            }
            return mMotionSensorEvaluator::toCoordSystem(
                worldVec, frame, sensor->getCoordSystem(), reference);
        };

        switch (sensor->getType()) {
        // --- 体传感器（位移/速度/加速度三类共用体帧数据） ---
        case MechMotionData::SensorType::BodyDisplacement:
        case MechMotionData::SensorType::BodyVelocity:
        case MechMotionData::SensorType::BodyAcceleration: {
            const auto* s = static_cast<const MechMotionData::mMotionBodySensor*>(sensor);
            const bool useRefPoint = !s->getReferencePoint().isNull();
            double lastRotation = 0.;
            bool hasLastRotation = false;
            for (const auto& frame : result->m_frames) {
                const auto* bf = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody));
                if (!bf)
                    return ResultSeries();

                double value = 0.0;
                switch (s->getVariable()) {
                case MechMotionData::BodySensorVariable::Position:
                    value = vectorValue(coordVec(useRefPoint
                        ? mMotionSensorEvaluator::pointPosition(*bf, s->getReferencePoint())
                        : bf->m_position, frame));
                    break;
                case MechMotionData::BodySensorVariable::Velocity:
                    value = vectorValue(coordVec(useRefPoint
                        ? mMotionSensorEvaluator::pointVelocity(*bf, s->getReferencePoint())
                        : bf->m_velocity, frame));
                    break;
                case MechMotionData::BodySensorVariable::Acceleration:
                    value = vectorValue(coordVec(useRefPoint
                        ? mMotionSensorEvaluator::pointAcceleration(*bf, s->getReferencePoint())
                        : bf->m_acceleration, frame));
                    break;
                case MechMotionData::BodySensorVariable::AngularVelocity:
                    value = vectorValue(coordVec(bf->m_angularVelocity * 57.29577951308232, frame));
                    break;
                case MechMotionData::BodySensorVariable::AngularAcceleration:
                    value = vectorValue(coordVec(bf->m_angularAcceleration * 57.29577951308232, frame));
                    break;
                case MechMotionData::BodySensorVariable::Rotation: {
                    // 与现有 Body 源曲线一致：欧拉角（度），不做坐标系变换
                    if (!finiteRotation(bf->m_rotation))return ResultSeries();
                    value = vectorValue(bf->m_rotation.toEulerAngles());
                    break;
                }
                default:
                    return ResultSeries();
                }

                if (!std::isfinite(value) || !std::isfinite(frame.m_time))return ResultSeries();
                if (s->getVariable() == MechMotionData::BodySensorVariable::Rotation &&
                    component != MechMotionData::CurveComponent::Magnitude) {
                    if (hasLastRotation)value += 360. * std::floor((lastRotation - value) / 360. + .5);
                    lastRotation = value;hasLastRotation = true;
                }
                series.times.push_back(frame.m_time);
                series.values.push_back(value);
            }
            return series;
        }

        // --- 关节反力传感器 ---
        case MechMotionData::SensorType::JointReaction: {
            const auto* s = static_cast<const MechMotionData::mMotionJointSensor*>(sensor);
            for (const auto& frame : result->m_frames) {
                const auto it = frame.m_jointFrames.find(mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionJoint));
                if (it == frame.m_jointFrames.end())
                    return ResultSeries();

                const QVector3D world = (s->getVariable() == MechMotionData::JointSensorVariable::ReactionForce)
                    ? it->second.m_reactionForce
                    : it->second.m_reactionTorque;

                const double value = vectorValue(coordVec(world, frame));
                if (!std::isfinite(value) || !std::isfinite(frame.m_time))return ResultSeries();
                series.times.push_back(frame.m_time);
                series.values.push_back(value);
            }
            return series;
        }

        // --- 驱动输出传感器 ---
        case MechMotionData::SensorType::DriveActuation: {
            const auto* s = static_cast<const MechMotionData::mMotionDriveSensor*>(sensor);
            for (const auto& frame : result->m_frames) {
                const auto it = frame.m_driveFrames.find(mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionDrive));
                if (it == frame.m_driveFrames.end())
                    return ResultSeries();

                const auto& df = it->second;
                double value = 0.0;
                switch (s->getVariable()) {
                case MechMotionData::DriveSensorVariable::ImposedValue: value = df.m_imposedValue; break;
                case MechMotionData::DriveSensorVariable::ImposedSpeed: value = df.m_imposedSpeed; break;
                case MechMotionData::DriveSensorVariable::ImposedAcc:   value = df.m_imposedAcc; break;
                case MechMotionData::DriveSensorVariable::ActuationForce:
                    value = vectorValue(coordVec(df.m_actuationForce, frame)); break;
                case MechMotionData::DriveSensorVariable::ActuationTorque:
                    value = vectorValue(coordVec(df.m_actuationTorque, frame)); break;
                case MechMotionData::DriveSensorVariable::ReactionForce:
                    value = vectorValue(coordVec(df.m_reactionForce, frame)); break;
                case MechMotionData::DriveSensorVariable::ReactionTorque:
                    value = vectorValue(coordVec(df.m_reactionTorque, frame)); break;
                default:
                    return ResultSeries();
                }

                if (!std::isfinite(value) || !std::isfinite(frame.m_time))return ResultSeries();
                series.times.push_back(frame.m_time);
                series.values.push_back(value);
            }
            return series;
        }

        // --- 距离传感器（使用各接受帧的真实点运动学） ---
        case MechMotionData::SensorType::Distance: {
            const auto* s = static_cast<const MechMotionData::mMotionDistanceSensor*>(sensor);
            for (const auto& frame : result->m_frames) {
                double value = 0.;
                if (!distanceValue(*s, frame, value) || !std::isfinite(frame.m_time))return ResultSeries();
                series.times.push_back(frame.m_time);
                series.values.push_back(value);
            }
            return series;
        }

        // --- 角度传感器 ---
        case MechMotionData::SensorType::Angle: {
            const auto* s = static_cast<const MechMotionData::mMotionAngleSensor*>(sensor);
            // 主值角度在帧序列上展开（跨 ±180° 补整圈），曲线即连续转角，
            // 与体 Rigid_Z / PID 反馈曲线可比；否则超过 180° 后整体反号
            double lastUnwrapped = 0.0;
            bool hasLast = false;
            for (const auto& frame : result->m_frames) {
                const auto* bfA = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyA));
                const auto* bfB = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyB));
                if ((mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyA) >= 0 && !bfA) || (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionBodyB) >= 0 && !bfB))
                    return ResultSeries();

                const QQuaternion qA = bfA ? bfA->m_rotation : QQuaternion();
                const QQuaternion qB = bfB ? bfB->m_rotation : QQuaternion();
                if (!finiteRotation(qA) || !finiteRotation(qB))return ResultSeries();
                const QQuaternion qRel = qA * qB.conjugated();

                QVector3D axis = s->getAxisDirection();
                if (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody) >= 0) {
                    const auto* axisFrame = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
                    if (!axisFrame)
                        return ResultSeries();
                    axis = axisFrame->m_rotation.rotatedVector(axis);
                }

                if (!finiteVector(axis) || axis.lengthSquared() < 1e-20f || !std::isfinite(frame.m_time))return ResultSeries();
                const double raw = mMotionSensorEvaluator::signedAngleAboutAxis(qRel, axis);
                if (!std::isfinite(raw))return ResultSeries();
                const double unwrapped = mMotionSensorEvaluator::unwrapAngle(raw, lastUnwrapped, hasLast);
                lastUnwrapped = unwrapped;
                hasLast = true;

                series.times.push_back(frame.m_time);
                series.values.push_back(qRadiansToDegrees(unwrapped));
            }
            return series;
        }

        default:
            return ResultSeries();
        }
    }

    double mMotionResultQueryService::getMarkerValue(
        const MechMotionData::mMotionMarkerFrame& frame,
        MechMotionData::CurveVariable variable,
        MechMotionData::CurveComponent component)
    {
        switch (variable) {
        case MechMotionData::CurveVariable::Position:
            return getVectorValue(frame.m_worldPosition, component);
        case MechMotionData::CurveVariable::Velocity:
            return getVectorValue(frame.m_linearVelocity, component);
        case MechMotionData::CurveVariable::Acceleration:
            return getVectorValue(frame.m_linearAcceleration, component);
        case MechMotionData::CurveVariable::Rotation:
            if (!finiteRotation(frame.m_worldRotation))return unavailableValue();
            return getVectorValue(frame.m_worldRotation.toEulerAngles(), component);
        case MechMotionData::CurveVariable::AngularVelocity:
            // 引擎记录为 rad/s，与 Rotation（度）统一：曲线输出 °/s
            return getVectorValue(frame.m_angularVelocity * 57.29577951308232, component);
        case MechMotionData::CurveVariable::AngularAcceleration:
            return getVectorValue(frame.m_angularAcceleration * 57.29577951308232, component);
        default:
            return unavailableValue();
        }
    }

    double mMotionResultQueryService::getVectorValue(
        const QVector3D& vector,
        MechMotionData::CurveComponent component)
    {
        if (!finiteVector(vector))return unavailableValue();
        switch (component) {
        case MechMotionData::CurveComponent::X:
            return vector.x();
        case MechMotionData::CurveComponent::Y:
            return vector.y();
        case MechMotionData::CurveComponent::Z:
            return vector.z();
        case MechMotionData::CurveComponent::Magnitude:
            return vector.length();
        default:
            return unavailableValue();
        }
    }

    bool mMotionResultQueryService::appendFrameValue(
        const MechMotionData::mMotionFrame& frame,
        const MechMotionData::mMotionCurveRequest& request,
        double& value)
    {
        value = 0.0;
        switch (request.m_sourceType) {
        case MechMotionData::CurveSourceType::Marker: {
            const auto item = frame.m_markerFrames.find(request.m_sourceId);
            if (item == frame.m_markerFrames.end())
                return false;
            value = getMarkerValue(item->second, request.m_variable, request.m_component);
            return true;
        }
        case MechMotionData::CurveSourceType::Body: {
            const auto item = frame.m_bodyFrames.find(request.m_sourceId);
            if (item == frame.m_bodyFrames.end())
                return false;
            const auto& body = item->second;
            switch (request.m_variable) {
            case MechMotionData::CurveVariable::Position:
                value = getVectorValue(body.m_position, request.m_component);
                return true;
            case MechMotionData::CurveVariable::Velocity:
                value = getVectorValue(body.m_velocity, request.m_component);
                return true;
            case MechMotionData::CurveVariable::Acceleration:
                value = getVectorValue(body.m_acceleration, request.m_component);
                return true;
            case MechMotionData::CurveVariable::Rotation:
                if (!finiteRotation(body.m_rotation))return false;
                value = getVectorValue(body.m_rotation.toEulerAngles(), request.m_component);
                return true;
            case MechMotionData::CurveVariable::AngularVelocity:
                value = getVectorValue(body.m_angularVelocity * 57.29577951308232, request.m_component);
                return true;
            case MechMotionData::CurveVariable::AngularAcceleration:
                value = getVectorValue(body.m_angularAcceleration * 57.29577951308232, request.m_component);
                return true;
            case MechMotionData::CurveVariable::Displacement:
                value = getVectorValue(body.m_displacement, request.m_component);
                return true;
            case MechMotionData::CurveVariable::PotentialEnergy:
                value = body.m_potentialEnergy;
                return true;
            case MechMotionData::CurveVariable::KineticEnergy:
                value = body.m_kineticEnergy;
                return true;
            default:
                return false;
            }
        }
        case MechMotionData::CurveSourceType::Joint:
        case MechMotionData::CurveSourceType::Coupling: {
            const auto& source = request.m_sourceType == MechMotionData::CurveSourceType::Joint
                ? frame.m_jointFrames
                : frame.m_couplingFrames;
            const auto item = source.find(request.m_sourceId);
            if (item == source.end())
                return false;
            const auto& joint = item->second;
            switch (request.m_variable) {
            case MechMotionData::CurveVariable::Position:
            case MechMotionData::CurveVariable::Velocity:
            case MechMotionData::CurveVariable::Acceleration:
            case MechMotionData::CurveVariable::Rotation:
            case MechMotionData::CurveVariable::AngularVelocity:
            case MechMotionData::CurveVariable::AngularAcceleration:
                value = getMarkerValue(joint.m_nodeI, request.m_variable, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ReactionForce:
                value = getVectorValue(joint.m_reactionForce, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ReactionTorque:
                value = getVectorValue(joint.m_reactionTorque, request.m_component);
                return true;
            default:
                return false;
            }
        }
        case MechMotionData::CurveSourceType::Drive: {
            const auto item = frame.m_driveFrames.find(request.m_sourceId);
            if (item == frame.m_driveFrames.end())
                return false;
            const auto& drive = item->second;
            switch (request.m_variable) {
            case MechMotionData::CurveVariable::Position:
            case MechMotionData::CurveVariable::Velocity:
            case MechMotionData::CurveVariable::Acceleration:
            case MechMotionData::CurveVariable::Rotation:
            case MechMotionData::CurveVariable::AngularVelocity:
            case MechMotionData::CurveVariable::AngularAcceleration:
                value = getMarkerValue(drive.m_absCoord, request.m_variable, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ImposedValue:
                value = drive.m_imposedValue;
                return true;
            case MechMotionData::CurveVariable::ImposedSpeed:
                value = drive.m_imposedSpeed;
                return true;
            case MechMotionData::CurveVariable::ImposedAcc:
                value = drive.m_imposedAcc;
                return true;
            case MechMotionData::CurveVariable::ActuationForce:
                value = getVectorValue(drive.m_actuationForce, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ActuationTorque:
                value = getVectorValue(drive.m_actuationTorque, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ReactionForce:
                value = getVectorValue(drive.m_reactionForce, request.m_component);
                return true;
            case MechMotionData::CurveVariable::ReactionTorque:
                value = getVectorValue(drive.m_reactionTorque, request.m_component);
                return true;
            default:
                return false;
            }
        }
        default:
            return false;
        }
    }

    void mMotionResultQueryService::interpolateBodyMatrix(
        const QMatrix4x4& first,
        const QMatrix4x4& second,
        double ratio,
        QMatrix4x4& matrix)
    {
        if (ratio <= 0.001) {
            matrix = first;
            return;
        }
        if (ratio >= 0.999) {
            matrix = second;
            return;
        }

        const auto blend = static_cast<float>(ratio);
        const QVector3D position = first.column(3).toVector3D() +
            (second.column(3).toVector3D() - first.column(3).toVector3D()) * blend;
        const QQuaternion rotation = QQuaternion::slerp(
            QQuaternion::fromRotationMatrix(first.normalMatrix()),
            QQuaternion::fromRotationMatrix(second.normalMatrix()),
            blend);

        matrix.setToIdentity();
        matrix.translate(position);
        matrix.rotate(rotation);
    }
}
