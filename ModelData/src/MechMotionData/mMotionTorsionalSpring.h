#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>

#include "MXMotionData_global.h"
#include "mMotionSpringBase.h"

namespace MechMotionData {

    /**
     * @brief 扭转弹簧数据类
     * @details 对应 ADMSpring 中 dof_type = TORSION
     *          界面选择：旋转副（Revolute）、圆柱副（Cylindrical）
     *          物理层映射：MotionLinkRSDA
     */
    class MXMOTIONDATA_EXPORT mMotionTorsionalSpring : public mMotionSpringBase {
    public:
        mMotionTorsionalSpring(int id, const QString& name = "");
        mMotionTorsionalSpring() {};
        ~mMotionTorsionalSpring() override;

        mMotionSpringBase* copySpring(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionTorsionalSpring& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionTorsionalSpring& mgr);
    };

}
