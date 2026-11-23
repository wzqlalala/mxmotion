#include "mOMotionJsonFile.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMutexLocker>

#include "mGlobalSignals.h"
#include "mModelData.h"
#include "mMapRelationManager.h"

#include "mMotionBodyManager.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionMarkerManager.h"
#include "mMotionScenarioManager.h"
#include "mMotionBody.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
#include "mMotionMarker.h"
#include "mMotionVectorForce.h"
#include "mMotionScalarForce.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include "mMotionScalarTorque.h"
#include "mMotionBushing.h"
#include "mMotionForceElementManager.h"
#include "mMotionScenario.h"
#include "mMotionCurveData.h"
#include "mMotionCurveRequest.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionContact.h"
#include "mMotionContactManager.h"

#include "mMotionSpringBase.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionSpringManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingCable.h"
#include "mMotionCouplingManager.h"

#include "mMotionSensorBase.h"
#include "mMotionBodySensor.h"
#include "mMotionJointSensor.h"
#include "mMotionDriveSensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"
#include "mMotionSensorManager.h"
#include "mMotionControllerManager.h"
#include "mMotionPIDController.h"
#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"
#include "mMotionAutoFlex.h"
#include "mMotionAutoFlexManager.h"

using namespace MxFunctions;
using namespace MechMotionData;

namespace MechIOFile
{
    // --------------------------------------------------
    // 辅助：Qt 类型 → JSON
    // --------------------------------------------------
    static QJsonObject vec3ToJson(const QVector3D& v) {
        QJsonObject obj;
        obj["x"] = v.x();
        obj["y"] = v.y();
        obj["z"] = v.z();
        return obj;
    }

    static QJsonObject quatToJson(const QQuaternion& q) {
        QJsonObject obj;
        obj["w"] = q.scalar();
        obj["x"] = q.x();
        obj["y"] = q.y();
        obj["z"] = q.z();
        return obj;
    }

    static QJsonObject colorToJson(const QColor& c) {
        QJsonObject obj;
        obj["r"] = c.red();
        obj["g"] = c.green();
        obj["b"] = c.blue();
        obj["a"] = c.alpha();
        return obj;
    }

    static QString jointTypeToString(JointType type) {
        switch (type) {
        case JointType::Revolute:         return "Revolute";
        case JointType::Translational:    return "Translational";
        case JointType::Cylindrical:      return "Cylindrical";
        case JointType::Spherical:        return "Spherical";
        case JointType::Fixed:            return "Fixed";
        case JointType::Universal:        return "Universal";
        case JointType::PointPlane:       return "PointPlane";
        case JointType::Line:             return "Line";
        case JointType::Parallel:         return "Parallel";
        case JointType::Perpendicular:    return "Perpendicular";
        case JointType::Direction:        return "Direction";
        case JointType::Centripetal:      return "Centripetal";
        case JointType::ConstantVelocity: return "ConstantVelocity";
        case JointType::Hooke:            return "Hooke";
        case JointType::Screw:            return "Screw";
        case JointType::Planar:           return "Planar";
        case JointType::Coupling:         return "Coupling";
        case JointType::Gear:             return "Gear";
        case JointType::PointLine:        return "PointLine";
        case JointType::PointOnCurve:     return "PointOnCurve";
        case JointType::PointOnSurface:   return "PointOnSurface";
        case JointType::CurveOnCurve:     return "CurveOnCurve";
        default:                          return "Unknown";
        }
    }

    static QString driveTargetTypeToString(DriveTargetType type) {
        switch (type) {
        case DriveTargetType::Joint: return "Joint";
        case DriveTargetType::Body:  return "Body";
        default:                     return "Unknown";
        }
    }

    static QString driveMotionTypeToString(DriveMotionType type) {
        switch (type) {
        case DriveMotionType::Translation: return "Translation";
        case DriveMotionType::Rotation:    return "Rotation";
        default:                           return "Unknown";
        }
    }

    static QString driveControlModeToString(DriveControlMode mode) {
        switch (mode) {
        case DriveControlMode::Position:       return "Position";
        case DriveControlMode::Velocity:       return "Velocity";
        case DriveControlMode::Acceleration:   return "Acceleration";
        case DriveControlMode::Force:          return "Force";
        case DriveControlMode::Torque:         return "Torque";
        default:                               return "Unknown";
        }
    }

    static QString integratorTypeToString(IntegratorType type) {
        switch (type) {
        case IntegratorType::EULER_IMPLICIT_LINEARIZED: return "EULER_IMPLICIT_LINEARIZED";
        case IntegratorType::EULER_IMPLICIT_PROJECTED:  return "EULER_IMPLICIT_PROJECTED";
        case IntegratorType::EULER_IMPLICIT:             return "EULER_IMPLICIT";
        case IntegratorType::TRAPEZOIDAL:                return "TRAPEZOIDAL";
        case IntegratorType::TRAPEZOIDAL_LINEARIZED:     return "TRAPEZOIDAL_LINEARIZED";
        case IntegratorType::HHT:                        return "HHT";
        case IntegratorType::HEUN:                       return "HEUN";
        case IntegratorType::RUNGEKUTTA45:               return "RUNGEKUTTA45";
        case IntegratorType::EULER_EXPLICIT:             return "EULER_EXPLICIT";
        case IntegratorType::LEAPFROG:                   return "LEAPFROG";
        case IntegratorType::NEWMARK:                    return "NEWMARK";
        case IntegratorType::CUSTOM_INTEGRATOR:          return "CUSTOM_INTEGRATOR";
        default:                                         return "Unknown";
        }
    }

    static QString solverTypeToString(SolverType type) {
        switch (type) {
        case SolverType::PSOR:               return "PSOR";
        case SolverType::PSSOR:              return "PSSOR";
        case SolverType::PJACOBI:            return "PJACOBI";
        case SolverType::PMINRES:            return "PMINRES";
        case SolverType::BARZILAIBORWEIN:    return "BARZILAIBORWEIN";
        case SolverType::APGD:               return "APGD";
        case SolverType::ADDM:               return "ADDM";
        case SolverType::SPARSE_LU:          return "SPARSE_LU";
        case SolverType::SPARSE_QR:          return "SPARSE_QR";
        case SolverType::PARDISO_MKL:        return "PARDISO_MKL";
        case SolverType::PARDISO_PROJECT:    return "PARDISO_PROJECT";
        case SolverType::MUMPS:              return "MUMPS";
        case SolverType::GMRES:              return "GMRES";
        case SolverType::MINRES:             return "MINRES";
        case SolverType::BICGSTAB:           return "BICGSTAB";
        case SolverType::CUSTOM_SOLVER:      return "CUSTOM_SOLVER";
        default:                             return "Unknown";
        }
    }

