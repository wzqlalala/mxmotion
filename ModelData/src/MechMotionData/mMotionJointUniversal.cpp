#include "mMotionJointUniversal.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionJointUniversal::mMotionJointUniversal(int id, JointType type, const QString& name)
        : mMotionJointBase(id, type, name),
        m_origin(0.0f, 0.0f, 0.0f),
        m_actionBodySpinVector(0.0f, 0.0f, 1.0f),
        m_actionBodyPinVector(1.0f, 0.0f, 0.0f),
        m_baseBodySpinVector(0.0f, 0.0f, 1.0f),
        m_baseBodyPinVector(0.0f, 1.0f, 0.0f)
    {
    }

    mMotionJointUniversal::~mMotionJointUniversal() {}

    void mMotionJointUniversal::setOrigin(const QVector3D& origin) { m_origin = origin; }

    QVector3D mMotionJointUniversal::getOrigin() const { return m_origin; }

    void mMotionJointUniversal::setActionBodySpinVector(const QVector3D& vector) { m_actionBodySpinVector = vector; }

    QVector3D mMotionJointUniversal::getActionBodySpinVector() const { return m_actionBodySpinVector; }

    void mMotionJointUniversal::setActionBodyPinVector(const QVector3D& vector) { m_actionBodyPinVector = vector; }

    QVector3D mMotionJointUniversal::getActionBodyPinVector() const { return m_actionBodyPinVector; }

    void mMotionJointUniversal::setBaseBodySpinVector(const QVector3D& vector) { m_baseBodySpinVector = vector; }

    QVector3D mMotionJointUniversal::getBaseBodySpinVector() const { return m_baseBodySpinVector; }

    void mMotionJointUniversal::setBaseBodyPinVector(const QVector3D& vector) { m_baseBodyPinVector = vector; }

    QVector3D mMotionJointUniversal::getBaseBodyPinVector() const { return m_baseBodyPinVector; }

    mMotionJointBase* mMotionJointUniversal::copyJoint(int newId, const QString& newName)
    {
        mMotionJointUniversal* newJoint = new mMotionJointUniversal(newId, m_type, newName);
        this->copyCommonDataTo(*newJoint);
        newJoint->setOrigin(this->getOrigin());
        newJoint->setActionBodySpinVector(this->getActionBodySpinVector());
        newJoint->setActionBodyPinVector(this->getActionBodyPinVector());
        newJoint->setBaseBodySpinVector(this->getBaseBodySpinVector());
        newJoint->setBaseBodyPinVector(this->getBaseBodyPinVector());

        return newJoint;
    }
}
