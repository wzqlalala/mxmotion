#include "MotionWindow.h"
#include "MotionWorkspace.h"
#include "MotionViewPages.h"
#include "compat/model/LegacyMotionContext.h"
#include "compat/picking/LegacyPickContext.h"
#include "MotionTreeIcons.h"
#include "OccViewport.h"
#include "RigidProject.h"

#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "mChineseTextEdit.h"
#include "mFoldSection.h"
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
#include <QClipboard>
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
#include <QFontDatabase>
#include <QFontInfo>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QSlider>
#include <QStatusBar>
#include <QSyntaxHighlighter>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTime>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QtConcurrentRun>
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
const auto projectFilter = QStringLiteral("MxMotion 项目 (*.mxmotion)");

constexpr int outputColorRole = QTextFormat::UserProperty;

class OutputHighlighter final : public QSyntaxHighlighter {
public:
    using QSyntaxHighlighter::QSyntaxHighlighter;

protected:
    void highlightBlock(const QString& text) override
    {
        const auto color = currentBlock().blockFormat().intProperty(outputColorRole);
        if (color == 0) return;
        const bool dark = UiPalette::theme() == UiPalette::Theme::Dark;
        const QColor error = dark ? QColor(255, 128, 128) : QColor(Qt::red);
        const QColor warning = dark ? QColor(123, 185, 255) : QColor(Qt::blue);
        setFormat(0, text.size(), color == 2 ? error : warning);
    }
};

class PartTransparencySlider final : public QSlider {
public:
    using QSlider::QSlider;
protected:
    void wheelEvent(QWheelEvent* event) override { event->ignore(); }
};

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

MotionWindow::MotionWindow(QWidget* parent) : MotionWindow(parent, nullptr, 1) {}

