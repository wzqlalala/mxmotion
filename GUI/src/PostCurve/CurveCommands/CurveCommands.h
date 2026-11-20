#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 撤销/重做命令（命令层）
//
// 所有结构变更与属性修改都封装为 QUndoCommand，经 mCurveDocController 的
// QUndoStack 执行；视图只订阅文档信号（nodeAdded/nodeRemoved/nodePropertyChanged），
// 不感知命令的存在——撤销/重做引起的变更与正常修改走同一条通知链路。
//
// 批量操作：pch 导入等"一图表+多曲线+样式"场景由调用方用
// QUndoStack::beginMacro()/endMacro() 把多条命令合成一个撤销单元。
//
// id 稳定性约定：命令构造时即分配 nodeId（doc->allocateNodeId()），
// 因此 redo 重建的节点与首次创建的节点 id 一致，外部持有的引用在
// undo/redo 循环中始终有效。
// =============================================================================

#include <QUndoCommand>
#include <QString>
#include <QVariant>

#include "../CurveCore/CurveDefine.h"

namespace PostCurve
{
	class CurveDocument;
	struct CurveNode;

	//新增节点命令
	//redo：文档中创建节点（Chart 类型自动创建 XAxis/YAxis/Legend/Header/Footer
	//      固定子节点，其id在每次redo时重新分配——固定子节点无外部引用，安全）；
	//undo：整棵删除（含子树）。
	//典型来源：Ribbon/树右键"新建"、pch 导入（addChart/addCurve API 内部 push）
	class CurveAddNodeCommand : public QUndoCommand
	{
	public:
		CurveAddNodeCommand(CurveDocument* doc, CurveNodeType type, int nodeId,
			int parentNodeId, const QString& name, QUndoCommand* parent = nullptr);
		//曲线创建变体（曲线形态 kind：0=普通曲线 x/y 两壳，1=复数曲线 x/ym/yp
		//三壳，2=柱状曲线仅 y 壳，3=三维曲线 x/y/z 三轴壳；redo 分派
		//createComplexCurve/createBarCurve/create3DCurve），
		//复数图/柱状图/三维图目标导入/定义曲线用（普通曲线仍走基础构造）
		CurveAddNodeCommand(CurveDocument* doc, int nodeId, int chartId,
			const QString& name, int curveKind, QUndoCommand* parent = nullptr);
		//Chart 带类型创建变体（R28 入树）：chartType>=0 时 redo 走 create3DChart
		//（固定骨架按对应 profile 声明，5=三维图仅 Legend）；-1=既有 createChart
		//路径。"从文件新建三维图"弹窗创建链用
		CurveAddNodeCommand(CurveDocument* doc, CurveNodeType type, int nodeId,
			int parentNodeId, const QString& name, int chartType, QUndoCommand* parent = nullptr);
		~CurveAddNodeCommand() override;
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;          //目标文档（不拥有）
		CurveNodeType _type;          //节点类型
		int _nodeId;                  //构造时预分配的节点id（redo后保持不变）
		int _parentNodeId;            //父节点id（Chart 时为根节点id）
		QString _name;                //节点名
		int _curveKind = 0;           //曲线形态（0 普通/1 复数/2 柱状/3 三维，见曲线创建变体）
		int _chartType = -1;          //Chart 带类型创建（>=0=create3DChart；-1=既有路径）
	};

	//删除节点命令（含子树）
	//构造时（入栈前）抓取整棵子树的深拷贝快照；
	//redo：从文档删除子树；undo：把快照的深拷贝挂回原父节点。
	//快照是"母本"永不入文档——undo 插入的是它的拷贝，保证多次 undo/redo
	//循环后母本依然可用；恢复的子树保留原 id，外部引用继续有效。
	class CurveRemoveNodeCommand : public QUndoCommand
	{
	public:
		CurveRemoveNodeCommand(CurveDocument* doc, int nodeId, QUndoCommand* parent = nullptr);
		~CurveRemoveNodeCommand() override;   //释放快照
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;
		int _nodeId;                  //被删节点id
		int _parentNodeId;            //原父节点id（构造时记录）
		CurveNode* _snapshot = nullptr;   //删除前子树快照（母本，undo时插入其深拷贝）
	};

