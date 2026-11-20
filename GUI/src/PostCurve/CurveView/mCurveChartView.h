#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 图表视图（渲染层，引擎：PlotView::mPlotView 即 Qwt 封装）
//
// 一个 mCurveChartView 绑定一个图表节点（chartId），是"模型 -> 画面"的单向投影：
//   - 构造时订阅控制器信号，节点增删/属性变化后按模型内容重渲染；
//   - 自身不持有业务状态，任何刷新都以 CurveDocument 中的数据为准，
//     因此撤销/重做与其他视图的修改自动反映到画面。
//
// 渲染映射（模型 -> PlotView）：
//   Curve    -> updateCurveData + setCurveStyle（颜色/线型/线宽/符号）
//   XAxis    -> AxisId::X_Bottom   YAxis -> Y_Left   YAxis2 -> Y_Right
//              （经 updateAxisScale/Log/Appearance/Range 四个粒度接口）
//   Legend   -> PlotSettings.showLegend
//   Header   -> setHeaderText/setHeaderStyle/setHeaderAlignment（引擎页眉条带，
//               绘图区上方；空文本自动隐藏）
//   Footer   -> setFooterText/setFooterStyle/setFooterAlignment（引擎页脚条带，
//               绘图区下方；接入方式与页眉对称）
//   Baseline -> addMarkerLine（纵向=固定X值，横向=固定Y值）+ 线样式
//   Annotation -> PlotView::mPlotAnnotation 库图元（边框文本框；曲线注释带
//              引出线且整体用曲线色，无锚点符号；配色/最小化/自动定位走
//              AnnotationSettings）。Ribbon"注释"按钮进入注释模式：左键打点创建
//              （曲线上=模板求值+引出线，曲线外=先置"视图注释"占位），
//              悬停曲线实时预览；退出后双击注释边框内部弹出注释文本对话框
//              （"视图注释"占位回填默认模板）。单击注释选中（主题蓝选中框+
//              八向手柄，设计图15），右键注释弹菜单（编辑弹属性编辑器/删除）
//
// 骨架阶段策略：任何相关变化都全量 rebuild（简单可靠，代价是丢失用户缩放状态），
// 后续可按节点类型做增量刷新。
// 例外：注释的创建/属性变化/删除走增量刷新（只增删对应渲染项），保留用户缩放。
//
// 交互层（设计图03.1/05/06，经画布/轴控件事件过滤器 + 引擎信号实现）：
//   单击曲线/图例项 -> 选中（曲线变主题蓝+加粗1px，树与参数栏经选择中枢联动）；
//   双击曲线/图例项 -> 请求弹出浮动属性编辑器（视窗左上角）；
//   右键曲线/图例项 -> 上下文菜单（编辑/删除等）；
//   hover 曲线 -> 仅变主题蓝不加粗。图例单击隐藏曲线的行为已在引擎侧关闭。
//   双击纵/横轴标题 -> 激活坐标轴属性编辑器（标题变主题蓝+加粗，视窗左上角弹窗，
//   设计图05/06；轴标题画在 QwtScaleWidget 上，画布过滤器覆盖不到，单独挂过滤器）
//   双击页眉/页脚条带 -> 激活页眉/页脚属性编辑器（条带文字主题蓝+加粗，同轴标题
//   先例；条带控件经引擎 headerWidget()/footerWidget() 访问器挂事件过滤器）
//
// 生命周期：由 mCurveViewportFactory 创建并放入 MDI 视口；图表节点被删除
// （含撤销新建）时发射 sig_requestClose，由工厂关闭对应视口。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "../CurveCore/CurveDefine.h"
//PlotView::AxisId（applyAxis 参数）与引擎轴设置结构；qwt 头随之一并可用
#include "mPlotStructs.h"

//QT
#include <QWidget>
#include <QHash>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPointer>

class QwtPlotItem;
class QwtPlotCurve;
class QEvent;

namespace PlotView
{
	class mPlotView;
	class mPlotAnnotation;
}

namespace PostCurve
{
	class mCurveReadoutItem;
	class mCurveTrimMarker;
	class mCurveBarLabelItem;
	class mCurveFrequencyCursor;
	class mCurveBaselineLabel;
	class mCurvePolarFrameItem;
	class mCurvePolarSeriesItem;
	class mCurveAxisStripWidget;
	struct CurveNode;   //transformedComplexBSeries 参数（实现经 cpp 包含 CurveDocument.h）

	class POSTCURVE_EXPORT mCurveChartView : public QWidget
	{
		Q_OBJECT
	public:
		//chartId：绑定的图表节点id（构造即订阅信号并做首次 rebuild）
		explicit mCurveChartView(int chartId, QWidget* parent = nullptr);
		~mCurveChartView() override;

		int chartId() const;    //绑定的图表节点id

		//数值变换（对齐 NVHCurveWgt::transformValue 语义）：0 Linear 原值 /
		//1 Logarithmic=log10 / 2 dB20=20log10(val/ref) / 3 dB10=10log10(val/ref)；
		//dbRef 即轴节点参考值（axis.dbRef，仅 dB 模式参与，<=0/NaN 按 1 兜底），
		//改参考值=整条曲线平移常数偏移（对齐 HyperGraph2024 交互）；
		//非法点（<=0）返回 NaN，由调用方过滤（对齐原型断点行为）。
		//公开静态：编辑曲线会话（视口工厂算数据界/浮动窗口"裁剪"落盘）与
		//本视图渲染共用同一变换，保证范围值与画面刻度一致
		static double transformValue(double val, int scaleMode, double dbRef = 1.0);
		//逆变换（渲染空间 -> 原始数据空间；Linear 恒等）："裁剪"按显示范围
		//过滤模型原始数据时使用（对数/dB 轴下必须逆变换回去再过滤）；
		//dB 逆变换同样带参考值（ref·10^(t/系数)），与正变换互逆
		//注意：仅撤销刻度变换、不撤销计权——标量无频率上下文，计权修正
		//（随频率逐点变化）无标量逆
		static double inverseTransformValue(double val, int scaleMode, double dbRef = 1.0);