MotionWindow::MotionWindow(QWidget* parent, MotionWorkspace* workspace, int modelNumber)
    : QMainWindow(parent), m_modelNumber(modelNumber), m_workspace(workspace), m_projectActive(!workspace)
{
    if (m_workspace) setWindowFlags(Qt::Widget);
    setObjectName(QStringLiteral("mMechMainWindow"));
    setWindowTitle(QStringLiteral("MxSim.Motion V1.0"));
    setWindowIcon(icon(QStringLiteral("logo.png")));
    QSize initialSize(1766, 828);
    if (const auto* screen = QApplication::primaryScreen())
        initialSize = initialSize.boundedTo(screen->availableGeometry().size());
    resize(initialSize);
    setMinimumSize(870, 520);
    if (!m_workspace) MechMainWindow::FramelessWindowHelper::installFrameless(this);
    if (m_workspace) m_viewManager = m_workspace->m_viewManager.get();
    else {
        m_ownedViewManager = std::make_unique<MotionViewPages>(nullptr, this);
        m_viewManager = m_ownedViewManager.get();
    }
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
    if (!m_workspace) {
        auto* title = new MechMainWindow::mWindowTitleBar(this);
        title->setSeparatorVisible(false);
        layout->addWidget(title);
    }
    auto* ribbon = createMotionRibbon(*this, wrapper);
    layout->addWidget(ribbon);
    setMenuWidget(wrapper);
    ThemeManager::registerDockThemeApplier([] {
        for (auto* widget : QApplication::allWidgets())
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
    QTimer::singleShot(0, this, [this] {
        resetLayout(false);
        if (m_viewport) m_viewport->fitAll();
    });
}

MotionWindow::~MotionWindow()
{
    if (m_running) m_running->stop.store(true, std::memory_order_relaxed);
    if (m_solverThread.joinable()) m_solverThread.join();
    if (m_worker && m_worker->state() != QProcess::NotRunning) {
        m_worker->disconnect(this);
        m_worker->kill();
        m_worker->waitForFinished(2000);
    }
    // Original editors retain pointers into the typed model and callbacks
    // into the viewport. Release them while both services still exist.
    closeEditor(true);
    if(m_original) {
        m_original->disconnect(this);
        m_original.reset();
    }
    if (m_viewManager) m_viewManager->removeProject(*this);
}

bool MotionWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    return (!m_workspace && MechMainWindow::FramelessWindowHelper::handleNativeEvent(this, message, result))
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
            if (m_workspace) action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            addAction(action);
        }
        connect(action, &QAction::triggered, this, fn);
        return action;
    };
    const auto home = QStringLiteral("homepage/");
    const auto geo = QStringLiteral("geometry/");
    const auto view = QStringLiteral("CenterMainWindow/");
    add(Command::New, tr("新建"), home + "xinjian3x.png", [this] {
        if (m_workspace) m_workspace->newProject();
        else newModel();
    }, QKeySequence::New);
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
        setProjectModified(true);
        updateTree();
        syncViewport();
        selectPart(-1);
    });
    add(Command::Box, tr("立方体"), geo + "立方体_大@3x.png", [this] { addPrimitive(Primitive::Box); });
    add(Command::Cylinder, tr("圆柱体"), geo + "圆柱体_大@3x.png", [this] { addPrimitive(Primitive::Cylinder); });
    add(Command::Sphere, tr("球体"), geo + "球体_大@3x.png", [this] { addPrimitive(Primitive::Sphere); });
    add(Command::Fit, tr("自适应视图"), view + "zishiyingshitu.png", [this] { m_viewport->fitAll(); }, QKeySequence(Qt::Key_F));
    add(Command::Front, tr("前视图"), view + "qianshitu.png", [this] { m_viewport->viewFront(); });
    add(Command::Back, tr("后视图"), view + "houshitu.png", [this] { m_viewport->viewBack(); });
    add(Command::Top, tr("俯视图"), view + "xiashitu.png", [this] { m_viewport->viewTop(); });
    add(Command::Bottom, tr("仰视图"), view + "shangshitu.png", [this] { m_viewport->viewBottom(); });
    add(Command::Left, tr("左视图"), view + "zuoshitu.png", [this] { m_viewport->viewLeft(); });
    add(Command::Right, tr("右视图"), view + "youshitu.png", [this] { m_viewport->viewRight(); });
    add(Command::Isometric, tr("轴侧视图"), view + "zhouceshitu.png", [this] { m_viewport->viewIsometric(); });

    auto* navigation = new QActionGroup(this);
    auto navigationMode = [&](Command id, const QString& label, OccViewport::MouseNavigationMode value,
                              const QString& hint, const char* objectName) {
        auto* action = add(id, label, view + "zhouceshitu.png", [this, value] {
            m_viewport->setMouseNavigationMode(value);
            setStatusNotice(m_viewport->mouseNavigationHint());
        });
        action->setObjectName(QString::fromLatin1(objectName));
        action->setCheckable(true);
        action->setToolTip(hint);
        navigation->addAction(action);
    };
    navigationMode(Command::NavigationClassic, tr("经典导航"), OccViewport::MouseNavigationMode::Classic,
        tr("左键拖动旋转；中键或右键拖动平移；滚轮按光标缩放；左键点选。"), "mouseNavigationClassic");
    navigationMode(Command::NavigationNx, tr("NX 导航"), OccViewport::MouseNavigationMode::Nx,
        tr("中键旋转；Shift+中键平移；Ctrl+中键拖动或滚轮缩放；左键点选。双击表面设旋转中心，双击空白适合全部。"), "mouseNavigationNx");
    command(Command::NavigationClassic)->setChecked(true);

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
    add(Command::ClearSelection, tr("清除选择"), "display/yincangquanbu.png", [this] {
        if (m_viewport->isPicking()) { m_viewport->endPick(); emit m_viewport->pickCancelled(); }
        else selectPart(-1);
    }, QKeySequence(Qt::Key_Escape));
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
        updateNativeFlexMeshForPart(m_partId);
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
    add(Command::Exit, tr("退出"), "close3x.png", [this] { window()->close(); }, QKeySequence::Quit);
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

    auto* viewDock = addPane("ViewTree", tr("视图"), createViewPageTree());
    m_modelDock = addPane("Tree", tr("模型"), createModelBrowser());
    m_parameterDock = addPane("Info", tr("参数"), createParameterPage());
    // Same Info-dock sizing rule as the uploaded mDockLayoutManager. A 225 px
    // host fitted the simplified editor but clips the original compound rows.
    const int parameterWidth = 12 * UiMetrics::baseH()
        + 8 * qMax(0, 6 - UiMetrics::baseG()) + 8;
    m_parameterDock->setMinimumSize(parameterWidth - 40, 150);
    m_modelDock->setMinimumWidth(220);
    auto* parameterArea = m_docks->addDockWidget(ads::LeftDockWidgetArea, m_parameterDock, center->dockAreaWidget());
    m_treeArea = m_docks->addDockWidget(ads::LeftDockWidgetArea, viewDock, parameterArea);
    m_docks->addDockWidgetTabToArea(m_modelDock, m_treeArea);
    m_modelDock->toggleView(true);
    m_parameterDock->toggleView(true);
    m_treeArea->setCurrentDockWidget(m_modelDock);

    auto* selectionPage = new QWidget;
    auto* selectionLayout = new QVBoxLayout(selectionPage);
    auto* selectionScroll = new QScrollArea(selectionPage);
    selectionScroll->setWidgetResizable(true);
    selectionScroll->setFrameShape(QFrame::NoFrame);
    selectionScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    selectionLayout->addWidget(selectionScroll);
    m_selectionDetails = new QTableWidget(1, 1, selectionScroll);
    m_selectionDetails->setObjectName(QStringLiteral("selectionInfoTable"));
    m_selectionDetails->horizontalHeader()->hide();
    m_selectionDetails->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_selectionDetails->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_selectionDetails->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_selectionDetails, &QTableWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        mUiMenu menu(m_selectionDetails);
        auto* copy = menu.addAction(tr("复制"));
        auto* refresh = menu.addAction(tr("刷新"));
        menu.addAction(tr("重新选择"));
        const auto* action = menu.exec(m_selectionDetails->viewport()->mapToGlobal(pos));
        if (action == refresh) m_selectionDetails->clearSelection();
        const auto items = m_selectionDetails->selectedItems();
        if (action != copy || items.isEmpty()) return;
        const int row = items.front()->row();
        QStringList values;
        for (int column = 0; column < m_selectionDetails->columnCount(); ++column) {
            const auto* item = m_selectionDetails->item(row, column);
            if (item && item->isSelected()) values.append(item->text());
        }
        QApplication::clipboard()->setText(values.join(QLatin1Char(' ')));
    });
    selectionScroll->setWidget(m_selectionDetails);
    m_selectionDock = addPane("Select", tr("选择信息"), selectionPage);
    auto* bottom = m_docks->addDockWidget(ads::BottomDockWidgetArea, m_selectionDock);
    auto* outputPage = new QWidget;
    auto* outputLayout = new QVBoxLayout(outputPage);
    outputLayout->setContentsMargins(10, 10, 10, 10);
    m_output = new MechMainWindow::mChineseTextEdit(outputPage);
    m_output->setObjectName(QStringLiteral("motionOutput"));
    m_output->setReadOnly(true);
    m_output->setAcceptRichText(false);
    m_output->document()->setMaximumBlockCount(200);
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
#ifdef Q_OS_WIN
    font.setFamily(QStringLiteral("NSimSun"));
    if (!QFontInfo(font).exactMatch()) font.setFamily(QStringLiteral("SimSun"));
#endif
    font.setStyleHint(QFont::TypeWriter);
    font.setPixelSize(14);
    m_output->setFont(font);
    m_output->setProperty("outputFontFamily", font.family());
    m_output->setTabStopDistance(QFontMetrics(font).horizontalAdvance(QLatin1Char(' ')) * 8);
    new OutputHighlighter(m_output->document());
    outputLayout->addWidget(m_output);
    m_outputDock = addPane("Output", tr("信息输出"), outputPage);
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
    if (key == QStringLiteral("Curve") || key == QStringLiteral("Select") || key == QStringLiteral("Output")) {
        // 内容已有自己的滚动和布局，直接随停靠窗缩放。
        dock->setWidget(content, ads::CDockWidget::ForceNoScrollArea);
        if (key == QStringLiteral("Curve")) dock->setMinimumSize(300, 240);
    } else {
        dock->setWidget(content);
    }
    return dock;
}

