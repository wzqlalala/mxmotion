#include "NativeMotionSymbols.h"
#include "CadDocument.h"
#include "NativeMotionDocument.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Standard_Failure.hxx>

#include <QJsonArray>
#include <QQuaternion>
#include <QVector3D>

#include <algorithm>
#include <cmath>
#include <optional>

namespace MotionOcc {
namespace {
using Point = std::array<double, 3>;
constexpr double pi = 3.14159265358979323846;
const QColor jointColor(153,115,0), baseColor(38,125,177), springColor(134,70,174),
    forceColor(205,65,55), markerColor(36,146,95), bodyColor(108,117,128), sensorColor(20,142,153);

Point add(const Point& a, const Point& b) { return {a[0]+b[0],a[1]+b[1],a[2]+b[2]}; }
Point sub(const Point& a, const Point& b) { return {a[0]-b[0],a[1]-b[1],a[2]-b[2]}; }
Point mul(const Point& a, double n) { return {a[0]*n,a[1]*n,a[2]*n}; }
double norm(const Point& a) { return std::hypot(a[0],std::hypot(a[1],a[2])); }
Point cross(const Point& a, const Point& b) { return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}; }
Point unit(const Point& a) { const double n=norm(a); return std::isfinite(n)&&n>1e-12 ? mul(a,1/n) : Point{0,0,0}; }
bool finite(const Point& p) { return std::all_of(p.begin(),p.end(),[](double x){return std::isfinite(x);}); }
std::optional<Point> point(const QJsonValue& value)
{
    if (!value.isObject()) return {};
    const auto row=value.toObject(); Point p{};
    for (int i=0;i<3;++i) {
        const auto item=row.value(i==0?"x":i==1?"y":"z");
        if (!item.isDouble()||!std::isfinite(item.toDouble())) return {};
        p[i]=item.toDouble();
    }
    return p;
}
Point vector(const QJsonObject& row,const char* key,Point fallback={0,0,1})
{
    const auto p=point(row.value(key)); return p ? *p : fallback;
}
void line(MotionSymbol& symbol,const Point& a,const Point& b)
{
    if (finite(a)&&finite(b)) { symbol.linePoints.push_back(a); symbol.linePoints.push_back(b); }
}
std::pair<Point,Point> basis(const Point& direction)
{
    const auto n=unit(direction);
    const auto u=unit(cross(n,std::abs(n[0])<.8 ? Point{1,0,0}:Point{0,1,0}));
    return {u,cross(n,u)};
}
void ring(MotionSymbol& symbol,const Point& center,const Point& direction,double radius,double start=0,double end=2*pi)
{
    if (norm(direction)<1e-12) return;
    const auto [u,v]=basis(direction);
    for (int i=0;i<32;++i) {
        const double a=start+(end-start)*i/32,b=start+(end-start)*(i+1)/32;
        line(symbol,add(center,add(mul(u,radius*std::cos(a)),mul(v,radius*std::sin(a)))),
                    add(center,add(mul(u,radius*std::cos(b)),mul(v,radius*std::sin(b)))));
    }
}
void arrow(MotionSymbol& symbol,const Point& start,const Point& direction,double length)
{
    const auto n=unit(direction); if (norm(n)<.5) return;
    const auto tip=add(start,mul(n,length));const auto [u,v]=basis(n);
    line(symbol,start,tip);
    const auto back=sub(tip,mul(n,length*.22));
    for (const auto& side:{u,mul(u,-1),v,mul(v,-1)}) line(symbol,tip,add(back,mul(side,length*.10)));
}
void square(MotionSymbol& symbol,const Point& center,const Point& normal,double radius)
{
    if (norm(normal)<1e-12) return;
    const auto [u,v]=basis(normal);
    const std::array<Point,4> corners{{add(center,mul(add(u,v),radius)),
        add(center,mul(sub(u,v),radius)),sub(center,mul(add(u,v),radius)),sub(center,mul(sub(u,v),radius))}};
    for (int i=0;i<4;++i) line(symbol,corners[i],corners[(i+1)%4]);
}
void crosshair(MotionSymbol& symbol,const Point& center,double radius)
{
    for (int i=0;i<3;++i) {Point d{};d[i]=radius;line(symbol,sub(center,d),add(center,d));}
}
void cylinder(MotionSymbol& symbol,const Point& center,const Point& axis,double scale)
{
    const auto n=unit(axis);if(norm(n)<.5)return;
    const auto a=sub(center,mul(n,.8*scale)),b=add(center,mul(n,.8*scale));
    const auto [u,v]=basis(n);
    ring(symbol,a,n,.45*scale);ring(symbol,b,n,.45*scale);
    for (const auto& d:{u,mul(u,-1),v,mul(v,-1)}) line(symbol,add(a,mul(d,.45*scale)),add(b,mul(d,.45*scale)));
}
void coil(MotionSymbol& symbol,const Point& a,const Point& b,double radius)
{
    const auto delta=sub(b,a);const double length=norm(delta);
    if(length<1e-12){crosshair(symbol,a,radius);return;}
    const auto axis=unit(delta);const auto [u,v]=basis(axis);
    radius=std::min(radius,length*.12);
    Point previous=a;
    for(int i=0;i<=80;++i){const double t=.12+.76*i/80,angle=10*pi*i/80;
        const auto p=add(add(a,mul(delta,t)),add(mul(u,radius*std::cos(angle)),mul(v,radius*std::sin(angle))));
        line(symbol,previous,p);previous=p;}
    line(symbol,previous,b);
}
double modelScale(const CadDocument& cad)
{
    try {
        Bnd_Box bounds;
        for (const auto& part:cad.parts()) BRepBndLib::AddOptimal(part.shape,bounds,false,false);
        if(!bounds.IsVoid()&&!bounds.IsOpen()){
            double x0,y0,z0,x1,y1,z1;bounds.Get(x0,y0,z0,x1,y1,z1);
            const double size=std::hypot(x1-x0,std::hypot(y1-y0,z1-z0));
            if(std::isfinite(size)&&size>1e-9)return std::max(.1,size*.018);
        }
    }catch(const Standard_Failure&){}
    return 5; // Display size only; never used as geometry or physics data.
}
}

