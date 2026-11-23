#include "MotionWindow.h"
#include "compat/model/LegacyMotionContext.h"
#include "mMotionResultData.h"
#include "NativeMotionSolver.h"
#include <QQuaternion>
#include <QSet>
#include <cmath>
#include <limits>

using namespace MechMotionData;
using namespace MxFunctions;
namespace MotionOcc {
namespace {
constexpr double degreesToRadians=3.14159265358979323846/180.;
template<class T> QVector<T> qvalues(const std::vector<T>& values) { return QVector<T>(values.begin(),values.end()); }
QVector3D vector(const Vector3d& value) { return QVector3D(value[0],value[1],value[2]); }
using ChannelKey=std::tuple<QString,int,QString>;
class ResultReader {
public:
    explicit ResultReader(const RigidResult& result):frameCount_(result.frames.size()) {
        for(const auto& c:result.channels) {
            const ChannelKey key{c.sourceType,c.sourceId,c.variable};
            const auto inserted=channels_.emplace(key,&c);
            // A duplicated channel is ambiguous, not a reason to use whichever
            // copy happened to be serialized last.
            if(!inserted.second)inserted.first->second=nullptr;
        }
    }
    const ResultChannel* channel(const QString& source,int id,const QString& variable) const {
        const auto it=channels_.find({source,id,variable});return it==channels_.end() ? nullptr : it->second;
    }
    double scalar(const QString& source,int id,const QString& name,size_t frame,int component=0,
                  int components=1,const QString& unit={}) const {
        const auto* c=channel(source,id,name);
        if(!c || component<0 || c->components!=components || component>=components
            || frame>=frameCount_ || c->values.size()!=frameCount_*size_t(components)
            || (!unit.isEmpty() && c->unit!=unit))return unavailable(source,id,name);
        const double value=c->values[frame*size_t(components)+component];
        return std::isfinite(value)?value:unavailable(source,id,name);
    }
    QVector3D vector3(const QString& source,int id,const QString& name,size_t frame,const QString& unit={}) const {
        return QVector3D(scalar(source,id,name,frame,0,3,unit),scalar(source,id,name,frame,1,3,unit),scalar(source,id,name,frame,2,3,unit));
    }
    mMotionMarkerFrame marker(const QString& source,int id,size_t frame,const QString& suffix={}) const {
        mMotionMarkerFrame value;value.m_markerId=id;
        value.m_worldPosition=vector3(source,id,"Position"+suffix,frame,"mm");
        value.m_worldRotation=QQuaternion::fromEulerAngles(vector3(source,id,"Rotation"+suffix,frame,QStringLiteral("°")));
        value.m_linearVelocity=vector3(source,id,"Velocity"+suffix,frame,"mm/s");
        value.m_linearAcceleration=vector3(source,id,"Acceleration"+suffix,frame,QStringLiteral("mm/s²"));
        // Original query service converts these radians to degrees itself.
        value.m_angularVelocity=vector3(source,id,"AngularVelocity"+suffix,frame,QStringLiteral("°/s"))*degreesToRadians;
        value.m_angularAcceleration=vector3(source,id,"AngularAcceleration"+suffix,frame,QStringLiteral("°/s²"))*degreesToRadians;
        return value;
    }
    std::set<int> ids(const QString& source) const { std::set<int> out;for(const auto& [key,c]:channels_)if(std::get<0>(key)==source)out.insert(std::get<1>(key));return out; }
    QStringList unavailableChannels() const { QStringList values=unavailable_.values();values.sort();return values; }
private:
    double unavailable(const QString& source,int id,const QString& name) const {
        unavailable_.insert(QStringLiteral("%1/%2/%3").arg(source).arg(id).arg(name));
        return std::numeric_limits<double>::quiet_NaN();
    }
    size_t frameCount_;
    std::map<ChannelKey,const ResultChannel*> channels_;
    mutable QSet<QString> unavailable_;
};
}
void MotionWindow::syncOriginalMotionResults(int scenarioId) {
    if(!m_original)return;
    const auto found=m_results.find(scenarioId);if(found==m_results.end())return;
    auto* scenario=m_original->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(scenarioId);
    if(!scenario)return;
    const auto& run=*found->second;const auto& result=run.result;
    auto typed=std::make_unique<mMotionResultData>();ResultReader reader(result);
    const auto& preparedBodies=run.input.nativeInput?run.input.nativeInput->bodies:run.input.bodies;
    std::map<int,QVector3D> initialCenters;
    for(const auto& prepared:preparedBodies)initialCenters.emplace(prepared.body.id,vector(prepared.properties.centerMm));
    const auto markerIds=reader.ids("markers"),jointIds=reader.ids("joints"),couplingIds=reader.ids("couplings"),driveIds=reader.ids("drives");
    typed->m_frames.reserve(result.frames.size());
    for(size_t index=0;index<result.frames.size();++index) {
        const auto& original=result.frames[index];mMotionFrame frame;frame.m_time=original.seconds;
        for(const auto& body:original.bodies) {
            const int id=body.bodyId;
            const auto center=initialCenters.find(id);
            const float missing=std::numeric_limits<float>::quiet_NaN();
            const QVector3D initial=center!=initialCenters.end()?center->second:QVector3D(missing,missing,missing);
            const auto q=body.rotation;const QQuaternion rotation(q[0],q[1],q[2],q[3]);
            QMatrix4x4 transform;transform.translate(vector(body.positionMm));transform.rotate(rotation);transform.translate(-initial);
            if(center!=initialCenters.end())frame.m_bodyTransforms[id]=transform;
            mMotionBodyFrame b;b.m_position=vector(body.positionMm);b.m_rotation=rotation;b.m_velocity=vector(body.velocityMmS);
            b.m_acceleration=reader.vector3("bodies",id,"Acceleration",index,QStringLiteral("mm/s²"));
            // The solver's immutable prepared centroid is authoritative. The
            // editable JSON may still contain a pre-recalculation mass centre.
            b.m_displacement=b.m_position-initial;
            b.m_angularVelocity=vector(body.angularVelocityDegS)*degreesToRadians;
            b.m_angularAcceleration=reader.vector3("bodies",id,"AngularAcceleration",index,QStringLiteral("°/s²"))*degreesToRadians;
            b.m_potentialEnergy=reader.scalar("bodies",id,"PotentialEnergy",index,0,1,"J");
            b.m_kineticEnergy=reader.scalar("bodies",id,"KineticEnergy",index,0,1,"J");
            frame.m_bodyFrames[id]=b;
        }
        for(int id:markerIds)frame.m_markerFrames[id]=reader.marker("markers",id,index);
        const auto joints=[&](const QString& collection,const std::set<int>& ids,std::map<int,mMotionJointFrame>& output) {
            for(int id:ids) {
                mMotionJointFrame j;j.m_nodeI=reader.marker(collection,id,index);j.m_nodeJ=reader.marker(collection,id,index,"J");
                j.m_reactionForce=reader.vector3(collection,id,"ReactionForce",index,"N");j.m_reactionTorque=reader.vector3(collection,id,"ReactionTorque",index,QStringLiteral("N·mm"));output[id]=j;
            }
        };
        joints("joints",jointIds,frame.m_jointFrames);joints("couplings",couplingIds,frame.m_couplingFrames);
        for(int id:driveIds) {
            mMotionDriveFrame d;d.m_absCoord=reader.marker("drives",id,index);
            const auto& document=run.input.nativeInput?run.input.nativeInput->document:m_original->document();
            d.m_isRotation=document.entity("drives",id).value("motionType").toString()=="Rotation";
            d.m_imposedValue=reader.scalar("drives",id,reader.channel("drives",id,"ImposedValue") ? "ImposedValue" : "Coordinate",index,0,1,d.m_isRotation?QStringLiteral("°"):QStringLiteral("mm"));
            d.m_imposedSpeed=reader.scalar("drives",id,reader.channel("drives",id,"ImposedSpeed") ? "ImposedSpeed" : "Rate",index,0,1,d.m_isRotation?QStringLiteral("°/s"):QStringLiteral("mm/s"));
            d.m_imposedAcc=reader.scalar("drives",id,reader.channel("drives",id,"ImposedAcc") ? "ImposedAcc" : "CoordinateAcceleration",index,0,1,d.m_isRotation?QStringLiteral("°/s²"):QStringLiteral("mm/s²"));
            d.m_actuationForce=reader.vector3("drives",id,"ActuationForce",index,"N");d.m_actuationTorque=reader.vector3("drives",id,"ActuationTorque",index,QStringLiteral("N·mm"));
            d.m_reactionForce=reader.vector3("drives",id,"ReactionForce",index,"N");d.m_reactionTorque=reader.vector3("drives",id,"ReactionTorque",index,QStringLiteral("N·mm"));frame.m_driveFrames[id]=d;
        }
        typed->m_frames.push_back(std::move(frame));
    }
    for(int id:reader.ids("controllers"))for(size_t i=0;i<result.frames.size();++i) {
        mMotionPidSample p;p.m_time=result.frames[i].seconds;p.m_target=reader.scalar("controllers",id,"PidTarget",i);
        p.m_feedback=reader.scalar("controllers",id,"PidFeedback",i);p.m_error=reader.scalar("controllers",id,"PidError",i);p.m_output=reader.scalar("controllers",id,"PidOutput",i);
        typed->m_pidHistories[id].push_back(p);
    }
    for(const auto& source:result.flexBodies) {
        mMotionFlexBodyData f;f.m_bodyId=source.bodyId;f.m_name=m_original->document().entity("autoFlexBodies",source.flexId).value("name").toString();
        f.m_nodeX0=qvalues(source.referenceNodes);f.m_tetIds=qvalues(source.tetrahedra);f.m_surfaceTris=qvalues(source.surfaceTriangles);
        f.m_psi=qvalues(source.psiInternal);f.m_nModes=source.modeCount;f.m_nInterfaces=source.interfaceCount;f.m_interfacePose0=qvalues(source.initialInterfacePoses);
        // Both real capture paths use [x,y,z,w,qx,qy,qz]. Preserve that order;
        // the old result-header prose incorrectly described xyzw.
        for(size_t i=0;i<source.modalCoordinates.size() && i<result.frames.size();++i) {
            mMotionFlexFrame frame;frame.m_time=result.frames[i].seconds;frame.m_modalQ=qvalues(source.modalCoordinates[i]);
            if(i<source.interfacePoses.size())frame.m_interfacePose=qvalues(source.interfacePoses[i]);f.m_frames.append(frame);
        }
        typed->m_flexBodies.append(f);
    }
    scenario->attachResult(typed.release());
    const auto unavailable=reader.unavailableChannels();
    if(!unavailable.isEmpty())setNotice(tr("部分结果通道缺失、单位或采样不匹配，相关曲线不可用：%1").arg(unavailable.join(QStringLiteral(", "))),true);
}
}