    static QString presetModeToString(PresetMode mode) {
        switch (mode) {
        case PresetMode::FAST:           return "FAST";
        case PresetMode::CONTACT:        return "CONTACT";
        case PresetMode::ROBUST:         return "ROBUST";
        case PresetMode::HIGH_PRECISION: return "HIGH_PRECISION";
        default:                         return "Unknown";
        }
    }

    static QString curveSourceTypeToString(CurveSourceType type) {
        switch (type) {
        case CurveSourceType::Marker:  return "Marker";
        case CurveSourceType::Body:    return "Body";
        case CurveSourceType::Joint:   return "Joint";
        case CurveSourceType::Drive:   return "Drive";
        case CurveSourceType::Coupling: return "Coupling";
        case CurveSourceType::Sensor:   return "Sensor";
        default:                        return "Unknown";
        case CurveSourceType::Controller:        return "Controller";
        }
    }

    static QString curveVariableToString(CurveVariable var) {
        switch (var) {
        case CurveVariable::Position:            return "Position";
        case CurveVariable::Velocity:            return "Velocity";
        case CurveVariable::Acceleration:        return "Acceleration";
        case CurveVariable::Rotation:            return "Rotation";
        case CurveVariable::AngularVelocity:     return "AngularVelocity";
        case CurveVariable::AngularAcceleration: return "AngularAcceleration";
        case CurveVariable::Displacement:        return "Displacement";
        case CurveVariable::PotentialEnergy:     return "PotentialEnergy";
        case CurveVariable::KineticEnergy:       return "KineticEnergy";
        case CurveVariable::ReactionForce:       return "ReactionForce";
        case CurveVariable::ReactionTorque:      return "ReactionTorque";
        case CurveVariable::ActuationForce:      return "ActuationForce";
        case CurveVariable::ActuationTorque:     return "ActuationTorque";
        case CurveVariable::ImposedValue:        return "ImposedValue";
        case CurveVariable::ImposedSpeed:        return "ImposedSpeed";
        case CurveVariable::ImposedAcc:          return "ImposedAcc";
        case CurveVariable::SensorValue:         return "SensorValue";
        case CurveVariable::PidTarget:           return "PidTarget";
        case CurveVariable::PidFeedback:         return "PidFeedback";
        case CurveVariable::PidError:            return "PidError";
        case CurveVariable::PidOutput:           return "PidOutput";
        default:                                 return "Unknown";
        }
    }

    static QString curveComponentToString(CurveComponent comp) {
        switch (comp) {
        case CurveComponent::X:          return "X";
        case CurveComponent::Y:          return "Y";
        case CurveComponent::Z:          return "Z";
        case CurveComponent::Magnitude:  return "Magnitude";
        default:                         return "Unknown";
        }
    }

    static QString vectorLoadTypeToString(VectorLoadType type) {
        switch (type) {
        case VectorLoadType::Force:           return "Force";
        case VectorLoadType::Torque:          return "Torque";
        case VectorLoadType::UniForce:        return "UniForce";
        case VectorLoadType::UniTorque:       return "UniTorque";
        case VectorLoadType::TriForce:        return "TriForce";
        case VectorLoadType::TriTorque:       return "TriTorque";
        case VectorLoadType::GeneralForce:    return "GeneralForce";
        case VectorLoadType::Gravity:         return "Gravity";
        case VectorLoadType::ForceField:      return "ForceField";
        case VectorLoadType::LinearSpringDamper: return "LinearSpringDamper";
        case VectorLoadType::TireForce:       return "TireForce";
        case VectorLoadType::Bushing:         return "Bushing";
        default:                              return "Unknown";
        }
    }

    static QString magnitudeModeToString(MagnitudeMode mode) {
        switch (mode) {
        case MagnitudeMode::Constant:    return "Constant";
        case MagnitudeMode::Expression:  return "Expression";
        default:                         return "Unknown";
        }
    }

    static QString directionModeToString(DirectionMode mode) {
        switch (mode) {
        case DirectionMode::GlobalFixed:  return "GlobalFixed";
        case DirectionMode::BodyFixed:    return "BodyFixed";
        default:                          return "Unknown";
        }
    }

    static QString contactTypeToString(MechMotionData::MotionContactType type) {
        switch (type) {
        case MechMotionData::MotionContactType::CADContact:   return "CADContact";
        case MechMotionData::MotionContactType::FacetContact: return "FacetContact";
        default:                                        return "Unknown";
        }
    }

    static QString couplingTypeToString(CouplingType type) {
        switch (type) {
        case CouplingType::GearRack: return "GearRack";
        case CouplingType::Gear:     return "Gear";
        case CouplingType::Cable:    return "Cable";
        default:                     return "Unknown";
        }
    }

    static QString springTypeToString(SpringType type) {
        switch (type) {
        case SpringType::Translational: return "Translational";
        case SpringType::Torsional:     return "Torsional";
        case SpringType::BodyPoint:     return "BodyPoint";
        default:                        return "Unknown";
        }
    }

    static QString springDirectionModeToString(SpringDirectionMode mode) {
        switch (mode) {
        case SpringDirectionMode::Bidirectional: return "Bidirectional";
        case SpringDirectionMode::PositiveOnly:  return "PositiveOnly";
        case SpringDirectionMode::NegativeOnly:  return "NegativeOnly";
        default:                                 return "Unknown";
        }
    }

    static QString sensorTypeToString(SensorType type) {
        switch (type) {
        case SensorType::BodyDisplacement:  return "BodyDisplacement";
        case SensorType::BodyVelocity:      return "BodyVelocity";
        case SensorType::BodyAcceleration:  return "BodyAcceleration";
        case SensorType::JointReaction:     return "JointReaction";
        case SensorType::DriveActuation:    return "DriveActuation";
        case SensorType::Distance:          return "Distance";
        case SensorType::Angle:             return "Angle";
        default:                            return "Unknown";
        }
    }

    static QString coordSystemToString(CoordSystem coord) {
        switch (coord) {
        case CoordSystem::GlobalFixed:  return "GlobalFixed";
        case CoordSystem::BodyFixed:    return "BodyFixed";
        default:                        return "Unknown";
        }
    }

    static QString componentModeToString(ComponentMode mode) {
        switch (mode) {
        case ComponentMode::X:          return "X";
        case ComponentMode::Y:          return "Y";
        case ComponentMode::Z:          return "Z";
        case ComponentMode::Magnitude:  return "Magnitude";
        case ComponentMode::All:        return "All";
        default:                        return "Unknown";
        }
    }

    static QString bodySensorVariableToString(BodySensorVariable var) {
        switch (var) {
        case BodySensorVariable::Position:              return "Position";
        case BodySensorVariable::Velocity:              return "Velocity";
        case BodySensorVariable::Acceleration:          return "Acceleration";
        case BodySensorVariable::AngularVelocity:       return "AngularVelocity";
        case BodySensorVariable::AngularAcceleration:   return "AngularAcceleration";
        case BodySensorVariable::Rotation:              return "Rotation";
        default:                                        return "Unknown";
        }
    }

