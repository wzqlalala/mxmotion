#include "NativeMotionRuntime.h"

#include <QJsonArray>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <tuple>

namespace MotionOcc {
namespace {
constexpr double radToDeg=57.295779513082320876;
using Vec=motion::MotionVector<>;
Vec vec(const motion::MotionVec3& v){return Vec(v.x,v.y,v.z);}
Vector3d triple(const motion::MotionVec3& v,double scale=1){return {v.x*scale,v.y*scale,v.z*scale};}

Vector3d eulerDegrees(const motion::Quaternion& rotation)
{
    const double scale = std::max({std::abs(rotation.e0()), std::abs(rotation.e1()),
                                   std::abs(rotation.e2()), std::abs(rotation.e3())});
    if (!std::isfinite(rotation.e0()) || !std::isfinite(rotation.e1()) ||
        !std::isfinite(rotation.e2()) || !std::isfinite(rotation.e3())) {
        const double invalid = std::numeric_limits<double>::quiet_NaN();
        return {invalid, invalid, invalid};
    }
    // The engine leaves some result frames without orientation, such as
    // coupling ports. Preserve the original zero-angle channel for them.
    if (scale == 0) return {0, 0, 0};

    const double w = rotation.e0() / scale;
    const double x = rotation.e1() / scale;
    const double y = rotation.e2() / scale;
    const double z = rotation.e3() / scale;
    const double norm2 = w*w + x*x + y*y + z*z;
    const double yawSin = 2 * (x*z + y*w);
    const double yawCos = norm2 - 2 * (x*x + y*y);
    const double pitchCos = std::hypot(yawSin, yawCos);

    // Keep Qt's Z-X-Y rotation order and [pitch, yaw, roll] output. Using
    // atan2 for pitch avoids asin's domain boundary at a 90-degree pitch.
    const double pitch = std::atan2(2 * (x*w - y*z), pitchCos);
    const double gimbalTolerance = std::sqrt(std::numeric_limits<double>::epsilon());
    if (pitchCos <= gimbalTolerance * norm2) {
        // At gimbal lock, absorb the remaining rotation into yaw with roll zero.
        const double yaw = std::atan2(2 * (y*w - x*z), norm2 - 2 * (y*y + z*z));
        return {pitch * radToDeg, yaw * radToDeg, 0.0};
    }
    const double yaw = std::atan2(yawSin, yawCos);
    const double roll = std::atan2(2 * (x*y + z*w), norm2 - 2 * (x*x + z*z));
    return {pitch * radToDeg, yaw * radToDeg, roll * radToDeg};
}

class Channels {
public:
    Channels(RigidResult& result,std::size_t frameCount):result(result),frameCount(frameCount){}

    void beginFrame(){nextIndex=0;}
    void endFrames(){nextIndex=unordered;}

