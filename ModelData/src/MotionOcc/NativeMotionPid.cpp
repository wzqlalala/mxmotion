#include "NativeMotionRuntime.h"

#include "motion/physics/LinkMotorLinear.h"
#include "motion/physics/LinkMotorRotation.h"

#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>

namespace MotionOcc {
namespace {
std::pair<double,double> targetAt(const NativePidRuntime& rt,double t)
{
    if(rt.trajectory.isEmpty())return {rt.definition.value("targetValue").toDouble(),0};
    const auto& o=rt.trajectory;
    if(o.value("inputType").toString()=="CustomCurve"){
        const auto points=o.value("curvePoints").toArray();
        if(points.isEmpty())throw std::runtime_error("PID 自定义目标轨迹没有采样点。");
        const auto point=[](const QJsonValue& p){const auto a=p.toArray();if(a.size()!=2)throw std::runtime_error("PID 轨迹点应为 [time,value]。");return std::pair<double,double>{a[0].toDouble(),a[1].toDouble()};};
        auto previous=point(points[0]);if(t<=previous.first)return {previous.second,0};
        for(int i=1;i<points.size();++i){const auto next=point(points[i]);if(next.first<=previous.first)throw std::runtime_error("PID 自定义轨迹时间不严格递增。");
            if(t<next.first){const double rate=(next.second-previous.second)/(next.first-previous.first);return {previous.second+(t-previous.first)*rate,rate};}previous=next;}
        return {previous.second,0};
    }
    const double first=o.value("timeStart").toDouble(),last=o.value("timeEnd").toDouble(1);
    if(!std::isfinite(first)||!std::isfinite(last)||last<first)throw std::runtime_error("PID 轨迹时间窗无效。");
    const double x=std::clamp(t,first,last);double value=0,rate=0,power=1;
    const auto coefficients=o.value("polyCoeffs").toArray();
    for(int i=0;i<coefficients.size();++i){const double c=coefficients[i].toDouble();value+=c*power;if(i>0)rate+=i*c*std::pow(x,i-1);power*=x;}
    if(t<=first||t>=last)rate=0;return {value,rate};
}

std::map<int,motion::state_kinematic> bodyStates(NativeMotionSystem& system)
{
    std::map<int,motion::state_kinematic> out;
    for(const auto& definition:system.parts_map){
        if(definition.first=="ground")continue;
        const auto found=system.nativeBodies.find(std::stoi(definition.first));
        if(found==system.nativeBodies.end())throw std::runtime_error("PID 传感器的运动体未找到。");
        const auto body=found->second;
        motion::state_kinematic s{};
        const auto set=[](motion::MotionVec3& target,const motion::MotionVector<>& v){target={v.x(),v.y(),v.z()};};
        set(s.pos,body->GetPos());set(s.vel,body->GetPos_dt());set(s.acc,body->GetPos_dtdt());
        s.rpos=body->GetRot();set(s.rvel,body->GetWvel_par());set(s.racc,body->GetWacc_par());out.emplace(std::stoi(definition.first),s);
    }
    return out;
}

double feedback(NativePidRuntime& rt,NativeMotionSystem& system)
{
    double value=0;
    if(!rt.sensor.isEmpty()){
        const auto sample=evaluateNativeSensor(rt.sensor,bodyStates(system));
        const auto component=rt.sensor.value("componentMode").toString("Magnitude");
        if(sample.size()==1)value=sample.front();
        else if(component=="X")value=sample[0];else if(component=="Y")value=sample[1];else if(component=="Z")value=sample[2];
        else value=std::hypot(sample[0],sample[1],sample[2]);
        if(rt.sensor.value("sensorType").toString()=="Angle"){
            if(rt.hasAngle)value+=360*std::floor((rt.unwrappedAngle-value)/360+.5);
            rt.unwrappedAngle=value;rt.hasAngle=true;
            if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motor))motor->SetMotorRot(value*.017453292519943295*rt.definition.value("feedbackSign").toDouble(1));
        }
    }else if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motor))
        value=rt.definition.value("feedbackType").toString()=="Velocity"?motor->GetMotorRot_dt():motor->GetMotorRot();
    else if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorLinear>(rt.motor))
        value=rt.definition.value("feedbackType").toString()=="Velocity"?motor->GetMotorPos_dt():motor->GetMotorPos();
    else throw std::runtime_error("PID 目标驱动不是线性或旋转电机。");
    return value*rt.definition.value("feedbackSign").toDouble(1);
}

