#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维寻点核心（CurveCore，纯逻辑无 GUI 依赖，R47/T47-1）
//
// 三维数值页（mPointSeek3DPage）寻点导航的算法单一实现：输入一条三维曲线的
// 点集（x/y/z 三列等长，N 点），输出各列步进序/全局极值/局部峰谷/列可用性，
// 供页面接线消费；无 GUI 依赖，可离屏单测（workspace tools/pointseektest）。
//
// 语义对齐 HyperGraph 寻点行为（20261117 用户拍板"完整语义"；需求
// 20261117-三维数值页寻点与坐标数据表接入.md；CurveReadMe §5.13 R47 条；
// T47-2 20261117 用户对齐修正两口径见 extremumPointSet/columnEnabled；
// T47-3 曲线类型 X 列按序号步进见 stepSeek）：
//   - 列语义：0=极值（高度 Z）/1=X轴（按 x 升序稳定排序）/2=Y轴（同 X
//     口径作用于 y）。
//   - 有效点：三坐标均 finite（qIsFinite；NaN/±inf 任一即 NaN 断口点，
//     不可寻/不可停驻/不判峰谷——对齐 R40/定义曲线弹窗修订四非有限值口径）。
//     全部接口只在有效点上产出结果。
//   - 极值点集（T47-2 20261117 用户拍板 ◄► 切换极值）：E=局部峰∪局部谷
//     （判定口径见 localPeak/ValleyIndices：严格大于/小于两邻、端点单侧、
//     邻 NaN 按该侧不存在、仅有限点），按下标升序排列。极值列首行 ◄► 在
//     E 上切换（◄=E 中小于当前下标的最大者/►=大于当前下标的最小者），X/Y
//     列步进序不变。
//   - 步进序：极值列=极值点集 E；X 列=曲线类型（plot3dType==1）按序号
//     （有效点下标升序，跳过非 finite——T47-3 20261117 用户拍板）/其余类型
//     该坐标升序**稳定**排序；Y 列=该坐标升序稳定排序。步进在该序上前后
//     移动，止于端点不环绕。seekOrder 仍返回各列全点排序（极值列=下标序
//     ——坐标数据表行序/网格摊平序口径；X/Y 列=坐标升序稳定排序）。
//   - 全局极值：该坐标在**有效点**中的最大/最小点（并列取下标最小者=取首；
//     NaN 断口点不入候选——落点必须可寻/可渲染）。
//   - 局部峰谷：沿**下标序**扫描，峰=该坐标严格大于两邻（端点单侧判定；邻
//     点该坐标非 finite 时按该侧不存在处理；平台值不判峰——严格大于口径）；
//     谷=严格小于，同理。候选点恒为有效点（NaN 断口点不判峰谷）。
//   - 列可用性：点数<2 或该坐标全部 finite 值极差==0（全程恒值）→ 整列禁用
//     （页面按此启停按钮列）。
// 精度约定：算法全程 double 域（qIsFinite/直接比较，不做 float 收缩）；
//   消费方（模型 series 数据/表格 'f',4 格式化）同为 double 口径。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QVector>
#include <QString>

namespace PostCurve
{
	class POSTCURVE_EXPORT mPointSeekCore
	{
	public:
		//列语义（导航按钮三列；CurveCore 无 GUI 依赖，枚举即语义）
		enum Column
		{
			ColValue = 0,   //极值列：作用坐标=zData，步进序=极值点集（T47-2）
			ColX = 1,       //X轴列：作用坐标=xData，步进序=曲线类型按序号/其余 x 升序稳定排序（T47-3）
			ColY = 2,       //Y轴列：作用坐标=yData，步进序=y 升序稳定排序
		};

		//点数（三列等长口径的最小值；三列空/不齐时以最短列为准，越界安全）
		static int pointCount(const QVector<double>& xs, const QVector<double>& ys,
			const QVector<double>& zs);

