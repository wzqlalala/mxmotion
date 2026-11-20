#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionCouplingBase.h"

#include <QVector3D>
#include <QVector>
#include <QString>


namespace MechMotionData {

    /**
    * @brief 线缆副数据类
    * @details 线缆副通过引用两个已创建的移动副建立位移传动关系：
    *          (q2-q20) - ratio * (reverse ? -1 : 1) * (q1-q10) = 0。
    *          运动体、原点、方向等信息由关联的运动副提供，本类只保留线缆副特有参数。
    */
    class MXMOTIONDATA_EXPORT mMotionCouplingCable : public mMotionCouplingBase {
    public:
        mMotionCouplingCable(int id, CouplingType type, const QString& name = "");
        mMotionCouplingCable() {};
        ~mMotionCouplingCable() override;

        /**
         * @brief 设置位移比（第一副单位行程对应的第二侧行程，有限非零有符号）
         */
        void setRatio(double ratio);
        /**
         * @brief 获取位移比
         */
        double getRatio() const;

        /**
         * @brief 设置是否反向（约束方程中额外乘以 -1）
         */
        void setReverse(bool reverse);
        /**
         * @brief 获取是否反向
         */
        bool getReverse() const;

        mMotionCouplingBase* copyCoupling(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionCouplingCable& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCouplingCable& mgr);

    private:
        double m_ratio;                 ///< 位移比（第二侧行程 / 第一侧行程，有限非零有符号）
        bool m_reverse;                 ///< 是否反向（额外乘以 -1）
    };

}
