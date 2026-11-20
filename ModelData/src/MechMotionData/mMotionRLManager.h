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
    class mMotionRL;

    class MXMOTIONDATA_EXPORT mMotionRLManager {
    public:
        mMotionRLManager();
        ~mMotionRLManager();

        mMotionRL* getData(int id) const;
        mMotionRL* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;
        void addData(mMotionRL* data);
        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        void deleteData(int id);
        void deleteAll();
        std::vector<int> getAllIDs() const;
        QStringList getAllNames() const;
        int createID();
        int getMaxID() const;
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionRLManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionRLManager& mgr);

    private:
        std::map<int, mMotionRL*> m_dataMap;
        int m_maxID;
    };

}
