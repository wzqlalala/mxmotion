#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"
#include "NativeMotionLoadExport.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/mViewportModelManager.h"
#include "mMotionLoadExportPanel.h"
#include "mMotionResultData.h"
#include "mMotionScenarioPanel.h"
#include "mPickWidgetController.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <stdexcept>

using namespace MotionOcc;
using namespace MechMainWindow;
using namespace MechMotionData;
using namespace MxFunctions;

namespace {
constexpr int BodyId=0,JointId=23,ScenarioId=11;
const QString BodyName=QStringLiteral("original load export rotor");
const QString JointName=QStringLiteral("original load export hinge");

void require(bool condition,const QString& message) {
    if(!condition)throw std::runtime_error(message.toStdString());
}
void expectNear(double value,double expected,double tolerance,const QString& message) {
    require(std::isfinite(value)&&std::abs(value-expected)<=tolerance,
        QStringLiteral("%1: %2 != %3").arg(message).arg(value,0,'g',16).arg(expected,0,'g',16));
}
bool waitFor(const std::function<bool()>& predicate,int timeout=5000) {
    if(predicate())return true;
    QEventLoop loop;QTimer poll;
    QObject::connect(&poll,&QTimer::timeout,&loop,[&] { if(predicate())loop.quit(); });
    poll.start(15);QTimer::singleShot(timeout,&loop,&QEventLoop::quit);loop.exec();return predicate();
}
QJsonObject xyz(double x,double y,double z) { return {{"x",x},{"y",y},{"z",z}}; }
QByteArray bytes(const QString& path) {
    QFile file(path);require(file.open(QIODevice::ReadOnly),QStringLiteral("Cannot read %1: %2").arg(path,file.errorString()));
    return file.readAll();
}
LegacyMotionContext* context() {
    require(getCVM&&getCVM->getModelData()&&getCVM->getModelData()->context,"Original model context is missing");
    return getCVM->getModelData()->context;
}
QTreeWidgetItem* node(QTreeWidget* tree,ModelNode::Kind kind,int id) {
    for(QTreeWidgetItemIterator it(tree);*it;++it)
        if((*it)->data(0,Qt::UserRole+1).toInt()==int(kind)&&(*it)->data(0,Qt::UserRole).toInt()==id)return *it;
    throw std::runtime_error(QStringLiteral("Missing original tree node %1/%2").arg(int(kind)).arg(id).toStdString());
}
void select(QTreeWidget* tree,ModelNode::Kind kind,int id) {
    auto* item=node(tree,kind,id);require(!item->isDisabled(),"Requested original tree result is disabled");
    tree->setCurrentItem(item);QApplication::processEvents();
}
template<class T>T* textWidget(QWidget* parent,const QString& label) {
    for(auto* item:parent->findChildren<T*>())if(item->text()==label)return item;
    throw std::runtime_error((QStringLiteral("Missing original control: ")+label).toStdString());
}
QComboBox* comboItems(QWidget* parent,const QStringList& labels) {
    for(auto* combo:parent->findChildren<QComboBox*>()) {
        QStringList actual;for(int i=0;i<combo->count();++i)actual.append(combo->itemText(i));
        if(actual==labels)return combo;
    }
    throw std::runtime_error((QStringLiteral("Missing original combo: ")+labels.join('/')).toStdString());
}
QComboBox* comboEntry(QWidget* parent,const QString& label,int id) {
    for(auto* combo:parent->findChildren<QComboBox*>())
        for(int i=0;i<combo->count();++i)
            if(combo->itemText(i)==label&&combo->itemData(i).isValid()&&combo->itemData(i).toInt()==id)return combo;
    throw std::runtime_error((QStringLiteral("Missing original object selector: ")+label).toStdString());
}
QLineEdit* placeholder(QWidget* parent,const QString& label) {
    for(auto* edit:parent->findChildren<QLineEdit*>())if(edit->placeholderText()==label)return edit;
    throw std::runtime_error((QStringLiteral("Missing original input: ")+label).toStdString());
}

// Drive the real Qt file chooser and dismiss actual completion messages. A
// validation failure is captured, not silently treated as a successful export.
// This timer also dismisses unexpected solver warnings so CI cannot hang in a
// nested modal event loop before its result timeout is reached.
class DialogDriver {
public:
    DialogDriver() {
        QObject::connect(&timer_,&QTimer::timeout,&timer_,[this] {
            const bool expired=!path_.isEmpty()&&elapsed_.elapsed()>15000;
            for(auto* top:QApplication::topLevelWidgets()) {
                if(!top->isVisible())continue;
                if(auto* file=qobject_cast<QFileDialog*>(top)) {
                    if(path_.isEmpty()||expired) {
                        failures_.append(expired?QStringLiteral("Save dialog timed out"):QStringLiteral("Unexpected save dialog"));
                        file->reject();continue;
                    }
                    file->setOption(QFileDialog::DontConfirmOverwrite,true);
                    file->selectFile(path_);
                    if(auto* edit=file->findChild<QLineEdit*>("fileNameEdit"))edit->setText(path_);
                    chooserSeen_=true;QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);
                } else if(auto* message=qobject_cast<QMessageBox*>(top)) {
                    if(message->icon()==QMessageBox::Warning||message->icon()==QMessageBox::Critical)
                        failures_.append(message->text());
                    else ++completionMessages_;
                    message->accept();
                }
            }
        });
        timer_.start(20);
    }
    void save(QPushButton* button,const QString& path) {
        require(button&&button->isEnabled(),"Original export button is disabled");
        check();path_=path;chooserSeen_=false;elapsed_.restart();
        const int previousMessages=completionMessages_;
        button->click();path_.clear();QApplication::processEvents();
        check();require(chooserSeen_,"Original export did not open its Qt file chooser");
        require(completionMessages_>previousMessages,"Original export did not report completion");
        require(QFile::exists(path),"Original export did not create its selected file");
    }
    void check() const { require(failures_.isEmpty(),QStringLiteral("Unexpected original GUI message: ")+failures_.join(" | ")); }
private:
    QTimer timer_;QElapsedTimer elapsed_;QString path_;QStringList failures_;
    bool chooserSeen_=false;int completionMessages_=0;
};

