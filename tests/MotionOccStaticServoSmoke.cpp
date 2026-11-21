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
void require(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
void near(double value,double expected,double tolerance,const QString& message){require(std::isfinite(value)&&std::abs(value-expected)<=tolerance,
    QStringLiteral("%1: %2 vs %3 (tol %4)").arg(message).arg(value,0,'g',15).arg(expected,0,'g',15).arg(tolerance));}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
void collection(NativeMotionDocument& document,const QString& name,const QJsonArray& values){auto data=document.root.value("motionData").toObject();data[name]=values;document.root["motionData"]=data;}
const ResultChannel& channel(const RigidResult& result,const QString& source,int id,const QString& variable){
    for(const auto& channel:result.channels)if(channel.sourceType==source&&channel.sourceId==id&&channel.variable==variable)return channel;
    throw std::runtime_error(QStringLiteral("missing %1/%2/%3").arg(source).arg(id).arg(variable).toStdString());
}
struct Fixture {
    CadDocument cad;
    NativeMotionDocument document;
    Fixture(bool angular=false){
        QString error;int part=0;require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        document.root={{"formatVersion",1}};document.bodyPartBindings.insert(0,{part});
        collection(document,"bodies",{QJsonObject{{"id",0},{"name","physical body"},{"geoEntityIds",QJsonArray{part}},
            {"mass",1.0},{"isCustomMassProps",true},{"centerOfMass",xyz(5,5,5)},
            {"inertia",QJsonArray{100.0/6,100.0/6,100.0/6,0,0,0}},
            {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}}});
        collection(document,"joints",{QJsonObject{{"id",0},{"jointType",angular?"Revolute":"Translational"},
            {"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(5,5,5)},{"axis",xyz(0,0,1)}}});
        collection(document,"scenarios",{QJsonObject{{"id",0},{"name","static/servo invariant"},{"endTime",.1},{"stepSize",.0005},
            {"gravity",xyz(0,0,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"},
            {"solverTolerance",1e-10},{"analysisType","Dynamic"}}});
    }
    void setting(const QString& key,const QJsonValue& value){auto scenario=document.entity("scenarios",0);scenario[key]=value;collection(document,"scenarios",{scenario});}
    void effort(bool angular,double value,double limit=0){collection(document,"drives",{QJsonObject{{"id",0},{"name","physical actuator"},{"driveType","JointDrive"},
        {"targetType","Joint"},{"targetJointId",0},{"motionType",angular?"Rotation":"Translation"},
        {"controlMode",angular?"Torque":"Force"},{"expression",QString::number(value,'g',17)},{"effortLimit",limit}}});}
    RigidResult solve(){NativeMotionInput input;QString error;const bool prepared=prepareNativeMotion(cad,document,0,input,error);require(prepared,"prepare: "+error);
        RigidResult result;const auto status=solveNativeMotion(input,result,error);require(status==SolveStatus::Completed,"solve: "+error);
        for(const auto& channel:result.channels){require(channel.values.size()==result.frames.size()*channel.components,"channel alignment");for(double value:channel.values)require(std::isfinite(value),"nonfinite channel");}
        return result;}
};

void springEquilibriumAndPreload(){
    Fixture f;
    f.setting("gravity",xyz(0,0,-9810));f.setting("analysisType","Static");
    collection(f.document,"springs",{QJsonObject{{"id",0},{"name","10 N/mm spring"},{"springType","Translational"},
        {"jointId",0},{"stiffness",10.0},{"damping",.05},{"freeValue",20.0},{"installValue",20.0}}});
    const auto equilibrium=f.solve();
    require(equilibrium.termination==ResultTermination::StaticEquilibrium&&equilibrium.frames.size()==1,"static must return one converged equilibrium frame");
    const double expected=5-9.81/10;
    near(equilibrium.frames.front().bodies.front().positionMm[2],expected,1e-7,"kx=mg equilibrium");
    near(equilibrium.frames.front().bodies.front().velocityMmS[2],0,1e-12,"static zero velocity");
    near(std::abs(channel(equilibrium,"springs",0,"SignedForce").values[0]),9.81,1e-6,"static spring supports weight");
    f.setting("analysisType","PreloadedDynamics");
    const auto transient=f.solve();
    require(transient.frames.size()>2&&transient.termination==ResultTermination::EndTime,"preload must continue dynamics");
    for(const auto& frame:transient.frames){near(frame.bodies.front().positionMm[2],expected,2e-6,"preload lost prestress/reference");
        near(frame.bodies.front().velocityMmS[2],0,1e-5,"preload injected an artificial impulse");}
    f.setting("analysisType","Static");f.setting("staticMaxIterations",1);f.setting("staticStepDamping",.5);
    NativeMotionInput limited;QString limitedError;require(prepareNativeMotion(f.cad,f.document,0,limited,limitedError),limitedError);
    RigidResult rejected;rejected.scenarioId=912;
    require(solveNativeMotion(limited,rejected,limitedError)==SolveStatus::Failed&&rejected.scenarioId==912,
        "static iteration exhaustion was falsely marked converged");
    f.setting("staticMaxIterations",100);f.setting("staticStepDamping",1.0);
    require(prepareNativeMotion(f.cad,f.document,0,limited,limitedError),limitedError);
    int polls=0;SolverCallbacks callbacks;callbacks.stopRequested=[&]{return ++polls>5;};
    require(solveNativeMotion(limited,rejected,limitedError,callbacks)==SolveStatus::Stopped&&rejected.scenarioId==912,
        "static cancellation did not preserve output");
    // Delete the physical restoring load. A free vertical guide under gravity
    // has no static equilibrium; a zero update is not successful convergence.
    collection(f.document,"springs",{});f.setting("analysisType","Static");
    NativeMotionInput input;QString error;require(prepareNativeMotion(f.cad,f.document,0,input,error),error);
    RigidResult unchanged;unchanged.scenarioId=717;
    require(solveNativeMotion(input,unchanged,error)==SolveStatus::Failed,"unbalanced free DOF falsely converged");
    require(unchanged.scenarioId==717&&!error.isEmpty(),"failed static changed output");
}

void torsionPreload(){
    Fixture f(true);f.setting("analysisType","Static");f.effort(true,.1);
    collection(f.document,"springs",{QJsonObject{{"id",0},{"springType","Torsional"},{"jointId",0},
        {"stiffness",.01},{"freeValue",0},{"installValue",0},{"damping",0}}});
    const auto result=f.solve();
    near(channel(result,"drives",0,"Coordinate").values.front(),10,1e-5,"torsional static T/k in public degrees");
    near(channel(result,"drives",0,"EffortApplied").values.front(),.1,1e-12,"static force actuator load");
}

void supportedWeightReaction(){
    Fixture f(true);f.setting("analysisType","Static");f.setting("gravity",xyz(0,0,-9810));
    const auto result=f.solve();
    near(result.frames.front().bodies.front().positionMm[2],5,1e-12,"joint support displaced in static solve");
    near(std::abs(channel(result,"joints",0,"ReactionForce").values[2]),9.81,1e-9,"static force residual omitted support reaction");
}

void contactPreload(){
    Fixture f;
    QString error;int source=0;require(f.cad.addPrimitive(Primitive::Box,20,20,2,source,error),error);
    gp_Trsf placement;placement.SetTranslation(gp_Vec(-5,-5,-2));
    const auto located=BRepBuilderAPI_Transform(f.cad.part(source)->shape,placement,true).Shape();
    QTemporaryDir directory;require(directory.isValid(),"contact fixture directory");
    const auto file=directory.filePath("floor.brep");require(BRepTools::Write(located,file.toUtf8().constData()),"contact fixture BREP");
    std::vector<int> parts;require(f.cad.importFile(file,parts,error),error);require(parts.size()==1,"contact floor part");
    auto ground=f.document.entity("bodies",0);ground["id"]=1;ground["name"]="floor";ground["isFixed"]=true;
    ground["geoEntityIds"]=QJsonArray{parts.front()};ground["centerOfMass"]=xyz(5,5,-1);
    auto bodies=f.document.collection("bodies");bodies.append(ground);collection(f.document,"bodies",bodies);f.document.bodyPartBindings.insert(1,{parts.front()});
    collection(f.document,"contacts",{QJsonObject{{"id",0},{"contactType","CADContact"},{"actionBodyId",0},{"baseBodyId",1},
        {"stiffness",1000.0},{"materialDamping",1.0},{"forceExponent",1.5},{"penetrationDepth",.0254},
        {"frictionEnabled",false},{"useConvexHullAction",true},{"useConvexHullBase",true}}});
    f.setting("gravity",xyz(0,0,-9810));f.setting("analysisType","Static");
    f.setting("usePreset",true);f.setting("presetMode","CONTACT");
    // Bullet contact geometry uses lower precision than the FEM coordinates.
    // Request and verify an explicit 0.01% force residual for this contact case.
    f.setting("staticTolerance",1e-4);
    const auto equilibrium=f.solve();
    const double height=equilibrium.frames.front().bodies.front().positionMm[2];
    require(height<5&&height>4.9,"static contact did not find a compressed supporting contact");
    near(channel(equilibrium,"joints",0,"ReactionForce").values[2],0,1e-5,"guide incorrectly supported weight instead of contact");
    f.setting("analysisType","PreloadedDynamics");f.setting("endTime",.02);f.setting("stepSize",.0001);
    const auto transient=f.solve();
    for(const auto& frame:transient.frames){near(frame.bodies.front().positionMm[2],height,1e-5,"contact preload changed during continuation");
        near(frame.bodies.front().velocityMmS[2],0,.01,"contact preload introduced impulse");}
}

void effortAccelerationAndLimits(){
    for(bool angular:{false,true}){
        Fixture f(angular);f.effort(angular,angular?.1:2,angular?.05:1);
        f.setting("endTime",.05);f.setting("stepSize",.0001);
        const auto result=f.solve();
        const double effort=angular?.05:1;
        const auto& applied=channel(result,"drives",0,"EffortApplied");
        const auto& requested=channel(result,"drives",0,"EffortCommand");
        for(std::size_t i=0;i<result.frames.size();++i){near(applied.values[i],effort,1e-12,"physical effort saturation");near(requested.values[i],2*effort,1e-12,"effort request retained");}
        const auto& final=result.frames.back().bodies.front();
        if(angular)near(final.angularVelocityDegS[2],effort*1000/(100.0/6)*.05*180/3.14159265358979323846,.005,"torque produces T/I acceleration");
        else near(final.velocityMmS[2],1000*effort*.05,.005,"force produces F/m acceleration");
        const auto& actuator=channel(result,"drives",0,angular?"ActuationTorque":"ActuationForce");
        near(actuator.values.back(),-effort,1e-10,"actuation wrench is the physical J-side load");
    }
    Fixture f;f.effort(false,20,10);
    auto drive=f.document.entity("drives",0);drive["powerLimit"]=.01;drive["noLoadSpeed"]=20;collection(f.document,"drives",{drive});
    const auto result=f.solve();
    const auto& power=channel(result,"drives",0,"MechanicalPower");
    const auto& applied=channel(result,"drives",0,"EffortApplied");
    for(std::size_t i=0;i<result.frames.size();++i){require(std::abs(power.values[i])<=.0100000001,"power limit exceeded");require(std::abs(applied.values[i])<=10.0000001,"effort limit exceeded");}
    require(result.frames.back().bodies.front().velocityMmS[2]<20.01,"no-load speed envelope ignored");
}

void effortServo(){
    for(bool angular:{false,true})for(double sign:{1.0,-1.0}){
        Fixture f(angular);f.effort(angular,0,angular?1:2);f.setting("endTime",angular?2:1.5);f.setting("stepSize",.0005);
        collection(f.document,"controllers",{QJsonObject{{"id",0},{"controllerType","PID"},{"enabled",true},{"targetDriveId",0},
            {"feedbackSource","DriveDof"},{"feedbackType","Position"},{"targetValue",sign*(angular?30:10)},{"feedbackSign",sign},
            {"kp",angular?.01:.1},{"ki",0},{"kd",angular?.0034:.02},{"outputLimit",angular?.5:1}}});
        const auto result=f.solve();
        const auto& target=channel(result,"controllers",0,"PidTarget");const auto& feedback=channel(result,"controllers",0,"PidFeedback");
        const auto& output=channel(result,"controllers",0,"PidOutput");
        near(feedback.values.back(),target.values.back(),angular?.03:.003,"effort PID settles with true inertia");
        for(double value:output.values)require(std::abs(value)<=(angular?.5:1)+1e-12,"effort PID saturation exceeded");
        require(output.unit==(angular?QStringLiteral("N·mm"):QStringLiteral("N")),"effort PID result units");
        require(feedback.unit==(angular?QStringLiteral("°"):QStringLiteral("mm")),"effort feedback result units");
        near(channel(result,"drives",0,"Coordinate").values.back(),angular?30:10,angular?.03:.003,"feedback sign changed intended physical travel");
        // A real force actuator does not teleport to its target at the first
        // step. This catches accidentally reusing a prescribed-speed motor.
        require(std::abs(feedback.values[1])<.01,"force servo prescribed a kinematic coordinate");
    }
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    springEquilibriumAndPreload();torsionPreload();supportedWeightReaction();contactPreload();effortAccelerationAndLimits();effortServo();
    qInfo()<<"Static equilibrium, preload preservation, no-equilibrium rejection, actual F/T motors and limited servos passed";return 0;
}catch(const std::exception& error){qCritical()<<error.what();return 1;}}