    template<class Values>
    void add(const QString& source,int id,const QString& variable,const QString& unit,const Values& v)
    {
        if(v.empty())throw std::runtime_error("结果通道不能有零个分量。");
        for(double x:v)if(!std::isfinite(x))throw std::runtime_error(QStringLiteral("%1 %2 的 %3 出现非有限值。").arg(source).arg(id).arg(variable).toStdString());
        auto& channel=channelFor(source,id,variable,unit,v.size());
        if(channel.components!=(int)v.size())throw std::runtime_error("结果通道分量数在求解中变化。");
        channel.values.insert(channel.values.end(),v.begin(),v.end());
    }
    void scalar(const QString& source,int id,const QString& variable,const QString& unit,double value)
    {
        add(source,id,variable,unit,std::array<double,1>{value});
    }
    void kinematic(const QString& source,int id,const motion::state_kinematic& state,bool secondJointFrame=false)
    {
        static const std::array<QString,7> first{
            QStringLiteral("Position"),QStringLiteral("Displacement"),QStringLiteral("Velocity"),QStringLiteral("Acceleration"),
            QStringLiteral("Rotation"),QStringLiteral("AngularVelocity"),QStringLiteral("AngularAcceleration")};
        static const std::array<QString,7> second{
            QStringLiteral("PositionJ"),QStringLiteral("DisplacementJ"),QStringLiteral("VelocityJ"),QStringLiteral("AccelerationJ"),
            QStringLiteral("RotationJ"),QStringLiteral("AngularVelocityJ"),QStringLiteral("AngularAccelerationJ")};
        const auto& names=secondJointFrame?second:first;
        add(source,id,names[0],QStringLiteral("mm"),triple(state.pos));
        add(source,id,names[1],QStringLiteral("mm"),triple(state.dpos));
        add(source,id,names[2],QStringLiteral("mm/s"),triple(state.vel));
        add(source,id,names[3],QStringLiteral("mm/s²"),triple(state.acc));
        add(source,id,names[4],QStringLiteral("°"),eulerDegrees(state.rpos));
        add(source,id,names[5],QStringLiteral("°/s"),triple(state.rvel,radToDeg));
        add(source,id,names[6],QStringLiteral("°/s²"),triple(state.racc,radToDeg));
    }
    void validate()const
    {
        for(const auto& channel:result.channels)if(channel.values.size()!=result.frames.size()*channel.components)
            throw std::runtime_error(QStringLiteral("结果通道 %1/%2/%3 没有覆盖每个输出帧。").arg(channel.sourceType).arg(channel.sourceId).arg(channel.variable).toStdString());
    }
private:
    ResultChannel& channelFor(const QString& source,int id,const QString& variable,const QString& unit,std::size_t components)
    {
        // 常规帧按首帧建立的通道顺序写入。遇到可选数据时仍按原键定位。
        if(nextIndex<result.channels.size()){
            auto& channel=result.channels[nextIndex];
            if(channel.sourceId==id&&channel.sourceType==source&&channel.variable==variable){
                ++nextIndex;
                return channel;
            }
        }
        nextIndex=unordered;
        const auto key=std::make_tuple(source,id,variable);
        const auto found=indices.find(key);
        std::size_t index;
        if(found==indices.end()){
            index=result.channels.size();indices.emplace(key,index);
            ResultChannel channel;
            channel.sourceType=source;channel.sourceId=id;channel.variable=variable;channel.unit=unit;
            channel.components=static_cast<int>(components);
            if(frameCount<=channel.values.max_size()/components)channel.values.reserve(frameCount*components);
            result.channels.push_back(std::move(channel));
        }else index=found->second;
        return result.channels[index];
    }
    static constexpr std::size_t unordered=std::numeric_limits<std::size_t>::max();
    RigidResult& result;
    const std::size_t frameCount;
    std::size_t nextIndex=unordered;
    std::map<std::tuple<QString,int,QString>,std::size_t> indices;
};

template<class History>
const auto& sample(const History& h,std::size_t index,double time)
{
    if(index>=h.data.size()||std::abs(h.data[index].time-time)>1e-8*std::max(1.0,std::abs(time)))
        throw std::runtime_error("引擎结果通道时间轴不一致；没有按帧号拼接不同时刻的数据。");
    return h.data[index];
}
QString sensorUnit(const QJsonObject& o,const NativeMotionDocument& doc)
{
    const auto type=o.value("sensorType").toString(),v=o.value("variable").toString();
    if(type=="Angle"||v=="Rotation")return "°";
    if(v=="AngularVelocity")return "°/s";if(v=="AngularAcceleration")return "°/s²";
    if(v.endsWith("Force"))return "N";if(v.endsWith("Torque"))return "N·mm";
    const bool angular=type=="DriveActuation"&&doc.entity("drives",o.value("targetDriveId").toInt(-1)).value("motionType").toString()=="Rotation";
    if(v=="Velocity"||v=="RelVelocity"||v=="ImposedSpeed"||v=="Rate")return angular?"°/s":"mm/s";
    if(v=="Acceleration"||v=="RelAcceleration"||v=="ImposedAcc"||v=="CoordinateAcceleration")return angular?"°/s²":"mm/s²";
    return angular?"°":"mm";
}
}

void extractNativeResults(const NativeMotionInput& input,const NativeMotionSystem& system,
                          const std::vector<NativeFlexRuntime>& flex,RigidResult& result)
{
    result.scenarioId=input.scenario.id;
    if(system.body_results.empty())throw std::runtime_error("引擎没有记录运动体结果。");
    const auto& timeline=system.body_results.begin()->second.data;
    if(timeline.empty())throw std::runtime_error("引擎结果时间轴为空。");
    Channels channels(result,timeline.size());
    result.frames.reserve(timeline.size());

    struct BodyHistory { int id; const motion::body_history* history; };
    std::vector<BodyHistory> bodies;
    for(const auto& item:system.body_results){
        if(item.first=="ground")continue;
        bool numeric=false;
        const int id=QString::fromStdString(item.first).toInt(&numeric);
        if(numeric)bodies.push_back({id,&item.second});
    }

    struct MarkerHistory { int id; const motion::marker_history* history; };
    std::vector<MarkerHistory> markers;
    for(const auto& item:system.marker_results){
        if(item.first.rfind("UM_",0)==0)markers.push_back({std::stoi(item.first.substr(3)),&item.second});
    }

    struct JointHistory {
        int id;
        const motion::joint_history* history;
        QString source;
        bool coupling,rotational;
    };
    std::vector<JointHistory> joints;
    for(const auto& item:system.joint_results){
        const bool coupling=item.first.rfind("CP_",0)==0;
        bool numeric=false;
        const int id=QString::fromStdString(coupling?item.first.substr(3):item.first).toInt(&numeric);
        if(!numeric)continue;
        const auto type=coupling?QString{}:input.document.entity(QStringLiteral("joints"),id).value("jointType").toString();
        const bool rotational=type=="Revolute"||type=="Cylindrical"||type=="Screw";
        joints.push_back({id,&item.second,coupling?QStringLiteral("couplings"):QStringLiteral("joints"),coupling,rotational});
    }

    struct DriveHistory {
        int id;
        const motion::motion_history* history;
        const NativeEffortRuntime* effort;
        bool bodyDrive,pose,imposesPosition,angular;
    };
    std::vector<DriveHistory> drives;
    for(const auto& item:system.motion_results){
        const auto name=QString::fromStdString(item.first);
        const bool bodyDrive=name.startsWith("BD_");
        bool numeric=false;
        const int id=(bodyDrive?name.mid(3):name).toInt(&numeric);
        if(!numeric)continue;
        if(bodyDrive){
            const auto mode=input.document.entity(QStringLiteral("bodyDrives"),id).value("bodyDriveMode").toString();
            drives.push_back({id,&item.second,nullptr,true,mode=="Pose",!mode.endsWith("Speed"),mode.startsWith("Rotation")});
        }else{
            const bool angular=input.document.entity(QStringLiteral("drives"),id).value("motionType").toString()=="Rotation";
            const auto effort=system.effortMotors.find(id);
            drives.push_back({id,&item.second,effort==system.effortMotors.end()?nullptr:effort->second.get(),false,false,false,angular});
        }
    }

    const std::map<std::string,QString> forceCollections{{"SPRING","springs"},{"TORSION_SPRING","springs"},{"TENSION_CABLE","tensionCables"},{"SOFT_STOP","softStops"},{"SCALAR_FORCE","scalarForces"},{"SCALAR_TORQUE","scalarTorques"},{"THREE_POINT_FORCE","threePointForces"},{"THREE_POINT_TORQUE","threePointTorques"},{"FORCE","vectorForces"},{"TORQUE","vectorForces"}};
    struct ForceHistory {
        int id;
        const motion::force_element_history* history;
        QString source;
        bool torsion,torque;
    };
    std::vector<ForceHistory> forces;
    for(const auto& item:system.force_results){
        if(item.second.id.rfind("HC_",0)==0)continue;
        const auto collection=forceCollections.find(item.second.type);
        if(collection==forceCollections.end())throw std::runtime_error("未知引擎力元结果类型："+item.second.type);
        const bool torsion=item.second.type=="TORSION_SPRING";
        forces.push_back({std::stoi(item.second.id),&item.second,collection->second,torsion,torsion||item.second.type.find("TORQUE")!=std::string::npos});
    }

    struct Sensor { QJsonObject definition; int id; QString unit; bool angle; };
    std::vector<Sensor> sensors;
    for(const auto& item:input.document.collection(QStringLiteral("sensors"))){
        const auto definition=item.toObject();
        if(definition.value("enabled").toBool(true))
            sensors.push_back({definition,definition.value("id").toInt(),sensorUnit(definition,input.document),
                               definition.value("sensorType").toString()=="Angle"});
    }
    const bool sampleSensors=!sensors.empty();
    std::map<int,double> jointAngles,sensorAngles;
    for(std::size_t fi=0;fi<timeline.size();++fi){
        const double time=timeline[fi].time;
        if(!std::isfinite(time)||(fi&&time<=result.frames.back().seconds))throw std::runtime_error("结果时间轴不严格递增。");
        channels.beginFrame();
        RigidFrame frame;frame.seconds=time;
        frame.bodies.reserve(bodies.size());
        frame.joints.reserve(joints.size());
        std::map<int,motion::state_kinematic> bodyStates;
        std::map<int,motion::joint_result> jointStates;
        std::map<int,motion::motion_result> driveStates;
        for(const auto& item:bodies){
            const int id=item.id;
            const auto& value=sample(*item.history,fi,time);const auto& state=value.cm;
            BodyState body;body.bodyId=id;body.positionMm=triple(state.pos);body.velocityMmS=triple(state.vel);body.angularVelocityDegS=triple(state.rvel,radToDeg);
            body.rotation={state.rpos.e0(),state.rpos.e1(),state.rpos.e2(),state.rpos.e3()};frame.bodies.push_back(body);
            if(sampleSensors)bodyStates.emplace(id,state);
            channels.kinematic(QStringLiteral("bodies"),id,state);channels.scalar(QStringLiteral("bodies"),id,QStringLiteral("PotentialEnergy"),QStringLiteral("J"),value.potential_energy*1e-6);channels.scalar(QStringLiteral("bodies"),id,QStringLiteral("KineticEnergy"),QStringLiteral("J"),value.kinetic_energy*1e-6);
        }
        for(const auto& item:markers)
            channels.kinematic(QStringLiteral("markers"),item.id,sample(*item.history,fi,time).kinematic);
        for(const auto& item:joints){
            const int id=item.id;
            const auto& source=item.source;
            const auto& value=sample(*item.history,fi,time);
            channels.kinematic(source,id,value.node_I);channels.kinematic(source,id,value.node_J,true);
            channels.add(source,id,QStringLiteral("ReactionForce"),QStringLiteral("N"),triple(value.reaction_force,.001));channels.add(source,id,QStringLiteral("ReactionTorque"),QStringLiteral("N·mm"),triple(value.reaction_torque,.001));
            if(!item.coupling){
                if(sampleSensors)jointStates.emplace(id,value);
                const bool rotational=item.rotational;
                const auto& a=value.node_I;const auto& b=value.node_J;
                JointState joint;joint.jointId=id;
                if(rotational){
                    const auto rel=b.rpos.GetConjugate()*a.rpos;
                    double angle=2*std::atan2(rel.e3(),rel.e0())*radToDeg;
                    const auto previous=jointAngles.find(id);if(previous!=jointAngles.end())angle+=360*std::floor((previous->second-angle)/360+.5);jointAngles[id]=angle;
                    joint.position=angle;joint.speed=b.rpos.RotateBack(vec(a.rvel)-vec(b.rvel)).z()*radToDeg;
                }else{
                    joint.position=b.rpos.RotateBack(vec(a.pos)-vec(b.pos)).z();joint.speed=b.rpos.RotateBack(vec(a.vel)-vec(b.vel)).z();
                }
                frame.joints.push_back(joint);
                channels.scalar(source,id,QStringLiteral("Coordinate"),rotational?QStringLiteral("°"):QStringLiteral("mm"),joint.position);channels.scalar(source,id,QStringLiteral("Rate"),rotational?QStringLiteral("°/s"):QStringLiteral("mm/s"),joint.speed);
            }
        }
        for(const auto& item:drives){
            const int id=item.id;
            const bool angular=item.angular;
            const double scale=angular?radToDeg:1;
            if (item.bodyDrive) {
                const auto& output = sample(*item.history, fi, time);
                channels.kinematic(QStringLiteral("bodyDrives"), id, output.abscoord);
                if (!item.pose) {
                    if (item.imposesPosition)
                        channels.scalar(QStringLiteral("bodyDrives"), id, QStringLiteral("ImposedValue"), angular ? QStringLiteral("°") : QStringLiteral("mm"), output.imposed_value * scale);
                    channels.scalar(QStringLiteral("bodyDrives"), id, QStringLiteral("ImposedSpeed"), angular ? QStringLiteral("°/s") : QStringLiteral("mm/s"), output.imposed_speed * scale);
                    channels.scalar(QStringLiteral("bodyDrives"), id, QStringLiteral("ImposedAcc"), angular ? QStringLiteral("°/s²") : QStringLiteral("mm/s²"), output.imposed_acc * scale);
                }
                // 旧单轴马达历史保存的是参考侧反力；统一为作用在驱动体上的驱动力。
                const double actionScale = item.pose ? .001 : -.001;
                channels.add(QStringLiteral("bodyDrives"), id, QStringLiteral("ActuationForce"), QStringLiteral("N"), triple(output.actuation_force, actionScale));
                channels.add(QStringLiteral("bodyDrives"), id, QStringLiteral("ActuationTorque"), QStringLiteral("N·mm"), triple(output.actuation_torque, actionScale));
                channels.add(QStringLiteral("bodyDrives"), id, QStringLiteral("ReactionForce"), QStringLiteral("N"), triple(output.reaction_force, .001));
                channels.add(QStringLiteral("bodyDrives"), id, QStringLiteral("ReactionTorque"), QStringLiteral("N·mm"), triple(output.reaction_torque, .001));
                continue;
            }
            const auto& value=sample(*item.history,fi,time);
            const bool isEffort=item.effort!=nullptr;
            channels.kinematic(QStringLiteral("drives"),id,value.abscoord);
            channels.scalar(QStringLiteral("drives"),id,isEffort?QStringLiteral("Coordinate"):QStringLiteral("ImposedValue"),angular?QStringLiteral("°"):QStringLiteral("mm"),value.imposed_value*scale);
            channels.scalar(QStringLiteral("drives"),id,isEffort?QStringLiteral("Rate"):QStringLiteral("ImposedSpeed"),angular?QStringLiteral("°/s"):QStringLiteral("mm/s"),value.imposed_speed*scale);
            channels.scalar(QStringLiteral("drives"),id,isEffort?QStringLiteral("CoordinateAcceleration"):QStringLiteral("ImposedAcc"),angular?QStringLiteral("°/s²"):QStringLiteral("mm/s²"),value.imposed_acc*scale);
            if(isEffort){
                const auto& history=item.effort->history;
                if(fi>=history.size() || std::abs(history[fi].time-time)>1e-8*std::max(1.0,std::abs(time)))
                    throw std::runtime_error("力/力矩执行器输出与结果时间轴不同步。");
                const auto& applied=history[fi];
                channels.scalar(QStringLiteral("drives"),id,QStringLiteral("EffortCommand"),angular?QStringLiteral("N·mm"):QStringLiteral("N"),applied.requested);
                channels.scalar(QStringLiteral("drives"),id,QStringLiteral("EffortApplied"),angular?QStringLiteral("N·mm"):QStringLiteral("N"),applied.applied);
                channels.scalar(QStringLiteral("drives"),id,QStringLiteral("MechanicalPower"),QStringLiteral("W"),applied.power);
                channels.scalar(QStringLiteral("drives"),id,QStringLiteral("EffortLimited"),QStringLiteral(""),applied.limited?1:0);
            }
            channels.add(QStringLiteral("drives"),id,QStringLiteral("ActuationForce"),QStringLiteral("N"),triple(value.actuation_force,.001));channels.add(QStringLiteral("drives"),id,QStringLiteral("ActuationTorque"),QStringLiteral("N·mm"),triple(value.actuation_torque,.001));
            channels.add(QStringLiteral("drives"),id,QStringLiteral("ReactionForce"),QStringLiteral("N"),triple(value.reaction_force,.001));channels.add(QStringLiteral("drives"),id,QStringLiteral("ReactionTorque"),QStringLiteral("N·mm"),triple(value.reaction_torque,.001));
            if(sampleSensors){
                auto sensorState=value;
                sensorState.imposed_value*=scale;sensorState.imposed_speed*=scale;sensorState.imposed_acc*=scale;
                driveStates.emplace(id,sensorState);
            }
        }
        for(const auto& item:forces){
            const int id=item.id;
            const auto& value=sample(*item.history,fi,time);
            const auto& source=item.source;
            const bool torsion=item.torsion,torque=item.torque;
            const double scale=torsion?radToDeg:1;
            if(value.has_kinematics){channels.scalar(source,id,QStringLiteral("Coordinate"),torsion?QStringLiteral("°"):QStringLiteral("mm"),value.coordinate*scale);channels.scalar(source,id,QStringLiteral("Deformation"),torsion?QStringLiteral("°"):QStringLiteral("mm"),value.deformation*scale);channels.scalar(source,id,QStringLiteral("Speed"),torsion?QStringLiteral("°/s"):QStringLiteral("mm/s"),value.speed*scale);}
            channels.scalar(source,id,QStringLiteral("Magnitude"),torque?QStringLiteral("N·mm"):QStringLiteral("N"),value.magnitude*.001);channels.scalar(source,id,QStringLiteral("SignedForce"),torque?QStringLiteral("N·mm"):QStringLiteral("N"),value.signed_force*.001);
            channels.scalar(source,id,QStringLiteral("Engaged"),QStringLiteral(""),value.engaged?1:0);channels.scalar(source,id,QStringLiteral("Power"),QStringLiteral("W"),value.power*1e-6);
            if(value.has_elastic_energy)channels.scalar(source,id,QStringLiteral("ElasticEnergy"),QStringLiteral("J"),value.elastic_energy*1e-6);
            channels.add(source,id,QStringLiteral("PointI"),QStringLiteral("mm"),triple(value.point_I));channels.add(source,id,QStringLiteral("PointJ"),QStringLiteral("mm"),triple(value.point_J));
            channels.add(source,id,QStringLiteral("ForceI"),QStringLiteral("N"),triple(value.force_I,.001));channels.add(source,id,QStringLiteral("ForceJ"),QStringLiteral("N"),triple(value.force_J,.001));
            channels.add(source,id,QStringLiteral("TorqueI"),QStringLiteral("N·mm"),triple(value.torque_I,.001));channels.add(source,id,QStringLiteral("TorqueJ"),QStringLiteral("N·mm"),triple(value.torque_J,.001));
        }
        for(const auto& sensor:sensors){
            const int id=sensor.id;
            auto values=evaluateNativeSensor(sensor.definition,bodyStates,jointStates,driveStates);
            if(sensor.angle){
                const auto last=sensorAngles.find(id);if(last!=sensorAngles.end())values[0]+=360*std::floor((last->second-values[0])/360+.5);sensorAngles[id]=values[0];
            }
            channels.add(QStringLiteral("sensors"),id,QStringLiteral("SensorValue"),sensor.unit,values);
        }
        for(const auto& controller:system.controllers){
            if(fi>=controller.history.size()||std::abs(controller.history[fi].time-time)>1e-8*std::max(1.0,std::abs(time)))
                throw std::runtime_error("PID 采样与结果帧时间轴不同步，没有沿用旧反馈填补缺失采样。");
            const auto& value=controller.history[fi];
            channels.scalar(QStringLiteral("controllers"),controller.id,QStringLiteral("PidTarget"),controller.feedbackUnit,value.target);channels.scalar(QStringLiteral("controllers"),controller.id,QStringLiteral("PidFeedback"),controller.feedbackUnit,value.feedback);
            channels.scalar(QStringLiteral("controllers"),controller.id,QStringLiteral("PidError"),controller.feedbackUnit,value.error);channels.scalar(QStringLiteral("controllers"),controller.id,QStringLiteral("PidOutput"),controller.outputUnit,value.output);
        }
        for(const auto& entry:system.hydraulicCylinders){
            const int id=entry.first;
            const auto& history=entry.second->history;
            if(fi>=history.size()||std::abs(history[fi].time-time)>1e-8*std::max(1.0,std::abs(time)))
                throw std::runtime_error("液压缸采样与结果帧时间轴不同步。");
            const auto& value=history[fi];
            const QString src=QStringLiteral("hydraulicCylinders");
            channels.scalar(src,id,QStringLiteral("PressureA"),QStringLiteral("MPa"),value.pressureA);
            channels.scalar(src,id,QStringLiteral("PressureB"),QStringLiteral("MPa"),value.pressureB);
            channels.scalar(src,id,QStringLiteral("Force"),QStringLiteral("N"),value.forceN);
            channels.scalar(src,id,QStringLiteral("Stroke"),QStringLiteral("mm"),value.strokeMm);
            channels.scalar(src,id,QStringLiteral("Velocity"),QStringLiteral("mm/s"),value.velocityMmS);
            channels.scalar(src,id,QStringLiteral("FlowA"),QStringLiteral("L/min"),value.flowALpm);
            channels.scalar(src,id,QStringLiteral("FlowB"),QStringLiteral("L/min"),value.flowBLpm);
            channels.scalar(src,id,QStringLiteral("ControlSignal"),QStringLiteral(""),value.command);
            channels.scalar(src,id,QStringLiteral("Power"),QStringLiteral("W"),value.mechPowerW);
            channels.scalar(src,id,QStringLiteral("FluidPower"),QStringLiteral("W"),value.fluidPowerW);
        }
        result.frames.push_back(std::move(frame));
    }
    channels.endFrames();
    for(const auto& runtime:flex){
        RigidFlexResult output;output.flexId=runtime.id;output.bodyId=runtime.bodyId;output.referenceNodes=runtime.result.node_x0;output.surfaceTriangles=runtime.result.surface_tris;
        output.sourceNodeIds=runtime.importedNodeIds;
        output.tetrahedra=runtime.tetrahedra;
        output.modeCount=runtime.result.n_modes;
        output.interfaceCount=static_cast<int>(runtime.result.interface_bodies.size());
        output.densityKgMm3=runtime.densityKgMm3;
        output.psiInternal=runtime.result.psi_internal;
        output.reducedDynamics=runtime.reducedDynamics;
        output.modalBasis=runtime.modalBasis;
        if(!runtime.importedBody)prepareAutoFlexContour(output);
        if(runtime.interfacePoses.empty())throw std::runtime_error("柔性体没有初始接口位姿。");
        output.initialInterfacePoses=runtime.referenceInterfacePoses.empty()?runtime.interfacePoses.front():runtime.referenceInterfacePoses;
        if(output.reducedDynamics && output.psiInternal.size()!=output.referenceNodes.size()*static_cast<std::size_t>(6*output.interfaceCount+output.modeCount))
            throw std::runtime_error("柔性体模态矩阵尺寸与节点/接口/模态数不一致。");
        for(const auto& frame:result.frames){
            const auto found=std::lower_bound(runtime.times.begin(),runtime.times.end(),frame.seconds-1e-9);
            if(found==runtime.times.end()||std::abs(*found-frame.seconds)>1e-8)throw std::runtime_error("柔性体采样与刚体结果时间轴不一致。");
            const std::size_t index=static_cast<std::size_t>(found-runtime.times.begin());output.nodePositions.push_back(runtime.nodePositions[index]);
            output.interfacePoses.push_back(runtime.interfacePoses.at(index));
            output.modalCoordinates.push_back(runtime.modalCoordinates.at(index));
            if(output.modeCount>0)channels.add(runtime.importedBody ? QStringLiteral("importedMnfBodies") : QStringLiteral("autoFlexBodies"),runtime.id,QStringLiteral("ModalCoordinates"),QStringLiteral(""),runtime.modalCoordinates[index]);
        }
        result.flexBodies.push_back(std::move(output));
    }
    channels.validate();
}

} // namespace MotionOcc
