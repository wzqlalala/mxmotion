#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 公共类型定义（数据契约）
//
// 树形结构：工程(Project根) -> 页(Page) -> 视口(Chart) -> 曲线/轴/图例/基准线...
// 本文件被模型/命令/树/视图/属性页所有层引用：
//   - CurveNodeType     : 曲线结构树的节点类型（对应UI设计图02）
//   - 各节点数据结构    : 每种节点类型的业务数据（挂在 CurveNode 上，按类型生效）
//   - CurveProp 属性名  : 属性修改走命令栈（撤销/重做）的统一属性键，
//                         CurveDocument::setNodeProperty 按 NodeType+属性名分发
//
// 新增节点类型的扩展步骤：
//   1. CurveNodeType 加枚举值 + curveNodeTypeDisplayName 加显示名
//   2. 定义数据结构并挂到 CurveNode（CurveDocument.h）
//   3. CurveDocument::nodeProperty/setNodeProperty 补属性分发
//   4. mCurvePropPageManager 注册对应属性页
//   5. mCurveChartView::rebuild() 补渲染分支
// =============================================================================

#include <QString>
#include <QStringList>
#include <QColor>
#include <QVector>
#include <QFont>

namespace PostCurve
{
	//树节点类型
	//层级：Project(工程/根) -> Page(页) -> Chart(视口) -> Curve/XAxis/YAxis/YAxis2/Legend/Header/Footer/Baseline/Annotation
	//一个工程可含多个页；一个页可含多个曲线视口（pch 导入一次创建=一页，
	//页内按布局模式放若干视口）；XAxis/YAxis/Legend/Header/Footer 为视口固定子节点，不可单独删除。
	//命名对齐 HyperGraph：页/视口树显示加位置前缀 "p1 : "/"w1 : "（显示层现算，
	//见 CurveDocument::nodeDisplayName）；主轴固定名 X1/Y1，副轴 X2/X3.../Y2/Y3...
	enum class CurveNodeType
	{
		Project = 0,     //根节点（工程，整树唯一）
		Page,            //页（工程的子节点，默认名 Page 1...，聚合若干曲线视口）
		Chart,           //曲线视口（页的子节点，默认名 XY Plot，对应一个 MDI 视口）
		Curve,           //曲线（默认名 Curve 1...，pch导入时用 feature 名命名）
	XAxis,           //横坐标轴（固定子节点，X1/底部）
	YAxis,           //纵坐标轴（固定子节点，Y1/左侧）
	YAxis2,          //第二纵坐标轴（右侧，按需添加可多枚：Y2/Y3...，首枚映射 PlotView Y_Right）
	XAxis2,          //第二横坐标轴（顶部，按需添加可多枚：X2/X3...，首枚映射 PlotView X_Top）
		Legend,          //图例（固定子节点）
		Header,          //页眉标题（固定子节点；页眉数据实际挂在图表节点上）
		Footer,          //页脚标题（固定子节点；页脚数据实际挂在图表节点上）
		Baseline,        //基准线：纵向(固定X值)/横向(固定Y值)
		Annotation,      //注释
		CurveXData,      //曲线X轴数据（曲线固定子节点"x"，设计图03.2；数据存父曲线，壳节点）
		CurveYData,      //曲线Y轴数据（曲线固定子节点"y"/复数曲线"ym"，设计图03.2；壳节点）
		CurveY2Data,     //复数曲线下面板数据（固定子节点"yp"：相位/虚部；壳节点，仅复数曲线创建）
		AxisX3DData,     //三维图X轴壳（三维曲线固定子节点"x"，R28 入树；轴状态存所属
		                 //图表节点 axis3d[0]，壳读写经文档重定向——多曲线共享同轴组）
		AxisY3DData,     //三维图Y轴壳（固定子节点"y"，重定向 axis3d[1]）
		AxisZ3DData,     //三维图Z轴壳（固定子节点"z"，重定向 axis3d[2]）
		DatumPlane3D,    //三维基准面（R41：三维图 chartType=5 的用户子节点，层级=Chart，
		                 //非固定子可删除；序列化按 int 存——尾部追加纪律，不得改动既有值）
	};

	//曲线类别（曲线统计弹窗的目标范围；普通曲线之外的类别由对应生成功能
	//打标——动刚度曲线/VTF曲线/NTF曲线按钮接入时写入，导入/定义曲线恒为普通）
	//序列化按 int 存（CurveDocSerialize V2/模板块）：新值只允许尾部追加，
	//不得改动既有枚举值，否则破坏已存工程/会话快照的 round-trip
	enum class CurveCategory
	{
		Normal = 0,        //普通曲线（pch导入/定义曲线，默认）
		DynamicStiffness,  //动刚度曲线（实测）
		VTF,               //振动传递率曲线（VTF，实测）
		NTF,               //噪声传递函数曲线（NTF，实测）
		TargetCurve,       //目标值曲线（生成功能创建目标线时写入，与同图表实测
		                   //类别 DynamicStiffness/VTF/NTF 配对供曲线统计配对）
	};

	//批量曲线数据落地契约（mCurveDocController::addCurveBatch 门面入参：模块外
	//数据源——后处理曲线提取等——一次成图用）。item=单条曲线（数据+分组键+
	//来源五元组）；layout 与曲线提取面板绘制选项下拉 0-4 一一对应
	struct CurveBatchItem
	{
		QString name;                //曲线名（入树/图例）
		QVector<double> xData;       //X 数据
		QVector<double> yData;       //Y 数据
		QString group;               //分组键（OnePerGroup 布局按此分图；空=该条独立一图）
		QString sourceFile;          //来源五元组（"更改曲线"溯源回显，同 CurveSeriesData）
		QString subcase;
		QString schema;
		QString object;
		QString component;
	};

	enum class CurveBatchLayout
	{
		AllInOne = 0,     //一张图收全部（落点页由落图 API 决定：addCurveBatch 落当前激活图表所在页；addCurveBatchToPage 落指定页）
		OnePerGroup,      //按 group 分组，每组一张图
		OnePerCurve,      //每条曲线一张图
		NewPage,          //强制新建页，一张图收全部
		ExistingChart,    //并入 existingChartId 指定的既有图表
	};

	//是否图表/曲线的固定子节点（不可由用户单独删除，removeNode 据此拦截）。
	//副轴（XAxis2/YAxis2）同为固定子节点：removeNode/剪切仍拦截，删除走轴区/
	//树右键"删除坐标轴"（removeSecondaryAxis 专用入口），Ctrl+Z 可撤销
	inline bool isFixedChildNode(CurveNodeType type)
	{
		return type == CurveNodeType::XAxis || type == CurveNodeType::YAxis
			|| type == CurveNodeType::YAxis2 || type == CurveNodeType::XAxis2
			|| type == CurveNodeType::Legend || type == CurveNodeType::Header || type == CurveNodeType::Footer
			|| type == CurveNodeType::CurveXData || type == CurveNodeType::CurveYData
			|| type == CurveNodeType::CurveY2Data
			|| type == CurveNodeType::AxisX3DData || type == CurveNodeType::AxisY3DData
			|| type == CurveNodeType::AxisZ3DData;
	}

