#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionRuntime.h"
#include "NativeMotionSolver.h"
#include "RigidMotion.h"

#include "motion/solver/SystemDescriptor.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool condition,const QString& message)
{
    if(!condition)throw std::runtime_error(message.toStdString());
}
void expectNear(double actual,double expected,double tolerance,const QString& message)
{
    require(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,
        QStringLiteral("%1: %2 != %3").arg(message).arg(actual,0,'g',16).arg(expected,0,'g',16));
}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}

void contourSemantics()
{
    RigidFlexResult flex;
    flex.referenceNodes={0,0,0, 4,0,0, 0,1,0, 0,0,1};
    flex.tetrahedra={0,1,2,3};flex.densityKgMm3=.001;
    flex.interfaceCount=1;flex.modeCount=2;
    // Nonzero boundary columns expose accidental inclusion of rigid/interface
    // motion. Different X/Y row contributions expose a wrong row stride.
    flex.psiInternal.assign(12*8,1000);
    for(int node=0;node<4;++node)for(int axis=0;axis<3;++axis){
        flex.psiInternal[(3*node+axis)*8+6]=axis==0?.1*node:0;
        flex.psiInternal[(3*node+axis)*8+7]=axis==1?.2*node:0;
    }
    flex.modalCoordinates={{0,0},{2,3}};
    flex.nodePositions={flex.referenceNodes,flex.referenceNodes};
    for(auto& frame:flex.nodePositions)for(std::size_t node=0;node<4;++node){
        const double x=frame[3*node],y=frame[3*node+1];
        frame[3*node]=20-y;frame[3*node+1]=-10+x;frame[3*node+2]+=3;
    }
    std::vector<double> values;QString error;
    require(flexNodeScalars(flex,0,FlexContourKind::ElasticDisplacement,values,error),error);
    for(double value:values)expectNear(value,0,0,"rigid rotation/translation leaked into elastic contour");
    require(flexNodeScalars(flex,1,FlexContourKind::ElasticDisplacement,values,error),error);
    for(int node=0;node<4;++node)expectNear(values[node],node*std::sqrt(.4),1e-14,"modal rows/columns");
    require(flexNodeScalars(flex,0,FlexContourKind::TotalDisplacement,values,error),error);
    expectNear(values[0],std::sqrt(509.0),1e-12,"total displacement includes rigid motion");
    require(flexNodeScalars(flex,0,FlexContourKind::LegacyFreeModeProjection,values,error),error);
    for(double value:values)expectNear(value,0,1e-14,"zero elastic mode must give zero legacy projection");
    require(flexNodeScalars(flex,1,FlexContourKind::LegacyFreeModeProjection,values,error),error);
    const auto baseline=values;
    for(double& q:flex.modalCoordinates[1])q*=3;
    require(flexNodeScalars(flex,1,FlexContourKind::LegacyFreeModeProjection,values,error),error);
    for(int node=0;node<4;++node)expectNear(values[node],3*baseline[node],1e-12,"legacy projection scaling");
    flex.modalCoordinates[0]={-6,-9};
    require(flexNodeScalarsAt(flex,0,1,.5,FlexContourKind::ElasticDisplacement,values,error),error);
    for(double value:values)expectNear(value,0,0,"interpolated opposite signed modes must have zero magnitude");
    const auto preserved=values;
    flex.nodePositions[1].pop_back();
    require(!flexNodeScalars(flex,1,FlexContourKind::TotalDisplacement,values,error)&&values==preserved,
        "malformed node frame was silently accepted or replaced valid scalars");
}

