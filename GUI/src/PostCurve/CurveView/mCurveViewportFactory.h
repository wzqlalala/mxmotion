#pragma once
#pragma execution_character_set("utf-8")
// =============================================================================
// 曲线模块 - 视口工厂（图表节点 <-> 视口卡片/图表视图 的自动联动，单例）
//
// 订阅文档信号，维护两张登记表（懒加载，对齐 HyperViewer"切到才渲染"）：
//   - _cardCharts：已登记视口卡片（常驻，占位内容）——节点加入即登记（openChart）；
//   - _chartViews：已物化图表视图（mCurveChartView + GL 画布，成本高）——只在
//     "切到该视口所在页 / 激活该视口 / 显式需要"时创建（materializeChart），
//     创建即从文档全量同步（构造 rebuild），配置持久化无损；
//   - 视口节点将删除（删除/撤销新建）-> 自动关闭对应卡片（视图随卡片释放）。
// 由此，任何来源（Ribbon、树右键、pch 导入、撤销重做）的视口增删都无需
// 手工管理视口，批量导入（VTF/动刚度/NTF 数百图）只物化落点页。
//
// 视口放置策略（ChartPlacement，由创建方按布局选择显式指定）：
//   ReuseCurrent/CreateNew 在新视图架构下均落为"登记空卡片"（历史 API 保留，
//   行为无差异）；批量导入配合 setDeferCardActivation(true) 延迟切页/激活，
//   收尾经 activatePageFirstChart 切到落点页并物化其首图。
// 注意：撤销后重做会重新走 nodeAdded，此时策略已还原（CreateNew），
// "使用当前绘图"的批次重做后按新建处理（骨架阶段接受的语义）。
//
// 与主窗口解耦：PostCurve 不依赖 MainWindow（依赖方向为 MainWindow -> PostCurve），
// 卡片登记/视图挂载/关闭/重标题/激活均经 sig_request* 信号请求，由 mMechMainWindow
// 连接响应（Qt 直连同步，emit 返回时卡片/视图已就绪）
//
// 曲线树 <-> 曲线视口 双向联动（本工厂为图表<->卡片映射的持有者，联动在此收口）：
//   - 视口 -> 树：容器激活变化 -> 主窗口回投 notifyChartViewportActivated ->
//     树选中对应图表节点（selectNode 会经选择中枢连带参数栏换页）；
//   - 树 -> 视口：双击图表节点（activateChartViewport）-> 物化（未物化时）并激活
//     其视口卡片；单击仅选中（参数栏等选择联动），不激活视口——防误触切换；
//   - 新建图表放置成功后做一次初始树选中。
//   防回环：同图守卫（树上已选中该图表的节点时不改选）+ 已激活视口早退 +
//   控制器 setCurrentNodeId 同值短路，三者保证联动链有界终止。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QObject>
#include <QHash>
#include <QPoint>
#include <QString>
#include <QList>
#include <QSet>
#include <QTimer>
#include <QElapsedTimer>
#include <functional>
#include <list>
#include <iterator>


namespace PostCurve
{
	//图表视口放置策略（见文件头注释）
	enum class ChartPlacement
	{
		CreateNew = 0,
		ReuseCurrent,
	};

	class mCurveChartView;

	class POSTCURVE_EXPORT mCurveViewportFactory : public QObject
	{
		Q_OBJECT
	public:
		static mCurveViewportFactory* getInstance();

		//图表放置方式（批量创建前设置；见 ChartPlacement 说明）
		void setPlacementOverride(ChartPlacement placement);
		void clearPlacementOverride();   //批量创建（宏）结束后还原为默认 CreateNew

		//--- 卡片/视图解耦（懒加载，对齐 HyperViewer"切到才渲染"） ---
		//批量建卡模式：on=true 期间空卡片登记不切页、不激活、不物化（VTF/动刚度/
		//NTF 批量导入用，落点页由创建收尾的 activatePageFirstChart 统一切换物化）
		void setDeferCardActivation(bool on) { _deferCardActivation = on; }
		bool deferCardActivation() const { return _deferCardActivation; }