	//节点类型显示名（用于右键菜单文案、浮动编辑器标题等）
	inline QString curveNodeTypeDisplayName(CurveNodeType type)
	{
		switch (type)
		{
		case CurveNodeType::Project:     return QStringLiteral("工程");
		case CurveNodeType::Page:        return QStringLiteral("页");
		case CurveNodeType::Chart:       return QStringLiteral("视口");
		case CurveNodeType::Curve:       return QStringLiteral("曲线");
		case CurveNodeType::XAxis:       return QStringLiteral("X1");
		case CurveNodeType::YAxis:       return QStringLiteral("Y1");
		case CurveNodeType::YAxis2:      return QStringLiteral("Y2");
		case CurveNodeType::XAxis2:      return QStringLiteral("X2");
		case CurveNodeType::Legend:      return QStringLiteral("Legend");
		case CurveNodeType::Header:      return QStringLiteral("Header");
		case CurveNodeType::Footer:      return QStringLiteral("Footer");
		case CurveNodeType::Baseline:    return QStringLiteral("基准线");
		case CurveNodeType::Annotation:  return QStringLiteral("注释");
		case CurveNodeType::CurveXData:  return QStringLiteral("X数据");
		case CurveNodeType::CurveYData:  return QStringLiteral("Y数据");
		case CurveNodeType::CurveY2Data: return QStringLiteral("Y数据");
		case CurveNodeType::AxisX3DData: return QStringLiteral("X轴");
		case CurveNodeType::AxisY3DData: return QStringLiteral("Y轴");
		case CurveNodeType::AxisZ3DData: return QStringLiteral("Z轴");
		case CurveNodeType::DatumPlane3D: return QStringLiteral("基准面");
		default:                         return QString();
	}
}

//节点默认名（对齐 HyperGraph：类型词+空格+数字，无前导零；编号为会话内全局
//递增计数，删除/撤销不回收——与树显示的位置前缀 p1/w1 是两套体系）。
//Chart 默认名固定 "XY Plot"（同名视口靠显示前缀 w1/w2 区分，num 忽略）；
//pch 导入的页传文件基名、曲线传 feature 名，不走本函数
inline QString curveNodeDefaultName(CurveNodeType type, int num)
{
	switch (type)
	{
	case CurveNodeType::Page:        return QStringLiteral("Page %1").arg(num);
	case CurveNodeType::Chart:       return QStringLiteral("XY Plot");
	case CurveNodeType::Curve:       return QStringLiteral("Curve %1").arg(num);
	case CurveNodeType::Baseline:    return QStringLiteral("Datum %1").arg(num);
	case CurveNodeType::Annotation:  return QStringLiteral("Note %1").arg(num);
	case CurveNodeType::DatumPlane3D: return QStringLiteral("基准 %1").arg(num);
	default:                         return curveNodeTypeDisplayName(type);
	}
}

//图表类型显示名（下拉框文案，图表属性编辑器与参数栏图表页共用；对应 chartType 0..5。
//三维图(5)仅经"从文件新建三维图"弹窗创建——图表属性页下拉中置灰占位，
//既有图表 kChartType 切入 3D 不支持（曲线无 zData 语义））
inline QStringList chartTypeDisplayNames()
{
	return {
		QStringLiteral("xy曲线图"),
		QStringLiteral("柱状图"),
		QStringLiteral("复数图"),
		QStringLiteral("极坐标图"),
		QStringLiteral("蛛网图"),
		QStringLiteral("三维图"),
	};
}

	//曲线单侧（X/Y）数据属性（x/y 壳子节点编辑，数据存父曲线节点）
	//变换链：缩放系数 -> 偏移量（applyTo，已接渲染管线）-> 相位角 -> 最大/最小值
	//裁剪（待接线），位于计权与轴刻度模式预变换之前，作用于原始数据副本
	//（模型始终存原始值）
	struct CurveDataRefData
	{
		double scale = 1.0;        //缩放系数
		double offset = 0.0;       //偏移量
		double phase = 0.0;        //相位角（度）
		bool clipEnabled = false;  //数据裁剪开关（按最大/最小值裁剪数据点）
		double clipMin = 0.0;      //裁剪下限（本侧数据坐标）
		double clipMax = 1.0;      //裁剪上限
		QString expression;        //用户定义式（设计图12 表达式数据源，占位暂不生效）
		int axisAssign = 0;        //坐标轴：0=主轴（y=Y1/x=底部），>=1=第 N 枚副轴
		                           //（Y2/Y3.../X2/X3...，下拉按图表实际副轴生成；引擎
		                           //只挂首枚副轴——y=右轴、x=上轴）
		QString unit;              //单位（显示用）

		//单侧数据显示值 = 原始值 * 有效缩放 + 偏移量（x/y 数据属性页"缩放系数/
		//偏移量"两输入框即此二参）。缩放系数语义对齐参考软件：正数 n = 放大 n 倍，
		//负数 -n = 缩小为 1/n（如 -2 即乘 1/2），不翻转曲线、不产生负值，避免
		//对数/dB 下整段 NaN 剔点；0 保持乘法原义（显示值恒为偏移量）。曲线绘制/
		//轴自动范围/编辑截取/工厂界值等逐点渲染换算统一在 transformPoint 之前
		//经此预变换
		double applyTo(double raw) const
		{
			const double effective = (scale < 0.0) ? 1.0 / (-scale) : scale;
			return raw * effective + offset;
		}
	};

	//曲线数据（Curve 节点）
	//数据源：目前为直接数据（pch导入）；后续扩展表达式计算/文件引用（设计图12）
	struct CurveSeriesData
	{
		QString name;                        //曲线名（同时是图表渲染与图例的键，树节点显示名一致）
		bool visible = true;                 //是否显示
		QColor color = QColor(0, 102, 204);  //线颜色
		int lineStyle = 0;                   //0实线/1虚线/2点线/3点划线，映射 PlotView::TecLinePattern
		double lineWidth = 2.0;              //线宽
		bool showSymbol = false;             //是否显示数据点符号
		int symbolSize = 6;                  //符号大小（遗留字段：尺寸改为"基准大小×缩放尺寸"推导，不再驱动渲染）
		QColor symbolColor = QColor(204, 0, 0); //符号颜色
		int symbolDensity = 1;               //符号密度（标记间隔：1=逐点，N=每N点画一个）
		double symbolScale = 1.0;            //缩放尺寸系数（实际大小=基准大小×系数：大圆点(1)基准12px，其余8px）
		QVector<double> xData;               //X 数据（数据体量大，不走命令栈，经 setSeriesData 直写；始终存原始值）
		QVector<double> yData;               //Y 数据（同上；刻度变换在渲染层现算，切换刻度模式即时生效）
		QStringList xTickLabels;             //X 类别刻度名（定义曲线"数值"来源写入；柱状图
		                                     //X 轴按此渲染类别刻度，空=数字刻度）
		int symbolStyle = 0;                 //符号形状：0无 1圆 2方 3菱 4三角 5十字 6倒三角 7星(五角星) 8交叉 9点（与绘图选项 symbolCombo 一致）

		//曲线标签（属性编辑器弹窗；渲染=图例文本，makeCurveSettings 解析后经
		//CurveSettings::legendText 下发，见 mCurveChartView.cpp）。
		//内容/前缀/后缀为模板文本（mCurveTemplateCombo 勾选变量拼接或手输），
		//出厂默认对齐参考软件——新建曲线即带默认模板；非空初值同时避免
		//控制器"新旧值相等即跳过"的短路吞掉清空操作（null 与空串相等）
		bool labelVisible = true;           //显示标签
		int labelContent = 0;                //标签内容：0 曲线名称 1 自定义文本（键停用保留）
		QString labelText = QStringLiteral("{y.MXRequest}-{y.MXComponent}");   //标签内容模板（默认=分量名）
		QString labelPrefix = QStringLiteral("{y.MXRequest}");   //标签前缀模板（默认=请求名）
		QString labelSuffix = QStringLiteral("Max={max(y)}");    //标签后缀模板（默认=Y最大值）
		bool labelPrefixVisible = false;      //显示标签前缀（前缀编辑行的显隐开关）
		bool labelSuffixVisible = false;      //显示标签后缀（同上）
		bool areaFill = false;               //区域填充（渲染层映射 PlotView CurveSettings::showFill）

