#include "mMotionSpringBase.h"
#include "mMotionSpringManager.h"
#include "mMotionTranslationalSpring.h"
#include "mMotionTorsionalSpring.h"

#include "mPhysicalFieldEnum.h"

namespace MechMotionData
{
    mMotionSpringManager::mMotionSpringManager() : m_maxID(0) {}

    mMotionSpringManager::~mMotionSpringManager() {
        deleteAll();
    }

    mMotionSpringBase* mMotionSpringManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void mMotionSpringManager::addData(mMotionSpringBase* spring) {
        if (spring) {
            m_dataMap.insert({ spring->getId(), spring });
            if (spring->getId() > m_maxID) {
                m_maxID = spring->getId();
            }
        }
    }

    void mMotionSpringManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionSpringManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            if (pair.second) {
                delete pair.second;
            }
        }
        m_dataMap.clear();
    }

    int mMotionSpringManager::createID() {
        return ++m_maxID;
    }

    mMotionSpringBase* mMotionSpringManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionSpringManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end() && it->second) {
            return it->second->getName();
        }
        return QString();
    }

    int mMotionSpringManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.first;
            }
        }
        return -1;
    }

    bool mMotionSpringManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionSpringManager::resetName(const QString& oldName, const QString& newName) {
        mMotionSpringBase* spring = getData(oldName);
        if (spring) {
            spring->setName(newName);
        }
    }

    std::vector<int> mMotionSpringManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    QStringList mMotionSpringManager::getAllNames() const
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

    int mMotionSpringManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionSpringManager::copyData(const int& id)
    {
        mMotionSpringBase* spring = getData(id);
        if (spring == nullptr)
            return 0;

        QString copyName = spring->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionSpringBase* copySpring = spring->copySpring(createID(), copyName);
        addData(copySpring);
        return copySpring->getId();
    }
}
