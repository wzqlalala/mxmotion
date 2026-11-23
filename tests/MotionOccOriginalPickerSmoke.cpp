#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "NativeMotionSymbols.h"
#include "NativeMotionPickWidgets.h"
#include "picking/LegacyPickContext.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"
#include "mGeneralPickUp.h"
#include "mCalcLineEdit.h"
#include "mParamBarStyle.h"
#include "PickCommonFunction.h"
#include <BRep_Builder.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_CompSolid.hxx>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResource>
#include <QPushButton>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
using namespace MotionOcc;
using namespace MechMainWindow;
using Type=MxFunctions::PickDataType;
void require(bool ok,const QString& message) { if(!ok)throw std::runtime_error(message.toStdString()); }
bool waitFor(const std::function<bool()>& test,int timeout=10000)
{
    if(test())return true;
    QEventLoop loop;QTimer timer;
    QObject::connect(&timer,&QTimer::timeout,&loop,[&]{if(test())loop.quit();});
    timer.start(15);QTimer::singleShot(timeout,&loop,&QEventLoop::quit);loop.exec();return test();
}
void click(QWidget& widget,const QPointF& position,Qt::MouseButton button=Qt::LeftButton)
{
    QMouseEvent press(QEvent::MouseButtonPress,position,widget.mapToGlobal(position.toPoint()),button,button,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,position,widget.mapToGlobal(position.toPoint()),button,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&widget,&press);QApplication::sendEvent(&widget,&release);
    QApplication::processEvents();
}
void click(OccViewport& view,const std::array<double,3>& p,int part)
{ click(view,view.projectPoint(p,part)); }
bool expectNear(const QVector3D& a,const QVector3D& b,float tol=1e-5f){return (a-b).length()<tol;}
int changedPixels(const QImage& before,const QImage& after)
{
    require(before.size()==after.size(),"Feedback comparison image sizes differ");
    int changed=0;
    for(int y=0;y<before.height();++y)for(int x=0;x<before.width();++x) {
        const auto a=before.pixelColor(x,y),b=after.pixelColor(x,y);
        if(std::abs(a.red()-b.red())+std::abs(a.green()-b.green())+std::abs(a.blue()-b.blue())>18)++changed;
    }
    return changed;
}
QToolButton* button(QWidget* parent,const QString& text)
{
    for(auto* b:parent->findChildren<QToolButton*>())
        if(b->text()==text||b->toolTip()==text||b->property("roundTooltipText").toString()==text)return b;
    return nullptr;
}
}

