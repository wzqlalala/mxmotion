#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include <QString>
#include <QVector>
#include <QVector3D>

namespace MechMotionData {

    /**
     * @brief 传感器类型枚举
     */
    enum class MXMOTIONDATA_EXPORT SensorType {
        BodyDisplacement,       ///< 体位移传感器
        BodyVelocity,           ///< 体速度传感器
        BodyAcceleration,       ///< 体加速度传感器
        JointReaction,          ///< 关节反力传感器
        DriveActuation,         ///< 驱动输出传感器
        Distance,               ///< 距离传感器
        Angle                   ///< 角度传感器
    };

    /**
     * @brief 坐标系类型枚举
     */
    enum class MXMOTIONDATA_EXPORT CoordSystem {
        GlobalFixed,            ///< 全局固定坐标系
        BodyFixed               ///< 体固定坐标系（随体转动）
    };

    /**
     * @brief 分量输出模式枚举
     */
    enum class MXMOTIONDATA_EXPORT ComponentMode {
        X = 0,                  ///< X分量
        Y = 1,                  ///< Y分量
        Z = 2,                  ///< Z分量
        Magnitude = 3,          ///< 幅值
        All = 4                 ///< 全部分量（X/Y/Z/Magnitude）
    };

    /**
     * @brief 体传感器可测量变量
     */
    enum class MXMOTIONDATA_EXPORT BodySensorVariable {
        Position,               ///< 位置
        Velocity,               ///< 速度
        Acceleration,           ///< 加速度
        AngularVelocity,        ///< 角速度
        AngularAcceleration,    ///< 角加速度
        Rotation                ///< 方向/角度
    };

    /**
     * @brief 关节传感器可测量变量
     */
    enum class MXMOTIONDATA_EXPORT JointSensorVariable {
        ReactionForce,          ///< 约束反力
        ReactionTorque          ///< 约束反力矩
    };

    /**
     * @brief 驱动传感器可测量变量
     */
    enum class MXMOTIONDATA_EXPORT DriveSensorVariable {
        ActuationForce,         ///< 驱动力
        ActuationTorque,        ///< 驱动力矩
        ImposedValue,           ///< 驱动位移
        ImposedSpeed,           ///< 驱动速度
        ImposedAcc,             ///< 驱动加速度
        ReactionForce,          ///< 约束反力
        ReactionTorque          ///< 约束反力矩
    };

    /**
     * @brief 距离传感器可测量变量
     */
    enum class MXMOTIONDATA_EXPORT DistanceVariable {
        Distance,               ///< 距离
        RelVelocity,            ///< 相对速度
        RelAcceleration         ///< 相对加速度
    };

    /**
     * @brief 传感器点引用（用于距离/角度传感器）
     */
    struct MXMOTIONDATA_EXPORT SensorPointRef {
        int bodyId = -1;                ///< 运动体ID（-1表示全局/大地）
        QVector3D localPos;             ///< 相对于体局部坐标系的位置

        bool operator==(const SensorPointRef& other) const {
            return bodyId == other.bodyId && localPos == other.localPos;
        }
    };

    // --- 静态辅助函数 ---

    /**
     * @brief 获取传感器类型的中文名称
     */
    MXMOTIONDATA_EXPORT QString getSensorTypeName(SensorType type);

    /**
     * @brief 获取坐标系类型的中文名称
     */
    MXMOTIONDATA_EXPORT QString getCoordSystemName(CoordSystem coord);

    /**
     * @brief 获取分量模式的中文名称
     */
    MXMOTIONDATA_EXPORT QString getComponentModeName(ComponentMode mode);

    /**
     * @brief 获取体传感器变量的中文名称
     */
    MXMOTIONDATA_EXPORT QString getBodySensorVariableName(BodySensorVariable var);

    /**
     * @brief 获取关节传感器变量的中文名称
     */
    MXMOTIONDATA_EXPORT QString getJointSensorVariableName(JointSensorVariable var);

    /**
     * @brief 获取驱动传感器变量的中文名称
     */
    MXMOTIONDATA_EXPORT QString getDriveSensorVariableName(DriveSensorVariable var);

    /**
     * @brief 获取距离传感器变量的中文名称
     */
    MXMOTIONDATA_EXPORT QString getDistanceVariableName(DistanceVariable var);

    /**
     * @brief 体传感器按类型可选的变量（类型=大类：位移类/速度类/加速度类，各含线量与角量）
     * @details 体位移:{位置,方向} 体速度:{速度,角速度} 体加速度:{加速度,角加速度}
     */
    MXMOTIONDATA_EXPORT QVector<BodySensorVariable> allowedBodyVariables(SensorType type);

    /**
     * @brief 校正历史数据中类型与变量不匹配的组合：按变量意图重新定型
     * @details 例：体位移类型 + 速度变量 → 体速度类型 + 速度变量
     */
    MXMOTIONDATA_EXPORT void normalizeBodySensor(SensorType& type, BodySensorVariable& variable);

}
