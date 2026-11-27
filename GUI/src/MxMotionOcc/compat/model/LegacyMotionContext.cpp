#include "LegacyMotionContext.h"
#include "LegacyMotionJsonAdapter.h"
#include "LegacyFlexAttachmentCodec.h"
#include "mViewportModelManager.h"
#include "mMechGlobalSignals.h"
#include "mGlobalSignals.h"
#include "NativeTopologyBinding.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"
#include "picking/OccPickController.h"
#include <QAbstractButton>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QSet>

using namespace MxFunctions;
namespace MotionOcc {
namespace {
struct Selector { const char* ids;const char* refs;PickKind kind;PickDataType type;TopAbs_ShapeEnum topology; };
const Selector selectors[]={
    {"surfaceFaceIds","occSurfaceFaces",PickKind::Face,PickDataType::GeoFace,TopAbs_FACE},
    {"curveEdgeIds","occCurveEdges",PickKind::Edge,PickDataType::GeoEdge,TopAbs_EDGE},
    {"curveIEdgeIds","occCurveIEdges",PickKind::Edge,PickDataType::GeoEdge,TopAbs_EDGE},
    {"curveJEdgeIds","occCurveJEdges",PickKind::Edge,PickDataType::GeoEdge,TopAbs_EDGE}};
QJsonObject byId(const QJsonArray& rows,int id) {
    for(const auto& value:rows)if(value.toObject().value("id").toInt(-1)==id)return value.toObject();
    return {};
}
}
LegacyMotionContext::LegacyMotionContext(QWidget* owner,OccViewport* view,const CadDocument& cad,
    const std::map<int,FemMesh>& meshes,const NativeMotionDocument& document)
    :QObject(owner),cad_(cad),meshes_(meshes),document_(document),
     mesh_(std::make_unique<mMeshData>(cad,meshes)),geometry_(std::make_unique<GeoNeutralData::GeoEngineData>(cad)),
     units_(std::make_unique<CommonData::mUnitSystemManager>()),
     picks_(std::make_unique<LegacyPickContext>(owner,view,cad,&meshes)),
     render_(std::make_unique<mxr::CCSMRendWidget>(picks_.get(),mesh_.get())),
     model_(std::make_unique<MechModelData::mModelData>()),
     viewport_(std::make_unique<MechPreData::mViewportModel>(model_.get(),render_.get())) {
    model_->context=this;model_->bindBackends(mesh_.get(),geometry_.get(),units_.get());
    MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(viewport_.get());
    LegacyMotionJsonAdapter::load(inputForOriginalGui(),*model_);
    baseline_=LegacyMotionJsonAdapter::save(*model_);
    refreshMeshes();
    flushTimer_.setSingleShot(true);flushTimer_.setInterval(0);
    connect(&flushTimer_,&QTimer::timeout,this,&LegacyMotionContext::flush);
    auto* bus=MechCommon::mMechGlobalSignals::getInstance();
#define MX_CONNECT_CHANGE(Name) connect(bus,&MechCommon::mMechGlobalSignals::Name,this,[this] { scheduleFlush(); });
    MX_CONNECT_CHANGE(createUpdateMotionBodySig)
    MX_CONNECT_CHANGE(createUpdateMotionBodyBatchSig)
    MX_CONNECT_CHANGE(createUpdateMotionJointSig)
    MX_CONNECT_CHANGE(createUpdateMotionCouplingSig)
    MX_CONNECT_CHANGE(createUpdateMotionDriveSig)
    MX_CONNECT_CHANGE(createUpdateMotionMarkerSig)
    MX_CONNECT_CHANGE(createUpdateMotionVectorForceSig)
    MX_CONNECT_CHANGE(createUpdateMotionContactSig)
    MX_CONNECT_CHANGE(createUpdateMotionSpringSig)
    MX_CONNECT_CHANGE(createUpdateMotionControlInputSig)
    MX_CONNECT_CHANGE(createUpdateMotionSensorSig)
    MX_CONNECT_CHANGE(createUpdateMotionControllerSig)
    MX_CONNECT_CHANGE(createUpdateMotionAutoFlexSig)
    MX_CONNECT_CHANGE(createUpdateMotionScenarioSig)
    MX_CONNECT_CHANGE(createUpdateMotionElectroImportSig)
    MX_CONNECT_CHANGE(createUpdateMotionElectroExportSig)
    MX_CONNECT_CHANGE(createUpdateMotionHydraulicSig)
    MX_CONNECT_CHANGE(createUpdateMotionRLSig)
    MX_CONNECT_CHANGE(createUpdateMaterialSig)
#undef MX_CONNECT_CHANGE
    connect(bus,&MechCommon::mMechGlobalSignals::activePickSig,this,[this](int type) {
        // Original toolbar/body-list filter order: 0=None, 1..4=CAD
        // vertex/edge/face/solid, 5..8=FEM entities, 9=Part.
        if(type==9)render_->beginPartPick();
        else if(type>0 && type<9)picks_->setPickType(static_cast<PickDataType>(type));
    });
    connect(bus,&MechCommon::mMechGlobalSignals::outputPermanentInfoSig,this,&LegacyMotionContext::statusMessage);
    connect(MxFunctions::mGlobalSignals::getInstance(),qOverload<const QString&,bool>(&MxFunctions::mGlobalSignals::outputMessageSig),this,&LegacyMotionContext::message);
    connect(MxFunctions::mGlobalSignals::getInstance(),qOverload<int,const QString&,bool>(&MxFunctions::mGlobalSignals::outputMessageSig),
        this,[this](int level,const QString& text,bool) { emit messageWithLevel(level,text); });
    connect(picks_.get(),&LegacyPickContext::selectionRejected,this,[this](const QString& text) { emit message(text,true); });
    viewport_->flushMotion=[this] { flush(); };
    viewport_->commitAutoFlexAttachments=[this](int id,bool confirmed) {
        flexAttachmentCommits_.insert(id);if(confirmed)confirmedFlexAttachments_.insert(id);scheduleFlush();
    };
}
LegacyMotionContext::~LegacyMotionContext() {
    endEdit();
    if(getCVM==viewport_.get())MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(nullptr);
}
QJsonObject LegacyMotionContext::inputForOriginalGui() const {
    auto data=document_.root.value("motionData").toObject();
    auto bodies=data.value("bodies").toArray();
    for(int i=0;i<bodies.size();++i) {
        auto body=bodies[i].toObject();const int id=body.value("id").toInt(-1);
        QJsonArray ids;for(int part:document_.bodyPartBindings.value(id))ids.append(part);
        body.insert("geoEntityIds",ids);bodies[i]=body;
    }
    data.insert("bodies",bodies);
    auto joints=data.value("joints").toArray();
    for(int i=0;i<joints.size();++i) {
        auto joint=joints[i].toObject();
        for(const auto& selector:selectors)if(!joint.value(selector.refs).toArray().isEmpty()) {
            QJsonArray ids;
            for(const auto& value:joint.value(selector.refs).toArray()) {
                const auto ref=value.toObject();PickResult pick;
                pick.kind=selector.kind;pick.partId=ref.value("partId").toInt(-1);pick.index=ref.value("index").toInt(-1);
                const int id=picks_->globalTopologyId(pick);if(id>0)ids.append(id);
            }
            joint.insert(selector.ids,ids);
        }
        joints[i]=joint;
    }
    data.insert("joints",joints);
    auto flexes=data.value("autoFlexBodies").toArray();
    for(int index=0;index<flexes.size();++index) {
        auto flex=flexes[index].toObject();
        flex.insert("attachPoints",legacyFlexGuiAttachmentIds(flex,*mesh_,
            document_.bodyPartBindings.value(flex.value("targetBodyId").toInt(-1))));
        flexes[index]=flex;
    }
    if(data.contains("autoFlexBodies"))data.insert("autoFlexBodies",flexes);
    return data;
}
void LegacyMotionContext::refreshMeshes() {
    mesh_->rebuildNodeIndex();picks_->setMeshes(&meshes_);picks_->refreshGeometry();
    for(const auto& [part,ids]:mesh_->nodeIds())picks_->setNodeIds(part,ids);
    if(meshSnapshotInitialized_ && nodeIdSnapshot_!=mesh_->nodeIds()) { meshSnapshotDirty_=true;scheduleFlush(); }
    nodeIdSnapshot_=mesh_->nodeIds();meshSnapshotInitialized_=true;
}
void LegacyMotionContext::adoptExternalDocument(const NativeMotionDocument& document) {
    // Used for mesh attachments and analysis metadata, which do not replace
    // the original objects while a property widget still holds their address.
    document_=document;
}
void LegacyMotionContext::reloadDocument(const NativeMotionDocument& document) {
    endEdit();
    flushTimer_.stop();
    document_ = document;
    picks_->refreshGeometry();
    MechPreData::mViewportModelManager::getInstance()->setCurrentViewportModel(viewport_.get());
    LegacyMotionJsonAdapter::load(inputForOriginalGui(), *model_);
    baseline_ = LegacyMotionJsonAdapter::save(*model_);
    flexAttachmentCommits_.clear();
    confirmedFlexAttachments_.clear();
}
void LegacyMotionContext::setExtensionFields(const QString& collection,int id,const QJsonObject& fields) {
    flush();auto data=document_.root.value("motionData").toObject();auto rows=data.value(collection).toArray();
    for(int i=0;i<rows.size();++i) {
        auto row=rows[i].toObject();if(row.value("id").toInt(-1)!=id)continue;
        bool changed=false;
        for(auto it=fields.begin();it!=fields.end();++it) {
            if(it.value().isNull() || it.value().isUndefined()) {
                if(row.contains(it.key())) { row.remove(it.key());changed=true; }
            } else if(row.value(it.key())!=it.value()) {
                row.insert(it.key(),it.value());changed=true;
            }
        }
        if(changed) { rows[i]=row;data.insert(collection,rows);document_.root.insert("motionData",data);emit documentChanged(document_); }
        return;
    }
}
QJsonValue LegacyMotionContext::applyFieldChanges(const QJsonValue& before,const QJsonValue& after,const QJsonValue& original) {
    if(before==after)return original;
    if(before.isObject() && after.isObject()) {
        auto result=original.toObject();const auto a=before.toObject(),b=after.toObject();
        for(auto it=a.begin();it!=a.end();++it)if(!b.contains(it.key()))result.remove(it.key());
        for(auto it=b.begin();it!=b.end();++it)if(a.value(it.key())!=it.value())
            result.insert(it.key(),applyFieldChanges(a.value(it.key()),it.value(),result.value(it.key())));
        return result;
    }
    if(before.isArray() && after.isArray()) {
        // Curve rows carry stable request IDs. Editing their visibility or
        // colour must retain newer per-row extensions and unedited doubles.
        const auto a=before.toArray(),b=after.toArray(),source=original.toArray();
        for(const auto* key:{"id","requestId"}) {
            const auto keyed=[key](const QJsonArray& rows) {
                QSet<int> ids;
                for(const auto& value:rows) {
                    const auto row=value.toObject();const auto id=row.value(key);
                    if(!value.isObject() || !id.isDouble() || ids.contains(id.toInt()))return false;
                    ids.insert(id.toInt());
                }
                return true;
            };
            if(a.isEmpty() || !keyed(a) || !keyed(b) || !keyed(source))continue;
            const auto find=[key](const QJsonArray& rows,int id)->QJsonValue {
                for(const auto& value:rows)if(value.toObject().value(key).toInt()==id)return value;
                return QJsonValue(QJsonValue::Undefined);
            };
            QJsonArray result;
            for(const auto& value:b) {
                const int id=value.toObject().value(key).toInt();
                result.append(applyFieldChanges(find(a,id),value,find(source,id)));
            }
            return result;
        }
    }
    return after;
}
void LegacyMotionContext::bindTopologyEdits(const QString& collection,const QJsonObject& before,const QJsonObject& after,QJsonObject& merged) {
    if(collection=="bodies" && !after.value("isCustomMassProps").toBool()
       && (before.isEmpty() || before.value("inertia")!=after.value("inertia") || before.value("mass")!=after.value("mass")))
        merged.insert("inertiaConvention","tensor");
    if(collection=="bodies" && before.value("geoEntityIds")!=after.value("geoEntityIds")) {
        QVector<int> ids;for(const auto& value:after.value("geoEntityIds").toArray())ids.append(value.toInt());
        const int id=after.value("id").toInt();document_.bodyPartBindings.insert(id,ids);
    }
    if(collection!="joints")return;
    for(const auto& selector:selectors)if(before.value(selector.ids)!=after.value(selector.ids)) {
        QJsonArray refs;
        for(const auto& value:after.value(selector.ids).toArray()) {
            const auto pick=picks_->resolveGeometry(selector.type,value.toInt());
            if(!pick.isValid())throw std::runtime_error(QString("找不到所选几何拓扑 %1").arg(value.toInt()).toStdString());
            refs.append(nativeTopologyReference(cad_,pick.partId,pick.index,selector.topology));
        }
        merged.insert(selector.refs,refs);
    }
}
void LegacyMotionContext::flush() {
    if(flushing_)return;flushTimer_.stop();flushing_=true;
    const auto previousDocument=document_;
    try {
        const auto current=LegacyMotionJsonAdapter::save(*model_);
        auto data=document_.root.value("motionData").toObject();bool changed=false;
        for(auto collection=current.begin();collection!=current.end();++collection) {
            const auto before=baseline_.value(collection.key()).toArray(),after=collection.value().toArray();
            if(before==after)continue;
            auto originals=data.value(collection.key()).toArray();QJsonArray mergedRows;QSet<int> written;
            for(const auto& value:originals) {
                const auto original=value.toObject();const int id=original.value("id").toInt(-1);
                const auto oldRow=byId(before,id),newRow=byId(after,id);
                if(newRow.isEmpty()) { if(oldRow.isEmpty())mergedRows.append(original);else if(collection.key()=="bodies")document_.bodyPartBindings.remove(id);continue; }
                auto merged=applyFieldChanges(oldRow,newRow,original).toObject();
                bindTopologyEdits(collection.key(),oldRow,newRow,merged);mergedRows.append(merged);written.insert(id);
            }
            for(const auto& value:after) {
                auto row=value.toObject();const int id=row.value("id").toInt(-1);if(written.contains(id))continue;
                bindTopologyEdits(collection.key(),{},row,row);mergedRows.append(row);
            }
            data.insert(collection.key(),mergedRows);changed=true;
        }
        if(changed||meshSnapshotDirty_||!flexAttachmentCommits_.isEmpty()) {
            const auto before=data;
            synchronizeLegacyFlexAttachments(previousDocument,document_,data,baseline_,current,*mesh_,
                flexAttachmentCommits_,confirmedFlexAttachments_);
            changed=changed||before!=data;
        }
        baseline_=current;flexAttachmentCommits_.clear();confirmedFlexAttachments_.clear();meshSnapshotDirty_=false;
        if(changed) { document_.root.insert("motionData",data);emit documentChanged(document_); }
    } catch(const std::exception& e) { document_=previousDocument;emit message(QString::fromUtf8(e.what()),true); }
    flushing_=false;
}
void LegacyMotionContext::scheduleFlush() { if(!ending_)flushTimer_.start(); }
void LegacyMotionContext::watch(QWidget* panel) {
    panel_=panel;
    // Connect after the original handlers so the queued write sees the final
    // typed value. Marker point edits, for example, originally have no tree
    // update signal but still need to survive save/reopen.
    for(auto* w:panel->findChildren<QLineEdit*>())connect(w,&QLineEdit::editingFinished,this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<QComboBox*>())connect(w,qOverload<int>(&QComboBox::currentIndexChanged),this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<QAbstractButton*>())connect(w,&QAbstractButton::clicked,this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<QDoubleSpinBox*>())connect(w,qOverload<double>(&QDoubleSpinBox::valueChanged),this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<QSpinBox*>())connect(w,qOverload<int>(&QSpinBox::valueChanged),this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<QTableWidget*>())connect(w,&QTableWidget::itemChanged,this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<MechMainWindow::mPointPickerWidget*>())connect(w,&MechMainWindow::mPointPickerWidget::coordinateChanged,this,&LegacyMotionContext::scheduleFlush);
    for(auto* w:panel->findChildren<MechMainWindow::mDirectionSelectorWidget*>())connect(w,&MechMainWindow::mDirectionSelectorWidget::directionChanged,this,&LegacyMotionContext::scheduleFlush);
    for(auto* c:panel->findChildren<MechMainWindow::mPickWidgetController*>())connect(c,&MechMainWindow::mPickWidgetController::dataChanged,this,&LegacyMotionContext::scheduleFlush);
    panel->setProperty("mxOriginalMotionPanel",true);
}
void LegacyMotionContext::endEdit() {
    if(ending_)return;ending_=true;flush();
    if(unedit) { auto callback=std::move(unedit);unedit={};callback(); }
    if(render_ && render_->GetSymbolController())render_->GetSymbolController()->clear();
    if(auto* active=MechMainWindow::mPickWidgetController::activeController())active->setPickActive(false);
    panel_.clear();ending_=false;
}
}
