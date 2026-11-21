#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"
#include "NativeMotionRuntime.h"
#include "RigidMotion.h"
#include "OccTessellation.h"

#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepGProp.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <cmath>
#include <functional>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool value,const QString& message){if(!value)throw std::runtime_error(message.toStdString());}
void near(double value,double expected,double tolerance,const QString& what){require(std::isfinite(value)&&std::abs(value-expected)<=tolerance,QStringLiteral("%1: got %2 expected %3 ± %4").arg(what).arg(value,0,'g',16).arg(expected,0,'g',16).arg(tolerance));}
QJsonObject xyz(double x,double y,double z){return { {"x",x},{"y",y},{"z",z} };}
QJsonObject body(int id,int part=0,bool fixed=false,double x=5,double y=5,double z=5){
    return {{"id",id},{"name",QString("body%1").arg(id)},{"geoEntityIds",part>0?QJsonArray{part}:QJsonArray{}},{"isFixed",fixed},{"mass",2.0},{"isCustomMassProps",true},{"centerOfMass",xyz(x,y,z)},{"inertia",QJsonArray{100.0/3,100.0/3,100.0/3,0,0,0}},{"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}};
}
void collection(NativeMotionDocument& doc,const QString& name,const QJsonArray& values){auto data=doc.root.value("motionData").toObject();data[name]=values;doc.root["motionData"]=data;}
QJsonObject joint(const QString& type,int id=0,int action=0,int base=-1){return {{"id",id},{"name",type},{"jointType",type},{"actionBodyId",action},{"baseBodyId",base},{"origin",xyz(5,5,5)},{"axis",xyz(0,0,1)}};}
QJsonObject drive(const QString& mode="Velocity",const QString& expression="120",int id=0,int target=0,const QString& motion="Rotation"){
    return {{"id",id},{"name","drive"},{"driveType","JointDrive"},{"targetType","Joint"},{"targetJointId",target},{"motionType",motion},{"controlMode",mode},{"expression",expression}};
}
struct Fixture {
    CadDocument cad;
    NativeMotionDocument doc;
    int part=0;
    Fixture(){QString error;require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        doc.root={{"formatVersion",1},{"motionData",QJsonObject{{"bodies",QJsonArray{body(0,part)}},{"scenarios",QJsonArray{QJsonObject{{"id",0},{"name","native"},{"endTime",.02},{"stepSize",.001},{"gravity",xyz(0,0,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"},{"solverTolerance",1e-10}}}}}}};
        doc.bodyPartBindings.insert(0,{part});
    }
    RigidResult solve(){NativeMotionInput input;QString error;require(prepareNativeMotion(cad,doc,0,input,error),"prepare: "+error);RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,"solve: "+error);
        require(result.scenarioId==0&&result.frames.size()>=2,"native zero IDs lost");near(result.frames.front().seconds,0,1e-12,"initial time");near(result.frames.back().seconds,input.scenario.duration,1e-10,"terminal time");
        for(const auto& channel:result.channels)require(channel.values.size()==result.frames.size()*channel.components,"unaligned channel");return result;}
    void scenario(double duration,double step){auto s=doc.entity("scenarios",0);s["endTime"]=duration;s["stepSize"]=step;collection(doc,"scenarios",{s});}
    int fixedPart(double dx,double dy,double dz){
        gp_Trsf transform;transform.SetTranslation(gp_Vec(dx,dy,dz));const auto shape=BRepBuilderAPI_Transform(cad.part(part)->shape,transform,true).Shape();
        QTemporaryDir dir;require(dir.isValid(),"temp");const auto path=dir.filePath("fixed.brep");require(BRepTools::Write(shape,path.toUtf8().constData()),"BREP write");QString error;std::vector<int> ids;require(cad.importFile(path,ids,error),error);require(ids.size()==1,"fixed part count");
        auto bodies=doc.collection("bodies");bodies.append(body(1,ids[0],true,5+dx,5+dy,5+dz));collection(doc,"bodies",bodies);doc.bodyPartBindings.insert(1,{ids[0]});return ids[0];
    }
};
const ResultChannel& channel(const RigidResult& result,const QString& type,int id,const QString& variable){for(const auto& c:result.channels)if(c.sourceType==type&&c.sourceId==id&&c.variable==variable)return c;throw std::runtime_error(QStringLiteral("missing %1/%2/%3").arg(type).arg(id).arg(variable).toStdString());}

void pinchedPlanarTessellation(){
    // The supplied engine cam has tangent circular inner/outer wires. Recreate
    // the same geometry independently; OCCT 7.8 reports self-intersection here.
    auto outer=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0,0,0),gp_Dir(0,0,1)),66.5)).Edge()).Wire();
    auto inner=BRepBuilderAPI_MakeWire(BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(gp_Pnt(0,-8,0),gp_Dir(0,0,1)),58.5)).Edge()).Wire();
    inner.Reverse();BRepBuilderAPI_MakeFace maker(gp_Pln(gp_Pnt(0,0,0),gp_Dir(0,0,1)),outer,true);maker.Add(inner);
    gp_Trsf pose;pose.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,1,1)),.7);pose.SetTranslationPart(gp_Vec(400,-30,20));
    for(int mode=0;mode<3;++mode){
        auto face=TopoDS::Face(BRepBuilderAPI_Copy(maker.Face(),true,false).Shape());
        if(mode)face.Location(TopLoc_Location(pose));if(mode==2)face.Reverse();
        const auto topology=face.TShape();const auto orientation=face.Orientation();const auto location=face.Location();
        GProp_GProps before,after;BRepGProp::SurfaceProperties(face,before);
        QString error;require(ensureOccFaceTriangulation(face,.05,error),error);
        BRepGProp::SurfaceProperties(face,after);near(after.Mass(),before.Mass(),1e-10,"tessellation changed exact analytic area");
        near(after.Mass(),1000*M_PI,1e-8,"tangent ring area");
        require(face.TShape()==topology&&face.Orientation()==orientation&&face.Location()==location,"tessellation replaced original topology or placement");
        TopLoc_Location meshLocation;const auto mesh=BRep_Tool::Triangulation(face,meshLocation);
        require(!mesh.IsNull()&&mesh->NbTriangles()>100,"pinched face triangles missing");
        const auto normal=BRepAdaptor_Surface(face).Plane().Axis().Direction();
        for(int n=1;n<=mesh->NbTriangles();++n){
            int a,b,c;mesh->Triangle(n).Get(a,b,c);
            const auto pa=mesh->Node(a).Transformed(meshLocation.Transformation()),pb=mesh->Node(b).Transformed(meshLocation.Transformation()),pc=mesh->Node(c).Transformed(meshLocation.Transformation());
            const auto cross=gp_Vec(pa,pb).Crossed(gp_Vec(pa,pc));
            require(cross.Dot(gp_Vec(normal))>=-1e-12,"merged triangulation has reversed normals");
            auto centroid=gp_Pnt((pa.X()+pb.X()+pc.X())/3,(pa.Y()+pb.Y()+pc.Y())/3,(pa.Z()+pb.Z()+pc.Z())/3);
            if(mode)centroid.Transform(pose.Inverted());
            near(centroid.Z(),0,1e-10,"located mesh is off its exact support");
            require(std::hypot(centroid.X(),centroid.Y()+8)>=58.5-.05,"tessellation filled the inner hole");
            require(std::hypot(centroid.X(),centroid.Y())<=66.5+.05,"tessellation expanded the outer boundary");
        }
    }
}

