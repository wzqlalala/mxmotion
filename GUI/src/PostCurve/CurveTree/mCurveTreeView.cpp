#include "mCurveTreeView.h"
#include "../mCurveTreeSelectDelegate.h"   //行选中条代理（浅蓝圆角条，与选取器/定义曲线树共用）
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/ChartTypeProfile.h"   //右键菜单项清单按图表类型声明查表
#include "../CurveCore/mCurveDocController.h"
#include "../CurveView/mCurveViewportFactory.h"   //双击图表节点激活视口卡片
#include "mTreeDelegate.h"   //层级虚线导引线（与视图树/前处理树同源）

#include <QPalette>
#include <QMenu>
#include "mUiMenu.h"
#include <QTimer>
#include <QShortcut>
#include <QKeySequence>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QCoreApplication>

namespace PostCurve
{
	namespace
	{
		//可见性切换菜单项（属性键按类型映射，见 CurveDefine.h::nodeVisibleToggleProp），
		//切换经控制器走命令栈（可撤销）
		void addToggleVisibleAction(QMenu& menu, mCurveDocController* controller, CurveDocument* doc,
			int nodeId, CurveNodeType type)
		{
			const QString prop = nodeVisibleToggleProp(type);
			const bool vis = doc->nodeProperty(nodeId, prop).toBool();
			QAction* act = menu.addAction(vis ? QStringLiteral("隐藏") : QStringLiteral("显示"));
			QObject::connect(act, &QAction::triggered, &menu, [controller, nodeId, prop, vis]()
			{
				controller->setNodeProperty(nodeId, prop, !vis);
			});
		}
	}

//--- mCurveTreeFilterProxy ---

	namespace
	{
		//骨架节点（工程/页/视口）：类型筛选时始终显示，保证树结构完整
		bool isSkeletonType(int type)
		{
			return type == static_cast<int>(CurveNodeType::Project)
				|| type == static_cast<int>(CurveNodeType::Page)
				|| type == static_cast<int>(CurveNodeType::Chart);
		}
	}

	mCurveTreeFilterProxy::mCurveTreeFilterProxy(QObject* parent)
		: QSortFilterProxyModel(parent)
	{
	}

	void mCurveTreeFilterProxy::setAllowedTypes(const QSet<int>& types)
	{
		_allowedTypes = types;
		invalidateFilter();
	}

	void mCurveTreeFilterProxy::clearFilter()
	{
		_allowedTypes.clear();
		invalidateFilter();
	}

	void mCurveTreeFilterProxy::setFilterText(const QString& text)
	{
		if (_filterText == text)
			return;
		_filterText = text;
		invalidateFilter();
	}

	bool mCurveTreeFilterProxy::rowAcceptedByType(const QModelIndex& idx) const
	{
		if (_allowedTypes.isEmpty())
			return true;
		int type = idx.data(CurveDocument::NodeTypeRole).toInt();
		if (isSkeletonType(type))
			return true;
		return _allowedTypes.contains(type);
	}

	bool mCurveTreeFilterProxy::rowNameMatches(const QModelIndex& idx) const
	{
		//按节点本名匹配：页/视口的 DisplayRole 带 "p1 : " 位置前缀，直接拿来搜
		//会让搜 "p1" 误命中所有页，经 NodeIdRole 取回本名再比
		const CurveDocument* doc = qobject_cast<const CurveDocument*>(sourceModel());
		const QString name = doc
			? doc->nodeName(idx.data(CurveDocument::NodeIdRole).toInt())
			: idx.data(Qt::DisplayRole).toString();
		return name.contains(_filterText, Qt::CaseInsensitive);
	}

	bool mCurveTreeFilterProxy::subtreeHasVisibleMatch(const QModelIndex& idx) const
	{
		const int rows = sourceModel()->rowCount(idx);
		for (int i = 0; i < rows; ++i)
		{
			QModelIndex child = sourceModel()->index(i, 0, idx);
			if (!rowAcceptedByType(child))
				continue;
			if (rowNameMatches(child))
				return true;
			if (subtreeHasVisibleMatch(child))
				return true;
		}
		return false;
	}

	bool mCurveTreeFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
	{
		if (_allowedTypes.isEmpty() && _filterText.isEmpty())
			return true;
		QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
		if (!idx.isValid())
			return true;
		if (!rowAcceptedByType(idx))
			return false;
		//未启用名称搜索：类型筛选（骨架节点始终显示，只筛选叶子层）
		if (_filterText.isEmpty())
			return true;
		//名称搜索：自身命中，或子树有命中节点时保留祖先链
		//（QSortFilterProxyModel 父行被拒则整棵子树不可见，故祖先须显式接受）
		if (rowNameMatches(idx))
			return true;
		return subtreeHasVisibleMatch(idx);
	}