		//统一逐点入口（"计权 -> 刻度"渲染管线，对齐原型 applyWeight(y, weight, x)
		//先加权后变换的顺序）：X 计权以自身原始值为频率，Y 计权以同一索引的
		//X 原始值为频率（线性增益乘原始值后再做刻度变换）。所有"逐点渲染
		//空间换算"（transformedSeries/自动范围/编辑截取/工厂界值）必须经此，
		//保证曲线与游标/基准线/读数等一切数值显示同源一致；
		//xDbRef/yDbRef 为挂轴侧轴节点参考值（dB 模式下除入 ref，默认 1=无参考），
		//与 xMode/yMode 同源（同一根轴节点的 axis.dbRef）；
		//变换出 NaN（负值取对数等）按原约定交调用方剔除
		static void transformPoint(double rawX, double rawY,
			int xMode, int yMode, int xWeight, int yWeight,
			double& tx, double& ty,
			double xDbRef = 1.0, double yDbRef = 1.0);

		//轴当前实际显示范围（原始数据空间）：读引擎轴当前刻度区间（自动范围
		//fit/用户缩放后的现值；副轴 X3../Y3.. 的自绘条带镜像的就是同一根轴），
		//按该轴刻度模式反变换。模型 min/max 是休眠的手动值（自动范围渲染现算
		//不回写），属性页在 autoScale 下经此回显实际值。失败（节点不属于本
		//图表/引擎范围非法）返回 false，调用方回退模型值
		bool displayedAxisRange(int axisNodeId, double* lo, double* hi) const;

		//跨视图查询：按轴节点所属图表找已打开视口取实际显示范围（无视口/
		//失败返回 false）。属性页族（CurvePages）经此查视图状态——依赖方向
		//Pages->View 单向（View 不依赖 Pages，维持既有分层）
		static bool queryAxisDisplayRange(int axisNodeId, double* lo, double* hi);

		//按模型内容全量刷新：清空曲线/基准线后遍历图表子节点逐一应用，最后适配范围。
		//注意：批量修改（如 pch 导入宏）应经 scheduleRebuild 合并，避免重建风暴
		void rebuild();

		//合并刷新（防抖）：信号触发的刷新统一延迟到事件循环，一次执行。
		//宏内几十条属性命令只产生一次重建（PlotView/Qwt 状态经不起高频清空重建，
		//且避免闪烁）。preserveView=true 时重建前后捕获/恢复"用户缩放过"的轴
		//分隔（样式/标签/增删曲线等值空间不变的变更不打断用户视图）
		void scheduleRebuild(bool preserveView = false);

		//宿主自绘层几何显式同步（幂等）：把"画布 Resize→0ms→updateAxisStrips"
		//事件链的终点收口为公开入口，供报告导出等"物化后不等事件循环立即 grab"
		//的场景显式调用（详见实现注释）。只写几何，不碰缩放/激活态/焦点；
		//已同步视图重复调用为同值写短路（Qt 原生同矩形 setGeometry 不重绘），
		//对正常显示页零干扰
		void syncSelfDrawnGeometry();

	//注释模式（Ribbon"注释"按钮经视口工厂广播）：开着时左键打点创建注释、
	//悬停曲线实时预览注释内容；关闭后注释保持显示，双击注释可编辑文本
	void setAnnotationMode(bool on);
	bool isAnnotationMode() const { return _annotationMode; }

	//基准线模式（Ribbon"基准线"下拉经视口工厂广播，设计图14）：开着时画布
	//左键单击创建基准线（纵向取X轴值/横向取Y轴值，悬停曲线时优先取吸附点）、
	//悬停曲线显示 X/Y 读数；关闭后双击基准线/右键"编辑"弹属性编辑器
	void setBaselineMode(bool on, int orientation);
	bool isBaselineMode() const { return _baselineMode; }

	//特定频率红线游标（NVH 模态贡献量，视图态不进命令栈，design D4）：
	//freq<0 关闭摘除，否则上/复数下面板红竖线（数据坐标随缩放联动）。
	//公开方法：面板经 chartView(chartId) 下发
	void setFrequencyCursor(double freq);

	//极坐标"首尾相接"形态开关（NVH 模态贡献量，视图态不进命令栈）：true=
	//向量端点按序连线（设计图04/14 语义），false=散开（极点->端点射线+箭头）。
	//公开方法：面板经 chartView(chartId) 下发
	void setPolarChainForm(bool chain);
	bool isPolarChainForm() const { return _polarChainForm; }

	signals:
		//图表节点本身被删除（撤销新建）时请求关闭所属视口（工厂连接）
		void sig_requestClose();

	//请求弹出浮动属性编辑器（图上双击曲线/图例双击/右键菜单"编辑"）。
	//globalPos 为弹窗落点（设计图03.1：双击曲线时弹在当前视窗左上角），
	//经视口工厂中继到 mMechMainWindow -> mCurvePropPageManager::showEditorPopupAt
	void sig_requestEditorPopup(int nodeId, const QPoint& globalPos);

	//Shift+左键点画布曲线的拾取广播（定义曲线弹窗"数学运算"来源 X/Y 向量
	//引用的拾取通道；经视口工厂 sig_curvePicked(chartId, curveNodeId) 中继，
	//曲线节点id 由监听方按文档 p/w/c 编号合成引用）。命中曲线才发射，
	//事件被消费不触发常规点选
	void sig_curvePicked(int curveNodeId);
	//NVH 模态贡献量：画布左键频率拾取（X 已吸附首条可见曲线最近数据点；工厂
	//中继时带 chartId，面板按产出集合过滤后三路回写，design D4）
	void sig_canvasFrequencyPicked(double xValue);

	private:
		//画布上最近曲线点的上下文（注释锚点吸附与 {Label}/{Slope}/{Curv} 求值用）。
		//坐标/斜率/曲率均为渲染（刻度变换后）空间
		struct CurvePointCtx
		{
			bool valid = false;
			QString curveName;   //所属曲线的模型名（{Label} 显示与注释 kAttachCurve
			                     //模型写入用；引擎侧查曲线须用键，见 keyFor）
			double x = 0.0;      //锚点/最近点数据坐标
			double y = 0.0;
			double slope = 0.0;  //dy/dx（中心差分，端点用单侧差分）
			double curv = 0.0;   //曲率（三点 Menger 曲率）
			int index = -1;      //最近点采样序号（0基，附着点顺序回显用）
		};