void drivesAndSensors(){
    Fixture f;collection(f.doc,"joints",{joint("Revolute")});collection(f.doc,"drives",{drive()});
    collection(f.doc,"markers",{QJsonObject{{"id",0},{"name","point"},{"bodyId",0},{"localPosition",xyz(7,5,5)},{"localRotation",QJsonObject{{"w",1},{"x",0},{"y",0},{"z",0}}}}});
    collection(f.doc,"sensors",{QJsonObject{{"id",0},{"name","omega"},{"sensorType","BodyVelocity"},{"targetBodyId",0},{"variable","AngularVelocity"},{"componentMode","Z"}},QJsonObject{{"id",1},{"name","drive"},{"sensorType","DriveActuation"},{"targetDriveId",0},{"variable","ImposedValue"}}});
    auto result=f.solve();const auto& omega=channel(result,"bodies",0,"AngularVelocity");near(omega.values.back(),120,1e-4,"native degrees/s");
    near(channel(result,"drives",0,"ImposedValue").values.back(),2.4,.01,"speed motor integrated coordinate");
    near(channel(result,"sensors",1,"SensorValue").values.back(),2.4,.01,"drive sensor degrees");
    const auto& marker=channel(result,"markers",0,"Position");near(marker.values[0],7,1e-10,"marker initial-global x");
    near(marker.values[marker.values.size()-3],5+2*std::cos(2.4*M_PI/180),1e-4,"marker rotates around COM");
    auto d=drive("Position","unused(invalid)");d["driveTableX"]=QJsonArray{0,.02};d["driveTableY"]=QJsonArray{0,5};collection(f.doc,"drives",{d});result=f.solve();
    near(channel(result,"drives",0,"ImposedValue").values.back(),5,1e-8,"table takes precedence over inactive formula");
    d=drive("Position","30*t*t");collection(f.doc,"drives",{d});result=f.solve();near(channel(result,"drives",0,"ImposedValue").values.back(),.012,1e-9,"expression position drive");
}

void exactAcceptedTimeline(){
    Fixture f;f.scenario(15.0,.001);
    auto source=f.doc.entity("bodies",0);source["initialVelocity"]=xyz(1,0,0);collection(f.doc,"bodies",{source});
    const auto result=f.solve();
    require(result.frames.size()==15001,"integer-duration solve added a fictitious tiny final dynamics step");
    for(std::size_t i=0;i<result.frames.size();++i){
        near(result.frames[i].seconds,i*.001,0,"accepted time grid");
        if(i)near(result.frames[i].seconds-result.frames[i-1].seconds,.001,2e-15,"uniform accepted interval");
    }
    near(result.frames.back().bodies[0].positionMm[0],20.0,2e-10,"long constant-velocity motion");
    for(double value:channel(result,"bodies",0,"Acceleration").values)near(value,0,1e-8,"no terminal acceleration spike");
    f.scenario(.0035,.001);const auto shortened=f.solve();
    require(shortened.frames.size()==5,"physical shortened final step lost");
    near(shortened.frames.back().seconds,.0035,0,"fractional end time");
    near(shortened.frames.back().seconds-shortened.frames[3].seconds,.0005,1e-16,"physical shortened interval");
}

