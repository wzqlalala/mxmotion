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
    class mMotionElectroImport;

    class MXMOTIONDATA_EXPORT mMotionElectroImportManager {
    public:
        mMotionElectroImportManager();
        ~mMotionElectroImportManager();

        mMotionElectroImport* getData(int id) const;
        mMotionElectroImport* getData(const QString& name) const;
        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;
        void addData(mMotionElectroImport* data);
        bool existName(const QString& name) const;
        void resetName(const QString& oldName, const QString& newName);
        void deleteData(int id);
        void deleteAll();
        std::vector<int> getAllIDs() const;
        QStringList getAllNames() const;
        int createID();
        int getMaxID() const;
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionElectroImportManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionElectroImportManager& mgr);

    private:
        std::map<int, mMotionElectroImport*> m_dataMap;
        int m_maxID;
    };

}
