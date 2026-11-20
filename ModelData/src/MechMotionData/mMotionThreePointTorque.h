#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>

#include "MXMotionData_global.h"
#include "mMotionForceElement.h"

namespace MechMotionData {

    /**
     * @brief 三点力矩数据类（UG NX Three Point Torque）
     * @details 力矩的施加位置与轴定义分离：
     *          - 应用点（动作体 + 局部点）：力矩实际施加的位置
     *          - 目标点（体 + 局部点）：轴连线终点
     *          - 出发点（体 + 局部点）：轴连线起点
     *          力矩轴 = 出发点→目标点的实时连线（随两参考体运动实时变化），
     *          力矩施加在应用体上。
     */
    class MXMOTIONDATA_EXPORT mMotionThreePointTorque : public mMotionForceElement {
    public:
        mMotionThreePointTorque(int id, const QString& name = "");
        mMotionThreePointTorque();
        ~mMotionThreePointTorque() override;

        // --- 基础标识（来自基类） ---
        using mMotionForceElement::setId;
        using mMotionForceElement::getId;
        using mMotionForceElement::setName;
        using mMotionForceElement::getName;

        VectorLoadType getLoadType() const override { return VectorLoadType::TriTorque; }

        // --- 应用点（力矩施加位置，动作体） ---

        void setLocalI(const QVector3D& pos);
        QVector3D getLocalI() const;

        // --- 目标点（轴连线终点，-1 表示大地） ---

        void setLocalTarget(const QVector3D& pos);
        QVector3D getLocalTarget() const;

        // --- 出发点（轴连线起点，-1 表示大地） ---

        void setLocalOrigin(const QVector3D& pos);
        QVector3D getLocalOrigin() const;

        // --- 常量力矩幅值 ---
        void setMagnitude(double magnitude);
        double getMagnitude() const;

        // --- 力矩函数表达式（可选，T = f(t)） ---
        void setTorqueFunction(const QString& expr);
        QString getTorqueFunction() const;

        /**
         * @brief 复制当前三点力矩对象
         */
        mMotionThreePointTorque* copyThreePointTorque(int newId, const QString& newName);
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        using mMotionForceElement::m_id;
        using mMotionForceElement::m_name;

        friend QDataStream& operator>>(QDataStream& in, mMotionThreePointTorque& torque);
        friend QDataStream& operator<<(QDataStream& out, mMotionThreePointTorque& torque);

    public:

        QVector3D m_localI;                 ///< 应用点在应用体局部坐标系下的位置

        QVector3D m_localTarget;            ///< 目标点在目标体局部坐标系下的位置

        QVector3D m_localOrigin;            ///< 出发点在出发体局部坐标系下的位置

        double m_magnitude;                 ///< 常量力矩幅值

        QString m_torqueFunction;           ///< 力矩函数表达式（可选）
    };

} // namespace MechMotionData
