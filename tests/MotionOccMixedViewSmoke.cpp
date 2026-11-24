#include "MotionWorkspace.h"
#include "MotionWindow.h"
#include "OccViewport.h"
#include "ViewportContainer.h"
#include "ViewportWidget.h"
#include "compat/mViewportModelManager.h"
#include "compat/model/LegacyMotionContext.h"
#include "mMotionBodyPanel.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPointer>
#include <QSlider>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <cmath>
#include <stdexcept>

namespace {
using namespace MotionOcc;
using MechMainWindow::ViewportContainer;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

template<class T> T* control(const QWidget& parent, const char* name)
{
    auto* child = parent.findChild<T*>(QString::fromLatin1(name));
    require(child, QStringLiteral("Missing control %1").arg(QString::fromLatin1(name)));
    return child;
}

void writeBox(const QString& path, double length, double width, double height)
{
    CadDocument document;
    QString error;
    int id = -1;
    require(document.addPrimitive(Primitive::Box, length, width, height, id, error), error);
    require(document.exportFile(path, id, error), error);
}

OccViewport* visibleView(ViewportContainer& container, int projectNumber)
{
    for (const auto* frame : container.visibleViewports()) {
        auto* view = frame->findChild<OccViewport*>();
        if (view && view->property("viewportProjectNumber").toInt() == projectNumber) return view;
    }
    return nullptr;
}

void click(OccViewport& view, const QPoint& point)
{
    QMouseEvent press(QEvent::MouseButtonPress, point, view.mapToGlobal(point),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &press);
    QMouseEvent release(QEvent::MouseButtonRelease, point, view.mapToGlobal(point),
                        Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&view, &release);
    QApplication::processEvents();
}

void clickViewNode(QTreeWidget& tree, QTreeWidgetItem& item, bool doubleClick = false)
{
    const auto point = tree.visualItemRect(&item).center();
    auto* surface = tree.viewport();
    const auto send = [&](QEvent::Type type, Qt::MouseButtons buttons) {
        QMouseEvent event(type, point, surface->mapToGlobal(point), Qt::LeftButton, buttons, Qt::NoModifier);
        QApplication::sendEvent(surface, &event);
    };
    send(QEvent::MouseButtonPress, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, Qt::NoButton);
    if (doubleClick) {
        send(QEvent::MouseButtonDblClick, Qt::LeftButton);
        send(QEvent::MouseButtonRelease, Qt::NoButton);
    }
    QApplication::processEvents();
}

QTreeWidget& treeFor(MotionWindow& project)
{
    auto* docks = qobject_cast<ads::CDockManager*>(project.centralWidget());
    auto* dock = docks ? docks->findDockWidget(QStringLiteral("Tree")) : nullptr;
    require(dock, QStringLiteral("The project's model tree dock is missing"));
    return *control<QTreeWidget>(*dock, "mTreeView");
}

QTreeWidgetItem* partFor(QTreeWidget& tree, const QString& name)
{
    for (QTreeWidgetItemIterator row(&tree); *row; ++row) {
        if ((*row)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Part) && (*row)->text(0) == name)
            return *row;
    }
    return nullptr;
}

void openBodyEditor(MotionWindow& project)
{
    project.dispatchOriginalMotionAction(QStringLiteral("createMotionBodyOneKey"));
    auto& tree = treeFor(project);
    QTreeWidgetItem* body = nullptr;
    for (QTreeWidgetItemIterator row(&tree); *row; ++row) {
        if ((*row)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Native) &&
            (*row)->data(0, Qt::UserRole + 2).toString() == QLatin1String("bodies")) {
            body = *row;
            break;
        }
    }
    require(body, QStringLiteral("The original motion body row is missing"));
    tree.setCurrentItem(body);
    QApplication::processEvents();
    auto* editor = control<MechMainWindow::mMotionBodyPanel>(project, "originalMotionEditor_bodies");
    require(editor->isVisible(), QStringLiteral("The active project's original motion body editor is hidden"));
}

LegacyMotionContext* activeContext()
{
    return getCVM ? getCVM->getModelData()->context : nullptr;
}

QVector3D cameraDirection(const OccViewport& view)
{
    const auto camera = view.queryCameraState();
    return (camera.eye - camera.target).normalized();
}

QTreeWidget& showLayoutTree(MotionWindow& project)
{
    auto* docks = qobject_cast<ads::CDockManager*>(project.centralWidget());
    auto* dock = docks ? docks->findDockWidget(QStringLiteral("ViewTree")) : nullptr;
    require(dock && dock->dockAreaWidget(), QStringLiteral("The view tree dock is missing"));
    dock->toggleView(true);
    dock->dockAreaWidget()->setCurrentDockWidget(dock);
    QApplication::processEvents();
    return *control<QTreeWidget>(*dock, "viewPageTree");
}

