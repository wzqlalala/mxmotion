#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>
#include <QVector>

#include "MXMotionData_global.h"
#include "mModelStruct.h"
#include "mMotionForceElement.h"

namespace MechMotionData {

    /**
     * @brief 矢量载荷类型枚举
     */
    /**
     * @brief 方向模式枚举
     * @details 决定载荷方向是固定于全局坐标系还是随作用体转动
     */
    enum class MXMOTIONDATA_EXPORT DirectionMode {
        GlobalFixed,  ///< 方向在全局坐标系中固定（Space Fixed）
        BodyFixed     ///< 方向相对于作用体固定（Body Fixed）
    };

    /**
     * @brief 大小模式枚举
     */
    enum class MXMOTIONDATA_EXPORT MagnitudeMode {
        Constant,     ///< 常量
        Expression    ///< 表达式（函数）
    };

    /**
     * @brief 矢量载荷（矢量力 / 矢量力矩）数据模型
     * @details 对应 UG Motion 中的 Vector Force / Vector Torque
     */
    class MXMOTIONDATA_EXPORT mMotionVectorForce : public mMotionForceElement {
    public:
        DirectionPickState dirPickState_;               ///< 方向输入的拾取会话状态
        mMotionVectorForce(int id, VectorLoadType loadType, const QString& name = "");
        mMotionVectorForce();
        ~mMotionVectorForce() override;

        // --- 基础标识（来自基类） ---
        using mMotionForceElement::setId;
        using mMotionForceElement::getId;
        using mMotionForceElement::setName;
        using mMotionForceElement::getName;

        void setLoadType(VectorLoadType type);
        VectorLoadType getLoadType() const;

        // --- 作用对象 ---

        // --- 作用点（相对于作用体局部坐标系）---
        void setActionPoint(const QVector3D& point);
        QVector3D getActionPoint() const;

        // --- 方向向量（未归一化存储，使用时需归一化）---
        void setDirection(const QVector3D& dir);
        QVector3D getDirection() const;

        // --- 大小数值 ---
        void setMagnitude(double magnitude);
        double getMagnitude() const;

        // --- 表达式 ---
        void setExpression(const QString& expr);
        QString getExpression() const;

        // --- 大小模式 ---
        void setMagnitudeMode(MagnitudeMode mode);
        MagnitudeMode getMagnitudeMode() const;

        // --- 方向模式 ---
        void setDirectionMode(DirectionMode mode);
        DirectionMode getDirectionMode() const;

        // --- 辅助方法 ---
        /**
         * @brief 获取载荷类型的中文名称
         */
        QString getLoadTypeName() const;

        /**
         * @brief 静态辅助函数：将枚举类型转换为中文名称
         */
        static QString getLoadTypeName(VectorLoadType type);

        /**
         * @brief 复制当前载荷对象（多态，子类重写以返回对应派生类型）
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的 mMotionVectorForce 实例
         */
        virtual mMotionVectorForce* copyForce(int newId, const QString& newName);
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        using mMotionForceElement::m_id;
        using mMotionForceElement::m_name;

        friend QDataStream& operator>>(QDataStream& in, mMotionVectorForce& force);
        friend QDataStream& operator<<(QDataStream& out, mMotionVectorForce& force);

    public:
        VectorLoadType m_loadType;      ///< 载荷类型（力/力矩）

        QVector3D m_actionPoint;        ///< 作用点在体局部坐标系下的位置
        QVector3D m_direction;          ///< 方向向量

        double m_magnitude;             ///< 常量大小（单位：力 N 或 力矩 N·mm）
        QString m_expression;           ///< 表达式字符串（支持 time 变量）
        MagnitudeMode m_magnitudeMode;  ///< 大小模式
        DirectionMode m_directionMode;  ///< 方向模式
    };

}
