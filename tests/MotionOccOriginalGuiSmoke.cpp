#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"
#include "NativeBodyMass.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/mViewportModelManager.h"
#include "mMotionJointPanel.h"
#include "mMotionMarkerPanel.h"
#include "mMotionScenarioPanel.h"
#include "mMotionBodyPanel.h"
#include "mMotionAnimationPanel.h"
#include "mMotionCurvePanel.h"
#include "mCurveWindow.h"
#include "mPlotView.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"
#include "mPickWidgetController.h"
#include "mPickWidgetViewBase.h"
#include "mMotionJointPointOnCurve.h"
#include "mMotionJointPointOnSurface.h"
#include "mMotionResultData.h"
#include "mMotionResultQueryService.h"
#include "mMotionBodySensor.h"
#include "mMotionDistanceSensor.h"
#include "mMotionAngleSensor.h"
#include "mMotionRefMap.h"
#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <TopoDS_Compound.hxx>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QJsonDocument>
#include <QLineEdit>
#include <QLineF>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QKeyEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QTabBar>
#include <cmath>
#include <limits>
#include <stdexcept>

using namespace MotionOcc;
using namespace MechMainWindow;
using namespace MechMotionData;
using namespace MxFunctions;
namespace {
void require(bool condition,const QString& reason) { if(!condition)throw std::runtime_error(reason.toStdString()); }
bool waitFor(const std::function<bool()>& predicate,int milliseconds=5000) {
    if(predicate())return true;QEventLoop loop;QTimer poll;poll.start(10);
    QObject::connect(&poll,&QTimer::timeout,&loop,[&] { if(predicate())loop.quit(); });
    QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();return predicate();
}
QJsonObject xyz(double x,double y,double z) { return {{"x",x},{"y",y},{"z",z}}; }
LegacyMotionContext* context() { require(getCVM && getCVM->getModelData()->context,"Original model facade is not active");return getCVM->getModelData()->context; }
QTreeWidgetItem* findNode(QTreeWidget* tree,ModelNode::Kind kind,int id,const QString& collection={}) {
    for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toInt()==int(kind)
        && (*it)->data(0,Qt::UserRole).toInt()==id && (collection.isEmpty() || (*it)->data(0,Qt::UserRole+2).toString()==collection))return *it;
    return nullptr;
}
void select(QTreeWidget* tree,ModelNode::Kind kind,int id,const QString& collection={}) {
    auto* item=findNode(tree,kind,id,collection);require(item,"Missing model tree item: "+collection+QString::number(id));tree->setCurrentItem(item);QApplication::processEvents();
}
template<class T> T* active(MotionWindow& window) {
    for(auto* item:window.findChildren<T*>())if(item->isVisible())return item;
    throw std::runtime_error("Original GUI panel not visible");
}
void enter(QLineEdit* edit,const QString& text) {
    require(edit,"Missing original numeric/coordinate input");edit->setFocus();edit->selectAll();edit->setText(text);
    QKeyEvent down(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(edit,&down);
    QKeyEvent up(QEvent::KeyRelease,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(edit,&up);QApplication::processEvents();
}
NativeMotionDocument fixture(const QDir& output,QString& cadFile,QString& jsonFile) {
    TopoDS_Compound compound;BRep_Builder builder;builder.MakeCompound(compound);
    builder.Add(compound,BRepPrimAPI_MakeBox(gp_Pnt(0,-5,-4),30,10,8).Shape());
    builder.Add(compound,BRepPrimAPI_MakeBox(gp_Pnt(-14,-8,-4),12,16,8).Shape());
    const auto brep=output.filePath("original-ui-fixture.brep");require(BRepTools::Write(compound,brep.toUtf8().constData()),"Cannot write BRep fixture");
    CadDocument cad;QString error;std::vector<int> ids;require(cad.importFile(brep,ids,error),error);
    cadFile=output.filePath("original-ui-fixture.step");require(cad.exportFile(cadFile,-1,error),error);
    QJsonArray bodies;for(const auto& part:cad.parts()) {
        NativeBodyMassProperties p;require(nativeBodyMassProperties(cad,{part.id},kDefaultNativeBodyDensityKgMm3,p,error),error);
        const bool fixed=p.centerMm[0]<0;QJsonArray inertia;for(double value:p.inertiaKgMm2)inertia.append(value);
        bodies.append(QJsonObject{{"id",fixed?37:0},{"name",fixed?"原始基座":"原始转子"},{"geoEntityIds",QJsonArray{fixed?205:901}},
            {"isFixed",fixed},{"isCustomMassProps",false},{"mass",p.massKg},{"centerOfMass",xyz(p.centerMm[0],p.centerMm[1],p.centerMm[2])},
            {"inertia",inertia},{"inertiaConvention","tensor"},{"futureMaterial",QJsonObject{{"value",1.2345678901234567}}}});
    }
    NativeMotionDocument doc;doc.root={{"formatVersion",1},{"vendorExtension",QJsonObject{{"untouched",9.876543210987654}}},{"motionData",QJsonObject{
        {"bodies",bodies},{"joints",QJsonArray{QJsonObject{{"id",23},{"name","原始铰链"},{"jointType","Revolute"},{"actionBodyId",0},{"baseBodyId",37},
            {"origin",xyz(0,0,0)},{"axis",xyz(0,0,1.000000000000001)},{"futureConstraint",QJsonObject{{"tolerance",1.2345678901234567e-12}}}}}},
        {"markers",QJsonArray{QJsonObject{{"id",11},{"name","原始标记点"},{"bodyId",0},{"localPosition",xyz(30,0,0)},{"localRotation",QJsonObject{{"w",1},{"x",0},{"y",0},{"z",0}}}}}},
        {"drives",QJsonArray{QJsonObject{{"id",9},{"name","原始驱动"},{"driveType","JointDrive"},{"targetType","Joint"},{"targetJointId",23},
            {"motionType","Rotation"},{"controlMode","Velocity"},{"expression","90"}}}},
        {"scenarios",QJsonArray{QJsonObject{{"id",1},{"name","原始求解方案"},{"endTime",.1},{"stepSize",.01},{"gravity",xyz(0,0,0)},
            {"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"},{"solverTolerance",1e-10}}}}}}};
    jsonFile=output.filePath("original-ui-fixture.motion.json");require(writeNativeMotionDocument(jsonFile,doc,error),error);return doc;
}
void snapshot(MotionWindow& window,const QDir& output,const QString& name) {
    QApplication::processEvents();require(window.grab().save(output.filePath(name)),"Cannot save original GUI screenshot");
}
void queryRegression(const mMotionResultData& accepted) {
    using Query=mMotionResultQueryService;
    mMotionCurveRequest request;request.m_sourceType=CurveSourceType::Body;request.m_sourceId=0;
    request.m_variable=CurveVariable::AngularVelocity;request.m_component=CurveComponent::Z;
    auto samples=Query::getResultSeries(&accepted,request);
    require(samples.values.size()==int(accepted.m_frames.size())&&std::abs(samples.values.last()-90)<1e-4,
        "Original body angular velocity was converted between degrees/radians twice");
    mMotionResultData trace;
    const double times[]{0.,.02,.09};
    for(int i=0;i<3;++i) {
        mMotionFrame frame;frame.m_time=times[i];mMotionBodyFrame body;
        body.m_position=QVector3D(2+std::pow(times[i],3),0,0);
        body.m_velocity=QVector3D(3*times[i]*times[i],0,0);body.m_acceleration=QVector3D(6*times[i],0,0);
        body.m_rotation=QQuaternion::fromAxisAndAngle(QVector3D(0,0,1),170+20*i);
        frame.m_bodyFrames[0]=body;mMotionJointFrame joint;joint.m_reactionTorque=QVector3D(0,0,123.25f);
        frame.m_jointFrames[23]=joint;trace.m_frames.push_back(frame);
    }
    request.m_variable=CurveVariable::Rotation;samples=Query::getResultSeries(&trace,request);
    require(samples.isValid()&&std::abs(samples.values[1]-190)<1e-4&&std::abs(samples.values[2]-210)<1e-4,
        "Original rotation curve jumped at +/-180 degrees");
    request.m_sourceType=CurveSourceType::Joint;request.m_sourceId=23;request.m_variable=CurveVariable::ReactionTorque;
    samples=Query::getResultSeries(&trace,request);
    require(samples.isValid()&&samples.values.last()==123.25,"Original reaction torque is not preserved in N mm");
    mMotionDistanceSensor distance(92001,SensorType::Distance);
    SensorPointRef a,b;a.bodyId=-1;b.bodyId=0;distance.setPointA(a);distance.setPointB(b);
    distance.setVariable(DistanceVariable::RelVelocity);
    samples=Query::getSensorSeries(&trace,&distance,CurveComponent::Magnitude);
    require(samples.isValid()&&samples.values.front()==0&&std::abs(samples.values.last()-3*.09*.09)<1e-7,
        "Distance velocity was differenced from display frames instead of actual point velocity");
    distance.setVariable(DistanceVariable::RelAcceleration);
    samples=Query::getSensorSeries(&trace,&distance,CurveComponent::Magnitude);
    require(samples.isValid()&&samples.values.front()==0&&std::abs(samples.values.last()-.54)<1e-6,
        "Distance acceleration was differenced from display frames instead of actual point acceleration");
    mMotionBodySensor rotation(92002,SensorType::BodyDisplacement);rotation.setVariable(BodySensorVariable::Rotation);
    mMotionRefMap::set(Physical_MotionSensor,rotation.getId(),Physical_MotionBody,MapRelation_None,0);
    samples=Query::getSensorSeries(&trace,&rotation,CurveComponent::Z);
    mMotionRefMap::set(Physical_MotionSensor,rotation.getId(),Physical_MotionBody,MapRelation_None,-1);
    require(samples.isValid()&&std::abs(samples.values.last()-210)<1e-4,"Body rotation sensor did not unwrap consistently with the body curve");
    mMotionResultData angleTrace;mMotionFrame angleFrame;angleFrame.m_time=0;
    mMotionBodyFrame base,rotor;base.m_rotation=QQuaternion::fromAxisAndAngle(QVector3D(1,0,0),90);
    rotor.m_rotation=QQuaternion::fromAxisAndAngle(QVector3D(0,0,1),45)*base.m_rotation;
    angleFrame.m_bodyFrames[0]=rotor;angleFrame.m_bodyFrames[37]=base;angleTrace.m_frames.push_back(angleFrame);
    mMotionAngleSensor angle(92003,SensorType::Angle);angle.setAxisDirection(QVector3D(0,0,1));
    mMotionRefMap::set(Physical_MotionSensor,angle.getId(),Physical_MotionBody,MapRelation_MotionBodyA,0);
    mMotionRefMap::set(Physical_MotionSensor,angle.getId(),Physical_MotionBody,MapRelation_MotionBodyB,37);
    samples=Query::getSensorSeries(&angleTrace,&angle,CurveComponent::Magnitude);
    bool ok=false;const double single=Query::sensorValueAtFrame(&angle,angleFrame,CurveComponent::Magnitude,ok);
    mMotionRefMap::set(Physical_MotionSensor,angle.getId(),Physical_MotionBody,MapRelation_MotionBodyA,-1);
    mMotionRefMap::set(Physical_MotionSensor,angle.getId(),Physical_MotionBody,MapRelation_MotionBodyB,-1);
    require(samples.isValid()&&std::abs(samples.values[0]-45)<1e-4&&ok&&std::abs(single-45)<1e-4,
        "Angle sensor mixed a local relative quaternion with a world-space axis");
    const float missing=std::numeric_limits<float>::quiet_NaN();
    trace.m_frames[1].m_bodyFrames.at(0).m_acceleration=QVector3D(missing,missing,missing);
    request.m_sourceType=CurveSourceType::Body;request.m_sourceId=0;request.m_variable=CurveVariable::Acceleration;
    require(!Query::getResultSeries(&trace,request).isValid()&&
        !Query::getSensorSeries(&trace,&distance,CurveComponent::Magnitude).isValid(),
        "Unavailable result channels became zero samples or a partial curve");
    request.m_variable=CurveVariable::Position;trace.m_frames[1].m_bodyFrames.erase(0);
    require(!Query::getResultSeries(&trace,request).isValid(),"An absent accepted body frame was silently skipped");
    require(!Query::locateTime(&accepted,std::numeric_limits<double>::quiet_NaN()).isValid(),"NaN query time was silently clamped");
}
}
int runMotionOccOriginalGuiSmoke(QApplication& app,const QString& directory) {
    QDir output(directory);QDir().mkpath(output.absolutePath());QFile report(output.filePath("original-gui-smoke.txt"));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;QTextStream log(&report);
    try {
        QString cadPath,jsonPath;const auto source=fixture(output,cadPath,jsonPath);QString savedProject;
        {
            MotionWindow window;window.resize(1440,960);window.show();
            auto* view=window.findChild<OccViewport*>();require(view && waitFor([&] { return view->glReady(); }),"Real OpenGL viewport did not initialize");
            require(window.importCad(cadPath),"STEP import failed");require(window.importMotionJson(jsonPath,false),"Original motion JSON did not bind to imported CAD");
            auto* tree=window.findChild<QTreeWidget*>("mTreeView");require(tree,"Missing original model tree");
            select(tree,ModelNode::Native,23,"joints");auto* joint=active<mMotionJointPanel>(window);
            require(!joint->findChildren<mPointPickerWidget*>().isEmpty(),"Original point picker was replaced");
            require(!joint->findChildren<mDirectionSelectorWidget*>().isEmpty(),"Original direction selector was replaced");
            require(joint->findChild<QPushButton*>("native_pick_origin")==nullptr,"Generic pick button remains in original joint panel");
            context()->flush();require(context()->document().entity("joints",23)==source.entity("joints",23),"Opening original joint panel changed unedited doubles/defaults/extensions");
            int icons=0;for(QTreeWidgetItemIterator it(tree);*it;++it) { require(!(*it)->icon(0).pixmap(16,16).isNull(),"Missing original tree icon: "+(*it)->text(0));++icons; }
            QSet<QString> jointActions;for(auto* button:window.findChildren<QAbstractButton*>())if(button->property("mxOriginalAction").toString()=="createMotionJoint")jointActions.insert(button->property("mxOriginalArgument").toString());
            for(auto* action:window.findChildren<QAction*>())if(action->property("mxOriginalAction").toString()=="createMotionJoint")jointActions.insert(action->property("mxOriginalArgument").toString());
            require(jointActions.size()>=17,"Original joint menu entries were collapsed or omitted");
            auto* tabs=window.findChild<QTabBar*>("MenuTabBar");require(tabs,"Original ribbon tab bar missing");
            for(int i=0;i<tabs->count();++i)if(tabs->tabText(i)==QStringLiteral("运动副与约束"))tabs->setCurrentIndex(i);
            QApplication::processEvents();
            auto* editorScroll=window.findChild<QScrollArea*>("originalMotionScroll_joints");
            require(editorScroll && editorScroll->horizontalScrollBar()->maximum()==0,"Original composite picker row is clipped by the parameter dock");
            snapshot(window,output,"restored-joint-panel.png");log<<"PASS original joint controls, "<<icons<<" tree icons and "<<jointActions.size()<<" individual joint actions\n";log.flush();
            select(tree,ModelNode::Native,11,"markers");auto* marker=active<mMotionMarkerPanel>(window);auto* point=marker->findChild<mPointPickerWidget*>();
            require(point,"Original marker point control missing");enter(point->findChild<QLineEdit*>(),"31.25, 2.5, -1.75");context()->flush();
            require(context()->document().entity("markers",11).value("localPosition").toObject()==xyz(31.25,2.5,-1.75),"Original marker editing was not persisted");
            require(context()->document().entity("bodies",0)==source.entity("bodies",0),"Editing marker changed unrelated body precision or source geometry IDs");
            require(context()->document().root.value("vendorExtension")==source.root.value("vendorExtension"),"Unknown root extension was lost");
            RigidModel model;model.nativeDocument=std::make_shared<NativeMotionDocument>(context()->document());syncNativeMotionModel(model);
            savedProject=output.filePath("restored-edit.mxmotion");QString error;
            require(saveRigidProject(savedProject,context()->picks()->document(),model,error),error);
            log<<"PASS original point input -> typed manager -> exact JSON delta -> saved project\n";log.flush();
            select(tree,ModelNode::Scenario,1);auto* scenario=active<mMotionScenarioPanel>(window);snapshot(window,output,"restored-scenario-panel.png");
            // Invoke the same public entry used by the original solve button;
            // suppress only completion dialogs as the original platform mode does.
            qApp->setProperty("mpcomm_solve_requested",true);scenario->onStartSolve(1);
            require(waitFor([&] { const auto* data=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1);return data && data->hasResult(); },60000),"Original scenario did not receive real engine results");
            const auto* result=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1)->getResult();
            require(result->m_frames.size()==11,"Original result adapter changed the accepted output frame count");
            require(std::abs(result->m_frames.back().m_time-.1)<1e-9,"Original result timeline has wrong end time");
            const auto drive=result->m_frames.back().m_driveFrames.at(9);
            require(std::abs(drive.m_imposedSpeed-90)<1e-5,"Original drive result units are not degrees/second");
            require(std::abs(drive.m_imposedValue-9)<.05,"Original drive result does not correspond to engine time");
            queryRegression(*result);
            log<<"PASS original result queries: accepted frames / deg and Nmm units / continuous angles / actual distance derivatives / unavailable channels\n";log.flush();
            log<<"PASS STEP + original JSON -> original scenario -> MotionEngine: 11 accepted frames, 90 deg/s, 9 deg at 0.1 s\n";log.flush();
            select(tree,ModelNode::Animation,1);auto* animation=active<mMotionAnimationPanel>(window);
            auto* timeline=animation->findChild<QSlider*>();require(timeline && timeline->maximum()==10,"Original animation did not receive all accepted frames");
            timeline->setValue(10);QApplication::processEvents();
            const auto* animatedResult=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1)->getResult();
            const auto mapped=animatedResult->m_frames.back().m_bodyTransforms.at(0).map(QVector3D(30,0,0));
            const int rotorPart=context()->document().bodyPartBindings.value(0).first();
            const auto expected=view->projectPoint({mapped.x(),mapped.y(),mapped.z()});
            const auto shown=view->projectPoint({30,0,0},rotorPart);
            require(QLineF(expected,shown).length()<.02,"Original animation slider and actual CAD pose differ");
            snapshot(window,output,"restored-animation-panel.png");timeline->setValue(0);
            QPushButton *play=nullptr,*pause=nullptr;
            for(auto* button:animation->findChildren<QPushButton*>()) { if(button->text()=="播放")play=button;if(button->text()=="暂停")pause=button; }
            require(play&&pause,"Original playback controls missing");play->click();
            require(waitFor([&] { return timeline->value()>0; },2000),"Original animation timer did not advance");pause->click();
            select(tree,ModelNode::Curves,1);auto* curves=active<mMotionCurvePanel>(window);
            auto* stableResult=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1)->getResult();
            const auto combos=curves->findChildren<QComboBox*>();require(combos.size()==3,"Original curve configuration fields changed");
            combos[0]->setCurrentIndex(combos[0]->findData(int(CurveSourceType::Drive)));
            combos[1]->setCurrentIndex(combos[1]->findData(9));
            int curveVariable=-1;for(int i=0;i<combos[2]->count();++i)if(combos[2]->itemData(i).value<QPair<int,int>>()
                ==qMakePair(int(CurveVariable::ImposedValue),int(CurveComponent::Magnitude)))curveVariable=i;
            require(curveVariable>=0,"Original drive curve variable missing");combos[2]->setCurrentIndex(curveVariable);
            QPushButton* add=nullptr;for(auto* button:curves->findChildren<QPushButton*>())if(button->text()=="添加曲线")add=button;
            require(add,"Original add-curve action missing");add->click();QApplication::processEvents();context()->flush();
            require(context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1)->getResult()==stableResult,
                "Editing a curve display request invalidated or replaced the physics result");
            auto* plot=mCurveWindow::getInstance()->getPlotView();require(plot->getCurveNames().size()==1,"Original curve did not reach the real Qwt view");
            const auto points=plot->getCurveSamples(plot->getCurveNames().first());require(points.size()==11
                &&std::abs(points.last().x()-.1)<1e-9&&std::abs(points.last().y()-9)<.05,"Original plotted samples differ from accepted engine frames or angular units");
            require(context()->document().entity("scenarios",1).value("curveRequests").toArray().size()==1,"Original curve request was not saved");
            QCheckBox* visible=nullptr;
            for(auto* check:curves->findChildren<QCheckBox*>())if(check->text()==QStringLiteral("显示"))visible=check;
            require(visible,"Rebuilt original curve visibility control missing");visible->setChecked(false);
            require(waitFor([&] { return !context()->document().entity("scenarios",1).value("curveRequests").toArray().first().toObject().value("visible").toBool(true); }),
                "Rebuilt curve visibility did not publish a model change without manual flush");
            visible->setChecked(true);
            require(waitFor([&] { return context()->document().entity("scenarios",1).value("curveRequests").toArray().first().toObject().value("visible").toBool(false); }),
                "Restoring curve visibility did not immediately persist");
            require(context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(1)->getResult()==stableResult,
                "Rebuilt row visibility invalidated the accepted physics result");
            snapshot(window,output,"restored-curve-panel.png");
            log<<"PASS original animation slider/timer -> actual CAD pose; original curve controls -> exact accepted samples; display edits preserve results\n";log.flush();
            // Original topology selectors store global IDs; persisted references
            // must retain the exact second-part subshape rather than its ordinal
            // in the first part, or a displayed triangle index.
            window.dispatchOriginalMotionAction("createMotionJoint","PointOnCurve");joint=active<mMotionJointPanel>(window);
            require(mCurveWindow::getInstance()->getPlotView()->getCurveNames().isEmpty(),
                "Changing the physical model left stale result curves in the original curve window");
            QComboBox* jointTypes=nullptr;
            for(auto* combo:joint->findChildren<QComboBox*>())if(combo->findText(QStringLiteral("点在线上副"))>=0)jointTypes=combo;
            require(jointTypes&&jointTypes->currentData().toInt()==int(JointType::PointOnCurve),
                "New original joint was not loaded into its type-specific edit controls");
            mPickWidgetController* collector=nullptr;
            for(auto* c:joint->findChildren<mPickWidgetController*>())if(c->widget()->isVisible() && c->allowedTypes()==QSet<PickDataType>{PickDataType::GeoEdge})collector=c;
            require(collector,"Original point-on-curve collector missing");
            require(collector->context()==context()->picks(),"Original curve collector is bound to a different model context");
            const int part=context()->picks()->document().parts().back().id;PickResult edge;edge.kind=PickKind::Edge;edge.partId=part;edge.index=1;
            const auto legacy=context()->picks()->toLegacy(edge,PickDataType::GeoEdge);require(legacy.second.result_id>1,"Global edge ID was replaced with part-local index");
            collector->replacePickedDataAndClearSets({{PickDataType::GeoEdge,{legacy.second}}});context()->flush();
            bool stored=false;for(const auto& value:context()->document().collection("joints")) {
                const auto row=value.toObject();if(row.value("jointType").toString()!="PointOnCurve")continue;
                const auto refs=row.value("occCurveEdges").toArray();if(refs.isEmpty())continue;
                const auto ref=refs.first().toObject();stored=ref.value("partId").toInt()==part && ref.value("index").toInt()==1 && !ref.value("geometry").toObject().isEmpty();
            }
            require(stored,"Original edge selector did not persist the real OCC topology identity");
            window.dispatchOriginalMotionAction("createMotionJoint","PointOnSurface");joint=active<mMotionJointPanel>(window);collector=nullptr;
            jointTypes=nullptr;
            for(auto* combo:joint->findChildren<QComboBox*>())if(combo->findText(QStringLiteral("点在面上副"))>=0)jointTypes=combo;
            require(jointTypes&&jointTypes->currentData().toInt()==int(JointType::PointOnSurface),
                "New original surface joint was not loaded into its edit controls");
            for(auto* c:joint->findChildren<mPickWidgetController*>())if(c->widget()->isVisible() && c->allowedTypes()==QSet<PickDataType>{PickDataType::GeoFace})collector=c;
            require(collector,"Original point-on-surface collector missing");PickResult face;face.kind=PickKind::Face;face.partId=part;face.index=1;
            const auto faceLegacy=context()->picks()->toLegacy(face,PickDataType::GeoFace);collector->replacePickedDataAndClearSets({{PickDataType::GeoFace,{faceLegacy.second}}});context()->flush();
            stored=false;for(const auto& value:context()->document().collection("joints")) { const auto row=value.toObject();const auto refs=row.value("occSurfaceFaces").toArray();if(!refs.isEmpty())stored=refs.first().toObject().value("partId").toInt()==part; }
            require(stored,"Original face selector did not persist real OCC face identity");snapshot(window,output,"restored-surface-picker.png");
            log<<"PASS original curve/face collectors, global topology IDs, exact OCC fingerprints\n";log.flush();
            // All restored original panel entry points must construct live
            // widgets; keep the original incomplete templates distinguishable.
            const QStringList entries{"createMotionBodyBatch","createMotionBody","createMotionCoupling","createMotionSpring","createMotionVectorForce",
                "createMotionContact","createMotionSensor","createMotionController","createControlInput","createMotionAutoFlex",
                "createElectroImport","createElectroExport","createHydraulic","createRLPanel"};
            for(const auto& action:entries) {
                window.dispatchOriginalMotionAction(action,{});QApplication::processEvents();
                bool visible=false;for(auto* widget:window.findChildren<QWidget*>())if(widget->objectName().startsWith("originalMotionEditor_") && widget->isVisible())visible=true;
                require(visible,"Original panel failed to open: "+action);
            }
            log<<"PASS restored original panel construction and repeated controller teardown\n";log.flush();
        }
        {
            MotionWindow reopened;reopened.resize(1440,960);reopened.show();require(reopened.openRigidProject(savedProject),"Saved original-GUI edit did not reopen");
            auto* tree=reopened.findChild<QTreeWidget*>("mTreeView");select(tree,ModelNode::Native,11,"markers");
            auto* panel=active<mMotionMarkerPanel>(reopened);const auto point=panel->findChild<mPointPickerWidget*>()->coordinate();
            require((point-QVector3D(31.25,2.5,-1.75)).length()<1e-6,"Original point value changed after project reopen");
            require(context()->document().entity("joints",23)==source.entity("joints",23),"Unedited topology-axis precision changed on reopen");
            snapshot(reopened,output,"restored-reopened-marker.png");log<<"PASS project reopen, original composite point control, untouched precision/extensions\n";
        }
        extern int runMotionOccOriginalAutoFlexSmoke(QApplication&,const QString&);
        require(runMotionOccOriginalAutoFlexSmoke(app,output.filePath("autoflex"))==0,"Original AutoFlex workflow failed; see autoflex/original-autoflex-smoke.txt");
        extern int runMotionOccOriginalTreeSmoke(QApplication&,const QString&);
        require(runMotionOccOriginalTreeSmoke(app,output.filePath("tree"))==0,"Original tree workflow failed; see tree/original-tree-smoke.txt");
        log<<"PASS original GUI desktop regression including original AutoFlex workflow\n";return 0;
    } catch(const std::exception& error) { log<<"FAIL: "<<error.what()<<'\n';return 1; }
}
