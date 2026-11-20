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
    class mMotionSpringBase;

    /**
     * @brief 弹簧管理类
     * @details 管理所有弹簧对象（拉压弹簧、扭转弹簧）的增删改查、ID分配
     */
    class MXMOTIONDATA_EXPORT mMotionSpringManager {
    public:
        mMotionSpringManager();
        ~mMotionSpringManager();

        mMotionSpringBase* getData(int id) const;

        mMotionSpringBase* getData(const QString& name) const;

        QString getNameByID(int id) const;

        int getIDByName(const QString& name) const;

        void addData(mMotionSpringBase* spring);

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

        friend QDataStream& operator>>(QDataStream& in, mMotionSpringManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionSpringManager& mgr);

    private:
        std::map<int, mMotionSpringBase*> m_dataMap; ///< 数据字典
        int m_maxID;                                  ///< 当前最大ID分配器
    };

}
