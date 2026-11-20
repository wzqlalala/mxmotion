#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechMotionData {
    class mMotionAutoFlex;

    /**
     * @brief 自动柔性体管理类
     * @details 管理自动柔性体对象的增删改查、ID分配
     */
    class MXMOTIONDATA_EXPORT mMotionAutoFlexManager {
    public:
        mMotionAutoFlexManager();
        ~mMotionAutoFlexManager();

        mMotionAutoFlex* getData(int id) const;
        mMotionAutoFlex* getData(const QString& name) const;

        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        void addData(mMotionAutoFlex* data);

        bool existName(const QString& name) const;

        void resetName(const QString& oldName, const QString& newName);

        void deleteData(int id);

        void deleteAll();

        std::vector<int> getAllIDs() const;

        QStringList getAllNames() const;

        int createID();

        int getMaxID() const;

        // 复制
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionAutoFlexManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionAutoFlexManager& mgr);

    private:
        std::map<int, mMotionAutoFlex*> m_dataMap; ///< 数据字典
        int m_maxID;                               ///< 当前最大ID分配器
    };

}
