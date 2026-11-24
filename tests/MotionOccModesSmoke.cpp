#include "CadDocument.h"
#include "NativeMotionSolver.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QTemporaryDir>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool yes,const QString& reason){if(!yes)throw std::runtime_error(reason.toStdString());}
void expectNear(double actual,double expected,double tolerance,const QString& reason){require(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,
    QStringLiteral("%1: %2 vs %3").arg(reason).arg(actual,0,'g',17).arg(expected,0,'g',17));}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}

struct Fixture {
    CadDocument cad;
    NativeMotionDocument document;
    Fixture(){
        int part=0;QString error;require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        const QJsonObject body{{"id",0},{"name","modal cube"},{"geoEntityIds",QJsonArray{part}},
            {"isCustomMassProps",true},{"mass",1},{"centerOfMass",xyz(5,5,5)},{"inertia",QJsonArray{17,17,17,0,0,0}}};
        const QJsonObject mesh{{"nodes",QJsonArray{QJsonArray{0,0,0},QJsonArray{10,0,0},QJsonArray{10,10,0},QJsonArray{0,10,0},
            QJsonArray{0,0,10},QJsonArray{10,0,10},QJsonArray{10,10,10},QJsonArray{0,10,10}}},
            {"tetrahedra",QJsonArray{QJsonArray{0,1,3,4},QJsonArray{1,2,3,6},QJsonArray{1,3,4,6},QJsonArray{1,4,5,6},QJsonArray{3,4,6,7}}}};
        const QJsonObject flex{{"id",0},{"name","real basis"},{"targetBodyId",0},{"enabled",true},{"modeCount",4},
            {"dampingType",0},{"material",QJsonObject{{"youngModulus",100},{"poissonRatio",.3},{"density",.001}}},
            // Wrap the inner array in an explicit QJsonValue: a single same-type
            // element in QJsonArray{...} list-initializes via the copy constructor
            // on MSVC (flattening [[0,3,4,7]] to [0,3,4,7]) but via the
            // initializer_list constructor on GCC, which nests as intended.
            {"mesh",mesh},{"attachGroups",QJsonArray{QJsonValue(QJsonArray{0,3,4,7})}}};
        QJsonObject data;
        data.insert("bodies",QJsonArray{body});data.insert("autoFlexBodies",QJsonArray{flex});
        data.insert("scenarios",QJsonArray{QJsonObject{{"id",0},{"endTime",1},{"stepSize",.01},{"gravity",xyz(0,0,0)}}});
        document.root={{"formatVersion",1},{"motionData",data}};
        document.bodyPartBindings.insert(0,{part});
    }
    void flexField(const QString& name,const QJsonValue& value){auto data=document.root.value("motionData").toObject();auto flex=document.entity("autoFlexBodies",0);
        flex[name]=value;data["autoFlexBodies"]=QJsonArray{flex};document.root["motionData"]=data;}
    RigidFlexResult inspect(){NativeMotionInput input;QString error;require(prepareNativeMotion(cad,document,0,input,error),error);
        RigidFlexResult result;require(inspectNativeFlexModes(input,0,result,error)==SolveStatus::Completed,error);return result;}
};

void physicalModes(){
    Fixture fixture;const auto original=fixture.document.root;
    const auto baseline=fixture.inspect();QString error;require(validateFlexModes(baseline,error),error);
    require(fixture.document.root==original,"inspection changed design JSON");
    require(baseline.modalBasis.kind=="FixedInterface","clamped modes mislabeled");
    require(baseline.modalBasis.frequenciesHz.size()==4&&baseline.modalBasis.requestedCount==4,"requested/actual mode count mismatch");
    const auto& frequencies=baseline.modalBasis.frequenciesHz;
    for(std::size_t mode=0;mode<frequencies.size();++mode){
        require(frequencies[mode]>0,"clamped elastic mode has zero frequency");
        if(mode)require(frequencies[mode]>=frequencies[mode-1],"frequencies not ordered");
        FlexModeSample sample;require(sampleFlexMode(baseline,int(mode),.2,1,sample,error),error);
        expectNear(*std::max_element(sample.magnitudes.begin(),sample.magnitudes.end()),1,1e-12,"shape not normalized by vector norm");
        for(int node:{0,3,4,7})expectNear(sample.magnitudes[node],0,1e-8,"clamped attachment moved in fixed-interface mode");
    }
    auto material=fixture.document.entity("autoFlexBodies",0).value("material").toObject();
    material["youngModulus"]=400;fixture.flexField("material",material);const auto stiff=fixture.inspect();
    for(std::size_t mode=0;mode<frequencies.size();++mode)expectNear(stiff.modalBasis.frequenciesHz[mode]/frequencies[mode],2,2e-7,"f must scale with sqrt(E)");
    material["youngModulus"]=100;material["density"]=.004;fixture.flexField("material",material);const auto heavy=fixture.inspect();
    for(std::size_t mode=0;mode<frequencies.size();++mode)expectNear(heavy.modalBasis.frequenciesHz[mode]/frequencies[mode],.5,2e-7,"f must scale with inverse sqrt(rho)");
    material["density"]=.001;fixture.flexField("material",material);
    const double cutoff=(frequencies[1]+frequencies[2])*.5;fixture.flexField("freqCutoff",cutoff);
    const auto cut=fixture.inspect();require(cut.modalBasis.frequenciesHz.size()>=1&&cut.modalBasis.frequenciesHz.size()<4,"cutoff did not remove modes");
    for(double f:cut.modalBasis.frequenciesHz)require(f<=cutoff*(1+1e-12),"mode exceeds cutoff");
    fixture.flexField("modeCount",1);fixture.flexField("freqCutoff",frequencies[0]*.5);
    NativeMotionInput input;require(prepareNativeMotion(fixture.cad,fixture.document,0,input,error),error);
    RigidFlexResult untouched;untouched.flexId=747;
    require(inspectNativeFlexModes(input,0,untouched,error)==SolveStatus::Failed&&untouched.flexId==747,"single-mode cutoff fabricated a retained mode");
    require(inspectNativeFlexModes(input,0,untouched,error,{[]{return true;},{}})==SolveStatus::Stopped,"modal cancellation not honored");
    qInfo()<<"real fixed-interface frequencies Hz:"<<frequencies[0]<<frequencies[1]<<frequencies[2]<<frequencies[3];
}

