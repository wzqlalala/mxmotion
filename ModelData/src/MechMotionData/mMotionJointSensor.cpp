#include "mMotionJointSensor.h"

namespace MechMotionData {

    mMotionJointSensor::mMotionJointSensor(int id, SensorType type, const QString& name, bool isUserRename)
        : mMotionSensorBase(id, type, name, isUserRename)
        
        , m_variable(JointSensorVariable::ReactionForce)
    {
    }

    mMotionJointSensor::mMotionJointSensor()
        : mMotionSensorBase()
        
        , m_variable(JointSensorVariable::ReactionForce)
    {
    }

    mMotionJointSensor::~mMotionJointSensor() {}

    SensorType mMotionJointSensor::getType() const {
        return m_type;
    }



    void mMotionJointSensor::setVariable(JointSensorVariable var) {
        m_variable = var;
    }

    JointSensorVariable mMotionJointSensor::getVariable() const {
        return m_variable;
    }

    QString mMotionJointSensor::getVariableName() const {
        return getJointSensorVariableName(m_variable);
    }

    mMotionSensorBase* mMotionJointSensor::copySensor(int newId, const QString& newName) const {
        mMotionJointSensor* copy = new mMotionJointSensor(newId, m_type, newName, m_isUserRename);
        copy->m_coordSystem = m_coordSystem;
        copy->m_componentMode = m_componentMode;
        copy->m_isEnabled = m_isEnabled;
        copy->m_isUserRename = m_isUserRename;
        copy->m_variable = m_variable;
        return copy;
    }

    void mMotionJointSensor::readFromStream(QDataStream& in) {
        int varInt = 0;
        in >> varInt;
        m_variable = static_cast<JointSensorVariable>(varInt);
    }

    void mMotionJointSensor::writeToStream(QDataStream& out) const {
        out << static_cast<int>(m_variable);
    }

}
