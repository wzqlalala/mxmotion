#include "mCurveViewportFactory.h"
#include "mCurveChartView.h"
#include "m3DChartView.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../CurveCore/ChartTypeProfile.h"
#include "../CurveTree/mCurveBrowserWindow.h"
#include "../CurveTree/mCurveTreeView.h"

#include <QCoreApplication>
#include <QDebug>

#include "mViewportModelManager.h"
//ViewportType 枚举（MxFunctions）
#include "mBasicEnum.h"

namespace PostCurve
{
	mCurveViewportFactory* mCurveViewportFactory::_instance = nullptr;

	mCurveViewportFactory* mCurveViewportFactory::getInstance()
	{
		if (_instance == nullptr)
			_instance = new mCurveViewportFactory;
		return _instance;
	}

	mCurveViewportFactory::mCurveViewportFactory(QObject* parent)
		: QObject(parent)
	{
		_idleClock.start();
		//控制器批量曲线落地（addCurveBatch）的静默建卡开关：宏期间只登记不切页
		//不激活不物化（控制层经信号协调，不直接依赖本视图层）
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_requestDeferCardActivation,
			this, &mCurveViewportFactory::setDeferCardActivation);
		//回收扫描：空闲超阈值 + 物化数超上限的统一驱动（回调内自检 pin/可见）
		_scanTimer.setInterval(5000);
		connect(&_scanTimer, &QTimer::timeout, this, &mCurveViewportFactory::onScanTick);
		_scanTimer.start();