void MotionWindow::resetLayout(bool resetVisibility)
{
    if (resetVisibility) m_docks->restoreState(m_defaultLayout);
    if (auto* treeArea = m_modelDock->dockAreaWidget()) {
        if (resetVisibility) treeArea->setCurrentDockWidget(m_modelDock);
        const auto sizes = m_docks->splitterSizes(treeArea);
        if (sizes.size() == 3) {
            const int suggested = 12 * UiMetrics::baseH()
                + 8 * qMax(0, 6 - UiMetrics::baseG()) + 8;
            const int parameterWidth = qMax(suggested, m_parameterDock->minimumWidth());
            m_docks->setSplitterSizes(treeArea, { 280, parameterWidth, qMax(400, width() - 280 - parameterWidth) });
        }
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
        "QWidget[mxSurface=\"true\"], QToolBar#viewToolBar, QToolBar#viewDisplayBar { background:%6; }"
        "QToolBar#viewToolBar, QToolBar#viewDisplayBar { border:none; spacing:1px; }"
        "QToolButton#viewportPickMode { min-width:90px; }"
        "ads--CDockSplitter::handle { background:#a8b3c2; }"
        "ads--CDockSplitter::handle:horizontal { width:1px; }"
        "ads--CDockSplitter::handle:vertical { height:1px; }")
        .arg(UiPalette::headerHex(), UiPalette::borderHex()).arg(UiMetrics::windowHeaderHeight())
        .arg(UiPalette::textHex()).arg(UiMetrics::bodyFontSize())
        .arg(UiPalette::surfaceHex(), UiPalette::hoverHex());
    m_docks->setStyleSheet(css);
    m_output->setStyleSheet(QStringLiteral(
        "QTextEdit#motionOutput { background:%1; color:%2; border:1px solid %3; font-family:'%4'; font-size:14px; }")
        .arg(UiPalette::surfaceHex(), UiPalette::theme() == UiPalette::Theme::Light
            ? QStringLiteral("#000000") : UiPalette::textHex(), UiPalette::borderHex(),
            m_output->property("outputFontFamily").toString()));
    m_output->document()->findChild<QSyntaxHighlighter*>()->rehighlight();
    statusBar()->setStyleSheet(QStringLiteral(
        "QStatusBar { background:%1; color:%2; border-top:1px solid %3; }"
        "QStatusBar::item { border:none; }"
        "QLabel#selectionStatus { background:transparent; color:%2; border:none; padding:0 5px; }"
        "QLabel#selectionStatus:hover { background:%4; }")
        .arg(UiPalette::surfaceHex(), UiPalette::textHex(), UiPalette::borderHex(), UiPalette::hoverHex()));
    updateSelectionInfo();
}

QWidget* MotionWindow::createViewportFrame()
{
    auto* frame = new QWidget;
    frame->setProperty("mxSurface", true);
    frame->setAttribute(Qt::WA_StyledBackground);
    auto* layout = new QVBoxLayout(frame);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* toolRow = new QHBoxLayout;
    toolRow->setContentsMargins(0, 0, 0, 0);
    toolRow->setSpacing(0);
    auto* viewTools = new QToolBar(tr("视图工具栏"), frame);
    viewTools->setObjectName(QStringLiteral("viewToolBar"));
    viewTools->setIconSize(QSize(20, 20));
    viewTools->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    viewTools->addAction(command(Command::Fit));
    const auto addViewTool = [this, viewTools](const QString& image, const QString& text, auto operation) {
        connect(viewTools->addAction(icon(QStringLiteral("CenterMainWindow/") + image), text),
            &QAction::triggered, this, operation);
    };
    addViewTool("suoxiaoyibei.png", tr("缩小一倍"), [this] { m_viewport->zoomView(false, 2); });
    addViewTool("fangdayibei.png", tr("放大一倍"), [this] { m_viewport->zoomView(true, 2); });
    addViewTool("nishizhenxuanzhuan.png", tr("逆时针旋转 30°"), [this] { m_viewport->rotateView("Sz", 30); });
    addViewTool("shunshizhenxuanzhuan.png", tr("顺时针旋转 30°"), [this] { m_viewport->rotateView("Sz", -30); });
    viewTools->addSeparator();
    for (const auto id : { Command::Front, Command::Back, Command::Bottom, Command::Top, Command::Left, Command::Right, Command::Isometric })
        viewTools->addAction(command(id));
    viewTools->addSeparator();
    for (const auto id : { Command::ShadedEdges, Command::Wireframe, Command::Shaded, Command::MeshDisplay })
        viewTools->addAction(command(id));
    viewTools->addSeparator();
    auto* pickButton = new QToolButton(frame);
    pickButton->setObjectName(QStringLiteral("viewportPickMode"));
    pickButton->setPopupMode(QToolButton::InstantPopup);
    pickButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    pickButton->setToolTip(tr("切换视口拾取模式"));
    auto* pickMenu = new mUiMenu(pickButton);
    ThemeManager::applyMenuStyle(pickMenu);
    pickButton->setMenu(pickMenu);
    auto* pickModes = new QActionGroup(pickButton);
    using PickType = MxFunctions::PickDataType;
    const auto viewportFilter = [](PickType type) -> std::optional<PickKind> {
        if (type == PickType::None) return std::nullopt;
        return LegacyPickContext::toKind(type);
    };
    const auto showMode = [pickButton](const QAction* action) {
        pickButton->setIcon(action->icon());
        pickButton->setText(action->text());
    };
    struct PickMode { PickType type; const char* image; QString label; const char* name; };
    const PickMode modes[] = {
        {PickType::None, "zhinengxuanze3x.png", tr("智能选择"), "pickSmart"},
        {PickType::GeoPoint, "shiqudingdian3x.png", tr("拾取顶点"), "pickVertex"},
        {PickType::GeoEdge, "shiqubian3x.png", tr("拾取边"), "pickEdge"},
        {PickType::GeoFace, "shiqumian3x.png", tr("拾取面"), "pickFace"},
        {PickType::GeoSolid, "shiquti3x.png", tr("拾取体"), "pickSolid"},
        {PickType::Node, "shiqujiedian3x.png", tr("拾取节点"), "pickNode"},
        {PickType::ElementEdge, "shiqudanyuanxian3x.png", tr("拾取单元线"), "pickMeshEdge"},
        {PickType::ElementFace, "shiqudanyuanmian3x.png", tr("拾取单元面"), "pickMeshFace"},
        {PickType::Element, "shiqudanyuan3x.png", tr("拾取单元"), "pickElement"},
        {PickType::Part, "shiqubujian3x.png", tr("拾取部件"), "pickPart"}
    };
    for (const auto& mode : modes) {
        auto* action = pickMenu->addAction(icon(QStringLiteral("CenterMainWindow/") + mode.image), mode.label);
        action->setObjectName(QString::fromLatin1(mode.name));
        action->setCheckable(true);
        action->setData(int(mode.type));
        pickModes->addAction(action);
        viewTools->addAction(action);
        connect(action, &QAction::triggered, this, [this, type = mode.type, action, viewportFilter, showMode] {
            if (auto* field = LegacyPickContext::activeForViewport(*m_viewport)) {
                if (type != field->pickType()) field->setPickType(type);
            } else if (!m_viewport->isPicking()) {
                m_viewport->setSelectionFilter(viewportFilter(type));
                showMode(action);
            }
        });
        if (mode.type == PickType::Part) {
            action->setChecked(true);
            showMode(action);
        }
    }
    viewTools->addAction(command(Command::ClearSelection));
    pickMenu->addSeparator();
    pickMenu->addAction(command(Command::ClearSelection));
    toolRow->addWidget(viewTools, 1);
    toolRow->addWidget(pickButton);

    auto* viewDisplay = new QToolBar(tr("视图显示栏"), frame);
    viewDisplay->setObjectName(QStringLiteral("viewDisplayBar"));
    viewDisplay->setIconSize(QSize(20, 20));
    toolRow->addWidget(viewDisplay);
    layout->addLayout(toolRow);

    m_refreshViewportPickControls = [this, pickModes, pickButton, viewportFilter, showMode](bool active) {
        const auto* field = LegacyPickContext::activeForViewport(*m_viewport);
        const auto allowed = field ? field->allowedTypes() : QSet<PickType>{};
        for (auto* action : pickModes->actions()) {
            const auto type = static_cast<PickType>(action->data().toInt());
            action->setEnabled(field ? allowed.contains(type) : !active);
            if (field ? type == field->pickType() : viewportFilter(type) == m_viewport->selectionFilter()) {
                action->setChecked(true);
                showMode(action);
            }
        }
        pickButton->setToolTip(field ? tr("切换当前参数支持的拾取模式") : active ? tr("当前拾取使用固定类型") : tr("切换视口拾取模式"));
        updateSelectionInfo();
    };
    layout->addWidget(createViewPages(viewDisplay, frame), 1);
    return frame;
}

