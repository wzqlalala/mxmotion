#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>
#include <QDataStream>
#include <QColor>
#include <QJsonObject>
#include "MXMotionData_global.h"

namespace MechMotionData {

    /**
     * @brief 曲线数据源类型
     */
    enum class CurveSourceType {
        Marker = 0,     ///< 标记点
        Body = 1,       ///< 运动体
        Joint = 2,      ///< 运动副
        Drive = 3,      ///< 驱动
        Coupling = 4,   ///< 耦合副
        Sensor = 5,     ///< 传感器（值由传感器定义决定，查询期求值）
        Controller = 6, ///< 控制器（求解期记录的 PID 历史）
        // Appended values keep the original stored enum ordinals unchanged.
        Spring = 7,
        SoftStop = 8,
        TensionCable = 9,
        VectorForce = 10,
        ScalarForce = 11,
        ScalarTorque = 12,
        ThreePointForce = 13,
        ThreePointTorque = 14,
        AutoFlexBody = 15,
        NativeChannel = 16
    };

    /**
     * @brief 可提取的物理变量
     * @details 不同源类型支持不同变量子集
     */
    enum class CurveVariable {
        Position = 0,           ///< 位置
        Velocity = 1,           ///< 速度
        Acceleration = 2,       ///< 加速度
        Rotation = 3,           ///< 方向/角度（标量幅值或分量）
        AngularVelocity = 4,    ///< 角速度
        AngularAcceleration = 5,///< 角加速度
        Displacement = 6,       ///< 位移增量
        PotentialEnergy = 7,    ///< 势能（仅Body）
        ReactionForce = 8,      ///< 约束反力（仅Joint/Drive）
        ReactionTorque = 9,     ///< 约束反力矩（仅Joint/Drive）
        ActuationForce = 10,    ///< 驱动力（仅Drive）
        ActuationTorque = 11,   ///< 驱动力矩（仅Drive）
        ImposedValue = 12,      ///< 驱动位移（仅Drive）
        ImposedSpeed = 13,      ///< 驱动速度（仅Drive）
        ImposedAcc = 14,        ///< 驱动加速度（仅Drive）
        SensorValue = 15,       ///< 传感器值（仅Sensor，变量在传感器定义里选）
        PidTarget = 16,         ///< PID 目标值（仅Controller）
        PidFeedback = 17,       ///< PID 反馈值（仅Controller）
        PidError = 18,          ///< PID 误差（仅Controller）
        PidOutput = 19,         ///< PID 输出（仅Controller）
        KineticEnergy = 20,     ///< 动能（仅Body）
        NativeChannel = 21     ///< Recorded channel named by m_nativeVariable.
    };

    /**
     * @brief 分量类型
     */
    enum class CurveComponent {
        X = 0,
        Y = 1,
        Z = 2,
        Magnitude = 3
    };

    /**
     * @brief 单条曲线请求
     * @details 描述用户想要查看的曲线：哪个对象的哪个变量的哪个分量
     */
    struct MXMOTIONDATA_EXPORT mMotionCurveRequest {
        int m_requestId = -1;           ///< 请求唯一ID
        QString m_requestName;          ///< 请求名称（用户可编辑，用于图例显示）

        CurveSourceType m_sourceType = CurveSourceType::Marker;   ///< 源类型
        int m_sourceId = -1;            ///< 源对象ID
        QString m_sourceName;           ///< 源对象名称（缓存，用于显示）

        CurveVariable m_variable = CurveVariable::Position;       ///< 变量
        CurveComponent m_component = CurveComponent::X;           ///< 分量

        QColor m_color;                     ///< 曲线颜色（无效色=显示时自动分配）
        bool m_visible = true;              ///< 曲线可见性

        // Native channels extend the original request without narrowing newer
        // force/flexible variables to a legacy enum or losing JSON extensions.
        // The old enum members above remain usable by every original caller.
        QString m_nativeSource;
        QString m_nativeVariable;
        int m_nativeComponent = 0;           ///< -1=magnitude, otherwise exact component index.
        QString m_nativeVariableLabel;      ///< UI label from the recorded channel catalogue.
        QJsonObject m_originalJson;          ///< Preserve schema and unknown per-request fields.

        bool isNativeChannel() const { return !m_nativeSource.isEmpty() && !m_nativeVariable.isEmpty(); }
        static CurveSourceType sourceTypeFromName(const QString& name);
        static QString collectionForSource(CurveSourceType type);
        static CurveVariable variableFromName(const QString& name);
        static QString nameForVariable(CurveVariable variable);

        mMotionCurveRequest() = default;
        ~mMotionCurveRequest() = default;

        // --- 便捷方法 ---
        QString getDefaultName() const;
        QString getVariableLabel() const;
        QString getComponentLabel() const;
        QString getFullLabel() const;

        friend inline QDataStream& operator>>(QDataStream& in, mMotionCurveRequest& req) {
            int sourceTypeInt = 0, varInt = 0, compInt = 0;
            in >> req.m_requestId;
            in >> req.m_requestName;
            in >> sourceTypeInt;
            in >> req.m_sourceId;
            in >> req.m_sourceName;
            in >> varInt;
            in >> compInt;
            req.m_sourceType = static_cast<CurveSourceType>(sourceTypeInt);
            req.m_variable = static_cast<CurveVariable>(varInt);
            req.m_component = static_cast<CurveComponent>(compInt);
            return in;
        }

        friend inline QDataStream& operator<<(QDataStream& out, const mMotionCurveRequest& req) {
            out << req.m_requestId;
            out << req.m_requestName;
            out << static_cast<int>(req.m_sourceType);
            out << req.m_sourceId;
            out << req.m_sourceName;
            out << static_cast<int>(req.m_variable);
            out << static_cast<int>(req.m_component);
            return out;
        }
    };

}
