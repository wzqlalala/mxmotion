#include "mMotionControllerBase.h"

namespace MechMotionData {

    mMotionControllerBase::mMotionControllerBase(int id, ControllerType type, const QString& name, bool isUserRename)
        : m_id(id)
        , m_type(type)
        , m_name(name)
        , m_isEnabled(true)
        , m_isUserRename(isUserRename)
    {
        if (m_name.isEmpty()) {
            m_name = QString("%1_%2").arg(getControllerTypeName(type)).arg(m_id);
        }
    }

    mMotionControllerBase::mMotionControllerBase()
        : m_id(0)
        , m_type(ControllerType::PID)
        , m_isEnabled(true)
        , m_isUserRename(false)
    {
    }

    mMotionControllerBase::~mMotionControllerBase() {}

    void mMotionControllerBase::setId(int id) {
        m_id = id;
    }

    int mMotionControllerBase::getId() const {
        return m_id;
    }

    void mMotionControllerBase::setName(const QString& name) {
        m_name = name;
    }

    void mMotionControllerBase::setName(const QString& name, bool isUserRename) {
        m_name = name;
        m_isUserRename = isUserRename;
    }

    bool mMotionControllerBase::getIsUserRename() const {
        return m_isUserRename;
    }

    void mMotionControllerBase::setType(ControllerType type) {
        m_type = type;
    }

    QString mMotionControllerBase::getName() const {
        return m_name;
    }

    QString mMotionControllerBase::getTypeName() const {
        return getControllerTypeName(m_type);
    }

    void mMotionControllerBase::setEnabled(bool enabled) {
        m_isEnabled = enabled;
    }

    bool mMotionControllerBase::isEnabled() const {
        return m_isEnabled;
    }

    // --- 序列化 ---
    QDataStream& operator>>(QDataStream& in, mMotionControllerBase& controller) {
        int typeInt = 0;
        in >> controller.m_id;
        in >> controller.m_name;
        in >> typeInt;
        controller.m_type = static_cast<ControllerType>(typeInt);
        in >> controller.m_isEnabled;
        in >> controller.m_isUserRename;
        controller.readFromStream(in);
        return in;
    }

    QDataStream& operator<<(QDataStream& out, const mMotionControllerBase& controller) {
        out << controller.m_id;
        out << controller.m_name;
        out << static_cast<int>(controller.m_type);
        out << controller.m_isEnabled;
        out << controller.m_isUserRename;
        controller.writeToStream(out);
        return out;
    }

}
