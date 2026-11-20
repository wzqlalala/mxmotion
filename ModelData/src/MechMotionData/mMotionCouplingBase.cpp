#include "mMotionCouplingBase.h"

#include <QMatrix4x4>
#include <QStringLiteral>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionCouplingBase::mMotionCouplingBase(int id, CouplingType type, const QString& name, bool isUserRename)
        : m_id(id), m_type(type), m_name(name), m_isUserRename(isUserRename)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Coupling_%1").arg(m_id);
        }
    }

    QString mMotionCouplingBase::getTypeName() const {
        return getTypeName(m_type);
    }

    QString mMotionCouplingBase::getTypeName(CouplingType type) {
        switch (type) {
        case CouplingType::GearRack:
            return QStringLiteral("齿轮齿条副");
        case CouplingType::Gear:
            return QStringLiteral("齿轮副");
        case CouplingType::Cable:
            return QStringLiteral("线缆副");
        default:
            return QStringLiteral("未知类型");
        }
    }

    mMotionCouplingBase::~mMotionCouplingBase() {}

    bool mMotionCouplingBase::getIsUserRename()
    {
        return m_isUserRename;
    }

    int mMotionCouplingBase::getId() const { return m_id; }

    QString mMotionCouplingBase::getName() const { return m_name; }

    void mMotionCouplingBase::setName(const QString& name, bool isUserRename) { 
        m_name = name; 
        m_isUserRename = isUserRename;
    }

    void mMotionCouplingBase::setType(CouplingType type)
    {
        m_type = type;
    }

    CouplingType mMotionCouplingBase::getType() const { return m_type; }




    QDataStream& operator<<(QDataStream& out, mMotionCouplingBase& mgr) {
        out << static_cast<int>(mgr.m_type);
        out << mgr.m_id;
        out << mgr.m_name;
        out << mgr.m_isUserRename;
        return out;
    }

    QDataStream& operator>>(QDataStream& in, mMotionCouplingBase& mgr) {
        int type = 0;
        in >> type;
        mgr.m_type = static_cast<CouplingType>(type);
        in >> mgr.m_id;
        in >> mgr.m_name;
        in >> mgr.m_isUserRename;
        return in;
    }
}
