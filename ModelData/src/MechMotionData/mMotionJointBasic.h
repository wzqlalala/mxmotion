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
    * @brief 基本运动副数据类 (适用于旋转副、移动副等仅需原点和方向的约束)
    */
    class MXMOTIONDATA_EXPORT mMotionJointBasic : public mMotionJointBase {
    public:
        DirectionPickState axisPickState_;             ///< 轴向输入的拾取会话状态（照集中力 dirPickState_）
        mMotionJointBasic(int id, JointType type, const QString& name = "");
        mMotionJointBasic() {};
        ~mMotionJointBasic() override;

        void setOrigin(const QVector3D& origin);
        QVector3D getOrigin() const;

        void setAxis(const QVector3D& axis);
        QVector3D getAxis() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointBasic& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointBasic& mgr);

    private:
        QVector3D m_origin;             ///< 约束空间原点
        QVector3D m_axis;               ///< 约束主轴方向矢量
    };

}