	    //来源信息（创建时一次性写入，供"更改曲线"（设计图13）等功能溯源回显；
	    //同时是 X/Y 侧横坐标形态的锚通道——侧独立来源取横坐标即取本通道数据块）
	    QString sourceFile;                //pch 等数据文件路径
	    QString subcase;                   //工况（如 "Subcase 1"）
	    QString schema;                    //结果类型
	    QString object;                    //对象（如 "Point id 8003"）
	    QString component;                 //分量（如 "X-Trans"）

	    //X/Y 侧独立来源：schema 为解析出的横坐标类型（"Time[Sec]"/"Frequency[Hz]"，随文件数据形态）时
	    //该侧取上方五元组锚通道数据块的记录横坐标，object/component 为空；为
	    //实际结果类型时该侧取 工况×结果类型×对象×分量 通道的分量值。schema 为
	    //空 = 旧数据未记录侧来源：X 视为横坐标形态、Y 视为五元组通道形态
	    QString xSideSubcase;
	    QString xSideSchema;
	    QString xSideObject;
	    QString xSideComponent;
	    QString ySideSubcase;
	    QString ySideSchema;
	    QString ySideObject;
	    QString ySideComponent;
        
	    //X/Y 数据属性（x/y 固定子节点为编辑入口，数据存本结构；设计图03.2）
	    CurveDataRefData xRef;             //X 侧：缩放/偏移/裁剪/坐标轴/单位等
	    CurveDataRefData yRef;             //Y 侧：同上（axisAssign 支持 Y1/Y2）

		//编辑曲线"隐藏"区间（设计图16）：三元组序列 [轴(0=X/1=Y), lo, hi]，
		//存原始数据空间；区间内数据点保留在模型中但渲染为断口（编辑曲线
		//"撤回"的快照覆盖本字段；空 = 无隐藏，旧数据零影响）
		QVector<double> hiddenRanges;

		//复数曲线：复数图目标导入/定义时单节点承载
		//上下面板两路数据（引擎 updateComplexCurveData 的 a/b 两参），X 共用
		//xData 等长；渲染层按 isComplex() 分派，不再跨节点按名配对
		QVector<double> complexYData;        //下面板数据（PHA/IMAG）
		CurveDataRefData complexRef;         //下面板侧数据属性（"yp" 壳节点编辑入口）
		int complexFormat = 0;               //展开方式：0 幅相(MagPhase) 1 实虚(RealImag)，对齐引擎 ComplexFormat
		QString complexSideSubcase;          //下面板侧来源四元组（形态同 ySide*；b 侧与 a 侧
		QString complexSideSchema;           //同数据块，仅分量令牌不同：MAG|方向 ↔ PHA|方向）
		QString complexSideObject;
		QString complexSideComponent;

		//是否复数曲线（complexYData 非空即按复数两面板渲染）
		bool isComplex() const { return !complexYData.isEmpty(); }

		//三维曲线（三维图 chartType=5 目标，"从文件新建三维图"弹窗写入；数据直写
		//不进命令栈，与 xData/yData 同口径）：zData 与 xData/yData 等长的第三维；
		//plot3dType=渲染序列类型（对齐弹窗"绘图类型"单选序）：
		//0 瀑布图(WaterfallSeries) 1 三维曲线(Line3DSeries) 2 曲面(Surface3DSeries)
		//3 柱状图(Bar3DSeries)；仅三维图下有意义，XY/柱状/复数曲线恒为默认值。
		//plot3dRows/Cols=曲面/瀑布的规则网格行列数（rows*cols==xData.size()；
		//线/柱状=0，渲染退化为 (N,1) 点序）——网格形状必须持久化，否则会话
		//恢复后曲面只剩扁平点列无法还原栅格
		QVector<double> zData;
		int plot3dType = 1;
		int plot3dRows = 0;
		int plot3dCols = 0;

		//三维曲线定义溯源（"定义曲线"三维弹窗应用时写入、重开弹窗载入还原；
		//会话快照 V13 起持久化，模板路径字段序冻结不含——旧数据缺省零影响）。
		//来源标记复用上方 sourceFile（kMathSourceTag/kNumericSourceTag/文件路径，
		//五元组/侧四元组三维定义不使用保持空）；X/Y 侧数学表达式复用上方
		//xRef/yRef.expression，Z 侧与文件分量列下标无既有字段落本组：
		QString z3DExpression;      //Z 侧数学表达式（非数学来源侧写空串清空）
		int x3DComponent = -1;      //文件来源 X 侧分量列下标（-1=未记录/非文件侧）
		int y3DComponent = -1;      //文件来源 Y 侧分量列下标（同上）
		int z3DComponent = -1;      //文件来源 Z 侧分量列下标（同上）

		//三维曲线"外观与着色"扩展（R36 七行翻活；仅 plot3dType==1 被 Line3DSeries
		//渲染消费，其余类型走色带渲染不消费——属性页按类型整组置灰防假编辑）。
		//线型/符号四件/线下区域填充复用上方既有键（lineStyle/symbolStyle/
		//symbolColor/symbolDensity/symbolSize/areaFill，字段均已存在零新增）；
		//三新键出厂默认即语义中性值（不填充/不云图/透明度对齐设计图滑块位置），
		//旧数据（序列化 V3 及更早快照无此段）读取保持缺省零影响
		int areaFillOpacity = 70;   //线下区域填充不透明度 0..100（语义=不透明度，
		                            //渲染 alpha=值/100；默认 70 对齐设计图滑块位置）
		bool contour = false;       //云图（按所选变量值映射色带渐变上色+画布显示色带）
		int contourVar = 0;         //云图变量：0=x / 1=y / 2=z（渐变取值维度）

		//三维色带类"显示模式"（R38，仅 plot3dType==2 曲面被 m3DChartView 消费：
		//rebuild 建 Surface3DSeries 后 setData 下发 setDrawStyle+样式增量分流）。
		//语义=引擎 DrawStyle 同值（Plot3DTypes.h）：0 着色曲面/1 等高线/
		//2 着色曲面+等高线；瀑布/柱状引擎无样式 API（属性页置灰占位不提交），
		//缺省 0=引擎默认 StyleSurface，旧快照（V4 及更早）缺省零行为变化
		int drawStyle = 0;

		//曲线类别（曲线统计按此过滤目标曲线；普通曲线之外的类别由对应
		//生成功能写入，见 CurveCategory——导入/定义曲线保持 Normal 不打标）
		CurveCategory curveCategory = CurveCategory::Normal;

		//点 i 是否落在隐藏区间内（i 越界或无隐藏区间时为 false）
		bool isPointHidden(int i) const
		{
			if (i < 0 || i >= qMin(xData.size(), yData.size()))
				return false;
			for (int r = 0; r + 2 < hiddenRanges.size(); r += 3)
			{
				const double v = (hiddenRanges.at(r) == 0.0)
					? xData.at(i) : yData.at(i);
				if (v >= hiddenRanges.at(r + 1) && v <= hiddenRanges.at(r + 2))
					return true;
			}
			return false;
		}
};

	//曲线数据来源标记串（"定义曲线"落模写入 sourceFile 的非文件形态：
	//mDefineCurveDlg 创建时写，模板序列化按此识别来源类别）
	inline const QString kMathSourceTag = QStringLiteral("数学运算");
	inline const QString kNumericSourceTag = QStringLiteral("数值");

	//曲线数据来源分类（模板序列化/应用管线共用判定）：
	//文件来源=真实数据文件路径（模板参数化+应用时重装配）；
	//数学来源=表达式定义（模板存表达式，应用时重新求值）；
	//数值来源=定义即数据（模板原样保留数据体）。旧数据可能全空（无文件无表达式）
	enum class CurveSourceKind { File, Math, Numeric };

