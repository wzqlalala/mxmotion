#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 文档控制器（模块唯一入口/门面，单例）
//
// 持有 CurveDocument（数据模型）+ QUndoStack（撤销栈）：
//   - 对外只暴露命令式 API（addChart/addCurve/removeNode/setNodeProperty...），
//     每次调用封装为 QUndoCommand 入栈，天然可撤销/重做；
//   - 文档的 nodeAdded/nodeRemoved/nodePropertyChanged 信号在此转发
//     （sig_ 前缀），树/图表视图/属性页只需 connect 控制器，不接触文档；
//   - 选择中枢：currentNodeId 记录全局选中节点，sig_currentNodeChanged
//     驱动"树选中 -> 参数栏属性页切换"联动（设计图02/03.1）。
//
// 使用规约：
//   - UI 层（树/视图/属性页/弹窗）修改数据一律走本类 API，禁止直改文档；
//   - 批量创建（如 pch 导入：一图表+多曲线+样式）用 undoStack() 的
//     beginMacro()/endMacro() 包裹，实现一次撤销整组；
//   - 跨模块（MainWindow）只 connect 信号，不直接调用修改类 API。
//
// 典型链路（属性修改）：
//   属性页 commit() -> 本类 setNodeProperty -> push 命令 -> 文档写入
//   -> nodePropertyChanged -> 图表视图重渲染 + 其他属性页回填
// =============================================================================

#include "../PostCurveGlobal.h"
#include "CurveDefine.h"

//QT
#include <QObject>
#include <QSet>
#include <QVector>

class QUndoStack;

namespace PostCurve
{
	class CurveDocument;
	struct CurveNode;

	class POSTCURVE_EXPORT mCurveDocController : public QObject
	{
		Q_OBJECT
	public:
		static mCurveDocController* getInstance();

		//--- 数据与撤销栈访问 ---
		CurveDocument* document() const;    //文档（只读查询用：nodeProperty/childNodeIds等）
		QUndoStack* undoStack() const;      //撤销栈（批量操作用 beginMacro/endMacro）

		//整体清空文档（根除外）与剪贴板/撤销栈/选中：直写不进命令栈。
		//会话恢复（.mxses）打开前重置曲线侧用；删除联动（视口工厂关闭图表
		//卡片）与树刷新随 removeNodeTree 信号自动完成
		void clearDocument();

