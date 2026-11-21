#include "CadDocument.h"
#include "NativeMotionLoadExport.h"
#include "NativeMotionSolver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
void near(double actual,double expected,double tolerance,const QString& message)
{
    require(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,
        QStringLiteral("%1: %2 != %3").arg(message).arg(actual,0,'g',16).arg(expected,0,'g',16));
}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
QByteArray bytes(const QString& path){QFile file(path);require(file.open(QIODevice::ReadOnly),file.errorString());return file.readAll();}

void physicalEquilibriumAndExport()
{
    CadDocument cad;QString error;int part=0;
    require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
    const QJsonObject body{{"id",0},{"name","body, one"},{"geoEntityIds",QJsonArray{part}},
        {"isFixed",false},{"mass",1.0},{"isCustomMassProps",true},{"centerOfMass",xyz(5,5,5)},
        {"inertia",QJsonArray{100.0/6,100.0/6,100.0/6,0,0,0}},
        {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}};
    const QJsonObject joint{{"id",0},{"name","root, support"},{"jointType","Fixed"},{"actionBodyId",0},
        {"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(0,0,1)}};
    const QJsonObject scenario{{"id",0},{"name","equilibrium"},{"endTime",.002},{"stepSize",.001},
        {"gravity",xyz(0,0,-9810)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},
        {"solverType","SPARSE_QR"},{"solverTolerance",1e-10}};
    NativeMotionDocument document;
    document.root={{"formatVersion",1},{"motionData",QJsonObject{{"bodies",QJsonArray{body}},
        {"joints",QJsonArray{joint}},{"scenarios",QJsonArray{scenario}}}}};
    document.bodyPartBindings.insert(0,{part});NativeMotionInput input;
    require(prepareNativeMotion(cad,document,0,input,error),error);
    RigidResult result;require(solveNativeMotion(input,result,error)==SolveStatus::Completed,error);
    std::vector<NativeLoadSample> i,j;
    require(nativeLoadSeries(input,result,{"joints",0},NativeLoadEnd::I,i,error),error);
    require(nativeLoadSeries(input,result,{"joints",0},NativeLoadEnd::J,j,error),error);
    near(i.back().forceN[2],9.81,1e-8,"I support force must balance 1 kg gravity");
    near(i.back().torqueNmm[1],-49.05,1e-7,"I support moment must balance 5 mm eccentric gravity");
    near(j.back().forceN[2],-9.81,1e-8,"J support reaction sign");
    near(j.back().torqueNmm[1],49.05,1e-7,"J support reaction moment sign and N mm units");
    QTemporaryDir directory;require(directory.isValid(),"temporary load exports");
    NativeLoadExportOptions options;options.frames=NativeLoadFrames::All;options.name="load, \"case\"";
    const auto csvPath=directory.filePath("loads.csv");
    require(exportNativeLoads(csvPath,input,result,{"joints",0},options,error),error);
    const auto csv=bytes(csvPath);
    require(csv.contains("Fz_N")&&csv.contains("My_Nmm")&&csv.contains("\"root, support\"")
        &&csv.contains("\"load, \"\"case\"\"\""),"CSV units or quoting");
    require(csv.count('\n')==static_cast<int>(result.frames.size()+1),"all frames CSV count");
    options.format=NativeLoadFormat::NastranBdf;options.frames=NativeLoadFrames::Specified;options.frameIndex=2;options.nodeId=123;
    options.useTargetPoint=true;options.targetPointMm={5,5,5};
    const auto bdfPath=directory.filePath("loads.bdf");
    require(exportNativeLoads(bdfPath,input,result,{"joints",0},options,error),error);
    const auto bdf=bytes(bdfPath);
    require(bdf.contains("FORCE,3,123,0,1.0,")&&bdf.contains("MOMENT,3,123,0,1.0,"),"BDF frame SID/grid mapping");
    // Transferring the clamp wrench to the COM cancels its eccentric moment.
    const auto momentStart=bdf.indexOf("MOMENT,");const auto moment=bdf.mid(momentStart).split('\n').front().split(',');
    require(moment.size()==8,"BDF moment field count");
    near(moment[6].toDouble(),0,1e-7,"moment transfer to COM omitted r cross F");
    options.nodeId=0;
    require(!exportNativeLoads(bdfPath,input,result,{"joints",0},options,error)&&bytes(bdfPath)==bdf,
        "failed BDF validation overwrote prior export");
}

struct Synthetic {
    NativeMotionInput input;
    RigidResult result;
    Synthetic(){
        input.scenario.id=0;input.scenario.name="known extrema";result.scenarioId=0;
        input.document.root={{"motionData",QJsonObject{
            {"bodies",QJsonArray{QJsonObject{{"id",0},{"name","analysis"},{"isFixed",false}},QJsonObject{{"id",1},{"name","other"},{"isFixed",false}}}},
            {"joints",QJsonArray{
                QJsonObject{{"id",0},{"name","I joint"},{"jointType","Fixed"},{"actionBodyId",0},{"baseBodyId",-1}},
                QJsonObject{{"id",1},{"name","J joint"},{"jointType","Fixed"},{"actionBodyId",1},{"baseBodyId",0}}}}}}};
        const double h=std::sqrt(.5);
        for(int frame=0;frame<7;++frame){
            RigidFrame sample;sample.seconds=frame;
            BodyState body;body.bodyId=0;body.positionMm=frame?Vector3d{11,22,33}:Vector3d{1,2,3};
            body.rotation=frame?Quaterniond{h,0,0,h}:Quaterniond{1,0,0,0};sample.bodies.push_back(body);
            result.frames.push_back(sample);
        }
        const std::vector<double> a{0,-2,0,-1,0,0,0},b{0,0,1,0,4,0,0};
        for(int id=0;id<2;++id){
            add(id,"ReactionForce","N",[&](int frame){return id?Vector3d{0,b[frame],0}:Vector3d{a[frame],0,0};});
            add(id,"ReactionTorque",QStringLiteral("N·mm"),[](int){return Vector3d{};});
            for(const QString& key:QStringList{"Position","PositionJ"})
                add(id,key,"mm",[](int frame){return frame?Vector3d{11,23,33}:Vector3d{2,2,3};});
        }
    }
    void add(int id,const QString& name,const QString& unit,const std::function<Vector3d(int)>& value){
        ResultChannel channel;channel.sourceType="joints";channel.sourceId=id;channel.variable=name;channel.unit=unit;channel.components=3;
        for(int frame=0;frame<7;++frame){auto sample=value(frame);channel.values.insert(channel.values.end(),sample.begin(),sample.end());}
        result.channels.push_back(channel);
    }
};

void extremaAndBodyInitial()
{
    Synthetic f;QString error;QJsonObject output;
    NativeLoadCaseOptions options;options.analysisBodyId=0;options.jointIds={0,1};options.peaksPerSignal=1;
    require(buildNativeLoadCases(f.input,f.result,options,output,error),error);
    auto cases=output.value("cases").toArray();
    require(cases.size()==2,"per-signal top extrema selection");
    near(cases[0].toObject().value("timeSeconds").toDouble(),1,0,"first peak time");
    near(cases[1].toObject().value("timeSeconds").toDouble(),4,0,"second peak time");
    const auto firstLoad=cases[0].toObject().value("loads").toArray().first().toObject();
    require(firstLoad.value("end").toString()=="I","feature I body end");
    near(firstLoad.value("components").toObject().value("FY").toDouble(),-2,1e-14,"force inverse rotation to initial body");
    auto point=firstLoad.value("actionPoint").toArray();
    near(point[0].toDouble(),2,1e-14,"point inverse rotation and translation X");
    near(point[1].toDouble(),2,1e-14,"point inverse rotation and translation Y");
    const auto secondLoad=cases[1].toObject().value("loads").toArray().first().toObject();
    require(secondLoad.value("end").toString()=="J","feature J body end");
    near(secondLoad.value("components").toObject().value("FX").toDouble(),4,1e-14,"J physical reaction rotated to initial body");
    options.synchronized=true;require(buildNativeLoadCases(f.input,f.result,options,output,error),error);
    cases=output.value("cases").toArray();
    for(const auto& entry:cases)require(entry.toObject().value("loads").toArray().size()==2,"synchronous case omitted non-extremal joint");
    options.mergeTolerance=3;require(buildNativeLoadCases(f.input,f.result,options,output,error),error);
    cases=output.value("cases").toArray();require(cases.size()==1,"time merging");
    const auto loads=cases[0].toObject().value("loads").toArray();
    near(loads[1].toObject().value("components").toObject().value("FM").toDouble(),0,0,
        "merged case mixed loads from different original times");
    auto mismatched=f.result;mismatched.channels.front().unit="mN";
    std::vector<NativeLoadSample> values;
    require(!nativeLoadSeries(f.input,mismatched,{"joints",0},NativeLoadEnd::I,values,error),"wrong force units silently exported");
    auto separated=f.result;
    for(auto& channel:separated.channels)if(channel.sourceId==0&&channel.variable=="PositionJ")
        for(std::size_t frame=0;frame<separated.frames.size();++frame)channel.values[3*frame+2]+=1;
    require(nativeLoadSeries(f.input,separated,{"joints",0},NativeLoadEnd::I,values,error),error);
    near(values[1].torqueNmm[1],2,1e-14,"J to I wrench transfer omitted endpoint lever arm");
    auto flat=f.result;for(auto& channel:flat.channels)if(channel.variable.startsWith("Reaction"))
        std::fill(channel.values.begin(),channel.values.end(),0);
    require(!buildNativeLoadCases(f.input,flat,options,output,error),"flat history invented peak cases");
}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        physicalEquilibriumAndExport();extremaAndBodyInitial();
        qInfo()<<"PASS: physical I/J signs, wrench transfer, CSV/BDF, extrema and synchronous body-initial load cases";
        return 0;
    }catch(const std::exception& exception){qCritical()<<exception.what();return 1;}
}