    static QString jointSensorVariableToString(JointSensorVariable var) {
        switch (var) {
        case JointSensorVariable::ReactionForce:    return "ReactionForce";
        case JointSensorVariable::ReactionTorque:   return "ReactionTorque";
        default:                                    return "Unknown";
        }
    }

    static QString driveSensorVariableToString(DriveSensorVariable var) {
        switch (var) {
        case DriveSensorVariable::ActuationForce:   return "ActuationForce";
        case DriveSensorVariable::ActuationTorque:  return "ActuationTorque";
        case DriveSensorVariable::ImposedValue:     return "ImposedValue";
        case DriveSensorVariable::ImposedSpeed:     return "ImposedSpeed";
        case DriveSensorVariable::ImposedAcc:       return "ImposedAcc";
        case DriveSensorVariable::ReactionForce:    return "ReactionForce";
        case DriveSensorVariable::ReactionTorque:   return "ReactionTorque";
        default:                                    return "Unknown";
        }
    }

    static QString distanceVariableToString(DistanceVariable var) {
        switch (var) {
        case DistanceVariable::Distance:            return "Distance";
        case DistanceVariable::RelVelocity:         return "RelVelocity";
        case DistanceVariable::RelAcceleration:     return "RelAcceleration";
        default:                                    return "Unknown";
        }
    }

    static QJsonObject sensorPointRefToJson(const SensorPointRef& ref) {
        QJsonObject obj;
        obj["bodyId"] = ref.bodyId;
        obj["localPos"] = vec3ToJson(ref.localPos);
        return obj;
    }

    // --------------------------------------------------
    // 构造函数 / 析构函数
    // --------------------------------------------------
    mOMotionJsonFile::mOMotionJsonFile(const QString& filename, MechModelData::mModelData* modelData)
        : _fileName(filename), _modelData(modelData), _isSuccess(false)
    {
    }

    mOMotionJsonFile::~mOMotionJsonFile()
    {
    }

    QString mOMotionJsonFile::getFileName() const
    {
        return _fileName;
    }

    bool mOMotionJsonFile::isSuccess() const
    {
        return _isSuccess;
    }

    // --------------------------------------------------
    // 主线程 run()
    // --------------------------------------------------
    void mOMotionJsonFile::run()
    {
        if (!_modelData) {
            emit exportFinished(false, "模型数据为空");
            return;
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("正在导出运动数据：%1").arg(_fileName));

        QJsonObject root;
        root["formatVersion"] = 1;
        root["exportTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonObject motionData;

        // 1. Bodies
        QJsonArray bodiesArr;
        auto& bodyMgr = _modelData->getManager<mMotionBodyManager>(Physical_MotionBody);
        for (int id : bodyMgr.getAllIDs()) {
            if (auto* body = bodyMgr.getData(id)) {
                bodiesArr.append(bodyToJson(body));
            }
        }
        motionData["bodies"] = bodiesArr;

        // 2. Joints
        QJsonArray jointsArr;
        auto& jointMgr = _modelData->getManager<mMotionJointManager>(Physical_MotionJoint);
        for (int id : jointMgr.getAllIDs()) {
            if (auto* joint = jointMgr.getData(id)) {
                jointsArr.append(jointToJson(joint));
            }
        }
        motionData["joints"] = jointsArr;

        // 2.5 Springs
        QJsonArray springsArr;
        auto& springMgr = _modelData->getManager<mMotionSpringManager>(Physical_MotionSpring);
        for (int id : springMgr.getAllIDs()) {
            if (auto* spring = springMgr.getData(id)) {
                springsArr.append(springToJson(spring));
            }
        }
        motionData["springs"] = springsArr;

        // 2.6 Couplings
        QJsonArray couplingsArr;
        auto& couplingMgr = _modelData->getManager<mMotionCouplingManager>(Physical_MotionCoupling);
        for (int id : couplingMgr.getAllIDs()) {
            if (auto* coupling = couplingMgr.getData(id)) {
                couplingsArr.append(couplingToJson(coupling));
            }
        }
        motionData["couplings"] = couplingsArr;

        // 3. Drives
        QJsonArray drivesArr;
        auto& driveMgr = _modelData->getManager<mMotionDriveManager>(Physical_MotionDrive);
        for (int id : driveMgr.getAllIDs()) {
            if (auto* drive = driveMgr.getData(id)) {
                drivesArr.append(driveToJson(drive));
            }
        }
        motionData["drives"] = drivesArr;

        // 4. Markers
        QJsonArray markersArr;
        auto& markerMgr = _modelData->getManager<mMotionMarkerManager>(Physical_MotionMarker);
        for (int id : markerMgr.getAllIDs()) {
            if (auto* marker = markerMgr.getData(id)) {
                markersArr.append(markerToJson(marker));
            }
        }
        motionData["markers"] = markersArr;

        // 5. ForceElements
        QJsonArray vectorForcesArr;
        QJsonArray scalarForcesArr;
        QJsonArray scalarTorquesArr;
        QJsonArray threePointForcesArr;
        QJsonArray threePointTorquesArr;
        auto& feMgr = _modelData->getManager<mMotionForceElementManager>(Physical_MotionVectorForce);
        for (int id : feMgr.getAllIDs()) {
            if (auto* fe = feMgr.getData(id)) {
                if (auto* vf = dynamic_cast<mMotionVectorForce*>(fe)) {
                    vectorForcesArr.append(vectorForceToJson(vf));
                }
                else if (auto* tpf = dynamic_cast<mMotionThreePointForce*>(fe)) {
                    threePointForcesArr.append(threePointForceToJson(tpf));
                }
                else if (auto* tpt = dynamic_cast<mMotionThreePointTorque*>(fe)) {
                    threePointTorquesArr.append(threePointTorqueToJson(tpt));
                }
                else if (auto* sf = dynamic_cast<mMotionScalarForce*>(fe)) {
                    scalarForcesArr.append(scalarForceToJson(sf));
                }
                else if (auto* st = dynamic_cast<mMotionScalarTorque*>(fe)) {
                    scalarTorquesArr.append(scalarTorqueToJson(st));
                }
            }
        }
        motionData["vectorForces"] = vectorForcesArr;
        motionData["threePointForces"] = threePointForcesArr;
        motionData["threePointTorques"] = threePointTorquesArr;
        motionData["scalarForces"] = scalarForcesArr;
        motionData["scalarTorques"] = scalarTorquesArr;

        // 6. Contacts
        QJsonArray contactsArr;
        auto& contactMgr = _modelData->getManager<mMotionContactManager>(Physical_MotionContact);
        for (int id : contactMgr.getAllIDs()) {
            if (auto* contact = contactMgr.getData(id)) {
                contactsArr.append(contactToJson(contact));
            }
        }
        motionData["contacts"] = contactsArr;

        // 7. Sensors
        QJsonArray sensorsArr;
        auto& sensorMgr = _modelData->getManager<mMotionSensorManager>(Physical_MotionSensor);
        for (int id : sensorMgr.getAllIDs()) {
            if (auto* sensor = sensorMgr.getData(id)) {
                sensorsArr.append(sensorToJson(sensor));
            }
        }
        motionData["sensors"] = sensorsArr;

        // 7.5 Controllers
        QJsonArray controllersArr;
        auto& controllerMgr = _modelData->getManager<mMotionControllerManager>(Physical_MotionController);
        for (int id : controllerMgr.getAllIDs()) {
            if (auto* controller = controllerMgr.getData(id)) {
                controllersArr.append(controllerToJson(controller));
            }
        }
        motionData["controllers"] = controllersArr;

        // 7.6 ControlInputs
        QJsonArray controlInputsArr;
        auto& controlInputMgr = _modelData->getManager<mMotionControlInputManager>(Physical_MotionControlInput);
        for (int id : controlInputMgr.getAllIDs()) {
            if (auto* input = controlInputMgr.getData(id)) {
                controlInputsArr.append(controlInputToJson(input));
            }
        }
        motionData["controlInputs"] = controlInputsArr;

        // 8. AutoFlex
        QJsonArray autoFlexArr;
        auto& autoFlexMgr = _modelData->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex);
        for (int id : autoFlexMgr.getAllIDs()) {
            if (auto* autoFlex = autoFlexMgr.getData(id)) {
                autoFlexArr.append(autoFlexToJson(autoFlex));
            }
        }
        motionData["autoFlexBodies"] = autoFlexArr;

        // 9. Scenarios
        QJsonArray scenariosArr;
        auto& scenarioMgr = _modelData->getManager<mMotionScenarioManager>(Physical_MotionScenario);
        for (int id : scenarioMgr.getAllIDs()) {
            if (auto* scenario = scenarioMgr.getData(id)) {
                scenariosArr.append(scenarioToJson(scenario));
            }
        }
        motionData["scenarios"] = scenariosArr;

        root["motionData"] = motionData;

        QJsonDocument doc(root);
        QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

        QFile file(_fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit exportFinished(false, QString("无法打开文件写入: %1").arg(_fileName));
            return;
        }
        file.write(jsonData);
        file.close();

        _isSuccess = true;
        emit exportFinished(true, QString("成功导出运动数据到: %1").arg(_fileName));
    }

