#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include "mMotionControllerType.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechMotionData {
    class mMotionControllerBase;

    /**
     * @brief 控制器管理类
     * @details 管理控制器对象的增删改查、ID分配
     */
    class MXMOTIONDATA_EXPORT mMotionControllerManager {
    public:
        mMotionControllerManager();
        ~mMotionControllerManager();

        mMotionControllerBase* getData(int id) const;
        mMotionControllerBase* getData(const QString& name) const;

        QString getNameByID(int id) const;
        int getIDByName(const QString& name) const;

        void addData(mMotionControllerBase* controller);

        bool existName(const QString& name) const;

        void resetName(const QString& oldName, const QString& newName);

        void deleteData(int id);

        void deleteAll();

        std::vector<int> getAllIDs() const;

        QStringList getAllNames() const;

        int createID();

        int getMaxID() const;

        // 按类型获取ID列表
        std::vector<int> getIDsByType(ControllerType type) const;

        // 复制
        int copyData(const int& id);

        friend QDataStream& operator>>(QDataStream& in, mMotionControllerManager& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionControllerManager& mgr);

    private:
        std::map<int, mMotionControllerBase*> m_dataMap; ///< 数据字典
        int m_maxID;                                     ///< 当前最大ID分配器
    };

}
