#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionDriveBase.h"

#include <QVector3D>
#include <QVector>
#include <QString>


namespace MechMotionData {

    /**
     * @brief 运动副驱动数据类
     */
    class MXMOTIONDATA_EXPORT mMotionJointDrive : public mMotionDriveBase {
    public:
        mMotionJointDrive(int id, const QString& name = "");
        mMotionJointDrive() {};
        ~mMotionJointDrive() override;


        mMotionDriveBase* copyDrive(int newId, const QString& newName) override;

        friend QDataStream& operator>>(QDataStream& in, mMotionJointDrive& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointDrive& mgr);

    private:
    };

}
