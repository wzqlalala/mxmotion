#include "mMotionSensorBase.h"

namespace MechMotionData {

    mMotionSensorBase::mMotionSensorBase(int id, SensorType type, const QString& name, bool isUserRename)
        : m_id(id)
        , m_type(type)
        , m_name(name)
        , m_coordSystem(CoordSystem::GlobalFixed)
        , m_componentMode(ComponentMode::All)
        , m_isEnabled(true)
        , m_isUserRename(isUserRename)
    {
        if (m_name.isEmpty()) {
            m_name = QString("%1_%2").arg(getSensorTypeName(type)).arg(m_id);
        }
    }

    mMotionSensorBase::mMotionSensorBase()
        : m_id(0)
        , m_type(SensorType::BodyDisplacement)
        , m_coordSystem(CoordSystem::GlobalFixed)
        , m_componentMode(ComponentMode::All)
        , m_isEnabled(true)
        , m_isUserRename(false)
    {
    }

    mMotionSensorBase::~mMotionSensorBase() {}

    void mMotionSensorBase::setId(int id) {
        m_id = id;
    }

    int mMotionSensorBase::getId() const {
        return m_id;
    }

    void mMotionSensorBase::setName(const QString& name) {
        m_name = name;
    }

    void mMotionSensorBase::setName(const QString& name, bool isUserRename) {
        m_name = name;
        m_isUserRename = isUserRename;
    }

    bool mMotionSensorBase::getIsUserRename() const {
        return m_isUserRename;
    }

    void mMotionSensorBase::setType(SensorType type) {
        m_type = type;
    }

    QString mMotionSensorBase::getName() const {
        return m_name;
    }

    QString mMotionSensorBase::getTypeName() const {
        return getSensorTypeName(m_type);
    }

    void mMotionSensorBase::setCoordSystem(CoordSystem coord) {
        m_coordSystem = coord;
    }

    CoordSystem mMotionSensorBase::getCoordSystem() const {
        return m_coordSystem;
    }

    void mMotionSensorBase::setComponentMode(ComponentMode mode) {
        m_componentMode = mode;
    }

    ComponentMode mMotionSensorBase::getComponentMode() const {
        return m_componentMode;
    }

    void mMotionSensorBase::setEnabled(bool enabled) {
        m_isEnabled = enabled;
    }

    bool mMotionSensorBase::isEnabled() const {
        return m_isEnabled;
    }

    // --- 序列化 ---
    QDataStream& operator>>(QDataStream& in, mMotionSensorBase& sensor) {
        int typeInt = 0, coordInt = 0, compInt = 0;
        in >> sensor.m_id;
        in >> sensor.m_name;
        in >> typeInt;
        sensor.m_type = static_cast<SensorType>(typeInt);
        in >> coordInt;
        sensor.m_coordSystem = static_cast<CoordSystem>(coordInt);
        in >> compInt;
        sensor.m_componentMode = static_cast<ComponentMode>(compInt);
        in >> sensor.m_isEnabled;
        in >> sensor.m_isUserRename;
        sensor.readFromStream(in);
        return in;
    }

    QDataStream& operator<<(QDataStream& out, const mMotionSensorBase& sensor) {
        out << sensor.m_id;
        out << sensor.m_name;
        out << static_cast<int>(sensor.m_type);
        out << static_cast<int>(sensor.m_coordSystem);
        out << static_cast<int>(sensor.m_componentMode);
        out << sensor.m_isEnabled;
        out << sensor.m_isUserRename;
        sensor.writeToStream(out);
        return out;
    }

}
