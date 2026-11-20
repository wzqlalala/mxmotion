#include "ChartTypeProfile.h"
#include "CurveDocument.h"   //displayOverride lambda 访问 CurveNode 成员

namespace PostCurve
{
	namespace
	{
		//--- 共享骨架：现状 createChart 的 5 个固定子节点 ---
		//顺序=树显示顺序（设计图02：曲线区之后是图例，轴/页眉/页脚排尾部）；
		//一期全部 removable=false / visible=true / 无显示名覆写，与现状逐一对照
		QVector<ChartFixedChild> defaultFixedChildren()
		{
			return {
				{ CurveNodeType::Legend, QStringLiteral("Legend"), QString(), false, true },
				{ CurveNodeType::XAxis,  QStringLiteral("X1"),     QString(), false, true },
				{ CurveNodeType::YAxis,  QStringLiteral("Y1"),     QString(), false, true },
				{ CurveNodeType::Header, QStringLiteral("Header"), QString(), false, true },
				{ CurveNodeType::Footer, QStringLiteral("Footer"), QString(), false, true },
			};
		}

		//--- 共享分区序：原 nodeZoneRank 的搬运 ---
		//曲线->图例->基准->注释 四分区，尾部区（轴/页眉页脚）= 不在表内
		QVector<CurveNodeType> defaultZoneOrder()
		{
			return { CurveNodeType::Curve, CurveNodeType::Legend,
				CurveNodeType::Baseline, CurveNodeType::Annotation };
		}

		//--- 共享菜单表：原 mCurveTreeView::contextMenuEvent 各 case 的逐项搬运 ---
		//（文案与顺序与现状一致；执行 lambda 在 mCurveTreeView::appendMenuAction）
		QHash<int, QVector<TreeMenuAction>> defaultMenus()
		{
			QHash<int, QVector<TreeMenuAction>> m;
			const auto key = [](CurveNodeType t) { return static_cast<int>(t); };

			//图表：显隐 + 新建三件套 + 多坐标轴入口 + 删除
			m.insert(key(CurveNodeType::Chart), {
				TreeMenuAction::ToggleVisible,
				TreeMenuAction::AddCurve,
				TreeMenuAction::AddBaseline,
				TreeMenuAction::AddAnnotation,
				TreeMenuAction::Separator,
				TreeMenuAction::AddXAxis,
				TreeMenuAction::AddYAxis,
				TreeMenuAction::Separator,
				TreeMenuAction::RemoveNode });

			//曲线：编辑数据属性 + 删除
			m.insert(key(CurveNodeType::Curve), {
				TreeMenuAction::EditCurveData,
				TreeMenuAction::Separator,
				TreeMenuAction::RemoveNode });

			//轴：显隐 + 多坐标轴入口（主轴/副轴同一份，副轴另追加"删除坐标轴"）
			const QVector<TreeMenuAction> axisMenu = {
				TreeMenuAction::ToggleVisible,
				TreeMenuAction::Separator,
				TreeMenuAction::AddXAxis,
				TreeMenuAction::AddYAxis };
			m.insert(key(CurveNodeType::XAxis), axisMenu);
			m.insert(key(CurveNodeType::YAxis), axisMenu);
			QVector<TreeMenuAction> secondaryMenu = axisMenu;
			secondaryMenu << TreeMenuAction::Separator << TreeMenuAction::RemoveSecondaryAxis;
			m.insert(key(CurveNodeType::XAxis2), secondaryMenu);
			m.insert(key(CurveNodeType::YAxis2), secondaryMenu);

			//页眉/页脚/图例（固定子）：仅显隐
			const QVector<TreeMenuAction> toggleOnly = { TreeMenuAction::ToggleVisible };
			m.insert(key(CurveNodeType::Legend), toggleOnly);
			m.insert(key(CurveNodeType::Header), toggleOnly);
			m.insert(key(CurveNodeType::Footer), toggleOnly);

			//基准线/注释（用户节点）：仅删除
			const QVector<TreeMenuAction> removeOnly = { TreeMenuAction::RemoveNode };
			m.insert(key(CurveNodeType::Baseline), removeOnly);
			m.insert(key(CurveNodeType::Annotation), removeOnly);
			return m;
		}
	}