QVector<MotionSymbol> nativeMotionSymbols(const CadDocument& cad,const NativeMotionDocument& document,
    const std::map<int,QMatrix4x4>& partPoses,const QString& selectedCollection,int selectedId)
{
    QVector<MotionSymbol> output;
    const double scale=modelScale(cad);
    std::map<int,QMatrix4x4> bodyPoses;
    for(auto it=document.bodyPartBindings.begin();it!=document.bodyPartBindings.end();++it)
        for(int part:it.value()) {const auto pose=partPoses.find(part);if(pose!=partPoses.end()){bodyPoses.emplace(it.key(),pose->second);break;}}
    const auto transform=[&](int body,const Point& value,bool direction=false){
        const auto pose=bodyPoses.find(body);if(body<0||pose==bodyPoses.end())return value;
        const QVector3D p{float(value[0]),float(value[1]),float(value[2])};
        const auto result=direction?pose->second.mapVector(p):pose->second.map(p);
        return Point{double(result.x()),double(result.y()),double(result.z())};
    };
    const auto sensorPoint=[&](int body,const Point& local){
        // Sensor points are COM-local in JSON, unlike marker/joint positions.
        const auto global=body<0 ? local : add(local,vector(document.entity("bodies",body),"centerOfMass",{0,0,0}));
        return transform(body,global);
    };
    const auto make=[&](const QString& collection,const QJsonObject& row,const Point& origin,const QColor& color){
        MotionSymbol symbol;symbol.collection=collection;symbol.id=row.value("id").toInt(-1);
        symbol.label=row.value("name").toString();symbol.origin=origin;symbol.color=color;
        symbol.emphasized=selectedCollection==collection&&(selectedId<0||selectedId==symbol.id);return symbol;
    };
    const auto visible=[](const QJsonObject& row){return row.value("visible").toBool(true)&&row.value("enabled").toBool(true);};
    const auto append=[&](MotionSymbol&& symbol){if(finite(symbol.origin))output.append(std::move(symbol));};
    const auto jointFrame=[&](const QJsonObject& row,int body){
        return std::make_pair(transform(body,vector(row,"origin",{0,0,0})),
            unit(transform(body,vector(row,"axis",vector(row,"baseSpin")),true)));};

    for(const auto& value:document.collection("bodies")){
        const auto row=value.toObject();if(!visible(row))continue;
        const auto center=point(row.value("centerOfMass"));if(!center)continue;
        const int body=row.value("id").toInt(-1);const auto origin=transform(body,*center);
        auto symbol=make("bodies",row,origin,bodyColor);
        crosshair(symbol,origin,scale*.3);ring(symbol,origin,{0,0,1},scale*.2);append(std::move(symbol));
    }
    for(const auto& value:document.collection("joints")){
        const auto row=value.toObject();if(!visible(row))continue;
        const int action=row.value("actionBodyId").toInt(-1),base=row.value("baseBodyId").toInt(-1);
        const auto [origin,axis]=jointFrame(row,action);const auto type=row.value("jointType").toString();
        auto symbol=make("joints",row,origin,jointColor);
        if(type=="Universal"||type=="Perpendicular"||type=="ConstantVelocity"){
            const auto actionDirection=unit(transform(action,vector(row,type=="Universal"?"actionSpin":"actionVector"),true));
            arrow(symbol,origin,actionDirection,scale*1.6);
            if(type=="Universal")line(symbol,sub(origin,mul(unit(transform(action,vector(row,"actionPin",{1,0,0}),true)),scale)),add(origin,mul(unit(transform(action,vector(row,"actionPin",{1,0,0}),true)),scale)));
            append(std::move(symbol));
            const auto baseOrigin=jointFrame(row,base).first;
            auto other=make("joints",row,baseOrigin,baseColor);
            arrow(other,baseOrigin,transform(base,vector(row,type=="Universal"?"baseSpin":"baseVector"),true),scale*1.6);
            if(type=="Universal")line(other,sub(baseOrigin,mul(unit(transform(base,vector(row,"basePin",{1,0,0}),true)),scale)),add(baseOrigin,mul(unit(transform(base,vector(row,"basePin",{1,0,0}),true)),scale)));
            append(std::move(other));continue;
        }
        if(type=="Revolute"||type=="Cylindrical"){
            cylinder(symbol,origin,axis,scale);
            if(type=="Cylindrical")arrow(symbol,sub(origin,mul(axis,scale)),axis,scale*2.2);
        }else if(type=="Spherical"){
            const auto [u,v]=basis(norm(axis)>0?axis:Point{0,0,1});
            ring(symbol,origin,axis,scale*.6);ring(symbol,origin,u,scale*.6);ring(symbol,origin,v,scale*.6);
        }else if(type=="Fixed"){
            square(symbol,origin,axis,scale*.65);const auto [u,v]=basis(axis);
            for(int i=-2;i<=2;++i){const auto p=add(origin,add(mul(u,scale*i*.26),mul(v,-scale*.65)));line(symbol,p,add(p,mul(add(u,mul(v,-1)),scale*.25)));}
        }else if(type=="Screw"){
            coil(symbol,sub(origin,mul(axis,scale)),add(origin,mul(axis,scale)),scale*.5);
            arrow(symbol,sub(origin,mul(axis,scale*1.2)),axis,scale*2.4);
        }else if(type=="Translational"){
            square(symbol,origin,axis,scale*.55);arrow(symbol,sub(origin,mul(axis,scale)),axis,scale*2);
        }else if(type=="Planar"||type=="PointPlane"||type=="PointOnSurface"){
            square(symbol,origin,axis,scale*.8);arrow(symbol,origin,axis,scale);
            if(type!="Planar")crosshair(symbol,origin,scale*.2);
        }else if(type=="Parallel"){
            const auto u=basis(axis).first;arrow(symbol,sub(origin,mul(u,scale*.3)),axis,scale*1.4);arrow(symbol,add(origin,mul(u,scale*.3)),axis,scale*1.4);
        }else if(type=="PointOnCurve"||type=="CurveOnCurve"){
            ring(symbol,sub(origin,mul(basis(axis).first,scale)),axis,scale,-pi*.5,pi*.5);
            if(type=="CurveOnCurve")ring(symbol,add(origin,mul(basis(axis).first,scale)),axis,scale,pi*.5,pi*1.5);
            crosshair(symbol,origin,scale*.2);
        }else{arrow(symbol,sub(origin,mul(axis,scale*.6)),axis,scale*1.5);crosshair(symbol,origin,scale*.2);}
        append(std::move(symbol));
    }
    for(const auto& value:document.collection("couplings")){
        const auto row=value.toObject();if(!visible(row))continue;
        const auto a=document.entity("joints",row.value("firstJointId").toInt(-1)),b=document.entity("joints",row.value("secondJointId").toInt(-1));
        if(a.isEmpty()||b.isEmpty())continue;
        const auto [pa,na]=jointFrame(a,a.value("actionBodyId").toInt(-1));
        const auto [pb,nb]=jointFrame(b,b.value("actionBodyId").toInt(-1));
        auto symbol=make("couplings",row,mul(add(pa,pb),.5),springColor);line(symbol,pa,pb);
        ring(symbol,pa,na,scale*.85);
        if(row.value("couplingType").toString()=="GearRack")line(symbol,sub(pb,mul(nb,scale)),add(pb,mul(nb,scale)));
        else ring(symbol,pb,nb,scale*.85);
        append(std::move(symbol));
    }
    for(const auto& collection:{"springs","tensionCables","softStops"})for(const auto& value:document.collection(collection)){
        const auto row=value.toObject();if(!visible(row))continue;
        auto a=transform(row.value("bodyIId").toInt(-1),vector(row,"localI",{0,0,0}));
        auto b=transform(row.value("bodyJId").toInt(-1),vector(row,"localJ",{0,0,0}));
        const auto type=row.value("springType").toString();
        if(QString::fromLatin1(collection)=="springs"&&type!="BodyPoint"){
            const auto joint=document.entity("joints",row.value("jointId").toInt(-1));if(joint.isEmpty())continue;
            const auto [origin,axis]=jointFrame(joint,joint.value("actionBodyId").toInt(-1));
            if(type=="Torsional"){
                auto symbol=make(collection,row,origin,springColor);
                for(int i=0;i<3;++i)ring(symbol,origin,axis,scale*(.35+.18*i),0,pi*1.75);
                append(std::move(symbol));continue;
            }
            a=origin;const auto raw=vector(joint,"origin",{0,0,0});
            b=transform(joint.value("baseBodyId").toInt(-1),add(raw,mul(unit(vector(joint,"axis",vector(joint,"baseSpin"))),row.value("installValue").toDouble())));
        }
        auto symbol=make(collection,row,mul(add(a,b),.5),springColor);
        if(QString::fromLatin1(collection)=="springs")coil(symbol,a,b,scale*.5);
        else if(QString::fromLatin1(collection)=="softStops"){
            const int mode=row.value("directionMode").toInt(1);
            const auto direction=unit(transform(mode==2?row.value("bodyIId").toInt(-1):mode==3?row.value("bodyJId").toInt(-1):-1,vector(row,"direction"),true));
            const auto stop=add(b,mul(direction,row.value("stopPosition").toDouble()));
            line(symbol,a,stop);square(symbol,stop,direction,scale*.6);arrow(symbol,a,mul(direction,row.value("side").toInt(1)),scale);
        }else line(symbol,a,b);
        crosshair(symbol,a,scale*.12);crosshair(symbol,b,scale*.12);append(std::move(symbol));
    }
    for(const auto& collection:{"vectorForces","scalarForces","scalarTorques","threePointForces","threePointTorques"})for(const auto& value:document.collection(collection)){
        const auto row=value.toObject();if(!visible(row))continue;const QString name=QString::fromLatin1(collection);
        Point origin{},direction{};
        const bool vectorForce=name=="vectorForces";
        if(vectorForce){
            const int body=row.value("bodyId").toInt(-1);origin=transform(body,vector(row,"actionPoint",{0,0,0}));
            const bool following=row.value("directionMode").toString()=="BodyFixed";
            direction=transform(following?body:-1,vector(row,"direction"),true);
        }else{
            const int body=row.value("bodyIId").toInt(-1);origin=transform(body,vector(row,"localI",{0,0,0}));
            if(name.startsWith("threePoint"))direction=sub(transform(row.value("targetBodyId").toInt(-1),vector(row,"localTarget",{0,0,0})),transform(row.value("originBodyId").toInt(-1),vector(row,"localOrigin",{0,0,0})));
            else{
                const bool torque=name=="scalarTorques";const int mode=row.value(torque?"axisMode":"directionMode").toInt(torque?1:0);
                direction=mode==0?sub(transform(row.value("bodyJId").toInt(-1),vector(row,"localJ",{0,0,0})),origin)
                    :transform(mode==2?body:mode==3?row.value("bodyJId").toInt(-1):-1,vector(row,torque?"fixedAxis":"fixedDirection"),true);
            }
        }
        if(row.value("magnitude").toDouble(1)<0)direction=mul(direction,-1);
        auto symbol=make(collection,row,origin,forceColor);
        if(vectorForce&&row.value("loadType").toString()=="Bushing"){
            cylinder(symbol,origin,direction,scale*.7);crosshair(symbol,origin,scale);
        }else{
            arrow(symbol,origin,direction,scale*2);
            if(name.endsWith("Torques")||(vectorForce&&row.value("loadType").toString()=="Torque"))ring(symbol,origin,direction,scale*.7,0,pi*1.75);
        }
        append(std::move(symbol));
    }
    for(const auto& value:document.collection("markers")){
        const auto row=value.toObject();if(!visible(row))continue;
        const int body=row.value("bodyId").toInt(-1);const auto position=point(row.value("localPosition"));if(!position)continue;
        const auto origin=transform(body,*position);const auto q=row.value("localRotation").toObject();
        QQuaternion orientation(float(q.value("w").toDouble(1)),float(q.value("x").toDouble()),float(q.value("y").toDouble()),float(q.value("z").toDouble()));
        if(orientation.lengthSquared()<1e-12)continue;orientation.normalize();
        const double requested=row.value("size").toDouble(scale);const double size=std::isfinite(requested)&&requested>0?requested:scale;
        const std::array<QColor,3> colors{{QColor(205,65,55),QColor(36,146,95),QColor(38,125,177)}};
        for(int i=0;i<3;++i){QVector3D axis;axis[i]=1;const auto rotated=orientation.rotatedVector(axis);
            auto symbol=make("markers",row,origin,colors[i]);
            arrow(symbol,origin,transform(body,{double(rotated.x()),double(rotated.y()),double(rotated.z())},true),size);append(std::move(symbol));}
    }
    for(const auto& value:document.collection("sensors")){
        const auto row=value.toObject();if(!visible(row))continue;
        if(row.value("sensorType").toString()=="Distance"){
            const auto a=row.value("pointA").toObject(),b=row.value("pointB").toObject();
            const auto pa=sensorPoint(a.value("bodyId").toInt(-1),vector(a,"localPos",{0,0,0}));
            const auto pb=sensorPoint(b.value("bodyId").toInt(-1),vector(b,"localPos",{0,0,0}));
            auto symbol=make("sensors",row,mul(add(pa,pb),.5),sensorColor);line(symbol,pa,pb);crosshair(symbol,pa,scale*.2);crosshair(symbol,pb,scale*.2);append(std::move(symbol));
        }else if(row.value("sensorType").toString().startsWith("Body")){
            const auto p=point(row.value("referencePoint"));if(!p)continue;
            const auto origin=sensorPoint(row.value("targetBodyId").toInt(-1),*p);
            auto symbol=make("sensors",row,origin,sensorColor);crosshair(symbol,origin,scale*.4);ring(symbol,origin,{0,0,1},scale*.4);append(std::move(symbol));
        }
    }
    return output;
}

} // namespace MotionOcc
