#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线文档（数据模型层，单一数据源）
//
// CurveDocument 是 QAbstractItemModel 树模型，同时承担两个角色：
//   1. 树模型   ：曲线树视图（mCurveTreeView）直接 setModel 本文档，
//                 结构变更经 beginInsertRows/endRemoveRows 自动刷新树；
//   2. 数据中心 ：图表视图（mCurveChartView）、属性页（mCurvePropPages）
//                 通过 nodeProperty/chartNodeId 等接口读取数据，
//                 通过 nodePropertyChanged 信号感知变更后重渲染/回填。
//
// 修改纪律（重要）：
//   - 结构变更（createChart/createNode/removeNodeTree/insertNodeTree）
//     只能由命令栈（CurveCommands）调用，保证可撤销；
//   - 属性变更（setNodeProperty）同上，经 CurveSetPropertyCommand 走撤销栈；
//   - 大数据/来源信息例外：曲线数据与来源经控制器 setSeriesData/setSeriesSource
//     直写（不撤销），写入后以 "data" 属性名发 nodePropertyChanged 通知刷新。
//   - UI 层不得直接调用结构变更接口，一律经 mCurveDocController。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "CurveDefine.h"

//QT
#include <QAbstractItemModel>
#include <QHash>
#include <QIcon>

namespace PostCurve
{
	//节点状态图标（绿√+类型图标，28x16 逻辑px）：曲线树与"曲线选取器"等
	//节点宿主共用同一套节点视觉（实现在 CurveDocument.cpp，带按类型缓存）
	POSTCURVE_EXPORT QIcon curveNodeStatusIcon(CurveNodeType type);

	//文档内部节点：持有 id/类型/名称/树关系 + 按类型生效的数据结构。
	//id 全文档唯一且只增不减（撤销重做循环中保持稳定，作为跨模块引用键）。
	//注意：不对外直接暴露操作，外部经 CurveDocument 的接口访问
	struct CurveNode
	{
		int id = -1;                       //唯一id（0 为根节点）
		CurveNodeType type = CurveNodeType::Project;
		QString name;                      //节点本名（Page 1/XY Plot/Curve 1/feature名...；树显示名经 nodeDisplayName 加 p1:/w1: 前缀）
		CurveNode* parent = nullptr;
		QVector<CurveNode*> children;      //子节点（顺序即树显示顺序）

		//按类型生效的数据（仅节点类型对应的结构有效，其余保持默认值）
		CurveSeriesData series;            //Curve
		CurveAxisData axis;                //XAxis/YAxis/YAxis2
		CurveLegendData legend;            //Legend
		CurveTextData header;              //Header（数据实际挂图表节点，此成员备用）
		CurveTextData footer;              //Footer（同上）
		CurveBaselineData baseline;        //Baseline
		CurveAnnotationData annotation;    //Annotation
		CurveDatumPlane3DData datumPlane3D; //DatumPlane3D（三维基准面，R41）
		CurvePageData page;                //Page
		CurveChartData chart;              //Chart
	};

	class POSTCURVE_EXPORT CurveDocument : public QAbstractItemModel
	{
		Q_OBJECT
	public:
		//自定义角色（供树视图/过滤代理按类型与id取数据）
		enum Roles
		{
			NodeTypeRole = Qt::UserRole + 1,   //CurveNodeType 的 int 值
			NodeIdRole,                        //节点id
		};

		explicit CurveDocument(QObject* parent = nullptr);
		~CurveDocument() override;

		//--- QAbstractItemModel（供 mCurveTreeView 显示） ---
		//internalPointer 存 CurveNode*；单列树（columnCount 恒为1）
		QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
		QModelIndex parent(const QModelIndex& child) const override;
		int rowCount(const QModelIndex& parent = QModelIndex()) const override;
		int columnCount(const QModelIndex& parent = QModelIndex()) const override;
	    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
	    Qt::ItemFlags flags(const QModelIndex& index) const override;
	    //树交互写入（复选框勾选/树内重命名）：
	    //不直接改文档，而是经 mCurveDocController::setNodeProperty 走命令栈（可撤销），
	    //命令 redo 回写本文档并发 dataChanged 完成界面刷新
	    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
        