		//画布尺寸变化时同步边框覆盖层的几何（覆盖层铺满画布绘制闭合矩形）；
		//注释模式的鼠标事件在此拦截（本视图的过滤器晚于引擎安装，先于引擎收到）；
		//非注释模式下实现单击选中/hover 高亮/双击弹编辑器（设计图03.1）
		bool eventFilter(QObject* watched, QEvent* event) override;

	//注释渲染（锚点符号+边框文本框，曲线注释带引出线），rebuild 与增量刷新共用。
	//只创建/挂载项，不主动重绘（由调用方统一 replot）
	void applyAnnotation(int annotationNodeId);
	//注释选中态同步：把 _selectedAnnotationId 写到各注释渲染项（主题蓝选中框）
	//并重绘；不重建项（增量刷新/rebuild 重建项时由 applyAnnotation 重放选中态）
	void applyAnnotationSelection();
		//清掉本视图全部注释标记（rebuild 起点调用；标记由本视图自管生命周期）
		void clearAnnotationMarkers();
		//注释增量刷新合并入口：节点创建/属性变化经 0ms singleShot 去重，
		//一次宏内多条信号只重画一遍（避免全量 rebuild 重置用户缩放状态）
		void scheduleAnnotationRefresh(int annotationNodeId);
		void flushAnnotationRefresh();
		//悬停实时预览：按模板求值注释显示在吸附锚点上（引擎 snapper 吸附信号驱动）
		void updateAnnotationHover(const CurvePointCtx& ctx);
		//最小化小方块悬停：命中则该注释临时还原完整样式，移出/离画布复原
		void updateAnnotationMinimizeHover(const QPoint& canvasPos);
		//隐藏悬停预览标记（必要时重绘）
		void hideAnnotationHover();
		//注释模式内左键：创建注释节点（一次撤销宏：类别/附着曲线点/文本/
		//框位置/标量格式化；posX/posY=注释框中心数据坐标）
		void addAnnotationAt(const QPoint& canvasPos);
		//退出注释模式后的双击命中测试：命中返回注释节点id，未命中返回 -1
		int hitAnnotation(const QPoint& canvasPos) const;
		//弹出注释文本对话框（mPostAnnotationDialog），保存则写回 kText 属性
		void showAnnotationEditor(int annotationNodeId);
		//选中注释左键拖动（选中后拖动）：已选中注释再按进入拖动，未选中首按
		//只选中（防误拖）；拖动中实时改渲染项框位（不进模型，无撤销垃圾），
		//松手一次提交 kPosX/kPosY（撤销宏）。最小化/曲线类自动定位注释不拖：
		//显示位置由锚点或策略派生，拖框看不到位移（视图类自动定位不生效可拖）
		void startAnnotationDrag(int annotationNodeId, const QPoint& canvasPos);
		void moveAnnotationDrag(const QPoint& canvasPos);
		void finishAnnotationDrag();
		//在指定曲线上找离画布位置最近的采样点（不限半径），填充上下文；
		//曲线不存在/不可见/无数据返回 false。第一参数为引擎键（R23，
		//引擎 getCurve 查找用），ctx.curveName 回填模型名
		bool nearestCurveContextOn(const QString& engineKey, const QPoint& canvasPos, CurvePointCtx& ctx) const;
		//取指定曲线第 index（0基，越界钳制）个采样点的上下文；曲线无效返回 false。
		//第一参数为引擎键（R23），ctx.curveName 回填模型名
		bool curvePointContextAt(const QString& engineKey, int index, CurvePointCtx& ctx) const;
		//曲线类注释附着回填（附着点序号优先，未指定则离注释框最近点），
		//占位文本回填曲线模板；序号越界回写钳制值；框按"曲线上创建"样式
		//摆到锚点右上 curveBoxOff 处（切换附着时挪框，而非只延长引出线）；
		//整体包成一个撤销宏。byPosition=true 忽略模型里的旧附着点序号，
		//按框当前位置取最近点（视图切曲线用：序号是上次曲线附着残留，
		//按它回跳不符合预期）
		void snapAnnotationToCurve(int annotationNodeId, bool byPosition = false);
		//注释模板求值：{X}/{Y}/{Time} 取锚点坐标；{Label}/{Slope}/{Curv} 取曲线
		//上下文（无曲线上下文时以 "-" 占位）。勾选标量值格式化时数值按
		//格式（0科学计数法 1定点格式）与精度显示，否则默认 g/6
		QString evaluateAnnotationText(const QString& tpl, double x, double y,
			const CurvePointCtx& ctx, bool fmtOn = false, int fmtMode = 0,
			int fmtPrecision = 5) const;
		//悬停预览/新建注释沿用的模板来源：本图表最后一个有效注释节点
		//（非空且非"视图注释"占位文本），无则返回 nullptr
		const CurveAnnotationData* templateSourceAnnotation() const;
		//悬停预览/新建注释用的模板：本图表最后一个注释节点的文本，无则默认
		QString currentAnnotationTemplate() const;
		//数据坐标 -> 画布像素（下/左轴映射）
		QPoint dataToCanvas(double x, double y) const;