void inertiaConventionsWithCadEvidence(){
    Fixture f;f.scenario(.0001,.0001);QString error;int half=0;
    require(f.cad.addPrimitive(Primitive::Box,1,4,8,half,error),error);
    gp_Trsf pose;pose.SetRotation(gp_Ax1(gp_Pnt(0,0,0),gp_Dir(1,2,3)),.7);
    const gp_Pnt center(12,-7,5),rotatedCenter=gp_Pnt(1,2,4).Transformed(pose);
    pose.SetTranslationPart(gp_Vec(rotatedCenter,center));
    QTemporaryDir directory;require(directory.isValid(),"inertia geometry directory");QVector<int> bound;
    for(int i=0;i<2;++i){gp_Trsf shift;shift.SetTranslation(gp_Vec(i,0,0));const auto shape=BRepBuilderAPI_Transform(f.cad.part(half)->shape,pose*shift,true).Shape();
        const auto path=directory.filePath(QString("half%1.brep").arg(i));require(BRepTools::Write(shape,path.toUtf8().constData()),"inertia BREP write");std::vector<int> ids;require(f.cad.importFile(path,ids,error)&&ids.size()==1,error);bound.push_back(ids[0]);}
    // Independently derived inertia of a 2 x 4 x 8 box, mass 3 kg, rotated in
    // all three axes. The two CAD solids must be aggregated with their offsets.
    Eigen::Matrix3d rotation;for(int i=0;i<3;++i)for(int j=0;j<3;++j)rotation(i,j)=pose.VectorialPart().Value(i+1,j+1);
    Eigen::Matrix3d local=Eigen::Matrix3d::Zero();local.diagonal()<<20,17,5;
    const Eigen::Matrix3d expected=rotation*local*rotation.transpose();
    const std::array<double,6> tensor{{expected(0,0),expected(1,1),expected(2,2),expected(0,1),expected(0,2),expected(1,2)}};
    QJsonArray products,matrix;for(int i=0;i<6;++i){matrix.append(tensor[i]);products.append(i<3?tensor[i]:-tensor[i]);}
    auto b=body(0,bound[0],false,center.X(),center.Y(),center.Z());b["mass"]=3.0;b["geoEntityIds"]=QJsonArray{bound[0],bound[1]};b["inertia"]=products;
    collection(f.doc,"bodies",{b});f.doc.bodyPartBindings[0]=bound;
    NativeMotionInput input;require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],tensor[i]*1e-6,1e-13,"CAD-proven products conversion");
    require(input.document.root==f.doc.root,"inertia convention recovery rewrote source JSON");
    collection(f.doc,"vectorForces",{QJsonObject{{"id",0},{"name","tensor probe"},{"loadType","Torque"},{"bodyId",0},{"actionPoint",xyz(12,-7,5)},{"direction",xyz(1,0,0)},{"magnitude",.001},{"magnitudeMode","Constant"},{"directionMode","GlobalFixed"}}});
    auto result=f.solve();require(!result.diagnostics.empty()&&result.diagnostics.join('\n').contains("CAD"),"automatic inertia recovery omitted its evidence diagnostic");
    const Eigen::Vector3d angularAcceleration=expected.inverse()*Eigen::Vector3d(1,0,0);
    const auto& actual=channel(result,"bodies",0,"AngularAcceleration");for(int i=0;i<3;++i)near(actual.values[i],angularAcceleration[i]*57.29577951308232,1e-5,"full non-diagonal torque response");
    b["inertiaConvention"]="tensor";collection(f.doc,"bodies",{b});require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],products[i].toDouble()*1e-6,1e-13,"explicit tensor convention changed");
    b["inertiaConvention"]="products";b["centerOfMass"]=xyz(13,-7,5);collection(f.doc,"bodies",{b});require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],tensor[i]*1e-6,1e-13,"explicit products convention ignored");
    b["inertiaConvention"]="auto";collection(f.doc,"bodies",{b});require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],products[i].toDouble()*1e-6,1e-13,"auto flipped products without matching COM evidence");
    b["centerOfMass"]=xyz(12,-7,5);b["inertia"]=matrix;collection(f.doc,"bodies",{b});require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],tensor[i]*1e-6,1e-13,"already-correct auto tensor changed");
    // This positive-definite matrix fails the physical inertia triangle test.
    // Flipping signs would make it physical, but unrelated CAD is no proof that
    // its author used products rather than a different custom mass distribution.
    b["inertia"]=QJsonArray{8.728274349937442,.3214865972899535,8.451220510598816,.15970788657836904,-.031573215247701264,-1.4194009978347097};collection(f.doc,"bodies",{b});
    require(!prepareNativeMotion(f.cad,f.doc,0,input,error)&&error.contains("惯量"),"invalid custom inertia was blindly sign-flipped");
    // Some supplied engineering models deliberately exclude display-only mass
    // and declare their export convention in the original generator metadata.
    // Such an explicit formula is evidence even for a body with no CAD at all.
    CadDocument noCad;auto declared=f.doc;declared.bodyPartBindings.clear();b["geoEntityIds"]=QJsonArray{};collection(declared,"bodies",{b});collection(declared,"vectorForces",{});
    declared.root["generatorNotes"]=QJsonObject{{"coordinateAndInertiaConvention",QJsonObject{{"inertia",QStringLiteral("kg*mm^2: [ Ixx, Iyy, Izz, -CAD_tensor_xy, -CAD_tensor_xz, -CAD_tensor_yz ]")}}}};
    const auto unchanged=declared.root;
    require(prepareNativeMotion(noCad,declared,0,input,error),error);
    for(int i=0;i<6;++i)near(input.bodies[0].properties.inertiaKgM2[i],b.value("inertia").toArray()[i].toDouble()*(i<3?1e-6:-1e-6),1e-15,"declared products convention without CAD");
    require(input.document.root==unchanged,"source convention declaration or numbers were rewritten");
    RigidResult declaredResult;require(solveNativeMotion(input,declaredResult,error)==SolveStatus::Completed,error);
    require(declaredResult.diagnostics.join('\n').contains("generatorNotes"),"declared convention provenance missing from diagnostics");
    b["inertiaConvention"]="tensor";collection(declared,"bodies",{b});require(!prepareNativeMotion(noCad,declared,0,input,error),"body tensor override ignored in favor of document declaration");
    b["inertiaConvention"]="auto";collection(declared,"bodies",{b});declared.root["generatorNotes"]=QJsonObject{{"coordinateAndInertiaConvention",QJsonObject{{"inertia","possibly products; no explicit formula"}}}};
    require(!prepareNativeMotion(noCad,declared,0,input,error),"ambiguous prose was treated as a physical convention declaration");
}

