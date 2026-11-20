#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QVector>

#include "MXMotionData_global.h"
#include "mMotionJointBasic.h"

namespace MechMotionData {

    /**
     * @brief 点在面上副数据类
     * @details 动作体上的点约束在基本体的离散曲面上滑动（UG Point on Surface）。
     *          作用点复用基类 m_origin；曲面宿主复用基类 m_baseBodyId；
     *          m_surfaceFaceIds 为用户拾取的约束面（基本体几何的面实体ID），
     *          为空时求解装配回退为基本体整体离散网格。
     */
    class MXMOTIONDATA_EXPORT mMotionJointPointOnSurface : public mMotionJointBasic {
    public:
        mMotionJointPointOnSurface(int id, JointType type, const QString& name = "");
        mMotionJointPointOnSurface() {};
        ~mMotionJointPointOnSurface() override;

        void setSurfaceFaceIds(const QVector<int>& faceIds);
        QVector<int> getSurfaceFaceIds() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointPointOnSurface& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointPointOnSurface& mgr);

    private:
        QVector<int> m_surfaceFaceIds;   ///< 约束面实体ID列表（空=整体网格）
    };

}
