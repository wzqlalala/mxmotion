#include "mMotionBodySensor.h"

namespace MechMotionData {

    mMotionBodySensor::mMotionBodySensor(int id, SensorType type, const QString& name, bool isUserRename)
        : mMotionSensorBase(id, type, name, isUserRename)
        
        , m_referencePoint(0, 0, 0)
        , m_variable(BodySensorVariable::Position)
    {
        clampVariableToType();
    }

    mMotionBodySensor::mMotionBodySensor()
        : mMotionSensorBase()
        
        , m_referencePoint(0, 0, 0)
        , m_variable(BodySensorVariable::Position)
    {
        clampVariableToType();
    }

    mMotionBodySensor::~mMotionBodySensor() {}

    SensorType mMotionBodySensor::getType() const {
        return m_type;
    }



    void mMotionBodySensor::setReferencePoint(const QVector3D& point) {
        m_referencePoint = point;
    }

    QVector3D mMotionBodySensor::getReferencePoint() const {
        return m_referencePoint;
    }

    void mMotionBodySensor::setVariable(BodySensorVariable var) {
        m_variable = var;
    }

    BodySensorVariable mMotionBodySensor::getVariable() const {
        return m_variable;
    }

    QString mMotionBodySensor::getVariableName() const {
        return getBodySensorVariableName(m_variable);
    }

    mMotionSensorBase* mMotionBodySensor::copySensor(int newId, const QString& newName) const {
        mMotionBodySensor* copy = new mMotionBodySensor(newId, m_type, newName, m_isUserRename);
        copy->m_coordSystem = m_coordSystem;
        copy->m_componentMode = m_componentMode;
        copy->m_isEnabled = m_isEnabled;
        copy->m_isUserRename = m_isUserRename;
        copy->m_referencePoint = m_referencePoint;
        copy->m_variable = m_variable;
        return copy;
    }

    void mMotionBodySensor::readFromStream(QDataStream& in) {
        int varInt = 0;
        in >> m_referencePoint;
        in >> varInt;
        m_variable = static_cast<BodySensorVariable>(varInt);
        // 校正历史数据中类型与变量不匹配的组合（按变量意图重新定型）
        normalizeBodySensor(m_type, m_variable);
    }

    void mMotionBodySensor::clampVariableToType() {
        const auto allowed = allowedBodyVariables(m_type);
        if (!allowed.contains(m_variable))
            m_variable = allowed.isEmpty() ? BodySensorVariable::Position : allowed.first();
    }

    void mMotionBodySensor::writeToStream(QDataStream& out) const {
        out << m_referencePoint;
        out << static_cast<int>(m_variable);
    }

}