		//--- 命令式API（全部走撤销栈） ---
		//新建页（工程的子节点；name 非空用作本名——pch 导入传文件基名，
		//页对齐时传所选视图页名；缺省名 Page 1... 见 curveNodeDefaultName）。返回页节点id。
		//pch 导入"创建"一次生成一页，页内按布局放若干视口
		int addPage(const QString& name = QString());
		//页对应视图页ID（视图树页，曲线页与视图页对齐；-1=未绑定/待新建）。
		//UI层映射，不经撤销栈
		int pageViewPageId(int pageId) const;
		void setPageViewPageId(int pageId, int viewPageId);
		//视图页→曲线页反查（页对齐反向映射；无绑定页返回 -1）。
		//读侧查询不经撤销栈；产出链"当前视图页打开新窗口"落点定位用
		//（先反查绑定页再考虑新建，杜绝空页误建视图页）
		int curvePageIdForViewPage(int viewPageId) const;
		//页改名（经命令栈kName；页对齐时曲线页采用视图页名）
		void setPageName(int pageId, const QString& name);
		//在页下新建视口（自动生成 X1/Y1/Legend/Header/Footer 固定子节点），
		//返回视口节点id。默认名固定 "XY Plot"（同名靠树/视口标题的 w1/w2 前缀区分）；
		//视口工厂监听 nodeAdded，视口创建后视口卡片自动打开
		int addChart(int pageId);
		//在图表下新建曲线，返回曲线节点id（name 为空时自动命名 Curve 1...；
		//pch 导入时传 feature 名如 "Point id 8003-MAG"）
		int addCurve(int chartId, const QString& name = QString());
		//在图表下新建复数曲线（曲线 + x/ym/yp 三壳，复数图目标导入/定义曲线用；
		//两路数据随后经 setComplexSeriesData 直写）。name 为空时自动命名，
		//同名允许创建（R23 口径，同 addCurve）。返回曲线节点id，失败返回 -1
		int addComplexCurve(int chartId, const QString& name = QString());
		//在图表下新建柱状曲线（曲线 + 仅 y 单壳：柱状图无横坐标数据语义，横轴为
		//类目轴；柱状图目标导入/定义曲线用）。name 为空时自动命名，同名允许
		//创建（R23 口径，同 addCurve）。返回曲线节点id，失败返回 -1
		int addBarCurve(int chartId, const QString& name = QString());
		//在页下新建三维图（chartType=5，固定骨架仅 Legend；R28 入树），
		//返回视口节点id，失败返回 -1
		int add3DChart(int pageId);
		//在三维图下新建三维曲线（曲线 + x/y/z 三轴壳；曲线名缺省=Curve N，
		//R23 同名允许口径同 addCurve）。返回曲线节点id，失败返回 -1
		int add3DCurve(int chartId, const QString& name = QString());
		//XY→柱状"提取数值转换"（add-chart-type-extract-convert，截图流程②③）：
		//图内全部曲线按提取个数 extractCount 转柱状——曲线名/样式/挂轴随整节点
		//快照保留，数据=均匀 N 点提取值（含首尾）+频率类目签（CurveBarExtract）；
		//与 kChartType 类型切换同宏提交（一次撤销整批还原，重做数据保全）。
		//可见点<2 的曲线跳过保留原样；全部不可转时仅切类型（等价空图切换）。
		//图表 id 非法返回 false，其余 true
		bool convertCurvesToBar(int chartId, int extractCount);
		//在图表下新建基准线/注释（默认参数，创建后经属性页调整）
		int addBaseline(int chartId);
		int addAnnotation(int chartId);
		//在三维图下新建基准面（R41，参数栏基准工具页"新增"入口，页面接线归 T41-3）：
		//目标必须是 chartType=5 三维图（addAnnotation 的 Chart 守卫口径 + 类型守卫），
		//否则拒绝返回 -1 且 *errMsg（非空时）带原因。一个撤销宏"新建基准面"：建节点+
		//出厂默认值入档（结构体默认成员=出厂默认，redo 重建不丢），默认名"基准 N"
		//按会话内全文档按类型计数（不回收，同 2D Datum/Note 机制）。创建后不自动
		//setCurrentNodeId——页面导航由工具页决定（与 R34 注释 Ribbon 按钮行为不同，
		//入口在工具页不在 Ribbon，备案）
		int addDatumPlane3D(int chartNodeId, QString* errMsg = nullptr);
		//三维图注释共享创建例程（R42 二期建、同日用户拍板修订为三维数据空间锚点；
		//Ribbon"3D曲线→注释"按钮与三维注释管理页"新增"两入口同源）：宏"新建注释"
		//内 addAnnotation（默认计数名）+ kAttachMode=1（模型值不变，3D 语义=数据
		//空间锚点注释）+ kText="新注释" + kPosX/kPosY/kPosZ=入参三维数据坐标
		//（注释锚定三维：渲染按相机投影随视角移动，{X}{Y}{Z} 模板=注释自身位置；
		//调用方经 m3DChartView::queryAxis3DDisplayRange 取轴显示范围中点传入，
		//无可见数据退化 0,0,0）。一次 Ctrl+Z 整体撤销（含文本/位置初值）。
		//返回注释节点 id；图表 id 非法/创建失败返回 -1（类型守卫=调用方职责，
		//对齐 addAnnotation 既有口径）
		int addWindowAnnotation(int chartId, double posX, double posY, double posZ);

