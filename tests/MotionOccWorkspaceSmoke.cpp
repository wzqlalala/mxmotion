#include "MotionWorkspace.h"
#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"
#include "compat/mViewportModelManager.h"
#include "compat/model/LegacyMotionContext.h"
#include "mMotionBodyPanel.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QJsonArray>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <stdexcept>

using namespace MotionOcc;

namespace {

void require(bool condition, const QString& failure)
{
    if (!condition) throw std::runtime_error(failure.toStdString());
}

QTreeWidget& modelTree(const MotionWindow& project)
{
    auto* docks = qobject_cast<ads::CDockManager*>(project.centralWidget());
    auto* dock = docks ? docks->findDockWidget(QStringLiteral("Tree")) : nullptr;
    auto* tree = dock ? dock->findChild<QTreeWidget*>(QStringLiteral("mTreeView")) : nullptr;
    require(tree, QStringLiteral("The active project has no model tree"));
    return *tree;
}

QTreeWidget& showViewTree(MotionWindow& project)
{
    auto* docks = qobject_cast<ads::CDockManager*>(project.centralWidget());
    auto* dock = docks ? docks->findDockWidget(QStringLiteral("ViewTree")) : nullptr;
    require(dock && dock->dockAreaWidget(), QStringLiteral("The active project has no view tree dock"));
    dock->toggleView(true);
    dock->dockAreaWidget()->setCurrentDockWidget(dock);
    QApplication::processEvents();
    auto* tree = dock->findChild<QTreeWidget*>(QStringLiteral("viewPageTree"));
    require(tree, QStringLiteral("The active project has no view tree"));
    return *tree;
}

int viewportCount(QTreeWidget& tree)
{
    int count = 0;
    for (QTreeWidgetItemIterator row(&tree); *row; ++row) if ((*row)->parent()) ++count;
    return count;
}

QTreeWidgetItem& viewNode(QTreeWidget& tree, const OccViewport& viewport)
{
    const auto id = viewport.property("viewportId");
    for (QTreeWidgetItemIterator row(&tree); *row; ++row) {
        if ((*row)->parent() && (*row)->data(0, Qt::UserRole + 1) == id) return **row;
    }
    throw std::runtime_error("The viewport is missing from the view tree");
}

void activateModel(MotionWindow& current, const OccViewport& viewport)
{
    auto& tree = showViewTree(current);
    QMetaObject::invokeMethod(&tree, "itemDoubleClicked", Qt::DirectConnection,
                              Q_ARG(QTreeWidgetItem*, &viewNode(tree, viewport)), Q_ARG(int, 0));
    QApplication::processEvents();
}

int partCount(const MotionWindow& project)
{
    int count = 0;
    for (QTreeWidgetItemIterator item(&modelTree(project)); *item; ++item) {
        if ((*item)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Part)) ++count;
    }
    return count;
}

QTreeWidgetItem* bodyRow(QTreeWidget& tree)
{
    for (QTreeWidgetItemIterator item(&tree); *item; ++item) {
        if ((*item)->data(0, Qt::UserRole + 1).toInt() == int(ModelNode::Native) &&
            (*item)->data(0, Qt::UserRole + 2).toString() == QLatin1String("bodies")) return *item;
    }
    return nullptr;
}

LegacyMotionContext* activeContext()
{
    return getCVM ? getCVM->getModelData()->context : nullptr;
}

void removeModel(MotionWindow& current, const OccViewport& viewport)
{
    auto& tree = showViewTree(current);
    bool invoked = false;
    QTimer::singleShot(0, &tree, [&] {
        auto* menu = qobject_cast<QMenu*>(QApplication::activePopupWidget());
        if (!menu) return;
        for (auto* action : menu->actions()) {
            if (action->text() != QStringLiteral("删除模型") || !action->isEnabled()) continue;
            menu->setActiveAction(action);
            QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
            invoked = true;
            QApplication::sendEvent(menu, &enter);
            return;
        }
        menu->close();
    });
    QMetaObject::invokeMethod(&tree, "customContextMenuRequested", Qt::DirectConnection,
                              Q_ARG(QPoint, tree.visualItemRect(&viewNode(tree, viewport)).center()));
    require(invoked, QStringLiteral("The view tree did not invoke Delete Model"));
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
}

