#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mMotionResultData.h"
#include "mMotionSensorType.h"

#include <QVector3D>
#include <QQuaternion>

namespace MechMainWindow
{
    /**
     * @brief 传感器查询期求值工具（纯 GUI 后处理）
     * @details 只依赖结果帧数据与传感器定义，不涉及求解器；全部为纯函数，
     *          Phase 2 求解期求值（阈值停止/闭环控制）可直接复用。
     */
    class MMAINWINDOW_EXPORT mMotionSensorEvaluator
    {
    public:
        // --- 体上参考点运动学（b 的局部偏移 localPos 处的世界量） ---
        static QVector3D pointPosition(const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos);
        static QVector3D pointVelocity(const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos);
        static QVector3D pointAcceleration(const MechMotionData::mMotionBodyFrame& b, const QVector3D& localPos);

        // 从帧取体帧；bodyId<0 或不存在返回 nullptr
        static const MechMotionData::mMotionBodyFrame* bodyFrameAt(
            const MechMotionData::mMotionFrame& frame, int bodyId);

        // 坐标系变换：GlobalFixed 原样返回；BodyFixed 用参考体帧 rotation 的共轭旋转 worldVec
        // （参考体无效或为全局时按全局处理）
        static QVector3D toCoordSystem(const QVector3D& worldVec, const MechMotionData::mMotionFrame& frame,
            MechMotionData::CoordSystem coordSystem, int referenceBodyId);

        // qRel 绕单位轴 axis 的带符号转角（弧度）；旋转向量模长接近 0 时返回 0
        // 返回值在 (-π, π]：跨 ±180° 会回绕，连续角度请用 unwrapAngle 展开
        static double signedAngleAboutAxis(const QQuaternion& qRel, const QVector3D& axis);

        // 将当前主值角度展开到上一次角度附近（补 ±2π 的整数倍），返回展开值
        // lastValid 为 false 时 lastAngle 无效，直接返回 rawAngle
        static double unwrapAngle(double rawAngle, double lastAngle, bool lastValid);
    };
}
