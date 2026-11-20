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

namespace MechMainWindow
{
    mMotionResultQueryService::FrameSpan mMotionResultQueryService::locateTime(
        const MechMotionData::mMotionResultData* result,
        double time,
        bool clampToRange)
    {
        FrameSpan span;
        if (!result || result->m_frames.empty())
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
            if (marker != frame.m_markerFrames.end())
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
        const bool unwrapRotation = request.m_variable == MechMotionData::CurveVariable::Rotation;
        double lastUnwrapped = 0.0;
        bool hasLast = false;

        for (const auto& frame : result->m_frames) {
            double value = 0.0;
            if (!appendFrameValue(frame, request, value))
                continue;

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
                continue;

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

        auto vectorValue = [component](const QVector3D& v) -> double {
            switch (component) {
            case CurveComponent::X:  return v.x();
            case CurveComponent::Y:  return v.y();
            case CurveComponent::Z:  return v.z();
            default:                 return v.length();
            }
        };
        auto coordVec = [&sensor](const QVector3D& worldVec, const mMotionFrame& f) {
            return mMotionSensorEvaluator::toCoordSystem(
                worldVec, f, sensor->getCoordSystem(), mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody));
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
                ok = true;
                return vectorValue(bf->m_rotation.toEulerAngles());
            default:
                return 0.0;
            }
        }

