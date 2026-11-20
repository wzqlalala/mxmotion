#include "mMotionAngleSensor.h"

namespace MechMotionData {

    mMotionAngleSensor::mMotionAngleSensor(int id, SensorType type, const QString& name, bool isUserRename)
        : mMotionSensorBase(id, type, name, isUserRename)
        
        , m_axisDirection(0, 0, 1)
        
        
    {
    }

    mMotionAngleSensor::mMotionAngleSensor()
        : mMotionSensorBase()
        
        , m_axisDirection(0, 0, 1)
        
        
    {
    }

    mMotionAngleSensor::~mMotionAngleSensor() {}

    SensorType mMotionAngleSensor::getType() const {
        return m_type;
    }



    void mMotionAngleSensor::setAxisDirection(const QVector3D& dir) {
        m_axisDirection = dir;
    }

    QVector3D mMotionAngleSensor::getAxisDirection() const {
        return m_axisDirection;
    }





    mMotionSensorBase* mMotionAngleSensor::copySensor(int newId, const QString& newName) const {
        mMotionAngleSensor* copy = new mMotionAngleSensor(newId, m_type, newName, m_isUserRename);
        copy->m_coordSystem = m_coordSystem;
        copy->m_componentMode = m_componentMode;
        copy->m_isEnabled = m_isEnabled;
        copy->m_isUserRename = m_isUserRename;
        copy->m_axisDirection = m_axisDirection;
        return copy;
    }

    void mMotionAngleSensor::readFromStream(QDataStream& in) {
        in >> m_axisDirection;
    }

    void mMotionAngleSensor::writeToStream(QDataStream& out) const {
        out << m_axisDirection;
    }

}