void closedLoopProjectionAndConflict(){
    Fixture f;f.scenario(.1,.01);
    auto scenario=f.doc.entity("scenarios",0);scenario["gravity"]=xyz(0,0,-9810);scenario["usePreset"]=true;scenario["presetMode"]="FAST";collection(f.doc,"scenarios",{scenario});
    // A small-inertia, redundant six-body ellipsograph reproduces the original
    // case's >1 mm error with only three position iterations per accepted step.
    const double properties[6][7]={
        {0,0,10.08265686,42.6059901119,1402690.2878,704413.0310,704413.0310},
        {157.53440857,0,20.65529633,.0235213439,1.096490033,1.075120243,.701460243},
        {78.76720428,61.60947418,27.5,.1537246442,2061.92101664,2060.93693181,1.624604179},
        {0,123.21894836,20.65529633,.0235213439,1.096490033,1.075120243,.701460243},
        {78.76720428,61.60947418,15,.00616537558,.831683477,.831683477,.0192667987},
        {39.38360214,30.80473709,-2.5,.0407913439,37.57665997,37.31770015,.428923749}};
    QJsonArray bodies;
    for(int i=0;i<6;++i){int part=f.part;QString error;if(i)require(f.cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        auto b=body(i,part,i==0,properties[i][0],properties[i][1],properties[i][2]);b["mass"]=properties[i][3];b["inertia"]=QJsonArray{properties[i][4],properties[i][5],properties[i][6],0,0,0};bodies.append(b);f.doc.bodyPartBindings.insert(i,{part});}
    collection(f.doc,"bodies",bodies);
    QJsonArray joints;
    const auto addJoint=[&](int id,const QString& type,int a,int b,QJsonObject origin,QJsonObject axis){auto j=joint(type,id,a,b);j["origin"]=origin;j["axis"]=axis;joints.append(j);};
    addJoint(1,"Translational",1,0,xyz(157.53440857,0,25),xyz(1,0,0));
    addJoint(2,"Revolute",1,2,xyz(157.53440857,0,25),xyz(0,0,1));
    addJoint(3,"Revolute",2,3,xyz(0,123.21894836,25),xyz(0,0,1));
    addJoint(4,"Translational",3,0,xyz(0,133.21894836,20),xyz(0,1,0));
    addJoint(5,"Revolute",2,4,xyz(78.76720428,61.60947418,-5),xyz(0,0,1));
    addJoint(6,"Revolute",4,5,xyz(78.76720428,61.60947418,-5),xyz(0,0,1));
    addJoint(7,"Revolute",5,0,xyz(0,0,0),xyz(0,0,1));
    collection(f.doc,"joints",joints);collection(f.doc,"drives",{drive("Velocity","57.29577951308232",0,7)});
    const auto result=f.solve();double maxClosure=0;
    for(const auto& frame:result.frames){
        std::map<int,const BodyState*> states;for(const auto& b:frame.bodies)states.emplace(b.bodyId,&b);
        const auto vec=[](const QJsonValue& value){const auto o=value.toObject();return motion::MotionVector<>(o.value("x").toDouble(),o.value("y").toDouble(),o.value("z").toDouble());};
        const auto rotation=[&](int id){const auto& q=states.at(id)->rotation;return motion::MotionQuaternion<>(q[0],q[1],q[2],q[3]);};
        const auto point=[&](int id,const motion::MotionVector<>& value){const auto& p=states.at(id)->positionMm;return motion::MotionVector<>(p[0],p[1],p[2])+rotation(id).Rotate(value-motion::MotionVector<>(properties[id][0],properties[id][1],properties[id][2]));};
        for(const auto& v:joints){const auto j=v.toObject();const int a=j.value("actionBodyId").toInt(),b=j.value("baseBodyId").toInt();const auto origin=vec(j.value("origin"));auto delta=point(a,origin)-point(b,origin);
            if(j.value("jointType").toString()=="Translational"){const auto direction=rotation(b).Rotate(vec(j.value("axis")));delta-=direction*delta.Dot(direction);}maxClosure=std::max(maxClosure,delta.Length());}
    }
    require(maxClosure<.05,QString("redundant closed loop drifted %1 mm").arg(maxClosure));

    Fixture conflict;conflict.scenario(.1,.01);int secondPart=0;QString error;require(conflict.cad.addPrimitive(Primitive::Box,10,10,10,secondPart,error),error);
    collection(conflict.doc,"bodies",{body(0,conflict.part),body(1,secondPart)});conflict.doc.bodyPartBindings.insert(1,{secondPart});
    auto first=joint("Revolute",0,0,-1),second=joint("Revolute",1,1,0),extra=joint("Universal",2,1,-1);
    second["axis"]=xyz(0,1,0);extra["actionSpin"]=xyz(0,0,1);extra["baseSpin"]=xyz(0,0,1);extra["actionPin"]=xyz(1,0,0);extra["basePin"]=xyz(0,1,0);
    collection(conflict.doc,"joints",{first,second,extra});collection(conflict.doc,"drives",{drive("Position","60*t",0,0),drive("Position","0",1,1)});
    NativeMotionInput input;require(prepareNativeMotion(conflict.cad,conflict.doc,0,input,error),error);
    RigidResult previous;previous.scenarioId=12345;
    require(solveNativeMotion(input,previous,error)==SolveStatus::Failed,"contradictory closed-loop constraints were reported as a successful animation");
    require(error.contains("t=")&&error.contains("误差")&&previous.scenarioId==12345,"constraint failure omitted physical diagnosis or replaced the prior result");
}

void jointFamilies(){
    for(const QString& type:QStringList{"Revolute","Translational","Cylindrical","Spherical","Fixed","Universal","Hooke","PointPlane","Parallel","Perpendicular","Direction","ConstantVelocity","Screw","Planar","PointLine"}){
        Fixture f;f.scenario(.002,.001);auto j=joint(type);j["lead"]=10;j["actionSpin"]=xyz(0,0,1);j["baseSpin"]=xyz(0,0,1);j["actionPin"]=xyz(1,0,0);j["basePin"]=xyz(0,1,0);j["actionVector"]=xyz(0,0,1);j["baseVector"]=type=="Perpendicular"?xyz(1,0,0):xyz(0,0,1);
        collection(f.doc,"joints",{j});const auto result=f.solve();channel(result,"joints",0,"ReactionForce");
        near(result.frames.back().bodies[0].positionMm[0],5,1e-7,type+" stationary closure");
    }
}

void forcesAndUnilateral(){
    Fixture f;f.scenario(.003,.001);
    collection(f.doc,"vectorForces",{QJsonObject{{"id",0},{"name","load"},{"loadType","Force"},{"bodyId",0},{"actionPoint",xyz(5,5,5)},{"direction",xyz(1,0,0)},{"magnitude",2.0},{"magnitudeMode","Constant"},{"directionMode","GlobalFixed"}}});
    auto result=f.solve();near(channel(result,"bodies",0,"Velocity").values[(result.frames.size()-1)*3],3,1e-8,"N to mN acceleration");near(channel(result,"vectorForces",0,"Magnitude").values.front(),2,1e-10,"force output N");
    collection(f.doc,"vectorForces",{});
    QJsonObject cable{{"id",0},{"name","rope"},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(15,5,5)},{"freeLength",8.0},{"stiffness",10.0},{"damping",0.0},{"engagementWidth",.1}};
    collection(f.doc,"tensionCables",{cable});result=f.solve();near(channel(result,"tensionCables",0,"Magnitude").values.front(),20,1e-8,"taut cable N");near(channel(result,"tensionCables",0,"ElasticEnergy").values.front(),.02,1e-10,"cable J");
    cable["freeLength"]=12.0;collection(f.doc,"tensionCables",{cable});result=f.solve();for(double x:channel(result,"tensionCables",0,"Magnitude").values)near(x,0,1e-12,"slack cable no force");
    collection(f.doc,"tensionCables",{});
    QJsonObject stop{{"id",0},{"name","stop"},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(0,5,5)},{"direction",xyz(1,0,0)},{"directionMode",1},{"side",1},{"stopPosition",3.0},{"stiffness",10.0},{"exponent",1.0},{"damping",0.0},{"dampingDepth",.1}};
    collection(f.doc,"softStops",{stop});result=f.solve();near(channel(result,"softStops",0,"Magnitude").values.front(),20,1e-8,"stop force N");near(channel(result,"softStops",0,"ForceI").values.front(),-20,1e-8,"stop opposes compression");
    stop["tableX"]=QJsonArray{0,1,2};stop["tableY"]=QJsonArray{0,3,7};stop["hasTable"]=true;collection(f.doc,"softStops",{stop});result=f.solve();near(channel(result,"softStops",0,"Magnitude").values.front(),7,1e-8,"tabular stop conversion");
}

