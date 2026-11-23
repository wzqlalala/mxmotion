#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"
#include "NativeBodyMass.h"
#include "NativeMotionSolver.h"
#include "NativeMotionGeometry.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/mViewportModelManager.h"
#include "mMotionAutoFlexPanel.h"
#include "mPickWidgetController.h"
#include "mPickWidgetViewBase.h"
#include <QApplication>
#include <QDoubleSpinBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>

using namespace MotionOcc;
using namespace MechMainWindow;
using namespace MxFunctions;
namespace {
void require(bool condition,const QString& message) { if(!condition)throw std::runtime_error(message.toStdString()); }
bool waitFor(const std::function<bool()>& predicate,int timeout=5000) {
    if(predicate())return true;QEventLoop loop;QTimer timer;
    QObject::connect(&timer,&QTimer::timeout,&loop,[&] { if(predicate())loop.quit(); });timer.start(15);
    QTimer::singleShot(timeout,&loop,&QEventLoop::quit);loop.exec();return predicate();
}
LegacyMotionContext* context() { require(getCVM&&getCVM->getModelData()->context,"Missing original model context");return getCVM->getModelData()->context; }
QJsonObject xyz(double x,double y,double z) { return {{"x",x},{"y",y},{"z",z}}; }
void selectFlex(MotionWindow& window) {
    auto* tree=window.findChild<QTreeWidget*>("modelTree");
    if(!tree)tree=window.findChild<QTreeWidget*>();require(tree,"Missing original model tree");
    for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+2).toString()=="autoFlexBodies"
        &&(*it)->data(0,Qt::UserRole).toInt()==13) { tree->setCurrentItem(*it);QApplication::processEvents();return; }
    throw std::runtime_error("Original AutoFlex node is missing");
}
mMotionAutoFlexPanel* activePanel(MotionWindow& window) {
    for(auto* panel:window.findChildren<mMotionAutoFlexPanel*>())if(panel->isVisible())return panel;
    throw std::runtime_error("Original AutoFlex panel was replaced or is not visible");
}
QPushButton* saveButton(mMotionAutoFlexPanel* panel) {
    for(auto* button:panel->findChildren<QPushButton*>())if(button->text()==QStringLiteral("保存"))return button;
    throw std::runtime_error("Original AutoFlex save button is missing");
}
}
int runMotionOccOriginalAutoFlexSmoke(QApplication&,const QString& directory) {
    QDir output(directory);QDir().mkpath(directory);QFile report(output.filePath("original-autoflex-smoke.txt"));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;QTextStream log(&report);
    try {
        CadDocument source;QString error;int part=-1;
        require(source.addPrimitive(Primitive::Box,40,10,10,part,error),error);
        const auto cadPath=output.filePath("original-autoflex.step");require(source.exportFile(cadPath,-1,error),error);
        NativeBodyMassProperties mass;require(nativeBodyMassProperties(source,{part},kDefaultNativeBodyDensityKgMm3,mass,error),error);
        QJsonArray inertia;for(double value:mass.inertiaKgMm2)inertia.append(value);
        const QJsonObject body{{"id",0},{"name","original flexible beam"},{"geoEntityIds",QJsonArray{901}},
            {"isFixed",false},{"isCustomMassProps",false},{"mass",mass.massKg},
            {"centerOfMass",xyz(mass.centerMm[0],mass.centerMm[1],mass.centerMm[2])},{"inertia",inertia},{"inertiaConvention","tensor"}};
        const QJsonObject material{{"id",0},{"name","original steel"},{"youngModulus",210000.0},{"poissonRatio",.3},{"density",7.85e-6}};
        const QJsonObject flex{{"id",13},{"name","original AutoFlex"},{"targetBodyId",0},{"materialId",0},
            {"enabled",true},{"elementSize",8.0},{"modeCount",2},{"connectionRadius",0.0},{"dampingType","None"}};
        NativeMotionDocument inputDocument;inputDocument.root={{"formatVersion",1},{"motionData",QJsonObject{
            {"bodies",QJsonArray{body}},{"materials",QJsonArray{material}},{"autoFlexBodies",QJsonArray{flex}},
            {"joints",QJsonArray{QJsonObject{{"id",23},{"name","fixed interface"},{"jointType","Fixed"},
                {"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(1,0,0)}}}},
            {"scenarios",QJsonArray{QJsonObject{{"id",1},{"name","original solve"},{"endTime",.02},{"stepSize",.01},
                {"gravity",xyz(0,0,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"}}}}}}};
        const auto jsonPath=output.filePath("original-autoflex.motion.json");require(writeNativeMotionDocument(jsonPath,inputDocument,error),error);
        auto windowOwner=std::make_unique<MotionWindow>();auto& window=*windowOwner;window.resize(1440,940);window.show();
        require(waitFor([&] { return window.isVisible(); }),"Original window did not show");
        require(window.importCad(cadPath)&&window.importMotionJson(jsonPath,false),"Original CAD / Motion JSON import failed");
        selectFlex(window);auto* panel=activePanel(window);
        QLineEdit* modulus=nullptr;QDoubleSpinBox* size=nullptr;
        for(auto* edit:panel->findChildren<QLineEdit*>())if(edit->toolTip().contains(QStringLiteral("弹性模量 E")))modulus=edit;
        for(auto* spin:panel->findChildren<QDoubleSpinBox*>())if(spin->suffix()==" mm"&&spin->value()==8.0)size=spin;
        require(modulus&&size,"Original material / element-size fields are missing");modulus->setText("200000");size->setValue(7.0);
        saveButton(panel)->click();
        require(waitFor([&] { context()->flush();return context()->document().entity("autoFlexBodies",13).value("mesh").toObject().value("tetrahedra").toArray().size()>5; },30000),
            "Saving original AutoFlex panel did not create/persist real Netgen tetrahedra");
        auto* original=context();auto* meshData=original->model()->getMeshData();
        const auto bodyParts=original->document().bodyPartBindings.value(0);require(bodyParts.size()==1,"Unexpected AutoFlex CAD binding");
        const int actualPart=bodyParts.front();const auto& mesh=meshData->meshes().at(actualPart);const auto ids=meshData->nodeIds().at(actualPart);
        require(mesh.nodes.size()>8&&mesh.tetrahedra.size()>5,"AutoFlex mesh was synthesized from primitive corners");
        auto saved=original->document().entity("autoFlexBodies",13);
        require(saved.value("elementSize").toDouble()==7.0&&saved.value("material").toObject().value("youngModulus").toDouble()==200000,
            "Original live material / mesh settings were not saved");
        auto* picker=panel->findChild<mPickWidgetController*>();auto* viewport=window.findChild<OccViewport*>();
        require(picker&&viewport,"Original AutoFlex picker is not owned by its field");
        picker->replacePickedDataAndClearSets({});picker->setPickActive(true);
        require(waitFor([&] { return viewport->isPicking(); }),"Original AutoFlex did not start real node picking");
        bool clicked=false;
        for(std::size_t node=0;node<mesh.nodes.size()&&!clicked;++node) {
            const auto screen=viewport->projectPoint(mesh.nodes[node],actualPart);const auto hit=viewport->pickAt(screen);
            if(hit.kind!=PickKind::Node||hit.nodeId!=ids[int(node)]||hit.partId!=actualPart)continue;
            QMouseEvent down(QEvent::MouseButtonPress,screen,viewport->mapToGlobal(screen.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
            QMouseEvent up(QEvent::MouseButtonRelease,screen,viewport->mapToGlobal(screen.toPoint()),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
            QApplication::sendEvent(viewport,&down);QApplication::sendEvent(viewport,&up);QApplication::processEvents();clicked=true;
        }
        require(clicked&&!picker->getAllData().pickedData.empty(),"Viewport hit did not reach the original attachment control");
        picker->setPickActive(false);
        std::vector<RenderPickResultData> endFace;
        for(std::size_t node=0;node<mesh.nodes.size();++node)if(std::abs(mesh.nodes[node][0])<1e-8) {
            RenderPickResultData selection;selection.part_id=actualPart;selection.result_id=ids[int(node)];endFace.push_back(selection);
        }
        require(endFace.size()>=3,"Real Netgen mesh lacks an attachment face");
        picker->replacePickedDataAndClearSets({{PickDataType::Node,endFace}});saveButton(panel)->click();original->flush();
        saved=original->document().entity("autoFlexBodies",13);
        require(!saved.value("attachmentMeshChanged").toBool()&&saved.value("attachPoints").toArray().size()==int(endFace.size())
            &&saved.value("mesh").toObject().value("nodeIds").toArray().size()==int(mesh.nodes.size()),"Original attachment selection lost its exact global-to-local mapping");
        require(window.grab().save(output.filePath("original-autoflex-real-mesh.png")),"Cannot capture original AutoFlex UI");
        struct RestoreWorker { QByteArray old=qgetenv("MXMOTION_MESH_WORKER");bool existed=qEnvironmentVariableIsSet("MXMOTION_MESH_WORKER");
            ~RestoreWorker(){ if(existed)qputenv("MXMOTION_MESH_WORKER",old);else qunsetenv("MXMOTION_MESH_WORKER"); } } restore;
        qputenv("MXMOTION_MESH_WORKER",output.filePath("disabled-worker-proves-reuse").toUtf8());
        NativeMotionInput solverInput;
        require(prepareNativeMotion(meshData->cad(),original->document(),1,solverInput,error),error);
        require(solverInput.geometry->flexBodies.size()==1&&solverInput.geometry->flexBodies[0].sourceBrep.isEmpty()
            &&solverInput.geometry->flexBodies[0].attachGroups.size()==1&&solverInput.geometry->flexBodies[0].attachPositions.empty(),
            "Native solver replaced original manual attachment with automatic joints");
        RigidResult result;require(solveNativeMotion(solverInput,result,error)==SolveStatus::Completed,error);
        require(!result.frames.empty()&&result.flexBodies.size()==1,"True engine did not return flexible frames");
        RigidModel persisted;persisted.nativeDocument=std::make_shared<NativeMotionDocument>(original->document());syncNativeMotionModel(persisted);
        const auto project=output.filePath("original-autoflex.mxmotion");require(saveRigidProject(project,meshData->cad(),persisted,error),error);
        const auto savedNodeCount=mesh.nodes.size(),savedTetCount=mesh.tetrahedra.size();
        window.hide();windowOwner.reset();QApplication::processEvents();
        MotionWindow reopened;reopened.resize(1440,940);reopened.show();require(reopened.openRigidProject(project),"Original AutoFlex project did not reopen");
        selectFlex(reopened);auto* reopenedPanel=activePanel(reopened);auto* reopenedPicker=reopenedPanel->findChild<mPickWidgetController*>();
        require(reopenedPicker,"Reopened original attachment field is missing");
        std::size_t selected=0;for(const auto& entry:reopenedPicker->getAllData().pickedData)selected+=entry.second.size();
        require(selected==endFace.size(),"Reopened original picker did not restore actual mesh node IDs");
        require(reopened.grab().save(output.filePath("original-autoflex-reopened.png")),"Cannot capture restored original AutoFlex UI");
        log<<"PASS: original CAD + JSON import; original AutoFlex save -> true Netgen mesh; real viewport node hit -> original field; manual end-face IDs -> native attachment; saved-mesh true flexible solve with worker disabled; original project and field reopen.\n";
        log<<"Nodes="<<savedNodeCount<<" Tets="<<savedTetCount<<" AttachmentNodes="<<endFace.size()<<" Frames="<<result.frames.size()<<'\n';
        return 0;
    } catch(const std::exception& exception) { log<<"FAIL: "<<exception.what()<<'\n';return 1; }
}
