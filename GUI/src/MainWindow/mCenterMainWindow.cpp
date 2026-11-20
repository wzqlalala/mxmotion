#include "mCenterMainWindow.h"
#include "mUiMenu.h"
#include "mCenterWindowProvider.h"
#include "mViewBrowserWindow.h"
#include "mViewportModelManager.h"
#include "ViewportContainer.h"
#include "ViewportWidget.h"
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QWidgetAction>
#include <QGridLayout>
#include <QPainter>
#include <QFile>
//【关闭挂起探针·临时】追加写关闭链路日志（定位完成后整体删除）
static void shutdownTrace(const QString& tag) {
    QFile file(QStringLiteral("E:/Mech_GUI/shutdown_trace.log"));
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        file.write(tag.toUtf8());
        file.write(QByteArray(1, 0x0A));
        file.close();
    }
}

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mSelectMenuFunction.h"
#include "CurveCore/mCurveDocController.h"   //布局互通：回写曲线页 kLayout
#include "CurveCore/CurveDefine.h"
#include "CurveCore/CurveDocument.h"

//Qt
#include <QDebug>

#include <QWidgetAction>
#include <QMenu>
#include <QTimer>
#include <QSignalMapper>
#include <QCheckBox>
#include <QLabel>
#include <QHBoxLayout>

//BasicFunction
#include "mGlobalSignals.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mFontRender.h"

#include "InteractionCore/Event/Action.h"
#include "InteractionCore/InteractionSystem.h"
#include "InteractionCore/Tool/CreateSketch2DGeometryTool.h"

//窗口
#include "mCurveWindow.h"
#include "mTableDataWindow.h"
#include "mOutputInfoWindow.h"
#include "mInfoWindow.h"
#include "mSectionWindow.h"
#include "IViewportWidget.h"
#include "PreViewportWidget.h"

#include "mShortcutManager.h"

//tip过滤器
#include "mCustomTooltip.h"
#include "ThemeManager.h"//中心窗嵌在 dock 内，applyAll 遍历顶层窗命中不到，经 extra applier 补发工具栏主题

#include "CameraManager.h"

//主题/尺寸色板（视口工具栏随主题重发，附 A）
#include "UiPalette.h"
#include "UiMetrics.h"
#include "mCheckComboBox.h"

using namespace mxr;
using namespace MechCommon;
namespace MechMainWindow
{
    static mCenterMainWindow* g_centerMainWindowInstance = nullptr;


    mCenterMainWindow* mCenterMainWindow::instance()
    {
        return g_centerMainWindowInstance;
    }

	mCenterMainWindow::mCenterMainWindow(QWidget *parent)
		: QMainWindow(parent)
	{
        g_centerMainWindowInstance = this;
        //GUIFoundation::mDockLayoutManager 预设重排钉正工具栏经此取件（解除其对MainWindow类型依赖）
        MechCommon::viewToolBarProvider() = [this]() -> QToolBar* { return getViewToolBar(); };
        MechCommon::viewDisplayBarProvider() = [this]() -> QToolBar* { return getViewDisplayBar(); };
        MechCommon::centerMainWindowProvider() = [this]() -> QMainWindow* { return this; };
        //动画工具栏壳通道（anim-tab-v13）：与上三者同注册点，PostWindow 面板经此取壳回挂
        MechCommon::animToolBarProvider() = [this]() -> QToolBar* { return _animBar; };

		init();

        // 切主题实时刷新兜底：本窗被 mDockLayoutManager 包进 ADS dock（非顶层窗口），
        // ThemeManager::applyAll 的 qobject_cast<QMainWindow*>(top) 遍历命中不到，
        // applyToolbarTheme 元方法不会被调用（表现为切主题要重启才生效）；
        // 经 dock 主题 extra applier 通道补发（applyAll 末尾必走 applyDockTheme）
        ThemeManager::addDockThemeExtraApplier([this]() { applyToolbarTheme(); });

        initShortcuts();
	}

	mCenterMainWindow::~mCenterMainWindow()
	{
		shutdownTrace("[centerWin] dtor begin");
		shutdownTrace("[centerWin] dtor end");
	}

    void mCenterMainWindow::initViewDisplayBar()
    {
        _viewBar = new QToolBar(QString::fromUtf8("视图显示栏"), this);
        _viewBar->setObjectName("viewDisplayBar");//布局持久化的键（saveState/restoreState 恢复位置用）
        _viewBar->setMovable(true);//显示拖动把手，可与其他工具栏一样拖拽停靠
        _viewBar->setIconSize(QSize(20, 20));//与视图工具栏（视图旋转中心等）图标同尺寸
        _viewBar->setStyleSheet(R"(
            QToolButton#layoutDropButton::menu-indicator,
            QToolButton#layoutDropButton::menu-arrow {
                image: none;
                width: 0px;
                height: 0px;
            }
            )");
        if (auto lay = _viewBar->layout())
        {
            lay->setSpacing(2);
            lay->setContentsMargins(4, 0, 4, 0);
        }

        //右对齐：先放一个可伸展的空白占位，把后续控件推到工具栏右端
        QWidget* barSpacer = new QWidget(_viewBar);
        barSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        _viewBar->addWidget(barSpacer);

        //1. 页下拉框
        _pageCombo = new MxFunctions::mCheckComboBox(_viewBar);
        _pageCombo->setMinimumWidth(200);
        _viewBar->addWidget(_pageCombo);
        connect(_pageCombo, QOverload<int>::of(&MxFunctions::mCheckComboBox::currentIndexChanged), this, [this](int index) {
            if (_pageComboSyncing || index < 0) return;
            const int pageID = _pageCombo->itemData(index).toInt();
            mViewBrowserWindow::getInstance()->getViewTree()->switchToPage(pageID);
            });

        //2. 视口布局：纯图标按钮，下拉为4列网格（每项一个布局图标，tooltip为名称）
        _layoutBtn = new QToolButton(_viewBar);
        _layoutBtn->setObjectName("layoutDropButton");//用于样式表隐藏下拉箭头
        _layoutBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _layoutBtn->setAutoRaise(true);
        _layoutBtn->setPopupMode(QToolButton::InstantPopup);
        _layoutBtn->setToolTip(QString::fromUtf8("视口布局"));