	//图表类型切换"提取数值转换"命令（add-chart-type-extract-convert）：
	//XY→柱状时图内每条曲线转为柱状曲线（仅 y 单壳）。数据快照进命令——
	//redo：删原曲线子树（母本在命令内）+ 经 createBarCurve 建柱 + setCurveBarData
	//直写柱数据（undo/redo 循环数据不丢；CurveAddNodeCommand 的 redo 只重建
	//空壳节点，数据直写不入栈，故本转换不用 Remove+Add 命令对）；undo：删柱 +
	//挂回母本深拷贝（CurveRemoveNodeCommand 同款快照策略）。图表类型属性切换
	//由外层宏的 SetProperty 命令承担（骨架协调/注释清理/默认名随类型），
	//本命令只负责曲线节点的替换
	class CurveConvertToBarCommand : public QUndoCommand
	{
	public:
		//一条曲线的转换记录
		struct BarRecord
		{
			int curveId = -1;               //原曲线节点 id（redo 删/undo 挂回，id 稳定）
			CurveNode* original = nullptr;  //原曲线子树母本快照（构造时深拷贝，永不入文档）
			int barId = -1;                 //柱状曲线节点 id（构造时预分配，redo/undo 循环不变）
			QString barName;                //柱状曲线名（=原名，渲染/图例键不变）
			QVector<double> xData;          //柱状 X 数据（类目序号空间 0..N-1）
			QVector<double> yData;          //柱状 Y 数据（提取值）
			QStringList tickLabels;         //类目签（提取频率值）
		};

		CurveConvertToBarCommand(CurveDocument* doc, int chartId,
			const QVector<BarRecord>& records, QUndoCommand* parent = nullptr);
		~CurveConvertToBarCommand() override;
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;
		int _chartId;
		QVector<BarRecord> _records;
	};

	//粘贴节点命令（剪贴板/深拷贝子树入文档）
	//构造时持有"母本"快照（控制器在入栈前完成 id 重编与同名改名，母本 id 定稿，
	//永不入文档）；redo：插入母本的深拷贝（母本保留，支持多次 undo/redo 循环）；
	//undo：整棵删除粘贴的子树（节点随文档释放）。
	class CurvePasteNodeCommand : public QUndoCommand
	{
	public:
		CurvePasteNodeCommand(CurveDocument* doc, CurveNode* snapshot, int parentNodeId,
			QUndoCommand* parent = nullptr);
		~CurvePasteNodeCommand() override;   //释放母本快照
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;
		CurveNode* _snapshot = nullptr;   //粘贴母本（id 定稿，undo/redo 插入其深拷贝）
		int _nodeId;                      //粘贴根节点 id（=_snapshot->id，构造时定格）
		int _parentNodeId;                //目标父节点 id
	};

	//移动同层子节点行（图表骨架协调对齐设计图树顺序，如复数图 Magnitude、
	//Phase、X1）：redo 移到 toRow，undo 移回原行（仅同层单行移动）
	class CurveMoveChildCommand : public QUndoCommand
	{
	public:
		CurveMoveChildCommand(CurveDocument* doc, int chartId, int nodeId, int toRow,
			QUndoCommand* parent = nullptr);
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;
		int _chartId;
		int _nodeId;
		int _fromRow;                 //构造时行号（undo 恢复用）
		int _toRow;
	};

	//修改属性命令
	//oldValue/newValue 由控制器在构造前查询文档获得（值无变化时不构造命令）。
	//redo/undo 都是对称的 setNodeProperty 调用，触发 nodePropertyChanged，
	//图表视图与属性页随之刷新——撤销时界面自动回退。
	class CurveSetPropertyCommand : public QUndoCommand
	{
	public:
		CurveSetPropertyCommand(CurveDocument* doc, int nodeId, const QString& prop,
			const QVariant& oldValue, const QVariant& newValue, QUndoCommand* parent = nullptr);
		void redo() override;
		void undo() override;
	private:
		CurveDocument* _doc;
		int _nodeId;                  //目标节点id
		QString _prop;                //属性名（CurveProp 常量）
		QVariant _oldValue;           //旧值（undo 恢复用）
		QVariant _newValue;           //新值（redo 应用用）
	};
}
