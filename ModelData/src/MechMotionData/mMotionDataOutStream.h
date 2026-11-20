#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include <QDataStream>

#include "mModelEnum.h"
#include "mModelStruct.h"
#include "mMotionBody.h"
#include "mMotionBodyManager.h"
#include "mMotionJointBase.h"
#include "mMotionJointBasic.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include "mMotionJointManager.h"
#include "mMotionCouplingBase.h"
#include "mMotionCouplingGearRack.h"
#include "mMotionCouplingGear.h"
#include "mMotionCouplingCable.h"
#include "mMotionCouplingManager.h"
#include "mMotionDriveManager.h"
#include "mMotionDriveBase.h"
#include "mMotionJointDrive.h"
#include "mMotionScenario.h"
#include "mMotionScenarioManager.h"
#include "mMotionCurveData.h"
#include "mMotionResultData.h"
#include "mMotionMarker.h"
#include "mMotionMarkerManager.h"
#include "mMotionVectorForce.h"
#include "mMotionBushing.h"
#include "mMotionScalarForce.h"
#include "mMotionThreePointForce.h"
#include "mMotionThreePointTorque.h"
#include "mMotionScalarTorque.h"
#include "mMotionForceElementManager.h"
#include "mMotionContact.h"
#include "mMotionContactManager.h"
#include "mMotionSpringBase.h"
#include "mMotionBodyPointSpring.h"
#include "mMotionTranslationalSpring.h"
#include "mMotionTorsionalSpring.h"
#include "mMotionSpringManager.h"
#include "mMotionControlInput.h"
#include "mMotionControlInputManager.h"
#include "mMotionElectroImport.h"
#include "mMotionElectroImportManager.h"
#include "mMotionElectroExport.h"
#include "mMotionElectroExportManager.h"
#include "mMotionHydraulic.h"
#include "mMotionHydraulicManager.h"
#include "mMotionRL.h"
#include "mMotionRLManager.h"

namespace MechMotionData
{
    // 运动副管理器 (包含多态)
    inline QDataStream& operator<<(QDataStream& out, mMotionJointManager& mgr)
    {
        out << mgr.getMaxID(); // 假设有对应的 getMaxID() 方法

        std::vector<int> jointIDs = mgr.getAllIDs();
        int count = jointIDs.size();
        out << count;

        for (int i = 0; i < count; ++i) {
            mMotionJointBase* obj = mgr.getData(jointIDs[i]);
            if (!obj) continue;

            JointType type = obj->getType();
            out << static_cast<qint32>(type); // 写入类型标记，反序列化时靠这个 new 对象

            switch (type)
            {
            case JointType::Revolute:
            case JointType::Translational:
            case JointType::Cylindrical:
            case JointType::Spherical:
            case JointType::Fixed:
            case JointType::PointPlane:
            case JointType::Line:
            case JointType::Parallel:
            case JointType::Direction:
            case JointType::Centripetal:
            case JointType::Hooke:
            case JointType::Planar:
            case JointType::Coupling:
            case JointType::Gear:
            case JointType::PointLine:
                // 这些类型目前底层用的都是 mMotionJointBasic 类
                out << *dynamic_cast<mMotionJointBasic*>(obj);
                break;
            case JointType::PointOnSurface:
                out << *dynamic_cast<mMotionJointPointOnSurface*>(obj);
                break;
            case JointType::PointOnCurve:
                out << *dynamic_cast<mMotionJointPointOnCurve*>(obj);
                break;
            case JointType::CurveOnCurve:
                out << *dynamic_cast<mMotionJointCurveOnCurve*>(obj);
                break;
            case JointType::Screw:
                out << *dynamic_cast<mMotionJointScrew*>(obj);
                break;
            case JointType::Universal:
                out << *dynamic_cast<mMotionJointUniversal*>(obj);
                break;
            case JointType::Perpendicular:
                out << *dynamic_cast<mMotionJointPerpendicular*>(obj);
                break;
            case JointType::ConstantVelocity:
                out << *dynamic_cast<mMotionJointConstantVelocity*>(obj);
                break;
                // 如果未来扩展了高级 Joint (如齿轮副 mMotionJointGear)，在这里加 case
                // case MechMotionData::JointType::Gear:
                //     out << *dynamic_cast<mMotionJointGear*>(obj);
                //     break;
            default:
                break;
            }
        }
        return out;
    }

