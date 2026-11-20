#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSensorBase.h"

namespace MechMotionData {

    /**
     * @brief 驱动输出传感器
     * @details 用于测量驱动的输出力/力矩、位移/速度/加速度、约束反力/力矩
     */
    class MXMOTIONDATA_EXPORT mMotionDriveSensor : public mMotionSensorBase {
    public:
        mMotionDriveSensor(int id, SensorType type, const QString& name = "", bool isUserRename = false);
        mMotionDriveSensor();
        ~mMotionDriveSensor() override;

        SensorType getType() const override;


        void setVariable(DriveSensorVariable var);
        DriveSensorVariable getVariable() const;

        QString getVariableName() const;

        mMotionSensorBase* copySensor(int newId, const QString& newName) const override;

    protected:
        void readFromStream(QDataStream& in) override;
        void writeToStream(QDataStream& out) const override;

    private:
        DriveSensorVariable m_variable;
    };

}
