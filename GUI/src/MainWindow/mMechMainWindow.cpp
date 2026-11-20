#include "mMechMainWindow.h"
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
#include "ui_mMechMainWindow.h"
#include "mFramelessWindowHelper.h"

//Qt
#include <QStatusBar>
#include <QLibrary>
#include <QDebug>
#include <QByteArray>
#include <QTextCodec>
#include <QSettings>
#include <QWindow>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QApplication>
#include <QEventLoop>
#include <DockWidgetTab.h>
#include <QtGui/qtguiglobal.h>
#include "mGlobalSignals.h"
#include "mWidgetOperation.h"
#include "DockManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"
#include "ThemeManager.h"
#include "mWindowTitleBar.h"
#include "DockAreaWidget.h"
#include "mMenuWidget.h"
#include "mInfoWindow.h"
#include "mCurveWindow.h"
#include "mTableDataWindow.h"
#include "mBrowserWindow.h"
#include "CurveTree/mCurveBrowserWindow.h"
#include "CurveTree/mCurveTreeView.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"
#include "CurveView/mCurveViewportFactory.h"
#include "CurveView/mCurveChartView.h"
#include "ViewportContainer.h"
#include "ViewportWidget.h"
#include "CurvePages/mCurvePropPageManager.h"
#include "CurvePages/mCurvePropPageBase.h"
#include "mMechGlobalSignals.h"
#include "mOpenCurveDlg.h"
#include "mCreate3DPlotDlg.h"
#include "mViewTreeFunction.h"
#include "mViewBrowserWindow.h"
#include "mPostDockProvider.h"
#include "mCenterMainWindow.h"
#include "mViewportManager.h"
#include "mSectionWindow.h"
#include "mDisplayMenuFunction.h"
#include "mSelectInfoWindow.h"
#include "mOutputInfoWindow.h"
#include "mLanguageVarible.h"
#include "mStatusBar.h"
#include "mDockLayoutManager.h"
#include "mOConfigurationFile.h"
#include "mPythonAgent.h"

//AI


//单位制
#include "mUnitSystemManager.h"
//案例库
#include "mCaseWindow.h"

//材料库
#include "mMaterialWindow.h"
//案例库
#include "mCaseWindow.h"

//快捷键
#include "mShortcutManager.h"
#include "mShortcutActionList.h"

//几何编辑信号
//#include "mGeoGlobalSignals.h" //【OCC摘除·S10】BasicData 旧栈全局信号头（拉 mGeoDataModelSet→OCC 链）
#include "mModelData.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mGeoSet.h"
#include "mMixSet.h"

//python命令注册
#include "mPythonCommand.h"

//求解器
#include "mSolveTaskCenter.h"
// 批量剖分 CLI
#include "mMeshBatchCLI.h"
#include "BatchCLIConfig.h"
#include "mAppConfiguration.h"
#include "mIConfigurationFile.h"

#ifdef Q_OS_WIN
// windows.h 仅用于 nativeEvent 转发声明；Win32 消息解析全在 mFramelessWindowHelper.cpp
//（该文件先包含 windows.h 规避 Qt5 qwindowdefs_win.h 的 tagMSG 冲突）
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

using namespace MxFunctions;
using namespace ads;
//曲线模块框架类（PostCurve库）
using namespace PostCurve;
namespace MechMainWindow
{

    IRibbonHost* mMechMainWindow::getRibbonHost() const
    {
        return m_Menu_;
    }

    mMechMainWindow* mMechMainWindow::_instance = nullptr;

    mMechMainWindow* mMechMainWindow::getInstance()
    {
        return _instance;
    }

