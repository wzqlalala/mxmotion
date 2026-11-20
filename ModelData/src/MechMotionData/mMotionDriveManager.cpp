#include "mMotionDriveBase.h"
#include "mMotionDriveManager.h"

#include "mPhysicalFieldEnum.h"

namespace MechMotionData
{
    mMotionDriveManager::mMotionDriveManager() : m_maxID(0) {}

    mMotionDriveManager::~mMotionDriveManager() {
        deleteAll();
    }

    mMotionDriveBase* mMotionDriveManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void mMotionDriveManager::addData(mMotionDriveBase* drive) {
        if (drive) {
            m_dataMap.insert({ drive->getId(), drive });
            if (drive->getId() > m_maxID) {
                m_maxID = drive->getId();
            }
        }
    }

    void mMotionDriveManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second; // 安全多态释放
            m_dataMap.erase(it);
        }
    }

    void mMotionDriveManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            if (pair.second) {
                delete pair.second;
            }
        }
        m_dataMap.clear();
    }

    int mMotionDriveManager::createID() {
        return ++m_maxID;
    }

    mMotionDriveBase* mMotionDriveManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionDriveManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end() && it->second) {
            return it->second->getName();
        }
        return QString();
    }

    int mMotionDriveManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.first;
            }
        }
        return -1;
    }

    bool mMotionDriveManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionDriveManager::resetName(const QString& oldName, const QString& newName) {
        mMotionDriveBase* drive = getData(oldName);
        if (drive) {
            drive->setName(newName);
        }
    }

    std::vector<int> mMotionDriveManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    QStringList mMotionDriveManager::getAllNames() const
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

    int mMotionDriveManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionDriveManager::copyData(const int& id)
    {
        mMotionDriveBase* drive = getData(id);
        if (drive == nullptr)
            return 0;

        QString copyName = drive->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionDriveBase* copyDrive = drive->copyDrive(createID(), copyName);
        addData(copyDrive);
        return copyDrive->getId();
    }
}