QWidget* MotionWindow::createModelBrowser()
{
    auto* panel = new QWidget;
    auto* outer = new QVBoxLayout(panel);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    auto* bar = new QWidget(panel);
    bar->setProperty("mxSurface", true);
    bar->setAttribute(Qt::WA_StyledBackground);
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
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
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
        const bool jointGroup = node.kind == ModelNode::None &&
            (node.collection == "joints" || node.collection == "drives" || node.collection == "couplings");
        if (node.kind == ModelNode::Part || node.kind == ModelNode::None)
            selectPart(node.id, jointGroup ? node.collection : QString{});
        else selectRigidNode(node);
    });
    connect(m_tree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem* item, int column) {
        const auto node = treeNode(item);
        if (m_syncingTree || column != 0) return;
        if (renameOriginalDynamicTreeItem(item)) return;
        if (node.kind != ModelNode::Part) return;
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
        else if (node.kind == ModelNode::Native && item->data(0,Qt::UserRole+4).toBool())
            editNativeEntity(node.collection,node.id,{});
        else if (node.kind == ModelNode::Scenario && item->data(0,Qt::UserRole+4).toBool())
            editOriginalMotionEntity("scenarios",node.id,{});
    });
    connect(m_tree, &QTreeWidget::customContextMenuRequested, this, &MotionWindow::showPartMenu);
    return panel;
}

