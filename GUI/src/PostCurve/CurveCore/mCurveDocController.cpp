#include "mCurveDocController.h"
#include "CurveDocument.h"
#include "ChartTypeProfile.h"
#include "../CurveBarExtract.h"
#include "../CurveCommands/CurveCommands.h"
#include "../mCurveDataSourceStore.h"

#include <QUndoStack>
#include <QHash>
#include <cmath>

//信息输出通道（MxFunctions 全局信号：信息输出窗口文本区，带颜色+时间戳+日志，
//statusBar 参数可同时上状态栏——全工程通用通道，窗口已连接）
#include "mGlobalSignals.h"
#include "mOutputMessage.h"

namespace PostCurve
{
	mCurveDocController* mCurveDocController::_instance = nullptr;

	mCurveDocController* mCurveDocController::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mCurveDocController;
		}
		return _instance;
	}

	mCurveDocController::mCurveDocController(QObject* parent)
		: QObject(parent)
	{
		_document = new CurveDocument(this);
		_undoStack = new QUndoStack(this);

		//文档信号转发，外部统一 connect 控制器
		connect(_document, &CurveDocument::nodeAdded, this, &mCurveDocController::sig_nodeAdded);
		connect(_document, &CurveDocument::nodeAboutToBeRemoved, this, &mCurveDocController::sig_nodeAboutToBeRemoved);
		connect(_document, &CurveDocument::nodeRemoved, this, &mCurveDocController::sig_nodeRemoved);
		connect(_document, &CurveDocument::nodePropertyChanged, this, &mCurveDocController::sig_nodePropertyChanged);
	}

	mCurveDocController::~mCurveDocController()
	{
		CurveDocument::freeNodeTree(_clipboard);
		_clipboard = nullptr;
	}

	CurveDocument* mCurveDocController::document() const
	{
		return _document;
	}

	QUndoStack* mCurveDocController::undoStack() const
	{
		return _undoStack;
	}

	void mCurveDocController::clearDocument()
	{
		//整体清空（根除外）：直写不进命令栈——会话恢复/打开新会话前的重置用。
		//逐顶层页 removeNodeTree：nodeAboutToBeRemoved 联动视口工厂自动关闭各页
		//图表卡片（页删除遍历子图表逐个关），树视图经模型行移除信号自动刷新。
		//默认编号/ID计数器保持只增不减（模块语义），不回拨
		const QList<int> pageIds = _document->childNodeIds(_document->rootNodeId());
		for (int pageId : pageIds)
			_document->removeNodeTree(pageId);
		CurveDocument::freeNodeTree(_clipboard);
		_clipboard = nullptr;
		//注释剪贴板（值快照）一并失效：会话恢复=新会话，旧会话注释数据不应带入
		_annoClipboard = CurveAnnotationData();
		_annoClipboardValid = false;
		_undoStack->clear();
		setCurrentNodeId(-1);
	}

	int mCurveDocController::addPage(const QString& name)
	{
		CurveDocument* doc = _document;
		int pageId = doc->allocateNodeId();
		//pch 导入传数据名（文件基名）不占编号；缺省 Page 1...（HyperGraph 风格）；
		//页对齐时 name 传所选视图页名
		QString pageName = name.trimmed();
		if (pageName.isEmpty())
			pageName = curveNodeDefaultName(CurveNodeType::Page, doc->nextDefaultNumber(CurveNodeType::Page));
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Page, pageId, doc->rootNodeId(), pageName));
		return pageId;
	}

	int mCurveDocController::pageViewPageId(int pageId) const
	{
		CurveNode* node = _document->node(pageId);
		return (node && node->type == CurveNodeType::Page) ? node->page.viewPageId : -1;
	}

	void mCurveDocController::setPageViewPageId(int pageId, int viewPageId)
	{
		CurveNode* node = _document->node(pageId);
		if (node && node->type == CurveNodeType::Page)
			node->page.viewPageId = viewPageId;
	}

	int mCurveDocController::curvePageIdForViewPage(int viewPageId) const
	{
		if (viewPageId < 0)
			return -1;
		//遍历页子层查绑定（页数有限，线性足够；会话序列化同款遍历惯例）
		const QList<int> pageIds = _document->childNodeIds(_document->rootNodeId());
		for (int pageId : pageIds)
		{
			CurveNode* node = _document->node(pageId);
			if (node && node->type == CurveNodeType::Page && node->page.viewPageId == viewPageId)
				return pageId;
		}
		return -1;
	}

	void mCurveDocController::setPageName(int pageId, const QString& name)
	{
		if (name.trimmed().isEmpty())
			return;
		setNodeProperty(pageId, CurveProp::kName, name.trimmed());
	}

	int mCurveDocController::addChart(int pageId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(pageId) || doc->nodeType(pageId) != CurveNodeType::Page)
			return -1;
		int chartId = doc->allocateNodeId();
		//视口默认名固定 "XY Plot"（HyperGraph）：同名靠树/MDI 标题的 w1/w2 位置前缀区分
		QString name = curveNodeDefaultName(CurveNodeType::Chart, 0);
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Chart, chartId, pageId, name));
		return chartId;
	}

	int mCurveDocController::addCurve(int chartId, const QString& name)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		QString curveName = name.trimmed();
		if (curveName.isEmpty())
			curveName = curveNodeDefaultName(CurveNodeType::Curve, doc->nextDefaultNumber(CurveNodeType::Curve));
		//R23 起允许同名创建（对齐 HyperGraph）：模型名可重复，不再查重拒绝——
		//引擎渲染键与模型名解耦，由视图层唯一化映射（无碰撞=名原样、碰撞按
		//"名#2"取不冲突键，见 CurveReadMe §1.2），重名曲线数据/样式/选中各自独立
		int curveId = doc->allocateNodeId();
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Curve, curveId, chartId, curveName));
		return curveId;
	}

	int mCurveDocController::addComplexCurve(int chartId, const QString& name)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		QString curveName = name.trimmed();
		if (curveName.isEmpty())
			curveName = curveNodeDefaultName(CurveNodeType::Curve, doc->nextDefaultNumber(CurveNodeType::Curve));
		//同名允许创建（同 addCurve 的 R23 口径，不再查重拒绝，引擎键由视图层映射）
		int curveId = doc->allocateNodeId();
		_undoStack->push(new CurveAddNodeCommand(doc, curveId, chartId, curveName, 1));
		return curveId;
	}

	int mCurveDocController::addBarCurve(int chartId, const QString& name)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		QString curveName = name.trimmed();
		if (curveName.isEmpty())
			curveName = curveNodeDefaultName(CurveNodeType::Curve, doc->nextDefaultNumber(CurveNodeType::Curve));
		//同名允许创建（同 addCurve 的 R23 口径，不再查重拒绝，引擎键由视图层映射）
		int curveId = doc->allocateNodeId();
		_undoStack->push(new CurveAddNodeCommand(doc, curveId, chartId, curveName, 2));
		return curveId;
	}

	int mCurveDocController::add3DChart(int pageId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(pageId) || doc->nodeType(pageId) != CurveNodeType::Page)
			return -1;
		int chartId = doc->allocateNodeId();
		//三维图默认名固定 "3D Plot"（profile(5).defaultChartName；同名靠 w 前缀区分，
		//同 addChart 口径）。带类型创建变体：固定骨架按 profile(5) 仅 Legend
		const QString name = QStringLiteral("3D Plot");
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Chart, chartId, pageId, name, 5));
		return chartId;
	}

	int mCurveDocController::add3DCurve(int chartId, const QString& name)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		QString curveName = name.trimmed();
		if (curveName.isEmpty())
			curveName = curveNodeDefaultName(CurveNodeType::Curve, doc->nextDefaultNumber(CurveNodeType::Curve));
		//同名允许创建（同 addCurve 的 R23 口径）
		int curveId = doc->allocateNodeId();
		_undoStack->push(new CurveAddNodeCommand(doc, curveId, chartId, curveName, 3));
		return curveId;
	}

	//XY→柱状"提取数值转换"（add-chart-type-extract-convert）：逐曲线提取装
	//BarRecord（母本快照+柱数据），与 kChartType 类型切换同宏提交——setNodeProperty
	//的 kChartType 分支自带骨架协调宏（注释清理/固定子补删/默认名随类型），嵌套
	//合成一个撤销单元，一次 Ctrl+Z 整批还原、重做由命令内快照保数据
	bool mCurveDocController::convertCurvesToBar(int chartId, int extractCount)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return false;
		QVector<CurveConvertToBarCommand::BarRecord> records;
		for (int childId : doc->childNodeIds(chartId))
		{
			if (doc->nodeType(childId) != CurveNodeType::Curve)
				continue;
			//均匀 N 点提取（含首尾）；可见点<2 返回空 → 跳过保留原样（design 边界②）
			const auto pts = CurveBarExtract::extractUniform(doc->node(childId)->series, extractCount);
			if (pts.isEmpty())
				continue;
			CurveConvertToBarCommand::BarRecord rec;
			rec.curveId = childId;
			rec.original = CurveDocument::cloneNodeTree(doc->node(childId));
			rec.barId = doc->allocateNodeId();
			rec.barName = doc->node(childId)->series.name;
			for (const CurveBarExtract::ExtractedPoint& p : pts)
			{
				rec.xData.append(double(rec.xData.size()));   //类目序号空间 0..N-1
				rec.yData.append(p.value);
			}
			rec.tickLabels = CurveBarExtract::categoryLabels(pts);
			records.append(rec);
		}
		if (records.isEmpty())
		{
			//无可转曲线（空图/全部不可转）：仅切类型，等价空图切换不打扰
			setNodeProperty(chartId, CurveProp::kChartType, 1);
			return true;
		}
		_undoStack->beginMacro(QStringLiteral("切换图表类型"));
		setNodeProperty(chartId, CurveProp::kChartType, 1);
		_undoStack->push(new CurveConvertToBarCommand(doc, chartId, records));
		_undoStack->endMacro();
		return true;
	}

	int mCurveDocController::addBaseline(int chartId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		int id = doc->allocateNodeId();
		QString name = curveNodeDefaultName(CurveNodeType::Baseline, doc->nextDefaultNumber(CurveNodeType::Baseline));
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Baseline, id, chartId, name));
		return id;
	}

	int mCurveDocController::addAnnotation(int chartId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		int id = doc->allocateNodeId();
		QString name = curveNodeDefaultName(CurveNodeType::Annotation, doc->nextDefaultNumber(CurveNodeType::Annotation));
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::Annotation, id, chartId, name));
		return id;
	}

	//三维基准面创建（R41）：类型守卫 + 撤销宏"新建基准面"。出厂默认值=结构体
	//默认成员（CurveDatumPlane3DData 法向 X/(0,0,0)/橙 221,147,96/不透明度 0.25/
	//标签居中/可见），Add 命令 redo 经 createNode 重建时同样取默认值，撤销/重做
	//循环不丢初值，无需逐键补提交。类型守卫拒绝时不开宏直接返回（无空宏残留）
	int mCurveDocController::addDatumPlane3D(int chartNodeId, QString* errMsg)
	{
		CurveDocument* doc = _document;
		const auto fail = [errMsg](const QString& msg)
		{
			if (errMsg)
				*errMsg = msg;
			return -1;
		};
		CurveNode* chart = doc->node(chartNodeId);
		if (!chart || doc->nodeType(chartNodeId) != CurveNodeType::Chart)
			return fail(QStringLiteral("新建基准面失败：目标图表不存在"));
		if (chart->chart.chartType != 5)
			return fail(QStringLiteral("新建基准面失败：仅三维图支持基准面"));
		_undoStack->beginMacro(QStringLiteral("新建基准面"));
		const int id = doc->allocateNodeId();
		const QString name = curveNodeDefaultName(CurveNodeType::DatumPlane3D,
			doc->nextDefaultNumber(CurveNodeType::DatumPlane3D));
		_undoStack->push(new CurveAddNodeCommand(doc, CurveNodeType::DatumPlane3D, id, chartNodeId, name));
		_undoStack->endMacro();
		return id;
	}

	//三维图注释共享创建例程（R42 二期建、同日修订为三维数据空间锚点）：新建+
	//kAttachMode=1+文本/位置初值包成一个撤销宏——一次 Ctrl+Z 整体撤销（含文本/
	//位置初值）。posX/posY/posZ=三维数据空间锚点（注释锚定三维：渲染按相机投影
	//随视角移动，{X}{Y}{Z} 模板=注释自身位置）；调用方经
	//m3DChartView::queryAxis3DDisplayRange 取轴显示范围中点传入（无可见数据
	//退化 0,0,0）。空宏（addAnnotation 失败时）QUndoStack 标准语义不入栈，无残留
	int mCurveDocController::addWindowAnnotation(int chartId, double posX, double posY, double posZ)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		_undoStack->beginMacro(QStringLiteral("新建注释"));
		const int annoId = addAnnotation(chartId);
		if (annoId > 0)
		{
			setNodeProperty(annoId, CurveProp::kAttachMode, 1);
			setNodeProperty(annoId, CurveProp::kText, QStringLiteral("新注释"));
			setNodeProperty(annoId, CurveProp::kPosX, posX);
			setNodeProperty(annoId, CurveProp::kPosY, posY);
			setNodeProperty(annoId, CurveProp::kPosZ, posZ);
		}
		_undoStack->endMacro();
		return annoId > 0 ? annoId : -1;
	}

	//--- 注释剪贴板（R42 二期；会话级内存，不序列化。快照与文档解耦：复制后
	//原文档节点变化/删除不影响剪贴板内容，粘贴=按当前剪贴板值逐字段重建） ---

	bool mCurveDocController::copyAnnotation(int annoId)
	{
		CurveDocument* doc = _document;
		CurveNode* n = doc->node(annoId);
		if (!n || n->type != CurveNodeType::Annotation)
			return false;
		//先判定再动剪贴板（cutNode 同款纪律：失败不污染剪贴板内容）
		_annoClipboard = n->annotation;
		_annoClipboardValid = true;
		return true;
	}

	bool mCurveDocController::cutAnnotation(int annoId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(annoId) || doc->nodeType(annoId) != CurveNodeType::Annotation)
			return false;
		if (!copyAnnotation(annoId))
			return false;
		//删除与复制包成一个宏"剪切注释"：一次 Ctrl+Z 整体恢复原节点（剪贴板
		//不受撤销影响，恢复后可原位粘贴回归）
		_undoStack->beginMacro(QStringLiteral("剪切注释"));
		_undoStack->push(new CurveRemoveNodeCommand(doc, annoId));
		_undoStack->endMacro();
		return true;
	}

	int mCurveDocController::pasteAnnotation(int chartId)
	{
		CurveDocument* doc = _document;
		if (!_annoClipboardValid || !doc->node(chartId)
			|| doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		_undoStack->beginMacro(QStringLiteral("粘贴注释"));
		const int annoId = addAnnotation(chartId);   //默认计数名"Note N"
		if (annoId > 0)
		{
			//逐字段提交剪贴板数据（走命令栈可撤销/重做）。kVisible 一并提交：
			//全字段一致含显隐态（剪切→粘贴恢复语义）；位置=原值原样不做偏移
			setNodeProperty(annoId, CurveProp::kText, _annoClipboard.text);
			setNodeProperty(annoId, CurveProp::kPosX, _annoClipboard.posX);
			setNodeProperty(annoId, CurveProp::kPosY, _annoClipboard.posY);
			setNodeProperty(annoId, CurveProp::kPosZ, _annoClipboard.posZ);
			setNodeProperty(annoId, CurveProp::kAlign, _annoClipboard.align);
			setNodeProperty(annoId, CurveProp::kColor, _annoClipboard.color);
			setNodeProperty(annoId, CurveProp::kFont, _annoClipboard.font);
			setNodeProperty(annoId, CurveProp::kBgColor, _annoClipboard.bgColor);
			setNodeProperty(annoId, CurveProp::kBorderMode, _annoClipboard.borderMode);
			setNodeProperty(annoId, CurveProp::kBorderColor, _annoClipboard.borderColor);
			setNodeProperty(annoId, CurveProp::kBorderWidth, _annoClipboard.borderWidth);
			setNodeProperty(annoId, CurveProp::kBorderShow, _annoClipboard.borderShow);
			setNodeProperty(annoId, CurveProp::kAttachMode, _annoClipboard.attachMode);
			setNodeProperty(annoId, CurveProp::kVisible, _annoClipboard.visible);
		}
		_undoStack->endMacro();
		return annoId > 0 ? annoId : -1;
	}

	bool mCurveDocController::hasAnnotationClipboard() const
	{
		return _annoClipboardValid;
	}

	int mCurveDocController::addAxisNode(int chartId, bool vertical)
	{
		CurveDocument* doc = _document;
		if (!doc->node(chartId) || doc->nodeType(chartId) != CurveNodeType::Chart)
			return -1;
		const CurveNodeType type = vertical ? CurveNodeType::YAxis2 : CurveNodeType::XAxis2;
		//默认名按同侧现存最大号 +1：X2/X3.../Y2/Y3...（主轴 X1/Y1 占 1 号）。
		//不能按"同侧副轴数 + 2"：删掉 Y2 后再添加会与现存 Y3 撞名
		int maxNo = 1;
		for (int childId : doc->childNodeIds(chartId))
		{
			const CurveNodeType t = doc->nodeType(childId);
			const bool sameSide = vertical
				? (t == CurveNodeType::YAxis || t == CurveNodeType::YAxis2)
				: (t == CurveNodeType::XAxis || t == CurveNodeType::XAxis2);
			if (!sameSide)
				continue;
			bool ok = false;
			const int no = doc->nodeName(childId).mid(1).toInt(&ok);   //"Y1"/"X2" 去首字母取号
			if (ok && no > maxNo)
				maxNo = no;
		}
		const QString name = QStringLiteral("%1%2").arg(vertical ? QStringLiteral("Y") : QStringLiteral("X")).arg(maxNo + 1);

		//新建 + 初值（同侧主轴的刻度模式/范围/字体/颜色 + 标题=轴名）写入包成一个
		//撤销宏：属性走命令栈，redo 重建时初值不丢（直写节点字段会被 Add 命令的
		//重新 createNode 冲掉）
		QUndoStack* stack = undoStack();
		stack->beginMacro(QStringLiteral("添加坐标轴"));
		const int id = doc->allocateNodeId();
		stack->push(new CurveAddNodeCommand(doc, type, id, chartId, name));
		const CurveNodeType primaryType = vertical ? CurveNodeType::YAxis : CurveNodeType::XAxis;
		for (int childId : doc->childNodeIds(chartId))
		{
			if (doc->nodeType(childId) == primaryType)
			{
				static const char* const kCopyProps[] = {
					CurveProp::kTitleVisible, CurveProp::kTitleColor, CurveProp::kScaleMode,
					CurveProp::kWeightMode,
					CurveProp::kAutoScale, CurveProp::kMin, CurveProp::kMax,
					CurveProp::kTitleFont, CurveProp::kLabelFont,
				};
				for (const char* prop : kCopyProps)
					setNodeProperty(id, prop, doc->nodeProperty(childId, prop));
				break;
			}
		}
		setNodeProperty(id, CurveProp::kTitle, name);
		stack->endMacro();
		return id;
	}

	int mCurveDocController::addYAxis2(int chartId)
	{
		//多坐标轴功能放开"每图表至多一枚"限制：右轴可叠加 Y2/Y3...
		return addAxisNode(chartId, true);
	}

	void mCurveDocController::removeNode(int nodeId)
	{
		CurveDocument* doc = _document;
		if (!doc->node(nodeId) || nodeId == doc->rootNodeId())
			return;
		//固定子节点（轴/图例/页眉页脚/曲线x y）不允许单独删除
		if (isFixedChildNode(doc->nodeType(nodeId)))
			return;
		_undoStack->push(new CurveRemoveNodeCommand(doc, nodeId));
	}

	void mCurveDocController::removeSecondaryAxis(int axisNodeId)
	{
		//次坐标轴（XAxis2/YAxis2，用户经"添加坐标轴"创建）专用删除入口：
		//主轴与曲线 x/y 等其余固定子节点仍按 isFixedChildNode 拦截不放开；
		//走命令栈，Ctrl+Z 可整体恢复（含子树快照）
		CurveDocument* doc = _document;
		if (!doc->node(axisNodeId))
			return;
		const CurveNodeType type = doc->nodeType(axisNodeId);
		if (type != CurveNodeType::XAxis2 && type != CurveNodeType::YAxis2)
			return;
		_undoStack->push(new CurveRemoveNodeCommand(doc, axisNodeId));
	}

	//--- 剪贴板 ---

	bool mCurveDocController::copyNode(int nodeId)
	{
		CurveDocument* doc = _document;
		CurveNode* n = doc->node(nodeId);
		if (!n || nodeId == doc->rootNodeId())
			return false;
		//深拷贝入剪贴板（替换旧内容）；快照与文档解耦，原文档后续变化不影响
		CurveNode* snapshot = CurveDocument::cloneNodeTree(n);
		CurveDocument::freeNodeTree(_clipboard);
		_clipboard = snapshot;
		return true;
	}

	bool mCurveDocController::cutNode(int nodeId)
	{
		CurveDocument* doc = _document;
		CurveNode* n = doc->node(nodeId);
		//固定子节点不可删除（与 removeNode 同规），剪切拒绝；先判定再动剪贴板，
		//避免"剪切失败却改了剪贴板内容"的语义漂移
		if (!n || nodeId == doc->rootNodeId() || isFixedChildNode(n->type))
			return false;
		if (!copyNode(nodeId))
			return false;
		removeNode(nodeId);   //删除走命令栈，Ctrl+Z 可恢复原节点（剪贴板不受影响）
		return true;
	}

	int mCurveDocController::pasteNode(int parentNodeId)
	{
		CurveDocument* doc = _document;
		CurveNode* parent = doc->node(parentNodeId);
		if (!parent || !_clipboard)
			return -1;
		//目标父节点须容纳剪贴板类型：视口<-曲线/基准线/注释/副轴，页<-视口，工程<-页
		const CurveNodeType clipboardType = _clipboard->type;
		const CurveNodeType parentType = parent->type;
		const bool accepted =
			(parentType == CurveNodeType::Chart
				&& (clipboardType == CurveNodeType::Curve || clipboardType == CurveNodeType::Baseline
					|| clipboardType == CurveNodeType::Annotation
					|| clipboardType == CurveNodeType::XAxis2 || clipboardType == CurveNodeType::YAxis2))
			|| (parentType == CurveNodeType::Page && clipboardType == CurveNodeType::Chart)
			|| (parentType == CurveNodeType::Project && clipboardType == CurveNodeType::Page);
		if (!accepted)
			return -1;

		//母本深拷贝并整树重编 id（母本 id 是复制时的旧 id，可能与现存节点冲突；
		//粘贴出的新节点 id 在入栈前定稿，undo/redo 循环保持稳定）
		CurveNode* subtree = CurveDocument::cloneNodeTree(_clipboard);
		doc->reassignNodeTreeIds(subtree);
		const int newNodeId = subtree->id;

		//粘贴同名自动加" (N)"序号后缀（保留原名优先：剪切后原位粘贴=原名回归，
		//语义同移动）；曲线同步 series.name 与节点名一致。R23 重名允许后此自动
		//后缀为兼容保留（既有粘贴语义零变化），模板应用的同名后缀机制同理
		const QString base = subtree->name;
		QString name = base;
		int suffix = 2;
		auto nameTaken = [doc, parentNodeId, clipboardType](const QString& candidate)
		{
			for (int childId : doc->childNodeIds(parentNodeId))
			{
				CurveNode* c = doc->node(childId);
				if (c && c->type == clipboardType && c->name == candidate)
					return true;
			}
			return false;
		};
		while (nameTaken(name))
			name = QStringLiteral("%1 (%2)").arg(base).arg(suffix++);
		if (name != base)
		{
			subtree->name = name;
			if (subtree->type == CurveNodeType::Curve)
				subtree->series.name = name;
		}

		_undoStack->push(new CurvePasteNodeCommand(doc, subtree, parentNodeId));
		return newNodeId;
	}

	bool mCurveDocController::hasClipboard() const
	{
		return _clipboard != nullptr;
	}

	void mCurveDocController::setNodeProperty(int nodeId, const QString& prop, const QVariant& value)
	{
		QVariant oldValue = _document->nodeProperty(nodeId, prop);
		if (!oldValue.isValid())
			return;
		if (oldValue == value)
			return;
		//kName（曲线重命名）R23 起允许撞名（对齐 HyperGraph，与创建同口径不再查重
		//拒绝，无需特判）：模型名可重复，引擎渲染键由视图层唯一化映射反查（无碰撞=
		//名原样、碰撞按"名#2"取不冲突键，见 CurveReadMe §1.2）；页/视口名本就允许
		//重复——唯一性由树/MDI 标题的 p/w 位置前缀承担
		//R29 守卫：既有图表 kChartType 切入三维图（5）不支持——曲线无 zData 语义、
		//骨架协调补不齐三维语义，且该入口正是 R28 化石树的最后一个产生向量。
		//静默返回不建命令（撤销栈无残留），0/1/2/3/4 间既有切换不受影响
		if (prop == CurveProp::kChartType && value.toInt() == 5
			&& _document->nodeType(nodeId) == CurveNodeType::Chart)
			return;
		//切换图表类型恒成宏：类型属性 + 清理该图表下全部注释（注释按旧图表形态
		//标注的位置在新形态（如 XY↔复数图双面板）下无意义）+ 骨架协调（固定子
		//补删/尾部顺序对齐设计图/默认名随类型，见 reconcileChartSkeleton；无可做
		//项时协调为空）。一次撤销同时恢复类型/注释/骨架/名称
		if (prop == CurveProp::kChartType
			&& _document->nodeType(nodeId) == CurveNodeType::Chart)
		{
			_undoStack->beginMacro(QStringLiteral("切换图表类型"));
			_undoStack->push(new CurveSetPropertyCommand(
				_document, nodeId, prop, oldValue, value));
			const QList<int> annoIds = _document->childNodeIds(nodeId);
			for (int childId : annoIds)
			{
				if (_document->nodeType(childId) == CurveNodeType::Annotation)
					_undoStack->push(new CurveRemoveNodeCommand(_document, childId));
			}
			reconcileChartSkeleton(nodeId, oldValue.toInt(), value.toInt());
			_undoStack->endMacro();
			return;
		}
		_undoStack->push(new CurveSetPropertyCommand(_document, nodeId, prop, oldValue, value));
	}

	//曲线类别打标：经 kCurveCategory 属性走命令栈（曲线统计按类别过滤的数据基础）。
	//节点级数据随节点走——删除/整批撤销时随 CurveRemoveNodeCommand 快照一并回滚，
	//无需额外处理；非曲线节点（或已失效 id）静默返回
	void mCurveDocController::setCurveCategory(int curveId, CurveCategory category)
	{
		if (curveId <= 0 || _document->nodeType(curveId) != CurveNodeType::Curve)
			return;
		setNodeProperty(curveId, CurveProp::kCurveCategory, static_cast<int>(category));
	}

	//图表类型切换的骨架协调（kChartType 宏内调用，结构变更全走命令栈）：
	//1) 旧声明有而新声明无的固定子类型 → 全部删除（如退出复数删 Phase/YAxis2，
	//   用户手加的同类型副轴一并删——类型切换本就清空曲线/注释，语义一致）；
	//2) 新声明有而图表缺失的固定子类型 → 补建（进入复数补 YAxis2"Phase"，初值
	//   复制同侧主轴同 addAxisNode；已存在同类型副轴时跳过）；
	//3) 尾部区（轴/页眉页脚）相对顺序对齐新声明（设计图：复数=Magnitude、Phase、X1）；
	//4) 图表名仍为旧类型默认名时随类型改名（XY Plot -> Complex Plot，改过名不动）
	void mCurveDocController::reconcileChartSkeleton(int chartId, int oldType, int newType)
	{
		Q_UNUSED(oldType)
		CurveDocument* doc = _document;
		CurveNode* chart = doc->node(chartId);
		if (!chart || doc->nodeType(chartId) != CurveNodeType::Chart)
			return;
		const ChartTypeProfile& oldProf = chartTypeProfile(oldType);
		const ChartTypeProfile& newProf = chartTypeProfile(newType);

		//1) 删除旧声明有、新声明无的固定子
		for (const ChartFixedChild& fc : oldProf.fixedChildren)
		{
			bool inNew = false;
			for (const ChartFixedChild& nf : newProf.fixedChildren)
			{
				if (nf.type == fc.type) { inNew = true; break; }
			}
			if (inNew)
				continue;
			const QList<int> ids = doc->childNodeIds(chartId);
			for (int childId : ids)
			{
				if (doc->nodeType(childId) == fc.type)
					_undoStack->push(new CurveRemoveNodeCommand(doc, childId));
			}
		}

		//2) 补建新声明有、图表缺失的固定子
		for (const ChartFixedChild& fc : newProf.fixedChildren)
		{
			bool exists = false;
			for (int childId : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(childId) == fc.type) { exists = true; break; }
			}
			if (exists)
				continue;
			const int id = doc->allocateNodeId();
			_undoStack->push(new CurveAddNodeCommand(doc, fc.type, id, chartId, fc.name));
			//副轴初值复制同侧主轴（同 addAxisNode：刻度模式/范围/字体/颜色）
			const CurveNodeType primaryType = (fc.type == CurveNodeType::YAxis2) ? CurveNodeType::YAxis
				: (fc.type == CurveNodeType::XAxis2) ? CurveNodeType::XAxis : CurveNodeType::Project;
			if (primaryType == CurveNodeType::Project)
				continue;
			static const char* const kCopyProps[] = {
				CurveProp::kTitleVisible, CurveProp::kTitleColor, CurveProp::kScaleMode,
				CurveProp::kWeightMode,
				CurveProp::kAutoScale, CurveProp::kMin, CurveProp::kMax,
				CurveProp::kTitleFont, CurveProp::kLabelFont,
			};
			for (int childId : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(childId) != primaryType)
					continue;
				for (const char* propKey : kCopyProps)
					setNodeProperty(id, propKey, doc->nodeProperty(childId, propKey));
				break;
			}
			//轴标题默认按声明显示名（复数 Y2="Phase"，对齐下面板轴语义），不用节点名 "Y2"
			setNodeProperty(id, CurveProp::kTitle,
				fc.displayName.isEmpty() ? fc.name : fc.displayName);
		}

		//3) 尾部区相对顺序对齐新声明（图例属中部分区不参与；同类型多枚副轴保持相对序）
		QList<int> desired;
		for (const ChartFixedChild& fc : newProf.fixedChildren)
		{
			if (fc.type == CurveNodeType::Legend)
				continue;
			if (chartZoneRank(newProf, fc.type) < newProf.zoneOrder.size())
				continue;   //分区内的用户节点类型不参与
			for (int childId : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(childId) == fc.type && !desired.contains(childId))
					desired.append(childId);
			}
		}
		if (!desired.isEmpty())
		{
			const QList<int> ids = doc->childNodeIds(chartId);
			int tailStart = -1;
			for (int id : desired)
			{
				const int r = ids.indexOf(id);
				if (r >= 0 && (tailStart < 0 || r < tailStart))
					tailStart = r;
			}
			for (int slot = 0; slot < desired.size(); ++slot)
			{
				const QList<int> cur = doc->childNodeIds(chartId);
				const int curRow = cur.indexOf(desired.at(slot));
				const int expect = tailStart + slot;
				//前序槽位已在位，期望位只可能 <= 当前位（左移）；右移一格为无操作
				if (curRow >= 0 && curRow != expect)
					_undoStack->push(new CurveMoveChildCommand(doc, chartId, desired.at(slot), expect));
			}
		}

		//4) 默认名随类型（用户改过名不动）
		if (!oldProf.defaultChartName.isEmpty()
			&& oldProf.defaultChartName != newProf.defaultChartName
			&& chart->name == oldProf.defaultChartName)
		{
			_undoStack->push(new CurveSetPropertyCommand(doc, chartId,
				CurveProp::kName, oldProf.defaultChartName, newProf.defaultChartName));
		}
	}

	void mCurveDocController::undo()
	{
		_undoStack->undo();
	}

	void mCurveDocController::redo()
	{
		_undoStack->redo();
	}

	int mCurveDocController::currentNodeId() const
	{
		return _currentNodeId;
	}

	void mCurveDocController::setCurrentNodeId(int nodeId)
	{
		if (_currentNodeId == nodeId)
			return;
		//数据隔离兜底：类型筛选下作用域外类型不写入中枢（正常路径已在
		//树 onSelectionChanged/图上点选各入口拦截，此处防新增入口遗漏）
		if (!canSelectNode(nodeId))
			return;
		_currentNodeId = nodeId;
		emit sig_currentNodeChanged(nodeId);
	}

	void mCurveDocController::setInteractiveTypes(const QSet<int>& types)
	{
		_interactiveTypes = types;
	}

	bool mCurveDocController::isTypeInteractive(int type) const
	{
		//空集合=完整结构树（全部可交互）
		return _interactiveTypes.isEmpty() || _interactiveTypes.contains(type);
	}

	bool mCurveDocController::canSelectNode(int nodeId) const
	{
		if (nodeId <= 0)
			return true;   //清选（-1）不受作用域限制
		return isTypeInteractive(static_cast<int>(_document->nodeType(nodeId)));
	}

	void mCurveDocController::reportInfo(const QString& msg)
	{
		QString text = QStringLiteral("[曲线] ") + msg;
		emit sig_info(text);
		emit MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
			static_cast<int>(MxFunctions::NormalMessage), text, true, true);
	}

	void mCurveDocController::reportWarning(const QString& msg)
	{
		QString text = QStringLiteral("[曲线] ") + msg;
		emit sig_warning(text);
		emit MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
			static_cast<int>(MxFunctions::WarningMessage), text, true, true);
	}

	void mCurveDocController::reportError(const QString& msg)
	{
		QString text = QStringLiteral("[曲线] ") + msg;
		emit sig_error(text);
		emit MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
			static_cast<int>(MxFunctions::ErrorMessage), text, true, true);
	}

	void mCurveDocController::setSeriesData(int curveId, const QVector<double>& xData, const QVector<double>& yData)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.xData = xData;
		n->series.yData = yData;
		emit sig_nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	void mCurveDocController::set3DSeriesData(int curveId, const QVector<double>& xData,
		const QVector<double>& yData, const QVector<double>& zData, int plot3dType,
		int rows, int cols)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.xData = xData;
		n->series.yData = yData;
		n->series.zData = zData;
		//三轴截齐兜底（同块装配天然等长；脏数据不进渲染层）
		const int nPts = qMin(n->series.xData.size(),
			qMin(n->series.yData.size(), n->series.zData.size()));
		n->series.xData.resize(nPts);
		n->series.yData.resize(nPts);
		n->series.zData.resize(nPts);
		n->series.plot3dType = plot3dType;
		//网格行列（曲面/瀑布）：点数不匹配即视为无效退化为点序（渲染端同口径防御）
		if (rows > 0 && cols > 0 && rows * cols == nPts)
		{
			n->series.plot3dRows = rows;
			n->series.plot3dCols = cols;
		}
		else
		{
			n->series.plot3dRows = 0;
			n->series.plot3dCols = 0;
		}
		emit sig_nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	void mCurveDocController::set3DCurveProvenance(int curveId, const QString& sourceFile,
		const QString& xExpression, const QString& yExpression, const QString& zExpression,
		int xComponent, int yComponent, int zComponent)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		//溯源标记 + pch 五元组/侧四元组清空（三维定义溯源不使用，残留旧值会
		//误导"更改曲线"等五元组消费方）
		n->series.sourceFile = sourceFile;
		n->series.subcase.clear();
		n->series.schema.clear();
		n->series.object.clear();
		n->series.component.clear();
		n->series.xSideSubcase.clear();
		n->series.xSideSchema.clear();
		n->series.xSideObject.clear();
		n->series.xSideComponent.clear();
		n->series.ySideSubcase.clear();
		n->series.ySideSchema.clear();
		n->series.ySideObject.clear();
		n->series.ySideComponent.clear();
		//三侧数学表达式（非数学侧空串=清空；X/Y 落既有 ref.expression，Z 落
		//三维专字段——ref 其余变换字段不动）
		n->series.xRef.expression = xExpression;
		n->series.yRef.expression = yExpression;
		n->series.z3DExpression = zExpression;
		//三侧文件分量列下标（非文件侧 -1=未记录）
		n->series.x3DComponent = xComponent;
		n->series.y3DComponent = yComponent;
		n->series.z3DComponent = zComponent;
	}

	void mCurveDocController::setComplexSeriesData(int curveId, const QVector<double>& xData,
		const QVector<double>& magData, const QVector<double>& phaseData)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.xData = xData;
		n->series.yData = magData;
		n->series.complexYData = phaseData;
		//a/b 截齐兜底（同块装配天然等长；脏数据不进渲染层）
		const int nPts = qMin(n->series.xData.size(),
			qMin(n->series.yData.size(), n->series.complexYData.size()));
		n->series.xData.resize(nPts);
		n->series.yData.resize(nPts);
		n->series.complexYData.resize(nPts);
		emit sig_nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	void mCurveDocController::setSeriesTickLabels(int curveId, const QStringList& labels)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.xTickLabels = labels;
		emit sig_nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	void mCurveDocController::setSeriesHiddenRanges(int curveId, const QVector<double>& ranges)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.hiddenRanges = ranges;
		emit sig_nodePropertyChanged(curveId, QStringLiteral("data"));
	}

	void mCurveDocController::setSeriesSource(int curveId, const QString& sourceFile, const QString& subcase,
		const QString& schema, const QString& object, const QString& component)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.sourceFile = sourceFile;
		n->series.subcase = subcase;
		n->series.schema = schema;
		n->series.object = object;
		n->series.component = component;
		//侧独立来源随整线换源对齐：Y 侧 = 新通道；X 侧保持自身形态——通道
		//形态（实际结果类型，X=通道分量值）原样保留（数据由调用方保持不变），
		//横坐标形态复位为未记录（回显按文件解析的横坐标类型，数据 = 新锚通道
		//的记录横坐标），与新 x/y 数据一致。X 通道形态不受 Y 侧换源影响
		n->series.ySideSubcase = subcase;
		n->series.ySideSchema = schema;
		n->series.ySideObject = object;
		n->series.ySideComponent = component;
		const bool xChannelMode = mCurveDataSourceStore::isChannelSchema(n->series.xSideSchema);
		if (!xChannelMode)
		{
			n->series.xSideSubcase.clear();
			n->series.xSideSchema.clear();
			n->series.xSideObject.clear();
			n->series.xSideComponent.clear();
		}
	}

	void mCurveDocController::setComplexSeriesSource(int curveId, const QString& sourceFile,
		const QString& subcase, const QString& schema, const QString& object,
		const QString& componentA, const QString& componentB, int complexFormat,
		const QString& subB, const QString& schemaB, const QString& objectB)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		n->series.sourceFile = sourceFile;
		n->series.subcase = subcase;
		n->series.schema = schema;
		n->series.object = object;
		n->series.component = componentA;
		n->series.complexFormat = complexFormat;
		//上面板侧 = a 分量（ySide* 同 setSeriesSource 规则）；下面板侧 = b 分量
		//（complexSide*：b 侧四元组缺省镜像 a 侧=同块配对，显式传入按实选落模，
		//创建时必写故读取不回退五元组）
		n->series.ySideSubcase = subcase;
		n->series.ySideSchema = schema;
		n->series.ySideObject = object;
		n->series.ySideComponent = componentA;
		n->series.complexSideSubcase = subB.isEmpty() ? subcase : subB;
		n->series.complexSideSchema = schemaB.isEmpty() ? schema : schemaB;
		n->series.complexSideObject = objectB.isEmpty() ? object : objectB;
		n->series.complexSideComponent = componentB;
		//X 侧保持自身形态，横坐标形态复位未记录（同 setSeriesSource）
		if (!mCurveDataSourceStore::isChannelSchema(n->series.xSideSchema))
		{
			n->series.xSideSubcase.clear();
			n->series.xSideSchema.clear();
			n->series.xSideObject.clear();
			n->series.xSideComponent.clear();
		}
	}

	//整线换源配对校验（"更改曲线"/Y 侧通道切换落模前调用）：X 侧为通道形态
	//（isChannelSchema，X=通道分量值）时，新 Y 数据须与现存 X 点数成对，不成对
	//报警并返回 false——调用方零副作用退出（校验须在压撤回快照/任何写之前）。
	//横坐标形态/未记录返回 true（X 将重取记录横坐标，无配对要求）
	bool mCurveDocController::checkSourcePairing(int curveId, const QVector<double>& newValues)
	{
		CurveNode* n = _document ? _document->node(curveId) : nullptr;
		if (!n || n->type != CurveNodeType::Curve)
			return true;
		if (!mCurveDataSourceStore::isChannelSchema(n->series.xSideSchema))
			return true;
		if (newValues.size() == n->series.xData.size())
			return true;
		reportWarning(QStringLiteral("[更改曲线] X/Y 数据点数不一致（X:%1 Y:%2），未切换")
			.arg(n->series.xData.size()).arg(newValues.size()));
		return false;
	}

	void mCurveDocController::setCurveSideSource(int curveId, bool xSide, const QString& subcase,
		const QString& schema, const QString& object, const QString& component)
	{
		CurveNode* n = _document->node(curveId);
		if (!n || n->type != CurveNodeType::Curve)
			return;
		QString& sub = xSide ? n->series.xSideSubcase : n->series.ySideSubcase;
		QString& sch = xSide ? n->series.xSideSchema : n->series.ySideSchema;
		QString& obj = xSide ? n->series.xSideObject : n->series.ySideObject;
		QString& comp = xSide ? n->series.xSideComponent : n->series.ySideComponent;
		sub = subcase;
		sch = schema;
		obj = object;
		comp = component;
	}

	int mCurveDocController::addCurveBatch(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
		const QString& pageName, int existingChartId, const QString& xTitle, const QString& yTitle,
		int currentChartId)
	{
		return addCurveBatchImpl(items, layout, pageName, existingChartId, xTitle, yTitle,
			false, -1, currentChartId, nullptr);
	}

	int mCurveDocController::addCurveBatchToPage(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
		const QString& pageName, int existingChartId, const QString& xTitle, const QString& yTitle,
		int currentPageId, QList<int>* createdChartIds)
	{
		return addCurveBatchImpl(items, layout, pageName, existingChartId, xTitle, yTitle,
			true, currentPageId, -1, createdChartIds);
	}

	int mCurveDocController::addCurveBatchImpl(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
		const QString& pageName, int existingChartId, const QString& xTitle, const QString& yTitle,
		bool pageLanding, int currentPageId, int currentChartId, QList<int>* createdChartIds)
	{
		if (items.isEmpty())
			return -1;
		CurveDocument* doc = _document;
		const QVector<QColor> kColors = curveDefaultColorWheel();

		//并入目标校验（失效早退，宏/defer 均未开启，零副作用）
		if (layout == CurveBatchLayout::ExistingChart)
		{
			CurveNode* node = doc->node(existingChartId);
			if (!node || node->type != CurveNodeType::Chart)
				return -1;
		}

		//单曲线装配（建线→数据直写→来源→色轮→线宽；无数据点跳过并警告）
		int created = 0;
		auto appendCurve = [&](int chartId, const CurveBatchItem& item, int colorIndex)
		{
			if (item.xData.isEmpty() || item.yData.isEmpty())
			{
				reportWarning(QStringLiteral("曲线“%1”无有效数据点，未创建").arg(item.name));
				return false;
			}
			const int curveId = addCurve(chartId, item.name);
			if (curveId < 0)
				return false;
			setSeriesData(curveId, item.xData, item.yData);
			setSeriesSource(curveId, item.sourceFile, item.subcase, item.schema,
				item.object, item.component);
			setNodeProperty(curveId, CurveProp::kColor, kColors.at(colorIndex % kColors.size()));
			setNodeProperty(curveId, CurveProp::kLineWidth, 2.0);
			return true;
		};
		//新建图表的 X1/Y1 轴标题（并入既有图表只追加曲线不覆盖既有设置）
		auto setAxisTitles = [&](int chartId)
		{
			for (int childId : doc->childNodeIds(chartId))
			{
				if (doc->nodeType(childId) == CurveNodeType::XAxis && !xTitle.isEmpty())
					setNodeProperty(childId, CurveProp::kTitle, xTitle);
				else if (doc->nodeType(childId) == CurveNodeType::YAxis && !yTitle.isEmpty())
					setNodeProperty(childId, CurveProp::kTitle, yTitle);
			}
		};

		//静默落图：批量建卡只登记不切页/不激活/不物化（用户当前视图页不动），
		//结束后恢复常规模式——经信号由视口工厂响应（未连接时空操作，无头安全）
		emit sig_requestDeferCardActivation(true);
		_undoStack->beginMacro(QStringLiteral("添加曲线"));

		int resultPageId = -1;
		if (layout == CurveBatchLayout::ExistingChart)
		{
			resultPageId = doc->parentNodeId(existingChartId);
			//并入配色从目标图表既有曲线数续起，避免与已有曲线同色
			int existCount = 0;
			for (int childId : doc->childNodeIds(existingChartId))
			{
				if (doc->nodeType(childId) == CurveNodeType::Curve)
					++existCount;
			}
			for (const CurveBatchItem& item : items)
				created += appendCurve(existingChartId, item, existCount + created) ? 1 : 0;
		}
		else
		{
			//分桶（桶=一张图）：OnePerCurve 每条一桶；OnePerGroup 按 group 聚桶
			//（空 group 该条独立一桶）；AllInOne/NewPage 全部一桶
			QList<QList<int>> buckets;
			if (layout == CurveBatchLayout::OnePerCurve)
			{
				for (int i = 0; i < items.size(); ++i)
					buckets.append(QList<int>{ i });
			}
			else if (layout == CurveBatchLayout::OnePerGroup)
			{
				QHash<QString, int> groupBucket;
				for (int i = 0; i < items.size(); ++i)
				{
					const QString g = items.at(i).group;
					if (g.isEmpty())
					{
						buckets.append(QList<int>{ i });
						continue;
					}
					if (!groupBucket.contains(g))
					{
						groupBucket.insert(g, buckets.size());
						buckets.append(QList<int>());
					}
					buckets[groupBucket.value(g)].append(i);
				}
			}
			else
			{
				QList<int> all;
				for (int i = 0; i < items.size(); ++i)
					all.append(i);
				buckets.append(all);
			}

			//页布局：容纳桶数的最小格数（超上限自动分多页，NTF 分页同口径）。
			//注意变量不可命名 slots——Qt 关键字宏（未定义 QT_NO_KEYWORDS 的目标里被替换为空）
			const QList<int> slotTable = curveViewportLayoutSlots();
			int layoutIdx = slotTable.size() - 1;
			for (int i = 0; i < slotTable.size(); ++i)
			{
				if (slotTable.at(i) >= buckets.size())
				{
					layoutIdx = i;
					break;
				}
			}
			const int cells = qMax(1, slotTable.value(layoutIdx, 1));
			const int pageCount = qMax(1, (buckets.size() + cells - 1) / cells);

			//落点页（两公开口在此分叉）：
			//pageLanding（addCurveBatchToPage）：AllInOne/OnePerGroup/OnePerCurve
			//落 currentPageId 指定页（非页节点/已删视为无效走新建），NewPage 无视
			//之强制新建（"新视图页"语义）
			//chartLanding（addCurveBatch 原口径）：AllInOne 复用当前激活图表所在页
			//（无则新建，同其余布局）；currentChartId 由调用方传入（控制层不反查
			//视图层）
			if (pageLanding)
			{
				int landPageId = (layout == CurveBatchLayout::NewPage) ? -1 : currentPageId;
				if (landPageId >= 0 && doc->nodeType(landPageId) != CurveNodeType::Page)
					landPageId = -1;
				if (landPageId >= 0)
					resultPageId = landPageId;
			}
			else if (layout == CurveBatchLayout::AllInOne && currentChartId >= 0)
			{
				resultPageId = doc->parentNodeId(currentChartId);
			}

			for (int p = 0; p < pageCount; ++p)
			{
				//复用页不动其 kLayout（用户布局保持），仅新建页设布局
				const bool reusePage = (p == 0 && resultPageId >= 0);
				int pageId = resultPageId;
				if (!reusePage)
				{
					pageId = addPage(pageName);
					if (pageId < 0)
						continue;
					if (resultPageId < 0)
						resultPageId = pageId;
					setNodeProperty(pageId, CurveProp::kLayout, layoutIdx);
				}
				for (int b = p * cells; b < buckets.size() && b < (p + 1) * cells; ++b)
				{
					const int chartId = addChart(pageId);
					if (chartId < 0)
						continue;
					if (createdChartIds)
						createdChartIds->append(chartId);
					int colorSeq = 0;
					for (int idx : buckets.at(b))
						created += appendCurve(chartId, items.at(idx), colorSeq++) ? 1 : 0;
					setAxisTitles(chartId);
				}
			}
		}

		_undoStack->endMacro();
		emit sig_requestDeferCardActivation(false);

		//全部失败：整宏回滚，不留空页/空图残
		if (created == 0)
		{
			_undoStack->undo();
			return -1;
		}
		return resultPageId;
	}
}