		//点 i 是否可寻（下标有效且 x/y/z 均 finite；NaN 断口点不可寻）
		static bool isValidPoint(int i, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//指定列的全点排序：有效点下标集合；极值列=下标序（坐标数据表行序/
		//网格摊平序口径，**非**步进序——步进序见 extremumPointSet），X/Y 列=
		//该坐标升序稳定排序（等值保原下标序）。空数据/无有效点返回空表
		static QVector<int> seekOrder(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//全局极值（该坐标在有效点中的最大/最小点，并列取首个）；无有效点
		//返回 -1
		static int globalMaxIndex(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);
		static int globalMinIndex(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//局部峰/谷下标表（沿下标序升序排列）：峰=该坐标严格大于两邻（端点
		//单侧；邻点该坐标非 finite 按该侧不存在；平台值不判峰），谷=严格小于
		//同理；候选点恒为有效点
		static QVector<int> localPeakIndices(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);
		static QVector<int> localValleyIndices(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//极值点集 E（T47-2 20261117 用户拍板 ◄► 切换极值）：局部峰∪局部谷
		//下标集合，按下标升序排列（判定口径与 localPeak/ValleyIndices 一致：
		//严格大于/小于两邻、端点单侧、邻 NaN 按该侧不存在、候选恒为有效点；
		//单点曲线该点同判峰与谷，去重）。空数据/全平台（无严格变化）返回空表
		static QVector<int> extremumPointSet(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//步进（首行 ◄►；plot3dType=节点 series.plot3dType 类型感知，与
		//columnEnabled 同口径）：极值列=在极值点集 E 上切换——◄=E 中小于当前
		//下标的最大者、►=E 中大于当前下标的最小者（当前点∈E 即取前后邻居）；
		//当前下标无效（-1/越界/NaN 断口点）=dir>0 取 E 首个、dir<0 取 E 末个；
		//方向无后继**止于端点不环绕**（返回 fromIndex 原值）。X 列=曲线类型
		//（plot3dType==1）按序号（有效点下标升序，跳过 NaN 断口点——T47-3
		//20261117 用户拍板）/其余类型（0/2/3）该坐标升序稳定排序（T47-1 口径）；
		//Y 列恒=该坐标升序稳定排序（曲线类型 Y 列整列禁用不可达，防御口径同
		//X 非曲线）。序内前后一步越端止步、不在序内=dir>0 落序首/dir<0 落序尾。
		//无点可寻返回 -1
		static int stepSeek(int plot3dType, int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs,
			int fromIndex, int dir);

		//方向最近峰/谷（◄∩/►∩/◄U/►U）：沿**下标序**自 fromIndex 前/后（dir=
		//-1/+1）最近一个局部峰（peak=true）/谷；fromIndex<0 时 dir>0 取首个、
		//dir<0 取末个；方向上无峰/谷返回 -1（原地不动，不环绕）
		static int nearestExtremum(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs,
			int fromIndex, bool peak, int dir);

		//列可用性：点数<2 或该坐标全部 finite 值极差==0（全程恒值/不足两个
		//finite 值）返回 false → 该列整列禁用
		static bool coordinateVaries(int column, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//三列启停统一口径（T47-2 20261117 用户拍板：曲线类型 Y 列固定禁用，
		//替代恒值推断口径——设计稿与截图中的曲线 Y 列灰即此口径）：
		//plot3dType==1（三维曲线，节点 series.plot3dType）→ Y 轴列固定整列
		//禁用（首行 ◄► 与 ∩/U 组一并灰）；其余类型（0 瀑布/2 曲面/3 柱状）
		//与其余列（极值/X轴）维持恒值兜底判定（coordinateVaries）
		static bool columnEnabled(int plot3dType, int column,
			const QVector<double>& xs, const QVector<double>& ys,
			const QVector<double>& zs);

		//导出表格 CSV 文本装配（T47-4 20261117 用户拍板口径）：表头行
		//"横坐标,纵坐标,高度"+数据行=全部点按下标序（坐标数据表行序——网格
		//类行主序摊平同序），三列横坐标/纵坐标/高度（'f',4 与表格显示一致；
		//**无序号列**）；非有限值导空串（对齐定义曲线弹窗数值表非有限行空串
		//口径）；逗号分隔+CRLF 行尾。BOM 不在此装配——写文件侧统一
		//QTextStream UTF-8 BOM 落盘（页面 writeTableCsv 同款，二进制模式写
		//规避 Text 模式 \n 再翻译叠加 \r\r\n，mDynStiffnessMapping 先例）
		static QString exportTableCsv(const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

	private:
		mPointSeekCore() = delete;   //静态工具类，不可实例化（mCurveExpressionEvaluator 同款）

		//指定列第 i 点的作用坐标（越界返回 NaN；column 越界按 ColValue 兜底）
		static double coordinateAt(int column, int i, const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);

		//有效点下标升序表（NaN 断口点不入序；seekOrder 全点序与 X 列曲线
		//类型步进序共用——T47-3）
		static QVector<int> validIndexOrder(const QVector<double>& xs,
			const QVector<double>& ys, const QVector<double>& zs);
	};
}
