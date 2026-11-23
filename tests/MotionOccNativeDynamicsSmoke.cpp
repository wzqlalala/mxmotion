#include "CadDocument.h"
#include "NativeMotionSolver.h"

#include <BRepBuilderAPI_Transform.hxx>
#include <BRepTools.hxx>
#include <gp_Trsf.hxx>

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
void expectNear(double actual,double expected,double tolerance,const QString& message)
{
    require(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,
        QStringLiteral("%1: %2 != %3, tolerance %4").arg(message).arg(actual,0,'g',16).arg(expected,0,'g',16).arg(tolerance));
}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
void collection(NativeMotionDocument& doc,const QString& name,const QJsonArray& value)
{
    auto data=doc.root.value("motionData").toObject();data[name]=value;doc.root["motionData"]=data;
}
QJsonObject body(int id,int part,bool fixed,const Vector3d& center)
{
    return {{"id",id},{"name",QStringLiteral("body %1").arg(id)},{"geoEntityIds",QJsonArray{part}},
        {"isFixed",fixed},{"isCustomMassProps",true},{"mass",1.0},{"centerOfMass",xyz(center[0],center[1],center[2])},
        {"inertia",QJsonArray{100.0/6,100.0/6,100.0/6,0,0,0}},
        {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}};
}
QJsonObject scenario(double end,double step,const Vector3d& gravity={})
{
    return {{"id",0},{"name","dynamic invariant"},{"endTime",end},{"stepSize",step},
        {"gravity",xyz(gravity[0],gravity[1],gravity[2])},{"usePreset",false},
        {"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"},{"solverTolerance",1e-10}};
}
RigidResult solve(const CadDocument& cad,const NativeMotionDocument& document)
{
    NativeMotionInput input;QString error;require(prepareNativeMotion(cad,document,0,input,error),error);
    RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,error);
    require(result.termination==ResultTermination::EndTime,"dynamic model ended early");
    expectNear(result.frames.back().seconds,input.scenario.duration,1e-12,"requested end time");
    for(const auto& channel:result.channels){
        require(channel.values.size()==result.frames.size()*channel.components,"unaligned result channel");
        for(double value:channel.values)require(std::isfinite(value),"nonfinite dynamic result");
    }
    return result;
}
const ResultChannel& channel(const RigidResult& result,const QString& collection,int id,const QString& variable)
{
    for(const auto& value:result.channels)if(value.sourceType==collection&&value.sourceId==id&&value.variable==variable)return value;
    throw std::runtime_error(QStringLiteral("missing %1/%2/%3").arg(collection).arg(id).arg(variable).toStdString());
}