QVector<QStringList> csvRows(const QString& path) {
    QVector<QStringList> rows;
    for(auto line:QString::fromUtf8(bytes(path)).split('\n')) {
        if(line.endsWith('\r'))line.chop(1);if(line.isEmpty())continue;
        QStringList fields;QString field;bool quoted=false;
        for(int i=0;i<line.size();++i) {
            const auto c=line[i];
            if(c=='"') {
                if(quoted&&i+1<line.size()&&line[i+1]=='"') { field+='"';++i; }
                else quoted=!quoted;
            } else if(c==','&&!quoted) { fields.append(field);field.clear(); }
            else field+=c;
        }
        require(!quoted,"CSV has an unterminated quoted field");fields.append(field);rows.append(fields);
    }
    return rows;
}
double number(const QStringList& row,int column) {
    require(column<row.size(),"CSV numeric column is missing");bool ok=false;const double value=row[column].toDouble(&ok);
    require(ok&&std::isfinite(value),"CSV contains a missing or nonfinite numeric value");return value;
}
const QStringList& csvHeader() {
    static const QStringList header{"LoadCase","SourceType","SourceName","FrameIndex","Time_s","NodeID","End",
        "PointX_mm","PointY_mm","PointZ_mm","Fx_N","Fy_N","Fz_N","Mx_Nmm","My_Nmm","Mz_Nmm"};
    return header;
}
void compareCsv(const QString& actualPath,const QString& referencePath,const QVector<double>& times) {
    const auto actual=csvRows(actualPath),reference=csvRows(referencePath);
    require(actual.size()==times.size()+1&&actual.size()==reference.size(),"CSV row count differs from accepted solver frames");
    require(actual.front()==csvHeader()&&actual.front()==reference.front(),"CSV omitted actual N / Nmm / seconds / mm unit headers");
    for(int frame=0;frame<times.size();++frame) {
        const auto& row=actual[frame+1];const auto& expected=reference[frame+1];
        require(row.size()==16&&row.size()==expected.size(),"CSV field count changed");
        for(int field:{0,1,2,3,5,6})require(row[field]==expected[field],"CSV source, end, name, frame or target-node mapping changed");
        expectNear(number(row,4),times[frame],1e-13,"CSV time is not the accepted sample time");
        for(int field=7;field<16;++field)
            expectNear(number(row,field),number(expected,field),1e-10*std::max(1.0,std::abs(number(expected,field))),"Original GUI load differs from native result export");
    }
}

