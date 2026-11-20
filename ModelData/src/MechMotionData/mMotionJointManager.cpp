#include "mMotionJointBase.h"
#include "mMotionJointManager.h"

#include "mPhysicalFieldEnum.h"

namespace MechMotionData
{
    mMotionJointManager::mMotionJointManager() : m_maxID(0) {}

    mMotionJointManager::~mMotionJointManager() {
        deleteAll();
    }

    mMotionJointBase* mMotionJointManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            return it->second;
        }
        return nullptr;
    }

    void mMotionJointManager::addData(mMotionJointBase* joint) {
        if (joint) {
            m_dataMap.insert({ joint->getId(), joint });
            if (joint->getId() > m_maxID) {
                m_maxID = joint->getId();
            }
        }
    }

    void mMotionJointManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second; // 触发虚析构函数，安全释放派生类
            m_dataMap.erase(it);
        }
    }

    void mMotionJointManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            if (pair.second) {
                delete pair.second;
            }
        }
        m_dataMap.clear();
    }

    int mMotionJointManager::createID() {
        return ++m_maxID;
    }

    mMotionJointBase* mMotionJointManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.second;
            }
        }
        return nullptr;
    }

    QString mMotionJointManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end() && it->second) {
            return it->second->getName();
        }
        return QString();
    }

    int mMotionJointManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) {
                return pair.first;
            }
        }
        return -1;
    }

    bool mMotionJointManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionJointManager::resetName(const QString& oldName, const QString& newName) {
        mMotionJointBase* joint = getData(oldName);
        if (joint) {
            joint->setName(newName);
        }
    }

    std::vector<int> mMotionJointManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    QStringList mMotionJointManager::getAllNames() const
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

    int mMotionJointManager::getMaxID() const
    {
        return m_maxID;
    }

    QStringList mMotionJointManager::getAllNamesByTypes(QVector<MxFunctions::JointType> types)
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

    int mMotionJointManager::copyData(const int& id)
    {
        mMotionJointBase* joint = getData(id);
        if (joint == nullptr)
            return 0;

        QString copyName = joint->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        mMotionJointBase* copyJoint = joint->copyJoint(createID(), copyName);
        addData(copyJoint);
        return copyJoint->getId();
    }
}