    // 驱动管理器 (包含多态)
    inline QDataStream& operator<<(QDataStream& out, mMotionDriveManager& mgr)
    {
        out << mgr.getMaxID();

        std::vector<int> driveIDs = mgr.getAllIDs();
        int count = driveIDs.size();
        out << count;

        for (int i = 0; i < count; ++i) {
            mMotionDriveBase* obj = mgr.getData(driveIDs[i]);
            if (!obj) continue;

            // 假设通过 dynamic_cast 判别具体派生类（因为驱动基类可能没有 type 枚举）
            // 这是一种无 Type 枚举的多态序列化写法
            if (auto* jointDrive = dynamic_cast<mMotionJointDrive*>(obj)) {
                out << (qint32)1; // 手动给 JointDrive 标定一个类型码，比如 1
                out << *jointDrive;
            }
            // else if (auto* pointDrive = dynamic_cast<mMotionPointDrive*>(obj)) {
            //     out << (qint32)2;
            //     out << *pointDrive;
            // }
        }
        return out;
    }

    // 耦合副管理器 (包含多态)
    inline QDataStream& operator<<(QDataStream& out, mMotionCouplingManager& mgr)
    {
        out << mgr.getMaxID();

        std::vector<int> couplingIDs = mgr.getAllIDs();
        int count = couplingIDs.size();
        out << count;

        for (int i = 0; i < count; ++i) {
            mMotionCouplingBase* obj = mgr.getData(couplingIDs[i]);
            if (!obj) continue;

            CouplingType type = obj->getType();
            out << static_cast<qint32>(type);

            switch (type)
            {
            case CouplingType::GearRack:
                out << *dynamic_cast<mMotionCouplingGearRack*>(obj);
                break;
            case CouplingType::Gear:
                out << *dynamic_cast<mMotionCouplingGear*>(obj);
                break;
            case CouplingType::Cable:
                out << *dynamic_cast<mMotionCouplingCable*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    // 耦合副基类
    inline QDataStream& operator<<(QDataStream& out, mMotionCouplingBase& obj) {
        out << static_cast<qint32>(obj.getType())
            << obj.m_isUserRename
            << obj.getId()
            << obj.getName();
        return out;
    }

    // 齿轮齿条副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionCouplingGearRack& obj) {
        out << static_cast<mMotionCouplingBase&>(obj);
        out << obj.getRadius();
        return out;
    }

    // 齿轮副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionCouplingGear& obj) {
        out << static_cast<mMotionCouplingBase&>(obj);
        out << obj.getGearRatio();
        out << obj.getToothCount1();
        out << obj.getToothCount2();
        out << obj.getPitchRadius1();
        out << obj.getPitchRadius2();
        return out;
    }

    // 线缆副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionCouplingCable& obj) {
        out << static_cast<mMotionCouplingBase&>(obj);
        out << obj.getRatio();
        out << obj.getReverse();
        return out;
    }

    // 运动体管理器 (无多态，直接遍历序列化)
    inline QDataStream& operator<<(QDataStream& out, mMotionBodyManager& mgr)
    {
        out << mgr.getMaxID(); // 保存当前分配的最大 ID

        std::vector<int> bodyIDs = mgr.getAllIDs();
        int count = bodyIDs.size();
        out << count; // 保存运动体总数

        for (int i = 0; i < count; ++i) {
            mMotionBody* obj = mgr.getData(bodyIDs[i]);
            if (!obj) continue;

            // 因为没有派生类，不需要写 type 标识，直接解引用调用 mMotionBody 的 operator<<
            out << *obj;
        }
        return out;
    }

    //运动副摩擦数据
    inline QDataStream& operator<<(QDataStream& out, const mMotionJointFrictionData& data)
    {
        out << data.enabled
            << data.staticCoefficient
            << data.kineticCoefficient
            << data.viscousCoefficient
            << data.effectiveRadius;

        return out;
    }

