#include "mIMotionJsonFile.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMutexLocker>

#include "mGlobalSignals.h"
#include "mModelData.h"
#include "mMapRelationManager.h"

#include "mMotionBodyManager.h"
#include "mMotionJointManager.h"
#include "mMotionDriveManager.h"
#include "mMotionMarkerManager.h"
#include "mMotionScenarioManager.h"
#include "mMotionCouplingManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingCable.h"
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
#include "mMotionTranslationalSpring.h"
#include "mMotionTorsionalSpring.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionSpringManager.h"

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
#include "mMechGlobalSignals.h"

using namespace MxFunctions;
using namespace MechMotionData;

namespace MechIOFile
{
    // --------------------------------------------------
    // 辅助：JSON → Qt 类型 / 枚举
    // --------------------------------------------------
    static QVector3D jsonToVec3(const QJsonObject& obj) {
        return QVector3D(
            obj.value("x").toDouble(0.0),
            obj.value("y").toDouble(0.0),
            obj.value("z").toDouble(0.0)
        );
    }

    static QQuaternion jsonToQuat(const QJsonObject& obj) {
        return QQuaternion(
            obj.value("w").toDouble(1.0),
            obj.value("x").toDouble(0.0),
            obj.value("y").toDouble(0.0),
            obj.value("z").toDouble(0.0)
        );
    }

    static QColor jsonToColor(const QJsonObject& obj) {
        return QColor(
            obj.value("r").toInt(255),
            obj.value("g").toInt(0),
            obj.value("b").toInt(0),
            obj.value("a").toInt(255)
        );
    }

    static JointType stringToJointType(const QString& str) {
        if (str == "Revolute")         return JointType::Revolute;
        if (str == "Translational")    return JointType::Translational;
        if (str == "Cylindrical")      return JointType::Cylindrical;
        if (str == "Spherical")        return JointType::Spherical;
        if (str == "Fixed")            return JointType::Fixed;
        if (str == "Universal")        return JointType::Universal;
        if (str == "PointPlane")       return JointType::PointPlane;
        if (str == "Line")             return JointType::Line;
        if (str == "Parallel")         return JointType::Parallel;
        if (str == "Perpendicular")    return JointType::Perpendicular;
        if (str == "Direction")        return JointType::Direction;
        if (str == "Centripetal")      return JointType::Centripetal;
        if (str == "ConstantVelocity") return JointType::ConstantVelocity;
        if (str == "Hooke")            return JointType::Hooke;
        if (str == "Screw")            return JointType::Screw;
        if (str == "Planar")           return JointType::Planar;
        if (str == "Coupling")         return JointType::Coupling;
        if (str == "Gear")             return JointType::Gear;
        if (str == "PointLine")        return JointType::PointLine;
        if (str == "PointOnCurve")     return JointType::PointOnCurve;
        if (str == "PointOnSurface")   return JointType::PointOnSurface;
        if (str == "CurveOnCurve")     return JointType::CurveOnCurve;
        return JointType::Revolute; // 默认
    }

    static DriveTargetType stringToDriveTargetType(const QString& str) {
        if (str == "Joint")  return DriveTargetType::Joint;
        if (str == "Body")   return DriveTargetType::Body;
        return DriveTargetType::Joint;
    }

    static DriveMotionType stringToDriveMotionType(const QString& str) {
        if (str == "Translation")  return DriveMotionType::Translation;
        if (str == "Rotation")     return DriveMotionType::Rotation;
        return DriveMotionType::Translation;
    }

    static DriveControlMode stringToDriveControlMode(const QString& str) {
        if (str == "Force") return DriveControlMode::Force;
        if (str == "Torque") return DriveControlMode::Torque;
        if (str == "Position")      return DriveControlMode::Position;
        if (str == "Velocity")      return DriveControlMode::Velocity;
        if (str == "Acceleration")  return DriveControlMode::Acceleration;
        return DriveControlMode::Position;
    }

    static IntegratorType stringToIntegratorType(const QString& str) {
        if (str == "EULER_IMPLICIT_LINEARIZED") return IntegratorType::EULER_IMPLICIT_LINEARIZED;
        if (str == "EULER_IMPLICIT_PROJECTED")  return IntegratorType::EULER_IMPLICIT_PROJECTED;
        if (str == "EULER_IMPLICIT")             return IntegratorType::EULER_IMPLICIT;
        if (str == "TRAPEZOIDAL")                return IntegratorType::TRAPEZOIDAL;
        if (str == "TRAPEZOIDAL_LINEARIZED")     return IntegratorType::TRAPEZOIDAL_LINEARIZED;
        if (str == "HHT")                        return IntegratorType::HHT;
        if (str == "HEUN")                       return IntegratorType::HEUN;
        if (str == "RUNGEKUTTA45")               return IntegratorType::RUNGEKUTTA45;
        if (str == "EULER_EXPLICIT")             return IntegratorType::EULER_EXPLICIT;
        if (str == "LEAPFROG")                   return IntegratorType::LEAPFROG;
        if (str == "NEWMARK")                    return IntegratorType::NEWMARK;
        if (str == "CUSTOM_INTEGRATOR")          return IntegratorType::CUSTOM_INTEGRATOR;
        return IntegratorType::HHT;
    }

    static SolverType stringToSolverType(const QString& str) {
        if (str == "PSOR")               return SolverType::PSOR;
        if (str == "PSSOR")              return SolverType::PSSOR;
        if (str == "PJACOBI")            return SolverType::PJACOBI;
        if (str == "PMINRES")            return SolverType::PMINRES;
        if (str == "BARZILAIBORWEIN")    return SolverType::BARZILAIBORWEIN;
        if (str == "APGD")               return SolverType::APGD;
        if (str == "ADDM")               return SolverType::ADDM;
        if (str == "SPARSE_LU")          return SolverType::SPARSE_LU;
        if (str == "SPARSE_QR")          return SolverType::SPARSE_QR;
        if (str == "PARDISO_MKL")        return SolverType::PARDISO_MKL;
        if (str == "PARDISO_PROJECT")    return SolverType::PARDISO_PROJECT;
        if (str == "MUMPS")              return SolverType::MUMPS;
        if (str == "GMRES")              return SolverType::GMRES;
        if (str == "MINRES")             return SolverType::MINRES;
        if (str == "BICGSTAB")           return SolverType::BICGSTAB;
        if (str == "CUSTOM_SOLVER")      return SolverType::CUSTOM_SOLVER;
        return SolverType::PSOR;
    }

    static PresetMode stringToPresetMode(const QString& str) {
        if (str == "FAST")            return PresetMode::FAST;
        if (str == "CONTACT")         return PresetMode::CONTACT;
        if (str == "ROBUST")          return PresetMode::ROBUST;
        if (str == "HIGH_PRECISION")  return PresetMode::HIGH_PRECISION;
        return PresetMode::FAST;
    }

    static CurveSourceType stringToCurveSourceType(const QString& str) {
        if (str == "Marker")   return CurveSourceType::Marker;
        if (str == "Body")     return CurveSourceType::Body;
        if (str == "Joint")    return CurveSourceType::Joint;
        if (str == "Drive")    return CurveSourceType::Drive;
        if (str == "Coupling") return CurveSourceType::Coupling;
        if (str == "Sensor")   return CurveSourceType::Sensor;
        if (str == "Controller") return CurveSourceType::Controller;
        return CurveSourceType::Marker;
    }

