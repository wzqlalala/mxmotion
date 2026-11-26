#include "NativeMotionActuators.h"
#include "NativeMotionRuntime.h"

#include "motion/functions/Function_Expr.h"
#include "motion/functions/Function_Table.h"
#include "motion/physics/HydraulicCylinder.h"
#include "motion/physics/LinkMotorLinearForce.h"
#include "motion/physics/LinkMotorRotationTorque.h"
#include "motion/physics/FlexBodyAuxRef.h"

#include <Eigen/QR>
#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace MotionOcc {
namespace {
constexpr double degrees=180.0/3.14159265358979323846;
[[noreturn]] void fail(const QString& message){throw std::runtime_error(message.toStdString());}

motion::MotionVector<> jsonVec(const QJsonValue& value, motion::MotionVector<> fallback = motion::VNULL)
{
    if (value.isUndefined() || value.isNull()) return fallback;
    motion::MotionVector<> out;
    if (value.isArray()) {
        const auto a = value.toArray();
        if (a.size() != 3) fail(QStringLiteral("液压缸坐标向量需要三个分量。"));
        out = motion::MotionVector<>(a[0].toDouble(NAN), a[1].toDouble(NAN), a[2].toDouble(NAN));
    } else {
        const auto o = value.toObject();
        out = motion::MotionVector<>(o.value("x").toDouble(NAN), o.value("y").toDouble(NAN), o.value("z").toDouble(NAN));
    }
    if (!std::isfinite(out.x()) || !std::isfinite(out.y()) || !std::isfinite(out.z()))
        fail(QStringLiteral("液压缸坐标向量包含非有限数值。"));
    return out;
}

std::shared_ptr<motion::MotionBody> findSystemBody(NativeMotionSystem& system, int bodyId)
{
    const std::string key = bodyId < 0 ? std::string("ground") : std::to_string(bodyId);
    auto body = system.SearchBody(key.c_str());
    if (!body) body = std::dynamic_pointer_cast<motion::MotionBody>(system.SearchFlex(key.c_str()));
    return body;
}

class EffortFunction final : public motion::MotionFunction {
public:
    explicit EffortFunction(std::shared_ptr<NativeEffortRuntime> runtime):runtime(std::move(runtime)){}
    EffortFunction* Clone() const override{return new EffortFunction(*this);}
    FunctionType Get_Type() const override{return FUNCT_CUSTOM;}
    double Get_y(double time) const override{
        if(!runtime->command)fail("执行器没有力/力矩命令。");
        return 1000*limitNativeEffort(*runtime,runtime->command->Get_y(time));
    }
private:
    std::shared_ptr<NativeEffortRuntime> runtime;
};

motion::MotionFrame<> linearFrame(const motion::MotionFrame<>& frame)
{
    // Legacy CAD joints use local Z; a linear motor acts on local X. Keep a
    // right-handed frame with X=old Z, Y=old X and Z=old Y.
    const motion::MotionMatrix33<> rotation(frame.GetRot());
    motion::MotionMatrix33<> axes;
    axes.Set_A_axis(rotation.Get_A_Zaxis(),rotation.Get_A_Xaxis(),rotation.Get_A_Yaxis());
    return motion::MotionFrame<>(frame.GetPos(),axes.Get_A_quaternion());
}
}

double nativeEffortSpeed(const NativeEffortRuntime& runtime)
{
    const auto motor=runtime.motor.lock();
    if(!motor)fail("力/力矩执行器已失效。");
    if(runtime.angular){
        const auto rotary=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(motor);
        if(!rotary)fail("力矩执行器类型不匹配。");
        return rotary->GetMotorRot_dt()*degrees;
    }
    const auto linear=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(motor);
    if(!linear)fail("力执行器类型不匹配。");
    return linear->GetMotorPos_dt();
}

