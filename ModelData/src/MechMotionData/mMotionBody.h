#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>


namespace MechMotionData {

    /**
     * @brief 运动体数据模型
     * @details 记录物理属性。绑定的部件集合经 mMapRelationManager 映射存储
     *          （{Physical_MotionBody,id} -> Physical_Part），不直存于数据类。
     */
    class MXMOTIONDATA_EXPORT mMotionBody {
    public:
        mMotionBody(int id, const QString& name = "");
        mMotionBody() {};
        ~mMotionBody();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        // --- 物理属性 ---
        void setIsFixed(bool fixed);
        bool getIsFixed() const;

        void setMass(double mass);
        double getMass() const;

        // --- 质心位置 ---
        void setCenterOfMass(const QVector3D& cm);
        QVector3D getCenterOfMass() const;

        // --- 初始线速度（全局坐标系） ---
        void setInitialVelocity(const QVector3D& vel);
        QVector3D getInitialVelocity() const;

        // --- 初始角速度（全局坐标系，界面单位 °/s，默认 0） ---
        void setInitialAngularVelocity(const QVector3D& omega);
        QVector3D getInitialAngularVelocity() const;

        // --- 质量属性输入模式 ---
        void setIsCustomMassProps(bool isCustom);
        bool getIsCustomMassProps() const;

        /**
         * @brief 设置转动惯量矩阵
         * @param inertia 长度为6的数组/向量，顺序为: [Ixx, Iyy, Izz, Ixy, Iyz, Izx]
         */
        void setInertia(const QVector<double>& inertia);
        QVector<double> getInertia() const;


        mMotionBody* copyBody(int newId, const QString& newName);

        friend QDataStream& operator>>(QDataStream& in, mMotionBody& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionBody& mgr);

    private:
        int m_id;                       ///< 运动体全局唯一ID
        QString m_name;                 ///< 运动体名称

        bool m_isCustomMassProps;   ///< 是否为用户自定义质量属性 (false: 自动计算, true: 自定义)

        bool m_isFixed;                 ///< 是否固定到大地 (Fixed to ground)
        double m_mass;                  ///< 质量 (Mass)
        QVector<double> m_inertia;      ///< 转动惯量 (Moments of inertia)
        QVector3D m_centerOfMass;   ///< 质心坐标 (Center of Mass)
        QVector3D m_initialVel;     ///< 初始线速度（全局系，默认 0）
        QVector3D m_initialOmega;   ///< 初始角速度（全局系，界面单位 °/s，默认 0）
    };

}
