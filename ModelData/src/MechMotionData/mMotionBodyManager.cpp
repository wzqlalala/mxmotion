#include "mMotionBody.h"
#include "mMotionBodyManager.h"

#include "mPhysicalFieldEnum.h"

namespace MechMotionData
{
    mMotionBodyManager::mMotionBodyManager() : m_maxID(0) {}

    mMotionBodyManager::~mMotionBodyManager() {
        deleteAll();
    }

    mMotionBody* mMotionBodyManager::getData(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second;
        return nullptr;
    }

    mMotionBody* mMotionBodyManager::getData(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.second;
        }
        return nullptr;
    }

    QString mMotionBodyManager::getNameByID(int id) const {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) return it->second->getName();
        return QString();
    }

    int mMotionBodyManager::getIDByName(const QString& name) const {
        for (const auto& pair : m_dataMap) {
            if (pair.second && pair.second->getName() == name) return pair.first;
        }
        return -1;
    }

    void mMotionBodyManager::addData(mMotionBody * body) {
        if (body) {
            m_dataMap.insert({ body->getId(), body });
            if (body->getId() > m_maxID) {
                m_maxID = body->getId();
            }
        }
    }

    bool mMotionBodyManager::existName(const QString& name) const {
        return getIDByName(name) != -1;
    }

    void mMotionBodyManager::resetName(const QString& oldName, const QString& newName) {
        mMotionBody* body = getData(oldName);
        if (body) body->setName(newName);
    }

    void mMotionBodyManager::deleteData(int id) {
        auto it = m_dataMap.find(id);
        if (it != m_dataMap.end()) {
            delete it->second;
            m_dataMap.erase(it);
        }
    }

    void mMotionBodyManager::deleteAll() {
        for (auto& pair : m_dataMap) {
            delete pair.second;
        }
        m_dataMap.clear();
    }

    std::vector<int> mMotionBodyManager::getAllIDs() const {
        std::vector<int> keys;
        keys.reserve(m_dataMap.size());
        for (const auto& pair : m_dataMap) keys.push_back(pair.first);
        return keys;
    }

    QStringList mMotionBodyManager::getAllNames() const
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

    int mMotionBodyManager::createID() {
        return ++m_maxID;
    }


    int mMotionBodyManager::getMaxID() const
    {
        return m_maxID;
    }

    int mMotionBodyManager::copyData(const int& id)
    {
        // 获取源数据
        mMotionBody* body = getData(id);
        if (body == nullptr)
            return 0;

        // 名字查重与生成
        QString copyName = body->getName() + "-copy";
        while (existName(copyName))
        {
            copyName = copyName + "-copy";
        }

        // 调用数据类自身的拷贝接口，传入新 ID 和新名字
        mMotionBody* copyBody = body->copyBody(createID(), copyName);

        // 加入管理器
        addData(copyBody);
        return copyBody->getId();
    }

}