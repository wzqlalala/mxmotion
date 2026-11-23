#include "OccPickController.h"
#include "LegacyPickContext.h"
#include "OccPickPersistence.h"
#include "mGetSetTypeTemplate.h"
#include "mxr/view/CCSMRendWidget.h"
#include <QEvent>
#include <QTimer>
#include <algorithm>

namespace MechMainWindow {
namespace {
using Type=MxFunctions::PickDataType;
void appendUnique(OccPickMap& target,const OccPickMap& source)
{
    for(const auto& entry:source) for(const auto& value:entry.second) {
        auto& values=target[entry.first];
        if(std::find(values.begin(),values.end(),value)==values.end()) values.push_back(value);
    }
}
}
QPointer<mPickWidgetController> mPickWidgetController::s_active;

mPickWidgetController::mPickWidgetController(std::shared_ptr<mPickWidgetHighlightServiceBase> service,
                                             mPickWidgetViewBase* view,QWidget* parent)
    : QObject(parent?static_cast<QObject*>(parent):view),m_view(view),m_service(std::move(service)),
      m_layer(QStringLiteral("controller:%1").arg(quintptr(this)))
{
    if(!view) return;
    if(!view->parentWidget()) view->setParent(parent);
    view->installEventFilter(this);
    m_context=MotionOcc::LegacyPickContext::forObject(parent);
    connect(view,&mPickWidgetViewBase::activatePick,this,[this]{setPickActive(true);});
    connect(view,&mPickWidgetViewBase::clearAllRequested,this,[this]{m_model.clear();changed();});
    connect(view,&mPickWidgetViewBase::removePickRequested,this,[this](const QList<PickDisplayItem>& items){
        for(const auto& item:items) {
            if(item.source==PickDataSource::Set) m_model.removeSelectedSetName(item.sourceName);
            else m_model.removePickedData(item.type,item.data);
        }
        changed();
    });
    connect(view,&mPickWidgetViewBase::removePickRequestedByType,this,[this](Type type){m_model.clearPickedDataByType(type);changed();});
    connect(view,&mPickWidgetViewBase::selectSetChanged,this,[this](const QSet<QString>& names){m_model.setSelectedSetNames(names);changed();});
    connect(view,&mPickWidgetViewBase::calcMainNodeRequested,this,[this]{emit calcMainNodeSig();});
}
mPickWidgetController::~mPickWidgetController()
{
    // A parentless factory result is owned by its field. QWidget destroys its
    // children while its own widget internals are already being dismantled;
    // never call setPickActive() back into that partially destroyed field.
    m_view=nullptr;
    detachActive();
    if(auto* c=context()) c->clearHighlightLayer(m_layer);
}
MotionOcc::LegacyPickContext* mPickWidgetController::context() const
{
    if(m_contextExplicit)return m_context;
    if(auto* owner=MotionOcc::LegacyPickContext::forObject(this,false))return owner;
    return m_context ? m_context.data() : MotionOcc::LegacyPickContext::current();
}
void mPickWidgetController::setContext(MotionOcc::LegacyPickContext* context)
{
    const bool active=m_active; if(active) setPickActive(false);
    if(auto* old=this->context()) old->clearHighlightLayer(m_layer);
    m_context=context;m_contextExplicit=true;
    if(active) setPickActive(true); else refreshHighlight();
}
void mPickWidgetController::setPickWidgetVisible(bool visible)
{
    m_visible=visible;
    if(!visible) { setPickActive(false); clearAllHighlight(); }
    if(m_view) m_view->setVisible(visible);
    if(visible) refreshHighlight();
}
void mPickWidgetController::setDefultTip(const QString& text,const QString& top) { if(m_view)m_view->setDefultTip(text,top); }
void mPickWidgetController::setSelectSetList(const QHash<QString,QString>& values) { if(m_view)m_view->setSelectSetList(values); }
void mPickWidgetController::setSelectSetBtnVisible(bool value) { if(m_view)m_view->setSelectSetBtnVisible(value); }
void mPickWidgetController::displayData(const PickWidgetData& data,bool highlight)
{
    m_model.clear();
    m_model.setPickedData(data.pickedData); m_model.setSelectedSetNames(data.selectedSetNames);
    enforceSinglePickLimit();
    // Original panels use the default false while loading their UI. This must
    // not replace the active viewport collector or publish an inactive layer.
    if(highlight)refreshAll();else refreshUI();
}
void mPickWidgetController::replacePickedDataAndClearSets(const OccPickMap& data)
{
    m_model.clear();m_model.setPickedData(data);changed();
}
void mPickWidgetController::setPickActive(bool active)
{
    if(active && (!m_visible || m_disabled || !m_view || !m_view->isEnabled() || !bindingAlive())) return;
    if(active==m_active) { if(active) refreshAll(); return; }
    if(active) {
        if(s_active && s_active!=this) s_active->setPickActive(false);
        auto* c=context(); if(!c || !c->viewport()) return;
        m_context=c; s_active=this; m_active=true;
        m_view->setPickActive(true); setPickType(); refreshHighlight();
        emit pickActiveChanged(true);
    } else {
        const bool alive=bindingAlive();
        if(alive) commit();
        m_active=false; if(s_active==this)s_active=nullptr;
        if(auto* c=context()) c->end(this);
        if(m_view)m_view->setPickActive(false);
        if(alive && m_keepHighlight) refreshHighlight(); else clearAllHighlight();
        if(alive) emit pickActiveChanged(false);
    }
}
void mPickWidgetController::detachActive()
{
    m_active=false; if(s_active==this)s_active=nullptr;
    if(auto* c=context()){c->end(this);c->clearHighlightLayer(m_layer);}
    if(m_view)m_view->setPickActive(false);
}
bool mPickWidgetController::eventFilter(QObject* watched,QEvent* event)
{
    if(watched==m_view) {
        if(event->type()==QEvent::Hide || (event->type()==QEvent::EnabledChange && !m_view->isEnabled())) {
            setPickActive(false);clearAllHighlight();
        } else if(event->type()==QEvent::Show) refreshHighlight();
    }
    return QObject::eventFilter(watched,event);
}
void mPickWidgetController::setPickMode(mPickMode mode,bool clearWhenSwitch)
{
    if(m_view)m_view->setPickMode(mode,clearWhenSwitch);
    if(clearWhenSwitch)m_model.clear();
    enforceSinglePickLimit(); if(m_active)setPickType();refreshAll();
}
void mPickWidgetController::setCloudChartMode(bool enable)
{
    if(m_cloud==enable)return;
    m_cloud=enable;
    if(m_service)m_service->setHighlightSuppressed(enable);
    if(auto* c=context())c->setPickFeedbackVisible(this,!enable);
    // Cloud rendering hides feedback only. Both the model and the viewport's
    // typed selection remain live, including changes made while it is hidden.
    refreshAll();
}
QSet<Type> mPickWidgetController::allowedTypes() const
{
    if(m_disabled)return {};
    const auto types=m_types?m_types():QSet<Type>{};
    return types.isEmpty()?MechCommon::pickAllGeoAndMeshItem():types;
}
void mPickWidgetController::setAllowedTypes(std::function<QSet<Type>()> getTypes)
{
    m_types=std::move(getTypes);
    if(m_view) {
        QStringList names;
        const auto allowed=allowedTypes();
        for(auto type:allowed)names.append(PickDataTypeToString(type));
        names.sort();m_view->setRoundTooltipText(names.join(QStringLiteral("、")));
    }
    if(m_active)setPickType();
}
void mPickWidgetController::setAllPickTypesDisabled(bool disabled)
{
    m_disabled=disabled;
    if(disabled)setPickActive(false);
    if(m_view)m_view->setPickWidgetEnabled(!disabled);
}
void mPickWidgetController::refreshUI()
{
    if(!m_view)return;
    std::vector<PickDisplayItem> items;
    for(const auto& entry:m_model.getAllData().pickedData) for(const auto& value:entry.second)
        items.emplace_back(entry.first,value);
    std::sort(items.begin(),items.end(),[](const auto& a,const auto& b){
        return std::tie(a.type,a.data.part_id,a.data.result_id,a.data.result_id2)<std::tie(b.type,b.data.part_id,b.data.result_id,b.data.result_id2);
    });
    m_view->setCurrentData(items,m_model.getAllData().selectedSetNames);
}
OccPickMap mPickWidgetController::resolvedData() const
{
    auto data=m_model.getAllData().pickedData;
    if(m_setResolver)for(const auto& name:m_model.getAllData().selectedSetNames)appendUnique(data,m_setResolver(name));
    return data;
}
void mPickWidgetController::refreshHighlight()
{
    auto* c=context();if(!c)return;
    const bool temporary=m_service && dynamic_cast<mPickWidgetHighlightService*>(m_service.get());
    if(m_cloud || !m_visible || !m_view || !m_view->isVisible() || (!m_active&&(!m_keepHighlight||!temporary))) {c->clearHighlightLayer(m_layer);return;}
    const auto picks=c->resolve(resolvedData());
    QColor color=m_view->colorBlockColor();if(!color.isValid())color=QColor(232,130,28);
    c->setHighlightLayer(m_layer,picks,color);
}
void mPickWidgetController::refreshAll()
{
    refreshUI();
    if(m_active)if(auto* c=context())c->updateSelection(this,resolvedData());
    refreshHighlight();
}
void mPickWidgetController::clearAllHighlight(){if(auto* c=context())c->clearHighlightLayer(m_layer);}
void mPickWidgetController::clearAllTempHighlight(){if(!m_active)clearAllHighlight();}
void mPickWidgetController::changed()
{
    if(!bindingAlive()){detachActive();return;}
    enforceSinglePickLimit();m_dirty=true;refreshAll();emit dataChanged();
}
void mPickWidgetController::enforceSinglePickLimit()
{
    if(pickMode()!=PickOne)return;
    const auto sets=m_model.getAllData().selectedSetNames;
    if(!sets.isEmpty()) {
        m_model.clearPickedData();
        if(sets.size()>1)m_model.setSelectedSetNames({*sets.begin()});
        return;
    }
    auto data=m_model.getAllData().pickedData;
    for(auto& entry:data)if(!entry.second.empty()) {
        const auto type=entry.first;const auto last=entry.second.back();
        m_model.setPickedData({{type,{last}}});break;
    }
}
void mPickWidgetController::receiveViewportSelection(Type type,const std::vector<RenderPickResultData>& picks)
{
    if(!m_active || !allowedTypes().contains(type))return;
    auto data=m_model.getAllData().pickedData;
    if(pickMode()==PickOne)data.clear();
    data.erase(type);
    std::vector<RenderPickResultData> accepted;
    for(const auto& value:picks) {
        const auto pick=context()->resolve(type,value);
        if(!pick.isValid() || (!m_allowedParts.empty()&&!m_allowedParts.count(pick.partId)))continue;
        if(m_edgeLoadFilter&&(type==Type::GeoEdge||type==Type::ElementEdge)&&!context()->acceptsEdgeLoad(type,value))continue;
        // This original API filters only mesh-element dimensions. It must not
        // discard nodes (the original AutoFlex attachment picker also sets it).
        if(type==Type::Element && !m_elementFilter.isEmpty()) {
            const int dimension=pick.nodeIds.size()==4?3:pick.nodeIds.size()==3?2:pick.nodeIds.size()==2?1:0;
            if(!m_elementFilter.contains(dimension))continue;
        }
        accepted.push_back(value);
    }
    if(!accepted.empty())data[type]=std::move(accepted);
    m_model.setPickedData(data);
    if(pickMode()==PickOne)m_model.setSelectedSetNames({});
    changed();
}
void mPickWidgetController::setPickType()
{
    if(!m_active)return;
    auto* c=context();if(!c)return;
    const auto allowed=allowedTypes();
    if(allowed.isEmpty()){setPickActive(false);return;}
    auto type=c->pickType();
    if(!allowed.contains(type)) {
        // Stable preference follows the original geometry/mesh toolbar order.
        const Type order[]={Type::GeoPoint,Type::GeoEdge,Type::GeoFace,Type::GeoSolid,Type::Node,Type::ElementEdge,Type::ElementFace,Type::Element,Type::Part};
        for(auto candidate:order)if(allowed.contains(candidate)){type=candidate;break;}
    }
    c->begin(this,type,pickMode()==PickMore,resolvedData(),m_allowedParts);
}
void mPickWidgetController::setActivePickType(Type type){if(s_active)if(auto* c=s_active->context())c->setPickType(type);}
QSet<Type> mPickWidgetController::activeAllowedTypes(){return s_active?s_active->allowedTypes():QSet<Type>{};}
void mPickWidgetController::commitActivePickWidget(){if(s_active&&s_active->bindingAlive())s_active->commit();}
void mPickWidgetController::commit()
{
    if(!m_dirty||!bindingAlive())return;
    if(m_strategy) {
        const auto& state=m_model.getAllData();
        if(state.pickedData.empty()&&state.selectedSetNames.size()==1)m_strategy->saveSingleSet(*state.selectedSetNames.begin());
        else m_strategy->save(resolvedData());
    }
    m_dirty=false;
}

void mPickWidgetHighlightServiceBase::setContext(MotionOcc::LegacyPickContext* context) { m_context=context; }
void mPickWidgetHighlightServiceBase::setRenderWidget(mxr::CCSMRendWidget* widget)
{
    if(m_context)clearAllHighlight();
    m_context=widget?widget->legacyPickContext():nullptr;
}
void mPickWidgetHighlightServiceBase::highlightSet(const OccPickMap& data,bool on)
{
    if(!m_context)m_context=MotionOcc::LegacyPickContext::current();
    if(!m_context)return;
    if(on&&!m_suppressed)m_context->setHighlightLayer(m_key,m_context->resolve(data),m_color);
    else m_context->clearHighlightLayer(m_key);
}
void mPickWidgetHighlightServiceBase::highlightPicked(const OccPickMap& data,bool on){highlightSet(data,on);}
void mPickWidgetHighlightServiceBase::clearOwnTempHighlight(){if(m_context)m_context->clearHighlightLayer(m_key);}
void mPickWidgetHighlightServiceBase::clearAllHighlight(){clearOwnTempHighlight();}
void mPickWidgetHighlightServiceBase::flushHighlightNow(){if(m_context&&m_context->viewport())m_context->viewport()->update();}
} // namespace MechMainWindow
