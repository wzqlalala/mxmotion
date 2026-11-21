#include "MotionWindow.h"
#include "OccViewport.h"
#include "RigidProject.h"

#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ViewportWidget.h"
#include "mFloatingDockFrameless.h"
#include "mFramelessDialog.h"
#include "mFramelessWindowHelper.h"
#include "mUiMenu.h"
#include "mWindowTitleBar.h"

#include <DockAreaWidget.h>
#include <DockManager.h>
#include <DockWidget.h>

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QCursor>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QScreen>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <functional>

using MechMainWindow::ThemeManager;
using MxFunctions::UiMetrics;
using MxFunctions::UiPalette;
using MxFunctions::mUiMenu;

namespace MotionOcc {
namespace {
constexpr int partRole = Qt::UserRole;
constexpr int nodeKindRole = Qt::UserRole + 1;
const auto cadFilters = QStringLiteral("CAD 几何 (*.step *.stp *.iges *.igs *.brep *.brp);;STEP (*.step *.stp);;IGES (*.iges *.igs);;BREP (*.brep *.brp)");
const auto exportFilters = QStringLiteral("STEP (*.step);;IGES (*.iges);;BREP (*.brep)");
const auto projectFilter = QStringLiteral("MxMotion 刚体项目 (*.mxmotion)");

ModelNode treeNode(const QTreeWidgetItem* item)
{
    if (!item) return {};
    return { static_cast<ModelNode::Kind>(item->data(0, nodeKindRole).toInt()), item->data(0, partRole).toInt(),
        item->data(0, Qt::UserRole + 2).toString() };
}

QString treeStateKey(const QTreeWidgetItem* item)
{
    const auto node = treeNode(item);
    if (node.kind != ModelNode::None)
        return QStringLiteral("%1:%2:%3").arg(int(node.kind)).arg(node.id).arg(node.collection);
    if (!item->parent()) return QStringLiteral("root");
    return treeStateKey(item->parent()) + QLatin1Char('/') + item->text(0);
}

QIcon icon(const QString& name)
{
    return QIcon(QStringLiteral(":/mMainWidgets/Icon/") + name);
}

QString withSuffix(QString path, const QString& suffix)
{
    if (!path.isEmpty() && QFileInfo(path).suffix().isEmpty())
        path += QLatin1Char('.') + suffix;
    return path;
}

QDoubleSpinBox* dimension(QWidget* parent, double value)
{
    auto* spin = new QDoubleSpinBox(parent);
    spin->setDecimals(4);
    spin->setRange(0.0001, 1e7);
    spin->setValue(value);
    spin->setSuffix(QStringLiteral(" mm"));
    return spin;
}
}

MotionWindow::MotionWindow(QWidget* parent) : QMainWindow(parent)
{
    setObjectName(QStringLiteral("mMechMainWindow"));
    setWindowTitle(QStringLiteral("MxSim.Motion V1.0"));
    setWindowIcon(icon(QStringLiteral("logo.png")));
    QSize initialSize(1766, 828);
    if (const auto* screen = QApplication::primaryScreen())
        initialSize = initialSize.boundedTo(screen->availableGeometry().size());
    resize(initialSize);
    setMinimumSize(870, 520);
    MechMainWindow::FramelessWindowHelper::installFrameless(this);
    createCommands();
    createRigidCommands();
    createWorkspace();
    createStatusBar();
    createRigidControls();

    // Original mMechMainWindow composition: title and ribbon share a menu wrapper.
    auto* wrapper = new QWidget(this);
    wrapper->setObjectName(QStringLiteral("menuWrapper"));
    wrapper->setAutoFillBackground(true);
    auto* layout = new QVBoxLayout(wrapper);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    auto* title = new MechMainWindow::mWindowTitleBar(this);
    title->setSeparatorVisible(false);
    layout->addWidget(title);
    auto* ribbon = createMotionRibbon(*this, wrapper);
    layout->addWidget(ribbon);
    setMenuWidget(wrapper);
    ThemeManager::registerDockThemeApplier([] {
        for (auto* widget : QApplication::topLevelWidgets())
            if (auto* window = qobject_cast<MotionWindow*>(widget)) window->applyDockTheme();
    });
    ThemeManager::instance()->applyAll(ribbon);
    applyDockTheme();

    m_worker = new QProcess(this);
    m_worker->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_worker, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this,
        [this](int code, QProcess::ExitStatus status) {
            endMesh(code, status == QProcess::CrashExit ? tr("网格进程异常退出。") : QString());
        });
    connect(m_worker, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            endMesh(-1, tr("无法启动网格进程：%1").arg(m_worker->errorString()));
    });
    m_meshTimeout = new QTimer(this);
    m_meshTimeout->setSingleShot(true);
    connect(m_meshTimeout, &QTimer::timeout, this, [this] {
        m_jobError = tr("网格划分超过 120 秒，已终止。请增大单元尺寸。");
        m_worker->kill();
    });
    updateTree();
    updateParameters();
    updateActions();
    setNotice(tr("就绪 · OCC / Netgen / MotionEngine / Qwt"));
    QTimer::singleShot(0, this, [this] {
        resetLayout(false);
        m_viewport->fitAll();
    });
}

MotionWindow::~MotionWindow()
{
    ThemeManager::registerDockThemeApplier({});
    if (m_running) m_running->stop.store(true, std::memory_order_relaxed);
    if (m_solverThread.joinable()) m_solverThread.join();
    if (m_worker && m_worker->state() != QProcess::NotRunning) {
        m_worker->disconnect(this);
        m_worker->kill();
        m_worker->waitForFinished(2000);
    }
}

bool MotionWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    return MechMainWindow::FramelessWindowHelper::handleNativeEvent(this, message, result)
        || QMainWindow::nativeEvent(eventType, message, result);
}