	inline CurveSourceKind curveSeriesSourceKind(const CurveSeriesData& s)
	{
		const bool hasExpression = !s.xRef.expression.isEmpty()
			|| !s.yRef.expression.isEmpty() || !s.complexRef.expression.isEmpty();
		if (s.sourceFile == kMathSourceTag)
			return CurveSourceKind::Math;
		if (s.sourceFile == kNumericSourceTag)
			return CurveSourceKind::Numeric;
		if (s.sourceFile.isEmpty())
			return hasExpression ? CurveSourceKind::Math : CurveSourceKind::Numeric;
		return CurveSourceKind::File;
	}

	//坐标轴数据（XAxis/YAxis/YAxis2 节点共用，设计图05~07）
	//刻度模式采用"数据预变换+线性轴"策略（复刻 NVHCurveWgt 原型语义）：
	//模型存原始数据，渲染层按 scaleMode 变换后交给 PlotView 线性轴，
	//刻度数字即变换空间的值（对数轴显示 1、2、3...），dB20/dB10 一并支持
	struct CurveAxisData
	{
		QString title;             //轴标题
		bool titleVisible = true;  //名称（轴标题）可见性（属性编辑器"名称可见性"，渲染清空标题实现）
		QColor titleColor = QColor(0, 0, 0); //名称颜色（属性编辑器"颜色"，渲染映射 AxisSettings.titleColor）
		bool visible = true;       //轴显隐
		int scaleMode = 0;         //刻度模式：0 Linear / 1 Logarithmic(log10) / 2 dB20 / 3 dB10
		int weightMode = 0;        //计权：0无/1A/2B/3C/4U（U=unweighted 与无等效；全刻度模式生效，渲染层现算，见 mCurveWeighting.h）
		bool autoScale = true;     //自动范围（false 时使用 min/max；自动时精确贴合数据边界：左/下=数据最小值，右/上=数据最大值）
		double min = 0.0;          //手动范围下限
		double max = 10.0;         //手动范围上限
	    QFont titleFont;           //轴标题字体（pch 创建时来自绘图选项 xFont/yFont）
	    QFont labelFont;           //刻度数字字体（原型为 9pt 常规体）
	    QString unit;              //单位（显示用，渲染接线下一期：追加到轴标题）
	    double offset = 0.0;       //缩放补偿（数据整体平移，渲染接线下一期）
	    double majorStep = 10.0;   //固定步长（固定步长法的主刻度间距，绝对倍数对齐；<=0=引擎自动）
	    int minorCount = 2;        //每主刻度网格数（每主刻度段细分出的次刻度/次网格个数，>0 生效；<=0=引擎自动份数）
	    double dbRef = 1.0;        //dB 参考值（dB20/dB10 共用一项，面板行标签随缩放方式切换；默认 1.0=原型口径，2026-11-12 D25，旧 2e-11 系声学参考作废）
	    int labelFormat = 0;       //刻度数字格式：0自动/1科学计数法/2定点格式
	    int tickGenMode = 0;       //刻度生成方式（值=下拉索引）：linear 0均匀分步法/1固定步长法；log/dB 0每十倍分步法(遗留默认)/1均匀分步法/2固定步长法（后两者在刻度变换空间生效；跨 linear↔log/dB 切换重置为 0）
	    double tickCount = 6.0;    //刻度总数（均匀分步法；默认 6=原型口径，2026-11-12 D25）
	    int decadeTickCount = 5;   //每十倍刻度数（非 linear 轴：logarithmic/dB20/dB10）
	    int decadeMinorCount = 11; //每十倍网格线数（非 linear 轴；默认 11=原型十等分 10 段线口径，2026-11-12 D25）
	    int precision = 5;         //刻度数字精度（默认 5=原型口径，2026-11-12 D25）
	    int categoryLabelAlign = 0; //类别轴刻度标签对齐：0水平/1对角 / 斜向/2垂直/3斜排30°（-30°，D10；仅柱状图类别 X 轴，参数栏"标签对齐方式"）
	    QVector<double> tickValues;  //列表标注法主刻度值列表（tickGenMode 2=列表标注法，2d-chart-split-align；界内成员落主刻度，空表回落自动）
    };

	//绘图选项/创建流程各"默认"字体的唯一来源（像素字号/加粗为原型约定）。
	//mDrawingOptions 建图写入与参数栏/属性编辑器各字体按钮的"默认"回显基准
	//共用：未显式选择字体时显示"默认"并使用这里的预写值，用户选择后用所选值
	inline QFont headerDefaultFont()
	{
		QFont f; f.setPixelSize(28); f.setBold(true); return f;   //页眉
	}
	inline QFont footerDefaultFont()
	{
		QFont f; f.setPixelSize(20); f.setBold(true); return f;   //页脚
	}
	inline QFont axisTitleDefaultFont()
	{
		QFont f; f.setPixelSize(16); f.setBold(true); return f;   //X/Y 轴标题
	}
	//图例默认字体（渲染层加粗，对齐原型）。模型默认值与图例属性页字体按钮的
	//"默认"回显基准共用此值（按钮见 mCurveFontButton）
	inline QFont legendDefaultFont()
	{
		return QFont(QStringLiteral("Microsoft YaHei"), 9);
	}

	//图例数据（Legend 节点；属性编辑器弹窗双击图例空白/右键"编辑"激活）
	struct CurveLegendData
	{
		bool visible = true;
		bool autoPosition = true;     //自动定位（true=四角自动避让曲线，优先图内右上角；勾选时编辑器隐藏图例位置行）
		int position = 2;             //图例位置：0图内-左上 1图内-左下 2图内-右上 3图内-右下 4图内-用户自定义
		                              //            5图外-顶部 6图外-底部 7图外-左侧 8图外-右侧（引擎外置图例经 Qwt insertLegend 托管）
		double posX = 80.0;           //用户自定义位置X（画布百分比0-100，position=4时生效）
		double posY = 10.0;           //用户自定义位置Y（同上）
		int borderStyle = 1;          //边框样式：0无 1实线 2虚线 3划线(长虚线) 4点划线
		double borderWidth = 1.0;     //边框线宽（px，参数栏/属性编辑器"边框线宽"行）
		QColor borderColor = QColor(0, 0, 0);        //边框颜色
		QFont font = legendDefaultFont(); //图例字体（渲染层加粗，对齐原型）
		int textColorMode = 0;        //曲线文本色：0跟随曲线颜色 1自定义（选自定义时编辑器显示文本颜色行）
		QColor textColor = QColor(0, 0, 0);          //文本颜色（textColorMode=1时生效）
		QColor bgColor = QColor(255, 255, 255, 200); //背景填充色（默认白底半透明）
		int leaderPos = 0;            //引线位置（线样图标相对文字）：0左侧 1下方
	};

	//页眉/页脚数据（Header/Footer 节点）
	//页眉文本约定：pch 创建时取首条曲线名（复刻原型 firstLabel 行为）
	//渲染：经 PlotView 页眉/页脚条带 API（setHeaderText/setFooterText 等）下发
	struct CurveTextData
	{
		bool visible = false;
		QString text;
		int align = 0;             //对齐：0居中 1左 2右（原型 headerAlign/footerAlign）
		QFont font;                //字体（原型 headerFont/footerFont，一级标题）
		//二级/三级标题字体（编辑字符弹窗按行分级）：族名为空=未显式选择，
		//属性页按钮回显"默认"，渲染按一级字号依级缩小 4（二级 -4/三级 -8）
		QFont font2;
		QFont font3;
		//强调行（需求02表头层级：NVH 产出图表第 2 行标题加粗+字号放大）：
		//0 起的行号，-1=无强调行走通用分级（第 2 行起依级缩小）。页脚暂不用，
		//序列化对称占位（快照 V3 追加字段）
		int emphLine = -1;
		QColor color = QColor(0, 0, 0);   //文字颜色（kHeaderColor/kFooterColor，已接线渲染：
		                                  //引擎条带逐行颜色，选中态高亮优先、未选中=模型色/无效兜底黑）
	};

