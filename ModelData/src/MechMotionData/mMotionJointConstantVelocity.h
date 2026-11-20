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
    * @brief 等速运动副数据类
    * @details 包含约束空间原点、运动体矢量、基本体矢量，
    *          两矢量方向保持等速（恒定转速比）传递
    */
    class MXMOTIONDATA_EXPORT mMotionJointConstantVelocity : public mMotionJointBase {
    public:
        DirectionPickState actionVecPickState_, baseVecPickState_;     ///< 双矢量拾取会话状态
        mMotionJointConstantVelocity(int id, JointType type, const QString& name = "");
        mMotionJointConstantVelocity() {};
        ~mMotionJointConstantVelocity() override;

        void setOrigin(const QVector3D& origin);
        QVector3D getOrigin() const;

        void setActionBodyVector(const QVector3D& vector);
        QVector3D getActionBodyVector() const;

        void setBaseBodyVector(const QVector3D& vector);
        QVector3D getBaseBodyVector() const;

        mMotionJointBase* copyJoint(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointConstantVelocity& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointConstantVelocity& mgr);

    private:
        QVector3D m_origin;              ///< 约束空间原点
        QVector3D m_actionBodyVector;    ///< 运动体矢量
        QVector3D m_baseBodyVector;      ///< 基本体矢量
    };

}