    static CurveVariable stringToCurveVariable(const QString& str) {
        if (str == "Position")            return CurveVariable::Position;
        if (str == "Velocity")            return CurveVariable::Velocity;
        if (str == "Acceleration")        return CurveVariable::Acceleration;
        if (str == "Rotation")            return CurveVariable::Rotation;
        if (str == "AngularVelocity")     return CurveVariable::AngularVelocity;
        if (str == "AngularAcceleration") return CurveVariable::AngularAcceleration;
        if (str == "Displacement")        return CurveVariable::Displacement;
        if (str == "PotentialEnergy")     return CurveVariable::PotentialEnergy;
        if (str == "KineticEnergy")       return CurveVariable::KineticEnergy;
        if (str == "ReactionForce")       return CurveVariable::ReactionForce;
        if (str == "ReactionTorque")      return CurveVariable::ReactionTorque;
        if (str == "ActuationForce")      return CurveVariable::ActuationForce;
        if (str == "ActuationTorque")     return CurveVariable::ActuationTorque;
        if (str == "ImposedValue")        return CurveVariable::ImposedValue;
        if (str == "ImposedSpeed")        return CurveVariable::ImposedSpeed;
        if (str == "ImposedAcc")          return CurveVariable::ImposedAcc;
        if (str == "SensorValue")         return CurveVariable::SensorValue;
        if (str == "PidTarget")           return CurveVariable::PidTarget;
        if (str == "PidFeedback")         return CurveVariable::PidFeedback;
        if (str == "PidError")            return CurveVariable::PidError;
        if (str == "PidOutput")           return CurveVariable::PidOutput;
        return CurveVariable::Position;
    }

    static CurveComponent stringToCurveComponent(const QString& str) {
        if (str == "X")           return CurveComponent::X;
        if (str == "Y")           return CurveComponent::Y;
        if (str == "Z")           return CurveComponent::Z;
        if (str == "Magnitude")   return CurveComponent::Magnitude;
        return CurveComponent::X;
    }

    static VectorLoadType stringToVectorLoadType(const QString& str) {
        if (str == "Force")           return VectorLoadType::Force;
        if (str == "Torque")          return VectorLoadType::Torque;
        if (str == "UniForce")        return VectorLoadType::UniForce;
        if (str == "UniTorque")       return VectorLoadType::UniTorque;
        if (str == "TriForce")        return VectorLoadType::TriForce;
        if (str == "TriTorque")       return VectorLoadType::TriTorque;
        if (str == "GeneralForce")    return VectorLoadType::GeneralForce;
        if (str == "Gravity")         return VectorLoadType::Gravity;
        if (str == "ForceField")      return VectorLoadType::ForceField;
        if (str == "LinearSpringDamper") return VectorLoadType::LinearSpringDamper;
        if (str == "TireForce")       return VectorLoadType::TireForce;
        if (str == "Bushing")         return VectorLoadType::Bushing;
        return VectorLoadType::Force;
    }

    static MagnitudeMode stringToMagnitudeMode(const QString& str) {
        if (str == "Constant")    return MagnitudeMode::Constant;
        if (str == "Expression")  return MagnitudeMode::Expression;
        return MagnitudeMode::Constant;
    }

    static DirectionMode stringToDirectionMode(const QString& str) {
        if (str == "GlobalFixed")  return DirectionMode::GlobalFixed;
        if (str == "BodyFixed")    return DirectionMode::BodyFixed;
        return DirectionMode::GlobalFixed;
    }

    static MechMotionData::MotionContactType stringToContactType(const QString& str) {
        if (str == "CADContact")   return MechMotionData::MotionContactType::CADContact;
        if (str == "FacetContact") return MechMotionData::MotionContactType::FacetContact;
        return MechMotionData::MotionContactType::CADContact;
    }

    static CouplingType stringToCouplingType(const QString& str) {
        if (str == "GearRack") return CouplingType::GearRack;
        if (str == "Gear")     return CouplingType::Gear;
        if (str == "Cable")    return CouplingType::Cable;
        return CouplingType::GearRack;
    }

    static SpringType stringToSpringType(const QString& str) {
        if (str == "Translational") return SpringType::Translational;
        if (str == "Torsional")     return SpringType::Torsional;
        if (str == "BodyPoint")     return SpringType::BodyPoint;
        return SpringType::Translational;
    }

    static SpringDirectionMode stringToSpringDirectionMode(const QString& str) {
        if (str == "Bidirectional") return SpringDirectionMode::Bidirectional;
        if (str == "PositiveOnly")  return SpringDirectionMode::PositiveOnly;
        if (str == "NegativeOnly")  return SpringDirectionMode::NegativeOnly;
        return SpringDirectionMode::Bidirectional;
    }

    static SensorType stringToSensorType(const QString& str) {
        if (str == "BodyDisplacement")  return SensorType::BodyDisplacement;
        if (str == "BodyVelocity")      return SensorType::BodyVelocity;
        if (str == "BodyAcceleration")  return SensorType::BodyAcceleration;
        if (str == "JointReaction")     return SensorType::JointReaction;
        if (str == "DriveActuation")    return SensorType::DriveActuation;
        if (str == "Distance")          return SensorType::Distance;
        if (str == "Angle")             return SensorType::Angle;
        return SensorType::BodyDisplacement;
    }

    static CoordSystem stringToCoordSystem(const QString& str) {
        if (str == "GlobalFixed")  return CoordSystem::GlobalFixed;
        if (str == "BodyFixed")    return CoordSystem::BodyFixed;
        return CoordSystem::GlobalFixed;
    }

    static ComponentMode stringToComponentMode(const QString& str) {
        if (str == "X")           return ComponentMode::X;
        if (str == "Y")           return ComponentMode::Y;
        if (str == "Z")           return ComponentMode::Z;
        if (str == "Magnitude")   return ComponentMode::Magnitude;
        if (str == "All")         return ComponentMode::All;
        return ComponentMode::X;
    }

    static BodySensorVariable stringToBodySensorVariable(const QString& str) {
        if (str == "Position")            return BodySensorVariable::Position;
        if (str == "Velocity")            return BodySensorVariable::Velocity;
        if (str == "Acceleration")        return BodySensorVariable::Acceleration;
        if (str == "AngularVelocity")     return BodySensorVariable::AngularVelocity;
        if (str == "AngularAcceleration") return BodySensorVariable::AngularAcceleration;
        if (str == "Rotation")            return BodySensorVariable::Rotation;
        return BodySensorVariable::Position;
    }

    static JointSensorVariable stringToJointSensorVariable(const QString& str) {
        if (str == "ReactionForce")   return JointSensorVariable::ReactionForce;
        if (str == "ReactionTorque")  return JointSensorVariable::ReactionTorque;
        return JointSensorVariable::ReactionForce;
    }

    static DriveSensorVariable stringToDriveSensorVariable(const QString& str) {
        if (str == "ActuationForce")   return DriveSensorVariable::ActuationForce;
        if (str == "ActuationTorque")  return DriveSensorVariable::ActuationTorque;
        if (str == "ImposedValue")     return DriveSensorVariable::ImposedValue;
        if (str == "ImposedSpeed")     return DriveSensorVariable::ImposedSpeed;
        if (str == "ImposedAcc")       return DriveSensorVariable::ImposedAcc;
        if (str == "ReactionForce")    return DriveSensorVariable::ReactionForce;
        if (str == "ReactionTorque")   return DriveSensorVariable::ReactionTorque;
        return DriveSensorVariable::ActuationForce;
    }

    static DistanceVariable stringToDistanceVariable(const QString& str) {
        if (str == "Distance")           return DistanceVariable::Distance;
        if (str == "RelVelocity")        return DistanceVariable::RelVelocity;
        if (str == "RelAcceleration")    return DistanceVariable::RelAcceleration;
        return DistanceVariable::Distance;
    }

    static SensorPointRef jsonToSensorPointRef(const QJsonObject& obj) {
        SensorPointRef ref;
        ref.bodyId = obj.value("bodyId").toInt(-1);
        ref.localPos = jsonToVec3(obj.value("localPos").toObject());
        return ref;
    }

    // --------------------------------------------------
    // 构造函数 / 析构函数
    // --------------------------------------------------
    mIMotionJsonFile::mIMotionJsonFile(const QString& filename, MechModelData::mModelData* modelData)
        : _fileName(filename), _modelData(modelData), _isSuccess(false)
    {
    }

    mIMotionJsonFile::~mIMotionJsonFile()
    {
    }

    QString mIMotionJsonFile::getFileName() const
    {
        return _fileName;
    }

    bool mIMotionJsonFile::isSuccess() const
    {
        return _isSuccess;
    }