QWidget* MotionWindow::createParameterPage()
{
    auto* panel = new QWidget;
    panel->setProperty("mxSurface", true);
    panel->setAttribute(Qt::WA_StyledBackground);
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

    auto* partSettings = new MechMainWindow::FoldSection(tr("设置"), panel);
    m_partSettings = partSettings;
    m_partTransparencySlider = new PartTransparencySlider(Qt::Horizontal, partSettings);
    m_partTransparencySlider->setObjectName(QStringLiteral("partTransparencySlider"));
    m_partTransparencySlider->setRange(0, 100);
    m_partTransparencySlider->setTracking(false);
    m_partTransparencySpin = new QDoubleSpinBox(partSettings);
    m_partTransparencySpin->setObjectName(QStringLiteral("partTransparencySpin"));
    m_partTransparencySpin->setRange(0.0, 1.0);
    m_partTransparencySpin->setDecimals(2);
    m_partTransparencySpin->setSingleStep(0.01);
    m_partTransparencySpin->setKeyboardTracking(false);
    m_partTransparencySpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    partSettings->addRow(new PanelLabel(tr("透明度"), partSettings), m_partTransparencySlider, m_partTransparencySpin);
    const auto applyTransparency = [this](int percent) {
        const auto* part = m_document.part(m_partId);
        if (!part || part->transparencyPercent == percent || !m_document.setPartTransparency(m_partId, percent)) return;
        const QSignalBlocker slider(m_partTransparencySlider), spin(m_partTransparencySpin);
        m_partTransparencySlider->setValue(percent);
        m_partTransparencySpin->setValue(percent / 100.0);
        m_viewport->setPartTransparency(m_partId, percent);
        setProjectModified(true);
    };
    connect(m_partTransparencySlider, &QSlider::sliderMoved, this, [this](int percent) {
        const QSignalBlocker spin(m_partTransparencySpin);
        m_partTransparencySpin->setValue(percent / 100.0);
    });
    connect(m_partTransparencySlider, &QSlider::valueChanged, this, applyTransparency);
    connect(m_partTransparencySpin, qOverload<double>(&QDoubleSpinBox::valueChanged), this,
        [applyTransparency](double value) { applyTransparency(qRound(value * 100)); });
    m_partSettings->hide();
    layout->addWidget(m_partSettings);

    m_meshForm = new QWidget(panel);
    auto* form = new QFormLayout(m_meshForm);
    form->setContentsMargins(0, 3, 0, 0);
    m_meshKind = new QComboBox(m_meshForm);
    m_meshKind->addItems({ tr("四面体体网格"), tr("三角形面网格") });
    m_meshSize = dimension(m_meshForm, 5.0);
    m_meshSize->setObjectName(QStringLiteral("meshElementSize"));
    m_meshSize->setRange(0.0001, 100000.0);
    form->addRow(tr("划分类型"), m_meshKind);
    form->addRow(tr("单元尺寸"), m_meshSize);
    auto* buttons = new QWidget(m_meshForm);
    auto* row = new QHBoxLayout(buttons);
    row->setContentsMargins(0, 2, 0, 0);
    m_meshGenerateButton = new QPushButton(tr("生成"), buttons);
    m_meshGenerateButton->setObjectName(QStringLiteral("generateMeshButton"));
    m_meshCancelButton = new QPushButton(tr("取消"), buttons);
    m_meshCancelButton->setObjectName(QStringLiteral("cancelMeshButton"));
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
    m_notice->setObjectName(QStringLiteral("motionStatusNotice"));
    m_notice->setTextFormat(Qt::PlainText);
    m_notice->setMargin(3);
    m_selection = new QLabel(bar);
    m_selection->setObjectName(QStringLiteral("selectionStatus"));
    m_selection->setTextFormat(Qt::RichText);
    m_selection->setTextInteractionFlags(Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
    m_selection->setToolTip(tr("打开选择信息"));
    m_selection->setCursor(Qt::PointingHandCursor);
    connect(m_selection, &QLabel::linkActivated, this, [this] {
        m_selectionDock->toggleView(true);
        if (auto* area = m_selectionDock->dockAreaWidget()) area->setCurrentDockWidget(m_selectionDock);
    });
    auto* unit = new QLabel(tr("mm"), bar);
    unit->setMinimumWidth(58);
    unit->setAlignment(Qt::AlignCenter);
    m_progress = new QProgressBar(bar);
    m_progress->setRange(0, 0);
    m_progress->setFixedSize(132, 14);
    m_progress->hide();
    m_cancelImportButton = new QPushButton(tr("取消导入"), bar);
    m_cancelImportButton->setObjectName(QStringLiteral("cancelGeometryImportButton"));
    m_cancelImportButton->setFixedHeight(18);
    m_cancelImportButton->hide();
    connect(m_cancelImportButton, &QPushButton::clicked, this, &MotionWindow::cancelGeometryImport);
    bar->addWidget(m_notice, 1);
    bar->addPermanentWidget(m_progress);
    bar->addPermanentWidget(m_cancelImportButton);
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
    root->setIcon(0, TreeIcons::item("root"));
    root->setData(0,Qt::UserRole,-1);
    root->setData(0,Qt::UserRole+2,QStringLiteral("__motionRoot"));
    root->setData(0,Qt::UserRole+3,true);
    const std::vector<std::pair<QString, QString>> groups = {
        {"parts", tr("部件")}, {"bodies", tr("运动体")}, {"autoFlexBodies", tr("柔性体")},
        {"joints", tr("运动副")}, {"couplings", tr("耦合副")}, {"drives", tr("驱动")},
        {"bodyDrives", tr("运动体驱动")}, {"hydraulicCylinders", tr("液压缸")},
        {"hydraulicMotors", tr("液压马达")}, {"hydraulicNodes", tr("液压节点")},
        {"hydraulicComponents", tr("液压元件")},
        {"springs", tr("弹簧")}, {"forceElements", tr("力元")}, {"sensors", tr("传感器")},
        {"controllers", tr("控制器")}, {"markers", tr("标记点")}, {"contacts", tr("接触")},
        {"controlInputs", tr("控制输入")}
    };
    std::map<QString, QTreeWidgetItem*> groupItems;
    for (const auto& entry : groups) {
        if ((entry.first == "bodyDrives" || entry.first == "hydraulicCylinders"
             || entry.first == "hydraulicMotors" || entry.first == "hydraulicNodes"
             || entry.first == "hydraulicComponents")
            && (!m_model.nativeDocument || m_model.nativeDocument->collection(entry.first).isEmpty())) continue;
        auto* group = new QTreeWidgetItem(root, { entry.second });
        groupItems.emplace(entry.second, group);
        group->setIcon(0, TreeIcons::item(entry.first));
        if (entry.first == "parts") m_partsItem = group;
        else {
            group->setData(0,Qt::UserRole,-1);
            group->setData(0,Qt::UserRole+2,entry.first);
            group->setData(0,Qt::UserRole+3,true);
            if (m_selectionNode.kind == ModelNode::None && m_selectionNode.collection == entry.first)
                m_tree->setCurrentItem(group);
        }
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
        item->setIcon(0, TreeIcons::item("parts"));
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

QString MotionWindow::partSummary() const
{
    const auto* part = m_document.part(m_partId);
    QString summary = part ? tr("部件编号：%1\n几何状态：%2").arg(part->id).arg(m_hidden.count(m_partId) ? tr("隐藏") : tr("显示"))
                           : tr("尚未选择部件。");
    if (part && !part->metreDisplaySource.IsNull())
        summary += tr("\n毫米拓扑未通过校验，按米制几何显示。");
    const auto mesh = m_meshes.find(m_partId);
    if (mesh != m_meshes.end()) {
        summary += tr("\n节点：%1\n三角面：%2\n四面体：%3")
            .arg(qulonglong(mesh->second.nodes.size())).arg(qulonglong(mesh->second.surface.size()))
            .arg(qulonglong(mesh->second.tetrahedra.size()));
    } else if (part) {
        summary += tr("\n有限元网格：无");
    }
    return summary;
}

void MotionWindow::updateParameters()
{
    updateSelectionInfo();
    if (m_editor) return;
    const auto* part = m_document.part(m_partId);
    m_parameterTitle->setText(!m_meshForm->isHidden() ? tr("生成网格") : part ? part->name : tr("参数设置"));
    m_parameterDetails->setText(partSummary());
    m_partSettings->setVisible(part && m_meshForm->isHidden());
    if (part) {
        const QSignalBlocker slider(m_partTransparencySlider), spin(m_partTransparencySpin);
        m_partTransparencySlider->setValue(part->transparencyPercent);
        m_partTransparencySpin->setValue(part->transparencyPercent / 100.0);
    }
}

void MotionWindow::updateSelectionInfo()
{
    if (!m_viewport || !m_selection || !m_selectionDetails) return;
    ++m_selectionInfoRevision;
    m_pendingPartSelection.clear();
    const bool picking = m_viewport->isPicking();
    auto picks = picking ? m_viewport->pickSelection() : m_viewport->viewportSelection();
    if (picks.isEmpty() && !picking && m_document.part(m_partId)) {
        PickResult pick;
        pick.partId = m_partId;
        picks.append(pick);
    }
    if (!picks.isEmpty() && std::all_of(picks.cbegin(), picks.cend(), [](const PickResult& pick) {
        return pick.kind == PickKind::Part;
    })) {
        // 先更新选中状态。体积和质心用当前部件的精确几何在后台计算。
        m_pendingPartSelection.reserve(picks.size());
        for (const auto& pick : std::as_const(picks)) {
            SelectedPartGeometry selected;
            selected.position = pick.position;
            if (const auto* part = m_document.part(pick.partId)) {
                selected.name = part->name;
                selected.shape = part->metreDisplaySource.IsNull() ? part->shape : part->metreDisplaySource;
                selected.millimetresPerUnit = part->metreDisplaySource.IsNull() ? 1.0 : 1000.0;
                const auto mesh = m_meshes.find(pick.partId);
                selected.meshCounts = mesh == m_meshes.end() ? QStringList{QStringLiteral("0"), QStringLiteral("0")}
                    : QStringList{QString::number(mesh->second.nodes.size()),
                        QString::number(mesh->second.tetrahedra.empty() ? mesh->second.surface.size() : mesh->second.tetrahedra.size())};
            }
            m_pendingPartSelection.append(std::move(selected));
        }
        auto status = tr("%1 部件").arg(picks.size());
        if (picks.size() == 1 && !m_pendingPartSelection.front().name.isEmpty())
            status += tr(" · %1").arg(m_pendingPartSelection.front().name);
        showSelectionInfo({status, {}, {}});
        startPartSelectionMeasurement();
        return;
    }
    showSelectionInfo(describeSelection(m_document, m_meshes, picks));
}

void MotionWindow::startPartSelectionMeasurement()
{
    if (m_partSelectionWorker || m_pendingPartSelection.isEmpty()) return;
    QVector<SelectedPartGeometry> request;
    request.swap(m_pendingPartSelection);
    const auto revision = m_selectionInfoRevision;
    auto* watcher = new QFutureWatcher<SelectionInfo>(this);
    m_partSelectionWorker = watcher;
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, revision] {
        m_partSelectionWorker = nullptr;
        watcher->deleteLater();
        // 快速连续选中时仅计算最新的候选，旧结果不能覆盖当前部件。
        if (revision == m_selectionInfoRevision) showSelectionInfo(watcher->result());
        startPartSelectionMeasurement();
    });
    watcher->setFuture(QtConcurrent::run([request = std::move(request)] {
        return describePartSelection(request);
    }));
}

void MotionWindow::showSelectionInfo(const SelectionInfo& info)
{
    const auto status = tr("选择：%1").arg(info.status);
    const auto label = m_selection->fontMetrics().elidedText(status, Qt::ElideRight, qMax(120, width() / 4));
    m_selection->setText(QStringLiteral("<a href='selection' style='color:%2;text-decoration:none'>%1</a>")
        .arg(label.toHtmlEscaped(), UiPalette::textHex()));
    m_selection->setToolTip(tr("打开选择信息\n%1").arg(status));

    m_selectionDetails->setUpdatesEnabled(false);
    m_selectionDetails->setRowCount(0);
    m_selectionDetails->setColumnCount(std::max(1, info.columns.size()));
    m_selectionDetails->setRowCount(info.rows.size() + 1);
    for (int column = 0; column < info.columns.size(); ++column) {
        auto* header = new QLabel(info.columns.at(column));
        header->setAlignment(Qt::AlignCenter);
        m_selectionDetails->setCellWidget(0, column, header);
    }
    for (int row = 0; row < info.rows.size(); ++row) {
        const auto& values = info.rows.at(row);
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values.at(column));
            item->setTextAlignment(Qt::AlignCenter);
            m_selectionDetails->setItem(row + 1, column, item);
        }
    }
    m_selectionDetails->resizeColumnsToContents();
    m_selectionDetails->setUpdatesEnabled(true);
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
    for (const auto id : { Command::New, Command::Open }) command(id)->setEnabled(m_workspace || !busy);
    for (const auto id : { Command::Import, Command::Box, Command::Cylinder, Command::Sphere })
        command(id)->setEnabled(!busy);
    updateRigidActions();
}