void forceFamilies(){
    for(const QString& kind:QStringList{"scalarForces","scalarTorques","threePointForces","threePointTorques","springs","bushing"}){
        Fixture f;f.scenario(.002,.001);QJsonObject o{{"id",0},{"name",kind},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(5,5,15)},{"magnitude",.01},{"directionMode",1},{"axisMode",1},{"fixedDirection",xyz(0,0,1)},{"fixedAxis",xyz(0,0,1)},
            {"targetBodyId",-1},{"originBodyId",-1},{"localTarget",xyz(0,0,1)},{"localOrigin",xyz(0,0,0)}};
        if(kind=="springs"){o["springType"]="BodyPoint";o["stiffness"]=1.0;o["freeValue"]=9.0;o["installValue"]=10.0;o["dampingEnabled"]=true;o["damping"]=.01;}
        if(kind=="bushing"){
            o={{"id",0},{"name","bushing"},{"loadType","Bushing"},{"bodyId",0},{"baseBodyId",-1},{"actionPoint",xyz(5,5,5)},{"direction",xyz(0,0,1)},{"stiffness",QJsonArray{1,1,1,1,1,1}},{"damping",QJsonArray{.1,.1,.1,.1,.1,.1}},{"preloadForce",QJsonArray{0,0,1}},{"preloadTorque",QJsonArray{0,0,0}}};collection(f.doc,"vectorForces",{o});
        }else collection(f.doc,kind,{o});
        const auto result=f.solve();if(kind!="bushing")require(channel(result,kind,0,"Magnitude").values.front()>0,"missing real force output "+kind);
    }
    Fixture invalid;collection(invalid.doc,"scalarForces",{QJsonObject{{"id",0},{"name","bad formula"},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(5,5,0)},{"forceFunction","unknown_parameter*t"}}});
    NativeMotionInput input;QString error;require(prepareNativeMotion(invalid.cad,invalid.doc,0,input,error),error);RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Failed&&!error.isEmpty(),"invalid formula silently accepted");
}

void pidAndTrajectories(){
    Fixture f;f.scenario(.05,.001);collection(f.doc,"joints",{joint("Revolute")});collection(f.doc,"drives",{drive("Velocity","0")});
    collection(f.doc,"controlInputs",{QJsonObject{{"id",0},{"name","trajectory"},{"inputType","CustomCurve"},{"timeStart",0.0},{"timeEnd",.05},{"curvePoints",QJsonArray{QJsonArray{0,0},QJsonArray{.05,.1}}}}});
    collection(f.doc,"controllers",{QJsonObject{{"id",0},{"name","servo"},{"controllerType","PID"},{"enabled",true},{"targetDriveId",0},{"feedbackType","Position"},{"feedbackSource","DriveDof"},{"targetValue",0.0},{"targetInputId",0},{"feedbackSensorId",-1},{"feedbackSign",1.0},{"kp",20.0},{"ki",1.0},{"kd",0.0},{"outputLimit",10.0}}});
    const auto result=f.solve();near(channel(result,"controllers",0,"PidTarget").values.back(),.1,1e-9,"custom trajectory endpoint");
    require(channel(result,"controllers",0,"PidFeedback").values.back()>.05,"PID did not move actual body");
}

void geometryAndContact(){
    Fixture f;f.scenario(.002,.001);const int fixed=f.fixedPart(0,0,-10);
    TopTools_IndexedMapOfShape faces;TopExp::MapShapes(f.cad.part(fixed)->shape,TopAbs_FACE,faces);int selected=0;
    for(int i=1;i<=faces.Extent();++i){GProp_GProps p;BRepGProp::SurfaceProperties(faces(i),p);if(std::abs(p.CentreOfMass().Z())<1e-9)selected=i;}
    require(selected>0,"top face");auto j=joint("PointOnSurface",0,0,1);j["origin"]=xyz(5,5,0);j["occSurfaceFaces"]=QJsonArray{QJsonObject{{"partId",fixed},{"index",selected}}};collection(f.doc,"joints",{j});auto result=f.solve();channel(result,"joints",0,"ReactionForce");
    auto separated=j;separated["origin"]=xyz(5,5,100);collection(f.doc,"joints",{separated});NativeMotionInput rejected;QString rejectedError;
    require(!prepareNativeMotion(f.cad,f.doc,0,rejected,rejectedError)&&rejectedError.contains("初始贴合"),"off-surface initial point was forcibly assembled");
    TopTools_IndexedMapOfShape edges;TopExp::MapShapes(f.cad.part(fixed)->shape,TopAbs_EDGE,edges);int edgeIndex=0;
    for(int i=1;i<=edges.Extent();++i){GProp_GProps p;BRepGProp::LinearProperties(edges(i),p);const auto c=p.CentreOfMass();if(std::abs(c.Z())<1e-9&&std::abs(c.Y())<1e-9&&std::abs(c.X()-5)<1e-9)edgeIndex=i;}
    require(edgeIndex>0,"top edge");j=joint("PointOnCurve",0,0,1);j["origin"]=xyz(5,0,0);j["occCurveEdges"]=QJsonArray{QJsonObject{{"partId",fixed},{"index",edgeIndex}}};collection(f.doc,"joints",{j});result=f.solve();channel(result,"joints",0,"ReactionForce");
    separated=j;separated["origin"]=xyz(5,100,0);collection(f.doc,"joints",{separated});
    require(!prepareNativeMotion(f.cad,f.doc,0,rejected,rejectedError)&&rejectedError.contains("初始贴合"),"off-curve initial point was forcibly assembled");collection(f.doc,"joints",{j});
    f.scenario(1,.01);auto moving=f.doc.entity("bodies",0);moving["initialVelocity"]=xyz(10,0,0);collection(f.doc,"bodies",{moving,f.doc.entity("bodies",1)});
    NativeMotionInput boundaryInput;QString boundaryError;require(prepareNativeMotion(f.cad,f.doc,0,boundaryInput,boundaryError),boundaryError);
    require(solveNativeMotion(boundaryInput,result,boundaryError)==SolveStatus::Completed,boundaryError);
    require(result.termination==ResultTermination::GeometryBoundary&&!result.diagnostics.empty(),"real curve-boundary stop lost its explicit reason");
    require(result.frames.back().seconds>.5&&result.frames.back().seconds<1,"geometry boundary stop did not preserve actual partial timeline");
    moving["initialVelocity"]=xyz(0,0,0);collection(f.doc,"bodies",{moving,f.doc.entity("bodies",1)});f.scenario(.002,.001);
    collection(f.doc,"joints",{});auto s=f.doc.entity("scenarios",0);s["usePreset"]=true;s["presetMode"]="CONTACT";s["gravity"]=xyz(0,0,-9810);collection(f.doc,"scenarios",{s});
    collection(f.doc,"contacts",{QJsonObject{{"id",0},{"name","contact"},{"contactType","CADContact"},{"actionBodyId",0},{"baseBodyId",1},{"stiffness",1000.0},{"materialDamping",1.0},{"forceExponent",1.5},{"penetrationDepth",.0254},{"frictionEnabled",true},{"staticFriction",.3},{"dynamicFriction",.2},{"useConvexHullAction",true},{"useConvexHullBase",true}}});result=f.solve();require(result.frames.back().bodies.size()==2,"contact body data");
}

