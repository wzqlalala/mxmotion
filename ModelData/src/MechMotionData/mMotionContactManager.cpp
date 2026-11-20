#include "mMotionContactManager.h"
#include "mMotionContact.h"

namespace MechMotionData
{
    mMotionContactManager::mMotionContactManager()
        : m_maxID(0)
    {
    }

    mMotionContactManager::~mMotionContactManager()
    {
        deleteAll();
    }

    mMotionContact* mMotionContactManager::getData(int id) const
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end())
            return it->second;
        return nullptr;
    }

    mMotionContact* mMotionContactManager::getData(const QString& name) const
    {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name)
                return pair.second;
        }
        return nullptr;
    }

    QString mMotionContactManager::getNameByID(int id) const
    {
        mMotionContact* data = getData(id);
        if (data)
            return data->getName();
        return QString();
    }

    int mMotionContactManager::getIDByName(const QString& name) const
    {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name)
                return pair.first;
        }
        return -1;
    }

    void mMotionContactManager::addData(mMotionContact* contact)
    {
        if (!contact) return;
        int id = contact->getId();
        if (id > m_maxID) m_maxID = id;
        m_dataMap[id] = contact;
    }

    bool mMotionContactManager::existName(const QString& name) const
    {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name)
                return true;
        }
        return false;
    }

    void mMotionContactManager::resetName(const QString& oldName, const QString& newName)
    {
        for (auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == oldName) {
                pair.second->setName(newName);
                break;
            }
        }
    }

    void mMotionContactManager::deleteData(int id)
    {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionContactManager::deleteAll()
    {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
        m_maxID = 0;
    }

    std::vector<int> mMotionContactManager::getAllIDs() const
    {
        std::vector<int> ids;
        for (const auto& pair : m_dataMap) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    QStringList mMotionContactManager::getAllNames() const
    {
        QStringList names;
        for (const auto& pair : m_dataMap) {
            if (pair.second)
                names.append(pair.second->getName());
        }
        return names;
    }

    int mMotionContactManager::createID()
    {
        return ++m_maxID;
    }

    int mMotionContactManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionContactManager::copyData(const int& id)
    {
        mMotionContact* src = getData(id);
        if (!src) return -1;

        int newId = createID();
        QString newName = src->getName() + "_copy";
        mMotionContact* copy = src->copyContact(newId, newName);
        addData(copy);
        return newId;
    }

}
