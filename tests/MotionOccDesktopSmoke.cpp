#include "MotionWindow.h"
#include "OccViewport.h"

#include <DockManager.h>
#include <DockWidget.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <QDoubleSpinBox>
#include <QSlider>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QPushButton>
#include <QTabBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QWheelEvent>
#include <functional>
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
        window.command(Command::Solve)->trigger();
        auto* curveDock = panes.value(QStringLiteral("Curve"));
        auto* plot = curveDock ? curveDock->widget()->findChild<QwtPlot*>() : nullptr;
        require(plot, QStringLiteral("QwtPlot object not created"));
        require(waitFor([&] { return plot->isVisible() && !plot->itemList(QwtPlotItem::Rtti_PlotCurve).isEmpty(); }, 20000),
            QStringLiteral("No solved series in the ADS Qwt dock"));
        auto* curve = static_cast<QwtPlotCurve*>(plot->itemList(QwtPlotItem::Rtti_PlotCurve).front());
        require(curve->dataSize() > 10 && curve->sample(curve->dataSize() - 1).x() >= 0.99,
            QStringLiteral("Qwt is missing the physical time series"));
        auto* timeline = window.findChild<QSlider*>(QStringLiteral("motionTimeline"));
        require(timeline, QStringLiteral("No playback slider"));
        const auto rest = frame(*viewport);
        timeline->setValue(timeline->maximum());
        require(frame(*viewport) != rest, QStringLiteral("Playback did not animate the real CAD/mesh frame"));
        require(window.grab().save(output.filePath(QStringLiteral("motion-qwt.png"))), QStringLiteral("Could not save Qwt screenshot"));
        timeline->setValue(0);
        require(frame(*viewport) == rest, QStringLiteral("Playback did not restore frame zero"));
        line(QStringLiteral("MotionEngine solve, Qwt real samples, and viewport playback passed"));
        line(QStringLiteral("Desktop smoke test passed"));
        return 0;
    } catch (const std::exception& e) {
        line(QStringLiteral("FAIL: %1").arg(QString::fromUtf8(e.what())));
        return 1;
    }
}