void MotionWindow::closeEvent(QCloseEvent* event)
{
    if (!confirmReplace()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

QAction* MotionWindow::command(Command id) const
{
    return m_commands.at(id);
}

void MotionWindow::createCommands()
{
    auto add = [this](Command id, const QString& text, const QString& image, auto fn,
                   QKeySequence shortcut = {}) {
        auto* action = new QAction(icon(image), text, this);
        m_commands.emplace(id, action);
        if (!shortcut.isEmpty()) {
            action->setShortcut(shortcut);
            addAction(action);
        }
        connect(action, &QAction::triggered, this, fn);
        return action;
    };
    const auto home = QStringLiteral("homepage/");
    const auto geo = QStringLiteral("geometry/");
    const auto view = QStringLiteral("CenterMainWindow/");
    add(Command::New, tr("新建"), home + "xinjian3x.png", [this] { newModel(); }, QKeySequence::New);
    add(Command::Open, tr("打开"), home + "dakai3x.png", [this] { openCadDialog(false); }, QKeySequence::Open);
    add(Command::Save, tr("保存"), home + "baocun3x.png", [this] { saveProject(false); }, QKeySequence::Save);
    add(Command::SaveAs, tr("另存为"), home + "lingcunwei3x.png", [this] { saveProject(true); }, QKeySequence::SaveAs);
    add(Command::Import, tr("导入几何模型"), home + "daoru3x.png", [this] { openCadDialog(true); });
    add(Command::Export, tr("导出几何模型"), home + "daochu3x.png", [this] { saveCad(false); });
    add(Command::Remove, tr("删除部件"), "BrowserTree/shanchu.png", [this] {
        const auto* part = m_document.part(m_partId);
        if (!part || !canRemovePart(m_partId) || QMessageBox::question(this, tr("删除部件"), tr("删除“%1”？").arg(part->name)) != QMessageBox::Yes)
            return;
        m_document.remove(m_partId);
        m_meshes.erase(m_partId);
        m_hidden.erase(m_partId);
        m_partId = -1;
        m_modified = true;
        updateTree();
        syncViewport();
        selectPart(-1);
    });
    add(Command::Box, tr("立方体"), geo + "立方体_大@3x.png", [this] { addPrimitive(Primitive::Box); });
    add(Command::Cylinder, tr("圆柱体"), geo + "圆柱体_大@3x.png", [this] { addPrimitive(Primitive::Cylinder); });
    add(Command::Sphere, tr("球体"), geo + "球体_大@3x.png", [this] { addPrimitive(Primitive::Sphere); });
    add(Command::Fit, tr("自适应视图"), view + "zishiyingshitu.png", [this] { m_viewport->fitAll(); }, QKeySequence(Qt::Key_F));
    add(Command::Front, tr("前视图"), view + "qianshitu.png", [this] { m_viewport->viewFront(); });
    add(Command::Top, tr("俯视图"), view + "xiashitu.png", [this] { m_viewport->viewTop(); });
    add(Command::Right, tr("右视图"), view + "youshitu.png", [this] { m_viewport->viewRight(); });
    add(Command::Isometric, tr("轴侧视图"), view + "zhouceshitu.png", [this] { m_viewport->viewIsometric(); });

    auto* modes = new QActionGroup(this);
    auto mode = [&](Command id, const QString& label, const QString& image, OccViewport::DisplayMode value) {
        auto* action = add(id, label, view + image, [this, value] { m_viewport->setDisplayMode(value); });
        action->setCheckable(true);
        modes->addAction(action);
    };
    mode(Command::ShadedEdges, tr("轮廓线"), "lunkuoxian.png", OccViewport::DisplayMode::ShadedEdges);
    mode(Command::Shaded, tr("实体显示"), "yincangxianmoxing.png", OccViewport::DisplayMode::Shaded);
    mode(Command::Wireframe, tr("线框模型"), "xiankuangmoxing.png", OccViewport::DisplayMode::Wireframe);
    mode(Command::MeshDisplay, tr("网格显示"), "wanggexianshi3x.png", OccViewport::DisplayMode::Mesh);
    command(Command::ShadedEdges)->setChecked(true);
    auto* pick = add(Command::PickPart, tr("部件选择"), "choice/weizhi3x.png",
        [this](bool checked) { m_viewport->setPickEnabled(checked); });
    pick->setCheckable(true);
    pick->setChecked(true);
    add(Command::ClearSelection, tr("清除选择"), "display/yincangquanbu.png", [this] { selectPart(-1); }, QKeySequence(Qt::Key_Escape));
    auto visibilityChanged = [this] { syncViewport(); updateTree(); updateParameters(); updateActions(); };
    add(Command::ShowAll, tr("显示全部"), "display/xianshiquanbu.png", [this, visibilityChanged] {
        m_hidden.clear();
        visibilityChanged();
    });
    add(Command::HideSelected, tr("隐藏当前"), "display/yincangdangqian.png", [this, visibilityChanged] {
        if (m_document.part(m_partId))
            m_hidden.insert(m_partId);
        visibilityChanged();
    });
    add(Command::HideOthers, tr("隐藏其他"), "display/yincangqita.png", [this, visibilityChanged] {
        if (!m_document.part(m_partId)) return;
        m_hidden.clear();
        for (const auto& part : m_document.parts())
            if (part.id != m_partId) m_hidden.insert(part.id);
        visibilityChanged();
    });
    add(Command::InvertVisibility, tr("反转显示"), "display/fanzhuanxianshi.png", [this, visibilityChanged] {
        for (const auto& part : m_document.parts())
            if (m_hidden.erase(part.id) == 0) m_hidden.insert(part.id);
        visibilityChanged();
    });
    add(Command::GenerateMesh, tr("生成网格"), "BrowserTree/chuangjianwangge.png", [this] { beginMeshSetup(); });
    add(Command::RemoveMesh, tr("删除网格"), "BrowserTree/shanchu.png", [this] {
        m_meshes.erase(m_partId);
        if (m_meshes.empty()) command(Command::ShadedEdges)->trigger();
        syncViewport();
        updateParameters();
        updateActions();
        setNotice(tr("已删除当前部件网格。"));
    });
    add(Command::ExportMesh, tr("导出网格"), home + "daochu3x.png", [this] { saveMesh(); });
    add(Command::ResetLayout, tr("重置布局"), home + "chongzhibuju3x.png", [this] { resetLayout(true); });
    add(Command::ManageLayout, tr("管理布局"), home + "guanlibuju3x.png", [this] {
        mUiMenu menu(this);
        for (auto* dock : m_docks->dockWidgetsMap())
            if (dock->objectName() != QStringLiteral("Center")) menu.addAction(dock->toggleViewAction());
        menu.addSeparator();
        menu.addAction(command(Command::ResetLayout));
        ThemeManager::applyMenuStyle(&menu);
        menu.exec(QCursor::pos());
    });
    add(Command::QuickShot, tr("快速截屏"), "post/kuaisujieping.png", [this] {
        const auto path = withSuffix(QFileDialog::getSaveFileName(this, tr("快速截屏"), {}, tr("PNG 图片 (*.png)")), "png");
        if (path.isEmpty()) return;
        if (!m_viewport->glReady() || !m_viewport->grabFramebuffer().save(path))
            setNotice(tr("无法保存视口截图。"), true);
        else setNotice(tr("已保存视口截图：%1").arg(QFileInfo(path).fileName()));
    });
    add(Command::About, tr("关于"), "logo.png", [this] {
        QMessageBox::about(this, tr("关于 MxSim.Motion"),
            tr("MxSim.Motion V1.0 · OCC 构建\n\n"
               "保留 MxMotion 界面，使用 OpenCASCADE、OpenGL 和 Netgen。\n"
               "支持几何文件、基础体和有限元网格。\n"
               "刚体求解使用 MotionEngine，结果绘图使用 Qwt。不包含 CGM、原版 .mxcae、柔性体或接触。"));
    });
    add(Command::Exit, tr("退出"), "close3x.png", [this] { close(); }, QKeySequence::Quit);
}

void MotionWindow::createWorkspace()
{
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    m_docks = new ads::CDockManager(this);
    MechMainWindow::mFloatingDockFrameless::attach(m_docks);
    setCentralWidget(m_docks);

    auto* center = new ads::CDockWidget(m_docks, tr("前处理"));
    center->setObjectName(QStringLiteral("Center"));
    center->setWidget(createViewportFrame());
    m_docks->setCentralWidget(center);

    auto* views = new QTreeWidget;
    views->setHeaderHidden(true);
    views->setIndentation(14);
    auto* viewRoot = new QTreeWidgetItem(views, { tr("视图页 1") });
    auto* viewItem = new QTreeWidgetItem(viewRoot, { tr("前处理") });
    viewItem->setIcon(0, icon(QStringLiteral("viewport/1.svg")));
    views->expandAll();
    connect(views, &QTreeWidget::itemDoubleClicked, this, [this] { m_viewport->fitAll(); });

    auto* viewDock = addPane("ViewTree", tr("视图"), views);
    m_modelDock = addPane("Tree", tr("模型"), createModelBrowser());
    m_parameterDock = addPane("Info", tr("参数"), createParameterPage());
    m_parameterDock->setMinimumSize(225, 150);
    m_modelDock->setMinimumWidth(220);
    auto* parameterArea = m_docks->addDockWidget(ads::LeftDockWidgetArea, m_parameterDock, center->dockAreaWidget());
    m_treeArea = m_docks->addDockWidget(ads::LeftDockWidgetArea, viewDock, parameterArea);
    m_docks->addDockWidgetTabToArea(m_modelDock, m_treeArea);
    m_modelDock->toggleView(true);
    m_parameterDock->toggleView(true);
    m_treeArea->setCurrentDockWidget(m_modelDock);

    m_selectionDetails = new QLabel(tr("当前没有选中的部件。"));
    m_selectionDetails->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_selectionDetails->setContentsMargins(8, 8, 8, 8);
    m_selectionDock = addPane("Select", tr("选择信息"), m_selectionDetails);
    auto* bottom = m_docks->addDockWidget(ads::BottomDockWidgetArea, m_selectionDock);
    m_output = new QPlainTextEdit;
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(200);
    m_outputDock = addPane("Output", tr("信息输出"), m_output);
    m_docks->addDockWidgetTabToArea(m_outputDock, bottom);
    createRigidDocks(bottom);
    m_selectionDock->closeDockWidget();
    m_outputDock->closeDockWidget();
    m_defaultLayout = m_docks->saveState();
}

ads::CDockWidget* MotionWindow::addPane(const QString& key, const QString& title, QWidget* content)
{
    auto* dock = new ads::CDockWidget(m_docks, title);
    dock->setObjectName(key);
    dock->setMinimumSizeHintMode(ads::CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
    if (key == QStringLiteral("Curve")) {
        // A plot canvas must resize with its dock. ADS's default scroll wrapper
        // can otherwise leave most of the axes outside a short bottom pane.
        dock->setWidget(content, ads::CDockWidget::ForceNoScrollArea);
        dock->setMinimumSize(300, 240);
    } else {
        dock->setWidget(content);
    }
    return dock;
}

void MotionWindow::resetLayout(bool resetVisibility)
{
    if (resetVisibility) m_docks->restoreState(m_defaultLayout);
    if (auto* treeArea = m_modelDock->dockAreaWidget()) {
        treeArea->setCurrentDockWidget(m_modelDock);
        const auto sizes = m_docks->splitterSizes(treeArea);
        if (sizes.size() == 3)
            m_docks->setSplitterSizes(treeArea, { 280, 250, qMax(400, width() - 530) });
    }
}

void MotionWindow::applyDockTheme()
{
    QFile style(QStringLiteral(":/mMainWidgets/default.css"));
    QString css;
    if (style.open(QIODevice::ReadOnly)) css = QString::fromUtf8(style.readAll());
    // Color and size tokens used by the original dock manager's preset.
    css += QStringLiteral(
        "ads--CDockAreaTitleBar,ads--CDockAreaTabBar,ads--CDockWidgetTab { background:%1; border:none; }"
        "ads--CDockAreaTitleBar { border-bottom:1px solid %2; }"
        "ads--CDockWidgetTab { height:%3px; }"
        "ads--CDockWidgetTab QLabel { color:%4; font-size:%5px; background:transparent; font-weight:400; }"
        "ads--CDockWidgetTab[activeTab=\"true\"] { background:%6; }"
        "ads--CDockWidgetTab:hover { background:%7; }"
        "ads--CDockSplitter::handle { background:#a8b3c2; }"
        "ads--CDockSplitter::handle:horizontal { width:1px; }"
        "ads--CDockSplitter::handle:vertical { height:1px; }")
        .arg(UiPalette::headerHex(), UiPalette::borderHex()).arg(UiMetrics::windowHeaderHeight())
        .arg(UiPalette::textHex()).arg(UiMetrics::bodyFontSize())
        .arg(UiPalette::surfaceHex(), UiPalette::hoverHex());
    m_docks->setStyleSheet(css);
}

QWidget* MotionWindow::createViewportFrame()
{
    auto* frame = new QWidget;
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* toolRow = new QHBoxLayout;
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(0);
    auto* viewTools = new QToolBar(tr("视图工具栏"), frame);
    viewTools->setObjectName(QStringLiteral("viewToolBar"));
    viewTools->setIconSize(QSize(20, 20));
    viewTools->addAction(command(Command::Fit));
    viewTools->addSeparator();
    for (const auto id : { Command::Front, Command::Top, Command::Right, Command::Isometric })
        viewTools->addAction(command(id));
    viewTools->addSeparator();
    for (const auto id : { Command::ShadedEdges, Command::Wireframe, Command::Shaded, Command::MeshDisplay })
        viewTools->addAction(command(id));
    viewTools->addSeparator();
    viewTools->addAction(command(Command::PickPart));
    viewTools->addAction(command(Command::ClearSelection));
    toolRow->addWidget(viewTools);
    toolRow->addStretch(1);

    auto* viewDisplay = new QToolBar(tr("视图显示栏"), frame);
    viewDisplay->setObjectName(QStringLiteral("viewDisplayBar"));
    viewDisplay->setIconSize(QSize(20, 20));
    auto* page = new QComboBox(viewDisplay);
    page->addItem(tr("视图页 1"));
    page->setMinimumWidth(105);
    viewDisplay->addWidget(page);
    auto* oneView = viewDisplay->addAction(icon(QStringLiteral("viewport/1.svg")), tr("单视图"));
    oneView->setCheckable(true);
    oneView->setChecked(true);
    connect(oneView, &QAction::triggered, this, [oneView] { oneView->setChecked(true); });
    viewDisplay->addWidget(new QLabel(QStringLiteral("1/1"), viewDisplay));
    toolRow->addWidget(viewDisplay);
    layout->addLayout(toolRow);

    m_viewport = new OccViewport;
    m_viewport->setObjectName(QStringLiteral("occViewport"));
    m_viewport->setToolTip(tr("左键拖动旋转 / 中键或右键平移 / 滚轮缩放 / 左键点选部件"));
    auto* viewportFrame = new MechMainWindow::ViewportWidget(1, m_viewport, frame);
    viewportFrame->setTitle(tr("前处理"));
    viewportFrame->setState(MechMainWindow::ViewportWidget::State::Maximized);
    layout->addWidget(viewportFrame, 1);

    connect(m_viewport, &OccViewport::partPicked, this, &MotionWindow::selectPart);
    connect(m_viewport, &OccViewport::graphicsError, this, [this](const QString& error) {
        setNotice(tr("OpenGL：%1").arg(error), true);
    });
    return frame;
}

QWidget* MotionWindow::createModelBrowser()
{
    auto* panel = new QWidget;
    auto* outer = new QVBoxLayout(panel);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto* bar = new QWidget(panel);
    auto* form = new QGridLayout(bar);
    form->setContentsMargins(6, 5, 6, 5);
    form->setHorizontalSpacing(4);
    form->setVerticalSpacing(4);
    auto* filterLabel = new QLabel(tr("过滤："), bar);
    auto* sortLabel = new QLabel(tr("排序："), bar);
    m_filter = new QLineEdit(bar);
    m_filter->setPlaceholderText(tr("搜索概要"));
    m_filter->setClearButtonEnabled(true);
    m_visibilityFilter = new QComboBox(bar);
    m_visibilityFilter->addItems({ tr("全部"), tr("显示"), tr("隐藏") });
    m_sortType = new QComboBox(bar);
    m_sortType->addItems({ tr("创建顺序"), tr("名称") });
    form->addWidget(filterLabel, 0, 0);
    form->addWidget(m_filter, 0, 1);
    form->addWidget(m_visibilityFilter, 0, 2);
    form->addWidget(sortLabel, 1, 0);
    form->addWidget(m_sortType, 1, 1, 1, 2);
    form->setColumnStretch(1, 1);
    outer->addWidget(bar);

    m_tree = new QTreeWidget(panel);
    m_tree->setObjectName(QStringLiteral("mTreeView"));
    m_tree->setHeaderHidden(true);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setIndentation(14);
    m_tree->setUniformRowHeights(true);
    m_tree->setIconSize(QSize(16, 16));
    m_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree->setStyleSheet(QStringLiteral("QTreeWidget { border:0; } QTreeWidget::item { min-height:24px; }"));
    outer->addWidget(m_tree, 1);
    ThemeManager::instance()->applyTreeTheme(m_tree);
    ThemeManager::applyTreeFilterBarTheme({ nullptr, m_visibilityFilter, m_sortType,
        m_filter, filterLabel, sortLabel, nullptr, bar });
    connect(m_filter, &QLineEdit::textChanged, this, &MotionWindow::filterTree);
    connect(m_visibilityFilter, qOverload<int>(&QComboBox::currentIndexChanged), this, &MotionWindow::filterTree);
    connect(m_sortType, qOverload<int>(&QComboBox::currentIndexChanged), this, &MotionWindow::updateTree);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        if (m_syncingTree) return;
        const auto node = treeNode(item);
        if (node.kind == ModelNode::Part || node.kind == ModelNode::None) selectPart(node.id);
        else selectRigidNode(node);
    });
    connect(m_tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        const auto node = treeNode(item);
        if (m_syncingTree || column != 0 || node.kind != ModelNode::Part) return;
        if (item->checkState(0) == Qt::Checked) m_hidden.erase(node.id);
        else m_hidden.insert(node.id);
        filterTree();
        syncViewport();
        updateParameters();
        updateActions();
    });
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        const auto node = treeNode(item);
        if (node.kind == ModelNode::Animation || node.kind == ModelNode::Curves || node.kind == ModelNode::Export)
            showRigidResult(node);
    });
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &MotionWindow::showPartMenu);
    return panel;
}

