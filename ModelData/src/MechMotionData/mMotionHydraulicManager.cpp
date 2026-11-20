#include "mMotionHydraulic.h"
#include "mMotionHydraulicManager.h"

namespace MechMotionData
{
    mMotionHydraulicManager::mMotionHydraulicManager() : m_maxID(0) {}

    mMotionHydraulicManager::~mMotionHydraulicManager() {
        deleteAll();
    }

    mMotionHydraulic* mMotionHydraulicManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionHydraulic* mMotionHydraulicManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionHydraulicManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionHydraulicManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionHydraulicManager::addData(mMotionHydraulic* data) {
        if (data) {
            m_dataMap.insert({ data->getId(), data });
            if (data->getId() > m_maxID) {
                m_maxID = data->getId();
            }
        }
    }

    bool mMotionHydraulicManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionHydraulicManager::resetName(const QString& oldName, const QString& newName) {
        mMotionHydraulic* data = getData(oldName);
        if (data) data->setName(newName);
    }

    void mMotionHydraulicManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionHydraulicManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionHydraulicManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionHydraulicManager::getAllNames() const {
        QStringList names;
        for (auto it : m_dataMap) {
            if (it.second == nullptr) continue;
            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionHydraulicManager::createID() {
        return ++m_maxID;
    }

    int mMotionHydraulicManager::getMaxID() const {
        return m_maxID;
    }

    int mMotionHydraulicManager::copyData(const int& id) {
        mMotionHydraulic* data = getData(id);
        if (data == nullptr) return 0;
        QString copyName = data->getName() + "-copy";
        while (existName(copyName)) {
            copyName = copyName + "-copy";
        }
        mMotionHydraulic* copyObj = new mMotionHydraulic(createID(), copyName);
        copyObj->setHydraulicType(data->getHydraulicType());
        copyObj->setModelFile(data->getModelFile());
        copyObj->setFluidDensity(data->getFluidDensity());
        copyObj->setFluidViscosity(data->getFluidViscosity());
        copyObj->setBulkModulus(data->getBulkModulus());
        copyObj->setPorts(data->getPorts());
        copyObj->setPipes(data->getPipes());
        addData(copyObj);
        return copyObj->getId();
    }

}