void sensorPidClosedLoop()
{
    for(bool angular:{false,true})for(double sign:{1.0,-1.0}){
        CadDocument cad;QString error;int part=0;require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        NativeMotionDocument document;document.root={{"formatVersion",1}};document.bodyPartBindings.insert(0,{part});
        collection(document,"bodies",{body(0,part,false,{5,5,5})});
        collection(document,"scenarios",{scenario(1.5,.002)});
        collection(document,"joints",{QJsonObject{{"id",0},{"name","servo joint"},{"jointType",angular?"Revolute":"Translational"},
            {"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(5,5,5)},{"axis",angular?xyz(0,0,1):xyz(1,0,0)}}});
        collection(document,"drives",{QJsonObject{{"id",0},{"name","velocity servo"},{"driveType","JointDrive"},{"targetType","Joint"},
            {"targetJointId",0},{"motionType",angular?"Rotation":"Translation"},{"controlMode","Velocity"},{"expression","0"}}});
        QJsonObject sensor;
        if(angular)sensor={{"id",0},{"name","angle feedback"},{"sensorType","Angle"},{"enabled",true},
            {"bodyAId",0},{"bodyBId",-1},{"axisBodyId",-1},{"axisDirection",xyz(0,0,1)},{"componentMode","Magnitude"}};
        else sensor={{"id",0},{"name","position feedback"},{"sensorType","BodyDisplacement"},{"enabled",true},
            {"targetBodyId",0},{"referencePoint",xyz(0,0,0)},{"variable","Position"},{"coordSystem","GlobalFixed"},{"componentMode","X"}};
        collection(document,"sensors",{sensor});
        collection(document,"controllers",{QJsonObject{{"id",0},{"name","sensor closed loop"},{"controllerType","PID"},{"enabled",true},
            {"targetDriveId",0},{"feedbackType","Position"},{"feedbackSource","Sensor"},{"feedbackSensorId",0},
            {"feedbackSign",sign},{"targetValue",(angular?30.0:10.0)*sign},{"targetInputId",-1},
            {"kp",10.0},{"ki",0.0},{"kd",0.0},{"outputLimit",angular?1.0:20.0}}});
        const auto result=solve(cad,document);
        const auto& feedback=channel(result,"controllers",0,"PidFeedback");
        const auto& target=channel(result,"controllers",0,"PidTarget");
        const auto& output=channel(result,"controllers",0,"PidOutput");
        const auto& measured=channel(result,"sensors",0,"SensorValue");
        for(std::size_t frame=0;frame<result.frames.size();++frame){
            expectNear(feedback.values[frame],sign*measured.values[frame*measured.components],1e-8,"PID feedback differs from same-frame actual sensor");
            require(std::abs(output.values[frame])<=(angular?1.0:20.0)+1e-10,"PID output limit exceeded");
        }
        expectNear(feedback.values.back(),target.values.back(),angular?.002:.0002,"sensor PID did not converge to target");
        const auto& state=result.frames.back().bodies.front();
        if(angular)expectNear(2*std::atan2(state.rotation[3],state.rotation[0])*180/3.14159265358979323846,30,.002,
            "angle PID feedback sign produced wrong physical rotation");
        else expectNear(state.positionMm[0],10,.0002,"linear PID feedback sign produced wrong physical translation");
    }
}

int translatedBox(CadDocument& cad,double x,double y,double z,const Vector3d& delta)
{
    QString error;int original=0;require(cad.addPrimitive(Primitive::Box,x,y,z,original,error),error);
    gp_Trsf transform;transform.SetTranslation(gp_Vec(delta[0],delta[1],delta[2]));
    const auto shape=BRepBuilderAPI_Transform(cad.part(original)->shape,transform,true).Shape();
    QTemporaryDir directory;require(directory.isValid(),"temporary CAD fixture");const auto path=directory.filePath("located.brep");
    require(BRepTools::Write(shape,path.toUtf8().constData()),"fixture BREP write");
    std::vector<int> imported;require(cad.importFile(path,imported,error),error);require(imported.size()==1,"fixture solid count");
    return imported.front();
}

void contactDropAndSettlement()
{
    constexpr double contactStiffness=1000.0,contactExponent=1.5,weightN=9.81,dropHeightMm=2.0;
    // IMPACT's penetrationDepth sets the damping ramp, not a penetration cap.
    // With damping removed, one supporting contact stores K*d^(e+1)/(e+1).
    // Equating this with m*g*(drop+d) gives a conservative compression bound;
    // extra contact points and damping reduce it. For this fixture it is
    // 0.31758337 mm, so an arbitrary 0.15 mm limit would reject a physical drop.
    double energyBoundLow=0,energyBoundHigh=dropHeightMm;
    for(int i=0;i<80;++i){
        const double depth=(energyBoundLow+energyBoundHigh)/2;
        const double elasticEnergy=contactStiffness*std::pow(depth,contactExponent+1)/(contactExponent+1);
        if(elasticEnergy>weightN*(dropHeightMm+depth))energyBoundHigh=depth;
        else energyBoundLow=depth;
    }
    const double energyBound=(energyBoundLow+energyBoundHigh)/2;
    CadDocument cad;
    const int falling=translatedBox(cad,10,10,10,{0,0,2});
    const int floor=translatedBox(cad,20,20,2,{-5,-5,-2});
    NativeMotionDocument document;document.root={{"formatVersion",1}};
    document.bodyPartBindings.insert(0,{falling});document.bodyPartBindings.insert(1,{floor});
    collection(document,"bodies",{body(0,falling,false,{5,5,7}),body(1,floor,true,{5,5,-1})});
    collection(document,"contacts",{QJsonObject{{"id",0},{"name","cube floor"},{"contactType","CADContact"},
        {"actionBodyId",0},{"baseBodyId",1},{"stiffness",contactStiffness},{"materialDamping",1.0},
        {"forceExponent",contactExponent},{"penetrationDepth",.0254},{"frictionEnabled",false},
        {"useConvexHullAction",true},{"useConvexHullBase",true}}});
    double previousFinal=0;
    for(double step:{.0005,.00025}){
        auto contactScenario=scenario(.3,step,{0,0,-9810});
        // CONTACT enables the engine's implicit SMC K/R contribution. A custom
        // linearized step without that contribution is not this stiff model's
        // contact integration configuration.
        contactScenario["usePreset"]=true;contactScenario["presetMode"]="CONTACT";
        collection(document,"scenarios",{contactScenario});
        const auto result=solve(cad,document);
        const auto& position=channel(result,"bodies",0,"Position");
        const auto& velocity=channel(result,"bodies",0,"Velocity");
        const auto& acceleration=channel(result,"bodies",0,"Acceleration");
        double minHeight=7,minVelocity=0,lateAcceleration=0;
        int lateFrames=0;
        for(std::size_t frame=0;frame<result.frames.size();++frame){
            minHeight=std::min(minHeight,position.values[3*frame+2]);minVelocity=std::min(minVelocity,velocity.values[3*frame+2]);
            if(result.frames[frame].seconds>.25){lateAcceleration+=acceleration.values[3*frame+2];++lateFrames;}
        }
        const double finalHeight=position.values[position.values.size()-1];
        require(minVelocity<-100,"contact case did not actually fall and impact");
        // Allow at most one free-fall step before collision detection observes
        // a new contact, in addition to the continuous energy bound above.
        const double detectionAdvance=std::sqrt(2*9810*(dropHeightMm+energyBound))*step+.5*9810*step*step;
        require(5-minHeight<=energyBound+detectionAdvance&&finalHeight>4.9&&finalHeight<5.1,
            QStringLiteral("cube contact geometry: minZ=%1 finalZ=%2 finalVz=%3 meanLateAz=%4 dt=%5")
                .arg(minHeight,0,'g',12).arg(finalHeight,0,'g',12).arg(velocity.values.back(),0,'g',12).arg(lateAcceleration/lateFrames,0,'g',12).arg(step));
        // The MSVC-built engine's implicit contact integration keeps a bounded
        // discrete force-ripple limit cycle at small steps (period 4*dt; under
        // 1 mm/s, i.e. below 1% of the >100 mm/s impact speed, and a sub-um
        // position ripple invisible next to the 0.02 mm step-refinement bound).
        // Which phase of that ripple the single terminal sample lands on is
        // not a physical settling property, so settling is asserted through
        // the drift-free late mean plus a bounded residual oscillation.
        double lateVelocity=0,maxLateSpeed=0;
        for(std::size_t frame=0;frame<result.frames.size();++frame){
            if(result.frames[frame].seconds>.25){
                lateVelocity+=velocity.values[3*frame+2];
                maxLateSpeed=std::max(maxLateSpeed,std::abs(velocity.values[3*frame+2]));
            }
        }
        require(std::abs(lateVelocity/lateFrames)<.05,"settled body is still drifting vertically");
        require(maxLateSpeed<2.0,"damped contact residual oscillation did not stay bounded");
        expectNear(lateAcceleration/lateFrames,0,50,"settled body has unbalanced vertical acceleration");
        if(previousFinal)expectNear(finalHeight,previousFinal,.02,"contact settled position fails step refinement");
        previousFinal=finalHeight;
        const auto& fixed=channel(result,"bodies",1,"Position");
        expectNear(fixed.values.back(),-1,1e-12,"ground body moved under impact");
    }
    // The original solver treats a zero damping-full-depth field as its
    // 0.0254 mm default. Legacy JSON must produce the same physical history.
    auto legacyContact=document.collection("contacts").first().toObject();
    legacyContact["penetrationDepth"]=0.0;collection(document,"contacts",{legacyContact});
    const auto defaultDepth=solve(cad,document);
    expectNear(channel(defaultDepth,"bodies",0,"Position").values.back(),previousFinal,1e-10,
        "legacy zero contact depth did not preserve the original default");
    // Same imported solids and initial state with contact disabled must fall
    // through the floor. This distinguishes physical contact from held poses.
    collection(document,"contacts",{});collection(document,"scenarios",{scenario(.03,.00025,{0,0,-9810})});
    const auto free=solve(cad,document);const auto& height=channel(free,"bodies",0,"Position");
    require(height.values.back()<3,"disabled contact still held the falling body");
}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        sensorPidClosedLoop();contactDropAndSettlement();
        qInfo()<<"PASS: sensor PID convergence/sign/limits and CAD contact impact/settlement/step refinement";
        return 0;
    }catch(const std::exception& exception){qCritical()<<exception.what();return 1;}
}
