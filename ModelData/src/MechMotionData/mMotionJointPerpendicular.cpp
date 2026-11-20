#include "mMotionJointPerpendicular.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointPerpendicular::mMotionJointPerpendicular(int id, JointType type, const QString& name)
        : mMotionJointBase(id, type, name),
        m_origin(0.0f, 0.0f, 0.0f),
        m_actionBodyVector(0.0f, 0.0f, 1.0f),
        m_baseBodyVector(1.0f, 0.0f, 0.0f)
    {
    }

    mMotionJointPerpendicular::~mMotionJointPerpendicular() {}

    void mMotionJointPerpendicular::setOrigin(const QVector3D& origin) { m_origin = origin; }

    QVector3D mMotionJointPerpendicular::getOrigin() const { return m_origin; }

    void mMotionJointPerpendicular::setActionBodyVector(const QVector3D& vector) { m_actionBodyVector = vector; }

    QVector3D mMotionJointPerpendicular::getActionBodyVector() const { return m_actionBodyVector; }

    void mMotionJointPerpendicular::setBaseBodyVector(const QVector3D& vector) { m_baseBodyVector = vector; }

    QVector3D mMotionJointPerpendicular::getBaseBodyVector() const { return m_baseBodyVector; }

    mMotionJointBase* mMotionJointPerpendicular::copyJoint(int newId, const QString& newName)
    {
        mMotionJointPerpendicular* newJoint = new mMotionJointPerpendicular(newId, m_type, newName);

        newJoint->setOrigin(this->getOrigin());
        newJoint->setActionBodyVector(this->getActionBodyVector());
        newJoint->setBaseBodyVector(this->getBaseBodyVector());

        return newJoint;
    }
}
