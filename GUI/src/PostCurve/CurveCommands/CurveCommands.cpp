#include "CurveCommands.h"
#include "../CurveCore/CurveDocument.h"

namespace PostCurve
{
	//--- CurveAddNodeCommand ---

	CurveAddNodeCommand::CurveAddNodeCommand(CurveDocument* doc, CurveNodeType type, int nodeId,
		int parentNodeId, const QString& name, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _type(type)
		, _nodeId(nodeId)
		, _parentNodeId(parentNodeId)
		, _name(name)
	{
		setText(QStringLiteral("新增%1").arg(curveNodeTypeDisplayName(type)));
	}

	CurveAddNodeCommand::CurveAddNodeCommand(CurveDocument* doc, int nodeId, int chartId,
		const QString& name, int curveKind, QUndoCommand* parent)
		: CurveAddNodeCommand(doc, CurveNodeType::Curve, nodeId, chartId, name, parent)
	{
		_curveKind = curveKind;
		if (_curveKind == 1)
			setText(QStringLiteral("新增复数曲线"));
		else if (_curveKind == 2)
			setText(QStringLiteral("新增柱状曲线"));
		else if (_curveKind == 3)
			setText(QStringLiteral("新增三维曲线"));
	}

	//带类型创建变体（Chart 专用，R28 入树）：chartType>=0 时 redo 走
	//create3DChart（固定骨架按 profile(5) 仅 Legend），-1=既有 createChart 路径
	CurveAddNodeCommand::CurveAddNodeCommand(CurveDocument* doc, CurveNodeType type, int nodeId,
		int parentNodeId, const QString& name, int chartType, QUndoCommand* parent)
		: CurveAddNodeCommand(doc, type, nodeId, parentNodeId, name, parent)
	{
		_chartType = chartType;
		if (type == CurveNodeType::Chart && _chartType == 5)
			setText(QStringLiteral("新建三维图"));
	}

	CurveAddNodeCommand::~CurveAddNodeCommand()
	{
		//若命令销毁时节点仍在文档中（如栈被清空但undo未执行）无需处理，
		//文档析构会释放全部节点
	}

	void CurveAddNodeCommand::redo()
	{
		if (_type == CurveNodeType::Chart)
		{
			if (_chartType >= 0)
				_doc->create3DChart(_nodeId, _parentNodeId, _name);
			else
				_doc->createChart(_nodeId, _parentNodeId, _name);
		}
		else if (_curveKind == 1)
			_doc->createComplexCurve(_parentNodeId, _nodeId, _name);
		else if (_curveKind == 2)
			_doc->createBarCurve(_parentNodeId, _nodeId, _name);
		else if (_curveKind == 3)
			_doc->create3DCurve(_parentNodeId, _nodeId, _name);
		else
			_doc->createNode(_type, _nodeId, _parentNodeId, _name);
	}

	void CurveAddNodeCommand::undo()
	{
		_doc->removeNodeTree(_nodeId);
	}

	//--- CurveMoveChildCommand ---

	CurveMoveChildCommand::CurveMoveChildCommand(CurveDocument* doc, int chartId, int nodeId,
		int toRow, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _chartId(chartId)
		, _nodeId(nodeId)
		, _fromRow(-1)
		, _toRow(toRow)
	{
		if (CurveNode* chart = doc->node(chartId))
		{
			if (CurveNode* n = doc->node(nodeId))
				_fromRow = chart->children.indexOf(n);
		}
		setText(QStringLiteral("调整节点顺序"));
	}

	void CurveMoveChildCommand::redo()
	{
		if (_fromRow >= 0)
			_doc->moveChildNode(_chartId, _nodeId, _toRow);
	}

	void CurveMoveChildCommand::undo()
	{
		if (_fromRow >= 0)
			_doc->moveChildNode(_chartId, _nodeId, _fromRow);
	}

	//--- CurveRemoveNodeCommand ---

	CurveRemoveNodeCommand::CurveRemoveNodeCommand(CurveDocument* doc, int nodeId, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _nodeId(nodeId)
		, _parentNodeId(doc->parentNodeId(nodeId))
	{
		//构造时（命令入栈前）抓取子树快照
		CurveNode* n = doc->node(nodeId);
		_snapshot = CurveDocument::cloneNodeTree(n);
		setText(QStringLiteral("删除%1").arg(curveNodeTypeDisplayName(doc->nodeType(nodeId))));
	}

	CurveRemoveNodeCommand::~CurveRemoveNodeCommand()
	{
		CurveDocument::freeNodeTree(_snapshot);
		_snapshot = nullptr;
	}

	void CurveRemoveNodeCommand::redo()
	{
		_doc->removeNodeTree(_nodeId);
	}

	void CurveRemoveNodeCommand::undo()
	{
		if (!_snapshot)
			return;
		//插入快照的深拷贝，母本保留以支持多次 undo/redo 循环
		CurveNode* copy = CurveDocument::cloneNodeTree(_snapshot);
		_doc->insertNodeTree(copy, _parentNodeId);
	}

	//--- CurveConvertToBarCommand ---

	CurveConvertToBarCommand::CurveConvertToBarCommand(CurveDocument* doc, int chartId,
		const QVector<BarRecord>& records, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _chartId(chartId)
		, _records(records)
	{
		setText(QStringLiteral("转换为柱状图"));
	}

	CurveConvertToBarCommand::~CurveConvertToBarCommand()
	{
		//母本从未入文档（undo 插入的是它的拷贝），由本命令负责释放
		for (const BarRecord& rec : _records)
			CurveDocument::freeNodeTree(rec.original);
	}

	void CurveConvertToBarCommand::redo()
	{
		for (const BarRecord& rec : _records)
		{
			//原曲线摘除（子树释放，母本在命令内）；柱状节点按预分配 id 重建
			_doc->removeNodeTree(rec.curveId);
			if (_doc->createBarCurve(_chartId, rec.barId, rec.barName))
			{
				//图例身份保留：柱沿用原曲线颜色（名经 createBarCurve 已带）
				if (rec.original && _doc->node(rec.barId))
					_doc->node(rec.barId)->series.color = rec.original->series.color;
				_doc->setCurveBarData(rec.barId, rec.xData, rec.yData, rec.tickLabels);
			}
		}
	}

	void CurveConvertToBarCommand::undo()
	{
		for (const BarRecord& rec : _records)
		{
			if (!_doc->node(rec.barId))
				continue;
			_doc->removeNodeTree(rec.barId);
		}
		//原曲线挂回：插入母本的深拷贝（拷贝保留原 id），多次 undo/redo 循环母本常在
		for (const BarRecord& rec : _records)
		{
			if (!rec.original)
				continue;
			CurveNode* copy = CurveDocument::cloneNodeTree(rec.original);
			_doc->insertNodeTree(copy, _chartId);
		}
	}

	//--- CurvePasteNodeCommand ---

	CurvePasteNodeCommand::CurvePasteNodeCommand(CurveDocument* doc, CurveNode* snapshot,
		int parentNodeId, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _snapshot(snapshot)
		, _nodeId(snapshot ? snapshot->id : -1)
		, _parentNodeId(parentNodeId)
	{
		setText(QStringLiteral("粘贴%1").arg(curveNodeTypeDisplayName(
			_snapshot ? _snapshot->type : CurveNodeType::Project)));
	}

	CurvePasteNodeCommand::~CurvePasteNodeCommand()
	{
		//母本从未入文档（redo 插入的是它的拷贝），由本命令负责释放
		CurveDocument::freeNodeTree(_snapshot);
		_snapshot = nullptr;
	}

	void CurvePasteNodeCommand::redo()
	{
		if (!_snapshot)
			return;
		//插入母本的深拷贝，母本保留以支持多次 undo/redo 循环（与删除命令同策略）
		CurveNode* copy = CurveDocument::cloneNodeTree(_snapshot);
		_doc->insertNodeTree(copy, _parentNodeId);
	}

	void CurvePasteNodeCommand::undo()
	{
		if (_nodeId > 0)
			_doc->removeNodeTree(_nodeId);
	}

	//--- CurveSetPropertyCommand ---

	CurveSetPropertyCommand::CurveSetPropertyCommand(CurveDocument* doc, int nodeId, const QString& prop,
		const QVariant& oldValue, const QVariant& newValue, QUndoCommand* parent)
		: QUndoCommand(parent)
		, _doc(doc)
		, _nodeId(nodeId)
		, _prop(prop)
		, _oldValue(oldValue)
		, _newValue(newValue)
	{
		setText(QStringLiteral("修改%1属性").arg(curveNodeTypeDisplayName(doc->nodeType(nodeId))));
	}

	void CurveSetPropertyCommand::redo()
	{
		_doc->setNodeProperty(_nodeId, _prop, _newValue);
	}

	void CurveSetPropertyCommand::undo()
	{
		_doc->setNodeProperty(_nodeId, _prop, _oldValue);
	}
}
