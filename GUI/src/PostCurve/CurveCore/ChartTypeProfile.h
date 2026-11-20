#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 图表类型声明（数据契约）
//
// 一种图表类型（CurveChartData.chartType）一份完整声明：树内容节 + 渲染行为节。
// "每种图表类型长什么样"全部在表里——新增/调整图表类型 = 注册/修改一份 profile，
// 不改散落的 switch 分支（树骨架/分区/菜单/显示名的既有硬编码已收口于此）。
//
// 查表入口 chartTypeProfile(chartType)：未注册的类型回退 chartType 0（XY），
// 与属性页"未注册弹窗页回退参数栏页"同一款回退策略。
//
// 消费点：
//   树内容节   - CurveDocument（createChart 骨架创建 / childInsertionRow 分区序 /
//               nodeDisplayText 显示名覆写）、mCurveTreeView（右键菜单组装）
//   渲染行为节 - mCurveChartView（按字段分期消费；当前仅注册"现状值"，未消费零影响）。
//               注册纪律：任何字段翻转注册值（改行为）之前，必须先有消费点且注册值=现状
//
// 两层词表的对号入座（新增东西时）：
//   新增/调整图表类型 -> 本文件注册一份 profile
//   新增节点类型     -> CurveDefine.h 节点类型词表（显示名/默认名/图标/
//                       nodeTypeRenameable/nodeVisibleToggleProp）
// =============================================================================

#include "CurveDefine.h"
#include "../PostCurveGlobal.h"
#include <QVector>
#include <QHash>
#include <QString>
#include <functional>

namespace PostCurve
{
	class CurveDocument;
	struct CurveNode;

	//右键菜单动作（纯枚举，与控制器命令式 API 一一对应；
	//菜单的组装与执行 lambda 仍在 mCurveTreeView，本枚举只描述"有什么菜单项"）
	enum class TreeMenuAction
	{
		ToggleVisible,        //显示/隐藏切换（属性键按类型映射，见 CurveDefine.h nodeVisibleToggleProp）
		AddPage,              //新建页
		AddChart,             //新建视口
		AddCurve,             //新建曲线
		AddBaseline,          //新建基准线
		AddAnnotation,        //新建注释
		AddXAxis,             //添加横坐标轴（副轴，X2/X3...）
		AddYAxis,             //添加纵坐标轴（副轴，Y2/Y3...）
		EditCurveData,        //编辑数据属性（定位并展开 x 数据子节点）
		RemoveNode,           //删除本节点（removeNode）
		RemoveSecondaryAxis,  //删除坐标轴（副轴专用入口 removeSecondaryAxis）
		ToggleComplexFormat,  //切换复数显示格式（幅值-相位 ↔ 实部-虚部；文案随当前格式）
		SwapComplexAxes,      //互换复数坐标轴（上下面板交换）
		Separator,            //分隔线
	};

	//图表固定骨架子节点声明（顺序即树显示顺序）
	struct ChartFixedChild
	{
		CurveNodeType type;               //节点类型
		QString       name;               //模型默认名（创建时写入 node->name，如 "X1"）
		QString       displayName;        //显示名覆写（空=显示本名；如复数图 Y 轴 "Magnitude"）
		bool          removable = false;  //可否删除（false 即 isFixedChildNode 语义。
		                                  //一致性约束：本表 removable=false 的类型不得
		                                  //出现在 CurveDefine.h::isFixedChildNode 之外，反之亦然）
		bool          visible = true;     //创建时默认可见（模型默认值，createChart 写入
		                                  //axis.visible；当前全部注册 true=现状，零影响）
	};

	//一种图表类型的完整声明（树内容节 + 渲染行为节）
	struct ChartTypeProfile
	{
		int     chartType = 0;         //对应 CurveChartData.chartType（0..4）
		QString typeName;              //对齐 chartTypeDisplayNames 文案（调试/日志用）

		//=============== 树内容节 ===============

		//(a) 图表固定骨架：数组顺序 = 树显示顺序，createChart 按此循环创建
		QVector<ChartFixedChild> fixedChildren;

		//(b) 用户节点分区顺序：类型在表中的下标即分区序（见 chartZoneRank）；
		//    不在表中的类型一律归"尾部区"（轴/页眉页脚），与原 nodeZoneRank default 等价
		QVector<CurveNodeType> zoneOrder;   //现状 = { Curve, Legend, Baseline, Annotation }