		//--- 注释剪贴板（R42 二期；会话级内存快照，不序列化不入文件） ---
		//生命周期=控制器单例存活期（跨会话恢复重置：clearDocument 一并失效）；
		//与节点剪贴板（copyNode 族）相互独立——注释剪贴板只存 CurveAnnotationData
		//值快照（不含节点名/树结构），粘贴按字段逐项提交进命令栈
		//深拷贝该注释节点 name 之外的全部 CurveAnnotationData（含显隐态 visible）
		//入注释剪贴板，替换旧内容。非 Annotation 节点/id 无效返回 false（剪贴板不动）
		bool copyAnnotation(int annoId);
		//复制 + 删除（与删除命令包成一个宏"剪切注释"，一次 Ctrl+Z 整体恢复原节点；
		//剪贴板不受撤销影响，可反复粘贴）。复制失败（节点无效）不动剪贴板返回 false
		bool cutAnnotation(int annoId);
		//把注释剪贴板粘贴为 chartId 图表下的新注释：宏"粘贴注释"内 addAnnotation
		//（默认计数名"Note N"）→ 逐字段提交剪贴板数据（kText/kPosX/kPosY/kPosZ/
		//kAlign/kColor/kFont/kBgColor/kBorderMode/kBorderColor/kBorderWidth/
		//kBorderShow/kAttachMode/kVisible——全字段一致含显隐态，剪切→粘贴恢复
		//语义要求）；粘贴位置=剪贴板原值原样不做偏移。走命令栈可撤销（撤销仅移除
		//粘贴节点，不清剪贴板）。返回新注释节点 id；剪贴板空/图表 id 非法返回 -1
		//（页面按钮态用 hasAnnotationClipboard 兜底）
		int pasteAnnotation(int chartId);
		//注释剪贴板是否有内容（粘贴按钮可用态判定）
		bool hasAnnotationClipboard() const;
	//在图表下添加副坐标轴（多坐标轴功能，对齐 HyperGraph）：vertical=false 顶部
	//横轴（XAxis2，默认名 X2/X3...），true 右侧纵轴（YAxis2，默认名 Y2/Y3...，
	//编号按同侧现存最大号 +1，主轴 X1/Y1 占 1 号）。初值复制同侧主轴（刻度模式/
	//范围/字体/颜色，新轴初始与主轴刻度对齐），标题默认=轴名；整个添加包成一个撤销宏
	int addAxisNode(int chartId, bool vertical);
	//旧入口（设计图07）：等价 addAxisNode(chartId, true)，已放开"每图表至多一枚"限制
	int addYAxis2(int chartId);
	//删除次坐标轴（仅 XAxis2/YAxis2，轴区/树右键"删除"入口）：走命令栈可撤销。
	//主轴与曲线 x/y 等其余固定子节点仍按 removeNode 的固定子节点规则拦截
	void removeSecondaryAxis(int axisNodeId);
		//删除节点（含子树）。固定子节点（轴/图例/页眉页脚）不允许单独删除；
		//删除前命令自动抓子树快照，撤销时原样恢复（id不变）
		void removeNode(int nodeId);

		//--- 剪贴板（模块内节点剪贴板：剪切/复制/粘贴的公共载体） ---
		//节点子树深拷贝入剪贴板（快照与文档解耦，原文档后续变化/删除不影响），
		//替换旧剪贴板内容。根节点不可复制；返回是否成功
		bool copyNode(int nodeId);
		//复制 + 删除（删除走命令栈，Ctrl+Z 可恢复原节点；剪贴板不受撤销影响，
		//可反复粘贴）。固定子节点不允许剪切（与 removeNode 同规）
		bool cutNode(int nodeId);
		//把剪贴板子树粘贴为 parentNodeId 的新子节点：id 全部新分配（母本快照
		//保留旧 id），目标父下同名自动加" (N)"序号后缀（曲线同步 series.name；
		//R23 重名允许后此自动后缀为兼容保留，既有行为零变化）。走命令栈可撤销
		//（撤销仅移除粘贴节点，不清剪贴板，可再粘贴）。
		//返回新节点 id；失败（无剪贴板/目标父不容纳该类型）返回 -1
		int pasteNode(int parentNodeId);
		//剪贴板是否有内容（粘贴菜单置灰判定）
		bool hasClipboard() const;

		//修改属性（属性名见 CurveProp；值无变化时不产生命令；kChartType 切入
		//三维图(5)被 R29 守卫静默拒绝——三维图仅经弹窗创建）
		void setNodeProperty(int nodeId, const QString& prop, const QVariant& value);