	//基准线数据（Baseline 节点）
	struct CurveBaselineData
	{
		QString label;                    //标签（线上文字，空=不显示；创建时初值=名称）
		int orientation = 1;              //0 横向(固定Y值，关联Y侧轴) 1 纵向(固定X值，关联X侧轴)
		double value = 0.0;               //位置（横向=Y值，纵向=X值）
		QColor color = QColor(0, 0, 255); //线条颜色，默认蓝
		int lineStyle = 1;                //Qt::PenStyle，默认实线
		double lineWidth = 1.0;           //线宽
		bool visible = true;
		QString layer = QStringLiteral("0");  //图层（参数栏"图层"行，占位）
		QFont labelFont;                  //标签字体（族名空="默认"按应用默认字体渲染）
		int labelPos = 4;                 //标签位置（9宫格）：0左上 1中上 2右上 3左中 4居中 5右中 6左下 7中下 8右下，默认居中（骑线）
		QString link;                     //关联轴名（X1/Y1/X2...；渲染暂只取方向，多轴定位下一期）
	};

	//三维基准面数据（DatumPlane3D 节点，R41）：三维图 chartType=5 的用户子节点，
	//树插入位置=图表基准分区（注释分区之后，ChartTypeProfile d3 份 zoneOrder 尾部追加）。
	//出厂默认=参考软件截图取值（法向 X 轴/(0,0,0)/橙/不透明度 0.25/标签居中/可见）。
	//序列化=会话快照 V7 起新节点类型分支携带（全字段无损往返）；模板路径本期
	//不纳入基准面（writeTemplateNode/readTemplateNode 无本结构字段——含基准面的
	//页存模板后，读入仅恢复名称、属性为出厂默认，模板 V2 字段序冻结零波及）
	struct CurveDatumPlane3DData
	{
		int type = 0;                      //基准类型：0=参考平面（本期唯一类型，字段入模保留）
		int normalAxis = 0;                //法向方向：0=X / 1=Y / 2=Z（默认 0=法向 X 轴；
		                                   //参考平面渲染只消费法向轴对应坐标）
		double posX = 0.0;                 //X 坐标（法向 X 时被渲染消费；另两轴字段入模
		                                   //保留——下一期类型/裁剪用，需求已备案）
		double posY = 0.0;                 //Y 坐标（法向 Y 时被渲染消费）
		double posZ = 0.0;                 //Z 坐标（法向 Z 时被渲染消费）
		QString text;                      //标签文字（空=渲染取本名"基准 N"）
		int labelPos = 4;                  //标签位置（平面内 9 宫格）：0左上 1中上 2右上 3左中
		                                   //4居中 5右中 6左下 7中下 8右下，默认居中（语义对齐
		                                   //2D 基准线 labelPos）
		QColor color = QColor(221, 147, 96); //基准颜色（橙，参考软件截图取色备案——同
		                                     //R38 曲面橙 221,147,96 一族）
		double opacity = 0.25;             //不透明度 0..1（默认 0.25=截图取值；半透明不写深度
		                                   //的渲染语义由视图层实现，模型只存标量）
		bool reverseClip = false;          //反向裁剪（本期占位字段入模保留，零消费）
		bool visible = true;               //单基准显隐（树右键/工具页列表勾选/属性页；图表级
		                                   //总开关另见 CurveChartData.datum3DVisible）
	};

	//注释数据（Annotation 节点）
	struct CurveAnnotationData
	{
		QString text;                        //注释文本
		double posX = 0.0;                   //注释框锚点 X（数据坐标；2D 曲线类=框中心引出线从附着点连到框，
		                                     //3D=R42 修订起框左上角锚点投影到画布随相机移动）
		double posY = 0.0;                   //注释框锚点 Y（数据坐标，语义同 posX）
		double posZ = 0.0;                   //注释框锚点 Z（数据坐标，仅 3D 注释消费=R42 修订
		                                     //三维数据空间锚点；2D 注释不消费零影响；快照
		                                     //V8 起持久化，V7 及更早读入缺省 0）
		QColor color;                        //文本颜色（无效=默认：跟随边框色）
		bool visible = true;                 //是否显示（树勾选/属性页开关）
		int align = 0;                       //对齐方式：0左 1中 2右（渲染接线下一期）
		QFont font;                          //文本字体（编辑/模型先行，渲染下一期；默认构造=应用默认）
		int borderMode = 0;                  //边框颜色：0跟随曲线颜色 1自定义 2无框（裸文字，不画框与底色）
		QColor borderColor;                  //边框自定义色（曲线注释 borderMode=1 生效；视图注释有效即生效）
		double borderWidth = 1.0;            //边框线宽（px）
		QColor bgColor;                      //背景颜色（无效=默认半透明白）
		bool borderShow = true;              //显示边框（R42 二期：3D 画布注释项边框显隐开关；
		                                     //2D 注释渲染暂不消费零影响；会话快照 V6 起持久化，
		                                     //V5 及更早快照读入缺省 true）
		bool autoPos = false;                //自动定位（按策略移动注释文本框）
		int autoAnchor = 0;                  //自动定位策略：0不重叠 1左上 2右上 3左中 4居中 5右中 6左下 7正下 8右下
		double autoOffset = 3.0;             //自动定位偏移（注释文本框到点的距离，px）
		int attachMode = 0;                  //注释类别：0曲线类（曲线上创建，带引出线）1视图类（曲线外创建）
		QString attachCurve;                 //附着曲线名（曲线类：吸附/取色/文本求值的目标曲线）
		int attachIndex = 0;                 //附着点顺序（1基，0=未指定；曲线类直接附着到该点）
		double boxPosX = qQNaN();            //注释框位置 X（仅 3D 内容锚定注释消费=R46：拖拽框体
		                                     //提交、锚点三键不动——牵引线拉长、{X}{Y}{Z} 稳定对齐
		                                     //二维；三维数据坐标=框左上角；NaN=未手动摆位，框=
		                                     //锚点+固定偏移；会话快照 V14 起持久化）
		double boxPosY = qQNaN();            //注释框位置 Y（语义同 boxPosX）
		double boxPosZ = qQNaN();            //注释框位置 Z（语义同 boxPosX）
		bool minimized = false;              //最小化（曲线类：锚点处画实心小方块，悬停还原完整样式；视图类：方块画在框中心）
		double minimizeRadius = 6.0;         //最小化方块边长（px）
		QColor minimizeColor;                //最小化颜色（无效=默认绿）
		bool scalarFmtOn = false;            //标量值格式化（勾选后 {X}/{Y}/{Slope}/{Curv} 等数值按下方格式/精度显示）
		int scalarFmtMode = 0;               //格式：0科学计数法('e') 1定点格式('f')
		int scalarFmtPrecision = 5;          //精度（小数位数，0..15）
	};

	//视图注释占位文本（曲线外点击先置此文本，编辑时回填默认模板；
	//图表视图双击编辑与属性页"文本"按钮共用）
	inline const QString kViewAnnotationPlaceholder = QStringLiteral("视图注释");

	//页数据（Page 节点，参考软件页面属性截图01；布局首消费=报告导出侧 20261102-R07，
	//视口平铺渲染下一期——curvePageLayoutGrid 派生表见 PostCurveGlobal.h）
	struct CurvePageData
	{
		double width = 100.0;        //页面宽度（面板暂不展示，键保留）
		QFont font;                  //页面字体（"字体设置"）
		int layout = 0;              //页面布局：视口布局图标索引（curveViewportIconPaths；
		                             //0=默认未选=自动，报告导出按 curvePageLayoutGrid 消费）
		QString pageTitle;           //页面标题模板（{p<w>c<n>.y.MX*} 花括号占位符，同 Y 轴标题
		                             //模板，渲染时解析；区别于 kName 树显示名）
		bool publishLayout = false;  //发布页面
		bool publishPageTitle = true; //发布页面标题
		int publishContentPos = 0;  //发布内容位置（面板暂不展示，键保留）
		int viewPageId = -1;        //对应视图页ID（视图树mViewTreeModel页，曲线页与视图页对齐；-1=未绑定/待新建）
	};