		//--- 节点查询（图表视图/属性页高频使用，均为 O(1) 或 O(子节点数)） ---
		CurveNode* node(int id) const;               //id 查节点（不存在返回 nullptr）
		int rootNodeId() const;                      //恒为 0
		CurveNodeType nodeType(int id) const;        //不存在时返回 Project
		QString nodeName(int id) const;
		//树/MDI 标题显示名（HyperGraph 风格）：页/视口/曲线 = "p<显示序> : <本名>"/
		//"w<显示序> : <本名>"/"c<显示序> : <本名>"，序为同层位置序（1基，删除自动
		//重排，与 mDefineCurveDlg 的 p1w1c1 引用编号同源）；其余类型 = 本名。
		//前缀是显示层现算，不进 node->name
		QString nodeDisplayName(int id) const;
		//全限定显示名：视口 = "p<页序>w<视口序> : <本名>"（多页视口同屏时 "w1 : ..."
		//跨页撞名，MDI 标题用它；语法与向量引用 p1w1c1 同源）；其余同 nodeDisplayName
		QString nodeFullDisplayName(int id) const;
		int parentNodeId(int id) const;              //根节点/不存在返回 -1
		QList<int> childNodeIds(int id) const;       //直接子节点id列表
		//向上查找所属图表id（图表自身返回自身id；非图表后代/不存在返回-1）
		//图表视图用它判断某节点变更是否与自己相关
		int chartNodeId(int id) const;
		//图表的类别刻度名（柱状图 X 轴）：取图表下第一条带刻度名曲线的
		//xTickLabels（定义曲线"数值"来源写入）；无则返回空（数字刻度）。
		//渲染层（applyAxis 类别刻度）与轴属性页（起始/终止下拉）共用
		QStringList chartCategoryTickLabels(int chartId) const;
		//图表的 X 数据值（柱状图 X 轴无类别刻度名时，起始/终止下拉的数字选项）：
		//取图表下第一条曲线的 xData，升序排序并去重；无曲线/空数据返回空
		QVector<double> chartBarXValues(int chartId) const;
		QModelIndex indexFromNodeId(int id) const;   //id 转模型索引（供树选中联动）

		//--- 属性读写（属性页经命令栈调用） ---
		//按 NodeType+属性名 读取（CurveProp 常量）；不支持/节点不存在返回无效 QVariant
		QVariant nodeProperty(int id, const QString& prop) const;
		//写入并触发通知（dataChanged + nodePropertyChanged）。
		//返回false表示节点不支持此属性；*oldValue 带回旧值（命令构造时取）
		bool setNodeProperty(int id, const QString& prop, const QVariant& value, QVariant* oldValue = nullptr);

		//--- 结构变更（仅由命令栈调用，UI 一律经 mCurveDocController） ---
		int allocateNodeId();                        //预分配id（命令构造时确定id，保证redo后id稳定）
		int nextDefaultNumber(CurveNodeType type);   //默认编号自增（Page 1/XY Plot/Curve 1/Datum 1...，见 curveNodeDefaultName）
		//创建页（工程的子节点），pageId 需先经 allocateNodeId 取得
		CurveNode* createPage(int pageId, const QString& name);
		//创建视口（页的子节点，自动生成 XAxis/YAxis/Legend/Header/Footer 固定子节点），
		//chartId 需先经 allocateNodeId 取得
		CurveNode* createChart(int chartId, int pageId, const QString& name);
		//创建三维图视口（chartType=5：固定骨架按 profile(5) 仅 Legend，无 X1/Y1/
		//Header/Footer；轴初值=axis3d 默认态 X/Y/Z + 0..10 + 刻度6 + 网格开 +
		//自动适配开），chartId 需先经 allocateNodeId 取得。"从文件新建三维图"
		//弹窗创建链用（R28 入树）
		CurveNode* create3DChart(int chartId, int pageId, const QString& name);
		//创建复数曲线（曲线 + x/"ym"/"yp" 三个数据壳，复数图目标导入/定义曲线用；
		//两路数据经控制器 setComplexSeriesData 直写），nodeId 需先经 allocateNodeId 取得
		CurveNode* createComplexCurve(int chartId, int nodeId, const QString& name);
		//创建柱状曲线（曲线 + 仅 "y" 一个数据壳：柱状图无横坐标数据语义，横轴为
		//类目轴，柱状图目标导入/定义曲线用），nodeId 需先经 allocateNodeId 取得
		CurveNode* createBarCurve(int chartId, int nodeId, const QString& name);
		//创建三维曲线（曲线 + "x"/"y"/"z" 三个轴壳：壳属性经重定向读写所属图表
		//axis3d 轴组；三维图目标"从文件新建三维图"用，数据经控制器
		//set3DSeriesData 直写），nodeId 需先经 allocateNodeId 取得
		CurveNode* create3DCurve(int chartId, int nodeId, const QString& name);
		//柱状曲线数据直写（"提取数值转换"命令 redo 专用）：xData/yData/xTickLabels
		//一次写入并广播 nodePropertyChanged("data")（控制器转发驱动图表视图重渲染）
		//——语义等同控制器 setSeriesData+setSeriesTickLabels 合体，但命令栈重放
		//不经过控制器，须走文档层；节点非曲线/不存在静默返回
		void setCurveBarData(int curveId, const QVector<double>& xData,
			const QVector<double>& yData, const QStringList& tickLabels);
		//创建普通节点（Curve/Baseline/Annotation/YAxis2等）。插入位置按设计图02的
		//分区顺序：曲线区 -> 图例 -> 基准区 -> 注释区 -> 轴/页眉/页脚尾部；
		//Curve 类型自动创建 x/y 两个固定子节点（数据存本节点 series.xRef/yRef）
		CurveNode* createNode(CurveNodeType type, int nodeId, int parentNodeId, const QString& name);
		//整棵删除（含子树），释放节点内存
		void removeNodeTree(int nodeId);
		//同层行移动：把 chartId 的直接子节点 nodeId 移到 toRow（骨架协调对齐
		//设计图树顺序用；beginMoveRows 保证树视图动画/选中安全）。返回是否移动
		bool moveChildNode(int chartId, int nodeId, int toRow);
		//深拷贝子树（保留原id；CurveRemoveNodeCommand 在删除前抓快照，undo时恢复）
		static CurveNode* cloneNodeTree(const CurveNode* src);
		//把快照子树挂回父节点（撤销删除），节点所有权转移给文档；
		//用于撤销恢复的子树id与删除前一致，外部持有的引用继续有效
		CurveNode* insertNodeTree(CurveNode* subtree, int parentNodeId);
		//整棵子树重编 id（allocateNodeId 顺序分配，根先子后）：粘贴出的新节点用，
		//母本快照的旧 id 可能与现存节点冲突；id 在命令入栈前定稿，undo/redo 稳定
		void reassignNodeTreeIds(CurveNode* node);
		//递归释放子树内存（快照析构时也用它）
		static void freeNodeTree(CurveNode* node);

