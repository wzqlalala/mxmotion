#include "LegacyRender.h"
#include "mGeneralPickUp.h"
#include "picking/OccPickController.h"
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <QLineF>
#include <cmath>

using MotionOcc::PickKind;
using MotionOcc::PickResult;
using MxFunctions::PickDataType;
namespace {
QColor color255(const QVector3D& v) { return QColor(int(v.x()),int(v.y()),int(v.z())); }
void originalSymbolIdentity(const QString& key,MotionOcc::MotionSymbol& symbol) {
    static const QMap<QString,QString> collections{{"motionJoint","joints"},{"motionCoupling","couplings"},
        {"motionSpring","springs"},{"vectorForce","vectorForces"},{"scalarForce","scalarForces"},
        {"scalarTorque","scalarTorques"},{"threePointForce","threePointForces"},{"threePointTorque","threePointTorques"}};
    const int separator=key.lastIndexOf('_');
    if(separator<0)return;
    bool valid=false;const int id=key.mid(separator+1).toInt(&valid);
    const auto collection=collections.value(key.left(separator));
    if(valid&&id>=0&&!collection.isEmpty()) { symbol.collection=collection;symbol.id=id; }
}
PickDataType pickType(mxr::SelectionInfoType type) {
    using T=mxr::SelectionInfoType;
    switch(type) {
    case T::GeoPoint:return PickDataType::GeoPoint;
    case T::GeoEdge:return PickDataType::GeoEdge;
    case T::GeoFace:return PickDataType::GeoFace;
    case T::GeoSolid:return PickDataType::GeoSolid;
    case T::GeoPart:case T::MeshPart:return PickDataType::Part;
    case T::MeshNode:return PickDataType::Node;
    case T::MeshEdge:return PickDataType::ElementEdge;
    case T::MeshFace:return PickDataType::ElementFace;
    case T::MeshElement:return PickDataType::Element;
    }
    return PickDataType::None;
}
}
namespace mxr {
void mSymbolController::UpdateAllSymbol() {
    auto* viewport = context_ ? context_->viewport() : nullptr;
    if(!viewport)return;
    QVector<MotionOcc::MotionSymbol> output;
    for(const auto& data:symbols_) {
        if(!data || !data->_showState || data->_vexter.isEmpty())continue;
        MotionOcc::MotionSymbol symbol;symbol.collection=QStringLiteral("originalGui");
        originalSymbolIdentity(data->key,symbol);
        symbol.emphasized=selected_.contains(data->key);
        symbol.label=data->_txt_pos.isEmpty() ? QString{} : data->_txt_pos.first().first;
        const auto origin=data->_txt_pos.isEmpty() ? data->_vexter.first() : data->_txt_pos.first().second;
        symbol.origin={origin.x(),origin.y(),origin.z()};symbol.color=color255(data->_lineColor);
        symbol.faceColor=color255(data->_faceColor);symbol.textColor=color255(data->_textColor);
        symbol.color.setAlphaF(std::clamp(double(data->_opacity),0.,1.));
        symbol.faceColor.setAlphaF(std::clamp(double(data->_opacity),0.,1.));
        symbol.textColor.setAlphaF(std::clamp(double(data->_opacity),0.,1.));
        symbol.drawOrigin=false;symbol.retainOpacity=true;
        symbol.lineDepthTest=data->_lineDepthState;symbol.faceDepthTest=data->_faceDepthState;
        for(const auto& label:data->_txt_pos) {
            const auto p=label.second;symbol.texts.push_back({label.first,{p.x(),p.y(),p.z()}});
        }
        for(size_t i=0;i+1<data->_lineIndex.size();i+=2) {
            const auto a=data->_lineIndex[i],b=data->_lineIndex[i+1];
            if(a>=unsigned(data->_vexter.size()) || b>=unsigned(data->_vexter.size()))continue;
            for(auto j:{a,b}) { const auto p=data->_vexter[int(j)];symbol.linePoints.push_back({p.x(),p.y(),p.z()}); }
        }
        for(size_t i=0;i+2<data->_faceIndex.size();i+=3) {
            const auto a=data->_faceIndex[i],b=data->_faceIndex[i+1],c=data->_faceIndex[i+2];
            if(a>=unsigned(data->_vexter.size()) || b>=unsigned(data->_vexter.size()) || c>=unsigned(data->_vexter.size()))continue;
            for(auto j:{a,b,c}) { const auto p=data->_vexter[int(j)];symbol.trianglePoints.push_back({p.x(),p.y(),p.z()}); }
        }
        output.append(symbol);
    }
    viewport->setAuxiliaryMotionSymbols(QStringLiteral("original:%1").arg(quintptr(this)),output);
}
LegacyHighLightRender::~LegacyHighLightRender() { if(context_)context_->clearHighlightLayer("original:"+key_); }
void LegacyHighLightRender::updateHighLightRender(const mGeoSelectData* value,HighStyle) {
    geometry_=value ? *value : mGeoSelectData{};mesh_.values.clear();publish();
}
void LegacyHighLightRender::updateHighLightRender(const MeshSelectMemoryData* value) {
    mesh_=value ? *value : MeshSelectMemoryData{};geometry_.values.clear();publish();
}
void LegacyHighLightRender::publish() {
    if(!context_)return;
    QVector<PickResult> picks;
    for(const auto& [type,ids]:geometry_.values) for(int id:ids) {
        if (type == SelectionInfoType::GeoPart) {
            if (context_->document().part(id)) {
                PickResult p; p.kind = PickKind::Part; p.partId = id;
                picks.append(p);
            }
            continue;
        }
        auto p=context_->resolve(pickType(type),RenderPickResultData{id,-1,-1});
        if(p.isValid())picks.append(p);
    }
    for(const auto& [type,ids]:mesh_.values)for(const auto& id:ids) {
        if (type == SelectionInfoType::MeshPart) {
            if (context_->document().part(id.part_id)) {
                PickResult p; p.kind = PickKind::Part; p.partId = id.part_id;
                picks.append(p);
            }
            continue;
        }
        const int value=type==SelectionInfoType::MeshNode ? id.data_id.node_id : id.data_id.element_id;
        auto p=context_->resolve(pickType(type),RenderPickResultData{value,-1,id.part_id});
        if(p.isValid())picks.append(p);
    }
    context_->setHighlightLayer("original:"+key_,picks,color_);
}
void LegacyCommonShapeRender::appendPoint(const QString& key,const QVector<QVector3D>& points,const QVector3D& color,float pointSize) {
    if(context_)context_->setPreviewPoints("original:"+key,points,QColor::fromRgbF(std::clamp(double(color.x()),0.,1.),std::clamp(double(color.y()),0.,1.),std::clamp(double(color.z()),0.,1.)),pointSize);
}
void LegacyCommonShapeRender::deletePoint(const QString& key) { if(context_)context_->clearPreviewPoints("original:"+key); }
void LegacyCommonShapeRender::appendCylinder(const QString& key,const QVector3D& center,const QVector3D& halfAxis,float radius,const QVector3D& color) {
    if(!symbols_ || halfAxis.lengthSquared()<1e-20f || radius<=0)return;
    auto data=symbols_->getOrCreateSymbolData<UniversalSymbolFont>("shape:"+key);
    data->_vexter.clear();data->_lineIndex.clear();data->_faceIndex.clear();
    data->_lineColor=data->_faceColor=color*255.f;data->_opacity=1.f;
    const auto axis=halfAxis.normalized();
    auto u=QVector3D::crossProduct(axis,std::abs(axis.z())<0.9f ? QVector3D(0,0,1):QVector3D(0,1,0)).normalized();
    auto v=QVector3D::crossProduct(axis,u);constexpr int segments=24;
    for(int end=0;end<2;++end)for(int i=0;i<segments;++i) {
        const float angle=float(i*2.*3.14159265358979323846/segments);
        data->_vexter.append(center+(end ? halfAxis : -halfAxis)+radius*(std::cos(angle)*u+std::sin(angle)*v));
        data->_lineIndex.push_back(end*segments+i);data->_lineIndex.push_back(end*segments+(i+1)%segments);
    }
    for(int i=0;i<segments;i+=6) { data->_lineIndex.push_back(i);data->_lineIndex.push_back(i+segments); }
    data->_vexter.append(center-halfAxis);data->_vexter.append(center+halfAxis);
    for(unsigned int i=0;i<segments;++i) {
        const auto j=(i+1)%segments;
        const unsigned int faces[]={i,j,i+segments,j,j+segments,i+segments,
                                    2*segments,j,i,2*segments+1,i+segments,j+segments};
        data->_faceIndex.insert(data->_faceIndex.end(),std::begin(faces),std::end(faces));
    }
    symbols_->UpdateAllSymbol();
}
void LegacyCommonShapeRender::deleteCylinder(const QString& key) { if(symbols_) { symbols_->deleteSymbol({"shape:"+key});symbols_->UpdateAllSymbol(); } }
void LegacyCommonShapeRender::appendCylinder(const QString& key,const QVector3D& center,const QVector3D& halfAxis,float radius,const QVector4D& color) {
    appendCylinder(key,center,halfAxis,radius,color.toVector3D());
    if(symbols_&&symbols_->isExistSymbol("shape:"+key)) {
        symbols_->getOrCreateSymbolData<UniversalSymbolFont>("shape:"+key)->_opacity=color.w();
        symbols_->UpdateAllSymbol();
    }
}
LegacyCameraController::State LegacyCameraController::getCameraState() const {
    const auto* view = context_ ? context_->viewport() : nullptr;
    if(!view)return {};
    const auto state=view->queryCameraState();
    return {{state.eye.x(),state.eye.y(),state.eye.z()},
            {state.target.x(),state.target.y(),state.target.z()},
            {state.up.x(),state.up.y(),state.up.z()},state.ortho_zoom};
}
CCSMRendWidget::CCSMRendWidget(MotionOcc::LegacyPickContext* context,mMeshData* mesh,QObject* parent)
    :QObject(parent),context_(context),selection_(this),highlight_(context),symbols_(context),common_(context,&symbols_),geoParts_(context->document()),model_{mesh,&geoParts_},camera_(context) {
    selection_.symbols.changed=[this] { symbols_.setSelectedSymbols(selection_.symbols.values); };
    auto* owner = qobject_cast<QWidget*>(context->parent());
    partPicker_=MechMainWindow::mGeneralPickUp::createDirectionPickWidgetPickMore(MechMainWindow::CBRed,owner ? owner : context->viewport());
    partPicker_->widget()->hide();partPicker_->setContext(context);
    partPicker_->setAllowedTypes([] { return QSet<PickDataType>{PickDataType::Part}; });
    connect(partPicker_,&MechMainWindow::mPickWidgetController::dataChanged,this,[this] {
        std::vector<int> ids;
        const auto& data=partPicker_->getAllData().pickedData;
        auto it=data.find(PickDataType::Part);
        if(it!=data.end())for(const auto& p:it->second)ids.push_back(p.part_id>0 ? p.part_id : p.result_id);
        selection_.geometry.values.clear();selection_.geometry.values[SelectionInfoType::GeoPart]=ids;
        emit selection_.ReadyToReadContainer();
    });
}
CCSMRendWidget::~CCSMRendWidget() {
    if(partPicker_) {
        partPicker_->detachActive();
        auto* field=partPicker_->widget();
        delete partPicker_;delete field;
    }
}
void CCSMRendWidget::updatePaint() { if(context_ && context_->viewport())context_->viewport()->update(); }
AABB CCSMRendWidget::getAABBOfView() const {
    AABB result;if(!context_||!context_->viewport())return result;
    result.populated=context_->viewport()->queryVisibleBounds(result.minEdge,result.maxEdge);
    return result;
}
void CCSMRendWidget::beginPartPick() { if(partPicker_)partPicker_->setPickActive(true); }
std::vector<int> CCSMRendWidget::selectedParts() const { return selection_.geometry.getSelectDataByType(SelectionInfoType::GeoPart); }
void CCSMRendWidget::setSelectedParts(const std::vector<int>& ids,bool notify) {
    selection_.geometry.values.clear();selection_.geometry.values[SelectionInfoType::GeoPart]=ids;
    MechMainWindow::PickWidgetData data;for(int id:ids)data.pickedData[PickDataType::Part].emplace_back(id,-1,id);
    // This API explicitly changes the shared selected-part container; unlike
    // ordinary panel UI loading, its active collector must follow the change.
    if(partPicker_)partPicker_->displayData(data,true);
    if(notify)emit selection_.ReadyToReadContainer();
}
}
