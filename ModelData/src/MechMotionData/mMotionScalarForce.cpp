#include "mMotionScalarForce.h"

namespace MechMotionData {

    mMotionScalarForce::mMotionScalarForce()
        : mMotionForceElement(-1, QString()),
        m_localI(0.0, 0.0, 0.0),
        m_localJ(0.0, 0.0, 0.0),
        m_directionMode(ScalarForceDirectionMode::Line),
        m_fixedDirection(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_forceFunction(),
        m_actionOnly(false)
    {
    }

    mMotionScalarForce::mMotionScalarForce(int id, const QString& name)
        : mMotionForceElement(id, name),
        m_localI(0.0, 0.0, 0.0),
        m_localJ(0.0, 0.0, 0.0),
        m_directionMode(ScalarForceDirectionMode::Line),
        m_fixedDirection(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_forceFunction(),
        m_actionOnly(false)
    {
        if (m_name.isEmpty()) {
            m_name = QString("ScalarForce_%1").arg(m_id);
        }
    }

    mMotionScalarForce::~mMotionScalarForce() {}


    void mMotionScalarForce::setLocalI(const QVector3D& pos) { m_localI = pos; }
    QVector3D mMotionScalarForce::getLocalI() const { return m_localI; }


    void mMotionScalarForce::setLocalJ(const QVector3D& pos) { m_localJ = pos; }
    QVector3D mMotionScalarForce::getLocalJ() const { return m_localJ; }

    void mMotionScalarForce::setDirectionMode(ScalarForceDirectionMode mode) { m_directionMode = mode; }
    ScalarForceDirectionMode mMotionScalarForce::getDirectionMode() const { return m_directionMode; }

    void mMotionScalarForce::setFixedDirection(const QVector3D& dir) { m_fixedDirection = dir; }
    QVector3D mMotionScalarForce::getFixedDirection() const { return m_fixedDirection; }

    void mMotionScalarForce::setMagnitude(double magnitude) { m_magnitude = magnitude; }
    double mMotionScalarForce::getMagnitude() const { return m_magnitude; }

    void mMotionScalarForce::setForceFunction(const QString& expr) { m_forceFunction = expr; }
    QString mMotionScalarForce::getForceFunction() const { return m_forceFunction; }

    void mMotionScalarForce::setActionOnly(bool actionOnly) { m_actionOnly = actionOnly; }
    bool mMotionScalarForce::getActionOnly() const { return m_actionOnly; }

    mMotionScalarForce* mMotionScalarForce::copyScalarForce(int newId, const QString& newName)
    {
        mMotionScalarForce* newForce = new mMotionScalarForce(newId, newName);
        newForce->setLocalI(this->getLocalI());
        newForce->setLocalJ(this->getLocalJ());
        newForce->setDirectionMode(this->getDirectionMode());
        newForce->setFixedDirection(this->getFixedDirection());
        newForce->setMagnitude(this->getMagnitude());
        newForce->setForceFunction(this->getForceFunction());
        newForce->setActionOnly(this->getActionOnly());
        return newForce;
    }

    mMotionForceElement* mMotionScalarForce::copyForceElement(int newId, const QString& newName)
    {
        return copyScalarForce(newId, newName);
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