int runMotionOccOriginalPickerSmoke(QApplication& app,const QString& outputDirectory)
{
    QDir output(outputDirectory);QDir().mkpath(output.absolutePath());
    QFile report(output.filePath("original-picker-smoke.txt"));
    if(!report.open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))return 2;
    QTextStream log(&report);
    try {
        if (!QFile::exists(QStringLiteral(":/mMainWidgets/Icon/pickIcon/clear.png")))
            QResource::registerResource(QCoreApplication::applicationDirPath()+QStringLiteral("/mMainWidgets.rcc"));
        CadDocument cad;QString error;int box=-1;
        require(cad.addPrimitive(Primitive::Box,40,30,20,box,error),error);
        TopoDS_CompSolid compound;BRep_Builder builder;builder.MakeCompSolid(compound);
        builder.Add(compound,BRepPrimAPI_MakeBox(gp_Pnt(60,0,0),20,20,20).Shape());
        builder.Add(compound,BRepPrimAPI_MakeBox(gp_Pnt(90,0,0),20,20,20).Shape());
        const auto brep=output.filePath("picker-two-solids.brep");
        require(BRepTools::Write(compound,brep.toUtf8().constData()),"Could not prepare real multi-solid BREP");
        std::vector<int> imported;require(cad.importFile(brep,imported,error),error);
        require(imported.size()==1,"BREP did not retain one part with two solids");
        const int solidPart=imported.front();
        FemMesh mesh;mesh.nodes={{{0,0,20}},{{40,0,20}},{{40,30,20}},{{0,30,20}}};
        mesh.surface={{{0,1,2}},{{0,2,3}}};std::map<int,FemMesh> meshes{{box,mesh}};

        QWidget window;window.setWindowTitle(QStringLiteral("原始拾取控件回归"));window.resize(1360,850);
        auto* layout=new QHBoxLayout(&window);
        auto* panel=new QWidget(&window);panel->setFixedWidth(430);
        auto* fields=new QVBoxLayout(panel);fields->setAlignment(Qt::AlignTop);
        applyParamBarStyle(panel);
        auto* view=new OccViewport(&window);layout->addWidget(panel);layout->addWidget(view,1);
        LegacyPickContext context(&window,view,cad,&meshes);
        {
            auto* detached=mGeneralPickUp::createPickWidget(CBBlue);
            QPointer<mPickWidgetController> lifetime(detached);
            fields->addWidget(detached->widget());
            require(panel->findChildren<mPickWidgetController*>().contains(detached)&&detached->context()==&context,
                    "Original parentless picker did not follow its field into the panel/context");
            delete detached->widget();require(lifetime.isNull(),"Original parentless picker controller outlived its field");
        }
        auto* point=new mPointPickerWidget(QStringLiteral("原点"),QStringLiteral("模型拾取"),panel);
        auto* direction=new mDirectionSelectorWidget(QStringLiteral("轴向"),false,QStringLiteral("大小"),false,{},false,panel,CBRed);
        auto* detailed=mGeneralPickUp::createPickWidget(CBBlue,panel);
        detailed->setDefultTip(QStringLiteral("请选择"),QStringLiteral("原多选拾取控件"));
        fields->addWidget(point);fields->addWidget(direction);fields->addWidget(detailed->widget());fields->addStretch();
        point->setCoordinate({1,2,3});
        QString graphicsError;
        QObject::connect(view,&OccViewport::graphicsError,&window,[&](const QString& e){graphicsError=e;});
        window.show();
        require(waitFor([&]{return view->glReady()||!graphicsError.isEmpty();})&&view->glReady(),graphicsError);
        view->setScene(cad,meshes);view->viewTop();view->fitAll();app.processEvents();
        log<<"Qt="<<qVersion()<<" DPR="<<view->devicePixelRatioF()<<" platform="<<QGuiApplication::platformName()<<'\n';

        require(!point->findChildren<QLineEdit*>().isEmpty(),"Original point editor is missing");
        auto* apply=button(point,QStringLiteral("应用"));require(apply,"Original point Apply control was replaced");
        require(point->pickController()->widget()->findChild<QLineEdit*>(),"Original read-only picker field was replaced");
        require(!point->findChild<QPushButton*>(),"Point picker was replaced by a single push button");
        point->pickController()->setAllowedTypes([]{return QSet<Type>{Type::GeoPoint};});
        auto* pointField=point->pickController()->widget()->findChild<QLineEdit*>();
        click(*pointField,pointField->rect().center());
        require(point->pickController()->isPickActive()&&view->isPicking(),"Original pick-field click did not activate viewport");
        click(*view,{0,0,20},box);
        require(!point->pickData().isEmpty(),"Real CAD vertex did not reach original point controller");
        require(expectNear(point->coordinate(),{1,2,3}),"Point pick bypassed original Apply workflow");
        apply->click();app.processEvents();
        require(expectNear(point->coordinate(),{0,0,20})&&point->pickData().isEmpty(),"Original Apply failed to transfer and clear pick");
        log<<"PASS original point field / Apply / coordinate / clear workflow\n";

        auto* modes=direction->findChild<QComboBox*>();require(modes,"Original direction modes missing");
        const QStringList expected{QStringLiteral("X轴"),QStringLiteral("Y轴"),QStringLiteral("Z轴"),QStringLiteral("起点-终点"),QStringLiteral("选择对象"),QStringLiteral("向量")};
        for(const auto& name:expected)require(modes->findText(name)>=0,"Original direction mode missing: "+name);
        direction->setDirectionMode(mDirectionSelectorWidget::XAxis);
        require(expectNear(direction->getDirection().value<QVector3D>(),{1,0,0}),"X axis mode incorrect");
        auto* reverse=button(direction,QStringLiteral("反向"));require(reverse&&!reverse->icon().isNull(),"Original reverse icon missing");
        reverse->click();require(expectNear(direction->getDirection().value<QVector3D>(),{-1,0,0}),"Original reverse action failed");
        direction->setReversed(false);direction->setDirectionMode(mDirectionSelectorWidget::SelectObject);app.processEvents();
        require(direction->objectPickController()->isPickActive()&&!point->pickController()->isPickActive(),"Original pick controllers are not mutually exclusive");
        mPickWidgetController::setActivePickType(Type::GeoEdge);
        click(*view,{20,0,20},box);
        const auto edgeDirection=direction->getDirection().value<QVector3D>();
        require(std::abs(edgeDirection.x())>.999f,"Original object direction did not resolve real edge tangent");
        const auto edgeState=direction->objectPickData();
        require(edgeState.pickedData.count(Type::GeoEdge)==1,"Original direction pick data has wrong type");
        reverse->click();require(QVector3D::dotProduct(edgeDirection,direction->getDirection().value<QVector3D>())<-.999f,"Geometry direction reverse failed");
        direction->setReversed(false);
        log<<"PASS original direction modes / reverse icon / CAD tangent / controller mutual exclusion\n";

        direction->setDirectionMode(mDirectionSelectorWidget::CustomCoordinate);app.processEvents();
        direction->startPickController()->setPickActive(true);mPickWidgetController::setActivePickType(Type::GeoPoint);
        click(*view,{0,0,20},box);
        QList<QToolButton*> applications;
        for(auto* b:direction->findChildren<QToolButton*>())if(b->text()==QStringLiteral("应用"))applications.append(b);
        require(applications.size()==2,"Original separate start/end Apply controls missing");
        applications[0]->click();direction->endPickController()->setPickActive(true);mPickWidgetController::setActivePickType(Type::GeoPoint);
        click(*view,{40,0,20},box);applications[1]->click();app.processEvents();
        require(expectNear(direction->startPoint(),{0,0,20})&&expectNear(direction->endPoint(),{40,0,20}),"Original two-point coordinate workflow failed");
        require(expectNear(direction->getDirection().value<QVector3D>(),{1,0,0}),"Two-point direction incorrect");
        log<<"PASS original start/end fields / independent Apply buttons / actual two-point direction\n";

        detailed->setAllowedTypes([]{return QSet<Type>{Type::GeoSolid};});detailed->setPickActive(true);
        click(*view,{70,10,20},solidPart);click(*view,{100,10,20},solidPart);
        const auto solids=detailed->getAllData().pickedData;
        require(solids.count(Type::GeoSolid)&&solids.at(Type::GeoSolid).size()==2,"Two solids in one part collapsed into one part selection");
        const auto first=context.resolve(Type::GeoSolid,solids.at(Type::GeoSolid)[0]);
        const auto second=context.resolve(Type::GeoSolid,solids.at(Type::GeoSolid)[1]);
        require(first.kind==PickKind::Solid&&second.kind==PickKind::Solid&&first.partId==second.partId&&first.index!=second.index,"Solid references are not actual subshape IDs");
        require(std::abs(std::abs(first.position[0]-second.position[0])-30)<1e-6,"Actual solid centroids not preserved");
        PickResult idOnly;idOnly.kind=PickKind::Solid;idOnly.partId=first.partId;idOnly.index=first.index;
        const auto converted=context.toLegacy(idOnly,Type::GeoSolid);
        require(context.resolve(Type::GeoSolid,converted.second).position==first.position,"ID-only topology selection invented a world-origin coordinate");
        require(button(detailed->widget(),QStringLiteral("当前拾取数据")),"Original multi-pick details control missing");
        detailed->setPickActive(false);require(!view->isPicking(),"Deactivation did not leave viewport picker");
        require(view->grabFramebuffer().save(output.filePath("original-solid-feedback.png")),"Could not capture solid feedback");
        log<<"PASS real multi-solid subshape selection / global topology identity / inactive feedback\n";

        const QVector<int> nodeIds{101,202,303,404};
        context.setNodeIds(box,nodeIds);
        view->setScene(cad,meshes); // Rebuilding the scene must retain IDs when activating the original controller.
        detailed->setPickFilter({int(Type::Node)}); // Original AutoFlex attachment setup.
        detailed->replacePickedDataAndClearSets({});detailed->setAllowedTypes([]{return QSet<Type>{Type::Node};});detailed->setPickActive(true);
        click(*view,{0,0,20},box);click(*view,{40,0,20},box);
        const auto nodes=detailed->getAllData().pickedData;
        require(nodes.count(Type::Node)&&nodes.at(Type::Node).size()==2,"Real FEM multi-node pick failed");
        require(nodes.at(Type::Node)[0].result_id==101&&nodes.at(Type::Node)[1].result_id==202,"FEM node IDs were replaced by display ordinals");
        click(*view,view->projectPoint({40,0,20},box),Qt::RightButton);
        require(detailed->getAllData().pickedData.at(Type::Node).size()==1,"Original node pick undo failed");
        QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);QApplication::sendEvent(view,&escape);app.processEvents();
        require(!detailed->isPickActive()&&!view->isPicking()&&!detailed->getAllData().isEmpty(),"Esc lost accepted original picker data");
        auto* clear=button(detailed->widget(),QStringLiteral("清空"));require(clear&&!clear->icon().isNull(),"Original picker Clear icon missing");
        clear->click();require(detailed->getAllData().isEmpty(),"Original Clear control failed");
        detailed->setPickFilter({});
        log<<"PASS FEM IDs / multi-selection / right undo / Esc commit / original Clear icon\n";

        // Loading a panel's fields must not publish selection/highlight changes.
        detailed->setAllowedTypes([]{return QSet<Type>{Type::Node};});
        PickWidgetData nodeA,nodeB;
        nodeA.pickedData[Type::Node].emplace_back(101,-1,box);
        nodeB.pickedData[Type::Node].emplace_back(202,-1,box);
        detailed->displayData(nodeA);detailed->setPickActive(true);
        const auto selectedBeforeLoad=view->pickSelection();
        require(selectedBeforeLoad.size()==1&&selectedBeforeLoad.first().nodeId==101,"Panel-loading fixture has no real selected node");
        const auto feedbackBeforeLoad=view->grabFramebuffer();
        detailed->displayData(nodeB,false);
        require(detailed->getAllData().pickedData.at(Type::Node).front().result_id==202,
                "displayData(false) did not update original UI data");
        require(view->pickSelection().size()==1&&view->pickSelection().first().nodeId==101,
                "displayData(false) replaced the active viewport selection");
        require(changedPixels(feedbackBeforeLoad,view->grabFramebuffer())==0,
                "displayData(false) wrote a feedback layer");
        point->pickController()->displayData(nodeB,false);
        require(!point->pickController()->isPickActive()&&detailed->isPickActive()&&
                changedPixels(feedbackBeforeLoad,view->grabFramebuffer())==0,
                "Inactive panel loading stole selection or published highlights");
        point->pickController()->displayData({});
        detailed->displayData(nodeB,true);
        require(view->pickSelection().size()==1&&view->pickSelection().first().nodeId==202&&
                changedPixels(feedbackBeforeLoad,view->grabFramebuffer())>10,
                "displayData(true) did not explicitly refresh the selected feedback");
        log<<"PASS original displayData false is UI-only / true synchronizes selection and feedback\n";

        // Cloud mode hides active nodes/hover/selection, while the original
        // controller and real viewport collector continue accepting input.
        detailed->setPickActive(false);detailed->clearAllHighlight();
        const auto noFeedback=view->grabFramebuffer();
        detailed->setPickActive(true);detailed->setCloudChartMode(true);
        require(detailed->getAllData().pickedData.at(Type::Node).front().result_id==202&&
                view->pickSelection().size()==1&&changedPixels(noFeedback,view->grabFramebuffer())==0,
                "Cloud mode cleared selection or left selectable-node feedback visible");
        const auto nodePosition=view->projectPoint({0,0,20},box);
        QMouseEvent hover(QEvent::MouseMove,nodePosition,view->mapToGlobal(nodePosition.toPoint()),
                          Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&hover);
        require(changedPixels(noFeedback,view->grabFramebuffer())==0,"Cloud mode leaked hover feedback");
        click(*view,{0,0,20},box);
        require(detailed->getAllData().pickedData.at(Type::Node).size()==2&&view->pickSelection().size()==2&&
                changedPixels(noFeedback,view->grabFramebuffer())==0,
                "Hidden cloud collector lost picking or redrew active feedback");
        detailed->setCloudChartMode(false);
        require(view->pickSelection().size()==2&&changedPixels(noFeedback,view->grabFramebuffer())>20,
                "Leaving cloud mode did not restore the retained selection feedback");
        detailed->setCloudChartMode(true);detailed->setPickActive(false);detailed->setCloudChartMode(false);
        detailed->replacePickedDataAndClearSets({});
        log<<"PASS cloud feedback suppression / live typed picking / hover and node overlay / restoration\n";

        // Keep an actual reference from the first document, then replace that
        // part ID with a different shape. Equal ordinals must not rebind it.
        TopTools_IndexedMapOfShape initialVertices;
        TopExp::MapShapes(cad.part(box)->shape,TopAbs_VERTEX,initialVertices);
        PickResult oldVertex;oldVertex.kind=PickKind::Vertex;oldVertex.partId=box;
        for(int i=1;i<=initialVertices.Extent();++i)
            if(std::abs(BRep_Tool::Pnt(TopoDS::Vertex(initialVertices(i))).Z()-20)<1e-8)oldVertex.index=i;
        require(oldVertex.index>2,"Replacement fixture lacks a visible box vertex absent from the sphere");
        const auto initialPoint=BRep_Tool::Pnt(TopoDS::Vertex(initialVertices(oldVertex.index)));
        oldVertex.position=oldVertex.hitPosition={initialPoint.X(),initialPoint.Y(),initialPoint.Z()};
        const auto legacyVertex=context.toLegacy(oldVertex,Type::GeoPoint);
        PickWidgetData vertexData;vertexData.pickedData[Type::GeoPoint].push_back(legacyVertex.second);
        detailed->setAllowedTypes([]{return QSet<Type>{Type::GeoPoint};});detailed->displayData(vertexData);
        const auto noCadFeedback=view->grabFramebuffer();
        detailed->setPickActive(true);detailed->setCloudChartMode(true);
        const auto cadPosition=view->projectPoint(oldVertex.position,box);
        QMouseEvent cadHover(QEvent::MouseMove,cadPosition,view->mapToGlobal(cadPosition.toPoint()),
                            Qt::NoButton,Qt::NoButton,Qt::NoModifier);
        QApplication::sendEvent(view,&cadHover);
        require(view->pickSelection().size()==1&&changedPixels(noCadFeedback,view->grabFramebuffer())==0,
                "Cloud mode leaked active CAD or CAD hover feedback");
        detailed->setCloudChartMode(false);
        require(changedPixels(noCadFeedback,view->grabFramebuffer())>10,"Cloud exit did not restore CAD feedback");
        detailed->setPickActive(false);detailed->replacePickedDataAndClearSets({});
        log<<"PASS cloud suppression also covers real CAD selected and hovered topology\n";
        for(const auto primitive:{Primitive::Box,Primitive::Sphere}) {
            CadDocument replacement;int replacementId=-1;
            require(replacement.addPrimitive(primitive,12,8,10,replacementId,error),error);
            require(replacementId==box,"Replacement fixture did not reuse the document-local part ID");
            view->setScene(cad,meshes);
            const auto beforeReference=view->grabFramebuffer();
            view->setAuxiliaryPickHighlights("reimport-test",{oldVertex},QColor(240,0,180));
            require(changedPixels(beforeReference,view->grabFramebuffer())>10,"Reimport fixture has no visible auxiliary marker");
            view->beginPick(PickKind::Vertex,false,{box},{oldVertex});
            require(view->pickSelection().size()==1,"Reimport fixture has no real vertex selection");
            const auto unchangedFrame=view->grabFramebuffer();
            view->setScene(cad,meshes);
            require(view->pickSelection().size()==1&&changedPixels(unchangedFrame,view->grabFramebuffer())==0,
                    "A display-only rebuild discarded valid selection or auxiliary feedback");
            view->setScene(replacement,{});
            require(view->pickSelection().isEmpty(),"Scene replacement rebound an old ordinal to a different OCC shape");
            const auto replacedFrame=view->grabFramebuffer();
            view->clearAuxiliaryPickHighlights("reimport-test");
            require(changedPixels(replacedFrame,view->grabFramebuffer())==0,
                    "Scene replacement retained a stale auxiliary geometry marker");
            view->endPick();
        }
        view->setScene(cad,meshes);
        context.setNodeIds(box,nodeIds);
        const auto validScene=view->grabFramebuffer();
        PickResult invalidVertex=oldVertex;invalidVertex.index=initialVertices.Extent()+1;
        view->setAuxiliaryPickHighlights("invalid-index",{invalidVertex},QColor(240,0,180));
        require(changedPixels(validScene,view->grabFramebuffer())==0,
                "An invalid topology index generated an auxiliary marker");
        auto invalidMesh=context.resolve(Type::ElementEdge,RenderPickResultData{101,202,box});
        require(invalidMesh.isValid()&&invalidMesh.nodeIds.size()==2,"Mesh-validation fixture has no real FEM edge");
        invalidMesh.nodeIds[1]=999;
        view->setAuxiliaryPickHighlights("invalid-connectivity",{invalidMesh},QColor(240,0,180));
        require(changedPixels(validScene,view->grabFramebuffer())==0,
                "Mismatched actual FEM connectivity generated an auxiliary marker");
        view->setAuxiliaryPickHighlights("deleted-part",{oldVertex},QColor(240,0,180));
        CadDocument empty;view->setScene(empty,{});view->setScene(cad,meshes);
        const auto afterReimport=view->grabFramebuffer();
        view->clearAuxiliaryPickHighlights("deleted-part");
        require(changedPixels(afterReimport,view->grabFramebuffer())==0,
                "Deleted-part auxiliary feedback returned after reimport");
        context.setNodeIds(box,nodeIds);
        log<<"PASS auxiliary index validation / changed OCC identity / fewer subshapes / deletion and reimport\n";

        TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(cad.part(box)->shape,TopAbs_VERTEX,vertices);
        LegacyPickMap average;
        for(int i=1;i<=vertices.Extent();++i) {
            const auto p=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));
            if(std::abs(p.Y())<1e-9&&std::abs(p.Z())<1e-9) {
                PickResult pick;pick.kind=PickKind::Vertex;pick.partId=box;pick.index=i;pick.position={p.X(),p.Y(),p.Z()};
                const auto legacy=context.toLegacy(pick,Type::GeoPoint);average[legacy.first].push_back(legacy.second);
            }
        }
        require(expectNear(context.averageCoordinate(average),{20,0,0}),"A real vertex at world origin was discarded as invalid");
        MxFunctions::mCalcLineEdit expression(MxFunctions::mCalcLineEdit::CalcMode::NonNegative);
        expression.setExpression("sqrt(16)+2^3");require(expression.text().toDouble()==12,"Original numeric input expression not evaluated");
        expression.setText("-1");require(!expression.hasAcceptableInput(),"Nonnegative numeric field accepted a negative value");
        expression.setText("1/0");require(!expression.hasAcceptableInput(),"Numeric field accepted non-finite value");
        log<<"PASS zero-origin geometry / original numeric-input API with expression and range validation\n";
        {
            const auto newVector=[&](const QString& title,const std::array<double,3>& initial) {
                auto* host=new QGroupBox(title,&window);auto* form=new QFormLayout(host);
                for(int i=0;i<3;++i) {
                    auto* edit=new QLineEdit(QString::number(initial[i],'g',17),host);
                    edit->setObjectName("native_"+QString(QChar('x'+i)));form->addRow(QString(QChar('x'+i)),edit);
                }
                return host;
            };
            NativePickSession extension(view,cad,{},&window);
            auto* host=newVector(QStringLiteral("重力"),{0,0,-9806.65});
            extension.decorateVector(qobject_cast<QFormLayout*>(host->layout()),host,"gravity","gravity");
            auto* original=host->findChild<mDirectionSelectorWidget*>();
            require(original&&original->directionMode()==mDirectionSelectorWidget::DirectInput,"Engine extension did not reuse original direction control");
            require(host->findChild<QLineEdit*>("native_z")->isHidden(),"Old XYZ fields remained visible alongside original control");
            auto* flip=button(original,QStringLiteral("反向"));require(flip,"Engine extension lost original reverse control");flip->click();
            const double reversed=host->findChild<QLineEdit*>("native_z")->text().toDouble();
            require(std::abs(reversed-9806.65)<.001,"Original extension control did not preserve gravity magnitude/sign");
            NativePickSession restored(view,cad,extension.references(),&window);
            auto* restoredHost=newVector(QStringLiteral("重力"),{0,0,reversed});
            restored.decorateVector(qobject_cast<QFormLayout*>(restoredHost->layout()),restoredHost,"gravity","gravity");
            auto* restoredControl=restoredHost->findChild<mDirectionSelectorWidget*>();
            require(restoredControl&&restoredControl->isReversed()&&expectNear(restoredControl->getDirection().value<QVector3D>(),{0,0,1}),"Original extension mode/reverse state did not round-trip");
            auto* pointHost=newVector(QStringLiteral("作用点"),{1,2,3});
            extension.decorateVector(qobject_cast<QFormLayout*>(pointHost->layout()),pointHost,"actionPoint","actionPoint");
            auto* originalPoint=pointHost->findChild<mPointPickerWidget*>();require(originalPoint,"Engine extension did not reuse original point control");
            QLineEdit* vectorEdit=nullptr;
            for(auto* input:originalPoint->findChildren<QLineEdit*>())if(!input->isReadOnly())vectorEdit=input;
            require(vectorEdit,"Original point vector input missing");vectorEdit->setText("9, 8, 7");
            QKeyEvent enter(QEvent::KeyPress,Qt::Key_Return,Qt::NoModifier);QApplication::sendEvent(vectorEdit,&enter);
            require(pointHost->findChild<QLineEdit*>("native_x")->text().toDouble()==9&&pointHost->findChild<QLineEdit*>("native_z")->text().toDouble()==7,"Original point input did not reach engine serialized fields");
            delete pointHost;delete restoredHost;delete host;
        }
        log<<"PASS new-engine fields reuse original controls / hidden serialization / magnitude and reverse round-trip\n";
        if(auto* active=mPickWidgetController::activeController())active->setPickActive(false);
        mxr::CCSMRendWidget render(&context,nullptr);
        render.beginPartPick();render.setSelectedParts({box},false);
        require(view->pickSelection().size()==1&&view->pickSelection().first().partId==box&&
                view->pickSelection().first().kind==PickKind::Part,
                "Explicit shared-part selection did not update its original collector");
        render.setSelectedParts({},false);
        require(view->pickSelection().isEmpty(),"Clearing shared-part selection retained an active viewport pick");
        if(auto* active=mPickWidgetController::activeController())active->setPickActive(false);
        MotionSymbol retained;retained.drawOrigin=false;retained.retainOpacity=true;retained.color=QColor(0,230,40);
        retained.collection="springs";retained.id=1;retained.linePoints={{{60,-5,20}},{{100,-5,20}}};
        MotionSymbol replaced=retained;replaced.collection="joints";replaced.id=7;replaced.color=QColor(15,15,250);
        replaced.linePoints={{{60,-11,20}},{{100,-11,20}}};view->setMotionSymbols({retained,replaced});
        auto* symbols=render.GetSymbolController();
        const QString symbolKey="motionJoint_7";
        auto face=symbols->getOrCreateSymbolData<mxr::UniversalSymbolFont>(symbolKey);
        face->_vexter={{8,5,10},{28,5,10},{8,23,10}};face->_faceIndex={0,1,2};face->_faceColor={245,10,25};
        symbols->UpdateAllSymbol();
        render.getSelectionContainer()->getSymbolSelectData()->UpdatePickData(mxr::SelectionOperate::ADD,mxr::SymbolSelectionInfoType::Symbol_All,{symbolKey});
        MechPreData::ClearPreWidgetSymbolPick(&render);
        require(symbols->isExistSymbol(symbolKey)&&render.getSelectionContainer()->getSymbolSelectData()->values.isEmpty(),"Clearing selected glyphs deleted the original symbol data");
        const auto sample=[&](const QImage& image,const std::array<double,3>& p) {
            const auto pixel=view->projectPoint(p)*view->devicePixelRatioF();
            return image.pixelColor(qRound(pixel.x()),qRound(pixel.y()));
        };
        auto filled=view->grabFramebuffer();const auto overlay=sample(filled,{13,10,10});
        filled.save(output.filePath("original-symbol-layers.png"));
        require(overlay.red()>210&&overlay.green()<50,"Original indexed symbol faces were lost or changed to wireframe");
        const auto linePixel=view->projectPoint({80,-5,20})*view->devicePixelRatioF();
        bool nativeLine=false;
        for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x) {
            const auto color=filled.pixelColor(qRound(linePixel.x())+x,qRound(linePixel.y())+y);
            nativeLine=nativeLine||(color.green()>120&&color.green()-color.red()>100);
        }
        require(nativeLine,"Original preview layer replaced native animation symbols");
        const auto blueAt=[&](const QImage& image) {
            const auto pixel=view->projectPoint({80,-11,20})*view->devicePixelRatioF();
            for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x) {
                const auto color=image.pixelColor(qRound(pixel.x())+x,qRound(pixel.y())+y);
                if(color.blue()-color.red()>100&&color.blue()-color.green()>100)return true;
            }
            return false;
        };
        require(!blueAt(filled),"Editing one entity drew both its original and native glyphs");
        symbols->setSymbolShowState({symbolKey},false);symbols->UpdateAllSymbol();
        require(blueAt(view->grabFramebuffer()),"Hiding the original preview failed to restore its native base glyph");
        symbols->setSymbolShowState({symbolKey},true);
        face->_faceDepthState=true;symbols->UpdateAllSymbol();
        const auto occluded=sample(view->grabFramebuffer(),{13,10,10});
        require(!(occluded.red()>210&&occluded.green()<50),"Explicit depth-tested symbol face leaked through CAD");
        symbols->clear();view->setMotionSymbols({});
        log<<"PASS original symbol selection clear / indexed face fill / independent layers / duplicate suppression / depth semantics\n";
        direction->setDirectionMode(mDirectionSelectorWidget::SelectObject);direction->setObjectPickData(edgeState);direction->objectPickController()->setPickActive(true);
        app.processEvents();require(window.grab().save(output.filePath("original-picker-controls.png")),"Could not capture original controls");
        log<<"PASS all original picker regression checks\n";log.flush();
        window.close();return 0;
    } catch(const std::exception& failure) {log<<"FAIL "<<failure.what()<<'\n';return 1;}
}