QWidget* MotionWindow::createParameterPage()
{
    auto* panel = new QWidget;
    auto* layout = new QVBoxLayout(panel);
    m_parameterLayout = layout;
    layout->setContentsMargins(8, 7, 8, 7);
    layout->setSpacing(6);
    m_parameterTitle = new QLabel(tr("参数设置"), panel);
    auto titleFont = m_parameterTitle->font();
    titleFont.setBold(true);
    m_parameterTitle->setFont(titleFont);
    m_parameterTitle->setTextFormat(Qt::PlainText);
    m_parameterDetails = new QLabel(panel);
    m_parameterDetails->setObjectName(QStringLiteral("partSummary"));
    m_parameterDetails->setTextFormat(Qt::PlainText);
    m_parameterDetails->setWordWrap(true);
    layout->addWidget(m_parameterTitle);
    layout->addWidget(m_parameterDetails);

    m_meshForm = new QWidget(panel);
    auto* form = new QFormLayout(m_meshForm);
    form->setContentsMargins(0, 3, 0, 0);
    m_meshKind = new QComboBox(m_meshForm);
    m_meshKind->addItems({ tr("四面体体网格"), tr("三角形面网格") });
    m_meshSize = dimension(m_meshForm, 5.0);
    m_meshSize->setRange(0.0001, 100000.0);
    form->addRow(tr("划分类型"), m_meshKind);
    form->addRow(tr("单元尺寸"), m_meshSize);
    auto* buttons = new QWidget(m_meshForm);
    auto* row = new QHBoxLayout(buttons);
    row->setContentsMargins(0, 2, 0, 0);
    m_meshGenerateButton = new QPushButton(tr("生成"), buttons);
    m_meshGenerateButton->setObjectName(QStringLiteral("generateMeshButton"));
    m_meshCancelButton = new QPushButton(tr("取消"), buttons);
    row->addWidget(m_meshGenerateButton);
    row->addWidget(m_meshCancelButton);
    form->addRow(buttons);
    connect(m_meshGenerateButton, &QPushButton::clicked, this, &MotionWindow::beginMesh);
    connect(m_meshCancelButton, &QPushButton::clicked, this, &MotionWindow::cancelMesh);
    m_meshForm->hide();
    layout->addWidget(m_meshForm);
    layout->addStretch(1);
    return panel;
}

