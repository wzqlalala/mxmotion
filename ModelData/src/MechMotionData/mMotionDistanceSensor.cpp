#include "mMotionDistanceSensor.h"

namespace MechMotionData {

    mMotionDistanceSensor::mMotionDistanceSensor(int id, SensorType type, const QString& name, bool isUserRename)
        : mMotionSensorBase(id, type, name, isUserRename)
        , m_pointA({ -1, QVector3D(0, 0, 0) })
        , m_pointB({ -1, QVector3D(0, 0, 0) })
        , m_variable(DistanceVariable::Distance)
    {
    }

    mMotionDistanceSensor::mMotionDistanceSensor()
        : mMotionSensorBase()
        , m_pointA({ -1, QVector3D(0, 0, 0) })
        , m_pointB({ -1, QVector3D(0, 0, 0) })
        , m_variable(DistanceVariable::Distance)
    {
    }

    mMotionDistanceSensor::~mMotionDistanceSensor() {}

    SensorType mMotionDistanceSensor::getType() const {
        return m_type;
    }

    void mMotionDistanceSensor::setPointA(const SensorPointRef& point) {
        m_pointA = point;
    }

    SensorPointRef mMotionDistanceSensor::getPointA() const {
        return m_pointA;
    }

    void mMotionDistanceSensor::setPointB(const SensorPointRef& point) {
        m_pointB = point;
    }

    SensorPointRef mMotionDistanceSensor::getPointB() const {
        return m_pointB;
    }

    void mMotionDistanceSensor::setVariable(DistanceVariable var) {
        m_variable = var;
    }

    DistanceVariable mMotionDistanceSensor::getVariable() const {
        return m_variable;
    }

    QString mMotionDistanceSensor::getVariableName() const {
        return getDistanceVariableName(m_variable);
    }

    mMotionSensorBase* mMotionDistanceSensor::copySensor(int newId, const QString& newName) const {
        mMotionDistanceSensor* copy = new mMotionDistanceSensor(newId, m_type, newName, m_isUserRename);
        copy->m_coordSystem = m_coordSystem;
        copy->m_componentMode = m_componentMode;
        copy->m_isEnabled = m_isEnabled;
        copy->m_isUserRename = m_isUserRename;
        copy->m_pointA = m_pointA;
        copy->m_pointB = m_pointB;
        copy->m_variable = m_variable;
        return copy;
    }

    void mMotionDistanceSensor::readFromStream(QDataStream& in) {
        int varInt = 0;
        in >> m_pointA.bodyId;
        in >> m_pointA.localPos;
        in >> m_pointB.bodyId;
        in >> m_pointB.localPos;
        in >> varInt;
        m_variable = static_cast<DistanceVariable>(varInt);
    }

    void mMotionDistanceSensor::writeToStream(QDataStream& out) const {
        out << m_pointA.bodyId;
        out << m_pointA.localPos;
        out << m_pointB.bodyId;
        out << m_pointB.localPos;
        out << static_cast<int>(m_variable);
    }

}
