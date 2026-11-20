#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMotionSensorBase.h"
#include "mModelStruct.h"

namespace MechMotionData {

    /**
     * @brief 角度传感器
     * @details 测量两体之间绕指定轴的夹角
     */
    class MXMOTIONDATA_EXPORT mMotionAngleSensor : public mMotionSensorBase {
    public:
        DirectionPickState axisDirPickState_;           ///< 轴方向输入的拾取会话状态
        mMotionAngleSensor(int id, SensorType type, const QString& name = "", bool isUserRename = false);
        mMotionAngleSensor();
        ~mMotionAngleSensor() override;

        SensorType getType() const override;


        void setAxisDirection(const QVector3D& dir);
        QVector3D getAxisDirection() const;



        mMotionSensorBase* copySensor(int newId, const QString& newName) const override;

    protected:
        void readFromStream(QDataStream& in) override;
        void writeToStream(QDataStream& out) const override;

    private:
        QVector3D m_axisDirection;  ///< 旋转轴方向向量
    };

}
