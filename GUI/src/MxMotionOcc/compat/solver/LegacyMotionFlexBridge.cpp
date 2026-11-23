#include "mMotionFlexBridge.h"
#include "mViewportModelManager.h"
#include "mMotionJointBasic.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointCurveOnCurve.h"
#include "mMotionJointUniversal.h"
#include "mMotionJointScrew.h"
#include "mMotionJointPerpendicular.h"
#include "mMotionJointConstantVelocity.h"
#include <QPointer>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

using namespace MxFunctions;
using namespace MechMotionData;
namespace mMotionFlexBridge {
namespace {
QPointer<MechPreData::mViewportModel> previewViewport;
void report(const std::function<void(const QString&)>& log,const QString& text) { if(log)log(text); }
bool collectMesh(MechModelData::mModelData* model,const std::set<int>& parts,
    std::vector<double>& coords,std::vector<int>& tets,std::map<int,int>& globalToLocal,
    const std::function<void(const QString&)>& log) {
    coords.clear();tets.clear();globalToLocal.clear();
    auto* mesh=model?model->getMeshData():nullptr;
    if(!mesh || parts.empty()) { report(log,QStringLiteral("自动柔性体未关联几何部件。"));return false; }
    mesh->rebuildNodeIndex();
    for(int part:parts) {
        const auto found=mesh->meshes().find(part);const auto ids=mesh->nodeIds().find(part);
        if(found==mesh->meshes().end() || found->second.tetrahedra.empty() || ids==mesh->nodeIds().end()
           || std::size_t(ids->second.size())!=found->second.nodes.size())return false;
        const auto& source=found->second;const int offset=int(coords.size()/3);
        for(std::size_t node=0;node<source.nodes.size();++node) {
            const int global=ids->second[int(node)];
            if(global<=0 || globalToLocal.count(global)) { report(log,QStringLiteral("体网格节点编号不唯一。"));return false; }
            for(double coordinate:source.nodes[node]) {
                if(!std::isfinite(coordinate)) { report(log,QStringLiteral("体网格含无效节点坐标。"));return false; }
                coords.push_back(coordinate);
            }
            globalToLocal.emplace(global,offset+int(node));
        }
        for(const auto& tet:source.tetrahedra)for(auto index:tet) {
            if(index>=source.nodes.size()) { report(log,QStringLiteral("体网格连接引用了不存在的节点。"));return false; }
            tets.push_back(offset+int(index));
        }
    }
    return !tets.empty();
}
void connectedGroups(const std::vector<int>& tets,std::set<int> selected,std::vector<std::vector<int>>& groups) {
    std::map<int,std::set<int>> adjacent;
    for(std::size_t offset=0;offset+3<tets.size();offset+=4)
        for(int a=0;a<4;++a)for(int b=a+1;b<4;++b)
            if(selected.count(tets[offset+a])&&selected.count(tets[offset+b])) {
                adjacent[tets[offset+a]].insert(tets[offset+b]);adjacent[tets[offset+b]].insert(tets[offset+a]);
            }
    while(!selected.empty()) {
        std::set<int> pending{*selected.begin()};selected.erase(selected.begin());std::vector<int> group;
        while(!pending.empty()) {
            const int node=*pending.begin();pending.erase(pending.begin());group.push_back(node);
            for(int neighbor:adjacent[node])if(selected.erase(neighbor))pending.insert(neighbor);
        }
        std::sort(group.begin(),group.end());groups.push_back(std::move(group));
    }
}
double squaredDistance(const std::vector<double>& coords,int a,int b) {
    double result=0;for(int c=0;c<3;++c) { const double d=coords[3*a+c]-coords[3*b+c];result+=d*d; }return result;
}
}

bool ensureTetraMesh(MechModelData::mModelData* model,const std::set<int>& parts,double size,
    const std::function<void(const QString&)>& log) {
    if(!model || !model->getMeshData() || parts.empty()) { report(log,QStringLiteral("自动柔性体未关联有效几何。"));return false; }
    std::vector<double> coords;std::vector<int> tets;std::map<int,int> ids;
    if(collectMesh(model,parts,coords,tets,ids,{}))return true;
    auto* viewport=getCVM;
    if(!viewport || viewport->getModelData()!=model || !viewport->ensureTetraMesh) {
        report(log,QStringLiteral("当前模型未连接真实四面体网格生成接口。"));return false;
    }
    QString error;
    if(!viewport->ensureTetraMesh(parts,size,error)) { report(log,error);return false; }
    if(!collectMesh(model,parts,coords,tets,ids,log)) {
        report(log,QStringLiteral("网格任务未生成目标部件的有效四面体单元。"));return false;
    }
    report(log,QStringLiteral("自动柔性体网格完成：%1 个节点、%2 个四面体。%3")
        .arg(coords.size()/3).arg(tets.size()/4).arg(error));return true;
}
bool collectBodyTetraMesh(MechModelData::mModelData* model,const std::set<int>& parts,double size,
    std::vector<double>& coords,std::vector<int>& tets,std::map<int,int>& ids,
    const std::function<void(const QString&)>& log) {
    if(collectMesh(model,parts,coords,tets,ids,{}))return true;
    return ensureTetraMesh(model,parts,size,log)&&collectMesh(model,parts,coords,tets,ids,log);
}
bool previewFlexBodyMesh(MechModelData::mModelData* model,int bodyId,const QString&) {
    auto* viewport=getCVM;
    if(!viewport || viewport->getModelData()!=model || !viewport->previewBodyMesh)return false;
    const auto parts=model->getMapRelationManager().getMapRelation<std::set<int>>({Physical_MotionBody,bodyId},Physical_Part,MapRelation_None);
    std::vector<double> coords;std::vector<int> tets;std::map<int,int> ids;
    if(!collectMesh(model,parts,coords,tets,ids,{})) { viewport->previewBodyMesh(bodyId,false);return false; }
    previewViewport=viewport;viewport->previewBodyMesh(bodyId,true);return true;
}
void clearFlexBodyPreview() {
    if(previewViewport && previewViewport->previewBodyMesh)previewViewport->previewBodyMesh(-1,false);
    previewViewport.clear();
}
std::vector<double> collectJointAttachPointsGui(MechModelData::mModelData* model,int bodyId) {
    std::vector<double> result;if(!model || bodyId<0)return result;
    auto& manager=model->getManager<mMotionJointManager>(Physical_MotionJoint);auto& relations=model->getMapRelationManager();
    const auto origin=[](mMotionJointBase* joint,QVector3D& point) {
#define MX_JOINT_ORIGIN(Type) if(auto* value=dynamic_cast<Type*>(joint)) { point=value->getOrigin();return true; }
        MX_JOINT_ORIGIN(mMotionJointBasic) MX_JOINT_ORIGIN(mMotionJointPointOnSurface)
        MX_JOINT_ORIGIN(mMotionJointPointOnCurve) MX_JOINT_ORIGIN(mMotionJointCurveOnCurve)
        MX_JOINT_ORIGIN(mMotionJointUniversal) MX_JOINT_ORIGIN(mMotionJointScrew)
        MX_JOINT_ORIGIN(mMotionJointPerpendicular) MX_JOINT_ORIGIN(mMotionJointConstantVelocity)
#undef MX_JOINT_ORIGIN
        return false;
    };
    for(int id:manager.getAllIDs()) {
        const int action=relations.getMapRelation<int>({Physical_MotionJoint,id},Physical_MotionBody,MapRelation_MotionActionBody);
        const int base=relations.getMapRelation<int>({Physical_MotionJoint,id},Physical_MotionBody,MapRelation_MotionBaseBody);
        if(action!=bodyId && base!=bodyId)continue;
        QVector3D point;if(!origin(manager.getData(id),point))continue;bool duplicate=false;
        for(std::size_t i=0;i+2<result.size();i+=3)
            if(std::abs(result[i]-point.x())<1e-6 && std::abs(result[i+1]-point.y())<1e-6 && std::abs(result[i+2]-point.z())<1e-6) { duplicate=true;break; }
        if(!duplicate)result.insert(result.end(),{point.x(),point.y(),point.z()});
    }
    return result;
}
std::vector<FlexInput> collectFlexInputs(motion::MotionSystem*,MechModelData::mModelData* model,
    const std::function<void(const QString&)>& log) {
    std::vector<FlexInput> result;if(!model)return result;
    auto& manager=model->getManager<mMotionAutoFlexManager>(Physical_MotionAutoFlex);
    for(int id:manager.getAllIDs()) {
        const auto* flex=manager.getData(id);if(!flex || !flex->isEnabled())continue;
        FlexInput input;input.flexId=id;input.name=flex->getName();input.modeCount=flex->getModeCount();
        input.connectionRadius=flex->getConnectionRadius();input.dampingType=int(flex->getDampingType());
        input.modalDampingRatio=flex->getModalDampingRatio();input.rayleighAlpha=flex->getRayleighMass();
        input.rayleighBeta=flex->getRayleighStiffness();input.freqCutoff=flex->getFreqCutoff();
        input.bodyId=model->getMapRelationManager().getMapRelation<int>({Physical_MotionAutoFlex,id},Physical_MotionBody,MapRelation_None);
        auto* body=model->getManager<mMotionBodyManager>(Physical_MotionBody).getData(input.bodyId);
        auto reject=[&](const QString& error) { input.error=error;report(log,error);result.push_back(input); };
        if(!body) { reject(QStringLiteral("自动柔性体 %1 未关联有效运动体。").arg(id));continue; }
        input.bodyFixed=body->getIsFixed();
        auto material=model->getManager<MechMaterialData::mMaterialManager>(Physical_Material).getMaterialData(flex->getMaterialId());
        if(!material) { reject(QStringLiteral("自动柔性体 %1 未设置有效材料。").arg(id));continue; }
        const auto props=material->getAllProps();
        auto property=[&](MaterialPropType key) {
            QVariant value=props.value(MAT_Isotropy).value(key);
            if(!value.isValid())for(const auto& category:props)if(category.contains(key)) { value=category.value(key);break; }
            bool ok=false;double number=value.toDouble(&ok);return ok?number:std::numeric_limits<double>::quiet_NaN();
        };
        input.E=property(MatProp_YoungsModulus);input.nu=property(MatProp_Botson);input.rho=property(MatProp_Density);
        if(!std::isfinite(input.E)||!std::isfinite(input.nu)||!std::isfinite(input.rho)||input.E<=0||input.rho<=0||input.nu<=-1||input.nu>=.5) {
            reject(QStringLiteral("自动柔性体 %1 的弹性模量、泊松比或密度无效。").arg(id));continue;
        }
        // This legacy input is used by the original .mx exporter (mm, tonne,
        // N, MPa). Native solving separately keeps kg/mm³ and converts E.
        input.rho/=1000.0;
        const auto parts=model->getMapRelationManager().getMapRelation<std::set<int>>({Physical_MotionBody,input.bodyId},Physical_Part,MapRelation_None);
        std::map<int,int> ids;
        if(!collectBodyTetraMesh(model,parts,flex->getElementSize(),input.nodeCoords,input.tetIds,ids,log)) {
            reject(QStringLiteral("自动柔性体 %1 的真实四面体网格不可用。").arg(id));continue;
        }
        input.nodeGlobalIds.resize(input.nodeCoords.size()/3);
        for(const auto& entry:ids)input.nodeGlobalIds[entry.second]=entry.first;
        std::set<int> selected;bool stale=false;
        for(int global:flex->getAttachPoints()) {
            auto found=ids.find(global);if(found==ids.end()) { stale=true;break; }selected.insert(found->second);
        }
        if(stale) { reject(QStringLiteral("自动柔性体 %1 的附着节点不在当前目标网格中，请重新拾取。").arg(id));continue; }
        if(input.connectionRadius>0 && !selected.empty()) {
            auto expanded=selected;const double radius2=input.connectionRadius*input.connectionRadius;
            for(int node:selected)for(int other=0;other<int(input.nodeGlobalIds.size());++other)
                if(squaredDistance(input.nodeCoords,node,other)<=radius2)expanded.insert(other);
            selected=std::move(expanded);
        }
        connectedGroups(input.tetIds,selected,input.attachGroups);
        if(input.attachGroups.empty())input.autoAttachPositions=collectJointAttachPointsGui(model,input.bodyId);
        input.valid=true;result.push_back(std::move(input));
    }
    return result;
}
} // namespace mMotionFlexBridge