void MotionWindow::createStatusBar()
{
    auto* bar = new QStatusBar(this);
    bar->setFixedHeight(24);
    bar->setSizeGripEnabled(false);
    m_notice = new QLabel(bar);
    m_notice->setTextFormat(Qt::PlainText);
    m_notice->setMargin(3);
    m_selection = new QLabel(bar);
    m_selection->setTextFormat(Qt::PlainText);
    auto* unit = new QLabel(tr("mm"), bar);
    unit->setMinimumWidth(58);
    unit->setAlignment(Qt::AlignCenter);
    m_progress = new QProgressBar(bar);
    m_progress->setRange(0, 0);
    m_progress->setFixedSize(132, 14);
    m_progress->hide();
    bar->addWidget(m_notice, 1);
    bar->addPermanentWidget(m_progress);
    bar->addPermanentWidget(m_selection);
    bar->addPermanentWidget(unit);
    setStatusBar(bar);
}

void MotionWindow::updateTree()
{
    const QSignalBlocker block(m_tree);
    // Refreshes from solving, editing and visibility must not undo the user's
    // navigation state by expanding every populated group again.
    std::map<QString, bool> expanded;
    std::function<void(QTreeWidgetItem*)> remember = [&](QTreeWidgetItem* item) {
        expanded.emplace(treeStateKey(item), item->isExpanded());
        for (int i = 0; i < item->childCount(); ++i) remember(item->child(i));
    };
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) remember(m_tree->topLevelItem(i));
    m_tree->clear();
    auto* root = new QTreeWidgetItem(m_tree, { tr("多体动力学-%1").arg(m_modelNumber) });
    root->setIcon(0, icon(QStringLiteral("BrowserTree/moxing3x.png")));
    const QStringList groups = { tr("部件"), tr("运动体"), tr("柔性体"), tr("运动副"), tr("耦合副"), tr("驱动"),
        tr("弹簧"), tr("力元"), tr("传感器"), tr("控制器"), tr("标记点"), tr("接触"), tr("控制输入") };
    std::map<QString, QTreeWidgetItem*> groupItems;
    for (const auto& name : groups) {
        auto* group = new QTreeWidgetItem(root, { name });
        groupItems.emplace(name, group);
        group->setIcon(0, icon(QStringLiteral("BrowserTree/genericPart.png")));
        if (name == groups.front()) m_partsItem = group;
    }
    std::vector<const CadPart*> sorted;
    for (const auto& part : m_document.parts()) sorted.push_back(&part);
    if (m_sortType->currentIndex() == 1)
        std::stable_sort(sorted.begin(), sorted.end(), [](const CadPart* a, const CadPart* b) {
            return QString::localeAwareCompare(a->name, b->name) < 0;
        });
    for (const auto* part : sorted) {
        auto* item = new QTreeWidgetItem(m_partsItem, { part->name });
        item->setData(0, partRole, part->id);
        item->setData(0, nodeKindRole, int(ModelNode::Part));
        item->setIcon(0, icon(QStringLiteral("BrowserTree/genericPart.png")));
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0, m_hidden.count(part->id) ? Qt::Unchecked : Qt::Checked);
        if (part->id == m_partId) m_tree->setCurrentItem(item);
    }
    appendRigidNodes(root, groupItems);
    root->setExpanded(true);
    m_partsItem->setExpanded(true);
    std::function<void(QTreeWidgetItem*)> restore = [&](QTreeWidgetItem* item) {
        const auto previous = expanded.find(treeStateKey(item));
        if (previous != expanded.end()) item->setExpanded(previous->second);
        for (int i = 0; i < item->childCount(); ++i) restore(item->child(i));
    };
    restore(root);
    if (auto* selected = m_tree->currentItem(); selected && !expanded.count(treeStateKey(selected)))
        for (auto* parent = selected->parent(); parent; parent = parent->parent()) parent->setExpanded(true);
    filterTree();
}

