#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechMotionData 
{
    class mMotionBody;
    /**
     * @brief 运动体数据管理类
     */
    class MXMOTIONDATA_EXPORT mMotionBodyManager {
    public:
        mMotionBodyManager();
        ~mMotionBodyManager();

        mMotionBody* getData(int id) const;

        mMotionBody* getData(const QString& name) const;

        QString getNameByID(int id) const;

        int getIDByName(const QString& name) const;

        void addData(mMotionBody* body);

        bool existName(const QString& name) const;

        void resetName(const QString& oldName, const QString& newName);

        void deleteData(int id);

        void deleteAll();

        std::vector<int> getAllIDs() const;

        QStringList getAllNames() const;

        int createID();

        int getMaxID() const;

        //复制
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionBodyManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionBodyManager& mgr);

    private:
        std::map<int, mMotionBody*> m_dataMap; ///< 数据字典
        int m_maxID;                          ///< 当前最大ID分配器
    };

}