		//懒物化：为图表创建 mCurveChartView 并挂进其既有卡片（幂等；卡片未登记/
		//节点已删时忽略）。创建即从文档全量同步（构造 rebuild），配置不丢
		void materializeChart(int chartId);
		//按视口卡片 id 列表批量物化（applyPageLayout 布局应用后对本页可见卡片调用；
		//同时刷新可见集——可见图表受回收保护）。批量内部分批创建（每轮至多
		//kMaterializeChunkSize 个新 GL 画布，经 0ms 定时器分摊到事件循环）：
		//大格数布局（12/16 格）整页一次同步物化时，画布创建/显示/强制刷新级联
		//堆在同一事件循环突发里，驱动侧同步重绘逐层嵌套会栈溢出（用户实测
		//9 格及以下正常、12/16 格必崩 nvoglv64.dll）；首批同步创建（落点首图
		//在首批内，activatePageFirstChart 激活链即用），后批的显隐由主窗口
		//挂载分支按文档补齐。新调用整队替换，旧批次残留不再投喂
		void materializeForCardIds(const QList<int>& cardIds);

		//--- Phase 2：LRU 上限 + 空闲回收 + pin（对齐 HyperViewer 的视口队列机制） ---
		//物化视图数上限：超过后按 LRU 从最久未用端回收（定时器与物化路径共同驱动）
		enum { kMaxMaterializedViews = 24 };
		//单轮批量物化的新建视图数上限（每轮落进独立的事件循环迭代，批间
		//布局/绘制得以收敛；上限取实测安全线 9 格以下的一半）
		enum { kMaterializeChunkSize = 4 };
		//空闲回收阈值（毫秒）：物化后该时长内未被触碰则回收
		static qint64 idleEvictMs() { return 60 * 1000; }
		//触碰：刷新图表的 LRU 位置与空闲计时（激活/物化时自动调用；交互密集场景可显式调）
		void touchChart(int chartId);
		//显式 pin/unpin：编辑会话/定义曲线等锁定目标在会话期禁止回收
		void pinChart(int chartId, bool pinned);
		//挂起/恢复回收扫描（会话恢复批量重建期间置 on，收尾恢复）
		void setEvictionSuspended(bool on) { _evictionSuspended = on; }
		//是否可回收（供调用方在借用视图前判断；pin/可见/当前激活均受保护）
		bool isEvictable(int chartId) const;

		//图表是否已登记视口卡片（含已物化）
		bool hasChartCard(int chartId) const { return _cardCharts.contains(chartId); }
		//按卡片 id 反查图表节点 id（未登记返回 -1）
		int chartIdForCard(int cardId) const { return _cardIdToChart.value(cardId, -1); }

		//主窗口回填：卡片登记完成（cardId 分配并建卡后调用，工厂据此维护双向映射）
		void notifyChartCardRegistered(int chartId, int cardId);

		//主窗口回填：当前视图页查询提供者（产出链落点解析用——"当前视图页打开
		//新窗口"需按触发时刻的当前视图页定位落点曲线页；PostCurve 不依赖
		//MainWindow，经提供者间接查询，同 postTreeDockProvider 惯例）
		void setViewPageProvider(std::function<int()> provider) { _viewPageProvider = provider; }
		//主窗口回填：视图页 id→页名查询提供者（新建曲线页与视图页页名对齐用，
		//同登记链建页分支"曲线页名回填为视图页名"）
		void setViewPageNameProvider(std::function<QString(int)> provider) { _viewPageNameProvider = provider; }
		//当前视图页 id（提供者未注册返回 -1）
		int currentViewPageId() const { return _viewPageProvider ? _viewPageProvider() : -1; }
		//视图页页名（提供者未注册/页不存在返回空串）
		QString viewPageName(int viewPageId) const
		{
			return _viewPageNameProvider ? _viewPageNameProvider(viewPageId) : QString();
		}

		//产出链落位收口（chart-viewport-placement 单写者，D1/D2）：批量产出完成后由
		//产出方调用一次——把本次产出集合交主窗口一次性落位：切到落点视图页→按
		//张数选最小格数布局→呈现集=本次集合→一次 applyLayout 收敛。工厂负责
		//chartId→卡片id 换算（未登记卡片的图表跳过）与曲线页→视图页换算
		void presentProducedCharts(int curvePageId, const QList<int>& chartIds);

		//静默登记收口（用户终态裁定：绘制结束不自动显示——不占空格、不浮出、
		//不动页布局，显示时机完全由用户自行调整页面布局决定）：把本次新卡片
		//交主窗口做呈现集冻结（=当前呈现视口，不含新卡——防节点序填格把新卡
		//补进空格自动显示）与曲线页 kLayout 回写（防建节点排队同步翻页布局）；
		//不切页不重放不呈现。工厂负责 chartId→卡片id 换算与曲线页→视图页换算。
		//曲线提取"云图与曲线同页、用户自调布局"用
		void registerChartsOnPage(int curvePageId, const QList<int>& chartIds);