NativeMotionInput preparedCantilever(CadDocument& cad)
{
    QString error;int part=0;
    require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
    QJsonObject body{{"id",0},{"name","flex body"},{"geoEntityIds",QJsonArray{part}},
        {"isFixed",false},{"mass",1.0},{"isCustomMassProps",true},{"centerOfMass",xyz(5,5,5)},
        {"inertia",QJsonArray{100.0/6,100.0/6,100.0/6,0,0,0}},
        {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}};
    QJsonObject joint{{"id",0},{"name","root clamp"},{"jointType","Fixed"},{"actionBodyId",0},
        {"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(0,0,1)}};
    QJsonObject scenario{{"id",0},{"name","flex result test"},{"endTime",.002},{"stepSize",.0001},
        {"gravity",xyz(0,-1000,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},
        {"solverType","MINRES"},{"solverTolerance",1e-10}};
    QJsonObject flex{{"id",0},{"name","cantilever"},{"targetBodyId",0},{"enabled",true},
        {"modeCount",2},{"dampingType",1},{"modalDampingRatio",.02},
        {"material",QJsonObject{{"youngModulus",100.0},{"poissonRatio",.3},{"density",.001}}},
        {"mesh",QJsonObject{{"nodes",QJsonArray{QJsonArray{0,0,0},QJsonArray{10,0,0},QJsonArray{10,10,0},QJsonArray{0,10,0},QJsonArray{0,0,10},QJsonArray{10,0,10},QJsonArray{10,10,10},QJsonArray{0,10,10}}},
            {"tetrahedra",QJsonArray{QJsonArray{0,1,3,4},QJsonArray{1,2,3,6},QJsonArray{1,3,4,6},QJsonArray{1,4,5,6},QJsonArray{3,4,6,7}}}}},
        // Explicit QJsonValue keeps [[0,3,4,7]] nested on MSVC as well as GCC
        // (single same-type braced element otherwise copy-constructs a flat array).
        {"attachGroups",QJsonArray{QJsonValue(QJsonArray{0,3,4,7})}}};
    NativeMotionDocument doc;
    doc.root={{"formatVersion",1},{"motionData",QJsonObject{{"bodies",QJsonArray{body}},
        {"joints",QJsonArray{joint}},{"scenarios",QJsonArray{scenario}},{"autoFlexBodies",QJsonArray{flex}}}}};
    doc.bodyPartBindings.insert(0,{part});
    NativeMotionInput input;require(prepareNativeMotion(cad,doc,0,input,error),error);return input;
}

void completePayloadAndPhysicalMotion()
{
    CadDocument cad;auto input=preparedCantilever(cad);RigidResult result;QString error;
    const auto status=solveNativeMotion(input,result,error);
    require(status==SolveStatus::Completed,error);
    require(result.termination==ResultTermination::EndTime&&result.frames.size()==21,"cantilever terminal frame");
    require(result.flexBodies.size()==1,"cantilever missing flexible output");
    const auto& flex=result.flexBodies.front();
    require(flex.referenceNodes.size()==24&&flex.tetrahedra==input.geometry->flexBodies[0].tetrahedra,
        "original tetrahedral payload missing");
    require(flex.modeCount==2&&flex.interfaceCount==1&&flex.psiInternal.size()==24*8,
        "unique-interface modal matrix shape");
    require(flex.initialInterfacePoses.size()==7&&flex.initialInterfacePoses==flex.interfacePoses.front(),
        "initial interface pose was captured at the end of the run");
    expectNear(flex.initialInterfacePoses[0],5,1e-8,"parser body origin must not move away from original COM");
    require(flex.nodePositions.size()==21&&flex.interfacePoses.size()==21&&flex.modalCoordinates.size()==21,
        "incomplete frame payload");
    for(std::size_t frame=0;frame<result.frames.size();++frame){
        require(flex.interfacePoses[frame].size()==7&&flex.modalCoordinates[frame].size()==2,"per-frame pose/modal sizes");
        for(int component=0;component<3;++component)
            expectNear(flex.interfacePoses[frame][component],5,1e-7,"clamped interface moved");
    }
    std::vector<double> elastic;require(flexNodeScalars(flex,20,FlexContourKind::ElasticDisplacement,elastic,error),error);
    require(*std::max_element(elastic.begin(),elastic.end())>1e-8,"gravity did not produce any elastic response");
    // Every scalar channel uses the same accepted times as the full modal data.
    for(const auto& channel:result.channels)
        require(channel.values.size()==result.frames.size()*channel.components,"result channel time-axis mismatch");
}

void divergenceKeepsCleanResults()
{
    CadDocument cad;auto input=preparedCantilever(cad);
    // A real, deliberately excessive acceleration generates an invalid trial
    // modal state. The guard must run before any rigid/force result is recorded.
    input.scenario.gravityMS2={0,-1e15,0};
    RigidResult result;QString error;
    const auto status=solveNativeMotion(input,result,error);
    require(status==SolveStatus::Completed,"divergence discarded all clean frames: "+error);
    require(result.termination==ResultTermination::FlexDivergence,"missing distinct modal-divergence termination");
    require(!result.frames.empty()&&result.frames.back().seconds<input.scenario.duration,"rejected trial was recorded");
    require(!result.diagnostics.isEmpty(),"partial modal result has no diagnostic");
    for(const auto& flex:result.flexBodies){
        require(flex.nodePositions.size()==result.frames.size()&&flex.interfacePoses.size()==result.frames.size()
            &&flex.modalCoordinates.size()==result.frames.size(),"partial flexible results not synchronized");
        for(const auto& q:flex.modalCoordinates)for(double value:q)
            require(std::isfinite(value)&&std::abs(value)<=1000,"bad modal sample survived rejection");
    }
    for(const auto& channel:result.channels)
        require(channel.values.size()==result.frames.size()*channel.components,"bad rigid/load sample survived rejection");
}

void multipleFlexCaptureIsAtomic()
{
    std::vector<NativeFlexRuntime> flex(2);
    for(int i=0;i<2;++i){
        flex[i].id=i;flex[i].result.n_modes=1;
        flex[i].result.assembly=std::make_shared<motion::modal::MotionModalAssembly>();
        flex[i].result.assembly->Get_modal_q().resize(1);
        flex[i].result.assembly->Get_modal_q()[0]=0;
    }
    flex[1].result.assembly->Get_modal_q()[0]=std::numeric_limits<double>::quiet_NaN();
    bool rejected=false;
    try{captureNativeFlex(flex,.1);}catch(const NativeFlexDivergence& failure){rejected=failure.flexId==1;}
    require(rejected,"second invalid flexible body was not rejected");
    for(const auto& runtime:flex)
        require(runtime.times.empty()&&runtime.modalCoordinates.empty()&&runtime.nodePositions.empty()&&runtime.interfacePoses.empty(),
            "earlier flexible body appended a frame before later body validation");
}

void nativeReactionConvention()
{
    CadDocument cad;auto input=preparedCantilever(cad);
    auto geometry=std::make_shared<NativeMotionGeometry>(*input.geometry);
    geometry->flexBodies.clear();input.geometry=geometry;input.scenario.gravityMS2={0,0,-9.81};
    RigidResult result;QString error;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,error);
    bool found=false;
    for(const auto& channel:result.channels)if(channel.sourceType=="joints"&&channel.variable=="ReactionForce"){
        expectNear(channel.values[channel.values.size()-1],-9.81,1e-8,"raw reaction is force on J/support, not I");found=true;
    }
    require(found,"missing raw joint reaction channel");
}

void rotatingOffsetFlexInterface()
{
    CadDocument cad;auto input=preparedCantilever(cad);auto document=input.document;
    auto data=document.root.value("motionData").toObject();
    auto joint=data.value("joints").toArray()[0].toObject();joint["jointType"]="Revolute";
    data["joints"]=QJsonArray{joint};
    data["drives"]=QJsonArray{QJsonObject{{"id",0},{"name","rotating clamp"},{"driveType","JointDrive"},
        {"targetType","Joint"},{"targetJointId",0},{"motionType","Rotation"},{"controlMode","Velocity"},{"expression","90"}}};
    auto scenario=data.value("scenarios").toArray()[0].toObject();
    scenario["endTime"]=.2;scenario["stepSize"]=.0005;scenario["gravity"]=xyz(0,0,0);
    data["scenarios"]=QJsonArray{scenario};document.root["motionData"]=data;
    QString error;require(prepareNativeMotion(cad,document,0,input,error),error);
    RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,error);
    require(result.termination==ResultTermination::EndTime&&result.frames.size()==401,"rotating flex did not reach 18 degrees");
    const auto& flex=result.flexBodies.front();
    double maxRootError=0;
    for(std::size_t frame=0;frame<result.frames.size();frame+=20){
        const double angle=result.frames[frame].seconds*3.14159265358979323846/2;
        const auto& pose=flex.interfacePoses[frame];
        expectNear(pose[0],5*std::cos(angle),1e-5,"offset interface COM circular X path");
        expectNear(pose[1],5+5*std::sin(angle),1e-5,"offset interface COM circular Y path");
        const motion::MotionQuaternion<> rotation(pose[3],pose[4],pose[5],pose[6]);
        const motion::MotionVector<> center(pose[0],pose[1],pose[2]);
        for(int node:{0,3,4,7}){
            const auto& x0=flex.referenceNodes;const auto& actual=flex.nodePositions[frame];
            const auto expected=center+rotation.Rotate(motion::MotionVector<>(x0[3*node]-5,x0[3*node+1]-5,x0[3*node+2]-5));
            maxRootError=std::max(maxRootError,std::hypot(actual[3*node]-expected.x(),actual[3*node+1]-expected.y(),actual[3*node+2]-expected.z()));
        }
    }
    require(maxRootError<.002,QStringLiteral("rotating attachment nodes separated from interface by %1 mm").arg(maxRootError,0,'g',12));
}

void rotatingInterfaceInertiaProjection()
{
    CadDocument cad;const auto input=preparedCantilever(cad);
    NativeMotionSystem system;
    auto body=std::make_shared<motion::MotionBody>();
    body->SetNameString("0");body->SetPos(motion::MotionVector<>(5,5,5));
    system.AddBody(body);
    const auto runtimes=buildNativeFlex(system,input,SolverCallbacks{});
    require(runtimes.size()==1,"missing reduced beam assembly");
    auto& assembly=*runtimes.front().result.assembly;
    const auto size=assembly.Get_modal_M().rows();
    require(size==8&&assembly.GetOffset_w()==0,"unexpected inertia regression fixture layout");

    const auto project=[&](const motion::MotionVector<>& translation){
        motion::MotionVectorDynamic<> input=motion::MotionVectorDynamic<>::Zero(size);
        motion::MotionVectorDynamic<> residual=motion::MotionVectorDynamic<>::Zero(size);
        input.head(3)<<translation.x(),translation.y(),translation.z();
        assembly.IntLoadResidual_Mv(0,residual,input,1);
        motion::MotionSystemDescriptor descriptor;
        assembly.InjectKRMmatrices(descriptor);
        assembly.KRMmatricesLoad(0,0,1);
        auto& blocks=descriptor.GetKblocksList();
        require(blocks.size()==1,"missing reduced effective matrix block");
        expectNear((blocks.front()->Get_K()*input-residual).norm(),0,1e-10,"effective mass and inertia residual disagree");
        return residual.tail(2).eval();
    };

    const motion::MotionVector<> direction(.4,-1,.7);
    const auto rest=project(direction);
    require(rest.norm()>1e-6,"rotation fixture has no translational modal coupling");
    const motion::MotionQuaternion<> quarterTurn(std::sqrt(.5),0,0,std::sqrt(.5));
    body->SetRot(quarterTurn);
    const auto rotated=project(quarterTurn.Rotate(direction));
    expectNear((rotated-rest).norm(),0,1e-10,"same body-frame acceleration changed modal inertia after a quarter turn");
}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        contourSemantics();completePayloadAndPhysicalMotion();divergenceKeepsCleanResults();multipleFlexCaptureIsAtomic();nativeReactionConvention();rotatingOffsetFlexInterface();
        rotatingInterfaceInertiaProjection();
        qInfo()<<"PASS: full modal payload, contour semantics, original reference frame, and synchronized partial results";
        return 0;
    }catch(const std::exception& exception){qCritical()<<exception.what();return 1;}
}