void invokeViewAction(QTreeWidget& tree, QTreeWidgetItem& item, const QString& actionName)
{
    bool invoked = false;
    QTimer::singleShot(0, &tree, [&] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu) return;
        for (auto* entry : menu->actions()) {
            if (entry->text() != actionName || !entry->isEnabled()) continue;
            menu->setActiveAction(entry);
            QKeyEvent activate(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            invoked = true;
            QApplication::sendEvent(menu, &activate);
            return;
        }
        menu->close();
    });
    QMetaObject::invokeMethod(&tree, "customContextMenuRequested", Qt::DirectConnection,
                            Q_ARG(QPoint, tree.visualItemRect(&item).center()));
    require(invoked, QStringLiteral("The view tree menu did not invoke %1").arg(actionName));
    QApplication::processEvents();
}

QTreeWidgetItem& viewNode(QTreeWidget& tree, const OccViewport& viewport)
{
    const auto id = viewport.property("viewportId");
    for (QTreeWidgetItemIterator row(&tree); *row; ++row) {
        if ((*row)->parent() && (*row)->data(0, Qt::UserRole + 1) == id) return **row;
    }
    throw std::runtime_error("The viewport is missing from the view tree");
}

void discardModel(QTreeWidget& tree, const OccViewport& viewport)
{
    bool discarded = false;
    QTimer accept;
    QObject::connect(&accept, &QTimer::timeout, &accept, [&] {
        for (auto* window : QApplication::topLevelWidgets()) {
            auto* box = qobject_cast<QMessageBox*>(window);
            if (!box || !box->isVisible()) continue;
            if (auto* button = box->button(QMessageBox::Discard)) {
                discarded = true;
                button->click();
            } else box->reject();
            return;
        }
    });
    accept.start(15);
    invokeViewAction(tree, viewNode(tree, viewport), QStringLiteral("删除模型"));
    accept.stop();
    require(discarded, QStringLiteral("Closing the changed project did not ask whether to discard changes"));
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
}

void checkPausedFrame(MotionWorkspace& workspace, ViewportContainer& container, OccViewport& comparisonView)
{
    auto* moving = workspace.newProject();
    moving->command(Command::LoadRigidExample)->trigger();
    auto* movingView = visibleView(container, 3);
    require(movingView && comparisonView.isVisible(), QStringLiteral("The test solve does not share the comparison page"));

    moving->command(Command::Solve)->trigger();
    auto& tree = treeFor(*moving);
    QElapsedTimer timeout;
    timeout.start();
    QTreeWidgetItem* animation = nullptr;
    while (!animation && timeout.elapsed() < 20000) {
        QApplication::processEvents();
        for (QTreeWidgetItemIterator row(&tree); *row; ++row) {
            if ((*row)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Animation) && !(*row)->isDisabled()) {
                animation = *row;
                break;
            }
        }
        if (!animation) QThread::msleep(10);
    }
    require(animation, QStringLiteral("The real MotionEngine solve did not produce animation frames"));
    tree.setCurrentItem(animation);
    QApplication::processEvents();
    auto* timeline = control<QSlider>(*moving, "motionTimeline");
    require(timeline->isEnabled() && timeline->maximum() > 3, QStringLiteral("The animation timeline is unavailable"));
    movingView->viewTop();
    const int parkedFrame = timeline->maximum() / 2;
    timeline->setValue(parkedFrame);
    const auto displacement = [movingView] {
        const std::array<double, 3> tip{65, 7, 6};
        // The parameter dock may resize the canvas when focus changes.
        // The orthographic camera measures its span against the shorter side.
        const int pixels = qMax(1, qMin(movingView->width(), movingView->height()));
        return (movingView->projectPoint(tip, 1) - movingView->projectPoint(tip)) / pixels;
    };
    const auto parked = displacement();
    require(std::hypot(parked.x(), parked.y()) > .05, QStringLiteral("The selected MotionEngine frame did not move the arm"));

    click(comparisonView, QPoint(12, 12));
    const auto whileInactive = displacement() - parked;
    require(workspace.activeProject() != moving && movingView->isVisible() &&
                std::hypot(whileInactive.x(), whileInactive.y()) < .002,
                QStringLiteral("The visible inactive project lost its paused result frame"));
    click(*movingView, QPoint(12, 12));
    const auto afterReturn = displacement() - parked;
    require(workspace.activeProject() == moving && timeline->value() == parkedFrame &&
                std::hypot(afterReturn.x(), afterReturn.y()) < .002,
                QStringLiteral("Returning to the result viewport lost its paused timeline and pose"));
}
} // namespace