		//(c) 右键菜单：图表管辖范围内节点类型 -> 动作序列（键 = CurveNodeType 的 int；
		//    未列出的类型无右键菜单，与原 contextMenuEvent default:return 一致）
		QHash<int, QVector<TreeMenuAction>> menus;

		//(d) 动态显示名规则（静态覆写用 ChartFixedChild::displayName）：
		//    返回覆写显示名，空串=不覆写（复数曲线壳节点已改为创建时直接命名
		//    x/ym/yp，本钩子当前无现役实现，保留作扩展点）
		std::function<QString(const CurveDocument*, const CurveNode*)> displayOverride;

		//(d2) 该类型图表的默认名（kChartType 切换骨架协调时，图表名仍为旧类型
		//默认名则随类型改名：XY Plot -> Complex Plot；用户改过名不动）
		QString defaultChartName;

		//=============== 渲染行为节（语义标志，模型数据一律不动） ===============
		//来回切类型不丢数据；若差异需要进模型/属性页可见可改，归"模型默认值"
		//（fixedChildren 初值 + 未来 kChartType 切换时的骨架协调），不在此节

		//(e) 画法分派：收编 mCurveChartView 里 chartType==1/2/3/5 的画法分支
		//    （复数图是独立渲染管线；接线前注册值须=现状分支行为）。
		//    Polar=极坐标（chartType=3，design D5 自绘图元族接管画布，
		//    引擎曲线退化为图例/命中链载体）；
		//    Plot3D=三维图（chartType=5，R28 入树）：消费点=视口工厂
		//    materializeChart 分流创建 m3DChartView（Plot3D 渲染管线），
		//    mCurveChartView 不参与三维图渲染
		enum class RenderKind { XYLike, Bar, Complex, Polar, Plot3D };
		RenderKind renderKind = RenderKind::XYLike;

		//(f) 零线样式：Cross=十字(x=0/y=0，双Linear现状)；BaselineOnly=仅水平基线"一"
		//    （柱状目标形态，配合引擎 barBaseValue=0）；None=不画。
		//    消费点：mCurveChartView::addZeroCrossLines（已接线，柱状注册 BaselineOnly）
		enum class ZeroLineStyle { Cross, BaselineOnly, None };
		ZeroLineStyle zeroLineStyle = ZeroLineStyle::Cross;

		//(g) X 刻度语义：NumericAuto=数字自动刻度（现状）；
		//    Category=仅用户设置的类别刻度（series.xTickLabels），不再自动外推。
		//    消费点：mCurveChartView applyAxis X 轴刻度生成（未接线前不得翻转注册值）
		enum class XTickSemantic { NumericAuto, Category };
		XTickSemantic xTickSemantic = XTickSemantic::NumericAuto;

		//(h) Ribbon 顶部入口可见性：激活图表为本类型时应隐藏的功能入口
		//    （不适用即整项隐藏，非置灰）。消费点：mPostMenuContributor::
		//    buildNVHCurvePage——按钮连工厂 sig_currentChartTypeChanged 按本表
		//    显隐；未注册类型/无激活图表回退 XY=全可见（chartTypeProfile 回退策略）
		enum class RibbonAction { NewCurve, EditCurve };
		QVector<RibbonAction> hiddenRibbonActions;
	};

	//查表入口：按 chartType 取声明；未注册/越界的类型回退 chartType 0（XY）。
	//导出：消费方不止本模块（Ribbon 侧按类型显隐按钮也查表）
	POSTCURVE_EXPORT const ChartTypeProfile& chartTypeProfile(int chartType);

	//分区序查询：type 在 profile.zoneOrder 中的下标；不在表内=尾部区
	//（返回 zoneOrder.size()，与原 CurveDocument::nodeZoneRank 的 default=4 等价：
	// 4 个分区之后恰好是尾部）
	inline int chartZoneRank(const ChartTypeProfile& profile, CurveNodeType type)
	{
		const int idx = profile.zoneOrder.indexOf(type);
		return idx >= 0 ? idx : profile.zoneOrder.size();
	}

	//Ribbon 入口隐藏查询：action 在 profile.hiddenRibbonActions 中=该类型下隐藏
	inline bool ribbonActionHidden(const ChartTypeProfile& profile,
		ChartTypeProfile::RibbonAction action)
	{
		return profile.hiddenRibbonActions.contains(action);
	}
}
