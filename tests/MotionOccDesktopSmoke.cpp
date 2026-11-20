#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidPlot.h"
#include "RigidProject.h"

#include <DockManager.h>
#include <DockAreaWidget.h>
#include <DockWidget.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_marker.h>
#include <qwt_scale_map.h>
#include <qwt_legend.h>
#include <qwt_legend_label.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QSlider>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPushButton>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWheelEvent>
#include <functional>
#include <cmath>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

bool waitFor(const std::function<bool()>& predicate, int timeoutMs)
{
    if (predicate()) return true;
    QEventLoop loop;
    QTimer check;
    QObject::connect(&check, &QTimer::timeout, &loop, [&] { if (predicate()) loop.quit(); });
    check.start(20);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return predicate();
}

void mouse(QWidget& widget, QEvent::Type type, const QPoint& pos, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    QMouseEvent event(type, pos, widget.mapToGlobal(pos), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(&widget, &event);
}

QImage frame(OccViewport& viewport)
{
    const auto image = viewport.grabFramebuffer();
    require(!image.isNull() && image.width() > 50 && image.height() > 50, QStringLiteral("Empty OpenGL framebuffer"));
    return image;
}

QTreeWidgetItem* findNode(QTreeWidget& tree, ModelNode::Kind kind)
{
    for (QTreeWidgetItemIterator it(&tree); *it; ++it)
        if ((*it)->data(0, Qt::UserRole + 1).toInt() == int(kind)) return *it;
    return nullptr;
}

void requireLegend(QwtPlot& plot)
{
    auto* legend = qobject_cast<QwtLegend*>(plot.legend());
    require(legend, QStringLiteral("Curve legend object is missing"));
    bool visible = legend->isVisible() && !legend->visibleRegion().isEmpty();
    for (auto* curve : plot.itemList(QwtPlotItem::Rtti_PlotCurve)) {
        const auto entries = legend->legendWidgets(plot.itemToInfo(curve));
        require(entries.size() == 1, QStringLiteral("A plotted curve has no unique legend entry"));
        auto* label = qobject_cast<QwtLegendLabel*>(entries.front());
        require(label && label->text().text() == curve->title().text(), QStringLiteral("Legend has lost a curve quantity/unit"));
        require(label->updatesEnabled(), QStringLiteral("Legend label remains explicitly paint-disabled after reopening its dock"));
        const auto pixels = label->grab().toImage();
        const QColor expected = static_cast<QwtPlotCurve*>(curve)->pen().color();
        int colored = 0;
        for (int y = 0; y < pixels.height(); ++y)
            for (int x = 0; x < pixels.width(); ++x) {
                const QColor actual = pixels.pixelColor(x, y);
                if (actual.alpha() > 128 && std::abs(actual.red() - expected.red()) < 20
                    && std::abs(actual.green() - expected.green()) < 20 && std::abs(actual.blue() - expected.blue()) < 20) ++colored;
            }
        require(colored >= 3, QStringLiteral("Legend label has no rendered series color"));
        visible = visible && label->isVisible() && !label->visibleRegion().isEmpty();
    }
    require(visible, QStringLiteral("A saved curve legend entry is hidden or clipped on first display"));
}

void requireCurveRows(QListWidget& list)
{
    bool fit = true;
    for (int index = 0; index < list.count(); ++index) {
        auto* row = list.itemWidget(list.item(index));
        require(row, QStringLiteral("A curve row is missing"));
        QPushButton* remove = nullptr;
        for (auto* button : row->findChildren<QPushButton*>())
            if (button->text() == QStringLiteral("删除")) remove = button;
        require(remove, QStringLiteral("Curve removal control is missing"));
        const auto left = remove->mapTo(list.viewport(), remove->rect().topLeft()).x();
        const auto right = remove->mapTo(list.viewport(), remove->rect().topRight()).x();
        fit = fit && left >= 0 && right < list.viewport()->width();
    }
    require(fit, QStringLiteral("Curve removal buttons are clipped by the narrow parameter column"));
}
}

