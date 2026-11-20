#include "mMotionThreePointTorque.h"

namespace MechMotionData {

    mMotionThreePointTorque::mMotionThreePointTorque()
        : mMotionForceElement(-1, QString()),
        m_localI(0.0, 0.0, 0.0),
        m_localTarget(0.0, 0.0, 0.0),
        m_localOrigin(0.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_torqueFunction()
    {
    }

    mMotionThreePointTorque::mMotionThreePointTorque(int id, const QString& name)
        : mMotionForceElement(id, name),
        m_localI(0.0, 0.0, 0.0),
        m_localTarget(0.0, 0.0, 0.0),
        m_localOrigin(0.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_torqueFunction()
    {
        if (m_name.isEmpty()) {
            m_name = QString("ThreePointTorque_%1").arg(m_id);
        }
    }

    mMotionThreePointTorque::~mMotionThreePointTorque() {}


    void mMotionThreePointTorque::setLocalI(const QVector3D& pos) { m_localI = pos; }
    QVector3D mMotionThreePointTorque::getLocalI() const { return m_localI; }


    void mMotionThreePointTorque::setLocalTarget(const QVector3D& pos) { m_localTarget = pos; }
    QVector3D mMotionThreePointTorque::getLocalTarget() const { return m_localTarget; }


    void mMotionThreePointTorque::setLocalOrigin(const QVector3D& pos) { m_localOrigin = pos; }
    QVector3D mMotionThreePointTorque::getLocalOrigin() const { return m_localOrigin; }

    void mMotionThreePointTorque::setMagnitude(double magnitude) { m_magnitude = magnitude; }
    double mMotionThreePointTorque::getMagnitude() const { return m_magnitude; }

    void mMotionThreePointTorque::setTorqueFunction(const QString& expr) { m_torqueFunction = expr; }
    QString mMotionThreePointTorque::getTorqueFunction() const { return m_torqueFunction; }

    mMotionThreePointTorque* mMotionThreePointTorque::copyThreePointTorque(int newId, const QString& newName)
    {
        mMotionThreePointTorque* newTorque = new mMotionThreePointTorque(newId, newName);
        newTorque->setLocalI(this->getLocalI());
        newTorque->setLocalTarget(this->getLocalTarget());
        newTorque->setLocalOrigin(this->getLocalOrigin());
        newTorque->setMagnitude(this->getMagnitude());
        newTorque->setTorqueFunction(this->getTorqueFunction());
        return newTorque;
    }

    mMotionForceElement* mMotionThreePointTorque::copyForceElement(int newId, const QString& newName)
    {
        return copyThreePointTorque(newId, newName);
    }

} // namespace MechMotionData
