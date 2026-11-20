#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>

#include "MXMotionData_global.h"
#include "mMotionSpringBase.h"

namespace MechMotionData {

    /**
     * @brief 拉压弹簧（轴向弹簧）数据类
     * @details 对应 ADMSpring 中 dof_type = AXIAL
     *          界面选择：移动副（Translational）
     *          物理层映射：MotionLinkTSDA
     */
    class MXMOTIONDATA_EXPORT mMotionTranslationalSpring : public mMotionSpringBase {
    public:
        mMotionTranslationalSpring(int id, const QString& name = "");
        mMotionTranslationalSpring() {};
        ~mMotionTranslationalSpring() override;

        mMotionSpringBase* copySpring(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionTranslationalSpring& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionTranslationalSpring& mgr);
    };

}
