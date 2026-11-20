#include "mMotionDriveBase.h"

#include <QMatrix4x4>

using namespace MxFunctions;
namespace MechMotionData
{
    mMotionDriveBase::mMotionDriveBase(int id, DriveTargetType targetType, const QString& name)
        : m_id(id),
        m_targetType(targetType),
        m_name(name),
        m_motionType(DriveMotionType::Translation),     // 默认假设为转动
        m_controlMode(DriveControlMode::Velocity)    // 默认假设为速度控制
    {
        if (m_name.isEmpty()) {
            m_name = QString("Drive_%1").arg(m_id);
        }
    }

    mMotionDriveBase::~mMotionDriveBase() {}

    int mMotionDriveBase::getId() const { return m_id; }

    void mMotionDriveBase::setName(const QString& name) { m_name = name; }

    QString mMotionDriveBase::getName() const { return m_name; }

    DriveTargetType mMotionDriveBase::getTargetType() const { return m_targetType; }

    void mMotionDriveBase::setMotionType(DriveMotionType type) { m_motionType = type; }

    DriveMotionType mMotionDriveBase::getMotionType() const { return m_motionType; }

    void mMotionDriveBase::setControlMode(DriveControlMode mode) { m_controlMode = mode; }
    DriveControlMode mMotionDriveBase::getControlMode() const { return m_controlMode; }

    void mMotionDriveBase::setExpression(const QString& expr) { m_expression = expr; }

    void mMotionDriveBase::setDriveTable(const QVector<double>& xs, const QVector<double>& ys) {
        m_driveTableX = xs;
        m_driveTableY = ys;
    }

    QVector<double> mMotionDriveBase::getDriveTableX() const { return m_driveTableX; }

    QVector<double> mMotionDriveBase::getDriveTableY() const { return m_driveTableY; }

    bool mMotionDriveBase::hasDriveTable() const {
        // 鑷冲皯涓よ涓旀暟閲忎竴鑷存墠鏈夋晥
        return m_driveTableX.size() >= 2 && m_driveTableX.size() == m_driveTableY.size();
    }

    QString mMotionDriveBase::getExpression() const { return m_expression; }

}