		//全部已登记卡片的图表节点id（含未物化）。报告导出等需遍历全部图表的功能使用
		//（配合 materializeChart + releaseChartView 逐个借用，避免全量常驻）
		QList<int> allCardChartIds() const;
		//立即销毁图表视图并回收（与定时器回收不同：delete 而非 deleteLater，供
		//批量借用场景逐个释放；pin/可见/当前激活图表自动保留不释放）
		void releaseChartView(int chartId);

		//把视口节点放置到 MDI 视口：按当前放置策略复用当前视口或新建带类型+名字的视口。
		//若该图表已有视口则忽略（幂等）。
		//懒加载语义：本接口只登记【空卡片】（占位内容，常驻），图表视图由
		//materializeChart 在"切到该视口所在页/激活该视口/显式需要"时创建挂载
		void openChart(int chartId);

		//双击图表节点（曲线树/图上等来源）：物化（未物化时）并激活其视口卡片。
		//nodeId 为图表或其子节点（经 chartNodeId 归一），无卡片时忽略。单击选中
		//不激活（防误触），溢出页"选中即替换呈现"由主窗口在激活请求通道上做重排
		void activateChartViewport(int nodeId);

		//按视口全限定显示名（如 "p1w1 : XY Plot"，即建视口时传入的 MDI 标题）查找已打开
		//的图表视图；未打开返回 nullptr。视口新建时以该名字命名（newViewport(type, name)）
		QWidget* chartViewByName(const QString& chartName) const;

		//创建批次落页：按格数分页创建后，激活链停在末块尾部（半空页），落点切回
		//首页并激活其首张已开图表卡片（页切换 + 卡片激活两条既有请求通道）
		void activatePageFirstChart(int curvePageId);

	//按图表节点id取已打开的曲线视图（未打开返回 nullptr）。轴属性页回显
	//实际显示范围（mCurveChartView::queryAxisDisplayRange）经此反查视口
	mCurveChartView* chartView(int chartId) const;

	//按图表节点id取已物化视图（QWidget 级，不限视图类：XY=mCurveChartView、
	//三维图=m3DChartView，R28 入树）。主窗口激活回投等类型无关消费用；
	//未打开返回 nullptr
	QWidget* chartViewWidget(int chartId) const;

	//按视图反查图表节点id（未打开/非本模块视图返回 -1）。会话保存采集页结构时
	//主窗口用它把卡片ID换算成可持久化的图表节点id（卡片ID每次运行重新分配）
	int chartIdForView(QWidget* view) const;

	//全部已打开图表的节点id（无序）。报告输出等需遍历截图各视窗的功能使用
	QList<int> openedChartIds() const;

	//图表的全限定显示名（同视口卡片标题，"p1w1 : XY Plot"）；图表未打开
	//或文档不可用时返回空串。报告导出的图题经此取
	QString chartDisplayName(int chartId) const;

		//当前激活视口承载的图表节点id；非曲线视口（模型/云图等）或无视口时返回 -1。
		//"使用当前绘图"布局据此判断新曲线并入既有图表还是新建（mOpenCurveDlg）；
		//定义/编辑曲线的开启目标也取自这里（Ribbon 按钮态已保证曲线视窗激活）
		int currentChartId() const;

		//当前激活图表的类型（CurveChartData.chartType）；无激活图表/反查失效
		//返回 -1（订阅方按 chartTypeProfile 回退类型 XY=全功能可见处理）。
		//随 sig_currentChartTypeChanged 广播，Ribbon 按类型显隐的按钮据此查询
		int currentChartType() const;

