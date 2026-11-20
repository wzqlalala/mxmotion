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
     * @brief 齿轮副数据类
     * @details 齿轮副通过引用两个已创建的旋转副建立耦合。
     *          运动体、原点、方向等信息由关联运动副提供。
     *          本类提供传动比、齿数、分度圆半径三种等价表示，保证
     *          gearRatio = toothCount2 / toothCount1 = pitchRadius2 / pitchRadius1。
     */
    class MXMOTIONDATA_EXPORT mMotionCouplingGear : public mMotionCouplingBase {
    public:
        mMotionCouplingGear(int id, CouplingType type, const QString& name = "");
        mMotionCouplingGear() {};
        ~mMotionCouplingGear() override;

        /**
         * @brief 设置传动比（默认1.0，表示第二个齿轮相对第一个齿轮的传动比）
         */
        void setGearRatio(double ratio);
        double getGearRatio() const;

        /**
         * @brief 设置第一个齿轮齿数
         */
        void setToothCount1(int count);
        int getToothCount1() const;

        /**
         * @brief 设置第二个齿轮齿数
         */
        void setToothCount2(int count);
        int getToothCount2() const;

        /**
         * @brief 设置第一个齿轮分度圆半径
         */
        void setPitchRadius1(double radius);
        double getPitchRadius1() const;

        /**
         * @brief 设置第二个齿轮分度圆半径
         */
        void setPitchRadius2(double radius);
        double getPitchRadius2() const;

        mMotionCouplingBase* copyCoupling(int newId, const QString& newName) override;

        /**
         * @brief 根据当前传动比重新计算并同步齿数与分度圆半径
         * @details 以齿数1 = 20 为基准，toothCount2 = toothCount1 * gearRatio；
         *          以分度圆半径1 = 100 mm 为基准，pitchRadius2 = pitchRadius1 * gearRatio。
         */
        void syncParametersFromGearRatio();

        friend QDataStream& operator>>(QDataStream& in, mMotionCouplingGear& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCouplingGear& mgr);

    private:
        double m_gearRatio;     ///< 传动比（第二个齿轮相对第一个齿轮）
        int m_toothCount1;      ///< 第一个齿轮齿数
        int m_toothCount2;      ///< 第二个齿轮齿数
        double m_pitchRadius1;  ///< 第一个齿轮分度圆半径
        double m_pitchRadius2;  ///< 第二个齿轮分度圆半径
    };

}
