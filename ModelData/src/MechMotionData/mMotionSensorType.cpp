#include "mMotionSensorType.h"

namespace MechMotionData {

    QString getSensorTypeName(SensorType type) {
        switch (type) {
        case SensorType::BodyDisplacement:
            return QStringLiteral("体位移传感器");
        case SensorType::BodyVelocity:
            return QStringLiteral("体速度传感器");
        case SensorType::BodyAcceleration:
            return QStringLiteral("体加速度传感器");
        case SensorType::JointReaction:
            return QStringLiteral("关节反力传感器");
        case SensorType::DriveActuation:
            return QStringLiteral("驱动输出传感器");
        case SensorType::Distance:
            return QStringLiteral("距离传感器");
        case SensorType::Angle:
            return QStringLiteral("角度传感器");
        default:
            return QStringLiteral("未知传感器");
        }
    }

    QString getCoordSystemName(CoordSystem coord) {
        switch (coord) {
        case CoordSystem::GlobalFixed:
            return QStringLiteral("全局固定");
        case CoordSystem::BodyFixed:
            return QStringLiteral("体固定");
        default:
            return QStringLiteral("未知");
        }
    }

    QString getComponentModeName(ComponentMode mode) {
        switch (mode) {
        case ComponentMode::X:
            return QStringLiteral("X分量");
        case ComponentMode::Y:
            return QStringLiteral("Y分量");
        case ComponentMode::Z:
            return QStringLiteral("Z分量");
        case ComponentMode::Magnitude:
            return QStringLiteral("幅值");
        case ComponentMode::All:
            return QStringLiteral("全部");
        default:
            return QStringLiteral("未知");
        }
    }

    QString getBodySensorVariableName(BodySensorVariable var) {
        switch (var) {
        case BodySensorVariable::Position:
            return QStringLiteral("位置");
        case BodySensorVariable::Velocity:
            return QStringLiteral("速度");
        case BodySensorVariable::Acceleration:
            return QStringLiteral("加速度");
        case BodySensorVariable::AngularVelocity:
            return QStringLiteral("角速度");
        case BodySensorVariable::AngularAcceleration:
            return QStringLiteral("角加速度");
        case BodySensorVariable::Rotation:
            return QStringLiteral("方向");
        default:
            return QStringLiteral("未知");
        }
    }

    QString getJointSensorVariableName(JointSensorVariable var) {
        switch (var) {
        case JointSensorVariable::ReactionForce:
            return QStringLiteral("约束反力");
        case JointSensorVariable::ReactionTorque:
            return QStringLiteral("约束反力矩");
        default:
            return QStringLiteral("未知");
        }
    }

    QString getDriveSensorVariableName(DriveSensorVariable var) {
        switch (var) {
        case DriveSensorVariable::ActuationForce:
            return QStringLiteral("驱动力");
        case DriveSensorVariable::ActuationTorque:
            return QStringLiteral("驱动力矩");
        case DriveSensorVariable::ImposedValue:
            return QStringLiteral("驱动位移");
        case DriveSensorVariable::ImposedSpeed:
            return QStringLiteral("驱动速度");
        case DriveSensorVariable::ImposedAcc:
            return QStringLiteral("驱动加速度");
        case DriveSensorVariable::ReactionForce:
            return QStringLiteral("约束反力");
        case DriveSensorVariable::ReactionTorque:
            return QStringLiteral("约束反力矩");
        default:
            return QStringLiteral("未知");
        }
    }

    QString getDistanceVariableName(DistanceVariable var) {
        switch (var) {
        case DistanceVariable::Distance:
            return QStringLiteral("距离");
        case DistanceVariable::RelVelocity:
            return QStringLiteral("相对速度");
        case DistanceVariable::RelAcceleration:
            return QStringLiteral("相对加速度");
        default:
            return QStringLiteral("未知");
        }
    }

    QVector<BodySensorVariable> allowedBodyVariables(SensorType type) {
        switch (type) {
        case SensorType::BodyDisplacement:
            return { BodySensorVariable::Position, BodySensorVariable::Rotation };
        case SensorType::BodyVelocity:
            return { BodySensorVariable::Velocity, BodySensorVariable::AngularVelocity };
        case SensorType::BodyAcceleration:
            return { BodySensorVariable::Acceleration, BodySensorVariable::AngularAcceleration };
        default:
            return {};
        }
    }

    void normalizeBodySensor(SensorType& type, BodySensorVariable& variable) {
        if (allowedBodyVariables(type).contains(variable))
            return;
        // 变量不属于当前类型：按用户配置的变量意图重新定型
        switch (variable) {
        case BodySensorVariable::Position:
        case BodySensorVariable::Rotation:
            type = SensorType::BodyDisplacement;
            break;
        case BodySensorVariable::Velocity:
        case BodySensorVariable::AngularVelocity:
            type = SensorType::BodyVelocity;
            break;
        case BodySensorVariable::Acceleration:
        case BodySensorVariable::AngularAcceleration:
            type = SensorType::BodyAcceleration;
            break;
        default:
            break;
        }
    }

}
