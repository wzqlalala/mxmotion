#include "mMotionDriveSensor.h"

namespace MechMotionData {

    mMotionDriveSensor::mMotionDriveSensor(int id, SensorType type, const QString& name, bool isUserRename)
        : mMotionSensorBase(id, type, name, isUserRename)
        
        , m_variable(DriveSensorVariable::ActuationForce)
    {
    }

    mMotionDriveSensor::mMotionDriveSensor()
        : mMotionSensorBase()
        
        , m_variable(DriveSensorVariable::ActuationForce)
    {
    }

    mMotionDriveSensor::~mMotionDriveSensor() {}

    SensorType mMotionDriveSensor::getType() const {
        return m_type;
    }



    void mMotionDriveSensor::setVariable(DriveSensorVariable var) {
        m_variable = var;
    }

    DriveSensorVariable mMotionDriveSensor::getVariable() const {
        return m_variable;
    }

    QString mMotionDriveSensor::getVariableName() const {
        return getDriveSensorVariableName(m_variable);
    }

    mMotionSensorBase* mMotionDriveSensor::copySensor(int newId, const QString& newName) const {
        mMotionDriveSensor* copy = new mMotionDriveSensor(newId, m_type, newName, m_isUserRename);
        copy->m_coordSystem = m_coordSystem;
        copy->m_componentMode = m_componentMode;
        copy->m_isEnabled = m_isEnabled;
        copy->m_isUserRename = m_isUserRename;
        copy->m_variable = m_variable;
        return copy;
    }

    void mMotionDriveSensor::readFromStream(QDataStream& in) {
        int varInt = 0;
        in >> varInt;
        m_variable = static_cast<DriveSensorVariable>(varInt);
    }

    void mMotionDriveSensor::writeToStream(QDataStream& out) const {
        out << static_cast<int>(m_variable);
    }

}
