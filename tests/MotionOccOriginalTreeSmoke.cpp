#include "MotionWindow.h"
#include "OccViewport.h"
#include "NativeMotionDocument.h"
#include "NativeBodyMass.h"
#include "RigidProject.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/mViewportModelManager.h"
#include "mMotionJointBase.h"
#include "mMotionScenario.h"
#include "mMechGlobalSignals.h"
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <stdexcept>

using namespace MotionOcc;
using namespace MxFunctions;
namespace {
constexpr const char* legacyOnlyCollections[] = {
    "electroImports", "electroExports", "hydraulics", "rlTasks", "mechanisms", "transmissions", "mechElecHyd"
};

void require(bool ok,const QString& reason) { if(!ok)throw std::runtime_error(reason.toStdString()); }
bool waitFor(const std::function<bool()>& predicate,int milliseconds=5000) {
    if(predicate())return true;QEventLoop loop;QTimer poll;poll.start(10);
    QObject::connect(&poll,&QTimer::timeout,&loop,[&] { if(predicate())loop.quit(); });
    QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();return predicate();
}
LegacyMotionContext* context() {
    require(getCVM && getCVM->getModelData()->context,"Original model facade is not active");
    return getCVM->getModelData()->context;
}
QTreeWidgetItem* node(QTreeWidget* tree,const QString& collection,int id,bool group=false) {
    for(QTreeWidgetItemIterator it(tree);*it;++it)
        if((*it)->data(0,Qt::UserRole+2).toString()==collection
            &&(*it)->data(0,Qt::UserRole+3).toBool()==group
            &&(group||(*it)->data(0,Qt::UserRole).toInt()==id))return *it;
    return nullptr;
}
void select(QTreeWidget* tree,const QString& collection,int id) {
    auto* item=node(tree,collection,id);require(item,"Missing original tree object: "+collection);
    tree->setCurrentItem(item);QApplication::processEvents();
}
bool hasJointGlyph(OccViewport& view, const QImage& frame, const std::array<double, 3>& origin) {
    require(!frame.isNull(), "Joint glyph check requires a rendered viewport");
    const auto point = view.projectPoint(origin);
    const double sx = double(frame.width()) / view.width(), sy = double(frame.height()) / view.height();
    const auto sample = QRectF((point.x() - 18) * sx, (point.y() - 18) * sy, 36 * sx, 36 * sy)
        .toAlignedRect().intersected(frame.rect());
    int gold = 0;
    for (int y = sample.top(); y <= sample.bottom(); ++y) for (int x = sample.left(); x <= sample.right(); ++x) {
        const auto pixel = frame.pixel(x, y);
        if (qRed(pixel) > qGreen(pixel) + 22 && qGreen(pixel) > qBlue(pixel) + 28) ++gold;
    }
    return gold >= 4;
}
QStringList menu(QTreeWidget* tree,QTreeWidgetItem* item,const QString& chosen={},const QString& type={}) {
    require(item,"Missing original tree menu target");tree->scrollToItem(item);QApplication::processEvents();
    const QPoint position=tree->visualItemRect(item).center();QString failure;QStringList entries;bool visited=false;
    QTimer trigger;trigger.setSingleShot(true);
    QObject::connect(&trigger,&QTimer::timeout,&trigger,[&] {
        visited=true;auto* popup=qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if(!popup)for(auto* candidate:tree->window()->findChildren<QMenu*>())
            if(candidate->objectName()=="originalMotionTreeMenu"&&candidate->isVisible()) { popup=candidate;break; }
        if(!popup) { failure="Original tree context menu did not open";return; }
        QAction* selected=nullptr;
        std::function<void(QMenu*)> inspect=[&](QMenu* current) {
            for(auto* action:current->actions()) {
                if(action->isSeparator())continue;
                if(current==popup)entries.append(action->text());
                if(action->text()==chosen && (type.isEmpty()||action->property("mxOriginalArgument").toString()==type))selected=action;
                if(action->menu())inspect(action->menu());
            }
        };
        inspect(popup);popup->close();
        if(!chosen.isEmpty()) {
            if(!selected||!selected->isEnabled()) { failure="Missing/enabled original tree action: "+chosen;return; }
            selected->trigger();
        }
    });
    trigger.start(0);
    QMetaObject::invokeMethod(tree,"customContextMenuRequested",Qt::DirectConnection,Q_ARG(QPoint,position));trigger.stop();
    require(visited&&failure.isEmpty(),failure.isEmpty()?"Original tree menu callback was not reached":failure);
    QApplication::processEvents();return entries;
}
void rename(QTreeWidget* tree,const QString& collection,int id,const QString& text) {
    menu(tree,node(tree,collection,id),"重命名");QLineEdit* editor=nullptr;
    for(auto* value:tree->findChildren<QLineEdit*>())if(value->isVisible())editor=value;
    require(editor,"Original inline tree rename editor did not open");editor->selectAll();editor->setText(text);
    QKeyEvent press(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(editor,&press);
    QApplication::processEvents();context()->flush();
    require(context()->document().entity(collection,id).value("name").toString()==text,"Original inline rename did not reach typed manager");
}
int copiedId(const NativeMotionDocument& document,const QString& collection,int original) {
    int result=-1;for(const auto& value:document.collection(collection)) {
        const int id=value.toObject().value("id").toInt(-1);if(id!=original)result=id;
    }return result;
}
QJsonObject xyz(double x,double y,double z) { return {{"x",x},{"y",y},{"z",z}}; }
NativeMotionDocument fixture(const QDir& directory,QString& geometry,QString& json) {
    const auto brep=directory.filePath("original-tree.brep");
    require(BRepTools::Write(BRepPrimAPI_MakeBox(30,10,8).Shape(),brep.toUtf8().constData()),"Cannot write tree fixture BRep");
    CadDocument cad;std::vector<int> ids;QString error;require(cad.importFile(brep,ids,error),error);
    geometry=directory.filePath("original-tree.step");require(cad.exportFile(geometry,-1,error),error);
    NativeBodyMassProperties mass;require(nativeBodyMassProperties(cad,{ids.front()},kDefaultNativeBodyDensityKgMm3,mass,error),error);
    QJsonArray inertia;for(double value:mass.inertiaKgMm2)inertia.append(value);
    const QJsonObject extra{{"precision",1.2345678901234567},{"newEngineField","preserve"}};
    QJsonObject data{
        {"bodies",QJsonArray{QJsonObject{{"id",0},{"name","树测试运动体"},{"geoEntityIds",QJsonArray{ids.front()}},
            {"isCustomMassProps",true},{"mass",mass.massKg},{"centerOfMass",xyz(mass.centerMm[0],mass.centerMm[1],mass.centerMm[2])},
            {"inertia",inertia},{"inertiaConvention","tensor"},{"futureData",extra}}}},
        {"joints",QJsonArray{QJsonObject{{"id",23},{"name","树测试运动副"},{"jointType","Revolute"},
            {"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(0,0,0)},{"axis",xyz(0,0,1)},{"futureData",extra}},
            QJsonObject{{"id",47},{"name","第二个运动副"},{"jointType","Revolute"},
            {"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(27,7,8)},{"axis",xyz(0,0,1)}}}},
        {"drives",QJsonArray{QJsonObject{{"id",9},{"name","树测试驱动"},{"targetType","Joint"},{"driveType","JointDrive"},
            {"targetJointId",23},{"motionType","Rotation"},{"controlMode","Velocity"},{"expression","0"}}}},
        {"markers",QJsonArray{QJsonObject{{"id",11},{"name","树测试标记点"},{"bodyId",0},
            {"localPosition",xyz(1.2345678901234567,2,3)},{"futureData",extra}}}},
        {"scenarios",QJsonArray{QJsonObject{{"id",1},{"name","树测试求解方案"},{"endTime",.1},{"stepSize",.01},{"gravity",xyz(0,0,0)}}}}
    };
    for(const auto* key:legacyOnlyCollections)
        data.insert(key,QJsonArray{QJsonObject{{"id",17},{"name",QString::fromLatin1(key)},{"futureData",extra}}});
    auto scenarios=data.value("scenarios").toArray();auto scenario=scenarios[0].toObject();
    const QJsonObject request{{"requestId",71},{"requestName","标记点位移"},{"sourceType","Marker"},{"sourceId",11},
        {"variable","Position"},{"component","X"},{"visible",true}};
    scenario.insert("curveRequests",QJsonArray{request});scenario.insert("currentCurveRequest",request);
    scenario.insert("trackedMarkerIds",QJsonArray{11});scenarios[0]=scenario;data.insert("scenarios",scenarios);
    NativeMotionDocument document;document.root={{"formatVersion",1},{"motionData",data}};
    json=directory.filePath("original-tree.motion.json");require(writeNativeMotionDocument(json,document,error),error);return document;
}
}
int runMotionOccOriginalTreeSmoke(QApplication&,const QString& directory) {
    QDir output(directory);QDir().mkpath(output.absolutePath());QFile report(output.filePath("original-tree-smoke.txt"));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;QTextStream log(&report);
    try {
        QString geometry,json;const auto source=fixture(output,geometry,json);
        MotionWindow window;window.resize(1440,960);window.show();auto* view=window.findChild<OccViewport*>();
        require(view&&waitFor([&] { return view->glReady(); }),"Tree test requires actual OpenGL viewport");
        require(window.importCad(geometry),"Tree fixture STEP failed");require(window.importMotionJson(json,false),"Tree fixture JSON failed");
        auto* tree=window.findChild<QTreeWidget*>("mTreeView");require(tree,"Original tree is absent");
        view->viewTop();
        const auto expectGlyphs = [&](bool first, bool second, const QString& message, const QString& screenshot = {}) {
            QApplication::processEvents();
            const auto frame = view->grabFramebuffer();
            if (!screenshot.isEmpty()) require(frame.save(output.filePath(screenshot)), "Cannot save joint glyph screenshot");
            require(hasJointGlyph(*view, frame, {0,0,0}) == first && hasJointGlyph(*view, frame, {27,7,8}) == second, message);
        };
        expectGlyphs(false, false, "Imported project rendered unselected joint glyphs");
        auto* group = node(tree, "joints", -1, true);
        require(group, "Joint group is absent");
        tree->setCurrentItem(group);
        expectGlyphs(true, true, "Joint group did not render both joints", "joint-group.png");
        require(tree->currentItem() == group && group->isSelected(), "Joint group lost its tree selection");
        select(tree, "joints", 23);
        expectGlyphs(true, false, "Joint leaf rendered a sibling glyph", "joint-child.png");
        const QPointF blank(view->width() - 30, view->height() - 60);
        QMouseEvent press(QEvent::MouseButtonPress, blank, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, blank, Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view, &press);
        QApplication::sendEvent(view, &release);
        require(tree->currentItem() == node(tree, "joints", 23) && tree->currentItem()->isSelected(),
            "Blank viewport click changed model tree selection");
        expectGlyphs(true, false, "Blank viewport click changed joint glyphs");
        select(tree, "joints", 47);
        expectGlyphs(false, true, "Joint change left the prior editor's cached glyph visible");
        select(tree, "drives", 9);
        expectGlyphs(true, false, "Drive did not restrict glyphs to its target joint");
        select(tree,"markers",11);
        expectGlyphs(false, false, "Unrelated tree node left a cached joint glyph visible", "joint-hidden.png");
        menu(tree, node(tree, "joints", 47), "删除");
        log << "PASS framebuffer joint group/leaf/drive/unrelated node and blank viewport click\n";log.flush();
        for(const auto* key:{"bodies","autoFlexBodies","joints","couplings","drives","springs","forceElements",
                "sensors","controllers","markers","contacts","controlInputs"}) {
            auto* group=node(tree,key,-1,true);require(group,"Missing original group: "+QString::fromLatin1(key));
            require(!group->icon(0).pixmap(16,16).isNull(),"Original tree group icon missing");
            const auto actions=menu(tree,group);require(actions.size()>=2 && actions.last().startsWith("删除所有"),"Original group creation/delete menu missing");
        }
        for(const auto* key:legacyOnlyCollections) {
            require(!node(tree,key,-1,true) && !node(tree,key,17),"旧配置没有从模型树隐藏："+QString::fromLatin1(key));
            require(context()->document().collection(key)==source.collection(key),"导入旧工程时丢失配置："+QString::fromLatin1(key));
        }
        auto rootActions=menu(tree,node(tree,"__motionRoot",-1,true));
#if defined(MX_HAS_BODY_DRIVES)
        const int bodyDriveAction = rootActions.indexOf(QStringLiteral("新建运动体驱动"));
        require(bodyDriveAction > 0 && rootActions.at(bodyDriveAction - 1) == QStringLiteral("新建驱动"),
            "Body drive creation submenu is missing from the root context menu");
        rootActions.removeAt(bodyDriveAction);
#endif
        for (const auto& hydTitle : {QStringLiteral("新建液压缸"), QStringLiteral("新建液压马达"),
                                     QStringLiteral("新建液压节点"), QStringLiteral("新建液压元件")}) {
            const int hydraulicAction = rootActions.indexOf(hydTitle);
            require(hydraulicAction > 0, "Hydraulic creation action is missing from the root context menu: " + hydTitle);
            rootActions.removeAt(hydraulicAction);
        }
        require(rootActions.size()==11&&rootActions.first()=="新建运动体"&&rootActions.last()=="删除运动数据","Original root context menu changed");
        require(menu(tree,node(tree,"markers",11))==QStringList{"删除","重命名","复制"},"Original child context menu changed");
        require(menu(tree,node(tree,"scenarios",1))==QStringList{"删除","重命名","求解"},"Original scenario context menu changed");
        log<<"PASS original tree groups, icons, root/group/entity/scenario menus and hidden legacy configurations\n";log.flush();
        for(const auto* key:{"markers","joints"}) {
            const QString collection=QString::fromLatin1(key);const int original=collection=="markers"?11:23;
            select(tree,collection,original);const auto originalRow=context()->document().entity(collection,original);
            menu(tree,node(tree,collection,original),"复制");const int copy=copiedId(context()->document(),collection,original);
            require(copy>=0,"Typed tree copy did not create an object");
            const auto copyRow=context()->document().entity(collection,copy);
            require(copyRow.value("futureData")==originalRow.value("futureData"),"Tree copy lost newer engine extension data");
            if(collection=="markers")require(copyRow.value("localPosition")==originalRow.value("localPosition"),"Tree copy rounded source coordinates");
            if(collection=="joints")require(copyRow.value("actionBodyId")==originalRow.value("actionBodyId"),"Tree copy lost original physical references");
            rename(tree,collection,copy,collection+"-renamed");
            menu(tree,node(tree,collection,copy),"删除");
            require(context()->document().entity(collection,copy).isEmpty(),"Typed tree delete left the copied object");
            require(!context()->document().entity(collection,original).isEmpty(),"Deleting copy removed original object");
        }
        select(tree,"bodies",0);menu(tree,node(tree,"bodies",0),"删除");
        require(!context()->document().entity("bodies",0).isEmpty(),"Tree deletion removed a still-referenced body");
        require(context()->document().entity("joints",23).value("actionBodyId").toInt(-1)==0,"Tree deletion silently converted a body reference to ground");
        log<<"PASS original typed copy/inline rename/delete, precision/extensions/references, live-panel deletion lifecycle\n";log.flush();
        for(const auto& pair:{qMakePair(QStringLiteral("平面副"),QStringLiteral("Planar")),qMakePair(QStringLiteral("点线副"),QStringLiteral("PointLine"))}) {
            menu(tree,node(tree,"joints",-1,true),pair.first,pair.second);
            const int created=copiedId(context()->document(),"joints",23);require(created>=0,"Original joint submenu did not create a typed joint");
            require(context()->document().entity("joints",created).value("jointType").toString()==pair.second,"Joint submenu created a wrong physical type");
            const auto* typed=context()->model()->getManager<MechMotionData::mMotionJointManager>(Physical_MotionJoint).getData(created);
            require(typed&&typed->getType()==(pair.second=="Planar"?JointType::Planar:JointType::PointLine),"Actual original typed joint enum differs from JSON");
            menu(tree,node(tree,"joints",created),"删除");
        }
        const auto count=context()->document().collection("joints").size();
        window.dispatchOriginalMotionAction("createMotionJoint","UnknownJointType");
        require(context()->document().collection("joints").size()==count,"Unknown joint type silently created a default revolute");
        struct ForceCase { const char* text;const char* type;const char* collection; };
        for(const auto& test:{ForceCase{"新建标量力","ScalarForce","scalarForces"},
                ForceCase{"新建标量力矩","ScalarTorque","scalarTorques"},
                ForceCase{"新建三点力","TriForce","threePointForces"},
                ForceCase{"新建三点力矩","TriTorque","threePointTorques"}}) {
            menu(tree,node(tree,"forceElements",-1,true),QString::fromUtf8(test.text),test.type);
            const auto rows=context()->document().collection(test.collection);require(rows.size()==1,"Original force type was saved to the wrong collection");
            const int id=rows.first().toObject().value("id").toInt();const auto* selected=tree->currentItem();
            require(selected&&selected->data(0,Qt::UserRole+2).toString()==test.collection
                &&selected->data(0,Qt::UserRole).toInt()==id,"Original force creation did not select its actual typed object");
            menu(tree,node(tree,test.collection,id),"复制");
            require(context()->document().collection(test.collection).size()==2,"Typed force copy failed");
            menu(tree,node(tree,"forceElements",-1,true),"删除所有力元");
            require(context()->document().collection(test.collection).isEmpty(),"Typed force group delete did not remove this force subtype");
        }
        window.dispatchOriginalMotionAction("createHydraulicMotor");
        auto* motorEditor = window.findChild<QWidget*>("nativeEditor_hydraulicMotors");
        require(motorEditor && !motorEditor->findChild<QWidget*>("native_name")
            && !motorEditor->findChild<QWidget*>("native_enabled"),
            "Hydraulic motor editor missing or leaked name/enabled controls");
        auto* motorApply = motorEditor->findChild<QPushButton*>("motionApplyButton");
        require(motorApply != nullptr, "Hydraulic motor editor missing apply button");
        motorApply->click();

        window.dispatchOriginalMotionAction("createHydraulicNode", "Tank");
        auto* nodeEditor = window.findChild<QWidget*>("nativeEditor_hydraulicNodes");
        require(nodeEditor && !nodeEditor->findChild<QWidget*>("native_name")
            && !nodeEditor->findChild<QWidget*>("native_enabled"),
            "Hydraulic node editor missing or leaked name/enabled controls");
        auto* nodeApply = nodeEditor->findChild<QPushButton*>("motionApplyButton");
        require(nodeApply != nullptr, "Hydraulic node editor missing apply button");
        nodeApply->click();

        window.dispatchOriginalMotionAction("createHydraulicComponent", "Pump");
        auto* compEditor = window.findChild<QWidget*>("nativeEditor_hydraulicComponents");
        require(compEditor && !compEditor->findChild<QWidget*>("native_name")
            && !compEditor->findChild<QWidget*>("native_enabled"),
            "Hydraulic component editor missing or leaked name/enabled controls");
        auto* compApply = compEditor->findChild<QPushButton*>("motionApplyButton");
        require(compApply != nullptr, "Hydraulic component editor missing apply button");
        compApply->click();

        require(node(tree, "hydraulicMotors", -1, true) && node(tree, "hydraulicNodes", -1, true)
            && node(tree, "hydraulicComponents", -1, true), "Stage 3 hydraulic groups did not appear in model tree");
        window.dispatchOriginalMotionAction("checkHydraulicCircuit");
        menu(tree, node(tree, "hydraulicComponents", -1, true), "删除所有液压元件");
        menu(tree, node(tree, "hydraulicNodes", -1, true), "删除所有液压节点");
        menu(tree, node(tree, "hydraulicMotors", -1, true), "删除所有液压马达");
        require(context()->document().collection("hydraulicMotors").isEmpty()
            && context()->document().collection("hydraulicNodes").isEmpty()
            && context()->document().collection("hydraulicComponents").isEmpty(),
            "Group delete did not clear Stage 3 hydraulic collections");

        menu(tree,node(tree,"markers",-1,true),"删除所有标记点");
        auto* markerGroup=node(tree,"markers",-1,true);require(markerGroup&&markerGroup->childCount()==0,"Empty fixed original group was removed");
        const auto scenario=context()->document().entity("scenarios",1);
        require(scenario.value("curveRequests").toArray().isEmpty()&&scenario.value("trackedMarkerIds").toArray().isEmpty()
            &&scenario.value("currentCurveRequest").toObject().isEmpty(),"Deleting display sources left dangling original curve/marker selections");
        QStringList solverStates;
        QObject statusObserver;
        const auto statusConnection=QObject::connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::mpcommSolverStatusChanged,&statusObserver,
            [&](const QString& state,const QVariantMap&) { solverStates.append(state); });
        qApp->setProperty("mpcomm_solve_requested",true);
        menu(tree,node(tree,"scenarios",1),"求解");
        require(solverStates.contains("started"),"Tree solve bypassed the original scenario onStartSolve lifecycle");
        require(waitFor([&] { return solverStates.contains("finished")||solverStates.contains("failed"); },15000),"Tree scenario solve did not complete");
        QObject::disconnect(statusConnection);
        const auto* solved=context()->model()->getManager<MechMotionData::mMotionScenarioManager>(Physical_MotionScenario).getData(1);
        require(solverStates.contains("finished")&&solved&&solved->hasResult(),"Tree solve did not return actual engine results through the original solver adapter");
        auto* solvedScenario = node(tree, "scenarios", 1);
        require(solvedScenario && solvedScenario->childCount() > 0 && !solvedScenario->child(0)->isDisabled(),
            "Solved scenario has no animation row");
        tree->setCurrentItem(solvedScenario->child(0));
        auto* playback = window.findChild<QWidget*>("originalMotionAnimationPanel");
        auto* timeline = playback ? playback->findChild<QSlider*>() : nullptr;
        require(timeline && timeline->maximum() > timeline->minimum(), "Original animation timeline did not open");
        timeline->setValue(timeline->maximum());
        expectGlyphs(false, false, "Playback restored unselected joint glyphs");
        tree->setCurrentItem(node(tree, "joints", -1, true));
        expectGlyphs(true, false, "Leaving animation did not restore requested joint group glyphs");
        require(window.grab().save(output.filePath("restored-original-tree.png")),"Cannot save original tree screenshot");
        log<<"PASS Planar/PointLine actual constructors/enums, unknown-type rejection, all four force subtype creation/selection/copy/delete, fixed group rules, curve-source cleanup\n";
        log<<"PASS original scenario tree solve -> original onStartSolve started/finished -> actual solver adapter and engine results\n";
        const auto path=output.filePath("legacy-configurations.mxmotion");
        RigidModel model;model.nativeDocument=std::make_shared<NativeMotionDocument>(context()->document());
        syncNativeMotionModel(model);
        QString error;
        require(saveRigidProject(path,context()->picks()->document(),model,error),error);
        CadDocument reopenedCad;RigidModel reopened;
        require(loadRigidProject(path,reopenedCad,reopened,error),error);
        require(bool(reopened.nativeDocument),"重开工程时丢失运动数据");
        for(const auto* key:legacyOnlyCollections)
            require(reopened.nativeDocument->collection(key)==source.collection(key),"工程往返丢失旧配置："+QString::fromLatin1(key));
        log<<"PASS project round-trip preserves all hidden legacy configurations and extension fields\n";
        return 0;
    } catch(const std::exception& error) { log<<"FAIL "<<error.what()<<'\n';return 1; }
}