void curveOnCurveRollingAndSliding(){
    Fixture f;f.scenario(.05,.001);const int fixed=f.fixedPart(0,0,-10);QString error;
    int rawCylinder=0;require(f.cad.addPrimitive(Primitive::Cylinder,.5,.1,0,rawCylinder,error),error);
    gp_Trsf pose;pose.SetTranslation(gp_Vec(5,.5,0));const auto located=BRepBuilderAPI_Transform(f.cad.part(rawCylinder)->shape,pose,true).Shape();
    QTemporaryDir folder;require(folder.isValid(),"cylinder directory");const auto path=folder.filePath("roller.brep");require(BRepTools::Write(located,path.toUtf8().constData()),"roller BREP write");
    std::vector<int> imported;require(f.cad.importFile(path,imported,error),error);require(imported.size()==1,"roller solid count");const int roller=imported[0];
    auto moving=body(0,roller,false,5,.5,.05);moving["mass"]=1.0;moving["inertia"]=QJsonArray{.1,.1,.1,0,0,0};moving["initialVelocity"]=xyz(.5,0,0);
    f.doc.bodyPartBindings[0]={roller};collection(f.doc,"bodies",{moving,f.doc.entity("bodies",1)});
    TopTools_IndexedMapOfShape edgesI,edgesJ;TopExp::MapShapes(f.cad.part(roller)->shape,TopAbs_EDGE,edgesI);TopExp::MapShapes(f.cad.part(fixed)->shape,TopAbs_EDGE,edgesJ);
    int circle=0,line=0,farLine=0;
    for(int i=1;i<=edgesI.Extent();++i){const BRepAdaptor_Curve curve(TopoDS::Edge(edgesI(i)));if(curve.GetType()==GeomAbs_Circle&&std::abs(curve.Circle().Location().Z())<1e-9)circle=i;}
    for(int i=1;i<=edgesJ.Extent();++i){GProp_GProps props;BRepGProp::LinearProperties(edgesJ(i),props);const auto p=props.CentreOfMass();if(std::abs(p.Z())<1e-9&&std::abs(p.X()-5)<1e-9){if(std::abs(p.Y())<1e-9)line=i;if(std::abs(p.Y()-10)<1e-9)farLine=i;}}
    require(circle>0&&line>0&&farLine>0,"real circle and straight CAD edges missing");
    auto j=joint("CurveOnCurve",0,0,1);j["origin"]=xyz(5,0,0);j["occCurveIEdges"]=QJsonArray{QJsonObject{{"partId",roller},{"index",circle}}};j["occCurveJEdges"]=QJsonArray{QJsonObject{{"partId",fixed},{"index",line}}};
    auto scenario=f.doc.entity("scenarios",0);scenario["integratorType"]="HHT";scenario["integratorMaxIters"]=80;scenario["integratorTol"]=1e-10;collection(f.doc,"scenarios",{scenario});
    collection(f.doc,"vectorForces",{QJsonObject{{"id",0},{"name","tangential load"},{"loadType","Force"},{"bodyId",0},{"actionPoint",xyz(5,.5,.05)},{"direction",xyz(1,0,0)},{"magnitude",.001},{"magnitudeMode","Constant"},{"directionMode","GlobalFixed"}}});
    for(bool sliding:{false,true}){
        j["allowSliding"]=sliding;collection(f.doc,"joints",{j});moving["initialAngularVelocity"]=xyz(0,0,(sliding?.7:-1.0)*57.29577951308232);collection(f.doc,"bodies",{moving,f.doc.entity("bodies",1)});
        const auto result=f.solve();channel(result,"joints",0,"ReactionForce");double maxSlip=0,minSlip=1e100;
        for(const auto& frame:result.frames){
            const auto& state=*std::find_if(frame.bodies.begin(),frame.bodies.end(),[](const BodyState& value){return value.bodyId==0;});
            near(state.positionMm[1],.5,2e-6,"rolling circle height");near(state.positionMm[2],.05,2e-6,"rolling plane placement");
            const double omega=state.angularVelocityDegS[2]/57.29577951308232;
            const double slip=std::abs(state.velocityMmS[0]+omega*.5);maxSlip=std::max(maxSlip,slip);minSlip=std::min(minSlip,slip);
            if(sliding)near(omega,.7,2e-5,"sliding contact must not invent rolling torque");
        }
        const auto& last=*std::find_if(result.frames.back().bodies.begin(),result.frames.back().bodies.end(),[](const BodyState& value){return value.bodyId==0;});
        if(sliding){require(minSlip>.5,"allowSliding did not allow physical slip");near(last.velocityMmS[0],.55,.002,"sliding force acceleration");}
        else{require(maxSlip<1e-4,"rolling curve constraint permits material slip");near(last.velocityMmS[0],.5+.05/1.4,.002,"rolling inertia changes acceleration");}
    }
    j["occCurveJEdges"]=QJsonArray{QJsonObject{{"partId",fixed},{"index",farLine}}};collection(f.doc,"joints",{j});NativeMotionInput input;
    require(!prepareNativeMotion(f.cad,f.doc,0,input,error)&&error.contains("初始贴合"),"separated curve pair was forcibly assembled");
}