    // --------------------------------------------------
    // 清空现有运动数据
    // --------------------------------------------------
    bool mIMotionJsonFile::clearExistingMotionData()
    {
        if (!_modelData) return false;

        _modelData->deleteAllByPhysicalType(Physical_MotionBody);
        _modelData->deleteAllByPhysicalType(Physical_MotionJoint);
        _modelData->deleteAllByPhysicalType(Physical_MotionDrive);
        _modelData->deleteAllByPhysicalType(Physical_MotionMarker);
        _modelData->deleteAllByPhysicalType(Physical_MotionVectorForce);
        _modelData->deleteAllByPhysicalType(Physical_MotionContact);
        _modelData->deleteAllByPhysicalType(Physical_MotionSensor);
        _modelData->deleteAllByPhysicalType(Physical_MotionController);
        _modelData->deleteAllByPhysicalType(Physical_MotionControlInput);
        _modelData->deleteAllByPhysicalType(Physical_MotionVectorForce);
        _modelData->deleteAllByPhysicalType(Physical_MotionAutoFlex);
        _modelData->deleteAllByPhysicalType(Physical_MotionSpring);
        _modelData->deleteAllByPhysicalType(Physical_MotionCoupling);
        _modelData->deleteAllByPhysicalType(Physical_MotionScenario);
        return true;
    }

    // --------------------------------------------------
    // 主线程 run()
    // --------------------------------------------------
    void mIMotionJsonFile::run()
    {
        if (!_modelData) {
            emit importFinished(false, "模型数据为空");
            return;
        }

        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            QString("正在导入运动数据：%1").arg(_fileName));

        QFile file(_fileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit importFinished(false, QString("无法打开文件: %1").arg(_fileName));
            return;
        }

        QByteArray jsonData = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(jsonData);
        if (doc.isNull() || !doc.isObject()) {
            emit importFinished(false, "JSON 解析失败，文件格式不正确");
            return;
        }

        QJsonObject root = doc.object();

        // 版本检查（可选，未来兼容用）
        int formatVersion = root.value("formatVersion").toInt(1);
        if (formatVersion > 1) {
            emit importFinished(false, QString("不支持的格式版本: %1").arg(formatVersion));
            return;
        }

        QJsonObject motionData = root.value("motionData").toObject();
        if (motionData.isEmpty()) {
            emit importFinished(false, "JSON 中未找到 motionData 节点");
            return;
        }

        // 清空现有数据
        clearExistingMotionData();

        // 按顺序导入：Bodies → Joints → Drives → Markers → Scenarios
        // 使用 try-catch 保护单个对象解析失败不中断整体导入

        // Bodies
        QJsonArray bodiesArr = motionData.value("bodies").toArray();
        auto& bodyMgr = _modelData->getManager<mMotionBodyManager>(Physical_MotionBody);
        for (const auto& val : bodiesArr) {
            try {
                if (auto* body = jsonToBody(val.toObject())) {
                    bodyMgr.addData(body);
                }
            } catch (...) {
                // 跳过损坏的body条目，继续导入
            }
        }

        // Joints
        QJsonArray jointsArr = motionData.value("joints").toArray();
        auto& jointMgr = _modelData->getManager<mMotionJointManager>(Physical_MotionJoint);
        for (const auto& val : jointsArr) {
            try {
                if (auto* joint = jsonToJoint(val.toObject())) {
                    jointMgr.addData(joint);
                }
            } catch (...) {
                // 跳过损坏的joint条目
            }
        }

        // Springs
        QJsonArray springsArr = motionData.value("springs").toArray();
        auto& springMgr = _modelData->getManager<mMotionSpringManager>(Physical_MotionSpring);
        for (const auto& val : springsArr) {
            try {
                if (auto* spring = jsonToSpring(val.toObject())) {
                    springMgr.addData(spring);
                }
            } catch (...) {
                // 跳过损坏的spring条目
            }
        }

        // Couplings
        QJsonArray couplingsArr = motionData.value("couplings").toArray();
        auto& couplingMgr = _modelData->getManager<mMotionCouplingManager>(Physical_MotionCoupling);
        for (const auto& val : couplingsArr) {
            try {
                if (auto* coupling = jsonToCoupling(val.toObject())) {
                    couplingMgr.addData(coupling);
                }
            } catch (...) {
                // 跳过损坏的coupling条目
            }
        }

        // Drives
        QJsonArray drivesArr = motionData.value("drives").toArray();
        auto& driveMgr = _modelData->getManager<mMotionDriveManager>(Physical_MotionDrive);
        for (const auto& val : drivesArr) {
            try {
                if (auto* drive = jsonToDrive(val.toObject())) {
                    driveMgr.addData(drive);
                }
            } catch (...) {
                // 跳过损坏的drive条目
            }
        }

        // Markers
        QJsonArray markersArr = motionData.value("markers").toArray();
        auto& markerMgr = _modelData->getManager<mMotionMarkerManager>(Physical_MotionMarker);
        for (const auto& val : markersArr) {
            try {
                if (auto* marker = jsonToMarker(val.toObject())) {
                    markerMgr.addData(marker);
                }
            } catch (...) {
                // 跳过损坏的marker条目
            }
        }

        // ForceElements
        auto& feMgr = _modelData->getManager<mMotionForceElementManager>(Physical_MotionVectorForce);

        // VectorForces
        QJsonArray vectorForcesArr = motionData.value("vectorForces").toArray();
        for (const auto& val : vectorForcesArr) {
            try {
                if (auto* vf = jsonToVectorForce(val.toObject())) {
                    feMgr.addData(vf);
                }
            } catch (...) {
                // 跳过损坏的vectorForce条目
            }
        }

        // ScalarTorques
        QJsonArray scalarTorquesArr = motionData.value("scalarTorques").toArray();
        for (const auto& val : scalarTorquesArr) {
            try {
                if (auto* st = jsonToScalarTorque(val.toObject())) {
                    _modelData->getManager<mMotionForceElementManager>(Physical_MotionVectorForce).addData(st);
                }
            } catch (...) {
                // 跳过损坏的scalarTorque条目
            }
        }

        // ScalarForces
        QJsonArray scalarForcesArr = motionData.value("scalarForces").toArray();
        for (const auto& val : scalarForcesArr) {
            try {
                if (auto* sf = jsonToScalarForce(val.toObject())) {
                    feMgr.addData(sf);
                }
            } catch (...) {
                // 跳过损坏的scalarForce条目
            }
        }

        // ThreePointForces
        QJsonArray threePointForcesArr = motionData.value("threePointForces").toArray();
        for (const auto& val : threePointForcesArr) {
            try {
                if (auto* tpf = jsonToThreePointForce(val.toObject())) {
                    feMgr.addData(tpf);
                }
            } catch (...) {
                // 跳过损坏的threePointForce条目
            }
        }

        // ThreePointTorques
        QJsonArray threePointTorquesArr = motionData.value("threePointTorques").toArray();
        for (const auto& val : threePointTorquesArr) {
            try {
                if (auto* tpt = jsonToThreePointTorque(val.toObject())) {
                    feMgr.addData(tpt);
                }
            } catch (...) {
                // 跳过损坏的threePointTorque条目
            }
        }

        // Contacts
        QJsonArray contactsArr = motionData.value("contacts").toArray();
        auto& contactMgr = _modelData->getManager<mMotionContactManager>(Physical_MotionContact);
        for (const auto& val : contactsArr) {
            try {
                if (auto* contact = jsonToContact(val.toObject())) {
                    contactMgr.addData(contact);
                }
            } catch (...) {
                // 跳过损坏的contact条目
            }
        }

        // Sensors
        QJsonArray sensorsArr = motionData.value("sensors").toArray();
        auto& sensorMgr = _modelData->getManager<mMotionSensorManager>(Physical_MotionSensor);
        for (const auto& val : sensorsArr) {
            try {
                if (auto* sensor = jsonToSensor(val.toObject())) {
                    sensorMgr.addData(sensor);
                }
            } catch (...) {
                // 跳过损坏的sensor条目
            }
        }