void update(NativePidRuntime& rt,NativeMotionSystem& system,double t)
{
    if(rt.hasSample&&t<=rt.lastTime)return;
    const auto target=targetAt(rt,t);const double fb=feedback(rt,system);
    const auto variable=rt.sensor.value("variable").toString();
    const bool angular=!rt.sensor.isEmpty()&&(rt.sensor.value("sensorType").toString()=="Angle"||variable=="Rotation"||variable=="AngularVelocity"||variable=="AngularAcceleration");
    const double scale=angular?.017453292519943295:1;
    const double error=(target.first-fb)*scale;
    const double dt=rt.hasSample?t-rt.lastTime:0;
    const double kp=rt.definition.value("kp").toDouble(1),ki=rt.definition.value("ki").toDouble(),kd=rt.definition.value("kd").toDouble();
    const double derivative=dt>1e-12?-kd*(fb*scale-rt.lastFeedback)/dt:0;
    const double candidate=rt.integral+error*dt;
    const double limit=rt.definition.value("outputLimit").toDouble();
    double output=target.second*scale+kp*error+ki*candidate+derivative;
    if(limit>0&&std::abs(output)>limit&&error*output>0)output=target.second*scale+kp*error+ki*rt.integral+derivative;
    else rt.integral=candidate;
    if(limit>0)output=std::clamp(output,-limit,limit);
    if(!rt.sensor.isEmpty())output*=rt.definition.value("feedbackSign").toDouble(1);
    if(!std::isfinite(output))throw std::runtime_error("PID 输出为非有限值。");
    rt.setpoint->SetSetpoint(output,t);
    if(rt.sensor.isEmpty())if(const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(rt.motor))motor->SetMotorRot(fb*rt.definition.value("feedbackSign").toDouble(1));
    rt.lastFeedback=fb*scale;rt.lastTime=t;rt.hasSample=true;
    rt.history.push_back({t,target.first,fb,target.first-fb,output});
}
}

void NativeMotionSystem::configureControllers(const NativeMotionInput& input)
{
    std::set<int> controlled;
    for(const auto& value:input.document.collection("controllers")){
        const auto o=value.toObject();if(!o.value("enabled").toBool(true))continue;
        if(o.value("controllerType").toString("PID")!="PID")throw std::runtime_error("未知控制器类型。");
        NativePidRuntime rt;rt.id=o.value("id").toInt();rt.driveId=o.value("targetDriveId").toInt(-1);rt.definition=o;
        if(!controlled.insert(rt.driveId).second)throw std::runtime_error("两个启用的 PID 控制器不能接管同一驱动。");
        const auto drive=input.document.entity("drives",rt.driveId);
        if(drive.isEmpty()||drive.value("controlMode").toString()!="Velocity")throw std::runtime_error("PID 控制器需要有效的速度控制驱动。");
        // Joint and drive IDs occupy different source collections and can be
        // numerically equal. SearchLink alone returns the first passive joint.
        for(const auto& link:Get_linklist()){
            const auto motor=std::dynamic_pointer_cast<motion::MotionLinkMotor>(link);
            if(motor&&motor->GetNameString()==std::to_string(rt.driveId)){
                if(rt.motor)throw std::runtime_error("同一驱动 ID 对应多个电机，无法安全接管。");
                rt.motor=motor;
            }
        }
        if(!rt.motor)throw std::runtime_error("无法按稳定驱动 ID 找到 PID 电机；未使用不确定的唯一体/名称兜底。");
        const int trajectory=o.value("targetInputId").toInt(-1);
        if(trajectory>=0){rt.trajectory=input.document.entity("controlInputs",trajectory);if(rt.trajectory.isEmpty())throw std::runtime_error("PID 目标控制输入不存在。");}
        if(o.value("feedbackSource").toString()=="Sensor"){
            rt.sensor=input.document.entity("sensors",o.value("feedbackSensorId").toInt(-1));
            if(rt.sensor.isEmpty()||!rt.sensor.value("enabled").toBool(true))throw std::runtime_error("PID 反馈传感器不存在或未启用。");
            const auto type=rt.sensor.value("sensorType").toString();
            if(type=="JointReaction"||type=="DriveActuation")throw std::runtime_error("原 PID 控制线程只支持体、距离和角度传感器反馈；反力反馈尚无步内采样接口。");
        }
        rt.setpoint=std::make_shared<motion::MotionFunction_Setpoint>();rt.motor->SetMotorFunction(rt.setpoint);
        update(rt,*this,GetMotionTime());controllers.push_back(std::move(rt));
    }
}

void NativeMotionSystem::CustomEndOfStep()
{
    // The native fixed-output-step loop adds dt repeatedly. Thousands of steps
    // can accumulate a larger error than its end tolerance and trigger an
    // artificial ~1e-12 s final dynamics step. Normalize accepted times before
    // PID, position projection, and recording, never after simulating that step.
    if(acceptedStepSize>0){
        const double exact=std::min(acceptedEndTime,(++acceptedStepIndex)*acceptedStepSize);
        const double current=GetMotionTime();
        const double tolerance=256*std::numeric_limits<double>::epsilon()*std::max({1.0,std::abs(current),std::abs(exact)});
        if(!std::isfinite(current)||std::abs(current-exact)>tolerance)
            throw std::runtime_error("积分器没有推进到请求的输出时刻，已中止以免记录错误时间轴。");
        SetMotionTime(exact);
        GetTimestepper()->SetTime(exact);
    }
    const double time=GetMotionTime();
    if(projectPositions)projectAcceptedPositions();
    if(acceptedStepSize>0)rejectNativeGeometryBoundary(*this);
    validateNativeAcceptedConstraints(*this);
    for(auto& controller:controllers)update(controller,*this,time);
}

} // namespace MotionOcc