void MotionWindow::syncViewport(bool fit, const std::shared_ptr<const PreparedCadScene>& prepared,
                               const std::vector<int>& preparedPartIds)
{
    m_viewport->setScene(m_document, m_meshes, m_hidden, prepared, preparedPartIds);
    // 场景数据刷新时才同步拾取拓扑和有限元节点编号。
    if (m_original) m_original->refreshMeshes();
    refreshMotionSymbols();
    m_viewport->selectPart(m_partId);
    if (fit) m_viewport->fitAll();
}

void MotionWindow::selectPart(int id, const QString& selectedGroup)
{
    m_viewport->clearViewportSelection();
    const int previous = m_partId;
    m_partId = m_document.part(id) ? id : -1;
    m_selectionNode = m_partId > 0 ? ModelNode{ModelNode::Part, m_partId}
        : ModelNode{ModelNode::None, -1, selectedGroup};
    leaveRigidAnimation();
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
    else if (selectedGroup.isEmpty() && current && current->data(0, partRole).isValid()) m_tree->setCurrentItem(nullptr);
    m_syncingTree = false;
    updateParameters();
    updateActions();
}

void MotionWindow::showPartMenu(const QPoint& position)
{
    auto* item = m_tree->itemAt(position);
    if (!item) return;
    if (showOriginalDynamicTreeMenu(item,m_tree->viewport()->mapToGlobal(position))) return;
    const auto node = treeNode(item);
    if (node.kind != ModelNode::None && node.kind != ModelNode::Part) {
        showRigidMenu(node, m_tree->viewport()->mapToGlobal(position));
        return;
    }
    mUiMenu menu(this);
    QAction* removeModel = nullptr;
    if (!item->parent() && m_workspace) {
        removeModel = menu.addAction(icon(QStringLiteral("BrowserTree/shanchu.png")), tr("删除模型"));
    } else if (item == m_partsItem) {
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
    if (auto* selected = menu.exec(m_tree->viewport()->mapToGlobal(position)); selected && selected == removeModel)
        m_workspace->closeProject(*this);
}

void MotionWindow::setNotice(const QString& message, bool error)
{
    const auto level = error ? NoticeLevel::Error : NoticeLevel::Normal;
    setStatusNotice(message, level);
    appendOutput(message, level);
}

void MotionWindow::setStatusNotice(const QString& message, NoticeLevel level)
{
    m_notice->setText(message.section(QLatin1Char('\n'), 0, 0));
    m_notice->setToolTip(message);
    m_notice->setStyleSheet(level == NoticeLevel::Error
        ? QStringLiteral("color:%1").arg(UiPalette::errorHex()) : QString());
}

void MotionWindow::appendOutput(const QString& message, NoticeLevel level, bool outputTime)
{
    if (message.isEmpty()) return;
    const auto text = outputTime
        ? QStringLiteral(">>  %1  [  %2  ]").arg(message, QTime::currentTime().toString(QStringLiteral("hh:mm:ss")))
        : message;
    auto* scrollbar = m_output->verticalScrollBar();
    const bool follow = scrollbar->value() == scrollbar->maximum();
    QTextCursor cursor(m_output->document());
    cursor.movePosition(QTextCursor::End);
    QTextBlockFormat block;
    block.setProperty(outputColorRole, static_cast<int>(level));
    cursor.beginEditBlock();
    if (m_output->document()->isEmpty()) cursor.setBlockFormat(block);
    else cursor.insertBlock(block, QTextCharFormat());
    cursor.insertText(text, QTextCharFormat());
    cursor.endEditBlock();
    if (follow && !m_output->textCursor().hasSelection()) scrollbar->setValue(scrollbar->maximum());
}

bool MotionWindow::importCad(const QString& path)
{
    if (motionBusy() || m_jobPart >= 0) return false;
    std::vector<int> ids;
    QString error;
    if (!m_document.importFile(path, ids, error)) {
        setNotice(error, true);
        return false;
    }
    showImportedCad(path, ids, false);
    return true;
}

bool MotionWindow::runGeometryImport(const QString& path, std::function<void()> prepare,
                                     std::function<void(const QString&)> finish)
{
    if (motionBusy() || m_jobPart >= 0) {
        setNotice(tr("当前项目有任务正在运行，请等待完成后再导入。"), true);
        return false;
    }
    closeEditor();
    const quint64 token = ++m_importGeneration;
    m_importingGeometry = true;
    m_progress->setRange(0, 0);
    m_progress->show();
    if (m_cancelImportButton) m_cancelImportButton->show();
    setNotice(tr("正在后台导入 %1…").arg(QFileInfo(path).fileName()));
    updateActions();
    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcherBase::finished, this, [this, watcher, token, finish = std::move(finish)] {
        if (token != m_importGeneration) {
            watcher->deleteLater();
            return;
        }
        const auto error = watcher->result();
        watcher->deleteLater();
        m_importingGeometry = false;
        m_progress->hide();
        if (m_cancelImportButton) m_cancelImportButton->hide();
        updateActions();
        finish(error);
    });
    watcher->setFuture(QtConcurrent::run([prepare = std::move(prepare)]() -> QString {
        try {
            prepare();
            return {};
        } catch (const std::exception& failure) {
            return QObject::tr("后台导入失败：%1").arg(QString::fromUtf8(failure.what()));
        } catch (...) {
            return QObject::tr("后台导入失败。");
        }
    }));
    return true;
}

