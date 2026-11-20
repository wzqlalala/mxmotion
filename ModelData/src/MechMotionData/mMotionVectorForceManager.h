#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXMotionData_global.h"
#include "mMotionForceElementManager.h"
#include "mMotionVectorForce.h"
#include <map>
#include <vector>
#include <QString>
#include <QStringList>

namespace MechMotionData
{
    class mMotionVectorForce;

    /**
     * @brief 矢量载荷管理类
     * @details 管理矢量力/矢量力矩对象的增删改查、ID分配
     */
    class MXMOTIONDATA_EXPORT mMotionVectorForceManager : public mMotionForceElementManager {
    public:
        using mMotionForceElementManager::mMotionForceElementManager;

        mMotionVectorForce* getData(int id) const;
        mMotionVectorForce* getData(const QString& name) const;

        void addData(mMotionVectorForce* force);
    };

}
