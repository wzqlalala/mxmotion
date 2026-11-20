#include "mMotionCurveRequest.h"

namespace MechMotionData {

    QString mMotionCurveRequest::getDefaultName() const {
        QString varLabel = getVariableLabel();
        QString compLabel = (m_sourceType == CurveSourceType::Sensor
            || m_sourceType == CurveSourceType::Controller) ? "" : getComponentLabel();
        return QString("%1_%2_%3").arg(m_sourceName).arg(varLabel).arg(compLabel);
    }

    QString mMotionCurveRequest::getVariableLabel() const {
        switch (m_variable) {
        case CurveVariable::Position: return "Position";
        case CurveVariable::Velocity: return "Velocity";
        case CurveVariable::Acceleration: return "Acceleration";
        case CurveVariable::Rotation: return "Rotation";
        case CurveVariable::AngularVelocity: return "AngularVel";
        case CurveVariable::AngularAcceleration: return "AngularAcc";
        case CurveVariable::Displacement: return "Displacement";
        case CurveVariable::PotentialEnergy: return "Energy";
        case CurveVariable::KineticEnergy: return "KineticEnergy";
        case CurveVariable::ReactionForce: return "ReactForce";
        case CurveVariable::ReactionTorque: return "ReactTorque";
        case CurveVariable::ActuationForce: return "ActForce";
        case CurveVariable::ActuationTorque: return "ActTorque";
        case CurveVariable::ImposedValue: return "ImposedPos";
        case CurveVariable::ImposedSpeed: return "ImposedVel";
        case CurveVariable::ImposedAcc: return "ImposedAcc";
        case CurveVariable::SensorValue: return "传感器值";
        case CurveVariable::PidTarget: return "目标值";
        case CurveVariable::PidFeedback: return "反馈值";
        case CurveVariable::PidError: return "误差";
        case CurveVariable::PidOutput: return "输出";
        default: return "Unknown";
        }
    }

    QString mMotionCurveRequest::getComponentLabel() const {
        switch (m_component) {
        case CurveComponent::X: return "X";
        case CurveComponent::Y: return "Y";
        case CurveComponent::Z: return "Z";
        case CurveComponent::Magnitude: return "Mag";
        default: return "";
        }
    }

    QString mMotionCurveRequest::getFullLabel() const {
        return m_requestName.isEmpty() ? getDefaultName() : m_requestName;
    }

}
