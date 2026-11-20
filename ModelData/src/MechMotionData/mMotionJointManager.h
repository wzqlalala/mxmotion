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
    class mMotionJointBase;

    /**
     * @brief 运动副数据管理类
     * @details 管理所有的运动副基类指针，支持多态
     */
    class MXMOTIONDATA_EXPORT mMotionJointManager {
    public:
        mMotionJointManager();
        ~mMotionJointManager();

        mMotionJointBase* getData(int id) const;
        void addData(mMotionJointBase* joint);
        void deleteData(int id);
        void deleteAll();
        int createID();

        mMotionJointBase* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        std::vector<int> getAllIDs() const;

        QStringList getAllNames() const;

        int getMaxID() const;

        /*
        * 获取所有运动副名称根据类型
        */
        QStringList getAllNamesByTypes(QVector<MxFunctions::JointType> type);

        //复制
        int copyData(const int& id);
        
        friend QDataStream& operator>>(QDataStream& in, mMotionJointManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionJointManager& mgr);

    private:
        std::map<int, mMotionJointBase*> m_dataMap;
        int m_maxID;
    };

}