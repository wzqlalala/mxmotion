#include "NativeMotionRuntime.h"
#include "motion/physics/LinkMotorRotation.h"

#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>

namespace MotionOcc {
namespace {

using Vec=motion::MotionVector<>;
constexpr double radToDeg=57.2957795130823208768;

struct AssemblySettings {
    NativeMotionSystem& system;
    const int iterations;
    const double tolerance,step;
    explicit AssemblySettings(NativeMotionSystem& value)
        :system(value),iterations(value.GetSolverMaxIterations()),tolerance(value.GetSolverTolerance()),step(value.GetStep())
    {
        system.SetSolverMaxIterations(std::max(iterations,300));
        system.SetSolverTolerance(1e-10);
        system.SetStep(1e-6);
    }
    ~AssemblySettings(){system.SetSolverMaxIterations(iterations);system.SetSolverTolerance(tolerance);system.SetStep(step);}
};

Vec vector(const QJsonValue& value,Vec fallback=motion::VECT_Z)
{
    if(value.isUndefined()||value.isNull())return fallback;
    if(value.isArray()){const auto a=value.toArray();return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble()};}
    const auto o=value.toObject();return {o.value("x").toDouble(),o.value("y").toDouble(),o.value("z").toDouble()};
}

} // namespace

void NativeMotionSystem::projectAcceptedPositions()
{
    auto& system=*this;
    // Same equations as MotionAssemblyAnalysis::POSITION, with no force/time
    // integration and no change to the user's solver or integrator. Prepare the
    // descriptor once, not by repeating DoAssembly's complete setup per Newton
    // iteration. The cap remains explicitly configurable in the scenario.
    system.Setup();system.Update();
    AssemblySettings settings(system);
    system.DescriptorPrepareInject(*system.GetSystemDescriptor());
    motion::MotionState position;
    motion::MotionStateDelta velocity,correction;
    position.setZero(system.GetNcoords_x(),&system);
    velocity.setZero(system.GetNcoords_v(),&system);
    correction.setZero(system.GetNcoords_v(),&system);
    motion::MotionVectorDynamic<> residual(system.GetNcoords_v()),constraints(system.GetNconstr()),reactions(system.GetNconstr());
    residual.setZero();
    for(int iteration=0;iteration<system.positionIterations;++iteration){
        double time=0;system.StateGather(position,velocity,time);
        constraints.setZero();system.LoadConstraint_C(constraints,1.0,false);
        if(!constraints.allFinite())throw std::runtime_error("位置约束残差不是有限数值。");
        // This is only a convergence test, not a reported physical error:
        // native residual rows mix mm and angular direction components. The
        // independent acceptance check below reports each in its own unit.
        if(constraints.size()==0||constraints.cwiseAbs().maxCoeff()<=1e-8)break;
        correction.setZero(system.GetNcoords_v(),&system);reactions.setZero();
        if(!system.StateSolveCorrection(correction,reactions,residual,constraints,1.0,0.0,0.0,
                                        position,velocity,time,false,false,true)||!correction.allFinite())
            throw std::runtime_error("位置约束投影求解失败。请检查闭环约束与求解器设置。");
        position+=correction;
        system.StateScatter(position,velocity,time,true);
    }
}