	//--- mCurveTreeView ---

	mCurveTreeView::mCurveTreeView(QWidget* parent)
		: QTreeView(parent)
	{
		setMouseTracking(true);
		viewport()->setMouseTracking(true);
		viewport()->setAttribute(Qt::WA_Hover);

		//展开防抖定时器（0ms + singleShot + restart 合并；见 _expandDebounce 注释）
		_expandDebounce.setSingleShot(true);
		_expandDebounce.setInterval(0);
		connect(&_expandDebounce, &QTimer::timeout, this, &QTreeView::expandAll);

		initCurveTreeView();
	}

	mCurveTreeView::~mCurveTreeView()
	{
	}

	void mCurveTreeView::drawBranches(QPainter* painter, const QRect& rect, const QModelIndex& index) const
	{
		//父子节点虚线连接 + 展开箭头（与视图树/前处理树同款），原生箭头经离屏渲染后贴近节点
		MechMainWindow::PaintDashBranches(this, painter, rect, index,
			[this](QPainter* p, const QRect& r, const QModelIndex& i) { QTreeView::drawBranches(p, r, i); });
	}

	void mCurveTreeView::initCurveTreeView()
	{
		//与 PaintDashBranches 箭头口径配套：箭头对齐父行符号列、且距自身内容约2px
		setIndentation(14);
		setStyleSheet(R"(
        QTreeView{
        border:none;
        }
        )");
	//行代理：选中条自绘（见 CurveTreeItemDelegate），不用原生深蓝选中底
	setItemDelegate(new CurveTreeItemDelegate(this));
	//原生 drawBranches 会用 Highlight 色画选中行的分支区底色（选中行左侧深蓝块），
	//与视图树同款置白屏蔽（选中条已由行代理自绘）
	QPalette pal = palette();
	pal.setColor(QPalette::Highlight, Qt::white);
	setPalette(pal);

		//过滤代理 + 文档模型
		_proxyModel = new mCurveTreeFilterProxy(this);
		_proxyModel->setSourceModel(mCurveDocController::getInstance()->document());
		setModel(_proxyModel);

	setHeaderHidden(true);
	//图标区与模型 DecorationRole 的组合图标同尺寸（绿√ 12 + 类型图标 16 = 28 逻辑像素）：
	//否则视图会把宽图等比压进默认 16px 图标区，对勾与类型图标一起被缩小
	setIconSize(QSize(28, 16));
	//树内重命名（设计图02）：双击/F2 进入编辑（仅可命名类型，flags 已按类型过滤；
	//提交经模型 setData -> kName 命令，曲线同步图例键 series.name）
	setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
		setUniformRowHeights(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setSelectionBehavior(QAbstractItemView::SelectRows);

	//选中 -> 选择中枢
	connect(selectionModel(), &QItemSelectionModel::selectionChanged,
		this, &mCurveTreeView::onSelectionChanged);

	//反向联动：选择中枢变化（图上点选曲线/图例/点空白清选）-> 树回选或清选。
	//来源为树自身时目标节点已是当前选中，selectNode 无实际变化；
	//程序化选中触发的 onSelectionChanged 会以同值再次写入中枢，被同值短路吸收
	connect(mCurveDocController::getInstance(), &mCurveDocController::sig_currentNodeChanged,
		this, &mCurveTreeView::onCurrentNodeChanged);

		//节点新增后自动展开（延迟到事件循环，批量插入只展开一次）
		connect(_proxyModel, &QAbstractItemModel::rowsInserted, this, &mCurveTreeView::onRowsInserted);

		//撤销/重做快捷键（曲线树获得焦点时生效）
		QShortcut* undoShortcut = new QShortcut(QKeySequence::Undo, this);
		connect(undoShortcut, &QShortcut::activated, this, []()
		{
			mCurveDocController::getInstance()->undo();
		});
		QShortcut* redoShortcut = new QShortcut(QKeySequence::Redo, this);
		connect(redoShortcut, &QShortcut::activated, this, []()
		{
			mCurveDocController::getInstance()->redo();
		});
	}

	int mCurveTreeView::currentNodeId() const
	{
		QModelIndex cur = currentIndex();
		if (!cur.isValid())
			return -1;
		return cur.data(CurveDocument::NodeIdRole).toInt();
	}

	void mCurveTreeView::selectNode(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		QModelIndex sourceIdx = doc->indexFromNodeId(nodeId);
		if (!sourceIdx.isValid())
			return;
		QModelIndex proxyIdx = _proxyModel->mapFromSource(sourceIdx);
		if (!proxyIdx.isValid())
			return;
		//展开祖先（根节点id为0，页为顶层项无需展开），保证程序化选中的节点可见
		//（树虽在节点插入后自动全展开，但用户可能已手动收起）
		for (int pid = doc->parentNodeId(nodeId); pid > 0; pid = doc->parentNodeId(pid))
		{
			QModelIndex ancestorIdx = _proxyModel->mapFromSource(doc->indexFromNodeId(pid));
			if (ancestorIdx.isValid())
				expand(ancestorIdx);
		}
		setCurrentIndex(proxyIdx);
		scrollTo(proxyIdx, QAbstractItemView::EnsureVisible);
	}

	mCurveTreeFilterProxy* mCurveTreeView::proxyModel() const
	{
		return _proxyModel;
	}

	void mCurveTreeView::clearSelectionIfFilteredOut()
	{
		//以中枢当前节点为准（同时覆盖树点选与图上点选两条来源）；
		//筛选只影响可见性，可见行（含骨架）全部可选中，仅当中选节点已被
		//筛掉（不在代理可见行内）时清选中
		const int nodeId = mCurveDocController::getInstance()->currentNodeId();
		if (nodeId < 0)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (_proxyModel->mapFromSource(doc->indexFromNodeId(nodeId)).isValid())
			return;
		//空选中不触发 onSelectionChanged（该槽对空 selected 早退），
		//中枢须显式写空：参数栏/图上高亮/树回选随 sig_currentNodeChanged 同步复位
		clearSelection();
		mCurveDocController::getInstance()->setCurrentNodeId(-1);
	}

	void mCurveTreeView::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
	{
		Q_UNUSED(deselected)
		//关闭销毁阶段（qApp 析构中树控件取消选中也会触发本槽），
		//此时再联动创建属性页会命中 "No style available without QApplication" 断言
		if (QCoreApplication::closingDown())
			return;
		if (selected.indexes().isEmpty())
			return;
		const QModelIndex idx = selected.indexes().first();
		int nodeId = idx.data(CurveDocument::NodeIdRole).toInt();
		mCurveDocController::getInstance()->setCurrentNodeId(nodeId);
	}

void mCurveTreeView::onRowsInserted()
{
	//防抖合并：数百图批次逐节点插行（数千次触发），restart 语义把同一事件循环
	//内的全部插入合并为恢复后的一次 expandAll（旧实现 singleShot(0) 每次插入
	//各排一个定时器，导入结束后数千次 expandAll 形成数秒 UI 冻结尾巴）
	_expandDebounce.start();
}

void mCurveTreeView::onCurrentNodeChanged(int nodeId)
{
	//关闭销毁阶段（qApp 析构中树控件取消选中也会触发本槽），
	//此时再联动选中会命中 "No style available without QApplication" 断言
	if (QCoreApplication::closingDown())
		return;
	if (nodeId < 0)
	{
		//图上点空白清选：树同步清空选中（空选中不触发 onSelectionChanged 写中枢）
		clearSelection();
		return;
	}
	selectNode(nodeId);
}

	QItemSelectionModel::SelectionFlags mCurveTreeView::selectionCommand(const QModelIndex& index, const QEvent* event) const
	{
		//单击不选中（防误触）：左键单击仅移动当前项、不出选中阴影，双击才程序化
		//选中（见 mouseDoubleClickEvent）。展开/收起箭头点击不受影响（属展开语义，
		//与选中无关）；键盘导航/程序化选中（event 为空）与右键维持默认语义
		if (event && event->type() == QEvent::MouseButtonPress)
		{
			const auto* me = static_cast<const QMouseEvent*>(event);
			if (me->button() == Qt::LeftButton)
				return QItemSelectionModel::NoUpdate;
		}
		return QTreeView::selectionCommand(index, event);
	}

	void mCurveTreeView::mouseDoubleClickEvent(QMouseEvent* event)
	{
		//双击先程序化选中（选中阴影+选择中枢联动，单击已不选中），再按类型分派：
		//图表节点=激活其视口卡片（溢出页"选中即替换呈现"随之触发）；其余有子节点
		//到此为止——不再展开/收起（选中语义与树形张合解耦，张合只由展开箭头负责，
		//也不进入基类防默认张合）；叶子=默认行为（可命名类型进入重命名编辑器）
		const QModelIndex idx = indexAt(event->pos());
		if (!idx.isValid())
		{
			QTreeView::mouseDoubleClickEvent(event);
			return;
		}
		selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect);
		const int nodeId = idx.data(CurveDocument::NodeIdRole).toInt();
		if (mCurveDocController::getInstance()->document()->nodeType(nodeId) == CurveNodeType::Chart)
		{
			mCurveViewportFactory::getInstance()->activateChartViewport(nodeId);
			return;
		}
		if (model()->hasChildren(idx))
			return;
		QTreeView::mouseDoubleClickEvent(event);
	}

