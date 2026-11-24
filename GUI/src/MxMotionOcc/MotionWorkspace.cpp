#include "MotionWorkspace.h"

#include "MotionWindow.h"
#include "MotionViewPages.h"
#include "mFramelessWindowHelper.h"
#include "mWindowTitleBar.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>
#include <QCloseEvent>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <memory>

namespace MotionOcc {

MotionWorkspace::MotionWorkspace(const QString& initialPath)
{
    setWindowTitle(QStringLiteral("MxSim.Motion V1.0"));
    setWindowIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/logo.png")));
    resize(1766, 828);
    MechMainWindow::FramelessWindowHelper::installFrameless(this);
    auto* title = new MechMainWindow::mWindowTitleBar(this);
    title->setSeparatorVisible(false);

    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_projects = new QStackedWidget(body);
    m_viewManager = std::make_unique<MotionViewPages>(this, body);
    layout->addWidget(title);
    layout->addWidget(m_projects, 1);
    setCentralWidget(body);

    if (!initialPath.isEmpty()) openProject(initialPath);
    if (!m_active) newProject();
}

MotionWorkspace::~MotionWorkspace()
{
    m_viewManager->beginShutdown();
    if (m_active) m_active->setProjectActive(false);
    m_active = nullptr;
    while (auto* project = projectAt(0)) {
        m_projects->removeWidget(project);
        delete project;
    }
}

MotionWindow* MotionWorkspace::activeProject() const { return m_active; }

MotionWindow* MotionWorkspace::projectAt(int index) const
{
    return qobject_cast<MotionWindow*>(m_projects->widget(index));
}

QString MotionWorkspace::projectName(const MotionWindow& project) const
{
    return project.m_projectPath.isEmpty() ? tr("多体动力学-%1").arg(project.m_modelNumber)
                                          : QFileInfo(project.m_projectPath).completeBaseName();
}

MotionWindow* MotionWorkspace::newProject()
{
    auto* project = new MotionWindow(m_projects, this, m_nextModelNumber++);
    addProject(project);
    return project;
}

bool MotionWorkspace::openProject(const QString& path)
{
    const QFileInfo file(path);
    for (int i = 0; i < m_projects->count(); ++i) {
        auto* project = projectAt(i);
        if (!project->m_projectPath.isEmpty() && QFileInfo(project->m_projectPath) == file) {
            m_viewManager->activateProject(*project);
            return true;
        }
    }
    auto project = std::unique_ptr<MotionWindow>(new MotionWindow(m_projects, this, m_nextModelNumber));
    if (!project->openWorkspaceFile(file.absoluteFilePath())) {
        if (!m_active) newProject();
        m_active->setNotice(project->m_notice->text(), true);
        return false;
    }
    if (project->geometryImportInProgress()) {
        connect(project.get(), &MotionWindow::geometryImportFinished, this, [this, loading = project.get(), pending = true](bool success) mutable {
            if (!pending) return;
            pending = false;
            if (success || m_projects->count() <= 1) return;
            const auto message = loading->m_notice->text();
            removeProject(*loading);
            m_active->setNotice(message, true);
        });
    }
    ++m_nextModelNumber;
    addProject(project.release());
    return true;
}

void MotionWorkspace::addProject(MotionWindow* project)
{
    m_projects->addWidget(project);
    connect(project, &MotionWindow::projectInfoChanged, this, [this, project] {
        m_viewManager->projectNameChanged(*project);
    });
    m_viewManager->projectNameChanged(*project);
    m_viewManager->registerProject(*project);
}

void MotionWorkspace::showProjectForViewport(MotionWindow& project, int viewId)
{
    const int index = m_projects->indexOf(&project);
    if (index < 0) return;
    if (&project == m_active) {
        project.activateViewport(viewId);
        return;
    }
    const auto* oldTreeDock = m_active && m_active->m_treeArea ? m_active->m_treeArea->currentDockWidget() : nullptr;
    const bool keepViewTree = oldTreeDock && oldTreeDock->objectName() == QLatin1String("ViewTree");
    if (m_active) m_active->setProjectActive(false);
    m_active = &project;
    m_active->activateViewport(viewId);
    m_viewManager->mount(project);
    m_projects->setCurrentIndex(index);
    m_active->setProjectActive(true);
    if (keepViewTree) {
        if (auto* dock = m_active->m_docks->findDockWidget(QStringLiteral("ViewTree"))) {
            dock->toggleView(true);
            if (auto* area = dock->dockAreaWidget()) area->setCurrentDockWidget(dock);
        }
    }
    m_active->setFocus();
}

void MotionWorkspace::closeProject(MotionWindow& project)
{
    if (m_projects->indexOf(&project) < 0 || !project.confirmProjectClose(true)) return;
    removeProject(project);
}

void MotionWorkspace::removeProject(MotionWindow& project)
{
    if (&project == m_active) {
        project.setProjectActive(false);
        m_active = nullptr;
    }
    m_projects->removeWidget(&project);
    m_viewManager->removeProject(project);
    project.m_viewManager = nullptr;
    project.hide();
    project.deleteLater();
    if (m_projects->count() == 0) newProject();
    else if (!m_active) m_viewManager->activateProject(*projectAt(m_projects->currentIndex()));
}

bool MotionWorkspace::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    return MechMainWindow::FramelessWindowHelper::handleNativeEvent(this, message, result)
        || QMainWindow::nativeEvent(eventType, message, result);
}

void MotionWorkspace::closeEvent(QCloseEvent* event)
{
    for (int i = 0; i < m_projects->count(); ++i) {
        if (!projectAt(i)->confirmProjectClose()) {
            event->ignore();
            return;
        }
    }
    QMainWindow::closeEvent(event);
}

} // namespace MotionOcc
