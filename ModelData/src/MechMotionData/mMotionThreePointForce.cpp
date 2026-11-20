#include "mMotionThreePointForce.h"

namespace MechMotionData {

    mMotionThreePointForce::mMotionThreePointForce()
        : mMotionForceElement(-1, QString()),
        m_localI(0.0, 0.0, 0.0),
        m_localTarget(0.0, 0.0, 0.0),
        m_localOrigin(0.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_forceFunction()
    {
    }

    mMotionThreePointForce::mMotionThreePointForce(int id, const QString& name)
        : mMotionForceElement(id, name),
        m_localI(0.0, 0.0, 0.0),
        m_localTarget(0.0, 0.0, 0.0),
        m_localOrigin(0.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_forceFunction()
    {
        if (m_name.isEmpty()) {
            m_name = QString("ThreePointForce_%1").arg(m_id);
        }
    }

    mMotionThreePointForce::~mMotionThreePointForce() {}


    void mMotionThreePointForce::setLocalI(const QVector3D& pos) { m_localI = pos; }
    QVector3D mMotionThreePointForce::getLocalI() const { return m_localI; }


    void mMotionThreePointForce::setLocalTarget(const QVector3D& pos) { m_localTarget = pos; }
    QVector3D mMotionThreePointForce::getLocalTarget() const { return m_localTarget; }


    void mMotionThreePointForce::setLocalOrigin(const QVector3D& pos) { m_localOrigin = pos; }
    QVector3D mMotionThreePointForce::getLocalOrigin() const { return m_localOrigin; }

    void mMotionThreePointForce::setMagnitude(double magnitude) { m_magnitude = magnitude; }
    double mMotionThreePointForce::getMagnitude() const { return m_magnitude; }

    void mMotionThreePointForce::setForceFunction(const QString& expr) { m_forceFunction = expr; }
    QString mMotionThreePointForce::getForceFunction() const { return m_forceFunction; }

    mMotionThreePointForce* mMotionThreePointForce::copyThreePointForce(int newId, const QString& newName)
    {
        mMotionThreePointForce* newForce = new mMotionThreePointForce(newId, newName);
        newForce->setLocalI(this->getLocalI());
        newForce->setLocalTarget(this->getLocalTarget());
        newForce->setLocalOrigin(this->getLocalOrigin());
        newForce->setMagnitude(this->getMagnitude());
        newForce->setForceFunction(this->getForceFunction());
        return newForce;
    }

    mMotionForceElement* mMotionThreePointForce::copyForceElement(int newId, const QString& newName)
    {
        return copyThreePointForce(newId, newName);
    }

} // namespace MechMotionData
