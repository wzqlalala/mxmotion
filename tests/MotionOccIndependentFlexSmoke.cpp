#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionRuntime.h"
#include "NativeMotionSolver.h"
#include "RigidMotion.h"
#include "RigidProject.h"

#include "motion/fea/ElementTetraCorot_4.h"
#include "motion/fea/Mesh.h"
#include "motion/fea/NodeFEAxyz.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool ok,const QString& message){if(!ok)throw std::runtime_error(message.toStdString());}
void near(double actual,double expected,double tolerance,const QString& message)
{
    require(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,
        QStringLiteral("%1: %2 != %3 (tol %4)").arg(message).arg(actual,0,'g',16).arg(expected,0,'g',16).arg(tolerance));
}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
struct Fixture {CadDocument cad; NativeMotionDocument document;};
void makeBeam(Fixture& fixture,int slices=4)
{
    QString error;int part=0;require(fixture.cad.addPrimitive(Primitive::Box,40,10,10,part,error),error);
    QJsonArray nodes,tets;
    for(int i=0;i<=slices;++i){const double x=40.0*i/slices;nodes.append(QJsonArray{x,0,0});nodes.append(QJsonArray{x,10,0});nodes.append(QJsonArray{x,0,10});nodes.append(QJsonArray{x,10,10});}
    const int cubeTets[6][4]={{0,1,2,6},{0,2,3,6},{0,3,7,6},{0,7,4,6},{0,4,5,6},{0,5,1,6}};
    for(int i=0;i<slices;++i){const int cube[8]={4*i,4*(i+1),4*(i+1)+1,4*i+1,4*i+2,4*(i+1)+2,4*(i+1)+3,4*i+3};
        for(const auto& tet:cubeTets)tets.append(QJsonArray{cube[tet[0]],cube[tet[1]],cube[tet[2]],cube[tet[3]]});}
    const QJsonObject body{{"id",0},{"name","axial beam"},{"geoEntityIds",QJsonArray{part}},
        {"isFixed",false},{"mass",41.0},{"isCustomMassProps",true},{"centerOfMass",xyz(20,5,5)},
        {"inertia",QJsonArray{10000,10000,10000,0,0,0}},
        {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}};
    const QJsonArray joints{
        QJsonObject{{"id",0},{"name","root clamp"},{"jointType","Fixed"},{"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(0,0,1)}},
        QJsonObject{{"id",1},{"name","independent slider"},{"jointType","Translational"},{"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(40,5,5)},{"axis",xyz(1,0,0)}}};
    const QJsonObject load{{"id",0},{"name","axial 10 N"},{"loadType","Force"},{"bodyId",0},
        {"actionPoint",xyz(40,5,5)},{"direction",xyz(1,0,0)},{"directionMode","GlobalFixed"},{"magnitudeMode","Constant"},{"magnitude",10.0},{"enabled",true}};
    const QJsonObject scenario{{"id",0},{"name","independent FEM equilibrium"},{"analysisType","Static"},
        {"endTime",.01},{"stepSize",.0005},{"gravity",xyz(0,0,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},
        {"solverType","SPARSE_QR"},{"solverTolerance",1e-10},{"staticTolerance",1e-8},{"staticConstraintTolerance",1e-8},{"staticMaxIterations",50}};
    const QJsonObject flex{{"id",0},{"name","independent beam"},{"targetBodyId",0},{"enabled",true},
        {"interfaceMode","Independent"},{"modeCount",3},{"dampingType",0},
        {"material",QJsonObject{{"youngModulus",100.0},{"poissonRatio",0.0},{"density",.001}}},
        {"mesh",QJsonObject{{"nodes",nodes},{"tetrahedra",tets}}},
        {"attachGroups",QJsonArray{QJsonArray{0,1,2,3},QJsonArray{4*slices,4*slices+1,4*slices+2,4*slices+3}}}};
    fixture.document.root={{"formatVersion",1},{"motionData",QJsonObject{{"bodies",QJsonArray{body}},{"joints",joints},
        {"vectorForces",QJsonArray{load}},{"scenarios",QJsonArray{scenario}},{"autoFlexBodies",QJsonArray{flex}}}}};
    fixture.document.bodyPartBindings.insert(0,{part});
}
NativeMotionInput prepare(Fixture& fixture)
{
    NativeMotionInput input;QString error;require(prepareNativeMotion(fixture.cad,fixture.document,0,input,error),error);return input;
}
RigidResult solve(const NativeMotionInput& input)
{
    RigidResult result;QString error;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,error);return result;
}
void staticAxialAndPortIdentity()
{
    Fixture fixture;makeBeam(fixture);const auto original=fixture.document.root;auto input=prepare(fixture);
    require(fixture.document.root==original,"preparing independent ports changed saved design");
    require(input.bodies.size()==2&&input.geometry->flexBodies[0].interfaceBodyIds.size()==2,"two real port bodies not created");
    const auto root=input.document.entity("joints",0),tip=input.document.entity("joints",1);
    require(root.value("actionBodyId").toInt()==0&&tip.value("actionBodyId").toInt()!=0,"independent joints still act on the same rigid body");
    require(input.document.entity("vectorForces",0).value("bodyId")==tip.value("actionBodyId"),"tip force did not route to the actual independent port");
    const auto result=solve(input);require(result.frames.size()==1,"static solution has transient frames");
    require(result.flexBodies.size()==1,"missing full FEM output");const auto& flex=result.flexBodies.front();
    require(!flex.reducedDynamics&&flex.interfaceCount==2&&flex.modeCount==0,"full FEM incorrectly advertised as reduced CMS");
    require(flex.modalCoordinates.front().empty(),"full FEM fabricated modal coordinate history");
    require(flex.modalBasis.frequenciesHz.size()==3&&flex.modalBasis.attachmentGroups.size()==2,"missing real clamped inspection basis");
    near(flex.interfacePoses[0][7]-flex.initialInterfacePoses[7],.04,2e-6,"independent axial compliance F L / E A");
    for(std::size_t i=0;i<flex.referenceNodes.size()/3;++i){
        const double x=flex.referenceNodes[3*i];
        near(flex.nodePositions[0][3*i]-x,.001*x,3e-6,"tetrahedral axial patch displacement");
    }
    std::vector<double> elastic;QString error;require(flexNodeScalars(flex,0,FlexContourKind::ElasticDisplacement,elastic,error),error);
    near(*std::max_element(elastic.begin(),elastic.end()),.04,3e-6,"full FEM elastic contour lost static preload");
    qInfo()<<"independent static beam: tip extension"<<flex.interfacePoses[0][7]-flex.initialInterfacePoses[7]<<"mm, analytic 0.04 mm";
}
void gravityMassOnlyOnce()
{
    Fixture fixture;makeBeam(fixture);auto data=fixture.document.root.value("motionData").toObject();
    data["vectorForces"]=QJsonArray{};auto scenario=data.value("scenarios").toArray()[0].toObject();scenario["gravity"]=xyz(0,-9810,0);
    data["scenarios"]=QJsonArray{scenario};fixture.document.root["motionData"]=data;
    const auto result=solve(prepare(fixture));double reaction=0;int found=0;
    for(const auto& channel:result.channels)if(channel.sourceType=="joints"&&channel.variable=="ReactionForce"){
        reaction+=channel.values[1];++found;
    }
    require(found==2,"missing independent support reactions");
    near(reaction,-39.24,2e-5,"FEM gravity mass must be 4 kg once, not original 41 kg or duplicated ports");
    qInfo()<<"independent gravity: summed support-side reaction"<<reaction<<"N, mesh weight -39.24 N";
}
void preloadedDynamicsKeepsStrain()
{
    Fixture fixture;makeBeam(fixture);auto data=fixture.document.root.value("motionData").toObject();
    auto scenario=data.value("scenarios").toArray()[0].toObject();scenario["analysisType"]="PreloadedDynamics";
    data["scenarios"]=QJsonArray{scenario};fixture.document.root["motionData"]=data;
    const auto result=solve(prepare(fixture));require(result.frames.size()==21,"preloaded FEM time axis");
    const auto& flex=result.flexBodies.front();
    for(const auto& poses:flex.interfacePoses)near(poses[7]-flex.initialInterfacePoses[7],.04,2e-5,"preload strain reset or initial transient impulse");
}
void independentPortTransmitsTorque()
{
    Fixture fixture;makeBeam(fixture);auto data=fixture.document.root.value("motionData").toObject();
    auto joints=data.value("joints").toArray();auto tip=joints[1].toObject();tip["jointType"]="Revolute";joints[1]=tip;data["joints"]=joints;
    auto torque=data.value("vectorForces").toArray()[0].toObject();torque["loadType"]="Torque";torque["magnitude"]=.1;
    data["vectorForces"]=QJsonArray{torque};fixture.document.root["motionData"]=data;
    const auto result=solve(prepare(fixture));const auto& flex=result.flexBodies.front();const auto& pose=flex.interfacePoses[0];
    const double angle=2*std::atan2(pose[11],pose[10]);
    require(std::isfinite(angle)&&angle>1e-8&&angle<1e-3,"independent torque port did not develop a small positive relative rotation");
    near(pose[4],0,1e-10,"root port rotated under tip torque");
    double total=0;int reactions=0;
    for(const auto& channel:result.channels)if(channel.sourceType=="joints"&&channel.variable=="ReactionTorque"){
        total+=channel.values[0];++reactions;
    }
    require(reactions==2,"missing actual torque reactions at independent ports");
    near(total,.1,2e-5,"root/support torque equilibrium at independently rotating port");
    const std::size_t corner=flex.referenceNodes.size()/3-1;
    require(flex.nodePositions[0][3*corner+1]<flex.referenceNodes[3*corner+1]-1e-8
        &&flex.nodePositions[0][3*corner+2]>flex.referenceNodes[3*corner+2]+1e-8,"actual tip patch did not twist with its independent port");
    const motion::MotionQuaternion<> rotation(pose[10],pose[11],pose[12],pose[13]);
    const auto expected=motion::MotionVector<>(pose[7],pose[8],pose[9])+rotation.Rotate(motion::MotionVector<>(0,5,5));
    for(int d=0;d<3;++d)near(flex.nodePositions[0][3*corner+d],expected[d],2e-8,"full FEM tip patch broke its real rotational weld");
    qInfo()<<"independent 0.1 Nmm torque: relative tip rotation"<<angle<<"rad, support-side torque"<<total<<"Nmm";
}
void noInternalModesStillElastic()
{
    Fixture fixture;makeBeam(fixture,1);const auto result=solve(prepare(fixture));
    const auto& flex=result.flexBodies.front();require(flex.modalBasis.frequenciesHz.empty(),"fully attached coarse mesh invented internal modes");
    near(flex.interfacePoses[0][7]-flex.initialInterfacePoses[7],.04,2e-6,"zero internal modes must not rigidify independent boundaries");
}
void sparseStaticAndEigenPaths()
{
    Fixture staticFixture;makeBeam(staticFixture,24);
    const auto result=solve(prepare(staticFixture));const auto& flex=result.flexBodies.front();
    near(flex.interfacePoses[0][7]-flex.initialInterfacePoses[7],.04,3e-6,"sparse FEM stiffness equilibrium F L / E A");
    Fixture modesFixture;makeBeam(modesFixture,103);const auto input=prepare(modesFixture);
    RigidFlexResult modes;QString error;const auto status=inspectNativeFlexModes(input,0,modes,error);
    require(status==SolveStatus::Completed,error);
    require(modes.referenceNodes.size()-24>1200&&modes.modalBasis.frequenciesHz.size()==3,"large mesh did not retain actual sparse eigenmodes");
    for(double f:modes.modalBasis.frequenciesHz)require(std::isfinite(f)&&f>0,"non-positive sparse eigenfrequency");
    qInfo()<<"sparse FEM equilibrium: 312 velocity DOF; sparse clamped eigenanalysis: 1224 free nodal DOF, f1"<<modes.modalBasis.frequenciesHz.front()<<"Hz";
}
void floatingInspectionDoesNotPretendCoupling()
{
    Fixture fixture;makeBeam(fixture);auto data=fixture.document.root.value("motionData").toObject();
    data["joints"]=QJsonArray{};auto definition=data.value("autoFlexBodies").toArray()[0].toObject();
    definition["interfaceMode"]="SharedBody";definition["attachGroups"]=QJsonArray{};definition["modeCount"]=8;
    data["autoFlexBodies"]=QJsonArray{definition};fixture.document.root["motionData"]=data;
    const auto input=prepare(fixture);QString error;RigidResult dynamics;
    const auto status=solveNativeMotion(input,dynamics,error);
    require(status==SolveStatus::Failed&&error.contains("实际附着接口"),"unconnected floating mesh was accepted as a coupled dynamic body: "+error);
    RigidFlexResult modes;const auto modeStatus=inspectNativeFlexModes(input,0,modes,error);
    require(modeStatus==SolveStatus::Completed,error);
    require(modes.modalBasis.kind=="FreeBody"&&modes.interfaceCount==0,"free-free inspector added fake attachment");
    require(modes.modalBasis.frequenciesHz.size()>=7,"free-free inspection omitted rigid/elastic basis");
    qInfo()<<"free-free inspection retained"<<modes.modalBasis.frequenciesHz.size()<<"actual modes; unconnected dynamics rejected";
}
void rigidRotationObjectivity()
{
    Fixture fixture;makeBeam(fixture);const auto input=prepare(fixture);NativeMotionSystem system;
    for(const auto& prepared:input.bodies){auto body=std::make_shared<motion::MotionBody>();body->SetNameString(std::to_string(prepared.body.id));
        const auto& p=prepared.properties.centerMm;body->SetPos(motion::MotionVector<>(p[0],p[1],p[2]));system.AddBody(body);}
    auto runtime=buildNativeFlex(system,input,{});require(runtime.size()==1&&!runtime[0].reducedDynamics,"full FEM runtime missing");
    auto& assembly=*runtime[0].result.assembly;const motion::MotionQuaternion<> rotation=motion::Q_from_AngAxis(1.2,motion::MotionVector<>(1,2,3).GetNormalized());
    const motion::MotionVector<> translation(7,-3,11);
    for(const auto& body:runtime[0].result.interface_bodies){near(body->GetMass(),0,0,"port retained duplicate rigid mass");body->SetPos(translation+rotation.Rotate(body->GetPos()));body->SetRot(rotation);}
    double maximumForce=0,mass=0;
    for(const auto& mesh:assembly.Get_internal_meshlist()){
        for(const auto& node:mesh->GetNodes()){const auto p=std::dynamic_pointer_cast<motion::fea::MotionNodeFEAxyz>(node);p->SetPos(translation+rotation.Rotate(p->GetPos()));}
        mesh->Update(0.0,true);
        motion::MotionVector<> center;motion::MotionMatrix33<> inertia;double partMass=0;mesh->ComputeMassProperties(partMass,center,inertia);mass+=partMass;
        for(const auto& element:mesh->GetElements()){
            const auto tet=std::dynamic_pointer_cast<motion::fea::MotionElementTetraCorot_4>(element);motion::MotionVectorDynamic<> force(12);tet->ComputeInternalForces(force);
            maximumForce=std::max(maximumForce,force.cwiseAbs().maxCoeff());
        }
    }
    near(mass,4.0,1e-12,"actual finite element mass");
    require(maximumForce<1e-6,QStringLiteral("pure 1.2 rad rigid rotation generated elastic force %1 mN").arg(maximumForce));
    captureNativeFlex(runtime,0.0);require(runtime[0].modalCoordinates[0].empty(),"full FEM capture invented modal state");
    qInfo()<<"full FEM pure rigid rotation: max internal force"<<maximumForce<<"mN";
}
}
int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        if(argc>1){
            Fixture fixture;makeBeam(fixture);RigidModel model;QString error;
            model.nativeDocument=std::make_shared<NativeMotionDocument>(fixture.document);syncNativeMotionModel(model);
            require(saveRigidProject(QString::fromLocal8Bit(argv[1]),fixture.cad,model,error),error);
            qInfo()<<"Saved real CAD / original-schema independent beam fixture"<<argv[1];
        }
        staticAxialAndPortIdentity();gravityMassOnlyOnce();preloadedDynamicsKeepsStrain();independentPortTransmitsTorque();noInternalModesStillElastic();rigidRotationObjectivity();sparseStaticAndEigenPaths();floatingInspectionDoesNotPretendCoupling();
        qInfo()<<"Independent flexible interfaces: physical static/dynamic/rotation checks passed";return 0;
    }catch(const std::exception& exception){qCritical().noquote()<<exception.what();return 1;}
}
