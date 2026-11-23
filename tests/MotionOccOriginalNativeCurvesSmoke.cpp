#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/model/LegacyMotionJsonAdapter.h"
#include "compat/mViewportModelManager.h"
#include "mMotionScenarioPanel.h"
#include "mMotionCurvePanel.h"
#include "mMotionResultData.h"
#include "mCurveWindow.h"
#include "mPlotView.h"
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace MotionOcc;
using namespace MechMainWindow;
using namespace MechMotionData;
using namespace MxFunctions;
namespace {
void require(bool condition,const QString& message) { if(!condition)throw std::runtime_error(message.toStdString()); }
bool waitFor(const std::function<bool()>& predicate,int milliseconds=5000) {
    if(predicate())return true;QEventLoop loop;QTimer poll;poll.start(10);
    QObject::connect(&poll,&QTimer::timeout,&loop,[&] { if(predicate())loop.quit(); });
    QTimer::singleShot(milliseconds,&loop,&QEventLoop::quit);loop.exec();return predicate();
}
LegacyMotionContext* context() {
    require(getCVM && getCVM->getModelData()->context,"Original motion context is not active");
    return getCVM->getModelData()->context;
}
void select(MotionWindow& window,ModelNode::Kind kind,int id) {
    auto* tree=window.findChild<QTreeWidget*>("mTreeView");require(tree,"Original model tree is absent");
    for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole+1).toInt()==int(kind)
        && (*it)->data(0,Qt::UserRole).toInt()==id) { tree->setCurrentItem(*it);QApplication::processEvents();return; }
    throw std::runtime_error("Original result tree entry is absent");
}
template<class T> T* active(MotionWindow& window) {
    for(auto* panel:window.findChildren<T*>())if(panel->isVisible())return panel;
    throw std::runtime_error("Expected original panel is not visible");
}
QJsonArray requests() { return context()->document().entity("scenarios",0).value("curveRequests").toArray(); }
QJsonObject request(int id) {
    for(const auto& value:requests())if(value.toObject().value("requestId").toInt(-1)==id)return value.toObject();
    return {};
}
void solve(MotionWindow& window) {
    select(window,ModelNode::Scenario,0);auto* panel=active<mMotionScenarioPanel>(window);
    qApp->setProperty("mpcomm_solve_requested",true);panel->onStartSolve(0);
    require(waitFor([] { const auto* scenario=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(0);
        return scenario && scenario->hasResult(); },60000),"Original scenario did not receive native-forces accepted results");
    const auto* result=context()->model()->getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(0)->getResult();
    require(result->getFrameCount()==101 && std::abs(result->m_frames.back().m_time-1)<1e-10,
        "Native force result timeline changed while adapting the original GUI");
}
void verifyPlot() {
    auto* plot=mCurveWindow::getInstance()->getPlotView();int visible=0;
    for(const auto& value:requests()) {
        const auto row=value.toObject();if(!row.value("visible").toBool(true))continue;++visible;
        const auto name=row.value("requestName").toString();
        require(plot->getCurveNames().contains(name),"Saved native curve disappeared from the original plot: "+name);
        QVector<double> times,values;QString error;
        require(getCVM->queryMotionCurve && getCVM->queryMotionCurve(0,row.value("sourceType").toString(),row.value("sourceId").toInt(),
            row.value("variable").toString(),row.value("component").toInt(),times,values,error),error);
        const auto actual=plot->getCurveSamples(name);
        require(actual.size()==101 && actual.size()==values.size(),"Original Qwt plot omitted accepted native samples");
        for(int i=0;i<actual.size();++i)require(std::abs(actual[i].x()-times[i])<1e-12 &&
            std::abs(actual[i].y()-values[i])<1e-10*std::max(1.,std::abs(values[i])),"Original plot changed native channel values/units");
        const QColor expected(row.value("color").toString());
        require(!expected.isValid() || plot->getCurveSettings(name).lineColor==expected,"Native curve color was replaced on import");
    }
    require(plot->getCurveNames().size()==visible,"Original plot contains stale or collapsed curve requests");
}
QPushButton* button(QWidget* parent,const QString& label) {
    for(auto* item:parent->findChildren<QPushButton*>())if(item->text()==label)return item;
    throw std::runtime_error("Original curve action button is absent");
}
void addChannel(mMotionCurvePanel* panel,CurveSourceType type,const QString& variable,const QString& name) {
    const auto combos=panel->findChildren<QComboBox*>();require(combos.size()==3,"Original three-combo curve layout changed");
    const int source=combos[0]->findData(int(type));require(source>=0,"Native source is missing from the original selector");
    combos[0]->setCurrentIndex(source);const int object=combos[1]->findData(1);
    require(object>=0,"Recorded native force object is missing from the original selector");combos[1]->setCurrentIndex(object);
    int variableIndex=-1;
    for(int i=0;i<combos[2]->count();++i) {
        const auto metadata=combos[2]->itemData(i,Qt::UserRole+1).toJsonObject();
        if(metadata.value("variable").toString()==variable && metadata.value("component").toInt(-2)==0)variableIndex=i;
    }
    require(variableIndex>=0,"Recorded native variable is missing from the original selector: "+variable);
    combos[2]->setCurrentIndex(variableIndex);
    const auto edits=panel->findChildren<QLineEdit*>();require(!edits.isEmpty(),"Original curve-name field is absent");
    edits.first()->setText(name);const int before=requests().size();button(panel,QStringLiteral("添加曲线"))->click();
    require(waitFor([&] { return requests().size()==before+1; }),"Original Add Curve did not persist the native channel");
    const auto added=requests().last().toObject();
    require(added.value("sourceType").toString()==mMotionCurveRequest::collectionForSource(type)
        && added.value("variable").toString()==variable && added.value("component").isDouble()
        && added.value("component").toInt()==0,"Native curve was narrowed to an old enum during Add Curve");
}
void codecRegression() {
    const QJsonObject extra{{"fraction",1.2345678901234567},{"futureOption",QJsonArray{1,2,"retained"}}};
    QJsonObject old{{"requestId",73},{"requestName","original schema"},{"sourceType","Drive"},{"sourceId",9},
        {"variable","ImposedValue"},{"component","Magnitude"},{"color",QJsonObject{{"r",1},{"g",2},{"b",3},{"a",255}}},{"visible",true},{"futureCurve",extra}};
    QJsonObject modal{{"id",74},{"name","mode 8"},{"sourceType","autoFlexBodies"},{"sourceId",4},
        {"variable","ModalCoordinates"},{"component",7},{"color","#1280A0"},{"visible",false},{"futureCurve",extra}};
    // Exercise the existing friend adapter via a complete isolated model;
    // do not widen the original codecs' private API for a test.
    MechModelData::mModelData model;
    LegacyMotionJsonAdapter::load(QJsonObject{{"scenarios",QJsonArray{QJsonObject{{"id",0},{"name","codec"},
        {"curveRequests",QJsonArray{old,modal}}}}}},model);
    const auto* scenario=model.getManager<mMotionScenarioManager>(Physical_MotionScenario).getData(0);
    require(scenario && scenario->getCurveRequests().size()==2,"Full original JSON import lost curve requests");
    const auto encodedRequests=LegacyMotionJsonAdapter::save(model).value("scenarios").toArray().first().toObject().value("curveRequests").toArray();
    auto parsed=scenario->getCurveRequests()[0];auto encoded=encodedRequests[0].toObject();
    require(!parsed.isNativeChannel(),"Original request schema unexpectedly became a native-only request");
    for(const auto* key:{"sourceType","variable","component","color","futureCurve"})require(encoded.value(key)==old.value(key),"Original request schema or extras did not round-trip");
    parsed=scenario->getCurveRequests()[1];encoded=encodedRequests[1].toObject();
    require(parsed.isNativeChannel() && parsed.m_nativeComponent==7 && !encoded.contains("requestId") && !encoded.contains("requestName"),
        "High-dimensional native component or id/name schema was narrowed");
    for(const auto* key:{"id","name","sourceType","variable","component","color","futureCurve","visible"})
        require(encoded.value(key)==modal.value(key),"Native request schema or extras did not round-trip");
}
}