		//--- 快照序列化支持（CurveDocSerialize 用，会话恢复直写路径） ---
		//默认编号/ID计数器集合：saveCurveDocument 采集、restoreCurveDocument 回填
		struct CurveDocCounters
		{
			int nextId = 1;
			int pageCount = 0;
			int chartCount = 0;
			int curveCount = 0;
			int baselineCount = 0;
			int annotationCount = 0;
			int datumPlane3DCount = 0;   //三维基准面默认编号计数（R41；会话快照 V7 起
			                             //持久化，V6 及更早快照读入保持缺省 0）
		};
		CurveDocCounters snapshotCounters() const;
		//整体回填计数器（不进命令栈；nextId 由调用方保证不回拨）
		void restoreCounters(const CurveDocCounters& counters);

	signals:
		//节点已加入文档（树已插入行）。视口工厂监听它：图表节点加入->自动开视口
		void nodeAdded(int id);
		//节点即将被删除（此时节点仍可查询——视口工厂用它判断被删的是否为图表）
		void nodeAboutToBeRemoved(int id);
		//节点已删除（树已移除行，节点不可再查询）
		void nodeRemoved(int id);
		//节点属性已变化（prop 为 CurveProp 属性名；曲线数据直写时为 "data"）。
		//图表视图据此重渲染，属性页据此回填（含撤销/重做引起的变更）
		void nodePropertyChanged(int id, const QString& prop);

	private:
		//节点转模型索引（工程根返回顶层第0行索引，其父为虚拟超根不可见）
		QModelIndex indexFromNode(const CurveNode* node) const;
		//节点显示名（data()/nodeDisplayName 共用）：页/视口/曲线加位置前缀
		//p<n> :/w<n> :/c<n> :，序只数同类型兄弟（页序=根下 Page 序，视口序=页下
		//Chart 序，曲线序=图表下 Curve 序，第一条 c1）
		QString nodeDisplayText(const CurveNode* n) const;
		//同类型兄弟中的位置序（1 基，数到自身为止；无父返回 0）
		int siblingPositionNumber(const CurveNode* n) const;
		//同层增删页/视口/曲线后对其父节点全部子行发 dataChanged（兄弟的 p/w/c 位置序变了）
		void refreshNumberedSiblings(CurveNode* parent);
		//按设计图02分区顺序计算新子节点的插入行（曲线->图例->基准->注释->轴/页眉页脚）：
		//返回第一个"分区序比新节点大"的子节点行号，无则追加到末尾
		int childInsertionRow(const CurveNode* parent, CurveNodeType type) const;
		//子树注册/注销到 _idMap（插入/删除时维护索引）
		void registerNodeTree(CurveNode* node);
		void unregisterNodeTree(CurveNode* node);
		//createNode 内部变体：autoDataShells=false 时不自动创建 x/y 数据壳
		//（柱状曲线走 createBarCurve 自建单 y 壳）
		CurveNode* createNode(CurveNodeType type, int nodeId, int parentNodeId,
			const QString& name, bool autoDataShells);

		CurveNode* _root = nullptr;
		QHash<int, CurveNode*> _idMap;      //id -> 节点（全树索引，含子树）
		int _nextId = 1;                    //节点id分配器（只增不减，撤销不回收）
		int _pageCount = 0;                 //页默认编号计数
		int _chartCount = 0;                //Image 默认编号计数
		int _curveCount = 0;                //Curve 默认编号计数（pch导入用feature名，不消耗编号）
		int _baselineCount = 0;             //基准线默认编号计数
		int _annotationCount = 0;           //注释默认编号计数
		int _datumPlane3DCount = 0;         //三维基准面默认编号计数（R41，会话内递增不回收）
		//nodeAdded 抑制（create3DChart 用）：Chart 节点创建信号在 createNode 尾部
		//发出，而 chartType/固定骨架此时还未就绪——视口工厂同步物化会按默认
		//类型(0=XY)选错视图类。抑制期间 createNode 不发信号，由 create3DChart
		//在骨架完成后补发（与 insertNodeTree"整树完成后发一次"同口径）
		bool _suppressNodeAdded = false;
	};
}