void MotionWindow::cancelGeometryImport()
{
    if (!m_importingGeometry) return;
    ++m_importGeneration;
    m_importingGeometry = false;
    m_progress->hide();
    if (m_cancelImportButton) m_cancelImportButton->hide();
    updateActions();
    setNotice(tr("已取消后台导入，当前工程保留。"));
    emit geometryImportFinished(false);
}

bool MotionWindow::startCadImport(const QString& path, bool replace, std::function<void(bool)> finished)
{
    if (replace && !confirmReplace()) return false;
    struct Import {
        CadDocument cad;
        std::vector<int> ids;
        QString error;
        std::shared_ptr<const PreparedCadScene> scene;
    };
    auto result = std::make_shared<Import>();
    return runGeometryImport(path, [path, result] {
        if (result->cad.importFile(path, result->ids, result->error))
            result->scene = OccViewport::prepareCadScene(result->cad);
    }, [this, path, replace, finished = std::move(finished), result](const QString& failure) {
        auto error = failure.isEmpty() ? result->error : failure;
        bool success = error.isEmpty();
        if (success && replace) replaceWithCad(std::move(result->cad));
        else if (success) success = m_document.appendImported(std::move(result->cad), path, result->ids, error);
        if (success) showImportedCad(path, result->ids, replace, result->scene);
        else setNotice(error, true);
        if (finished) finished(success);
        emit geometryImportFinished(success);
    });
}

void MotionWindow::showImportedCad(const QString& path, const std::vector<int>& ids, bool replace,
                                   const std::shared_ptr<const PreparedCadScene>& prepared)
{
    setProjectModified(!replace);
    m_partId = ids.front();
    updateTree();
    syncViewport(true, prepared, ids);
    selectPart(m_partId);
    for (const auto& note : m_document.importNotes()) appendOutput(note);
    setNotice((replace ? tr("已打开 %1 · %2 个部件") : tr("已导入 %1 · %2 个部件"))
        .arg(QFileInfo(path).fileName()).arg(ids.size())
        + (m_document.importNotes().isEmpty() ? QString{} : tr(" · %1 条导入说明").arg(m_document.importNotes().size())));
}

void MotionWindow::openCadDialog(bool addToModel)
{
    const auto path = QFileDialog::getOpenFileName(this, addToModel ? tr("导入几何模型") : tr("打开项目或 CAD 几何"), {},
        addToModel ? cadFilters : projectFilter + QStringLiteral(";;运动数据 (*.json);;") + cadFilters);
    if (path.isEmpty()) return;
    if (addToModel) {
        startCadImport(path);
        return;
    }
    if (m_workspace) {
        m_workspace->openProject(path);
        return;
    }
    openWorkspaceFile(path);
}

bool MotionWindow::openWorkspaceFile(const QString& path)
{
    const auto suffix = QFileInfo(path).suffix();
    if (suffix.compare(QStringLiteral("mxmotion"), Qt::CaseInsensitive) == 0) return openRigidProject(path);
    if (suffix.compare(QStringLiteral("json"), Qt::CaseInsensitive) == 0) return startMotionJsonImport(path);
    return startCadImport(path, true);
}