		//--- 基准线交互（Ribbon"基准线"下拉经视口工厂广播，设计图14） ---
		//基准线节点应用到 PlotView（rebuild 与增量刷新共用）：可见则建/改
		//markerLine（名 baseline_%1，Z50 在曲线上层）与线上标签图元（文本=标签，
		//空不显示，字体/9宫格位置随"显示"组），不可见则一并移除；
		//只增删改项，不主动 replot（由调用方统一重绘）
		void applyBaseline(int baselineNodeId);
		//摘除单条基准线的标签图元（显隐/删除走；析构即 detach，不 replot）
		void removeBaselineLabel(int baselineNodeId);
		//清掉本视图全部基准线标签图元（rebuild 起点/析构调用）
		void clearBaselineLabels();
		//柱状图柱顶数值标注（自管图元，chartType=1 重建时随首条可见曲线重算；
		//非柱状图/无可见曲线摘除。只增删改项，不主动 replot）
		void applyBarLabels();
		//清掉柱顶标注图元（rebuild 起点/析构/非柱状图调用）
		void clearBarLabelItem();
		//极坐标画布接管（design D5，自管图元随全量重建重建）：
		//applyPolarDecor=底图+向量条目重算重挂（半径域/刻度归一收口于此）；
		//clearPolarItems=重建起点/析构/非极坐标图摘除；isPolarChart=本图表
		//chartType==3 判定；hitPolarSeries=画布点选的向量端点命中（返回曲线名）
		void applyPolarDecor();
		void clearPolarItems();
		bool isPolarChart() const;
		bool hitPolarSeries(const QPoint& pos, QString& name) const;
		//游标重放（rebuild 尾部调用：复数下面板随重建销毁重建，重挂+回写值；
		//setFrequencyCursor 公开入口走此实现）
		void applyFrequencyCursorVisuals();
		//画布点选 X 吸附：首条可见曲线 xData 最近点（无可见曲线原值返回）
		double snapToNearestDataX(double x) const;
		//基准线增量刷新合并入口：节点创建/属性变化经 0ms singleShot 去重，
		//一次宏内多条信号只重画一遍（不走全量 rebuild，保留用户缩放状态，
		//与注释同策略）
		void scheduleBaselineRefresh(int baselineNodeId);
		void flushBaselineRefresh();
		//轴刻度细项增量刷新合并入口（刻度总数/生成方式/步长/网格数/格式/精度/
		//每十倍参数）：0ms singleShot 去重后只重推受影响轴的刻度引擎/网格/数字
		//格式，不走全量 rebuild（保留用户缩放/平移状态）
		void scheduleAxisRefresh(int axisNodeId);
		void flushAxisRefresh();
		//曲线外观增量刷新合并入口（线色/线宽/线型/区域填充/符号属性）：
		//0ms singleShot 去重后按 applySeries 直发引擎，不走全量 rebuild
		//（保留用户缩放/平移状态，与注释/基准线/轴刻度同策略）
		void scheduleSeriesStyleRefresh(int curveNodeId);
		void flushSeriesStyleRefresh();
		//基准线模式内左键：创建基准线节点（一次撤销宏：方向/位置值；悬停
		//曲线时位置取吸附点，否则取点击处轴坐标）
		void addBaselineAt(const QPoint& canvasPos);
		//退出基准线模式后的命中测试：命中返回基准线节点id，未命中返回 -1
		//（命中带=线两侧 kBaselineHitPx 像素，后画的线在上层优先）
		int hitBaseline(const QPoint& canvasPos) const;
		//基准线模式内悬停读数：吸附点处显示"X: … Y: …"（值格式与轴刻度一致）
		void updateBaselineReadout(const CurvePointCtx& ctx);
		//隐藏悬停读数项（必要时重绘）
		void hideBaselineReadout();

private slots:
	//节点加入：属于本图表（或就是本图表）-> scheduleRebuild
	void onNodeAdded(int nodeId);
	//节点删除：是本图表 -> 发 sig_requestClose；否则保守合并刷新
	//（节点已删无法反查所属，骨架阶段可接受）
	void onNodeRemoved(int nodeId);
	//属性变化：属于本图表 -> scheduleRebuild（含曲线数据 "data" 直写触发）
	void onNodePropertyChanged(int nodeId, const QString& prop);
	//全局选中节点变化（树点击/图上点选/图例点击都汇聚到选择中枢）：
	//选中节点是本图表的曲线 -> 高亮之；是本图表的注释 -> 点亮主题蓝选中框；
	//否则清除本图高亮与注释选中
	void onCurrentNodeChanged(int nodeId);
	//引擎 snapper（仅吸附模式）悬停广播
	void onSnapHover(bool active, const QString& curveName, int sampleIndex, const QPointF& valuePos);

	//用户平移/缩放（引擎实时平移/滚轮缩放广播）：同步引擎管不到的自绘
	//副轴条带（自动范围=镜像引擎轴当前范围）；只轻量刷条带，不走 rebuild
	void onViewRangeChanged();
	//向控制器广播本图表全部轴"当前显示范围已变化"（缩放/平移/fit 后），
	//轴属性页"最小值/最大值"收到后回查回显（只影响面板数字，不写模型）
	void notifyAxisDisplayRanges();

	//--- 编辑曲线会话（视口工厂广播；非目标视窗的调用全部早退） ---
	//会话开启且目标为本图表：建立裁剪线/压暗覆盖层
	void onEditCurveStarted(int chartId);
	//目标曲线集更换：重建各目标截取段并重压暗（段样式随各自目标）
	void onEditCurveTargetChanged();
	//范围变化（拖拽/输入/步进）：同步裁剪线与截取段
	void onEditCurveRangeChanged(double start, double end);
	//方向变化（水平/垂直）：裁剪线换向并重采样
	void onEditCurveDirectionChanged(int direction);
	//会话结束：清理覆盖层并还原曲线样式
	void onEditCurveStopped();

private:

	//单条曲线应用到 PlotView：按曲线 x/y 侧的挂轴设置（axisAssign）与对应轴刻度
	//模式做数据预变换（模型存原始值），滤除变换后非法点（对齐原型断点行为），
	//应用线条/符号样式；y 侧 1=Y2(引擎右轴)/x 侧 1=X2(引擎上轴)，副轴节点
	//不存在时钳回主轴
	void applySeries(int curveNodeId);
	//数据预变换（applySeries 与复数收集共用）：按给定刻度模式（负值=取主轴
	//当前模式）+ 计权模式（负值=取主轴当前计权）+ 参考值（负值=取主轴当前
	//axis.dbRef）逐点变换 + 滤非法点，有效点为空返回 false
	bool transformedSeries(const CurveSeriesData& s,
		QVector<double>& xs, QVector<double>& ys, int xMode = -1, int yMode = -1,
		int xWeight = -1, int yWeight = -1,
		double xDbRef = -1.0, double yDbRef = -1.0) const;
	//复数图重建（图表类型2，设计图02-02）：复数曲线为单节点（series 自带
	//上/下面板两路数据，complexYData=相位/虚部），按 isComplex() 分派——
	//复数节点经引擎 updateComplexCurveData 走双面板（名字即引擎键）；普通
	//曲线（含相位侧为空的降级）走上面板常规路径。旧"按命名约定跨节点配对"
	//（classifyComplexName）随模型合并退役
	void applyComplexSeries();
	//复数曲线单节点显示序列（上/下面板索引对齐）：逐点同时变换两路——上面板
	//y 走 yRef+主轴模式，下面板 y 走 complexRef+Phase 轴模式；隐藏区间同索引
	//NaN 占位（严格等长，满足引擎 updateComplexCurveData 等长校验）
	bool transformedComplexPair(const CurveNode* n, QVector<double>& xs,
		QVector<double>& aOut, QVector<double>& bOut) const;
	//复数曲线增量刷新：数据+样式一并下发（updateComplexCurveData 缺失自动
	//重建上/下面板 + setComplexCurveStyle 样式；flushSeriesStyleRefresh 分派）
	void refreshComplexCurve(int curveNodeId);
	//单根坐标轴应用到 PlotView 的指定引擎轴（X_Bottom/Y_Left/X_Top/Y_Right）：
	//标题/字体/网格；恒为线性轴（刻度模式经数据预变换实现）；手动范围在此下发。
	//引擎上/右轴为副轴专用：启用刻度/数字组件（构造期默认全关）、不画网格
	//（主轴已画，副轴再画会叠出双线）
	void applyAxis(int axisNodeId, PlotView::AxisId engineAxis,
		const QString& titleOverride = QString());
	//图例应用到 PlotView（设计图08）：显隐/字体(加粗)/背景走 PlotSettings，
	//边框(样式+颜色)/曲线文本色(跟随曲线或自定义)/引线位置/选中蓝框走引擎
	//mPlotLegendItem setter；位置=自动定位(四角避让)或手动(图内四角/自定义
	//百分比；图外四向待引擎外置图例，暂映回图内右上)
	void applyLegend(int legendNodeId);
	//图例位置-手动：模型 9 位置枚举 -> 引擎 LegendPosition（4=用户自定义
	//经 setFloatingPos 下发画布百分比）
	void applyLegendPosition(const CurveLegendData& legend);
	//图例位置-自动定位：图内四角按"右上优先"试摆，取首个不遮挡可见曲线
	//的位置（全遮挡时取遮挡采样点最少者）
	void applyLegendAutoPosition();
	//图例选中态同步：_selectedLegendId 写入引擎（主题蓝外框）并重绘；
	//全量 rebuild 由 applyLegend 重放
	void applyLegendSelection();
	//可见曲线采样点落入 rect 的计数（抽样上限约 200 点/线，自动定位评遮挡用）
	int countCurvePointsInRect(const QRect& rect) const;
	//页眉应用：引擎 PlotView 页眉条带（绘图区上方，空文本自动隐藏），
	//自选字体+左/中/右对齐（引擎条带独立于 Qwt 内部布局，导出图像一并导出）。
	//逐行颜色/加粗按选中态着色（仿 applyComplexLowerAxis 先例）：选中=
	//主题蓝+全行加粗（视图状态，重放/rebuild 内自动生效）；未选中=模型色
	//（CurveTextData.color，kHeaderColor；无效兜底黑）+模型字体原样
	void applyHeader();
	//页脚应用：引擎 PlotView 页脚条带（绘图区下方），数据与渲染同页眉对称
	void applyFooter();
	//自动范围轴的数据适配：变换空间取 min/max，精确贴合数据边界不留白
	//（左/下=数据最小值，右/上=数据最大值；刻度模式切换后重建即按此适配）
	void fitChartAxes();
	//零线十字：双 Linear 时绘制 x=0/y=0 黑色 2px 实线（对齐原型）
	void addZeroCrossLines();
	//取本图表某类型轴节点的刻度模式（无该轴节点返回 0=Linear）
	int axisScaleModeOf(CurveNodeType type) const;
	//取本图表某类型轴节点的计权模式（镜像 axisScaleModeOf 遍历规则，无返回 0=无）
	int axisWeightModeOf(CurveNodeType type) const;
	//取本图表某类型轴节点的 dB 参考值（镜像 axisScaleModeOf 遍历规则，无返回 1=无参考）
	double axisDbRefOf(CurveNodeType type) const;

	//--- 引擎键 <-> 节点映射（R23 曲线允许重名：引擎 m_curves 按名哈希无法承载
	//两条同名曲线，渲染键与模型名解耦——键视口内唯一，名可重复） ---
	//对本图表全部 Curve 节点生成引擎键并重建双向映射（rebuild 起点、引擎
	//clearAll 之后调用）：模型名首次出现 -> 键=名（无碰撞图表键=名，存量
	//行为逐位不变）；第 N 次重复 -> 依次尝试 名#2/名#3… 取首个不与任何模型名
	//及已生成键冲突者（防用户真建了名为 "STRAIN#2" 的曲线时串键）。复数曲线
	//（单节点）同规则。增量路径（增删曲线/换图表类型/撤销重做）均经
	//scheduleRebuild 到达本函数全量重建；曲线改名（kName）同样走全量重建——
	//改名可改变同名组的"首个"归属（如首条 STRAIN 改名后第二条的键由
	//"STRAIN#2" 回归 "STRAIN"），任一键变化都伴随引擎全量重画，映射随
	//rebuild 同步重建，不做增量键补算（改名低频，安全优先）
	void ensureEngineCurveKeys();
	//曲线节点id -> 引擎键（映射缺失兜底回退模型名，等同既有按名行为）
	QString keyFor(int curveNodeId) const;
	//模型名 -> 引擎键（本图表内首个同名曲线节点；无匹配回退原名——键=名的
	//无碰撞路径两者恒等）
	QString engineKeyForCurveName(const QString& modelName) const;
	//引擎键 -> 模型名（{Label} 模板显示与注释 kAttachCurve 模型写入用；
	//键未登记时回退键本身）
	QString curveModelName(const QString& engineKey) const;

