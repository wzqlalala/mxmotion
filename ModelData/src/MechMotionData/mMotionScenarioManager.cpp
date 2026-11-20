#include "mMotionScenarioManager.h"
#include "mMotionScenario.h"

namespace MechMotionData {

    mMotionScenarioManager::mMotionScenarioManager() : m_maxID(0) {}

    mMotionScenarioManager::~mMotionScenarioManager() {
        deleteAll();
    }

    mMotionScenario* mMotionScenarioManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionScenario* mMotionScenarioManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionScenarioManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end() && it->second) return it->second->getName();
        return QString();
    }

    int mMotionScenarioManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionScenarioManager::addData(mMotionScenario* config) {
        if (config) {
            m_dataMap.insert({ config->getId(), config });
            if (config->getId() > m_maxID) {
                m_maxID = config->getId();
            }
        }
    }

    bool mMotionScenarioManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionScenarioManager::resetName(const QString& oldName, const QString& newName) {
        mMotionScenario* config = getData(oldName);
        if (config) config->setName(newName);
    }

    void mMotionScenarioManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }
    void mMotionScenarioManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            if (pair.second) delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionScenarioManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionScenarioManager::getAllNames() const
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

    int mMotionScenarioManager::createID() {
        return ++m_maxID;
    }


    int mMotionScenarioManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionScenarioManager::copyData(const int& id)
    {
        mMotionScenario* scenario = getData(id);
        if (scenario == nullptr)
            return 0;

        QString copyName = scenario->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionScenario* copyScenario = scenario->copyScenario(createID(), copyName);
        addData(copyScenario);
        return copyScenario->getId();
    }
}