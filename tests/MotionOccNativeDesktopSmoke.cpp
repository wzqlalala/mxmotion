#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"

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
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QGroupBox>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QTemporaryDir>
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
            slider->setValue(slider->maximum()/2);
            QApplication::processEvents();
            require(real.grab().save(output.filePath("native-real-case.png")), "Could not capture real supplied case");
            log("Real supplied case public geometry + original JSON import, full solve and animation passed: " + realMotion);
        }
        log("Native desktop regression passed");
        return 0;
    } catch (const std::exception& error) {
        log(QStringLiteral("FAIL: %1").arg(QString::fromUtf8(error.what())));
        return 1;
    }
}