NativeMotionDocument fixture(CadDocument& cad,int& part) {
    QString error;require(cad.addPrimitive(Primitive::Box,40,10,10,part,error),error);
    NativeMotionDocument document;document.bodyPartBindings.insert(BodyId,{part});
    const QJsonObject body{{"id",BodyId},{"name",BodyName},{"geoEntityIds",QJsonArray{part}},
        {"isFixed",false},{"mass",1.0},{"isCustomMassProps",true},{"centerOfMass",xyz(20,5,5)},
        {"inertia",QJsonArray{200.0/12,1700.0/12,1700.0/12,0,0,0}},{"inertiaConvention","tensor"}};
    const QJsonObject joint{{"id",JointId},{"name",JointName},{"jointType","Revolute"},
        {"actionBodyId",BodyId},{"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(0,0,1)}};
    const QJsonObject drive{{"id",9},{"name","original load export motor"},{"driveType","JointDrive"},
        {"targetType","Joint"},{"targetJointId",JointId},{"motionType","Rotation"},{"controlMode","Velocity"},{"expression","360"}};
    const QJsonObject scenario{{"id",ScenarioId},{"name","original load export history"},{"endTime",1.0},{"stepSize",.01},
        {"gravity",xyz(0,0,-9810)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},
        {"solverType","SPARSE_QR"},{"solverTolerance",1e-10}};
    document.root={{"formatVersion",1},{"motionData",QJsonObject{{"bodies",QJsonArray{body}},{"joints",QJsonArray{joint}},
        {"drives",QJsonArray{drive}},{"scenarios",QJsonArray{scenario}}}}};return document;
}
QVector<double> solveOriginal(MotionWindow& window,QTreeWidget* tree,DialogDriver& dialogs,int expectedFrames) {
    select(tree,ModelNode::Scenario,ScenarioId);
    mMotionScenarioPanel* original=nullptr;
    for(auto* panel:window.findChildren<mMotionScenarioPanel*>())if(panel->isVisible())original=panel;
    require(original&&original->isVisible(),"Solve did not use the original scenario panel");
    qApp->setProperty("mpcomm_solve_requested",true);
    require(window.command(Command::Solve)->isEnabled(),"Original Solve command is disabled");
    window.command(Command::Solve)->trigger();
    require(waitFor([&] {
        if(!getCVM||!getCVM->getModelData())return false;
        const auto* scenario=getCVM->getModelData()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(ScenarioId);
        return scenario&&scenario->hasResult()&&!scenario->getResult()->m_frames.empty()
            &&!window.command(Command::StopSolve)->isEnabled();
    },60000),"Original queued solver did not publish accepted load frames");
    dialogs.check();
    const auto* result=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(ScenarioId)->getResult();
    require(result->getFrameCount()==expectedFrames,"Original load test received an unexpected accepted frame count");
    QVector<double> times;for(const auto& frame:result->m_frames)times.append(frame.m_time);return times;
}
mMotionLoadExportPanel* openExport(MotionWindow& window,QTreeWidget* tree) {
    select(tree,ModelNode::Export,ScenarioId);
    auto* panel=window.findChild<mMotionLoadExportPanel*>("originalMotionLoadExportPanel");
    require(panel&&panel->isVisible(),"Result Export did not open the visible original panel");return panel;
}
struct PickedNode { int id=-1,part=-1;Vector3d point{}; };
PickedNode pickTarget(MotionWindow& window,QWidget* manual) {
    auto* view=window.findChild<OccViewport*>();require(view,"Missing real viewport");
    auto* edit=placeholder(manual,"3091");auto* pick=textWidget<QPushButton>(manual,QStringLiteral("拾取"));
    pick->click();require(waitFor([&] { return view->isPicking(); }),"Original node button did not activate real FEM picking");
    auto* mesh=context()->model()->getMeshData();const int part=context()->document().bodyPartBindings.value(BodyId).value(0,-1);
    require(mesh->meshes().count(part)&&mesh->nodeIds().count(part),"Real tetrahedral mesh / stable node IDs are missing");
    const auto& actual=mesh->meshes().at(part);const auto& ids=mesh->nodeIds().at(part);
    require(actual.nodes.size()>8&&actual.tetrahedra.size()>5,"Load target mesh is not an actual Netgen mesh");
    for(std::size_t index=0;index<actual.nodes.size();++index) {
        if(actual.nodes[index][0]<25)continue; // A nonzero, observable moving lever arm.
        const auto screen=view->projectPoint(actual.nodes[index],part);const auto hit=view->pickAt(screen);
        if(hit.kind!=PickKind::Node||hit.partId!=part||hit.nodeId!=ids[int(index)])continue;
        QMouseEvent down(QEvent::MouseButtonPress,screen,view->mapToGlobal(screen.toPoint()),Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
        QMouseEvent up(QEvent::MouseButtonRelease,screen,view->mapToGlobal(screen.toPoint()),Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&down);QApplication::sendEvent(view,&up);QApplication::processEvents();
        require(waitFor([&] { return !view->isPicking(); }),"Original load node pick did not end after one accepted node");
        require(edit->text().toInt()==ids[int(index)]&&edit->property("mxPickedNodeId").toInt()==ids[int(index)]
            &&edit->property("mxPickedNodePartId").toInt()==part,"Original node field lost the actual global node / part ID");
        const auto point=edit->property("mxPickedNodePosition").toList();require(point.size()==3,"Original target did not retain the actual picked point");
        for(int component=0;component<3;++component)expectNear(point[component].toDouble(),actual.nodes[index][component],1e-6,"Original target point changed");
        return {ids[int(index)],part,actual.nodes[index]};
    }
    throw std::runtime_error("No actual exposed target mesh node was picked through the original button");
}
void snapshot(MotionWindow& window,const QDir& output,const QString& name) {
    QApplication::processEvents();require(window.grab().save(output.filePath(name)),"Cannot capture original load export panel");
}

struct ExportCall { int count=0,id=-1;NativeLoadSource source;NativeLoadExportOptions options; };
auto observeExports(const std::shared_ptr<ExportCall>& calls) {
    const auto real=getCVM->exportMotionLoads;require(bool(real),"Original export bridge is not connected");
    getCVM->exportMotionLoads=[real,calls](int id,const QString& path,const NativeLoadSource& source,
                                       const NativeLoadExportOptions& options,QString& error) {
        ++calls->count;calls->id=id;calls->source=source;calls->options=options;
        return real(id,path,source,options,error); // The actual accepted engine run always writes the output.
    };
    return real;
}
void checkMappedOptions(const ExportCall& call,const NativeLoadExportOptions& expected) {
    require(call.count>0&&call.id==ScenarioId&&call.source.collection=="joints"&&call.source.id==JointId,
        "Original source/scenario selectors did not map to the actual joint result");
    const auto& value=call.options;
    require(value.name==expected.name&&value.end==expected.end&&value.frames==expected.frames&&value.format==expected.format
        &&value.frameIndex==expected.frameIndex&&value.nodeId==expected.nodeId&&value.scale==expected.scale
        &&value.components==expected.components&&value.useTargetPoint==expected.useTargetPoint&&value.targetBodyId==expected.targetBodyId,
        "Original load controls did not map their actual values to the backend");
    for(int component=0;component<3;++component)expectNear(value.targetPointMm[component],expected.targetPointMm[component],1e-6,"Picked target was not passed to load export");
}
void verifyWrenchTransfer(const QString& targetPath,const QString& markerPath) {
    const auto target=csvRows(targetPath),marker=csvRows(markerPath);require(target.size()==marker.size(),"Marker/target histories differ");
    double movement=0,momentChange=0;
    for(int frame=1;frame<target.size();++frame) {
        Vector3d r{},force{};for(int c=0;c<3;++c) {
            r[c]=number(marker[frame],7+c)-number(target[frame],7+c);force[c]=number(marker[frame],10+c);
            expectNear(number(target[frame],10+c),force[c],1e-10,"Moving target changed the force vector");
            movement=std::max(movement,std::abs(number(target[frame],7+c)-number(target[1],7+c)));
        }
        const Vector3d cross{r[1]*force[2]-r[2]*force[1],r[2]*force[0]-r[0]*force[2],r[0]*force[1]-r[1]*force[0]};
        for(int c=0;c<3;++c) {
            const double expected=number(marker[frame],13+c)+cross[c];
            expectNear(number(target[frame],13+c),expected,1e-8*std::max(1.0,std::abs(expected)),"Target moment is not marker moment plus r cross F");
            momentChange=std::max(momentChange,std::abs(cross[c]));
        }
    }
    require(movement>10&&momentChange>10,"Fixture did not exercise a genuinely moving target and moment transfer");
}
void exportCasePage(MotionWindow& window,mMotionLoadExportPanel* panel,int pageIndex,const QDir& output,
                    const QVector<double>& times,DialogDriver& dialogs,QTextStream& log) {
    auto* mode=comboItems(panel,{QStringLiteral("手动提取"),QStringLiteral("特征工况导出"),QStringLiteral("连杆同步工况导出")});
    auto* stack=panel->findChild<QStackedWidget*>();require(stack&&stack->count()==3,"Original three load pages are incomplete");
    mode->setCurrentIndex(pageIndex);QApplication::processEvents();auto* page=stack->currentWidget();
    require(page&&page->isVisible()&&stack->currentIndex()==pageIndex,"Original export mode did not show its real page");
    auto* body=comboEntry(page,BodyName,BodyId);body->setCurrentIndex(body->findData(BodyId));
    bool jointChecked=false;for(auto* check:page->findChildren<QCheckBox*>())
        if(check->text().startsWith(JointName+" [")) { check->setChecked(true);jointChecked=true; }
    require(jointChecked&&!page->findChildren<mPickWidgetController*>().isEmpty(),"Original joint selection / geometry-pick row was omitted");
    auto* peaks=page->findChild<QSpinBox*>();auto* tolerance=page->findChild<QDoubleSpinBox*>();
    require(peaks&&tolerance,"Original case selection parameters are missing");peaks->setValue(2);tolerance->setValue(1e-6);
    textWidget<QCheckBox>(page,QStringLiteral("直接生成静力工况"))->setChecked(false);
    textWidget<QCheckBox>(page,QStringLiteral("同时导出工况 JSON"))->setChecked(true);
    const auto real=getCVM->buildMotionLoadCases;require(bool(real),"Original case builder is not connected to accepted loads");
    NativeLoadCaseOptions expected;expected.analysisBodyId=BodyId;expected.jointIds={JointId};expected.peaksPerSignal=2;
    expected.mergeTolerance=1e-6;expected.synchronized=pageIndex==2;
    QJsonObject reference;QString error;require(real(ScenarioId,expected,reference,error),error);
    const auto name=pageIndex==2?QStringLiteral("original-synchronized-cases.json"):QStringLiteral("original-feature-cases.json");
    const auto label=pageIndex==2?QStringLiteral("筛选并导出同步工况 JSON"):QStringLiteral("筛选并导出工况 JSON");
    dialogs.save(textWidget<QPushButton>(page,label),output.filePath(name));
    const auto actual=QJsonDocument::fromJson(bytes(output.filePath(name))).object();
    require(actual==reference,"Original case page changed selected IDs, peak count, time tolerance or synchronization semantics");
    const auto info=actual.value("info").toObject();
    require(info.value("format")=="MxMotionLoadCases"&&info.value("coordinateSystem")=="bodyInitial"
        &&info.value("unitSystem").toObject().value("moment")=="N*mm","Original case JSON lost physical units / reference frame");
    const auto cases=actual.value("cases").toArray();require(!cases.isEmpty(),"Actual dynamic load history produced no internal peak cases");
    for(const auto& item:cases) {
        const auto row=item.toObject();const int frame=row.value("frameIndex").toInt(-1);
        require(frame>0&&frame+1<times.size(),"Case selector invented an endpoint / unaccepted frame");
        expectNear(row.value("timeSeconds").toDouble(),times[frame],1e-13,"Case time is not an accepted solver frame");
        const auto loads=row.value("loads").toArray();require(loads.size()==1,"Original page omitted its selected joint load");
        const auto load=loads[0].toObject();require(load.value("jointId").toInt()==JointId&&load.value("bodyId").toInt()==BodyId&&load.value("end")=="I",
            "Case load references the wrong physical body end");
        expectNear(load.value("components").toObject().value("FZ").toDouble(),9.81,2e-5,"Case force lost N units / actual gravity support");
    }
    snapshot(window,output,pageIndex==2?"original-load-synchronized.png":"original-load-features.png");
    log<<"PASS original "<<(pageIndex==2?"synchronized":"feature")<<" page: "<<cases.size()<<" actual peak cases, selected joint/body, accepted times, initial-body axes and N/Nmm JSON\n";log.flush();
}
}

int runMotionOccOriginalLoadExportSmoke(QApplication&,const QString& directory) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QDir output(directory);QDir().mkpath(output.absolutePath());QFile report(output.filePath("original-load-export-smoke.txt"));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;QTextStream log(&report);
    try {
        DialogDriver dialogs;CadDocument cad;int part=-1;QString error;const auto document=fixture(cad,part);
        RigidModel model;model.nativeDocument=std::make_shared<NativeMotionDocument>(document);syncNativeMotionModel(model);
        const auto project=output.filePath("original-load-export.mxmotion");require(saveRigidProject(project,cad,model,error),error);
        MotionWindow window;window.resize(1440,960);window.show();
        auto* view=window.findChild<OccViewport*>();require(view&&waitFor([&] { return view->glReady(); }),"Real viewport did not initialize");
        require(window.openRigidProject(project),"Could not open real load-export fixture");
        auto* tree=window.findChild<QTreeWidget*>("mTreeView");require(tree,"Original model tree is missing");
        select(tree,ModelNode::Part,part);require(window.command(Command::GenerateMesh)->isEnabled(),"Real mesh command is disabled");
        window.command(Command::GenerateMesh)->trigger();
        auto* meshSize=window.findChild<QDoubleSpinBox*>("meshElementSize");auto* meshStart=window.findChild<QPushButton*>("generateMeshButton");
        require(meshSize&&meshStart&&meshStart->isVisible(),"Real Netgen mesh setup was not shown");meshSize->setValue(7);meshStart->click();
        require(waitFor([&] { return window.command(Command::RemoveMesh)->isEnabled(); },60000),"Actual Netgen mesh generation failed");
        dialogs.check();const auto times=solveOriginal(window,tree,dialogs,101);
        auto* panel=openExport(window,tree);auto* stack=panel->findChild<QStackedWidget*>();require(stack&&stack->count()==3,"Original load exporter lacks its three pages");
        auto* manual=stack->currentWidget();require(stack->currentIndex()==0,"Original manual export was not the default page");
        auto* sourceType=comboItems(manual,{QStringLiteral("运动副"),QStringLiteral("耦合副"),QStringLiteral("驱动器")});
        auto* sourceId=comboEntry(manual,JointName,JointId);
        require(sourceType->currentIndex()==0&&sourceId->currentData().toInt()==JointId
            &&textWidget<QRadioButton>(manual,"I Marker")->isChecked(),"Original default load source / I end changed");
        const auto picked=pickTarget(window,manual);
        const auto calls=std::make_shared<ExportCall>();const auto realExport=observeExports(calls);
        NativeLoadExportOptions expected;expected.name=QStringLiteral("original, load \"case\"");expected.frames=NativeLoadFrames::All;
        expected.nodeId=picked.id;expected.scale=1.25;expected.useTargetPoint=true;expected.targetBodyId=BodyId;expected.targetPointMm=picked.point;
        // editLoadExportPanel replaces the constructor placeholder with the
        // current scenario's original default load-case name.
        placeholder(manual,"original load export history_LoadCase")->setText(expected.name);
        textWidget<QRadioButton>(manual,QStringLiteral("全部帧"))->click();
        auto* scale=manual->findChild<QDoubleSpinBox*>();require(scale,"Original scale input is missing");scale->setValue(expected.scale);
        const auto dynamicPath=output.filePath("original-moving-target.csv");
        dialogs.save(textWidget<QPushButton>(manual,QStringLiteral("导出载荷文件")),dynamicPath);checkMappedOptions(*calls,expected);
        const auto referencePath=output.filePath("reference-moving-target.csv");
        require(realExport(ScenarioId,referencePath,{"joints",JointId},expected,error),error);compareCsv(dynamicPath,referencePath,times);
        auto markerOptions=expected;markerOptions.useTargetPoint=false;markerOptions.targetBodyId=-1;markerOptions.targetPointMm={};
        const auto markerPath=output.filePath("reference-marker.csv");require(realExport(ScenarioId,markerPath,{"joints",JointId},markerOptions,error),error);
        verifyWrenchTransfer(dynamicPath,markerPath);
        expectNear(number(csvRows(dynamicPath).back(),12),9.81*expected.scale,2e-5,"Manual export force scale / N units are incorrect");
        snapshot(window,output,"original-load-manual.png");
        log<<"PASS actual project -> Netgen -> original queued Scenario solve -> original Result Export -> Qt file chooser CSV\n";
        log<<"PASS "<<times.size()<<" accepted frames, original source/end/components/scale/name, real global node "<<picked.id
           <<", moving body pose and M_target = M_marker + (p_marker - p_target) x F\n";log.flush();
        exportCasePage(window,panel,1,output,times,dialogs,log);exportCasePage(window,panel,2,output,times,dialogs,log);

        // Reuse this geometry and actual mesh, importing the same mechanism as
        // a static equilibrium. Removing the imposed angular speed leaves an
        // unloaded yaw DOF; the real hinge supports the eccentric gravity load.
        NativeMotionDocument staticDocument=document;auto data=staticDocument.root.value("motionData").toObject();
        data["drives"]=QJsonArray{};auto scenario=data.value("scenarios").toArray().first().toObject();
        scenario["analysisType"]="Static";scenario["name"]="original single equilibrium load";data["scenarios"]=QJsonArray{scenario};
        staticDocument.root["motionData"]=data;
        const auto staticJson=output.filePath("original-static-load.motion.json");require(writeNativeMotionDocument(staticJson,staticDocument,error),error);
        require(window.importMotionJson(staticJson,false),"Static motion JSON did not import on the existing real CAD / mesh");
        const auto staticTimes=solveOriginal(window,tree,dialogs,1);panel=openExport(window,tree);stack=panel->findChild<QStackedWidget*>();manual=stack->currentWidget();
        auto* frame=manual->findChild<QSpinBox*>();require(frame&&frame->minimum()==0&&frame->maximum()==0,"Static frame selector does not represent accepted frame zero");
        require(textWidget<QPushButton>(manual,QStringLiteral("导出载荷文件"))->isEnabled()
            &&textWidget<QPushButton>(stack->widget(1),QStringLiteral("筛选并导出工况 JSON"))->isEnabled()
            &&textWidget<QPushButton>(stack->widget(2),QStringLiteral("筛选并导出同步工况 JSON"))->isEnabled(),
            "A real single-frame static result is still disabled in the original exporter");
        const auto staticNode=pickTarget(window,manual);const auto staticCalls=std::make_shared<ExportCall>();const auto staticExport=observeExports(staticCalls);
        NativeLoadExportOptions staticOptions;staticOptions.name="original equilibrium";staticOptions.frames=NativeLoadFrames::Specified;
        staticOptions.nodeId=staticNode.id;staticOptions.useTargetPoint=true;staticOptions.targetBodyId=BodyId;staticOptions.targetPointMm=staticNode.point;
        placeholder(manual,"original single equilibrium load_LoadCase")->setText(staticOptions.name);
        textWidget<QRadioButton>(manual,QStringLiteral("指定帧"))->click();
        const auto staticCsv=output.filePath("original-single-frame.csv"),staticReference=output.filePath("reference-single-frame.csv");
        dialogs.save(textWidget<QPushButton>(manual,QStringLiteral("导出载荷文件")),staticCsv);checkMappedOptions(*staticCalls,staticOptions);
        require(staticExport(ScenarioId,staticReference,{"joints",JointId},staticOptions,error),error);compareCsv(staticCsv,staticReference,staticTimes);
        const auto equilibrium=csvRows(staticCsv).at(1);
        expectNear(number(equilibrium,10),0,1e-7,"Static support Fx");expectNear(number(equilibrium,11),0,1e-7,"Static support Fy");
        expectNear(number(equilibrium,12),9.81,1e-6,"Static support does not balance 1 kg in N");
        expectNear(number(equilibrium,13),(5-staticNode.point[1])*9.81,1e-5,"Static picked-node Mx does not balance eccentric gravity in Nmm");
        expectNear(number(equilibrium,14),(staticNode.point[0]-20)*9.81,1e-5,"Static picked-node My does not balance eccentric gravity in Nmm");
        expectNear(number(equilibrium,15),0,1e-7,"Static support Mz");
        comboItems(manual,{QStringLiteral("CSV (通用)"),QStringLiteral("Nastran BDF (载荷卡片)")})->setCurrentIndex(1);
        staticOptions.format=NativeLoadFormat::NastranBdf;const auto bdf=output.filePath("original-single-frame.bdf"),bdfReference=output.filePath("reference-single-frame.bdf");
        dialogs.save(textWidget<QPushButton>(manual,QStringLiteral("导出载荷文件")),bdf);checkMappedOptions(*staticCalls,staticOptions);
        require(staticExport(ScenarioId,bdfReference,{"joints",JointId},staticOptions,error),error);
        require(bytes(bdf)==bytes(bdfReference)&&bytes(bdf).contains("FORCE,1,"+QByteArray::number(staticNode.id)+",0,1.0,")
            &&bytes(bdf).contains("MOMENT,1,"+QByteArray::number(staticNode.id)+",0,1.0,"),"Original single-frame BDF lost its actual node, frame SID or physical load");
        snapshot(window,output,"original-load-static-bdf.png");dialogs.check();
        log<<"PASS same CAD / Netgen mesh + original Static JSON import / solve: one accepted equilibrium frame; original CSV and BDF buttons export real N/Nmm support and picked-node moment\n";
        log<<"PASS all three visible original load-export pages, actual engine bridge and styled save dialogs\n";log.flush();return 0;
    } catch(const std::exception& exception) { log<<"FAIL: "<<exception.what()<<'\n';log.flush();return 1; }
}