		//主窗口卡片激活变化回投（容器 sig_activeViewportChanged -> 主窗口反查图表卡片
		//后调用；chartView=nullPtr=激活的不是本模块图表）：维护当前图表并广播
		//sig_currentChartChanged（含 -1），替代旧 MDI 的 onViewportActivated 通道。
		//直连同步链：openChart 建卡片即激活也会经此路广播，无需放置后手工补发
		void notifyChartViewportActivated(QWidget* chartView);

//--- 注释模式（Ribbon"注释"按钮的全局开关） ---
//模式是全局的：一次切换广播给全部已打开图表视图（sig_annotationModeChanged，
//各 mCurveChartView 自行响应）；按钮保持按下直至再次点击。
//前条件（判据=存在已打开视口，区别于编辑曲线的"激活视口"判据——本模式目标
//在点击创建时才确定，不锁定视窗）：无已打开图表视口时开启被拒（警告提示+
//广播 off 回弹按钮）；最后一个视口关闭时自动退出；新建视口由工厂同步承接
void setAnnotationMode(bool on);
	bool annotationMode() const { return _annotationMode; }

	//--- 三维图注释模式（Ribbon"3D曲线-标注-注释"按钮的全局开关，R46；对齐
	//二维注释模式交互：按钮检出态激活→画布内容点击创建牵引线注释→再次点击
	//退出。模式是全局的：一次切换广播给全部已打开三维图视图
	//（sig_annotation3DModeChanged，各 m3DChartView 自行响应）。前条件=存在
	//已登记的三维图卡片（chartType==5）；与二维注释模式互斥（两按钮同为
	//"注释"概念，一次只允许一种激活）。三维画布左键语义与基准线/编辑曲线/
	//定义曲线（均为二维画布模式）无交集，不互踢）
	void setAnnotation3DMode(bool on);
	bool annotation3DMode() const { return _annotation3DMode; }

	//--- 寻点标记（R47，三维数值页寻点） ---
	//全局单标记状态 {_pointSeekChartId, _pointSeekCurveId, _pointSeekPointIndex}
	//（-1=无）：数值页寻点动作经此下发，工厂转发给对应已物化 m3DChartView 绘制
	//橙点+坐标文本；跨图寻点旧图标记自动清除；目标曲线/图表删除自动清除
	//（sig_nodeAboutToBeRemoved 联动）。标记为**视图态**——不进命令栈、不写
	//文档、不序列化、不触发 rebuild（纯 QWidget 覆盖层投影绘制，相机交互零
	//影响）；新物化三维视图经 materializeChart 承接当前状态（R46 注释模式
	//同款先例）
	void setPointSeekMarker(int chartId, int curveId, int pointIndex);
	int pointSeekChartId() const { return _pointSeekChartId; }
	int pointSeekCurveId() const { return _pointSeekCurveId; }
	int pointSeekPointIndex() const { return _pointSeekPointIndex; }

	//--- 基准线模式（Ribbon"基准线"下拉按钮的全局开关，设计图14） ---
	//模式是全局的：一次切换广播给全部已打开图表视图（sig_baselineModeChanged）；
	//on=true 时 orientation 指定方向（0 横向 1 纵向，同 CurveBaselineData），
	//模式内画布左键连续创建、悬停曲线显示 X/Y 读数，直至再次切换关闭；
//与注释模式互斥（开启一个自动关闭另一个：两者共用画布左键语义与 snapper）；
//前条件/全关收口/新视口承接同注释模式（见其注释）
void setBaselineMode(bool on, int orientation = 1);
	bool baselineMode() const { return _baselineMode; }
	int baselineOrientation() const { return _baselineOrientation; }

	//--- 定义曲线模式（Ribbon"定义曲线"按钮，UI设计图10~13） ---
	//模式是全局的：开启时锁定当前激活视口承载的图表为目标并广播
	//（sig_defineCurveStarted），浮动窗口（mDefineCurveDlg）只连信号显隐回填，
	//本工厂不接触对话框（同编辑曲线会话的解耦策略）。四种功能（定义曲线/
	//编辑曲线/基准线/注释）互斥在各自开启接口收口：开启一个自动退出其余三种
	//（一次只允许一种功能；曲线"选择"类多选交互——编辑曲线目标集等——不受限）
	void setDefineCurveMode(bool on);
	bool defineMode() const { return _defineMode; }

	//--- 编辑曲线会话（Ribbon"编辑曲线"按钮，UI设计图16） ---
	//会话是全局单例的：开启时锁定"当前激活视窗 + 其第一条曲线"为目标，
	//范围/方向/步长等状态由本工厂持有并广播——浮动窗口（mEditCurveDlg）
	//与目标视窗（mCurveChartView）都只连信号同步，互不直接依赖（同注释
	//模式的广播策略）。范围值存"渲染空间"（与轴刻度一致，对数/dB 轴下
	//与十字光标语义相同；"裁剪"落盘时反变换回原始数据空间）。
	void setEditCurveMode(bool on);
	bool editCurveMode() const { return _editMode; }

