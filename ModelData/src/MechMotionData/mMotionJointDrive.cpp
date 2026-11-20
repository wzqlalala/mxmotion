#include "mMotionJointDrive.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointDrive::mMotionJointDrive(int id, const QString& name)
        : mMotionDriveBase(id, DriveTargetType::Joint, name)
    {
    }

    mMotionJointDrive::~mMotionJointDrive() {}


    mMotionDriveBase* mMotionJointDrive::copyDrive(int newId, const QString& newName)
    {
        // 1. 实例化出正确的派生类对象
        mMotionJointDrive* newDrive = new mMotionJointDrive(newId, newName);

        // 3. 深度拷贝该类特有的所有业务属性
        newDrive->setMotionType(this->getMotionType());       // 运动类型：Rotation 或 Translation
        newDrive->setControlMode(this->getControlMode());     // 控制模式：Speed 或 Position
        newDrive->setExpression(this->getExpression());       // 驱动表达式或常数值的 QString

        // 如果你后期还加了初始位移、初始速度等字段，也在这里一并 Set 过去
        // newDrive->setInitialDisplacement(this->getInitialDisplacement());

        return newDrive;
    }
}
