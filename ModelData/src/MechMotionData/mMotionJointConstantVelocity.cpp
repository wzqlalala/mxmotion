#include "mMotionJointConstantVelocity.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointConstantVelocity::mMotionJointConstantVelocity(int id, JointType type, const QString& name)
        : mMotionJointBase(id, type, name),
        m_origin(0.0f, 0.0f, 0.0f),
        m_actionBodyVector(0.0f, 0.0f, 1.0f),
        m_baseBodyVector(1.0f, 0.0f, 0.0f)
    {
    }

    mMotionJointConstantVelocity::~mMotionJointConstantVelocity() {}

    void mMotionJointConstantVelocity::setOrigin(const QVector3D& origin) { m_origin = origin; }

    QVector3D mMotionJointConstantVelocity::getOrigin() const { return m_origin; }

    void mMotionJointConstantVelocity::setActionBodyVector(const QVector3D& vector) { m_actionBodyVector = vector; }

    QVector3D mMotionJointConstantVelocity::getActionBodyVector() const { return m_actionBodyVector; }

    void mMotionJointConstantVelocity::setBaseBodyVector(const QVector3D& vector) { m_baseBodyVector = vector; }

    QVector3D mMotionJointConstantVelocity::getBaseBodyVector() const { return m_baseBodyVector; }

    mMotionJointBase* mMotionJointConstantVelocity::copyJoint(int newId, const QString& newName)
    {
        mMotionJointConstantVelocity* newJoint = new mMotionJointConstantVelocity(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setActionBodyVector(this->getActionBodyVector());
        newJoint->setBaseBodyVector(this->getBaseBodyVector());

        return newJoint;
    }
}
