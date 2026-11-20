#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSensorBase.h"

namespace MechMotionData {

    /**
     * @brief 距离传感器
     * @details 测量两点之间的距离、相对速度、相对加速度
     */
    class MXMOTIONDATA_EXPORT mMotionDistanceSensor : public mMotionSensorBase {
    public:
        mMotionDistanceSensor(int id, SensorType type, const QString& name = "", bool isUserRename = false);
        mMotionDistanceSensor();
        ~mMotionDistanceSensor() override;

        SensorType getType() const override;

        void setPointA(const SensorPointRef& point);
        SensorPointRef getPointA() const;

        void setPointB(const SensorPointRef& point);
        SensorPointRef getPointB() const;

        void setVariable(DistanceVariable var);
        DistanceVariable getVariable() const;

        QString getVariableName() const;

        mMotionSensorBase* copySensor(int newId, const QString& newName) const override;

    protected:
        void readFromStream(QDataStream& in) override;
        void writeToStream(QDataStream& out) const override;

    private:
        SensorPointRef m_pointA;
        SensorPointRef m_pointB;
        DistanceVariable m_variable;
    };

}