		mCurveDocController* controller = mCurveDocController::getInstance();
		connect(controller, &mCurveDocController::sig_nodeAdded, this, &mCurveViewportFactory::onNodeAdded);
		//关闭要用 AboutToBeRemoved：此时节点仍在文档中，类型可查
		connect(controller, &mCurveDocController::sig_nodeAboutToBeRemoved, this, [this](int nodeId)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			if (doc->nodeType(nodeId) == CurveNodeType::Chart)
			{
				closeChartViewport(nodeId);
				//剩余同页视口的 w 位置序重排，只重算本页卡片标题
				refreshChartCardTitlesForPage(doc->parentNodeId(nodeId));
			}
			//页删除（子树整体移除只对页节点本身发一次本信号，图表子节点不逐个发）：
			//逐图关闭卡片，避免页删后视口卡片悬挂
			else if (doc->nodeType(nodeId) == CurveNodeType::Page)
			{
				for (int chartId : doc->childNodeIds(nodeId))
				{
					if (doc->nodeType(chartId) == CurveNodeType::Chart)
						closeChartViewport(chartId);
				}
			}
			//编辑曲线会话的失效联动：目标曲线或其所属图表被删（含撤销路径）
			//-> 从目标集剔除并广播；目标集清空（或图表删除）-> 结束会话
			if (_editMode && _editCurveIds.contains(nodeId))
			{
				_editCurveIds.removeOne(nodeId);
				if (_editCurveIds.isEmpty() || nodeId == _editChartId)
					endEditSession();
			else
			{
				emit sig_editCurveTargetsChanged();
				refreshEditBounds();
			}
		}
		else if (_editMode && nodeId == _editChartId)
			endEditSession();
		//寻点标记失效联动（R47）：标记所在曲线/图表被删 → 清除全局标记（视图
		//态随之消失；state-only 收口，目标视图未物化时同样生效；AboutToBeRemoved
		//时机节点类型可查，与上方卡片关闭同口径）
		if (nodeId == _pointSeekCurveId || nodeId == _pointSeekChartId)
			setPointSeekMarker(-1, -1, -1);
		});
			//视口显隐（设计图02 树勾选/图表属性"显示"）：kChartVisible 变化 ->
			//显示/隐藏对应 MDI 视口窗口（不销毁，撤销勾选即恢复）。
			//卡片挂载形态经信号由主窗口代办（叠加呈现集守卫，见信号声明）：
			//工厂直接 setVisible(true) 对溢出隐藏/他页卡片会以过期几何浮于当前
			//布局之上（2026-11-17 布局切换挤压同根钉因）；独立窗口兜底不受布局管，
			//工厂内直接应用
			connect(controller, &mCurveDocController::sig_nodePropertyChanged, this,
				[this](int nodeId, const QString& prop)
			{
				CurveDocument* doc = mCurveDocController::getInstance()->document();
				if (prop == CurveProp::kChartVisible)
				{
					if (doc->nodeType(nodeId) != CurveNodeType::Chart)
						return;
					QWidget* view = _chartViews.value(nodeId, nullptr);
					if (!view)
						return;
					const bool on = doc->nodeProperty(nodeId, CurveProp::kChartVisible).toBool();
					if (view->parentWidget())
						emit sig_requestApplyChartVisibility(nodeId, on);
					else
						view->setVisible(on);
					return;
				}
			//图表重命名 -> 卡片标题跟随（标题=nodeDisplayName 复合显示名，单卡重算）
			if (prop == CurveProp::kName && doc->nodeType(nodeId) == CurveNodeType::Chart)
				retitleChartCard(nodeId);
			//当前图表类型切换（属性编辑器下拉/撤销重做，kChartType 宏内首发）->
			//广播新类型，Ribbon"新建/编辑曲线"等按类型显隐的按钮随之刷新；编辑
			//曲线会话目标图表切到不适用类型（声明见 ChartTypeProfile 注册表）->
			//会话随之结束（同目标失效语义，否则按钮已隐藏而会话仍在进行）
			if (prop == CurveProp::kChartType)
			{
				if (nodeId == _currentChartId)
					emit sig_currentChartTypeChanged(currentChartType());
				if (_editMode && nodeId == _editChartId
					&& ribbonActionHidden(chartTypeProfile(
						doc->nodeProperty(nodeId, CurveProp::kChartType).toInt()),
						ChartTypeProfile::RibbonAction::EditCurve))
					endEditSession();
			}
		});

		//双向联动：MDI 视口激活 -> 曲线树选中对应图表节点。
		//新架构下视口激活经 currentViewportChanged(int) 发 ID，onViewportActivated 仍按
		//QMdiSubWindow 实现（旧架构），暂时断开；待曲线视口接入新视口框架后恢复
		//MechPreData::mViewportModelManager* vmManager = MechPreData::mViewportModelManager::getInstance();
		//if (vmManager)
		//{
		//	connect(vmManager, &MechPreData::mViewportModelManager::currentViewportChanged,
		//		this, &mCurveViewportFactory::onViewportActivated);
		//}
		//双向联动：树选中图表节点（或其子节点）-> 激活其对应视口
		connect(controller, &mCurveDocController::sig_currentNodeChanged,
			this, &mCurveViewportFactory::onCurrentNodeChanged);
	}

	void mCurveViewportFactory::setPlacementOverride(ChartPlacement placement)
	{
		_hasPlacementOverride = true;
		_placementOverride = placement;
	}

	void mCurveViewportFactory::clearPlacementOverride()
	{
		_hasPlacementOverride = false;
		_placementOverride = ChartPlacement::CreateNew;
	}

	void mCurveViewportFactory::openChart(int chartId)
	{
		//幂等：已登记卡片（含已物化）忽略
		if (_cardCharts.contains(chartId) || _chartViews.contains(chartId))
			return;

		mCurveDocController* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();
		if (chartId < 0 || !doc->node(chartId))
			return;
		//视口卡片标题用全限定显示名 "p1w1 : XY Plot"（HyperGraph 风格）：w 序是页内的，
		//多页视口同屏时 "w1 : ..." 会跨页撞名，带页序才唯一（语法与向量引用
		//p1w1c1 同源）；宿主视图树按创建时传入的名字登记，重命名不回写
		QString chartName = doc->nodeFullDisplayName(chartId);
		//图表所属曲线页及其对应视图页（页对齐：-1=请求主窗口新建视图页并回填映射）
		const int curvePageId = doc->parentNodeId(chartId);
		const int viewPageId = controller->pageViewPageId(curvePageId);

		//懒加载第一段：仅登记空卡片（常驻，内容为占位 widget），不创建视图——
		//图表视图由 materializeChart 在"切到该视口所在页/激活该视口/显式需要"时
		//创建并经 sig_requestAttachChartViewport 挂进同一卡片。登记是否立即物化/
		//切页/激活由主窗口按 deferCardActivation 与当前页决定
		_cardCharts.insert(chartId);
		emit sig_requestChartCard(chartId, chartName, curvePageId, viewPageId);
	}

	void mCurveViewportFactory::materializeChart(int chartId)
	{
		//幂等：已物化直接返回；卡片未登记/节点已删时忽略（懒物化只发生在卡片上）
		if (_chartViews.contains(chartId) || !_cardCharts.contains(chartId))
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();
		if (QCoreApplication::closingDown() || !doc->node(chartId))
			return;

		//按图表类型画法分派（RenderKind，ChartTypeProfile 注册表）：Plot3D=
		//三维图（R28 入树）走 m3DChartView（Plot3D 渲染管线），其余走
		//mCurveChartView（Qwt 画布）。两条链共用工厂的登记/LRU/attach/激活/
		//重标题/ChartVisible 体系（以 QWidget 为载体，类型无关）
		const int chartType = doc->nodeProperty(chartId, CurveProp::kChartType).toInt();
		mCurveChartView* curveView = nullptr;
		QWidget* view = nullptr;
		if (chartTypeProfile(chartType).renderKind == ChartTypeProfile::RenderKind::Plot3D)
		{
			auto* view3d = new m3DChartView(chartId);
			connect(view3d, &m3DChartView::sig_requestClose, this, [this, chartId]()
			{
				closeChartViewport(chartId);
			});
			view = view3d;
		}
		else
		{
			//图表视图创建即从文档全量同步（构造 rebuild）：数据/样式/轴（含手动范围）/
			//页眉页脚模板均持久化在文档节点，回收后重建配置无损
			curveView = new mCurveChartView(chartId);
			connect(curveView, &mCurveChartView::sig_requestClose, this, [this, chartId]()
			{
				closeChartViewport(chartId);
			});
			//图上双击/右键"编辑"的弹窗请求中继给主窗口（-> showEditorPopupAt）
			connect(curveView, &mCurveChartView::sig_requestEditorPopup,
				this, &mCurveViewportFactory::sig_requestEditorPopup);
			//Shift+左键点曲线的拾取中继（定义曲线弹窗"数学运算"X/Y 向量引用拾取）：
			//带上图表节点id，监听方无需自行反查
			connect(curveView, &mCurveChartView::sig_curvePicked, this,
				[this, chartId](int curveNodeId)
			{
				emit sig_curvePicked(chartId, curveNodeId);
			});
			//NVH 频率拾取中继（模态贡献量三联动，面板侧 chartId 过滤+三路回写）
			connect(curveView, &mCurveChartView::sig_canvasFrequencyPicked, this,
				[this, chartId](double xValue)
			{
				emit sig_canvasFrequencyPicked(chartId, xValue);
			});
			view = curveView;
		}
		_chartViews.insert(chartId, view);
		touchInternal(chartId);   //新物化即最近使用（LRU 头挪尾 + 空闲计时起点）
		//新视口承接当前全局交互模式（工厂是模式状态唯一持有者，视图构造期只连
		//信号不查状态，不补发则模式开启后新建/晚物化的视口无注释/基准线语义）；
		//三维图承接三维注释模式（R46 起，R42 前无注释语义的旧备案随本轮翻新
		//失效），退出重建后自然一致
		if (curveView)
		{
			curveView->setAnnotationMode(_annotationMode);
			curveView->setBaselineMode(_baselineMode, _baselineOrientation);
		}
		else if (auto* view3d = qobject_cast<m3DChartView*>(view))
		{
			view3d->setAnnotation3DMode(_annotation3DMode);
			//寻点标记承接（R47）：工厂全局单标记状态回放给晚物化三维视图
			//（LRU 回收再物化/晚物化场景标记不丢；状态经 materializeChart
			//统一承接，R46 注释模式同款先例）
			if (_pointSeekChartId == chartId && _pointSeekCurveId > 0
				&& _pointSeekPointIndex >= 0)
				view3d->setSeekMarker(_pointSeekCurveId, _pointSeekPointIndex);
		}
		//视口窗口被用户直接关闭（非节点删除路径）时视图会被销毁，同步清除映射，
		//避免后续按 chartId 取到悬空指针
		connect(view, &QObject::destroyed, this, [this, chartId, view]()
		{
			//同 chartId 复用防护：closeChartViewport 走 deleteLater 延迟销毁，
			//销毁兑现前新视图可能已按同 chartId 重建入表（同进程二次会话恢复：
			//图表节点 id 随文档快照保留），此时表中已非本视图，不得误删新映射
			if (_chartViews.value(chartId, nullptr) == view)
				handleChartViewRemoved(chartId);
		});

		//请求主窗口：占位内容替换为视图（卡片与页登记不动）
		emit sig_requestAttachChartViewport(view, chartId);
		//物化后超上限：立即从 LRU 最久未用端回收（定时器兜底）
		if (_chartViews.size() > kMaxMaterializedViews)
			evictForCapacity();
	}

	void mCurveViewportFactory::materializeForCardIds(const QList<int>& cardIds)
	{
		//可见集整体刷新：本页可见图表受回收保护（空闲/超限回收均跳过）
		_visibleChartIds.clear();
		for (int cardId : cardIds)
		{
			const int chartId = _cardIdToChart.value(cardId, -1);
			if (chartId > 0)
				_visibleChartIds.insert(chartId);
		}
		//分批物化：整页一次同步物化时，各视图 GL 画布
		//（QwtPlotOpenGLCanvas，ImmediatePaint 同步重绘）的创建/挂载/显示与
		//强制刷新全部堆在同一事件循环突发里，同步重绘逐层嵌套，格数越大驱动
		//侧栈消耗越深。首批同步创建 kMaterializeChunkSize 个新视图（落点首图必在
		//首批，activatePageFirstChart 的激活链即用），其余经 0ms 定时器逐批
		//分摊到事件循环，每轮新增可见 GL 画布不超过批大小。已物化卡片零成本
		//幂等跳过不占额；新调用整队替换
		_pendingMaterializeCards.clear();
		int freshCount = 0;
		for (int cardId : cardIds)
		{
			const int chartId = _cardIdToChart.value(cardId, -1);
			const bool wasMaterialized = _chartViews.contains(chartId);
			if (!wasMaterialized && freshCount >= kMaterializeChunkSize)
			{
				_pendingMaterializeCards.append(cardId);
				continue;
			}
			materializeChart(chartId);
			if (!wasMaterialized && _chartViews.contains(chartId))
				++freshCount;
		}
		if (!_pendingMaterializeCards.isEmpty())
			scheduleMaterializeBatch();

		//布局应用后按文档补施隐藏：kChartVisible=false 的图表即使在本页呈现集内
		//也保持隐藏——容器 applyLayout 会整组 setVisible(true)，不补施则"隐藏"
		//在切页往返后静默丢失（新物化视图的初挂载已由主窗口挂载分支按文档补齐，
		//此处覆盖已物化视图被整组重新显示的场景）。只补隐藏、绝不补显示：显示的
		//唯一权威是容器布局应用（呈现集），对列表内溢出隐藏卡片 setVisible(true)
		//会以过期几何浮于当前布局之上（2026-11-17 布局切换曲线图挤压钉因）；
		//勾回显示的即时呈现由属性分支（kChartVisible 变更）经主窗口代办负责。
		//隐藏图表仍留在可见集内不受回收：否则"未物化时勾回显示"会因无视图而
		//丢变更、卡片永停在占位态
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc)
			return;
		for (int cardId : cardIds)
		{
			const int chartId = _cardIdToChart.value(cardId, -1);
			QWidget* view = _chartViews.value(chartId, nullptr);
			if (!view || !view->parentWidget())
				continue;
			if (!doc->nodeProperty(chartId, CurveProp::kChartVisible).toBool())
				view->parentWidget()->setVisible(false);
		}
	}

	//分批物化排队：0ms 定时器逐轮投喂（幂等去重，一轮在队即不重复排）；
	//定时器以本单例为上下文对象，析构后回调自动失效
	void mCurveViewportFactory::scheduleMaterializeBatch()
	{
		if (_materializeBatchScheduled)
			return;
		_materializeBatchScheduled = true;
		QTimer::singleShot(0, this, [this]()
		{
			_materializeBatchScheduled = false;
			materializeBatchStep();
		});
	}

	void mCurveViewportFactory::materializeBatchStep()
	{
		//每轮至多创建 kMaterializeChunkSize 个新视图（已物化/卡片已关/节点已删
		//的队列项经 materializeChart 幂等早退，不计入配额即出队丢弃）；
		//余量未完再排下一轮——批间事件循环得以处理布局与绘制收敛
		int freshCount = 0;
		while (!_pendingMaterializeCards.isEmpty() && freshCount < kMaterializeChunkSize)
		{
			const int cardId = _pendingMaterializeCards.takeFirst();
			const int chartId = _cardIdToChart.value(cardId, -1);
			const bool wasMaterialized = _chartViews.contains(chartId);
			materializeChart(chartId);
			if (!wasMaterialized && _chartViews.contains(chartId))
				++freshCount;
		}
		if (!_pendingMaterializeCards.isEmpty())
			scheduleMaterializeBatch();
	}

	void mCurveViewportFactory::notifyChartCardRegistered(int chartId, int cardId)
	{
		if (chartId <= 0 || cardId < 0)
			return;
		_chartToCardId.insert(chartId, cardId);
		_cardIdToChart.insert(cardId, chartId);
	}

	//--- Phase 2：LRU 队列 + 空闲回收 + pin ---

	void mCurveViewportFactory::touchInternal(int chartId)
	{
		if (chartId <= 0 || !_chartViews.contains(chartId))
			return;
		//挪到 LRU 尾（最近使用），刷新空闲计时起点
		if (_lruPos.contains(chartId))
		{
			_lru.erase(_lruPos.value(chartId));
			_lruPos.remove(chartId);
		}
		_lru.push_back(chartId);
		_lruPos.insert(chartId, std::prev(_lru.end()));
		_lastTouchMs.insert(chartId, _idleClock.msecsSinceReference());
	}

	void mCurveViewportFactory::touchChart(int chartId)
	{
		touchInternal(chartId);
	}

	void mCurveViewportFactory::pinChart(int chartId, bool pinned)
	{
		if (chartId <= 0)
			return;
		if (pinned)
			_pinned.insert(chartId);
		else
			_pinned.remove(chartId);
	}

	bool mCurveViewportFactory::isEvictable(int chartId) const
	{
		if (!_chartViews.contains(chartId))
			return false;                 //未物化无可回收
		if (_evictionSuspended)
			return false;                 //会话恢复等批量场景挂起回收
		if (_pinned.contains(chartId))
			return false;                 //当前激活/编辑目标/定义目标/显式 pin
		if (_visibleChartIds.contains(chartId))
			return false;                 //当前页可见（用户正在看）
		if (_editMode && chartId == _editChartId)
			return false;                 //编辑会话目标双保险
		return true;
	}

	QList<int> mCurveViewportFactory::allCardChartIds() const
	{
		return _cardCharts.values();
	}

	void mCurveViewportFactory::releaseChartView(int chartId)
	{
		//借用后立即回收：pin/可见（当前页在用）图表保留——批量借用不误伤活跃视图
		if (!isEvictable(chartId))
			return;
		QWidget* view = _chartViews.value(chartId, nullptr);
		if (!view)
			return;
		//立即 delete（非 deleteLater）：导出等循环场景逐个释放，内存不随图表数累积；
		//destroyed 联动 handleChartViewRemoved 清登记表（同 chartId 复用守卫适用）
		delete view;
	}

	bool mCurveViewportFactory::evictChart(int chartId)
	{
		if (!isEvictable(chartId))
			return false;
		QWidget* view = _chartViews.value(chartId, nullptr);
		if (!view)
			return false;
		//先摘除 LRU/计时登记（防扫描重复进入）；_chartViews 由视图销毁后的
		//handleChartViewRemoved 清理（deleteLater 兑现前容量计数含在途项）
		if (_lruPos.contains(chartId))
		{
			_lru.erase(_lruPos.value(chartId));
			_lruPos.remove(chartId);
		}
		_lastTouchMs.remove(chartId);
		//请求主窗口卸载：卡片占位回填，视图 deleteLater（卡片/页登记/标题保留）
		emit sig_requestUnmountChartViewport(chartId);
		return true;
	}

	void mCurveViewportFactory::evictForCapacity()
	{
		int over = _chartViews.size() - kMaxMaterializedViews;
		if (over <= 0)
			return;
		//从 LRU 最久未用端（头）收集候选：_lru 头=最早使用、尾=最近使用（touchInternal
		//挪尾）；evictChart 会擦除 _lru 元素，故先收集候选再统一回收，
		//受保护项（pin/可见）跳过；全保护时候选不足即自然收口
		QList<int> candidates;
		for (auto it = _lru.begin(); it != _lru.end() && candidates.size() < over; ++it)
		{
			if (isEvictable(*it))
				candidates.append(*it);
		}
		for (int chartId : candidates)
			evictChart(chartId);
	}

	void mCurveViewportFactory::onScanTick()
	{
		if (_evictionSuspended || QCoreApplication::closingDown())
			return;
		//空闲回收：超阈值未触碰的物化视图（可见/pin 在 evictChart 内拦截）
		const qint64 now = _idleClock.msecsSinceReference();
		QList<int> idle;
		for (auto it = _lastTouchMs.constBegin(); it != _lastTouchMs.constEnd(); ++it)
		{
			if (now - it.value() > idleEvictMs())
				idle.append(it.key());
		}
		for (int chartId : idle)
			evictChart(chartId);
		//容量兜底（物化路径已有即时超限回收，此处覆盖 pin 变化等滞后场景）
		evictForCapacity();
	}

	int mCurveViewportFactory::currentChartId() const
	{
		//新视图架构：图表挂容器卡片（不进渲染视口注册表），当前图表由主窗口监听容器
		//sig_activeViewportChanged 后经 notifyChartViewportActivated 回投维护
		return _currentChartId;
	}

	int mCurveViewportFactory::currentChartType() const
	{
		//非曲线视口/无视口/图表已不在文档中（关闭销毁阶段反查）一律 -1：
		//订阅方按 chartTypeProfile 回退类型（XY，全功能可见）处理
		if (_currentChartId < 0)
			return -1;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (doc->nodeType(_currentChartId) != CurveNodeType::Chart)
			return -1;
		return doc->nodeProperty(_currentChartId, CurveProp::kChartType).toInt();
	}

	void mCurveViewportFactory::notifyChartViewportActivated(QWidget* chartView)
	{
		//chartView=nullPtr 表示激活的不是本模块图表卡片（模型/云图视口等），
		//广播 -1；树上选中保持原状（同旧 onViewportActivated 的非曲线视口语义）
		int chartId = -1;
		if (chartView)
		{
			for (auto it = _chartViews.constBegin(); it != _chartViews.constEnd(); ++it)
			{
				if (it.value() == chartView)
				{
					chartId = it.key();
					break;
				}
			}
		}
		if (chartId > 0)
		{
			touchInternal(chartId);   //激活即触碰（重复激活同图也刷新空闲计时）
			//激活即呈现：纳入可见集防空闲误回收——溢出页替换呈现等经激活链物化的
			//图表不走 materializeForCardIds，缺此步会在 60s 空闲后被回收（呈现中的
			//图表从卡片上消失）。集随下次布局应用整体刷新
			_visibleChartIds.insert(chartId);
		}
		if (_currentChartId == chartId)
			return;
		//pin 随当前激活转移：旧当前解 pin，新当前 pin（当前图表永不被回收）
		if (_currentChartId > 0)
			_pinned.remove(_currentChartId);
		_currentChartId = chartId;
		if (chartId > 0)
			_pinned.insert(chartId);
		//当前图表变化广播（含非曲线视口的 -1）：按钮启停类订阅方（Ribbon"定义曲线"）
		//只依赖本信号；关闭销毁阶段不再联动树选中
		emit sig_currentChartChanged(chartId);
		//激活图表变化伴随类型变化（不同类型图表间切换/激活非曲线视口）：按类型
		//显隐的 Ribbon 按钮据此刷新（-1=回退 XY 全可见）
		emit sig_currentChartTypeChanged(currentChartType());
		if (chartId < 0 || QCoreApplication::closingDown())
			return;
		syncTreeSelectionForChart(chartId);
	}

	QWidget* mCurveViewportFactory::chartViewByName(const QString& chartName) const
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		//遍历 工程->页->视口，按全限定显示名（即建视口时传入的 MDI 标题，含 "p1w1 : " 前缀）查找
		for (int pageId : doc->childNodeIds(doc->rootNodeId()))
		{
			for (int chartId : doc->childNodeIds(pageId))
			{
				if (doc->nodeType(chartId) == CurveNodeType::Chart
					&& doc->nodeFullDisplayName(chartId) == chartName)
				{
					return _chartViews.value(chartId, nullptr);
				}
			}
		}
		return nullptr;
	}

	void mCurveViewportFactory::activatePageFirstChart(int curvePageId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		const int viewPageId = mCurveDocController::getInstance()->pageViewPageId(curvePageId);
		for (int chartId : doc->childNodeIds(curvePageId))
		{
			if (doc->nodeType(chartId) != CurveNodeType::Chart)
				continue;
			if (!_cardCharts.contains(chartId))
				continue;
			//先切页（applyPageLayout 物化本页可见卡片），再物化首图（幂等兜底）并激活
			if (viewPageId >= 0)
				emit sig_requestSwitchViewPage(viewPageId);
			materializeChart(chartId);
			emit sig_requestActivateChartCard(_chartToCardId.value(chartId, -1));
			return;
		}
	}

	mCurveChartView* mCurveViewportFactory::chartView(int chartId) const
	{
		return qobject_cast<mCurveChartView*>(_chartViews.value(chartId, nullptr));
	}

	QWidget* mCurveViewportFactory::chartViewWidget(int chartId) const
	{
		//QWidget 级访问（类型无关）：三维图卡片（m3DChartView，R28 入树）经此
		//参与激活回投/曲线上下文联动；XY 图表仍可经 chartView() 取具体类型
		return _chartViews.value(chartId, nullptr);
	}

	int mCurveViewportFactory::chartIdForView(QWidget* view) const
	{
		if (!view)
			return -1;
		for (auto it = _chartViews.constBegin(); it != _chartViews.constEnd(); ++it)
		{
			if (it.value() == view)
				return it.key();
		}
		return -1;
	}

	QList<int> mCurveViewportFactory::openedChartIds() const
	{
		return _chartViews.keys();
	}

	QString mCurveViewportFactory::chartDisplayName(int chartId) const
	{
		//懒加载：按卡片登记判断（导出"物化→截图→回收"后视图已卸，标题仍可取）
		if (!_cardCharts.contains(chartId))
			return QString();
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		return doc ? doc->nodeFullDisplayName(chartId) : QString();
	}