	//--- 曲线选中/hover 交互（设计图03.1） ---
	//引擎键 -> 本图表曲线节点id（经 _nodeByEngineKey 反查，重名曲线各自命中；
	//键未登记时回退按 series.name 匹配首条——映射缺失/键=名的历史路径兜底，
	//无匹配返回 -1）
	int engineNodeByKey(const QString& key) const;
	//写入选择中枢（树/参数栏/曲线树回选联动经控制器信号闭环）
	void selectCurve(int curveNodeId);
	//选中/hover 高亮应用与恢复：直改 QwtPlotCurve 画笔（选中=主题蓝+加粗1px，
	//hover=仅主题蓝），不碰引擎 CurveSettings（选中是视图状态非模型属性，
	//图例图标本色不受影响）；选中与 hover 叠加时选中优先。
	//编辑曲线会话期间直接返回：压暗（20%不透明度）是该期间的视图状态
	void applyHighlightVisuals();
	//基准线 hover 高亮应用与恢复：hover=主题蓝+加粗1px（视图状态不进模型/
	//命令栈，同曲线高亮策略），恢复时颜色/线宽从模型重读；全量重建与基准线
	//增量刷新重置画笔后重放（rebuild 尾部 / flushBaselineRefresh）
	void applyBaselineHoverVisuals();
	//--- 编辑曲线会话覆盖层（设计图16；范围/目标等状态只存视口工厂，本视图纯投影） ---
	//覆盖层构建：截取段旁路曲线 + 两条裁剪线（不进引擎注册表，不占图例），
	//随后压暗全部曲线；清理=删除图元并按引擎设置还原样式
	void buildEditOverlay();
	void clearEditOverlay();
	//全部引擎曲线置 20% 不透明度（含符号；设计图16编辑态视觉）
	void applyEditDimming();
	//按引擎缓存设置重设曲线样式（画笔/符号回满不透明度，会话结束还原用）
	void restoreEditCurveStyles();
	//截取段重采样 + 裁剪线位置/方向/拖拽高亮同步（范围/目标/方向变化后）
	void refreshEditSegment();
	//裁剪线命中（±5px 像素带）：0=起始 1=终止，未命中返回 -1
	int hitEditTrimLine(const QPoint& canvasPos) const;
	//编辑方向轴上：画布像素 -> 渲染空间值（水平取x / 垂直取y）
	double editAxisPixelToValue(const QPoint& canvasPos) const;
	//--- 轴标题激活交互（设计图05/06） ---
	//本图表 YAxis/XAxis 子节点id（无返回 -1）
	int yAxisNodeId() const;
	int xAxisNodeId() const;
	//本图表 Legend 子节点id（无返回 -1；双击图例空白/右键"编辑"的弹窗目标）
	int legendNodeId() const;
	//本图表 Header/Footer 占位子节点id（type 取 Header/Footer，无返回 -1；
	//双击条带选中与弹窗目标，仿 legendNodeId 遍历）
	int bandNodeId(CurveNodeType type) const;
	//轴标题带命中：QwtScaleWidget 无公开标题区域 API，按布局近似——左轴标题
	//旋转90°绘制在控件最左侧条带（带宽自左边起算），下轴标题水平绘制在控件
	//最下侧条带（带高自底边起算），带=边距+标题文本高度+间距+2px 容差，
	//刻度数字在带外天然区分；空标题不占条带，不命中
	bool hitYAxisTitle(const QPoint& pos) const;
	bool hitXAxisTitle(const QPoint& pos) const;
	//轴标题激活态应用与恢复（X/Y 两轴）：选中=主题蓝+加粗（视图状态不进模型/
	//命令栈，同曲线高亮策略）；恢复时标题颜色/字体从模型重读（kTitleColor
	//无效兜底黑；引擎缓存可能残留高亮加粗）。全量 rebuild 重置外观后由
	//rebuild 尾部重放
	void applyAxisTitleHighlight();
	//--- 多坐标轴（顶部 X2..Xn / 右侧 Y2..Yn，对齐 HyperGraph） ---
	//本图表同侧副轴节点列表（树序=创建序，即渲染堆叠由内到外）
	QList<int> secondaryAxisNodes(bool vertical) const;
	//同侧首枚副轴节点id——映射引擎原生轴（xTop/yRight），无副轴返回 -1
	int nativeSecondaryAxisId(bool vertical) const;
	//副轴统一应用（rebuild 尾部调用，曲线与范围适配之后）：引擎上/右轴复位->
	//首枚副轴走引擎原生轴（启用刻度/数字组件）->第2枚起自绘条带->条带排布
	//副轴应用到引擎（fitChartAxes 之后）：首枚上/右副轴走引擎原生轴（applyAxis），
	//第2枚起自绘条带；复数图形态改道——下面板轴节点经 applyComplexLowerAxis
	//落引擎下面板 Y，上/右原生轴与条带不适用
	void applySecondaryAxes();
	//复数图上/下面板轴节点 id（HyperGraph 2024 排布：相位/虚部侧（Y2）默认在
	//上，幅值/实部侧（Y1）在下；kComplexSwapped 互换态对调）。任一出参可为空
	void complexPanelAxisIds(int* topAxisId, int* bottomAxisId) const;
	//复数图下面板轴节点（排布映射见 complexPanelAxisIds）属性下发引擎下面板
	//Y 轴：标题=面板分量名（随显示格式派生，见 complexPanelComponentTitle）/
	//可见/刻度引擎/数字格式/范围（手动关引擎 AutoFit + setAxisScale）
	void applyComplexLowerAxis(int axisNodeId);
	//画布曲线命中（上/下面板通用）：复数模式下面板 miss 后试下面板
	//（hitTestLowerCurve，画布局部坐标经 mapFromGlobal 换算）；命中的 name=
	//引擎键（R23，须经 engineNodeByKey 反查节点）。
	//maxDistPx=拾取半径（px，缺省 10 与引擎一致）；右键菜单路径传更大容差，
	//陡峭峰线附近"视觉在线上"的点击也能命中，避免落成图表空白菜单
	bool hitCurveAnyPanel(const QPoint& pos, QString& name, double maxDistPx = 10.0) const;
	//引擎上/右轴复位为隐藏（无主干线/无刻度/无数字）：rebuild 起点/无副轴时不残留
	void resetSecondaryEngineAxes();
	//自绘副轴条带重建（读模型配置刻度与外观；清旧建新）
	void rebuildAxisStrips();
	//条带排布：视图布局边距外扩让位 + 条带几何对齐画布（rebuild/画布尺寸变化时）
	void updateAxisStrips();
	//条带排布的延迟合并入口：Resize 可能在布局激活中被同步派发（applyAxis 的
	//enableAxis），事件回调里同步改布局会重入崩溃，统一推迟到事件循环
	void scheduleAxisStripLayout();
	//柱状图柱几何 resize 重算（D4 柱宽按画布宽/类目数现算——applySeries 落在
	//布局前画布仅 ~80px 宽，针状柱；画布定型后无重算入口。挂画布 Resize 延迟
	//合并入口，与条带排布同一去重通道）
	void scheduleBarGeometryRefresh();
	void refreshBarGeometry();
	void clearAxisStrips();
	//轴区（左/下/上/右轴控件与自绘条带）右键菜单：复制/粘贴置灰占位、
	//添加坐标轴（同侧再叠加一枚）、编辑（该轴属性编辑器）
	void onAxisZoneContextMenu(int axisNodeId, bool vertical, const QPoint& globalPos);
	//轴区 hover 高亮覆盖层（浅蓝，鼠标穿透；随所在轴区显示/隐藏）
	void showZoneHighlight(const QRect& rectInView);
	void hideZoneHighlight();
	//上/右（引擎原生副轴）标题带命中：近似同左/下轴（最外侧条带），双击弹轴编辑器
	bool hitTopAxisTitle(const QPoint& pos) const;
	bool hitRightAxisTitle(const QPoint& pos) const;
	//复数图下面板 Y/X 轴标题带命中：布局近似同上面板左/下轴，几何取下面板控件
	//（标题=面板分量名，双击弹对应轴节点编辑器）
	bool hitLowerYAxisTitle(const QPoint& pos) const;
	bool hitLowerXAxisTitle(const QPoint& pos) const;

