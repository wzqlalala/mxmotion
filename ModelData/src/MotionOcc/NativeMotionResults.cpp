#include "NativeMotionRuntime.h"

#include <QJsonArray>
#include <QQuaternion>

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <tuple>

namespace MotionOcc {
namespace {
constexpr double radToDeg=57.295779513082320876;
using Vec=motion::MotionVector<>;
Vec vec(const motion::MotionVec3& v){return Vec(v.x,v.y,v.z);}
Vector3d triple(const motion::MotionVec3& v,double scale=1){return {v.x*scale,v.y*scale,v.z*scale};}
std::vector<double> values(const motion::MotionVec3& v,double scale=1){return {v.x*scale,v.y*scale,v.z*scale};}

class Channels {
public:
    explicit Channels(RigidResult& result):result(result){}
    void add(const QString& source,int id,const QString& variable,const QString& unit,const std::vector<double>& v)
    {
        if(v.empty())throw std::runtime_error("结果通道不能有零个分量。");
        for(double x:v)if(!std::isfinite(x))throw std::runtime_error(QStringLiteral("%1 %2 的 %3 出现非有限值。").arg(source).arg(id).arg(variable).toStdString());
        const auto key=std::make_tuple(source,id,variable);const auto found=indices.find(key);
        std::size_t index;
        if(found==indices.end()){
            index=result.channels.size();indices.emplace(key,index);
            ResultChannel channel;channel.sourceType=source;channel.sourceId=id;channel.variable=variable;channel.unit=unit;channel.components=static_cast<int>(v.size());result.channels.push_back(std::move(channel));
        }else index=found->second;
        auto& channel=result.channels[index];if(channel.components!=(int)v.size())throw std::runtime_error("结果通道分量数在求解中变化。");
        channel.values.insert(channel.values.end(),v.begin(),v.end());
    }
    void scalar(const QString& source,int id,const QString& variable,const QString& unit,double value){add(source,id,variable,unit,{value});}
    void kinematic(const QString& source,int id,const motion::state_kinematic& state,const QString& suffix={})
    {
        add(source,id,"Position"+suffix,"mm",values(state.pos));add(source,id,"Displacement"+suffix,"mm",values(state.dpos));
        add(source,id,"Velocity"+suffix,"mm/s",values(state.vel));add(source,id,"Acceleration"+suffix,"mm/s²",values(state.acc));
        const auto q=state.rpos;const auto e=QQuaternion(q.e0(),q.e1(),q.e2(),q.e3()).normalized().toEulerAngles();
        add(source,id,"Rotation"+suffix,"°",{e.x(),e.y(),e.z()});
        add(source,id,"AngularVelocity"+suffix,"°/s",values(state.rvel,radToDeg));add(source,id,"AngularAcceleration"+suffix,"°/s²",values(state.racc,radToDeg));
    }
    void validate()const
    {
        for(const auto& channel:result.channels)if(channel.values.size()!=result.frames.size()*channel.components)
            throw std::runtime_error(QStringLiteral("结果通道 %1/%2/%3 没有覆盖每个输出帧。").arg(channel.sourceType).arg(channel.sourceId).arg(channel.variable).toStdString());
    }
private:
    RigidResult& result;
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
    if(v=="Velocity"||v=="RelVelocity"||v=="ImposedSpeed")return angular?"°/s":"mm/s";
    if(v=="Acceleration"||v=="RelAcceleration"||v=="ImposedAcc")return angular?"°/s²":"mm/s²";
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
    Channels channels(result);
    std::map<int,double> jointAngles,sensorAngles;
    for(std::size_t fi=0;fi<timeline.size();++fi){
        const double time=timeline[fi].time;
        if(!std::isfinite(time)||(fi&&time<=result.frames.back().seconds))throw std::runtime_error("结果时间轴不严格递增。");
        RigidFrame frame;frame.seconds=time;
        std::map<int,motion::state_kinematic> bodyStates;
        std::map<int,motion::joint_result> jointStates;
        std::map<int,motion::motion_result> driveStates;
        for(const auto& item:system.body_results){
            if(item.first=="ground")continue;bool numeric=false;const int id=QString::fromStdString(item.first).toInt(&numeric);if(!numeric)continue;
            const auto& value=sample(item.second,fi,time);const auto& state=value.cm;
            BodyState body;body.bodyId=id;body.positionMm=triple(state.pos);body.velocityMmS=triple(state.vel);body.angularVelocityDegS=triple(state.rvel,radToDeg);
            body.rotation={state.rpos.e0(),state.rpos.e1(),state.rpos.e2(),state.rpos.e3()};frame.bodies.push_back(body);bodyStates.emplace(id,state);
            channels.kinematic("bodies",id,state);channels.scalar("bodies",id,"PotentialEnergy","J",value.potential_energy*1e-6);channels.scalar("bodies",id,"KineticEnergy","J",value.kinetic_energy*1e-6);
        }
        for(const auto& item:system.marker_results){
            if(item.first.rfind("UM_",0)!=0)continue;
            const int id=std::stoi(item.first.substr(3));channels.kinematic("markers",id,sample(item.second,fi,time).kinematic);
        }
        for(const auto& item:system.joint_results){
            const bool coupling=item.first.rfind("CP_",0)==0;bool numeric=false;
            const int id=QString::fromStdString(coupling?item.first.substr(3):item.first).toInt(&numeric);if(!numeric)continue;
            const auto& value=sample(item.second,fi,time);const QString source=coupling?"couplings":"joints";
            channels.kinematic(source,id,value.node_I);channels.kinematic(source,id,value.node_J,"J");
            channels.add(source,id,"ReactionForce","N",values(value.reaction_force,.001));channels.add(source,id,"ReactionTorque","N·mm",values(value.reaction_torque,.001));
            if(!coupling){
                jointStates.emplace(id,value);
                const auto definition=input.document.entity("joints",id);const auto type=definition.value("jointType").toString();
                const bool rotational=type=="Revolute"||type=="Cylindrical"||type=="Screw";
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
                channels.scalar(source,id,"Coordinate",rotational?"°":"mm",joint.position);channels.scalar(source,id,"Rate",rotational?"°/s":"mm/s",joint.speed);
            }
        }
        for(const auto& item:system.motion_results){
            bool numeric=false;const int id=QString::fromStdString(item.first).toInt(&numeric);if(!numeric)continue;
            const auto& value=sample(item.second,fi,time);const bool angular=input.document.entity("drives",id).value("motionType").toString()=="Rotation";const double scale=angular?radToDeg:1;
            channels.kinematic("drives",id,value.abscoord);
            channels.scalar("drives",id,"ImposedValue",angular?"°":"mm",value.imposed_value*scale);
            channels.scalar("drives",id,"ImposedSpeed",angular?"°/s":"mm/s",value.imposed_speed*scale);
            channels.scalar("drives",id,"ImposedAcc",angular?"°/s²":"mm/s²",value.imposed_acc*scale);
            channels.add("drives",id,"ActuationForce","N",values(value.actuation_force,.001));channels.add("drives",id,"ActuationTorque","N·mm",values(value.actuation_torque,.001));
            channels.add("drives",id,"ReactionForce","N",values(value.reaction_force,.001));channels.add("drives",id,"ReactionTorque","N·mm",values(value.reaction_torque,.001));
            auto sensorState=value;sensorState.imposed_value*=scale;sensorState.imposed_speed*=scale;sensorState.imposed_acc*=scale;driveStates.emplace(id,sensorState);
        }
        const std::map<std::string,QString> forceCollections{{"SPRING","springs"},{"TORSION_SPRING","springs"},{"TENSION_CABLE","tensionCables"},{"SOFT_STOP","softStops"},{"SCALAR_FORCE","scalarForces"},{"SCALAR_TORQUE","scalarTorques"},{"THREE_POINT_FORCE","threePointForces"},{"THREE_POINT_TORQUE","threePointTorques"},{"FORCE","vectorForces"},{"TORQUE","vectorForces"}};
        for(const auto& item:system.force_results){
            const auto collection=forceCollections.find(item.second.type);if(collection==forceCollections.end())throw std::runtime_error("未知引擎力元结果类型："+item.second.type);
            const int id=std::stoi(item.second.id);const auto& value=sample(item.second,fi,time);const auto& source=collection->second;
            const bool torsion=item.second.type=="TORSION_SPRING";const bool torque=torsion||item.second.type.find("TORQUE")!=std::string::npos;
            const double scale=torsion?radToDeg:1;
            if(value.has_kinematics){channels.scalar(source,id,"Coordinate",torsion?"°":"mm",value.coordinate*scale);channels.scalar(source,id,"Deformation",torsion?"°":"mm",value.deformation*scale);channels.scalar(source,id,"Speed",torsion?"°/s":"mm/s",value.speed*scale);}
            channels.scalar(source,id,"Magnitude",torque?"N·mm":"N",value.magnitude*.001);channels.scalar(source,id,"SignedForce",torque?"N·mm":"N",value.signed_force*.001);
            channels.scalar(source,id,"Engaged","",value.engaged?1:0);channels.scalar(source,id,"Power","W",value.power*1e-6);
            if(value.has_elastic_energy)channels.scalar(source,id,"ElasticEnergy","J",value.elastic_energy*1e-6);
            channels.add(source,id,"PointI","mm",values(value.point_I));channels.add(source,id,"PointJ","mm",values(value.point_J));
            channels.add(source,id,"ForceI","N",values(value.force_I,.001));channels.add(source,id,"ForceJ","N",values(value.force_J,.001));
            channels.add(source,id,"TorqueI","N·mm",values(value.torque_I,.001));channels.add(source,id,"TorqueJ","N·mm",values(value.torque_J,.001));
        }
        for(const auto& item:input.document.collection("sensors")){
            const auto definition=item.toObject();if(!definition.value("enabled").toBool(true))continue;const int id=definition.value("id").toInt();
            auto values=evaluateNativeSensor(definition,bodyStates,jointStates,driveStates);
            if(definition.value("sensorType").toString()=="Angle"){
                const auto last=sensorAngles.find(id);if(last!=sensorAngles.end())values[0]+=360*std::floor((last->second-values[0])/360+.5);sensorAngles[id]=values[0];
            }
            channels.add("sensors",id,"SensorValue",sensorUnit(definition,input.document),values);
        }
        for(const auto& controller:system.controllers){
            if(controller.history.empty())throw std::runtime_error("PID 控制器没有实际采样历史。");
            auto it=std::upper_bound(controller.history.begin(),controller.history.end(),time,[](double t,const NativePidSample& s){return t<s.time;});
            if(it==controller.history.begin())it=controller.history.begin();else --it;
            channels.scalar("controllers",controller.id,"PidTarget","",it->target);channels.scalar("controllers",controller.id,"PidFeedback","",it->feedback);
            channels.scalar("controllers",controller.id,"PidError","",it->error);channels.scalar("controllers",controller.id,"PidOutput","",it->output);
        }
        result.frames.push_back(std::move(frame));
    }
    for(const auto& runtime:flex){
        RigidFlexResult output;output.flexId=runtime.id;output.bodyId=runtime.bodyId;output.referenceNodes=runtime.result.node_x0;output.surfaceTriangles=runtime.result.surface_tris;
        output.tetrahedra=runtime.tetrahedra;
        output.modeCount=runtime.result.n_modes;
        output.interfaceCount=static_cast<int>(runtime.result.interface_bodies.size());
        output.densityKgMm3=runtime.densityKgMm3;
        output.psiInternal=runtime.result.psi_internal;
        if(runtime.interfacePoses.empty())throw std::runtime_error("柔性体没有初始接口位姿。");
        output.initialInterfacePoses=runtime.interfacePoses.front();
        if(output.psiInternal.size()!=output.referenceNodes.size()*static_cast<std::size_t>(6*output.interfaceCount+output.modeCount))
            throw std::runtime_error("柔性体模态矩阵尺寸与节点/接口/模态数不一致。");
        for(const auto& frame:result.frames){
            const auto found=std::lower_bound(runtime.times.begin(),runtime.times.end(),frame.seconds-1e-9);
            if(found==runtime.times.end()||std::abs(*found-frame.seconds)>1e-8)throw std::runtime_error("柔性体采样与刚体结果时间轴不一致。");
            const std::size_t index=static_cast<std::size_t>(found-runtime.times.begin());output.nodePositions.push_back(runtime.nodePositions[index]);
            output.interfacePoses.push_back(runtime.interfacePoses.at(index));
            output.modalCoordinates.push_back(runtime.modalCoordinates.at(index));
            channels.add("autoFlexBodies",runtime.id,"ModalCoordinates","",runtime.modalCoordinates[index]);
        }
        result.flexBodies.push_back(std::move(output));
    }
    channels.validate();
}

} // namespace MotionOcc
