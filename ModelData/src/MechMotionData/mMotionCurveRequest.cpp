#include "mMotionCurveRequest.h"

namespace MechMotionData {

    CurveSourceType mMotionCurveRequest::sourceTypeFromName(const QString& name) {
        static const char* aliases[]{"Marker","Body","Joint","Drive","Coupling","Sensor","Controller",
            "Spring","SoftStop","TensionCable","VectorForce","ScalarForce","ScalarTorque",
            "ThreePointForce","ThreePointTorque","AutoFlexBody"};
        for(int i=0;i<16;++i)if(name==QLatin1String(aliases[i]) || name==collectionForSource(static_cast<CurveSourceType>(i)))
            return static_cast<CurveSourceType>(i);
        return CurveSourceType::NativeChannel;
    }

    QString mMotionCurveRequest::collectionForSource(CurveSourceType type) {
        static const char* collections[]{"markers","bodies","joints","drives","couplings","sensors","controllers",
            "springs","softStops","tensionCables","vectorForces","scalarForces","scalarTorques",
            "threePointForces","threePointTorques","autoFlexBodies"};
        const int index=static_cast<int>(type);
        return index>=0&&index<16?QString::fromLatin1(collections[index]):QString{};
    }

    QString mMotionCurveRequest::nameForVariable(CurveVariable variable) {
        static const char* names[]{"Position","Velocity","Acceleration","Rotation","AngularVelocity","AngularAcceleration",
            "Displacement","PotentialEnergy","ReactionForce","ReactionTorque","ActuationForce","ActuationTorque",
            "ImposedValue","ImposedSpeed","ImposedAcc","SensorValue","PidTarget","PidFeedback","PidError","PidOutput","KineticEnergy"};
        const int index=static_cast<int>(variable);
        return index>=0&&index<21?QString::fromLatin1(names[index]):QString{};
    }

    CurveVariable mMotionCurveRequest::variableFromName(const QString& name) {
        for(int i=0;i<21;++i)if(name==nameForVariable(static_cast<CurveVariable>(i)))return static_cast<CurveVariable>(i);
        return CurveVariable::NativeChannel;
    }

    QString mMotionCurveRequest::getDefaultName() const {
        if(isNativeChannel())return QStringLiteral("%1 · %2").arg(m_sourceName,
            m_nativeVariableLabel.isEmpty()?m_nativeVariable:m_nativeVariableLabel);
        QString varLabel = getVariableLabel();
        QString compLabel = (m_sourceType == CurveSourceType::Sensor
            || m_sourceType == CurveSourceType::Controller) ? "" : getComponentLabel();
        return QString("%1_%2_%3").arg(m_sourceName).arg(varLabel).arg(compLabel);
    }

    QString mMotionCurveRequest::getVariableLabel() const {
        if(isNativeChannel())return m_nativeVariableLabel.isEmpty()?m_nativeVariable:m_nativeVariableLabel;
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