void failureAndCancellation(){
    for(const QString& type:QStringList{"Line","Centripetal","Coupling","Gear"}){Fixture f;collection(f.doc,"joints",{joint(type)});NativeMotionInput input;QString error;require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Failed&&!error.isEmpty(),"unsupported joint silently disappeared");}
    for(const QString& solver:QStringList{"ADDM","BICGSTAB"}){Fixture f;auto scenario=f.doc.entity("scenarios",0);scenario["solverType"]=solver;collection(f.doc,"scenarios",{scenario});NativeMotionInput input;QString error;require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Failed&&error.contains(solver),"unimplemented solver silently fell back to PSOR");}
    Fixture f;NativeMotionInput input;QString error;require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);RigidResult preserved;preserved.scenarioId=777;SolverCallbacks stop;stop.stopRequested=[](){return true;};require(solveNativeMotion(input,preserved,error,stop)==SolveStatus::Stopped&&preserved.scenarioId==777,"cancellation replaced previous result");
}

void limitsDisabledAndSpringAxis(){
    Fixture f;f.scenario(.00001,.000001);auto j=joint("Revolute");
    j["limit"]=QJsonObject{{"enabled",true},{"lowerEnabled",false},{"upperEnabled",true},{"upperValue",5.0},{"penaltyOnly",true},{"upperCushion",10.0},{"upperStiffness",2.0},{"upperDamping",0.0}};
    collection(f.doc,"joints",{j});auto result=f.solve();
    const double expected=-10*1000/(100.0/3)*57.29577951308232;
    near(channel(result,"bodies",0,"AngularAcceleration").values[2],expected,.002,"Nmm/degree soft rotational limit");
    auto limit=j.value("limit").toObject();limit["penaltyOnly"]=false;j["limit"]=limit;collection(f.doc,"joints",{j});NativeMotionInput input;QString error;require(prepareNativeMotion(f.cad,f.doc,0,input,error),error);RigidResult previous;require(solveNativeMotion(input,previous,error)==SolveStatus::Failed,"hard boxed limit accepted by incompatible direct solver");
    Fixture disabled;collection(disabled.doc,"tensionCables",{QJsonObject{{"id",0},{"name","disabled"},{"enabled",false},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(20,5,5)},{"freeLength",2.0},{"stiffness",100.0}}});
    collection(disabled.doc,"softStops",{QJsonObject{{"id",0},{"name","disabled"},{"enabled",false},{"bodyIId",0},{"bodyJId",-1},{"localI",xyz(5,5,5)},{"localJ",xyz(0,5,5)},{"direction",xyz(1,0,0)},{"directionMode",1},{"side",1},{"stopPosition",0.0},{"stiffness",100.0},{"exponent",1.0}}});
    result=disabled.solve();for(const auto& c:result.channels)require(c.sourceType!="tensionCables"&&c.sourceType!="softStops","disabled force was created");near(result.frames.back().bodies[0].positionMm[0],5,1e-12,"disabled loads leave free body stationary");
    Fixture spring;spring.scenario(.001,.0001);j=joint("Translational");j["axis"]=xyz(1,0,0);collection(spring.doc,"joints",{j});collection(spring.doc,"springs",{QJsonObject{{"id",0},{"name","joint axis spring"},{"springType","Translational"},{"jointId",0},{"stiffness",1.0},{"freeValue",9.0},{"installValue",10.0},{"dampingEnabled",false}}});
    result=spring.solve();const auto& force=channel(result,"springs",0,"ForceI");near(force.values[0],1,1e-10,"linear spring follows global joint X");near(force.values[1],0,1e-10,"no wrong-axis Y spring force");near(force.values[2],0,1e-10,"no wrong-axis Z spring force");
}

void couplingFamilies(){
    for(const QString& kind:QStringList{"Gear","GearRack","Cable"}){
        Fixture f;f.scenario(.003,.001);const int other=f.fixedPart(20,0,0);
        auto bodies=f.doc.collection("bodies");auto b=bodies[1].toObject();b["isFixed"]=false;bodies[1]=b;collection(f.doc,"bodies",bodies);
        auto a=joint(kind=="Cable"?"Translational":"Revolute",0,0,-1),c=joint(kind=="Gear"?"Revolute":"Translational",1,1,-1);
        c["origin"]=xyz(25,5,5);
        if(kind=="Cable"){a["axis"]=xyz(1,0,0);c["axis"]=xyz(1,0,0);}
        if(kind=="GearRack")c["axis"]=xyz(0,1,0);
        collection(f.doc,"joints",{a,c});collection(f.doc,"couplings",{QJsonObject{{"id",0},{"name",kind},{"couplingType",kind},{"firstJointId",0},{"secondJointId",1},{"gearRatio",1.0},{"toothCount1",20},{"toothCount2",20},{"pitchRadius1",10.0},{"pitchRadius2",10.0},{"radius",10.0},{"ratio",2.0},{"reverse",false}}});
        if(kind=="Cable")collection(f.doc,"drives",{drive("Velocity","10",0,0,"Translation")});
        const auto result=f.solve();channel(result,"couplings",0,"ReactionForce");
        if(kind=="Cable"){
            const auto& p=channel(result,"bodies",1,"Position");near(p.values[p.values.size()-3]-25,.06,1e-8,"bilateral cable ratio remains separate from tension law");
        }
        (void)other;
    }
}