	const ChartTypeProfile& chartTypeProfile(int chartType)
	{
		//0=XY 1=柱状 2=复数 3=极坐标 4=蛛网 5=三维图；未注册/越界回退 0（XY）
		static const QVector<ChartTypeProfile> registry = [] {
			QVector<ChartTypeProfile> v(6);
			for (int t = 0; t < v.size(); ++t)
			{
				v[t].chartType = t;
				v[t].typeName = chartTypeDisplayNames().value(t);
				v[t].fixedChildren = defaultFixedChildren();
				v[t].zoneOrder = defaultZoneOrder();
				v[t].menus = defaultMenus();
				v[t].defaultChartName = QStringLiteral("XY Plot");
			//渲染行为节：renderKind 收编既有画法分支；zeroLineStyle 已接线
			//（消费点 mCurveChartView::addZeroCrossLines）——柱状份翻转注册值
			//BaselineOnly（仅水平基线"一"，竖线 x=0 不画），其余保持 Cross=现状；
			//xTickSemantic 仍保持默认（NumericAuto）=现状，Category 待接线后翻转
			v[t].renderKind = (t == 1) ? ChartTypeProfile::RenderKind::Bar
				: (t == 2) ? ChartTypeProfile::RenderKind::Complex
				: (t == 5) ? ChartTypeProfile::RenderKind::Plot3D
				: (t == 3) ? ChartTypeProfile::RenderKind::Polar
				: ChartTypeProfile::RenderKind::XYLike;
			v[t].zeroLineStyle = (t == 1)
				? ChartTypeProfile::ZeroLineStyle::BaselineOnly
				: (t == 3)
				? ChartTypeProfile::ZeroLineStyle::None
				: ChartTypeProfile::ZeroLineStyle::Cross;
			}
			//柱状图（chartType=1）增量：X 坐标轴为类目轴（类别刻度），树显示名
			//"Category"（对齐参考软件柱状图树：Legend、Category、Y1）
			//默认名 "Bar Plot"（提取数值转换后树形态同参考软件；reconcile 第 4 项
			//"仍为旧默认名时随类型改名"机制消费本值）
			v[1].defaultChartName = QStringLiteral("Bar Plot");
			for (ChartFixedChild& fc : v[1].fixedChildren)
			{
				if (fc.type == CurveNodeType::XAxis)
					fc.displayName = QStringLiteral("Category");
			}
			//Ribbon 顶部入口增量（不适用即隐藏，见 ChartTypeProfile.h (h)）：
			//柱状图数据为离散类目（定义曲线数值表），"新建"（导入 csv/pch 生成
			//曲线）与"编辑曲线"（连续数据裁剪截取）均不适用；复数图仅"编辑曲线"
			//不适用（上下面板成对曲线无单线裁剪语义）；其余类型全可见
			v[1].hiddenRibbonActions = { ChartTypeProfile::RibbonAction::NewCurve,
				ChartTypeProfile::RibbonAction::EditCurve };
			v[2].hiddenRibbonActions = { ChartTypeProfile::RibbonAction::EditCurve };
			//复数图（chartType=2）增量：
			//  固定骨架按设计图 Complex Plot 顺序（Legend、Magnitude、Phase、X1、
			//  页眉页脚）——Y 轴显示名 Magnitude，新增 Y2 显示名 Phase（下面板轴，
			//  kChartType 切换时由骨架协调补建/删除）；
			//  Chart 菜单追加复数专属两项（设计图01：切换至实部/虚部、互换复数
			//  坐标轴，文案随当前格式动态）；
			//  复数曲线为单节点（x/ym/yp 三壳创建时直接命名），动态显示名规则退场；
			//  默认名 Complex Plot
			{
				ChartTypeProfile& cx = v[2];
				cx.defaultChartName = QStringLiteral("Complex Plot");
				cx.fixedChildren.clear();
				cx.fixedChildren.append({ CurveNodeType::Legend, QStringLiteral("Legend"), QString(), false, true });
				cx.fixedChildren.append({ CurveNodeType::YAxis,  QStringLiteral("Y1"), QStringLiteral("Magnitude"), false, true });
				cx.fixedChildren.append({ CurveNodeType::YAxis2, QStringLiteral("Y2"), QStringLiteral("Phase"), false, true });
				cx.fixedChildren.append({ CurveNodeType::XAxis,  QStringLiteral("X1"), QString(), false, true });
				cx.fixedChildren.append({ CurveNodeType::Header, QStringLiteral("Header"), QString(), false, true });
				cx.fixedChildren.append({ CurveNodeType::Footer, QStringLiteral("Footer"), QString(), false, true });
				cx.menus.insert(static_cast<int>(CurveNodeType::Chart), {
					TreeMenuAction::ToggleVisible,
					TreeMenuAction::AddCurve,
					TreeMenuAction::AddBaseline,
					TreeMenuAction::AddAnnotation,
					TreeMenuAction::Separator,
					TreeMenuAction::AddXAxis,
					TreeMenuAction::AddYAxis,
					TreeMenuAction::Separator,
					TreeMenuAction::ToggleComplexFormat,
					TreeMenuAction::SwapComplexAxes,
					TreeMenuAction::Separator,
					TreeMenuAction::RemoveNode });
			}
			//三维图（chartType=5，R28 入树）增量：
			//  固定骨架仅 Legend（三维图特殊性——无 X1/Y1 轴区、无页眉页脚；
			//  轴属性挂在曲线下 x/y/z 壳节点，轴状态存图表 axis3d）；
			//  默认名 "3D Plot"（kChartType 不支持切入 3D，本值仅树创建默认名用）；
			//  菜单：图表=显隐/删除、曲线/注释=删除（追加曲线仅经"从文件新建三维图"
			//  弹窗目标选择，树不提供"新建曲线"入口——数据来源为解析文件；
			//  R34 起注释=同 2D 注释菜单仅删除口径，新建入口在 Ribbon"3D曲线"
			//  页签"标注"组"注释"按钮，不加 AddAnnotation——对齐"追加曲线仅经
			//  弹窗"先例）；
			//  renderKind=Plot3D（消费点=视口工厂 materializeChart 分流 m3DChartView）；
			//  Ribbon"新建/编辑曲线"不适用（同柱状图口径，隐藏整项）
			{
				ChartTypeProfile& d3 = v[5];
				d3.defaultChartName = QStringLiteral("3D Plot");
				d3.fixedChildren.clear();
				d3.fixedChildren.append({ CurveNodeType::Legend, QStringLiteral("Legend"), QString(), false, true });
				d3.menus.insert(static_cast<int>(CurveNodeType::Chart), {
					TreeMenuAction::ToggleVisible,
					TreeMenuAction::Separator,
					TreeMenuAction::RemoveNode });
				d3.menus.insert(static_cast<int>(CurveNodeType::Curve), {
					TreeMenuAction::RemoveNode });
				//注释（R34）：同 2D 注释菜单仅删除口径；新建入口在 Ribbon"3D曲线"
				//页签"标注"组，不加 AddAnnotation
				d3.menus.insert(static_cast<int>(CurveNodeType::Annotation), {
					TreeMenuAction::RemoveNode });
				//基准面（R41）：菜单同注释仅删除口径（新建入口在参数栏基准工具页，
				//对齐注释"入口不在树"先例，不加 AddDatumPlane3D）；用户分区在注释
				//分区之后尾部追加基准面分区——树插入位置由 childInsertionRow 按
				//分区序查表决定（createNode/insertNodeTree 共用），基准节点落在
				//本图表注释区之后；固定子守卫（isFixedChildNode）不含本类型，
				//RemoveNode 可删、显隐切换走 nodeVisibleToggleProp 默认 kVisible
				d3.zoneOrder.append(CurveNodeType::DatumPlane3D);
				d3.menus.insert(static_cast<int>(CurveNodeType::DatumPlane3D), {
					TreeMenuAction::RemoveNode });
				d3.hiddenRibbonActions = { ChartTypeProfile::RibbonAction::NewCurve,
					ChartTypeProfile::RibbonAction::EditCurve };
			}
			//极坐标图（chartType=3）增量（design D5）：固定骨架沿用默认五件套
			//（X1/Y1 在极坐标下语义覆写为相位/恒值，属性页只读回显随 §5.5）；
			//默认名 Polar Plot；自绘画布接管后"新建曲线"（直角连续数据导入）与
			//"编辑曲线"（裁剪截取）均不适用，Ribbon 入口隐藏同柱状口径
			{
				ChartTypeProfile& px = v[3];
				px.defaultChartName = QStringLiteral("Polar Plot");
				px.hiddenRibbonActions = { ChartTypeProfile::RibbonAction::NewCurve,
					ChartTypeProfile::RibbonAction::EditCurve };
			}
			return v;
		}();
		if (chartType < 0 || chartType >= registry.size())
			return registry.first();
		return registry.at(chartType);
	}
}