        case SensorType::Distance: {
            const auto* s = static_cast<const mMotionDistanceSensor*>(sensor);
            // 相对速度/加速度需要序列差分，不适合单帧求值
            if (s->getVariable() != DistanceVariable::Distance)
                return 0.0;
            const auto* bfA = mMotionSensorEvaluator::bodyFrameAt(frame, s->getPointA().bodyId);
            const auto* bfB = mMotionSensorEvaluator::bodyFrameAt(frame, s->getPointB().bodyId);
            if ((s->getPointA().bodyId >= 0 && !bfA) || (s->getPointB().bodyId >= 0 && !bfB))
                return 0.0;
            const QVector3D pA = bfA
                ? mMotionSensorEvaluator::pointPosition(*bfA, s->getPointA().localPos)
                : s->getPointA().localPos;
            const QVector3D pB = bfB
                ? mMotionSensorEvaluator::pointPosition(*bfB, s->getPointB().localPos)
                : s->getPointB().localPos;
            ok = true;
            return static_cast<double>((pB - pA).length());
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
            const QQuaternion qRel = qB.conjugated() * qA;
            QVector3D axis = s->getAxisDirection();
            if (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody) >= 0) {
                const auto* axisFrame = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
                if (!axisFrame)
                    return 0.0;
                axis = axisFrame->m_rotation.rotatedVector(axis);
            }
            ok = true;
            return qRadiansToDegrees(mMotionSensorEvaluator::signedAngleAboutAxis(qRel, axis));
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
            switch (component) {
            case MechMotionData::CurveComponent::X:  return v.x();
            case MechMotionData::CurveComponent::Y:  return v.y();
            case MechMotionData::CurveComponent::Z:  return v.z();
            default:                                  return v.length();
            }
        };
        // 坐标系变换仅作用于矢量量
        auto coordVec = [&sensor](const QVector3D& worldVec, const MechMotionData::mMotionFrame& frame) {
            return mMotionSensorEvaluator::toCoordSystem(
                worldVec, frame, sensor->getCoordSystem(), mMotionRefMap::get(Physical_MotionSensor, sensor->getId(), Physical_MotionBody, MapRelation_MotionReferenceBody));
        };

        switch (sensor->getType()) {
        // --- 体传感器（位移/速度/加速度三类共用体帧数据） ---
        case MechMotionData::SensorType::BodyDisplacement:
        case MechMotionData::SensorType::BodyVelocity:
        case MechMotionData::SensorType::BodyAcceleration: {
            const auto* s = static_cast<const MechMotionData::mMotionBodySensor*>(sensor);
            const bool useRefPoint = !s->getReferencePoint().isNull();
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
                    value = vectorValue(bf->m_rotation.toEulerAngles());
                    break;
                }
                default:
                    return ResultSeries();
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

                series.times.push_back(frame.m_time);
                series.values.push_back(vectorValue(coordVec(world, frame)));
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

                series.times.push_back(frame.m_time);
                series.values.push_back(value);
            }
            return series;
        }

        // --- 距离传感器（相对速度/加速度对距离序列做中心差分） ---
        case MechMotionData::SensorType::Distance: {
            const auto* s = static_cast<const MechMotionData::mMotionDistanceSensor*>(sensor);
            QVector<double> times, dists;
            for (const auto& frame : result->m_frames) {
                const auto* bfA = mMotionSensorEvaluator::bodyFrameAt(frame, s->getPointA().bodyId);
                const auto* bfB = mMotionSensorEvaluator::bodyFrameAt(frame, s->getPointB().bodyId);
                if ((s->getPointA().bodyId >= 0 && !bfA) || (s->getPointB().bodyId >= 0 && !bfB))
                    return ResultSeries();

                // 大地点（bodyId=-1）世界坐标即 localPos，静止
                const QVector3D pA = bfA
                    ? mMotionSensorEvaluator::pointPosition(*bfA, s->getPointA().localPos)
                    : s->getPointA().localPos;
                const QVector3D pB = bfB
                    ? mMotionSensorEvaluator::pointPosition(*bfB, s->getPointB().localPos)
                    : s->getPointB().localPos;

                times.push_back(frame.m_time);
                dists.push_back(static_cast<double>((pB - pA).length()));
            }

            const int n = static_cast<int>(times.size());
            if (n == 0)
                return ResultSeries();

            if (s->getVariable() == MechMotionData::DistanceVariable::Distance)
                return { times, dists };

            // 一阶中心差分（端点单侧）
            auto firstDiff = [&times](const QVector<double>& y) {
                QVector<double> out(y.size(), 0.0);
                for (int i = 0; i < static_cast<int>(y.size()); ++i) {
                    const int lo = qMax(0, i - 1), hi = qMin(static_cast<int>(y.size()) - 1, i + 1);
                    if (hi == lo)
                        continue;
                    out[i] = (y[hi] - y[lo]) / (times[hi] - times[lo]);
                }
                return out;
            };

            const QVector<double> velocities = firstDiff(dists);
            if (s->getVariable() == MechMotionData::DistanceVariable::RelVelocity)
                return { times, velocities };

            // 二阶：对速度序列再做一次同样的差分
            return { times, firstDiff(velocities) };
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
                const QQuaternion qRel = qB.conjugated() * qA;

                QVector3D axis = s->getAxisDirection();
                if (mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody) >= 0) {
                    const auto* axisFrame = mMotionSensorEvaluator::bodyFrameAt(frame, mMotionRefMap::get(Physical_MotionSensor, s->getId(), Physical_MotionBody, MapRelation_MotionAxisBody));
                    if (!axisFrame)
                        return ResultSeries();
                    axis = axisFrame->m_rotation.rotatedVector(axis);
                }

                const double raw = mMotionSensorEvaluator::signedAngleAboutAxis(qRel, axis);
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
            return getVectorValue(frame.m_worldRotation.toEulerAngles(), component);
        case MechMotionData::CurveVariable::AngularVelocity:
            // 引擎记录为 rad/s，与 Rotation（度）统一：曲线输出 °/s
            return getVectorValue(frame.m_angularVelocity * 57.29577951308232, component);
        case MechMotionData::CurveVariable::AngularAcceleration:
            return getVectorValue(frame.m_angularAcceleration * 57.29577951308232, component);
        default:
            return 0.0;
        }
    }

    double mMotionResultQueryService::getVectorValue(
        const QVector3D& vector,
        MechMotionData::CurveComponent component)
    {
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
            return 0.0;
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
