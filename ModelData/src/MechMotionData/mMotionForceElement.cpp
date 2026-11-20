#include "mMotionForceElement.h"

namespace MechMotionData {

    mMotionForceElement::mMotionForceElement()
        : m_id(-1),
        m_name(),
        m_isUserRename(false)
    {
    }

    mMotionForceElement::mMotionForceElement(int id, const QString& name, bool isUserRename)
        : m_id(id),
        m_name(name),
        m_isUserRename(isUserRename)
    {
    }

    mMotionForceElement::~mMotionForceElement() {}

    void mMotionForceElement::setId(int id) { m_id = id; }
    int mMotionForceElement::getId() const { return m_id; }

    void mMotionForceElement::setName(const QString& name, bool isUserRename)
    {
        m_name = name;
        m_isUserRename = isUserRename;
    }
    QString mMotionForceElement::getName() const { return m_name; }
    bool mMotionForceElement::getIsUserRename() const { return m_isUserRename; }

}
