#include "MotionWindow.h"
#include "OccViewport.h"
#include "NativeMotionSolver.h"
#include "RigidProject.h"

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QPointer>
#include <QPushButton>
#include <QScreen>
#include <QSlider>
#include <QTableWidget>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
using namespace MotionOcc;
void require(bool yes,const QString& why){if(!yes)throw std::runtime_error(why.toStdString());}
bool waitFor(const std::function<bool()>& test,int timeout=5000){
    if(test())return true;QEventLoop loop;QTimer poll;
    QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(test())loop.quit();});poll.start(20);
    QTimer::singleShot(timeout,&loop,&QEventLoop::quit);loop.exec();return test();
}
template<class T>T* widget(QWidget& parent,const char* name){auto* result=parent.findChild<T*>(name);require(result,QStringLiteral("Missing %1").arg(name));return result;}
QJsonObject xyz(double x,double y,double z){return {{"x",x},{"y",y},{"z",z}};}
}

int runMotionOccModalDesktopSmoke(QApplication& app,const QString& outputDirectory)
{
    QDir output(outputDirectory);QDir().mkpath(output.absolutePath());
    QFile report(output.filePath("modal-desktop-smoke.txt"));if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;QTextStream log(&report);
    try{
        CadDocument cad;int part=0;QString error;require(cad.addPrimitive(Primitive::Box,10,10,10,part,error),error);
        auto document=std::make_shared<NativeMotionDocument>();document->bodyPartBindings.insert(0,{part});
        const QJsonObject body{{"id",0},{"name",QStringLiteral("模态检查试件")},{"geoEntityIds",QJsonArray{part}},
            {"isCustomMassProps",true},{"mass",1},{"centerOfMass",xyz(5,5,5)},{"inertia",QJsonArray{17,17,17,0,0,0}}};
        const QJsonObject mesh{{"nodes",QJsonArray{QJsonArray{0,0,0},QJsonArray{10,0,0},QJsonArray{10,10,0},QJsonArray{0,10,0},
            QJsonArray{0,0,10},QJsonArray{10,0,10},QJsonArray{10,10,10},QJsonArray{0,10,10}}},
            {"tetrahedra",QJsonArray{QJsonArray{0,1,3,4},QJsonArray{1,2,3,6},QJsonArray{1,3,4,6},QJsonArray{1,4,5,6},QJsonArray{3,4,6,7}}}};
        const QJsonObject flex{{"id",0},{"name",QStringLiteral("真实夹持模态")},{"targetBodyId",0},{"enabled",true},{"modeCount",4},
            {"dampingType",0},{"material",QJsonObject{{"youngModulus",100},{"poissonRatio",.3},{"density",.001}}},
            {"mesh",mesh},{"attachGroups",QJsonArray{QJsonArray{0,3,4,7}}}};
        QJsonObject data;
        data.insert("bodies",QJsonArray{body});data.insert("autoFlexBodies",QJsonArray{flex});
        data.insert("scenarios",QJsonArray{QJsonObject{{"id",0},{"name",QStringLiteral("未运行的动力学方案")},{"endTime",1},{"stepSize",.01},{"gravity",xyz(0,0,0)}}});
        document->root={{"formatVersion",1},{"motionData",data}};
        RigidModel model;model.nativeDocument=document;syncNativeMotionModel(model);
        const auto project=output.filePath("modal-fixture.mxmotion");require(saveRigidProject(project,cad,model,error),error);
        MotionWindow window;window.resize(1350,930);window.show();
        require(window.openRigidProject(project),"could not open modal project");
        auto* tree=widget<QTreeWidget>(window,"mTreeView");QTreeWidgetItem* flexNode=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toInt()==int(ModelNode::Native)&&(*it)->data(0,Qt::UserRole+2).toString()=="autoFlexBodies")flexNode=*it;
        require(flexNode,"missing flexible-body tree item");tree->setCurrentItem(flexNode);
        auto* inspect=widget<QPushButton>(window,"nativeInspectModes");require(inspect->isEnabled(),"saved model inspection disabled");
        inspect->click();QPointer<QDialog> dialog;
        require(waitFor([&]{dialog=window.findChild<QDialog*>("nativeModeInspector");return !dialog.isNull();},60000),"modal analysis did not open inspector");
        auto* frequencies=widget<QTableWidget>(*dialog,"nativeModeFrequencies");
        require(frequencies->rowCount()==4,"GUI frequency count differs from solver request");
        auto* view=widget<OccViewport>(*dialog,"nativeModeViewport");require(waitFor([&]{return view->glReady();},10000),"modal viewport failed to initialize");
        require(widget<QLabel>(*dialog,"nativeModeDescription")->text().contains(QStringLiteral("固定接口")),"mode boundary conditions not described");
        auto* patches=widget<QTableWidget>(*dialog,"nativeModePatches");require(patches->rowCount()==1&&patches->item(0,1)->text()=="4","actual attachment nodes missing");
        auto* phase=widget<QSlider>(*dialog,"nativeModePhase");phase->setValue(1000);app.processEvents();const auto plus=view->grabFramebuffer();
        phase->setValue(-1000);app.processEvents();const auto minus=view->grabFramebuffer();require(!plus.isNull()&&plus!=minus,"signed phase did not change rendered real mode");
        frequencies->selectRow(1);phase->setValue(1000);app.processEvents();require(view->grabFramebuffer()!=plus,"mode selection did not change geometry/colors");
        auto* showPatches=widget<QCheckBox>(*dialog,"nativeModeAttachments");showPatches->setChecked(false);app.processEvents();const auto without=view->grabFramebuffer();
        showPatches->setChecked(true);app.processEvents();require(view->grabFramebuffer()!=without,"actual attachment overlay not rendered");
        const auto withLegend=view->grabFramebuffer();const double dpr=view->devicePixelRatioF();
        const int barX=qRound((view->width()-156)*dpr),barHeight=std::min(220,view->height()-100);
        const auto high=withLegend.pixelColor(barX,qRound((78+barHeight*.1)*dpr));
        const auto low=withLegend.pixelColor(barX,qRound((78+barHeight*.9)*dpr));
        require(high.red()>high.blue()*2&&low.blue()>low.red()*2,"normalized modal color legend missing or obscured");
        auto* play=widget<QPushButton>(*dialog,"nativeModePlay");play->click();
        require(waitFor([&]{return phase->value()<900;},5000),"mode animation timer did not advance");play->click();phase->setValue(1000);
        dialog->repaint();app.processEvents();
        require(dialog->screen()->grabWindow(dialog->winId()).save(output.filePath("native-modal-inspector.png")),"failed to save modal screenshot");
        const auto csvPath=output.filePath("real-modes.csv");bool picked=false;QTimer chooser;chooser.setInterval(20);
        QObject::connect(&chooser,&QTimer::timeout,&window,[&]{
            for(auto* top:QApplication::topLevelWidgets())if(auto* file=qobject_cast<QFileDialog*>(top))if(file->isVisible()){
                file->setOption(QFileDialog::DontConfirmOverwrite,true);
                file->selectFile(csvPath);if(auto* name=file->findChild<QLineEdit*>("fileNameEdit"))name->setText(csvPath);
                picked=true;QMetaObject::invokeMethod(file,"accept",Qt::DirectConnection);chooser.stop();return;
            }
        });chooser.start();QTimer::singleShot(5000,&chooser,[&]{chooser.stop();for(auto* top:QApplication::topLevelWidgets())if(auto* file=qobject_cast<QFileDialog*>(top))file->reject();});
        widget<QPushButton>(*dialog,"nativeModeExport")->click();require(picked,"mode CSV chooser was not exercised");
        QFile csv(csvPath);require(csv.open(QIODevice::ReadOnly)&&csv.readAll().contains("phi_x_raw"),"mode CSV did not preserve true basis values");
        log<<"Saved CAD + native JSON -> standalone real modal solve -> 4 frequencies, clamp node map, mode/phase/overlay/playback and CSV passed\n";
        dialog->close();require(waitFor([&]{return dialog.isNull();}),"modal inspector did not release its snapshot");
        // The modal-only run must not masquerade as a transient scenario result.
        for(QTreeWidgetItemIterator it(tree);*it;++it)
            if((*it)->data(0,Qt::UserRole+1).toInt()==int(ModelNode::Animation))
                require((*it)->isDisabled(),"modal inspection enabled a transient animation result");
        log<<"Modal inspection left model and transient results unchanged\n";
        // Exercise the new physical path through the real window as well:
        // two independent end patches, no interior fixed-interface modes,
        // static load, one converged frame and the actual nodal contour.
        auto independent=std::make_shared<NativeMotionDocument>(*document);
        auto staticData=independent->root.value("motionData").toObject();
        auto beam=flex;beam["interfaceMode"]="Independent";
        beam["attachGroups"]=QJsonArray{QJsonArray{0,3,4,7},QJsonArray{1,2,5,6}};
        staticData["autoFlexBodies"]=QJsonArray{beam};
        staticData["joints"]=QJsonArray{
            QJsonObject{{"id",0},{"name",QStringLiteral("左端夹持")},{"jointType","Fixed"},{"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(0,5,5)},{"axis",xyz(1,0,0)}},
            QJsonObject{{"id",1},{"name",QStringLiteral("右端独立接口")},{"jointType","Translational"},{"actionBodyId",0},{"baseBodyId",-1},{"origin",xyz(10,5,5)},{"axis",xyz(1,0,0)}}};
        staticData["vectorForces"]=QJsonArray{QJsonObject{{"id",0},{"name",QStringLiteral("右端拉力")},{"enabled",true},{"loadType","Force"},
            {"bodyId",0},{"actionPoint",xyz(10,5,5)},{"direction",xyz(1,0,0)},{"directionMode","GlobalFixed"},{"magnitudeMode","Constant"},{"magnitude",100}}};
        auto staticScenario=staticData.value("scenarios").toArray()[0].toObject();staticScenario["analysisType"]="Static";staticScenario["name"]=QStringLiteral("独立接口静力平衡");
        staticData["scenarios"]=QJsonArray{staticScenario};independent->root["motionData"]=staticData;
        RigidModel staticModel;staticModel.nativeDocument=independent;syncNativeMotionModel(staticModel);
        const auto staticProject=output.filePath("independent-static.mxmotion");require(saveRigidProject(staticProject,cad,staticModel,error),error);
        require(window.openRigidProject(staticProject),"could not reopen independent FEM project");
        require(window.command(Command::Solve)->isEnabled(),"static solve command unavailable");window.command(Command::Solve)->trigger();
        require(waitFor([&]{return !window.command(Command::StopSolve)->isEnabled();},60000),"independent static solve did not finish");
        QTreeWidgetItem* animation=nullptr;
        for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toInt()==int(ModelNode::Animation))animation=*it;
        require(animation && !animation->isDisabled(),"static solve produced no reviewable equilibrium frame");tree->setCurrentItem(animation);
        auto* contour=widget<QComboBox>(window,"motionFlexContour");
        require(!widget<QPushButton>(window,"motionPlayButton")->isEnabled(),"single static frame should not fabricate time playback");
        auto* mainView=window.findChild<OccViewport*>();require(mainView,"main viewport missing");
        contour->setCurrentIndex(0);app.processEvents();const auto plain=mainView->grabFramebuffer();
        contour->setCurrentIndex(contour->findData(int(FlexContourKind::ElasticDisplacement)));app.processEvents();
        require(mainView->grabFramebuffer()!=plain,"independent FEM static contour did not render");
        require(!widget<QPushButton>(window,"motionInspectModes")->isEnabled(),"zero internal basis invented modal inspection modes");
        window.repaint();app.processEvents();
        require(window.screen()->grabWindow(window.winId()).save(output.filePath("native-independent-static.png")),"failed to save full FEM static screenshot");
        log<<"Independent full FEM -> real static solve -> single-frame deformed mesh / elastic contour passed, no fake internal modes\n";
        window.close();log<<"PASS\n";log.flush();return 0;
    }catch(const std::exception& exception){log<<"FAIL: "<<exception.what()<<'\n';log.flush();return 1;}
}