	//三维图单轴状态（三维图 chartType=5，图表级唯一——单视口单轴组；曲线下
	//x/y/z 壳节点经文档重定向读写本状态，同图表各曲线的壳互为同一轴的视图）
	//生效字段=Plot3D 引擎现成能力（label→setLabels、min/max→setRange、
	//tickCount→setTickCounts、gridVisible→setGridVisible）；autoFit 为宿主侧
	//语义：勾选（默认）=视图按全部曲线数据并集现算范围（追加曲线自动扩），取消=
	//使用 min/max 手动范围
	struct CurveAxis3DState
	{
		QString label = QStringLiteral("X");  //轴名称（渲染 setLabels；轴组实际缺省
		                                      //x/y/z 逐轴见 CurveChart3DData 构造）
		double min = 0.0;                     //手动范围下限（autoFit=false 时生效）
		double max = 10.0;                    //手动范围上限
		int tickCount = 6;                    //主刻度数（渲染 setTickCounts）
		bool gridVisible = true;              //显示网格（渲染 setGridVisible，本轴网格）
		bool autoFit = true;                  //自动适配（范围=全部曲线数据并集）
		//刻度类型（R43；值表与二维 CurveAxisData::scaleMode 同表）：
		//0 Linear / 1 Logarithmic(log10) / 2 dB20 / 3 dB10。渲染=宿主侧数据
		//预变换+线性轴策略（模型存原始值，m3DChartView 按本值逐点变换副本后
		//下发引擎，引擎零改动）；dB 参考值恒 1.0 纯 dB——三维页无参考值行
		//（用户拍板），不设 dbRef 字段。序列化=会话快照 V9/模板 V3 起逐轴追写
		int scaleType = 0;
	};

	//三维图轴组（挂在 CurveChartData.axis3d，仅 chartType=5 有效；序列化=会话
	//快照 V3 起追加、模板 V2 起追加）
	struct CurveChart3DData
	{
		CurveChart3DData()
		{
			//轴名称逐轴缺省=坐标轴 x/y/z（默认成员初始化对三轴同为同一值，
			//构造期改写；旧快照/模板有轴段则按存储值回读，无轴段落此缺省）
			axes[0].label = QStringLiteral("X");
			axes[1].label = QStringLiteral("Y");
			axes[2].label = QStringLiteral("Z");
		}
		CurveAxis3DState axes[3];            //0=X 1=Y 2=Z（壳节点类型→下标映射见
		                                     //CurveDocument 属性重定向）
		//网格样式（R44，图表级）：0 三维框（三固定背景面填充+网格，现状观感）/
		//1 线框（仅轴线+刻度+数字，无背景面无网格——宿主侧把三轴 gridVisible
		//有效旗标置 false 复用引擎 setGridVisible"关=该轴面填充与网格都不画"
		//语义，引擎零新 API）。默认 0=现状观感零变化；写侧不钳制（页侧保证
		//0..1，沿既有键口径；序列化读侧钳制防脏数据）。会话快照 V10/模板 V4
		//起随背景两色追写持久化
		int gridStyle = 0;
		//背景面底填充色（R44，图表级）：驱动 Axis3D::setBackdropFillColor；
		//默认 240,240,240=引擎原硬编码 0.94 灰的 255 域等效（0.94×255≈239.7→240，
		//8bit 帧缓冲像素同值），默认观感零变化。不复用二维 chart.backgroundColor
		//（255,255,255 默认语义冲突）。序列化=会话 V10/模板 V4 起
		QColor backdropColor = QColor(240, 240, 240);
		//背景面网格线颜色（R44，图表级）：驱动 Axis3D::setBackdropGridColor；
		//默认 191,191,191=引擎原硬编码 0.75 灰的 255 域等效（0.75×255≈191.25→191，
		//像素同值）。不复用二维 chart.gridColor（200,200,200 默认语义冲突）。
		//序列化=会话 V10/模板 V4 起
		QColor backdropGridColor = QColor(191, 191, 191);
	};

	//视口数据（Chart 节点，设计图02-02 图表属性；背景/网格渲染下一期）
	struct CurveChartData
	{
		bool visible = true;                     //视口显隐（树勾选联动 MDI 视口窗口）
		int chartType = 0;                       //图表类型：0 XY曲线图 1柱状图 2复数图（已接渲染，切换=全量重绘；复数图按"对象-MAG|分量"命名约定配对）；3极坐标图 4蛛网图（引擎暂无绘制能力，下拉置灰占位）；5三维图（Plot3D 渲染，仅经弹窗创建，属性页下拉置灰）
		QColor backgroundColor = QColor(255, 255, 255);    //图表背景颜色
		QColor gridColor = QColor(200, 200, 200);          //网格线颜色
		//复数图显示状态（设计图01 幅值-相位与实部-虚部灵活切换，右键菜单入口）：
		//显示格式为图表级状态，节点仍存创建时的原始两路数据（complexYData 与
		//complexFormat），切换在渲染层现算换算（幅相↔实虚互转），来回切换无损
		int complexDisplayFormat = 0;            //显示格式：0 幅值-相位 1 实部-虚部
		bool complexSwapped = false;             //复数面板互换：false=第一路(幅值/实部)在上，true=第二路(相位/虚部)在上
		CurveChart3DData axis3d;                 //三维图轴组（chartType=5 专用，其余类型保持默认值）
		bool datum3DVisible = true;              //三维基准面图表级总开关"显示基准"（R41：关=隐藏本
		                                         //图表全部 DatumPlane3D 子节点；仅 Chart 节点放行，
		                                         //2D 图表无消费零影响；会话快照 V7 起持久化，V6 及
		                                         //更早快照读入缺省 true）
		bool annotationsVisible = true;          //注释图表级总开关（R42 二期：三维图画布注释项全显隐，
		                                         //三维注释管理页"显示注释"行消费；2D 图表无消费、
		                                         //零影响；会话快照 V6 起持久化，V5 及更早快照读入缺省 true）
	};

	//选中/hover 高亮主题蓝（设计图03.1：单击选中变蓝加粗、hover仅变蓝）。
    //属视图层临时状态，不进模型与命令栈；色值沿用模块默认曲线色（主题蓝）
    inline QColor curveHighlightBlue() { return QColor(0, 102, 204); }

