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
    class mMotionDriveBase;

    /**
     * @brief 驱动激励数据管理类
     * @details 管理所有的驱动基类指针，统一调度
     */
    class MXMOTIONDATA_EXPORT mMotionDriveManager {
    public:
        mMotionDriveManager();
        ~mMotionDriveManager();

        mMotionDriveBase* getData(int id) const;
        void addData(mMotionDriveBase* drive);
        void deleteData(int id);
        void deleteAll();
        int createID();

        mMotionDriveBase* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        std::vector<int> getAllIDs() const;
        QStringList getAllNames() const;

        int getMaxID() const;

        //复制
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionDriveManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionDriveManager& mgr);

    private:
        std::map<int, mMotionDriveBase*> m_dataMap;
        int m_maxID;
    };

}