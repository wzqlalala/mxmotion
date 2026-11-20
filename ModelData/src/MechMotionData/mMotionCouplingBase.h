#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <vector>

#include "MXMotionData_global.h"
#include "mPhysicalFieldEnum.h"

#include <QVector3D>
#include <QVector>
#include <QString>

using namespace MxFunctions;
namespace MechMotionData {

    /**
     * @brief 耦合副基类
     * @details 耦合副基于两个已创建的运动副建立约束关系，
     *          不直接存储运动体、原点或方向信息，这些信息由关联的运动副提供。
     */
    class MXMOTIONDATA_EXPORT mMotionCouplingBase {
    public:
        mMotionCouplingBase(int id, CouplingType type, const QString& name = "", bool isUserRename = false);

        mMotionCouplingBase() {};
        virtual ~mMotionCouplingBase();

        /*
        * 获取是否为用户重命名
        */
        bool getIsUserRename();

        int getId() const;
        QString getName() const;
        void setName(const QString& name, bool isUserRename = true);
        void setType(CouplingType type);
        CouplingType getType() const;

        /**
         * @brief 获取当前耦合副类型的中文名称
         * @return QString 中文名称 (如 "齿轮齿条副")
         */
        QString getTypeName() const;

        /**
         * @brief 静态辅助函数：将枚举类型转换为中文名称
         * @param type 耦合副枚举类型
         * @return QString 中文名称
         */
        static QString getTypeName(CouplingType type);

        /**
         * @brief 设置第一个关联运动副ID
         */
        /**
         * @brief 获取第一个关联运动副ID
         */
        /**
         * @brief 设置第二个关联运动副ID
         */
        /**
         * @brief 获取第二个关联运动副ID
         */

        virtual mMotionCouplingBase* copyCoupling(int newId, const QString& newName) = 0;

        friend QDataStream& operator>>(QDataStream& in, mMotionCouplingBase& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionCouplingBase& mgr);

    protected:
        //是否为用户重命名
        bool m_isUserRename;

        int m_id;                       ///< 耦合副ID
        QString m_name;                 ///< 耦合副名称
        CouplingType m_type;            ///< 耦合副类型
    };

}
