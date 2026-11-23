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
#include <QImage>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <set>
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
bool showsNodeHighlight(OccViewport& viewport, const QPointF& point, const QColor& color)
{
    const auto image = viewport.grabFramebuffer();
    if (image.isNull()) return false;
    const int x = qRound(point.x() * image.width() / viewport.width());
    const int y = qRound(point.y() * image.height() / viewport.height());
    for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
        if (!image.rect().contains(x + dx, y + dy)) continue;
        const auto actual = image.pixelColor(x + dx, y + dy);
        if (std::abs(actual.red() - color.red()) + std::abs(actual.green() - color.green())
            + std::abs(actual.blue() - color.blue()) < 35) return true;
    }
    return false;
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
        const double boxRadius=QApplication::startDragDistance()+2;
        struct ClickTarget {std::size_t node; QPointF screen;};
        std::vector<ClickTarget> targets;
        for(std::size_t node=0;node<mesh.nodes.size()&&targets.size()<2;++node) {
            const auto screen=viewport->projectPoint(mesh.nodes[node],actualPart);const auto hit=viewport->pickAt(screen);
            if(hit.kind!=PickKind::Node||hit.nodeId!=ids[int(node)]||hit.partId!=actualPart)continue;
            if(!QRectF(viewport->rect()).adjusted(boxRadius,boxRadius,-boxRadius,-boxRadius).contains(screen))continue;
            if(!targets.empty()&&QRectF(targets.front().screen-QPointF(boxRadius,boxRadius),
                                        targets.front().screen+QPointF(boxRadius,boxRadius)).contains(screen))continue;
            targets.push_back({node,screen});
        }
        require(targets.size()==2,"Original AutoFlex mesh lacks two separately pickable nodes");
        const auto sendMouse=[&](QEvent::Type type,const QPointF& at,Qt::MouseButton button,Qt::MouseButtons buttons,
                                 Qt::KeyboardModifiers modifiers) {
            QMouseEvent event(type,at,viewport->mapToGlobal(at.toPoint()),button,buttons,modifiers);
            QApplication::sendEvent(viewport,&event);
        };
        const auto gesture=[&](Qt::MouseButton button,const QPointF& first,const QPointF& last,
                                Qt::KeyboardModifiers modifiers=Qt::NoModifier) {
            sendMouse(QEvent::MouseButtonPress,first,button,button,modifiers);
            if(first!=last)sendMouse(QEvent::MouseMove,last,Qt::NoButton,button,modifiers);
            sendMouse(QEvent::MouseButtonRelease,last,button,Qt::NoButton,modifiers);
            QApplication::processEvents();
        };
        const auto selectedIds=[&] {
            std::set<int> selected;
            for(const auto& entry:picker->getAllData().pickedData) {
                require(entry.first==PickDataType::Node,"AutoFlex attachment picker received a non-node");
                for(const auto& pick:entry.second)
                    require(pick.part_id==actualPart&&selected.insert(pick.result_id).second,
                            "AutoFlex attachment picker stored a duplicate or wrong-part node");
            }
            return selected;
        };
        const auto first=targets[0].screen,second=targets[1].screen;
        const int firstId=ids[int(targets[0].node)],secondId=ids[int(targets[1].node)];
        gesture(Qt::LeftButton,first,first);
        gesture(Qt::LeftButton,second,second);
        gesture(Qt::LeftButton,first,first);
        require(selectedIds()==std::set<int>{firstId,secondId},"Left-click add was not idempotent");
        gesture(Qt::RightButton,first,first);
        require(selectedIds()==std::set<int>{secondId},"Right-click did not remove the hit attachment node");
        gesture(Qt::RightButton,first,first);
        require(selectedIds()==std::set<int>{secondId},"Right-click on an unselected node removed a different attachment");

        const QPointF boxDelta(boxRadius,boxRadius);
        const QRectF box(first-boxDelta,first+boxDelta);
        std::set<int> expected{secondId};
        for(std::size_t node=0;node<mesh.nodes.size();++node)
            if(box.contains(viewport->projectPoint(mesh.nodes[node],actualPart)))expected.insert(ids[int(node)]);
        require(expected.count(firstId)&&!box.contains(second),"Invalid AutoFlex box fixture");
        gesture(Qt::LeftButton,box.topLeft(),box.bottomRight());
        require(selectedIds()==expected,"Plain left rectangle did not add the projected attachment nodes");
        gesture(Qt::RightButton,box.bottomRight(),box.topLeft());
        require(selectedIds()==std::set<int>{secondId},"Right rectangle did not preserve the outside attachment node");
        gesture(Qt::LeftButton,box.topLeft(),box.bottomRight(),Qt::ControlModifier);
        require(selectedIds()==expected,"Ctrl+left rectangle no longer adds attachment nodes");
        require(viewport->projectPoint(mesh.nodes[targets[0].node],actualPart)==first&&
                    viewport->projectPoint(mesh.nodes[targets[1].node],actualPart)==second,
                "Attachment rectangle gestures unexpectedly moved the camera");
        const auto highlight = picker->widget()->colorBlockColor();
        require(showsNodeHighlight(*viewport, second, highlight), "Active attachment node has no normal pick highlight");
        picker->setPickActive(false);
        require(showsNodeHighlight(*viewport, viewport->projectPoint(mesh.nodes[targets[1].node],actualPart), highlight),
            "Inactive attachment node has no temporary highlight");
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
        auto* reopenedViewport=reopened.findChild<OccViewport*>();
        require(reopenedViewport && waitFor([&] { return reopenedViewport->glReady(); }),"Reopened attachment viewport is not ready");
        auto* restored=context();
        const auto restoredPicks=restored->picks()->resolve(reopenedPicker->getAllData().pickedData);
        const auto restoredHighlight=reopenedPicker->widget()->colorBlockColor();
        require(std::any_of(restoredPicks.cbegin(),restoredPicks.cend(),[&](const auto& pick){
            return showsNodeHighlight(*reopenedViewport,reopenedViewport->projectPoint(pick.position,pick.partId),restoredHighlight);
        }),"Reopened attachments did not render temporary highlights");
        require(std::size_t(restoredPicks.size())==endFace.size(),"Reopened original picker did not restore actual mesh node IDs");
        require(reopened.grab().save(output.filePath("original-autoflex-reopened.png")),"Cannot capture restored original AutoFlex UI");
        log<<"PASS: original CAD + JSON import; original AutoFlex save -> true Netgen mesh; left-add/right-subtract click and rectangle; active and temporary node highlights; native attachment and solve; original project and field reopen.\n";
        log<<"Nodes="<<savedNodeCount<<" Tets="<<savedTetCount<<" AttachmentNodes="<<endFace.size()<<" Frames="<<result.frames.size()<<'\n';
        return 0;
    } catch(const std::exception& exception) { log<<"FAIL: "<<exception.what()<<'\n';return 1; }
}
