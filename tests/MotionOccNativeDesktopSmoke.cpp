#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"
#include "NativeMotionGeometry.h"
#include "mFoldSection.h"

#include <BRep_Builder.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <DockManager.h>
#include <DockWidget.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
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
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProcess>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QToolButton>
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
        // QFileDialog::selectFile() leaves the focused name field untouched
        // once the widget dialog is visible, so on Windows accepting right
        // away keeps the initial "model.mxmotion" name and saves the wrong
        // file. Write the absolute target into the field itself and accept
        // only once the dialog really reports that path as its selection.
        if (auto* filename = dialog->findChild<QLineEdit*>("fileNameEdit")) filename->setText(path);
        if (!dialog->selectedFiles().contains(path)) return;
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

void clickViewport(OccViewport& viewport, const std::array<double,3>& position)
{
    const QPointF pixel=viewport.projectPoint(position);
    require(viewport.rect().contains(pixel.toPoint()),"Geometric test target is outside the visible viewport");
    QMouseEvent hover(QEvent::MouseMove,pixel,Qt::NoButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&viewport,&hover);
    QMouseEvent press(QEvent::MouseButtonPress,pixel,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,pixel,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QApplication::sendEvent(&viewport,&press);QApplication::sendEvent(&viewport,&release);
    QApplication::processEvents();
}

void escapePick(OccViewport& viewport)
{
    QKeyEvent escape(QEvent::KeyPress,Qt::Key_Escape,Qt::NoModifier);
    QApplication::sendEvent(&viewport,&escape);QApplication::processEvents();
    require(!viewport.isPicking(),"Esc did not end the active field picking session");
}

void selectPickMode(QWidget& editor,const QString& path,const QString& mode)
{
    auto* combo=widget<QComboBox>(editor,"native_pickMode_"+path);
    const int index=combo->findData(mode);require(index>=0,"Original picker mode is missing: "+mode);
    combo->setCurrentIndex(index);
}

void requireVector(QWidget& editor,const QString& path,const std::array<double,3>& expected)
{
    auto* box=widget<QComboBox>(editor,"native_pickMode_"+path)->parentWidget();
    for(int i=0;i<3;++i){const auto value=widget<QLineEdit>(*box,QStringLiteral("native_")+QChar('x'+i))->text().toDouble();
        require(std::isfinite(value)&&std::abs(value-expected[i])<1e-6,
            QStringLiteral("Picker %1 axis %2: %3 != %4").arg(path).arg(i).arg(value,0,'g',17).arg(expected[i],0,'g',17));}
}

void originalPickerAndMassWorkflow(const QDir& output,const QString& cadPath,const QString& motionPath,
                                  const std::function<void(const QString&)>& log)
{
    // Independent window: these edits deliberately move the joint pivot and
    // change the mass. They must not alter the original 90-degree solve fixture.
    MotionWindow window;show(window);
    require(window.importCad(cadPath)&&window.importMotionJson(motionPath,false),"Picker/mass fixture import failed");
    auto* tree=widget<QTreeWidget>(window,"mTreeView");
    auto* viewport=widget<OccViewport>(window,"occViewport");
    window.command(Command::Top)->trigger();QApplication::processEvents();
    int leakedPartPicks=0;
    QObject::connect(viewport,&OccViewport::partPicked,&window,[&](int){++leakedPartPicks;});
    tree->setCurrentItem(node(*tree,ModelNode::Native,23,"joints"));
    QPointer<QWidget> joint=widget<QWidget>(window,"nativeEditor_joints");
    selectPickMode(*joint,"origin","vertex");
    widget<QPushButton>(*joint,"native_pick_origin")->click();
    require(viewport->isPicking(),"Joint origin field did not activate viewport picking");
    clickViewport(*viewport,{30,5,4});
    require(joint&&!joint->isHidden()&&leakedPartPicks==0,"Field picking leaked a part selection or closed the joint editor");
    requireVector(*joint,"origin",{30,5,4});
    widget<QPushButton>(*joint,"native_pick_origin")->click();

    selectPickMode(*joint,"axis","twoVertices");
    widget<QPushButton>(*joint,"native_pick_axis")->click();
    clickViewport(*viewport,{0,5,4});clickViewport(*viewport,{30,5,4});
    requireVector(*joint,"axis",{1,0,0});
    escapePick(*viewport);requireVector(*joint,"axis",{0,0,1});
    widget<QPushButton>(*joint,"native_pick_axis")->click();
    clickViewport(*viewport,{0,5,4});clickViewport(*viewport,{30,5,4});
    widget<QPushButton>(*joint,"native_pick_axis")->click();
    widget<QPushButton>(*joint,"native_reverse_axis")->click();
    requireVector(*joint,"axis",{-1,0,0});
    require(leakedPartPicks==0,"Two-point direction selection leaked whole-part selection events");
    widget<QPushButton>(*joint,"motionApplyButton")->click();

    window.command(Command::AddMarker)->trigger();
    auto* marker=widget<QWidget>(window,"nativeEditor_markers");
    choose(*marker,"bodyId",0);
    selectPickMode(*marker,"localPosition","vertex");
    widget<QPushButton>(*marker,"native_pick_localPosition")->click();
    clickViewport(*viewport,{30,-5,4});
    requireVector(*marker,"localPosition",{30,-5,4});
    widget<QPushButton>(*marker,"native_pick_localPosition")->click();
    widget<QPushButton>(*marker,"motionApplyButton")->click();

    bool sensorTypeChosen=false;QTimer sensorDialog;
    QObject::connect(&sensorDialog,&QTimer::timeout,&window,[&]{
        auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        auto* combo=dialog->findChild<QComboBox*>();if(combo){combo->setCurrentIndex(0);sensorTypeChosen=true;dialog->accept();}
    });
    sensorDialog.start(20);window.command(Command::AddSensor)->trigger();sensorDialog.stop();
    require(sensorTypeChosen,"Body sensor chooser did not open");
    auto* sensor=widget<QWidget>(window,"nativeEditor_sensors");
    choose(*sensor,"targetBodyId",0);
    selectPickMode(*sensor,"referencePoint","vertex");
    widget<QPushButton>(*sensor,"native_pick_referencePoint")->click();clickViewport(*viewport,{30,5,4});
    requireVector(*sensor,"referencePoint",{30,5,4});
    widget<QPushButton>(*sensor,"native_pick_referencePoint")->click();
    widget<QPushButton>(*sensor,"motionApplyButton")->click();

    bool forceTypeChosen=false;QTimer forceDialog;
    QObject::connect(&forceDialog,&QTimer::timeout,&window,[&]{
        auto* dialog=qobject_cast<QInputDialog*>(QApplication::activeModalWidget());if(!dialog)return;
        auto* combo=dialog->findChild<QComboBox*>();if(combo){combo->setCurrentIndex(0);forceTypeChosen=true;dialog->accept();}
    });
    forceDialog.start(20);window.command(Command::AddVectorForce)->trigger();forceDialog.stop();
    require(forceTypeChosen,"Vector force chooser did not open");
    const auto applyLaw=[&](QWidget& editor,const QString& field,const QString& expression){
        bool used=false;QTimer editorDialog;
        QObject::connect(&editorDialog,&QTimer::timeout,&window,[&]{
            auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if(!dialog||dialog->objectName()!="nativeFunctionEditor")return;
            auto* modes=dialog->findChild<QComboBox*>("nativeFunctionMode");
            modes->setCurrentIndex(1);
            dialog->findChild<QPlainTextEdit*>("nativeFunctionExpression")->setPlainText(expression);
            used=true;dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons")->button(QDialogButtonBox::Ok)->click();
        });
        editorDialog.start(20);widget<QPushButton>(editor,"native_functionBuilder_"+field)->click();editorDialog.stop();
        require(used&&widget<QLineEdit>(editor,"native_"+field)->text()==expression,
            "Original force function builder did not apply its compiled expression to the real model field");
    };
    auto* force=widget<QWidget>(window,"nativeEditor_vectorForces");
    applyLaw(*force,"expression","2*sin(2*pi*t)");
    require(widget<QComboBox>(*force,"native_magnitudeMode")->currentData().toString()=="Expression",
        "Vector formula editor left the solver in inactive constant-magnitude mode");
    widget<QPushButton>(*force,"motionApplyButton")->click();
    const auto* vectorNode=node(*tree,ModelNode::Native,-1,"vectorForces");
    require(vectorNode,"Vector formula entity did not commit");
    const int vectorForceId=vectorNode->data(0,Qt::UserRole).toInt();
    window.command(Command::AddScalarForce)->trigger();
    auto* scalar=widget<QWidget>(window,"nativeEditor_scalarForces");
    applyLaw(*scalar,"forceFunction","2*x+v+t");
    widget<QPushButton>(*scalar,"motionApplyButton")->click();
    const auto* scalarNode=node(*tree,ModelNode::Native,-1,"scalarForces");
    require(scalarNode,"Scalar formula entity did not commit");
    const int scalarForceId=scalarNode->data(0,Qt::UserRole).toInt();

    tree->setCurrentItem(node(*tree,ModelNode::Native,0,"bodies"));
    auto* body=widget<QWidget>(window,"nativeEditor_bodies");
    auto* custom=widget<QCheckBox>(*body,"native_isCustomMassProps");
    require(!custom->isChecked(),"Original automatic body flag was lost");
    widget<QPushButton>(*body,"nativeRecalculateMass")->click();
    require(std::abs(widget<QLineEdit>(*body,"native_mass")->text().toDouble()-.01884)<1e-10,
        "Automatic body mass does not match 2400 mm3 of steel");
    requireVector(*body,"centerOfMass",{15,0,0});
    number(*body,"densityKgMm3",2.7e-6);
    QMetaObject::invokeMethod(widget<QLineEdit>(*body,"native_densityKgMm3"),"editingFinished",Qt::DirectConnection);
    require(std::abs(widget<QLineEdit>(*body,"native_mass")->text().toDouble()-.00648)<1e-10,
        "Automatic body mass did not respond to density editing");
    auto* parts=widget<QListWidget>(*body,"nativeBodyParts");QListWidgetItem* bound=nullptr;
    for(int i=0;i<parts->count();++i)if(parts->item(i)->checkState()==Qt::Checked){bound=parts->item(i);break;}
    require(bound,"Automatic body has no selected real geometry");
    bound->setCheckState(Qt::Unchecked);
    auto* massStatus=widget<QLabel>(*body,"nativeMassStatus");
    require(massStatus->property("massError").toBool(),"Automatic mass accepted an empty geometry selection");
    QPointer<QWidget> invalidBody=body;
    widget<QPushButton>(*body,"motionApplyButton")->click();
    require(invalidBody&&massStatus->property("massError").toBool(),"Invalid automatic mass committed instead of remaining editable");
    custom->setChecked(true);bound->setCheckState(Qt::Checked);
    require(!massStatus->property("massError").toBool(),"Switching to valid custom mass retained an obsolete automatic error");
    number(*body,"mass",1.234);
    widget<QPushButton>(*body,"motionApplyButton")->click();
    const auto project=output.filePath("native-original-pickers.mxmotion");saveThroughUi(window,project);
    CadDocument storedCad;RigidModel model;QString error;
    require(loadRigidProject(project,storedCad,model,error)&&model.nativeDocument,error);
    const auto& document=*model.nativeDocument;
    const auto savedJoint=document.entity("joints",23);
    const auto refs=savedJoint.value("occPickReferences").toObject();
    require(refs.value("origin").toArray().size()==1&&refs.value("axis").toArray().size()==2,
        "Point/direction topology identities were not persisted with their values");
    const auto reference=refs.value("origin").toArray().first().toObject();
    const auto* part=storedCad.part(reference.value("partId").toInt());require(part,"Saved picker CAD part is missing");
    TopTools_IndexedMapOfShape vertices;TopExp::MapShapes(part->shape,TopAbs_VERTEX,vertices);
    const int index=reference.value("index").toInt();require(index>=1&&index<=vertices.Extent(),"Saved vertex topology number is invalid");
    const auto vertex=BRep_Tool::Pnt(TopoDS::Vertex(vertices(index)));
    require(vertex.Distance(gp_Pnt(30,5,4))<1e-8,"Saved topology number names a different CAD vertex");
    require(document.entity("markers",1).value("localPosition").toObject()==xyz(30,-5,4),
        "Marker point lost the original global CAD coordinate contract");
    require(document.entity("sensors",1).value("referencePoint").toObject()==xyz(15,5,4),
        "Sensor pick was not converted from global CAD to the solver's COM-local point");
    require(document.entity("vectorForces",vectorForceId).value("magnitudeMode").toString()=="Expression"
        &&document.entity("vectorForces",vectorForceId).value("expression").toString()=="2*sin(2*pi*t)"
        &&document.entity("scalarForces",scalarForceId).value("forceFunction").toString()=="2*x+v+t",
        "Original vector/scalar formula fields did not survive project serialization");
    require(document.entity("bodies",0).value("isCustomMassProps").toBool()
        &&std::abs(document.entity("bodies",0).value("mass").toDouble()-1.234)<1e-12,
        "Custom mass fallback was recomputed or lost on save");
    require(window.grab().save(output.filePath("native-original-pickers.png")),"Cannot capture original picker workflow");

    MotionWindow reopened;show(reopened);require(reopened.openRigidProject(project),"Cannot reopen picked original model");
    auto* reopenedTree=widget<QTreeWidget>(reopened,"mTreeView");
    reopenedTree->setCurrentItem(node(*reopenedTree,ModelNode::Native,23,"joints"));
    auto* editor=widget<QWidget>(reopened,"nativeEditor_joints");
    requireVector(*editor,"origin",{30,5,4});requireVector(*editor,"axis",{-1,0,0});
    require(widget<QComboBox>(*editor,"native_pickMode_axis")->currentData().toString()=="twoVertices",
        "Reopening a model lost the direction pick mode");
    widget<QPushButton>(*editor,"native_pick_axis")->click();
    auto* reopenedViewport=widget<OccViewport>(reopened,"occViewport");
    require(reopenedViewport->pickSelection().size()==2,"Reentering the direction picker lost its selected topology");
    requireVector(*editor,"axis",{-1,0,0});escapePick(*reopenedViewport);requireVector(*editor,"axis",{-1,0,0});
    reopenedTree->setCurrentItem(node(*reopenedTree,ModelNode::Native,1,"sensors"));
    requireVector(*widget<QWidget>(reopened,"nativeEditor_sensors"),"referencePoint",{30,5,4});
    log("Original pickers: real viewport vertex -> joint origin/marker, two-vertex axis, reverse, Esc rollback, editor retention, no whole-part leakage, exact CAD topology + reversed direction restored after save/reopen");
    log("Sensor point: world CAD (30,5,4) picked, COM-local (15,5,4) stored, reopened field restored to world (30,5,4)");
    log("Original force builders: time-dependent vector force sets Expression mode; scalar force accepts actual x/v state law; both compiled fields persist in the native model");
    log("Automatic mass: actual 2400 mm3 solid -> 0.01884 kg steel / 0.00648 kg at 2.7e-6 density, global COM, invalid selection rejection and explicit custom mass fallback persisted");
}

void typedPanelWorkflow(const QDir& output, const QString& cadPath, const QString& motionPath,
                        const std::function<void(const QString&)>& log)
{
    NativeMotionDocument source; QString error;
    require(readNativeMotionDocument(motionPath,source,error),error);
    auto data = source.root.value("motionData").toObject();
    auto drives = data.value("drives").toArray(); auto drive = drives[0].toObject();
    drive.insert("controlMode","Torque"); drive.insert("effortLimit",10.0);
    drive.insert("vendorExtension",QJsonObject{{"calibration",17},{"tags",QJsonArray{"original","keep"}}});
    drives[0] = drive; data.insert("drives",drives);
    data.insert("sensors",QJsonArray{
        QJsonObject{{"id",101},{"name","body position"},{"sensorType","BodyDisplacement"},{"targetBodyId",0},
            {"referencePoint",xyz(15,5,4)},{"variable","Position"},{"componentMode","Z"},{"enabled",true},
            {"coordSystem","GlobalFixed"},{"vendorExtension",QJsonObject{{"note","retain measurement metadata"}}}},
        QJsonObject{{"id",102},{"name","reaction output"},{"sensorType","JointReaction"},{"targetJointId",23},
            {"variable","ReactionForce"},{"componentMode","Z"},{"enabled",true}},
        QJsonObject{{"id",103},{"name","actual motor angle"},{"sensorType","DriveActuation"},{"targetDriveId",9},
            {"variable","Coordinate"},{"componentMode","Z"},{"enabled",true}},
        QJsonObject{{"id",104},{"name","legacy angle variable"},{"sensorType","Angle"},
            {"bodyAId",0},{"bodyBId",37},{"axisBodyId",-1},{"axisDirection",xyz(0,0,1)},
            {"variable","AngularVelocity"},{"enabled",true}}});
    data.insert("controlInputs",QJsonArray{QJsonObject{{"id",7},{"name","target trajectory"},{"inputType","CustomCurve"},
        {"curvePoints",QJsonArray{QJsonArray{0,0},QJsonArray{1,45}}}}});
    data.insert("controllers",QJsonArray{QJsonObject{{"id",3},{"name","effort servo"},{"controllerType","PID"},
        {"targetDriveId",9},{"targetInputId",-1},{"feedbackSource","DriveDof"},{"feedbackType","Position"},
        {"feedbackSensorId",101},{"targetValue",0.0},{"kp",1.0},{"ki",0.0},{"kd",0.0},{"outputLimit",0.0},{"enabled",true},
        {"vendorExtension",QJsonObject{{"tuningRevision",3}}}}});
    auto scenarios = data.value("scenarios").toArray(); auto scenario = scenarios[0].toObject();
    scenario.insert("vendorExtension",QJsonObject{{"study","keep original"}}); scenarios[0]=scenario; data.insert("scenarios",scenarios);
    source.root.insert("motionData",data);
    const auto fixture = output.filePath("typed-panels.motion.json");
    require(writeNativeMotionDocument(fixture,source,error),error);
    MotionWindow window; show(window);
    require(window.importCad(cadPath) && window.importMotionJson(fixture,false),"Typed panel STEP/JSON import failed");
    auto* tree = widget<QTreeWidget>(window,"mTreeView");
    const auto select = [&](const QString& collection,int id) {
        auto* item = node(*tree, collection=="scenarios" ? ModelNode::Scenario : ModelNode::Native,id,
            collection=="scenarios" ? QString{} : collection);
        require(item,"Typed panel model item missing: "+collection);
        // Applying an edit closes its form while retaining the tree selection.
        // Re-enter through the existing selection-change path when revisiting
        // that same entity; setCurrentItem(sameItem) emits no Qt signal.
        if (tree->currentItem() == item) tree->setCurrentItem(nullptr);
        tree->setCurrentItem(item); QApplication::processEvents();
        return widget<QWidget>(window,"nativeEditor_"+collection);
    };
    const auto option = [](QWidget& editor,const QString& key,const QString& value) {
        auto* combo = widget<QComboBox>(editor,"native_"+key); const int index=combo->findData(value);
        require(index>=0,"Typed choice missing: "+key+"="+value); combo->setCurrentIndex(index);
    };
    const auto apply = [&](QWidget* editor) {
        QPointer<QWidget> previous=editor; widget<QPushButton>(*editor,"motionApplyButton")->click();
        require(waitFor([&]{return previous.isNull();}),"Typed editor did not commit: "+error);
    };
    auto* editor=select("scenarios",0);
    for (const auto& group:{"nativeScenarioAnalysis","nativeScenarioTime","nativeScenarioEnvironment","nativeScenarioPreset","nativeScenarioAdvanced"})
        require(widget<QGroupBox>(*editor,group),"Specialised scenario group missing");
    require(widget<QLabel>(*editor,"nativeScenarioFrameCount")->text().contains("101"),"Initial frame was omitted from expected frame count");
    number(*editor,"endTime",.105); number(*editor,"stepSize",.01);
    require(widget<QLabel>(*editor,"nativeScenarioFrameCount")->text().contains("12"),"Partial final step missing from frame estimate");
    option(*editor,"analysisType","Static");
    require(!widget<QGroupBox>(*editor,"nativeScenarioStatic")->isHidden()
        && !widget<QLineEdit>(*editor,"native_endTime")->isEnabled()
        && widget<QLabel>(*editor,"nativeScenarioFrameCount")->text().contains(QStringLiteral("1 帧"))
        && widget<QLabel>(*editor,"nativeTypedIssue")->text().contains("PID"),"Static scenario controls do not reflect one equilibrium frame / incompatible active PID");
    option(*editor,"analysisType","PreloadedDynamics");
    widget<QCheckBox>(*editor,"native_usePreset")->setChecked(true); option(*editor,"presetMode","FAST");
    require(!widget<QComboBox>(*editor,"native_solverType")->isEnabled()
        && !widget<QLineEdit>(*editor,"native_integratorTol")->isEnabled()
        && widget<QLabel>(*editor,"nativeScenarioEffectiveSettings")->text().contains("PSOR"),"Preset shadowed parameters look editable or actual rule missing");
    option(*editor,"presetMode","ROBUST");
    require(!widget<QLineEdit>(*editor,"native_solverTolerance")->isEnabled()
        && widget<QLabel>(*editor,"nativeScenarioEffectiveSettings")->text().contains("HHT"),"ROBUST active settings were not exposed");
    widget<QCheckBox>(*editor,"native_usePreset")->setChecked(false); option(*editor,"integratorType","HHT");
    require(widget<QComboBox>(*editor,"native_solverType")->isEnabled()
        && widget<QLineEdit>(*editor,"native_integratorAlpha")->isEnabled(),"Custom HHT controls do not enable meaningful parameters");
    require(waitFor([&] {
        auto* help=widget<QLabel>(*editor,"nativeScenarioAnalysisHelp");
        auto* environment=widget<QGroupBox>(*editor,"nativeScenarioEnvironment");
        auto* equilibrium=widget<QGroupBox>(*editor,"nativeScenarioStatic");
        auto* reverse=widget<QPushButton>(*editor,"native_reverse_gravity");
        const int reverseBottom=reverse->mapTo(environment,QPoint(0,reverse->height())).y();
        return help->height()>=help->heightForWidth(help->width())
            && reverseBottom<environment->height()
            && environment->geometry().bottom()<equilibrium->geometry().top();
    }),"Narrow scenario dock clips wrapped help or overlaps the gravity controls with the static section");
    require(window.grab().save(output.filePath("native-scenario-panel.png")),"Cannot capture scenario panel"); apply(editor);

    editor=select("sensors",101);
    require(widget<QComboBox>(*editor,"native_sensorType")->isEnabled(),"Sensor type is not editable");
    requireVector(*editor,"referencePoint",{30,5,4});
    option(*editor,"sensorType","Distance"); option(*editor,"variable","RelVelocity");
    require(widget<QGroupBox>(*editor,"nativeSensorBody")->isHidden()
        && !widget<QGroupBox>(*editor,"nativeSensorDistance")->isHidden()
        && widget<QGroupBox>(*editor,"nativeSensorCoordinates")->isHidden()
        && widget<QLabel>(*editor,"nativeSensorQuantity")->text().contains("mm/s"),"Distance sensor did not switch targets/variables/units");
    option(*editor,"sensorType","Angle");
    require(!widget<QGroupBox>(*editor,"nativeSensorAngle")->isHidden()
        && widget<QLabel>(*editor,"nativeSensorQuantity")->text().contains(QStringLiteral("°")),"Angle sensor does not show actual reference-axis contract");
    option(*editor,"sensorType","BodyVelocity");
    require(widget<QComboBox>(*editor,"native_variable")->findData("Position")<0,"Velocity sensor exposes incompatible displacement choice");
    option(*editor,"variable","AngularVelocity");
    require(widget<QLabel>(*editor,"nativeSensorQuantity")->text().contains(QStringLiteral("°/s")),"Angular velocity unit missing");
    option(*editor,"sensorType","BodyDisplacement");
    require(widget<QComboBox>(*editor,"native_variable")->currentData().toString()=="Position","Returning to sensor type lost its previous selection");
    requireVector(*editor,"referencePoint",{30,5,4});
    require(window.grab().save(output.filePath("native-sensor-panel.png")),"Cannot capture sensor panel"); apply(editor);

    editor=select("sensors",103);
    auto* variables=widget<QComboBox>(*editor,"native_variable");
    require(variables->findData("Coordinate")>=0 && variables->findData("Rate")>=0 && variables->findData("ImposedValue")<0,
        "Effort actuator sensor confused imposed motion with measured motion");
    option(*editor,"variable","Rate");
    require(widget<QLabel>(*editor,"nativeSensorQuantity")->text().contains(QStringLiteral("°/s")),"Actual angular rate sensor unit incorrect"); apply(editor);

    editor=select("controllers",3);
    require(widget<QLabel>(*editor,"nativePidUnits")->text().contains(QStringLiteral("N·mm"))
        && widget<QLabel>(*editor,"nativePidUnits")->text().contains(QStringLiteral("°")),"Effort PID units are not user degree / physical torque");
    option(*editor,"feedbackSource","Sensor"); choose(*editor,"feedbackSensorId",102);
    require(widget<QLabel>(*editor,"nativeTypedIssue")->text().contains(QStringLiteral("反力")),"Unsupported reaction feedback has no configuration-time explanation");
    widget<QPushButton>(*editor,"motionApplyButton")->click();
    require(editor->isVisible(),"Unsupported feedback configuration was committed");
    choose(*editor,"feedbackSensorId",104);
    require(widget<QLabel>(*editor,"nativeTypedIssue")->text().isEmpty()
        && widget<QLabel>(*editor,"nativePidUnits")->text().contains(QStringLiteral("目标与反馈：°；")),
        "Legacy Angle variable incorrectly blocks angle feedback or changes its unit");
    choose(*editor,"feedbackSensorId",101);
    require(widget<QLabel>(*editor,"nativeTypedIssue")->text().isEmpty(),"Valid kinematic feedback remains rejected");
    option(*editor,"targetSource","Input"); choose(*editor,"targetInputId",7);
    require(!widget<QLineEdit>(*editor,"native_targetValue")->isEnabled()
        && widget<QComboBox>(*editor,"native_targetInputId")->isEnabled(),"Trajectory target left constant field active");
    require(window.grab().save(output.filePath("native-pid-panel.png")),"Cannot capture PID panel"); apply(editor);
    editor=select("controllers",3); option(*editor,"targetSource","Constant"); option(*editor,"feedbackSource","DriveDof");
    number(*editor,"targetValue",45); number(*editor,"outputLimit",5); apply(editor);

    editor=select("sensors",104);
    require(widget<QComboBox>(*editor,"native_variable")->currentData().toString()=="Angle"
        && widget<QLabel>(*editor,"nativeSensorQuantity")->text().contains(QStringLiteral("单位 °"))
        && widget<QLabel>(*editor,"nativeTypedIssue")->text().isEmpty(),
        "Legacy Angle sensor did not canonicalise the ignored variable to actual angle");
    apply(editor);

    editor=select("drives",9);
    require(!widget<QGroupBox>(*editor,"nativeDriveLimits")->isHidden()
        && widget<QLabel>(*editor,"nativeDriveCommandUnits")->text().contains(QStringLiteral("N·mm")),"Effort actuator limits or units missing");
    option(*editor,"controlMode","Force");
    require(!widget<QLabel>(*editor,"nativeTypedIssue")->text().isEmpty(),"Rotational Force mismatch not diagnosed");
    widget<QPushButton>(*editor,"motionApplyButton")->click(); require(editor->isVisible(),"Force/rotation mismatch saved");
    option(*editor,"controlMode","Torque"); number(*editor,"effortLimit",12); number(*editor,"powerLimit",20); number(*editor,"noLoadSpeed",300); apply(editor);
    const auto project=output.filePath("native-typed-panels.mxmotion"); saveThroughUi(window,project);
    CadDocument storedCad; RigidModel model; require(loadRigidProject(project,storedCad,model,error) && model.nativeDocument,error);
    const auto& saved=*model.nativeDocument;
    require(saved.entity("sensors",104).value("variable").toString()=="Angle",
        "Legacy Angle variable was not canonicalised when saving the sensor");
    require(saved.entity("scenarios",0).value("analysisType").toString()=="PreloadedDynamics"
        && saved.entity("scenarios",0).value("vendorExtension")==scenario.value("vendorExtension"),"Scenario edit lost new mode or original extension");
    require(saved.entity("sensors",101).value("referencePoint").toObject()==xyz(15,5,4)
        && saved.entity("sensors",101).value("vendorExtension")==data.value("sensors").toArray()[0].toObject().value("vendorExtension"),
        "Sensor switching changed COM-local reference or extension metadata");
    require(saved.entity("controllers",3).value("targetInputId").toInt()!=7
        && saved.entity("controllers",3).value("targetInputId").toInt()==-1
        && saved.entity("controllers",3).value("targetValue").toDouble()==45
        && saved.entity("controllers",3).value("outputLimit").toDouble()==5
        && saved.entity("controllers",3).value("vendorExtension")==data.value("controllers").toArray()[0].toObject().value("vendorExtension"),
        "PID target-source switch did not persist exactly or lost extensions");
    require(saved.entity("drives",9).value("effortLimit").toDouble()==12
        && saved.entity("drives",9).value("powerLimit").toDouble()==20
        && saved.entity("drives",9).value("noLoadSpeed").toDouble()==300
        && saved.entity("drives",9).value("vendorExtension")==drive.value("vendorExtension"),"Actuator limits or original metadata lost on save");
    MotionWindow reopened; show(reopened); require(reopened.openRigidProject(project),"Typed panel project did not reopen");
    auto* reopenedTree=widget<QTreeWidget>(reopened,"mTreeView"); reopenedTree->setCurrentItem(node(*reopenedTree,ModelNode::Scenario,0));
    require(widget<QComboBox>(*widget<QWidget>(reopened,"nativeEditor_scenarios"),"native_analysisType")->currentData().toString()=="PreloadedDynamics",
        "Preloaded scenario did not restore its specialised controls");
    log("Typed panels: actual Scenario grouped controls, exact initial/final frame counts, preset/custom active settings, static/preload modes; sensor type/variable/target/unit switching; scalar PID feedback rejection, trajectory/constant targets, physical torque units and limits; original extensions and COM-local point preserved through save/reopen");
}

void loadExportWorkflow(MotionWindow& window,QTreeWidget& tree,const QDir& output,
                        const std::function<void(const QString&)>& log)
{
    auto* item=node(tree,ModelNode::Export,0);require(item&&!item->isDisabled(),"Solved native export tree item is missing");
    for (auto* parent=item->parent();parent;parent=parent->parent()) parent->setExpanded(true);
    tree.setCurrentItem(item);tree.scrollToItem(item);QApplication::processEvents();
    // Changing result panes queues a dock resize. Scroll after that layout has
    // settled, since an item visible at the old height may now be below it.
    require(waitFor([&]{tree.scrollToItem(item,QAbstractItemView::PositionAtCenter);
        const auto rect=tree.visualItemRect(item).intersected(tree.viewport()->rect()); return !rect.isEmpty()
        &&tree.itemAt(rect.center())==item;}),
        "Native export tree row is not visible for double-clicking");
    const QPointF pixel=tree.visualItemRect(item).intersected(tree.viewport()->rect()).center();
    QMouseEvent press(QEvent::MouseButtonPress,pixel,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease,pixel,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
    QMouseEvent twice(QEvent::MouseButtonDblClick,pixel,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
    QApplication::sendEvent(tree.viewport(),&press);QApplication::sendEvent(tree.viewport(),&release);
    QApplication::sendEvent(tree.viewport(),&twice);QApplication::sendEvent(tree.viewport(),&release);
    require(waitFor([&]{return window.findChild<QWidget*>("nativeLoadExportPanel")!=nullptr;}),
        "Double-clicking native result export did not open the original load workflow");
    auto* panel=widget<QWidget>(window,"nativeLoadExportPanel");
    auto* source=widget<QComboBox>(*panel,"nativeLoadSource");const int joint=source->findData("joints:23");
    require(joint>=0,"Load export did not expose the physical joint result source");source->setCurrentIndex(joint);
    widget<QComboBox>(*panel,"nativeLoadFrames")->setCurrentIndex(1);
    widget<QSpinBox>(*panel,"nativeLoadFrame")->setValue(50);
    widget<QSpinBox>(*panel,"nativeLoadNodeId")->setValue(42);
    auto* button=widget<QPushButton>(*panel,"nativeExportLoad");
    const auto save=[&](const QString& path){
        QFile::remove(path);bool chosen=false;QTimer dialogTimer;
        QObject::connect(&dialogTimer,&QTimer::timeout,&window,[&]{
            auto* dialog=qobject_cast<QFileDialog*>(QApplication::activeModalWidget());if(!dialog)return;
            dialog->selectFile(path);
            if (auto* filename=dialog->findChild<QLineEdit*>("fileNameEdit")) filename->setText(path);
            chosen=true;QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
        });
        dialogTimer.start(20);button->click();dialogTimer.stop();
        require(chosen&&QFile::exists(path),"Load export dialog did not produce the selected file: "+path);
        QFile file(path);require(file.open(QIODevice::ReadOnly),"Cannot read exported loads");return QString::fromUtf8(file.readAll());
    };
    widget<QComboBox>(*panel,"nativeLoadFormat")->setCurrentIndex(0);
    const auto csv=save(output.filePath("native-joint-load.csv"));
    const auto lines=csv.split('\n',Qt::SkipEmptyParts);require(lines.size()==2,"Specified-frame load CSV must contain exactly one data row");
    const auto header=lines[0].split(','),values=lines[1].split(',');
    require(header.size()==16&&values.size()==header.size()&&header[4]=="Time_s"&&header[10]=="Fx_N"&&header[15]=="Mz_Nmm",
        "Manual load CSV lost its declared time/force/moment units");
    require(values[3].toInt()==50&&std::abs(values[4].toDouble()-.5)<1e-12&&values[5].toInt()==42&&values[6]=="I",
        "Manual load UI did not apply the selected physical frame/node/end");
    bool nonzero=false;
    for(int i=7;i<values.size();++i){bool ok=false;const double value=values[i].toDouble(&ok);
        require(ok&&std::isfinite(value),"Exported load has a nonnumeric/nonfinite physical component");
        if(i>=10)nonzero|=std::abs(value)>1e-9;}
    require(nonzero,"Loaded rotating fixture exported a zero placeholder wrench");
    widget<QComboBox>(*panel,"nativeLoadFormat")->setCurrentIndex(1);
    const auto bdf=save(output.filePath("native-joint-load.bdf"));
    require(bdf.contains("$ Units: N, mm, N*mm, s; CID=0")&&bdf.contains("ENDDATA"),"BDF load export lacks units or a complete ending");
    int cards=0;
    for(const auto& line:bdf.split('\n',Qt::SkipEmptyParts))if(line.startsWith("FORCE,")||line.startsWith("MOMENT,")){
        const auto fields=line.split(',');require(fields.size()==8&&fields[1].toInt()==51&&fields[2].toInt()==42&&fields[3]=="0",
            "BDF UI export lost its frame load-set or target grid");
        const int start=line.startsWith("FORCE,")?10:13;
        for(int i=0;i<3;++i){bool ok=false;const double value=fields[i+5].toDouble(&ok);
            require(ok&&std::isfinite(value)&&std::abs(value-values[start+i].toDouble())<1e-10,
                "CSV and BDF physical load components differ at the same frame");}
        ++cards;
    }
    require(cards==2,"BDF did not write both FORCE and MOMENT cards");
    widget<QComboBox>(*panel,"nativeLoadEnd")->setCurrentIndex(1);
    widget<QComboBox>(*panel,"nativeLoadFormat")->setCurrentIndex(0);
    const auto opposite=save(output.filePath("native-joint-load-opposite.csv")).split('\n',Qt::SkipEmptyParts).at(1).split(',');
    require(opposite.size()==16&&opposite[6]=="J","Load end selection was not exported");
    for(int i=10;i<13;++i)require(std::abs(values[i].toDouble()+opposite[i].toDouble())<1e-8,
        "I/J export force components violate action/reaction");
    require(window.grab().save(output.filePath("native-load-export.png")),"Cannot capture the restored load export panel");
    log("Load export UI: double-click result tree -> actual joint frame 50 / 0.5 s -> finite nonzero physical CSV, matching FORCE/MOMENT BDF at grid 42, and I/J action-reaction sign verified");
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
    require(widget<QPushButton>(*meshPanel,"nativeInspectModes")->isEnabled(),"Saved AutoFlex has no modal inspection entry");
    auto* interfaceMode=widget<QComboBox>(*meshPanel,"native_interfaceMode");
    require(interfaceMode->currentData().toString()=="SharedBody" && interfaceMode->findData("Independent")>=0,
        "AutoFlex lost compatible shared-body mode or genuine independent-interface choice");
    interfaceMode->setCurrentIndex(interfaceMode->findData("Independent"));
    require(!widget<QPushButton>(*meshPanel,"nativeFlexInterfaceGroups")->isHidden()
        && widget<QLineEdit>(*meshPanel,"native_modeCount")->isEnabled()
        && widget<QLabel>(*meshPanel,"nativeFlexInterfaceInfo")->text().contains(QStringLiteral("全有限元")),
        "Independent mode does not expose grouping or labels reduced/full-order semantics incorrectly");
    interfaceMode->setCurrentIndex(interfaceMode->findData("SharedBody"));
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
    // Exercise the real viewport node picker before the separate list workflow.
    // A visible corner of the generated clamp face has an unambiguous node ID.
    auto* nodeEditor=widget<QWidget>(window,"nativeEditor_autoFlexBodies");
    auto* pickViewport=widget<OccViewport>(window,"occViewport");
    window.command(Command::Top)->trigger();QApplication::processEvents();
    const auto coordinates=mesh.value("nodes").toArray();int corner=-1;
    for(int i=0;i<coordinates.size();++i){const auto p=coordinates[i].toArray();
        if(p.size()==3&&std::abs(p[0].toDouble())<1e-8&&std::abs(p[1].toDouble()+4)<1e-8&&std::abs(p[2].toDouble()-3)<1e-8){corner=i;break;}}
    require(corner>=0,"Generated clamp face lost its exact CAD corner node");
    auto* attachmentTable=widget<QTableWidget>(*nodeEditor,"native_attachPoints");
    require(attachmentTable->rowCount()==0,"AutoFlex viewport picker fixture must initially use automatic attachment");
    widget<QPushButton>(*nodeEditor,"nativeFlexPickNodes")->click();
    require(pickViewport->isPicking(),"AutoFlex attachment field did not start node picking");
    QPointer<QWidget> retainedEditor=nodeEditor;
    clickViewport(*pickViewport,{0,-4,3});
    require(retainedEditor&&attachmentTable->rowCount()==1
        &&attachmentTable->item(0,0)->text().toInt()==corner+1,
        "AutoFlex viewport pick did not write the actual generated mesh node ID");
    escapePick(*pickViewport);
    require(attachmentTable->rowCount()==0,"Esc did not restore the original AutoFlex attachment selection");
    widget<QPushButton>(*nodeEditor,"nativeFlexPickNodes")->click();clickViewport(*pickViewport,{0,-4,3});
    require(window.grab().save(output.filePath("autoflex-node-picking.png")),"Cannot capture actual node picking");
    widget<QPushButton>(*nodeEditor,"nativeFlexPickNodes")->click();
    require(!pickViewport->isPicking()&&attachmentTable->rowCount()==1,"Finishing node picking lost the selected real mesh node");
    log(QStringLiteral("AutoFlex viewport node picking: real CAD corner maps to actual mesh node %1, field remains open, Esc rolls back, finished selection feeds the attachment editor").arg(corner+1));
    // Replacing the attachment mode must retire the previous session before
    // clearing its data. Otherwise Esc restores that session's old node IDs.
    widget<QPushButton>(*nodeEditor,"nativeFlexPickNodes")->click();
    require(pickViewport->isPicking(),"AutoFlex attachment picker did not resume its saved selection");
    widget<QPushButton>(*nodeEditor,"nativeFlexAutoAttach")->click();
    require(retainedEditor&&!pickViewport->isPicking()&&attachmentTable->rowCount()==0,
        "Switching to automatic attachment left an old viewport node session active");
    // This Esc probes the retired picking session itself. With no active
    // session the viewport ignores the key and the application-level
    // ClearSelection shortcut (Esc) legitimately closes the whole parameter
    // editor on an active desktop window. Keep that unrelated window command
    // from swallowing the key so the probe observes only the stale-session
    // rollback it is meant to catch.
    auto* clearSelection=window.command(Command::ClearSelection);
    const QKeySequence clearSelectionShortcut=clearSelection->shortcut();
    clearSelection->setShortcut(QKeySequence());
    escapePick(*pickViewport);
    clearSelection->setShortcut(clearSelectionShortcut);
    require(retainedEditor&&attachmentTable->rowCount()==0,
        "Esc restored stale manual node IDs after switching to automatic attachment");
    widget<QPushButton>(*nodeEditor,"nativeFlexPickNodes")->click();
    clickViewport(*pickViewport,{0,-4,3});
    require(pickViewport->isPicking()&&attachmentTable->rowCount()==1,
        "AutoFlex could not restart node picking after switching to automatic attachment");
    // Select the actual generated end-face nodes using the same UI list the
    // user sees while the viewport session is still active. The list must
    // retire that session before replacing the committed attachment nodes.
    auto* attachmentEditor=widget<QWidget>(window,"nativeEditor_autoFlexBodies");
    int selectedNodes=0; bool listOpenedAfterPickFinished=false; QTimer nodePicker;
    QObject::connect(&nodePicker,&QTimer::timeout,&window,[&]{
        auto* dialog=qobject_cast<QDialog*>(QApplication::activeModalWidget());
        auto* list=dialog ? dialog->findChild<QListWidget*>("nativeFlexNodeList") : nullptr;
        if (!list) return;
        listOpenedAfterPickFinished=!pickViewport->isPicking();
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
    require(listOpenedAfterPickFinished&&retainedEditor&&!pickViewport->isPicking()
        &&attachmentTable->rowCount()==selectedNodes,
        "The node-list selector did not retire the old viewport session before replacing its selection");
    escapePick(*pickViewport);
    require(retainedEditor&&attachmentTable->rowCount()==selectedNodes,
        "Esc rolled back the committed list selection through a stale viewport callback");
    log("AutoFlex attachment transitions: active node picker -> automatic clears nodes, active node picker -> list commits the real face nodes, subsequent Esc preserves both results");
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
    const auto plain=viewport->grabFramebuffer();
    auto* contour=widget<QComboBox>(window,"motionFlexContour");
    auto* range=widget<QComboBox>(window,"motionFlexRange");
    const int elastic=contour->findData(int(FlexContourKind::ElasticDisplacement));
    const int total=contour->findData(int(FlexContourKind::TotalDisplacement));
    require(elastic>=0&&total>=0&&range->findData(1)>=0,"Original flexible contour/range choices are missing");
    contour->setCurrentIndex(elastic);
    require(waitFor([&]{return viewport->grabFramebuffer()!=plain;}),"Elastic displacement cloud did not change mesh coloring and legend");
    const auto elasticImage=viewport->grabFramebuffer();
    require(elasticImage.save(output.filePath("autoflex-elastic-contour.png")),"Cannot capture elastic cloud");
    contour->setCurrentIndex(total);
    require(waitFor([&]{return viewport->grabFramebuffer()!=elasticImage;}),"Total displacement cloud selection did not update the viewport/legend");
    require(viewport->grabFramebuffer().save(output.filePath("autoflex-total-contour.png")),"Cannot capture total displacement cloud");
    contour->setCurrentIndex(elastic);timeline->setValue(0);range->setCurrentIndex(range->findData(0));QApplication::processEvents();
    const auto adaptiveZero=viewport->grabFramebuffer();
    range->setCurrentIndex(range->findData(1));
    require(waitFor([&]{return viewport->grabFramebuffer()!=adaptiveZero;}),"Whole-history color range did not replace the zero-frame adaptive legend");
    require(viewport->grabFramebuffer().save(output.filePath("autoflex-global-range.png")),"Cannot capture fixed whole-history color range");
    timeline->setValue(timeline->maximum());
    log("AutoFlex animation cloud UI: elastic and total displacement colors/legends, current-frame adaptive range and whole-history fixed range produce verified framebuffer updates");
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

#ifdef MX_HAS_BODY_DRIVES
int runMotionOccBodyDriveDesktopSmoke(const QString& outputDirectory)
{
    using namespace MotionOcc;
    try {
        QDir output(outputDirectory);
        require(output.mkpath("."), "Cannot create body drive smoke directory");
        QString cadPath, motionPath;
        makeFixture(output.absolutePath(), cadPath, motionPath);
        MotionWindow window;
        show(window);
        require(window.importCad(cadPath) && window.importMotionJson(motionPath, false), "Cannot import body drive editor fixture");
        auto* tree = widget<QTreeWidget>(window, "mTreeView");
        auto* dock = widget<ads::CDockWidget>(window, "Info");
        auto* manager = dock->dockManager();
        const auto checkMode = [](QWidget& panel, const QString& mode) {
            auto* target = widget<MechMainWindow::FoldSection>(panel, "bodyDriveTargetSection");
            auto* field = widget<QComboBox>(*target, "native_bodyDriveMode");
            require(target->isExpanded() && !field->isEnabled() && field->currentData().toString() == mode,
                "Body drive type is not shown and locked in the target section");
            require(widget<MechMainWindow::FoldSection>(panel, "bodyDriveCoordinateSection")->isExpanded(),
                "Body drive coordinates are not grouped in an expanded section");
        };
        const auto checkChannel = [](QWidget& section, const QString& axis, const QString& label) {
            auto* input = widget<QLineEdit>(section, "native_bodyDriveChannel" + axis);
            auto* button = widget<QToolButton>(section, "native_bodyDriveFunction" + axis);
            const auto labels = section.findChildren<QLabel*>();
            require(std::any_of(labels.cbegin(), labels.cend(), [&](QLabel* row) { return row->text() == label; }) &&
                    button->isEnabled() && !button->toolTip().isEmpty(),
                "Missing body drive unit label or function tool: " + label);
            return input;
        };
        const auto captureNarrow = [&](QWidget& panel, const QString& file) {
            QApplication::processEvents();
            const auto area = dock->dockAreaWidget();
            auto sizes = manager->splitterSizes(area);
            require(sizes.size() == 3, "Cannot find the dock splitter for the compact parameter layout");
            const int width = 12 * MxFunctions::UiMetrics::baseH() + 8 * qMax(0, 6 - MxFunctions::UiMetrics::baseG()) + 8;
            sizes[2] += sizes[1] - width;
            sizes[1] = width;
            manager->setSplitterSizes(area, sizes);
            auto* ancestor = panel.parentWidget();
            while (ancestor && !qobject_cast<QScrollArea*>(ancestor)) ancestor = ancestor->parentWidget();
            auto* scroll = qobject_cast<QScrollArea*>(ancestor);
            require(scroll, "Body drive editor is missing its scroll area");
            const bool fits = waitFor([&] {
                return dock->width() <= width + 10 && scroll->horizontalScrollBar()->maximum() == 0 &&
                    panel.width() <= scroll->viewport()->width();
            });
            require(panel.grab().save(output.filePath(file)), "Cannot capture body drive editor: " + file);
            const auto actualSizes = manager->splitterSizes(area);
            QStringList splitterWidths;
            for (const int size : actualSizes) splitterWidths.append(QString::number(size));
            require(fits, QStringLiteral("Body drive editor does not fit the compact dock width (%1 px requested, "
                "dock %2 px, dock minimum %3 px, viewport %4 px, panel %5 px, panel minimum hint %6 px, "
                "horizontal scroll maximum %7 px, splitter widths [%8])")
                .arg(width).arg(dock->width()).arg(dock->minimumWidth()).arg(scroll->viewport()->width())
                .arg(panel.width()).arg(panel.minimumSizeHint().width()).arg(scroll->horizontalScrollBar()->maximum())
                .arg(splitterWidths.join(", ")));
        };
        QAction* axisCommand = nullptr;
        for (auto* action : window.findChildren<QAction*>()) {
            if (action->property("mxOriginalAction").toString() == "createBodyDrive" &&
                action->property("mxOriginalArgument").toString() == "LinearPosition") {
                axisCommand = action;
                break;
            }
        }
        require(axisCommand && axisCommand->isEnabled(), "Linear body drive command missing");
        axisCommand->trigger();
        auto* editor = widget<QWidget>(window, "nativeEditor_bodyDrives");
        checkMode(*editor, "LinearPosition");
        auto* excitation = widget<MechMainWindow::FoldSection>(*editor, "bodyDriveExcitationSection");
        require(excitation->isExpanded(), "Single-axis excitation section is collapsed");
        auto* input = checkChannel(*excitation, "X", QStringLiteral("平移位移 (mm)"));
        require(!editor->findChild<QLineEdit*>("native_bodyDriveChannelY"), "Single-axis editor shows extra excitation channels");
        choose(*editor, "targetBodyId", 0);
        choose(*editor, "referenceBodyId", 37);
        input->setText("12*t*t");
        widget<QCheckBox>(*editor, "native_guide")->setChecked(false);
        captureNarrow(*editor, "single-axis-editor.png");
        widget<QPushButton>(*editor, "motionApplyButton")->click();
        auto* axisNode = node(*tree, ModelNode::Native, -1, "bodyDrives");
        auto* jointDriveNode = node(*tree, ModelNode::Native, 9, "drives");
        require(axisNode && jointDriveNode && axisNode->parent() && jointDriveNode->parent() &&
                axisNode->parent()->text(0) == QStringLiteral("运动体驱动") &&
                jointDriveNode->parent()->text(0) == QStringLiteral("驱动") &&
                axisNode->parent()->parent() == jointDriveNode->parent()->parent(),
            "Body drive and joint drive must appear in separate sibling model-tree groups");

        QToolButton* poseCommand = nullptr;
        for (auto* button : window.findChildren<QToolButton*>()) {
            if (button->property("mxOriginalAction").toString() == "createBodyDrive" &&
                button->property("mxOriginalArgument").toString() == "Pose") {
                poseCommand = button;
                break;
            }
        }
        require(poseCommand && poseCommand->isEnabled(), "Pose body drive ribbon button missing");
        poseCommand->click();
        editor = nullptr;
        require(waitFor([&] {
            for (auto* panel : window.findChildren<QWidget*>("nativeEditor_bodyDrives")) {
                if (panel->findChild<QLineEdit*>("native_bodyDriveChannelY")) {
                    editor = panel;
                    return true;
                }
            }
            return false;
        }), "Pose editor did not open after closing axis editor");
        checkMode(*editor, "Pose");
        auto* translation = widget<MechMainWindow::FoldSection>(*editor, "bodyDriveTranslationSection");
        auto* rotation = widget<MechMainWindow::FoldSection>(*editor, "bodyDriveRotationSection");
        require(translation->isExpanded() && rotation->isExpanded(), "Pose excitation sections are collapsed");
        for (const auto axis : QStringLiteral("XYZ"))
            checkChannel(*translation, QString(axis), QString(axis) + " (mm)");
        for (const auto axis : QStringLiteral("ABC"))
            checkChannel(*rotation, QString(axis), QStringLiteral("%1 (°)").arg(axis));
        choose(*editor, "targetBodyId", 0);
        choose(*editor, "referenceBodyId", -1);
        widget<QLineEdit>(*editor, "native_bodyDriveChannelY")->setText("2*t*t");
        widget<QLineEdit>(*editor, "native_bodyDriveChannelC")->setText("5*t*t");

        bool tableApplied = false;
        QTimer functionDialog;
        functionDialog.setSingleShot(true);
        QObject::connect(&functionDialog, &QTimer::timeout, &window, [&] {
            auto* dialog = window.findChild<QDialog*>("nativeFunctionEditor");
            if (!dialog) {
                if (auto* active = qobject_cast<QDialog*>(QApplication::activeModalWidget())) active->reject();
                return;
            }
            auto* mode = dialog->findChild<QComboBox*>("nativeFunctionMode");
            auto* table = dialog->findChild<QTableWidget*>("nativeFunctionTable");
            auto* buttons = dialog->findChild<QDialogButtonBox*>("nativeFunctionButtons");
            auto* apply = buttons ? buttons->button(QDialogButtonBox::Ok) : nullptr;
            if (mode && mode->count() > 3 && table && apply && table->horizontalHeaderItem(1) &&
                table->horizontalHeaderItem(1)->text() == "mm") {
                mode->setCurrentIndex(3);
                table->setRowCount(2);
                table->setItem(0, 0, new QTableWidgetItem("0"));
                table->setItem(0, 1, new QTableWidgetItem("0"));
                table->setItem(1, 0, new QTableWidgetItem("1"));
                table->setItem(1, 1, new QTableWidgetItem("4"));
                apply->click();
                tableApplied = dialog->result() == QDialog::Accepted;
            }
            if (!tableApplied) dialog->reject();
        });
        functionDialog.start(20);
        widget<QToolButton>(*translation, "native_bodyDriveFunctionZ")->click();
        functionDialog.stop();
        require(tableApplied && widget<QLineEdit>(*translation, "native_bodyDriveChannelZ")->isReadOnly(),
            "Pose translation function editor did not apply the time table in mm");
        captureNarrow(*editor, "pose-editor.png");
        widget<QPushButton>(*editor, "motionApplyButton")->click();
        axisNode = node(*tree, ModelNode::Native, -1, "bodyDrives");
        require(axisNode && axisNode->parent() && axisNode->parent()->childCount() == 2,
            "The model-tree body drive group must contain both modes");
        tree->scrollToItem(axisNode->parent());
        require(tree->grab().save(output.filePath("body-drive-tree.png")), "Cannot capture body drive model tree");

        QString error;
        const auto path = output.filePath("body-drive.mxmotion");
        saveThroughUi(window, path);
        CadDocument cad;
        RigidModel model;
        require(loadRigidProject(path, cad, model, error) && model.nativeDocument, "Cannot reopen body drive project: " + error);
        const auto saved = model.nativeDocument->collection("bodyDrives");
        require(saved.size() == 2, "Both body drives must survive reopening");
        const auto axis = saved.at(0).toObject();
        const auto pose = saved.at(1).toObject();
        require(!axis.value("guide").toBool() && axis.value("referenceBodyId").toInt() == 37 &&
                axis.value("channels").toObject().value("X").toObject().value("expression").toString() == "12*t*t",
                "Reopening lost axis settings");
        const auto poseChannels = pose.value("channels").toObject();
        const auto table = poseChannels.value("Z").toObject();
        require(pose.value("bodyDriveMode").toString() == "Pose" &&
                poseChannels.value("Y").toObject().value("expression").toString() == "2*t*t" &&
                poseChannels.value("C").toObject().value("expression").toString() == "5*t*t" &&
                table.value("driveTableX").toArray() == QJsonArray{0, 1} &&
                table.value("driveTableY").toArray() == QJsonArray{0, 4},
                "Reopening lost pose expressions or the time table");
        qInfo() << "已验证运动体驱动分组、窄栏布局、单位、时间表及保存重开";
        return 0;
    } catch (const std::exception& error) {
        qCritical().noquote() << QString::fromUtf8(error.what());
        return 1;
    }
}
#endif

int runMotionOccNativeDesktopSmoke(QApplication&, const QString& outputDirectory)
{
#ifdef MX_MOTION_OCC_COMPAT
    // The previous assertions below target the removed generated JSON forms.
    // Keep the command-line/CI entry stable while exercising the original GUI.
    extern int runMotionOccOriginalGuiSmoke(QApplication&,const QString&);
    return runMotionOccOriginalGuiSmoke(*qobject_cast<QApplication*>(QCoreApplication::instance()),outputDirectory);
#else
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
        typedPanelWorkflow(output,cadPath,motionPath,log);
        originalPickerAndMassWorkflow(output,cadPath,motionPath,log);
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
        loadExportWorkflow(window,*tree,output,log);
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
#endif
}
