#include "mMotionForceElement.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionForceElementManager.h"

namespace MechMotionData
{
    mMotionForceElementManager::mMotionForceElementManager() : m_maxID(0) {}

    mMotionForceElementManager::~mMotionForceElementManager() {
        deleteAll();
    }

    mMotionForceElement* mMotionForceElementManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionForceElement* mMotionForceElementManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionForceElementManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionForceElementManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionForceElementManager::addData(mMotionForceElement* force) {
        if (force) {
            m_dataMap.insert({ force->getId(), force });
            if (force->getId() > m_maxID) {
                m_maxID = force->getId();
            }
        }
    }

    bool mMotionForceElementManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionForceElementManager::resetName(const QString& oldName, const QString& newName) {
        mMotionForceElement* force = getData(oldName);
        if (force) force->setName(newName);
    }

    void mMotionForceElementManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionForceElementManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionForceElementManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionForceElementManager::getAllNames() const
    {
        QStringList names;
        for (auto it : m_dataMap)
        {
            if (it.second == nullptr)
                continue;

            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionForceElementManager::createID() {
        return ++m_maxID;
    }

    int mMotionForceElementManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionForceElementManager::copyData(const int& id)
    {
        mMotionForceElement* force = getData(id);
        if (force == nullptr)
            return 0;

        QString copyName = force->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionForceElement* copyForce = force->copyForceElement(createID(), copyName);
        addData(copyForce);
        return copyForce->getId();
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
