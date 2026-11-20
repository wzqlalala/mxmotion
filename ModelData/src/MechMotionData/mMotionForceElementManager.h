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
    class mMotionForceElement;

    /**
     * @brief 力元管理类
     * @details 统一管理软件中所有力元对象（矢量力/力矩/衬套/标量力等）的增删改查、ID分配
     */
    class MXMOTIONDATA_EXPORT mMotionForceElementManager {
    public:
        mMotionForceElementManager();
        ~mMotionForceElementManager();

        mMotionForceElement* getData(int id) const;

        mMotionForceElement* getData(const QString& name) const;

        QString getNameByID(int id) const;

        int getIDByName(const QString& name) const;

        void addData(mMotionForceElement* force);

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

        friend QDataStream& operator>>(QDataStream& in, mMotionForceElementManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionForceElementManager& mgr);

    private:
        std::map<int, mMotionForceElement*> m_dataMap; ///< 数据字典
        int m_maxID;                                   ///< 当前最大ID分配器
    };

}
