#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector3D>
#include <QVector>

#include "MXMotionData_global.h"
#include "mMotionVectorForce.h"

namespace MechMotionData {

    /**
     * @brief 衬套数据类（ADAMS BUSHING / UG(NX) 常规类型）
     * @details 6 自由度黏弹性连接。继承通用矢量载荷，扩展基本体、刚度/阻尼、预载与偏置。
     *          - marker_I(动作侧)：复用基类 m_actionPoint(原点) + m_direction(定向矢量)
     *          - marker_J(基本侧)：仅基本体ID，原点与方向在装配时复用 marker_I（同点同向）
     *          刚度 m_stiffness[6] / 阻尼 m_damping[6] / 预载 F0/T0 / 偏置 D0 均为本类专属
     */
    class MXMOTIONDATA_EXPORT mMotionBushing : public mMotionVectorForce {
        using mMotionVectorForce::m_id;
        using mMotionVectorForce::m_name;
    public:
        mMotionBushing(int id, const QString& name = "");
        mMotionBushing();
        ~mMotionBushing() override;

        // --- 刚度 / 阻尼 ---
        void setStiffness(const QVector<double>& k);   // K[6]: Kxx,Kyy,Kzz,Krx,Kry,Krz
        QVector<double> getStiffness() const;

        void setDamping(const QVector<double>& c);     // C[6]: Cxx,Cyy,Czz,Crx,Cry,Crz
        QVector<double> getDamping() const;

        // --- 基本体 / marker_J ---

        // --- 偏置（marker_J 相对 I 的初始位置偏移） ---
        void setApplyOffset(bool apply);        // 是否应用偏置
        bool getApplyOffset() const;

        void setOffset(const QVector3D& offset); // 平移偏置 (x,y,z)
        QVector3D getOffset() const;

        // --- 预载 ---
        void setPreloadForce(const QVector<double>& f0);   // 平移预载力 F0[3]
        QVector<double> getPreloadForce() const;

        void setPreloadTorque(const QVector<double>& t0);  // 预载扭矩 T0[3]
        QVector<double> getPreloadTorque() const;

        /**
         * @brief 复制当前衬套对象（多态）
         */
        mMotionVectorForce* copyForce(int newId, const QString& newName) override;

        /**
         * @brief 基类多态复制接口
         */
        mMotionForceElement* copyForceElement(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionBushing& bushing);
        friend QDataStream& operator<<(QDataStream& out, mMotionBushing& bushing);

    private:
        // --- 刚度 / 阻尼 ---
        QVector<double> m_stiffness;            ///< 刚度 Kxx,Kyy,Kzz,Krx,Kry,Krz
        QVector<double> m_damping;              ///< 阻尼 Cxx,Cyy,Czz,Crx,Cry,Crz

        // --- marker_J / 基本体 ---

        // --- 偏置 ---
        bool m_applyOffset = false;             ///< 是否应用偏置
        QVector3D m_offset;                     ///< 平移偏置 (x,y,z)，marker_J 相对 I 的初始位置偏移

        // --- 预载 ---
        QVector<double> m_preloadForce;         ///< 平移预载力 F0x,F0y,F0z
        QVector<double> m_preloadTorque;        ///< 预载扭矩 T0x,T0y,T0z
    };

}