void MotionWindow::replaceWithCad(CadDocument&& next)
{
    closeEditor();
    clearResults();
    m_original.reset();
    m_model = {};
    m_selectionNode = {};
    m_document = std::move(next);
    m_meshes.clear();
    m_hidden.clear();
    m_meshForm->hide();
    command(Command::ShadedEdges)->trigger();
    m_projectPath.clear();
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
    if(m_original)m_original->flush();
    if (!m_modified) return true;
    return QMessageBox::question(this, tr("未保存的项目"), tr("放弃当前未保存的项目更改？"),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Discard;
}

bool MotionWindow::confirmProjectClose(bool deletingModel)
{
    if (motionBusy() || m_jobPart >= 0) {
        setNotice(tr("当前项目正在运行任务。请等待任务结束，或停止任务后关闭。"), true);
        return false;
    }
    if (m_original) m_original->flush();
    const auto name = m_projectPath.isEmpty() ? tr("多体动力学-%1").arg(m_modelNumber) : QFileInfo(m_projectPath).fileName();
    if (!m_modified) {
        return !deletingModel || QMessageBox::question(window(), tr("删除模型"), tr("确定要删除模型“%1”吗？").arg(name),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Yes;
    }
    const auto message = deletingModel ? tr("删除“%1”前，是否保存更改？").arg(name) : tr("是否保存“%1”的更改？").arg(name);
    const auto choice = QMessageBox::question(window(), tr("未保存的项目"), message,
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    return choice == QMessageBox::Discard || (choice == QMessageBox::Save && saveProject(false));
}

void MotionWindow::setProjectModified(bool modified)
{
    m_modified = modified;
    emit projectInfoChanged();
}

void MotionWindow::setProjectActive(bool active)
{
    if (m_projectActive == active) return;
    m_projectActive = active;
    if (!active) {
        m_restoreProjectEditor = m_editor && m_editor->property("mxOriginalMotionPanel").toBool()
            && m_selectionNode.kind != ModelNode::None && m_selectionNode.kind != ModelNode::Part
            && m_selectionNode.collection != QLatin1String("importedMnfBodies");
        for (auto* inspector : findChildren<QDialog*>(QStringLiteral("nativeModeInspector"))) inspector->close();
        setPlayback(false);
        closeEditor(true);
        if (m_original) {
            m_original->flush();
            m_original->disconnect(this);
            m_original.reset();
        }
        for (auto* picks : findChildren<LegacyPickContext*>(QString(), Qt::FindDirectChildrenOnly)) delete picks;
        deactivateOriginalCurveHost();
        m_docks->hideManagerAndFloatingWidgets();
        return;
    }
    if (m_projectCurveWindow) ensureOriginalCurveHost();
    if (m_running && !m_rigidPoll->isActive()) m_rigidPoll->start();
    if (m_restoreProjectEditor) {
        m_restoreProjectEditor = false;
        restoreProjectEditor();
    }
    updateParameters();
    m_docks->show();
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
    setProjectModified(false);
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
    setProjectModified(true);
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
    beginPartMesh(m_partId, m_meshSize->value(), m_meshKind->currentIndex()==0 ? MeshKind::Volume : MeshKind::Surface);
}

bool MotionWindow::beginPartMesh(int partId, double elementSize, MeshKind kind)
{
    const auto* part = m_document.part(partId);
    if (!part || m_jobPart != -1 || !(elementSize>0)) return false;
    if (!part->metreDisplaySource.IsNull()) {
        setNotice(tr("零件“%1”的毫米拓扑未通过校验，无法直接生成有限元网格。").arg(part->name), true);
        return false;
    }
    auto dir = std::make_unique<QTemporaryDir>(QDir::temp().filePath(QStringLiteral("MxMotionMesh-XXXXXX")));
    if (!dir->isValid()) {
        setNotice(tr("无法创建网格工作目录。"), true);
        return false;
    }
    QString error;
    if (!m_document.exportFile(dir->filePath(QStringLiteral("source.brep")), partId, error)) {
        setNotice(error, true);
        return false;
    }
#ifdef Q_OS_WIN
    const auto workerName = QStringLiteral("MxMotionMeshWorker.exe");
#else
    const auto workerName = QStringLiteral("MxMotionMeshWorker");
#endif
    m_worker->setProgram(qEnvironmentVariable("MXMOTION_MESH_WORKER", QDir(QApplication::applicationDirPath()).filePath(workerName)));
    m_worker->setWorkingDirectory(dir->path());
    m_jobKind = kind;
    m_worker->setArguments({ m_jobKind == MeshKind::Volume ? "volume" : "surface",
        "source.brep", "result.vol", QString::number(elementSize, 'g', 16) });
    m_jobPart = partId;
    m_jobError.clear();
    m_jobDir = std::move(dir);
    m_progress->show();
    updateActions();
    setNotice(tr("正在划分网格…"));
    m_meshTimeout->start(120000);
    m_worker->start();
    return true;
}

void MotionWindow::endMesh(int exitCode, const QString& processError)
{
    if (m_jobPart == -1) return;
    m_meshTimeout->stop();
    const int part = m_jobPart;
    const auto log = QString::fromLocal8Bit(m_worker->readAllStandardOutput()).trimmed();
    if (!log.isEmpty()) appendOutput(log.right(10000), NoticeLevel::Normal, false);
    QString message = m_jobError.isEmpty() ? processError : m_jobError;
    FemMesh mesh;
    bool meshUpdated=false;
    if (message.isEmpty() && exitCode == 0) {
        if (!m_jobDir) message = tr("网格工作目录已消失。");
        else if (readNetgenMesh(m_jobDir->filePath(QStringLiteral("result.vol")), m_jobKind, mesh, message)) {
            const auto nodes = qulonglong(mesh.nodes.size());
            const auto faces = qulonglong(mesh.surface.size());
            const auto tets = qulonglong(mesh.tetrahedra.size());
            m_meshes[part] = std::move(mesh);
            meshUpdated=true;
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
    if (meshUpdated) updateNativeFlexMeshForPart(part);
    m_progress->hide();
    updateParameters();
    updateActions();
    if (!message.isEmpty()) setNotice(message, true);
    auto completed=std::move(m_meshCompletion);
    m_meshCompletion={};
    if (completed) completed(message.isEmpty());
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