void mCurveViewportFactory::setAnnotationMode(bool on)
{
	if (_annotationMode == on)
		return;
	//开启前条件：至少存在一个已登记的图表卡片（判据区别于编辑曲线的激活视口
	//检查——本模式目标在点击创建时才确定；懒加载下未物化的卡片也计数，
	//点击空卡片激活链会先物化）；被拒时广播 off 让 Ribbon 按钮回弹
	if (on && _cardCharts.isEmpty())
	{
		mCurveDocController::getInstance()->reportWarning(
			QStringLiteral("注释：请先打开曲线视窗"));
		emit sig_annotationModeChanged(false);
		return;
	}
	_annotationMode = on;
		//互斥：一次只允许一种"注释"激活——开启二维注释模式先退出三维图注释
		//模式（同为注释概念，R46 起；直接置位+广播，回弹三维 Ribbon 按钮）
		if (on && _annotation3DMode)
		{
			_annotation3DMode = false;
			emit sig_annotation3DModeChanged(false);
		}
		//互斥：开启注释模式先退出基准线模式（两者共用画布左键语义与 snapper）；
		//先关基准线再广播注释开关，Ribbon 两个按钮的按下态各经其信号同步
		if (on && _baselineMode)
		{
			_baselineMode = false;
			emit sig_baselineModeChanged(false, _baselineOrientation);
		}
		//互斥：一次只允许一种功能——开启注释同时退出编辑曲线会话与定义曲线
		//模式（经公开接口关闭，联动信号齐全）
		if (on && _editMode)
			endEditSession();
		if (on && _defineMode)
			setDefineCurveMode(false);
		emit sig_annotationModeChanged(on);
	}

	void mCurveViewportFactory::setAnnotation3DMode(bool on)
	{
		if (_annotation3DMode == on)
			return;
		//开启前条件：至少存在一个已登记的三维图卡片（chartType==5，含未物化
		//——判据同二维注释模式的"已登记卡片计数"口径；按钮启停联动已保证
		//当前激活图表为三维图才可点击，此处防御兜底）；被拒时广播 off 让
		//Ribbon 按钮回弹
		if (on)
		{
			CurveDocument* doc = mCurveDocController::getInstance()->document();
			bool has3D = false;
			for (int chartId : _cardCharts)
			{
				const CurveNode* chart = doc ? doc->node(chartId) : nullptr;
				if (chart && chart->type == CurveNodeType::Chart
					&& chart->chart.chartType == 5)
				{
					has3D = true;
					break;
				}
			}
			if (!has3D)
			{
				mCurveDocController::getInstance()->reportWarning(
					QStringLiteral("注释：请先打开三维图视窗"));
				emit sig_annotation3DModeChanged(false);
				return;
			}
		}
		_annotation3DMode = on;
		//互斥：一次只允许一种"注释"激活——开启三维图注释模式先退出二维注释
		//模式（经其公开接口关闭，联动信号齐全；二维侧同步踢出基准线/编辑/
		//定义等互斥模式）
		if (on && _annotationMode)
			setAnnotationMode(false);
		emit sig_annotation3DModeChanged(on);
	}

	void mCurveViewportFactory::setPointSeekMarker(int chartId, int curveId, int pointIndex)
	{
		//非法入参归一为"无标记"（pointIndex<0 = 清除口径）
		if (chartId <= 0 || curveId <= 0 || pointIndex < 0)
			chartId = curveId = pointIndex = -1;
		//同值守卫（幂等：重复寻同一点不重复重摆）
		if (chartId == _pointSeekChartId && curveId == _pointSeekCurveId
			&& pointIndex == _pointSeekPointIndex)
			return;
		//跨图寻点：旧图已物化视图先清标记（工厂全局仅一个标记，旧图标记被
		//新状态替换清除；旧视图未物化则无事可做——状态即真相）
		if (_pointSeekChartId > 0 && _pointSeekChartId != chartId)
		{
			if (auto* oldView = qobject_cast<m3DChartView*>(
				_chartViews.value(_pointSeekChartId, nullptr)))
				oldView->setSeekMarker(-1, -1);
		}
		_pointSeekChartId = chartId;
		_pointSeekCurveId = curveId;
		_pointSeekPointIndex = pointIndex;
		//新图视图承接：仅 m3DChartView 有标记语义（数值页仅三维图，非三维
		//视图不该到达——qobject_cast 失败安全短路不崩溃）
		if (chartId > 0)
		{
			if (auto* view = qobject_cast<m3DChartView*>(_chartViews.value(chartId, nullptr)))
				view->setSeekMarker(curveId, pointIndex);
		}
	}

	void mCurveViewportFactory::setBaselineMode(bool on, int orientation)
	{
	//同向重复开启/退出方向的重复关闭不重复广播
	if (_baselineMode == on && (!on || _baselineOrientation == orientation))
		return;
	//开启前条件：同注释模式（按已登记卡片计数，未物化也计数）；检查先于互斥
	//踢出——开启被拒不应误伤已开着的注释模式
	if (on && _cardCharts.isEmpty())
	{
		mCurveDocController::getInstance()->reportWarning(
			QStringLiteral("基准线：请先打开曲线视窗"));
		emit sig_baselineModeChanged(false, _baselineOrientation);
		return;
	}
	//互斥：开启基准线模式先退出注释模式（经其公开接口，联动信号齐全；
		//先于 _baselineMode 置位，保证其内部广播的基准线关闭信号与状态一致）
		if (on && _annotationMode)
			setAnnotationMode(false);
		//互斥：一次只允许一种功能——开启基准线同时退出编辑曲线会话与定义
		//曲线模式（经公开接口关闭，联动信号齐全）
		if (on && _editMode)
			endEditSession();
		if (on && _defineMode)
			setDefineCurveMode(false);
		_baselineMode = on;
		if (on)
			_baselineOrientation = orientation;
		emit sig_baselineModeChanged(_baselineMode, _baselineOrientation);
	}

	QWidget* mCurveViewportFactory::editCurveView() const
	{
		return _chartViews.value(_editChartId, nullptr);
	}

	bool mCurveViewportFactory::computeEditBounds(double& minV, double& maxV) const
	{
		if (_editCurveIds.isEmpty() || _editChartId <= 0)
			return false;
		CurveDocument* doc = mCurveDocController::getInstance()->document();

		//两轴刻度/计权模式与 dB 参考值（与视窗 applySeries 的预变换一致，保证界值与渲染对齐）
		int xMode = 0;
		int yMode = 0;
		int xWeight = 0;
		int yWeight = 0;
		double xDbRef = 1.0;
		double yDbRef = 1.0;
		for (int childId : doc->childNodeIds(_editChartId))
		{
			CurveNode* n = doc->node(childId);
			if (!n)
				continue;
			if (n->type == CurveNodeType::XAxis)
			{
				xMode = n->axis.scaleMode;
				xWeight = n->axis.weightMode;
				xDbRef = n->axis.dbRef;
			}
			else if (n->type == CurveNodeType::YAxis)
			{
				yMode = n->axis.scaleMode;
				yWeight = n->axis.weightMode;
				yDbRef = n->axis.dbRef;
			}
		}

		//界值取全部目标曲线的并集（裁剪线范围对每条目标同语义，须都能覆盖）
		bool hasValue = false;
		for (int curveId : _editCurveIds)
		{
			CurveNode* curve = doc->node(curveId);
			if (!curve || curve->type != CurveNodeType::Curve)
				continue;
			const CurveSeriesData& s = curve->series;
			const int count = qMin(s.xData.size(), s.yData.size());
			for (int i = 0; i < count; ++i)
			{
				//任一侧变换非法（<=0 取对数）渲染时会被过滤，界值只按可见点算；
				//与渲染同源：先经 x/y 侧 缩放系数->偏移量 预变换再计权/刻度变换
				double tx = 0.0, ty = 0.0;
				mCurveChartView::transformPoint(s.xRef.applyTo(s.xData.at(i)),
					s.yRef.applyTo(s.yData.at(i)),
					xMode, yMode, xWeight, yWeight, tx, ty, xDbRef, yDbRef);
				if (qIsNaN(tx) || qIsNaN(ty))
					continue;
				const double v = (_editDirection == 0) ? tx : ty;
				if (!hasValue)
				{
					minV = maxV = v;
					hasValue = true;
				}
				else
				{
					minV = qMin(minV, v);
					maxV = qMax(maxV, v);
				}
			}
		}
		return hasValue;
	}

	void mCurveViewportFactory::setDefineCurveMode(bool on)
	{
		if (_defineMode == on)
			return;
		if (on)
		{
			//开启：目标=当前激活视口承载的图表（Ribbon 按钮态已保证曲线视窗
			//激活，此处双保险）；开启失败也发结束信号：Ribbon 按钮回弹
			const int chartId = currentChartId();
			if (chartId < 0)
			{
				mCurveDocController::getInstance()->reportWarning(QStringLiteral(
					"[定义曲线] 请先激活一个曲线视窗"));
				emit sig_defineCurveStopped();
				return;
			}
			//互斥：一次只允许一种功能——开启定义曲线先退出编辑曲线会话与
			//注释/基准线模式（校验通过后才退出，开启失败不波及已开启的功能；
			//各自经公开接口关闭，联动信号齐全）
			setEditCurveMode(false);
			setAnnotationMode(false);
			setBaselineMode(false);
			_defineMode = true;
			_defineChartId = chartId;
			_pinned.insert(chartId);   //定义目标 pin：会话期不回收（浮动窗口持有目标语义）
			emit sig_defineCurveStarted(chartId);
		}
		else
		{
			_defineMode = false;
			if (_defineChartId > 0)
				_pinned.remove(_defineChartId);   //会话目标解 pin
			_defineChartId = -1;
			emit sig_defineCurveStopped();
		}
	}

	void mCurveViewportFactory::setEditCurveMode(bool on)
	{
		if (on)
		{
			if (_editMode)
				return;
			//开启：锁定当前激活图表与其第一条曲线（树序第一条，UI设计图16）
			mCurveDocController* controller = mCurveDocController::getInstance();
			CurveDocument* doc = controller->document();
			const int chartId = currentChartId();
			int curveId = -1;
			if (chartId > 0)
			{
				for (int childId : doc->childNodeIds(chartId))
				{
					CurveNode* n = doc->node(childId);
					if (n && n->type == CurveNodeType::Curve)
					{
						curveId = childId;
						break;
					}
				}
			}
			if (chartId < 0 || curveId < 0)
			{
				controller->reportWarning(QStringLiteral(
					"编辑曲线：请先激活包含曲线的图表视窗"));
				emit sig_editCurveStopped();   //开启失败也发结束信号：Ribbon 按钮回弹
				return;
			}
			_editChartId = chartId;
			_editCurveIds = QList<int>{ curveId };
			_editDirection = 0;   //默认水平（竖直裁剪线，沿X截取）
			_editStep = 1.0;
			double minV = 0.0;
			double maxV = 1.0;
			if (!computeEditBounds(minV, maxV))
			{
				controller->reportWarning(QStringLiteral("编辑曲线：目标曲线无有效数据"));
				_editChartId = -1;
				_editCurveIds.clear();
				emit sig_editCurveStopped();
				return;
			}
			_editBoundMin = minV;
			_editBoundMax = maxV;
			_editStart = minV;   //初始范围为曲线全量（两条裁剪线贴曲线两端）
			_editEnd = maxV;
			//互斥：一次只允许一种功能——开启编辑曲线先退出定义曲线模式与
			//注释/基准线模式（至此校验已全部通过，开启失败路径不会误关已
			//开启的功能；各自经公开接口关闭，联动信号齐全）
			setDefineCurveMode(false);
			setAnnotationMode(false);
			setBaselineMode(false);
			_editMode = true;
			_pinned.insert(_editChartId);   //会话目标 pin：切页浏览期间目标视图不被回收
			emit sig_editCurveStarted(_editChartId);
		}
		else
		{
			if (!_editMode)
				return;
			endEditSession();
		}
	}

	void mCurveViewportFactory::endEditSession()
	{
		_editMode = false;
		if (_editChartId > 0)
			_pinned.remove(_editChartId);   //会话目标解 pin（恢复正常回收资格）
		_editChartId = -1;
		_editCurveIds.clear();
		emit sig_editCurveStopped();
	}

	void mCurveViewportFactory::setEditTargetCurve(int curveNodeId)
	{
		//Ctrl+点选：单条替换整个目标集
		setEditTargetCurves(QList<int>{ curveNodeId });
	}

	void mCurveViewportFactory::setEditTargetCurves(const QList<int>& curveNodeIds)
	{
		if (!_editMode || curveNodeIds.isEmpty())
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		QList<int> valid;
		for (int curveNodeId : curveNodeIds)
		{
			if (curveNodeId <= 0 || valid.contains(curveNodeId))
				continue;
			CurveNode* n = doc->node(curveNodeId);
			if (!n || n->type != CurveNodeType::Curve)
				continue;
			//本期限本图表内换目标（设计图预留跨视窗同时编辑，待扩展）
			if (doc->chartNodeId(curveNodeId) != _editChartId)
				continue;
			valid.append(curveNodeId);
		}
		if (valid.isEmpty() || valid == _editCurveIds)
			return;
		_editCurveIds = valid;
		emit sig_editCurveTargetsChanged();
		refreshEditBounds();   //换目标数据界随之变化：复位全量范围（内部再发范围信号）
	}

	void mCurveViewportFactory::setEditRange(double start, double end)
	{
		if (!_editMode)
			return;
		//各自先钳制到数据界；交叉时"变动的一侧"优先（拖拽/输入语义：
		//用户正在动哪条裁剪线，哪条生效，另一条被压住）
		start = qBound(_editBoundMin, start, _editBoundMax);
		end = qBound(_editBoundMin, end, _editBoundMax);
		if (start > end)
		{
			const bool startMoved = !qFuzzyCompare(start + 1.0, _editStart + 1.0);
			if (startMoved)
				end = start;
			else
				start = end;
		}
		if (qFuzzyCompare(start + 1.0, _editStart + 1.0)
			&& qFuzzyCompare(end + 1.0, _editEnd + 1.0))
			return;
		_editStart = start;
		_editEnd = end;
		emit sig_editCurveRangeChanged(start, end);
	}

	void mCurveViewportFactory::setEditDirection(int direction)
	{
		if (!_editMode)
			return;
		const int dir = (direction == 1) ? 1 : 0;
		if (_editDirection == dir)
			return;
		_editDirection = dir;
		emit sig_editCurveDirectionChanged(_editDirection);
		refreshEditBounds();   //换截取轴后数据界不同：重算并复位全量范围
	}

	void mCurveViewportFactory::setEditStep(double step)
	{
		if (!_editMode)
			return;
		_editStep = qMax(0.0, step);
	}

	void mCurveViewportFactory::stepEditBound(int bound, int dir, bool jump)
	{
		if (!_editMode || (bound != 0 && bound != 1))
			return;
		if (jump)
		{
			//直达数据端：dir<0 -> 数据下界（|<）/ dir>0 -> 数据上界（>|），
			//起始/终止两侧行为一致；越界与交叉（追过另一侧时对侧让位）
			//由 setEditRange 的钳制/让位语义收口
			const double target = (dir < 0) ? _editBoundMin : _editBoundMax;
			if (bound == 0)
				setEditRange(target, _editEnd);
			else
				setEditRange(_editStart, target);
			return;
		}
		const double delta = (dir < 0 ? -1.0 : 1.0) * _editStep;
		if (bound == 0)
			setEditRange(_editStart + delta, _editEnd);
		else
			setEditRange(_editStart, _editEnd + delta);
	}

	void mCurveViewportFactory::refreshEditBounds()
	{
		if (!_editMode)
			return;
		double minV = 0.0;
		double maxV = 1.0;
		if (!computeEditBounds(minV, maxV))
		{
			//数据被裁空/曲线失效：会话无法继续，收口结束
			mCurveDocController::getInstance()->reportInfo(QStringLiteral(
				"编辑曲线：目标曲线已无有效数据，编辑结束"));
			endEditSession();
			return;
		}
		_editBoundMin = minV;
		_editBoundMax = maxV;
		_editStart = minV;   //复位全量范围（两条裁剪线重新贴曲线两端）
		_editEnd = maxV;
		emit sig_editCurveRangeChanged(_editStart, _editEnd);
	}

	void mCurveViewportFactory::onNodeAdded(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (doc->nodeType(nodeId) == CurveNodeType::Chart)
		{
			//懒加载：只登记空卡片（是否立即物化由主窗口按当前页/批处理模式决定）；
			//新卡片改变同页兄弟的 w 位置序，只重算本页卡片标题（避免全量 O(N²)）
			openChart(nodeId);
			refreshChartCardTitlesForPage(doc->parentNodeId(nodeId));
		}
	}

	void mCurveViewportFactory::refreshChartCardTitlesForPage(int curvePageId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc)
			return;
		//只重算本页已登记卡片的标题（w 序是页内的，跨页互不影响）
		for (int chartId : doc->childNodeIds(curvePageId))
		{
			if (!_cardCharts.contains(chartId))
				continue;
			emit sig_requestRetitleChartCard(chartId, doc->nodeFullDisplayName(chartId));
		}
	}

	void mCurveViewportFactory::retitleChartCard(int chartId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc || !_cardCharts.contains(chartId))
			return;
		emit sig_requestRetitleChartCard(chartId, doc->nodeFullDisplayName(chartId));
	}