	QVector<TreeMenuAction> mCurveTreeView::menuActionsFor(const CurveDocument* doc, int nodeId, CurveNodeType type) const
	{
		Q_UNUSED(nodeId)
		//空白处/工程/页：不属图表管辖，不随图表类型变，本地常量（与原实现一致）
		switch (type)
		{
		case CurveNodeType::Project:
			return { TreeMenuAction::AddPage };
		case CurveNodeType::Page:
			return { TreeMenuAction::AddChart, TreeMenuAction::Separator, TreeMenuAction::RemoveNode };
		default:
			break;
		}
		//图表后代：按所属图表的类型声明出菜单项（menus 未列出的类型返回空=不出菜单）
		const int chartId = doc->chartNodeId(nodeId);
		if (chartId < 0)
			return {};
		return chartTypeProfile(doc->node(chartId)->chart.chartType)
			.menus.value(static_cast<int>(type));
	}

	void mCurveTreeView::appendMenuAction(QMenu& menu, TreeMenuAction action,
		mCurveDocController* controller, CurveDocument* doc, int nodeId, CurveNodeType type)
	{
		switch (action)
		{
		case TreeMenuAction::Separator:
			menu.addSeparator();
			break;
		case TreeMenuAction::ToggleVisible:
			//显隐切换（联动 MDI 视口窗口/属性面板；属性键按类型映射）
			addToggleVisibleAction(menu, controller, doc, nodeId, type);
			break;
		case TreeMenuAction::AddPage:
		{
			QAction* act = menu.addAction(QStringLiteral("新建页"));
			connect(act, &QAction::triggered, this, [controller]() { controller->addPage(); });
			break;
		}
		case TreeMenuAction::AddChart:
		{
			QAction* act = menu.addAction(QStringLiteral("新建视口"));
			connect(act, &QAction::triggered, this, [controller, nodeId]() { controller->addChart(nodeId); });
			break;
		}
		case TreeMenuAction::AddCurve:
		{
			QAction* act = menu.addAction(QStringLiteral("新建曲线"));
			//柱状图目标走柱状曲线变体（仅 y 单壳：无横坐标数据语义，横轴为类目轴）
			CurveNode* chartNode = doc->node(doc->chartNodeId(nodeId));
			const bool barChart = chartNode && chartNode->type == CurveNodeType::Chart
				&& chartNode->chart.chartType == 1;
			connect(act, &QAction::triggered, this, [controller, nodeId, barChart]()
			{
				if (barChart)
					controller->addBarCurve(nodeId);
				else
					controller->addCurve(nodeId);
			});
			break;
		}
		case TreeMenuAction::AddBaseline:
		{
			QAction* act = menu.addAction(QStringLiteral("新建基准线"));
			connect(act, &QAction::triggered, this, [controller, nodeId]() { controller->addBaseline(nodeId); });
			break;
		}
		case TreeMenuAction::AddAnnotation:
		{
			QAction* act = menu.addAction(QStringLiteral("新建注释"));
			connect(act, &QAction::triggered, this, [controller, nodeId]() { controller->addAnnotation(nodeId); });
			break;
		}
		case TreeMenuAction::AddXAxis:
		{
			//多坐标轴入口（对齐 HyperGraph）：横=顶部 X2/X3...，可反复添加
			//（每侧首枚占引擎原生轴，其余自绘条带）；图表节点与轴节点两入口
			//同归所属图表（chartNodeId 对图表自身返回自身）
			QAction* act = menu.addAction(QStringLiteral("添加横坐标轴"));
			connect(act, &QAction::triggered, this, [controller, doc, nodeId]()
			{
				controller->addAxisNode(doc->chartNodeId(nodeId), false);
			});
			break;
		}
		case TreeMenuAction::AddYAxis:
		{
			//多坐标轴入口：纵=右侧 Y2/Y3...，语义同上
			QAction* act = menu.addAction(QStringLiteral("添加纵坐标轴"));
			connect(act, &QAction::triggered, this, [controller, doc, nodeId]()
			{
				controller->addAxisNode(doc->chartNodeId(nodeId), true);
			});
			break;
		}
		case TreeMenuAction::EditCurveData:
		{
			//编辑数据属性：定位并展开 x 数据子节点（参数栏随之切换到 X 数据属性页）；
			//柱状曲线无 x 壳（仅 y 单壳），回退定位到 y 数据子节点
			QAction* act = menu.addAction(QStringLiteral("编辑数据属性"));
			connect(act, &QAction::triggered, this, [this, doc, nodeId]()
			{
				int yFallback = -1;
				for (int childId : doc->childNodeIds(nodeId))
				{
					const CurveNodeType t = doc->nodeType(childId);
					if (t == CurveNodeType::CurveXData)
					{
						selectNode(childId);
						return;
					}
					if (t == CurveNodeType::CurveYData)
						yFallback = childId;
				}
				if (yFallback >= 0)
					selectNode(yFallback);
			});
			break;
		}
		case TreeMenuAction::RemoveNode:
		{
			//删除文案沿旧实现：图表用"删除图表"（不用显示名"视口"），其余=显示名拼接
			const QString text = (type == CurveNodeType::Chart)
				? QStringLiteral("删除图表")
				: QStringLiteral("删除%1").arg(curveNodeTypeDisplayName(type));
			QAction* act = menu.addAction(text);
			connect(act, &QAction::triggered, this, [controller, nodeId]()
			{
				controller->removeNode(nodeId);
			});
			break;
		}
		case TreeMenuAction::RemoveSecondaryAxis:
		{
			//次坐标轴删除专用入口（主轴为固定子节点不提供，误加经撤销）
			QAction* act = menu.addAction(QStringLiteral("删除坐标轴"));
			connect(act, &QAction::triggered, this, [controller, nodeId]()
			{
				controller->removeSecondaryAxis(nodeId);
			});
			break;
		}
		case TreeMenuAction::ToggleComplexFormat:
		{
			//切换复数显示格式（幅值-相位 ↔ 实部-虚部）：文案随当前格式（设计图01），
			//提交 kComplexDisplayFormat 走命令栈可撤销；渲染层现算换算两路数据
			const int chartId = doc->chartNodeId(nodeId);
			const bool magPhase = doc->nodeProperty(chartId,
				CurveProp::kComplexDisplayFormat).toInt() == 0;
			QAction* act = menu.addAction(magPhase
				? QStringLiteral("切换至实部 / 虚部") : QStringLiteral("切换至相位 / 幅值"));
			connect(act, &QAction::triggered, this, [controller, doc, chartId]()
			{
				const int cur = doc->nodeProperty(chartId, CurveProp::kComplexDisplayFormat).toInt();
				controller->setNodeProperty(chartId, CurveProp::kComplexDisplayFormat, cur == 0 ? 1 : 0);
			});
			break;
		}
		case TreeMenuAction::SwapComplexAxes:
		{
			//互换复数坐标轴（上下面板交换，设计图01）：提交 kComplexSwapped 走命令栈
			const int chartId = doc->chartNodeId(nodeId);
			QAction* act = menu.addAction(QStringLiteral("互换复数坐标轴"));
			connect(act, &QAction::triggered, this, [controller, doc, chartId]()
			{
				controller->setNodeProperty(chartId, CurveProp::kComplexSwapped,
					!doc->nodeProperty(chartId, CurveProp::kComplexSwapped).toBool());
			});
			break;
		}
		}
	}

	void mCurveTreeView::contextMenuEvent(QContextMenuEvent* event)
	{
		QModelIndex idx = indexAt(event->pos());
		mCurveDocController* controller = mCurveDocController::getInstance();
		CurveDocument* doc = controller->document();

		int nodeId = -1;
		CurveNodeType type = CurveNodeType::Project;
		if (idx.isValid())
		{
			nodeId = idx.data(CurveDocument::NodeIdRole).toInt();
			type = static_cast<CurveNodeType>(idx.data(CurveDocument::NodeTypeRole).toInt());
		}
		//菜单项清单按类型查表（图表后代查所属图表的 ChartTypeProfile.menus；
		//空白/工程/页本地常量），再逐项组装；全部经控制器命令式 API（可撤销）
		MechMainWindow::mUiMenu menu(this);
		for (TreeMenuAction action : menuActionsFor(doc, nodeId, type))
			appendMenuAction(menu, action, controller, doc, nodeId, type);
		//清单为空的类型（x/y 数据壳节点等）不出菜单，与原 default:return 一致
		if (menu.actions().isEmpty())
			return;
		menu.exec(event->globalPos());
	}
}
