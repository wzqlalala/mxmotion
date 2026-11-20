#include "mMotionRL.h"
#include "mMotionRLManager.h"

namespace MechMotionData
{
    mMotionRLManager::mMotionRLManager() : m_maxID(0) {}

    mMotionRLManager::~mMotionRLManager() {
        deleteAll();
    }

    mMotionRL* mMotionRLManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionRL* mMotionRLManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionRLManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionRLManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionRLManager::addData(mMotionRL* data) {
        if (data) {
            m_dataMap.insert({ data->getId(), data });
            if (data->getId() > m_maxID) {
                m_maxID = data->getId();
            }
        }
    }

    bool mMotionRLManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionRLManager::resetName(const QString& oldName, const QString& newName) {
        mMotionRL* data = getData(oldName);
        if (data) data->setName(newName);
    }

    void mMotionRLManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionRLManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionRLManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionRLManager::getAllNames() const {
        QStringList names;
        for (auto it : m_dataMap) {
            if (it.second == nullptr) continue;
            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionRLManager::createID() {
        return ++m_maxID;
    }

    int mMotionRLManager::getMaxID() const {
        return m_maxID;
    }

    int mMotionRLManager::copyData(const int& id) {
        mMotionRL* data = getData(id);
        if (data == nullptr) return 0;
        QString copyName = data->getName() + "-copy";
        while (existName(copyName)) {
            copyName = copyName + "-copy";
        }
        mMotionRL* copyObj = new mMotionRL(createID(), copyName);
        copyObj->setAlgorithm(data->getAlgorithm());
        copyObj->setEnvType(data->getEnvType());
        copyObj->setStateSpace(data->getStateSpace());
        copyObj->setActionSpace(data->getActionSpace());
        copyObj->setHiddenLayers(data->getHiddenLayers());
        copyObj->setLearningRate(data->getLearningRate());
        copyObj->setDiscountFactor(data->getDiscountFactor());
        copyObj->setMaxEpisodes(data->getMaxEpisodes());
        copyObj->setMaxSteps(data->getMaxSteps());
        copyObj->setDatasetPath(data->getDatasetPath());
        copyObj->setModelSavePath(data->getModelSavePath());
        addData(copyObj);
        return copyObj->getId();
    }

}
