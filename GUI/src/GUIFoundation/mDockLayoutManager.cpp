#include "mDockLayoutManager.h"
#include "mUiMenu.h"
#include "mCenterWindowProvider.h"	//中心窗/工具栏经提供者解耦（mCenterMainWindow自注册）
#include "mFloatingDockFrameless.h"	//浮动 dock 容器自绘标题栏注入（拖出窗口与主窗风格统一）
#include <QToolBar>
#include "DockManager.h"
#include "DockAreaWidget.h"
#include "DockAreaTitleBar.h"
#include "DockAreaTabBar.h"
#include "ElidingLabel.h"
#include "DockWidget.h"
#include "DockWidgetTab.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "UiPalette.h"	//dock 窗口头/主体取语义色板（规则 0.2/2）
#include "UiMetrics.h"	//窗口头高/动作按钮/内留白按档位公式（规则 2）
#include "mFoldSection.h"	//折叠组布局刷新（applyThemeToAll）
#include "mCurveWindow.h"	//函数与曲线窗口底色刷新
#include "ThemeManager.h"	//注册 dock 主题重发入口（applyAll 统一驱动）

//Qt
#include <QMainWindow>
#include <QToolButton>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QSplitter>
#include <QCoreApplication>
#include <QLabel>
#include <QAbstractButton>
#include <QScrollArea>
#include <QLayout>
#include <QStyle>

using namespace ads;
using namespace MxFunctions;

