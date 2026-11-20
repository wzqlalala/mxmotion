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
    class mMotionMarker;

    /**
     * @brief 标记点管理类
     */
    class MXMOTIONDATA_EXPORT mMotionMarkerManager {
    public:
        mMotionMarkerManager();
        ~mMotionMarkerManager();

        mMotionMarker* getData(int id) const;

        mMotionMarker* getData(const QString& name) const;

        QString getNameByID(int id) const;

        int getIDByName(const QString& name) const;

        void addData(mMotionMarker* marker);

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

        friend QDataStream& operator>>(QDataStream& in, mMotionMarkerManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionMarkerManager& mgr);

    private:
        std::map<int, mMotionMarker*> m_dataMap; ///< 数据字典
        int m_maxID;                             ///< 当前最大ID分配器
    };

}