void mCurveViewportFactory::handleChartViewRemoved(int chartId)
{
	//remove 幂等：closeChartViewport 与视图 destroyed 两路径可安全重复进入
	_chartViews.remove(chartId);
	//回收登记同步清理（LRU/计时/pin/可见集；closeChartViewport 已清理时为幂等空操作）
	if (_lruPos.contains(chartId))
	{
		_lru.erase(_lruPos.value(chartId));
		_lruPos.remove(chartId);
	}
	_lastTouchMs.remove(chartId);
	_pinned.remove(chartId);
	_visibleChartIds.remove(chartId);
	//编辑曲线会话的目标视窗被关闭 -> 结束会话（浮动窗口隐藏/按钮回弹）
	if (_editMode && chartId == _editChartId)
		endEditSession();
	//定义曲线模式的目标图表被关闭 -> 退出模式（同编辑曲线的失效联动）
	if (_defineMode && chartId == _currentChartId)
		setDefineCurveMode(false);
	//当前激活图表被关闭 -> 广播 -1（按钮启停类订阅方随信号复位）
	if (chartId == _currentChartId)
	{
		_currentChartId = -1;
		emit sig_currentChartChanged(-1);
	}
	//最后一个视口关闭：注释/基准线/三维注释模式无处生效，广播退出（按钮随
	//信号回弹）
	if (_chartViews.isEmpty())
	{
		if (_annotationMode)
			setAnnotationMode(false);
		if (_annotation3DMode)
			setAnnotation3DMode(false);
		if (_baselineMode)
			setBaselineMode(false, _baselineOrientation);
	}
}