class ProjectDialogs {
public:
    ProjectDialogs()
    {
        QObject::connect(&poll, &QTimer::timeout, &poll, [this] {
            for (auto* window : QApplication::topLevelWidgets()) {
                if (!window->isVisible()) continue;
                if (auto* message = qobject_cast<QMessageBox*>(window)) {
                    if (auto* button = message->button(answer)) {
                        ++confirmed;
                        answer = QMessageBox::NoButton;
                        button->click();
                    } else {
                        unexpected = message->text();
                        message->reject();
                    }
                } else if (auto* file = qobject_cast<QFileDialog*>(window)) {
                    if (savePath.isEmpty()) {
                        unexpected = file->windowTitle();
                        file->reject();
                        continue;
                    }
                    const QString target = savePath;
                    savePath.clear();
                    ++saveDialogs;
                    file->setOption(QFileDialog::DontConfirmOverwrite);
                    file->selectFile(target);
                    if (auto* filename = file->findChild<QLineEdit*>(QStringLiteral("fileNameEdit")))
                        filename->setText(target);
                    QMetaObject::invokeMethod(file, "accept", Qt::DirectConnection);
                }
            }
        });
        poll.start(15);
    }

    void check() const
    {
        require(unexpected.isEmpty(), QStringLiteral("Unexpected project dialog: %1").arg(unexpected));
    }

    QMessageBox::StandardButton answer = QMessageBox::NoButton;
    QString savePath;
    int confirmed = 0;
    int saveDialogs = 0;

private:
    QTimer poll;
    QString unexpected;
};

} // namespace