        // Controllers
        QJsonArray controllersArr = motionData.value("controllers").toArray();
        auto& controllerMgr = _modelData->getManager<mMotionControllerManager>(Physical_MotionController);
        for (const auto& val : controllersArr) {
            try {
                if (auto* controller = jsonToController(val.toObject())) {
                    controllerMgr.addData(controller);
                }
            } catch (...) {
                // 跳过损坏的controller条目
            }
        }

        // AutoFlex
        QJsonArray autoFlexArr = motionData.value("autoFlexBodies").toArray();
        auto& autoFlexMgr = _modelData->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex);
        for (const auto& val : autoFlexArr) {
            try {
                if (auto* autoFlex = jsonToAutoFlex(val.toObject())) {
                    autoFlexMgr.addData(autoFlex);
                    // 模型树刷新（与面板保存同款信号；此前导入后树不显示柔性体）
                    emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateMotionAutoFlexSig(true, autoFlex->getId());
                }
            } catch (...) {
                // 跳过损坏的autoFlex条目
            }
        }

        // ControlInputs
        QJsonArray controlInputsArr = motionData.value("controlInputs").toArray();
        auto& controlInputMgr = _modelData->getManager<mMotionControlInputManager>(Physical_MotionControlInput);
        for (const auto& val : controlInputsArr) {
            try {
                if (auto* input = jsonToControlInput(val.toObject())) {
                    controlInputMgr.addData(input);
                }
            } catch (...) {
                // 跳过损坏的controlInput条目
            }
        }

        // Scenarios
        QJsonArray scenariosArr = motionData.value("scenarios").toArray();
        auto& scenarioMgr = _modelData->getManager<mMotionScenarioManager>(Physical_MotionScenario);
        for (const auto& val : scenariosArr) {
            try {
                if (auto* scenario = jsonToScenario(val.toObject())) {
                    scenarioMgr.addData(scenario);
                }
            } catch (...) {
                // 跳过损坏的scenario条目
            }
        }

        _isSuccess = true;
        emit importFinished(true, QString("成功导入运动数据: %1").arg(_fileName));
    }

    // --------------------------------------------------
    // JSON → 各数据类型
    // --------------------------------------------------
    mMotionBody* mIMotionJsonFile::jsonToBody(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* body = new mMotionBody(id, name);

        QJsonArray geoArr = obj.value("geoEntityIds").toArray();
        QVector<int> geoIds;
        for (const auto& v : geoArr) {
            geoIds.append(v.toInt());
        }
        for (int g : geoIds) { _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionBody, body->getId() }, { Physical_Part, g }); }

        body->setIsFixed(obj.value("isFixed").toBool(false));
        body->setMass(obj.value("mass").toDouble(0.0));
        body->setCenterOfMass(jsonToVec3(obj.value("centerOfMass").toObject()));
        body->setInitialVelocity(jsonToVec3(obj.value("initialVelocity").toObject()));
        // 初始角速度（°/s，全局系）：可选字段，旧格式缺省为 0
        if (obj.contains("initialAngularVelocity"))
        {
            body->setInitialAngularVelocity(jsonToVec3(obj.value("initialAngularVelocity").toObject()));
        }
        body->setIsCustomMassProps(obj.value("isCustomMassProps").toBool(false));

        QJsonArray inertiaArr = obj.value("inertia").toArray();
        QVector<double> inertia;
        for (const auto& v : inertiaArr) {
            inertia.append(v.toDouble(0.0));
        }
        body->setInertia(inertia);

        return body;
    }

    mMotionJointBase* mIMotionJsonFile::jsonToJoint(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        JointType type = stringToJointType(obj.value("jointType").toString());
        if (id < 0) return nullptr;

        mMotionJointBase* joint = nullptr;
        if (type == JointType::Universal) {
            auto* universal = new mMotionJointUniversal(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, universal->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, universal->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            universal->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            universal->setActionBodySpinVector(jsonToVec3(obj.value("actionSpin").toObject()));
            universal->setActionBodyPinVector(jsonToVec3(obj.value("actionPin").toObject()));
            universal->setBaseBodySpinVector(jsonToVec3(obj.value("baseSpin").toObject()));
            universal->setBaseBodyPinVector(jsonToVec3(obj.value("basePin").toObject()));
            joint = universal;
        }
        else if (type == JointType::PointOnSurface) {
            auto* pos = new MechMotionData::mMotionJointPointOnSurface(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, pos->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, pos->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            pos->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            pos->setAxis(jsonToVec3(obj.value("axis").toObject()));
            QVector<int> faceIds;
            const QJsonArray faceArr = obj.value("surfaceFaceIds").toArray();
            for (const auto& v : faceArr) faceIds.append(v.toInt());
            pos->setSurfaceFaceIds(faceIds);
            joint = pos;
        }
        else if (type == JointType::PointOnCurve) {
            auto* poc = new MechMotionData::mMotionJointPointOnCurve(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, poc->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, poc->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            poc->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            poc->setAxis(jsonToVec3(obj.value("axis").toObject()));
            QVector<int> edgeIds;
            const QJsonArray edgeArr = obj.value("curveEdgeIds").toArray();
            for (const auto& v : edgeArr) edgeIds.append(v.toInt());
            poc->setCurveEdgeIds(edgeIds);
            joint = poc;
        }
        else if (type == JointType::CurveOnCurve) {
            auto* cvcv = new MechMotionData::mMotionJointCurveOnCurve(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, cvcv->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, cvcv->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            cvcv->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            cvcv->setAxis(jsonToVec3(obj.value("axis").toObject()));
            QVector<int> edgeIdsI;
            const QJsonArray edgeArrI = obj.value("curveIEdgeIds").toArray();
            for (const auto& v : edgeArrI) edgeIdsI.append(v.toInt());
            cvcv->setCurveIEdgeIds(edgeIdsI);
            QVector<int> edgeIdsJ;
            const QJsonArray edgeArrJ = obj.value("curveJEdgeIds").toArray();
            for (const auto& v : edgeArrJ) edgeIdsJ.append(v.toInt());
            cvcv->setCurveJEdgeIds(edgeIdsJ);
            // 旧工程文件没有 allowSliding 字段：缺省 false（无滑滚动，UG 默认）
            cvcv->setAllowSliding(obj.value("allowSliding").toBool(false));
            joint = cvcv;
        }
        else if (type == JointType::Screw) {
            auto* screw = new mMotionJointScrew(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, screw->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, screw->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            screw->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            screw->setAxis(jsonToVec3(obj.value("axis").toObject()));
            screw->setLead(obj.value("lead").toDouble(0.01));
            joint = screw;
        }
        else if (type == JointType::Perpendicular) {
            auto* perpendicular = new mMotionJointPerpendicular(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, perpendicular->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, perpendicular->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            perpendicular->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            perpendicular->setActionBodyVector(jsonToVec3(obj.value("actionVector").toObject()));
            perpendicular->setBaseBodyVector(jsonToVec3(obj.value("baseVector").toObject()));
            joint = perpendicular;
        }
        else if (type == JointType::ConstantVelocity) {
            auto* constantVelocity = new mMotionJointConstantVelocity(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, constantVelocity->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, constantVelocity->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            constantVelocity->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            constantVelocity->setActionBodyVector(jsonToVec3(obj.value("actionVector").toObject()));
            constantVelocity->setBaseBodyVector(jsonToVec3(obj.value("baseVector").toObject()));
            joint = constantVelocity;
        }
        else {
            auto* basic = new mMotionJointBasic(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, basic->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionJoint, basic->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
            basic->setOrigin(jsonToVec3(obj.value("origin").toObject()));
            basic->setAxis(jsonToVec3(obj.value("axis").toObject()));
            joint = basic;
        }

        bool isUserRename = obj.value("isUserRename").toBool(true);
        if (!isUserRename) {
            joint->setName(name, false);
        }

        {
            const QJsonObject frictionObj = obj.value("friction").toObject();
            if (!frictionObj.isEmpty()) {
                mMotionJointFrictionData friction;
                friction.enabled = frictionObj.value("enabled").toBool(false);
                friction.staticCoefficient = frictionObj.value("staticCoefficient").toDouble(0.3);
                friction.kineticCoefficient = frictionObj.value("kineticCoefficient").toDouble(0.1);
                friction.viscousCoefficient = frictionObj.value("viscousCoefficient").toDouble(0.0);
                friction.effectiveRadius = frictionObj.value("effectiveRadius").toDouble(0.0);
                joint->setFrictionData(friction);
            }
        }

        // 限位配置：旋转副数值单位为"度"（刚度 N·mm/°），移动副为 mm（刚度 N/mm）。
        {
            const QJsonObject limitObj = obj.value("limit").toObject();
            if (!limitObj.isEmpty()) {
                mMotionJointLimitData limit;
                limit.enabled = limitObj.value("enabled").toBool(false);
                limit.lowerEnabled = limitObj.value("lowerEnabled").toBool(false);
                limit.upperEnabled = limitObj.value("upperEnabled").toBool(false);
                limit.lowerValue = limitObj.value("lowerValue").toDouble(0.0);
                limit.upperValue = limitObj.value("upperValue").toDouble(0.0);
                limit.penaltyOnly = limitObj.value("penaltyOnly").toBool(false);
                limit.lowerCushion = limitObj.value("lowerCushion").toDouble(0.0);
                limit.upperCushion = limitObj.value("upperCushion").toDouble(0.0);
                limit.lowerStiffness = limitObj.value("lowerStiffness").toDouble(0.0);
                limit.upperStiffness = limitObj.value("upperStiffness").toDouble(0.0);
                limit.lowerDamping = limitObj.value("lowerDamping").toDouble(0.0);
                limit.upperDamping = limitObj.value("upperDamping").toDouble(0.0);
                joint->setLimitData(limit);
            }
        }

        return joint;
    }

    mMotionCouplingBase* mIMotionJsonFile::jsonToCoupling(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        CouplingType type = stringToCouplingType(obj.value("couplingType").toString());
        if (id < 0) return nullptr;

        mMotionCouplingBase* coupling = nullptr;
        if (type == CouplingType::GearRack) {
            auto* gearRack = new mMotionCouplingGearRack(id, type, name);
            gearRack->setRadius(obj.value("radius").toDouble(1.0));
            coupling = gearRack;
        }
        else if (type == CouplingType::Gear) {
            auto* gear = new mMotionCouplingGear(id, type, name);
            gear->setGearRatio(obj.value("gearRatio").toDouble(1.0));
            gear->setToothCount1(obj.value("toothCount1").toInt(20));
            gear->setToothCount2(obj.value("toothCount2").toInt(20));
            gear->setPitchRadius1(obj.value("pitchRadius1").toDouble(100.0));
            gear->setPitchRadius2(obj.value("pitchRadius2").toDouble(100.0));
            coupling = gear;
        }
        else if (type == CouplingType::Cable) {
            auto* cable = new mMotionCouplingCable(id, type, name);
            cable->setRatio(obj.value("ratio").toDouble(1.0));
            cable->setReverse(obj.value("reverse").toBool(false));
            coupling = cable;
        }

        if (!coupling) {
            coupling = new mMotionCouplingGearRack(id, type, name);
        }

        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionCoupling, coupling->getId() }, { Physical_MotionJoint, obj.value("firstJointId").toInt(-1) }, MxFunctions::MapRelation_MotionFirstJoint);
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionCoupling, coupling->getId() }, { Physical_MotionJoint, obj.value("secondJointId").toInt(-1) }, MxFunctions::MapRelation_MotionSecondJoint);
        coupling->setName(name, obj.value("isUserRename").toBool(true));

        return coupling;
    }

    mMotionSpringBase* mIMotionJsonFile::jsonToSpring(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        SpringType type = stringToSpringType(obj.value("springType").toString());
        if (id < 0) return nullptr;

        mMotionSpringBase* spring = nullptr;
        if (type == SpringType::Translational)
            spring = new mMotionTranslationalSpring(id);
        else if (type == SpringType::Torsional)
            spring = new mMotionTorsionalSpring(id);
        else
            spring = new mMotionBodyPointSpring(id);

        spring->setName(name, obj.value("isUserRename").toBool(true));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSpring, spring->getId() }, { Physical_MotionJoint, obj.value("jointId").toInt(-1) });
        spring->setStiffness(obj.value("stiffness").toDouble(0.0));
        spring->setDamping(obj.value("damping").toDouble(0.0));
        spring->setPreload(obj.value("preload").toDouble(0.0));
        spring->setSpringDirectionMode(stringToSpringDirectionMode(obj.value("springDirectionMode").toString()));
        spring->setDampingDirectionMode(stringToSpringDirectionMode(obj.value("dampingDirectionMode").toString()));
        spring->setFreeValue(obj.value("freeValue").toDouble(0.0));
        spring->setInstallValue(obj.value("installValue").toDouble(0.0));
        spring->setDampingEnabled(obj.value("dampingEnabled").toBool(false));
        spring->setForceFunction(obj.value("forceFunction").toString());
        spring->setDampingFunction(obj.value("dampingFunction").toString());

        QJsonArray tableXArr = obj.value("tableX").toArray();
        QJsonArray tableYArr = obj.value("tableY").toArray();
        QVector<double> tableX, tableY;
        for (const auto& v : tableXArr) tableX.append(v.toDouble(0.0));
        for (const auto& v : tableYArr) tableY.append(v.toDouble(0.0));
        if (!tableX.isEmpty() && !tableY.isEmpty())
            spring->setTable(tableX, tableY);

        // 体-点弹簧端点信息
        if (auto* bpSpring = dynamic_cast<mMotionBodyPointSpring*>(spring)) {
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSpring, bpSpring->getId() }, { Physical_MotionBody, obj.value("bodyIId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyI);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSpring, bpSpring->getId() }, { Physical_MotionBody, obj.value("bodyJId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyJ);
            bpSpring->setLocalI(jsonToVec3(obj.value("localI").toObject()));
            bpSpring->setLocalJ(jsonToVec3(obj.value("localJ").toObject()));
            bpSpring->setLocalRotI(jsonToQuat(obj.value("localRotI").toObject()));
            bpSpring->setLocalRotJ(jsonToQuat(obj.value("localRotJ").toObject()));
        }

        return spring;
    }

    mMotionDriveBase* mIMotionJsonFile::jsonToDrive(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        QString driveType = obj.value("driveType").toString();
        if (id < 0) return nullptr;

        mMotionDriveBase* drive = nullptr;

        if (driveType == "JointDrive") {
            auto* jointDrive = new mMotionJointDrive(id, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionDrive, jointDrive->getId() }, { Physical_MotionJoint, obj.value("targetJointId").toInt(-1) });
            drive = jointDrive;
        }
        else {
            // 未知类型，默认创建 JointDrive
            drive = new mMotionJointDrive(id, name);
        }

        drive->setMotionType(stringToDriveMotionType(obj.value("motionType").toString()));
        drive->setControlMode(stringToDriveControlMode(obj.value("controlMode").toString()));
        drive->setExpression(obj.value("expression").toString());
        // 表格驱动（旧文件缺省为空表）
        {
            QVector<double> tx, ty;
            for (const auto& v : obj.value("driveTableX").toArray()) tx.append(v.toDouble());
            for (const auto& v : obj.value("driveTableY").toArray()) ty.append(v.toDouble());
            if (tx.size() >= 2 && tx.size() == ty.size())
                drive->setDriveTable(tx, ty);
        }

        return drive;
    }

    mMotionMarker* mIMotionJsonFile::jsonToMarker(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* marker = new mMotionMarker(id, name);
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionMarker, marker->getId() }, { Physical_MotionBody, obj.value("bodyId").toInt(-1) });
        marker->setLocalPosition(jsonToVec3(obj.value("localPosition").toObject()));
        marker->setLocalRotation(jsonToQuat(obj.value("localRotation").toObject()));
        marker->setColor(jsonToColor(obj.value("color").toObject()));
        marker->setSize(obj.value("size").toDouble(5.0));
        marker->setVisible(obj.value("visible").toBool(true));

        return marker;
    }

    mMotionVectorForce* mIMotionJsonFile::jsonToVectorForce(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        VectorLoadType loadType = stringToVectorLoadType(obj.value("loadType").toString());
        if (id < 0) return nullptr;

        // Bushing 类型创建子类对象，其余创建基类对象
        mMotionVectorForce* force = nullptr;
        if (loadType == VectorLoadType::Bushing)
            force = new mMotionBushing(id, name);
        else
            force = new mMotionVectorForce(id, loadType, name);
        // 与运动副导入一致：字段缺失时按"用户已命名"处理，避免类型切换时改写自定义名
        force->setName(name, obj.value("isUserRename").toBool(true));

        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("bodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
        force->setActionPoint(jsonToVec3(obj.value("actionPoint").toObject()));
        force->setDirection(jsonToVec3(obj.value("direction").toObject()));
        force->setMagnitude(obj.value("magnitude").toDouble(0.0));
        force->setExpression(obj.value("expression").toString());
        force->setMagnitudeMode(stringToMagnitudeMode(obj.value("magnitudeMode").toString()));
        force->setDirectionMode(stringToDirectionMode(obj.value("directionMode").toString()));

        // Bushing 专属字段
        if (auto* bushing = dynamic_cast<mMotionBushing*>(force)) {
            // 刚度 / 阻尼
            QJsonArray stiffnessArr = obj.value("stiffness").toArray();
            if (stiffnessArr.size() == 6) {
                QVector<double> k;
                for (int i = 0; i < 6; ++i) k.append(stiffnessArr[i].toDouble(0.0));
                bushing->setStiffness(k);
            }

            QJsonArray dampingArr = obj.value("damping").toArray();
            if (dampingArr.size() == 6) {
                QVector<double> c;
                for (int i = 0; i < 6; ++i) c.append(dampingArr[i].toDouble(0.0));
                bushing->setDamping(c);
            }

            // 基本体 (marker_J)
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, bushing->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);

            // 偏置 (D0)
            bushing->setApplyOffset(obj.value("applyOffset").toBool(false));
            bushing->setOffset(jsonToVec3(obj.value("offset").toObject()));

            // 预载 (F0/T0)
            QJsonArray f0Arr = obj.value("preloadForce").toArray();
            if (f0Arr.size() == 3) {
                QVector<double> f0;
                for (int i = 0; i < 3; ++i) f0.append(f0Arr[i].toDouble(0.0));
                bushing->setPreloadForce(f0);
            }

            QJsonArray t0Arr = obj.value("preloadTorque").toArray();
            if (t0Arr.size() == 3) {
                QVector<double> t0;
                for (int i = 0; i < 3; ++i) t0.append(t0Arr[i].toDouble(0.0));
                bushing->setPreloadTorque(t0);
            }
        }

        return force;
    }

    MechMotionData::mMotionScalarTorque* mIMotionJsonFile::jsonToScalarTorque(const QJsonObject& obj)
    {
        using namespace MechMotionData;
        int id = obj.value("id").toInt(-1);
        if (id < 0) return nullptr;

        auto* st = new mMotionScalarTorque(id, obj.value("name").toString());
        st->setName(st->getName(), obj.value("isUserRename").toBool(true));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, st->getId() }, { Physical_MotionBody, obj.value("bodyIId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyI);
        st->setLocalI(jsonToVec3(obj.value("localI").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, st->getId() }, { Physical_MotionBody, obj.value("bodyJId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyJ);
        st->setLocalJ(jsonToVec3(obj.value("localJ").toObject()));
        st->setAxisMode(static_cast<ScalarTorqueAxisMode>(obj.value("axisMode").toInt(0)));
        st->setFixedAxis(jsonToVec3(obj.value("fixedAxis").toObject()));
        st->setMagnitude(obj.value("magnitude").toDouble(0.0));
        st->setTorqueFunction(obj.value("torqueFunction").toString());
        st->setActionOnly(obj.value("actionOnly").toBool(false));
        return st;
    }

    MechMotionData::mMotionThreePointTorque* mIMotionJsonFile::jsonToThreePointTorque(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* torque = new MechMotionData::mMotionThreePointTorque(id, name);
        torque->setName(name, obj.value("isUserRename").toBool(true));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, torque->getId() }, { Physical_MotionBody, obj.value("bodyIId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyI);
        torque->setLocalI(jsonToVec3(obj.value("localI").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, torque->getId() }, { Physical_MotionBody, obj.value("targetBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
        torque->setLocalTarget(jsonToVec3(obj.value("localTarget").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, torque->getId() }, { Physical_MotionBody, obj.value("originBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
        torque->setLocalOrigin(jsonToVec3(obj.value("localOrigin").toObject()));
        torque->setMagnitude(obj.value("magnitude").toDouble(0.0));
        torque->setTorqueFunction(obj.value("torqueFunction").toString());
        return torque;
    }

    MechMotionData::mMotionThreePointForce* mIMotionJsonFile::jsonToThreePointForce(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* force = new MechMotionData::mMotionThreePointForce(id, name);
        force->setName(name, obj.value("isUserRename").toBool(true));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("bodyIId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyI);
        force->setLocalI(jsonToVec3(obj.value("localI").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("targetBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
        force->setLocalTarget(jsonToVec3(obj.value("localTarget").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("originBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
        force->setLocalOrigin(jsonToVec3(obj.value("localOrigin").toObject()));
        force->setMagnitude(obj.value("magnitude").toDouble(0.0));
        force->setForceFunction(obj.value("forceFunction").toString());
        return force;
    }

    mMotionScalarForce* mIMotionJsonFile::jsonToScalarForce(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* force = new mMotionScalarForce(id, name);
        force->setName(name, obj.value("isUserRename").toBool(true));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("bodyIId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyI);
        force->setLocalI(jsonToVec3(obj.value("localI").toObject()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionVectorForce, force->getId() }, { Physical_MotionBody, obj.value("bodyJId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyJ);
        force->setLocalJ(jsonToVec3(obj.value("localJ").toObject()));
        force->setDirectionMode(static_cast<ScalarForceDirectionMode>(obj.value("directionMode").toInt(0)));
        force->setFixedDirection(jsonToVec3(obj.value("fixedDirection").toObject()));
        force->setMagnitude(obj.value("magnitude").toDouble(0.0));
        force->setForceFunction(obj.value("forceFunction").toString());
        force->setActionOnly(obj.value("actionOnly").toBool(false));
        return force;
    }

    mMotionScenario* mIMotionJsonFile::jsonToScenario(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* scenario = new mMotionScenario(id, name);
        scenario->setEndTime(obj.value("endTime").toDouble(1.0));
        scenario->setStepSize(obj.value("stepSize").toDouble(0.01));
        scenario->setGravity(jsonToVec3(obj.value("gravity").toObject()));
        scenario->setUsePreset(obj.value("usePreset").toBool(true));
        scenario->setPresetMode(stringToPresetMode(obj.value("presetMode").toString()));
        scenario->setIntegratorType(stringToIntegratorType(obj.value("integratorType").toString()));
        scenario->setIntegratorAlpha(obj.value("integratorAlpha").toDouble(-0.2));
        scenario->setIntegratorBeta(obj.value("integratorBeta").toDouble(0.25));
        scenario->setIntegratorMaxIters(obj.value("integratorMaxIters").toInt(50));
        scenario->setIntegratorTol(obj.value("integratorTol").toDouble(1e-8));
        scenario->setSolverType(stringToSolverType(obj.value("solverType").toString()));
        scenario->setSolverMaxIters(obj.value("solverMaxIters").toInt(100));
        scenario->setSolverTolerance(obj.value("solverTolerance").toDouble(1e-10));
        scenario->setSolverVerbose(obj.value("solverVerbose").toBool(false));

        // trackedMarkerIds
        QJsonArray trackedArr = obj.value("trackedMarkerIds").toArray();
        for (const auto& v : trackedArr) {
            scenario->addTrackedMarkerId(v.toInt());
        }

        // curveNode
        QJsonValue curveVal = obj.value("curveNode");
        if (!curveVal.isNull() && curveVal.isObject()) {
            if (auto* curve = jsonToCurve(curveVal.toObject())) {
                scenario->setCurveNode(curve);
            }
        }

        // currentCurveRequest
        QJsonValue reqVal = obj.value("currentCurveRequest");
        if (!reqVal.isNull() && reqVal.isObject()) {
            scenario->setCurrentCurveRequest(jsonToCurveRequest(reqVal.toObject()));
        }

        // curveRequests（多曲线列表；旧文件缺省时把单条迁移为列表首项）
        QJsonValue reqArrVal = obj.value("curveRequests");
        if (reqArrVal.isArray()) {
            scenario->clearCurveRequests();
            const QJsonArray reqArr = reqArrVal.toArray();
            for (const auto& v : reqArr) {
                if (v.isObject()) {
                    scenario->addCurveRequest(jsonToCurveRequest(v.toObject()));
                }
            }
        }
        else if (scenario->hasCurrentCurveRequest()) {
            scenario->addCurveRequest(scenario->getCurrentCurveRequest());
        }
        // addCurveRequest updates the last-edited request as a convenience.
        // Restore an explicitly saved selection after importing the list.
        if (reqVal.isObject())scenario->setCurrentCurveRequest(jsonToCurveRequest(reqVal.toObject()));

        return scenario;
    }

    mMotionCurveData* mIMotionJsonFile::jsonToCurve(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* curve = new mMotionCurveData(id, name);
        curve->m_xAxisLabel = obj.value("xAxisLabel").toString();
        curve->m_yAxisLabel = obj.value("yAxisLabel").toString();

        QJsonArray xArr = obj.value("xData").toArray();
        QJsonArray yArr = obj.value("yData").toArray();
        for (const auto& v : xArr) {
            curve->m_xData.push_back(v.toDouble(0.0));
        }
        for (const auto& v : yArr) {
            curve->m_yData.push_back(v.toDouble(0.0));
        }

        return curve;
    }

    mMotionCurveRequest mIMotionJsonFile::jsonToCurveRequest(const QJsonObject& obj)
    {
        mMotionCurveRequest req;
        req.m_originalJson = obj;
        req.m_requestId = obj.value("requestId").toInt(obj.value("id").toInt(-1));
        req.m_requestName = obj.value("requestName").toString(obj.value("name").toString());
        const QString source = obj.value("sourceType").toString();
        const QString variable = obj.value("variable").toString();
        req.m_sourceType = mMotionCurveRequest::sourceTypeFromName(source);
        req.m_sourceId = obj.value("sourceId").toInt(-1);
        req.m_sourceName = obj.value("sourceName").toString();
        req.m_variable = mMotionCurveRequest::variableFromName(variable);
        const auto component = obj.value("component");
        req.m_component = component.isDouble()
            ? component.toInt() == -1 ? CurveComponent::Magnitude : static_cast<CurveComponent>(component.toInt())
            : stringToCurveComponent(component.toString());
        const QString collection = mMotionCurveRequest::collectionForSource(req.m_sourceType);
        if (component.isDouble() || source == collection || int(req.m_sourceType) >= int(CurveSourceType::Spring)
            || req.m_variable == CurveVariable::NativeChannel) {
            req.m_nativeSource = collection.isEmpty() ? source : collection;
            req.m_nativeVariable = variable;
            req.m_nativeComponent = component.isDouble() ? component.toInt()
                : req.m_component == CurveComponent::Magnitude ? -1 : int(req.m_component);
        }
        QJsonValue colorVal = obj.value("color");
        if (colorVal.isObject()) {
            req.m_color = jsonToColor(colorVal.toObject());
        } else if (colorVal.isString()) req.m_color = QColor(colorVal.toString());
        req.m_visible = obj.value("visible").toBool(true);
        return req;
    }

    mMotionContact* mIMotionJsonFile::jsonToContact(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* contact = new mMotionContact(id, name);
        contact->setContactType(stringToContactType(obj.value("contactType").toString()));
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionContact, contact->getId() }, { Physical_MotionBody, obj.value("actionBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionActionBody);
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionContact, contact->getId() }, { Physical_MotionBody, obj.value("baseBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionBaseBody);
        contact->setUseConvexHullAction(obj.value("useConvexHullAction").toBool(true));
        contact->setUseConvexHullBase(obj.value("useConvexHullBase").toBool(true));
        contact->setStiffness(obj.value("stiffness").toDouble(100000.0));
        contact->setForceExponent(obj.value("forceExponent").toDouble(2.0));
        contact->setMaterialDamping(obj.value("materialDamping").toDouble(10.0));
        contact->setPenetrationDepth(obj.value("penetrationDepth").toDouble(0.0254));
        contact->setFrictionEnabled(obj.value("frictionEnabled").toBool(false));
        contact->setStaticFriction(obj.value("staticFriction").toDouble(0.3));
        contact->setDynamicFriction(obj.value("dynamicFriction").toDouble(0.2));
        contact->setForceModel(obj.value("forceModel").toString("自动"));
        contact->setForceModelTolerance(obj.value("forceModelTolerance").toDouble(5.0));
        contact->setActionUseFacetFile(obj.value("actionUseFacetFile").toBool(false));
        contact->setActionDistanceTolerance(obj.value("actionDistanceTolerance").toDouble(0.0));
        contact->setActionAngleTolerance(obj.value("angleTolerance").toDouble(25.0));
        contact->setActionUseMaxFacetSize(obj.value("actionUseMaxFacetSize").toBool(false));
        contact->setActionMaxFacetSize(obj.value("actionMaxFacetSize").toDouble(1000.0));
        contact->setBaseUseFacetFile(obj.value("baseUseFacetFile").toBool(false));
        contact->setBaseDistanceTolerance(obj.value("baseDistanceTolerance").toDouble(0.0));
        contact->setBaseUseMaxFacetSize(obj.value("baseUseMaxFacetSize").toBool(false));
        contact->setBaseMaxFacetSize(obj.value("baseMaxFacetSize").toDouble(1000.0));

        return contact;
    }

    static void readBaseSensorFields(mMotionSensorBase* sensor, const QJsonObject& obj, MechModelData::mModelData* modelData) {
        sensor->setCoordSystem(stringToCoordSystem(obj.value("coordSystem").toString()));
        modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, sensor->getId() }, { Physical_MotionBody, obj.value("referenceBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionReferenceBody);
        sensor->setComponentMode(stringToComponentMode(obj.value("componentMode").toString()));
        sensor->setEnabled(obj.value("enabled").toBool(true));
        bool isUserRename = obj.value("isUserRename").toBool(false);
        sensor->setName(sensor->getName(), isUserRename);
    }

    mMotionSensorBase* mIMotionJsonFile::jsonToSensor(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        SensorType type = stringToSensorType(obj.value("sensorType").toString());
        if (id < 0) return nullptr;

        mMotionSensorBase* sensor = nullptr;

        if (type == SensorType::BodyDisplacement || type == SensorType::BodyVelocity || type == SensorType::BodyAcceleration) {
            BodySensorVariable var = stringToBodySensorVariable(obj.value("variable").toString());
            // 校正历史数据中类型与变量不匹配的组合（按变量意图重新定型）
            SensorType actualType = type;
            normalizeBodySensor(actualType, var);
            auto* bs = new mMotionBodySensor(id, actualType, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, bs->getId() }, { Physical_MotionBody, obj.value("targetBodyId").toInt(-1) });
            bs->setReferencePoint(jsonToVec3(obj.value("referencePoint").toObject()));
            bs->setVariable(var);
            sensor = bs;
        }
        else if (type == SensorType::JointReaction) {
            auto* js = new mMotionJointSensor(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, js->getId() }, { Physical_MotionJoint, obj.value("targetJointId").toInt(-1) });
            js->setVariable(stringToJointSensorVariable(obj.value("variable").toString()));
            sensor = js;
        }
        else if (type == SensorType::DriveActuation) {
            auto* ds = new mMotionDriveSensor(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, ds->getId() }, { Physical_MotionDrive, obj.value("targetDriveId").toInt(-1) });
            ds->setVariable(stringToDriveSensorVariable(obj.value("variable").toString()));
            sensor = ds;
        }
        else if (type == SensorType::Distance) {
            auto* dist = new mMotionDistanceSensor(id, type, name);
            dist->setPointA(jsonToSensorPointRef(obj.value("pointA").toObject()));
            dist->setPointB(jsonToSensorPointRef(obj.value("pointB").toObject()));
            dist->setVariable(stringToDistanceVariable(obj.value("variable").toString()));
            sensor = dist;
        }
        else if (type == SensorType::Angle) {
            auto* ang = new mMotionAngleSensor(id, type, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, ang->getId() }, { Physical_MotionBody, obj.value("axisBodyId").toInt(-1) }, MxFunctions::MapRelation_MotionAxisBody);
            ang->setAxisDirection(jsonToVec3(obj.value("axisDirection").toObject()));
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, ang->getId() }, { Physical_MotionBody, obj.value("bodyAId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyA);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionSensor, ang->getId() }, { Physical_MotionBody, obj.value("bodyBId").toInt(-1) }, MxFunctions::MapRelation_MotionBodyB);
            sensor = ang;
        }

        if (sensor) {
            readBaseSensorFields(sensor, obj, _modelData);
        }

        return sensor;
    }

    static MechMotionData::ControllerType stringToControllerType(const QString& str)
    {
        if (str == QStringLiteral("PID")) return MechMotionData::ControllerType::PID;
        return MechMotionData::ControllerType::PID;
    }

    static MechMotionData::PidFeedbackType stringToPidFeedbackType(const QString& str)
    {
        if (str == QStringLiteral("Velocity")) return MechMotionData::PidFeedbackType::Velocity;
        return MechMotionData::PidFeedbackType::Position;
    }

    MechMotionData::mMotionControlInput* mIMotionJsonFile::jsonToControlInput(const QJsonObject& obj)
    {
        using namespace MechMotionData;
        int id = obj.value("id").toInt(-1);
        if (id < 0) return nullptr;

        const QString typeStr = obj.value("inputType").toString("Constant");
        ControlInputType type = ControlInputType::Constant;
        if (typeStr == "Linear") type = ControlInputType::Linear;
        else if (typeStr == "Quadratic") type = ControlInputType::Quadratic;
        else if (typeStr == "Cubic") type = ControlInputType::Cubic;
        else if (typeStr == "CustomCurve") type = ControlInputType::CustomCurve;

        auto* input = new mMotionControlInput(id, obj.value("name").toString());
        input->setInputType(type);
        input->setTimeStart(obj.value("timeStart").toDouble(0.0));
        input->setTimeEnd(obj.value("timeEnd").toDouble(1.0));

        QVector<double> coeffs;
        for (const auto& v : obj.value("polyCoeffs").toArray()) {
            coeffs.append(v.toDouble());
        }
        if (coeffs.isEmpty()) coeffs.append(0.0);
        input->setPolyCoeffs(coeffs);

        QVector<QPair<double, double>> pts;
        for (const auto& v : obj.value("curvePoints").toArray()) {
            QJsonArray pair = v.toArray();
            if (pair.size() >= 2)
                pts.append({ pair.at(0).toDouble(), pair.at(1).toDouble() });
        }
        input->setCurvePoints(pts);

        return input;
    }

    MechMotionData::mMotionControllerBase* mIMotionJsonFile::jsonToController(const QJsonObject& obj)
    {
        using namespace MechMotionData;
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        ControllerType type = stringToControllerType(obj.value("controllerType").toString());
        if (id < 0) return nullptr;

        mMotionControllerBase* controller = nullptr;

        if (type == ControllerType::PID) {
            auto* pid = new mMotionPIDController(id, name);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionController, pid->getId() }, { Physical_MotionDrive, obj.value("targetDriveId").toInt(-1) });
            pid->setFeedbackType(stringToPidFeedbackType(obj.value("feedbackType").toString()));
            pid->setTargetValue(obj.value("targetValue").toDouble(0.0));
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionController, pid->getId() }, { Physical_MotionControlInput, obj.value("targetInputId").toInt(-1) });
            pid->setFeedbackSource(obj.value("feedbackSource").toString() == QStringLiteral("Sensor")
                ? PidFeedbackSource::Sensor : PidFeedbackSource::DriveDof);
            _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionController, pid->getId() }, { Physical_MotionSensor, obj.value("feedbackSensorId").toInt(-1) });
            pid->setFeedbackSign(obj.value("feedbackSign").toDouble(1.0));
            pid->setGains(obj.value("kp").toDouble(1.0),
                          obj.value("ki").toDouble(0.0),
                          obj.value("kd").toDouble(0.0));
            pid->setOutputLimit(obj.value("outputLimit").toDouble(0.0));
            controller = pid;
        }

        if (controller) {
            controller->setEnabled(obj.value("enabled").toBool(true));
            if (obj.value("isUserRename").toBool(false)) {
                controller->setName(name, true);
            }
        }

        return controller;
    }

    mMotionAutoFlex* mIMotionJsonFile::jsonToAutoFlex(const QJsonObject& obj)
    {
        int id = obj.value("id").toInt(-1);
        QString name = obj.value("name").toString();
        if (id < 0) return nullptr;

        auto* autoFlex = new mMotionAutoFlex(id, name);
        _modelData->getMapRelationManager().appendMapRelation({ Physical_MotionAutoFlex, autoFlex->getId() }, { Physical_MotionBody, obj.value("targetBodyId").toInt(-1) });
        autoFlex->setMaterialId(obj.value("materialId").toInt(-1));

        QVector<int> attachPoints;
        QJsonArray attachArr = obj.value("attachPoints").toArray();
        for (const auto& v : attachArr) {
            attachPoints.append(v.toInt(-1));
        }
        autoFlex->setAttachPoints(attachPoints);

        // 网格参数（旧文件兼容：缺省使用默认值）
        autoFlex->setElementSize(obj.value("elementSize").toDouble(5.0));
        autoFlex->setMeshType(static_cast<FlexMeshType>(obj.value("meshType").toInt(0)));
        autoFlex->setMinElementSize(obj.value("minElementSize").toDouble(1.0));
        autoFlex->setCurvatureRefinement(obj.value("curvatureRefinement").toBool(true));

        // 模态分析参数
        autoFlex->setModeCount(obj.value("modeCount").toInt(10));
        autoFlex->setFreqCutoff(obj.value("freqCutoff").toDouble(100.0));
        autoFlex->setSolverType(static_cast<FlexSolverType>(obj.value("solverType").toInt(0)));
        autoFlex->setFreqLower(obj.value("freqLower").toDouble(0.0));
        autoFlex->setFreqUpper(obj.value("freqUpper").toDouble(1e6));
        autoFlex->setRigidBodyModeFilter(obj.value("rigidBodyModeFilter").toBool(true));

        // 界面连接参数
        autoFlex->setConnectionType(static_cast<FlexConnectionType>(obj.value("connectionType").toInt(0)));
        autoFlex->setInterfaceDofs(obj.value("interfaceDofs").toInt(0x3F));
        autoFlex->setConnectionRadius(obj.value("connectionRadius").toDouble(0.0));

        // 输出控制
        autoFlex->setOutputModeShapes(obj.value("outputModeShapes").toBool(true));
        autoFlex->setOutputStressStrain(obj.value("outputStressStrain").toBool(true));
        autoFlex->setOutputFormat(static_cast<FlexOutputFormat>(obj.value("outputFormat").toInt(0)));

        // 阻尼参数
        autoFlex->setDampingType(static_cast<FlexDampingType>(obj.value("dampingType").toInt(1)));
        autoFlex->setModalDampingRatio(obj.value("modalDampingRatio").toDouble(0.02));
        autoFlex->setRayleighMass(obj.value("rayleighMass").toDouble(0.0));
        autoFlex->setRayleighStiffness(obj.value("rayleighStiffness").toDouble(0.0));

        autoFlex->setEnabled(obj.value("enabled").toBool(true));

        return autoFlex;
    }
}
