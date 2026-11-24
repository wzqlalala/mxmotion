#include "MotionWindow.h"
#include "OccViewport.h"
#include "ViewportContainer.h"
#include "ViewportWidget.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QTextStream>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

template<class T>
T* widget(QObject& parent, const char* name)
{
    auto* child = parent.findChild<T*>(QString::fromLatin1(name));
    require(child, QStringLiteral("Missing control %1").arg(QString::fromLatin1(name)));
    return child;
}

void menuAction(QTreeWidget& tree, QTreeWidgetItem* item, const QString& title)
{
    bool chosen = false;
    QTimer::singleShot(0, &tree, [&] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu) return;
        for (auto* action : menu->actions()) {
            if (action->text() != title || !action->isEnabled()) continue;
            chosen = true;
            menu->setActiveAction(action);
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            QApplication::sendEvent(menu, &press);
            return;
        }
        menu->close();
    });
    QMetaObject::invokeMethod(&tree, "customContextMenuRequested", Qt::DirectConnection,
                              Q_ARG(QPoint, tree.visualItemRect(item).center()));
    require(chosen, QStringLiteral("View tree menu action unavailable: %1").arg(title));
    QApplication::processEvents();
}

void click(OccViewport& viewport, const QPoint& pos)
{
    QMouseEvent press(QEvent::MouseButtonPress, pos, viewport.mapToGlobal(pos), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&viewport, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, pos, viewport.mapToGlobal(pos), Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&viewport, &release);
}

} // namespace

int runMotionOccViewPagesSmoke(QApplication& app, const QString& outputDirectory)
{
    const QDir output(outputDirectory);
    if (!QDir().mkpath(output.absolutePath())) return 2;
    QFile report(output.filePath(QStringLiteral("view-pages-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) return 2;
    QTextStream log(&report);
    try {
        CadDocument cad;
        QString error;
        int id = -1;
        require(cad.addPrimitive(Primitive::Box, 32, 22, 11, id, error), error);
        const auto cadPath = output.filePath(QStringLiteral("view-fixture.brep"));
        require(cad.exportFile(cadPath, id, error), error);
        MotionWindow window;
        window.resize(1280, 840);
        window.show();
        app.processEvents();
        require(window.importCad(cadPath), QStringLiteral("Could not import the shared viewport scene"));
        auto* docks = qobject_cast<ads::CDockManager*>(window.centralWidget());
        require(docks, QStringLiteral("Missing dock manager"));
        auto* dock = docks->findDockWidget(QStringLiteral("ViewTree"));
        require(dock, QStringLiteral("Missing view tree dock"));
        dock->toggleView(true);
        dock->dockAreaWidget()->setCurrentDockWidget(dock);
        app.processEvents();
        auto* tree = widget<QTreeWidget>(*dock, "viewPageTree");
        auto* container = widget<MechMainWindow::ViewportContainer>(window, "occViewportContainer");
        auto* page = widget<QComboBox>(window, "viewportPageCombo");
        auto* count = widget<QLabel>(window, "viewportPageCount");
        auto* original = widget<OccViewport>(window, "occViewport");
        require(tree->topLevelItemCount() == 1 && count->text() == QStringLiteral("1/1"), QStringLiteral("Wrong initial view page"));
        menuAction(*tree, tree->topLevelItem(0), QStringLiteral("新建视口"));
        require(container->visibleViewports().size() == 1 && tree->topLevelItem(0)->childCount() == 2,
            QStringLiteral("New and overflow viewports were not retained in single-view layout"));

        widget<QToolButton>(window, "viewportLayout_2")->click();
        app.processEvents();
        require(container->visibleViewports().size() == 2, QStringLiteral("Split layout did not show two real viewports"));
        auto* second = widget<OccViewport>(window, "occViewport");
        require(second != original, QStringLiteral("New viewport never became active"));
        original->viewFront();
        second->viewTop();
        const auto cameraDirection = [](const OccViewport& viewport) {
            const auto camera = viewport.queryCameraState();
            return (camera.eye - camera.target).normalized();
        };
        require(QVector3D::dotProduct(cameraDirection(*second), cameraDirection(*original)) < .1f,
            QStringLiteral("Viewports cannot maintain independent orientations"));
        auto* link = widget<QToolButton>(window, "viewportCameraSync");
        link->click();
        require(QVector3D::dotProduct(cameraDirection(*second), cameraDirection(*original)) > .99f,
            QStringLiteral("Enabling camera sync did not synchronize visible viewports"));
        second->viewIsometric();
        require(!second->grabFramebuffer().isNull() && !original->grabFramebuffer().isNull(),
            QStringLiteral("Split views did not render valid OpenGL output"));
        require(QVector3D::dotProduct(cameraDirection(*second), cameraDirection(*original)) > .99f,
            QStringLiteral("Linked views did not track a subsequent camera update"));
        link->click();

        click(*original, QPoint(12, 12));
        require(widget<OccViewport>(window, "occViewport") == original, QStringLiteral("Mouse did not activate the clicked split"));
        original->beginPick(PickKind::Edge);
        click(*second, QPoint(12, 12));
        require(widget<OccViewport>(window, "occViewport") == original && original->isPicking(),
            QStringLiteral("Switching views disrupted an active typed pick"));
        original->endPick();

        menuAction(*tree, tree->topLevelItem(0), QStringLiteral("新建页"));
        require(page->count() == 2 && page->currentIndex() == 1 && count->text() == QStringLiteral("2/2")
            && container->visibleViewports().isEmpty(), QStringLiteral("New page did not start empty"));
        menuAction(*tree, tree->topLevelItem(1), QStringLiteral("新建视口"));
        require(container->visibleViewports().size() == 1, QStringLiteral("Could not add an independent viewport to the new page"));
        widget<QAction>(window, "nextViewportPage")->trigger();
        require(page->currentIndex() == 0 && count->text() == QStringLiteral("1/2") && container->visibleViewports().size() == 2,
            QStringLiteral("Next page did not wrap or restore the previous layout"));
        widget<QAction>(window, "previousViewportPage")->trigger();
        require(page->currentIndex() == 1 && count->text() == QStringLiteral("2/2"), QStringLiteral("Previous page did not wrap"));
        menuAction(*tree, tree->topLevelItem(1), QStringLiteral("删除页"));
        require(page->count() == 1 && count->text() == QStringLiteral("1/1") && container->visibleViewports().size() == 2,
            QStringLiteral("Deleting the additional page damaged the original view layout"));
        require(window.grab().save(output.filePath(QStringLiteral("view-pages.png"))), QStringLiteral("Could not save view page screenshot"));
        log << "View tree, split, cameras, typed picking, paging, and page deletion passed.\n";
        return 0;
    } catch (const std::exception& failure) {
        log << failure.what() << '\n';
        return 1;
    }
}