int runMotionOccDesktopSmoke(QApplication&, const QString& outputDirectory)
{
    using namespace MotionOcc;
    const QDir output(outputDirectory);
    QFile report(output.filePath(QStringLiteral("desktop-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate)) return 2;
    const auto line = [&report](const QString& value) {
        report.write(value.toUtf8());
        report.write("\n");
        report.flush();
    };

    try {
        QTemporaryDir temp;
        require(temp.isValid(), QStringLiteral("No fixture directory"));
        CadDocument fixture;
        int id = -1;
        QString error;
        require(fixture.addPrimitive(Primitive::Box, 30, 20, 16, id, error), QStringLiteral("Could not create CAD fixture"));
        const auto step = temp.filePath(QStringLiteral("OCC 桌面测试.step"));
        require(fixture.exportFile(step, id, error), QStringLiteral("Could not write STEP fixture: %1").arg(error));

        // A project opened from the command line has its requests before the
        // initially closed ADS curve dock has ever been shown. Exercise that
        // lifecycle independently of adding fresh curves to an already shown plot.
        RigidModel savedModel;
        savedModel.bodies.push_back({41, QStringLiteral("转子 é"), id, false, 2.5, 2500});
        savedModel.joints.push_back({73, QStringLiteral("斜轴铰链"), JointKind::Revolute, 41, 0, {17, -13, 11}, {2, -3, 6}});
        savedModel.drives.push_back({109, QStringLiteral("恒速驱动"), 73, DriveKind::Speed, 105});
        savedModel.scenarios.push_back({151, QStringLiteral("偏置旋转"), 2.003, 0.005, {0.4, 0.1, -9.8}});
        savedModel.scenarios.front().curveRequests = {
            {1, RigidCurveMetric::JointCoordinate, 73, 0, QStringLiteral("旋转副相对转角"), QStringLiteral("#2176c1"), true},
            {2, RigidCurveMetric::BodyPosition, 41, 0, QStringLiteral("质心世界位置 X"), QStringLiteral("#d35f39"), true},
            {3, RigidCurveMetric::BodyDisplacement, 41, 0, QStringLiteral("质心位移增量 X"), QStringLiteral("#24946b"), false}
        };
        const auto savedProject = temp.filePath(QStringLiteral("saved-curves.mxmotion"));
        require(saveRigidProject(savedProject, fixture, savedModel, error), QStringLiteral("Could not save curve project fixture: %1").arg(error));
        {
            MotionWindow storedWindow;
            storedWindow.resize(1300, 760);
            require(storedWindow.openRigidProject(savedProject), QStringLiteral("Could not open saved curve project"));
            storedWindow.showNormal();
            auto* storedViewport = storedWindow.findChild<OccViewport*>(QStringLiteral("occViewport"));
            require(storedViewport && waitFor([&] { return storedViewport->glReady(); }, 12000),
                    QStringLiteral("Saved project viewport did not initialize"));
            auto* storedTree = storedWindow.findChild<QTreeWidget*>(QStringLiteral("mTreeView"));
            require(storedTree && findNode(*storedTree, ModelNode::Scenario), QStringLiteral("Saved scenario is missing"));
            storedTree->setCurrentItem(findNode(*storedTree, ModelNode::Scenario));
            storedWindow.command(Command::Solve)->trigger();
            require(waitFor([&] {
                const auto* node = findNode(*storedTree, ModelNode::Curves);
                return node && !node->isDisabled() && !storedWindow.command(Command::StopSolve)->isEnabled();
            }, 20000), QStringLiteral("Saved project did not solve"));
            auto* storedDocks = storedWindow.findChild<ads::CDockManager*>();
            auto* storedCurveDock = storedDocks ? storedDocks->dockWidgetsMap().value(QStringLiteral("Curve")) : nullptr;
            auto* storedPlot = storedCurveDock ? storedCurveDock->widget()->findChild<QwtPlot*>() : nullptr;
            require(storedPlot, QStringLiteral("Saved project curve dock has no plot"));
            require(storedCurveDock->isClosed() && !storedPlot->isVisible(), QStringLiteral("Saved requests opened the plot before curve selection"));
            storedTree->setCurrentItem(findNode(*storedTree, ModelNode::Curves));
            QApplication::processEvents();
            require(storedPlot->isVisible() && storedPlot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
                    QStringLiteral("First saved-project curve display lost visible requests"));
            require(storedWindow.grab().save(output.filePath(QStringLiteral("persisted-curve-configuration.png"))),
                    QStringLiteral("Could not capture saved-project curve configuration"));
            requireLegend(*storedPlot);
            auto* storedList = storedWindow.findChild<QListWidget*>(QStringLiteral("motionCurveList"));
            require(storedList && storedList->count() == 3, QStringLiteral("Saved curve list lost a hidden request"));
            requireCurveRows(*storedList);
            line(QStringLiteral("Saved requests loaded before first display: visible legends, hidden request and accessible row controls passed"));
        }

        MotionWindow window;
        window.resize(1300, 760);
        window.showNormal();
        auto* viewport = window.findChild<OccViewport*>(QStringLiteral("occViewport"));
        require(viewport != nullptr, QStringLiteral("Viewport missing"));
        QString graphicsError;
        QObject::connect(viewport, &OccViewport::graphicsError, &window, [&graphicsError](const QString& reason) {
            graphicsError = reason;
        });
        require(waitFor([&] { return viewport->glReady() || !graphicsError.isEmpty(); }, 12000)
                && viewport->glReady(), QStringLiteral("OpenGL initialization failed: %1").arg(graphicsError));
        const auto gl = viewport->context()->format();
        line(QStringLiteral("Desktop OpenGL %1.%2 core initialized").arg(gl.majorVersion()).arg(gl.minorVersion()));

        const auto* tabs = window.findChild<QTabBar*>(QStringLiteral("MenuTabBar"));
        require(tabs && tabs->count() == 10 && tabs->tabText(1) == QStringLiteral("几何")
            && tabs->tabText(7) == QStringLiteral("求解"), QStringLiteral("Original Motion tabs missing"));
        const auto* docks = window.findChild<ads::CDockManager*>();
        require(docks != nullptr, QStringLiteral("ADS manager missing"));
        const auto panes = docks->dockWidgetsMap();
        for (const auto& key : { QStringLiteral("Center"), QStringLiteral("Tree"), QStringLiteral("Info") }) {
            auto* pane = panes.value(key);
            require(pane && pane->isVisible() && !pane->isClosed(), QStringLiteral("ADS pane not visible: %1").arg(key));
        }
        line(QStringLiteral("ADS model, parameters and viewport visible; 10 Motion ribbon tabs"));

        const auto empty = frame(*viewport);
        require(window.importCad(step), QStringLiteral("STEP import failed through the window"));
        require(waitFor([&] { return frame(*viewport) != empty; }, 4000), QStringLiteral("No CAD frame after import"));
        auto* tree = window.findChild<QTreeWidget*>(QStringLiteral("mTreeView"));
        require(tree && tree->currentItem() && tree->currentItem()->parent()
            && tree->currentItem()->parent()->text(0) == QStringLiteral("部件"), QStringLiteral("No imported part selected in tree"));

        window.command(Command::ClearSelection)->trigger();
        const auto center = viewport->rect().center();
        mouse(*viewport, QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, center, Qt::LeftButton, Qt::NoButton);
        require(tree->currentItem() && tree->currentItem()->parent()
            && tree->currentItem()->parent()->text(0) == QStringLiteral("部件"), QStringLiteral("Viewport picking did not update the model tree"));
        const auto original = frame(*viewport);
        require(original.save(output.filePath(QStringLiteral("cad-viewport.png"))), QStringLiteral("CAD frame could not be saved"));

        const auto end = center + QPoint(54, 24);
        mouse(*viewport, QEvent::MouseButtonPress, center, Qt::LeftButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseMove, end, Qt::NoButton, Qt::LeftButton);
        mouse(*viewport, QEvent::MouseButtonRelease, end, Qt::LeftButton, Qt::NoButton);
        const auto rotated = frame(*viewport);
        require(rotated != original, QStringLiteral("CAD frame did not change after orbit"));
        QWheelEvent wheel(center, viewport->mapToGlobal(center), {}, { 0, 120 }, Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(viewport, &wheel);
        require(frame(*viewport) != rotated, QStringLiteral("CAD frame did not change after zoom"));
        window.command(Command::Isometric)->trigger();
        window.command(Command::Fit)->trigger();
        line(QStringLiteral("STEP import, part picking, orbit, zoom and fit passed"));

        window.command(Command::GenerateMesh)->trigger();
        auto* generate = window.findChild<QPushButton*>(QStringLiteral("generateMeshButton"));
        require(generate && generate->isVisible() && generate->isEnabled(), QStringLiteral("Mesh parameters not available"));
        generate->click();
        require(waitFor([&] { return window.command(Command::ExportMesh)->isEnabled(); }, 30000),
            QStringLiteral("No validated mesh from the window's Netgen worker"));
        auto* summary = window.findChild<QLabel*>(QStringLiteral("partSummary"));
        require(summary && summary->text().contains(QStringLiteral("四面体："))
            && !summary->text().contains(QStringLiteral("四面体：0")), QStringLiteral("No tetrahedra shown in parameters"));
        auto* grid = window.command(Command::MeshDisplay);
        require(grid->isEnabled() && grid->isChecked(), QStringLiteral("Real mesh display not selected"));
        const auto mesh = frame(*viewport);
        window.command(Command::ShadedEdges)->trigger();
        require(frame(*viewport) != mesh, QStringLiteral("Finite-element grid did not change the OpenGL frame"));
        grid->trigger();
        require(mesh.save(output.filePath(QStringLiteral("mesh-viewport.png"))), QStringLiteral("Mesh frame could not be saved"));
        require(window.grab().save(output.filePath(QStringLiteral("mxmotion.png"))), QStringLiteral("Window image could not be saved"));
        line(summary->text());
        auto applyForm = [&] {
            const auto buttons = window.findChildren<QPushButton*>(QStringLiteral("motionApplyButton"));
            for (auto* button : buttons) if (button->isVisible()) { button->click(); return; }
            require(false, QStringLiteral("Motion parameter editor not visible"));
        };
        window.command(Command::AddBody)->trigger();
        applyForm();
        require(window.command(Command::AddRevolute)->isEnabled(), QStringLiteral("Motion body not created"));
        window.command(Command::AddRevolute)->trigger();
        applyForm();
        require(window.command(Command::AddSpeedDrive)->isEnabled(), QStringLiteral("Revolute joint not created"));
        window.command(Command::AddSpeedDrive)->trigger();
        for (auto* value : window.findChildren<QDoubleSpinBox*>())
            if (value->isVisible() && value->suffix().contains(QStringLiteral("°/s"))) value->setValue(90);
        applyForm();
        window.command(Command::NewScenario)->trigger();
        applyForm();
        auto resultNode = [tree](ModelNode::Kind kind) -> QTreeWidgetItem* {
            for (QTreeWidgetItemIterator it(tree); *it; ++it)
                if ((*it)->data(0, Qt::UserRole + 1).toInt() == int(kind)) return *it;
            return nullptr;
        };
        auto partGroup = [tree]() -> QTreeWidgetItem* {
            for (QTreeWidgetItemIterator it(tree); *it; ++it)
                if ((*it)->text(0) == QStringLiteral("部件")) return *it;
            return nullptr;
        };
        require(partGroup(), QStringLiteral("Missing parts group"));
        partGroup()->setExpanded(false);
        window.command(Command::Solve)->trigger();
        auto* curveDock = panes.value(QStringLiteral("Curve"));
        auto* plot = curveDock ? curveDock->widget()->findChild<QwtPlot*>() : nullptr;
        require(plot, QStringLiteral("QwtPlot object not created"));
        require(waitFor([&] {
            const auto* node = resultNode(ModelNode::Animation);
            return node && !node->isDisabled() && !window.command(Command::StopSolve)->isEnabled();
        }, 20000), QStringLiteral("MotionEngine did not enable the result nodes"));
        require(curveDock->isClosed() && plot->itemList(QwtPlotItem::Rtti_PlotCurve).isEmpty(),
            QStringLiteral("Solving must not automatically create a curve or enter animation"));
        require(!partGroup()->isExpanded(), QStringLiteral("Solving reset the model tree's collapsed group"));

        tree->setCurrentItem(resultNode(ModelNode::Curves));
        QApplication::processEvents();
        auto* configuration = window.findChild<QWidget*>(QStringLiteral("motionCurveConfiguration"));
        require(configuration && configuration->isVisible(), QStringLiteral("Curve configuration is missing from the parameter column"));
        auto* source = configuration->findChild<QComboBox*>(QStringLiteral("motionCurveSource"));
        auto* variable = configuration->findChild<QComboBox*>(QStringLiteral("motionCurveVariable"));
        auto* addCurve = configuration->findChild<QPushButton*>(QStringLiteral("motionAddCurveButton"));
        require(source && variable && addCurve, QStringLiteral("Original curve request controls missing"));
        source->setCurrentIndex(source->findData(1)); // Joint relative angle, not world position.
        variable->setCurrentIndex(variable->findData(int(RigidCurveMetric::JointCoordinate) * 4));
        addCurve->click();
        require(waitFor([&] { return plot->isVisible() && plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 1; }, 4000),
            QStringLiteral("Explicit Add Curve did not create the requested series"));
        auto* curve = static_cast<QwtPlotCurve*>(plot->itemList(QwtPlotItem::Rtti_PlotCurve).front());
        require(curve->dataSize() > 10 && curve->sample(curve->dataSize() - 1).x() >= 0.99,
            QStringLiteral("Qwt is missing the physical time series"));
        require(std::abs(curve->sample(curve->dataSize() - 1).y() - 90.0) < 0.5,
            QStringLiteral("Joint angle curve disagrees with the solved 90 degree motion"));
        const auto physicalFrames = curve->dataSize();
        source->setCurrentIndex(source->findData(0));
        variable->setCurrentIndex(variable->findData(int(RigidCurveMetric::BodyPosition) * 4));
        addCurve->click();
        require(plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
            QStringLiteral("Adding another channel replaced the existing curve"));
        auto* rigidPlot = window.findChild<RigidPlot*>();
        require(rigidPlot && rigidPlot->exportCurvesCsv(temp.filePath(QStringLiteral("selected-curves.csv")), error),
            QStringLiteral("Could not export selected result curves: %1").arg(error));
        double clickedTime = -1;
        QObject::connect(rigidPlot, &RigidPlot::timeRequested, &window,
            [&clickedTime](double time) { clickedTime = time; });
        const auto markers = plot->itemList(QwtPlotItem::Rtti_PlotMarker);
        require(markers.size() == 1 && !markers.front()->isVisible(), QStringLiteral("Animation marker is visible outside animation"));
        // ADS schedules splitter/layout changes when a previously closed dock
        // opens. Compare rendered poses only after the canvas has its real size.
        QApplication::processEvents();
        require(window.grab().save(output.filePath(QStringLiteral("curve-configuration.png"))), QStringLiteral("Could not capture curve configuration"));
        requireLegend(*plot);

        const auto initialCad = frame(*viewport);
        initialCad.save(output.filePath(QStringLiteral("before-animation.png")));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        QApplication::processEvents();
        auto* timeline = window.findChild<QSlider*>(QStringLiteral("motionTimeline"));
        require(timeline && timeline->isEnabled() && timeline->maximum() == int(physicalFrames) - 1,
            QStringLiteral("Playback slider does not index real result frames"));
        require(markers.front()->isVisible(), QStringLiteral("Animation did not activate the curve time marker"));
        const auto rest = frame(*viewport);
        timeline->setValue(timeline->maximum());
        require(frame(*viewport) != rest, QStringLiteral("Playback did not animate the real CAD/mesh frame"));
        const auto* cursor = static_cast<QwtPlotMarker*>(markers.front());
        require(std::abs(cursor->xValue() - 1.0) < 1e-9, QStringLiteral("Curve marker is not at the physical playback time"));
        require(window.grab().save(output.filePath(QStringLiteral("motion-qwt.png"))), QStringLiteral("Could not save Qwt screenshot"));

        auto* follow = window.findChild<QCheckBox*>(QStringLiteral("motionFollowCurveMarker"));
        auto* jump = window.findChild<QCheckBox*>(QStringLiteral("motionClickCurveJump"));
        auto* play = window.findChild<QPushButton*>(QStringLiteral("motionPlayButton"));
        auto* pause = window.findChild<QPushButton*>(QStringLiteral("motionPauseButton"));
        auto* stop = window.findChild<QPushButton*>(QStringLiteral("motionStopButton"));
        require(follow && jump && play && pause && stop, QStringLiteral("Original animation controls missing"));
        follow->setChecked(false);
        require(!cursor->isVisible(), QStringLiteral("Disabling marker follow did not hide the marker"));
        follow->setChecked(true);
        const auto clickTime = [plot](double time) {
            auto* canvas = plot->canvas();
            const QPoint point(int(plot->canvasMap(QwtPlot::xBottom).transform(time)), canvas->height() / 2);
            mouse(*canvas, QEvent::MouseButtonPress, point, Qt::LeftButton, Qt::LeftButton);
            mouse(*canvas, QEvent::MouseButtonRelease, point, Qt::LeftButton, Qt::NoButton);
        };
        jump->setChecked(false);
        clickTime(0.5);
        require(timeline->value() == timeline->maximum(), QStringLiteral("Disabled curve jump still moved the animation"));
        jump->setChecked(true);
        clickTime(0.5);
        require(std::abs(timeline->value() - timeline->maximum() / 2) <= 2,
            QStringLiteral("Clicking the curve did not seek the matching animation frame: frame %1/%2, requested %3 s, slider enabled %4")
                .arg(timeline->value()).arg(timeline->maximum()).arg(clickedTime).arg(timeline->isEnabled()));
        play->click();
        require(waitFor([&] { return timeline->value() > timeline->maximum() / 2 + 2; }, 3000),
            QStringLiteral("Playback did not advance"));
        pause->click();
        require(play->isEnabled() && !pause->isEnabled(), QStringLiteral("Pause did not preserve a resumable playback state"));
        stop->click();
        require(timeline->value() == 0, QStringLiteral("Stop did not restore the first physical frame"));
        require(frame(*viewport) == rest, QStringLiteral("Playback did not restore frame zero"));
        timeline->setValue(timeline->maximum());
        auto* animationDock = panes.value(QStringLiteral("MotionAnimation"));
        require(animationDock, QStringLiteral("Animation parameter dock missing"));
        animationDock->closeDockWidget();
        const auto afterAnimation = frame(*viewport);
        afterAnimation.save(output.filePath(QStringLiteral("after-animation.png")));
        require(afterAnimation == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Exiting animation did not restore initial CAD and deactivate curve linkage: before %1x%2, after %3x%4, same pixels %5, slider enabled %6, marker visible %7")
                .arg(initialCad.width()).arg(initialCad.height()).arg(afterAnimation.width()).arg(afterAnimation.height())
                .arg(afterAnimation == initialCad).arg(timeline->isEnabled()).arg(cursor->isVisible()));
        require(plot->itemList(QwtPlotItem::Rtti_PlotCurve).size() == 2,
            QStringLiteral("Exiting animation discarded configured curves"));
        tree->setCurrentItem(resultNode(ModelNode::Curves));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        require(timeline->isEnabled(), QStringLiteral("Animation could not be reopened"));
        timeline->setValue(timeline->maximum());
        auto* parameters = panes.value(QStringLiteral("Info"));
        require(parameters && parameters->dockAreaWidget(), QStringLiteral("Parameter host missing"));
        parameters->dockAreaWidget()->setCurrentDockWidget(parameters);
        require(frame(*viewport) == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Switching back to parameter tab left a stale animated model"));
        // Switching ADS tabs leaves the tree's current item on Animation.
        // Navigate away first, as a fresh selection must emit currentItemChanged.
        tree->setCurrentItem(resultNode(ModelNode::Curves));
        tree->setCurrentItem(resultNode(ModelNode::Animation));
        require(timeline->isEnabled(), QStringLiteral("Animation could not be reopened before export selection"));
        timeline->setValue(timeline->maximum());
        tree->setCurrentItem(resultNode(ModelNode::Export));
        require(frame(*viewport) == initialCad && !timeline->isEnabled() && !cursor->isVisible(),
            QStringLiteral("Selecting the export node left a stale animated model"));
        for (auto* widget : QApplication::topLevelWidgets())
            require(!qobject_cast<QFileDialog*>(widget) || !widget->isVisible(),
                QStringLiteral("Selecting the export node unexpectedly opened a save dialog"));
        line(QStringLiteral("MotionEngine solve, explicit multi-curve configuration/CSV, frame indexing, pause/stop, independent curve linkage and CAD restoration passed"));
        line(QStringLiteral("Desktop smoke test passed"));
        return 0;
    } catch (const std::exception& e) {
        line(QStringLiteral("FAIL: %1").arg(QString::fromUtf8(e.what())));
        return 1;
    }
}
