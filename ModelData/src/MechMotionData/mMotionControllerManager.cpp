#include "mMotionControllerManager.h"
#include "mMotionControllerBase.h"
#include "mMotionPIDController.h"

namespace MechMotionData {

    mMotionControllerManager::mMotionControllerManager()
        : m_maxID(0)
    {
    }

    mMotionControllerManager::~mMotionControllerManager() {
        deleteAll();
    }

    mMotionControllerBase* mMotionControllerManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    mMotionControllerBase* mMotionControllerManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionControllerManager::getNameByID(int id) const {
        auto data = getData(id);
        if (data) {
            return data->getName();
        }
        return QString();
    }

    int mMotionControllerManager::getIDByName(const QString& name) const {
        auto data = getData(name);
        if (data) {
            return data->getId();
        }
        return -1;
    }

    void mMotionControllerManager::addData(mMotionControllerBase* controller) {
        if (!controller) return;
        m_dataMap[controller->getId()] = controller;
        if (controller->getId() > m_maxID) {
            m_maxID = controller->getId();
        }
    }

    bool mMotionControllerManager::existName(const QString& name) const {
        return getData(name) != nullptr;
    }

    void mMotionControllerManager::resetName(const QString& oldName, const QString& newName) {
        auto data = getData(oldName);
        if (data) {
            data->setName(newName);
        }
    }

    void mMotionControllerManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionControllerManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
        m_maxID = 0;
    }

    std::vector<int> mMotionControllerManager::getAllIDs() const {
        std::vector<int> ids;
        ids.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    QStringList mMotionControllerManager::getAllNames() const {
        QStringList names;
        for (const auto& pair : m_dataMap) {
            if (pair.second) {
                names.append(pair.second->getName());
            }
        }
        return names;
    }

    int mMotionControllerManager::createID() {
        return ++m_maxID;
    }

    int mMotionControllerManager::getMaxID() const {
        return m_maxID;
    }

    std::vector<int> mMotionControllerManager::getIDsByType(ControllerType type) const {
        std::vector<int> ids;
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getType() == type) {
                ids.push_back(pair.first);
            }
        }
        return ids;
    }

    int mMotionControllerManager::copyData(const int& id) {
        auto data = getData(id);
        if (!data) return -1;

        int newId = createID();
        QString newName = data->getName() + QStringLiteral("_副本");
        mMotionControllerBase* copy = data->copyController(newId, newName);
        addData(copy);
        return newId;
    }

    QDataStream& operator>>(QDataStream& in, mMotionControllerManager& mgr) {
        int count = 0;
        in >> count;
        mgr.deleteAll();
        for (int i = 0; i < count; ++i) {
            int typeInt = 0;
            in >> typeInt;
            ControllerType type = static_cast<ControllerType>(typeInt);
            mMotionControllerBase* controller = nullptr;
            switch (type) {
            case ControllerType::PID:
                controller = new mMotionPIDController();
                break;
            }
            if (controller) {
                in >> *controller;
                mgr.addData(controller);
            }
        }
        return in;
    }

    QDataStream& operator<<(QDataStream& out, mMotionControllerManager& mgr) {
        int count = static_cast<int>(mgr.m_dataMap.size());
        out << count;
        for (auto& pair : mgr.m_dataMap) {
            if (pair.second) {
                out << static_cast<int>(pair.second->getType());
                out << *pair.second;
            }
        }
        return out;
    }

}