double limitNativeEffort(const NativeEffortRuntime& runtime,double requested)
{
    if(!std::isfinite(requested))fail("执行器命令为非有限数值。");
    const double speed=nativeEffortSpeed(runtime);
    if(!std::isfinite(speed))fail("执行器速度为非有限数值。");
    const double effort=runtime.definition.value("effortLimit").toDouble();
    const double power=runtime.definition.value("powerLimit").toDouble();
    const double noLoad=runtime.definition.value("noLoadSpeed").toDouble();
    double bound=effort>0?effort:std::numeric_limits<double>::infinity();
    // A linear torque-speed envelope limits motoring. At overspeed braking is
    // still possible; clipping both directions to zero would remove braking.
    if(noLoad>0&&requested*speed>0)bound=std::min(bound,effort*std::max(0.0,1-std::abs(speed)/noLoad));
    const double workRate=std::abs(speed)*(runtime.angular?1/degrees:1);
    if(power>0&&workRate>1e-15)bound=std::min(bound,power*1000/workRate);
    return std::clamp(requested,-bound,bound);
}

void createNativeEffortMotors(NativeMotionSystem& system,const NativeMotionInput& input)
{
    for(const auto& value:input.document.collection("drives")){
        const auto definition=value.toObject();
        const auto mode=definition.value("controlMode").toString();
        if(mode!="Force"&&mode!="Torque")continue;
        auto runtime=std::make_shared<NativeEffortRuntime>();
        runtime->driveId=definition.value("id").toInt();runtime->definition=definition;runtime->angular=mode=="Torque";
        if(runtime->angular!=(definition.value("motionType").toString()=="Rotation"))fail("力驱动仅用于平移；力矩驱动仅用于旋转。");
        const auto joint=system.joints_map.find(std::to_string(definition.value("targetJointId").toInt(-1)));
        if(joint==system.joints_map.end())fail("执行器目标副不存在。");
        const auto markerI=system.SearchMarker(joint->second->marker_I.c_str());
        const auto markerJ=system.SearchMarker(joint->second->marker_J.c_str());
        if(!markerI||!markerJ)fail("执行器目标副 Marker 不存在。");
        const auto bodyI=system.SearchBody(markerI->GetBody()->GetName());
        const auto bodyJ=system.SearchBody(markerJ->GetBody()->GetName());
        if(!bodyI||!bodyJ)fail("执行器端点运动体不存在。");
        std::vector<double> times,values;
        for(const auto& v:definition.value("driveTableX").toArray())times.push_back(v.toDouble());
        for(const auto& v:definition.value("driveTableY").toArray())values.push_back(v.toDouble());
        if(!times.empty()||!values.empty()){
            auto function=std::make_shared<motion::MotionFunction_Table>(times,values);
            if(!function->IsValid())fail("力/力矩驱动的表格无效。");
            runtime->command=function;
        }else{
            auto function=std::make_shared<motion::MotionFunction_Expr>(definition.value("expression").toString("0").toStdString());
            if(!function->IsValid())fail("力/力矩驱动表达式无效："+QString::fromStdString(function->GetCompileError()));
            runtime->command=function;
        }
        std::shared_ptr<motion::MotionLinkMotor> motor;
        if(runtime->angular){
            auto rotary=std::make_shared<motion::MotionLinkMotorRotationTorque>();
            rotary->SetSpindleConstraint(motion::MotionLinkMotorRotation::SpindleConstraint::FREE);
            rotary->Initialize(bodyI,bodyJ,false,motion::MotionFrame<>(markerI->GetAbsCoord()),motion::MotionFrame<>(markerJ->GetAbsCoord()));
            motor=rotary;
        }else{
            auto linear=std::make_shared<motion::MotionLinkMotorLinearForce>();
            linear->SetGuideConstraint(motion::MotionLinkMotorLinear::GuideConstraint::FREE);
            linear->Initialize(bodyI,bodyJ,false,linearFrame(motion::MotionFrame<>(markerI->GetAbsCoord())),linearFrame(motion::MotionFrame<>(markerJ->GetAbsCoord())));
            motor=linear;
        }
        motor->SetNameString(std::to_string(runtime->driveId));runtime->motor=motor;
        motor->SetMotorFunction(std::make_shared<EffortFunction>(runtime));
        system.AddLink(motor);system.effortMotors.emplace(runtime->driveId,runtime);
        // Keep marker mapping and rotational units available to all existing
        // result and structural-load export paths. This record is deliberately
        // added after the legacy parser: it is an effort, not a motion law.
        motion::adams_motion_struct record;
        record.id=std::to_string(runtime->driveId);record.marker_I=joint->second->marker_I;record.marker_J=joint->second->marker_J;
        record.resolved_joint_id=joint->first;record.is_rotation=runtime->angular;record.is_translation=!runtime->angular;
        system.motions_map.emplace(record.id,record);
    }
}