void mCurveViewportFactory::closeChartViewport(int chartId)
{
	const bool hasView = _chartViews.contains(chartId);
	if (!hasView && !_cardCharts.contains(chartId))
		return;   //既无视图也无卡片：幂等返回
	//登记表同步清理（卡片与视图生命周期从此解耦：视图销毁经 destroyed 联动
	//handleChartViewRemoved 清 _chartViews；卡片登记表在此收口）
	_cardCharts.remove(chartId);
	const int cardId = _chartToCardId.take(chartId);
	if (cardId >= 0)
		_cardIdToChart.remove(cardId);
	_pinned.remove(chartId);
	_visibleChartIds.remove(chartId);
	//编辑曲线会话/定义曲线模式/当前激活图表的目标被关闭 -> 失效联动（同
	//handleChartViewRemoved 语义；无视图路径也必须收口）
	if (_editMode && chartId == _editChartId)
		endEditSession();
	if (_defineMode && chartId == _currentChartId)
		setDefineCurveMode(false);
	if (chartId == _currentChartId)
	{
		_currentChartId = -1;
		emit sig_currentChartChanged(-1);
	}
	if (hasView)
	{
		//有视图：主窗口移卡片并释放视图（视图 destroyed 联动清 _chartViews）
		emit sig_requestCloseChartViewport(chartId);
	}
	else
	{
		//懒加载下未物化：仅移除空卡片（占位内容随卡片释放，无视图可释放）
		emit sig_requestCloseChartCard(chartId);
	}
	//最后一个卡片关闭：注释/基准线/三维注释模式无处生效，广播退出（按钮随
	//信号回弹）
	if (_cardCharts.isEmpty())
	{
		if (_annotationMode)
			setAnnotationMode(false);
		if (_annotation3DMode)
			setAnnotation3DMode(false);
		if (_baselineMode)
			setBaselineMode(false, _baselineOrientation);
	}
}

	//旧MDI架构的视口激活通道（QMdiSubWindow 反查）。新视图架构下由主窗口监听容器
	//sig_activeViewportChanged 后回投 notifyChartViewportActivated（含非曲线视口的 -1
	//广播与树上选中联动，语义同下），本槽与 chartIdForSubWindow 不再需要
	//void mCurveViewportFactory::onViewportActivated(QMdiSubWindow* subWindow)
	//{
	//	if (QCoreApplication::closingDown() || !subWindow)
	//		return;
	//	int chartId = chartIdForSubWindow(subWindow);
	//	emit sig_currentChartChanged(chartId);
	//	if (chartId < 0)
	//		return;
	//	syncTreeSelectionForChart(chartId);
	//}

	void mCurveViewportFactory::onCurrentNodeChanged(int nodeId)
	{
		if (QCoreApplication::closingDown())
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();
		//页节点选中 -> 切换到其对应的视图页（曲线页与视图页对齐；未绑定页忽略）
		if (doc->nodeType(nodeId) == CurveNodeType::Page)
		{
			const int vp = controller->pageViewPageId(nodeId);
			if (vp >= 0)
				emit sig_requestSwitchViewPage(vp);
			return;
		}
		//图表（或其子节点）选中仅驱动参数栏等选择联动，不激活视口卡片：
		//激活与溢出页替换呈现由双击显式发起（activateChartViewport），防单击误触
	}

	//产出链落位收口（chart-viewport-placement 单写者）：换算卡片/视图页后交主窗口
	//一次性落位。未登记卡片/未绑定视图页的图表跳过；集合全空时不发请求
	void mCurveViewportFactory::presentProducedCharts(int curvePageId, const QList<int>& chartIds)
	{
		QList<int> cardIds;
		for (int chartId : chartIds)
		{
			const int cardId = _chartToCardId.value(chartId, -1);
			if (cardId >= 0)
				cardIds << cardId;
		}
		const int viewPageId = mCurveDocController::getInstance()->pageViewPageId(curvePageId);
		if (cardIds.isEmpty() || viewPageId < 0)
			return;
		emit sig_requestPresentCharts(viewPageId, cardIds);
	}

	//静默登记：换算同 presentProducedCharts（chartId→卡片id、曲线页→视图页），
	//主窗口侧做呈现集冻结+kLayout 回写（不呈现）；集合全空/未绑定视图页时不发请求
	void mCurveViewportFactory::registerChartsOnPage(int curvePageId, const QList<int>& chartIds)
	{
		QList<int> cardIds;
		for (int chartId : chartIds)
		{
			const int cardId = _chartToCardId.value(chartId, -1);
			if (cardId >= 0)
				cardIds << cardId;
		}
		const int viewPageId = mCurveDocController::getInstance()->pageViewPageId(curvePageId);
		if (cardIds.isEmpty() || viewPageId < 0)
			return;
		emit sig_requestRegisterChartsOnPage(viewPageId, cardIds);
	}

	void mCurveViewportFactory::activateChartViewport(int nodeId)
	{
		if (QCoreApplication::closingDown())
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		//图表节点（自身或其子节点）对应视口；页/工程根等非图表节点忽略
		const int chartId = doc->chartNodeId(nodeId);
		if (chartId < 0 || !_cardCharts.contains(chartId))
			return;
		//懒加载：未物化的卡片先切到其所在页（applyPageLayout 物化本页可见卡片）、
		//物化自身（幂等兜底）再激活——对齐 HyperViewer"切到该视口才创建图表"
		QWidget* view = _chartViews.value(chartId, nullptr);
		if (!view)
		{
			const int curvePageId = doc->parentNodeId(chartId);
			const int viewPageId = mCurveDocController::getInstance()->pageViewPageId(curvePageId);
			if (viewPageId >= 0)
				emit sig_requestSwitchViewPage(viewPageId);
			materializeChart(chartId);
			emit sig_requestActivateChartCard(_chartToCardId.value(chartId, -1));
			return;
		}
		//已物化：独立窗口兜底时直接置前，否则经主窗口激活其卡片
		if (view->isWindow())
		{
			view->show();
			view->raise();
			view->activateWindow();
		}
		else
		{
			emit sig_requestActivateChartCard(_chartToCardId.value(chartId, -1));
		}
	}

	void mCurveViewportFactory::syncTreeSelectionForChart(int chartId)
	{
		if (chartId < 0)
			return;
		mCurveDocController* controller = mCurveDocController::getInstance();
		//同图守卫：树上已选中该图表（自身或其子节点）时保持不动——
		//1) 视口激活不顶掉用户在树上的子节点选中（曲线/坐标轴等，参数栏页面随之保留）；
		//2) 阻断 树->视口->树 联动回环（激活引发的视口变化信号在此终止）
		if (controller->document()->chartNodeId(controller->currentNodeId()) == chartId)
			return;
		mCurveBrowserWindow::getInstance()->getCurveTreeView()->selectNode(chartId);
	}
}
