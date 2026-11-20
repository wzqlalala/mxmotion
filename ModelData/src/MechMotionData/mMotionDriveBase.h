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
      * @brief 驱动运动类型枚举 (自由度类型)
      */
    enum class MXMOTIONDATA_EXPORT DriveMotionType {
        Translation,    ///< 平动 (沿着轴向)
        Rotation        ///< 转动 (绕着轴向)
    };

    /**
     * @brief 驱动控制模式枚举 (物理量类型)
     * @details 对应 ADAMS 里的 is_position / is_speed / is_acceleration
     */
    enum class MXMOTIONDATA_EXPORT DriveControlMode {
        Position,       ///< 位置控制 (位移/角度)
        Velocity,       ///< 速度控制 (线速度/角速度)
        Acceleration    ///< 加速度控制 (线加速度/角加速度)
    };

    /**
      * @brief 驱动基类
      */
    class MXMOTIONDATA_EXPORT mMotionDriveBase {
    public:
        mMotionDriveBase(int id, DriveTargetType targetType, const QString& name = "");
        mMotionDriveBase() {};
        virtual ~mMotionDriveBase();

        int getId() const;
        void setName(const QString& name);
        QString getName() const;
        DriveTargetType getTargetType() const;

        void setMotionType(DriveMotionType type);
        DriveMotionType getMotionType() const;

        // --- 控制模式 (位置/速度/加速度) ---
        void setControlMode(DriveControlMode mode);
        DriveControlMode getControlMode() const;

        void setExpression(const QString& expr);
        QString getExpression() const;

        // --- 表格驱动 ---
        void setDriveTable(const QVector<double>& xs, const QVector<double>& ys);
        QVector<double> getDriveTableX() const;
        QVector<double> getDriveTableY() const;
        bool hasDriveTable() const;

        //拷贝
        virtual mMotionDriveBase* copyDrive(int newId, const QString& newName) = 0;

        friend QDataStream& operator>>(QDataStream& in, mMotionDriveBase& mgr);
        friend QDataStream& operator<<(QDataStream& out, mMotionDriveBase& mgr);

    protected:
        int m_id;                       ///< 驱动ID
        QString m_name;                 ///< 驱动名称
        DriveTargetType m_targetType;   ///< 驱动目标类型 (运动副/运动体)
        DriveMotionType m_motionType;   ///< 激励类型 (平动/转动)
        DriveControlMode m_controlMode; ///< 控制模式 (位置/速度/加速度)
        QString m_expression;           ///< 运动函数表达式

        // 表格驱动：时间-值表（严格递增）；非空时优先于表达式
        QVector<double> m_driveTableX;  ///< 时间序列 (s)
        QVector<double> m_driveTableY;  ///< 激励值序列
    };

}
