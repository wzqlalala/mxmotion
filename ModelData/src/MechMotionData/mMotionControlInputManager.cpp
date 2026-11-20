#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"

namespace MechMotionData
{
    mMotionControlInputManager::mMotionControlInputManager() : m_maxID(0) {}

    mMotionControlInputManager::~mMotionControlInputManager() {
        deleteAll();
    }

    mMotionControlInput* mMotionControlInputManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionControlInput* mMotionControlInputManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionControlInputManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionControlInputManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionControlInputManager::addData(mMotionControlInput* data) {
        if (data) {
            m_dataMap.insert({ data->getId(), data });
            if (data->getId() > m_maxID) {
                m_maxID = data->getId();
            }
        }
    }

    bool mMotionControlInputManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionControlInputManager::resetName(const QString& oldName, const QString& newName) {
        mMotionControlInput* data = getData(oldName);
        if (data) data->setName(newName);
    }

    void mMotionControlInputManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionControlInputManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionControlInputManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionControlInputManager::getAllNames() const {
        QStringList names;
        for (auto it : m_dataMap) {
            if (it.second == nullptr) continue;
            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionControlInputManager::createID() {
        return ++m_maxID;
    }

    int mMotionControlInputManager::getMaxID() const {
        return m_maxID;
    }

    int mMotionControlInputManager::copyData(const int& id) {
        mMotionControlInput* data = getData(id);
        if (data == nullptr) return 0;
        QString copyName = data->getName() + "-copy";
        while (existName(copyName)) {
            copyName = copyName + "-copy";
        }
        mMotionControlInput* copyObj = new mMotionControlInput(createID(), copyName);
        copyObj->setInputType(data->getInputType());
        copyObj->setTimeStart(data->getTimeStart());
        copyObj->setTimeEnd(data->getTimeEnd());
        copyObj->setPolyCoeffs(data->getPolyCoeffs());
        copyObj->setCurvePoints(data->getCurvePoints());
        addData(copyObj);
        return copyObj->getId();
    }

}