int runMotionOccWorkspaceSmoke(QApplication&, const QString& directory)
{
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    const QDir output(directory);
    QDir().mkpath(output.absolutePath());
    QFile report(output.filePath(QStringLiteral("workspace-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return 2;
    QTextStream log(&report);

    try {
        QString error;
        CadDocument fixture;
        int boxId = -1;
        require(fixture.addPrimitive(Primitive::Box, 22, 17, 9, boxId, error), error);
        const auto geometry = output.filePath(QStringLiteral("workspace-box.step"));
        require(fixture.exportFile(geometry, boxId, error), error);

        ProjectDialogs dialogs;
        MotionWorkspace workspace;
        workspace.resize(1280, 860);
        workspace.show();
        QApplication::processEvents();
        require(!workspace.findChild<QWidget*>(QStringLiteral("motionProjectTabs")), QStringLiteral("An extra project tab bar is visible"));
        auto* first = workspace.activeProject();
        require(first && first->importCad(geometry) && partCount(*first) == 1,
                QStringLiteral("Could not edit the first project"));
        QApplication::processEvents();

        first->dispatchOriginalMotionAction(QStringLiteral("createMotionBodyOneKey"));
        auto& firstTree = modelTree(*first);
        auto* firstBody = bodyRow(firstTree);
        require(firstBody, QStringLiteral("One-click generation did not create a typed motion body"));
        const int bodyId = firstBody->data(0, Qt::UserRole).toInt();
        firstTree.setCurrentItem(firstBody);
        QApplication::processEvents();
        const auto editorName = QStringLiteral("originalMotionEditor_bodies");
        auto* firstEditor = first->findChild<MechMainWindow::mMotionBodyPanel*>(editorName);
        QPointer<OccViewport> firstView = first->findChild<OccViewport*>(QStringLiteral("occViewport"));
        require(firstEditor && firstEditor->isVisible() && firstView && firstView->isPicking() &&
                activeContext() && activeContext()->parent() == first,
                QStringLiteral("The first project did not open its original motion body editor and part picker"));

        first->command(Command::New)->trigger();
        QApplication::processEvents();
        dialogs.check();
        auto* second = workspace.activeProject();
        require(second && second != first && partCount(*second) == 0 && viewportCount(showViewTree(*second)) == 2,
                QStringLiteral("New did not create an independent empty project"));
        require((!activeContext() || activeContext()->parent() == second) && !firstView->isPicking(),
                QStringLiteral("New left the first project's model context or picker active"));
        require(second->importCad(geometry) && second->importCad(geometry) && partCount(*second) == 2,
                QStringLiteral("Could not edit the second project"));
        QPointer<OccViewport> secondView = second->findChild<OccViewport*>(QStringLiteral("occViewport"));
        require(secondView, QStringLiteral("The second project's active viewport is missing"));
        activateModel(*second, *firstView);
        require(workspace.activeProject() == first && partCount(*first) == 1,
                QStringLiteral("Switching projects lost the first model or its unsaved changes"));
        const auto* restoredBody = firstTree.currentItem();
        const auto* restoredEditor = first->findChild<MechMainWindow::mMotionBodyPanel*>(editorName);
        require(restoredBody && restoredBody == bodyRow(firstTree) && restoredBody->isSelected() &&
                restoredBody->data(0, Qt::UserRole).toInt() == bodyId &&
                restoredEditor && restoredEditor->isVisible() &&
                activeContext() && activeContext()->parent() == first,
                QStringLiteral("Returning to the first project did not restore the typed body, editor and selection"));
        activateModel(*first, *secondView);
        require(workspace.activeProject() == second && partCount(*second) == 2,
                QStringLiteral("Switching projects lost the second model"));
        log << "PASS New and view tree switching isolate model data and unsaved state\n";
        log << "PASS Switching away releases the original picker and returning restores the original body editor\n";

        QFile invalid(output.filePath(QStringLiteral("invalid.mxmotion")));
        require(invalid.open(QIODevice::WriteOnly | QIODevice::Truncate), invalid.errorString());
        require(invalid.write("{") == 1, invalid.errorString());
        invalid.close();
        const auto secondLabel = viewNode(showViewTree(*second), *secondView).text(0);
        require(!workspace.openProject(invalid.fileName()) && viewportCount(showViewTree(*second)) == 2 &&
                workspace.activeProject() == second && partCount(*second) == 2 &&
                viewNode(showViewTree(*second), *secondView).text(0) == secondLabel,
                QStringLiteral("A failed open changed an existing model or added a viewport"));
        dialogs.check();
        log << "PASS A malformed project leaves the workspace intact\n";

        dialogs.answer = QMessageBox::Cancel;
        removeModel(*second, *firstView);
        dialogs.check();
        require(firstView && viewportCount(showViewTree(*second)) == 2 && dialogs.confirmed == 1 && dialogs.saveDialogs == 0,
                QStringLiteral("Cancel did not leave both models open"));

        const auto savedPath = output.filePath(QStringLiteral("first-project.mxmotion"));
        dialogs.answer = QMessageBox::Save;
        dialogs.savePath = savedPath;
        removeModel(*second, *firstView);
        dialogs.check();
        require(!firstView && viewportCount(showViewTree(*second)) == 1 && dialogs.confirmed == 2 && dialogs.saveDialogs == 1 &&
                workspace.activeProject() == second && partCount(*second) == 2,
                QStringLiteral("Closing and saving one project changed the other project"));
        CadDocument reopened;
        RigidModel model;
        require(loadRigidProject(savedPath, reopened, model, error), error);
        const auto savedBodies = model.nativeDocument ? model.nativeDocument->collection(QStringLiteral("bodies")) : QJsonArray{};
        require(reopened.parts().size() == 1 && savedBodies.size() == 1 &&
                savedBodies.first().toObject().value(QStringLiteral("id")).toInt(-1) == bodyId,
                QStringLiteral("Save while closing wrote geometry or motion data from the wrong project"));
        log << "PASS Cancel keeps both models open and Save writes the correct model\n";

        workspace.newProject();
        require(workspace.openProject(savedPath), QStringLiteral("Could not reopen the saved project"));
        QApplication::processEvents();
        auto* restored = workspace.activeProject();
        require(restored && restored != second && partCount(*restored) == 1,
                QStringLiteral("The saved geometry did not reopen"));
        require(secondView, QStringLiteral("Opening a saved model destroyed the surviving viewport"));
        activateModel(*restored, *secondView);
        require(workspace.activeProject() == second && partCount(*second) == 2,
                QStringLiteral("Opening the saved project replaced the independent second project"));
        dialogs.check();
        log << "PASS Reopening a saved model preserves the other open project\n";
        return 0;
    } catch (const std::exception& exception) {
        log << "FAIL: " << exception.what() << '\n';
        return 1;
    }
}