    //属性名常量：命令栈（CurveSetPropertyCommand）与属性页统一使用的属性键。
	//修改链路：属性页控件变更 -> commit(属性名, 值) -> 控制器 -> 命令栈（可撤销）-> 文档分发。
	//kName 所有节点可用；其余属性仅对应类型节点支持（不支持时 setNodeProperty 返回 false）。
	//注意：lineStyle 在曲线侧是 TecLinePattern(0..3)，在基准线侧是 Qt::PenStyle(1..4)，由属性页负责换算
	namespace CurveProp
	{
		//通用
		inline const char* const kName = "name";
		//页
		inline const char* const kPageTitle = "pageTitle"; //页面标题模板（{p<w>c<n>.y.MX*}，同 Y 轴标题模板；区别于 kName）
		//图表
		inline const char* const kHeaderVisible = "headerVisible";
		inline const char* const kHeaderText = "headerText";
		inline const char* const kFooterVisible = "footerVisible";
		inline const char* const kFooterText = "footerText";
		//曲线
		inline const char* const kVisible = "visible";
		inline const char* const kColor = "color";
		inline const char* const kLineStyle = "lineStyle";
		inline const char* const kLineWidth = "lineWidth";
		inline const char* const kShowSymbol = "showSymbol";
		inline const char* const kSymbolSize = "symbolSize";
		inline const char* const kSymbolDensity = "symbolDensity";   //符号密度（标记间隔：1=逐点）
		inline const char* const kSymbolScale = "symbolScale";       //缩放尺寸系数（实际大小=基准×系数）
		inline const char* const kSymbolColor = "symbolColor";
		//坐标轴
		inline const char* const kTitle = "title";
		inline const char* const kTitleVisible = "titleVisible";
		inline const char* const kTitleColor = "titleColor";
		inline const char* const kScaleMode = "scaleMode";
		inline const char* const kWeightMode = "weightMode";   //计权（0无/1A/2B/3C/4U，U=unweighted 与无等效）
		inline const char* const kAutoScale = "autoScale";
		inline const char* const kMin = "min";
		inline const char* const kMax = "max";
		inline const char* const kTitleFont = "titleFont";
		inline const char* const kLabelFont = "labelFont";
		//图表页眉页脚
		inline const char* const kHeaderAlign = "headerAlign";
		inline const char* const kHeaderFont = "headerFont";
		inline const char* const kHeaderFont2 = "headerFont2";   //二级标题字体
		inline const char* const kHeaderFont3 = "headerFont3";   //三级标题字体
		inline const char* const kHeaderEmphLine = "headerEmphLine"; //表头强调行（-1=无，0起）
		inline const char* const kFooterAlign = "footerAlign";
		inline const char* const kFooterFont = "footerFont";
		inline const char* const kFooterFont2 = "footerFont2";   //二级标题字体
		inline const char* const kFooterFont3 = "footerFont3";   //三级标题字体
		//曲线符号
		inline const char* const kSymbolStyle = "symbolStyle";
		//曲线类别标记（CurveCategory 枚举的 int 形态；生成功能经控制器
		//setCurveCategory 写入，曲线统计按类别过滤——非属性页编辑项，无面板行）
		inline const char* const kCurveCategory = "curveCategory";
		//曲线标签与区域填充（属性编辑器弹窗；标签渲染=图例文本，已接线）
		inline const char* const kLabelVisible = "labelVisible";
		inline const char* const kLabelContent = "labelContent";
		inline const char* const kLabelText = "labelText";
		inline const char* const kLabelPrefix = "labelPrefix";
		inline const char* const kLabelSuffix = "labelSuffix";
		inline const char* const kLabelPrefixVisible = "labelPrefixVisible";
		inline const char* const kLabelSuffixVisible = "labelSuffixVisible";
		inline const char* const kAreaFill = "areaFill";
		//三维曲线外观与着色（R36；线型/符号四件/区域填充复用既有键不新增——
		//kLineStyle/kSymbolStyle/kSymbolColor/kSymbolDensity/kSymbolSize/kAreaFill
		//在三维图语境下由 mCurve3DPropPage 提交、m3DChartView 渲染消费）
		inline const char* const kAreaFillOpacity = "areaFillOpacity"; //填充不透明度 0..100（语义=不透明度，默认 70）
		inline const char* const kContour = "contour";                 //云图开关（按所选变量值渐变上色+显示色带）
		inline const char* const kContourVar = "contourVar";           //云图变量 0=x/1=y/2=z
		inline const char* const kDrawStyle = "drawStyle";             //色带类显示模式（R38，0着色曲面/1等高线/2着色曲面+等高线；仅曲面消费）
		//基准线
		inline const char* const kLabel = "label";
		inline const char* const kOrientation = "orientation";
		inline const char* const kValue = "value";
		inline const char* const kLayer = "layer";
		//注释
		inline const char* const kText = "text";
		inline const char* const kPosX = "posX";
		inline const char* const kPosY = "posY";
		//kPosZ 复用基准面侧同名键（R41 定义，分发按节点类型区分——3D 注释锚点
		//Z 数据坐标 R42 修订起消费，仅 Annotation 节点放行）
		//页
		inline const char* const kWidth = "width";
		inline const char* const kFont = "font";
		inline const char* const kLayout = "layout";                 //页面布局（视口布局图标索引）
		inline const char* const kPublishLayout = "publishLayout";   //发布页面
		inline const char* const kPublishPageTitle = "publishPageTitle"; //发布页面标题
		inline const char* const kPublishContentPos = "publishContentPos";
		//视口（图表属性）
		inline const char* const kChartVisible = "chartVisible";
		inline const char* const kChartType = "chartType";
		inline const char* const kBackgroundColor = "backgroundColor";
		inline const char* const kGridColor = "gridColor";
		inline const char* const kComplexDisplayFormat = "complexDisplayFormat"; //复数图显示格式（0幅相/1实虚，右键切换）
		inline const char* const kComplexSwapped = "complexSwapped";           //复数面板互换（上下面板交换）
		inline const char* const kAnnotationsVisible = "annotationsVisible";   //注释图表级总开关（R42，三维图
		                                                                       //画布注释项全显隐；仅 Chart 节点放行）
		//三维图轴（AxisX3DData/Y/Z 壳节点读写，重定向到所属图表 axis3d[轴下标]；
		//键不分轴——由壳节点类型决定写入哪条轴）
		inline const char* const kAxis3DLabel = "axis3dLabel";           //轴名称（渲染 setLabels）
		inline const char* const kAxis3DMin = "axis3dMin";               //手动范围下限（autoFit 关时生效）
		inline const char* const kAxis3DMax = "axis3dMax";               //手动范围上限
		inline const char* const kAxis3DTickCount = "axis3dTickCount";   //主刻度数（渲染 setTickCounts）
		inline const char* const kAxis3DGridVisible = "axis3dGridVisible"; //显示网格（渲染 setGridVisible 本轴）
		inline const char* const kAxis3DAutoFit = "axis3dAutoFit";       //自动适配（范围=全部曲线数据并集）
		inline const char* const kAxis3DScaleType = "axis3dScaleType";   //刻度类型（R43，0 Linear/1 log10/2 dB20/3 dB10，值表同二维 kScaleMode；渲染=宿主侧数据预变换，ref 恒 1.0 纯 dB）
		//三维图图表级网格样式/背景面配色（R44，仅 Chart 节点放行，存
		//CurveChartData.axis3d 三字段；**图表级**——任意轴壳页编辑全图生效，
		//不随「应用到」扇出；三行提交目标=所属图表节点。命名带 chart3d 前缀
		//区分于轴级 kAxis3D* 与二维图表 kBackgroundColor/kGridColor——默认值
		//语义冲突不复用，三维用新字段保证默认观感零变化）
		inline const char* const kChart3DGridStyle = "chart3dGridStyle";             //网格样式（0 三维框/1 线框；线框=宿主侧三轴旗标关断复用引擎既有语义，引擎零新 API）
		inline const char* const kChart3DBackdropColor = "chart3dBackdropColor";     //背景面底填充色（默认 240,240,240=引擎硬编码 0.94 灰等效）
		inline const char* const kChart3DBackdropGridColor = "chart3dBackdropGridColor"; //背景面网格线颜色（默认 191,191,191=引擎硬编码 0.75 灰等效）
		//曲线 X/Y 数据（设计图03.2，CurveXData/CurveYData 壳节点重定向到父曲线的 xRef/yRef，
		//键不分侧——由节点类型决定写入 xRef 还是 yRef）
		inline const char* const kScale = "scale";
		inline const char* const kOffset = "offset";
		inline const char* const kPhase = "phase";
		inline const char* const kClipEnabled = "clipEnabled";
		inline const char* const kClipMin = "clipMin";
		inline const char* const kClipMax = "clipMax";
		inline const char* const kExpression = "expression";
		inline const char* const kAxisAssign = "axisAssign";
		inline const char* const kUnit = "unit";
		//曲线数据来源回显（只读：x/y 壳节点查询，创建时经控制器 setSeriesSource 写入）
		inline const char* const kSourceFile = "sourceFile";
		inline const char* const kSubcase = "subcase";
		inline const char* const kSchema = "schema";
		inline const char* const kObject = "object";
		inline const char* const kComponent = "component";
		//图例（kPosX/kPosY 复用注释侧同名键——分发按节点类型区分，图例侧语义=画布百分比）
		inline const char* const kAutoPosition = "autoPosition";
		inline const char* const kPosition = "position";
		inline const char* const kBorderStyle = "borderStyle";
		inline const char* const kLegendBorderWidth = "legendBorderWidth";   //图例边框线宽（px；kBorderWidth 已被注释占用）
		inline const char* const kBorderColor = "borderColor";
		inline const char* const kLegendFont = "legendFont";
		inline const char* const kTextColor = "textColor";
		inline const char* const kBgColor = "bgColor";
		inline const char* const kLeaderPos = "leaderPos";
		inline const char* const kTextColorMode = "textColorMode";
		//页眉页脚颜色
		inline const char* const kHeaderColor = "headerColor";
		inline const char* const kFooterColor = "footerColor";
		//基准线标签（"标签字体"复用轴侧 kLabelFont 同名键——分发按节点类型区分）
		inline const char* const kLabelPos = "labelPos";
		inline const char* const kLink = "link";
		//注释显示
		inline const char* const kAlign = "align";
		inline const char* const kBorderMode = "borderMode";
		inline const char* const kBorderWidth = "borderWidth";
		inline const char* const kBorderShow = "borderShow";   //显示边框（R42 二期，3D 画布注释项边框显隐；
		                                                       //仅 Annotation 节点放行）
		//注释边框/背景自定义色复用图例侧 kBorderColor/kBgColor 同名键（分发按节点类型区分）
		inline const char* const kAutoPos = "autoPos";
		inline const char* const kAutoAnchor = "autoAnchor";
		inline const char* const kAutoOffset = "autoOffset";
		//注释类别：曲线上创建=曲线类，曲线外=视图类；
		//附着于下拉切换即切换类别（曲线类锚点吸附附着曲线最近点）
		inline const char* const kAttachMode = "attachMode";
		inline const char* const kAttachCurve = "attachCurve";
		inline const char* const kAttachIndex = "attachIndex";
		//3D 注释框位置三键（R46，仅 Annotation 节点放行——内容锚定注释拖拽框体
		//提交，锚点 kPosX/kPosY/kPosZ 不动=牵引线拉长、{X}{Y}{Z} 稳定对齐二维；
		//键名独立不复用 kPosX 族——语义是"框左上角"而非"锚点"，按 NaN=未摆位区分）
		inline const char* const kBoxPosX = "boxPosX";
		inline const char* const kBoxPosY = "boxPosY";
		inline const char* const kBoxPosZ = "boxPosZ";
		//注释最小化：曲线类锚点处实心小方块，悬停还原完整样式；
		//视图类方块画在注释框中心位置
		inline const char* const kMinimized = "minimized";
		inline const char* const kMinimizeRadius = "minimizeRadius";
		inline const char* const kMinimizeColor = "minimizeColor";
		//注释标量值格式化：勾选后注释文本中 {X}/{Y}/{Slope}/{Curv} 等
		//求值数值按格式（0科学计数法 1定点格式）与精度显示
		inline const char* const kScalarFmtOn = "scalarFmtOn";
		inline const char* const kScalarFmtMode = "scalarFmtMode";
		inline const char* const kScalarFmtPrecision = "scalarFmtPrecision";
		//轴（单位/缩放补偿/刻度细项，渲染接线下一期）
		inline const char* const kAxisOffset = "axisOffset";
		inline const char* const kMajorStep = "majorStep";
		inline const char* const kMinorCount = "minorCount";
		inline const char* const kDbRef = "dbRef";                       //dB 参考值（dB20/dB10 共用）
		inline const char* const kLabelFormat = "labelFormat";           //刻度数字格式（0自动/1科学计数法/2定点格式）
		inline const char* const kTickGenMode = "tickGenMode";           //刻度生成方式（linear 0均匀/1固定步长；log/dB 0每十倍/1均匀/2固定步长）
		inline const char* const kTickCount = "tickCount";               //刻度总数（均匀分步法）
		inline const char* const kDecadeTickCount = "decadeTickCount";   //每十倍刻度数（非 linear 轴）
		inline const char* const kDecadeMinorCount = "decadeMinorCount"; //每十倍网格线数（非 linear 轴）
		inline const char* const kPrecision = "precision";               //刻度数字精度
		inline const char* const kCategoryLabelAlign = "categoryLabelAlign"; //类别轴刻度标签对齐（0水平/1对角 / 斜向/2垂直，仅柱状图类别 X 轴）
		inline const char* const kTickValues = "tickValues";             //列表标注法主刻度值列表（tickGenMode 2；QVector<double> 值类型）
		//三维基准面（R41，DatumPlane3D 节点）：kName/kVisible/kColor/kText/kLabelPos
		//复用既有键（分发按节点类型区分——kText 注释侧失焦提交语义、kLabelPos 基准线
		//9 宫格语义同源，由页面层负责换算，模型层只管存取）；kPosX/kPosY 复用注释/
		//图例侧同名键（基准面侧语义=X/Y 参考坐标）
		inline const char* const kDatumType = "datumType";           //基准类型（0=参考平面，本期唯一；字段入模保留）
		inline const char* const kNormalAxis = "normalAxis";         //法向方向（0=X/1=Y/2=Z，默认 0；渲染只消费该轴坐标）
		inline const char* const kPosZ = "posZ";                     //Z 坐标（X/Y 坐标复用 kPosX/kPosY；参考平面只消费法向轴坐标，另两轴入模保留——需求备案）
		inline const char* const kDatumOpacity = "datumOpacity";     //不透明度 0..1（默认 0.25；不复用 kAreaFillOpacity——那是曲线线下填充 0..100 口径）
		inline const char* const kReverseClip = "reverseClip";       //反向裁剪（本期占位字段入模保留，零消费）
		inline const char* const kDatum3DVisible = "datum3DVisible"; //图表级"显示基准"总开关（仅 Chart 节点放行，默认 true，
		                                                             //存 CurveChartData.datum3DVisible；单基准显隐走 kVisible）
	}