	//会话目标与状态（渲染空间；无会话时 chartId 为 -1、目标集为空。
	//目标为集合——设计图16 支持同时编辑多条曲线，首条为主目标）
	int editChartId() const { return _editChartId; }
	int editCurveId() const { return _editCurveIds.isEmpty() ? -1 : _editCurveIds.first(); }
	QList<int> editCurveIds() const { return _editCurveIds; }
	int editDirection() const { return _editDirection; }   //0水平(沿X截取) 1垂直(沿Y截取)
	double editStart() const { return _editStart; }
	double editEnd() const { return _editEnd; }
	double editStep() const { return _editStep; }
	double editBoundMin() const { return _editBoundMin; }  //目标曲线数据界（渲染空间，取目标集并集）
	double editBoundMax() const { return _editBoundMax; }
	QWidget* editCurveView() const;                        //目标视窗（无返回 nullptr）

	//换目标曲线（Ctrl+点选；单条，替换整个目标集；本期限本图表内）
	void setEditTargetCurve(int curveNodeId);
	//设置目标曲线集（曲线选取器多选；合法集合与当前不同才生效，本图表内）
	void setEditTargetCurves(const QList<int>& curveNodeIds);
	//改范围（各自先钳制到数据界，交叉时"变动的一侧"优先）与方向（重算界并复位全范围）
	void setEditRange(double start, double end);
	void setEditDirection(int direction);
	void setEditStep(double step);
	//步进：bound 0=起始 1=终止；dir -1/+1 按步长移动，jump=true 直达数据端
	void stepEditBound(int bound, int dir, bool jump = false);
	//按模型数据重算数据界并把范围复位到全量（裁剪落盘/换目标/换方向后）
	void refreshEditBounds();

signals:
	//请求主窗口：为图表 chartId 登记空视口卡片（懒加载第一段：卡片常驻，内容为
	//占位 widget；图表视图由 materializeChart 物化后经 sig_requestAttachChartViewport
	//挂进同一卡片）。curvePageId=图表所属曲线页节点；viewPageId=该页对应的视图页
	//（-1=请求新建视图页，新建后由主窗口回填曲线页映射）。
	//与旧 sig_requestChartViewport（视图随卡片一次性创建）的差异：本信号不携带
	//视图，主窗口登记后按 deferCardActivation/当前页决定是否立即物化
	void sig_requestChartCard(int chartId, const QString& chartName, int curvePageId, int viewPageId);

	//请求主窗口：把已物化的图表视图挂进其既有卡片（占位内容替换为视图）
	void sig_requestAttachChartViewport(QWidget* chartView, int chartId);

	//请求主窗口：应用图表 chartId 的显隐（kChartVisible 变更，on=目标显隐）。
	//卡片显隐唯一权威=容器布局应用（呈现集），工厂访问不到容器（分层约束），
	//须由主窗口代办：on=false 立即隐藏；on=true 仅当卡片在最近一次布局呈现集
	//内才显示——溢出隐藏/他页卡片直接翻可见会以过期几何浮于当前布局之上
	//（2026-11-17 布局切换挤压同根钉因），其显示延后到进入呈现集时由布局应用/
	//挂载分支呈现。仅卡片挂载形态发出；独立窗口兜底由工厂内直接处理
	void sig_requestApplyChartVisibility(int chartId, bool on);

	//请求主窗口：LRU/空闲回收——把图表 chartId 的视图从其卡片卸下（占位内容回填），
	//卡片/页登记/标题全部保留（与"关闭"的区别：关闭连卡片一起删）。
	//视图 deleteLater 后经 destroyed 联动清工厂登记表
	void sig_requestUnmountChartViewport(int chartId);

	//请求主窗口：按图表节点 id 关闭其视口卡片并释放视图（关闭树节点/图表请求
	//关闭时发出；携带 chartId 而非视图指针——视图可能尚未物化）
	void sig_requestCloseChartViewport(int chartId);

	//请求主窗口：仅移除图表 chartId 的视口卡片（视图未物化的关闭路径，无视图可释放）
	void sig_requestCloseChartCard(int chartId);

