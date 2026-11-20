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
    * @brief 齿轮齿条副数据类
    * @details 齿轮齿条副通过引用两个已创建的运动副建立耦合：
    *          - 第一个运动副通常为旋转副（齿轮）
    *          - 第二个运动副通常为移动副（齿条）
    *          运动体、原点、方向等信息由关联运动副提供，本类只保留齿轮齿条副特有参数。
    */
    class MXMOTIONDATA_EXPORT mMotionCouplingGearRack : public mMotionCouplingBase {
    public:
        mMotionCouplingGearRack(int id, CouplingType type, const QString& name = "");
        mMotionCouplingGearRack() {};
        ~mMotionCouplingGearRack() override;

        /**
         * @brief 设置齿轮半径
         */
        void setRadius(double radius);
        /**
         * @brief 获取齿轮半径
         */
        double getRadius() const;

        mMotionCouplingBase* copyCoupling(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionCouplingGearRack& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCouplingGearRack& mgr);

    private:
        double m_radius;                ///< 齿轮半径（用于齿轮转角与齿条位移的关系）
    };

}
