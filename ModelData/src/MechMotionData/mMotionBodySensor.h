#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSensorBase.h"

namespace MechMotionData {

    /**
     * @brief 体状态传感器
     * @details 用于测量运动体的位移、速度、加速度、角速度、角加速度、方向等
     */
    class MXMOTIONDATA_EXPORT mMotionBodySensor : public mMotionSensorBase {
    public:
        /**
         * @brief 构造函数
         * @param id 传感器ID
         * @param type 传感器类型（必须是 BodyDisplacement / BodyVelocity / BodyAcceleration）
         * @param name 名称
         */
        mMotionBodySensor(int id, SensorType type, const QString& name = "", bool isUserRename = false);

        /**
         * @brief 默认构造函数
         */
        mMotionBodySensor();

        ~mMotionBodySensor() override;

        SensorType getType() const override;

        // --- 被测体 ---

        // --- 测量点（相对于体局部坐标系）---
        void setReferencePoint(const QVector3D& point);
        QVector3D getReferencePoint() const;

        // --- 测量变量 ---
        void setVariable(BodySensorVariable var);
        BodySensorVariable getVariable() const;

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
        // 构造后把默认变量收敛到类型允许的集合（类型优先，如体速度类型默认速度变量）
        void clampVariableToType();

        QVector3D m_referencePoint; ///< 测量点（体局部坐标系）
        BodySensorVariable m_variable; ///< 测量变量
    };

}
