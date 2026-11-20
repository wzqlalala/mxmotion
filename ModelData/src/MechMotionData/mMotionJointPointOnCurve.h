#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QVector>

#include "MXMotionData_global.h"
#include "mMotionJointBasic.h"

namespace MechMotionData {

    /**
     * @brief 点在线上副数据类
     * @details 动作体上的点约束在基本体的离散曲线上滑动（UG Point on Curve）。
     *          作用点复用基类 m_origin；曲线宿主复用基类 m_baseBodyId；
     *          m_curveEdgeIds 为用户拾取的约束边（基本体几何的边实体ID），
     *          求解装配时按共端点自动串接成一条折线（单条边为特例）。
     *          与点在面上副不同：曲线必须显式拾取，无整体网格回退，为空即非法。
     */
    class MXMOTIONDATA_EXPORT mMotionJointPointOnCurve : public mMotionJointBasic {
    public:
        mMotionJointPointOnCurve(int id, JointType type, const QString& name = "");
        mMotionJointPointOnCurve() {};
        ~mMotionJointPointOnCurve() override;

        void setCurveEdgeIds(const QVector<int>& edgeIds);
        QVector<int> getCurveEdgeIds() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointPointOnCurve& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointPointOnCurve& mgr);

    private:
        QVector<int> m_curveEdgeIds;   ///< 约束边实体ID列表（按共端点串接为一条折线）
    };

}
