#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"
#include "NativeMotionGeometry.h"

#include <BRep_Builder.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS_Compound.hxx>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QPushButton>
#include <QProcess>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWheelEvent>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool value, const QString& message)
{
    if (!value) throw std::runtime_error(message.toStdString());
}

bool waitFor(const std::function<bool()>& predicate, int timeout = 5000)
{
    if (predicate()) return true;
    QEventLoop loop;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] { if (predicate()) loop.quit(); });
    poll.start(20);
    QTimer::singleShot(timeout, &loop, &QEventLoop::quit);
    loop.exec();
    return predicate();
}

template<class T> T* widget(QWidget& parent, const QString& name)
{
    auto* found = parent.findChild<T*>(name);
    require(found, QStringLiteral("Missing widget: %1").arg(name));
    return found;
}

QTreeWidgetItem* node(QTreeWidget& tree, ModelNode::Kind kind, int id = -1, const QString& collection = {})
{
    for (QTreeWidgetItemIterator it(&tree); *it; ++it)
        if ((*it)->data(0, Qt::UserRole + 1).toInt() == int(kind)
            && (id < 0 || (*it)->data(0, Qt::UserRole).toInt() == id)
            && (collection.isEmpty() || (*it)->data(0, Qt::UserRole + 2).toString() == collection)) return *it;
    return nullptr;
}

int requireTreeIcons(QTreeWidget& tree)
{
    int count = 0;
    for (QTreeWidgetItemIterator it(&tree); *it; ++it) {
        const auto* item = *it;
        const auto image = item->icon(0).pixmap(tree.iconSize(),
            item->isDisabled() ? QIcon::Disabled : QIcon::Normal).toImage().convertToFormat(QImage::Format_ARGB32);
        bool painted = false;
        for (int y = 0; y < image.height() && !painted; ++y)
            for (int x = 0; x < image.width() && !painted; ++x) painted = qAlpha(image.pixel(x, y)) != 0;
        require(painted, QStringLiteral("Model tree icon is missing or blank: %1").arg(item->text(0)));
        ++count;
    }
    require(count > 0, "Model tree has no icon-bearing items");
    return count;
}

void requireOriginalIcon(const QTreeWidgetItem* item, const char* path)
{
    require(item, "Model tree item for original icon is missing");
    const auto expected = QIcon(QString::fromLatin1(path)).pixmap(16, 16).toImage();
    require(!expected.isNull() && item->icon(0).pixmap(16, 16).toImage() == expected,
        QStringLiteral("Model tree did not retain its original icon: %1").arg(item->text(0)));
}

void number(QWidget& panel, const QString& name, double value)
{
    widget<QLineEdit>(panel, "native_" + name)->setText(QString::number(value, 'g', 17));
}

void choose(QWidget& panel, const QString& name, int value)
{
    auto* field = widget<QComboBox>(panel, "native_" + name);
    const int index = field->findData(value);
    require(index >= 0, QStringLiteral("Missing native reference %1=%2").arg(name).arg(value));
    field->setCurrentIndex(index);
}

QGroupBox* vector(QWidget& panel, const QString& group, double x, double y, double z)
{
    for (auto* box : panel.findChildren<QGroupBox*>()) if (box->title().startsWith(group)) {
        auto* first = widget<QLineEdit>(*box, "native_x");
        auto* second = widget<QLineEdit>(*box, "native_y");
        auto* third = widget<QLineEdit>(*box, "native_z");
        require(waitFor([&] {
            return first->height() >= 16 && second->height() >= 16 && third->height() >= 16
                && first->geometry().bottom() < second->geometry().top()
                && second->geometry().bottom() < third->geometry().top();
        }), "Native XYZ fields are collapsed instead of three independently editable rows: " + group);
        number(*box, "x", x); number(*box, "y", y); number(*box, "z", z); return box;
    }
    require(false, QStringLiteral("Missing vector editor: %1").arg(group));
    return nullptr;
}

QJsonObject xyz(double x, double y, double z) { return {{"x", x}, {"y", y}, {"z", z}}; }

NativeMotionDocument makeFixture(const QString& directory, QString& cadPath, QString& motionPath)
{
    BRep_Builder builder;
    TopoDS_Compound compound;
    builder.MakeCompound(compound);
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(0, -5, -4), 30, 10, 8).Shape());
    builder.Add(compound, BRepPrimAPI_MakeBox(gp_Pnt(-14, -8, -4), 12, 16, 8).Shape());
    const auto brep = QDir(directory).filePath(QStringLiteral("native-fixture.brep"));
    require(BRepTools::Write(compound, brep.toUtf8().constData()), "Could not write BREP fixture");
    CadDocument cad;
    std::vector<int> ids;
    QString error;
    require(cad.importFile(brep, ids, error) && ids.size() == 2, "Could not import two-part CAD fixture: " + error);
    cadPath = QDir(directory).filePath(QStringLiteral("native-fixture.step"));
    require(cad.exportFile(cadPath, -1, error), "Could not write STEP fixture: " + error);
    QJsonArray bodies;
    for (const auto& part : cad.parts()) {
        GProp_GProps properties;
        BRepGProp::VolumeProperties(part.shape, properties);
        const auto center = properties.CentreOfMass();
        const auto tensor = properties.MatrixOfInertia();
        const bool fixed = center.X() < 0;
        const double mass = fixed ? 4 : 2, scale = mass / properties.Mass();
        bodies.append(QJsonObject{{"id", fixed ? 37 : 0}, {"name", fixed ? QStringLiteral("原始基座 37") : QStringLiteral("原始转子 0")},
            {"geoEntityIds", QJsonArray{fixed ? 205 : 901}}, {"isFixed", fixed}, {"isCustomMassProps", false}, {"mass", mass},
            {"centerOfMass", xyz(center.X(), center.Y(), center.Z())},
            {"inertia", QJsonArray{tensor(1,1)*scale, tensor(2,2)*scale, tensor(3,3)*scale,
                tensor(1,2)*scale, tensor(1,3)*scale, tensor(2,3)*scale}},
            {"initialVelocity", xyz(0,0,0)}, {"initialAngularVelocity", xyz(0,0,0)}});
    }
    NativeMotionDocument document;
    document.root = {{"formatVersion", 1}, {"motionData", QJsonObject{
        {"bodies", bodies},
        {"joints", QJsonArray{QJsonObject{{"id", 23}, {"name", QStringLiteral("原始铰链 23")}, {"jointType", "Revolute"},
            {"actionBodyId", 0}, {"baseBodyId", 37}, {"origin", xyz(0,0,0)}, {"axis", xyz(0,0,1)}}}},
        {"drives", QJsonArray{QJsonObject{{"id", 9}, {"name", QStringLiteral("原始驱动 9")}, {"driveType", "JointDrive"},
            {"targetType", "Joint"}, {"targetJointId", 23}, {"motionType", "Rotation"}, {"controlMode", "Velocity"}, {"expression", "90"}}}},
        {"scenarios", QJsonArray{QJsonObject{{"id", 0}, {"name", QStringLiteral("原始方案 0")}, {"endTime", 1.0}, {"stepSize", 0.01},
            {"gravity", xyz(0,0,0)}, {"usePreset", false}, {"integratorType", "EULER_IMPLICIT_LINEARIZED"},
            {"solverType", "SPARSE_QR"}, {"solverTolerance", 1e-10}, {"curveRequests", QJsonArray{}}}}}}}};
    motionPath = QDir(directory).filePath(QStringLiteral("native-fixture.motion.json"));
    require(writeNativeMotionDocument(motionPath, document, error), "Could not write original JSON fixture: " + error);
    return document;
}