namespace MechMainWindow
{
    mDockLayoutManager* mDockLayoutManager::_instance = nullptr;
    mDockLayoutManager* mDockLayoutManager::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mDockLayoutManager();
        }
        return _instance;
    }
    mDockLayoutManager::~mDockLayoutManager()
    {

    }
    mDockLayoutManager::mDockLayoutManager()
    {
       
    }

    void mDockLayoutManager::setCDockManager(ads::CDockManager* dockManager, QMainWindow* mainwin)
    {
        _dockManager = dockManager;
        // 拖出的浮动 dock 容器统一注入自绘标题栏（与主窗/首选项同款，
        // 标题栏拖动仍可拖回停靠；须在首个浮动容器出现前接入）
        mFloatingDockFrameless::attach(dockManager);
        applyTheme();
        // 切主题/切档由 ThemeManager::applyAll 统一驱动（Common 不反向链接 GUIFoundation，
        // 经函数指针注册解耦）
        MechMainWindow::ThemeManager::registerDockThemeApplier(
            []() { mDockLayoutManager::getInstance()->applyTheme(); });
        //dock 同一条 applyDockTheme 下发链补发（函数指针注册，不经换肤信号）：
        //折叠组布局令牌 + 函数与曲线窗口/画布底色，切档/切主题时随 dock 一起重发
        static bool s_uiAppliersRegistered = false;
        if (!s_uiAppliersRegistered)
        {
            s_uiAppliersRegistered = true;
            MechMainWindow::ThemeManager::addDockThemeExtraApplier(
                []() { MechMainWindow::FoldWidget::applyThemeToAll(); });
            MechMainWindow::ThemeManager::addDockThemeExtraApplier(
                []() { if (mCurveWindow* cw = mCurveWindow::getInstance()) cw->applyWindowTheme(); });
        }

        m_mainWin = mainwin;
    }

    void mDockLayoutManager::applyTheme()
    {
        if (!_dockManager) return;

        // 基础 QSS 只读一次（qrc 文件 IO + 大文本读取在每次切档时重复做是纯浪费）
        static const QString kBaseCss = []() {
            QFile styleSheetFile(QStringLiteral(":/mMainWidgets/default.css"));
            styleSheetFile.open(QIODevice::ReadOnly);
            QTextStream styleSheetStream(&styleSheetFile);
            QString css = styleSheetStream.readAll();
            styleSheetFile.close();
            return css;
        }();
        QString Stylesheet = kBaseCss;

        // 规则 2：dock 公共窗口头不套顶部功能区页签的选中蓝字样式。
        // 激活/非激活只承担切换功能，标题统一 text 色、常规 400 字重；
        // 保留 label 色区分非激活页签，其他公共头部不随激活态换色。
        // 标题字号 12px；页签不额外加左边距。
        // 头部高 H+2、动作按钮 H−2、图形 icon、按钮间距 4px。
        const int headerH = MxFunctions::UiMetrics::windowHeaderHeight();
        const int closeSz = MxFunctions::UiMetrics::windowHeaderControl();
        const int iconSz = qMax(MxFunctions::UiMetrics::baseH() / 2 + 3,
                                MxFunctions::UiMetrics::baseG() + 10);
        const int bodySz = MxFunctions::UiMetrics::bodyFontSize();
        Stylesheet += QStringLiteral(
            "ads--CDockAreaTitleBar { background:%1; border:none; border-bottom:1px solid %2; padding:0; }"
            "ads--CDockAreaTitleBar QLabel { font-family:'%13'; font-size:%3px; font-weight:400; }"
            "ads--CDockAreaTabBar { background:%1; border:none; }"
            "ads--CDockWidgetTab { background:%1; color:%4; border:none; padding:0; height:%5px; }"
            "ads--CDockWidgetTab QLabel { color:%4; background:transparent; border:none; font-family:'%13'; font-size:%3px; font-weight:400; }"
            "ads--CDockWidgetTab[activeTab=\"false\"] { background:%1; }"
            "ads--CDockWidgetTab[activeTab=\"false\"] QLabel { color:%11; background:transparent; }"
            "ads--CDockWidgetTab[activeTab=\"true\"] { background:%12; }"
            "ads--CDockWidgetTab[activeTab=\"true\"] QLabel { color:%4; background:transparent; font-weight:400; }"
            "ads--CDockWidgetTab:hover { background:%7; }"
            "#tabCloseButton { width:%8px; height:%8px; background:transparent; border:none; padding:0; margin:0 0 0 4px; qproperty-iconSize:%9px %9px; }"
            "#tabCloseButton:hover { background:%7; border:none; border-radius:4px; }"
            "#tabCloseButton:pressed { background:%10; }"
            "ads--CDockAreaTitleBar QLabel { background:transparent; color:%4; font-family:'%13'; font-size:%3px; font-weight:400; }"
            "ads--CDockAreaTitleBar ads--CTitleBarButton { width:%8px; height:%8px; background:transparent; border:none; padding:0; margin:0 0 0 4px; qproperty-iconSize:%9px %9px; }"
            "ads--CDockAreaTitleBar ads--CTitleBarButton:hover { background:%7; border-radius:4px; }"
            "ads--CDockAreaTitleBar ads--CTitleBarButton:pressed { background:%10; }"
            "ads--CAutoHideDockContainer ads--CDockAreaTitleBar { background:%1; color:%4; border-bottom:1px solid %2; }"
            "ads--CAutoHideDockContainer ads--CDockAreaWidget[focused=\"true\"] ads--CDockAreaTitleBar { background:%1; }"
            "ads--CAutoHideDockContainer ads--CTitleBarButton:hover { background:%7; }"
            "ads--CAutoHideDockContainer ads--CTitleBarButton:pressed { background:%10; }"
        )
            .arg(UiPalette::header().name())                 // %1 公共窗口头底
            .arg(UiPalette::border().name())                 // %2 头部下边线
            .arg(bodySz)                                     // %3 正文字号
            .arg(UiPalette::text().name())                   // %4 标题统一 text，不用 selectedText
            .arg(headerH)                                    // %5 头部高
            .arg(UiPalette::hover().name())                  // %7 悬停
            .arg(closeSz)                                    // %8 动作按钮
            .arg(iconSz)                                     // %9 图形盒
            .arg(UiPalette::selected().name())               // %10 按下
            .arg(UiPalette::label().name())                  // %11 非激活页签文字
            .arg(UiPalette::surface().name())                // %12 激活页签与主体底色一致
            .arg(UiMetrics::uiFontFamily());                 // %13 平台 UI 字体族

        // dock 区域间分割条（模型树|参数栏等所有 ADS 分界线）与 auto-hide 侧栏边线。
        // default.css 里这些位置写死 #F0F3F6/palette(dark)/#E5ECF0，只适配浅色底，
        // 深/护眼下与面板 surface 底对比不足，分界线几乎不可分辨。
        // 分割条取 separator：规则 0.2 里专为"各主题下都比 surface 明显更深"定义的
        // 分隔线语义色（浅 #a8b3c2 / 深 #7d8ca3 / 护眼 #7e9c72）；
        // auto-hide 侧栏/调整把手边线取 border，与窗口头下边线同源。
        // 注意伪态用单冒号 :horizontal/:vertical，双冒号是子控件语法不会命中。
        Stylesheet += QStringLiteral(
            "ads--CDockContainerWidget ads--CDockSplitter::handle { background:%1; }"
            "ads--CDockContainerWidget ads--CDockSplitter::handle:horizontal { width:1px; background:%1; }"
            "ads--CDockContainerWidget ads--CDockSplitter::handle:vertical { height:1px; background:%1; }"
            "ads--CAutoHideSideBar { background:%2; border:none; }"
            "ads--CAutoHideSideBar[sideBarLocation=\"0\"] { border-bottom:1px solid %3; }"
            "ads--CAutoHideSideBar[sideBarLocation=\"1\"] { border-right:1px solid %3; }"
            "ads--CAutoHideSideBar[sideBarLocation=\"2\"] { border-left:1px solid %3; }"
            "ads--CAutoHideSideBar[sideBarLocation=\"3\"] { border-top:1px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"false\"][sideBarLocation=\"0\"],"
            "ads--CAutoHideTab[iconOnly=\"false\"][sideBarLocation=\"2\"] { border-top:6px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"false\"][sideBarLocation=\"1\"],"
            "ads--CAutoHideTab[iconOnly=\"false\"][sideBarLocation=\"3\"] { border-bottom:6px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"true\"][sideBarLocation=\"0\"] { border-top:6px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"true\"][sideBarLocation=\"1\"] { border-left:6px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"true\"][sideBarLocation=\"2\"] { border-right:6px solid %3; }"
            "ads--CAutoHideTab[iconOnly=\"true\"][sideBarLocation=\"3\"] { border-bottom:6px solid %3; }"
            "ads--CResizeHandle { background:%2; }"
            "ads--CAutoHideDockContainer[sideBarLocation=\"0\"] ads--CResizeHandle { border-top:1px solid %3; }"
            "ads--CAutoHideDockContainer[sideBarLocation=\"1\"] ads--CResizeHandle { border-left:1px solid %3; }"
            "ads--CAutoHideDockContainer[sideBarLocation=\"2\"] ads--CResizeHandle { border-right:1px solid %3; }"
            "ads--CAutoHideDockContainer[sideBarLocation=\"3\"] ads--CResizeHandle { border-top:1px solid %3; }"
        )
            .arg(UiPalette::separator().name())              // %1 分割条（各主题可辨）
            .arg(UiPalette::header().name())                 // %2 auto-hide 侧栏/把手底
            .arg(UiPalette::border().name());                // %3 边线

        // 相等跳过：dock 管理器样式表覆盖整个停靠区，串不变时（如纯档位切换
        // 且相关尺寸未变）整区重抛光是切档卡顿的大头之一
        if (_dockManager->styleSheet() != Stylesheet)
            _dockManager->setStyleSheet(Stylesheet);

        // 新区域可能由拖动、恢复布局或切换页签动态产生，不能只在注册时处理一次
        for (ads::CDockAreaWidget* area : _dockManager->findChildren<ads::CDockAreaWidget*>())
            applyAreaHeaderRules(area);
        for (ads::CDockWidget* dock : _dockManager->findChildren<ads::CDockWidget*>())
            applyDockHeaderRules(dock);

        // 信息输出窗的可折叠分割条（CollapsibleSplitterHandle，自绘 handle）：
        // 中线/折叠按钮在其 applyTheme 里按色板重画（把手不铺底色），这里经元方法触发
        applyCollapsibleHandlesTheme();

        // 已注册 dock 的底色随主题；PostWidow 的业务绘图主体保持原样。
        // 同串跳过：Qt 的 setStyleSheet 不比较新旧值
        for (auto it = m_docks.begin(); it != m_docks.end(); ++it)
        {
            ads::CDockWidget* dock = it.value();
            if (!dock) continue;
            if (!it.key().contains("PostWidow"))
            {
                const QString dockQss = QStringLiteral("background-color: %1;").arg(UiPalette::surface().name());
                if (dock->styleSheet() != dockQss)
                    dock->setStyleSheet(dockQss);
            }
        }

        // 页签颜色由 updateTabBackground 写在页签自身的内联样式表决定，
        // 优先级高于上面的 manager 级样式表：切主题时若不逐个重发，
        // 旧主题色会一直残留，直到切换一次标签（activeTabChanged）才恢复。
        // 上面的 findChildren<CDockWidget*> 遍历依赖 ADS 内部父子关系
        //（dock 关闭/浮动/自动隐藏后可能脱离 manager 子树，遍历会漏），
        // 这里以 m_docks 注册表为准再补一轮，保证每个已注册 dock 的页签都拿到新色
        for (auto it = m_docks.begin(); it != m_docks.end(); ++it)
        {
            if (it.value())
                applyDockHeaderRules(it.value());
        }

        // ADS 在布局恢复、拖动和延迟显示时还会重建/显示区域，补一次收尾即可
        QTimer::singleShot(0, this, [this]() { applyHeaderRulesToChildren(); });
    }

    void mDockLayoutManager::setCenterWidget(QWidget* w)
    {
        m_center = _dockManager->createDockWidget("中心视图");
        // 去掉 default.css 里 ads--CDockWidget 的 1px 顶边框：中心 dock 紧贴菜单栏
        // 下方，这 1px 浅色边框正夹在菜单栏底边线与视图工具栏顶边线之间，
        // 表现为两区域间的一条缝隙；去掉后两条边界线重合为一条
        m_center->setStyleSheet(QStringLiteral("border:none;"));
        m_center->setWidget(w);
        _dockManager->setCentralWidget(m_center);
    }

    void mDockLayoutManager::activateDock(const QString& key)
    {
        //重置布局的移除过程中会经视口/树选中信号同步重入到这里，此时dock处于半移除状态，直接忽略
        if (m_resettingLayout)
            return;

        if (!m_docks.contains(key))
            return;

        auto dock = m_docks[key];
        if (!dock || !dock->dockManager())
            return;
        dock->toggleView(true);
        //// 如果 Dock 被关闭，先显示出来
        //if (!dock->isVisible())
        //    dock->show();

        //// 将 Dock 聚焦
        //dock->raise();                       // 放到最上层
        //dock->activateWindow();              // 激活窗口
        //dock->setFocus(Qt::OtherFocusReason);
        //auto tabBar = dock->tabWidget();
        //auto area = dock->dockAreaWidget();
        //if(area)
        //    area->show();
        //    area->setCurrentDockWidget(dock);

       // 更新 QAction 状态 
       if (m_actions.contains(key)) {
           QAction* action = m_actions[key];
           action->setChecked(true); // 设置当前 QAction 为选中状态 
       }
    }

    ads::CDockWidget* mDockLayoutManager::GetDock(const QString& key)
    {
        return m_docks[key];
    }

    void mDockLayoutManager::applyCollapsibleHandlesTheme()
    {
        if (!_dockManager) return;
        // 自绘分割条（中线/折叠按钮在 applyTheme 里按色板重画；信息输出窗把手不铺底色）：
        // 信息输出窗 CollapsibleSplitterHandle 与曲线窗 CurveSplitterHandle，
        // 这里经元方法统一触发（两类都带 Q_OBJECT + Q_INVOKABLE applyTheme）
        for (QWidget* w : _dockManager->findChildren<QWidget*>())
        {
            const char* cls = w->metaObject()->className();
            if (qstrcmp(cls, "CollapsibleSplitterHandle") == 0 ||
                qstrcmp(cls, "CurveSplitterHandle") == 0)
                QMetaObject::invokeMethod(w, "applyTheme", Qt::DirectConnection);
        }
    }

    void mDockLayoutManager::applyDockHeaderRules(ads::CDockWidget* dock)
    {
        if (!dock) return;

        // dock 主体滚动按需出现：内容减少后 Qt 立即回收滚动条占位，不预留空槽
        for (QScrollArea* scroll : dock->findChildren<QScrollArea*>())
        {
            // 只统一 dock 的通用滚动容器；业务树/表格/功能区的滚动策略不在这里改写
            if (scroll->objectName() != QStringLiteral("dockWidgetScrollArea"))
                continue;
            scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            scroll->setWidgetResizable(true);
        }

        if (auto* tab = dock->tabWidget())
        {
            // 完整标题始终保留；ADS 的 CElidingLabel 负责在空间不足时省略。
            // 不用 setFixedWidth，避免省略后把右侧关闭按钮挤出窗口头。
            tab->setElideMode(Qt::ElideRight);
            tab->setToolTip(dock->windowTitle());
            tab->setFixedHeight(UiMetrics::windowHeaderHeight());
            tab->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            tab->setProperty("mxDockHeaderRules", true);
            updateTabBackground(tab);
            for (QLabel* label : tab->findChildren<QLabel*>())
                label->setToolTip(dock->windowTitle());
            if (auto* close = tab->findChild<QAbstractButton*>(QStringLiteral("tabCloseButton")))
            {
                close->setFixedSize(UiMetrics::windowHeaderControl(), UiMetrics::windowHeaderControl());
                close->setIconSize(QSize(qMax(UiMetrics::baseH() / 2 + 3, UiMetrics::baseG() + 10),
                                         qMax(UiMetrics::baseH() / 2 + 3, UiMetrics::baseG() + 10)));
                close->setToolTip(QStringLiteral("关闭"));
            }
            if (!tab->property("mxDockTitleConnected").toBool())
            {
                tab->setProperty("mxDockTitleConnected", true);
                connect(dock, &ads::CDockWidget::titleChanged, tab, [tab](const QString& title) {
                    tab->setToolTip(title);
                    for (QLabel* label : tab->findChildren<QLabel*>())
                        label->setToolTip(title);
                });
                connect(tab, &ads::CDockWidgetTab::activeTabChanged, this, [this, tab]() {
                    updateTabBackground(tab);
                });
            }
        }

        if (dock->dockAreaWidget())
            applyAreaHeaderRules(dock->dockAreaWidget());
    }

    void mDockLayoutManager::applyAreaHeaderRules(ads::CDockAreaWidget* area)
    {
        if (!area || !area->titleBar()) return;
        auto* titleBar = area->titleBar();
        titleBar->setFixedHeight(UiMetrics::windowHeaderHeight());
        titleBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        if (auto* lay = titleBar->layout())
        {
            // QSS 与页签均不加 padding：按当前界面要求标题左缘不留额外空白。
            lay->setSpacing(0); // 动作按钮间距由其左边距单独控制，不影响页签区
            lay->setContentsMargins(0, 0, 0, 0);
        }

        const int buttonEdge = UiMetrics::windowHeaderControl();
        const int iconEdge = qMax(UiMetrics::baseH() / 2 + 3, UiMetrics::baseG() + 10);
        // 只调整窗口头动作按钮；页签溢出滚动箭头也挂在 titleBar 下，
        // 但不能被拉成 24px，否则会把标题和动作区向右撑出一段空白。
        for (ads::TitleBarButton id : { ads::TitleBarButtonTabsMenu,
                                       ads::TitleBarButtonUndock,
                                       ads::TitleBarButtonClose,
                                       ads::TitleBarButtonAutoHide,
                                       ads::TitleBarButtonMinimize })
        {
            if (auto* button = titleBar->button(id))
            {
                button->setFixedSize(buttonEdge, buttonEdge);
                button->setIconSize(QSize(iconEdge, iconEdge));
            }
        }
        if (auto* label = titleBar->autoHideTitleLabel())
        {
            label->setElideMode(Qt::ElideRight);
            label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        }

        if (!titleBar->property("mxDockHeaderSignals").toBool())
        {
            titleBar->setProperty("mxDockHeaderSignals", true);
            // 页签显隐、拖动和恢复布局后，ADS 会更新可见按钮；届时重发尺寸。
            connect(area, &ads::CDockAreaWidget::currentChanged, this, [this, area]() {
                applyAreaHeaderRules(area);
            });
        }
    }

    void mDockLayoutManager::applyHeaderRulesToChildren()
    {
        if (!_dockManager) return;
        for (ads::CDockAreaWidget* area : _dockManager->findChildren<ads::CDockAreaWidget*>())
            applyAreaHeaderRules(area);
        for (ads::CDockWidget* dock : _dockManager->findChildren<ads::CDockWidget*>())
            applyDockHeaderRules(dock);
    }

    void mDockLayoutManager::updateTabBackground(ads::CDockWidgetTab* tab)
    {
        if (!tab) return;

        // 激活页签与 dock 主体同为 surface 底；标题仍用 text 色，不恢复蓝色。
        // 非激活页签保留 header 底和 label 字色以区分可切换项。
        const QColor fg = tab->isActiveTab() ? UiPalette::text() : UiPalette::label();
        const QColor bg = tab->isActiveTab() ? UiPalette::surface() : UiPalette::header();
        tab->setStyleSheet(QStringLiteral(
            "ads--CDockWidgetTab { background:%1; color:%2; border:none; }"
            "ads--CDockWidgetTab QLabel { background:transparent; color:%2; font-weight:400; }")
            .arg(bg.name(), fg.name()));

        // 新样式表写入后强制重抛光并重绘：页签内联样式表与 manager 样式表的
        // activeTab 属性选择器共存时，QStyleSheetStyle 可能沿用旧抛光结果不重绘
        //（表现为切主题后页签颜色不变），与菜单主题刷新（ThemeManager::applyMetrics）同手法兜底
        if (QStyle* st = tab->style())
        {
            st->unpolish(tab);
            st->polish(tab);
        }
        for (QLabel* label : tab->findChildren<QLabel*>())
        {
            if (QStyle* lst = label->style())
            {
                lst->unpolish(label);
                lst->polish(label);
            }
            label->update();
        }
        tab->update();
    }

    ads::CDockWidget* mDockLayoutManager::registerDock(QWidget* w, QString titleName, QString keyName, const QColor& titleBarColor)
    {
        m_dockNameList.append(keyName);
        m_docks[keyName] = _dockManager->createDockWidget(titleName);
        m_docks[keyName]->setWidget(w);
        m_docks[keyName]->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromDockWidgetMinimumSize);
        m_docks[keyName]->setMinimumSize(100, 100);
        m_dockTitleBarColors[keyName] = titleBarColor;

        if (!keyName.contains("PostWidow"))//不是后处理中的停靠窗口，htx添加判断，防止设置之后，导致绘制的曲线出现问题
        {
            // 底色取 surface 语义色（规则 0.2），切主题后由 applyAll → applyTreeTheme 链重发
            m_docks[keyName]->setStyleSheet(QStringLiteral(
                "background-color: %1;").arg(MxFunctions::UiPalette::surface().name()));
        }
        applyDockHeaderRules(m_docks[keyName]);
        if (keyName == "Info")
        {
            // 规则 2：参数面板标准建议宽 320px、最小 280px（标准档 panel−40）。
            // 只设置建议宽和最小宽，不用 setFixedWidth，保留用户拖宽能力。
            auto* infoDock = m_docks[keyName];
            const int suggestedWidth = 12 * UiMetrics::baseH()
                + 8 * qMax(0, 6 - UiMetrics::baseG()) + 8;
            infoDock->setMinimumSize(suggestedWidth - 40, 150);
            infoDock->resize(qMax(infoDock->width(), suggestedWidth), infoDock->height());
        }
        if (keyName == "NVHToolSet")
        {
            // NVH工具集窗口页：最小宽 280（参数面板家族标准最小宽——右分栏落点下
            // 挡 ADS 分栏压缩，面板本体另有最小宽 265）
            m_docks[keyName]->setMinimumSize(280, 150);
        }
        return m_docks[keyName];
    }

    void mDockLayoutManager::addDockToBottom(QWidget* w, QString titleName, QString keyName)
    {
        // 1. 注册 dock（若已存在则复用）
        ads::CDockWidget* dock = nullptr;
        if (m_docks.contains(keyName))
        {
            dock = m_docks[keyName];
            dock->setWidget(w);  // 更新 widget
        }
        else
        {
            dock = registerDock(w, titleName, keyName);
        }
        if (!dock)
            return;

        // 2. 若 dock 已存在其它区域，先移除，防止被 ADS 误判位置
        if (dock->dockAreaWidget())
        {
            _dockManager->removeDockWidget(dock);
        }

        // 3. 查找所有 DockArea 中全局坐标最靠下的一个
        ads::CDockAreaWidget* bottomArea = nullptr;
        int maxCenterY = -1;

        int areaCount = _dockManager->dockAreaCount();
        for (int i = 0; i < areaCount; ++i)
        {
            auto* area = _dockManager->dockArea(i);
            if (!area)
                continue;

            // 跳过中心区域
            if (m_center && area == m_center->dockAreaWidget())
                continue;

            // 用 area 的中心点全局 Y 坐标来判断"靠下"程度
            int centerY = area->mapToGlobal(area->rect().center()).y();
            if (centerY > maxCenterY)
            {
                maxCenterY = centerY;
                bottomArea = area;
            }
        }

        // 4. 添加到底部区域（存在则加入 Tab，不存在则新建底部区域）
        if (bottomArea)
        {
            _dockManager->addDockWidgetTabToArea(dock, bottomArea);
        }
        else
        {
            _dockManager->addDockWidget(ads::BottomDockWidgetArea, dock);
        }

        // 5. 激活显示
        activateDock(keyName);
    }

    void mDockLayoutManager::updateTreeWindows(MxFunctions::ViewportType type)
    {
        //三棵树同一时刻只存在一棵：先把目标树放入当前树的标签位并显示，再隐藏其余，
        //先显示后隐藏可保证目标树精确占据原树（如前处理树）的位置
        const QString targetKey = treeKeyForType(type);
        showTreeDock(targetKey);
        for (const QString& k : { QString("Tree"), QString("PostTree"), QString("CurveTree") })
        {
            if (k != targetKey)
            {
                setDockVisible(k, false);
            }
        }
    }

    QString mDockLayoutManager::treeKeyForType(MxFunctions::ViewportType type) const
    {
        switch (type)
        {
        case MxFunctions::ViewportPost3D:
            return "PostTree";
        case MxFunctions::ViewportCurve:
            return "CurveTree";
        default:
            return "Tree";
        }
    }

    void mDockLayoutManager::showTreeDock(const QString& key)
    {
        if (!_dockManager)
        {
            return;
        }
        auto dock = m_docks.value(key, nullptr);
        if (!dock)
        {
            return;
        }

        //树标签位：任一当前打开的树dock所在区域（视图树常开时即其所在标签组）
        ads::CDockAreaWidget* area = nullptr;
        for (const QString& k : { QString("Tree"), QString("PostTree"), QString("CurveTree"), QString("ViewTree") })
        {
            auto d = m_docks.value(k, nullptr);
            if (d && k != key && !d->isClosed() && d->dockAreaWidget())
            {
                area = d->dockAreaWidget();
                break;
            }
        }

        if (dock->isClosed())
        {
            if (area)
            {
                //关闭态重开：显式放回树标签位，避免ADS按历史位置落到别处
                _dockManager->addDockWidgetTabToArea(dock, area);
            }
            else
            {
                _dockManager->addDockWidgetTab(ads::LeftDockWidgetArea, dock);
            }
            dock->toggleView(true);
            dock->raise();
        }
        //已打开时不做任何搬动/置前：用户手动浮动或另行停靠的树尊重其排布，
        //只有"自动隐藏后重开"的树才需要归位到树标签位
    }

    void mDockLayoutManager::showTreeDockForType(MxFunctions::ViewportType type)
    {
        const QString key = treeKeyForType(type);
        auto dock = m_docks.value(key, nullptr);
        if (!dock || !_dockManager)
        {
            return;
        }
        //显隐互斥统一收口于此：跳转与视口/菜单切换共用 updateTreeWindows，
        //任何路径展示目标树都会隐藏其余两棵树，"三棵树同一时刻只存在一棵"
        //不再依赖调用方选择入口（否则带外切树后同ID视口点击被去重早退，
        //互斥隐藏永不执行，出现模型树/曲线树并存）
        updateTreeWindows(type);//目标树关闭态：归位打开（内部已置前）
        if (!dock->isClosed())
        {
            dock->raise();//打开态：显式跳转时置前为当前标签
        }
    }

    void mDockLayoutManager::setDockVisible(const QString& key, bool visible)
    {
        if (!_dockManager)
        {
            return;
        }
        auto dock = m_docks.value(key, nullptr);
        if (!dock)
        {
            return;
        }
        if (visible)
        {
            if (dock->isClosed())
            {
                dock->toggleView(true);
                //树窗口为同区标签页：仅在从关闭状态重新打开时置为当前标签，
                //避免每次视口切换都把标签从用户正在查看的页面抢走
                dock->raise();
            }
        }
        else
        {
            if (!dock->isClosed())
            {
                dock->toggleView(false);
            }
        }
    }

    QMenu* mDockLayoutManager::createLayoutMenuButton()
    {
        QMenu* layoutMenu = new mUiMenu("布局管理", m_mainWin);
        for(auto key : m_dockNameList)
        {
            //NVH工具集不进布局管理菜单：唯一入口=曲线页按钮（防菜单绕过唤醒门控，
            //add-nvh-toolset-window-page 2026-11-16 裁定）
            if (key == "NVHToolSet")
                continue;
            ads::CDockWidget* dock = m_docks.value(key); // 获取 CDockWidget
            if (!dock)
                continue;
            QAction* action = dock->toggleViewAction();
            layoutMenu->addAction(action);
            m_actions[key] = action;
        }
        //三棵树同一时刻只存在一棵：菜单弹出时仅显示当前视口类型对应的树（视图树不在过滤范围，常显），
        //同时把各动作勾选态与 dock 实际开关状态同步（关闭按钮/自动切换不会主动刷新菜单勾选）
        connect(layoutMenu, &QMenu::aboutToShow, this, [this]() {
            QString curTreeKey;
            auto priv = VMManager ? VMManager->getCurrentViewportPrivate() : nullptr;
            if (priv)
            {
                curTreeKey = treeKeyForType(priv->getViewportType());
            }
            for (auto it = m_actions.begin(); it != m_actions.end(); ++it)
            {
                const QString& k = it.key();
                const bool isTreeKey = (k == "Tree" || k == "PostTree" || k == "CurveTree");
                it.value()->setVisible(!isTreeKey || k == curTreeKey);

                auto dock = m_docks.value(k, nullptr);
                if (dock)
                {
                    //阻断信号：setChecked 会触发 toggled -> toggleView，同态调用也是多余开销
                    it.value()->blockSignals(true);
                    it.value()->setChecked(!dock->isClosed());
                    it.value()->blockSignals(false);
                }
            }
            });
        // 菜单样式统一由 ThemeManager::applyMenuStyle（容器）+ mUiStyle（行内容/勾选框）下发，
        // 不再局部 setStyleSheet（旧样式表会把勾选指示器压成无边框凹槽，观感异常）
        return layoutMenu;
    }

    void mDockLayoutManager::saveLayout()
    {
        auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        QSettings Settings(path, QSettings::IniFormat);
        Settings.setValue("mainWindow/State", m_mainWin->saveState());
        Settings.setValue("mainWindow/DockingState", _dockManager->saveState());
        //中心窗口（mCenterMainWindow）的工具栏状态：主窗口 saveState 不递归子窗口，
        //视图工具栏/视图显示栏的位置需单独保存
        if (auto* center = MechCommon::centerMainWindowProvider() ? MechCommon::centerMainWindowProvider()() : nullptr)
        {
            Settings.setValue("centerWindow/State", center->saveState());
        }
    }

    bool mDockLayoutManager::restoreLayout()
    {
        for (auto w : m_docks)
        {
            _dockManager->addDockWidgetTab(ads::LeftDockWidgetArea, w);
            //w->closeDockWidget();
        }
        //applyPreset1();
        auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
        QSettings Settings(path, QSettings::IniFormat);
        if (!Settings.contains("mainWindow/State"))
        {
            applyPreset1();
        }
        m_mainWin->restoreState(Settings.value("mainWindow/State").toByteArray());
        bool ok = _dockManager->restoreState(Settings.value("mainWindow/DockingState").toByteArray());
        //anim-tab-v13 老布局防御（v1.4 动画工具栏 QToolBar 化）：动画工具已改挂中心窗口
        //QToolBar（animDisplayBar，animToolBarProvider 通道），ADS"动画播放"页签随形态
        //退役——旧版 saveState 落盘的 DockingState 可能残留该 dock 定义，restoreState 重建
        //其占位区域会出现"页签+工具栏"双入口；恢复后按名清理（幂等，无残留时零动作）
        if (ads::CDockWidget* legacyAnimDock = _dockManager->findDockWidget(QStringLiteral("动画播放")))
            _dockManager->removeDockWidget(legacyAnimDock);
        //恢复中心窗口的工具栏状态（视图工具栏/视图显示栏的位置）
        if (auto* center = MechCommon::centerMainWindowProvider() ? MechCommon::centerMainWindowProvider()() : nullptr)
        {
            if (Settings.contains("centerWindow/State"))
            {
                center->restoreState(Settings.value("centerWindow/State").toByteArray());
                //通知中心窗口：布局已恢复，showEvent 不要再钉正工具栏位置。
                //经 provider 解耦，GUIFoundation 不直接依赖 mCenterMainWindow 类型，
                //改用动态属性标记（mCenterMainWindow::showEvent 读取该属性）
                center->setProperty("skipToolbarPinning", true);
                //anim-tab-v13（T-2/J-02）：动画工具栏显隐归面板上下文驱动（前处理隐藏/
                //后处理常显）——saveState 落盘含工具栏显隐，上一会话后处理期保存的"可见"
                //会被 restoreState 写回，前处理启动即见常显工具栏=泄漏；恢复后强制回
                //隐藏初值，进入后处理由面板 slot_viewportContextChanged 再行驱动
                if (MechCommon::animToolBarProvider())
                {
                    if (QToolBar* animBar = MechCommon::animToolBarProvider()())
                        animBar->setVisible(false);
                }
            }
        }
        //强制 dock 区域重新布局一次，避免宽度为 0
        for (auto* d : m_docks) {
            //if (d->isVisible())
                d->widget()->adjustSize();
        }

        //新增dock（如曲线树）不在历史布局中，恢复后可能处于浮动/未停靠状态：
        //默认停靠到模型树所在区域（tab位于模型树右侧）；用户自己保存过位置的不受影响
        auto treeDock = m_docks.value("Tree");
        auto curveTreeDock = m_docks.value("CurveTree");
        if (treeDock && curveTreeDock && treeDock->dockAreaWidget())
        {
            bool needDefaultPlace = curveTreeDock->isFloating() || (curveTreeDock->dockAreaWidget() == nullptr);
            if (needDefaultPlace)
                _dockManager->addDockWidgetTabToArea(curveTreeDock, treeDock->dockAreaWidget());
        }
        //NVH工具集（后注册的新 dock）不在历史布局中时同理归位：「函数与曲线」窗口页
        //所在区分栏右挂（与预设右分栏落点一致）；用户自己保存过位置的不受影响
        auto nvhToolSetDock = m_docks.value("NVHToolSet");
        auto curveWorkbenchDock = m_docks.value("Curve");
        if (nvhToolSetDock && curveWorkbenchDock && curveWorkbenchDock->dockAreaWidget())
        {
            bool nvhNeedDefaultPlace = nvhToolSetDock->isFloating() || (nvhToolSetDock->dockAreaWidget() == nullptr);
            if (nvhNeedDefaultPlace)
                _dockManager->addDockWidget(ads::RightDockWidgetArea, nvhToolSetDock, curveWorkbenchDock->dockAreaWidget());
        }
        //布局恢复后强制关闭（2026-11-16 裁定：唯一出现方式=曲线页按钮唤醒，存档
        //开着也不恢复为开；归位先行保证下次唤醒时落位正确）
        if (nvhToolSetDock)
            nvhToolSetDock->closeDockWidget();
        // restoreState 会重建区域和页签可见性，最后统一补发公共窗口头规则
        applyHeaderRulesToChildren();
        QTimer::singleShot(0, this, [this]() { applyHeaderRulesToChildren(); });
        return ok;
    }

    void mDockLayoutManager::resetDockNetwork()
    {
        //仅清理 ADS dock
        for (auto w : m_docks)
        {
            _dockManager->removeDockWidget(w);
        }
        m_resettingLayout = false;
    }

    
    void mDockLayoutManager::applyPreset1()
    {
        //auto centerArea = m_center->dockAreaWidget();
        //int width = m_center->width();
        //int height = m_center->height();

        this->resetDockNetwork();//移除窗口会导致center大小变化

        auto tree = m_docks["Tree"];
        auto NVHTree = m_docks["CurveTree"];
        auto viewTree = m_docks["ViewTree"];
        auto postTree = m_docks["PostTree"];
        auto info = m_docks["Info"];
        auto section = m_docks["Section"];
        auto select = m_docks["Select"];
        auto table = m_docks["Table"];
        auto curve = m_docks["Curve"];
        auto output = m_docks["Output"];
        auto _case = m_docks["Case"];
        //视频 dock 已取消（2026-11-17 用户裁定）：视频经叠加/背景/独立视口显示
        auto nvhToolSet = m_docks["NVHToolSet"];    //NVH工具集面板（模态贡献量分析）

        // 底部区域：选择信息 + 表格（tab）+ 曲线 + 输出
        auto bottomDock = _dockManager->addDockWidgetTab(BottomDockWidgetArea, select);
        _dockManager->addDockWidgetTabToArea(table, bottomDock);
        _dockManager->addDockWidgetTabToArea(curve, bottomDock);
        _dockManager->addDockWidgetTabToArea(output, bottomDock);
        //NVH工具集窗口页：底部标签组右侧并排分栏（曲线图表与工具同屏对照，
        //add-nvh-toolset-window-page 2026-11-16 裁定），初始分栏宽真机可拖
        _dockManager->addDockWidget(RightDockWidgetArea, nvhToolSet, bottomDock);
        auto bottomSizes = _dockManager->splitterSizes(bottomDock);
        bottomSizes[0] = 720;
        bottomSizes[1] = 300;
        _dockManager->setSplitterSizes(bottomDock, bottomSizes);

        // 左侧区域：视图树/前处理树/后处理树/NVH 同一个dock（标签页，最左侧），
        // 参数（Info）在该dock右边，剖面（Section）与参数同区标签
        auto treeDock = _dockManager->addDockWidgetTab(LeftDockWidgetArea, viewTree);
        _dockManager->addDockWidgetTabToArea(tree, treeDock);
        _dockManager->addDockWidgetTabToArea(postTree, treeDock);
        _dockManager->addDockWidgetTabToArea(NVHTree, treeDock);
        auto infoDock = _dockManager->addDockWidget(RightDockWidgetArea, info, treeDock);
        _dockManager->addDockWidgetTabToArea(section, infoDock);

        // 左边区域， 案例
         _dockManager->addDockWidget(RightDockWidgetArea, _case);


        // 默认隐藏部分
        section->closeDockWidget();
        select->closeDockWidget();
        table->closeDockWidget();
        curve->closeDockWidget();
        output->closeDockWidget();
        _case->closeDockWidget();
        //初始仅有前处理视口：后处理/NVH树窗口默认隐藏，随视口类型切换显示
        postTree->closeDockWidget();
        NVHTree->closeDockWidget();
        //NVH工具集默认关闭：唯一出现方式=曲线页按钮唤醒（add-nvh-toolset-window-page）
        nvhToolSet->closeDockWidget();

        //由于模型树标题总是消失
        tree->closeDockWidget();
        this->activateDock("Tree");

        //树dock与参数dock的横向大小
        auto leftSizes = _dockManager->splitterSizes(treeDock);
        if (leftSizes.size() >= 2)
        {
            leftSizes[0] = 280;
            leftSizes[1] = 250;
            _dockManager->setSplitterSizes(treeDock, leftSizes);
        }

        //QTimer::singleShot(0, this, [this, width, height]() {
            //m_center->resize(width, height);
            //});

        //重置布局必须显式钉正工具栏，不受启动缓存保护限制。
        if (auto* center = MechCommon::centerMainWindowProvider() ? MechCommon::centerMainWindowProvider()() : nullptr)
        {
            QMetaObject::invokeMethod(center, "resetViewToolBarLayout", Qt::DirectConnection);
        }


        //设置纵向布局大小
        //auto bottomSizes = _dockManager->splitterSizes(bottomDock);
        //bottomSizes[0] = 220;
        //bottomSizes[1] = 250;
    }

    void mDockLayoutManager::applyPreset2()
    {
        //auto centerArea = m_center->dockAreaWidget();
        //int width = m_center->width();
        //int height = m_center->height();

        resetDockNetwork();

        auto tree = m_docks["Tree"];
        auto info = m_docks["Info"];
        auto NVHTree = m_docks["CurveTree"];
        auto viewTree = m_docks["ViewTree"];
        auto postTree = m_docks["PostTree"];
        auto section = m_docks["Section"];
        auto select = m_docks["Select"];
        auto table = m_docks["Table"];
        auto curve = m_docks["Curve"];
        auto output = m_docks["Output"];
        auto _case = m_docks["Case"];
        //视频 dock 已取消（2026-11-17 用户裁定）：视频经叠加/背景/独立视口显示

        auto bottom = _dockManager->addDockWidgetTab(ads::BottomDockWidgetArea, select);
        _dockManager->addDockWidgetTabToArea(table, bottom);
        _dockManager->addDockWidgetTabToArea(curve, bottom);
        _dockManager->addDockWidgetTabToArea(output, bottom);
        //NVH工具集窗口页：底部标签组右侧并排分栏（曲线图表与工具同屏对照，
        //add-nvh-toolset-window-page 2026-11-16 裁定），初始分栏宽真机可拖
        auto nvhToolSet = m_docks["NVHToolSet"];
        _dockManager->addDockWidget(ads::RightDockWidgetArea, nvhToolSet, bottom);
        auto nvhBottomSizes = _dockManager->splitterSizes(bottom);
        nvhBottomSizes[0] = 720;
        nvhBottomSizes[1] = 300;
        _dockManager->setSplitterSizes(bottom, nvhBottomSizes);

        // 左侧区域：视图树/前处理树/后处理树/NVH 同一个dock（标签页，最左侧），
        // 参数（Info）在该视图dock的下方，剖面（Section）与参数同区标签
        auto treeDock = _dockManager->addDockWidgetTab(ads::LeftDockWidgetArea, viewTree);
        _dockManager->addDockWidgetTabToArea(tree, treeDock);
        _dockManager->addDockWidgetTabToArea(postTree, treeDock);
        _dockManager->addDockWidgetTabToArea(NVHTree, treeDock);
        auto infoDock = _dockManager->addDockWidget(ads::BottomDockWidgetArea, info, treeDock);
        _dockManager->addDockWidgetTabToArea(section, infoDock);

        _dockManager->addDockWidget(RightDockWidgetArea, _case);

        // 默认隐藏部分
        section->closeDockWidget();
        select->closeDockWidget();
        table->closeDockWidget();
        curve->closeDockWidget();
        output->closeDockWidget();
        _case->closeDockWidget();
        //初始仅有前处理视口：后处理/NVH树窗口默认隐藏，随视口类型切换显示
        postTree->closeDockWidget();
        NVHTree->closeDockWidget();
        //NVH工具集默认关闭：唯一出现方式=曲线页按钮唤醒（add-nvh-toolset-window-page）
        nvhToolSet->closeDockWidget();

        //由于模型树标题总是消失
        tree->closeDockWidget();
        this->activateDock("Tree");

        //参数栏占据左列下半部（树dock在上、参数dock在下各占一半）。
        //addDockWidget(BottomDockWidgetArea, info, treeDock) 的返回即参数区域的容器
        if (infoDock)
        {
            auto leftSizes = _dockManager->splitterSizes(infoDock);
            if (leftSizes.size() >= 2)
            {
                leftSizes[0] = 1;
                leftSizes[1] = 1;
                _dockManager->setSplitterSizes(infoDock, leftSizes);
            }
        }

        //m_center->resize(width, height);

        //重置布局必须显式钉正工具栏，不受启动缓存保护限制。
        if (auto* center = MechCommon::centerMainWindowProvider() ? MechCommon::centerMainWindowProvider()() : nullptr)
        {
            QMetaObject::invokeMethod(center, "resetViewToolBarLayout", Qt::DirectConnection);
        }

        // 设置 splitter 比例(还是无法获取到左侧窗口与中心视图的尺寸)
        /*
        QTimer::singleShot(0, _dockManager, [this, leftTop, leftBottom, bottom]() {

            // 左列 vs 中心（通过左列的 parent splitter）
            auto leftSplitter = leftTop->parentWidget();
            if (auto splitter = qobject_cast<QSplitter*>(leftSplitter)) {
                QList<int> sizes = splitter->sizes();
                if (sizes.size() >= 2) {
                    sizes[0] = 220;  // 左列宽度
                    sizes[1] = 600;  // 中间宽度
                    splitter->setSizes(sizes);
                }
            }

            // 左列纵向：tree vs section/info
            auto verticalSplitter = leftBottom->parentWidget();
            if (auto splitter = qobject_cast<QSplitter*>(verticalSplitter)) {
                QList<int> sizes = splitter->sizes();
                if (sizes.size() >= 2) {
                    sizes[0] = 300;  // tree
                    sizes[1] = 200;  // section/info
                    splitter->setSizes(sizes);
                }
            }

            // 底部 splitter：中心 vs bottom
            auto bottomSplitter = bottom->parentWidget();
            if (auto splitter = qobject_cast<QSplitter*>(bottomSplitter)) {
                QList<int> sizes = splitter->sizes();
                if (sizes.size() >= 2) {
                    sizes[0] = 600;  // 中心
                    sizes[1] = 200;  // 底部
                    splitter->setSizes(sizes);
                }
            }
            });
            */
    }
    
}
