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
    class mMotionCouplingBase;

    /**
     * @brief 耦合副数据管理类
     * @details 管理所有的耦合副基类指针，支持多态
     */
    class MXMOTIONDATA_EXPORT mMotionCouplingManager {
    public:
        mMotionCouplingManager();
        ~mMotionCouplingManager();

        mMotionCouplingBase* getData(int id) const;
        void addData(mMotionCouplingBase* coupling);
        void deleteData(int id);
        void deleteAll();
        int createID();

        mMotionCouplingBase* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        std::vector<int> getAllIDs() const;

        QStringList getAllNames() const;

        int getMaxID() const;

        /*
        * 获取所有耦合副名称根据类型
        */
        QStringList getAllNamesByTypes(QVector<MxFunctions::CouplingType> types);

        //复制
        int copyData(const int& id);
        
        friend QDataStream& operator>>(QDataStream& in, mMotionCouplingManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCouplingManager& mgr);

    private:
        std::map<int, mMotionCouplingBase*> m_dataMap;
        int m_maxID;
    };

}