		//曲线类别打标（动刚度/VTF/NTF/目标值曲线生成功能创建曲线时写入，曲线统计
		//按类别过滤；kCurveCategory 走命令栈，撤销/重做与多视口同步语义同
		//setNodeProperty）。非曲线节点安全返回
		void setCurveCategory(int curveId, CurveCategory category);

		void undo();    //撤销（等价 undoStack()->undo()，快捷键已挂在树/图表上）
		void redo();    //重做

		//--- 信息上报（模块内所有出错/提示点经此发信号，由主窗口接到状态栏/信息输出） ---
		void reportInfo(const QString& msg);
		void reportWarning(const QString& msg);
		void reportError(const QString& msg);

		//--- 轴显示范围广播（视图缩放/平移驱动） ---
		//轴属性页"最小值/最大值"对自动范围轴回显的是引擎现值（模型 min/max 为
		//休眠值），须随缩放联动刷新；范围数值由页面经 queryAxisDisplayRange 回查
		void notifyAxisDisplayRangeChanged(int axisNodeId)
		{
			emit sig_axisDisplayRangeChanged(axisNodeId);
		}

		//--- 选择中枢 ---
		//全局选中节点id（树点击/图上点选都汇聚到这里，-1表示无选中）
		int currentNodeId() const;
		void setCurrentNodeId(int nodeId);   //变化时发射 sig_currentNodeChanged

		//--- 数据隔离：类型筛选交互作用域 ---
		//结构树类型筛选的交互允许集（筛选按钮行驱动，与树代理的显示过滤同步设置）；
		//空集合=完整结构树=全部类型可交互。树/图上点选写中枢前统一过 canSelectNode
		void setInteractiveTypes(const QSet<int>& types);
		//节点能否被交互选中（id<=0 视为清选恒可；否则按其节点类型判定）
		bool canSelectNode(int nodeId) const;
		bool isTypeInteractive(int type) const;