	//节点"显示/隐藏"切换的属性键（树右键菜单等入口共用；Chart/Header/Footer 用独立键，
	//其余类型统一 kVisible——原 mCurveTreeView::addToggleVisibleAction 的映射搬运）
	inline QString nodeVisibleToggleProp(CurveNodeType type)
	{
		switch (type)
		{
		case CurveNodeType::Chart:  return QString(CurveProp::kChartVisible);
		case CurveNodeType::Header: return QString(CurveProp::kHeaderVisible);
		case CurveNodeType::Footer: return QString(CurveProp::kFooterVisible);
		default:                    return QString(CurveProp::kVisible);
		}
	}

	//是否可树内重命名（双击/F2 进入编辑；固定子节点名固定不可编辑。
	//节点类型词表：与图表类型无关，不进 ChartTypeProfile）
	inline bool nodeTypeRenameable(CurveNodeType type)
	{
		switch (type)
		{
		case CurveNodeType::Page:
		case CurveNodeType::Chart:
		case CurveNodeType::Curve:
		case CurveNodeType::Baseline:
		case CurveNodeType::Annotation:
		case CurveNodeType::DatumPlane3D:   //三维基准面（R41）：树内可重命名（kName 既有分支）
			return true;
		default:
			return false;
		}
	}
}
