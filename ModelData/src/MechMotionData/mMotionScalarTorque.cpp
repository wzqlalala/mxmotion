#include "mMotionScalarTorque.h"

namespace MechMotionData {

    mMotionScalarTorque::mMotionScalarTorque()
        : mMotionForceElement(-1, QString()),
        m_localI(0.0, 0.0, 0.0),
        m_localJ(0.0, 0.0, 0.0),
        m_axisMode(ScalarTorqueAxisMode::Line),
        m_fixedAxis(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_torqueFunction(),
        m_actionOnly(false)
    {
    }

    mMotionScalarTorque::mMotionScalarTorque(int id, const QString& name)
        : mMotionForceElement(id, name),
        m_localI(0.0, 0.0, 0.0),
        m_localJ(0.0, 0.0, 0.0),
        m_axisMode(ScalarTorqueAxisMode::Line),
        m_fixedAxis(1.0, 0.0, 0.0),
        m_magnitude(0.0),
        m_torqueFunction(),
        m_actionOnly(false)
    {
        if (m_name.isEmpty()) {
            m_name = QString("ScalarTorque_%1").arg(m_id);
        }
    }

    mMotionScalarTorque::~mMotionScalarTorque() {}


    void mMotionScalarTorque::setLocalI(const QVector3D& pos) { m_localI = pos; }
    QVector3D mMotionScalarTorque::getLocalI() const { return m_localI; }


    void mMotionScalarTorque::setLocalJ(const QVector3D& pos) { m_localJ = pos; }
    QVector3D mMotionScalarTorque::getLocalJ() const { return m_localJ; }

    void mMotionScalarTorque::setAxisMode(ScalarTorqueAxisMode mode) { m_axisMode = mode; }
    ScalarTorqueAxisMode mMotionScalarTorque::getAxisMode() const { return m_axisMode; }

    void mMotionScalarTorque::setFixedAxis(const QVector3D& dir) { m_fixedAxis = dir; }
    QVector3D mMotionScalarTorque::getFixedAxis() const { return m_fixedAxis; }

    void mMotionScalarTorque::setMagnitude(double magnitude) { m_magnitude = magnitude; }
    double mMotionScalarTorque::getMagnitude() const { return m_magnitude; }

    void mMotionScalarTorque::setTorqueFunction(const QString& expr) { m_torqueFunction = expr; }
    QString mMotionScalarTorque::getTorqueFunction() const { return m_torqueFunction; }

    void mMotionScalarTorque::setActionOnly(bool actionOnly) { m_actionOnly = actionOnly; }
    bool mMotionScalarTorque::getActionOnly() const { return m_actionOnly; }

    mMotionScalarTorque* mMotionScalarTorque::copyScalarTorque(int newId, const QString& newName)
    {
        mMotionScalarTorque* newForce = new mMotionScalarTorque(newId, newName);
        newForce->setLocalI(this->getLocalI());
        newForce->setLocalJ(this->getLocalJ());
        newForce->setAxisMode(this->getAxisMode());
        newForce->setFixedAxis(this->getFixedAxis());
        newForce->setMagnitude(this->getMagnitude());
        newForce->setTorqueFunction(this->getTorqueFunction());
        newForce->setActionOnly(this->getActionOnly());
        return newForce;
    }

    mMotionForceElement* mMotionScalarTorque::copyForceElement(int newId, const QString& newName)
    {
        return copyScalarTorque(newId, newName);
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