void selectSpringType(MotionWindow& window)
{
    bool accepted = false;
    QTimer poll;
    QElapsedTimer elapsed;
    elapsed.start();
    QObject::connect(&poll, &QTimer::timeout, &window, [&] {
        auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto* combo = dialog->findChild<QComboBox*>();
        const int index = combo ? combo->findText(QStringLiteral("体点弹簧")) : -1;
        if (index >= 0) { combo->setCurrentIndex(index); accepted = true; dialog->accept(); }
        else if (elapsed.elapsed() > 3000) dialog->reject();
    });
    poll.start(20);
    window.command(Command::AddSpring)->trigger();
    poll.stop();
    require(accepted, "Body-point spring type chooser did not offer the original type");
}

void saveThroughUi(MotionWindow& window, const QString& path, Command command = Command::SaveAs)
{
    QFile::remove(path); // This test owns its output artifact.
    bool accepted = false;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &window, [&] {
        auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
        if (!dialog) return;
        dialog->selectFile(path);
        accepted = true;
        QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection);
    });
    poll.start(20);
    window.command(command)->trigger();
    poll.stop();
    require(accepted && QFile::exists(path), "Native model did not save through the requested command");
}

void solve(MotionWindow& window, QTreeWidget& tree, int scenarioId, int timeout = 60000)
{
    auto* scenario = node(tree, ModelNode::Scenario, scenarioId);
    require(scenario, "Native scenario is missing from the model tree");
    tree.setCurrentItem(scenario);
    require(window.command(Command::Solve)->isEnabled(), "Native Solve command is disabled");
    window.command(Command::Solve)->trigger();
    const bool completed = waitFor([&] {
        const auto* curves = node(tree, ModelNode::Curves, scenarioId);
        return curves && !curves->isDisabled() && !window.command(Command::StopSolve)->isEnabled();
    }, timeout);
    require(completed, "Native UI did not produce an accepted real solver result");
}

void removeThroughTree(MotionWindow& window, QTreeWidget& tree, const QString& collection, int id)
{
    auto* item = node(tree, ModelNode::Native, id, collection);
    require(item, "Native item to remove is missing");
    tree.setCurrentItem(item);
    tree.scrollToItem(item);
    QApplication::processEvents();
    bool triggered = false;
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &window, [&] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu) return;
        for (auto* action : menu->actions()) if (action->text() == QStringLiteral("删除")) {
            triggered = true;
            action->trigger();
            menu->close();
            return;
        }
        menu->close();
    });
    poll.start(20);
    QMetaObject::invokeMethod(&tree, "customContextMenuRequested", Qt::DirectConnection,
        Q_ARG(QPoint, tree.visualItemRect(item).center()));
    poll.stop();
    require(triggered && !node(tree, ModelNode::Native, id, collection),
        "Existing display curves prevented removal through the native tree menu");
}

void addCurve(MotionWindow& window, const QString& type, int id, const QString& variable, int component)
{
    auto* panel = widget<QWidget>(window, "motionCurveConfiguration");
    auto* source = widget<QComboBox>(*panel, "motionCurveSource");
    auto* object = widget<QComboBox>(*panel, "motionCurveObject");
    auto* value = widget<QComboBox>(*panel, "motionCurveVariable");
    const int typeIndex = source->findData(type);
    require(typeIndex >= 0, "Native result source is missing: " + type);
    source->setCurrentIndex(typeIndex);
    const int objectIndex = object->findData(id);
    require(objectIndex >= 0, QStringLiteral("Native result ID %1/%2 is missing").arg(type).arg(id));
    object->setCurrentIndex(objectIndex);
    const int variableIndex = value->findData(variable + QLatin1Char(':') + QString::number(component));
    require(variableIndex >= 0, "Native result variable is missing: " + variable);
    value->setCurrentIndex(variableIndex);
    widget<QPushButton>(*panel, "motionAddCurveButton")->click();
}

void show(MotionWindow& window)
{
    const auto geometry = QApplication::primaryScreen()->availableGeometry();
    window.resize(std::min(1280, geometry.width() - 24), std::min(850, geometry.height() - 24));
    window.showNormal();
    auto* viewport = widget<OccViewport>(window, "occViewport");
    require(waitFor([&] { return viewport->glReady(); }, 12000), "Native desktop OpenGL context did not initialize");
}

void writeVolumeFixture(const QString& path, const FemMesh& mesh)
{
    QFile file(path);
    require(file.open(QIODevice::WriteOnly | QIODevice::Truncate),"Cannot write real mesh VOL fixture");
    QTextStream stream(&file); stream.setLocale(QLocale::c()); stream.setRealNumberPrecision(17);
    stream << "mesh3d\ndimension\n3\npoints\n" << qulonglong(mesh.nodes.size()) << '\n';
    for(const auto& p:mesh.nodes)stream << p[0] << ' ' << p[1] << ' ' << p[2] << '\n';
    stream << "surfaceelements\n" << qulonglong(mesh.surface.size()) << '\n';
    for(const auto& t:mesh.surface)stream << "1 1 1 0 3 " << t[0]+1 << ' ' << t[1]+1 << ' ' << t[2]+1 << '\n';
    stream << "volumeelements\n" << qulonglong(mesh.tetrahedra.size()) << '\n';
    for(const auto& t:mesh.tetrahedra)stream << "1 4 " << t[0]+1 << ' ' << t[1]+1 << ' ' << t[2]+1 << ' ' << t[3]+1 << '\n';
    stream << "endmesh\n"; stream.flush();
    require(stream.status()==QTextStream::Ok && file.error()==QFileDevice::NoError,"Cannot finish real mesh VOL fixture");
}

