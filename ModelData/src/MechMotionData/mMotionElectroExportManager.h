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
    class mMotionElectroExport;

    class MXMOTIONDATA_EXPORT mMotionElectroExportManager {
    public:
        mMotionElectroExportManager();
        ~mMotionElectroExportManager();

        mMotionElectroExport* getData(int id) const;
        mMotionElectroExport* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;
        void addData(mMotionElectroExport* data);
        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        void deleteData(int id);
        void deleteAll();
        std::vector<int> getAllIDs() const;
        QStringList getAllNames() const;
        int createID();
        int getMaxID() const;
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionElectroExportManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionElectroExportManager& mgr);

    private:
        std::map<int, mMotionElectroExport*> m_dataMap;
        int m_maxID;
    };

}