	//请求主窗口：更新图表 chartId 视口卡片的标题（图表增删改变兄弟 w 位置序/
	//图表重命名后重算时逐卡片发出；按 chartId 定位，未物化同样生效）
	void sig_requestRetitleChartCard(int chartId, const QString& newTitle);

	//请求主窗口：按卡片 id 激活图表卡片（双击图表节点/落点页首图联动；激活前
	//调用方负责物化，未物化卡片激活会被主窗口懒物化兜底）
	void sig_requestActivateChartCard(int cardId);

	//请求主窗口：切换到视图页 viewPageId（曲线树选中页节点时联动，页对齐）
	void sig_requestSwitchViewPage(int viewPageId);

	//请求主窗口：产出链落位收口（presentProducedCharts 转发）——切到目标视图页、
	//按本次产出张数选最小格数布局、呈现集=本次集合、一次 applyLayout 收敛
	//（chart-viewport-placement：呈现集唯一性/浮卡禁令的单写者通道）
	void sig_requestPresentCharts(int viewPageId, const QList<int>& cardIds);

	//请求主窗口：静默登记收口（registerChartsOnPage 转发）——呈现集冻结为目标页
	//当前呈现视口（不含新卡）、kLayout 回写，新卡保持隐藏至用户调整页面布局；
	//不切页不呈现
	void sig_requestRegisterChartsOnPage(int viewPageId, const QList<int>& cardIds);

	//图表视图请求弹出浮动属性编辑器（图上双击曲线/图例双击/右键"编辑"，
	//设计图03.1）的中继：主窗口连接 -> mCurvePropPageManager::showEditorPopupAt。
	//维持依赖方向（CurveView 不直接依赖 CurvePages，仍由 MainWindow 居中转）
	void sig_requestEditorPopup(int nodeId, const QPoint& globalPos);

	//当前图表变化广播，两处发射：MDI 视口激活变化时（视口当前承载的图表id，
	//非曲线视口为 -1）；图表视图放入 MDI 视口后（openChart 各放置路径——激活
	//信号早于 setWidget，彼时反查为 -1，需放置后补发）。Ribbon"定义曲线"等
	//仅在曲线视窗激活时可用的按钮据此启停——不依赖各处对视口管理器
	// currentViewportChanged 信号的连接顺序
	void sig_currentChartChanged(int chartId);

	//当前图表类型变化广播（三处发射：激活视口变化携带新图表的类型；当前图表的
	//kChartType 属性变化——属性编辑器切换/撤销重做；图表放入视口补发时。
	//chartType=-1 = 当前无曲线视口/反查失效，订阅方按 chartTypeProfile 回退
	//类型 XY=全功能可见处理）。Ribbon"新建/编辑曲线"等按图表类型显隐的按钮
	//据此驱动（隐藏声明见 ChartTypeProfile.h (h)）
	void sig_currentChartTypeChanged(int chartType);

	//视窗 Shift+左键点曲线的拾取中继（定义曲线弹窗"数学运算"来源 X/Y 向量
	//引用的拾取通道）：chartId=曲线所在图表节点id，监听方按文档 p/w/c 编号
	//合成引用（同曲线引用树的编号规则）
	void sig_curvePicked(int chartId, int curveNodeId);
	//NVH 模态贡献量频率拾取中继（视图画布左键，X 已吸附最近数据点；面板按
	//自己产出的 chartId 集合过滤，design D4）
	void sig_canvasFrequencyPicked(int chartId, double xValue);

		//注释模式开关变化（Ribbon"注释"按钮 -> 各图表视图）
		void sig_annotationModeChanged(bool on);

		//三维图注释模式开关变化（Ribbon"3D曲线-标注-注释"按钮 -> 各三维图视图，
		//R46；各 m3DChartView 据此启停画布左键打点创建语义）
		void sig_annotation3DModeChanged(bool on);

		//基准线模式开关/方向变化（Ribbon"基准线"下拉 -> 各图表视图）；
		//on=false 时 orientation 保持最后一次值（便于 Ribbon 菜单回显）
		void sig_baselineModeChanged(bool on, int orientation);

		//--- 定义曲线模式（浮动窗口/Ribbon 按钮连这些信号同步） ---
		//模式开启（携带目标图表节点id）：浮动窗口重绑目标、复位并弹出，Ribbon
		//按钮压下；模式结束（再次点击按钮/关闭浮动窗口/其他功能开启/开启失败/
		//目标图表删除）：浮动窗口关闭（未应用的定义丢弃）、按钮回弹
		void sig_defineCurveStarted(int chartId);
		void sig_defineCurveStopped();

