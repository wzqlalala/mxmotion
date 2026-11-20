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
    * @brief 万向节运动副数据类
    * @details 包含运动体自旋矢量、运动体十字销矢量、基本体自旋矢量、基本体十字销矢量
    */
    class MXMOTIONDATA_EXPORT mMotionJointUniversal : public mMotionJointBase {
    public:
        DirectionPickState actionSpinPickState_, actionPinPickState_, baseSpinPickState_, basePinPickState_; ///< 四矢量拾取会话状态
        mMotionJointUniversal(int id, JointType type, const QString& name = "");
        mMotionJointUniversal() {};
        ~mMotionJointUniversal() override;

        void setOrigin(const QVector3D& origin);
        QVector3D getOrigin() const;

        void setActionBodySpinVector(const QVector3D& vector);
        QVector3D getActionBodySpinVector() const;

        void setActionBodyPinVector(const QVector3D& vector);
        QVector3D getActionBodyPinVector() const;

        void setBaseBodySpinVector(const QVector3D& vector);
        QVector3D getBaseBodySpinVector() const;

        void setBaseBodyPinVector(const QVector3D& vector);
        QVector3D getBaseBodyPinVector() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointUniversal& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointUniversal& mgr);

    private:
        QVector3D m_origin;                  ///< 约束空间原点
        QVector3D m_actionBodySpinVector;     ///< 运动体自旋矢量
        QVector3D m_actionBodyPinVector;     ///< 运动体十字销矢量
        QVector3D m_baseBodySpinVector;      ///< 基本体自旋矢量
        QVector3D m_baseBodyPinVector;       ///< 基本体十字销矢量
    };

}
