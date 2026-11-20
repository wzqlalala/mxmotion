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
    // -----------------------------------------------------------------
    // 1. 底层结果数据读取
    // -----------------------------------------------------------------

    template<typename MapType>
    static void readMap(QDataStream& in, MapType& map) {
        int count = 0;
        in >> count;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            typename MapType::mapped_type value;
            in >> value;
            map[id] = value;
        }
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionMarkerFrame& frame) {
        in >> frame.m_markerId;
        in >> frame.m_worldPosition;
        in >> frame.m_worldRotation;
        in >> frame.m_linearVelocity;
        in >> frame.m_linearAcceleration;
        in >> frame.m_angularVelocity;
        in >> frame.m_angularAcceleration;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionBodyFrame& frame) {
        in >> frame.m_position;
        in >> frame.m_velocity;
        in >> frame.m_acceleration;
        in >> frame.m_rotation;
        in >> frame.m_angularVelocity;
        in >> frame.m_angularAcceleration;
        in >> frame.m_displacement;
        in >> frame.m_potentialEnergy;
        in >> frame.m_kineticEnergy;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointFrame& frame) {
        in >> frame.m_nodeI;
        in >> frame.m_nodeJ;
        in >> frame.m_reactionForce;
        in >> frame.m_reactionTorque;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionDriveFrame& frame) {
        in >> frame.m_absCoord;
        in >> frame.m_imposedValue;
        in >> frame.m_imposedSpeed;
        in >> frame.m_imposedAcc;
        in >> frame.m_isRotation;
        in >> frame.m_actuationForce;
        in >> frame.m_actuationTorque;
        in >> frame.m_reactionForce;
        in >> frame.m_reactionTorque;
        return in;
    }

    // 单帧数据
    inline QDataStream& operator>>(QDataStream& in, MechMotionData::mMotionFrame& obj)
    {
        in >> obj.m_time;
        readMap(in, obj.m_bodyTransforms);
        readMap(in, obj.m_markerFrames);
        readMap(in, obj.m_bodyFrames);
        readMap(in, obj.m_jointFrames);
        readMap(in, obj.m_couplingFrames);
        readMap(in, obj.m_driveFrames);
        return in;
    }

    // 动画结果集
    // 柔性体帧数据（刚柔耦合）
    inline QDataStream& operator>>(QDataStream& in, MechMotionData::mMotionFlexFrame& f)
    {
        in >> f.m_time >> f.m_modalQ >> f.m_interfacePose;
        return in;
    }

    // 柔性体结果数据（刚柔耦合）
    inline QDataStream& operator>>(QDataStream& in, MechMotionData::mMotionFlexBodyData& d)
    {
        in >> d.m_bodyId >> d.m_name >> d.m_nodeX0 >> d.m_tetIds >> d.m_surfaceTris
           >> d.m_psi >> d.m_nModes >> d.m_nInterfaces >> d.m_interfacePose0;
        qint32 n = 0;
        in >> n;
        d.m_frames.resize(n > 0 ? n : 0);
        for (qint32 i = 0; i < n; ++i)
            in >> d.m_frames[i];
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, MechMotionData::mMotionResultData& obj)
    {
        int frameCount = 0;
        in >> frameCount;
        obj.m_frames.resize(frameCount);
        for (int i = 0; i < frameCount; ++i) {
            in >> obj.m_frames[i];
        }
        // 柔性体数据（旧工程文件无此段：EOF 时读到 0，兼容）
        qint32 flexCount = 0;
        in >> flexCount;
        if (flexCount > 0) {
            obj.m_flexBodies.resize(flexCount);
            for (qint32 i = 0; i < flexCount; ++i)
                in >> obj.m_flexBodies[i];
        }
        return in;
    }

    // 标记点数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionMarker& marker) {
        in >> marker.m_id;
        in >> marker.m_name;
        in >> marker.m_localPosition;
        in >> marker.m_localRotation;
        in >> marker.m_color;
        in >> marker.m_size;
        in >> marker.m_visible;
        return in;
    }

    // 标记点管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionMarkerManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionMarker* marker = new mMotionMarker();
            in >> *marker;
            mgr.addData(marker);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 矢量力数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionVectorForce& force) {
        qint32 loadTypeVal, magModeVal, dirModeVal;
        in >> force.m_id >> force.m_name;
        in >> loadTypeVal;
        force.m_loadType = static_cast<VectorLoadType>(loadTypeVal);
        in >> force.m_actionPoint;
        in >> force.m_direction;
        in >> force.m_magnitude;
        in >> force.m_expression;
        in >> magModeVal;
        force.m_magnitudeMode = static_cast<MagnitudeMode>(magModeVal);
        in >> dirModeVal;
        force.m_directionMode = static_cast<DirectionMode>(dirModeVal);
        return in;
    }

    // 衬套数据类（基类通用段 + Bushing 扩展段）
    inline QDataStream& operator>>(QDataStream& in, mMotionBushing& bushing) {
        // 先读基类通用字段
        in >> static_cast<mMotionVectorForce&>(bushing);
        // 再读 Bushing 扩展字段（刚度/阻尼 + marker_J + 预载 + 偏置）
        in >> bushing.m_stiffness;
        in >> bushing.m_damping;
        in >> bushing.m_preloadForce;
        in >> bushing.m_preloadTorque;
        in >> bushing.m_applyOffset;
        in >> bushing.m_offset;
        return in;
    }

    // 力元管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionForceElementManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            // 先读通用段：typeTag + 公共字段
            qint32 typeTag = 0;
            in >> typeTag;
            if (typeTag == static_cast<qint32>(VectorLoadType::ScalarTorque)) {
                mMotionScalarTorque* torque = new mMotionScalarTorque();
                in >> torque->m_id >> torque->m_name;
                in >> torque->m_localI;
                in >> torque->m_localJ;
                qint32 axisModeVal;
                in >> axisModeVal;
                torque->m_axisMode = static_cast<ScalarTorqueAxisMode>(axisModeVal);
                in >> torque->m_fixedAxis;
                in >> torque->m_magnitude;
                in >> torque->m_torqueFunction;
                in >> torque->m_actionOnly;
                bool torqueUserRename = false;
                in >> torqueUserRename;
                torque->setName(torque->getName(), torqueUserRename);
                torque->setId(id);
                mgr.addData(torque);
                if (id > mgr.m_maxID) mgr.m_maxID = id;
            }
            else if (typeTag == static_cast<qint32>(VectorLoadType::TriTorque)) {
                mMotionThreePointTorque* torque = new mMotionThreePointTorque();
                in >> torque->m_id >> torque->m_name;
                in >> torque->m_localI;
                in >> torque->m_localTarget;
                in >> torque->m_magnitude;
                in >> torque->m_torqueFunction;
                bool torqueUserRename = false;
                in >> torqueUserRename;
                torque->setName(torque->getName(), torqueUserRename);
                torque->setId(id);
                mgr.addData(torque);
                if (id > mgr.m_maxID) mgr.m_maxID = id;
            }
            else if (typeTag == static_cast<qint32>(VectorLoadType::TriForce)) {
                mMotionThreePointForce* force = new mMotionThreePointForce();
                in >> force->m_id >> force->m_name;
                in >> force->m_localI;
                in >> force->m_localTarget;
                in >> force->m_magnitude;
                in >> force->m_forceFunction;
                bool forceUserRename = false;
                in >> forceUserRename;
                force->setName(force->getName(), forceUserRename);
                force->setId(id);
                mgr.addData(force);
                if (id > mgr.m_maxID) mgr.m_maxID = id;
            }
            else if (typeTag == static_cast<qint32>(VectorLoadType::ScalarForce) || typeTag == 1) {
                mMotionScalarForce* force = new mMotionScalarForce();
                in >> force->m_id >> force->m_name;
                in >> force->m_localI;
                in >> force->m_localJ;
                qint32 dirModeVal;
                in >> dirModeVal;
                force->m_directionMode = static_cast<ScalarForceDirectionMode>(dirModeVal);
                in >> force->m_fixedDirection;
                in >> force->m_magnitude;
                in >> force->m_forceFunction;
                in >> force->m_actionOnly;
                bool forceUserRename = false;
                in >> forceUserRename;
                force->setName(force->getName(), forceUserRename);
                force->setId(id);
                mgr.addData(force);
                if (id > mgr.m_maxID) mgr.m_maxID = id;
            } else {
                // 先读通用段以判定是否为衬套
                mMotionVectorForce* probe = new mMotionVectorForce();
                in >> probe->m_id >> probe->m_name;
                qint32 loadTypeVal;
                in >> loadTypeVal;
                probe->m_loadType = static_cast<VectorLoadType>(loadTypeVal);
                in >> probe->m_actionPoint;
                in >> probe->m_direction;
                in >> probe->m_magnitude;
                in >> probe->m_expression;
                qint32 magModeVal, dirModeVal;
                in >> magModeVal >> dirModeVal;
                probe->m_magnitudeMode = static_cast<MagnitudeMode>(magModeVal);
                probe->m_directionMode = static_cast<DirectionMode>(dirModeVal);
                bool probeUserRename = false;
                in >> probeUserRename;
                probe->setName(probe->getName(), probeUserRename);
                if (probe->getLoadType() == VectorLoadType::Bushing) {
                    mMotionBushing* bushing = new mMotionBushing();
                    bushing->setId(probe->getId());
                    bushing->setName(probe->getName(), probe->getIsUserRename());
                    bushing->setLoadType(probe->getLoadType());
                    bushing->setActionPoint(probe->getActionPoint());
                    bushing->setDirection(probe->getDirection());
                    bushing->setMagnitude(probe->getMagnitude());
                    bushing->setExpression(probe->getExpression());
                    bushing->setMagnitudeMode(probe->getMagnitudeMode());
                    bushing->setDirectionMode(probe->getDirectionMode());
                    delete probe;
                    // 补读 Bushing 扩展字段
                    QVector<double> stiffness, damping, preloadForce, preloadTorque;
                    bool applyOffset;
                    QVector3D offset;
                    in >> stiffness >> damping
                       >> preloadForce >> preloadTorque
                       >> applyOffset >> offset;
                    bushing->setStiffness(stiffness);
                    bushing->setDamping(damping);
                    bushing->setPreloadForce(preloadForce);
                    bushing->setPreloadTorque(preloadTorque);
                    bushing->setApplyOffset(applyOffset);
                    bushing->setOffset(offset);
                    mgr.addData(bushing);
                    if (id > mgr.m_maxID) mgr.m_maxID = id;
                } else {
                    mgr.addData(probe);
                    if (id > mgr.m_maxID) mgr.m_maxID = id;
                }
            }
        }
        return in;
    }

    // 3D 接触数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionContact& contact) {
        qint32 typeVal;
        in >> contact.m_id >> contact.m_name;
        in >> typeVal;
        contact.m_contactType = static_cast<MotionContactType>(typeVal);
        in >> contact.m_stiffness;
        in >> contact.m_forceExponent;
        in >> contact.m_materialDamping;
        in >> contact.m_penetrationDepth;
        in >> contact.m_frictionEnabled;
        in >> contact.m_staticFriction;
        in >> contact.m_dynamicFriction;
        in >> contact.m_forceModel;
        in >> contact.m_forceModelTolerance;
        in >> contact.m_useConvexHullAction >> contact.m_useConvexHullBase;
        in >> contact.m_actionUseFacetFile;
        in >> contact.m_actionDistanceTolerance;
        in >> contact.m_angleTolerance;
        in >> contact.m_actionUseMaxFacetSize;
        in >> contact.m_actionMaxFacetSize;
        in >> contact.m_baseUseFacetFile;
        in >> contact.m_baseDistanceTolerance;
        in >> contact.m_baseUseMaxFacetSize;
        in >> contact.m_baseMaxFacetSize;
        return in;
    }

    // 3D 接触管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionContactManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionContact* contact = new mMotionContact();
            in >> *contact;
            mgr.addData(contact);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 曲线数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionCurveData& obj)
    {
        in >> obj.m_id >> obj.m_name;
        in >> obj.m_xAxisLabel >> obj.m_yAxisLabel;
        in >> obj.m_xData >> obj.m_yData;
        return in;
    }

    // -----------------------------------------------------------------
    // 2. 运动体与求解方案 (无多态)
    // -----------------------------------------------------------------
    inline QDataStream& operator>>(QDataStream& in, mMotionBody& obj)
    {
        // 假设你使用 setter。如果是公有成员或友元，可直接 in >> obj._id;
        int id; QString name;
        bool isFixed; double mass; QVector3D cm; QVector<double> inertia;
        QVector3D initialVel;
        QVector3D initialOmega;

        in >> id >> name;
        in >> isFixed >> mass >> cm >> inertia;
        in >> initialVel;
        in >> initialOmega;

        obj.setId(id); obj.setName(name);
        obj.setIsFixed(isFixed); obj.setMass(mass);
        obj.setInitialVelocity(initialVel);
        obj.setInitialAngularVelocity(initialOmega);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionScenario& obj)
    {
        int id; QString name;
        double endTime, stepSize; QVector3D gravity;
       
        in >> id >> name;
        in >> endTime >> stepSize >> gravity;
        obj.setId(id); obj.setName(name);
        obj.setEndTime(endTime); obj.setStepSize(stepSize);
        obj.setGravity(gravity);

        bool usePreset; qint32 presetModeVal;
        qint32 integratorTypeVal;
        double integratorAlpha, integratorBeta, integratorTol;
        int integratorMaxIters;
        qint32 solverTypeVal;
        int solverMaxIters;
        double solverTolerance;
        bool solverVerbose;
        in >> usePreset >> presetModeVal;
        in  >> integratorTypeVal >> integratorAlpha >> integratorBeta >> integratorMaxIters >> integratorTol;
        in >> solverTypeVal >> solverMaxIters >> solverTolerance >> solverVerbose;
        obj.setUsePreset(usePreset);
        obj.setPresetMode(PresetMode(presetModeVal));
        obj.setIntegratorType(IntegratorType(integratorTypeVal));
        obj.setIntegratorAlpha(integratorAlpha);
        obj.setIntegratorBeta(integratorBeta);
        obj.setIntegratorMaxIters(integratorMaxIters);
        obj.setIntegratorTol(integratorTol);
        obj.setSolverType(SolverType(solverTypeVal));
        obj.setSolverMaxIters(solverMaxIters);
        obj.setSolverTolerance(solverTolerance);
        obj.setSolverVerbose(solverVerbose);

        // 读取内嵌曲线（一个方案一个曲线节点）
        obj.clearCurveNode();
        qint8 hasCurveNode;
        in >> hasCurveNode;
        if (hasCurveNode == 1) {
            mMotionCurveData* curve = new mMotionCurveData(0, "");
            in >> *curve;
            obj.setCurveNode(curve);
        }

        // 读取内嵌动画结果
        qint8 hasResult;
        in >> hasResult;
        if (hasResult == 1) {
            mMotionResultData* res = new mMotionResultData();
            in >> *res;
            obj.attachResult(res);
        }
        else {
            obj.attachResult(nullptr);
        }

        // 读取曲线请求（标志：0=无 1=旧单条（迁移为列表首项） 2=列表）
        qint8 curveReqFlag = 0;
        in >> curveReqFlag;
        obj.clearCurrentCurveRequest();
        obj.clearCurveRequests();
        if (curveReqFlag == 2) {
            qint32 reqCount = 0;
            in >> reqCount;
            for (qint32 i = 0; i < reqCount; ++i) {
                mMotionCurveRequest req;
                in >> req;
                qint8 hasColor = 0;
                in >> hasColor;
                if (hasColor == 1) {
                    qint32 rgba = 0;
                    in >> rgba;
                    req.m_color = QColor(QRgb(rgba));
                }
                in >> req.m_visible;
                obj.addCurveRequest(req);
            }
        }
        else if (curveReqFlag == 1) {
            mMotionCurveRequest req;
            in >> req;
            obj.setCurrentCurveRequest(req);
            obj.addCurveRequest(req);
        }
        return in;
    }

    // -----------------------------------------------------------------
    // 3. 运动副读取 (基类 + 派生类)
    // -----------------------------------------------------------------
    inline QDataStream& operator>>(QDataStream& in, mMotionJointFrictionData& data)
    {
        in >> data.enabled
            >> data.staticCoefficient
            >> data.kineticCoefficient
            >> data.viscousCoefficient
            >> data.effectiveRadius;

        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointLimitData& data)
    {
        in >> data.enabled
            >> data.lowerEnabled
            >> data.upperEnabled
            >> data.lowerValue
            >> data.upperValue
            >> data.penaltyOnly
            >> data.lowerCushion
            >> data.upperCushion
            >> data.lowerStiffness
            >> data.upperStiffness
            >> data.lowerDamping
            >> data.upperDamping;

        return in;
    }


    inline QDataStream& operator>>(QDataStream& in, mMotionJointBase& obj)
    {
        qint32 typeVal; int id; QString name;bool isUserName;mMotionJointFrictionData frictionData;mMotionJointLimitData limitData;
        in >> typeVal >> isUserName >> id >> name >> frictionData >> limitData;

        obj.setType(static_cast<JointType>(typeVal));
        obj.m_id = (id); 
        obj.setName(name, isUserName);
        obj.setFrictionData(frictionData);
        obj.setLimitData(limitData);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointBasic& obj)
    {
        // 1. 先读基类属性
        in >> static_cast<mMotionJointBase&>(obj);

        // 2. 读自己独有属性
        QVector3D origin, axis;
        in >> origin >> axis;
        obj.setOrigin(origin); obj.setAxis(axis);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointUniversal& obj)
    {
        in >> static_cast<mMotionJointBase&>(obj);

        QVector3D origin, actionSpin, actionPin, baseSpin, basePin;
        in >> origin >> actionSpin >> actionPin >> baseSpin >> basePin;
        obj.setOrigin(origin);
        obj.setActionBodySpinVector(actionSpin);
        obj.setActionBodyPinVector(actionPin);
        obj.setBaseBodySpinVector(baseSpin);
        obj.setBaseBodyPinVector(basePin);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointPointOnSurface& obj)
    {
        in >> static_cast<mMotionJointBasic&>(obj);

        QVector<int> faceIds;
        in >> faceIds;
        obj.setSurfaceFaceIds(faceIds);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointPointOnCurve& obj)
    {
        in >> static_cast<mMotionJointBasic&>(obj);

        QVector<int> edgeIds;
        in >> edgeIds;
        obj.setCurveEdgeIds(edgeIds);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointCurveOnCurve& obj)
    {
        in >> static_cast<mMotionJointBasic&>(obj);

        QVector<int> edgeIdsI, edgeIdsJ;
        in >> edgeIdsI;
        in >> edgeIdsJ;
        obj.setCurveIEdgeIds(edgeIdsI);
        obj.setCurveJEdgeIds(edgeIdsJ);
        // 旧文件没有 allowSliding 字段（追加在末尾），读取时判流状态避免越界
        bool allowSliding = false;
        if (!in.atEnd())
            in >> allowSliding;
        obj.setAllowSliding(allowSliding);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointScrew& obj)
    {
        in >> static_cast<mMotionJointBase&>(obj);

        QVector3D origin, axis;
        double lead;
        in >> origin >> axis >> lead;
        obj.setOrigin(origin);
        obj.setAxis(axis);
        obj.setLead(lead);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointPerpendicular& obj)
    {
        in >> static_cast<mMotionJointBase&>(obj);

        QVector3D origin, actionVec, baseVec;
        in >> origin >> actionVec >> baseVec;
        obj.setOrigin(origin);
        obj.setActionBodyVector(actionVec);
        obj.setBaseBodyVector(baseVec);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointConstantVelocity& obj)
    {
        in >> static_cast<mMotionJointBase&>(obj);

        QVector3D origin, actionVec, baseVec;
        in >> origin >> actionVec >> baseVec;
        obj.setOrigin(origin);
        obj.setActionBodyVector(actionVec);
        obj.setBaseBodyVector(baseVec);
        return in;
    }

    // -----------------------------------------------------------------
    // 4. 驱动读取 (基类 + 派生类)
    // -----------------------------------------------------------------
    // ==========================================
    // 基类：读取 (operator>>)
    // ==========================================
    inline QDataStream& operator>>(QDataStream& in, mMotionDriveBase& obj)
    {
        int id; QString name;
        qint32 tType, mType, cMode;
        QString expr;

        // 严格按照保存的顺序读取
        in >> id >> name >> tType >> mType >> cMode >> expr;

        // 赋值 (视你的封装情况使用直接赋值或 Setter)
        obj.m_id = id;
        obj.m_name = name; // 如果你用 obj.setName(name) 也可以
        obj.m_targetType = static_cast<DriveTargetType>(tType);
        obj.m_motionType = static_cast<DriveMotionType>(mType);
        obj.m_controlMode = static_cast<DriveControlMode>(cMode);
        obj.m_expression = expr;

        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionJointDrive& obj)
    {
        // 1. 读基类 (这行代码会自动跳到上面的基类 operator>> 读取 6 个属性)
        in >> static_cast<mMotionDriveBase&>(obj);

        // 2. 读自己独有的属性（目标运动副引用已迁移至映射关系存储）
        return in;
    }

    // -----------------------------------------------------------------
    // 5. 管理器读取 (无多态：Body / Scenario)
    // -----------------------------------------------------------------
    inline QDataStream& operator>>(QDataStream& in, mMotionBodyManager& mgr)
    {
        mgr.deleteAll(); // 极其重要：读取新工程前必须清空旧数据！

        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = (maxId); // 假设你有类似 setMaxID() 的方法恢复内部计数器

        for (int i = 0; i < count; ++i) {
            mMotionBody* newBody = new mMotionBody();
            in >> *newBody;
            mgr.addData(newBody);
        }
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionScenarioManager& mgr)
    {
        mgr.deleteAll();

        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = (maxId);

        for (int i = 0; i < count; ++i) {
            mMotionScenario* newScenario = new mMotionScenario();
            in >> *newScenario;
            mgr.addData(newScenario);
        }
        return in;
    }

    // -----------------------------------------------------------------
    // 6. 管理器读取 (带多态：Joint / Drive)
    // -----------------------------------------------------------------

    inline QDataStream& operator>>(QDataStream& in, mMotionJointManager& mgr)
    {
        mgr.deleteAll();
        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = (maxId);
        for (int i = 0; i < count; ++i) {
            qint32 typeVal;
            in >> typeVal; // 读出写入时的类型枚举标记！
            JointType type = static_cast<JointType>(typeVal);
            mMotionJointBase* newJoint = nullptr;
            // 【核心】：根据标记创建正确的派生类
            switch (type) {
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
                // 这些类型在底层都是 mMotionJointBasic 类
                newJoint = new mMotionJointBasic();
                in >> *dynamic_cast<mMotionJointBasic*>(newJoint); // 将流数据灌入派生类
                break;
            case JointType::PointOnSurface:
                newJoint = new mMotionJointPointOnSurface();
                in >> *dynamic_cast<mMotionJointPointOnSurface*>(newJoint);
                break;
            case JointType::PointOnCurve:
                newJoint = new mMotionJointPointOnCurve();
                in >> *dynamic_cast<mMotionJointPointOnCurve*>(newJoint);
                break;
            case JointType::CurveOnCurve:
                newJoint = new mMotionJointCurveOnCurve();
                in >> *dynamic_cast<mMotionJointCurveOnCurve*>(newJoint);
                break;
            case JointType::Screw:
                newJoint = new mMotionJointScrew();
                in >> *dynamic_cast<mMotionJointScrew*>(newJoint);
                break;
            case JointType::Universal:
                newJoint = new mMotionJointUniversal();
                in >> *dynamic_cast<mMotionJointUniversal*>(newJoint);
                break;
            case JointType::Perpendicular:
                newJoint = new mMotionJointPerpendicular();
                in >> *dynamic_cast<mMotionJointPerpendicular*>(newJoint);
                break;
            case JointType::ConstantVelocity:
                newJoint = new mMotionJointConstantVelocity();
                in >> *dynamic_cast<mMotionJointConstantVelocity*>(newJoint);
                break;
            default:
                break;
            }
            if (newJoint) {
                mgr.addData(newJoint);
            }
        }
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionDriveManager& mgr)
    {
        mgr.deleteAll();
        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = (maxId);
        for (int i = 0; i < count; ++i) {
            qint32 typeVal;
            in >> typeVal; // 之前写入的标记，比如 1 代表 JointDrive
            mMotionDriveBase* newDrive = nullptr;
            if (typeVal == 1) {
                newDrive = new mMotionJointDrive();
                in >> *dynamic_cast<mMotionJointDrive*>(newDrive);
            }
            if (newDrive) {
                mgr.addData(newDrive);
            }
        }
        return in;
    }

    // -----------------------------------------------------------------
    // 7. 耦合副读取 (基类 + 派生类)
    // -----------------------------------------------------------------
    inline QDataStream& operator>>(QDataStream& in, mMotionCouplingBase& obj)
    {
        qint32 typeVal; int id; QString name; bool isUserName;
        in >> typeVal >> isUserName >> id >> name;

        obj.setType(static_cast<CouplingType>(typeVal));
        obj.m_id = (id);
        obj.setName(name, isUserName);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionCouplingGearRack& obj)
    {
        in >> static_cast<mMotionCouplingBase&>(obj);

        double radius;
        in >> radius;
        obj.setRadius(radius);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionCouplingGear& obj)
    {
        in >> static_cast<mMotionCouplingBase&>(obj);

        in >> obj.m_gearRatio;
        in >> obj.m_toothCount1;
        in >> obj.m_toothCount2;
        in >> obj.m_pitchRadius1;
        in >> obj.m_pitchRadius2;
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionCouplingCable& obj)
    {
        in >> static_cast<mMotionCouplingBase&>(obj);

        double ratio;
        bool reverse;
        in >> ratio >> reverse;
        obj.setRatio(ratio);
        obj.setReverse(reverse);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionCouplingManager& mgr)
    {
        mgr.deleteAll();
        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = (maxId);
        for (int i = 0; i < count; ++i) {
            qint32 typeVal;
            in >> typeVal;
            CouplingType type = static_cast<CouplingType>(typeVal);
            mMotionCouplingBase* newCoupling = nullptr;
            switch (type) {
            case CouplingType::GearRack:
                newCoupling = new mMotionCouplingGearRack();
                in >> *dynamic_cast<mMotionCouplingGearRack*>(newCoupling);
                break;
            case CouplingType::Gear:
                newCoupling = new mMotionCouplingGear();
                in >> *dynamic_cast<mMotionCouplingGear*>(newCoupling);
                break;
            case CouplingType::Cable:
                newCoupling = new mMotionCouplingCable();
                in >> *dynamic_cast<mMotionCouplingCable*>(newCoupling);
                break;
            default:
                break;
            }
            if (newCoupling) {
                mgr.addData(newCoupling);
            }
        }
        return in;
    }

    // 弹簧基类
    inline QDataStream& operator>>(QDataStream& in, mMotionSpringBase& obj) {
        qint32 typeVal, springDirVal, dampingDirVal;
        in >> obj.m_id >> obj.m_name >> typeVal;
        obj.m_type = static_cast<SpringType>(typeVal);
        in >> obj.m_stiffness >> obj.m_damping >> obj.m_preload;
        in >> springDirVal >> dampingDirVal;
        obj.m_springDirectionMode = static_cast<SpringDirectionMode>(springDirVal);
        obj.m_dampingDirectionMode = static_cast<SpringDirectionMode>(dampingDirVal);
        in >> obj.m_freeValue >> obj.m_installValue;
        in >> obj.m_tableX >> obj.m_tableY >> obj.m_hasTable;
        in >> obj.m_forceFunction >> obj.m_dampingFunction;
        if (!in.atEnd())
            in >> obj.m_dampingEnabled;
        else
            obj.m_dampingEnabled = false;
        return in;
    }

    // 拉压弹簧
    inline QDataStream& operator>>(QDataStream& in, mMotionTranslationalSpring& obj) {
        in >> static_cast<mMotionSpringBase&>(obj);
        return in;
    }

    // 扭转弹簧
    inline QDataStream& operator>>(QDataStream& in, mMotionTorsionalSpring& obj) {
        in >> static_cast<mMotionSpringBase&>(obj);
        return in;
    }

    inline QDataStream& operator>>(QDataStream& in, mMotionBodyPointSpring& obj) {
        in >> static_cast<mMotionSpringBase&>(obj);
        in >> obj.m_localI >> obj.m_localJ;
        in >> obj.m_localRotI >> obj.m_localRotJ;
        return in;
    }

    // 弹簧管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionSpringManager& mgr) {
        mgr.deleteAll();
        int maxId, count;
        in >> maxId >> count;
        mgr.m_maxID = maxId;
        for (int i = 0; i < count; ++i) {
            qint32 typeVal;
            in >> typeVal;
            SpringType type = static_cast<SpringType>(typeVal);
            mMotionSpringBase* newSpring = nullptr;
            switch (type) {
            case SpringType::Translational:
                newSpring = new mMotionTranslationalSpring();
                in >> *dynamic_cast<mMotionTranslationalSpring*>(newSpring);
                break;
            case SpringType::Torsional:
                newSpring = new mMotionTorsionalSpring();
                in >> *dynamic_cast<mMotionTorsionalSpring*>(newSpring);
                break;
            case SpringType::BodyPoint:
                newSpring = new mMotionBodyPointSpring();
                in >> *dynamic_cast<mMotionBodyPointSpring*>(newSpring);
                break;
            default:
                break;
            }
            if (newSpring) {
                mgr.addData(newSpring);
            }
        }
        return in;
    }

    // 控制输入数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionControlInput& obj) {
        qint32 typeVal;
        in >> obj.m_id >> obj.m_name;
        in >> typeVal;
        obj.m_inputType = static_cast<ControlInputType>(typeVal);
        in >> obj.m_timeStart >> obj.m_timeEnd;
        in >> obj.m_polyCoeffs;
        int cpCount = 0;
        in >> cpCount;
        obj.m_curvePoints.clear();
        for (int i = 0; i < cpCount; ++i) {
            QPair<double, double> p;
            in >> p.first >> p.second;
            obj.m_curvePoints.append(p);
        }
        return in;
    }

    // 控制输入管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionControlInputManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionControlInput* obj = new mMotionControlInput();
            in >> *obj;
            mgr.addData(obj);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 机电导入数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionElectroImport& obj) {
        qint32 typeVal;
        in >> obj.m_id >> obj.m_name;
        in >> typeVal;
        obj.m_coSimType = static_cast<CoSimType>(typeVal);
        in >> obj.m_platform >> obj.m_modelFile >> obj.m_headerFile;
        int ipCount = 0, opCount = 0;
        in >> ipCount;
        obj.m_inputPorts.clear();
        for (int i = 0; i < ipCount; ++i) {
            QPair<QString, QString> p;
            in >> p.first >> p.second;
            obj.m_inputPorts.append(p);
        }
        in >> opCount;
        obj.m_outputPorts.clear();
        for (int i = 0; i < opCount; ++i) {
            QPair<QString, QString> p;
            in >> p.first >> p.second;
            obj.m_outputPorts.append(p);
        }
        return in;
    }

    // 机电导入管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionElectroImportManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionElectroImport* obj = new mMotionElectroImport();
            in >> *obj;
            mgr.addData(obj);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 机电导出数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionElectroExport& obj) {
        qint32 typeVal;
        in >> obj.m_id >> obj.m_name;
        in >> typeVal;
        obj.m_exportType = static_cast<ExportType>(typeVal);
        in >> obj.m_portListFile;
        int ipCount = 0, opCount = 0;
        in >> ipCount;
        obj.m_inputPorts.clear();
        for (int i = 0; i < ipCount; ++i) {
            mMotionElectroExport::PortConfig p;
            in >> p.portName >> p.order >> p.scale >> p.offset >> p.unit;
            obj.m_inputPorts.append(p);
        }
        in >> opCount;
        obj.m_outputPorts.clear();
        for (int i = 0; i < opCount; ++i) {
            mMotionElectroExport::PortConfig p;
            in >> p.portName >> p.order >> p.scale >> p.offset >> p.unit;
            obj.m_outputPorts.append(p);
        }
        return in;
    }

    // 机电导出管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionElectroExportManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionElectroExport* obj = new mMotionElectroExport();
            in >> *obj;
            mgr.addData(obj);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 液压数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionHydraulic& obj) {
        qint32 typeVal;
        in >> obj.m_id >> obj.m_name;
        in >> typeVal;
        obj.m_hydraulicType = static_cast<HydraulicType>(typeVal);
        in >> obj.m_modelFile;
        in >> obj.m_fluidDensity >> obj.m_fluidViscosity >> obj.m_bulkModulus;
        int portCount = 0, pipeCount = 0;
        in >> portCount;
        obj.m_ports.clear();
        for (int i = 0; i < portCount; ++i) {
            mMotionHydraulic::PortInfo p;
            in >> p.portName >> p.diameter >> p.length;
            obj.m_ports.append(p);
        }
        in >> pipeCount;
        obj.m_pipes.clear();
        for (int i = 0; i < pipeCount; ++i) {
            mMotionHydraulic::PipeInfo p;
            in >> p.pipeName >> p.diameter >> p.length;
            obj.m_pipes.append(p);
        }
        return in;
    }

    // 液压管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionHydraulicManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionHydraulic* obj = new mMotionHydraulic();
            in >> *obj;
            mgr.addData(obj);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

    // 强化学习数据类
    inline QDataStream& operator>>(QDataStream& in, mMotionRL& obj) {
        qint32 algoVal, envVal;
        in >> obj.m_id >> obj.m_name;
        in >> algoVal;
        obj.m_algorithm = static_cast<RLAlgorithm>(algoVal);
        in >> envVal;
        obj.m_envType = static_cast<RLEnvType>(envVal);
        in >> obj.m_stateSpace;
        in >> obj.m_actionSpace;
        in >> obj.m_hiddenLayers;
        in >> obj.m_learningRate >> obj.m_discountFactor;
        in >> obj.m_maxEpisodes >> obj.m_maxSteps;
        in >> obj.m_datasetPath >> obj.m_modelSavePath;
        return in;
    }

    // 强化学习管理器
    inline QDataStream& operator>>(QDataStream& in, mMotionRLManager& mgr) {
        int count = 0;
        in >> count;
        mgr.m_maxID = 0;
        for (int i = 0; i < count; ++i) {
            int id;
            in >> id;
            mMotionRL* obj = new mMotionRL();
            in >> *obj;
            mgr.addData(obj);
            if (id > mgr.m_maxID) mgr.m_maxID = id;
        }
        return in;
    }

}