int runMotionOccOriginalNativeCurvesSmoke(QApplication& app,const QString& directory) {
    Q_UNUSED(app);QDir output(directory);QDir().mkpath(output.absolutePath());
    QFile report(output.filePath("original-native-curves-smoke.txt"));if(!report.open(QIODevice::WriteOnly|QIODevice::Text))return 2;
    QTextStream log(&report);
    try {
        const auto sourcePath=QDir(QFileInfo(QString::fromUtf8(__FILE__)).absolutePath()).absoluteFilePath("../examples/native-forces.mxmotion");
        QFile source(sourcePath);require(source.open(QIODevice::ReadOnly),"Cannot open examples/native-forces.mxmotion");
        auto project=QJsonDocument::fromJson(source.readAll()).object();auto model=project.value("model").toObject();
        auto native=model.value("nativeMotion").toObject();auto document=native.value("document").toObject();
        auto data=document.value("motionData").toObject();auto scenarios=data.value("scenarios").toArray();auto scenario=scenarios.first().toObject();
        auto originalRequests=scenario.value("curveRequests").toArray();
        require(originalRequests.size()==4,"Native force fixture no longer contains its four saved requests");
        auto first=originalRequests[0].toObject();first["futureCurve"]=QJsonObject{{"precision",1.2345678901234567},{"retain",true}};originalRequests[0]=first;
        scenario["curveRequests"]=originalRequests;auto selected=originalRequests[2].toObject();selected["selectionExtension"]="retained";
        scenario["currentCurveRequest"]=selected;scenarios[0]=scenario;data["scenarios"]=scenarios;document["motionData"]=data;
        native["document"]=document;model["nativeMotion"]=native;project["model"]=model;
        const auto fixturePath=output.filePath("native-forces-with-extensions.mxmotion");
        QFile fixture(fixturePath);require(fixture.open(QIODevice::WriteOnly),"Cannot write native curve fixture");fixture.write(QJsonDocument(project).toJson());fixture.close();
        const auto savedPath=output.filePath("original-native-curves-roundtrip.mxmotion");QJsonArray expectedRequests;
        {
            MotionWindow window;window.resize(1440,960);window.show();auto* viewport=window.findChild<OccViewport*>();
            require(viewport && waitFor([&] { return viewport->glReady(); }),"Real viewport failed to initialize");
            require(window.openRigidProject(fixturePath),"Native force project did not load");select(window,ModelNode::Scenario,0);codecRegression();solve(window);
            select(window,ModelNode::Curves,0);auto* panel=active<mMotionCurvePanel>(window);
            auto* list=panel->findChild<QListWidget*>();require(list && list->count()==4,"Original curve list lost saved native requests");
            const auto combos=panel->findChildren<QComboBox*>();require(combos.size()==3,"Original curve form was replaced");
            require(combos[0]->currentData().toInt()==int(CurveSourceType::SoftStop)
                && combos[2]->currentData(Qt::UserRole+1).toJsonObject().value("variable").toString()=="Engaged",
                "Opening original curve form overwrote the saved current request");
            require(context()->document().entity("scenarios",0).value("currentCurveRequest")==selected,
                "Applying a saved curve selection modified its schema or extras");
            verifyPlot();auto* plot=mCurveWindow::getInstance()->getPlotView();
            const auto drive=plot->getCurveSamples(originalRequests[0].toObject().value("requestName").toString());
            require(std::abs(drive.last().y()-90)<.05,"Saved drive curve did not retain degrees");
            const auto cable=plot->getCurveSamples(originalRequests[1].toObject().value("requestName").toString());
            require(std::any_of(cable.begin(),cable.end(),[](const QPointF& point) { return point.y()>0; }),"Native cable load was replaced by zero");
            const auto stop=plot->getCurveSamples(originalRequests[2].toObject().value("requestName").toString());
            require(std::any_of(stop.begin(),stop.end(),[](const QPointF& point) { return point.y()==1; })
                &&std::any_of(stop.begin(),stop.end(),[](const QPointF& point) { return point.y()==0; }),"Native stop engagement history was lost");
            log<<"PASS native-forces real solve: 101 accepted frames and all four saved requests through original controls/Qwt\n";log.flush();

            addChannel(panel,CurveSourceType::SoftStop,"Coordinate",QStringLiteral("止挡真实坐标"));
            addChannel(panel,CurveSourceType::TensionCable,"SignedForce",QStringLiteral("绳索真实拉力"));verifyPlot();
            list=panel->findChild<QListWidget*>();QWidget* row=list->itemWidget(list->item(0));auto* visible=row->findChild<QCheckBox*>();
            require(visible,"Original native visibility control is absent");visible->setChecked(false);
            require(waitFor([] { return !request(1).value("visible").toBool(true); }),"Native visibility failed to persist");verifyPlot();
            visible->setChecked(true);require(waitFor([] { return request(1).value("visible").toBool(); }),"Native visibility restore failed");
            QPushButton* color=nullptr;for(auto* candidate:row->findChildren<QPushButton*>())if(candidate->text().isEmpty())color=candidate;
            require(color,"Original curve color button is absent");
            const bool noNative=QApplication::testAttribute(Qt::AA_DontUseNativeDialogs);QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,true);
            QTimer choose;choose.setInterval(20);QObject::connect(&choose,&QTimer::timeout,[&] {
                if(auto* dialog=qobject_cast<QColorDialog*>(QApplication::activeModalWidget())) { dialog->setCurrentColor(QColor("#7d3c98"));dialog->accept();choose.stop(); }
            });choose.start();color->click();choose.stop();QApplication::setAttribute(Qt::AA_DontUseNativeDialogs,noNative);
            require(waitFor([] { return request(1).value("color").toString()=="#7d3c98"; }),"Native string color did not persist through the original color control");
            list=panel->findChild<QListWidget*>();row=list->itemWidget(list->item(list->count()-1));button(row,QStringLiteral("删除"))->click();
            require(waitFor([] { return requests().size()==5; }),"Original Delete Curve did not remove a native request");
            require(!request(2).isEmpty(),"Deleting added cable curve also removed the original cable request");
            addChannel(panel,CurveSourceType::TensionCable,"SignedForce",QStringLiteral("绳索真实拉力"));verifyPlot();
            for(int i=0;i<4;++i) {
                const auto old=originalRequests[i].toObject(),current=request(old.value("requestId").toInt());
                for(const auto* key:{"sourceType","sourceId","variable","component","requestName","futureCurve"})
                    require(current.value(key)==old.value(key),"Editing native curve display corrupted its channel identity or extensions");
            }
            QVector<double> missingTimes,missingValues;QString error;
            require(!getCVM->queryMotionCurve(0,"softStops",1,"UnrecordedChannel",0,missingTimes,missingValues,error)
                &&!error.isEmpty()&&missingValues.isEmpty(),"Unavailable native channel was replaced with zero samples");
            context()->flush();expectedRequests=requests();RigidModel saved;saved.nativeDocument=std::make_shared<NativeMotionDocument>(context()->document());syncNativeMotionModel(saved);
            require(saveRigidProject(savedPath,context()->picks()->document(),saved,error),error);
            require(window.grab().save(output.filePath("original-native-force-curves.png")),"Cannot capture original native force curves");
            log<<"PASS original add/visibility/color/delete controls, exact channel identities and extension-preserving project save\n";log.flush();
        }
        {
            MotionWindow window;window.resize(1440,960);window.show();require(window.openRigidProject(savedPath),"Native curve round-trip project did not reopen");
            select(window,ModelNode::Scenario,0);require(requests()==expectedRequests,"Saved native requests changed on reopen");
            solve(window);select(window,ModelNode::Curves,0);require(active<mMotionCurvePanel>(window)->findChildren<QComboBox*>().size()==3,"Reopened curve form changed layout");verifyPlot();
            require(requests()==expectedRequests,"Reopening and plotting native requests rewrote their schema");
            log<<"PASS project reopen -> original solve/panel/Qwt with all six native requests preserved\n";
        }
        return 0;
    } catch(const std::exception& exception) { log<<"FAIL: "<<exception.what()<<'\n';return 1; }
}
