#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT VectorLoadType {
        Force,
        Torque,
        UniForce,
        UniTorque,
        TriForce,
        TriTorque,
        GeneralForce,
        Gravity,
        ForceField,
        LinearSpringDamper,
        TireForce,
        Bushing,
        ScalarForce,
        ScalarTorque
    };

    /**
     * @brief 力元基类
     * @details 统一管理矢量力与标量力。仅提供公共标识与多态接口，
     *          具体数据字段保留在派生类中。
     */
    class MXMOTIONDATA_EXPORT mMotionForceElement {
    public:
        mMotionForceElement(int id, const QString& name, bool isUserRename = false);
        mMotionForceElement();
        virtual ~mMotionForceElement();

        // --- 基础标识 ---
        void setId(int id);
        int getId() const;

        void setName(const QString& name, bool isUserRename = true);
        QString getName() const;

        /*
        * 获取是否为用户重命名（与运动副 mMotionJointBase 一致：
        * 用户重命名过的力元在类型切换时保留名称，默认名则跟随新类型）
        */
        bool getIsUserRename() const;

        /**
         * @brief 当前力元类型
         */
        virtual VectorLoadType getLoadType() const = 0;

        /**
         * @brief 多态复制
         * @param newId 新ID
         * @param newName 新名称
         * @return 新的派生类实例
         */
        virtual mMotionForceElement* copyForceElement(int newId, const QString& newName) = 0;

    protected:
        int m_id;       ///< 力元ID
        QString m_name; ///< 力元名称
        bool m_isUserRename; ///< 是否被用户重命名过
    };

}
