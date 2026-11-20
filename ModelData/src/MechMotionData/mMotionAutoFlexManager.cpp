#include "mMotionAutoFlexManager.h"
#include "mMotionAutoFlex.h"

namespace MechMotionData {

    mMotionAutoFlexManager::mMotionAutoFlexManager()
        : m_maxID(0)
    {
    }

    mMotionAutoFlexManager::~mMotionAutoFlexManager()
    {
        deleteAll();
    }

    mMotionAutoFlex* mMotionAutoFlexManager::getData(int id) const
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end())
            return it->second;
        return nullptr;
    }

    mMotionAutoFlex* mMotionAutoFlexManager::getData(const QString& name) const
    {
        for (auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name)
                return pair.second;
        }
        return nullptr;
    }

    QString mMotionAutoFlexManager::getNameByID(int id) const
    {
        auto* data = getData(id);
        if (data)
            return data->getName();
        return QString();
    }

    int mMotionAutoFlexManager::getIDByName(const QString& name) const
    {
        auto* data = getData(name);
        if (data)
            return data->getId();
        return -1;
    }

    void mMotionAutoFlexManager::addData(mMotionAutoFlex* data)
    {
        if (!data) return;
        m_dataMap[data->getId()] = data;
        if (data->getId() > m_maxID)
            m_maxID = data->getId();
    }

    bool mMotionAutoFlexManager::existName(const QString& name) const
    {
        for (auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name)
                return true;
        }
        return false;
    }

    void mMotionAutoFlexManager::resetName(const QString& oldName, const QString& newName)
    {
        auto* data = getData(oldName);
        if (data)
            data->setName(newName);
    }

    void mMotionAutoFlexManager::deleteData(int id)
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionAutoFlexManager::deleteAll()
    {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
        m_maxID = 0;
    }

    std::vector<int> mMotionAutoFlexManager::getAllIDs() const
    {
        std::vector<int> ids;
        for (auto& pair : m_dataMap) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    QStringList mMotionAutoFlexManager::getAllNames() const
    {
        QStringList names;
        for (auto& pair : m_dataMap) {
            if (pair.second)
                names.append(pair.second->getName());
        }
        return names;
    }

    int mMotionAutoFlexManager::createID()
    {
        return ++m_maxID;
    }

    int mMotionAutoFlexManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionAutoFlexManager::copyData(const int& id)
    {
        auto* src = getData(id);
        if (!src) return -1;

        int newId = createID();
        QString newName = src->getName() + "_副本";
        mMotionAutoFlex* newData = src->copy(newId, newName);
        addData(newData);
        return newId;
    }

    QDataStream& operator>>(QDataStream& in, mMotionAutoFlexManager& mgr)
    {
        int count = 0;
        in >> count;
        for (int i = 0; i < count; ++i) {
            mMotionAutoFlex* data = new mMotionAutoFlex();
            in >> *data;
            mgr.addData(data);
        }
        return in;
    }

    QDataStream& operator<<(QDataStream& out, mMotionAutoFlexManager& mgr)
    {
        int count = static_cast<int>(mgr.m_dataMap.size());
        out << count;
        for (auto& pair : mgr.m_dataMap) {
            if (pair.second)
                out << *pair.second;
        }
        return out;
    }

}
