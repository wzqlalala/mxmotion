#include "mMotionSensorManager.h"
#include "mMotionSensorBase.h"
#include "mMotionBodySensor.h"
#include "mMotionJointSensor.h"
#include "mMotionDriveSensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"

namespace MechMotionData {

    mMotionSensorManager::mMotionSensorManager()
        : m_maxID(0)
    {
    }

    mMotionSensorManager::~mMotionSensorManager() {
        deleteAll();
    }

    mMotionSensorBase* mMotionSensorManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    mMotionSensorBase* mMotionSensorManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionSensorManager::getNameByID(int id) const {
        auto data = getData(id);
        if (data) {
            return data->getName();
        }
        return QString();
    }

    int mMotionSensorManager::getIDByName(const QString& name) const {
        auto data = getData(name);
        if (data) {
            return data->getId();
        }
        return -1;
    }

    void mMotionSensorManager::addData(mMotionSensorBase* sensor) {
        if (!sensor) return;
        m_dataMap[sensor->getId()] = sensor;
        if (sensor->getId() > m_maxID) {
            m_maxID = sensor->getId();
        }
    }

    bool mMotionSensorManager::existName(const QString& name) const {
        return getData(name) != nullptr;
    }

    void mMotionSensorManager::resetName(const QString& oldName, const QString& newName) {
        auto data = getData(oldName);
        if (data) {
            data->setName(newName);
        }
    }

    void mMotionSensorManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionSensorManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
        m_maxID = 0;
    }

    std::vector<int> mMotionSensorManager::getAllIDs() const {
        std::vector<int> ids;
        ids.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            ids.push_back(pair.first);
        }
        return ids;
    }

    QStringList mMotionSensorManager::getAllNames() const {
        QStringList names;
        for (const auto& pair : m_dataMap) {
            if (pair.second) {
                names.append(pair.second->getName());
            }
        }
        return names;
    }

    int mMotionSensorManager::createID() {
        return ++m_maxID;
    }

    int mMotionSensorManager::getMaxID() const {
        return m_maxID;
    }

    std::vector<int> mMotionSensorManager::getIDsByType(SensorType type) const {
        std::vector<int> ids;
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getType() == type) {
                ids.push_back(pair.first);
            }
        }
        return ids;
    }

    int mMotionSensorManager::copyData(const int& id) {
        auto data = getData(id);
        if (!data) return -1;

        int newId = createID();
        QString newName = data->getName() + QStringLiteral("_副本");
        mMotionSensorBase* copy = data->copySensor(newId, newName);
        addData(copy);
        return newId;
    }

    QDataStream& operator>>(QDataStream& in, mMotionSensorManager& mgr) {
        int count = 0;
        in >> count;
        mgr.deleteAll();
        for (int i = 0; i < count; ++i) {
            int typeInt = 0;
            in >> typeInt;
            SensorType type = static_cast<SensorType>(typeInt);
            mMotionSensorBase* sensor = nullptr;
            switch (type) {
            case SensorType::BodyDisplacement:
            case SensorType::BodyVelocity:
            case SensorType::BodyAcceleration:
                sensor = new mMotionBodySensor();
                break;
            case SensorType::JointReaction:
                sensor = new mMotionJointSensor();
                break;
            case SensorType::DriveActuation:
                sensor = new mMotionDriveSensor();
                break;
            case SensorType::Distance:
                sensor = new mMotionDistanceSensor();
                break;
            case SensorType::Angle:
                sensor = new mMotionAngleSensor();
                break;
            }
            if (sensor) {
                in >> *sensor;
                mgr.addData(sensor);
            }
        }
        return in;
    }

    QDataStream& operator<<(QDataStream& out, mMotionSensorManager& mgr) {
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
