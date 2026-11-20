#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>

#include "MXMotionData_global.h"
#include "mModelStruct.h"
#include "mMotionForceElement.h"

namespace MechMotionData {

    /**
     * @brief 标量力矩轴模式枚举
     * @details 对应求解器 ADMScalarTorque 的 axis_mode
     *          0 = 两点连线方向（line）
     *          1 = 全局固定方向（fixed abs）
     *          2 = 作用体局部固定方向（fixed local A）
     *          3 = 基本体局部固定方向（fixed local B）
     */
    enum class MXMOTIONDATA_EXPORT ScalarTorqueAxisMode {
        Line = 0,        ///< 两点连线方向
        FixedGlobal = 1, ///< 全局固定方向
        FixedLocalA = 2, ///< 作用体（marker_I 所在体）局部固定方向
        FixedLocalB = 3  ///< 基本体（marker_J 所在体）局部固定方向
    };

    /**
     * @brief 标量力矩数据类
     * @details 对应求解器 ADMScalarTorque，两端分别作用在两个运动体上的局部点，
     *          力矩大小由常量 magnitude 或函数表达式决定，绕轴模式指定的方向施加。
     */
    class MXMOTIONDATA_EXPORT mMotionScalarTorque : public mMotionForceElement {
    public:
        DirectionPickState fixedAxisPickState_;         ///< 固定轴输入的拾取会话状态
        mMotionScalarTorque(int id, const QString& name = "");
        mMotionScalarTorque();
        ~mMotionScalarTorque() override;

        // --- 基础标识（来自基类） ---
        using mMotionForceElement::setId;
        using mMotionForceElement::getId;
        using mMotionForceElement::setName;
        using mMotionForceElement::getName;

        VectorLoadType getLoadType() const override { return VectorLoadType::ScalarTorque; }

        // --- 作用体 I（动作侧） ---

        void setLocalI(const QVector3D& pos);
        QVector3D getLocalI() const;

        // --- 作用体 J（基本侧，-1 表示大地） ---

        void setLocalJ(const QVector3D& pos);
        QVector3D getLocalJ() const;

        // --- 轴模式与固定轴 ---
        void setAxisMode(ScalarTorqueAxisMode mode);
        ScalarTorqueAxisMode getAxisMode() const;

        void setFixedAxis(const QVector3D& axis);
        QVector3D getFixedAxis() const;

        // --- 常量力矩幅值 ---
        void setMagnitude(double magnitude);
        double getMagnitude() const;

        // --- 力矩函数表达式（可选） ---
        void setTorqueFunction(const QString& expr);
        QString getTorqueFunction() const;

        // --- 仅作用在基本体 ---
        void setActionOnly(bool actionOnly);
        bool getActionOnly() const;

        /**
         * @brief 复制当前标量力矩对象
         */
        virtual mMotionScalarTorque* copyScalarTorque(int newId, const QString& newName);
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        using mMotionForceElement::m_id;
        using mMotionForceElement::m_name;

        friend QDataStream& operator>>(QDataStream& in, mMotionScalarTorque& torque);
        friend QDataStream& operator<<(QDataStream& out, mMotionScalarTorque& torque);

    public:

        QVector3D m_localI;                 ///< 作用点 I 在动作体局部坐标系下的位置

        QVector3D m_localJ;                 ///< 作用点 J 在基本体局部坐标系下的位置

        ScalarTorqueAxisMode m_axisMode;    ///< 轴模式
        QVector3D m_fixedAxis;              ///< 固定轴向量（未归一化存储）

        double m_magnitude;                 ///< 常量力矩幅值

        QString m_torqueFunction;           ///< 力矩函数表达式（可选）
        bool m_actionOnly;                  ///< 是否仅作用在基本体
    };

}