		//--- 直写接口（不进命令栈） ---
		//曲线数据填充（pch导入等数据源；数据体量大不适合撤销）。
		//写入后以 "data" 属性名发 sig_nodePropertyChanged 触发图表刷新
		void setSeriesData(int curveId, const QVector<double>& xData, const QVector<double>& yData);
		//三维曲线数据填充（"从文件新建三维图"弹窗；R28 入树）：xData/yData/zData
		//等长三轴 + 渲染序列类型（0 瀑布/1 曲线/2 曲面/3 柱状，对齐弹窗单选序）；
		//rows/cols=曲面/瀑布规则网格行列数（rows*cols==点数；线/柱状传 0）。
		//数据直写不进命令栈，以 "data" 属性名触发三维视图重渲染
		void set3DSeriesData(int curveId, const QVector<double>& xData,
			const QVector<double>& yData, const QVector<double>& zData, int plot3dType,
			int rows = 0, int cols = 0);
		//复数曲线两路数据填充（上面板=magData 幅值/实部，下面板=phaseData 相位/虚部，
		//X 共用 xData；复数图目标导入/定义曲线/成对换源用）。数据直写，a/b 截齐兜底，
		//以 "data" 属性名触发刷新
		void setComplexSeriesData(int curveId, const QVector<double>& xData,
			const QVector<double>& magData, const QVector<double>& phaseData);
		//三维曲线定义溯源落模（"定义曲线"三维弹窗应用时写入，重开弹窗载入既有
		//曲线时还原定义）：sourceFile=溯源标记（kMathSourceTag/kNumericSourceTag/
		//文件路径），pch 五元组与侧四元组三维定义不使用一并清空；三侧数学表达式
		//（X/Y 落 xRef/yRef.expression，Z 落 z3DExpression；非数学来源侧传空串
		//清空）与三侧文件分量列下标（落 x/y/z3DComponent；非文件来源侧传 -1）。
		//同 set3DSeriesData 数据直写口径不进命令栈、不发信号（溯源不触发渲染）
		void set3DCurveProvenance(int curveId, const QString& sourceFile,
			const QString& xExpression, const QString& yExpression,
			const QString& zExpression, int xComponent, int yComponent, int zComponent);
		//编辑曲线"隐藏"区间（设计图16，[轴,lo,hi] 三元组序列，原始数据空间）。
		//同样数据直写，以 "data" 属性名触发刷新
		void setSeriesHiddenRanges(int curveId, const QVector<double>& ranges);
		//X 类别刻度名（定义曲线"数值"来源写入；柱状图 X 轴按此渲染类别刻度）。
		//数据直写，以 "data" 属性名触发刷新
		void setSeriesTickLabels(int curveId, const QStringList& labels);
		//曲线来源信息（创建时一次性写入，供"更改曲线"（设计图13）溯源回显；
		//侧独立来源随之对齐：Y 侧 = 本通道，X 侧保持自身形态——通道形态保留
		//（数据由调用方保持不变），横坐标形态复位未记录。换源场景须先过
		//checkSourcePairing，保证 X 保持形态时数据点数成对）
		void setSeriesSource(int curveId, const QString& sourceFile, const QString& subcase,
			const QString& schema, const QString& object, const QString& component);
		//复数曲线来源信息（五元组 + 上/下面板侧分量令牌 + 展开方式；a 侧写 ySide*、
		//b 侧写 complexSide*）。b 侧缺省镜像 a 侧四元组（同块配对，subB/schemaB/objB
		//传空即旧行为）；显式传入时按实选落模（yp 与 a 侧不同块，模板管线映射
		//改配/定义曲线实选回写用）。语义同 setSeriesSource（X 侧保持形态/横坐标
		//形态复位未记录）
		void setComplexSeriesSource(int curveId, const QString& sourceFile, const QString& subcase,
			const QString& schema, const QString& object, const QString& componentA,
			const QString& componentB, int complexFormat,
			const QString& subB = QString(), const QString& schemaB = QString(),
			const QString& objectB = QString());
		//整线换源配对校验（"更改曲线"/Y 侧通道切换落模前调用）：X 侧为通道形态
		//（isChannelSchema，X=通道分量值）时，新 Y 数据须与现存 X 点数成对，
		//不成对报警并返回 false——调用方零副作用退出（校验须在压撤回快照/
		//任何写之前）。横坐标形态/未记录返回 true（X 重取记录横坐标，无配对要求）
		bool checkSourcePairing(int curveId, const QVector<double>& newValues);
	//x/y 数据参数页侧独立换源：直写侧来源四字段（静默不发信号，与
	//setSeriesSource 同语义，页面手动回填；schema 为横坐标类型时
	//object/component 传空）。非曲线节点安全返回
	void setCurveSideSource(int curveId, bool xSide, const QString& subcase,
		const QString& schema, const QString& object, const QString& component);

	//--- 批量曲线数据落地（对外门面：模块外数据源一次成图） ---
	//宏包裹整组一次撤销；静默落图（卡片只登记不切页/不激活/不物化，用户当前
	//视图页不动，双击树节点/点卡片时由懒物化机制渲染）；配色 curveDefaultColorWheel
	//轮换（并入既有图表时从其曲线数续起避免撞色）；xTitle/yTitle 非空时设到各
	//新建图表的 X1/Y1 轴标题（并入既有图表只追加曲线不覆盖既有设置）。
	//AllInOne 落 currentChartId 所属页（无/无效则新建页），NewPage/OnePerGroup/
	//OnePerCurve 新建页（图表数超页布局上限 16 时自动分多页，NTF 分页同口径）；
	//currentChartId 由调用方取视口工厂当前激活图表传入（控制层不反查视图层，
	//保持 CurveCore 无 CurveView 依赖——无头单测可直接编译本层）。
	//返回落地页 id（ExistingChart 返回并入图表所属页）；items 为空、目标失效
	//或全部曲线失败（整组回滚不留空页）返回 -1
	int addCurveBatch(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
		const QString& pageName = QString(), int existingChartId = -1,
		const QString& xTitle = QString(), const QString& yTitle = QString(),
		int currentChartId = -1);

