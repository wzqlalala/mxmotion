#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSensorBase.h"

namespace MechMotionData {

    /**
     * @brief 关节反力传感器
     * @details 用于测量运动副的约束反力和约束反力矩
     */
    class MXMOTIONDATA_EXPORT mMotionJointSensor : public mMotionSensorBase {
    public:
        /**
         * @brief 构造函数
         * @param id 传感器ID
         * @param type 传感器类型（必须是 JointReaction）
         * @param name 名称
         */
        mMotionJointSensor(int id, SensorType type, const QString& name = "", bool isUserRename = false);

        /**
         * @brief 默认构造函数
         */
        mMotionJointSensor();

        ~mMotionJointSensor() override;

        SensorType getType() const override;

        // --- 被测关节 ---

        // --- 测量变量 ---
        void setVariable(JointSensorVariable var);
        JointSensorVariable getVariable() const;

        /**
         * @brief 获取变量中文名称
         */
        QString getVariableName() const;

        // --- 复制 ---
        mMotionSensorBase* copySensor(int newId, const QString& newName) const override;

    protected:
        void readFromStream(QDataStream& in) override;
        void writeToStream(QDataStream& out) const override;

    private:
        JointSensorVariable m_variable; ///< 测量变量
    };

}