void recordNativeEffortMotors(NativeMotionSystem& system)
{
    const double time=system.GetMotionTime();
    for(auto& entry:system.effortMotors){
        auto& runtime=*entry.second;
        if(!runtime.history.empty()&&runtime.history.back().time>=time)continue;
        const auto motor=runtime.motor.lock();if(!motor)fail("记录执行器结果时电机不存在。");
        NativeEffortSample sample;sample.time=time;sample.requested=runtime.command->Get_y(time);
        sample.applied=limitNativeEffort(runtime,sample.requested);sample.speed=nativeEffortSpeed(runtime);
        sample.power=sample.applied*sample.speed*(runtime.angular?1/degrees:1)*.001;
        sample.limited=std::abs(sample.applied-sample.requested)>1e-12*std::max(1.0,std::abs(sample.requested));
        runtime.history.push_back(sample);
        auto& history=system.motion_results[std::to_string(runtime.driveId)];
        if(history.data.empty()||std::abs(history.data.back().time-time)>1e-10)fail("执行器结果时间轴未与系统结果同步。");
        auto& value=history.data.back();
        const auto frame=motor->GetFrame2()>>*motor->GetBody2();
        const auto effort=frame.GetRot().Rotate(runtime.angular?motion::MotionVector<>(0,0,sample.applied*1000):motion::MotionVector<>(sample.applied*1000,0,0));
        // Existing reaction channels describe the wrench on J. Applied effort
        // is equal/opposite on I; keep the established endpoint contract.
        if(runtime.angular){value.actuation_torque={-effort.x(),-effort.y(),-effort.z()};
            const auto rotary=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(motor);
            value.imposed_value=rotary->GetMotorRot();value.imposed_speed=rotary->GetMotorRot_dt();value.imposed_acc=rotary->GetMotorRot_dtdt();}
        else{value.actuation_force={-effort.x(),-effort.y(),-effort.z()};
            const auto linear=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(motor);
            value.imposed_value=linear->GetMotorPos();value.imposed_speed=linear->GetMotorPos_dt();value.imposed_acc=linear->GetMotorPos_dtdt();}
    }
}