        QMenu* layoutMenu = new mUiMenu(_layoutBtn);
        QWidget* grid = new QWidget(layoutMenu);
        QGridLayout* gridLay = new QGridLayout(grid);
        gridLay->setContentsMargins(4, 4, 4, 4);
        gridLay->setSpacing(4);
        const auto& layouts = viewLayoutTable();
        for (int i = 0; i < layouts.size(); ++i)
        {
            QToolButton* cell = new QToolButton(grid);
            cell->setIcon(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + layouts[i].icon));
            cell->setIconSize(QSize(36, 28));
            cell->setToolButtonStyle(Qt::ToolButtonIconOnly);
            cell->setAutoRaise(true);
            cell->setFixedSize(46, 34);
            cell->setToolTip(QString::fromUtf8(layouts[i].name));
            const int layoutIndex = i;
            connect(cell, &QToolButton::clicked, this, [this, layoutIndex, layoutMenu]() {
                auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
                viewTree->setCurrentPageLayout(layoutIndex);
                //布局互通回写：当前视图页绑定了曲线页时，把布局下标提交到其 kLayout
                //（主窗口布局同步据此维持曲线对话框网格/页面属性/此处三方一致）
                {
                    auto* controller = PostCurve::mCurveDocController::getInstance();
                    auto* doc = controller->document();
                    for (int pageId : doc->childNodeIds(doc->rootNodeId()))
                    {
                        if (doc->nodeType(pageId) == PostCurve::CurveNodeType::Page
                            && controller->pageViewPageId(pageId) == viewTree->curPageID())
                        {
                            if (doc->nodeProperty(pageId, PostCurve::CurveProp::kLayout).toInt() != layoutIndex)
                                controller->setNodeProperty(pageId, PostCurve::CurveProp::kLayout, layoutIndex);
                            break;
                        }
                    }
                }
                layoutMenu->close();
                });
            gridLay->addWidget(cell, i / 4, i % 4);//4列网格
        }
        QWidgetAction* gridAction = new QWidgetAction(layoutMenu);
        gridAction->setDefaultWidget(grid);
        layoutMenu->addAction(gridAction);
        _layoutBtn->setMenu(layoutMenu);
        _viewBar->addWidget(_layoutBtn);

        //4. 视口联动（图标开关按钮，位于视口布局按钮之后）
        _syncBtn = new QToolButton(_viewBar);
        _syncBtn->setIcon(QIcon(":/mMainWidgets/Icon/CenterMainWindow/视口联动.svg"));
        _syncBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        _syncBtn->setAutoRaise(true);
        _syncBtn->setCheckable(true);
        _syncBtn->setToolTip(QString::fromUtf8("视口联动"));
        _viewBar->addWidget(_syncBtn);

        //5. <  1/2  > 翻页（整体容器，箭头使用SVG图标），置于工具栏末尾
        QWidget* pager = new QWidget(_viewBar);
        QHBoxLayout* pagerLay = new QHBoxLayout(pager);
        pagerLay->setContentsMargins(0, 0, 0, 0);
        pagerLay->setSpacing(0);

        QToolButton* prevBtn = new QToolButton(pager);
        prevBtn->setIcon(QIcon(":/mMainWidgets/Icon/CenterMainWindow/左箭头.svg"));
        prevBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        prevBtn->setAutoRaise(true);
        prevBtn->setToolTip(QString::fromUtf8("上一页"));
        pagerLay->addWidget(prevBtn);

        _pageLabel = new QLabel("1/1", pager);
        _pageLabel->setAlignment(Qt::AlignCenter);
        _pageLabel->setFixedWidth(38);
        pagerLay->addWidget(_pageLabel);

        QToolButton* nextBtn = new QToolButton(pager);
        nextBtn->setIcon(QIcon(":/mMainWidgets/Icon/CenterMainWindow/右箭头.svg"));
        nextBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        nextBtn->setAutoRaise(true);
        nextBtn->setToolTip(QString::fromUtf8("下一页"));
        pagerLay->addWidget(nextBtn);

        auto turnPage = [this](int delta) {
            auto view = mViewBrowserWindow::getInstance()->getViewTree();
            const auto pages = view->getModel()->pageList();
            if (pages.isEmpty()) return;
            int idx = 0;
            for (int i = 0; i < pages.size(); ++i)
            {
                if (pages[i].second == view->curPageID()) { idx = i; break; }
            }
            //循环翻页：1/2 点 "<" 跳到 2/2，末页点 ">" 回到首页
            idx = (idx + delta + pages.size()) % pages.size();
            view->switchToPage(pages[idx].second);
            };
        connect(prevBtn, &QToolButton::clicked, this, [turnPage]() { turnPage(-1); });
        connect(nextBtn, &QToolButton::clicked, this, [turnPage]() { turnPage(1); });

        _viewBar->addWidget(pager);
        connect(_syncBtn, &QToolButton::toggled, this, [this](bool checked) {
            VMManager->setViewportSync(checked);
            });

        //固定排在视图工具栏(_toolBar)之后（恢复后由 showEvent 兜底钉正）
        this->insertToolBar(_toolBar, _viewBar);

        //页状态变化 -> 刷新控件；初始刷新延迟到事件循环（视图树此时才完成同步）
        connect(mViewBrowserWindow::getInstance()->getViewTree(), &mViewTreeView::sig_pageStateChanged,
            this, &mCenterMainWindow::refreshViewBar);
        QTimer::singleShot(0, this, &mCenterMainWindow::refreshViewBar);
    }

    //动画工具栏壳（anim-tab-v13）：主窗口预创建空壳——壳在 restoreLayout（saveState/
    //restoreState）之前已存在，用户拖动过的位置可被持久化恢复（03-技术路线 §2.1 方案 A
    //核心依据）；内容件由 mPost3DResultPanel 经 MechCommon::animToolBarProvider 回挂。
    //可拖动重停/拖出悬浮/无标题栏紧凑单行（QToolBar 原生 movable 承担原型 grip，
    //01 FR-01 第 2 轮拍板）；初始隐藏——无后处理上下文不显示，显隐由面板
    //slot_viewportContextChanged 驱动（前处理隐藏/后处理常显，开放问题 6 结论）
    void mCenterMainWindow::initAnimToolBar()
    {
        _animBar = new QToolBar(QString::fromUtf8("动画"), this);
        _animBar->setObjectName("animDisplayBar");  //布局持久化的键（saveState/restoreState 恢复位置用）
        _animBar->setMovable(true);                 //显示拖动把手（原型 grip 由原生 handle 承担）
        _animBar->setFloatable(true);               //可拖出悬浮
        _animBar->setIconSize(QSize(20, 20));       //与视图显示栏同尺寸（applyToolbarTheme 统一重发）
        if (auto lay = _animBar->layout())
        {
            lay->setSpacing(2);
            lay->setContentsMargins(4, 0, 4, 0);
        }
        //视图工具栏/视图显示栏之后队尾追加（04 J-05 选①，落位口径见 init() 调用点注释）
        this->addToolBar(Qt::TopToolBarArea, _animBar);
        _animBar->setVisible(false);    //初始隐藏（初值同步=面板 slot_viewportContextChanged）
    }

    void mCenterMainWindow::applyToolbarTheme()
    {
        // 视口工具栏（视图工具栏/视图显示栏）按 UiPalette/UiMetrics 重发（附 A）：
        // 表面= surface 底、text 字、边框 border；按钮透明底、悬停 hover、选中/按下 selected；
        // 尺寸取 H（控件外高）与 icon 档值（规则 1.1 顶部图标按钮同口径）。
        const QString surface = MxFunctions::UiPalette::surface().name();
        const QString text = MxFunctions::UiPalette::text().name();
        const QString border = MxFunctions::UiPalette::border().name();
        const QString hover = MxFunctions::UiPalette::hover().name();
        const QString selected = MxFunctions::UiPalette::selected().name();
        const QString disabled = MxFunctions::UiPalette::disabled().name();
        const QString panel = MxFunctions::UiPalette::panel().name();
        const int ctlH = MxFunctions::UiMetrics::controlHeight();
        const int iconSz = MxFunctions::UiMetrics::controlIconSize();
        const int bodySz = MxFunctions::UiMetrics::bodyFontSize();

        for (QToolBar* bar : { _toolBar, _viewBar, _animBar })
        {
            if (!bar) continue;
            bar->setIconSize(QSize(iconSz, iconSz));
            // 保留原"隐藏下拉指示器"规则（mappingDropButton/snapDropButton/layoutDropButton），
            // 追加主题色部分（原只写死隐藏指示器，底色/文字/按钮色均未随主题）
            // border-top:none：工具栏紧贴菜单栏下方，顶边线与菜单栏底边线重合为
            // 一条（由菜单栏 RibbonPage 底边线承担），避免出现两条平行边线
            bar->setStyleSheet(QStringLiteral(
                "QToolBar { background:%1; color:%2; border:1px solid %3; border-top:none; spacing:2px; }"
                "QToolBar::separator { background:%3; width:1px; height:1px; margin:4px 4px; }"
                // width/height 覆盖全局 QSS 的 QToolButton 尺寸（=baseH）：工具栏
                // 按钮宽高贴栏级图标尺寸（iconSz=controlIconSize），行高不随 baseH
                // 抬高；QSS 宽高为下限，带文字按钮（映射/捕捉）仍按内容加宽
                "QToolButton { color:%2; background:transparent; border:none; width:%7px; height:%7px; }"
                "QToolButton:hover { background:%4; border-radius:3px; }"
                "QToolButton:pressed, QToolButton:checked { background:%5; border-radius:3px; }"
                "QToolButton:disabled { color:%6; }"
                "QToolButton#mappingDropButton::menu-indicator,"
                "QToolButton#snapDropButton::menu-indicator,"
                "QToolButton#layoutDropButton::menu-indicator,"
                "QToolButton#mappingDropButton::menu-arrow,"
                "QToolButton#snapDropButton::menu-arrow,"
                "QToolButton#layoutDropButton::menu-arrow { image:none; width:0px; height:0px; }"
            ).arg(surface, text, border, hover, selected, disabled, QString::number(iconSz)));

            // 栏内子控件（下拉框/数字框/标签/吸附菜单行）随主题
            for (QWidget* w : bar->findChildren<QWidget*>())
            {
                if (qobject_cast<QToolButton*>(w))
                    continue; // 已由上方 QToolBar 的 QSS 覆盖
                if (auto* combo = qobject_cast<QComboBox*>(w))
                {
                    combo->setFixedHeight(ctlH);
                    combo->setStyleSheet(QStringLiteral(
                        "QComboBox { font-family:'%10'; font-size:%1px; color:%2; background:%3; border:1px solid %4; border-radius:2px; padding:1px 4px; }"
                        "QComboBox:hover { border-color:%5; }"
                        "QComboBox:focus { border-color:%5; }"
                        "QComboBox:disabled { color:%6; background:%7; }"
                        "QComboBox::drop-down { subcontrol-origin:padding; subcontrol-position:top right; width:18px; border:none; }"
                        "QComboBox QAbstractItemView { background:%3; color:%2; border:1px solid %4;"
                        " selection-background-color:%8; selection-color:%9; }"
                    ).arg(bodySz).arg(text, surface, border)
                     .arg(MxFunctions::UiPalette::focus().name())
                     .arg(disabled, panel)
                     .arg(MxFunctions::UiPalette::selected().name(), MxFunctions::UiPalette::selectedText().name())
                     .arg(MxFunctions::UiMetrics::uiFontFamily()));
                    continue;
                }
                if (auto* spin = qobject_cast<QDoubleSpinBox*>(w))
                {
                    spin->setFixedHeight(ctlH);
                    spin->setStyleSheet(QStringLiteral(
                        "QDoubleSpinBox { font-family:'%8'; font-size:%1px; color:%2; background:%3; border:1px solid %4; border-radius:2px; padding:1px 4px; }"
                        "QDoubleSpinBox:hover { border-color:%5; }"
                        "QDoubleSpinBox:focus { border-color:%5; }"
                        "QDoubleSpinBox:disabled { color:%6; background:%7; }"
                    ).arg(bodySz).arg(text, surface, border)
                     .arg(MxFunctions::UiPalette::focus().name())
                     .arg(disabled, panel)
                     .arg(MxFunctions::UiMetrics::uiFontFamily()));
                    continue;
                }
                if (auto* lbl = qobject_cast<QLabel*>(w))
                {
                    if (lbl->pixmap())
                        continue; // 图标标签不盖底色（避免遮住贴图）
                    lbl->setStyleSheet(QStringLiteral(
                        "QLabel { font-family:'%3'; font-size:%1px; color:%2; background:transparent; border:none; }"
                    ).arg(bodySz).arg(text)
                     .arg(MxFunctions::UiMetrics::uiFontFamily()));
                    continue;
                }
                if (auto* chk = qobject_cast<QCheckBox*>(w))
                {
                    chk->setStyleSheet(QStringLiteral(
                        "QCheckBox { color:%1; background:transparent; }"
                        "QCheckBox:disabled { color:%2; }"
                    ).arg(text, disabled));
                    continue;
                }
            }
        }

        // 吸附/映射下拉菜单及网格布局菜单：与全局命令菜单统一（规则 1.0）。
        // 统一走 ThemeManager::applyMenuStyle（清空 QSS + mUiStyle 全自绘），
        // 不再内联 QSS——只要菜单挂 QSS，Qt 的 QStyleSheetStyle 就会接管行/指示器
        // 绘制，自绘的打勾线/凹槽拦截永远不生效（勾选框退回底座凹槽的根因）
        for (QMenu* m : findChildren<QMenu*>())
            ThemeManager::applyMenuStyle(m);
        // 吸附菜单行内控件（QWidgetAction 宿主）：行内 QLabel/QCheckBox 已在上方 findChildren
        // 覆盖不到（其父是 QMenu 而非工具栏），这里补发
        if (_snapMenu)
        {
            for (QWidget* w : _snapMenu->findChildren<QWidget*>())
            {
                if (auto* lbl = qobject_cast<QLabel*>(w))
                {
                    if (lbl->pixmap())
                        continue; // 图标标签不盖样式（避免遮住贴图）
                    lbl->setStyleSheet(QStringLiteral(
                        "QLabel { font-family:'%3'; font-size:%1px; color:%2; background:transparent; }"
                    ).arg(bodySz).arg(text)
                     .arg(MxFunctions::UiMetrics::uiFontFamily()));
                }
                else if (auto* chk = qobject_cast<QCheckBox*>(w))
                {
                    chk->setStyleSheet(QStringLiteral(
                        "QCheckBox { color:%1; background:transparent; }"
                    ).arg(text));
                }
            }
        }
    }

    void mCenterMainWindow::refreshViewBar()
    {
        auto view = mViewBrowserWindow::getInstance()->getViewTree();
        const auto pages = view->getModel()->pageList();

        _pageComboSyncing = true;
        _pageCombo->blockSignals(true);
        _pageCombo->clear();
        int curIdx = 0;
        for (int i = 0; i < pages.size(); ++i)
        {
            _pageCombo->addItem(pages[i].first, pages[i].second);
            if (pages[i].second == view->curPageID()) curIdx = i;
        }
        if (_pageCombo->count() > 0) _pageCombo->setCurrentIndex(curIdx);
        _pageCombo->blockSignals(false);
        _pageComboSyncing = false;

        _pageLabel->setText(QString("%1/%2").arg(curIdx + 1).arg(pages.size()));

        //布局索引钳制到有效范围，越界（旧数据）回退单视口
        int layoutIdx = view->curPageLayout();
        const auto& layouts = viewLayoutTable();
        if (layoutIdx < 0 || layoutIdx >= layouts.size()) layoutIdx = 0;
        _layoutBtn->setIcon(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + layouts[layoutIdx].icon));
    }

    void mCenterMainWindow::init()
    {
        _toolBar = new QToolBar("视图工具栏", this);
        _toolBar->setObjectName("viewToolBar");//布局持久化的键
        _toolBar->setIconSize(QSize(20, 20));
        _toolBar->setStyleSheet(R"(
            QToolBar { spacing: 0px; }
            QToolButton#mappingDropButton::menu-indicator,
            QToolButton#snapDropButton::menu-indicator {
                image: none;
                width: 0px;
                height: 0px;
                subcontrol-position: right center;
            }
            QToolButton#mappingDropButton::menu-arrow,
            QToolButton#snapDropButton::menu-arrow {
                image: none;
                width: 0px;
                height: 0px;
            }
            )");
        if (auto layout = _toolBar->layout()) {
            layout->setSpacing(0);
            layout->setContentsMargins(2, 0, 2, 0);
        }

        auto createAction = [this](const QString& iconPath, const QString& ttTitle, const QString& ttBody) {
            QAction* action = new QAction(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + iconPath), "", this);
            action->setProperty("ttTitle", ttTitle);
            action->setProperty("ttBody", ttBody);
            _toolBar->addAction(action);
            return action;
            };

        //图标路径、标题文本、提示文本
        struct ModeInfo {
            QString iconPath;
            QString ttTitle;
            QString ttBody;
        };

        QVector<ModeInfo> viewModes = {
            {"kongjianxuanzhuan1.png", "空间旋转","空间旋转"},
            {"pingmianxuanzhuan.png", "平面旋转","平面旋转"},
            {"pingyi3x.png", "平移","平移"},//后面添加分割线
        };
        _leftViewActionGroup = new QActionGroup(this);
        _leftViewActionGroup->setExclusive(true);
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(true);
            _leftViewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::triggered, this, [this, index](bool checked) {
                this->slot_setLeftButtonViewMode(index);
                });
        }
        _toolBar->addSeparator();
        _leftViewActionGroup->actions().at(0)->setChecked(true);

        _scaleViewActionGroup = new QActionGroup(this);
        _scaleViewActionGroup->setExclusive(false);
        viewModes = QVector<ModeInfo>{ {"zishiyingshitu.png", "自适应视图","自适应视图"},
            {"suoxiaoyibei.png", "缩小一倍","缩小一倍"},
            {"fangdayibei.png", "放大一倍","放大一倍"}
        };
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(false);
            _scaleViewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::triggered, this, [this, index](bool checked) {
                this->slot_setScaleViewMode(index);
                });
        }
        _toolBar->addSeparator();

        _rotateViewActionGroup = new QActionGroup(this);
        _rotateViewActionGroup->setExclusive(false);
        viewModes = QVector<ModeInfo>{ {"shituxuanzhuanzhongxin.png", "视图旋转中心","视图旋转中心"},
            {"moxingxuanzhuanzhongxin.png", "模型旋转中心","模型旋转中心"},
            {"zidingyixuanzhuanzhongxin.png", "自定义旋转中心","自定义旋转中心"},
            {"nishizhenxuanzhuan.png", "逆时针旋转","逆时针旋转"},
            {"shunshizhenxuanzhuan.png", "顺时针旋转","顺时针旋转"}
        };
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(false);
            _rotateViewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::triggered, this, [this, index](bool checked) {
                this->slot_setRotateViewMode(index);
                });
            if (i == 3)
            {
                _rotateSpinBox = new QDoubleSpinBox();
                _rotateSpinBox->setDecimals(1);
                _rotateSpinBox->setRange(0, 360);
                _rotateSpinBox->setValue(30.0);
                _rotateSpinBox->setButtonSymbols(QAbstractSpinBox::ButtonSymbols::NoButtons);
                _toolBar->addWidget(_rotateSpinBox);
            }
        }
        _toolBar->addSeparator();

        _viewActionGroup = new QActionGroup(this);
        _viewActionGroup->setExclusive(false);
        viewModes = QVector<ModeInfo>{ {"qianshitu.png", "前视图","前视图"},
            {"houshitu.png", "后视图","后视图"},
            {"shangshitu.png", "仰视图","仰视图"},
            {"xiashitu.png", "俯视图","俯视图"},
            {"zuoshitu.png", "左视图","左视图"},
            {"youshitu.png", "右视图","右视图"},
            {"zhouceshitu.png", "轴侧视图","轴侧视图"},
            {"baocunshijiao.png", "保存视角","保存视角"},
            {"diaoyongshijiao.png", "调用视角","调用视角"},
        };
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(false);
            _viewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::triggered, this, [this, index](bool checked) {
                this->slot_setViewMode(index);
                });
        }
        _toolBar->addSeparator();

        _showViewActionGroup = new QActionGroup(this);
        _showViewActionGroup->setExclusive(true);
        viewModes = {
            {"lunkuoxian.png", "轮廓线","轮廓线"},
            {"xiankuangmoxing.png", "线框模型","线框模型"},
            {"yincangxianmoxing.png", "隐藏线模型","隐藏线模型"},
            {"wanggemoxing.png", "网格模型","网格模型"},
        };
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(true);
            _showViewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::toggled, this, [this, index](bool checked) {
                if (checked)
                {
                    this->slot_setShowMode(index);
                }
                });
        }
        _showViewActionGroup->actions().at(3)->setChecked(true);

        _actionShowMesh = createAction("wanggexianshi3x.png", "网格显示", "显示网格");
        _actionShowMesh->setCheckable(true);
        connect(_actionShowMesh, &QAction::toggled, this, [this](bool checked) {
            if (!getCVM)
            {
                return;
            }
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            if (viewportPrivate == nullptr)
            {
                return;
            }
            auto preViewport = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivate);
            if (preViewport == nullptr)
            {
                return;
            }
            if (checked == preViewport->isActiveShowMesh())
            {
                return;
            }
            preViewport->setActiveShowMesh(checked);//设置显隐状态
            if (_currentSecondMenuName == "网格" || checked)
            {
                getCVM->getPreRenderWidget()->getPreRender()->setGeoShowFuntion(ShowFuntion::ALL);
                slot_setShowMode(preViewport->_showViewCheckedStatus);
            }
            else
            {
                getCVM->getPreRenderWidget()->getPreRender()->setGeoShowFuntion(ShowFuntion::ALL);
                getCVM->getPreRenderWidget()->getPreRender()->setMeshShowFuntion(ShowFuntion::None);
            }
            if (preViewport->isActiveShowMesh())
            {
                _showViewActionGroup->setEnabled(true);
            }
            else
            {
                _showViewActionGroup->setEnabled(false);
            }
            });
        _actionShowMesh->setChecked(true);
        _toolBar->addSeparator();

        _selectViewActionGroup = new QActionGroup(this);
        _selectViewActionGroup->setExclusive(true);
        viewModes = {
            {"juxing3x.png", "矩形","矩形框选"},
            {"yuanxing3x.png", "圆形","圆形框选"},
            {"duobianxing3x.png", "多边形","多边形框选"},
        };
        for (int i = 0; i < viewModes.size(); i++)
        {
            auto viewMode = viewModes.at(i);
            auto action = createAction(viewMode.iconPath, viewMode.ttTitle, viewMode.ttBody);
            action->setCheckable(true);
            _selectViewActionGroup->addAction(action);
            int index = i;
            connect(action, &QAction::toggled, this, [this, index](bool checked) {
                this->slot_setselectMode(index);
                });
        }
        _selectViewActionGroup->actions().at(0)->setChecked(true);

        // 互斥 Action 组
        QActionGroup* group = new QActionGroup(this);
        group->setExclusive(true);

        QVector<ModeInfo> modes = {
        {"zhinengxuanze3x.png", "智能选择","根据鼠标位置选择或突出选择模型上的\n面、边和顶点，点击三下显示母体"},
        {"shiqudingdian3x.png", "拾取顶点","选择或突出显示模型上的顶点"},
        {"shiqubian3x.png",   "拾取边","选择或突出显示模型上的边"},
        {"shiqumian3x.png", "拾取面","选择或突出显示模型上的面"},
        {"shiquti3x.png", "拾取体","选择或高亮显示模型主体"},
        {"shiqujiedian3x.png",   "拾取节点","选择或突出显示网格顶点"},
        {"shiqudanyuanxian3x.png", "拾取单元线","选择或突出显示网格单元线"},
        {"shiqudanyuanmian3x.png",   "拾取单元面","选择或突出显示网格单元面"},
        {"shiqudanyuan3x.png", "拾取单元","选择或突出显示模型上的单元"},
        {"shiqubujian3x.png", "拾取部件","选择或突出显示模型上的部件"}
        };

        //提前创建
        _mappingMenu = new mUiMenu(this);
        // 循环创建
        for (int i = 0; i < modes.size(); ++i)
        {
            QAction* action = new QAction(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + modes[i].iconPath), "", this);
            mInfoWindow::getInstance()->_actions->append(action);
            connect(action, &QAction::toggled, this, [this](bool checked) {
                if (checked)
                {
                    QAction* action = qobject_cast<QAction*>(sender());
                    int modeIndex = action->data().toInt();
                    slot_onPickModeSelected(modeIndex);
                    MxFunctions::mGlobalSignals::getInstance()->pickFilterChangeSig();

                    // 映射菜单改用统一函数控制
                    if (modeIndex != 0)
                    {
                        // 非智能选择：额外禁用与当前拾取模式同类型的映射项
                        updateMappingMenuState(false, modeIndex - 1);
                    }
                    else
                    {
                        // 智能选择：无额外禁用项
                        updateMappingMenuState();
                    }
                }
                });
            action->setCheckable(true);
            action->setData(i); // 存储模式编号
            group->addAction(action);

            // 用 QToolButton 来定制外观
            QToolButton* btn = new QToolButton();
            btn->setDefaultAction(action);
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
            btn->setAutoRaise(true); // 扁平化样式
            btn->installEventFilter(new TooltipInstaller(modes[i].ttTitle,
                QString(":/mMainWidgets/Icon/CenterMainWindow/") + modes[i].iconPath, modes[i].ttBody, 500, btn));
            _toolBar->addWidget(btn);
            _actionPick.append(action);
            if (i == 0)
                action->setChecked(true); // 默认选中第一个
        }

        _toolBar->addSeparator();
        // ===================== 下拉按钮 =====================
        QToolButton* dropButton = new QToolButton();
        dropButton->setText("映射");
        dropButton->setIcon(QIcon(":/mMainWidgets/Icon/CenterMainWindow/yinshe@3x.png"));
        dropButton->setPopupMode(QToolButton::InstantPopup); // 立即弹出菜单
        dropButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        dropButton->setAutoRaise(true);
        dropButton->installEventFilter(new TooltipInstaller("映射",
            QString(":/mMainWidgets/Icon/CenterMainWindow/yinshe.png"), "", 500, dropButton));
        // 创建下拉菜单
        //QMenu* menu = new mUiMenu(dropButton);
        dropButton->setMenu(_mappingMenu);
        _mappingMenu->setAttribute(Qt::WA_Hover, true);
        _mappingMenu->installEventFilter(new MenuTipFilter(_mappingMenu, 400, this));

        modes = QVector<ModeInfo>{
            {"dingdian3x.png", "点",""},
            {"xian3x.png",   "线",""},
            {"mian3x.png", "面",""},
            {"ti3x.png", "体",""},
            {"jiedian3x.png",   "节点",""},
            {"danyuanxian3x.png", "单元线",""},
            {"danyuanmian3x.png",   "单元面",""},
            {"danyuan3x.png", "单元",""},
            {"ti3x.png", "部件",""}
        };
        // 循环创建
        for (int i = 0; i < modes.size(); ++i)
        {
            QAction* action = new QAction(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + modes[i].iconPath), modes[i].ttTitle, _mappingMenu);
            _mappingMenu->addAction(action);
            action->setCheckable(false);
            action->setData(i + 1); // 存储模式编号
            action->setProperty("ttTitle", modes[i].ttTitle);
            action->setProperty("ttBody", modes[i].ttBody);
            action->setToolTip("");
        }

        dropButton->setMenu(_mappingMenu);
        _toolBar->addWidget(dropButton);

        // 菜单点击槽：映射
        connect(_mappingMenu, &QMenu::triggered, this, [=](QAction* act) {
            int modeIndex = act->data().toInt();
            slot_onPickMapping(modeIndex);
            });

        _toolBar->addSeparator();

        // ===================== 对象捕捉下拉按钮 =====================
        _snapButton = new QToolButton();
        _snapButton->setObjectName("snapDropButton");
        _snapButton->setText("草图吸附设置");
        _snapButton->setIcon(QIcon(":/mMainWidgets/Icon/CenterMainWindow/duixiangbuzhuo.png"));
        _snapButton->setPopupMode(QToolButton::InstantPopup);
        _snapButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        _snapButton->setAutoRaise(true);
        _snapButton->installEventFilter(new TooltipInstaller("对象捕捉",
            QString(":/mMainWidgets/Icon/CenterMainWindow/duixiangbuzhuo@3x.png"), "设置对象捕捉类型", 500, _snapButton));

        _snapMenu = new mUiMenu(_snapButton);
        _snapMenu->setAttribute(Qt::WA_Hover, true);
        _snapMenu->installEventFilter(new MenuTipFilter(_snapMenu, 400, this));
        _snapButton->setMenu(_snapMenu);

        struct SnapModeInfo {
            QString iconPath;
            QString text;
        };
        QVector<SnapModeInfo> snapModes = {
            {"yuanxin.png", "圆心"},
            {"zhongdian.png", "中点"},
            {"sifenzhiyidian.png", "四分之一点"},
            {"xingxindian.png", "形心点"},
            {"duandian.png", "端点"},
            {"jiaodian.png", "交点"},
            {"pingmianwangedian.png", "平面网格点"}
        };

        for (int i = 0; i < snapModes.size(); ++i)
        {
            QWidgetAction* widgetAction = new QWidgetAction(_snapMenu);
            QWidget* itemWidget = new QWidget();
            QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
            itemLayout->setContentsMargins(6, 2, 6, 2);
            itemLayout->setSpacing(6);

            QCheckBox* checkBox = new QCheckBox();
            checkBox->setChecked(true);
            QLabel* iconLabel = new QLabel();
            iconLabel->setPixmap(QIcon(QString(":/mMainWidgets/Icon/CenterMainWindow/") + snapModes[i].iconPath).pixmap(16, 16));
            QLabel* textLabel = new QLabel(snapModes[i].text);
            textLabel->setStyleSheet(QStringLiteral("font-family: '%1'; font-size: 12px;")
                .arg(MxFunctions::UiMetrics::uiFontFamily()));

            itemLayout->addWidget(checkBox);
            itemLayout->addWidget(iconLabel);
            itemLayout->addWidget(textLabel);
            itemLayout->addStretch();
            itemWidget->setLayout(itemLayout);

            widgetAction->setDefaultWidget(itemWidget);
            _snapMenu->addAction(widgetAction);
            _snapActions.append(widgetAction);

            int index = i;
            connect(checkBox, &QCheckBox::toggled, this, [this, index](bool checked) {
                slot_snapOptionChanged(index, checked);
                });

            // 初始同步一次默认勾选状态到草图吸附工具
            slot_snapOptionChanged(i, true);
        }

        _toolBar->addWidget(_snapButton);
        _toolBar->addSeparator();
    
		//添加窗口同步功能
		//QAction* actSync = new QAction(QIcon(QString(":/mMainWidgets/Icon/Custom center.png")), "窗口同步", this);
		//connect(actSync, &QAction::triggered, this, &mCenterMainWindow::slot_winSync);
		//actSync->setCheckable(true);
		//QToolButton* syncBtn = new QToolButton();
		//syncBtn->setDefaultAction(actSync);
		//syncBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
		//syncBtn->setAutoRaise(true);
		//_toolBar->addWidget(syncBtn);

        // 添加到主窗口
        this->addToolBar(_toolBar);
        _toolBar->installEventFilter(new ToolbarTipInstaller(_toolBar, 450, _toolBar));

        //视图显示栏（页选择/翻页/布局/联动）
        initViewDisplayBar();

        //动画工具栏壳（anim-tab-v13）：视图显示栏之后队尾追加（04 J-05 选①——
        //insertToolBar(before,bar) 语义为"插到 before 之前"，与"紧随视图显示栏"矛盾；
        //addToolBar 追加队尾+resetViewToolBarLayout 尾部同序补挂）
        initAnimToolBar();

        // 初始主题下发一次（切主题后由 ThemeManager::applyAll 经元方法重发）
        applyToolbarTheme();

        //初始化视口容器（新架构）并设为中央部件
        //先注册创建回调（容器定义在 MainWindow 模块，PreData 通过回调反向打通）
        VMManager->setViewportContainerCreateFunc([]() -> ViewportContainer* {
            return new ViewportContainer();
            });
        VMManager->initViewportContainer();
        if (auto* container = VMManager->getViewportContainer())
        {
            this->setCentralWidget(container);
            // 容器激活信号驱动管理器恢复当前模型/视口（发出 currentViewportChanged(int)）。
            // DirectConnection（默认）：激活是低频用户操作，同步处理保证面板状态立即一致；
            // 删除/析构场景的激活转移已在容器侧屏蔽信号（activateNextAvailableViewport 不发信号），
            // 不会走到这里
            connect(container, &ViewportContainer::sig_activeViewportChanged,
                VMManager, &MechPreData::mViewportModelManager::slot_viewportActivated);

            // 激活切换同步轴标定位模式：激活视口=默认位置（旋转时文字恒在视口内），
            // 未激活视口=按当前姿态可见内容包围盒贴左下角（渲染库 SetViewportActive）
            connect(container, &ViewportContainer::sig_activeViewportChanged,
                VMManager, &MechPreData::mViewportModelManager::slot_syncViewportAxisActive);

            // 视口从隐藏转为可见（切页/单格布局激活换入/窗口还原）时补一帧渲染：
            // 隐藏期间 updatePaint 触发的 update() 对隐藏部件无效，重新显示的是旧帧
            connect(container, &ViewportContainer::sig_viewportShown,
                VMManager, &MechPreData::mViewportModelManager::refreshViewportRender);

            // 注册容器操作回调到 mViewportModelManager
            VMManager->setViewportContainerCallbacks(
                [container](int viewportID, QWidget* content) {
                    container->addViewport(viewportID, content);
                },
                [container](int viewportID) {
                    container->removeViewport(viewportID);
                },
                [container](int viewportID) {
                    container->setActiveViewport(viewportID);
                }
            );
            // 注册容器查询回调（可见性/标题同步/当前激活视口ID——供轴标定位同步等消费）
            VMManager->setViewportContainerQueryCallbacks(
                [container](int viewportID) -> bool {
                    auto* vw = container->viewport(viewportID);
                    return vw && vw->isVisible();
                },
                [container](int viewportID, const QString& name) {
                    if (auto* vw = container->viewport(viewportID))
                    {
                        vw->setTitle(name);
                    }
                },
                [container]() -> int {
                    return container->activeViewportID();
                }
            );

            // 登记容器创建之前已存在的视口（管理器构造时创建的首个视口），
            // 否则启动后中央区域无任何视口可见
            int firstViewportID = -1;
            for (int modelID : VMManager->getAllModelDataID())
            {
                auto* viewportModel = VMManager->getViewportModel(modelID);
                if (!viewportModel) continue;
                const QList<QWidget*> subWindows = viewportModel->getSubWindows();
                for (auto* sub : subWindows)
                {
                    auto* priv = viewportModel->getViewportPrivate(sub);
                    if (!priv) continue;
                    if (container->viewport(priv->getViewportID())) continue; //幂等
                    container->addViewport(priv->getViewportID(),
                        priv->getWidget() ? priv->getWidget()->asWidget() : nullptr);
                    //容器标题栏初始为占位名（视口-ID），同步真实名称
                    if (auto* vw = container->viewport(priv->getViewportID()))
                    {
                        vw->setTitle(priv->getViewportName());
                    }
                    if (firstViewportID == -1)
                    {
                        firstViewportID = priv->getViewportID();
                    }
                }
            }
            // 视图树懒汉构造可能早于容器登记（syncFromViewports 的布局空跑），
            // 登记完成后补一次布局确保首个视口铺满中央区
            mViewBrowserWindow::getInstance()->getViewTree()->refreshCurrentPageLayout();
            // 激活首个视口恢复当前模型/视口
            if (firstViewportID != -1)
            {
                VMManager->activateViewportByID(firstViewportID);
            }
        }
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::activePickSig, this, [this, group](int pickFilter) {
            auto actions = group->actions();
            if (actions.size() >= pickFilter)
            {
                actions[pickFilter]->setChecked(true);
            }
            });

		// 通用拾取控件 setAllowedTypes 转发：置灰白名单外拾取按钮，从根上禁用非法拾取数据。
		// 空 QVector=全部启用。若当前勾选类型被禁用则自动切到首个可用类型。
		connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::setPickTypesEnabledSig,
            this, [this, group](const QVector<bool>& enabled) {
                _pickTypesEnabled = enabled;  // 保存全局拾取类型启用状态
                auto actions = group->actions();
                const int n = actions.size();
                const bool noLimit = enabled.isEmpty();
                for (int i = 0; i < n; ++i)
                {
                    actions[i]->setEnabled(noLimit || (i < enabled.size() && enabled[i]));
                }
                // 当前勾选类型被禁用（或处于无工具栏项的模式，如偏置数/符号拾取）
                // 时自动切换。模式切换决策统一收敛在此处（原控制器侧还有一份
                // switch 推导与其互相覆盖，已移除）：
                // 优先网格类(5-9)，其次智能拾取(0)，最后几何类(1-4)——
                // 原实现取首个可用恒为 0=智能拾取，白名单含节点的场景会被兜到智能
                static const int kFallbackOrder[] = { 5, 6, 7, 8, 9, 0, 1, 2, 3, 4 };
                QAction* checkedAction = group->checkedAction();
                if (!checkedAction || !checkedAction->isEnabled())
                {
                    for (int idx : kFallbackOrder)
                    {
                        if (idx < n && actions[idx]->isEnabled())
                        {
                            actions[idx]->setChecked(true);
                            break;
                        }
                    }
                }
                // 同步更新映射菜单状态
                updateMappingMenuState();
            });

		connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mCenterMainWindow::slot_subWindowActivated);
		connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::switchMenuBarWindow, this, &mCenterMainWindow::slot_switchMenuBarWindow);//切换菜单信号
	}

	void mCenterMainWindow::setViewToolBarVisibility(bool isShow, QAction* action)
	{
		_toolBar->setVisible(isShow);
		connect(_toolBar, &QToolBar::visibilityChanged, this, [this, action](bool visible) {
			action->setChecked(visible);
			}, Qt::UniqueConnection);
	}

    void mCenterMainWindow::showEvent(QShowEvent* event)
    {
        QMainWindow::showEvent(event);
        //布局恢复后跳过钉正：restoreState 已恢复用户保存的工具栏区域，
        //不能再 insertToolBar 重置区域。
        //同时支持动态属性标记（mDockLayoutManager::restoreLayout 经 provider 设置）
        if (_skipToolbarPinning || property("skipToolbarPinning").toBool()) return;
        // 无保存布局时，等当前显示事件及布局恢复任务结束后再校正顺序；
        // 不再 removeToolBar/insertToolBar 连续拆装，也不覆盖用户显隐状态。
        if (!_viewBarOrderPinned && _toolBar && _viewBar)
        {
            _viewBarOrderPinned = true;
            QTimer::singleShot(0, this, [this]()
            {
                if (!_toolBar || !_viewBar || _skipToolbarPinning
                    || property("skipToolbarPinning").toBool())
                    return;
                resetViewToolBarLayout();
            });
        }
    }

    void mCenterMainWindow::resetViewToolBarLayout()
    {
        if (!_toolBar || !_viewBar)
            return;

        _viewBarOrderPinned = true;
        QTimer::singleShot(0, this, [this]()
        {
            if (!_toolBar || !_viewBar)
                return;
            this->removeToolBar(_toolBar);
            this->removeToolBar(_viewBar);
            _toolBar->setParent(nullptr);
            _viewBar->setParent(nullptr);
            this->addToolBar(Qt::TopToolBarArea, _toolBar);
            this->addToolBar(Qt::TopToolBarArea, _viewBar);
            _toolBar->setVisible(true);
            _viewBar->setVisible(true);
            //v1.4（anim-tab-v13）：动画壳同序补挂（04 J-05①——全新环境无保存布局时本
            //函数只重挂两栏，_animBar 漂移到两栏之左与"视图显示栏之后队尾"口径不符；
            //按 [_toolBar,_viewBar,_animBar] 同序收口）。显隐不动：归面板
            //slot_viewportContextChanged 上下文驱动
            if (_animBar != nullptr)
            {
                this->removeToolBar(_animBar);
                _animBar->setParent(nullptr);
                this->addToolBar(Qt::TopToolBarArea, _animBar);
            }
        });
    }

    void mCenterMainWindow::setViewToolBarArea(Qt::ToolBarArea toolBarArea)
	{
		this->removeToolBar(_toolBar);
		this->addToolBar(toolBarArea, _toolBar);
		_toolBar->setVisible(true);
        //视图显示栏跟随到同一区域并固定在视图工具栏之后
        if (_viewBar)
        {
            this->insertToolBar(_toolBar, _viewBar);
        }
	}

    void mCenterMainWindow::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
        // TODO: 待按表核对 - 主工具栏按钮使能规则（窗口激活时）
        // 当前规则：仅在前处理3D视口(Viewport3D)时启用 Mapping菜单 与 前5个拾取模式按钮
        if (!getCVM)
        {
            return;
        }

        //获取显示网格状态
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        auto rend = viewportPrivate->getRendWidget();
        if (rend == nullptr)
        {
            return;
        }
        auto preViewport = dynamic_cast<MechPreData::mPreViewportPrivate*>(viewportPrivate);
        if (preViewport != nullptr)//前处理特有
        {
            _actionShowMesh->setChecked(preViewport->isActiveShowMesh());
            if (preViewport->isActiveShowMesh())
            {
                _showViewActionGroup->setEnabled(true);
            }
            else
            {
                _showViewActionGroup->setEnabled(false);
            }
        }

        if (_showViewActionGroup && _showViewActionGroup->actions().size() == 4)
        {
            _showViewActionGroup->actions().at(viewportPrivate->_showViewCheckedStatus)->setChecked(true);
        }
        if (_leftViewActionGroup && _leftViewActionGroup->actions().size() == 3)
        {
            _leftViewActionGroup->actions().at(viewportPrivate->_leftViewCheckedStatus)->setChecked(true);
        }
        if (_selectViewActionGroup && _selectViewActionGroup->actions().size() == 3)
        {
            _selectViewActionGroup->actions().at(viewportPrivate->_multiplyPickStatus)->setChecked(true);
        }

        if (_connPick1)
        {
            QObject::disconnect(_connPick1);
        }
        // TODO: 待按表核对 - Mapping菜单与拾取按钮使能：
        // - Viewport3D: enable=true, Mapping菜单全部启用, _actionPick[0..4]启用
        // - 非Viewport3D: enable=false, 上述全部禁用
        bool enable = viewportPrivate->getViewportType() == Viewport3D ? true : false;

        // 拾取按钮启用逻辑保持不变
        for (int i = 0; i < _actionPick.size(); ++i)
        {
            auto action = _actionPick.at(i);
            action->setEnabled(enable);
            if (i == 4)
            {
                break;
            }
        }

        // 映射菜单改用统一函数控制（叠加全局拾取类型禁用 + 视口总开关）
        updateMappingMenuState();

        if (viewportPrivate->getViewportType() == Viewport3D && getCVM->getPreRenderWidget())
        {
            _connPick1 = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mCenterMainWindow::slot_finishedPick);
            slot_finishedPick();
        }
        else if (viewportPrivate->getViewportType() == ViewportPost3D)
        {

        }

        //获取拾取模式
        int mode = viewportPrivate->_pickFilter;

        // 发射信号
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(mode);
    }

    void mCenterMainWindow::slot_finishedPick()
    {
        // TODO: 待按表核对 - Mapping菜单使能规则（拾取完成时）
        // 当前规则：
        // - 无拾取对象时：Mapping菜单全部禁用
        // - 有拾取对象时：Mapping菜单全部启用，但当前 pickDataType 对应的映射项禁用
        //   （避免将对象映射到自身类型，例如 GeoFace->GeoFace）
        if (!getCVM)
        {
            return;
        }
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        if (viewportPrivate->getViewportType() == Viewport3D)
        {
            int pickNum = 0;
            auto pickDataType = getCVM->getCurrentPickDataType(pickNum);

            if (pickNum == 0)
            {
                // 无拾取对象：全部禁用
                updateMappingMenuState(true);
                return;
            }

            // 有拾取对象：启用所有合法项 + 禁用与拾取类型一致的项
            updateMappingMenuState(false, static_cast<int>(pickDataType));
        }
        else if (viewportPrivate->getViewportType() == ViewportPost3D)
        {
        }
    }

    void mCenterMainWindow::slot_switchMenuBarWindow(const QString& name)
    {
        if (!getCVM->getPreRenderWidget())
            return;
        _currentSecondMenuName = name;
        MechPreData::mPreViewportPrivate* m_currentViewportPrivate = dynamic_cast<MechPreData::mPreViewportPrivate*>(VMManager->getCurrentViewportPrivate());
        if (m_currentViewportPrivate != nullptr)//前处理特有
        {
            bool checked = m_currentViewportPrivate->isActiveShowMesh();

            if (_currentSecondMenuName == "网格" || checked)
            {
                getCVM->getPreRenderWidget()->getPreRender()->setGeoShowFuntion(ShowFuntion::ALL);
                slot_setShowMode(m_currentViewportPrivate->_showViewCheckedStatus);
            }
            else
            {
                getCVM->getPreRenderWidget()->getPreRender()->setGeoShowFuntion(ShowFuntion::ALL);
                getCVM->getPreRenderWidget()->getPreRender()->setMeshShowFuntion(ShowFuntion::None);
            }
        }
    }

    void mCenterMainWindow::slot_onPickModeSelected(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        // mode = 0, 1, 2
        //qDebug() << "当前模式编号:" << mode;
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        auto render = viewportPrivate->getRendWidget();
        if (render == nullptr) return; // render 可能为空

        // 记录修改前的过滤器
        int oldMode = viewportPrivate->_pickFilter;
        PickFilter newFilter = PickFilter::PickGeoPoint;

        // 根据 mode 设置新的过滤器
        switch (mode)
        {
        case 0: newFilter = PickFilter::PickGeoSmartPick; break;
        case 1: newFilter = PickFilter::PickGeoPoint; break;
        case 2: newFilter = PickFilter::PickGeoLine; break;
        case 3: newFilter = PickFilter::PickGeoFace; break;
        case 4: newFilter = PickFilter::PickGeoSolid; break;
        case 5: newFilter = PickFilter::PickMeshNode; break;
        case 6: newFilter = PickFilter::PickMeshLine; break;
        case 7: newFilter = PickFilter::PickMeshFace; break;
        case 8: newFilter = PickFilter::PickMeshElement; break;
        case 9: newFilter = PickFilter::PickGeoPart | PickFilter::PickMeshPart; break;
        default: break;
        }
        viewportPrivate->_pickFilter = mode;
        // 设置新的 pickFilter
        render->setPickFilter(newFilter);

        // 判断新旧是否匹配
        //if (mode != oldMode)
        //{
        //	//清空拾取
        //	getCVM->clearPick();
        //}

    }

    void mCenterMainWindow::slot_onPickMapping(int mode)
    {
        switch (mode)
        {
        case 0:	// 智能拾取
            //render->setPickFilter(PickFilter::SmartPickGeo);
            break;
        case 1:	// 几何点
            mSelectMenuFunction::getInstance()->slot_mappingGeoPoint();
            break;
        case 2:	// 几何边
            mSelectMenuFunction::getInstance()->slot_mappingGeoEdge();
            break;
        case 3:	// 几何面
            mSelectMenuFunction::getInstance()->slot_mappingGeoFace();
            break;
        case 4:	// 几何体
            mSelectMenuFunction::getInstance()->slot_mappingGeoSolid();
            break;
        case 5:	// 网格点
            mSelectMenuFunction::getInstance()->slot_mappingNode();
            break;
        case 6:	// 网格线
            mSelectMenuFunction::getInstance()->slot_mappingMeshLine();
            break;
        case 7:	// 网格面
            mSelectMenuFunction::getInstance()->slot_mappingMeshFace();
            break;
        case 8:	// 网格单元
            mSelectMenuFunction::getInstance()->slot_mappingEle();
            break;
        case 9:	// 部件
            mSelectMenuFunction::getInstance()->slot_mappingPart();
            break;
        default:
            break;
        }

    }

    void mCenterMainWindow::slot_setLeftButtonViewMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        viewportPrivate->_leftViewCheckedStatus = mode;
        rend = viewportPrivate->getRendWidget();
        auto widiget = viewportPrivate->getWidget();
        if (rend == nullptr)
        {
            return;
        }
        auto& input_map = widiget->interaction()->keymap()->base(mxr::Domain::Navigation);
        switch (mode)
        {
        case 0:
            input_map.clear();
            input_map.bind(InputChord{ GestureType::Press, Qt::LeftButton, Qt::NoModifier }, Action::Cam_Orbit);
            input_map.bind(InputChord{ GestureType::Press, Qt::MiddleButton, Qt::NoModifier }, Action::Cam_Pan);   // 平移
            input_map.bind(InputChord{ GestureType::Wheel, Qt::NoButton, Qt::NoModifier }, Action::Cam_Zoom);      // 缩放
            rend->setCameraKeys({ Qt::LeftButton, Qt::NoModifier }, CameraOperateMode::Rotate);
            rend->SetRotateType(RotateType::Rotate_XY);
            break;
        case 1:
            input_map.clear();
            input_map.bind(InputChord{ GestureType::Press, Qt::LeftButton, Qt::NoModifier }, Action::Cam_Orbit);
            input_map.bind(InputChord{ GestureType::Press, Qt::MiddleButton, Qt::NoModifier }, Action::Cam_Pan);   // 平移
            input_map.bind(InputChord{ GestureType::Wheel, Qt::NoButton, Qt::NoModifier }, Action::Cam_Zoom);      // 缩放
            rend->setCameraKeys({ Qt::LeftButton, Qt::NoModifier }, CameraOperateMode::Rotate);
            rend->SetRotateType(RotateType::Rotate_Z);
            break;
        case 2:
            input_map.clear();
            input_map.bind(InputChord{ GestureType::Press, Qt::LeftButton, Qt::NoModifier }, Action::Cam_Pan);
            input_map.bind(InputChord{ GestureType::Press, Qt::MiddleButton, Qt::NoModifier }, Action::Cam_Pan);   // 平移
            input_map.bind(InputChord{ GestureType::Wheel, Qt::NoButton, Qt::NoModifier }, Action::Cam_Zoom);      // 缩放
            rend->setCameraKeys({ Qt::LeftButton, Qt::NoModifier }, CameraOperateMode::Translate);
            break;
        }
        rend->updatePaint();
    }

    void mCenterMainWindow::slot_setScaleViewMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        rend = viewportPrivate->getRendWidget();
        if (rend == nullptr)
        {
            return;
        }
        switch (mode)
        {
        case 0:
        {
            rend->FitView();
            VMManager->setCameraSyncTimer(500, 30, [rend]() {
                emit rend->signal_CameraSync();
                });
        }
        break;
        case 1:rend->SetZoomAtViewCenter_ByButton(ScaleDirection::Zoom_in);
            break;
        case 2:rend->SetZoomAtViewCenter_ByButton(ScaleDirection::Zoom_out);
            break;
        }
        rend->updatePaint();
    }

    void mCenterMainWindow::slot_setRotateViewMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        rend = viewportPrivate->getRendWidget();
        auto viewport_widget = viewportPrivate->getWidget();
        if (rend == nullptr)
        {
            return;
        }
        switch (mode)
        {
        case 0:rend->SetRotateCenterToViewCenter();
            break;
        case 1:rend->SetRotateCenterToModelCenter();
            break;
        case 2:
            //rend->SetRotateCenterToPoint();
            viewport_widget->interaction()->dispatch(Action::SetRotateCenter);
            break;
        case 3:rend->slotSetRotate_ByButton(_rotateSpinBox->value());///2025.11.4调整旋转函数由 void SetRotate_ByButton(float angle)改为void slotSetRotate_ByButton(float angle, ModeGeomTransf _axis_type = Z_AXIS_PLUS)。
            break;
        case 4:rend->slotSetRotate_ByButton(-_rotateSpinBox->value());///2025.11.4调整旋转函数由 void SetRotate_ByButton(float angle)改为void slotSetRotate_ByButton(float angle, ModeGeomTransf _axis_type = Z_AXIS_PLUS)。
        }
        rend->updatePaint();
    }

    void mCenterMainWindow::slot_setViewMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        rend = viewportPrivate->getRendWidget();
        if (rend == nullptr)
        {
            return;
        }
        switch (mode)
        {
        case 0:rend->SetPerspective(Perspective::Front);
            break;
        case 1:rend->SetPerspective(Perspective::Rear);
            break;
        case 2:rend->SetPerspective(Perspective::Top);
            break;
        case 3:rend->SetPerspective(Perspective::Bottom);
            break;
        case 4:rend->SetPerspective(Perspective::Left);
            break;
        case 5:rend->SetPerspective(Perspective::Right);
            break;
        case 6:rend->SetPerspective(Perspective::ISO1);
            break;
        case 7:rend->SaveCurrentView();
            break;
        case 8:rend->CallSavedView();
            break;
        }
        VMManager->setCameraSyncTimer(500, 30, [rend]() {
            emit rend->signal_CameraSync();
            });
        rend->updatePaint();
    }

    void mCenterMainWindow::slot_setselectMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        mBaseRend* rend = nullptr;

        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }
        viewportPrivate->_multiplyPickStatus = mode;
        rend = viewportPrivate->getRendWidget();
        if (rend == nullptr)
        {
            return;
        }
        switch (mode)
        {
        case 0:rend->setMultiplyPickMode(MultiplyPickMode::QuadPick);
            break;
        case 1:rend->setMultiplyPickMode(MultiplyPickMode::RoundPick);
            break;
        case 2:rend->setMultiplyPickMode(MultiplyPickMode::PolygonPick);
            break;
        }
        VMManager->setCameraSyncTimer(500, 30, [rend]() {
            emit rend->signal_CameraSync();
            });
        rend->updatePaint();
    }

    void mCenterMainWindow::slot_setShowMode(int mode)
    {
        if (!getCVM)
        {
            return;
        }
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        if (viewportPrivate == nullptr)
        {
            return;
        }

        viewportPrivate->_showViewCheckedStatus = mode;
        if (viewportPrivate->getViewportType() == Viewport3D)
        {
            CCSMRendWidget* rend = getCVM->getPreRenderWidget();
            if (rend == nullptr)
            {
                return;
            }
            auto render = rend->getPreRender();
            if (render == nullptr)
            {
                return;
            }
            mxr::ShowFuntion showFuntion;
            switch (mode)
            {
            case 0:showFuntion = ShowFuntion::WireEdge;
                break;
            case 1:showFuntion = ShowFuntion::WireFrame;
                break;
            case 2:showFuntion = ShowFuntion::ElementFace;
                break;
            case 3:showFuntion = ShowFuntion::ALL;
                break;
            }
            render->setGeoShowFuntion(showFuntion);
            render->setMeshShowFuntion(showFuntion);
            rend->updatePaint();
        }

        else
        {
            REBUILD_PostRend* rend = getCVM->getPostRenderWidget();
            if (rend == nullptr)
            {
                return;
            }
            auto render = rend->getPostRender();
            if (render == nullptr)
            {
                return;
            }
            switch (mode)
            {
            case 0:render->setShowFuntion(ShowFuntion::WireEdge);
                break;
            case 1:render->setShowFuntion(ShowFuntion::WireFrame);
                break;
            case 2:render->setShowFuntion(ShowFuntion::ElementFace);
                break;
            case 3:render->setShowFuntion(ShowFuntion::SmoothShaded);
                break;
            }
            rend->updatePaint();
        }
    }

    void mCenterMainWindow::slot_winSync()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        if (action == nullptr)	return;

		int isChecked = action->isChecked();
		int mode /*= Common::mViewportManager::getInstance()->getViewportDisplayMode()*/;
		if (isChecked)
		{
			if (mode == 1)
			{
				REBUILD_PostRend* Render1/* = Common::mViewportManager::getInstance()->getRendByViewName("AIPredict")*/;
				PostCommon::CameraManager::getInstance().addObserver((mBaseRend*)Render1);

				CCSMRendWidget* Render2 = getCVM->getPreRenderWidget();
				PostCommon::CameraManager::getInstance().addObserver((mBaseRend*)Render2);

			}
			else if (mode == 2)
			{
				//REBUILD_PostRend* Render1 = Common::mViewportManager::getInstance()->getRendByViewName("ModelAssess_1");
				//PostCommon::CameraManager::getInstance().addObserver((mBaseRend*)Render1);

				//REBUILD_PostRend* Render2 = Common::mViewportManager::getInstance()->getRendByViewName("ModelAssess_2");
				//PostCommon::CameraManager::getInstance().addObserver((mBaseRend*)Render2);

				//REBUILD_PostRend* Render3 = Common::mViewportManager::getInstance()->getRendByViewName("ModelAssess_3");
				//PostCommon::CameraManager::getInstance().addObserver((mBaseRend*)Render3);
			}
			//Common::mViewportManager::getInstance()->setViewportSync(true);

		}
		else
		{
			//PostCommon::CameraManager::getInstance().removeAllObserver();
			//Common::mViewportManager::getInstance()->setViewportSync(false);
		}
	}

    std::vector<int> mCenterMainWindow::getCheckedSnapTypes() const
    {
        std::vector<int> checkedIndexes;
        for (int i = 0; i < _snapActions.size(); ++i)
        {
            auto widgetAction = qobject_cast<QWidgetAction*>(_snapActions.at(i));
            if (!widgetAction) continue;
            auto itemWidget = widgetAction->defaultWidget();
            if (!itemWidget) continue;
            auto checkBox = itemWidget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked())
            {
                checkedIndexes.push_back(i);
            }
        }
        return checkedIndexes;
    }

    void mCenterMainWindow::slot_snapOptionChanged(int index, bool checked)
    {
        // index 与 AdsorbPointType 的对应关系
        // 0-圆心(arcOrCircle), 1-中点(edgeMiddle), 2-四分之一点(quarterEdge)
        // 3-形心点(faceCenter), 4-端点(geoPoint), 5-交点(intersection), 6-平面网格点(planeGrid)
        if (!getCVM || !getCVM->getPreViewportWidget())
        {
            return;
        }
        auto tool = getCVM->getPreViewportWidget()->interaction()->getToolManager()->
            getTool<mxr::InteractionCreateSketch2DGeometryTool>(mxr::InteractionToolType::CreateSketch2DGeometry);
        if (tool == nullptr)
        {
            return;
        }

        static const std::vector<mxr::AdsorbPointType> kIndexToAdsorb = {
            mxr::AdsorbPointType::arcOrCircle,
            mxr::AdsorbPointType::edgeMiddle,
            mxr::AdsorbPointType::quarterEdge,
            mxr::AdsorbPointType::faceCenter,
            mxr::AdsorbPointType::geoPoint,
            mxr::AdsorbPointType::intersection,
            mxr::AdsorbPointType::planeGrid
        };

        // 收集当前所有勾选的吸附类型
        std::vector<mxr::AdsorbPointType> enabledTypes;
        for (int i = 0; i < _snapActions.size() && i < static_cast<int>(kIndexToAdsorb.size()); ++i)
        {
            auto widgetAction = qobject_cast<QWidgetAction*>(_snapActions.at(i));
            if (!widgetAction) continue;
            auto itemWidget = widgetAction->defaultWidget();
            if (!itemWidget) continue;
            auto checkBox = itemWidget->findChild<QCheckBox*>();
            if (checkBox && checkBox->isChecked())
            {
                enabledTypes.push_back(kIndexToAdsorb[i]);
            }
        }

        if (enabledTypes.empty())
        {
            // 没有任何捕捉类型勾选时，取消吸附模式
            tool->enableAdsorbTypes(false);
        }
        else
        {
            // 设置吸附模式
            tool->enableAdsorbTypes(true, enabledTypes);
        }
    }

    void mCenterMainWindow::initShortcuts()
    {
        auto mgr = MechMainWindow::ShortcutManager::instance(this);

        // ---- Global ----
        mgr->registerShortcut(ShortcutModule::Global, "fix_viewer",
            ShortcutItem{ QKeySequence("Ctrl+C"), "视图居中",
              [&] { this->slot_setScaleViewMode(0); } });

        mgr->dumpAllShortcuts();
    }

    void mCenterMainWindow::updateMappingMenuState(bool allDisable, int extraDisableIndex)
    {
        auto viewportPrivate = VMManager->getCurrentViewportPrivate();
        // 总开关：非3D视口直接全部禁用
        bool isViewport3D = viewportPrivate && viewportPrivate->getViewportType() == Viewport3D;
        auto dropActions = _mappingMenu->actions();
        const bool noPickLimit = _pickTypesEnabled.isEmpty();

        for (int i = 0; i < dropActions.size(); ++i)
        {
            bool enabled = isViewport3D && !allDisable;

            if (enabled)
            {
                // 第一层：与拾取按钮置灰同步（全局拾取类型禁用）
                // 映射索引i 对应 拾取索引 i+1（拾取0是智能选择，无对应映射项）
                int pickIndex = i + 1;
                enabled = noPickLimit || (pickIndex < _pickTypesEnabled.size() && _pickTypesEnabled[pickIndex]);
            }

            // 第二层：额外禁用（当前拾取模式 / 当前拾取到的对象类型）
            if (enabled && extraDisableIndex >= 0 && i == extraDisableIndex)
            {
                enabled = false;
            }

            dropActions[i]->setEnabled(enabled);
        }
    }
}