void displayAndExport(){
    Fixture fixture;const auto result=fixture.inspect();const auto raw=result.modalBasis.shapes;
    FlexModeSample positive,negative,zero;QString error;
    require(sampleFlexMode(result,0,2,1,positive,error)&&sampleFlexMode(result,0,2,-1,negative,error)
        &&sampleFlexMode(result,0,2,0,zero,error),error);
    for(std::size_t row=0;row<result.referenceNodes.size();++row){
        expectNear((positive.positions[row]+negative.positions[row])*.5,result.referenceNodes[row],1e-12,"signed mode phase");
        expectNear(zero.positions[row],result.referenceNodes[row],0,"zero phase changed geometry");
    }
    for(double value:zero.magnitudes)expectNear(value,0,0,"zero phase retained end-state color");
    require(result.modalBasis.shapes==raw,"preview changed solver basis");
    auto malformed=result;malformed.modalBasis.shapes.pop_back();const auto preserved=positive.positions;
    require(!sampleFlexMode(malformed,0,1,1,positive,error)&&positive.positions==preserved,"invalid basis overwrote display");
    malformed=result;malformed.modalBasis.frequenciesHz[0]=std::numeric_limits<double>::quiet_NaN();
    require(!validateFlexModes(malformed,error),"nonfinite frequency accepted");
    QTemporaryDir directory;require(directory.isValid(),"temporary export directory failed");
    const auto path=directory.filePath("modes.csv");require(exportFlexModesCsv(path,result,error),error);
    QFile file(path);require(file.open(QIODevice::ReadOnly),file.errorString());const auto content=file.readAll();
    require(content.startsWith("basis,mode,frequency_hz,node_index_0")&&content.count('\n')==1+int(result.referenceNodes.size()/3*result.modalBasis.frequenciesHz.size()),"CSV basis dimensions lost");
    file.close();require(!exportFlexModesCsv(path,malformed,error),"malformed basis exported");require(file.open(QIODevice::ReadOnly)&&file.readAll()==content,"failed export replaced valid file");
}

void fullFemContour(){
    RigidFlexResult result;result.reducedDynamics=false;result.interfaceCount=2;
    result.referenceNodes={0,0,0, 1,0,0, 0,1,0, 0,0,1, -2,0,0};
    result.tetrahedra={0,1,2,3, 0,4,2,3};
    const auto frame=[&](double scale,double direction){
        auto positions=result.referenceNodes;
        for(std::size_t i=0;i<positions.size();i+=3){
            positions[i]=20-direction*scale*result.referenceNodes[i+1];
            positions[i+1]=direction*scale*result.referenceNodes[i]-5;
            positions[i+2]=scale*result.referenceNodes[i+2]+3;
        }
        return positions;
    };
    result.nodePositions={frame(1,1),frame(1,-1),frame(1.2,1),frame(.8,-1)};
    QString error;std::vector<double> values;
    require(flexNodeScalars(result,0,FlexContourKind::ElasticDisplacement,values,error),error);
    for(double value:values)expectNear(value,0,1e-12,"full FEM rigid motion leaked into elastic contour");
    require(flexNodeScalarsAt(result,0,1,.5,FlexContourKind::ElasticDisplacement,values,error),error);
    for(double value:values)expectNear(value,0,1e-12,"interpolated rigid motion produced fake contraction");
    prepareAutoFlexContour(result);
    require(flexNodeScalarsAt(result,2,3,.5,FlexContourKind::ElasticDisplacement,values,error),error);
    for(double value:values)expectNear(value,0,1e-12,"opposite signed stretch must cancel after rigid fit");
    // The two tetrahedra have a 1:2 volume ratio. Their combined mass center
    // is (-.25,.25,.25). A prestretched first frame uses the undeformed mesh.
    result.nodePositions.erase(result.nodePositions.begin(),result.nodePositions.begin()+2);
    require(flexNodeScalars(result,0,FlexContourKind::ElasticDisplacement,values,error),error);
    for(std::size_t i=0;i<values.size();++i)
        expectNear(values[i],.2*std::hypot(result.referenceNodes[3*i]+.25,
            result.referenceNodes[3*i+1]-.25,result.referenceNodes[3*i+2]-.25),1e-12,
            "contour must use the volume-weighted stress-free reference, not the preloaded frame");
}
}

int main(int argc,char** argv){QCoreApplication app(argc,argv);try{physicalModes();displayAndExport();fullFemContour();qInfo()<<"MotionOcc.Modes passed";return 0;}
    catch(const std::exception& exception){qCritical()<<exception.what();return 1;}}
