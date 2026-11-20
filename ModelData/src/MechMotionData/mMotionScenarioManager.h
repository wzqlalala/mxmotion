#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechMotionData {
    class mMotionScenario;

    /**
     * @brief 求解设置管理类
     * @details 管理多个求解方案（用户可能在一个模型里建立多套计算参数进行对比）
     */
    class MXMOTIONDATA_EXPORT mMotionScenarioManager {
    public:
        mMotionScenarioManager();
        ~mMotionScenarioManager();

        mMotionScenario* getData(int id) const;
        mMotionScenario* getData(const QString& name) const;

        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        void addData(mMotionScenario* config);
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

        friend QDataStream& operator>>(QDataStream& in, mMotionScenarioManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionScenarioManager& mgr);
    private:
        std::map<int, mMotionScenario*> m_dataMap;
        int m_maxID;
    };
}