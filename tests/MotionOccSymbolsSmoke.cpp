#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeMotionSymbols.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonArray>

#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
namespace {
void require(bool condition,const QString& message){if(!condition)throw std::runtime_error(message.toStdString());}
void expectNear(double value,double expected,double tolerance,const QString& name){require(std::isfinite(value)&&std::abs(value-expected)<=tolerance,QStringLiteral("%1: %2 != %3").arg(name).arg(value,0,'g',17).arg(expected,0,'g',17));}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
const MotionSymbol& find(const QVector<MotionSymbol>& symbols,const QString& collection,int id){for(const auto& symbol:symbols)if(symbol.collection==collection&&symbol.id==id)return symbol;throw std::runtime_error("symbol missing");}
}

int main(int argc,char** argv)
{
    QCoreApplication app(argc,argv);
    try{
        CadDocument cad;int part=0;QString error;require(cad.addPrimitive(Primitive::Box,10,20,30,part,error),error);
        NativeMotionDocument doc;
        const QJsonObject body{{"id",7},{"name","body"},{"centerOfMass",xyz(5,10,15)}};
        const QJsonObject joint{{"id",4},{"name","revolute"},{"jointType","Revolute"},{"actionBodyId",7},{"baseBodyId",-1},{"origin",xyz(2,3,4)},{"axis",xyz(0,0,1)}};
        const QJsonObject spring{{"id",2},{"name","spring"},{"springType","BodyPoint"},{"bodyIId",7},{"bodyJId",-1},{"localI",xyz(5,10,15)},{"localJ",xyz(50,0,0)}};
        const QJsonObject cable{{"id",3},{"name","cable"},{"bodyIId",7},{"bodyJId",-1},{"localI",xyz(1,2,3)},{"localJ",xyz(20,30,40)}};
        const QJsonObject force{{"id",5},{"name","force"},{"loadType","Force"},{"bodyId",7},{"actionPoint",xyz(1,2,3)},{"direction",xyz(1,0,0)},{"directionMode","BodyFixed"},{"magnitude",1}};
        auto fixedForce=force;fixedForce["id"]=6;fixedForce["directionMode"]="GlobalFixed";
        auto stop=cable;stop["id"]=9;stop["direction"]=xyz(0,0,1);stop["directionMode"]=1;stop["stopPosition"]=10;stop["side"]=1;
        const QJsonObject hiddenMarker{{"id",8},{"visible",false},{"bodyId",7},{"localPosition",xyz(9,9,9)}};
        const QJsonObject data{{"bodies",QJsonArray{body}},{"joints",QJsonArray{joint}},
            {"springs",QJsonArray{spring}},{"tensionCables",QJsonArray{cable}},
            {"softStops",QJsonArray{stop}},{"vectorForces",QJsonArray{force,fixedForce}},
            {"markers",QJsonArray{hiddenMarker}},
            {"sensors",QJsonArray{QJsonObject{{"id",11},{"sensorType","BodyDisplacement"},
                {"targetBodyId",7},{"referencePoint",xyz(1,2,3)}}}}};
        doc.root={{"motionData",data}};
        doc.bodyPartBindings.insert(7,{part});
        const auto rest=nativeMotionSymbols(cad,doc,{},"joints",4);
        const auto& j=find(rest,"joints",4);require(j.emphasized&&!j.linePoints.empty(),"selected joint has no highlighted glyph");
        expectNear(j.origin[0],2,1e-12,"joint global origin");expectNear(find(rest,"bodies",7).origin[0],5,1e-12,"body global COM");
        for(const auto& s:rest){require(s.collection!="markers","hidden marker was displayed");require(s.linePoints.size()%2==0,"segments must be paired");}
        QMatrix4x4 pose;pose.translate(100,20,30);pose.rotate(90,0,0,1);
        const auto animated=nativeMotionSymbols(cad,doc,{{part,pose}},"tensionCables",3);
        const auto& c=find(animated,"tensionCables",3);
        require(c.emphasized&&c.partId==-1,"animated glyph would be transformed twice");
        // Initial global (1,2,3), rotated 90 deg then translated. The ground
        // endpoint must remain exactly (20,30,40); it does not follow body I.
        expectNear(c.linePoints[0][0],98,1e-5,"animated cable endpoint x");
        expectNear(c.linePoints[0][1],21,1e-5,"animated cable endpoint y");
        expectNear(c.linePoints[0][2],33,1e-5,"animated cable endpoint z");
        expectNear(c.linePoints[1][0],20,1e-12,"ground cable endpoint x");
        expectNear(c.linePoints[1][1],30,1e-12,"ground cable endpoint y");
        const auto& f=find(animated,"vectorForces",5);
        require(f.linePoints.size()>=2,"force arrow missing");
        expectNear(f.linePoints[1][0]-f.linePoints[0][0],0,1e-5,"body-fixed direction x");
        require(f.linePoints[1][1]>f.linePoints[0][1],"body-fixed force did not rotate");
        const auto& fixed=find(animated,"vectorForces",6);
        expectNear(fixed.linePoints[1][1]-fixed.linePoints[0][1],0,1e-5,"global-fixed direction y");
        require(fixed.linePoints[1][0]>fixed.linePoints[0][0],"global-fixed force rotated with body");
        const auto& limit=find(animated,"softStops",9);
        expectNear(limit.linePoints[1][0],20,1e-12,"stop reference plane x");
        expectNear(limit.linePoints[1][1],30,1e-12,"stop reference plane y");
        expectNear(limit.linePoints[1][2],50,1e-12,"stop position measured from reference endpoint");
        const auto& sensor=find(animated,"sensors",11);
        expectNear(sensor.origin[0],88,1e-5,"sensor COM-local point transformed x");
        expectNear(sensor.origin[1],26,1e-5,"sensor COM-local point transformed y");
        expectNear(sensor.origin[2],48,1e-5,"sensor COM-local point transformed z");
        const auto source=doc.root;
        nativeMotionSymbols(cad,doc,{{part,pose}});
        require(doc.root==source,"display helper changed solver data");
        // A flexible interface is a real body without a distinct CAD part;
        // glyphs must follow its own pose after the CAD rigid pose is removed.
        auto portDoc=doc;portDoc.bodyPartBindings.clear();
        const auto independent=nativeMotionSymbols(cad,portDoc,{},"joints",4,{{7,pose}});
        const auto& portJoint=find(independent,"joints",4);
        expectNear(portJoint.origin[0],97,1e-5,"independent port joint x stayed at rest");
        expectNear(portJoint.origin[1],22,1e-5,"independent port joint y stayed at rest");
        expectNear(find(independent,"sensors",11).origin[2],48,1e-5,"independent port sensor did not follow its actual body pose");
        qInfo()<<"Native symbols: topology-independent glyphs, global coordinates and separate body/ground animation passed";
        return 0;
    }catch(const std::exception& failure){qCritical()<<failure.what();return 1;}
}
