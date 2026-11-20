#include "mMotionSensorEvaluator.h"

#include <QtMath>

#include <cmath>

namespace MechMainWindow
{
    QVector3D mMotionSensorEvaluator::pointPosition(
        const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos)
    {
        // p = p_body + R·local
        return b.m_position + b.m_rotation.rotatedVector(localPos);
    }

    QVector3D mMotionSensorEvaluator::pointVelocity(
        const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos)
    {
        // v = v_body + ω × r,  r = R·local
        const QVector3D r = b.m_rotation.rotatedVector(localPos);
        return b.m_velocity + QVector3D::crossProduct(b.m_angularVelocity, r);
    }

    QVector3D mMotionSensorEvaluator::pointAcceleration(
        const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos)
    {
        // a = a_body + α × r + ω × (ω × r)
        const QVector3D r = b.m_rotation.rotatedVector(localPos);
        return b.m_acceleration
            + QVector3D::crossProduct(b.m_angularAcceleration, r)
            + QVector3D::crossProduct(b.m_angularVelocity, QVector3D::crossProduct(b.m_angularVelocity, r));
    }

    const MechMotionData::mMotionBodyFrame* mMotionSensorEvaluator::bodyFrameAt(
        const MechMotionData::mMotionFrame& frame, int bodyId)
    {
        if (bodyId < 0)
            return nullptr;
        const auto it = frame.m_bodyFrames.find(bodyId);
        return it != frame.m_bodyFrames.end() ? &it->second : nullptr;
    }

    QVector3D mMotionSensorEvaluator::toCoordSystem(
        const QVector3D& worldVec, const MechMotionData::mMotionFrame& frame,
        MechMotionData::CoordSystem coordSystem, int referenceBodyId)
    {
        if (coordSystem != MechMotionData::CoordSystem::BodyFixed)
            return worldVec;

        const MechMotionData::mMotionBodyFrame* refFrame = bodyFrameAt(frame, referenceBodyId);
        if (!refFrame)
            return worldVec;

        return refFrame->m_rotation.conjugated().rotatedVector(worldVec);
    }

    double mMotionSensorEvaluator::signedAngleAboutAxis(const QQuaternion& qRel, const QVector3D& axis)
    {
        // θ = 2·atan2( axis·v, w )，v 为四元数矢量部分，不可归一化：
        // |v| = |sin(θ/2)| 携带角度信息，归一化会使半角恒等式失效（微小角被跳读到 ±90° 带）
        const QVector3D n = axis.normalized();
        const QVector3D v(qRel.x(), qRel.y(), qRel.z());
        const double vLen = static_cast<double>(v.length());
        if (vLen < 1e-12)
            return 0.0;
        return 2.0 * std::atan2(
            static_cast<double>(QVector3D::dotProduct(n, v)),
            static_cast<double>(qRel.scalar()));
    }

    double mMotionSensorEvaluator::unwrapAngle(double rawAngle, double lastAngle, bool lastValid)
    {
        // 主值角度在 (-π, π]，跨 ±180° 时跳变 2π；展开即选取与上一次角度最接近的等价角
        if (!lastValid)
            return rawAngle;
        constexpr double kTwoPi = 6.2831853071795864769;
        return rawAngle + kTwoPi * std::floor((lastAngle - rawAngle) / kTwoPi + 0.5);
    }
}
