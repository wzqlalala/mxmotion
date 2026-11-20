#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QMainWindow>
#include <QHash>
#include <QList>
#include <QVector>
#include <QStringList>

namespace MGeoData
{
	struct mEditHistoryTag;
}

namespace Ui
{
	class mMechMainWindow;
}
namespace ads
{
	class CDockManager;
}
namespace MechMainWindow
{
	class mMenuWidget;
	class mCenterMainWindow;
	class mStatusBar;
	class mMaterialWindow;
	class mMainMouseEventFilter;
	class mWindowTitleBar;
	class IRibbonHost;
	class MMAINWINDOW_EXPORT mMechMainWindow : public QMainWindow
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mMechMainWindow(QWidget *parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMechMainWindow();

		/*
		* 全局唯一实例（应用单窗体，mAppInitializer 创建；供 mMxsesRestore 等
		* 静态编排代码访问图表卡片登记与恢复模式开关）
		*/
		static mMechMainWindow* getInstance();

		/*
		* Ribbon宿主接口（供PostWindow等模块经组合根注册菜单页；实现返回菜单栏m_Menu_）
		*/
		IRibbonHost* getRibbonHost() const;

        /*
        * 命令行导入求解
        */
        void cmdExecute(QStringList cmdList);

        /*
        * 跳过关闭确认（脚本/无界面自动关闭场景设置，避免closeEvent弹窗阻塞）
        */
        void setSkipCloseConfirm(bool skip) { _skipCloseConfirm = skip; }

		/**
		 * @brief 若命令行请求批量剖分，则执行批量任务并返回退出码；
		 *        未请求批量模式时返回 -1，由调用方继续正常 GUI 启动流程。
		 */
		static int runBatchMeshIfRequested(const QStringList& arguments);

		//溢出页"选中即替换呈现"重排（双击显式发起，单击选中不触发）：cardId
		//双击的图表卡片未呈现时与切换前选中的视口 prevViewportId 交换呈现位——
		//新卡片顶入其格位，被顶替者退到隐藏位；prev 无效（首次激活/跨页切换/
		//前任已隐藏）时默认与首位交换。呈现位取认：页有手动布局可见组（选视口+
		//"布局"设置）则按组（保持组大小与格位序，未更改布局前持久）；否则按节点
		//序前 cells 个（页内视口数多于布局格数才动作）。非溢出页/已呈现卡片不动作；
		//当前页立即 switchToPage 重排，非当前页仅变节点序、切到时生效。无实例状态
		static void presentChartOnOverflowPage(int cardId, int prevViewportId);

//--- 会话保存/恢复（.mxses）支持：图表卡片登记查询与恢复模式开关 ---
//曲线图表视口卡片的卡片ID段起点：视口ID >= 此值即图表卡片（会话采集/收尾
//据此判别激活视口类型）。卡片不进渲染视口注册表，激活卡片不影响当前渲染视口。
//三维图（R28 入树）同为曲线文档图表节点，卡片同落本段（旧 2000000 独立段
//随 T6 退役：三维卡片获得关闭/重命名/会话恢复全链）
static constexpr int ChartViewportIdBase = 1000000;
	//卡片ID -> 曲线文档图表节点id（会话采集把页内卡片换算成可持久化引用；
	//未知卡片返回 -1。卡片ID每次运行重新分配，不进会话文件）
	int chartIdForCardId(int cardId) const;
	//曲线文档图表节点id -> 卡片ID（会话恢复按页可见组换算新卡片ID用；
	//图表未开卡片返回 -1）
	int cardIdForChartId(int chartId) const;
	//会话恢复批量重建图表卡片期间置位：抑制卡片创建联动的激活曲线dock/
	//切页/激活卡片副作用（页归属仍按 sig_requestChartCard 携带的
	//viewPageId 正常登记，最终页切换与激活由恢复编排收尾统一执行）
	void setRestoringSession(bool on);
	bool isRestoringSession() const;
	//恢复收尾上下文终态对齐：按容器激活视口对齐菜单工作模式/树窗口，并复位
	//参数栏残留的曲线属性页。恢复激活视口与当前激活同ID时容器去重早退、无
	//激活信号，菜单/参数栏联动缺席；恢复期曲线树选中回放（sig_currentNodeChanged
	//全局联动）被恢复守卫拦下后，上下文终态以此对齐为准。图表卡片激活
	//（>=ChartViewportIdBase）不在此对齐（曲线上下文由卡片激活信号链维护）
	void alignContextToActiveViewport();

	protected:

		/*
		* 关闭事件
		*/
		void closeEvent(QCloseEvent* event) override;

		/*
		* 原生事件
		*/
		bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;

		void resizeEvent(QResizeEvent* event);
		void moveEvent(QMoveEvent* event);
		void changeEvent(QEvent* event) override;

	private:
		//自绘标题栏装配：窗口框架保留（阴影边界/平滑缩放/恢复动画由系统提供），
		//自绘标题栏置于菜单栏之上（菜单栏经setMenuWidget 挂载于 QMainWindow 布局顶部）
		void setupCustomTitleBar();

	private slots:
		/*
		* 激活dock窗口
		*/
		void slot_activateDockWidget(QDockWidget* dock);

		      /*
		      * 是否启动模型读写线程
		      */
		      void slot_modelDataThreadStatusChanged(bool isOpen);

    private:
        /*
        * 根据几何编辑历史映射更新几何集合和 MixSet 中的几何 ID
        * @param editHistoryTag: 旧ID -> 新IDs 的映射
        */
        void updateMixSetGeoIdsByEditHistory(const MGeoData::mEditHistoryTag& editHistoryTag);

        /*
        * 根据指定类型的 ID 映射更新所有几何集合和 MixSet
        * @param mapping: 旧ID -> 新IDs 的映射
        * @param shapeType: 几何类型 (0=vertex, 1=edge, 2=face, 3=solid)
        */
        void updateMixSetGeoIds(const QHash<int, QVector<int>>& mapping, int shapeType);
	private:
		Ui::mMechMainWindow* _ui;
		static mMechMainWindow* _instance;   //全局唯一实例（getInstance）
		mMenuWidget* m_Menu_ = nullptr;
		mCenterMainWindow* _center = nullptr;
		mMaterialWindow* _material = nullptr;

	//曲线图表视口卡片：图表视图挂ViewportContainer的独立ID段（不进渲染视口注册表，
	//激活卡片不影响当前渲染视口；生命周期与曲线树映射由mCurveViewportFactory按chartId自管）。
	//懒加载：卡片（常驻）与视图（按需物化）生命周期解耦，故两表均按 chartId/cardId 做
	//键——未物化的图表同样能反查卡片（改标题/激活/关闭不依赖视图存在）
	QHash<int, int> _chartCards;     //chartId -> 卡片ID
	QHash<int, int> _cardChartOf;    //卡片ID -> chartId（反向反查）
	int _chartCardSeq = 0;
	//空卡片占位内容（懒加载：卡片登记与 LRU/空闲回收卸载共用同一构造）
	QWidget* createChartPlaceholder() const;

	//对比视频-独立视口（图表卡片同款：不进渲染视口注册表，激活不影响当前渲染
	//视口）。ID 段独立于图表卡（1000000）与已退役三维卡旧段（2000000），取
	//3000000 全新段；单实例（同一时刻至多一个视频格子）
	static constexpr int VideoViewportIdBase = 3000000;
	int _videoViewportId = -1;
	//建视频格前当前模型的云图视口 ID（关格后恢复其激活与呈现——视频格激活/
	//删除均无树侧激活转移，不恢复则页布局按节点顺序重放、前处理视口占格，
	//云图被挤出呈现集=切叠加后"依旧错误"的根因）
	int _videoPrevCloudViewportId = -1;
	//关闭视频格子（摘离内容保 mVideoWindow 单例→摘树节点→删格→本体隐藏待选形态）；
	//notifyClosed=外部删除路径（视图树删节点）时发 sig_videoViewportClosed 回弹面板
	void closeVideoViewportCard(bool notifyClosed);

		//曲线页视口布局同步：以曲线页 kLayout（与视图树 viewLayoutTable 同一下标）
		//为单一数据源，只做其绑定视图页的页内重排，不增删视图页——页数量在创建期
		//（打开曲线对话框按格数分页）一次定死；格数多于图表数时余格留空
		void syncCurvePageLayout(int curvePageId);

        //是否跳过关闭确认（脚本/无界面模式置true）
        bool _skipCloseConfirm = false;

		//会话恢复批量重建期间置位（见 setRestoringSession）
		bool _restoringSession = false;

		mStatusBar* _statusBar = nullptr;

		//布局管理器
		ads::CDockManager* _dockManager;

		//python脚本
		QList<QString> _pythonlist;

		//事件管理器
		mMainMouseEventFilter* _filter;

		//自绘标题栏（Frameless 模式下替代原生非客户区标题栏）
		mWindowTitleBar* _titleBar = nullptr;

        //注册快捷键
        void initShortcuts();
	};
}