void createNativeHydraulicCylinders(NativeMotionSystem& system, const NativeMotionInput& input)
{
    const auto fluid = input.document.root.value("motionData").toObject().value("hydraulicFluid").toObject();
    const double bulkModulus = fluid.value("bulkModulus").toDouble(1400.0);
    const double density = fluid.value("density").toDouble(860.0);
    const double supplyPressure = fluid.value("supplyPressure").toDouble(21.0);
    const double tankPressure = fluid.value("tankPressure").toDouble(0.1);

    for (const auto& value : input.document.collection("hydraulicCylinders")) {
        const auto row = value.toObject();
        if (!row.value("enabled").toBool(true)) continue;
        const int id = row.value("id").toInt();
        const double stroke = row.value("stroke").toDouble(400.0);
        const double initialStroke = row.value("initialStroke").toDouble(200.0);

        std::shared_ptr<motion::MotionBody> bodyI, bodyJ;
        motion::MotionVector<> worldI, worldJ;
        if (row.value("mountMode").toString("Points") == QLatin1String("Joint")) {
            const int jointId = row.value("targetJointId").toInt(-1);
            const auto jointRow = input.document.entity("joints", jointId);
            if (jointRow.isEmpty()) fail(QStringLiteral("液压缸 %1 关联的运动副不存在。").arg(id));
            const auto jType = jointRow.value("jointType").toString();
            if (jType != QLatin1String("Translational") && jType != QLatin1String("Cylindrical"))
                fail(QStringLiteral("液压缸 %1 仅支持关联移动副或柱面副。").arg(id));
            bodyI = findSystemBody(system, jointRow.value("actionBodyId").toInt(-1));
            bodyJ = findSystemBody(system, jointRow.value("baseBodyId").toInt(-1));
            const auto origin = jsonVec(jointRow.value("origin"));
            auto axis = jsonVec(jointRow.value("axis"), motion::VECT_Z);
            if (axis.Length() < 1e-12) fail(QStringLiteral("液压缸 %1 关联运动副轴向为零。").arg(id));
            axis.Normalize();
            const double baseLen = std::max(50.0, stroke);
            worldJ = origin;
            worldI = origin + axis * (baseLen + initialStroke);
        } else {
            bodyI = findSystemBody(system, row.value("bodyIId").toInt(-1));
            bodyJ = findSystemBody(system, row.value("bodyJId").toInt(-1));
            worldI = jsonVec(row.value("localI"));
            worldJ = jsonVec(row.value("localJ"));
        }
        if (!bodyI || !bodyJ || bodyI == bodyJ)
            fail(QStringLiteral("液压缸 %1 端点运动体不存在或连接到同一运动体。").arg(id));
        if ((worldI - worldJ).Length() <= 1e-6)
            fail(QStringLiteral("液压缸 %1 初始两端点距离为零。").arg(id));

        auto cyl = std::make_shared<motion::MotionHydraulicCylinder>();
        cyl->SetNameString("HC_" + std::to_string(id));
        cyl->SetUnitScales(1.0, 1000.0);
        cyl->SetGeometry(row.value("pistonDiameter").toDouble(80.0),
                         row.value("rodDiameter").toDouble(45.0),
                         stroke, initialStroke);
        cyl->SetDeadVolumesMm3(row.value("deadVolumeA").toDouble(50000.0),
                               row.value("deadVolumeB").toDouble(50000.0));
        cyl->SetFluidProperties(bulkModulus, density);
        cyl->SetViscousFriction(row.value("viscousFriction").toDouble(1.0));
        cyl->SetInternalLeakage(row.value("internalLeakage").toDouble(0.0));
        cyl->SetSupplyPressures(supplyPressure, tankPressure);
        cyl->SetValveRating(row.value("ratedFlowLpm").toDouble(40.0),
                            row.value("ratedPressureDrop").toDouble(3.5));

        const QString supplyMode = row.value("supplyMode").toString("DirectionalValve");
        if (supplyMode == QLatin1String("ClosedPort"))
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::ClosedPort);
        else if (supplyMode == QLatin1String("FlowCommand"))
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::FlowCommand);
        else
            cyl->SetSupplyMode(motion::MotionHydraulicCylinder::SupplyMode::DirectionalValve);

        cyl->SetInitialPressures(row.value("initialPressureA").toDouble(1.0),
                                 row.value("initialPressureB").toDouble(1.0));

        std::vector<double> tx, ty;
        for (const auto& v : row.value("commandTableX").toArray()) tx.push_back(v.toDouble());
        for (const auto& v : row.value("commandTableY").toArray()) ty.push_back(v.toDouble());
        if (!tx.empty() || !ty.empty()) {
            auto table = std::make_shared<motion::MotionFunction_Table>(tx, ty);
            if (!table->IsValid()) fail(QStringLiteral("液压缸 %1 控制时间表无效。").arg(id));
            cyl->SetCommandFunction(table);
        } else {
            const QString expr = row.value("commandExpression").toString("0").trimmed();
            auto fn = std::make_shared<motion::MotionFunction_Expr>((expr.isEmpty() ? QStringLiteral("0") : expr).toStdString());
            if (!fn->IsValid())
                fail(QStringLiteral("液压缸 %1 控制表达式无效：%2").arg(id).arg(QString::fromStdString(fn->GetCompileError())));
            cyl->SetCommandFunction(fn);
        }

        cyl->Initialize(bodyI, bodyJ, false, worldI, worldJ);
        system.AddLink(cyl);

        auto runtime = std::make_shared<NativeHydraulicRuntime>();
        runtime->cylinderId = id;
        runtime->definition = row;
        runtime->cylinder = cyl;
        system.hydraulicCylinders.emplace(id, std::move(runtime));
    }
}