void validateNativeAcceptedConstraints(const NativeMotionSystem& system)
{
    if(!system.sourceInput)return;
    const auto& input=*system.sourceInput;
    std::map<int,Vec> centers;
    double scale=1;
    for(const auto& body:input.bodies){
        const Vec center(body.properties.centerMm[0],body.properties.centerMm[1],body.properties.centerMm[2]);
        centers.emplace(body.body.id,center);scale=std::max(scale,center.Length());
    }
    const double linearTolerance=std::max(.05,scale*1e-4);
    constexpr double angularToleranceDeg=.01;
    const auto rotation=[&](int id){return id<0?motion::QUNIT:system.nativeBodies.at(id)->GetRot();};
    const auto point=[&](int id,const Vec& original){
        return id<0?original:system.nativeBodies.at(id)->GetPos()+rotation(id).Rotate(original-centers.at(id));
    };
    const auto axis=[&](int id,Vec direction){direction.Normalize();return rotation(id).Rotate(direction);};
    const auto report=[&](const QJsonObject& joint,double error,double tolerance,const QString& unit,const QString& kind){
        if(std::isfinite(error)&&error<=tolerance)return;
        const QString message=QStringLiteral("求解方案 %1：运动副 %2（%3，%4）在 t=%5 s 的%6误差为 %7 %8，超过 %9 %8。已中止，未输出违反约束的动画。请核对闭环/冗余运动副的原点与轴向；也可减小时间步或增加位置迭代次数。")
            .arg(input.scenario.id).arg(joint.value("id").toInt()).arg(joint.value("name").toString(),joint.value("jointType").toString())
            .arg(system.GetMotionTime(),0,'g',12).arg(kind).arg(error,0,'g',12).arg(unit).arg(tolerance,0,'g',12);
        throw std::runtime_error(message.toStdString());
    };
    for(const auto& value:input.document.collection("joints")){
        const auto joint=value.toObject();const auto type=joint.value("jointType").toString();
        const int a=joint.value("actionBodyId").toInt(-1),b=joint.value("baseBodyId").toInt(-1);
        const Vec origin=vector(joint.value("origin"),motion::VNULL),difference=point(a,origin)-point(b,origin);
        double linearError=0,angularError=0;
        if(type=="Revolute"||type=="Spherical"||type=="Fixed"||type=="Universal"||type=="Hooke")
            linearError=difference.Length();
        else if(type=="Translational"||type=="Cylindrical"||type=="PointLine"){
            const auto direction=axis(b,vector(joint.value("axis")));
            linearError=(difference-direction*difference.Dot(direction)).Length();
        }else if(type=="Planar"||type=="PointPlane")
            linearError=std::abs(difference.Dot(axis(b,vector(joint.value("axis")))));
        if(type=="Revolute"||type=="Cylindrical"||type=="Planar"){
            const auto i=axis(a,vector(joint.value("axis"))),j=axis(b,vector(joint.value("axis")));
            angularError=std::atan2(i.Cross(j).Length(),i.Dot(j))*radToDeg;
        }else if(type=="Universal"||type=="Hooke"){
            const auto i=axis(a,vector(joint.value("actionPin"),motion::VECT_X)),j=axis(b,vector(joint.value("basePin"),motion::VECT_Y));
            angularError=std::asin(std::clamp(std::abs(i.Dot(j)),0.0,1.0))*radToDeg;
        }else if(type=="Fixed"||type=="Translational"||type=="Direction"){
            const auto i=rotation(a),j=rotation(b);
            const double dot=std::abs(i.e0()*j.e0()+i.e1()*j.e1()+i.e2()*j.e2()+i.e3()*j.e3());
            angularError=2*std::acos(std::clamp(dot,0.0,1.0))*radToDeg;
        }
        report(joint,linearError,linearTolerance,QStringLiteral("mm"),QStringLiteral("闭合"));
        report(joint,angularError,angularToleranceDeg,QStringLiteral("°"),QStringLiteral("方向"));
    }
    // ParserAdams creates FREE-guide motors because their passive joint
    // already supplies the other constraints. Their sole residual is mm for
    // translation or sin(angle-error/2) for rotation, including speed motors'
    // integrated target. This detects contradictory drives even when passive
    // joints happen to close after a least-squares solve.
    for(const auto& item:system.nativeMotors){
        const auto residual=item.second->GetConstraintViolation();
        if(system.effortMotors.count(item.first)){
            if(residual.size()!=0)throw std::runtime_error("力/力矩执行器意外增加了运动约束，已中止以避免伪伺服。");
            continue;
        }
        if(residual.size()!=1)throw std::runtime_error("驱动的约束维度与原生适配约定不一致。");
        const bool angular=bool(std::dynamic_pointer_cast<motion::MotionLinkMotorRotation>(item.second));
        const double error=angular?2*std::asin(std::clamp(std::abs(residual[0]),0.0,1.0))*radToDeg:std::abs(residual[0]);
        const double tolerance=angular?angularToleranceDeg:linearTolerance;
        if(std::isfinite(residual[0])&&std::isfinite(error)&&error<=tolerance)continue;
        const auto drive=input.document.entity("drives",item.first);
        throw std::runtime_error(QStringLiteral("求解方案 %1：驱动 %2（目标运动副 %3）在 t=%4 s 的约束误差为 %5 %6，超过 %7 %6。驱动与其他约束可能冲突，已中止，未输出违反约束的动画。")
            .arg(input.scenario.id).arg(item.first).arg(drive.value("targetJointId").toInt(-1))
            .arg(system.GetMotionTime(),0,'g',12).arg(error,0,'g',12).arg(angular?QStringLiteral("°"):QStringLiteral("mm")).arg(tolerance,0,'g',12).toStdString());
    }
}

} // namespace MotionOcc