void MotionWindow::filterTree()
{
    if (!m_tree) return;
    const auto needle = m_filter->text().trimmed();
    const int visibility = m_visibilityFilter->currentIndex();
    std::function<bool(QTreeWidgetItem*, bool)> filter = [&](QTreeWidgetItem* item, bool parentMatches) {
        const bool matches = parentMatches || item->text(0).contains(needle, Qt::CaseInsensitive);
        bool childMatches = false;
        for (int i = 0; i < item->childCount(); ++i)
            childMatches = filter(item->child(i), matches) || childMatches;
        bool visible = matches || childMatches;
        const auto node = treeNode(item);
        if (node.kind == ModelNode::Part) {
            const bool shown = m_hidden.count(node.id) == 0;
            visible = visible && (visibility == 0 || (visibility == 1 && shown) || (visibility == 2 && !shown));
        }
        // Keep structural ancestors only if they have a matching descendant;
        // preserve empty groups in the unfiltered original framework.
        if (item->childCount() && node.kind == ModelNode::None && !needle.isEmpty())
            visible = childMatches || matches;
        item->setHidden(!visible);
        return visible;
    };
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) filter(m_tree->topLevelItem(i), false);
}

void MotionWindow::updateParameters()
{
    if (m_editor) return;
    const auto* part = m_document.part(m_partId);
    QString summary = part ? tr("部件编号：%1\n几何状态：%2").arg(part->id).arg(m_hidden.count(m_partId) ? tr("隐藏") : tr("显示"))
                           : tr("尚未选择部件。");
    const auto mesh = m_meshes.find(m_partId);
    if (mesh != m_meshes.end()) {
        summary += tr("\n节点：%1\n三角面：%2\n四面体：%3")
            .arg(qulonglong(mesh->second.nodes.size())).arg(qulonglong(mesh->second.surface.size()))
            .arg(qulonglong(mesh->second.tetrahedra.size()));
    } else if (part) {
        summary += tr("\n有限元网格：无");
    }
    m_parameterTitle->setText(!m_meshForm->isHidden() ? tr("生成网格") : part ? part->name : tr("参数设置"));
    m_parameterDetails->setText(summary);
    m_selectionDetails->setText(summary);
    m_selection->setText(part ? tr("选择：%1").arg(part->name) : tr("选择：无"));
}

