#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>

#include "MXMotionData_global.h"
#include "mMotionForceElement.h"

namespace MechMotionData {

    /**
     * @brief 三点力数据类（UG NX Three Point Force）
     * @details 力的施加位置与方向定义分离：
     *          - 应用点（动作体 + 局部点）：力实际施加的位置
     *          - 目标点（体 + 局部点）：方向连线终点
     *          - 出发点（体 + 局部点）：方向连线起点
     *          力方向 = 出发点→目标点的实时连线（随两参考体运动实时变化），
     *          力施加在应用点上，等大反向的一对力作用于应用体与参考体。
     */
    class MXMOTIONDATA_EXPORT mMotionThreePointForce : public mMotionForceElement {
    public:
        mMotionThreePointForce(int id, const QString& name = "");
        mMotionThreePointForce();
        ~mMotionThreePointForce() override;

        // --- 基础标识（来自基类） ---
        using mMotionForceElement::setId;
        using mMotionForceElement::getId;
        using mMotionForceElement::setName;
        using mMotionForceElement::getName;

        VectorLoadType getLoadType() const override { return VectorLoadType::TriForce; }

        // --- 应用点（力施加位置，动作体） ---

        void setLocalI(const QVector3D& pos);
        QVector3D getLocalI() const;

        // --- 目标点（方向连线终点，-1 表示大地） ---

        void setLocalTarget(const QVector3D& pos);
        QVector3D getLocalTarget() const;

        // --- 出发点（方向连线起点，-1 表示大地） ---

        void setLocalOrigin(const QVector3D& pos);
        QVector3D getLocalOrigin() const;

        // --- 常量力幅值 ---
        void setMagnitude(double magnitude);
        double getMagnitude() const;

        // --- 力函数表达式（可选，F = f(t)） ---
        void setForceFunction(const QString& expr);
        QString getForceFunction() const;

        /**
         * @brief 复制当前三点力对象
         */
        mMotionThreePointForce* copyThreePointForce(int newId, const QString& newName);
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        using mMotionForceElement::m_id;
        using mMotionForceElement::m_name;

        friend QDataStream& operator>>(QDataStream& in, mMotionThreePointForce& force);
        friend QDataStream& operator<<(QDataStream& out, mMotionThreePointForce& force);

    public:

        QVector3D m_localI;                 ///< 应用点在应用体局部坐标系下的位置

        QVector3D m_localTarget;            ///< 目标点在目标体局部坐标系下的位置

        QVector3D m_localOrigin;            ///< 出发点在出发体局部坐标系下的位置

        double m_magnitude;                 ///< 常量力幅值

        QString m_forceFunction;            ///< 力函数表达式（可选）
    };

} // namespace MechMotionData
