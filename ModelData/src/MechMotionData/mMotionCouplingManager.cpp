#include "mMotionCouplingBase.h"
#include "mMotionCouplingManager.h"

#include "mPhysicalFieldEnum.h"

namespace MechMotionData
{
    mMotionCouplingManager::mMotionCouplingManager() : m_maxID(0) {}

    mMotionCouplingManager::~mMotionCouplingManager() {
        deleteAll();
    }

    mMotionCouplingBase* mMotionCouplingManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void mMotionCouplingManager::addData(mMotionCouplingBase* coupling) {
        if (coupling) {
            m_dataMap.insert({ coupling->getId(), coupling });
            if (coupling->getId() > m_maxID) {
                m_maxID = coupling->getId();
            }
        }
    }

    void mMotionCouplingManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second; // 触发虚析构函数，安全释放派生类
            m_dataMap.erase(it);
        }
    }

    void mMotionCouplingManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            if (pair.second) {
                delete pair.second;
            }
        }
        m_dataMap.clear();
    }

    int mMotionCouplingManager::createID() {
        return ++m_maxID;
    }

    mMotionCouplingBase* mMotionCouplingManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionCouplingManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end() && it->second) {
            return it->second->getName();
        }
        return QString();
    }

    int mMotionCouplingManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.first;
            }
        }
        return -1;
    }

    bool mMotionCouplingManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionCouplingManager::resetName(const QString& oldName, const QString& newName) {
        mMotionCouplingBase* coupling = getData(oldName);
        if (coupling) {
            coupling->setName(newName);
        }
    }

    std::vector<int> mMotionCouplingManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    QStringList mMotionCouplingManager::getAllNames() const
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

    int mMotionCouplingManager::getMaxID() const
    {
        return m_maxID;
    }

    QStringList mMotionCouplingManager::getAllNamesByTypes(QVector<MxFunctions::CouplingType> types)
    {
        QStringList names;
        for (auto pair : m_dataMap)
        {
            if (!pair.second)
            {
                continue;
            }
            if (types.contains(pair.second->getType()))
            {
                names.append(pair.second->getName());
            }
        }
        return names;
    }

    int mMotionCouplingManager::copyData(const int& id)
    {
        mMotionCouplingBase* coupling = getData(id);
        if (coupling == nullptr)
            return 0;

        QString copyName = coupling->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionCouplingBase* copyCoupling = coupling->copyCoupling(createID(), copyName);
        addData(copyCoupling);
        return copyCoupling->getId();
    }
}