void autoFlexWorkflow(const QDir& output, const std::function<void(const QString&)>& log)
{
    const auto shape=BRepPrimAPI_MakeBox(gp_Pnt(0,-4,-3),30,8,6).Shape();
    const auto brep=output.filePath("autoflex-cantilever.brep");
    require(BRepTools::Write(shape,brep.toUtf8().constData()), "Cannot write generated AutoFlex CAD");
    CadDocument cad; std::vector<int> partIds; QString error;
    require(cad.importFile(brep,partIds,error) && partIds.size()==1,error);
    const auto step=output.filePath("autoflex-cantilever.step");
    require(cad.exportFile(step,-1,error),error);
    NativeMotionDocument document;
    document.root={{"formatVersion",1},{"motionData",QJsonObject{
        {"bodies",QJsonArray{QJsonObject{{"id",0},{"name","synthetic cantilever"},{"geoEntityIds",QJsonArray{707}},
            {"isFixed",false},{"isCustomMassProps",false},{"mass",1.44},{"centerOfMass",xyz(15,0,0)},
            {"inertia",QJsonArray{12.0,112.32,115.68,0,0,0}}, {"initialVelocity",xyz(0,0,0)},{"initialAngularVelocity",xyz(0,0,0)}}}},
        {"joints",QJsonArray{QJsonObject{{"id",7},{"name","clamp"},{"jointType","Fixed"},{"actionBodyId",0},{"baseBodyId",-1},
            {"origin",xyz(0,0,0)},{"axis",xyz(1,0,0)}}}},
        {"materials",QJsonArray{QJsonObject{{"id",42},{"name","synthetic compliant material"},{"youngModulus",120.0},{"poissonRatio",.3},{"density",.001}}}},
        {"autoFlexBodies",QJsonArray{QJsonObject{{"id",13},{"name","automatic cantilever"},{"targetBodyId",0},{"materialId",42},
            {"elementSize",4.0},{"modeCount",2},{"connectionRadius",4.0},{"dampingType",1},{"modalDampingRatio",.02},{"enabled",true},{"attachPoints",QJsonArray{}}}}},
        {"scenarios",QJsonArray{QJsonObject{{"id",0},{"name","cantilever gravity"},{"endTime",.05},{"stepSize",.001},
            {"gravity",xyz(0,-5000,0)},{"usePreset",false},{"integratorType","EULER_IMPLICIT_LINEARIZED"},{"solverType","SPARSE_QR"},
            {"solverTolerance",1e-10},{"curveRequests",QJsonArray{}}}}}}}};
    const auto motion=output.filePath("autoflex-cantilever.motion.json");
    require(writeNativeMotionDocument(motion,document,error),error);
    MotionWindow window; show(window);
    require(window.importCad(step) && window.importMotionJson(motion,false), "AutoFlex public CAD + original JSON import failed");
    auto* tree=widget<QTreeWidget>(window,"mTreeView");
    tree->setCurrentItem(node(*tree,ModelNode::Native,13,"autoFlexBodies"));
    auto* editor=widget<QWidget>(window,"nativeEditor_autoFlexBodies");
    require(widget<QComboBox>(*editor,"native_materialId")->currentData().toInt()==42
        && widget<QLineEdit>(*editor,"native_youngModulus")->text().toDouble()==120.0,
        "AutoFlex editor replaced an existing material reference with default steel");
    number(*editor,"youngModulus",100.0);
    require(window.grab().save(output.filePath("autoflex-parameters.png")),"Cannot capture AutoFlex material and mesh controls");
    widget<QPushButton>(*editor,"motionApplyButton")->click();
    require(waitFor([&] { return window.command(Command::Solve)->isEnabled() && window.command(Command::MeshDisplay)->isChecked(); },120000),
        "Saving AutoFlex did not generate and preview a real tetra mesh");
    const auto project=output.filePath("autoflex-cantilever.mxmotion");
    saveThroughUi(window,project);
    CadDocument storedCad; RigidModel storedModel;
    require(loadRigidProject(project,storedCad,storedModel,error) && storedModel.nativeDocument,error);
    auto stored=*storedModel.nativeDocument;
    const auto flex=stored.entity("autoFlexBodies",13);
    const auto mesh=flex.value("mesh").toObject();
    const int nodes=mesh.value("nodes").toArray().size(), tets=mesh.value("tetrahedra").toArray().size();
    require(nodes>8 && tets>5 && !mesh.value("surfaceTriangles").toArray().isEmpty(),"AutoFlex did not save the real Netgen nodes/cells");
    require(flex.value("materialId").toInt()==42 && flex.value("material").toObject().isEmpty()
        && stored.entity("materials",42).value("youngModulus").toDouble()==100.0,
        "AutoFlex material edits were not written back to the referenced material object");
    auto* meshPanel=widget<QWidget>(window,"nativeEditor_autoFlexBodies");
    auto* meshStatus=widget<QLabel>(*meshPanel,"nativeFlexMeshStatus");
    auto* meshScroll=widget<QScrollArea>(window,"nativeParameterScroll");
    for(const auto& field:std::vector<std::pair<QWidget*,QString>>{
            {widget<QComboBox>(*meshPanel,"native_materialId"),QStringLiteral("材料")},
            {meshStatus,QStringLiteral("体网格")},
            {widget<QLineEdit>(*meshPanel,"native_name"),QStringLiteral("名称")}}) {
        QLabel* fieldLabel=nullptr;
        for(auto* layout:meshPanel->findChildren<QFormLayout*>())
            if(auto* label=qobject_cast<QLabel*>(layout->labelForField(field.first))) { fieldLabel=label; break; }
        require(fieldLabel && fieldLabel->text()==field.second,"AutoFlex form lost a material/mesh/name label");
        meshScroll->ensureWidgetVisible(fieldLabel,0,8);
        require(waitFor([&]{
            const QRect inPanel(fieldLabel->mapTo(meshPanel,QPoint{}),fieldLabel->size());
            return fieldLabel->width()>0 && fieldLabel->height()>0 && !fieldLabel->visibleRegion().isEmpty()
                && meshPanel->rect().contains(inPanel);
        }),"AutoFlex material/mesh/name form label has no visible layout region");
    }
    meshScroll->ensureWidgetVisible(meshStatus,0,8);
    require(waitFor([&]{
        const QRect bounds(meshStatus->mapTo(meshScroll->viewport(),QPoint{}),meshStatus->size());
        return meshScroll->horizontalScrollBar()->maximum()==0 && !meshStatus->visibleRegion().isEmpty()
            && meshScroll->viewport()->rect().contains(bounds);
    }),"AutoFlex mesh status is clipped outside the default narrow parameter viewport");
    require(window.grab().save(output.filePath("autoflex-real-mesh.png")),"Cannot capture the real tetra mesh preview");
    // Select the actual generated end-face nodes using the same UI list the
    // user sees. No old/global node number is guessed from a new Netgen mesh.
    auto* attachmentEditor=widget<QWidget>(window,"nativeEditor_autoFlexBodies");
    int selectedNodes=0; QTimer nodePicker;
    QObject::connect(&nodePicker,&QTimer::timeout,&window,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        auto* list=dialog ? dialog->findChild<QListWidget*>("nativeFlexNodeList") : nullptr;
        if (!list) return;
        const auto coordinates=mesh.value("nodes").toArray();
        for(int i=0;i<list->count();++i) {
            const int index=list->item(i)->data(Qt::UserRole).toInt()-1;
            if(index>=0 && index<coordinates.size() && std::abs(coordinates[index].toArray()[0].toDouble())<1e-8) {
                list->item(i)->setCheckState(Qt::Checked); ++selectedNodes;
            }
        }
        dialog->accept();
    });
    nodePicker.start(20); widget<QPushButton>(*attachmentEditor,"nativeFlexSelectNodes")->click(); nodePicker.stop();
    require(selectedNodes>=3,"The real tetra mesh did not expose a selectable attachment face");
    number(*attachmentEditor,"connectionRadius",0.0);
    widget<QPushButton>(*attachmentEditor,"motionApplyButton")->click();
    window.command(Command::Save)->trigger();
    require(loadRigidProject(project,storedCad,storedModel,error),error); stored=*storedModel.nativeDocument;
    require(stored.entity("autoFlexBodies",13).value("attachPoints").toArray().size()==selectedNodes
        && stored.entity("autoFlexBodies",13).value("mesh").toObject().value("nodeIds").toArray().size()==nodes,
        "Selected real attachment nodes were not persisted with their exact mesh mapping");

    // Disabling the worker after the UI generated its mesh proves that both
    // the solver and a re-opened project consume that exact mesh snapshot.
    struct RestoreWorker {
        QByteArray previous=qgetenv("MXMOTION_MESH_WORKER"); bool existed=qEnvironmentVariableIsSet("MXMOTION_MESH_WORKER");
        ~RestoreWorker(){ if(existed)qputenv("MXMOTION_MESH_WORKER",previous);else qunsetenv("MXMOTION_MESH_WORKER"); }
    } restore;
    qputenv("MXMOTION_MESH_WORKER",output.filePath("mesh-worker-disabled-to-verify-reuse").toUtf8());
    NativeMotionInput input;
    require(prepareNativeMotion(storedCad,stored,0,input,error),error);
    require(input.geometry->flexBodies.size()==1 && input.geometry->flexBodies[0].sourceBrep.isEmpty()
        && int(input.geometry->flexBodies[0].nodes.size())==3*nodes,"Solver ignored the saved viewport mesh");
    require(input.geometry->flexBodies[0].attachGroups.size()==1 && input.geometry->flexBodies[0].attachPositions.empty(),
        "UI-selected attachment face was overwritten or supplemented by automatic joint attachments");
    RigidResult result;
    require(solveNativeMotion(input,result,error)==SolveStatus::Completed,"Saved-mesh AutoFlex numerical solve: "+error);
    require(result.flexBodies.size()==1 && result.flexBodies[0].nodePositions.size()==result.frames.size(),"Actual deformed node frames are missing");
    const auto& deformation=result.flexBodies[0]; double maximum=0;
    for (std::size_t i=0;i<deformation.referenceNodes.size();++i)
        maximum=std::max(maximum,std::abs(deformation.nodePositions.back()[i]-deformation.referenceNodes[i]));
    require(maximum>1e-4 && maximum<30,"Cantilever did not produce a finite, nonzero elastic deformation");
    for (const auto& channel:result.channels) if(channel.sourceType=="bodies" && channel.sourceId==0 && channel.variable=="Position")
        for(std::size_t i=0;i<channel.values.size();++i) require(std::abs(channel.values[i]-channel.values[i%3])<1e-7,
            "The clamped body's rigid pose moved instead of its flexible nodes deforming");
    solve(window,*tree,0);
    tree->setCurrentItem(node(*tree,ModelNode::Curves,0));
    addCurve(window,"autoFlexBodies",13,"ModalCoordinates",0);
    auto* plot=widget<RigidPlot>(window,"motionCurvePlot");
    require(plot->exportCurvesCsv(output.filePath("autoflex-modal-coordinate.csv"),error),error);
    auto* viewport=widget<OccViewport>(window,"occViewport");
    window.command(Command::Top)->trigger();
    tree->setCurrentItem(node(*tree,ModelNode::Animation,0));
    auto* timeline=widget<QSlider>(window,"motionTimeline");
    require(timeline->maximum()>=50,"AutoFlex timeline is missing accepted solver frames");
    timeline->setValue(0); QApplication::processEvents();
    const auto initial=viewport->grabFramebuffer();
    timeline->setValue(timeline->maximum());
    require(waitFor([&]{return viewport->grabFramebuffer()!=initial;}),"Modal node deformation did not reach the viewport animation");
    require(initial.save(output.filePath("autoflex-rest.png")) && viewport->grabFramebuffer().save(output.filePath("autoflex-deformed.png")),"Cannot capture AutoFlex deformation frames");
    window.command(Command::Save)->trigger();
    MotionWindow reopened; show(reopened);
    require(reopened.openRigidProject(project),"Cannot reopen a saved AutoFlex project");
    auto* reopenedTree=widget<QTreeWidget>(reopened,"mTreeView");
    reopenedTree->setCurrentItem(node(*reopenedTree,ModelNode::Native,13,"autoFlexBodies"));
    auto* reopenedEditor=widget<QWidget>(reopened,"nativeEditor_autoFlexBodies");
    require(widget<QLabel>(*reopenedEditor,"nativeFlexMeshStatus")->text().contains(QString::number(tets)),"Reopened editor lost its mesh status");
    solve(reopened,*reopenedTree,0);
    require(reopened.importMotionJson(motion,false),"Cannot reimport AutoFlex JSON over the existing CAD/volume mesh");
    solve(reopened,*reopenedTree,0); // The imported JSON has no mesh: use the existing model's actual volume mesh.
    const auto reused=output.filePath("autoflex-reused-mesh.motion.json");
    saveThroughUi(reopened,reused,Command::ExportMotionJson);
    NativeMotionDocument exported;
    require(readNativeMotionDocument(reused,exported,error)
        && exported.entity("autoFlexBodies",13).value("mesh").toObject().value("tetrahedra").toArray().size()==tets,
        "Import + solve failed to reuse/persist the model's existing volume mesh");
    MotionWindow lifecycle; show(lifecycle);
    require(lifecycle.openRigidProject(project),"Cannot open AutoFlex mesh lifecycle fixture");
    auto* lifecycleTree=widget<QTreeWidget>(lifecycle,"mTreeView");
    lifecycleTree->setCurrentItem(node(*lifecycleTree,ModelNode::Part));
    lifecycle.command(Command::GenerateMesh)->trigger();
    auto* generate=widget<QPushButton>(lifecycle,"generateMeshButton");
    generate->click();
    require(waitFor([&]{return generate->isEnabled();}),"A failed mesh worker did not return the UI to idle");
    const auto lifecyclePath=output.filePath("autoflex-mesh-lifecycle.mxmotion");
    saveThroughUi(lifecycle,lifecyclePath);
    CadDocument lifecycleCad; RigidModel lifecycleModel;
    require(loadRigidProject(lifecyclePath,lifecycleCad,lifecycleModel,error),error);
    auto lifecycleFlex=lifecycleModel.nativeDocument->entity("autoFlexBodies",13);
    require(lifecycleFlex.value("mesh").toObject().value("nodes")==mesh.value("nodes")
        && !lifecycleFlex.value("attachmentMeshChanged").toBool(),"Failed remeshing overwrote a valid mesh or attachment mapping");
    if(restore.existed)qputenv("MXMOTION_MESH_WORKER",restore.previous);else qunsetenv("MXMOTION_MESH_WORKER");
    lifecycle.command(Command::GenerateMesh)->trigger();
    widget<QDoubleSpinBox>(lifecycle,"meshElementSize")->setValue(.75);
    generate->click();
    auto* worker=lifecycle.findChild<QProcess*>();
    require(worker && waitFor([&]{return worker->state()==QProcess::Running;}),"Real Netgen worker did not start for cancellation check");
    widget<QPushButton>(lifecycle,"cancelMeshButton")->click();
    require(waitFor([&]{return generate->isEnabled();}),"Cancelling the real Netgen worker did not return the UI to idle");
    lifecycle.command(Command::Save)->trigger();
    require(loadRigidProject(lifecyclePath,lifecycleCad,lifecycleModel,error),error);
    lifecycleFlex=lifecycleModel.nativeDocument->entity("autoFlexBodies",13);
    require(lifecycleFlex.value("mesh").toObject().value("nodes")==mesh.value("nodes")
        && !lifecycleFlex.value("attachmentMeshChanged").toBool(),"Cancelled remeshing overwrote the accepted mesh or manual attachment mapping");
    lifecycle.command(Command::RemoveMesh)->trigger();
    lifecycle.command(Command::Save)->trigger();
    require(loadRigidProject(lifecyclePath,lifecycleCad,lifecycleModel,error),error);
    lifecycleFlex=lifecycleModel.nativeDocument->entity("autoFlexBodies",13);
    require(lifecycleFlex.value("mesh").toObject().isEmpty() && lifecycleFlex.value("attachmentMeshChanged").toBool(),
        "Deleting the viewport mesh kept a stale AutoFlex mesh/mapping");
    NativeMotionInput invalidated;
    require(!prepareNativeMotion(lifecycleCad,*lifecycleModel.nativeDocument,0,invalidated,error) && error.contains("旧附着节点"),
        "Stale manual node IDs were accepted after mesh deletion");
    lifecycleTree->setCurrentItem(node(*lifecycleTree,ModelNode::Native,13,"autoFlexBodies"));
    auto* regenerate=widget<QWidget>(lifecycle,"nativeEditor_autoFlexBodies");
    widget<QPushButton>(*regenerate,"motionApplyButton")->click();
    require(waitFor([&]{return lifecycle.command(Command::Solve)->isEnabled() && lifecycle.command(Command::MeshDisplay)->isChecked();},120000),
        "AutoFlex did not regenerate its deleted real mesh");
    auto* regenerated=widget<QWidget>(lifecycle,"nativeEditor_autoFlexBodies");
    widget<QPushButton>(*regenerated,"nativeFlexAutoAttach")->click();
    widget<QPushButton>(*regenerated,"motionApplyButton")->click();
    lifecycle.command(Command::Save)->trigger();
    require(loadRigidProject(lifecyclePath,lifecycleCad,lifecycleModel,error)
        && prepareNativeMotion(lifecycleCad,*lifecycleModel.nativeDocument,0,invalidated,error),error);
    require(!lifecycleModel.nativeDocument->entity("autoFlexBodies",13).value("attachmentMeshChanged").toBool()
        && invalidated.geometry->flexBodies.front().attachGroups.empty() && !invalidated.geometry->flexBodies.front().attachPositions.empty(),
        "Explicitly returning to automatic attachment did not clear stale node mappings");
    log("AutoFlex mesh lifecycle: failed worker and cancelled real Netgen preserve accepted mesh; deletion invalidates manual node IDs; regeneration and explicit automatic attachment recover cleanly");

    // Renumber the actual Netgen nodes and update every cell consistently.
    // This is the same physical mesh, but an unchanged numeric attachment
    // index now names a different position: a bounds check cannot catch it.
    FemMesh originalMesh; require(readFemMeshJson(mesh,originalMesh,error),error);
    FemMesh replacement=originalMesh; std::reverse(replacement.nodes.begin(),replacement.nodes.end());
    const auto last=std::uint32_t(replacement.nodes.size()-1);
    for(auto& cell:replacement.tetrahedra)for(auto& index:cell)index=last-index;
    for(auto& cell:replacement.surface)for(auto& index:cell)index=last-index;
    QJsonArray oldGroups,newGroups; bool differentAttachmentPosition=false;
    for(const auto& group:input.geometry->flexBodies.front().attachGroups) {
        QJsonArray oldGroup,newGroup;
        for(int index:group) {
            require(index>=0 && std::size_t(index)<replacement.nodes.size(),"External replacement fixture must retain valid old index numbers");
            differentAttachmentPosition |= originalMesh.nodes[index]!=replacement.nodes[index];
            oldGroup.append(index); newGroup.append(int(last)-index);
        }
        oldGroups.append(oldGroup); newGroups.append(newGroup);
    }
    require(differentAttachmentPosition && !oldGroups.isEmpty(),"External replacement fixture did not change old attachment positions");
    const auto externalVol=output.filePath("autoflex-renumbered-real-mesh.vol");
    writeVolumeFixture(externalVol,replacement);
    FemMesh checkedVol; require(readNetgenMesh(externalVol,MeshKind::Volume,checkedVol,error),error);
    require(checkedVol.nodes==replacement.nodes && checkedVol.tetrahedra==replacement.tetrahedra,
        "Real external VOL did not retain the intended node renumbering");
    auto externalFlex=stored.entity("autoFlexBodies",13);
    externalFlex.insert("attachGroups",oldGroups); externalFlex.insert("attachPoints",QJsonArray{});
    const auto motionWithFlex=[&](const QJsonObject& value) {
        auto fixture=stored; auto data=fixture.root.value("motionData").toObject();
        data.insert("autoFlexBodies",QJsonArray{value}); fixture.root.insert("motionData",data); return fixture;
    };
    const auto externalMotion=output.filePath("autoflex-external-replacement.motion.json");
    require(writeNativeMotionDocument(externalMotion,motionWithFlex(externalFlex),error),error);
    MotionWindow external; show(external);
    require(external.importCad(step) && external.importMotionJson(externalMotion,false),"Cannot open external-mesh replacement fixture");
    auto* externalTree=widget<QTreeWidget>(external,"mTreeView");
    externalTree->setCurrentItem(node(*externalTree,ModelNode::Native,13,"autoFlexBodies"));
    auto* externalEditor=widget<QWidget>(external,"nativeEditor_autoFlexBodies");
    widget<QLineEdit>(*externalEditor,"native_meshFile")->setText(externalVol);
    widget<QPushButton>(*externalEditor,"motionApplyButton")->click();
    const auto externalProject=output.filePath("autoflex-external-replacement.mxmotion");
    saveThroughUi(external,externalProject);
    CadDocument externalCad; RigidModel externalModel;
    require(loadRigidProject(externalProject,externalCad,externalModel,error),error);
    auto replaced=externalModel.nativeDocument->entity("autoFlexBodies",13);
    require(replaced.value("meshFile").toString().isEmpty() && replaced.value("attachmentMeshChanged").toBool()
        && replaced.value("attachGroups").toArray()==oldGroups
        && replaced.value("mesh").toObject().value("nodes")==femMeshJson(checkedVol).value("nodes"),
        "Replacing external VOL silently reused old in-range attachment indices or discarded their data");
    NativeMotionInput externalInput;
    require(!prepareNativeMotion(externalCad,*externalModel.nativeDocument,0,externalInput,error) && error.contains("旧附着节点"),
        "A real external VOL replacement with old in-range node groups was allowed to solve");
    auto* replacementEditor=widget<QWidget>(external,"nativeEditor_autoFlexBodies");
    widget<QPushButton>(*replacementEditor,"nativeFlexAutoAttach")->click();
    widget<QPushButton>(*replacementEditor,"motionApplyButton")->click();
    external.command(Command::Save)->trigger();
    require(loadRigidProject(externalProject,externalCad,externalModel,error)
        && prepareNativeMotion(externalCad,*externalModel.nativeDocument,0,externalInput,error),error);
    require(externalInput.geometry->flexBodies.front().attachGroups.empty()
        && !externalInput.geometry->flexBodies.front().attachPositions.empty(),"Explicit automatic attachment did not recover the imported replacement mesh");
    solve(external,*externalTree,0);

    // First loading a meshFile with groups already defined for that mesh is
    // valid. It must not be mistaken for replacing an existing embedded mesh.
    externalFlex.remove("mesh"); externalFlex.remove("meshSourceFile"); externalFlex.remove("attachmentMeshChanged");
    externalFlex.insert("meshFile",externalVol); externalFlex.insert("attachGroups",newGroups);
    const auto firstExternalMotion=output.filePath("autoflex-first-external.motion.json");
    require(writeNativeMotionDocument(firstExternalMotion,motionWithFlex(externalFlex),error),error);
    require(external.importMotionJson(firstExternalMotion,false),"Cannot import a first external mesh with its own attachment groups");
    externalTree->setCurrentItem(node(*externalTree,ModelNode::Native,13,"autoFlexBodies"));
    auto* firstExternalEditor=widget<QWidget>(external,"nativeEditor_autoFlexBodies");
    widget<QPushButton>(*firstExternalEditor,"motionApplyButton")->click();
    external.command(Command::Save)->trigger();
    require(loadRigidProject(externalProject,externalCad,externalModel,error)
        && prepareNativeMotion(externalCad,*externalModel.nativeDocument,0,externalInput,error),error);
    require(!externalModel.nativeDocument->entity("autoFlexBodies",13).value("attachmentMeshChanged").toBool()
        && externalModel.nativeDocument->entity("autoFlexBodies",13).value("attachGroups").toArray()==newGroups,
        "First external mesh loading incorrectly invalidated groups that belong to that mesh");
    log("AutoFlex external VOL: renumbered real Netgen mesh rejects old in-range attachment groups, preserves them for reselection, explicit automatic attachment solves, first external mesh with its own groups stays valid; narrow mesh status fully visible");
    log(QStringLiteral("AutoFlex real STEP/JSON import -> material 42 editing -> save/Netgen preview -> %1 nodes/%2 tets persisted -> Modal solve -> %3 mm elastic displacement -> visible node animation -> reopen/solve and JSON reimport/existing-mesh solve without mesh worker passed")
        .arg(nodes).arg(tets).arg(maximum,0,'g',8));
}
}

