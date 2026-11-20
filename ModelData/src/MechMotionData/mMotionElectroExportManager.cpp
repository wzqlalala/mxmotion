#include "mMotionElectroExport.h"
#include "mMotionElectroExportManager.h"

namespace MechMotionData
{
    mMotionElectroExportManager::mMotionElectroExportManager() : m_maxID(0) {}

    mMotionElectroExportManager::~mMotionElectroExportManager() {
        deleteAll();
    }

    mMotionElectroExport* mMotionElectroExportManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionElectroExport* mMotionElectroExportManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionElectroExportManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionElectroExportManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionElectroExportManager::addData(mMotionElectroExport* data) {
        if (data) {
            m_dataMap.insert({ data->getId(), data });
            if (data->getId() > m_maxID) {
                m_maxID = data->getId();
            }
        }
    }

    bool mMotionElectroExportManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionElectroExportManager::resetName(const QString& oldName, const QString& newName) {
        mMotionElectroExport* data = getData(oldName);
        if (data) data->setName(newName);
    }

    void mMotionElectroExportManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionElectroExportManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionElectroExportManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionElectroExportManager::getAllNames() const {
        QStringList names;
        for (auto it : m_dataMap) {
            if (it.second == nullptr) continue;
            names.append(it.second->getName());
        }
        return names;
    }

    int mMotionElectroExportManager::createID() {
        return ++m_maxID;
    }

    int mMotionElectroExportManager::getMaxID() const {
        return m_maxID;
    }

    int mMotionElectroExportManager::copyData(const int& id) {
        mMotionElectroExport* data = getData(id);
        if (data == nullptr) return 0;
        QString copyName = data->getName() + "-copy";
        while (existName(copyName)) {
            copyName = copyName + "-copy";
        }
        mMotionElectroExport* copyObj = new mMotionElectroExport(createID(), copyName);
        copyObj->setExportType(data->getExportType());
        copyObj->setPortListFile(data->getPortListFile());
        copyObj->setInputPorts(data->getInputPorts());
        copyObj->setOutputPorts(data->getOutputPorts());
        addData(copyObj);
        return copyObj->getId();
    }

}