    mMechMainWindow::mMechMainWindow(QWidget* parent)
        : QMainWindow(parent)
    {
        _instance = this;

        // 自绘标题栏：必须在 setupUi/任何子控件创建与窗口 show 之前完成
        //（Frameless 标志决定窗口句柄的非客户区形态，晚设会重建句柄）
        setupCustomTitleBar();
        _ui = new Ui::mMechMainWindow();
        _ui->setupUi(this);

        if (MAppConfiguration::mAppConfiguration::getInstance()->_softwareName.isEmpty())
        {
            //配置未指定软件名时按形态回退（多体产品为 MxSim.Motion；配置中
            //*MXSOFTWARENAME 非空时始终以配置为准，两形态共用同一配置文件）
            this->setWindowTitle(
                MechCommon::resolveRibbonForm() == MechCommon::MenuRibbonForm::Motion
                ? QString::fromUtf8("MxSim.Motion V1.0")
                : QString::fromUtf8("MxSim.Mechanical V3.0"));
        }
        else
        {
            this->setWindowTitle(MAppConfiguration::mAppConfiguration::getInstance()->_softwareName);
        }

		//中心视图
		_center = new mCenterMainWindow;
		_material = new mMaterialWindow(this);
		//this->setCentralWidget(_center);
		
		// uncomment the following line to enable focus highlighting of the dock
		// widget that has the focus
		CDockManager::setConfigFlag(CDockManager::FocusHighlighting, true);

        // uncomment if you would like to enable dock widget auto hiding
        CDockManager::setAutoHideConfigFlags({ CDockManager::DefaultAutoHideConfig });

        //布局管理器
        _dockManager = new CDockManager(this);
        mDockLayoutManager::getInstance()->setCDockManager(_dockManager, this);
        _dockManager->setDockWidgetToolBarStyle(Qt::ToolButtonIconOnly, ads::CDockWidget::StateFloating);
        mInfoWindow::getInstance()->setParent(this);
        //参数区宿主底色随主题：与折叠组/曲线窗口同一批 dock applier 链刷新
        static bool s_infoHostApplier = false;
        if (!s_infoHostApplier)
        {
            s_infoHostApplier = true;
            ThemeManager::addDockThemeExtraApplier(
                []() { mInfoWindow::getInstance()->applyHostTheme(); });
        }

        auto w = mDockLayoutManager::getInstance()->registerDock(mBrowserWindow::getInstance(), "模型", "Tree"); //w->setMinimumSize(200, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mViewBrowserWindow::getInstance(), "视图", "ViewTree");
        w = mDockLayoutManager::getInstance()->registerDock(MechCommon::postTreeDockProvider()(), "云图", "PostTree");//经提供者间接获取（mPostBrowserWindow迁移PostWindow后零改动）
        //PostCurve 曲线树：key=CurveTree，与 treeKeyForType/activateDock("CurveTree") 对齐
        w = mDockLayoutManager::getInstance()->registerDock(mCurveBrowserWindow::getInstance(), "曲线", "CurveTree");
        //NVH工具集面板（模态贡献量分析）：key=NVHToolSet，经提供者间接获取（PostWindow 侧自注册），
        //落底部条「函数与曲线」标签组右侧并排分栏（applyPreset1/2）；唯一入口="曲线"页-诊断组按钮
        //（门控=出现层单层：按钮只在曲线工作模式可见即可点，2026-11-16 二次裁定）
        w = mDockLayoutManager::getInstance()->registerDock(MechCommon::nvhToolSetDockProvider()(), "NVH工具集", "NVHToolSet");
        w = mDockLayoutManager::getInstance()->registerDock(mInfoWindow::getInstance(), "参数", "Info");//w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mSectionWindow::getInstance(), "剖面", "Section"); //w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mTableDataWindow::getInstance(), "表格数据", "Table");// w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mCurveWindow::getInstance(), "函数与曲线", "Curve"); w->setMinimumSizeHintMode(CDockWidget::MinimumSizeHintFromContent);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mSelectInfoWindow::getInstance(), "选择信息", "Select"); //w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住	
        w = mDockLayoutManager::getInstance()->registerDock(mOutputInfoWindow::getInstance(), "信息输出", "Output"); //w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        w = mDockLayoutManager::getInstance()->registerDock(mCaseWindow::getInstance(), "案例库", "Case"); //w->setMinimumSize(300, 150);//要设置最小尺寸，防止其他界面隐藏后字被挡住
        //视频 dock 已取消（用户裁定 2026-11-17）：视频显示形态=叠加/背景/独立视口
        //三选一（叠加显示组），mVideoWindow 本体不再常驻 dock——未勾形态时无画面
        //窗口，独立视口形态经格子承载；mDockLayoutManager 两套预设布局的 Video
        // 引用已同步移除

        //mDockLayoutManager::getInstance()->addDockWidget(Qt::RightDockWidgetArea, w,leftTop);

        //产出链落点解析回填（PostCurve 不依赖 MainWindow，经提供者间接查询）：
        //"当前视图页打开新窗口"按触发时刻的当前视图页定位落点曲线页；新建
        //曲线页时页名对齐取视图页名（同下方登记链建页分支）
        mCurveViewportFactory::getInstance()->setViewPageProvider([this]() -> int {
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                return viewTree->curPageID();
            return -1;
        });
        mCurveViewportFactory::getInstance()->setViewPageNameProvider([this](int viewPageId) -> QString {
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
            {
                for (const auto& p : viewTree->pageList())
                {
                    if (p.second == viewPageId)
                        return p.first;
                }
            }
            return QString();
        });

        //PostCurve模块不依赖MainWindow：图表视口卡片经信号请求，主窗口统一响应。
        //图表空卡片登记（懒加载第一段，卡片常驻）：占位内容挂进视口容器并按
        //viewPageId 归属到对应视图页；图表视图（mCurveChartView）由工厂物化后经
        //sig_requestAttachChartViewport 挂进同一卡片——卡片与视图生命周期解耦，
        //_chartCards/_cardChartOf 按 chartId/cardId 做键，未物化也能反查卡片
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestChartCard, this,
            [this](int chartId, const QString& chartName, int curvePageId, int viewPageId)
        {
            auto* factory = mCurveViewportFactory::getInstance();
            //批量建卡（deferCardActivation）时跳过 dock 激活：数百图导入不逐卡
            //刷新停靠布局，激活链由收尾 activatePageFirstChart 一次性完成
            if (!_restoringSession && !factory->deferCardActivation())
                mDockLayoutManager::getInstance()->activateDock("CurveTree");
            auto* container = VMManager->getViewportContainer();
            auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
            if (container == nullptr || viewTree == nullptr)
            {
                //容器/视图树不可用兜底：不建卡片（物化挂载时走独立窗口兜底）
                mCurveDocController::getInstance()->setPageViewPageId(curvePageId, viewPageId);
                return;
            }
            const int cardId = ChartViewportIdBase + (_chartCardSeq++);
            _chartCards.insert(chartId, cardId);
            _cardChartOf.insert(cardId, chartId);
            //占位内容（懒加载：切到该视口时才由工厂物化图表视图替换）
            auto* card = container->addViewport(cardId, createChartPlaceholder());
            if (card)
                card->setTitle(chartName);
            //建卡即隐（浮卡禁令）：卡片显隐唯一权威写入口是布局应用（applyLayout
            //呈现集收敛）——登记时刻不承担呈现职责，批量登记与收尾布局之间的
            //时序缺口不允许卡片以建卡初始几何浮于当前布局之上（2026-11-11 真机
            //缺陷）。交互路径随后 switchToPage+applyPageLayout 就地呈现；批量
            //（defer）路径由产出收尾 presentProducedCharts 一次性呈现；会话恢复
            //由恢复编排收尾布局统一呈现
            if (card)
                card->setVisible(false);
            factory->notifyChartCardRegistered(chartId, cardId);

            if (_restoringSession)
            {
                //会话恢复批量重建：页归属仍按 viewPageId 正常登记，仅抑制
                //激活dock/切页/激活卡片副作用（收尾由恢复编排统一执行）
                mCurveDocController::getInstance()->setPageViewPageId(curvePageId, viewPageId);
                return;
            }

            //目标视图页：-1=新建（命名走视图树统一的"视图页-N"自动规则）；
            //绑定后回填曲线页映射与页名（页对齐：两棵树显示同名，对应关系由viewPageId绑定）
            int targetPage = viewPageId;
            if (!viewTree->attachViewportToPage(targetPage, cardId, chartName))
            {
                targetPage = viewTree->createPageAndGetId();
                viewTree->attachViewportToPage(targetPage, cardId, chartName);
                //新建视图页场景：曲线页名回填为视图页名（创建宏内，随宏撤销）
                QString newPageName;
                for (const auto& p : viewTree->pageList())
                {
                    if (p.second == targetPage) { newPageName = p.first; break; }
                }
                if (!newPageName.isEmpty())
                    mCurveDocController::getInstance()->setPageName(curvePageId, newPageName);
            }
            mCurveDocController::getInstance()->setPageViewPageId(curvePageId, targetPage);

            //批量建卡（VTF/动刚度/NTF 导入）：不切页不激活，落点页由创建收尾的
            //activatePageFirstChart 统一切换——空卡片登记本身零渲染成本，
            //数百图导入不再产生逐卡切页/激活风暴
            if (factory->deferCardActivation())
            {
                //静默批量：卡片登记即隐藏（addViewport 默认可见+居中叠放，静默
                //场景新页布局未应用会悬空显示在当前页——切到所属页时
                //applyPageLayout→applyLayout 重新显示/摆格，NTF 收尾切页同路）
                if (card)
                    card->hide();
                return;
            }
            viewTree->switchToPage(targetPage);
            factory->materializeChart(chartId);
            container->setActiveViewport(cardId);
        });