    // --------------------------------------------------
    // 各数据类型 → JSON
    // --------------------------------------------------
    QJsonObject mOMotionJsonFile::bodyToJson(mMotionBody* body) const
    {
        QJsonObject obj;
        obj["id"] = body->getId();
        obj["name"] = body->getName();

        QJsonArray geoArr;
        QJsonArray geoNameArr;
        for (int gid : _modelData->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_MotionBody, body->getId() }, Physical_Part)) {
            geoArr.append(gid);
            //几何部件名：与 geoEntityIds 按下标一一对应，供外部 AI 理解运动体对应的几何；
            //读取侧仍以 geoEntityIds（id）为准，本字段不参与反序列化
            geoNameArr.append(_modelData->getNameByPhyID(Physical_Part, gid));
        }
        obj["geoEntityIds"] = geoArr;
        obj["geoEntityNames"] = geoNameArr;

        obj["isFixed"] = body->getIsFixed();
        obj["mass"] = body->getMass();
        obj["centerOfMass"] = vec3ToJson(body->getCenterOfMass());
        obj["initialVelocity"] = vec3ToJson(body->getInitialVelocity());
        // 初始角速度（°/s，全局系）：非零才写入；旧格式无此字段视为 0
        const QVector3D initOmega = body->getInitialAngularVelocity();
        if (!initOmega.isNull())
        {
            obj["initialAngularVelocity"] = vec3ToJson(initOmega);
        }
        obj["isCustomMassProps"] = body->getIsCustomMassProps();

        QJsonArray inertiaArr;
        for (double v : body->getInertia()) {
            inertiaArr.append(v);
        }
        obj["inertia"] = inertiaArr;

        return obj;
    }

    QJsonObject mOMotionJsonFile::jointToJson(mMotionJointBase* joint) const
    {
        QJsonObject obj;
        obj["id"] = joint->getId();
        obj["name"] = joint->getName();
        obj["jointType"] = jointTypeToString(joint->getType());
        obj["actionBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionJoint, joint->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        obj["baseBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionJoint, joint->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);
        obj["isUserRename"] = joint->getIsUserRename();

        // 摩擦配置（仅支持类型在界面/求解阶段生效；仍统一保存，便于类型切换后恢复）
        {
            const auto& friction = joint->getFrictionData();
            QJsonObject frictionObj;
            frictionObj["enabled"] = friction.enabled;
            frictionObj["staticCoefficient"] = friction.staticCoefficient;
            frictionObj["kineticCoefficient"] = friction.kineticCoefficient;
            frictionObj["viscousCoefficient"] = friction.viscousCoefficient;
            frictionObj["effectiveRadius"] = friction.effectiveRadius;
            obj["friction"] = frictionObj;
        }

        // 限位配置。旋转副下限/上限/缓冲区以"度"存储（与面板一致），刚度 N·mm/°、
        // 阻尼 N·mm/(°/s)；移动副为 mm、N/mm、N·s/mm（求解导出处统一换算 rad 与 mN 系）。
        {
            const auto& limit = joint->getLimitData();
            QJsonObject limitObj;
            limitObj["enabled"] = limit.enabled;
            limitObj["lowerEnabled"] = limit.lowerEnabled;
            limitObj["upperEnabled"] = limit.upperEnabled;
            limitObj["lowerValue"] = limit.lowerValue;
            limitObj["upperValue"] = limit.upperValue;
            limitObj["penaltyOnly"] = limit.penaltyOnly;
            limitObj["lowerCushion"] = limit.lowerCushion;
            limitObj["upperCushion"] = limit.upperCushion;
            limitObj["lowerStiffness"] = limit.lowerStiffness;
            limitObj["upperStiffness"] = limit.upperStiffness;
            limitObj["lowerDamping"] = limit.lowerDamping;
            limitObj["upperDamping"] = limit.upperDamping;
            obj["limit"] = limitObj;
        }

        if (auto* pos = dynamic_cast<mMotionJointPointOnSurface*>(joint)) {
            obj["origin"] = vec3ToJson(pos->getOrigin());
            obj["axis"] = vec3ToJson(pos->getAxis());
            QJsonArray faceArr;
            for (int fid : pos->getSurfaceFaceIds()) faceArr.append(fid);
            obj["surfaceFaceIds"] = faceArr;
        }
        else if (auto* poc = dynamic_cast<mMotionJointPointOnCurve*>(joint)) {
            obj["origin"] = vec3ToJson(poc->getOrigin());
            obj["axis"] = vec3ToJson(poc->getAxis());
            QJsonArray edgeArr;
            for (int eid : poc->getCurveEdgeIds()) edgeArr.append(eid);
            obj["curveEdgeIds"] = edgeArr;
        }
        else if (auto* cvcv = dynamic_cast<mMotionJointCurveOnCurve*>(joint)) {
            obj["origin"] = vec3ToJson(cvcv->getOrigin());
            obj["axis"] = vec3ToJson(cvcv->getAxis());
            QJsonArray edgeArrI;
            for (int eid : cvcv->getCurveIEdgeIds()) edgeArrI.append(eid);
            obj["curveIEdgeIds"] = edgeArrI;
            QJsonArray edgeArrJ;
            for (int eid : cvcv->getCurveJEdgeIds()) edgeArrJ.append(eid);
            obj["curveJEdgeIds"] = edgeArrJ;
            obj["allowSliding"] = cvcv->getAllowSliding();
        }
        else if (auto* basic = dynamic_cast<mMotionJointBasic*>(joint)) {
            obj["origin"] = vec3ToJson(basic->getOrigin());
            obj["axis"] = vec3ToJson(basic->getAxis());
        }
        else if (auto* universal = dynamic_cast<mMotionJointUniversal*>(joint)) {
            obj["origin"] = vec3ToJson(universal->getOrigin());
            obj["actionSpin"] = vec3ToJson(universal->getActionBodySpinVector());
            obj["actionPin"] = vec3ToJson(universal->getActionBodyPinVector());
            obj["baseSpin"] = vec3ToJson(universal->getBaseBodySpinVector());
            obj["basePin"] = vec3ToJson(universal->getBaseBodyPinVector());
        }
        else if (auto* screw = dynamic_cast<mMotionJointScrew*>(joint)) {
            obj["origin"] = vec3ToJson(screw->getOrigin());
            obj["axis"] = vec3ToJson(screw->getAxis());
            obj["lead"] = screw->getLead();
        }
        else if (auto* perpendicular = dynamic_cast<mMotionJointPerpendicular*>(joint)) {
            obj["origin"] = vec3ToJson(perpendicular->getOrigin());
            obj["actionVector"] = vec3ToJson(perpendicular->getActionBodyVector());
            obj["baseVector"] = vec3ToJson(perpendicular->getBaseBodyVector());
        }
        else if (auto* constantVelocity = dynamic_cast<mMotionJointConstantVelocity*>(joint)) {
            obj["origin"] = vec3ToJson(constantVelocity->getOrigin());
            obj["actionVector"] = vec3ToJson(constantVelocity->getActionBodyVector());
            obj["baseVector"] = vec3ToJson(constantVelocity->getBaseBodyVector());
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::couplingToJson(mMotionCouplingBase* coupling) const
    {
        QJsonObject obj;
        obj["id"] = coupling->getId();
        obj["name"] = coupling->getName();
        obj["couplingType"] = couplingTypeToString(coupling->getType());
        obj["firstJointId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionCoupling, coupling->getId() }, Physical_MotionJoint, MxFunctions::MapRelation_MotionFirstJoint);
        obj["secondJointId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionCoupling, coupling->getId() }, Physical_MotionJoint, MxFunctions::MapRelation_MotionSecondJoint);
        obj["isUserRename"] = coupling->getIsUserRename();

        if (auto* gearRack = dynamic_cast<mMotionCouplingGearRack*>(coupling)) {
            obj["radius"] = gearRack->getRadius();
        }
        else if (auto* gear = dynamic_cast<mMotionCouplingGear*>(coupling)) {
            obj["gearRatio"] = gear->getGearRatio();
            obj["toothCount1"] = gear->getToothCount1();
            obj["toothCount2"] = gear->getToothCount2();
            obj["pitchRadius1"] = gear->getPitchRadius1();
            obj["pitchRadius2"] = gear->getPitchRadius2();
        }
        else if (auto* cable = dynamic_cast<mMotionCouplingCable*>(coupling)) {
            obj["ratio"] = cable->getRatio();
            obj["reverse"] = cable->getReverse();
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::springToJson(mMotionSpringBase* spring) const
    {
        QJsonObject obj;
        obj["id"] = spring->getId();
        obj["name"] = spring->getName();
        obj["springType"] = springTypeToString(spring->getType());
        obj["jointId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSpring, spring->getId() }, Physical_MotionJoint);
        obj["isUserRename"] = spring->getIsUserRename();
        obj["stiffness"] = spring->getStiffness();
        obj["damping"] = spring->getDamping();
        obj["preload"] = spring->getPreload();
        obj["springDirectionMode"] = springDirectionModeToString(spring->getSpringDirectionMode());
        obj["dampingDirectionMode"] = springDirectionModeToString(spring->getDampingDirectionMode());
        obj["freeValue"] = spring->getFreeValue();
        obj["installValue"] = spring->getInstallValue();
        obj["dampingEnabled"] = spring->isDampingEnabled();
        obj["forceFunction"] = spring->getForceFunction();
        obj["dampingFunction"] = spring->getDampingFunction();

        QJsonArray tableXArr, tableYArr;
        if (spring->hasTable()) {
            for (double v : spring->getTableX()) tableXArr.append(v);
            for (double v : spring->getTableY()) tableYArr.append(v);
        }
        obj["tableX"] = tableXArr;
        obj["tableY"] = tableYArr;

        // 体-点弹簧端点信息
        if (auto* bpSpring = dynamic_cast<mMotionBodyPointSpring*>(spring)) {
            obj["bodyIId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSpring, bpSpring->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyI);
            obj["bodyJId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSpring, bpSpring->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyJ);
            obj["localI"] = vec3ToJson(bpSpring->getLocalI());
            obj["localJ"] = vec3ToJson(bpSpring->getLocalJ());
            obj["localRotI"] = quatToJson(bpSpring->getLocalRotI());
            obj["localRotJ"] = quatToJson(bpSpring->getLocalRotJ());
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::driveToJson(mMotionDriveBase* drive) const
    {
        QJsonObject obj;
        obj["id"] = drive->getId();
        obj["name"] = drive->getName();

        // 判别具体派生类
        if (auto* jointDrive = dynamic_cast<mMotionJointDrive*>(drive)) {
            obj["driveType"] = "JointDrive";
            obj["targetJointId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionDrive, jointDrive->getId() }, Physical_MotionJoint);
        }
        else {
            obj["driveType"] = "Unknown";
        }

        obj["targetType"] = driveTargetTypeToString(drive->getTargetType());
        obj["motionType"] = driveMotionTypeToString(drive->getMotionType());
        obj["controlMode"] = driveControlModeToString(drive->getControlMode());
        obj["expression"] = drive->getExpression();

        // 表格驱动
        QJsonArray tx, ty;
        for (double v : drive->getDriveTableX()) tx.append(v);
        for (double v : drive->getDriveTableY()) ty.append(v);
        obj["driveTableX"] = tx;
        obj["driveTableY"] = ty;

        return obj;
    }

    QJsonObject mOMotionJsonFile::markerToJson(mMotionMarker* marker) const
    {
        QJsonObject obj;
        obj["id"] = marker->getId();
        obj["name"] = marker->getName();
        obj["bodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionMarker, marker->getId() }, Physical_MotionBody);
        obj["localPosition"] = vec3ToJson(marker->getLocalPosition());
        obj["localRotation"] = quatToJson(marker->getLocalRotation());
        obj["color"] = colorToJson(marker->getColor());
        obj["size"] = marker->getSize();
        obj["visible"] = marker->getVisible();
        return obj;
    }

    QJsonObject mOMotionJsonFile::vectorForceToJson(mMotionVectorForce* force) const
    {
        QJsonObject obj;
        obj["id"] = force->getId();
        obj["name"] = force->getName();
        obj["isUserRename"] = force->getIsUserRename();
        obj["loadType"] = vectorLoadTypeToString(force->getLoadType());
        obj["bodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        obj["actionPoint"] = vec3ToJson(force->getActionPoint());
        obj["direction"] = vec3ToJson(force->getDirection());
        obj["magnitude"] = force->getMagnitude();
        obj["expression"] = force->getExpression();
        obj["magnitudeMode"] = magnitudeModeToString(force->getMagnitudeMode());
        obj["directionMode"] = directionModeToString(force->getDirectionMode());

        // Bushing 专属字段，仅 Bushing 类型导出
        if (auto* bushing = dynamic_cast<mMotionBushing*>(force)) {
            // 刚度 / 阻尼
            QJsonArray stiffnessArr;
            for (double v : bushing->getStiffness()) stiffnessArr.append(v);
            obj["stiffness"] = stiffnessArr;

            QJsonArray dampingArr;
            for (double v : bushing->getDamping()) dampingArr.append(v);
            obj["damping"] = dampingArr;

            // 基本体 (marker_J)
            obj["baseBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, bushing->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);

            // 偏置 (D0)
            obj["applyOffset"] = bushing->getApplyOffset();
            obj["offset"] = vec3ToJson(bushing->getOffset());

            // 预载 (F0/T0)
            QJsonArray f0Arr;
            for (double v : bushing->getPreloadForce()) f0Arr.append(v);
            obj["preloadForce"] = f0Arr;

            QJsonArray t0Arr;
            for (double v : bushing->getPreloadTorque()) t0Arr.append(v);
            obj["preloadTorque"] = t0Arr;
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::threePointTorqueToJson(MechMotionData::mMotionThreePointTorque* torque) const
    {
        QJsonObject obj;
        obj["id"] = torque->getId();
        obj["name"] = torque->getName();
        obj["isUserRename"] = torque->getIsUserRename();
        obj["bodyIId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, torque->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyI);
        obj["localI"] = vec3ToJson(torque->getLocalI());
        obj["targetBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, torque->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);
        obj["localTarget"] = vec3ToJson(torque->getLocalTarget());
        obj["originBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, torque->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        obj["localOrigin"] = vec3ToJson(torque->getLocalOrigin());
        obj["magnitude"] = torque->getMagnitude();
        obj["torqueFunction"] = torque->getTorqueFunction();
        return obj;
    }

    QJsonObject mOMotionJsonFile::threePointForceToJson(MechMotionData::mMotionThreePointForce* force) const
    {
        QJsonObject obj;
        obj["id"] = force->getId();
        obj["name"] = force->getName();
        obj["isUserRename"] = force->getIsUserRename();
        obj["bodyIId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyI);
        obj["localI"] = vec3ToJson(force->getLocalI());
        obj["targetBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);
        obj["localTarget"] = vec3ToJson(force->getLocalTarget());
        obj["originBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        obj["localOrigin"] = vec3ToJson(force->getLocalOrigin());
        obj["magnitude"] = force->getMagnitude();
        obj["forceFunction"] = force->getForceFunction();
        return obj;
    }

    QJsonObject mOMotionJsonFile::scalarForceToJson(mMotionScalarForce* force) const
    {
        QJsonObject obj;
        obj["id"] = force->getId();
        obj["name"] = force->getName();
        obj["isUserRename"] = force->getIsUserRename();
        obj["bodyIId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyI);
        obj["localI"] = vec3ToJson(force->getLocalI());
        obj["bodyJId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, force->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyJ);
        obj["localJ"] = vec3ToJson(force->getLocalJ());
        obj["directionMode"] = static_cast<int>(force->getDirectionMode());
        obj["fixedDirection"] = vec3ToJson(force->getFixedDirection());
        obj["magnitude"] = force->getMagnitude();
        obj["forceFunction"] = force->getForceFunction();
        obj["actionOnly"] = force->getActionOnly();
        return obj;
    }

    QJsonObject mOMotionJsonFile::scalarTorqueToJson(MechMotionData::mMotionScalarTorque* torque) const
    {
        QJsonObject obj;
        obj["id"] = torque->getId();
        obj["name"] = torque->getName();
        obj["isUserRename"] = torque->getIsUserRename();
        obj["bodyIId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, torque->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyI);
        obj["localI"] = vec3ToJson(torque->getLocalI());
        obj["bodyJId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionVectorForce, torque->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyJ);
        obj["localJ"] = vec3ToJson(torque->getLocalJ());
        obj["axisMode"] = static_cast<int>(torque->getAxisMode());
        obj["fixedAxis"] = vec3ToJson(torque->getFixedAxis());
        obj["magnitude"] = torque->getMagnitude();
        obj["torqueFunction"] = torque->getTorqueFunction();
        obj["actionOnly"] = torque->getActionOnly();
        return obj;
    }

    QJsonObject mOMotionJsonFile::scenarioToJson(mMotionScenario* scenario) const
    {
        QJsonObject obj;
        obj["id"] = scenario->getId();
        obj["name"] = scenario->getName();
        obj["endTime"] = scenario->getEndTime();
        obj["stepSize"] = scenario->getStepSize();
        obj["gravity"] = vec3ToJson(scenario->getGravity());
        obj["usePreset"] = scenario->getUsePreset();
        obj["presetMode"] = presetModeToString(scenario->getPresetMode());
        obj["integratorType"] = integratorTypeToString(scenario->getIntegratorType());
        obj["integratorAlpha"] = scenario->getIntegratorAlpha();
        obj["integratorBeta"] = scenario->getIntegratorBeta();
        obj["integratorMaxIters"] = scenario->getIntegratorMaxIters();
        obj["integratorTol"] = scenario->getIntegratorTol();
        obj["solverType"] = solverTypeToString(scenario->getSolverType());
        obj["solverMaxIters"] = scenario->getSolverMaxIters();
        obj["solverTolerance"] = scenario->getSolverTolerance();
        obj["solverVerbose"] = scenario->getSolverVerbose();

        // trackedMarkerIds
        QJsonArray trackedArr;
        for (int mid : scenario->getTrackedMarkerIds()) {
            trackedArr.append(mid);
        }
        obj["trackedMarkerIds"] = trackedArr;

        // curveNode
        if (scenario->hasCurveNode()) {
            obj["curveNode"] = curveToJson(scenario->getCurveNode());
        }
        else {
            obj["curveNode"] = QJsonValue::Null;
        }

        // currentCurveRequest（最近编辑状态，兼容旧字段）
        if (scenario->hasCurrentCurveRequest()) {
            obj["currentCurveRequest"] = curveRequestToJson(scenario->getCurrentCurveRequest());
        }
        else {
            obj["currentCurveRequest"] = QJsonValue::Null;
        }

        // curveRequests（多曲线列表）
        QJsonArray reqArr;
        for (const auto& req : scenario->getCurveRequests()) {
            reqArr.append(curveRequestToJson(req));
        }
        obj["curveRequests"] = reqArr;

        return obj;
    }

    QJsonObject mOMotionJsonFile::curveToJson(mMotionCurveData* curve) const
    {
        QJsonObject obj;
        obj["id"] = curve->getId();
        obj["name"] = curve->getName();
        obj["xAxisLabel"] = curve->m_xAxisLabel;
        obj["yAxisLabel"] = curve->m_yAxisLabel;

        QJsonArray xArr, yArr;
        for (double v : curve->m_xData) xArr.append(v);
        for (double v : curve->m_yData) yArr.append(v);
        obj["xData"] = xArr;
        obj["yData"] = yArr;

        return obj;
    }

    QJsonObject mOMotionJsonFile::curveRequestToJson(const mMotionCurveRequest& req) const
    {
        QJsonObject obj = req.m_originalJson;
        obj[obj.contains("id") && !obj.contains("requestId") ? "id" : "requestId"] = req.m_requestId;
        obj[obj.contains("name") && !obj.contains("requestName") ? "name" : "requestName"] = req.m_requestName;
        if (req.isNativeChannel()) {
            const auto previousSource = obj.value("sourceType").toString();
            const auto previousType = mMotionCurveRequest::sourceTypeFromName(previousSource);
            if (mMotionCurveRequest::collectionForSource(previousType) != req.m_nativeSource)
                obj["sourceType"] = req.m_nativeSource;
            else if (!obj.contains("sourceType"))obj["sourceType"] = req.m_nativeSource;
            obj["variable"] = req.m_nativeVariable;
            if (obj.value("component").isString() && req.m_nativeComponent >= -1 && req.m_nativeComponent <= 2)
                obj["component"] = req.m_nativeComponent == -1 ? QStringLiteral("Magnitude")
                    : QString(QChar('X' + req.m_nativeComponent));
            else obj["component"] = req.m_nativeComponent;
        } else {
            obj["sourceType"] = curveSourceTypeToString(req.m_sourceType);
            obj["variable"] = curveVariableToString(req.m_variable);
            obj["component"] = curveComponentToString(req.m_component);
        }
        obj["sourceId"] = req.m_sourceId;
        if (!req.m_sourceName.isEmpty() || obj.contains("sourceName"))obj["sourceName"] = req.m_sourceName;
        if (req.m_color.isValid()) {
            if (obj.value("color").isString() || (req.isNativeChannel() && !obj.contains("color"))) {
                if (QColor(obj.value("color").toString()) != req.m_color)
                    obj["color"] = req.m_color.name(req.m_color.alpha()==255 ? QColor::HexRgb : QColor::HexArgb);
            } else {
                auto color = obj.value("color").toObject();const auto rgba = colorToJson(req.m_color);
                for (auto it=rgba.begin();it!=rgba.end();++it)color[it.key()] = it.value();
                obj["color"] = color;
            }
        }
        obj["visible"] = req.m_visible;
        return obj;
    }

    QJsonObject mOMotionJsonFile::contactToJson(mMotionContact* contact) const
    {
        QJsonObject obj;
        obj["id"] = contact->getId();
        obj["name"] = contact->getName();
        obj["contactType"] = contactTypeToString(contact->getContactType());
        obj["actionBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionContact, contact->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionActionBody);
        obj["baseBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionContact, contact->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBaseBody);
        obj["useConvexHullAction"] = contact->getUseConvexHullAction();
        obj["useConvexHullBase"] = contact->getUseConvexHullBase();
        obj["stiffness"] = contact->getStiffness();
        obj["forceExponent"] = contact->getForceExponent();
        obj["materialDamping"] = contact->getMaterialDamping();
        obj["penetrationDepth"] = contact->getPenetrationDepth();
        obj["frictionEnabled"] = contact->getFrictionEnabled();
        obj["staticFriction"] = contact->getStaticFriction();
        obj["dynamicFriction"] = contact->getDynamicFriction();
        obj["forceModel"] = contact->getForceModel();
        obj["forceModelTolerance"] = contact->getForceModelTolerance();
        obj["actionUseFacetFile"] = contact->getActionUseFacetFile();
        obj["actionDistanceTolerance"] = contact->getActionDistanceTolerance();
        obj["angleTolerance"] = contact->getActionAngleTolerance();
        obj["actionUseMaxFacetSize"] = contact->getActionUseMaxFacetSize();
        obj["actionMaxFacetSize"] = contact->getActionMaxFacetSize();
        obj["baseUseFacetFile"] = contact->getBaseUseFacetFile();
        obj["baseDistanceTolerance"] = contact->getBaseDistanceTolerance();
        obj["baseUseMaxFacetSize"] = contact->getBaseUseMaxFacetSize();
        obj["baseMaxFacetSize"] = contact->getBaseMaxFacetSize();
        return obj;
    }

    static void writeBaseSensorFields(QJsonObject& obj, const mMotionSensorBase* sensor, MechModelData::mModelData* modelData) {
        obj["coordSystem"] = coordSystemToString(sensor->getCoordSystem());
        obj["referenceBodyId"] = modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, sensor->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionReferenceBody);
        obj["componentMode"] = componentModeToString(sensor->getComponentMode());
        obj["enabled"] = sensor->isEnabled();
        obj["isUserRename"] = sensor->getIsUserRename();
    }

    QJsonObject mOMotionJsonFile::sensorToJson(mMotionSensorBase* sensor) const
    {
        QJsonObject obj;
        obj["id"] = sensor->getId();
        obj["name"] = sensor->getName();
        obj["sensorType"] = sensorTypeToString(sensor->getType());

        writeBaseSensorFields(obj, sensor, _modelData);

        SensorType type = sensor->getType();

        if (type == SensorType::BodyDisplacement || type == SensorType::BodyVelocity || type == SensorType::BodyAcceleration) {
            if (auto* bs = dynamic_cast<mMotionBodySensor*>(sensor)) {
                obj["targetBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, bs->getId() }, Physical_MotionBody);
                obj["referencePoint"] = vec3ToJson(bs->getReferencePoint());
                obj["variable"] = bodySensorVariableToString(bs->getVariable());
            }
        }
        else if (type == SensorType::JointReaction) {
            if (auto* js = dynamic_cast<mMotionJointSensor*>(sensor)) {
                obj["targetJointId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, js->getId() }, Physical_MotionJoint);
                obj["variable"] = jointSensorVariableToString(js->getVariable());
            }
        }
        else if (type == SensorType::DriveActuation) {
            if (auto* ds = dynamic_cast<mMotionDriveSensor*>(sensor)) {
                obj["targetDriveId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, ds->getId() }, Physical_MotionDrive);
                obj["variable"] = driveSensorVariableToString(ds->getVariable());
            }
        }
        else if (type == SensorType::Distance) {
            if (auto* dist = dynamic_cast<mMotionDistanceSensor*>(sensor)) {
                obj["pointA"] = sensorPointRefToJson(dist->getPointA());
                obj["pointB"] = sensorPointRefToJson(dist->getPointB());
                obj["variable"] = distanceVariableToString(dist->getVariable());
            }
        }
        else if (type == SensorType::Angle) {
            if (auto* ang = dynamic_cast<mMotionAngleSensor*>(sensor)) {
                obj["axisBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, ang->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionAxisBody);
                obj["axisDirection"] = vec3ToJson(ang->getAxisDirection());
                obj["bodyAId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, ang->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyA);
                obj["bodyBId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionSensor, ang->getId() }, Physical_MotionBody, MxFunctions::MapRelation_MotionBodyB);
            }
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::controllerToJson(MechMotionData::mMotionControllerBase* controller) const
    {
        using namespace MechMotionData;
        QJsonObject obj;
        obj["id"] = controller->getId();
        obj["name"] = controller->getName();
        obj["controllerType"] = (controller->getType() == ControllerType::PID) ? QStringLiteral("PID") : QStringLiteral("Unknown");
        obj["enabled"] = controller->isEnabled();
        obj["isUserRename"] = controller->getIsUserRename();

        if (auto* pid = dynamic_cast<mMotionPIDController*>(controller)) {
            obj["targetDriveId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionController, pid->getId() }, Physical_MotionDrive);
            obj["feedbackType"] = (pid->getFeedbackType() == PidFeedbackType::Velocity) ? QStringLiteral("Velocity") : QStringLiteral("Position");
            obj["targetValue"] = pid->getTargetValue();
            obj["targetInputId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionController, pid->getId() }, Physical_MotionControlInput);
            obj["feedbackSource"] = (pid->getFeedbackSource() == PidFeedbackSource::Sensor)
                ? QStringLiteral("Sensor") : QStringLiteral("DriveDof");
            obj["feedbackSensorId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionController, pid->getId() }, Physical_MotionSensor);
            obj["feedbackSign"] = pid->getFeedbackSign();
            obj["kp"] = pid->getKp();
            obj["ki"] = pid->getKi();
            obj["kd"] = pid->getKd();
            obj["outputLimit"] = pid->getOutputLimit();
        }

        return obj;
    }

    QJsonObject mOMotionJsonFile::controlInputToJson(MechMotionData::mMotionControlInput* input) const
    {
        using namespace MechMotionData;
        QJsonObject obj;
        obj["id"] = input->getId();
        obj["name"] = input->getName();
        const int typeInt = static_cast<int>(input->getInputType());
        obj["inputType"] = (typeInt == 1) ? "Linear"
            : (typeInt == 2) ? "Quadratic"
            : (typeInt == 3) ? "Cubic"
            : (typeInt == 4) ? "CustomCurve"
            : "Constant";
        obj["timeStart"] = input->getTimeStart();
        obj["timeEnd"] = input->getTimeEnd();

        QJsonArray coeffsArr;
        for (double c : input->getPolyCoeffs()) {
            coeffsArr.append(c);
        }
        obj["polyCoeffs"] = coeffsArr;

        QJsonArray ptsArr;
        for (const auto& pt : input->getCurvePoints()) {
            QJsonArray pair;
            pair.append(pt.first);
            pair.append(pt.second);
            ptsArr.append(pair);
        }
        obj["curvePoints"] = ptsArr;

        return obj;
    }

    QJsonObject mOMotionJsonFile::autoFlexToJson(mMotionAutoFlex* autoFlex) const
    {
        QJsonObject obj;
        obj["id"] = autoFlex->getId();
        obj["name"] = autoFlex->getName();
        obj["targetBodyId"] = _modelData->getMapRelationManager().getMapRelation<int>({ Physical_MotionAutoFlex, autoFlex->getId() }, Physical_MotionBody);
        obj["materialId"] = autoFlex->getMaterialId();

        QJsonArray attachArr;
        for (int nodeId : autoFlex->getAttachPoints()) {
            attachArr.append(nodeId);
        }
        obj["attachPoints"] = attachArr;

        // 网格参数
        obj["elementSize"] = autoFlex->getElementSize();
        obj["meshType"] = static_cast<int>(autoFlex->getMeshType());
        obj["minElementSize"] = autoFlex->getMinElementSize();
        obj["curvatureRefinement"] = autoFlex->getCurvatureRefinement();

        // 模态分析参数
        obj["modeCount"] = autoFlex->getModeCount();
        obj["freqCutoff"] = autoFlex->getFreqCutoff();
        obj["solverType"] = static_cast<int>(autoFlex->getSolverType());
        obj["freqLower"] = autoFlex->getFreqLower();
        obj["freqUpper"] = autoFlex->getFreqUpper();
        obj["rigidBodyModeFilter"] = autoFlex->getRigidBodyModeFilter();

        // 界面连接参数
        obj["connectionType"] = static_cast<int>(autoFlex->getConnectionType());
        obj["interfaceDofs"] = autoFlex->getInterfaceDofs();
        obj["connectionRadius"] = autoFlex->getConnectionRadius();

        // 输出控制
        obj["outputModeShapes"] = autoFlex->getOutputModeShapes();
        obj["outputStressStrain"] = autoFlex->getOutputStressStrain();
        obj["outputFormat"] = static_cast<int>(autoFlex->getOutputFormat());

        // 阻尼参数
        obj["dampingType"] = static_cast<int>(autoFlex->getDampingType());
        obj["modalDampingRatio"] = autoFlex->getModalDampingRatio();
        obj["rayleighMass"] = autoFlex->getRayleighMass();
        obj["rayleighStiffness"] = autoFlex->getRayleighStiffness();

        obj["enabled"] = autoFlex->isEnabled();
        return obj;
    }
}