	//画布右键菜单：图例项/曲线命中时弹出（剪切/复制/粘贴置灰/编辑/删除）
	void onCanvasContextMenu(const QPoint& pos, const QPoint& globalPos);
	//把事件过滤器挂到当前画布（canvas 可能因 setUseOpenGL 重建，需重挂）
	void ensureCanvasFilter();

	int _chartId;                       //绑定的图表节点id
	PlotView::mPlotView* _plot = nullptr;   //渲染引擎（页眉/页脚条带由引擎内部提供）

	//--- 引擎键 <-> 节点双向映射（R23 曲线允许重名） ---
	//引擎键视口内唯一（键=名 或 名#2/名#3…），模型名可重复；进引擎
	//（setCurveData/setCurveStyle/getCurve/hitTest 回查等）一律按键。
	//rebuild 全量重建，增量路径经 scheduleRebuild 收口（见 ensureEngineCurveKeys）
	QHash<int, QString> _engineKeyByNode;    //曲线节点id -> 引擎键
	QHash<QString, int> _nodeByEngineKey;    //引擎键 -> 曲线节点id（图例点击/hitTest 回查）

	bool _rebuildPending = false;      //防抖标志：待刷新（配合 scheduleRebuild）
	bool _rebuildPreserveView = false; //本次重建是否恢复重建前的用户缩放（同批以最后一次入队为准）
	bool _hasPreRebuildDiv = false;    //重建前主轴显示范围有效（preserveView 重建时记录）
	double _preRebuildXDiv[2] = { 0.0, 10.0 };  //重建前底部轴显示范围（新建副轴初值对齐用）
	double _preRebuildYDiv[2] = { 0.0, 10.0 };  //重建前左侧轴显示范围（新建副轴初值对齐用）
	int _selectedCurveId = -1;         //选中的曲线节点id（<=0 无选中）
	int _hoveredCurveId = -1;          //hover 中的曲线节点id
	int _hoveredBaselineId = -1;       //hover 中的基准线节点id（主题蓝+加粗1px，挪开恢复）
	int _selectedAnnotationId = -1;    //选中的注释节点id（<=0 无选中；主题蓝选中框，设计图15）
	int _dragAnnotationId = -1;        //拖动中的注释节点id（-1无；选中态左键拖框，松手提交）
	QPointF _dragGrabPx;               //按下点->框中心的画布像素偏移（拖动中保持，框不跳变）
	double _dragPosX = 0.0;            //拖动中最后一次实时框位（数据坐标，松手比对提交）
	double _dragPosY = 0.0;
	int _selectedAxisId = -1;          //选中的坐标轴节点id（X/Y轴，<=0 无选中；标题主题蓝+加粗，设计图05/06）
	int _selectedLegendId = -1;        //选中的图例节点id（<=0 无选中；主题蓝外框，设计图08）
	int _selectedBandId = -1;          //选中的页眉/页脚节点id（<=0 无选中；条带文字主题蓝+加粗。
	                                   //Header/Footer 两类型共用一个状态：选择中枢单选，
	                                   //同一时刻至多一个被选中；视图状态不进模型/命令栈）
	QWidget* _filteredCanvas = nullptr; //已挂事件过滤器的画布（重建后需重挂）
	QPointer<QWidget> _filteredLowerCanvas; //复数下面板画布（同挂过滤器；面板销毁自动置空）
	//复数下面板轴控件（左轴/下轴，右键轴区菜单与 hover 高亮同上面板；
	//面板销毁自动置空，下一轮重装）
	QPointer<QWidget> _filteredLowerYAxis;
	QPointer<QWidget> _filteredLowerXAxis;
	//引擎页眉/页脚条带控件（引擎构造期一次性创建、setUseOpenGL 只重建画布不动
	//条带，正常仅装一次过滤器，重复安装幂等）。QPointer 防悬挂：视图析构随
	//引擎销毁自动置空；条带隐藏（不可见/无文本）时 Qt 不派发鼠标事件，
	//双击天然无响应
	QPointer<QWidget> _headerBandWgt;
	QPointer<QWidget> _footerBandWgt;

	//画布边框覆盖层（1px 闭合矩形，鼠标穿透）。QPointer：画布可能因
	//setUseOpenGL 被引擎重建，旧画布销毁时其子控件（覆盖层）一并销毁，
	//ensureCanvasFilter 检测到画布更换后在新区重建（防悬垂指针）
	QPointer<QWidget> _borderOverlay;

	//--- 多坐标轴状态（副轴条带为视图自管，全量重建时重建） ---
	QList<mCurveAxisStripWidget*> _topStrips;    //X3..Xn 自绘条带（X2 占引擎 xTop）；
	                                             //挂引擎内部布局（页眉与 QwtPlot 之间），几何自动排布
	QList<mCurveAxisStripWidget*> _rightStrips;  //Y3..Yn 自绘条带（Y2 占引擎 yRight）；
	                                             //挂本视图外扩右边距，手动几何与画布纵向对齐
	QWidget* _zoneHighlight = nullptr;           //轴区 hover 高亮覆盖层（浅蓝，鼠标穿透）
	QRect _zoneHighlightRect;                    //当前高亮区（去重，避免重复 show/raise）
	bool _axisStripLayoutPending = false;        //条带排布已排队（0ms 去重合并）
	bool _barGeomPending = false;                //柱几何重算已排队（0ms 去重合并）
	//--- 注释交互状态 ---
	bool _annotationMode = false;                  //注释模式（工厂广播驱动）
	CurvePointCtx _lastSnapCtx;                    //引擎 snapper 最近一次吸附结果
	                                               //（valid=false 未吸附；左键创建取用）
	PlotView::mPlotAnnotation* _hoverItem = nullptr; //悬停实时预览项（模式内复用，本视图持有，
	                                               //蓝色样式+锚点符号）
	QMap<int, QwtPlotItem*> _annotationMarkers;    //注释节点id -> 渲染项（增量维护）
	QSet<int> _pendingAnnotationIds;               //待增量刷新的注释节点id（0ms 合并去重）
	bool _annotationRefreshPending = false;        //增量刷新是否已在事件循环排队
	bool _snappingAnnotation = false;              //吸附提交中（屏蔽 kAttachMode/kAttachCurve 再入）
	int _hoveredAnnotationId = -1;                 //悬停中的最小化注释（临时还原完整样式）

