#include "mMotionMarker.h"
#include "mMotionMarkerManager.h"

namespace MechMotionData
{
    mMotionMarkerManager::mMotionMarkerManager() : m_maxID(0) {}

    mMotionMarkerManager::~mMotionMarkerManager() {
        deleteAll();
    }

    mMotionMarker* mMotionMarkerManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionMarker* mMotionMarkerManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionMarkerManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionMarkerManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionMarkerManager::addData(mMotionMarker* marker) {
        if (marker) {
            m_dataMap.insert({ marker->getId(), marker });
            if (marker->getId() > m_maxID) {
                m_maxID = marker->getId();
            }
        }
    }

    bool mMotionMarkerManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionMarkerManager::resetName(const QString& oldName, const QString& newName) {
        mMotionMarker* marker = getData(oldName);
        if (marker) marker->setName(newName);
    }

    void mMotionMarkerManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionMarkerManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionMarkerManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionMarkerManager::getAllNames() const
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

    int mMotionMarkerManager::createID() {
        return ++m_maxID;
    }

    int mMotionMarkerManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionMarkerManager::copyData(const int& id)
    {
        mMotionMarker* marker = getData(id);
        if (marker == nullptr)
            return 0;

        QString copyName = marker->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionMarker* copyMarker = marker->copyMarker(createID(), copyName);
        addData(copyMarker);
        return copyMarker->getId();
    }

    // operator>>/operator<< 已集中到 mMotionDataInStream.h / mMotionDataOutStream.h

}