QString initializeSteadyNativeHydraulics(NativeMotionSystem& system, const QJsonObject& scenario)
{
    if (!scenario.value("hydraulicSteadyInit").toBool(false) || system.hydraulicCylinders.empty())
        return {};

    std::vector<NativeHydraulicRuntime*> runtimes;
    runtimes.reserve(system.hydraulicCylinders.size());
    for (auto& entry : system.hydraulicCylinders) {
        runtimes.push_back(entry.second.get());
        entry.second->cylinder->SetDisabled(true);
    }

    system.Setup();
    system.Update(0.0, false);
    system.prepareStaticDescriptor();
    system.ConstraintsLoadJacobians();

    const int n = system.GetNcoords_v();
    const int m = system.GetNconstr();
    const int k = static_cast<int>(runtimes.size());

    if (n > 0) {
        motion::MotionVectorDynamic<> force(n);
        force.setZero();
        system.LoadResidual_F(force, 1.0);

        Eigen::MatrixXd A = Eigen::MatrixXd::Zero(n, m + k);
        if (m > 0) {
            motion::MotionSparseMatrix cq;
            system.GetConstraintJacobianMatrix(&cq);
            if (cq.rows() == m && cq.cols() == n) {
                A.leftCols(m) = Eigen::MatrixXd(cq.transpose());
            }
        }
        for (int r = 0; r < k; ++r) {
            auto& cyl = *runtimes[r]->cylinder;
            auto* b1 = static_cast<motion::MotionBody*>(cyl.GetBody1());
            auto* b2 = static_cast<motion::MotionBody*>(cyl.GetBody2());
            const auto p1 = b1->TransformPointLocalToParent(cyl.GetPoint1Rel());
            const auto p2 = b2->TransformPointLocalToParent(cyl.GetPoint2Rel());
            const auto dir = (p1 - p2).GetNormalized();
            if (b1->Variables().IsActive()) {
                const int off1 = static_cast<int>(b1->GetOffset_w());
                const auto lt1 = b1->TransformDirectionParentToLocal(motion::Vcross(p1 - b1->GetCoord().pos, dir));
                A.block<3, 1>(off1, m + r) = dir.eigen();
                A.block<3, 1>(off1 + 3, m + r) = lt1.eigen();
            }
            if (b2->Variables().IsActive()) {
                const int off2 = static_cast<int>(b2->GetOffset_w());
                const auto lt2 = b2->TransformDirectionParentToLocal(motion::Vcross(p2 - b2->GetCoord().pos, -dir));
                A.block<3, 1>(off2, m + r) = (-dir).eigen();
                A.block<3, 1>(off2 + 3, m + r) = lt2.eigen();
            }
        }

        const Eigen::VectorXd sol = A.completeOrthogonalDecomposition().solve(-force);
        for (int r = 0; r < k; ++r) {
            auto& rt = *runtimes[r];
            rt.cylinder->SetDisabled(false);
            const double fReqN = (sol.size() > m + r && std::isfinite(sol(m + r))) ? sol(m + r) * 0.001 : 0.0;
            const double preload = std::max(rt.cylinder->GetTankPressureMPa(),
                std::min(rt.definition.value("initialPressureA").toDouble(1.0),
                         rt.definition.value("initialPressureB").toDouble(1.0)));
            rt.cylinder->ApplySteadyInitialPressures(fReqN, preload);
        }
    } else {
        for (auto* rt : runtimes) rt->cylinder->SetDisabled(false);
    }

    system.Setup();
    system.Update(0.0, true);
    return QStringLiteral("液压稳态初始化已完成：%1 个液压缸按 t=0 静载平衡初始腔压。").arg(k);
}

void recordNativeHydraulicCylinders(NativeMotionSystem& system)
{
    const double time = system.GetMotionTime();
    for (auto& entry : system.hydraulicCylinders) {
        auto& rt = *entry.second;
        if (!rt.history.empty() && rt.history.back().time >= time) continue;
        const auto& cyl = *rt.cylinder;
        NativeHydraulicSample sample;
        sample.time = time;
        sample.pressureA = cyl.GetPressureAMPa();
        sample.pressureB = cyl.GetPressureBMPa();
        sample.forceN = cyl.GetTotalForceN();
        sample.strokeMm = cyl.GetStrokeMm();
        sample.velocityMmS = cyl.GetStrokeRateMmS();
        sample.flowALpm = cyl.GetFlowALpm();
        sample.flowBLpm = cyl.GetFlowBLpm();
        sample.command = cyl.GetCommandValue(time);
        sample.mechPowerW = cyl.GetMechanicalPowerW();
        sample.fluidPowerW = cyl.GetFluidPowerW();
        rt.history.push_back(sample);
    }
}
}