    //运动副限位数据
    inline  QDataStream& operator<<(QDataStream& out, const mMotionJointLimitData& data)
    {
        out << data.enabled
            << data.lowerEnabled
            << data.upperEnabled
            << data.lowerValue
            << data.upperValue
            << data.penaltyOnly
            << data.lowerCushion
            << data.upperCushion
            << data.lowerStiffness
            << data.upperStiffness
            << data.lowerDamping
            << data.upperDamping;

        return out;
    }

    // 运动副基类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointBase& obj) {
        out << static_cast<qint32>(obj.getType())
            << obj.m_isUserRename
            << obj.getId()
            << obj.getName()
            << obj.getFrictionData()
            << obj.getLimitData();
        return out;
    }

    // 基本运动副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointBasic& obj) {
        out << static_cast<mMotionJointBase&>(obj);
        out << obj.getOrigin() << obj.getAxis();
        return out;
    }

    // 点在面上副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointPointOnSurface& obj) {
        out << static_cast<mMotionJointBasic&>(obj);
        out << obj.getSurfaceFaceIds();
        return out;
    }

    // 点在线上副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointPointOnCurve& obj) {
        out << static_cast<mMotionJointBasic&>(obj);
        out << obj.getCurveEdgeIds();
        return out;
    }

    // 线在线上副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointCurveOnCurve& obj) {
        out << static_cast<mMotionJointBasic&>(obj);
        out << obj.getCurveIEdgeIds();
        out << obj.getCurveJEdgeIds();
        out << obj.getAllowSliding();
        return out;
    }

    // 螺纹副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointScrew& obj) {
        out << static_cast<mMotionJointBase&>(obj);
        out << obj.getOrigin() << obj.getAxis() << obj.getLead();
        return out;
    }

    // 万向节运动副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointUniversal& obj) {
        out << static_cast<mMotionJointBase&>(obj);
        out << obj.getOrigin()
            << obj.getActionBodySpinVector()
            << obj.getActionBodyPinVector()
            << obj.getBaseBodySpinVector()
            << obj.getBaseBodyPinVector();
        return out;
    }

    // 垂直约束运动副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointPerpendicular& obj) {
        out << static_cast<mMotionJointBase&>(obj);
        out << obj.getOrigin()
            << obj.getActionBodyVector()
            << obj.getBaseBodyVector();
        return out;
    }

    // 等速运动副数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointConstantVelocity& obj) {
        out << static_cast<mMotionJointBase&>(obj);
        out << obj.getOrigin()
            << obj.getActionBodyVector()
            << obj.getBaseBodyVector();
        return out;
    }

    // 驱动基类
    inline QDataStream& operator<<(QDataStream& out, mMotionDriveBase& obj) {
        out << obj.getId() << obj.getName();
        out << static_cast<qint32>(obj.getTargetType())
            << static_cast<qint32>(obj.getMotionType())
            << static_cast<qint32>(obj.getControlMode());
        out << obj.getExpression();
        return out;
    }

    // 运动副驱动数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionJointDrive& obj) {
        out << static_cast<mMotionDriveBase&>(obj);
        return out;
    }

    // 求解方案数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionScenario& obj) {
        out << obj.getId() << obj.getName();
        out << obj.getEndTime() << obj.getStepSize() << obj.getGravity();
        out << obj.getUsePreset()
            << static_cast<qint32>(obj.getPresetMode())
            << static_cast<qint32>(obj.getIntegratorType())
            << obj.getIntegratorAlpha()
            << obj.getIntegratorBeta()
            << obj.getIntegratorMaxIters()
            << obj.getIntegratorTol()
            << static_cast<qint32>(obj.getSolverType())
            << obj.getSolverMaxIters()
            << obj.getSolverTolerance()
            << obj.getSolverVerbose();

        // 内嵌曲线节点
        if (obj.getCurveNode()) {
            out << static_cast<qint8>(1);
            out << *obj.getCurveNode();
        } else {
            out << static_cast<qint8>(0);
        }

        // 内嵌动画结果
        if (obj.getResult()) {
            out << static_cast<qint8>(1);
            out << *obj.getResult();
        } else {
            out << static_cast<qint8>(0);
        }

        // 曲线请求（标志：0=无 1=旧单条 2=列表；列表条目在旧字段后追加深色与可见性）
        if (!obj.getCurveRequests().empty()) {
            out << static_cast<qint8>(2);
            out << static_cast<qint32>(obj.getCurveRequests().size());
            for (const auto& req : obj.getCurveRequests()) {
                out << req;
                out << static_cast<qint8>(req.m_color.isValid() ? 1 : 0);
                if (req.m_color.isValid()) {
                    out << static_cast<qint32>(req.m_color.rgba());
                }
                out << req.m_visible;
            }
        }
        else if (obj.hasCurrentCurveRequest()) {
            out << static_cast<qint8>(1);
            out << obj.getCurrentCurveRequest();
        }
        else {
            out << static_cast<qint8>(0);
        }
        return out;
    }

    // 求解方案管理器 (无多态，直接遍历序列化)
    inline QDataStream& operator<<(QDataStream& out, mMotionScenarioManager& mgr)
    {
        out << mgr.getMaxID();

        std::vector<int> scenarioIDs = mgr.getAllIDs();
        int count = scenarioIDs.size();
        out << count; // 保存方案总数

        for (int i = 0; i < count; ++i) {
            mMotionScenario* obj = mgr.getData(scenarioIDs[i]);
            if (!obj) continue;

            // 直接序列化具体的方案对象
            out << *obj;
        }
        return out;
    }

    // 运动体数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionBody& obj)
    {
        // 假设你有对应的 Getter 或将其声明为了友元
        out << obj.getId() << obj.getName();
        out << obj.getIsFixed() << obj.getMass() << obj.getCenterOfMass() << obj.getInertia();

        // Qt 的 QList/QVector 自带 QDataStream 重载，直接存即可
        out << obj.getInitialVelocity();
        out << obj.getInitialAngularVelocity();
        return out;
    }

    // 曲线数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionCurveData& obj)
    {
        out << obj.m_id << obj.m_name;
        out << obj.m_xAxisLabel << obj.m_yAxisLabel;

        // std::vector 自带 QDataStream 支持，直接写入极其方便
        out << obj.m_xData;
        out << obj.m_yData;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const mMotionMarkerFrame& frame) {
        out << frame.m_markerId;
        out << frame.m_worldPosition;
        out << frame.m_worldRotation;
        out << frame.m_linearVelocity;
        out << frame.m_linearAcceleration;
        out << frame.m_angularVelocity;
        out << frame.m_angularAcceleration;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const mMotionBodyFrame& frame) {
        out << frame.m_position;
        out << frame.m_velocity;
        out << frame.m_acceleration;
        out << frame.m_rotation;
        out << frame.m_angularVelocity;
        out << frame.m_angularAcceleration;
        out << frame.m_displacement;
        out << frame.m_potentialEnergy;
        out << frame.m_kineticEnergy;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const mMotionJointFrame& frame) {
        out << frame.m_nodeI;
        out << frame.m_nodeJ;
        out << frame.m_reactionForce;
        out << frame.m_reactionTorque;
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const mMotionDriveFrame& frame) {
        out << frame.m_absCoord;
        out << frame.m_imposedValue;
        out << frame.m_imposedSpeed;
        out << frame.m_imposedAcc;
        out << frame.m_isRotation;
        out << frame.m_actuationForce;
        out << frame.m_actuationTorque;
        out << frame.m_reactionForce;
        out << frame.m_reactionTorque;
        return out;
    }

    template<typename MapType>
    static void writeMap(QDataStream& out, const MapType& map) {
        out << static_cast<int>(map.size());
        for (const auto& pair : map) {
            out << pair.first << pair.second;
        }
    }

    // 动画单帧数据
    inline QDataStream& operator<<(QDataStream& out, const mMotionFrame& obj)
    {
        out << obj.m_time;
        writeMap(out, obj.m_bodyTransforms);
        writeMap(out, obj.m_markerFrames);
        writeMap(out, obj.m_bodyFrames);
        writeMap(out, obj.m_jointFrames);
        writeMap(out, obj.m_couplingFrames);
        writeMap(out, obj.m_driveFrames);
        return out;
    }

    // 动画结果集
    // 柔性体帧数据（刚柔耦合）
    inline QDataStream& operator<<(QDataStream& out, const mMotionFlexFrame& f)
    {
        out << f.m_time << f.m_modalQ << f.m_interfacePose;
        return out;
    }

    // 柔性体结果数据（刚柔耦合）
    inline QDataStream& operator<<(QDataStream& out, const mMotionFlexBodyData& d)
    {
        out << d.m_bodyId << d.m_name << d.m_nodeX0 << d.m_tetIds << d.m_surfaceTris
            << d.m_psi << d.m_nModes << d.m_nInterfaces << d.m_interfacePose0;
        qint32 n = static_cast<qint32>(d.m_frames.size());
        out << n;
        for (qint32 i = 0; i < n; ++i)
            out << d.m_frames[i];
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, const mMotionResultData& obj)
    {
        qint32 frameCount = static_cast<qint32>(obj.m_frames.size());
        out << frameCount;

        for (qint32 i = 0; i < frameCount; ++i) {
            out << obj.m_frames[i]; // 调用上面写好的 mMotionFrame 序列化
        }

        // 柔性体数据（追加在尾部：旧工程文件读取时 EOF 得 count=0，向后兼容）
        qint32 flexCount = static_cast<qint32>(obj.m_flexBodies.size());
        out << flexCount;
        for (qint32 i = 0; i < flexCount; ++i)
            out << obj.m_flexBodies[i];
        return out;
    }

    // 标记点数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionMarker& marker) {
        out << marker.m_id;
        out << marker.m_name;
        out << marker.m_localPosition;
        out << marker.m_localRotation;
        out << marker.m_color;
        out << marker.m_size;
        out << marker.m_visible;
        return out;
    }

    // 标记点管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionMarkerManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 矢量力数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionVectorForce& force) {
        out << force.m_id << force.m_name;
        out << static_cast<qint32>(force.m_loadType);
        out << force.m_actionPoint;
        out << force.m_direction;
        out << force.m_magnitude;
        out << force.m_expression;
        out << static_cast<qint32>(force.m_magnitudeMode);
        out << static_cast<qint32>(force.m_directionMode);
        out << force.m_isUserRename;
        return out;
    }

    // 衬套数据类（基类通用段 + Bushing 扩展段）
    inline QDataStream& operator<<(QDataStream& out, mMotionBushing& bushing) {
        // 先写基类通用字段
        out << static_cast<mMotionVectorForce&>(bushing);
        // 再写 Bushing 扩展字段（刚度/阻尼 + marker_J + 预载 + 偏置）
        out << bushing.m_stiffness;
        out << bushing.m_damping;
        out << bushing.m_preloadForce;
        out << bushing.m_preloadTorque;
        out << bushing.m_applyOffset;
        out << bushing.m_offset;
        return out;
    }

    // 力元管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionForceElementManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            // 多态分派：先写类型标记，再写对应子类流（必须覆盖所有子类，否则流错位）
            if (auto* scalarForce = dynamic_cast<mMotionScalarForce*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::ScalarForce);
                out << *scalarForce;
            } else if (auto* threePointForce = dynamic_cast<mMotionThreePointForce*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::TriForce);
                out << *threePointForce;
            } else if (auto* threePointTorque = dynamic_cast<mMotionThreePointTorque*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::TriTorque);
                out << *threePointTorque;
            } else if (auto* scalarTorque = dynamic_cast<mMotionScalarTorque*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::ScalarTorque);
                out << *scalarTorque;
            } else if (auto* bushing = dynamic_cast<mMotionBushing*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::Force);
                out << *bushing;
            } else if (auto* vectorForce = dynamic_cast<mMotionVectorForce*>(pair.second)) {
                out << static_cast<qint32>(VectorLoadType::Force);
                out << *vectorForce;
            }
        }
        return out;
    }

    // 三点力矩数据类（与 operator>> 的字段顺序严格一致）
    inline QDataStream& operator<<(QDataStream& out, mMotionThreePointTorque& torque) {
        out << torque.m_id << torque.m_name;
        out << torque.m_localI;
        out << torque.m_localTarget;
        out << torque.m_magnitude;
        out << torque.m_torqueFunction;
        out << torque.m_isUserRename;
        return out;
    }

    // 三点力数据类（与 operator>> 的字段顺序严格一致）
    inline QDataStream& operator<<(QDataStream& out, mMotionThreePointForce& force) {
        out << force.m_id << force.m_name;
        out << force.m_localI;
        out << force.m_localTarget;
        out << force.m_magnitude;
        out << force.m_forceFunction;
        out << force.m_isUserRename;
        return out;
    }

    // 标量力数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionScalarForce& force) {
        out << force.m_id << force.m_name;
        out << force.m_localI;
        out << force.m_localJ;
        out << static_cast<qint32>(force.m_directionMode);
        out << force.m_fixedDirection;
        out << force.m_magnitude;
        out << force.m_forceFunction;
        out << force.m_actionOnly;
        out << force.m_isUserRename;
        return out;
    }

    // 标量力矩数据类（与 operator>> 的字段顺序严格一致）
    inline QDataStream& operator<<(QDataStream& out, mMotionScalarTorque& torque) {
        out << torque.m_id << torque.m_name;
        out << torque.m_localI;
        out << torque.m_localJ;
        out << static_cast<qint32>(torque.m_axisMode);
        out << torque.m_fixedAxis;
        out << torque.m_magnitude;
        out << torque.m_torqueFunction;
        out << torque.m_actionOnly;
        out << torque.m_isUserRename;
        return out;
    }

    // 3D 接触数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionContact& contact) {
        out << contact.m_id << contact.m_name;
        out << static_cast<qint32>(contact.m_contactType);
        out << contact.m_stiffness;
        out << contact.m_forceExponent;
        out << contact.m_materialDamping;
        out << contact.m_penetrationDepth;
        out << contact.m_frictionEnabled;
        out << contact.m_staticFriction;
        out << contact.m_dynamicFriction;
        out << contact.m_forceModel;
        out << contact.m_forceModelTolerance;
        out << contact.m_useConvexHullAction << contact.m_useConvexHullBase;
        out << contact.m_actionUseFacetFile;
        out << contact.m_actionDistanceTolerance;
        out << contact.m_angleTolerance;
        out << contact.m_actionUseMaxFacetSize;
        out << contact.m_actionMaxFacetSize;
        out << contact.m_baseUseFacetFile;
        out << contact.m_baseDistanceTolerance;
        out << contact.m_baseUseMaxFacetSize;
        out << contact.m_baseMaxFacetSize;
        return out;
    }

    // 3D 接触管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionContactManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 弹簧基类
    inline QDataStream& operator<<(QDataStream& out, mMotionSpringBase& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_type);
        out << obj.m_stiffness << obj.m_damping << obj.m_preload;
        out << static_cast<qint32>(obj.m_springDirectionMode) << static_cast<qint32>(obj.m_dampingDirectionMode);
        out << obj.m_freeValue << obj.m_installValue;
        out << obj.m_tableX << obj.m_tableY << obj.m_hasTable;
        out << obj.m_forceFunction << obj.m_dampingFunction;
        out << obj.m_dampingEnabled;
        return out;
    }

    // 拉压弹簧
    inline QDataStream& operator<<(QDataStream& out, mMotionTranslationalSpring& obj) {
        out << static_cast<mMotionSpringBase&>(obj);
        return out;
    }

    // 扭转弹簧
    inline QDataStream& operator<<(QDataStream& out, mMotionTorsionalSpring& obj) {
        out << static_cast<mMotionSpringBase&>(obj);
        return out;
    }

    inline QDataStream& operator<<(QDataStream& out, mMotionBodyPointSpring& obj) {
        out << static_cast<mMotionSpringBase&>(obj);
        out << obj.m_localI << obj.m_localJ;
        out << obj.m_localRotI << obj.m_localRotJ;
        return out;
    }

    // 弹簧管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionSpringManager& mgr) {
        out << mgr.getMaxID();

        std::vector<int> springIDs = mgr.getAllIDs();
        int count = springIDs.size();
        out << count;

        for (int i = 0; i < count; ++i) {
            mMotionSpringBase* obj = mgr.getData(springIDs[i]);
            if (!obj) continue;

            SpringType type = obj->getType();
            out << static_cast<qint32>(type);

            switch (type) {
            case SpringType::Translational:
                out << *dynamic_cast<mMotionTranslationalSpring*>(obj);
                break;
            case SpringType::Torsional:
                out << *dynamic_cast<mMotionTorsionalSpring*>(obj);
                break;
            case SpringType::BodyPoint:
                out << *dynamic_cast<mMotionBodyPointSpring*>(obj);
                break;
            default:
                break;
            }
        }
        return out;
    }

    // 控制输入数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionControlInput& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_inputType);
        out << obj.m_timeStart << obj.m_timeEnd;
        out << obj.m_polyCoeffs;
        out << static_cast<int>(obj.m_curvePoints.size());
        for (const auto& p : obj.m_curvePoints) {
            out << p.first << p.second;
        }
        return out;
    }

    // 控制输入管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionControlInputManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 机电导入数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionElectroImport& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_coSimType);
        out << obj.m_platform << obj.m_modelFile << obj.m_headerFile;
        out << static_cast<int>(obj.m_inputPorts.size());
        for (const auto& p : obj.m_inputPorts) {
            out << p.first << p.second;
        }
        out << static_cast<int>(obj.m_outputPorts.size());
        for (const auto& p : obj.m_outputPorts) {
            out << p.first << p.second;
        }
        return out;
    }

    // 机电导入管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionElectroImportManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 机电导出数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionElectroExport& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_exportType);
        out << obj.m_portListFile;
        out << static_cast<int>(obj.m_inputPorts.size());
        for (const auto& p : obj.m_inputPorts) {
            out << p.portName << p.order << p.scale << p.offset << p.unit;
        }
        out << static_cast<int>(obj.m_outputPorts.size());
        for (const auto& p : obj.m_outputPorts) {
            out << p.portName << p.order << p.scale << p.offset << p.unit;
        }
        return out;
    }

    // 机电导出管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionElectroExportManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 液压数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionHydraulic& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_hydraulicType);
        out << obj.m_modelFile;
        out << obj.m_fluidDensity << obj.m_fluidViscosity << obj.m_bulkModulus;
        out << static_cast<int>(obj.m_ports.size());
        for (const auto& p : obj.m_ports) {
            out << p.portName << p.diameter << p.length;
        }
        out << static_cast<int>(obj.m_pipes.size());
        for (const auto& p : obj.m_pipes) {
            out << p.pipeName << p.diameter << p.length;
        }
        return out;
    }

    // 液压管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionHydraulicManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

    // 强化学习数据类
    inline QDataStream& operator<<(QDataStream& out, mMotionRL& obj) {
        out << obj.m_id << obj.m_name;
        out << static_cast<qint32>(obj.m_algorithm);
        out << static_cast<qint32>(obj.m_envType);
        out << obj.m_stateSpace;
        out << obj.m_actionSpace;
        out << obj.m_hiddenLayers;
        out << obj.m_learningRate << obj.m_discountFactor;
        out << obj.m_maxEpisodes << obj.m_maxSteps;
        out << obj.m_datasetPath << obj.m_modelSavePath;
        return out;
    }

    // 强化学习管理器
    inline QDataStream& operator<<(QDataStream& out, mMotionRLManager& mgr) {
        out << static_cast<int>(mgr.m_dataMap.size());
        for (const auto& pair : mgr.m_dataMap) {
            out << pair.first;
            out << *pair.second;
        }
        return out;
    }

}

