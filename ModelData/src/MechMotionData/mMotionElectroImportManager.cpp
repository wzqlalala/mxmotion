#include "mMotionElectroImport.h"
#include "mMotionElectroImportManager.h"

namespace MechMotionData
{
    mMotionElectroImportManager::mMotionElectroImportManager() : m_maxID(0) {}

    mMotionElectroImportManager::~mMotionElectroImportManager() {
        deleteAll();
    }

    mMotionElectroImport* mMotionElectroImportManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionElectroImport* mMotionElectroImportManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionElectroImportManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionElectroImportManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionElectroImportManager::addData(mMotionElectroImport* data) {
        if (data) {
            m_dataMap.insert({ data->getId(), data });
            if (data->getId() > m_maxID) {
                m_maxID = data->getId();
            }
        }
    }

    bool mMotionElectroImportManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionElectroImportManager::resetName(const QString& oldName, const QString& newName) {
        mMotionElectroImport* data = getData(oldName);
        if (data) data->setName(newName);
    }

    void mMotionElectroImportManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionElectroImportManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionElectroImportManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionElectroImportManager::getAllNames() const {
        QStringList names;
        for (auto it : m_dataMap) {
            if (it.second == nullptr) continue;
            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionElectroImportManager::createID() {
        return ++m_maxID;
    }

    int mMotionElectroImportManager::getMaxID() const {
        return m_maxID;
    }

    int mMotionElectroImportManager::copyData(const int& id) {
        mMotionElectroImport* data = getData(id);
        if (data == nullptr) return 0;
        QString copyName = data->getName() + "-copy";
        while (existName(copyName)) {
            copyName = copyName + "-copy";
        }
        mMotionElectroImport* copyObj = new mMotionElectroImport(createID(), copyName);
        copyObj->setCoSimType(data->getCoSimType());
        copyObj->setPlatform(data->getPlatform());
        copyObj->setModelFile(data->getModelFile());
        copyObj->setHeaderFile(data->getHeaderFile());
        copyObj->setInputPorts(data->getInputPorts());
        copyObj->setOutputPorts(data->getOutputPorts());
        addData(copyObj);
        return copyObj->getId();
    }

}
