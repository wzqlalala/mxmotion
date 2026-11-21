#include "NativeMotionActuators.h"
#include "NativeMotionRuntime.h"

#include "motion/functions/Function_Expr.h"
#include "motion/functions/Function_Table.h"
#include "motion/physics/LinkMotorLinearForce.h"
#include "motion/physics/LinkMotorRotationTorque.h"

#include <QJsonArray>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace MotionOcc {
namespace {
constexpr double degrees=180.0/3.14159265358979323846;
[[noreturn]] void fail(const QString& message){throw std::runtime_error(message.toStdString());}

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
}