void MotionWindow::updateActions()
{
    const bool hasPart = m_document.part(m_partId) != nullptr;
    const bool anyPart = !m_document.parts().empty();
    const bool hasMesh = m_meshes.count(m_partId) > 0;
    const bool busy = m_jobPart != -1 || motionBusy();
    for (const auto id : { Command::Export, Command::InvertVisibility })
        command(id)->setEnabled(anyPart);
    for (const auto id : { Command::Save, Command::SaveAs })
        command(id)->setEnabled(!busy && (anyPart || !m_model.scenarios.empty()));
    command(Command::ShowAll)->setEnabled(!m_hidden.empty());
    command(Command::HideSelected)->setEnabled(hasPart && m_hidden.count(m_partId) == 0);
    command(Command::HideOthers)->setEnabled(hasPart && m_document.parts().size() > 1);
    command(Command::Remove)->setEnabled(hasPart && !busy && canRemovePart(m_partId));
    command(Command::GenerateMesh)->setEnabled(hasPart && !busy);
    command(Command::RemoveMesh)->setEnabled(hasMesh && !busy);
    command(Command::ExportMesh)->setEnabled(hasMesh);
    command(Command::MeshDisplay)->setEnabled(!m_meshes.empty());
    m_meshGenerateButton->setEnabled(hasPart && !busy);
    m_meshKind->setEnabled(!busy);
    m_meshSize->setEnabled(!busy);
    for (const auto id : { Command::New, Command::Open, Command::Import, Command::Box, Command::Cylinder, Command::Sphere })
        command(id)->setEnabled(!busy);
    updateRigidActions();
}

void MotionWindow::syncViewport(bool fit)
{
    m_viewport->setScene(m_document, m_meshes, m_hidden);
    m_viewport->selectPart(m_partId);
    if (fit) m_viewport->fitAll();
}

void MotionWindow::selectPart(int id)
{
    leaveRigidAnimation();
    const int previous = m_partId;
    m_partId = m_document.part(id) ? id : -1;
    m_selectionNode = m_partId > 0 ? ModelNode{ModelNode::Part, m_partId} : ModelNode{};
    if (m_partId != previous || m_partId <= 0) closeEditor();
    if (m_jobPart == -1 && m_partId != previous) m_meshForm->hide();
    m_viewport->selectPart(m_partId);
    m_syncingTree = true;
    QTreeWidgetItem* item = nullptr;
    if (m_partId >= 0 && m_partsItem) {
        for (int i = 0; i < m_partsItem->childCount(); ++i)
            if (m_partsItem->child(i)->data(0, partRole).toInt() == m_partId) {
                item = m_partsItem->child(i);
                break;
            }
    }
    const auto* current = m_tree->currentItem();
    if (item) m_tree->setCurrentItem(item);
    else if (current && current->data(0, partRole).isValid()) m_tree->setCurrentItem(nullptr);
    m_syncingTree = false;
    updateParameters();
    updateActions();
}