int runMotionOccNativeDesktopSmoke(QApplication&, const QString& outputDirectory)
{
    using namespace MotionOcc;
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QDir output(outputDirectory);
    output.mkpath(".");
    QFile report(output.filePath("native-desktop-smoke.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 2;
    const auto log = [&](const QString& text) { report.write(text.toUtf8()); report.write("\n"); report.flush(); };
    try {
        QString cadPath, motionPath;
        makeFixture(output.absolutePath(), cadPath, motionPath);
        MotionWindow window;
        show(window);
        require(window.importCad(cadPath), "Public CAD import failed");
        require(window.importMotionJson(motionPath, false), "Public original JSON import or geometric binding failed");
        auto* tree = widget<QTreeWidget>(window, "mTreeView");
        require(node(*tree, ModelNode::Native, 0, "bodies") && node(*tree, ModelNode::Native, 37, "bodies"),
                "Nonsequential native body IDs were replaced by CAD part IDs");
        requireTreeIcons(*tree);
        requireOriginalIcon(node(*tree, ModelNode::Native, 0, "bodies"), ":/mMainWidgets/Icon/Motion/movingBody.png");
        requireOriginalIcon(node(*tree, ModelNode::Native, 37, "bodies"), ":/mMainWidgets/Icon/Motion/FixedDeputy.png");
        const auto* originalJoint = node(*tree, ModelNode::Native, 23, "joints");
        require(originalJoint, "Imported revolute joint is missing from the model tree");
        requireOriginalIcon(originalJoint->parent(),
            ":/mMainWidgets/Icon/BrowserTree/motion_connectorParentItem.png");
        requireOriginalIcon(node(*tree, ModelNode::Animation, 0), ":/mMainWidgets/Icon/post/animation.png");
        requireOriginalIcon(node(*tree, ModelNode::Curves, 0), ":/mMainWidgets/Icon/BrowserTree/motion_curveChildItem.png");
        log("Public STEP + original JSON import: two bodies, native IDs 0/37, geometry IDs 901/205, scenario ID 0");
        window.command(Command::AddSoftStop)->trigger();
        auto* stop = widget<QWidget>(window, "nativeEditor_softStops");
        widget<QLineEdit>(*stop, "native_name")->setText(QStringLiteral("桌面止挡"));
        choose(*stop, "bodyIId", 0); choose(*stop, "bodyJId", -1);
        vector(*stop, QStringLiteral("作用点 I"), 25,0,0); vector(*stop, QStringLiteral("作用点 J"), 0,0,-100);
        vector(*stop, QStringLiteral("方向"), 1,0,0);
        number(*stop, "stopPosition", 20); number(*stop, "stiffness", 0.25); number(*stop, "damping", 0.001);
        widget<QPushButton>(*stop, "motionApplyButton")->click();
        require(node(*tree, ModelNode::Native, 1, "softStops"), "New stop parameters did not commit");
        window.command(Command::AddTensionCable)->trigger();
        auto* cable = widget<QWidget>(window, "nativeEditor_tensionCables");
        widget<QLineEdit>(*cable, "native_name")->setText(QStringLiteral("桌面只拉绳索"));
        choose(*cable, "bodyIId", 0); choose(*cable, "bodyJId", -1);
        vector(*cable, QStringLiteral("作用点 I"), 25,0,0); vector(*cable, QStringLiteral("作用点 J"), 0,0,-100);
        number(*cable, "freeLength", 95); number(*cable, "stiffness", 0.01); number(*cable, "damping", 0.001);
        widget<QPushButton>(*cable, "motionApplyButton")->click();
        require(node(*tree, ModelNode::Native, 1, "tensionCables"), "New tension cable parameters did not commit");
        selectSpringType(window);
        auto* spring = widget<QWidget>(window, "nativeEditor_springs");
        widget<QLineEdit>(*spring, "native_name")->setText(QStringLiteral("桌面体点弹簧"));
        choose(*spring, "bodyIId", 0); choose(*spring, "bodyJId", -1);
        auto* visiblePoint = vector(*spring, QStringLiteral("作用点 I"), 15,0,0);
        vector(*spring, QStringLiteral("作用点 J"), 0,0,-100);
        number(*spring, "freeValue", 100); number(*spring, "installValue", 100); number(*spring, "stiffness", 0.02);
        number(*spring, "damping", 0.001);
        auto* ancestor = spring->parentWidget();
        while (ancestor && !qobject_cast<QScrollArea*>(ancestor)) ancestor = ancestor->parentWidget();
        auto* nativeScroll = qobject_cast<QScrollArea*>(ancestor);
        require(nativeScroll && nativeScroll->objectName() == QStringLiteral("nativeParameterScroll"),
            "Native parameter editor has no scroll area preserving natural field heights");
        nativeScroll->ensureWidgetVisible(visiblePoint, 0, 12);
        require(waitFor([&] {
            return !widget<QLineEdit>(*visiblePoint, "native_x")->visibleRegion().isEmpty()
                && !widget<QLineEdit>(*visiblePoint, "native_y")->visibleRegion().isEmpty()
                && !widget<QLineEdit>(*visiblePoint, "native_z")->visibleRegion().isEmpty();
        }), "Native XYZ coordinates cannot be brought visibly into the parameter column");
        require(window.grab().save(output.filePath("native-parameters.png")), "Could not capture native parameter editor");
        widget<QPushButton>(*spring, "motionApplyButton")->click();
        require(node(*tree, ModelNode::Native, 1, "springs"), "Body-point spring parameters did not commit");
        const int iconCount = requireTreeIcons(*tree);
        tree->scrollToTop();
        QApplication::processEvents();
        require(tree->grab().save(output.filePath("native-tree-icons.png")), "Could not capture the restored model tree icons");
        log(QStringLiteral("Original model tree icons: %1 nonblank rendered icons, including fixed/moving bodies, groups, results and new force elements").arg(iconCount));
        const auto projectPath = output.filePath("native-desktop.mxmotion");
        saveThroughUi(window, projectPath);
        CadDocument storedCad;
        RigidModel storedModel;
        QString error;
        require(loadRigidProject(projectPath, storedCad, storedModel, error) && storedModel.nativeDocument,
            "Native project did not round trip: " + error);
        const auto& document = *storedModel.nativeDocument;
        require(document.entity("softStops", 1).value("stopPosition").toDouble() == 20
            && document.entity("tensionCables", 1).value("freeLength").toDouble() == 95
            && document.entity("springs", 1).value("springType").toString() == "BodyPoint",
            "Stop, cable or body-point spring fields were lost on save");
        require(document.bodyPartBindings.contains(0) && document.bodyPartBindings.contains(37), "Native zero-ID geometry bindings were lost");
        log("New stop/cable/body-point spring commands: edited parameters, references and project round trip passed");
        solve(window, *tree, 0);
        requireTreeIcons(*tree); // Solving rebuilds the tree and enables the result leaves.
        tree->setCurrentItem(node(*tree, ModelNode::Curves, 0));
        addCurve(window, "drives", 9, "ImposedValue", 0);
        addCurve(window, "tensionCables", 1, "Magnitude", 0);
        addCurve(window, "softStops", 1, "Engaged", 0);
        addCurve(window, "springs", 1, "Deformation", 0);
        auto* plot = widget<RigidPlot>(window, "motionCurvePlot");
        auto* chart = widget<QwtPlot>(*plot, "motionQwtPlot");
        const auto curves = chart->itemList(QwtPlotItem::Rtti_PlotCurve);
        require(curves.size() == 4, "Native explicit multi-curve workflow did not display four channels");
        auto* drive = static_cast<QwtPlotCurve*>(curves.front());
        require(drive->dataSize() >= 100 && std::abs(drive->sample(drive->dataSize()-1).y()-90) < 0.01,
            "Native drive curve does not match the real 90 degree solve");
        require(plot->exportCurvesCsv(output.filePath("native-selected-curves.csv"), error), "Native selected curve CSV failed: " + error);
        auto* viewport = widget<OccViewport>(window, "occViewport");
        window.command(Command::Top)->trigger();
        // Leave room for the rotor's full swept envelope while keeping the
        // camera fixed throughout playback and the image comparisons.
        const auto center = viewport->rect().center();
        QWheelEvent zoomOut(center, viewport->mapToGlobal(center), {}, {0, -360},
            Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(viewport, &zoomOut);
        tree->setCurrentItem(node(*tree, ModelNode::Animation, 0));
        auto* timeline = widget<QSlider>(window, "motionTimeline");
        require(timeline->isEnabled() && timeline->maximum() >= 100, "Native result animation did not expose physical frames");
        timeline->setValue(0);
        QApplication::processEvents();
        const auto first = viewport->grabFramebuffer();
        require(!first.isNull(), "Native animation has no framebuffer");
        timeline->setValue(timeline->maximum());
        require(waitFor([&] { return viewport->grabFramebuffer() != first; }), "Native solved poses did not move the CAD geometry");
        require(viewport->grabFramebuffer().save(output.filePath("native-animation.png")), "Could not save native animation framebuffer");
        require(window.grab().save(output.filePath("native-result-window.png")), "Could not capture native result window");
        widget<QPushButton>(window, "motionStopButton")->click();
        require(timeline->value() == 0, "Native animation Stop did not return to the first sample");
        widget<QPushButton>(window, "motionPlayButton")->click();
        require(waitFor([&] { return timeline->value() > 0; }), "Native animation timer did not advance");
        widget<QPushButton>(window, "motionPauseButton")->click();
        const int paused = timeline->value();
        QEventLoop pause; QTimer::singleShot(80, &pause, &QEventLoop::quit); pause.exec();
        require(timeline->value() == paused, "Native Pause continued changing the frame");
        window.command(Command::Save)->trigger();
        require(loadRigidProject(projectPath, storedCad, storedModel, error)
            && storedModel.scenarios.size() == 1 && storedModel.scenarios.front().curveRequests.size() == 4,
            "Native curve requests were not retained by the ordinary Save command");
        const auto& savedCurve = storedModel.scenarios.front().curveRequests.front();
        require(savedCurve.metric == RigidCurveMetric::NativeChannel && savedCurve.sourceType == "drives"
            && savedCurve.sourceId == 9 && savedCurve.variable == "ImposedValue",
            "Saved native curve uses a transient menu index instead of a stable result key");
        const auto exportedPath = output.filePath("native-edited.motion.json");
        saveThroughUi(window, exportedPath, Command::ExportMotionJson);
        NativeMotionDocument exported;
        require(readNativeMotionDocument(exportedPath, exported, error), "Original JSON export could not be reimported: " + error);
        RigidModel imported;
        imported.nativeDocument = std::make_shared<NativeMotionDocument>(exported);
        syncNativeMotionModel(imported);
        require(imported.scenarios.size() == 1 && imported.scenarios.front().curveRequests.size() == 4
            && imported.scenarios.front().curveRequests.front().sourceId == 9
            && imported.scenarios.front().curveRequests.front().variable == "ImposedValue",
            "Original JSON export/reimport dropped the selected native curves");
        log("Real native solve + four requested curves/CSV + frame-indexed CAD animation + play/pause/stop passed");
        const auto examplePath = output.filePath("native-forces.mxmotion");
        QFile::remove(examplePath); // Retain the generated, verified four-curve example before the deletion regression.
        require(QFile::copy(projectPath, examplePath), "Could not retain the generated native force example");
        removeThroughTree(window, *tree, "softStops", 1);
        require(node(*tree, ModelNode::Native, 1, "tensionCables") && node(*tree, ModelNode::Native, 1, "springs"),
            "Deleting one native source removed another collection with the same ID");
        window.command(Command::Save)->trigger();
        require(loadRigidProject(projectPath, storedCad, storedModel, error)
            && storedModel.nativeDocument->collection("softStops").isEmpty()
            && storedModel.scenarios.front().curveRequests.size() == 3,
            "Native source removal did not clean only its own saved curve request");
        log("Native source deletion with saved curves: removed only the stop and its display request, preserved same-ID cable/spring");
        autoFlexWorkflow(output,log);
        const auto realCad = qEnvironmentVariable("MX_NATIVE_SMOKE_CAD"), realMotion = qEnvironmentVariable("MX_NATIVE_SMOKE_MOTION");
        if (!realCad.isEmpty() || !realMotion.isEmpty()) {
            require(!realCad.isEmpty() && !realMotion.isEmpty(), "Provide both real-case CAD and motion environment paths");
            MotionWindow real;
            show(real);
            require(real.importCad(realCad) && real.importMotionJson(realMotion, false), "Real supplied case public import failed");
            auto* realTree = widget<QTreeWidget>(real, "mTreeView");
            auto* scenario = node(*realTree, ModelNode::Scenario);
            require(scenario, "Real supplied case has no scenario");
            const int id = scenario->data(0, Qt::UserRole).toInt();
            solve(real, *realTree, id, 120000);
            realTree->setCurrentItem(node(*realTree, ModelNode::Animation, id));
            auto* slider = widget<QSlider>(real, "motionTimeline");
            require(slider->isEnabled() && slider->maximum() > 0, "Real supplied case has no animation samples");
            NativeMotionDocument source;
            require(readNativeMotionDocument(realMotion,source,error),error);
            int movingBody=-1;
            for(const auto& value:source.collection("bodies")) {
                const auto body=value.toObject();if(!body.value("isFixed").toBool()){movingBody=body.value("id").toInt();break;}
            }
            if(movingBody>=0) {
                realTree->setCurrentItem(node(*realTree,ModelNode::Curves,id));
                for(int component=0;component<3;++component)addCurve(real,"bodies",movingBody,"Displacement",component);
                auto* actualPlot=widget<RigidPlot>(real,"motionCurvePlot");
                auto* actualChart=widget<QwtPlot>(*actualPlot,"motionQwtPlot");
                const auto actualCurves=actualChart->itemList(QwtPlotItem::Rtti_PlotCurve);
                require(actualCurves.size()>=3,"Real supplied displacement components did not reach Qwt");
                for(const auto* item:actualCurves) {
                    const auto* curve=static_cast<const QwtPlotCurve*>(item);
                    require(int(curve->dataSize())==slider->maximum()+1,"Real supplied curve and animation have different accepted-frame counts");
                    double previous=-1;
                    for(std::size_t sample=0;sample<curve->dataSize();++sample){const auto p=curve->sample(sample);
                        require(std::isfinite(p.x())&&std::isfinite(p.y())&&p.x()>previous,"Real supplied curve has nonfinite/nonmonotonic samples");previous=p.x();}
                }
                require(actualPlot->exportCurvesCsv(output.filePath("native-real-case-displacement.csv"),error),error);
                QApplication::processEvents();
                require(real.grab().save(output.filePath("native-real-case-curves.png")),"Could not capture original-case curves");
                realTree->setCurrentItem(node(*realTree,ModelNode::Animation,id));
            }
            slider->setValue(slider->maximum()/2);
            QApplication::processEvents();
            require(real.grab().save(output.filePath("native-real-case.png")), "Could not capture real supplied case");
            log("Real supplied CAD + original JSON import, solve to actual termination, displacement curves/CSV and frame-synchronized animation passed: " + realMotion);
        }
        log("Native desktop regression passed");
        return 0;
    } catch (const std::exception& error) {
        log(QStringLiteral("FAIL: %1").arg(QString::fromUtf8(error.what())));
        return 1;
    }
}