void flexReductionAndAutomaticMesh(){
    Fixture f;f.scenario(.0002,.0001);auto j=joint("Fixed");j["origin"]=xyz(0,5,5);collection(f.doc,"joints",{j});
    auto scenario=f.doc.entity("scenarios",0);scenario["gravity"]=xyz(0,-1000,0);collection(f.doc,"scenarios",{scenario});
    QJsonObject flex{{"id",0},{"name","cantilever"},{"targetBodyId",0},{"enabled",true},{"modeCount",2},{"dampingType",1},{"modalDampingRatio",.02},{"connectionRadius",3.0},{"elementSize",4.0},
        {"material",QJsonObject{{"youngModulus",100.0},{"poissonRatio",.3},{"density",.001}}},
        {"mesh",QJsonObject{{"nodes",QJsonArray{QJsonArray{0,0,0},QJsonArray{10,0,0},QJsonArray{10,10,0},QJsonArray{0,10,0},QJsonArray{0,0,10},QJsonArray{10,0,10},QJsonArray{10,10,10},QJsonArray{0,10,10}}},
                           {"tetrahedra",QJsonArray{QJsonArray{0,1,3,4},QJsonArray{1,2,3,6},QJsonArray{1,3,4,6},QJsonArray{1,4,5,6},QJsonArray{3,4,6,7}}}}},
        {"attachGroups",QJsonArray{QJsonArray{0,3,4,7}}}};
    collection(f.doc,"autoFlexBodies",{flex});auto result=f.solve();
    require(result.flexBodies.size()==1&&result.flexBodies[0].nodePositions.size()==result.frames.size(),"actual flex playback frames missing");
    require(result.flexBodies[0].referenceNodes.size()==24&&!result.flexBodies[0].surfaceTriangles.empty(),"flex topology missing");
    require(channel(result,"autoFlexBodies",0,"ModalCoordinates").components==2,"modal coordinates missing");
    double displacement=0;const auto& moved=result.flexBodies[0].nodePositions.back();for(std::size_t i=0;i<moved.size();++i)displacement=std::max(displacement,std::abs(moved[i]-result.flexBodies[0].referenceNodes[i]));
    require(displacement>1e-10,"flex gravity produced no deformation");
    flex.remove("mesh");flex.remove("attachGroups");collection(f.doc,"autoFlexBodies",{flex});result=f.solve();
    require(result.flexBodies.size()==1&&result.flexBodies[0].referenceNodes.size()>24,"automatic OCC Netgen tetra mesh did not run");
}

void originalCurveRequests(){
    Fixture f;
    auto scenario=f.doc.entity("scenarios",0);
    scenario["curveRequests"]=QJsonArray{QJsonObject{{"requestId",0},{"requestName",QStringLiteral("原始位移幅值")},
        {"sourceType","Body"},{"sourceId",0},{"variable","Displacement"},{"component","Magnitude"},
        {"visible",false},{"color",QJsonObject{{"r",18},{"g",52},{"b",86},{"a",255}}}}};
    collection(f.doc,"scenarios",{scenario});
    RigidModel model;model.nativeDocument=std::make_shared<NativeMotionDocument>(f.doc);
    syncNativeMotionModel(model);
    require(model.scenarios.size()==1&&model.scenarios.front().id==0,"original scenario ID zero lost during UI sync");
    require(model.scenarios.front().curveRequests.size()==1,"original curve request dropped during UI sync");
    const auto& request=model.scenarios.front().curveRequests.front();
    require(request.metric==RigidCurveMetric::NativeChannel&&request.sourceType=="bodies"&&request.sourceId==0
        &&request.variable=="Displacement"&&request.component==-1,"original native curve key or magnitude changed");
    require(request.name==QStringLiteral("原始位移幅值")&&!request.visible&&request.color=="#123456",
        "original curve name, visibility or RGB color changed");
    auto exported = *model.nativeDocument;
    auto exportedScenario = exported.entity("scenarios",0);
    auto exportedCurves = exportedScenario.value("curveRequests").toArray();
    auto exportedCurve = exportedCurves[0].toObject();
    exportedCurve["sourceType"]="bodies";exportedCurve["component"]=-1;exportedCurve["color"]="#123456";
    exportedCurves[0]=exportedCurve;exportedScenario["curveRequests"]=exportedCurves;collection(exported,"scenarios",{exportedScenario});
    QTemporaryDir folder;require(folder.isValid(),"native curve export directory");QString exportError;
    const auto path=folder.filePath("curves.motion.json");
    const bool exportedOk=writeNativeMotionDocument(path,exported,exportError);
    require(exportedOk,"native curve JSON export: "+exportError);
    NativeMotionDocument reread;const bool importedOk=readNativeMotionDocument(path,reread,exportError);
    require(importedOk,"native curve JSON reimport: "+exportError);
    RigidModel imported;imported.nativeDocument=std::make_shared<NativeMotionDocument>(reread);syncNativeMotionModel(imported);
    require(imported.scenarios.size()==1&&imported.scenarios.front().curveRequests.size()==1
        &&imported.scenarios.front().curveRequests.front().component==-1
        &&imported.scenarios.front().curveRequests.front().sourceType=="bodies",
        "original JSON export/reimport lost native integer component or canonical source");
    model.scenarios.front().curveRequests.front().name="edited";
    syncNativeMotionModel(model);
    require(model.scenarios.front().curveRequests.front().name=="edited","model sync reset edited curve requests");
    model.scenarios.front().curveRequests.clear();
    syncNativeMotionModel(model);
    require(model.scenarios.front().curveRequests.empty(),"model sync re-created an intentionally cleared curve list");
}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        originalCurveRequests();
        pinchedPlanarTessellation();
        drivesAndSensors();exactAcceptedTimeline();inertiaConventionsWithCadEvidence();closedLoopProjectionAndConflict();jointFamilies();forcesAndUnilateral();forceFamilies();pidAndTrajectories();geometryAndContact();curveOnCurveRollingAndSliding();failureAndCancellation();
        limitsDisabledAndSpringAxis();couplingFamilies();flexReductionAndAutomaticMesh();
        qInfo()<<"Native solver parity smoke passed: original zero IDs, 15 joint families, expressions/tables, forces/springs/bushing, PID trajectory, OCC curves/surface/contact, tension cable and stop units.";return 0;
    }catch(const std::exception& error){qCritical().noquote()<<"Native solver smoke failed:"<<error.what();return 1;}
}
