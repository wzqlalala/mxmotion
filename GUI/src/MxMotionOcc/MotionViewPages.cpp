#include "MotionViewPages.h"

#include "MotionWindow.h"
#include "MotionWorkspace.h"
#include "OccViewport.h"
#include "ViewportContainer.h"
#include "ViewportLayouts.h"
#include "ViewportWidget.h"
#include "picking/LegacyPickContext.h"

#include <QAction>
#include <QScopedValueRollback>
#include <QSet>
#include <QSignalBlocker>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

using MechMainWindow::ViewportContainer;
using MechMainWindow::viewLayoutSlotCount;
using MechMainWindow::viewLayoutTable;

namespace MotionOcc {
namespace {
QString viewportObjectName(int id)
{
    return QStringLiteral("occViewport-%1").arg(id);
}
}

MotionViewPages::MotionViewPages(MotionWorkspace* workspace, QWidget* parking)
    : m_workspace(workspace), m_parking(parking), m_container(std::make_unique<ViewportContainer>(parking))
{
    m_container->setObjectName(QStringLiteral("occViewportContainer"));
    m_pages.append(newPage());
    connect(m_container.get(), &ViewportContainer::sig_activeViewportChanged, this,
        [this](int id) { selectViewport(id); });
    connect(m_container.get(), &ViewportContainer::sig_viewportShown, this, [this](int id) {
        const auto view = m_views.constFind(id);
        if (view != m_views.cend()) view->widget->update();
    });
}

MotionViewPages::~MotionViewPages() = default;

MotionViewPages::Page MotionViewPages::newPage()
{
    Page page;
    page.name = tr("视图页-%1").arg(m_nextPageNumber++);
    return page;
}

QWidget* MotionViewPages::createHost(MotionWindow& project, QWidget* parent)
{
    auto* host = new QWidget(parent);
    auto* layout = new QVBoxLayout(host);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    project.m_viewHost = host;
    project.activateViewport(createViewport(project));
    if (!m_workspace) {
        m_standaloneProject = &project;
        registerProject(project);
    }
    return host;
}

int MotionViewPages::createViewport(MotionWindow& project)
{
    const int id = m_nextViewportId++;
    auto* view = new OccViewport;
    view->setObjectName(viewportObjectName(id));
    view->setProperty("viewportId", id);
    view->setProperty("viewportProjectNumber", project.m_modelNumber);
    if (project.m_viewport) {
        view->copyPresentationFrom(*project.m_viewport);
        view->copyCameraFrom(*project.m_viewport);
    }
    view->setToolTip(view->mouseNavigationHint());
    m_views.insert(id, {&project, view, {}, project.m_nextProjectViewNumber++});
    auto* frame = m_container->addViewport(id, view);
    frame->setTitle(viewportDisplayTitle(id));
    m_container->setViewportVisible(id, false);
    project.m_viewports.insert(id, view);
    project.refreshViewportPeers();

    connect(view, &OccViewport::cameraChanged, this, [this, id] { synchronizeCameras(id); });
    connect(view, &OccViewport::partPicked, &project, [&project, view](int partId) {
        if (project.m_projectActive && view == project.m_viewport) project.selectPart(partId);
    });
    connect(view, &OccViewport::viewportSelectionChanged, &project, [&project, view] {
        if (project.m_projectActive && view == project.m_viewport) project.updateSelectionInfo();
    });
    view->setProperty("originalAssistConnected", true);
    connect(view, &OccViewport::pickSelectionChanged, &project, [&project, view] {
        if (!project.m_projectActive || view != project.m_viewport) return;
        project.updateSelectionInfo();
        const auto mode = project.property("originalPickAssistMode").toString();
        if (!mode.isEmpty() && mode != QLatin1String("None"))
            QTimer::singleShot(0, &project, [&project] { project.dispatchOriginalMotionAction(QStringLiteral("applyPickAssist")); });
    });
    connect(view, &OccViewport::pickSessionChanged, &project, [&project, view](bool active) {
        if (view != project.m_viewport) return;
        for (auto* other : project.m_viewports) if (other != view) other->setPickEnabled(!active);
        project.m_refreshViewportPickControls(active);
    });
    connect(view, &OccViewport::rotationCenterPickModeChanged, &project, [&project, view](bool active) {
        if (view != project.m_viewport) return;
        if (auto* action = project.command(Command::CustomRotationCenter)) {
            const QSignalBlocker blocker(action);
            action->setChecked(active);
        }
        if (active && project.m_notice)
            project.setStatusNotice(project.tr("自定义旋转中心：请点击模型顶点、棱边或表面设定旋转中心，右键或按 Esc 取消。"));
    });
    connect(view, &OccViewport::rotationCenterChanged, &project, [&project, view](const std::array<double, 3>& pt) {
        if (!project.m_projectActive || view != project.m_viewport || !project.m_notice) return;
        const QString coords = QStringLiteral("(%1, %2, %3) mm")
            .arg(pt[0], 0, 'f', 2).arg(pt[1], 0, 'f', 2).arg(pt[2], 0, 'f', 2);
        switch (view->rotationCenterMode()) {
        case OccViewport::RotationCenterMode::ModelCenter:
            project.setStatusNotice(project.tr("已恢复默认旋转中心（模型包围盒中心）：%1").arg(coords));
            break;
        case OccViewport::RotationCenterMode::ViewCenter:
            project.setStatusNotice(project.tr("已设为视图旋转中心：%1").arg(coords));
            break;
        case OccViewport::RotationCenterMode::CustomPoint:
            project.setStatusNotice(project.tr("已设置自定义旋转中心：%1").arg(coords));
            break;
        }
    });
    connect(view, &OccViewport::graphicsError, &project, [&project](const QString& error) {
        if (project.m_notice) project.setNotice(project.tr("OpenGL：%1").arg(error), true);
    });
    return id;
}

void MotionViewPages::registerProject(MotionWindow& project)
{
    const int id = project.m_viewports.firstKey();
    auto& page = m_pages[m_currentPage];
    page.viewports.append(id);
    changeSelection(m_currentPage, id);
    refreshControls(true);
}

void MotionViewPages::activateProject(MotionWindow& project)
{
    if (project.m_viewports.isEmpty()) return;
    const auto& current = m_pages[m_currentPage];
    const int last = project.m_viewport->property("viewportId").toInt();
    if (current.viewports.contains(last)) {
        selectViewport(last);
        return;
    }
    for (int id : current.viewports) {
        if (project.m_viewports.contains(id)) {
            selectViewport(id);
            return;
        }
    }
    selectViewport(last);
}

void MotionViewPages::mount(MotionWindow& project)
{
    auto* host = project.m_viewHost;
    if (m_container->parentWidget() != host) {
        host->layout()->addWidget(m_container.get());
        m_container->show();
    }
}

void MotionViewPages::beginShutdown()
{
    m_shuttingDown = true;
    m_container->hide();
    m_container->setParent(m_parking);
}

int MotionViewPages::pageOf(int id) const
{
    for (int i = 0; i < m_pages.size(); ++i) if (m_pages[i].viewports.contains(id)) return i;
    return -1;
}

QString MotionViewPages::viewportDisplayTitle(int id) const
{
    const auto view = m_views.constFind(id);
    if (view == m_views.cend()) return {};
    if (!view->customName.isEmpty()) return view->customName;
    const auto title = tr("视口-%1").arg(view->projectViewNumber);
    return m_workspace ? tr("%1· %2").arg(m_workspace->projectName(*view->project), title) : title;
}

void MotionViewPages::projectNameChanged(const MotionWindow& project)
{
    bool changed = false;
    for (int id : project.m_viewports.keys()) {
        auto* frame = m_container->viewport(id);
        const auto title = viewportDisplayTitle(id);
        if (frame->title() == title) continue;
        frame->setTitle(title);
        changed = true;
    }
    if (changed) refreshControls(true);
}

void MotionViewPages::addLayout(int preset)
{
    auto* project = m_workspace ? m_workspace->activeProject() : m_standaloneProject;
    if (project && project->m_viewport->isPicking()) {
        project->setNotice(tr("请先结束当前拾取，再新建布局。"));
        return;
    }
    auto page = newPage();
    page.layout = preset;
    m_pages.append(page);
    selectPage(m_pages.size() - 1);
    refreshControls(true);
}

void MotionViewPages::addViewport(int pageIndex, MotionWindow& project)
{
    if (project.m_projectActive && project.m_viewport->isPicking()) return;
    const int id = createViewport(project);
    m_pages[pageIndex].viewports.append(id);
    changeSelection(pageIndex, id);
    refreshControls(true);
}

void MotionViewPages::selectPage(int index)
{
    if (index < 0 || index >= m_pages.size()) return;
    if (m_pages[index].active >= 0) {
        changeSelection(index, m_pages[index].active);
        return;
    }
    if (const auto previous = m_views.constFind(m_activeViewport); previous != m_views.cend()) {
        if (previous->widget->isPicking()) {
            refreshControls(false);
            return;
        }
        previous->widget->setObjectName(viewportObjectName(previous.key()));
    }
    m_currentPage = index;
    m_activeViewport = -1;
    applyLayout();
    refreshControls(false);
}

void MotionViewPages::selectViewport(int id)
{
    const int page = pageOf(id);
    if (page >= 0) changeSelection(page, id);
}

bool MotionViewPages::changeSelection(int pageIndex, int id)
{
    if (m_shuttingDown || m_changingSelection || !m_views.contains(id)) return false;
    const auto previous = m_views.constFind(m_activeViewport);
    const auto next = m_views.value(id);
    const bool ownerChanged = previous == m_views.cend() || previous->project != next.project;
    if (previous != m_views.cend() && previous.key() != id && previous->project == next.project && previous->widget->isPicking()) {
        m_container->setActiveViewport(m_activeViewport, false);
        refreshControls(false);
        return false;
    }
    const QScopedValueRollback<bool> switching(m_changingSelection, true);
    if (previous != m_views.cend() && previous.key() != id)
        previous->widget->setObjectName(viewportObjectName(previous.key()));
    m_currentPage = pageIndex;
    m_activeViewport = id;
    m_pages[pageIndex].active = id;
    next.widget->setObjectName(QStringLiteral("occViewport"));
    if (m_workspace) m_workspace->showProjectForViewport(*next.project, id);
    else {
        mount(*next.project);
        next.project->activateViewport(id);
    }
    applyLayout();
    refreshControls(ownerChanged);
    return true;
}

QList<int> MotionViewPages::visibleViewports(const Page& page) const
{
    const int slotCount = viewLayoutSlotCount(viewLayoutTable()[page.layout]);
    auto visible = (page.visibleViewports.isEmpty() ? page.viewports : page.visibleViewports).mid(0, slotCount);
    if (!visible.contains(page.active) && page.active >= 0) {
        if (visible.size() == slotCount) visible.removeLast();
        visible.append(page.active);
    }
    return visible;
}

void MotionViewPages::applyLayout()
{
    const auto& page = m_pages[m_currentPage];
    m_container->applyLayout(page.layout, visibleViewports(page));
    m_container->setActiveViewport(page.active, false);
}

void MotionViewPages::setLayout(int layout)
{
    auto& page = m_pages[m_currentPage];
    page.layout = layout;
    page.visibleViewports.clear();
    applyLayout();
    refreshControls(false);
}

void MotionViewPages::tile(int pageIndex, const QList<int>& chosen)
{
    auto& page = m_pages[pageIndex];
    const int active = chosen.contains(page.active) ? page.active : chosen.value(0, -1);
    if (!changeSelection(pageIndex, active)) return;
    page.visibleViewports = chosen;
    applyLayout();
}

void MotionViewPages::renamePage(int index, const QString& name)
{
    m_pages[index].name = name;
    refreshControls(false);
}

void MotionViewPages::renameViewport(int id, const QString& name)
{
    m_views[id].customName = name;
    m_container->viewport(id)->setTitle(viewportDisplayTitle(id));
    refreshControls(false);
}

bool MotionViewPages::canRemoveViewport(int id) const
{
    const auto view = m_views.constFind(id);
    return view != m_views.cend() && view->project->m_viewports.size() > 1 && !view->widget->isPicking();
}

void MotionViewPages::eraseViewport(int id)
{
    const auto view = m_views.take(id);
    auto& project = *view.project;
    project.m_viewports.remove(id);
    if (view.widget == project.m_viewport) {
        if (project.m_viewports.isEmpty()) project.m_viewport = nullptr;
        else project.activateViewport(project.m_viewports.firstKey());
    }
    project.refreshViewportPeers();
    view.widget->setPresentationPeers({});
    view.widget->setObjectName(viewportObjectName(id));
    view.widget->disconnect(&project);
    m_container->setViewportVisible(id, false);
    m_container->removeViewport(id);

    const int pageIndex = pageOf(id);
    if (pageIndex < 0) return;
    auto& page = m_pages[pageIndex];
    page.viewports.removeAll(id);
    page.visibleViewports.removeAll(id);
    if (page.active == id) page.active = page.viewports.value(0, -1);
}

void MotionViewPages::repairLayout()
{
    selectPage(m_currentPage);
    refreshControls(true);
}

void MotionViewPages::eraseEmptyPage(int index)
{
    if (m_pages.size() == 1 || !m_pages[index].viewports.isEmpty()) return;
    m_pages.removeAt(index);
    if (m_currentPage > index || m_currentPage == m_pages.size()) --m_currentPage;
}

void MotionViewPages::removeViewport(int id)
{
    if (!canRemoveViewport(id)) return;
    eraseViewport(id);
    repairLayout();
}

void MotionViewPages::removeViewportModel(int id)
{
    const auto view = m_views.constFind(id);
    if (m_workspace && view != m_views.cend()) m_workspace->closeProject(*view->project);
}

void MotionViewPages::removePage(int index)
{
    if (m_pages.size() <= 1) return;
    const auto ids = m_pages[index].viewports;
    for (int id : ids) if (canRemoveViewport(id)) eraseViewport(id);
    eraseEmptyPage(index);
    repairLayout();
}

void MotionViewPages::removeProject(MotionWindow& project)
{
    if (m_standaloneProject == &project) m_standaloneProject = nullptr;
    QSet<int> affectedPages;
    const auto ids = project.m_viewports.keys();
    for (int id : ids) {
        if (const int page = pageOf(id); page >= 0) affectedPages.insert(page);
        eraseViewport(id);
    }
    if (affectedPages.isEmpty() || m_shuttingDown) return;
    for (int page = m_pages.size() - 1; page >= 0; --page)
        if (affectedPages.contains(page)) eraseEmptyPage(page);
    repairLayout();
}

void MotionViewPages::refreshControls(bool rebuildTree)
{
    if (m_shuttingDown) return;
    auto* project = m_workspace ? m_workspace->activeProject() : m_standaloneProject;
    if (project) project->refreshViewPageControls(rebuildTree);
}

void MotionViewPages::setLinked(bool linked)
{
    m_linked = linked;
    if (linked) synchronizeCameras(m_activeViewport);
}

void MotionViewPages::synchronizeCameras(int source)
{
    if (!m_linked || m_shuttingDown || !m_views.contains(source) || !m_container->isViewportLayoutShown(source)) return;
    for (int id : visibleViewports(m_pages[m_currentPage])) {
        if (id != source) m_views[id].widget->copyCameraFrom(*m_views[source].widget);
    }
}

void MotionWindow::activateViewport(int id)
{
    auto* next = m_viewports.value(id);
    if (!next || next == m_viewport) return;
    if (m_viewport) {
        m_viewport->setAutoRotation({}, false);
        for (auto* button : findChildren<QToolButton*>()) {
            if (button->property("mxOriginalAction").toString() != QLatin1String("slot_autoRotation")) continue;
            const QSignalBlocker blocker(button);
            button->setChecked(false);
        }
    }
    m_viewport = next;
    if (auto* context = LegacyPickContext::forObject(this, false)) context->setViewport(next);
    command(next->mouseNavigationMode() == OccViewport::MouseNavigationMode::Nx ? Command::NavigationNx : Command::NavigationClassic)->setChecked(true);
    const Command leftModes[] = {Command::OrbitSpaceMode, Command::OrbitPlaneMode, Command::PanDragMode};
    command(leftModes[static_cast<int>(next->leftButtonMode())])->setChecked(true);
    {
        auto* customPivot = command(Command::CustomRotationCenter);
        const QSignalBlocker blocker(customPivot);
        customPivot->setChecked(next->isPickingRotationCenter());
    }
    const Command displayModes[] = {Command::ShadedEdges, Command::Shaded, Command::Wireframe, Command::MeshDisplay};
    command(displayModes[static_cast<int>(next->displayMode())])->setChecked(true);
    if (m_viewPageTree) m_refreshViewportPickControls(false);
}

void MotionWindow::refreshViewportPeers()
{
    const auto peers = m_viewports.values();
    for (auto* peer : peers) peer->setPresentationPeers(peers);
}

} // namespace MotionOcc
