#include "MotionWindow.h"
#include "OccViewport.h"
#include "NativeMotionSymbols.h"
#include "OriginalEngineExtensions.h"
#include "compat/model/LegacyMotionContext.h"
#include "mMotionBodyPanel.h"
#include "mMotionBodyBatchPanel.h"
#include "mMotionJointPanel.h"
#include "mMotionCouplingPanel.h"
#include "mMotionDrivePanel.h"
#include "mMotionSpringPanel.h"
#include "mMotionVectorForcePanel.h"
#include "mMotionContactPanel.h"
#include "mMotionMarkerPanel.h"
#include "mMotionSensorPanel.h"
#include "mMotionControllerPanel.h"
#include "mMotionControlInputPanel.h"
#include "mMotionScenarioPanel.h"
#include "mMotionAutoFlexPanel.h"
#include "mMotionElectroImportPanel.h"
#include "mMotionElectroExportPanel.h"
#include "mMotionHydraulicPanel.h"
#include "mMotionRLPanel.h"
#include "mMotionMechanismPanel.h"
#include "mMotionTransmissionPanel.h"
#include "mMotionMechElecHydPanel.h"
#include <QAction>
#include <QAbstractButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QEventLoop>
#include <optional>

using namespace MechMainWindow;
using namespace MechMotionData;
using namespace MxFunctions;
namespace MotionOcc {
namespace {
template<class T> T namedType(const QString& name,const std::initializer_list<std::pair<const char*,T>>& values,T fallback) {
    for(const auto& value:values)if(name==QLatin1String(value.first))return value.second;
    return fallback;
}
std::optional<JointType> jointType(const QString& name) {
#define J(Name) {#Name,JointType::Name}
    static const QMap<QString,JointType> types{J(Revolute),J(Translational),J(Cylindrical),J(Spherical),J(Fixed),J(Universal),
        J(PointPlane),J(Line),J(Parallel),J(Perpendicular),J(Direction),J(Centripetal),J(ConstantVelocity),J(Screw),
        J(Hooke),J(Planar),J(Coupling),J(Gear),J(PointLine),J(PointOnCurve),J(PointOnSurface),J(CurveOnCurve)};
#undef J
    if(name.isEmpty())return JointType::Revolute;
    const auto found=types.constFind(name);
    if(found==types.cend())return std::nullopt;
    return found.value();
}
VectorLoadType loadType(const QString& name) {
#define L(Name) {#Name,VectorLoadType::Name}
    return namedType<VectorLoadType>(name,{L(Force),L(Torque),L(Bushing),L(ScalarForce),L(ScalarTorque),L(TriForce),L(TriTorque)},VectorLoadType::Force);
#undef L
}
SensorType sensorType(const QString& name) {
#define S(Name) {#Name,SensorType::Name}
    return namedType<SensorType>(name,{S(BodyDisplacement),S(BodyVelocity),S(BodyAcceleration),S(JointReaction),S(DriveActuation),S(Distance),S(Angle)},SensorType::BodyDisplacement);
#undef S
}
QJsonObject physicsFields(QJsonObject root) {
    auto data=root.value("motionData").toObject();
    auto scenarios=data.value("scenarios").toArray();
    for(int i=0;i<scenarios.size();++i) {
        auto row=scenarios.at(i).toObject();
        for(const auto* key:{"curveRequests","currentCurveRequest","curveNode","trackedMarkerIds"})row.remove(key);
        scenarios[i]=row;
    }
    data["scenarios"]=scenarios;root["motionData"]=data;return root;
}
}
void MotionWindow::ensureOriginalMotionContext() {
    ensureOriginalCurveHost();
    if(!m_model.nativeDocument)return;
    if(m_original && m_original->document().root==m_model.nativeDocument->root
       && m_original->document().bodyPartBindings==m_model.nativeDocument->bodyPartBindings
       && m_original->document().baseDirectory==m_model.nativeDocument->baseDirectory
       && m_original->document().projectAssetDirectory==m_model.nativeDocument->projectAssetDirectory) {
        m_original->refreshMeshes();return;
    }
    m_original.reset();
    m_original=std::make_unique<LegacyMotionContext>(this,m_viewport,m_document,m_meshes,*m_model.nativeDocument);
    connect(m_original.get(),&LegacyMotionContext::documentChanged,this,&MotionWindow::commitOriginalMotionDocument);
    connect(m_original.get(),&LegacyMotionContext::message,this,&MotionWindow::setNotice);
    auto* bridge=m_original->viewportModel();
    bridge->validateMotion=[this](int id) {
        if(m_original)m_original->flush();
        RigidInput input;QString error;
        if(prepareRigidModel(m_document,m_model,id,input,error))return QStringList{};
        return QStringList{error};
    };
    bridge->solveMotion=[this](int id) { startRigidSolve(id); };
    bridge->syncMotionResults=[this](int id) { syncOriginalMotionResults(id); };
    bridge->stopMotion=[this] { if(m_running)m_running->stop.store(true,std::memory_order_relaxed); };
    bridge->meshAutoFlex=[this](int id) { if(m_original)m_original->flush();ensureNativeFlexMesh(id); };
    bridge->inspectAutoFlex=[this](int id) { if(m_original)m_original->flush();inspectFlexModes(id); };
    bridge->closePanel=[this] { QTimer::singleShot(0,this,[this] { closeEditor(); }); };
    bridge->ensureTetraMesh=[this](const std::set<int>& parts,double size,QString& error) {
        if(motionBusy() || m_jobPart>=0) { error=tr("当前有求解或网格任务正在运行。");return false; }
        for(int part:parts) {
            const auto existing=m_meshes.find(part);
            if(existing!=m_meshes.end() && !existing->second.tetrahedra.empty())continue;
            QEventLoop wait;bool done=false,success=false;
            m_meshCompletion=[&](bool ok) { success=ok;done=true;wait.quit(); };
            if(!beginPartMesh(part,size,MeshKind::Volume)) { m_meshCompletion={};error=tr("无法为部件 %1 启动体网格划分。").arg(part);return false; }
            if(!done)wait.exec(QEventLoop::ExcludeUserInputEvents);
            if(!success) { error=tr("部件 %1 的真实体网格生成失败，请查看信息输出。").arg(part);return false; }
        }
        if(m_original) { m_original->refreshMeshes();if(m_model.nativeDocument)m_original->adoptExternalDocument(*m_model.nativeDocument); }
        return true;
    };
    bridge->previewBodyMesh=[this,savedMode=std::optional<OccViewport::DisplayMode>{}](int bodyId,bool show) mutable {
        if(!show) {
            if(m_original)m_original->picks()->clearHighlightLayer("autoFlexMesh");
            if(savedMode) { if(m_viewport)m_viewport->setDisplayMode(*savedMode);savedMode.reset(); }
            return;
        }
        if(!m_original||!m_viewport)return;
        if(!savedMode)savedMode=m_viewport->displayMode();
        QVector<PickResult> picks;
        const auto ids=m_original->model()->getMapRelationManager().getMapRelation<std::set<int>>({Physical_MotionBody,bodyId},Physical_Part);
        for(int part:ids)if(m_meshes.count(part)) { PickResult pick;pick.partId=part;pick.kind=PickKind::Part;picks.append(pick); }
        m_original->picks()->setHighlightLayer("autoFlexMesh",picks,QColor(0,150,210));
        m_viewport->setDisplayMode(OccViewport::DisplayMode::Mesh);
    };
    for(const auto& [id,result]:m_results)syncOriginalMotionResults(id);
}
void MotionWindow::commitOriginalMotionDocument(const NativeMotionDocument& document) {
    // Original panels edit live typed objects and permit incomplete drafts.
    // Physics validation remains at solve/export, not on every intermediate
    // body/point combo selection. In particular this must not close the editor.
    if(m_model.nativeDocument && m_model.nativeDocument->root==document.root
       && m_model.nativeDocument->bodyPartBindings==document.bodyPartBindings)return;
    const bool physicsChanged=!m_model.nativeDocument
        || physicsFields(m_model.nativeDocument->root)!=physicsFields(document.root)
        || m_model.nativeDocument->bodyPartBindings!=document.bodyPartBindings;
    if(physicsChanged) {
        // Preserve edits made while the worker owns its immutable input, but
        // never attach that worker's old result to the newly edited model.
        if(m_running) { m_runningInputChanged=true;m_running->stop.store(true,std::memory_order_relaxed); }
        clearResults();
    }
    m_model.nativeDocument=std::make_shared<NativeMotionDocument>(document);
    // The original scenario manager owns curve requests. Reparse those
    // changes instead of retaining the retired generated editor's copy.
    m_model.scenarios.clear();
    syncNativeMotionModel(m_model);m_modified=true;
    updateTree();updateActions();
    if(physicsChanged)m_viewport->setMotionSymbols(nativeMotionSymbols(m_document,document,{},m_selectionNode.collection,m_selectionNode.id));
}
void MotionWindow::publishOriginalSolveProgress(int id,int percent) {
    if(m_original)emit m_original->viewportModel()->motionSolveProgress(id,percent);
}
void MotionWindow::publishOriginalSolveFinished(int id,bool success,const QString& message) {
    if(!m_original)return;
    emit m_original->viewportModel()->motionSolveMessage(id,message);
    emit m_original->viewportModel()->motionSolveFinished(id,success);
}
bool MotionWindow::editOriginalMotionEntity(const QString& collection,int id,const QString& type) {
    static const QSet<QString> supported{"bodies","bodyBatch","joints","couplings","drives","springs","vectorForces",
        "scalarForces","scalarTorques","threePointForces","threePointTorques","contacts","markers","sensors","controllers",
        "controlInputs","scenarios","autoFlexBodies","electroImports","electroExports","hydraulics","rlTasks",
        "mechanisms","transmissions","mechElecHyd"};
    if(!supported.contains(collection))return false;
    if(motionBusy() || m_jobPart>=0)return true;
    if(collection=="joints" && id<0 && !jointType(type)) {
        setNotice(tr("无法创建未知运动副类型：%1").arg(type),true);return true;
    }
    if(!ensureNativeModel())return true;
    closeEditor();ensureOriginalMotionContext();
    QSet<int> previousIds;for(const auto& row:m_original->document().collection(collection))previousIds.insert(row.toObject().value("id").toInt(-1));
    QWidget* panel=nullptr;std::function<void()> enter,leave;
    std::function<void(int)> editCreated;
    // These are the uploaded GUI classes, with their original constructors,
    // folding groups, icon controls, type switching and pick workflows.
#define PANEL(Class,NewCall,EditCall,UneditCall) \
    { auto* p=new Class;panel=p;QPointer<Class> pointer(p); \
      enter=[=] { if(id<0) p->NewCall;else p->EditCall; }; \
      editCreated=[pointer](int id) { if(pointer)pointer->EditCall; }; \
      leave=[pointer] { if(pointer)pointer->UneditCall; }; }
    if(collection=="bodies")PANEL(mMotionBodyPanel,newMotionBodyPanel(),editMotionBodyPanel(id),uneditMotionBodyPanel())
    else if(collection=="bodyBatch")PANEL(mMotionBodyBatchPanel,newBatchPanel(),newBatchPanel(),uneditBatchPanel())
    else if(collection=="joints")PANEL(mMotionJointPanel,newMotionJointPanel(*jointType(type)),editMotionJointPanel(id),uneditMotionJointPanel())
    else if(collection=="couplings") {
        const auto kind=namedType<CouplingType>(type,{{"Gear",CouplingType::Gear},{"GearRack",CouplingType::GearRack},{"Cable",CouplingType::Cable}},CouplingType::Gear);
        PANEL(mMotionCouplingPanel,newMotionCouplingPanel(kind),editMotionCouplingPanel(id),uneditMotionCouplingPanel())
    } else if(collection=="drives") {
        const auto mode=type=="Position" ? DriveControlMode::Position : type=="Acceleration" ? DriveControlMode::Acceleration : DriveControlMode::Velocity;
        PANEL(mMotionDrivePanel,newMotionDrivePanel(DriveTargetType::Joint,mode),editMotionDrivePanel(id),uneditMotionDrivePanel())
    } else if(collection=="springs") {
        const auto kind=namedType<SpringType>(type,{{"BodyPoint",SpringType::BodyPoint},{"Translational",SpringType::Translational},{"Torsional",SpringType::Torsional}},SpringType::BodyPoint);
        PANEL(mMotionSpringPanel,newMotionSpringPanel(kind),editMotionSpringPanel(id),uneditMotionSpringPanel())
    } else if(QStringList{"vectorForces","scalarForces","scalarTorques","threePointForces","threePointTorques"}.contains(collection)) {
        const auto kind=collection=="scalarForces" ? VectorLoadType::ScalarForce : collection=="scalarTorques" ? VectorLoadType::ScalarTorque
            : collection=="threePointForces" ? VectorLoadType::TriForce : collection=="threePointTorques" ? VectorLoadType::TriTorque : loadType(type);
        PANEL(mMotionVectorForcePanel,newMotionVectorForcePanel(kind),editMotionVectorForcePanel(id),uneditMotionVectorForcePanel())
    } else if(collection=="contacts")PANEL(mMotionContactPanel,newMotionContactPanel(MotionContactType::CADContact),editMotionContactPanel(id),uneditMotionContactPanel())
    else if(collection=="markers")PANEL(mMotionMarkerPanel,newMotionMarkerPanel(),editMotionMarkerPanel(id),uneditMotionMarkerPanel())
    else if(collection=="sensors")PANEL(mMotionSensorPanel,newMotionSensorPanel(sensorType(type)),editMotionSensorPanel(id),uneditMotionSensorPanel())
    else if(collection=="controllers")PANEL(mMotionControllerPanel,newMotionControllerPanel(),editMotionControllerPanel(id),uneditMotionControllerPanel())
    else if(collection=="controlInputs")PANEL(mMotionControlInputPanel,newControlInputPanel(),editControlInputPanel(id),uneditControlInputPanel())
    else if(collection=="scenarios")PANEL(mMotionScenarioPanel,newMotionScenarioPanel(),editMotionScenarioPanel(id),uneditMotionScenarioPanel())
    else if(collection=="autoFlexBodies")PANEL(mMotionAutoFlexPanel,newMotionAutoFlexPanel(),editMotionAutoFlexPanel(id),uneditMotionAutoFlexPanel())
    else if(collection=="electroImports")PANEL(mMotionElectroImportPanel,newElectroImportPanel(),editElectroImportPanel(id),uneditElectroImportPanel())
    else if(collection=="electroExports")PANEL(mMotionElectroExportPanel,newElectroExportPanel(),editElectroExportPanel(id),uneditElectroExportPanel())
    else if(collection=="hydraulics")PANEL(mMotionHydraulicPanel,newHydraulicPanel(),editHydraulicPanel(id),uneditHydraulicPanel())
    else if(collection=="rlTasks")PANEL(mMotionRLPanel,newRLPanel(),editRLPanel(id),uneditRLPanel())
    else if(collection=="mechanisms")PANEL(mMotionMechanismPanel,newMechanismPanel(),editMechanismPanel(id),uneditMechanismPanel())
    else if(collection=="transmissions")PANEL(mMotionTransmissionPanel,newTransmissionPanel(),editTransmissionPanel(id),uneditTransmissionPanel())
    else if(collection=="mechElecHyd")PANEL(mMotionMechElecHydPanel,newMechElecHydPanel(),editMechElecHydPanel(id),uneditMechElecHydPanel())
#undef PANEL
    if(!panel)return false;
    panel->setObjectName("originalMotionEditor_"+collection);
    showOriginalParameterPanel(panel, "originalMotionScroll_"+collection);
    m_original->watch(panel);m_original->unedit=std::move(leave);
    enter();m_original->flush();
    if(id<0) {
        for(const auto& row:m_original->document().collection(collection)) {
            const int candidate=row.toObject().value("id").toInt(-1);
            if(!previousIds.contains(candidate)) { id=candidate;break; }
        }
        // The uploaded tree selects a newly created item and enters its edit
        // method. NewCall creates data; EditCall fills the original controls
        // and reveals type-specific groups (notably curve/face collectors).
        // Batch/template panels that create no entity remain in their New UI.
        if(id>=0 && editCreated) { editCreated(id);m_original->flush(); }
    }
    appendOriginalEngineExtensions(panel,m_original.get(),collection,id);
    if(id>=0)m_selectionNode=collection=="scenarios" ? ModelNode{ModelNode::Scenario,id} : ModelNode{ModelNode::Native,id,collection};
    updateTree();return true;
}
void MotionWindow::dispatchOriginalMotionAction(const QString& action,const QVariant& argument) {
    if (action == "importMotionMnf") { importMnf(argument.toString()); return; }
    if(action=="createMotionVectorForce") {
        static const QMap<QString,QString> collections{{"ScalarForce","scalarForces"},{"ScalarTorque","scalarTorques"},
            {"TriForce","threePointForces"},{"TriTorque","threePointTorques"}};
        const auto type=argument.toString();
        editOriginalMotionEntity(collections.value(type,QStringLiteral("vectorForces")),-1,type);return;
    }
    if(action=="createMotionBodyOneKey") {
        if(!ensureNativeModel() || motionBusy())return;
        closeEditor();ensureOriginalMotionContext();QStringList skipped;
        const int count=mMotionBodyBatchPanel::batchCreateMotionBodies(m_original->model()->getMeshData()->getAllPartIds(),&skipped);
        m_original->flush();setNotice(tr("已生成 %1 个运动体。").arg(count));return;
    }
    static const QMap<QString,QString> actions{
        {"createMotionBody","bodies"},{"createMotionBodyBatch","bodyBatch"},{"createMotionJoint","joints"},
        {"createMotionCoupling","couplings"},{"createMotionDrive","drives"},{"createMotionSpring","springs"},
        {"createMotionVectorForce","vectorForces"},{"createMotionContact","contacts"},{"createMotionMarker","markers"},
        {"createMotionSensor","sensors"},{"createMotionController","controllers"},{"createControlInput","controlInputs"},
        {"createMotionScenario","scenarios"},{"createMotionAutoFlex","autoFlexBodies"},{"createElectroImport","electroImports"},
        {"createElectroExport","electroExports"},{"createHydraulic","hydraulics"},{"createRLPanel","rlTasks"},
        {"createMechanism","mechanisms"},{"createTransmission","transmissions"},{"createMechElecHyd","mechElecHyd"}};
    if(actions.contains(action)) { editOriginalMotionEntity(actions.value(action),-1,argument.toString());return; }
    if(action=="createMotionSoftStop" || action=="createSoftStop") { if(ensureNativeModel())editNativeEntity("softStops");return; }
    if(action=="createMotionTensionCable" || action=="createTensionCable") { if(ensureNativeModel())editNativeEntity("tensionCables");return; }
    if(dispatchOriginalWorkspaceAction(action,argument))return;
    QString title;
    const auto matches=[&](const QObject* object) {
        return object->property("mxOriginalAction").toString()==action
            && object->property("mxOriginalArgument")==argument;
    };
    for(const auto* item:findChildren<QAction*>())if(matches(item)) { title=item->text();break; }
    if(title.isEmpty())for(const auto* button:findChildren<QAbstractButton*>())if(matches(button)) { title=button->text();break; }
    setNotice(title.isEmpty() ? tr("该操作尚未连接到当前几何和网格后端。")
                             : tr("“%1”尚未连接到当前几何和网格后端。").arg(title),true);
}
}