int runMotionOccMixedViewSmoke(QApplication& app, const QString& directory)
{
    const QDir output(directory);
    if (!QDir().mkpath(output.absolutePath())) return 2;
    QFile report(output.filePath(QStringLiteral("mixed-view-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return 2;
    QTextStream log(&report);

    try {
        const auto firstFile = output.filePath(QStringLiteral("compact-bracket.brep"));
        const auto secondFile = output.filePath(QStringLiteral("long-bracket.brep"));
        writeBox(firstFile, 24, 16, 12);
        writeBox(secondFile, 63, 12, 15);

        MotionWorkspace workspace;
        workspace.resize(1440, 880);
        workspace.show();
        app.processEvents();
        require(!workspace.findChild<QWidget*>(QStringLiteral("motionProjectTabs")),
                QStringLiteral("The workspace added an extra project tab bar"));
        auto* first = workspace.activeProject();
        require(first && !first->isWindow() && first->window() == &workspace,
                QStringLiteral("The first project is not embedded in the main workspace"));
        require(first->importCad(firstFile), QStringLiteral("Could not import the first project's geometry"));
        auto& layouts = showLayoutTree(*first);
        auto* container = control<ViewportContainer>(*first, "occViewportContainer");
        auto* pageCombo = control<QComboBox>(*first, "viewportPageCombo");
        require(layouts.topLevelItem(0), QStringLiteral("The first view page is missing"));
        invokeViewAction(layouts, *layouts.topLevelItem(0), QStringLiteral("新建页"));
        auto* layoutButton = control<QToolButton>(*first, "layoutDropButton");
        auto* preset = control<QToolButton>(*layoutButton, "viewportLayout_2");
        bool paletteShown = false;
        QTimer::singleShot(0, layoutButton, [&] {
            paletteShown = preset->isVisible();
            if (paletteShown) preset->click();
            else layoutButton->menu()->close();
        });
        layoutButton->showMenu();
        require(paletteShown, QStringLiteral("The original viewport layout button did not show the preset palette"));
        app.processEvents();
        auto* split = layouts.topLevelItem(1);
        require(split && split->childCount() == 0 && layouts.topLevelItemCount() == 2 &&
                pageCombo->currentIndex() == 1 && container->visibleViewports().isEmpty(),
                QStringLiteral("The new two-pane layout did not start as an empty second page"));
        invokeViewAction(layouts, *split, QStringLiteral("新建视口"));
        require(layouts.topLevelItem(1)->childCount() == 1 && visibleView(*container, 1),
                QStringLiteral("The first project's viewport was not added to the selected layout"));
        log << "PASS The visible layout tree creates an empty split before adding project viewports\n";

        first->command(Command::New)->trigger();
        auto* second = workspace.activeProject();
        require(second && second != first && second->importCad(secondFile), QStringLiteral("Could not open the second project on the same page"));
        require(!second->isWindow() && second->window() == &workspace,
                QStringLiteral("The second project is not embedded in the main workspace"));
        auto& secondLayouts = showLayoutTree(*second);
        require(control<ViewportContainer>(*second, "occViewportContainer") == container && secondLayouts.topLevelItem(1) &&
                secondLayouts.topLevelItem(1)->childCount() == 2,
                QStringLiteral("The second project's viewport did not join the selected layout"));
        QPointer<OccViewport> firstView = visibleView(*container, 1);
        QPointer<OccViewport> secondView = visibleView(*container, 2);
        require(container->visibleViewports().size() == 2 && firstView && secondView,
                QStringLiteral("The split does not contain two visible project-owned OpenGL viewports"));

        QVector3D minimum, maximum;
        require(firstView->queryVisibleBounds(minimum, maximum) && std::abs((maximum - minimum).x() - 24.0) < .02,
                QStringLiteral("The first viewport displays the wrong project geometry"));
        require(secondView->queryVisibleBounds(minimum, maximum) && std::abs((maximum - minimum).x() - 63.0) < .02,
                QStringLiteral("The second viewport displays the wrong project geometry"));
        firstView->viewFront();
        secondView->viewTop();
        firstView->fitAll();
        secondView->fitAll();
        const auto firstFrame = firstView->grabFramebuffer();
        const auto secondFrame = secondView->grabFramebuffer();
        require(firstFrame.width() > 100 && firstFrame.height() > 100 && secondFrame.width() > 100 && secondFrame.height() > 100,
                QStringLiteral("Both visible projects must have real OpenGL frames"));
        const auto firstCamera = cameraDirection(*firstView);
        require(std::abs(QVector3D::dotProduct(firstCamera, cameraDirection(*secondView))) < .1,
                QStringLiteral("Project view cameras are not independent"));
        require(workspace.grab().save(output.filePath(QStringLiteral("mixed-views.png"))),
                QStringLiteral("Cannot save the mixed-project screenshot"));
        log << "PASS Both projects render distinct geometry in the same split with independent cameras\n";

        auto& firstTree = treeFor(*first);
        auto& secondTree = treeFor(*second);
        auto* firstPart = partFor(firstTree, QStringLiteral("compact-bracket"));
        auto* secondPart = partFor(secondTree, QStringLiteral("long-bracket"));
        require(firstPart && secondPart && firstPart->data(0, Qt::UserRole) == secondPart->data(0, Qt::UserRole),
                QStringLiteral("Fixture projects must contain different parts with the same local ID"));

        clickViewNode(secondLayouts, viewNode(secondLayouts, *firstView));
        require(workspace.activeProject() == second, QStringLiteral("A single click in the view tree switched models"));
        clickViewNode(secondLayouts, viewNode(secondLayouts, *firstView), true);
        require(workspace.activeProject() == first && firstTree.currentItem() == firstPart && firstTree.isVisible(),
                QStringLiteral("Activating the first viewport did not show its project and retain the selected part"));
        require(control<ViewportContainer>(*first, "occViewportContainer") == container && container->visibleViewports().size() == 2,
                QStringLiteral("Changing active project replaced the mixed page"));
        openBodyEditor(*first);
        require(firstView->isPicking() && !secondView->isPicking() && activeContext() && activeContext()->parent() == first,
                QStringLiteral("The original typed picker did not bind to the first project's viewport"));

        click(*secondView, QPoint(12, 12));
        require(workspace.activeProject() == second && secondTree.currentItem() == secondPart,
                QStringLiteral("Activating the second viewport did not switch to its independent model tree"));
        require(!firstView->isPicking() && (!activeContext() || activeContext()->parent() == second),
                QStringLiteral("Changing projects left the previous project's typed picker active"));
        openBodyEditor(*second);
        require(!firstView->isPicking() && secondView->isPicking() && activeContext() && activeContext()->parent() == second,
                QStringLiteral("The original typed picker did not bind to the second project's viewport"));
        click(*firstView, QPoint(12, 12));
        require(workspace.activeProject() == first && !secondView->isPicking() && firstView->isPicking() &&
                activeContext() && activeContext()->parent() == first,
                QStringLiteral("Returning to the first viewport did not restore its original editor and picker"));
        log << "PASS View tree double-click and viewport focus switch model tree, editor and pick ownership\n";

        firstTree.setCurrentItem(partFor(firstTree, QStringLiteral("compact-bracket")));
        auto& activeLayouts = showLayoutTree(*first);
        require(activeLayouts.topLevelItem(1) && activeLayouts.topLevelItem(1)->childCount() == 2,
                QStringLiteral("The shared layout does not list both project viewports"));
        clickViewNode(activeLayouts, viewNode(activeLayouts, *secondView), true);
        secondTree.setCurrentItem(partFor(secondTree, QStringLiteral("long-bracket")));
        auto& receivingLayouts = showLayoutTree(*second);
        require(receivingLayouts.topLevelItem(1), QStringLiteral("The shared layout is missing after switching models"));
        invokeViewAction(receivingLayouts, *receivingLayouts.topLevelItem(1), QStringLiteral("新建视口"));
        QPointer<OccViewport> extraView = visibleView(*container, 2);
        require(workspace.activeProject() == second && extraView && extraView != secondView && firstView->isVisible() &&
                container->findChildren<OccViewport*>().size() == 4,
                QStringLiteral("The additional viewport is not active on the same page"));
        log << "PASS New Viewport adds a view of the active model to the selected page\n";

        const int firstViewId = firstView->property("viewportId").toInt();
        discardModel(receivingLayouts, *extraView);
        require(workspace.activeProject() == first && firstView && !secondView && !extraView,
                QStringLiteral("Closing the second project removed or kept the wrong viewports"));
        require(container->visibleViewports().size() == 1 && visibleView(*container, 1) == firstView &&
                firstView->property("viewportId").toInt() == firstViewId &&
                QVector3D::dotProduct(cameraDirection(*firstView), firstCamera) > .99 &&
                control<QComboBox>(*first, "viewportPageCombo")->currentIndex() == 1 &&
                container->findChildren<OccViewport*>().size() == 2,
                QStringLiteral("Closing another project changed the surviving page or its viewport camera"));
        log << "PASS Closing one project removes its viewport and preserves the other project's page and camera\n";
        checkPausedFrame(workspace, *container, *firstView);
        log << "PASS A real solved project preserves its paused frame while another viewport is active\n";
        return 0;
    } catch (const std::exception& failure) {
        log << "FAIL: " << failure.what() << '\n';
        return 1;
    }
}