void MotionWindow::showPartMenu(const QPoint& position)
{
    auto* item = m_tree->itemAt(position);
    if (!item) return;
    const auto node = treeNode(item);
    if (node.kind != ModelNode::None && node.kind != ModelNode::Part) {
        showRigidMenu(node, m_tree->viewport()->mapToGlobal(position));
        return;
    }
    mUiMenu menu(this);
    if (item == m_partsItem) {
        menu.addAction(command(Command::Import));
        auto* exportAll = menu.addAction(command(Command::Export)->icon(), tr("导出全部部件"));
        exportAll->setEnabled(!m_document.parts().empty());
        connect(exportAll, &QAction::triggered, this, [this] { saveCad(true); });
        menu.addSeparator();
        menu.addAction(command(Command::ShowAll));
    } else if (node.kind == ModelNode::Part) {
        selectPart(node.id);
        menu.addAction(command(Command::Export));
        if (m_hidden.count(m_partId)) {
            auto* show = menu.addAction(tr("显示部件"));
            connect(show, &QAction::triggered, this, [this] {
                m_hidden.erase(m_partId);
                syncViewport();
                updateTree();
                updateParameters();
                updateActions();
            });
        } else menu.addAction(command(Command::HideSelected));
        menu.addAction(command(Command::Remove));
        menu.addSeparator();
        menu.addAction(command(Command::GenerateMesh));
        menu.addAction(command(Command::RemoveMesh));
        menu.addAction(command(Command::ExportMesh));
    } else return;
    ThemeManager::applyMenuStyle(&menu);
    menu.exec(m_tree->viewport()->mapToGlobal(position));
}

void MotionWindow::setNotice(const QString& message, bool error)
{
    m_notice->setText(message);
    m_notice->setToolTip(message);
    m_notice->setStyleSheet(error ? QStringLiteral("color:%1").arg(UiPalette::errorHex()) : QString());
    m_output->appendPlainText(message);
}

bool MotionWindow::importCad(const QString& path)
{
    if (motionBusy()) return false;
    std::vector<int> ids;
    QString error;
    if (!m_document.importFile(path, ids, error)) {
        setNotice(error, true);
        return false;
    }
    m_modified = true;
    m_partId = ids.front();
    updateTree();
    syncViewport(true);
    selectPart(m_partId);
    for (const auto& note : m_document.importNotes()) m_output->appendPlainText(note);
    setNotice(tr("已导入 %1 · %2 个部件").arg(QFileInfo(path).fileName()).arg(ids.size())
        + (m_document.importNotes().isEmpty() ? QString{} : tr(" · %1 条导入说明").arg(m_document.importNotes().size())));
    return true;
}

void MotionWindow::openCadDialog(bool addToModel)
{
    const auto path = QFileDialog::getOpenFileName(this, addToModel ? tr("导入几何模型") : tr("打开项目或 CAD 几何"), {},
        addToModel ? cadFilters : projectFilter + QStringLiteral(";;") + cadFilters);
    if (path.isEmpty()) return;
    if (addToModel) {
        importCad(path);
        return;
    }
    if (QFileInfo(path).suffix().compare(QStringLiteral("mxmotion"), Qt::CaseInsensitive) == 0) {
        openRigidProject(path);
        return;
    }
    if (!confirmReplace()) return;
    CadDocument next;
    QString error;
    std::vector<int> ids;
    if (!next.importFile(path, ids, error)) {
        setNotice(error, true);
        return;
    }
    closeEditor();
    clearResults();
    m_model = {};
    m_selectionNode = {};
    m_document = std::move(next);
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath.clear();
    m_modified = false;
    m_partId = ids.front();
    updateTree();
    syncViewport(true);
    selectPart(m_partId);
    setNotice(tr("已打开 %1 · %2 个部件").arg(QFileInfo(path).fileName()).arg(ids.size()));
}

void MotionWindow::saveCad(bool all)
{
    if (m_document.parts().empty()) return;
    const int id = all ? -1 : m_partId;
    const auto* part = m_document.part(id);
    QString path;
    {
        auto filter = exportFilters.section(QStringLiteral(";;"), 0, 0);
        const auto initial = all ? tr("多体动力学-%1.step").arg(m_modelNumber)
                                 : (part ? part->name + QStringLiteral(".step") : QString());
        path = QFileDialog::getSaveFileName(this, all ? tr("保存 CAD 几何") : tr("导出几何模型"), initial, exportFilters, &filter);
        const auto suffix = filter.startsWith("IGES") ? QStringLiteral("iges")
                          : filter.startsWith("BREP") ? QStringLiteral("brep") : QStringLiteral("step");
        path = withSuffix(path, suffix);
    }
    if (path.isEmpty()) return;
    QString error;
    if (!m_document.exportFile(path, id, error)) {
        setNotice(error, true);
        return;
    }
    setNotice(tr("已保存几何：%1").arg(QFileInfo(path).fileName()));
}

