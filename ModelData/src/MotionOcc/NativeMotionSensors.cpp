#include "NativeMotionRuntime.h"

#include <QJsonArray>
#include <QQuaternion>

#include <cmath>
#include <stdexcept>

namespace MotionOcc {
namespace {
using Vec=motion::MotionVector<>;
using Quat=motion::MotionQuaternion<>;
constexpr double radToDeg=57.295779513082320876;
Vec vec(const QJsonValue& value)
{
    if(value.isArray()){const auto a=value.toArray();return Vec(a[0].toDouble(),a[1].toDouble(),a[2].toDouble());}
    const auto o=value.toObject();return Vec(o.value("x").toDouble(),o.value("y").toDouble(),o.value("z").toDouble());
}
Vec vec(const motion::MotionVec3& value){return Vec(value.x,value.y,value.z);}
std::vector<double> values(const Vec& v){return {v.x(),v.y(),v.z()};}
const motion::state_kinematic* body(const std::map<int,motion::state_kinematic>& bodies,int id)
{
    if(id<0)return nullptr;
    const auto it=bodies.find(id);if(it==bodies.end())throw std::runtime_error("传感器引用的运动体结果不存在。");return &it->second;
}
Vec position(const motion::state_kinematic* b,const Vec& p){return b?vec(b->pos)+b->rpos.Rotate(p):p;}
Vec velocity(const motion::state_kinematic* b,const Vec& p){return b?vec(b->vel)+Vcross(vec(b->rvel),b->rpos.Rotate(p)):motion::VNULL;}
Vec acceleration(const motion::state_kinematic* b,const Vec& p)
{
    if(!b)return motion::VNULL;const auto r=b->rpos.Rotate(p);
    return vec(b->acc)+Vcross(vec(b->racc),r)+Vcross(vec(b->rvel),Vcross(vec(b->rvel),r));
}
Vec euler(const Quat& q){const auto e=QQuaternion(q.e0(),q.e1(),q.e2(),q.e3()).toEulerAngles();return Vec(e.x(),e.y(),e.z());}
}

std::vector<double> evaluateNativeSensor(const QJsonObject& o,const std::map<int,motion::state_kinematic>& bodies,
                                       const std::map<int,motion::joint_result>& joints,
                                       const std::map<int,motion::motion_result>& drives)
{
    const auto type=o.value("sensorType").toString(),variable=o.value("variable").toString();
    const auto coordinate=[&](Vec v){
        if(o.value("coordSystem").toString()=="BodyFixed"){
            const auto* ref=body(bodies,o.value("referenceBodyId").toInt(-1));if(ref)v=ref->rpos.RotateBack(v);
        }return values(v);
    };
    if(type=="BodyDisplacement"||type=="BodyVelocity"||type=="BodyAcceleration"){
        const auto* b=body(bodies,o.value("targetBodyId").toInt(-1));if(!b)throw std::runtime_error("体传感器必须绑定运动体。");
        const auto p=vec(o.value("referencePoint"));
        if(variable=="Position")return coordinate(position(b,p));
        if(variable=="Velocity")return coordinate(velocity(b,p));
        if(variable=="Acceleration")return coordinate(acceleration(b,p));
        if(variable=="AngularVelocity")return coordinate(vec(b->rvel)*radToDeg);
        if(variable=="AngularAcceleration")return coordinate(vec(b->racc)*radToDeg);
        if(variable=="Rotation")return values(euler(b->rpos));
    }else if(type=="Distance"){
        const auto pa=o.value("pointA").toObject(),pb=o.value("pointB").toObject();
        const auto* a=body(bodies,pa.value("bodyId").toInt(-1));const auto* b=body(bodies,pb.value("bodyId").toInt(-1));
        const auto la=vec(pa.value("localPos")),lb=vec(pb.value("localPos"));
        const auto r=position(b,lb)-position(a,la);const double length=r.Length();
        if(variable=="Distance"||variable.isEmpty())return {length};
        const auto v=velocity(b,lb)-velocity(a,la);
        const double rate=length>1e-12?r.Dot(v)/length:0;
        if(variable=="RelVelocity")return {rate};
        if(variable=="RelAcceleration"){
            const auto ac=acceleration(b,lb)-acceleration(a,la);
            return {length>1e-12?(v.Dot(v)+r.Dot(ac)-rate*rate)/length:0};
        }
    }else if(type=="Angle"){
        const auto* a=body(bodies,o.value("bodyAId").toInt(-1));const auto* b=body(bodies,o.value("bodyBId").toInt(-1));
        const auto qa=a?a->rpos:motion::QUNIT,qb=b?b->rpos:motion::QUNIT;
        auto axis=vec(o.value("axisDirection"));if(axis.Length()<1e-12)throw std::runtime_error("角度传感器轴向为零。");axis.Normalize();
        const auto* axisBody=body(bodies,o.value("axisBodyId").toInt(-1));if(axisBody)axis=axisBody->rpos.Rotate(axis);
        // Both relative rotation and axis are expressed in the world frame.
        const auto relative=qa*qb.GetConjugate();
        return {2*std::atan2(axis.x()*relative.e1()+axis.y()*relative.e2()+axis.z()*relative.e3(),relative.e0())*radToDeg};
    }else if(type=="JointReaction"){
        const auto it=joints.find(o.value("targetJointId").toInt(-1));if(it==joints.end())throw std::runtime_error("传感器引用的运动副结果不存在。");
        if(variable=="ReactionForce")return coordinate(vec(it->second.reaction_force)*.001);
        if(variable=="ReactionTorque")return coordinate(vec(it->second.reaction_torque)*.001);
    }else if(type=="DriveActuation"){
        const auto it=drives.find(o.value("targetDriveId").toInt(-1));if(it==drives.end())throw std::runtime_error("传感器引用的驱动结果不存在。");const auto& r=it->second;
        if(variable=="ActuationForce")return coordinate(vec(r.actuation_force)*.001);
        if(variable=="ActuationTorque")return coordinate(vec(r.actuation_torque)*.001);
        if(variable=="ReactionForce")return coordinate(vec(r.reaction_force)*.001);
        if(variable=="ReactionTorque")return coordinate(vec(r.reaction_torque)*.001);
        if(variable=="ImposedValue")return {r.imposed_value};
        if(variable=="ImposedSpeed")return {r.imposed_speed};
        if(variable=="ImposedAcc")return {r.imposed_acc};
    }
    throw std::runtime_error(QStringLiteral("传感器 %1 的类型/变量没有求值实现：%2/%3。").arg(o.value("id").toInt()).arg(type,variable).toStdString());
}

} // namespace MotionOcc
