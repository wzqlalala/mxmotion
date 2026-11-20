#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mModelStruct.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionJointBase.h"

#include <QVector3D>
#include <QVector>
#include <QString>


namespace MechMotionData {

    /**
    * @brief 螺纹副数据类
    * @details 包含约束空间原点、约束主轴方向矢量及螺纹导程
    */
    class MXMOTIONDATA_EXPORT mMotionJointScrew : public mMotionJointBase {
    public:
        DirectionPickState axisPickState_;             ///< 轴向输入的拾取会话状态
        mMotionJointScrew(int id, JointType type, const QString& name = "");
        mMotionJointScrew() {};
        ~mMotionJointScrew() override;

        void setOrigin(const QVector3D& origin);
        QVector3D getOrigin() const;

        void setAxis(const QVector3D& axis);
        QVector3D getAxis() const;

        void setLead(double lead);
        double getLead() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointScrew& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointScrew& mgr);

    private:
        QVector3D m_origin;             ///< 约束空间原点
        QVector3D m_axis;               ///< 约束主轴方向矢量
        double m_lead;                  ///< 螺纹导程（每旋转一圈沿轴向移动的距离）
    };

}