        //图表视图物化完成：占位内容替换为视图（卡片/页登记不动）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestAttachChartViewport, this,
            [this](QWidget* chartView, int chartId)
        {
            const int cardId = _chartCards.value(chartId, -1);
            auto* container = VMManager->getViewportContainer();
            if (cardId < 0 || container == nullptr)
            {
                //容器不可用兜底：独立窗口（同旧 openChart 兜底语义）
                chartView->resize(800, 600);
                chartView->show();
                return;
            }
            if (auto* card = container->viewport(cardId))
            {
                //setContentWidget 只摘离旧内容不销毁：显式回收被孤儿化的旧占位
                QWidget* old = card->contentWidget();
                card->setContentWidget(chartView);
                if (old && old != chartView)
                    old->deleteLater();
                //懒加载补施显隐：kChartVisible 变更只在已物化视图上即时生效（工厂
                //属性分支），未物化期间的勾选状态在物化挂载时在此按文档统一补齐
                //（文档节点为唯一真相源，chart.visible 默认 true）。同时守卫布局
                //呈现集：卡片不在最近一次布局呈现集内（溢出隐藏/他页卡片）时不得
                //挂载即显示——否则新图卡片以建卡初始几何（容器居中 2/3）浮于当前
                //布局之上（2026-11-11 真机缺陷：NVH 显示链新图浮于旧图正中）；
                //该卡随其所在页的下一次布局应用/激活换入恢复呈现
                if (auto* doc = mCurveDocController::getInstance()->document())
                    card->setVisible(doc->nodeProperty(chartId, CurveProp::kChartVisible).toBool()
                        && container->isViewportLayoutShown(cardId));
            }
        });

        //图表显隐应用（kChartVisible 变更，工厂属性分支经信号代办——工厂访问
        //不到容器/呈现集，分层约束）：与上方挂载分支同口径——隐藏立即生效；
        //显示须在最近一次布局呈现集内，溢出隐藏/他页卡片不得翻可见（否则以
        //过期几何浮于当前布局之上，2026-11-17 布局切换挤压同根钉因），勾了
        //显示的格外卡片随其进入呈现集（激活换入/布局应用）时自然呈现
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestApplyChartVisibility, this,
            [this](int chartId, bool on)
        {
            const int cardId = _chartCards.value(chartId, -1);
            auto* container = VMManager->getViewportContainer();
            if (cardId < 0 || container == nullptr)
                return;
            if (auto* card = container->viewport(cardId))
                card->setVisible(on && container->isViewportLayoutShown(cardId));
        });

        //LRU/空闲回收卸载：视图从卡片摘下、占位内容回填——卡片/页登记/标题全部
        //保留（与"关闭"的区别：关闭连卡片一起删）。视图 deleteLater 后经 destroyed
        //联动清工厂登记表（LRU/计时已在 evictChart 发出前摘除）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestUnmountChartViewport, this,
            [this](int chartId)
        {
            const int cardId = _chartCards.value(chartId, -1);
            //QWidget 级取视图（类型无关：XY=mCurveChartView / 三维图=m3DChartView，
            //R28 入树；chartView() 的具体类型 cast 对三维视图会漏）
            QWidget* chartView = mCurveViewportFactory::getInstance()->chartViewWidget(chartId);
            auto* container = VMManager->getViewportContainer();
            if (cardId < 0 || container == nullptr || chartView == nullptr)
            {
                //无卡片（独立窗口兜底路径）：直接释放视图（同主路径：同步 delete
                //防批量宏撤销/重做循环内 deleteLater 积压）
                if (chartView)
                    delete chartView;
                return;
            }
            if (auto* card = container->viewport(cardId))
            {
                //setContentWidget 只摘离旧内容不销毁：旧视图由下方同步 delete 释放，
                //若旧内容非本视图（异常态的旧占位）也一并回收
                QWidget* old = card->contentWidget();
                card->setContentWidget(createChartPlaceholder());
                if (old && old != chartView)
                    old->deleteLater();
            }
            //同步 delete（非 deleteLater）：本信号仅由工厂定时器/容量回收发出，
            //调用栈上没有该视图，立即销毁安全；且批量宏的撤销/重做是同步循环，
            //事件循环不回归——deleteLater 会令全部被回收视图积压到循环结束
            //（数百图重做时内存无界反弹），必须就地释放
            delete chartView;
        });

        //关闭图表（携带 chartId——视图可能尚未物化）：从视图页移除登记节点、
        //移除其视口卡片并释放视图
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestCloseChartViewport, this,
            [this](int chartId)
        {
            const int cardId = _chartCards.take(chartId);
            _cardChartOf.remove(cardId);
            QWidget* chartView = mCurveViewportFactory::getInstance()->chartViewWidget(chartId);
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                viewTree->detachViewportNode(cardId);
            if (auto* container = VMManager->getViewportContainer())
                container->removeViewport(cardId);
            if (chartView)
                chartView->deleteLater();   //destroyed 联动工厂清 _chartViews
        });

        //关闭未物化图表（懒加载：无视图可释放，仅移除空卡片）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestCloseChartCard, this,
            [this](int chartId)
        {
            const int cardId = _chartCards.take(chartId);
            if (cardId < 0)
                return;
            _cardChartOf.remove(cardId);
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                viewTree->detachViewportNode(cardId);
            if (auto* container = VMManager->getViewportContainer())
                container->removeViewport(cardId);
        });

        //对比视频-独立视口（图表卡片同款"信号请求+主窗口代办"）：mVideoWindow
        //（PostWindow）不依赖 MainWindow，经全局信号请求建/关视口格子。内容=
        //mVideoWindow 单例本体（画布+控制条整体搬迁），dock 与格子互斥
        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::sig_requestVideoViewportOpen, this,
            [this]()
        {
            if (_videoViewportId >= 0)
                return;   //单实例幂等
            auto* container = VMManager->getViewportContainer();
            auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
            QWidget* videoWnd = (MechCommon::videoDockProvider() ? MechCommon::videoDockProvider()() : nullptr);
            if (container == nullptr || viewTree == nullptr || videoWnd == nullptr)
                return;
            //video dock 已取消：本体无父隐藏/旧格摘离态直达，addViewport 内部
            //setParent 接管；无 dock 摘取动作
            _videoViewportId = VideoViewportIdBase;
            if (auto* card = container->addViewport(_videoViewportId, videoWnd))
            {
                card->setTitle(QString::fromUtf8("视频"));
                //建卡即隐（浮卡禁令，同图表卡片）：呈现交由挂页+切页的布局应用
                card->setVisible(false);
            }
            //挂当前视图页（无可用页则新建），切页呈现并激活
            int targetPage = viewTree->curPageID();
            if (!viewTree->attachViewportToPage(targetPage, _videoViewportId, QString::fromUtf8("视频")))
            {
                targetPage = viewTree->createPageAndGetId();
                viewTree->attachViewportToPage(targetPage, _videoViewportId, QString::fromUtf8("视频"));
            }
            viewTree->switchToPage(targetPage);
            //记录建格前的云图视口（关格恢复其激活呈现），随后激活视频格
            if (auto* model = VMManager->getCurrentViewportModel())
            {
                if (QWidget* postSub = model->activatePostViewport())
                    if (auto* priv = model->getViewportPrivate(postSub))
                        _videoPrevCloudViewportId = priv->getViewportID();
            }
            container->setActiveViewport(_videoViewportId);
        });

        connect(MechCommon::mMechGlobalSignals::getInstance(), &MechCommon::mMechGlobalSignals::sig_requestVideoViewportClose, this,
            [this]() { closeVideoViewportCard(false); });

        //曲线树/视图树双击图表节点 -> 激活其视口卡片（先切到卡片所属页——卡片随页显隐；
        //单击仅选中不激活，替换呈现由双击显式发起；未物化卡片由工厂先物化再发本请求）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestActivateChartCard, this,
            [this](int cardId)
        {
            if (cardId < 0)
                return;
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
            {
                const int pageId = viewTree->pageOfViewport(cardId);
                if (pageId >= 0)
                    viewTree->switchToPage(pageId);
            }
            if (auto* container = VMManager->getViewportContainer())
            {
                //失步纠偏守卫（2026-11-12 真机钉因链）：容器布局态是树页布局的
                //应用副本，dock 显隐/换页/替换路径的时序可使其失步——此后激活
                //不在可见格的卡片会按容器陈旧布局类型替换满铺（单格 Maximized 化，
                //另一卡滞留原格叠在其下：双图表头/轴同时可见=串扰）。目标卡不在
                //容器呈现集时先按树页布局重放一次（树=布局唯一权威，与收口
                //sig_requestPresentCharts 同源），再走激活换位；在格激活零动作，
                //"激活信号路径不重排"裁定不变
                auto* syncTree = mViewBrowserWindow::getInstance()->getViewTree();
                if (syncTree != nullptr && !container->isViewportLayoutShown(cardId))
                    syncTree->refreshCurrentPageLayout();
                //切换前选中的视口=溢出页替换呈现目标（激活后容器内已是新卡片，先取）
                const int prevViewportId = container->activeViewportID();
                container->setActiveViewport(cardId);
                //溢出页"选中即替换呈现"：新卡片在未呈现位时与切换前选中视口交换格位。
                //替换只挂在本请求通道（双击）与视图树双击处：激活信号路径不重排，
                //挂载即激活的批量建图等来源不会打乱"图表依序落页"
                presentChartOnOverflowPage(cardId, prevViewportId);
            }
        });

        //曲线树选中页节点 -> 切换到其对应视图页（页对齐联动）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestSwitchViewPage, this,
            [this](int viewPageId)
        {
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                viewTree->switchToPage(viewPageId);
        });

        //产出链落位收口（chart-viewport-placement 单写者）：工厂批量产出完成请求。
        //切到落点视图页 → 按本次产出张数选最小格数布局（同格数优先条带横排=
        //左右并排，对齐需求稿 02）→ 呈现集=本次产出集合 → 一次 applyLayout
        //收敛：中间态零浮卡，旧图溢出隐藏（不删，树保留可经布局切换唤回）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestPresentCharts, this,
            [this](int viewPageId, const QList<int>& cardIds)
        {
            auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
            auto* container = VMManager->getViewportContainer();
            if (viewTree == nullptr || container == nullptr || viewPageId < 0 || cardIds.isEmpty())
                return;
            //最小格数布局（D2）：格数 ≥K 的最小者；同格数优先纵分（条带横排左右并排）
            const auto& table = viewLayoutTable();
            int layoutIdx = 0;
            int bestSlotCount = 1 << 30;
            for (int i = 0; i < table.size(); ++i)
            {
                //注意：局部名不可用 slots（Qt 宏，空展开会吃掉声明符）
                const int slotCount = viewLayoutSlotCount(table[i]);
                const bool sideBySide = !table[i].verticalBands;
                if (slotCount >= cardIds.size()
                    && (slotCount < bestSlotCount
                        || (slotCount == bestSlotCount && sideBySide)))
                {
                    layoutIdx = i;
                    bestSlotCount = slotCount;
                }
            }
            //布局选择回写曲线页 kLayout（与页面属性/打开曲线网格/视图显示栏同源的
            //第④写入口）：本次按产出张数选定的最小格数布局若不落回数据源属性，
            //产出建节点触发的排队同步 syncCurvePageLayout 会在事件循环里按旧
            //kLayout（单视口）把刚收敛的并排布局翻回满铺（2026-11-11 真机探针钉因）。
            //同值不写，防多余 sig_nodePropertyChanged 排队回环（对齐
            //mCenterMainWindow 布局按钮路径的守值写法）
            {
                auto* controller = mCurveDocController::getInstance();
                const int curvePageId = controller->curvePageIdForViewPage(viewPageId);
                if (curvePageId >= 0)
                {
                    auto* doc = controller->document();
                    if (doc->nodeProperty(curvePageId, CurveProp::kLayout).toInt() != layoutIdx)
                        controller->setNodeProperty(curvePageId, CurveProp::kLayout, layoutIdx);
                }
            }
            viewTree->switchToPage(viewPageId);
            viewTree->setCurrentPageLayout(layoutIdx);
            //呈现集=本次产出集合（按产出顺序占格：响应曲线首格）。setCurrentPageLayout
            //仅布局下标变更时清手动可见组，随后此处显式覆写为本次集合——两条路径
            //均收敛到唯一写入口 applyPageLayout
            viewTree->getModel()->setPageVisibleViewportIDs(viewPageId, cardIds);
            viewTree->refreshCurrentPageLayout();   //公开包装器=applyPageLayout(当前页)
        });

        //静默登记收口（registerChartsOnPage 转发；用户终态裁定：绘制结束不自动
        //显示——不占空格、不浮出、不动页布局，显示时机完全由用户自行调整页面
        //布局决定）：①呈现集冻结=目标页当前呈现视口且不含本次新卡——手动可见
        //组已有则以原组为准（本就不含新卡）；为空（未手动布局，节点序填格会随
        //后把新卡补进空格自动显示）则以"页内全部视口-新卡"落成手动组，锁住现状；
        //用户随后点"布局"（布局下标变更清手动组回到节点序填格）或树勾选+布局
        //即可让新卡显示。②曲线页 kLayout 回写=页当前布局：本批建节点触发的排队
        //同步 syncCurvePageLayout 会按曲线页 kLayout 重放页布局，新建曲线页默认
        //单格，不回写会把用户页布局翻掉。不切页、不重放、不呈现
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestRegisterChartsOnPage, this,
            [this](int viewPageId, const QList<int>& cardIds)
        {
            auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
            if (viewTree == nullptr || viewPageId < 0 || cardIds.isEmpty())
                return;
            const auto& table = viewLayoutTable();
            int layoutIdx = viewTree->getModel()->pageLayout(viewPageId);
            if (layoutIdx < 0 || layoutIdx >= table.size())
                layoutIdx = 0;
            {
                auto* controller = mCurveDocController::getInstance();
                const int curvePageId = controller->curvePageIdForViewPage(viewPageId);
                if (curvePageId >= 0)
                {
                    auto* doc = controller->document();
                    if (doc->nodeProperty(curvePageId, CurveProp::kLayout).toInt() != layoutIdx)
                        controller->setNodeProperty(curvePageId, CurveProp::kLayout, layoutIdx);
                }
            }
            //呈现集冻结：已有手动组保持（本就不含新卡）；为空时以"页内全部视口
            //-新卡"落成手动组——否则节点序填格会在任何一次布局重放时把新卡补进
            //空格（mxdb 导入页常为多格，2026-11-13 真机实测浮出钉因）
            QList<int> frozen = viewTree->getModel()->pageVisibleViewportIDs(viewPageId);
            if (frozen.isEmpty())
            {
                const QList<int> all = viewTree->getModel()->pageViewportIDs(viewPageId);
                for (int id : all)
                {
                    if (!cardIds.contains(id))
                        frozen.append(id);
                }
                if (!frozen.isEmpty())
                    viewTree->getModel()->setPageVisibleViewportIDs(viewPageId, frozen);
            }
        });

        //图表标题重算（图表增删改变兄弟 w 位置序/图表重命名后工厂重算）：
        //按 chartId 更新对应视口卡片标题，并同步宿主视图树节点显示名——卡片已作为
        //"视口项"列在视图树，只刷卡片会让树停留在登记时的旧名（图表增删改变 w 序/
        //重命名后漂移）；未物化图表的卡片标题同样生效
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestRetitleChartCard,
            this, [this](int chartId, const QString& newTitle)
        {
            const int cardId = _chartCards.value(chartId, -1);
            if (cardId < 0)
                return;
            if (auto* container = VMManager->getViewportContainer())
            {
                if (auto* card = container->viewport(cardId))
                    card->setTitle(newTitle);
            }
            if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                viewTree->getModel()->renameViewportNode(cardId, newTitle);
        });

        //视口卡片激活变化 -> 工厂当前图表回投（新视图架构替代旧 MDI onViewportActivated
        //的通道）：卡片ID属图表段（>=ChartViewportIdBase）时反查图表——未物化的先物化
        //（对齐 HyperViewer"切到该视口才创建图表"），再回投其视图；否则回投
        //nullptr（激活的是渲染视口——模型/云图等，工厂广播当前图表 -1）。
        //直连同步链：工厂建卡片即激活也会经此路广播 sig_currentChartChanged
        if (auto* chartCardContainer = VMManager->getViewportContainer())
        connect(chartCardContainer, &ViewportContainer::sig_activeViewportChanged,
            this, [this](int viewportID, int previousViewportID)
        {
            Q_UNUSED(previousViewportID);
            QWidget* chartView = nullptr;
            if (viewportID >= ChartViewportIdBase)
            {
                auto* factory = mCurveViewportFactory::getInstance();
                const int chartId = _cardChartOf.value(viewportID, -1);
                if (chartId > 0)
                {
                    //懒加载：切到未物化图表卡片时创建其视图（占位内容就地替换）
                    factory->materializeChart(chartId);
                    //QWidget 级取视图（类型无关：三维图 m3DChartView 经此同样
                    //参与激活回投与曲线上下文联动，R28 入树）
                    chartView = factory->chartViewWidget(chartId);
                }
            }
            mCurveViewportFactory::getInstance()->notifyChartViewportActivated(chartView);

            //图表卡片激活 -> 显式进入曲线上下文，对齐渲染视口激活的菜单/树联动：
            //卡片ID不在渲染视口注册表，slot_viewportActivated 反查为空，菜单/树各自的
            //slot_subWindowActivated 均在 getCurrentViewportPrivate() 判空处早退，渲染
            //视口驱动的自动联动对卡片不生效，须在此补齐（与视图树双击图表卡片节点的
            //补齐路径一致）。setWorkMode同模式幂等，卡片之间互切不打扰用户当前页签；
            //树按"三棵树同一时刻只存在一棵"规则切到曲线树（渲染视口激活时由
            //mTreeView::updateTreeSectionsForViewport 走同一入口）
            if (chartView != nullptr)
            {
                if (m_Menu_ != nullptr)
                    m_Menu_->setWorkMode(MenuWorkMode::Curve);
                mDockLayoutManager::getInstance()->updateTreeWindows(MxFunctions::ViewportCurve);

                //视图树选中补齐：卡片激活不触发 currentViewportChanged，视图树的
                //slot_currentViewportChanged（激活节点加粗+选中）不会执行，选中停留在
                //上一个渲染视口节点（如云图）。此处按卡片ID同步（卡片节点已由
                //attachViewportToPage 登记进视图树，与渲染视口节点同构）；后续渲染
                //视口激活时 setActiveViewportNode(渲染ID) 会自然取消卡片加粗，语义对称
                if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                    viewTree->syncActiveViewportNode(viewportID);

                //参数栏补齐：notifyChartViewportActivated 内 syncTreeSelectionForChart 的
                //同图守卫命中时（中枢已持有该图表内节点——离开曲线上下文操作云图后返回的
                //典型场景）跳过重选，sig_currentNodeChanged 不发，参数栏停留在云图等面板；
                //守卫未命中时 selectNode 级联已按图表节点切过参数栏，此处判重后跳过，
                //不产生二次卸载/重挂（避免打断属性页控件编辑态）
                const int chartId = mCurveViewportFactory::getInstance()->currentChartId();
                auto* controller = mCurveDocController::getInstance();
                const int curNode = controller->currentNodeId();
                if (chartId >= 0 && controller->document()->chartNodeId(curNode) == chartId)
                {
                    auto* curPage = qobject_cast<mCurvePropPageBase*>(mInfoWindow::getInstance()->currentMainWidget());
                    if (curPage == nullptr || curPage->nodeId() != curNode)
                        mInfoWindow::getInstance()->editCurveNode(curNode);
                }
            }
            else if (!QCoreApplication::closingDown())
            {
                //渲染视口激活（模型/云图等）= 离开曲线上下文：参数栏当前是曲线属性页时
                //复位之（曲线页与曲线节点绑定，在云图等视口下展示无意义），与上方卡片
                //激活时的参数栏补齐方向对称。其余面板（云图参数等）保持既有粘性行为，
                //不随视口激活切换。关门期保护：复位含 widget 卸载，析构阶段执行有崩溃
                //风险（同 editCurveNode 的 closingDown 判空）
                if (qobject_cast<mCurvePropPageBase*>(mInfoWindow::getInstance()->currentMainWidget()) != nullptr)
                    mInfoWindow::getInstance()->unEditInfoWindow();
            }
        });

        //用户点击/聚焦视口 -> 菜单模式显式跟随：激活变化经currentViewportChanged
        //已有联动，此处兜"点击的已是当前激活视口"（同ID被容器/管理器去重早退，
        //无激活信号，菜单停留旧模式）。m_Menu_晚于本连接创建，触发期判空
        if (auto* focusContainer = VMManager->getViewportContainer())
        connect(focusContainer, &ViewportContainer::sig_viewportUserFocused,
            this, [this](int viewportID)
        {
            if (m_Menu_ != nullptr)
                m_Menu_->slot_viewportUserFocused(viewportID);
        });

        //--- 曲线页视口布局互通（打开曲线对话框网格/页面属性"页面布局"/视图显示栏
        //布局按钮三方同源）：单一数据源=曲线页 kLayout 属性（下标与视图树
        //viewLayoutTable 一致）。节点增删/kLayout 变化 -> syncCurvePageLayout 做绑定
        //视图页的页内重排；页数量在创建期按格数分页定死，此处不增删页，格数多于
        //图表数时余格留空。队列连接：同步在事件循环中执行，保证工厂（直连）已
        //先行关闭/挂载卡片，视图树归属是最终状态
        {
            auto* controller = mCurveDocController::getInstance();
            auto syncByNode = [this](int nodeId)
            {
                auto* doc = mCurveDocController::getInstance()->document();
                const auto type = doc->nodeType(nodeId);
                if (type == PostCurve::CurveNodeType::Page)
                    syncCurvePageLayout(nodeId);
                else if (type == PostCurve::CurveNodeType::Chart)
                    syncCurvePageLayout(doc->parentNodeId(nodeId));
            };
            connect(controller, &mCurveDocController::sig_nodeAdded, this, syncByNode, Qt::QueuedConnection);
            connect(controller, &mCurveDocController::sig_nodeAboutToBeRemoved, this, syncByNode, Qt::QueuedConnection);
            connect(controller, &mCurveDocController::sig_nodePropertyChanged, this,
                [this](int nodeId, const QString& prop)
            {
                if (prop != CurveProp::kLayout)
                    return;
                auto* doc = mCurveDocController::getInstance()->document();
                if (doc->nodeType(nodeId) == PostCurve::CurveNodeType::Page)
                    syncCurvePageLayout(nodeId);
            }, Qt::QueuedConnection);
        }

        //曲线模块：树节点选中 -> 参数栏切换属性页（设计图02）
        connect(mCurveDocController::getInstance(), &mCurveDocController::sig_currentNodeChanged, this, [this](int nodeId)
        {
            //会话恢复期守卫：恢复编排回放曲线树选中（clearDocument/-1、openChart内
            //syncTreeSelectionForChart、收尾setCurrentNodeId(保存值)）只还原曲线模块
            //内部选中，菜单模式/参数栏等全局上下文不随切——否则恢复含图表会话后
            //菜单/参数栏停在曲线而激活视口是前处理（恢复激活同ID去重早退无信号
            //可纠偏）；终态由 finishSessionRestore 按激活视口统一对齐
            //（alignContextToActiveViewport）
            if (_restoringSession)
                return;
            if (nodeId >= 0)
                mInfoWindow::getInstance()->editCurveNode(nodeId);
            //曲线树节点选中（含图上点选的sig_currentNodeChanged闭环）即处于曲线上下文：
            //菜单栏切入曲线模式。曲线图表卡片不进渲染视口注册表，选中不会触发视口激活，
            //需在此显式切换（setWorkMode同模式幂等，已在曲线模式时不打扰当前页签）
            if (m_Menu_ != nullptr)
                m_Menu_->setWorkMode(MenuWorkMode::Curve);
        });

        //曲线模块：图上双击曲线/图例项、右键菜单"编辑" -> 浮动属性编辑器
        //落在请求视窗的左上角（设计图03.1，经视口工厂中继）
        connect(mCurveViewportFactory::getInstance(), &mCurveViewportFactory::sig_requestEditorPopup,
            this, [this](int nodeId, const QPoint& globalPos)
        {
            mCurvePropPageManager::getInstance()->showEditorPopupAt(nodeId, globalPos);
        });

        m_Menu_ = new mMenuWidget;
        m_Menu_->setObjectName(QString::fromUtf8("mMenuWidget"));
        // 菜单栏最小高 = 页签高 + 功能区高，由 ThemeManager::applyAll 统一下发
        m_Menu_->setMinimumHeight(MxFunctions::UiMetrics::menuBarTotalHeight());
        m_Menu_->setCenterMainWindow(_center);

        // 自绘标题栏 + 菜单栏一起包进垂直容器，再经 setMenuWidget 挂到主窗：
        // QMainWindowLayout 只认"菜单栏项在第 0 行"，无法把自绘标题栏插到
        // 菜单栏之上。包装后整个容器就是"菜单栏"，容器内标题栏自然在最顶行
        auto* menuWrapper = new QWidget(this);
        menuWrapper->setObjectName(QStringLiteral("menuWrapper"));
        auto* wrapperLay = new QVBoxLayout(menuWrapper);
        wrapperLay->setContentsMargins(0, 0, 0, 0);
        wrapperLay->setSpacing(0);
		wrapperLay->addWidget(_titleBar);
		wrapperLay->addWidget(m_Menu_);
		// 包装容器补底：QMainWindow 中心部件不覆盖 menuWrapper 区域时，
		// wrapper 背景透明会露出桌面，需显式刷主题底色
		menuWrapper->setAutoFillBackground(true);
		QPalette wrapperPal = menuWrapper->palette();
		wrapperPal.setColor(QPalette::Window, UiPalette::surface());
		menuWrapper->setPalette(wrapperPal);
		this->setMenuWidget(menuWrapper);

		_titleBar->setVisible(true);    // 包装进布局后显示标题栏

		mDockLayoutManager::getInstance()->setCenterWidget(_center);

        // 恢复 or 应用默认
        if (!mDockLayoutManager::getInstance()->restoreLayout())
        {
            mDockLayoutManager::getInstance()->applyPreset1();
        }

        //状态栏
        _statusBar = new mStatusBar(this);
        this->setStatusBar(_statusBar);

        //连接信号
        connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::activateDockWidgetSig, this, &mMechMainWindow::slot_activateDockWidget);
        connect(mSelectInfoWindow::getInstance(), &mSelectInfoWindow::updateStatusBarSelectInfo, _statusBar, &mStatusBar::slot_updateSelectInfo);
        connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::modelDataThreadStatusChangedSig, this, &mMechMainWindow::slot_modelDataThreadStatusChanged);

        // 连接几何编辑完成信号，更新所有几何集合和 MixSet 中的几何 ID
        //【OCC摘除·S10 保档】mGeoGlobalSignals 随旧栈冻结岛退役（头拉 mGeoDataModelSet→OCC 链）
        /*connect(MGeoData::mGeoGlobalSignals::getInstance(),
            &MGeoData::mGeoGlobalSignals::finishEditShape,
            this,
            &mMechMainWindow::updateMixSetGeoIdsByEditHistory);*/

        qApp->installEventFilter(this);

        initShortcuts();

        //python 命令注册
        mPythonCommand::getInstance()->registerAPI();

        // 主窗标题栏（Windows 非客户区）按当前主题改色。
        // mMenuWidget 构造里的 applyAll 在本窗尚未 setMenuWidget 完成/未进顶层列表时
        // 可能漏掉主窗，这里在装配完成后对本窗再补发一次（幂等）。
        MechMainWindow::ThemeManager::applyTitleBarTheme(this);
    }

	void mMechMainWindow::setupCustomTitleBar()
	{
		// 统一经 FramelessWindowHelper 装配窗口标志：Windows 保留原生框架
		//（WS_THICKFRAME/WS_CAPTION），仅经 nativeEvent 的 WM_NCCALCSIZE 把
		// 非客户区清零去掉原生标题栏，换自绘标题栏（主题切换时 Qt 原生标题栏
		// 无法跟随换色）。框架仍在，DWM 阴影边界线、平滑边框缩放与 Aero Snap
		// 动画都由系统原生提供。
		// Linux 无 WM_NCCALCSIZE 机制，helper 内部改设 FramelessWindowHint 去
		// 系统标题栏（否则系统标题栏与自绘标题栏并存），边缘缩放由 helper 补齐。
		// 必须在窗口 show 之前调用：窗口标志在显示后动态切换会触发句柄重建/闪烁。
		FramelessWindowHelper::installFrameless(this);

		// 标题栏仅创建，不插入 QMainWindow 布局；布局装配在构造函数末尾
		// setMenuWidget 时经包装容器统一完成（见构造函数中 menuWrapper 段）
		_titleBar = new mWindowTitleBar(this);
		// 主窗口标题栏下方紧接菜单栏（菜单栏自带底边分界线），
		// 标题栏底部分隔线在此关闭，仅对话框/浮窗保留
		_titleBar->setSeparatorVisible(false);
		_titleBar->setVisible(false);   // 先隐藏，等 setMenuWidget 包装后再显示
	}

    void mMechMainWindow::changeEvent(QEvent* event)
    {
        // 窗口状态变化（最大化/还原/激活/最小化恢复）→ 同步自绘标题栏
        //（最大化/还原图标切换、激活态文字与图标颜色、标题栏高度补偿）
        if (event->type() == QEvent::WindowStateChange
            || event->type() == QEvent::ActivationChange
            || event->type() == QEvent::WindowTitleChange
            || event->type() == QEvent::WindowIconChange)
        {
		if (_titleBar != nullptr)
			_titleBar->updateWindowState();
	}
        QMainWindow::changeEvent(event);
    }


    mMechMainWindow::~mMechMainWindow()
    {
        shutdownTrace("[mainWin] dtor begin");
        if (m_Menu_)
        {
            delete m_Menu_;
            m_Menu_ = nullptr;
        }
        shutdownTrace("[mainWin] menu deleted");
        if (_center)
        {
            delete _center;
            _center = nullptr;
        }
        shutdownTrace("[mainWin] center deleted");
        if (_material)
        {
            delete _material;
            _material = nullptr;
        }
        if (_instance == this)
        {
            _instance = nullptr;
        }
        shutdownTrace("[mainWin] material deleted");
    }

    //曲线页视口布局互通同步：以曲线页 kLayout（打开曲线对话框网格/页面属性
    //"页面布局"提交，下标与视图树 viewLayoutTable 一致）为单一数据源，把布局
    //落到其绑定的视图页。页数量已在创建期（按格数分页，两棵树按页 1:1 对齐）
    //定死，本同步只做页内重排、不增删视图页——切换布局不改变页数量；布局格数
    //多于图表数时余格留空（容器空格子背景），不创建空视口占位部件。
    //页对齐尚未建立（建图宏内首卡片挂载前）时直接返回，首卡片挂载回填
    //viewPageId 后的入队同步会接手
    void mMechMainWindow::syncCurvePageLayout(int curvePageId)
    {
        auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
        if (!viewTree)
            return;
        auto* controller = mCurveDocController::getInstance();
        const int viewPageId = controller->pageViewPageId(curvePageId);
        if (viewPageId < 0)
            return;

        //布局下标取曲线页 kLayout，越界回退单视口
        int layoutIdx = controller->document()->nodeProperty(curvePageId, CurveProp::kLayout).toInt();
        const auto& table = viewLayoutTable();
        if (layoutIdx < 0 || layoutIdx >= table.size())
            layoutIdx = 0;

        //当前页立即重排并刷新视图显示栏（setCurrentPageLayout 同值也强制重放，
        //图表增删后格子分配随之刷新），非当前页切换到时生效
        if (viewTree->curPageID() == viewPageId)
            viewTree->setCurrentPageLayout(layoutIdx);
        else
            viewTree->getModel()->setPageLayout(viewPageId, layoutIdx);
    }

    //--- 会话保存/恢复（.mxses）支持：图表卡片登记查询与恢复模式开关 ---

    int mMechMainWindow::chartIdForCardId(int cardId) const
    {
        //卡片ID每次运行重新分配，会话持久化引用换算成曲线文档图表节点id
        return _cardChartOf.value(cardId, -1);
    }

    int mMechMainWindow::cardIdForChartId(int chartId) const
    {
        //按 chartId 直查（懒加载：未物化图表的卡片同样可查——卡片与视图生命周期解耦）
        return _chartCards.value(chartId, -1);
    }

    QWidget* mMechMainWindow::createChartPlaceholder() const
    {
        //空卡片占位（懒加载未物化态/LRU 回收卸载态共用）：深灰底，切到该视口时
        //由工厂物化图表视图就地替换
        QWidget* placeholder = new QWidget;
        placeholder->setAutoFillBackground(true);
        QPalette pal = placeholder->palette();
        pal.setColor(QPalette::Window, QColor(45, 45, 48));
        placeholder->setPalette(pal);
        return placeholder;
    }

    void mMechMainWindow::setRestoringSession(bool on)
    {
        //会话恢复批量重建期间挂起 LRU/空闲回收（恢复收尾统一物化落点页后恢复）
        mCurveViewportFactory::getInstance()->setEvictionSuspended(on);
        _restoringSession = on;
    }

    void mMechMainWindow::closeVideoViewportCard(bool notifyClosed)
    {
        if (_videoViewportId < 0)
            return;
        auto* container = VMManager->getViewportContainer();
        //摘离内容保单例：removeViewport 会 deleteLater 格子包装件并连带销毁其
        //子部件——mVideoWindow（单例，承载同步/背景全状态）必须先脱离父子链
        if (container)
        {
            if (auto* card = container->viewport(_videoViewportId))
            {
                QWidget* content = card->contentWidget();
                card->setContentWidget(nullptr);   //摘离+隐藏，不销毁
                //dock 已取消：摘离后无默认承载——隐藏待选形态（叠加/背景/再进格子）
                if (content != nullptr)
                {
                    content->setParent(nullptr);
                    content->hide();
                }
            }
        }
        if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
            viewTree->detachViewportNode(_videoViewportId);
        if (container)
            container->removeViewport(_videoViewportId);
        //关视频格后恢复云图视口的激活与呈现（用户裁定：记录建格前云图视口，
        //完成切换）：removeViewport 复位激活为-1、页布局按节点顺序重放会把
        //前处理视口顶进格子、云图被挤出呈现集（叠加挂上也看不见）。恢复=
        //切回云图所在页 + setActiveViewport（容器激活换入自动把云图换回呈现；
        //菜单经激活联动切到云图页，属本方案固有代价，用户已接受）
        if (container)
        {
            auto* model = VMManager->getCurrentViewportModel();
            int target = (_videoPrevCloudViewportId >= 0
                && container->viewport(_videoPrevCloudViewportId) != nullptr)
                ? _videoPrevCloudViewportId : -1;
            if (target < 0 && model != nullptr)
            {
                if (QWidget* postSub = model->activatePostViewport())
                    if (auto* priv = model->getViewportPrivate(postSub))
                        target = priv->getViewportID();
            }
            if (target >= 0 && container->viewport(target) != nullptr)
            {
                if (auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree())
                {
                    const int pageId = viewTree->pageOfViewport(target);
                    if (pageId >= 0)
                        viewTree->switchToPage(pageId);
                }
                container->setActiveViewport(target);
            }
        }
        _videoViewportId = -1;
        _videoPrevCloudViewportId = -1;
        if (notifyClosed)
            emit MechCommon::mMechGlobalSignals::getInstance()->sig_videoViewportClosed();
    }

    bool mMechMainWindow::isRestoringSession() const
    {
        return _restoringSession;
    }

    void mMechMainWindow::alignContextToActiveViewport()
    {
        //关门期保护：模式/树/参数栏操作均为 widget 操作，析构阶段执行有崩溃风险
        if (QCoreApplication::closingDown())
            return;
        auto* container = VMManager ? VMManager->getViewportContainer() : nullptr;
        if (container == nullptr)
            return;
        const int activeID = container->activeViewportID();
        if (activeID < 0 || activeID >= ChartViewportIdBase)
            return;
        //渲染视口：菜单模式+树窗口按视口类型对齐（与启动恢复对齐同映射同组合，
        //不含 slot_subWindowActivated 的单位制/按钮启停等重联动——那些由正常
        //激活信号链驱动，恢复去重早退场景本就无需重放）
        if (m_Menu_ != nullptr)
            m_Menu_->alignModeToViewport(activeID);
        //参数栏：渲染视口激活=离开曲线上下文，残留的曲线属性页复位（与
        //sig_activeViewportChanged 渲染视口分支的复位语义一致；其余面板粘性保持）
        if (qobject_cast<mCurvePropPageBase*>(mInfoWindow::getInstance()->currentMainWidget()) != nullptr)
            mInfoWindow::getInstance()->unEditInfoWindow();
    }

    //溢出页"选中即替换呈现"重排（双击显式发起，见头文件注释）。两种呈现模式：
    //- 手动布局可见组（视图树选视口+"布局"设置）：组未更改前保持组大小与格位序，
    //  双击图表不在组内时顶替"切换前选中视口"的组位（默认组首位），节点序同步交换
    //  （之后清除手动组时节点序与呈现一致）——如页内4个图表选3个布局后，切换仍是
    //  3个视口，不会变回4个
    //- 默认（无手动组）：布局按页节点序填格（前 cells 个呈现，其余隐藏），双击的
    //  图表卡片处于未呈现位时，与"切换前选中的视口"交换节点位——新卡片顶入其格位
    //  呈现，被顶替者退到新卡片原隐藏位，页内其余节点位置不变（grid 稳定，只有目标
    //  格位换内容）
    //非溢出页/已呈现卡片/未挂载卡片不动作。重排后当前页立即 switchToPage 重放布局
    //（同页早退分支，仅 applyLayout 不动选中），非当前页仅变节点序、切到时生效
    void mMechMainWindow::presentChartOnOverflowPage(int cardId, int prevViewportId)
    {
        auto* viewTree = mViewBrowserWindow::getInstance()->getViewTree();
        if (!viewTree || cardId < 0)
            return;
        auto* model = viewTree->getModel();
        const int pageId = viewTree->pageOfViewport(cardId);
        if (pageId < 0)
            return;
        const QList<int> pageViewports = model->pageViewportIDs(pageId);

        //手动布局可见组：保持组大小与格位序，双击图表不在组内时顶替切换前选中的
        //视口（不在组内/无效时默认组尾格，与 setActiveViewport 换入规则对齐）；
        //节点序同步交换保持两种模式一致
        QList<int> group = model->pageVisibleViewportIDs(pageId);
        if (!group.isEmpty())
        {
            if (!group.contains(cardId))
            {
                int removeId = group.contains(prevViewportId) ? prevViewportId : group.last();
                const int slot = group.indexOf(removeId);
                group.replace(slot, cardId);
                model->setPageVisibleViewportIDs(pageId, group);
                model->swapViewportNodes(pageId, cardId, removeId);
            }
            if (viewTree->curPageID() == pageId)
                viewTree->switchToPage(pageId);
            return;
        }

        //溢出判定：页内视口数多于当前布局格数
        int layoutIdx = model->pageLayout(pageId);
        const auto& table = viewLayoutTable();
        if (layoutIdx < 0 || layoutIdx >= table.size())
            layoutIdx = 0;
        const int cells = viewLayoutSlotCount(table[layoutIdx]);
        if (pageViewports.size() <= cells)
            return;
        //已在呈现位（前 cells 个）不动作
        const int pos = pageViewports.indexOf(cardId);
        if (pos < cells)
            return;
        //替换目标：与 ViewportContainer::setActiveViewport 的换入规则对齐——
        //prev 在可见位时替换 prev 所在格，否则默认替换尾格（不是首位）
        int targetId = pageViewports[qMin(cells, pageViewports.size()) - 1];
        const int prevPos = pageViewports.indexOf(prevViewportId);
        if (prevPos >= 0 && prevPos < cells)
            targetId = prevViewportId;
        model->swapViewportNodes(pageId, cardId, targetId);
        if (viewTree->curPageID() == pageId)
            viewTree->switchToPage(pageId);
    }

    void mMechMainWindow::cmdExecute(QStringList cmdList)
    {
        m_Menu_->cmdExecute(cmdList);
    }

    void mMechMainWindow::closeEvent(QCloseEvent* event)
    {
        //退出确认：先处理读写任务保护与保存询问；脚本等自动关闭场景跳过
        if (!_skipCloseConfirm)
        {
            if (m_Menu_ != nullptr && m_Menu_->isThreadCloseOrFileSave() != 0)
            {
                event->ignore();
                return;
            }
        }

        //写配置文件
        MechIOFile::mOConfigurationFile* oconfig = new MechIOFile::mOConfigurationFile;
        oconfig->writeConfigureData();
        delete oconfig;

		//保存布局
		mDockLayoutManager::getInstance()->saveLayout();
		//工作模式与DockingState同源同刻写入（setWorkMode不再即时写盘）：异常退出时
		//不会留下“新workMode+旧树布局”的矛盾组合，启动恢复出自洽状态
		if (m_Menu_ != nullptr)
			m_Menu_->saveWorkModeSetting();
		QMainWindow::closeEvent(event);

		//delete this;
        //删除求解器进程
        mSolveTaskCenter::getInstance()->interruptAll();
	}

	void mMechMainWindow::resizeEvent(QResizeEvent* event)
	{
		//截屏框重绘接收端在PostWindow::mPostInfoHub，经总线解耦
		emit MechCommon::mMechGlobalSignals::getInstance()->requestRepaintRecordWindowSig();
		QMainWindow::resizeEvent(event);
	}

    void mMechMainWindow::moveEvent(QMoveEvent* event)
    {
        emit MechCommon::mMechGlobalSignals::getInstance()->requestRepaintRecordWindowSig();
        // 与 resizeEvent 一样必须调基类实现：原生框架下窗口移动还承担
        // DWM/Aero Snap 等默认处理，吞掉基类会出现移动后的布局/重绘异常
        QMainWindow::moveEvent(event);
    }

    void mMechMainWindow::slot_activateDockWidget(QDockWidget* dock)
    {
        // 1. 确保dock可见
        dock->show();

        // 2. 如果被tabified，需要特殊处理
        QList<QDockWidget*> tabified = tabifiedDockWidgets(dock);
        if (!tabified.isEmpty()) {
            // 将其设为当前tab
            tabifyDockWidget(dock, tabified.first());
        }

        // 3. 激活窗口
        dock->raise();
        dock->activateWindow();

        // 4. 确保所在区域可见
        Qt::DockWidgetArea area = dockWidgetArea(dock);
        if (area != Qt::NoDockWidgetArea) {
            // 可以添加区域高亮逻辑
        }
    }

    bool mMechMainWindow::nativeEvent(const QByteArray& eventType, void* message, long* result)
    {
#ifdef Q_OS_WIN
        // 自绘标题栏窗口原生消息（详见 mFramelessWindowHelper.cpp 注释）
        if (MechMainWindow::FramelessWindowHelper::handleNativeEvent(this, message, result))
            return true;
#endif

        return QMainWindow::nativeEvent(eventType, message, result);
    }


    void mMechMainWindow::slot_modelDataThreadStatusChanged(bool isOpen)
    {
        if (isOpen)
        {
            mBrowserWindow::getInstance()->setEnabled(false);
            mViewBrowserWindow::getInstance()->setEnabled(false);
            m_Menu_->setEnabled(false);
        }
        else
        {
            mBrowserWindow::getInstance()->setEnabled(true);
            mViewBrowserWindow::getInstance()->setEnabled(true);
            m_Menu_->setEnabled(true);
        }
    }

    int mMechMainWindow::runBatchMeshIfRequested(const QStringList& arguments)
    {
        MGUIMesh::BatchCLIConfig cliCfg;
        if (!MGUIMesh::mMeshBatchCLI::parseArguments(arguments, cliCfg))
            return -1;

        // 标记 headless 模式，避免 mViewportModelManager 创建渲染视口
        // 必须尽早设置，确保后续单例初始化不进入渲染分支
        MechPreData::mViewportModelManager::setHeadlessMode(true);

        // 初始化中英互译
        mLanguageVarible::initLanguageVarible();

        // 读取配置文件
        MechIOFile::mIConfigurationFile configReader;
        configReader.readConfigureData();

        // 初始化语言
        MxFunctions::mGlobalSignals::getInstance()->updateTranslator(
            MAppConfiguration::mAppConfiguration::getInstance()->_language);

        qInfo().noquote() << QString("批量模式初始化完成");

        MGUIMesh::mMeshBatchCLI* batchCLI = new MGUIMesh::mMeshBatchCLI(cliCfg);

        QEventLoop loop;
        int exitCode = -1;
        QObject::connect(batchCLI, &MGUIMesh::mMeshBatchCLI::finished,
            &loop, [&](int code) {
                exitCode = code;
                loop.quit();
            });

        // 在主线程事件循环就绪后启动批量任务
        QTimer::singleShot(0, batchCLI, [batchCLI]() {
            batchCLI->start();
        });

        loop.exec();
        delete batchCLI;
        return exitCode;
    }

    void mMechMainWindow::initShortcuts()
    {
        auto mgr = ShortcutManager::instance(this);
        ShortcutActionList::instance();
        // ---- Global ----
        mgr->registerShortcut(ShortcutModule::Global, "new",
            { QKeySequence(Qt::CTRL | Qt::Key_N),"新建",
              [this] {
                  if (auto* btn = findChild<QToolButton*>(QStringLiteral("QuickNewBtn")))
                      btn->click();
              } });
        mgr->registerShortcut(ShortcutModule::Global, "open",
            { QKeySequence(Qt::CTRL | Qt::Key_O),"打开",
              [this] {
                  if (auto* btn = findChild<QToolButton*>(QStringLiteral("QuickOpenBtn")))
                      btn->click();
              } });
        mgr->registerShortcut(ShortcutModule::Global, "save",
            { QKeySequence(Qt::CTRL | Qt::Key_S),"保存",
              [] { ShortcutActionList::instance()->save(); } });

        mgr->registerShortcut(ShortcutModule::Global, "clearPick",
            { QKeySequence(Qt::Key_Escape),"清空全部拾取",
              [] { ShortcutActionList::instance()->clearPick(); } });

        mgr->registerShortcut(ShortcutModule::Global, "pickAll",
            { QKeySequence(Qt::CTRL | Qt::Key_A),"拾取全部",
              [] { ShortcutActionList::instance()->pickAllEqualGeo(); } });

        mgr->registerShortcut(ShortcutModule::Global, "screen_shot",
            { QKeySequence("Ctrl+Shift+A"), "截图",
              [] { emit MechCommon::mMechGlobalSignals::getInstance()->clickAniQuickScreenshotSig(); } });

        mgr->registerShortcut(ShortcutModule::Global, "create_anim",
            { QKeySequence("Ctrl+G"), "生成动画",
              [] { emit MechCommon::mMechGlobalSignals::getInstance()->clickAnimationSig(1); } });

        mgr->registerShortcut(ShortcutModule::Global, "hide_element",
            { QKeySequence("Ctrl+D"), "隐藏单元",
              [] { mDisplayMenuFunction::getInstance()->slot_maskModel(); } });

        // ---- Geometry ----
        mgr->registerShortcut(ShortcutModule::Geometry, "geo_undo",
            { QKeySequence(Qt::CTRL | Qt::Key_Z), "撤消几何操作",
              [] { ShortcutActionList::instance()->geometryUndo(); } });

        mgr->registerShortcut(ShortcutModule::Geometry, "geo_redo",
            { QKeySequence(Qt::CTRL | Qt::Key_Y), "重做几何操作",
              [] { ShortcutActionList::instance()->geometryRedo(); } });

        mgr->registerShortcut(ShortcutModule::Geometry, "feature_remove",
            { QKeySequence(Qt::Key_F), "特征去除",
              [] { ShortcutActionList::instance()->geometryDeleteFeature(); } });

        mgr->registerShortcut(ShortcutModule::Geometry, "delete_geo",
            { QKeySequence(Qt::Key_Delete), "删除几何体",
              [] { ShortcutActionList::instance()->geometryDelete(); } });

        mgr->dumpAllShortcuts();
    }

    void mMechMainWindow::updateMixSetGeoIdsByEditHistory(const MGeoData::mEditHistoryTag& editHistoryTag)
    {
        // 一次性处理所有类型的映射
        updateMixSetGeoIds(editHistoryTag._faceHistoryTag, 2);
        updateMixSetGeoIds(editHistoryTag._edgeHistoryTag, 1);
        updateMixSetGeoIds(editHistoryTag._vertexHistoryTag, 0);
        updateMixSetGeoIds(editHistoryTag._solidHistoryTag, 3);
    }

    void mMechMainWindow::updateMixSetGeoIds(const QHash<int, QVector<int>>& mapping, int shapeType)
    {
        if (mapping.isEmpty())
        {
            return;
        }

        auto modelData = getCVM->getModelData();
        if (modelData == nullptr)
        {
            return;
        }

        MechSetData::mSetManager& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        std::vector<int> allSetIds = setManager.getAllSetID();

        for (int setId : allSetIds)
        {
            MechSetData::mSetBase* set = setManager.getSetData(setId);
            if (set == nullptr)
            {
                continue;
            }

            // 判断是否为几何集合类型（几何点集、几何线集、几何面集、几何实体集合）
            SetType setType = set->getSetType();
            bool isGeoSet = (setType == SetType::GeoPoint || setType == SetType::GeoEdge ||
                setType == SetType::GeoFace || setType == SetType::GeoSolid);

            if (isGeoSet)
            {
                // 处理几何集合
                MechSetData::mGeoSet* geoSet = dynamic_cast<MechSetData::mGeoSet*>(set);
                if (geoSet == nullptr)
                {
                    continue;
                }

                // 根据 shapeType 确定目标 SetType
                SetType targetSetType;
                switch (shapeType)
                {
                case 0: targetSetType = SetType::GeoPoint; break;
                case 1: targetSetType = SetType::GeoEdge; break;
                case 2: targetSetType = SetType::GeoFace; break;
                case 3: targetSetType = SetType::GeoSolid; break;
                default: continue;
                }

                // 只更新匹配类型的几何集合
                if (setType != targetSetType)
                {
                    continue;
                }

                // 获取并更新几何集合数据
                std::vector<int> geoData = geoSet->getGeoSetData();
                bool updated = false;
                std::vector<int> newGeoData;

                for (int oldId : geoData)
                {
                    bool found = false;
                    for (auto mapIt = mapping.constBegin(); mapIt != mapping.constEnd(); ++mapIt)
                    {
                        if (mapIt.key() == oldId)
                        {
                            for (int newId : mapIt.value())
                            {
                                newGeoData.push_back(newId);
                            }
                            found = true;
                            updated = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        newGeoData.push_back(oldId);
                    }
                }

                if (updated)
                {
                    geoSet->setGeoSetData(newGeoData);
                }
                continue;
            }

            // 处理 MixSet
            if (setType != SetType::Mix)
            {
                continue;
            }

            MechSetData::mMixSet* mixSet = dynamic_cast<MechSetData::mMixSet*>(set);
            if (mixSet == nullptr)
            {
                continue;
            }

            // 获取当前 MixSet 数据
            auto mixSetData = mixSet->getMixSetData();
            bool updated = false;

            // 根据 shapeType 确定要更新的 SetType
            PickDataType targetSetType;
            switch (shapeType)
            {
            case 0: targetSetType = PickDataType::GeoPoint; break;
            case 1: targetSetType = PickDataType::GeoEdge; break;
            case 2: targetSetType = PickDataType::GeoFace; break;
            case 3: targetSetType = PickDataType::GeoSolid; break;
            default: continue;
            }

            // 查找并更新对应类型的数据
            auto it = mixSetData.find(targetSetType);
            if (it != mixSetData.end())
            {
                std::vector<RenderPickResultData>& dataList = it->second;
                std::vector<RenderPickResultData> newDataList;

                for (const RenderPickResultData& data : dataList)
                {
                    int oldId = data.result_id;
                    bool found = false;

                    // 在映射中查找旧 ID
                    for (auto mapIt = mapping.constBegin(); mapIt != mapping.constEnd(); ++mapIt)
                    {
                        if (mapIt.key() == oldId)
                        {
                            // 为每个新 ID 创建新的 RenderPickResultData
                            for (int newId : mapIt.value())
                            {
                                RenderPickResultData newData = data;
                                newData.result_id = newId;
                                newDataList.push_back(newData);
                            }
                            found = true;
                            updated = true;
                            break;
                        }
                    }

                    // 如果没有找到映射，保留原数据
                    if (!found)
                    {
                        newDataList.push_back(data);
                    }
                }

                if (updated)
                {
                    mixSetData[targetSetType] = newDataList;
                }
            }

            // 如果有更新，写回 MixSet
            if (updated)
            {
                mixSet->setMixSetData(mixSetData);
            }
        }
    }
}