	//--- 基准线交互状态 ---
	bool _baselineMode = false;                    //基准线模式（工厂广播驱动）
	int _baselineOrientation = 1;                  //方向：0 横向 1 纵向（同 CurveBaselineData）
	mCurveReadoutItem* _baselineReadout = nullptr; //悬停读数项（模式内复用，本视图持有）
	QSet<int> _pendingBaselineIds;                 //待增量刷新的基准线节点id（0ms 合并去重）
	bool _baselineRefreshPending = false;          //增量刷新是否已在事件循环排队
	QSet<int> _pendingStyleIds;                   //待增量刷新的曲线节点id（曲线外观键，0ms 合并去重）
	bool _styleRefreshPending = false;            //外观增量刷新是否已在事件循环排队
	QMap<int, mCurveBaselineLabel*> _baselineLabels; //基准线节点id -> 线上标签图元
	mCurveBarLabelItem* _barLabelItem = nullptr; //柱状图柱顶数值标注图元（chartType=1 时持有）
	mCurveFrequencyCursor* _freqCursorTop = nullptr;    //频率红线游标（上面板）
	mCurvePolarFrameItem* _polarFrameItem = nullptr;    //极坐标底图（chartType=3 时持有）
	mCurvePolarSeriesItem* _polarSeriesItem = nullptr;  //极坐标向量层（chartType=3 时持有）
	bool _polarChainForm = false;                       //极坐标首尾相接形态（视图态；默认=面板 displayPhase 默认不显示相位/非首尾相接——2026-11-13 大图01"默认选项如下"权威对齐，旧 true 口径作废；产出链仍显式下发，11-12 补发时序修复后首渲染即正确）
	//极坐标滚轮缩放（视图态，2026-11-12 新增交互；2026-11-16 D35 商软对齐重裁定）：
	//半径有效域乘子，1=适配域。每档 ×0.8（放大，域收缩）/×1.2（缩小，域外扩）
	//连乘无极态（商软五样张实证：域可越过数据上界外扩至向量不可见、可收缩至
	//向量穿出画布缘）；不动模型（Y1 min/max/autoScale 保持），全量重建不保视图
	//（rebuild !preserveView 分支）时复位 1
	double _polarRadiusZoom = 1.0;
	//极坐标径向域存值（D25 回显链）：applyPolarDecor 每次解析域后写入，
	//displayedAxisRange(Y1) 极坐标分支回读供面板"所见即所得"回显/自动档退出
	//回写（载体 [-1.05,1.05] 只作引擎映射不进回显）；decor 尾部统一广播
	bool _polarDomainValid = false;
	double _polarDomainLo = 0.0;
	double _polarDomainHi = 1.0;
	//滚轮缩放期刻度步长（D28 捕获机制保留；2026-11-16 D35 商软对齐语义升级）：
	//0=未冻结（面板口径照常重推）；≠0 时 applyPolarDecor 以该步长（变换空间）
	//横跨缩放域产主环。缩放档位口径（D35 五样张实证）：格数=域/step 钳 [4,6]——
	//4~6 格内 step 不动（原型图13/14：域 1.2E-04/8.0E-05 恒 2.0E-05 步）；越界
	//step=域/6 或 域/4 重算并置 _polarZoomStepDirty（脏值继承，图15 0.17118=
	//域/6、图16 5.55030E-07=域/4 逐字实证）；域恒=格数×step 齐网格。首次离开
	//1.0 捕获、回到 1.0 或非保视图重建复位清零
	double _polarZoomTickStep = 0.0;
	//缩放衍生脏步长标记（D35）：step 经格数钳越界重算后置位——径向自动档标签
	//切换 6 位有效格式（量级 [0.01,1e4) 定点 5 位小数、之外科学大写 E，图15
	//"0.17118"/图16 "5.55030E-07" 实证）；浅档整洁值仍走 'E',1（D31 口径不动）。
	//回 1.0 或非保视图重建复位
	bool _polarZoomStepDirty = false;
	//最近一次 applyPolarDecor 渲染的主刻度步长（变换空间，majors[1]-majors[0]；
	//不足 2 条主刻度不更新）——_polarZoomTickStep 的捕获源
	double _polarLastMajorStep = 0.0;
	mCurveFrequencyCursor* _freqCursorBottom = nullptr; //频率红线游标（复数下面板）
	double _freqCursorX = -1.0;                         //游标 X（数据坐标；<0 关闭）
	                                               //（增量维护，随线显隐/删除同步摘除）

	//--- 轴刻度细项增量刷新状态 ---
	QSet<int> _pendingAxisIds;                     //待轻量刷新的轴节点id（0ms 合并去重）
	bool _axisRefreshPending = false;              //轻量刷新是否已在事件循环排队

	//--- 编辑曲线交互状态（会话数据的单一数据源在视口工厂，此处只存覆盖层） ---
	bool _editMode = false;                        //本视图为编辑目标视窗（会话进行中）
	QList<int> _editCurveIds;                      //目标曲线节点id集（首条为主目标）
	QHash<int, QwtPlotCurve*> _editSegments;       //各目标截取段不透明重绘（旁路自管，Z=12，按曲线id）
	mCurveTrimMarker* _editTrimStart = nullptr;    //起始裁剪线（旁路自管，Z=60）
	mCurveTrimMarker* _editTrimEnd = nullptr;      //终止裁剪线
	int _editDragLine = -1;                        //拖拽中的裁剪线（-1无 0起始 1终止）
	};
}