		//--- 编辑曲线会话（目标视窗/浮动窗口/Ribbon 按钮都连这些信号同步） ---
		//会话开启（锁定目标视窗与目标曲线集，默认首条曲线）：视窗建裁剪线+
		//压暗覆盖层，浮动窗口回填并弹出，Ribbon 按钮压下
		void sig_editCurveStarted(int chartId);
		//目标曲线集更换（Ctrl+点选 / 曲线选取器多选）：视窗重采样截取段、
		//窗口回填名称（无参，监听方经 editCurveIds() 取当前目标集）
		void sig_editCurveTargetsChanged();
		//范围变化（拖拽裁剪线/输入框/步进按钮）：视窗同步裁剪线与截取段、窗口回填数值
		void sig_editCurveRangeChanged(double start, double end);
		//方向变化（水平/垂直）：视窗切换裁剪线方向、窗口回填下拉
		void sig_editCurveDirectionChanged(int direction);
		//会话结束（再次点击按钮/关闭浮动窗口/目标失效/开启失败）：视窗还原、
		//窗口隐藏、按钮回弹
		void sig_editCurveStopped();

	private slots:
		//文档信号：新增节点为视口 -> 自动放置（新建/重做都会走到这里）
		void onNodeAdded(int nodeId);

		//双向联动：MDI 视口激活变化 -> 曲线树同步选中对应图表节点。
		//非曲线视口（模型/云图等，chartIdForSubWindow 返回 -1）时曲线树保持原选中。
		//新架构下暂未接入（currentViewportChanged 发视口ID），实现与连接暂时注释
		//void onViewportActivated(QMdiSubWindow* subWindow);

		//双向联动：选中节点变化 -> 激活其所属图表的视口。
		//节点为图表自身或其子节点时生效；页/工程根节点不改变当前激活视口
		void onCurrentNodeChanged(int nodeId);

	private:
		mCurveViewportFactory(QObject* parent = nullptr);
		//重算指定曲线页内已登记卡片的显示标题（=nodeFullDisplayName 全限定显示名
		//"p1w1 : XY Plot"）：该页图表增删（含撤销/重做）改变兄弟 w 位置序后调用。
		//只刷本页（图表重命名走 retitleChartCard 单卡），避免全量 O(N²)
		void refreshChartCardTitlesForPage(int curvePageId);
		//单卡片重算标题（图表重命名 kName 变化）
		void retitleChartCard(int chartId);
		//关闭图表视口：有视图走 sig_requestCloseChartViewport（主窗口移卡片并释放
		//视图），无视图走 sig_requestCloseChartCard（仅移卡片）；均同步清理登记表（幂等）
		void closeChartViewport(int chartId);
		//图表视图移除的统一收口（closeChartViewport 与视图 destroyed 两路径共用）：
		//清映射；目标视窗关闭时结束编辑曲线会话；最后一个视口关闭时退出注释/
		//基准线模式（广播 off，Ribbon 按钮回弹）
		void handleChartViewRemoved(int chartId);
		//旧MDI"放入当前空视口"通道的残留声明已随懒加载改造移除（新视图架构下
		//恒降级为空卡片登记，见 openChart/materializeChart）
		//把曲线树当前选中同步为指定图表节点（同图守卫：当前选中已属于该图表——自身或
		//其子节点——时不改选，避免视口激活顶掉树上的子节点选中，也阻断联动回环）
		void syncTreeSelectionForChart(int chartId);
		//按目标曲线模型数据（含两轴刻度模式预变换）重算渲染空间数据界；
		//曲线无效/无有效数据返回 false
		bool computeEditBounds(double& minV, double& maxV) const;
		//结束会话并广播（主动退出/目标失效等路径统一收口）
		void endEditSession();
		//--- Phase 2 回收机制内部件 ---
		//回收单个物化视图：经 sig_requestUnmountChartViewport 请求主窗口卸载
		//（pin/可见保护由 isEvictable 判定，调用方先查）。可回收返回 true
		bool evictChart(int chartId);
		//物化数超上限：从 LRU 最久未用端逐个回收直至达标（受保护项自动跳过）
		void evictForCapacity();
		//定时器扫描：空闲超阈值回收 + 超上限回收（5s 周期；挂起时早退）
		void onScanTick();
		//批量物化的排队与步进（materializeForCardIds 分批创建的后半段）：
		//scheduleMaterializeBatch 排一轮 0ms 定时器（幂等去重），
		//materializeBatchStep 每轮至多创建 kMaterializeChunkSize 个新视图
		//（已物化/已失效卡片零成本跳过不占额），余量未完再排下一轮
		void scheduleMaterializeBatch();
		void materializeBatchStep();
		//LRU/空闲计时登记（物化与触碰共用；未物化忽略）
		void touchInternal(int chartId);

