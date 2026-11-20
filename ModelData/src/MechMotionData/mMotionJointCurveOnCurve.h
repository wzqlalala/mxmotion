#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QVector>

#include "MXMotionData_global.h"
#include "mMotionJointBasic.h"

namespace MechMotionData {

    /**
     * @brief 线在线上副数据类
     * @details 动作体上的曲线与基本体上的曲线始终保持相切接触（UG Curve on Curve）。
     *          m_curveIEdgeIds 为动作体上的约束边（运动体几何的边实体ID），
     *          m_curveJEdgeIds 为基本体上的约束边，两组边各自按共端点串接成
     *          一条折线（单条边为特例），为空即非法（两侧都必须显式拾取）。
     *          作用点复用基类 m_origin（初始接触点）；两个连接体走 mMotionRefMap。
     */
    class MXMOTIONDATA_EXPORT mMotionJointCurveOnCurve : public mMotionJointBasic {
    public:
        mMotionJointCurveOnCurve(int id, JointType type, const QString& name = "");
        mMotionJointCurveOnCurve() {};
        ~mMotionJointCurveOnCurve() override;

        void setCurveIEdgeIds(const QVector<int>& edgeIds);
        QVector<int> getCurveIEdgeIds() const;
        void setCurveJEdgeIds(const QVector<int>& edgeIds);
        QVector<int> getCurveJEdgeIds() const;

        /// 允许滑动（UG Allow Sliding）：true=相切+允许切向滑移（文档默认语义）；
        /// false=无滑滚动（默认，额外锁接触点切向速度）。
        void setAllowSliding(bool allow);
        bool getAllowSliding() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointCurveOnCurve& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointCurveOnCurve& mgr);

    private:
        QVector<int> m_curveIEdgeIds;   ///< 动作体约束边实体ID列表（串接为一条折线）
        QVector<int> m_curveJEdgeIds;   ///< 基本体约束边实体ID列表（串接为一条折线）
        bool m_allowSliding = false;    ///< true=允许滑动，false=无滑滚动（默认）
    };

}