	//addCurveBatch 的显式落点页变体（新增接口，既有 addCurveBatch 语义零变化；
	//曲线提取"云图与曲线同页对比"用）：差异仅落点页——AllInOne/OnePerGroup/
	//OnePerCurve 落 currentPageId 指定页（无/无效/非页节点则新建，页名 pageName），
	//NewPage 无视之强制新建，ExistingChart 并入既有图表（两者同 addCurveBatch）；
	//createdChartIds 非空时回填本次新建图表节点 id（ExistingChart 并入不新建
	//图表不回填；整组回滚后 id 失效，消费方仅作即时呈现/追踪用）。返回值口径
	//同 addCurveBatch
	int addCurveBatchToPage(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
		const QString& pageName = QString(), int existingChartId = -1,
		const QString& xTitle = QString(), const QString& yTitle = QString(),
		int currentPageId = -1, QList<int>* createdChartIds = nullptr);

signals:
	//信息上报（主窗口连接 -> 状态栏/信息输出窗口；文本建议带"[曲线]"前缀）
	void sig_info(const QString& msg);
	void sig_warning(const QString& msg);
	void sig_error(const QString& msg);
	//批量建卡静默开关请求（addCurveBatch 宏期间发 true/false；由视口工厂构造时
	//连接响应 setDeferCardActivation——控制层经信号协调视图层，不直接依赖
	//CurveView，无头单测层编译/运行不受影响，未连接时为空操作）
	void sig_requestDeferCardActivation(bool on);
		//选中节点变化：mMechMainWindow 连接它 -> mInfoWindow::editCurveNode 切换参数栏属性页
		void sig_currentNodeChanged(int nodeId);
		//以下为文档信号转发（外部统一 connect 控制器，不必接触文档）：
		void sig_nodeAdded(int nodeId);                 //节点已加入（视口工厂：图表->自动开视口）
		void sig_nodeAboutToBeRemoved(int nodeId);      //节点将删除（此时尚可查询类型）
		void sig_nodeRemoved(int nodeId);               //节点已删除
		void sig_nodePropertyChanged(int nodeId, const QString& prop);  //属性已变化（含撤销/重做）
		void sig_axisDisplayRangeChanged(int axisNodeId);  //轴当前显示范围变化（缩放/平移）

	private:
		mCurveDocController(QObject* parent = nullptr);
		~mCurveDocController() override;
		mCurveDocController(const mCurveDocController&) = delete;
		mCurveDocController& operator=(const mCurveDocController&) = delete;

		static mCurveDocController* _instance;   //单例
		CurveDocument* _document;                //文档（父子关系，随控制器销毁）
		QUndoStack* _undoStack;                  //撤销栈（批量操作用 beginMacro/endMacro）
		CurveNode* _clipboard = nullptr;         //节点剪贴板（子树快照，模块自持，析构释放）
		CurveAnnotationData _annoClipboard;      //注释剪贴板（R42 二期：CurveAnnotationData 值快照，
		                                         //会话级内存不序列化，随控制器单例存活）
		bool _annoClipboardValid = false;        //注释剪贴板有效位（默认值快照与"无剪贴板"需区分）
		int _currentNodeId = -1;                 //当前选中节点id
		QSet<int> _interactiveTypes;             //类型筛选交互允许集（空=完整结构树）

	private:
		//addCurveBatch/addCurveBatchToPage 共用实现（两公开口语义见各自文档）：
		//pageLanding=true 按 currentPageId 落页（NewPage 无视，非页节点/已删走
		//新建），false 按 currentChartId 落页（仅 AllInOne 生效）；createdChartIds
		//非空回填本次新建图表 id
		int addCurveBatchImpl(const QList<CurveBatchItem>& items, CurveBatchLayout layout,
			const QString& pageName, int existingChartId, const QString& xTitle,
			const QString& yTitle, bool pageLanding, int currentPageId,
			int currentChartId, QList<int>* createdChartIds);

		//图表类型切换的骨架协调（kChartType 宏内调用，结构变更全走命令栈）：
		//按新旧类型声明的 fixedChildren 补删固定子（进入复数补 YAxis2"Phase"并复制
		//主轴初值，退出删除）、尾部区顺序对齐设计图、图表名仍为旧类型默认名时随类型
		//改名（XY Plot -> Complex Plot；用户改过名不动）
		void reconcileChartSkeleton(int chartId, int oldType, int newType);
	};
}