		static mCurveViewportFactory* _instance;        //单例
		QHash<int, QWidget*> _chartViews;               //chartId -> mCurveChartView（已物化的图表视图）
		QSet<int> _cardCharts;                          //已登记视口卡片的图表节点id（含已物化；懒加载登记表）
		QHash<int, int> _chartToCardId;                 //chartId -> 视口卡片id（notifyChartCardRegistered 维护）
		QHash<int, int> _cardIdToChart;                 //视口卡片id -> chartId（materializeForCardIds 反查用）
		bool _deferCardActivation = false;              //批量建卡：登记不切页不激活不物化（批量导入宏期间）
		//--- Phase 2：LRU 队列 + 空闲回收 + pin ---
		std::list<int> _lru;                            //物化顺序（尾 = 最近使用；头 = 最久未用，优先回收）
		QHash<int, std::list<int>::iterator> _lruPos;   //chartId -> _lru 迭代器（O(1) 挪位）
		QElapsedTimer _idleClock;                       //单调时钟（构造即启动）
		QHash<int, qint64> _lastTouchMs;                //chartId -> 最近使用时刻（_idleClock 毫秒）
		QSet<int> _pinned;                              //禁止回收集合（当前激活/编辑目标/定义目标/显式 pin）
		QSet<int> _visibleChartIds;                     //当前页可见图表（applyPageLayout 布局应用时整体刷新）
		QList<int> _pendingMaterializeCards;            //分批物化待处理卡片队列（新调用整队替换）
		bool _materializeBatchScheduled = false;        //批量物化定时器已排队（幂等去重）
		QTimer _scanTimer;                              //回收扫描（5s 周期）
		bool _evictionSuspended = false;                //会话恢复等批量场景挂起回收
		int _currentChartId = -1;                       //当前激活卡片承载的图表（notifyChartViewportActivated 维护）
		int _defineChartId = -1;                        //定义曲线模式锁定的目标图表（会话期 pin，防回收）
		bool _hasPlacementOverride = false;             //创建方是否设置了放置策略（宏期间）
		ChartPlacement _placementOverride = ChartPlacement::CreateNew;
		std::function<int()> _viewPageProvider;         //主窗口回填：当前视图页查询（落点解析用）
		std::function<QString(int)> _viewPageNameProvider; //主窗口回填：视图页 id→页名（页名对齐用）
		bool _annotationMode = false;                   //注释模式（全局，见 setAnnotationMode）
		bool _annotation3DMode = false;                 //三维图注释模式（全局，见 setAnnotation3DMode）
		int _pointSeekChartId = -1;                     //寻点标记所在图表节点id（R47，-1=无标记）
		int _pointSeekCurveId = -1;                     //寻点标记所在曲线节点id（R47）
		int _pointSeekPointIndex = -1;                  //寻点标记点下标（R47）
		bool _baselineMode = false;                     //基准线模式（全局，见 setBaselineMode）
		int _baselineOrientation = 1;                   //基准线方向：0 横向 1 纵向
		bool _defineMode = false;                       //定义曲线模式（全局，见 setDefineCurveMode）

		//--- 编辑曲线会话状态（单一数据源，视窗/浮动窗口只读投影） ---
		bool _editMode = false;                         //会话进行中
		int _editChartId = -1;                          //目标图表节点id
		QList<int> _editCurveIds;                       //目标曲线节点id集（首条为主目标）
		int _editDirection = 0;                         //0水平(沿X截取) 1垂直(沿Y截取)
		double _editStart = 0.0;                        //范围下界（渲染空间）
		double _editEnd = 1.0;                          //范围上界（渲染空间）
		double _editStep = 1.0;                         //步进按钮步长
		double _editBoundMin = 0.0;                     //目标曲线数据下界（渲染空间）
		double _editBoundMax = 1.0;                     //目标曲线数据上界（渲染空间）
	};
}