bool MotionWindow::confirmReplace()
{
    if (motionBusy()) return false;
    if (!m_modified) return true;
    return QMessageBox::question(this, tr("未保存的项目"), tr("放弃当前未保存的项目更改？"),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Discard;
}

void MotionWindow::newModel()
{
    if (!confirmReplace()) return;
    closeEditor();
    clearResults();
    m_model = {};
    m_selectionNode = {};
    m_document.clear();
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath.clear();
    m_modified = false;
    m_partId = -1;
    ++m_modelNumber;
    updateTree();
    syncViewport(true);
    selectPart(-1);
    setNotice(tr("已新建模型。"));
}

void MotionWindow::addPrimitive(Primitive primitive)
{
    MechMainWindow::mFramelessDialog dialog(this);
    dialog.setWindowTitle(command(primitive == Primitive::Box ? Command::Box
                                 : primitive == Primitive::Cylinder ? Command::Cylinder : Command::Sphere)->text());
    auto* content = new QWidget(&dialog);
    auto* form = new QFormLayout(content);
    auto* first = dimension(content, primitive == Primitive::Box ? 30.0 : 12.0);
    auto* second = dimension(content, 20.0);
    auto* third = dimension(content, 16.0);
    form->addRow(primitive == Primitive::Box ? tr("长度") : tr("半径"), first);
    if (primitive != Primitive::Sphere) form->addRow(primitive == Primitive::Box ? tr("宽度") : tr("高度"), second);
    else second->hide();
    if (primitive == Primitive::Box) form->addRow(tr("高度"), third);
    else third->hide();
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, content);
    form->addRow(buttons);
    dialog.setContentWidget(content);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    ThemeManager::instance()->applyDialogTheme(content);
    if (dialog.exec() != QDialog::Accepted) return;
    QString error;
    int id = -1;
    if (!m_document.addPrimitive(primitive, first->value(), second->value(), third->value(), id, error)) {
        setNotice(error, true);
        return;
    }
    m_modified = true;
    m_partId = id;
    updateTree();
    syncViewport(true);
    selectPart(id);
    setNotice(tr("已创建 %1").arg(m_document.part(id)->name));
}

void MotionWindow::beginMeshSetup()
{
    leaveRigidAnimation();
    closeEditor();
    const auto* part = m_document.part(m_partId);
    if (!part || m_jobPart != -1) return;
    const auto& verts = part->display.vertices;
    if (!verts.empty()) {
        QVector3D low = verts.front(), high = low;
        for (const auto& v : verts) {
            for (int i = 0; i < 3; ++i) {
                low[i] = qMin(low[i], v[i]);
                high[i] = qMax(high[i], v[i]);
            }
        }
        const auto span = high - low;
        m_meshSize->setValue(qBound(0.0001, double(qMax(span.x(), qMax(span.y(), span.z()))) / 8.0, 100000.0));
    }
    m_meshForm->show();
    m_parameterDock->toggleView(true);
    if (auto* area = m_parameterDock->dockAreaWidget()) area->setCurrentDockWidget(m_parameterDock);
    updateParameters();
}

void MotionWindow::beginMesh()
{
    if (!m_document.part(m_partId) || m_jobPart != -1) return;
    auto dir = std::make_unique<QTemporaryDir>(QDir::temp().filePath(QStringLiteral("MxMotionMesh-XXXXXX")));
    if (!dir->isValid()) {
        setNotice(tr("无法创建网格工作目录。"), true);
        return;
    }
    QString error;
    if (!m_document.exportFile(dir->filePath(QStringLiteral("source.brep")), m_partId, error)) {
        setNotice(error, true);
        return;
    }
#ifdef Q_OS_WIN
    const auto workerName = QStringLiteral("MxMotionMeshWorker.exe");
#else
    const auto workerName = QStringLiteral("MxMotionMeshWorker");
#endif
    m_worker->setProgram(QDir(QApplication::applicationDirPath()).filePath(workerName));
    m_worker->setWorkingDirectory(dir->path());
    m_jobKind = m_meshKind->currentIndex() == 0 ? MeshKind::Volume : MeshKind::Surface;
    m_worker->setArguments({ m_jobKind == MeshKind::Volume ? "volume" : "surface",
        "source.brep", "result.vol", QString::number(m_meshSize->value(), 'g', 16) });
    m_jobPart = m_partId;
    m_jobError.clear();
    m_jobDir = std::move(dir);
    m_progress->show();
    updateActions();
    setNotice(tr("正在划分网格…"));
    m_meshTimeout->start(120000);
    m_worker->start();
}

void MotionWindow::endMesh(int exitCode, const QString& processError)
{
    if (m_jobPart == -1) return;
    m_meshTimeout->stop();
    const int part = m_jobPart;
    const auto log = QString::fromLocal8Bit(m_worker->readAllStandardOutput()).trimmed();
    if (!log.isEmpty()) m_output->appendPlainText(log.right(10000));
    QString message = m_jobError.isEmpty() ? processError : m_jobError;
    FemMesh mesh;
    if (message.isEmpty() && exitCode == 0) {
        if (!m_jobDir) message = tr("网格工作目录已消失。");
        else if (readNetgenMesh(m_jobDir->filePath(QStringLiteral("result.vol")), m_jobKind, mesh, message)) {
            const auto nodes = qulonglong(mesh.nodes.size());
            const auto faces = qulonglong(mesh.surface.size());
            const auto tets = qulonglong(mesh.tetrahedra.size());
            m_meshes[part] = std::move(mesh);
            command(Command::MeshDisplay)->setEnabled(true);
            command(Command::MeshDisplay)->trigger();
            syncViewport();
            m_meshForm->hide();
            setNotice(tr("网格完成：%1 节点 / %2 三角面 / %3 四面体").arg(nodes).arg(faces).arg(tets));
        }
    } else if (message.isEmpty()) {
        message = tr("网格划分失败（%1）。请检查几何、尺寸或体是否闭合。详见信息输出。").arg(exitCode);
    }
    m_jobDir.reset();
    m_jobPart = -1;
    m_progress->hide();
    updateParameters();
    updateActions();
    if (!message.isEmpty()) setNotice(message, true);
}

void MotionWindow::cancelMesh()
{
    if (m_jobPart != -1) {
        m_jobError = tr("已取消网格划分。");
        m_worker->kill();
    } else {
        m_meshForm->hide();
        updateParameters();
    }
}

void MotionWindow::saveMesh()
{
    const auto mesh = m_meshes.find(m_partId);
    if (mesh == m_meshes.end()) return;
    const auto path = withSuffix(QFileDialog::getSaveFileName(this, tr("导出网格"), {}, tr("VTK 非结构网格 (*.vtk)")), "vtk");
    if (path.isEmpty()) return;
    QString error;
    if (exportMesh(path, mesh->second, error))
        setNotice(tr("已导出网格：%1").arg(QFileInfo(path).fileName()));
    else setNotice(error, true);
}

} // namespace MotionOcc
