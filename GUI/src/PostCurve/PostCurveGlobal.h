#pragma once

#include <QtCore/qglobal.h>
#include <QColor>
#include <QList>
#include <QStringList>
#include <QVector>

#ifndef BUILD_STATIC
# if defined(POSTCURVE_LIB)
#  define POSTCURVE_EXPORT Q_DECL_EXPORT
# else
#  define POSTCURVE_EXPORT Q_DECL_IMPORT
# endif
#else
# define POSTCURVE_EXPORT
#endif

namespace PostCurve
{
	//曲线默认轮换配色（六色，NVHCurveWgt 先例）：pch 创建曲线与"定义曲线"新增
	//曲线的取色基准；并入/新增时从目标图表既有曲线数续起，避免与已有曲线同色
	inline QVector<QColor> curveDefaultColorWheel()
	{
		return {
			QColor(220, 50, 50), QColor(230, 180, 0), QColor(0, 150, 70),
			QColor(130, 70, 180), QColor(255, 130, 0), QColor(40, 40, 40)
		};
	}

	//视口布局图标（20种栅格布局）：mOpenCurveDlg"布局"下拉旁的网格选择与页面属性
	//"页面布局"共用这份路径表。顺序与主窗口视图树的 viewLayoutTable（mViewBrowserWindow）
	//严格一一对应——本表下标即曲线页 kLayout 属性值与视图树页布局下标，三方
	//（对话框网格/页面属性/视图显示栏布局按钮）经同一下标互通
	inline QStringList curveViewportIconPaths()
	{
		return {
			QStringLiteral(":/mMainWidgets/Icon/viewport/1.svg"),             //索引 0
			QStringLiteral(":/mMainWidgets/Icon/viewport/2@横分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/2@纵分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@上1下2.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@上2下1.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@左1右2.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@左2右1.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@横分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/3@纵分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/4@均分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/4@横分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/4@纵分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/6@上3下3.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/6@左3右3.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/8@上4下4.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/8@左4右4.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/9@均分.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/12@上4中4下4.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/12@左4中4右4.svg"),
			QStringLiteral(":/mMainWidgets/Icon/viewport/16@均分.svg"),
		};
	}

	//与 curveViewportIconPaths 同序的布局格数（镜像主窗口 viewLayoutTable 的
	//格数；模块内建图分页使用——PostCurve 不依赖 MainWindow，取不到源表）。
	//两表顺序已对齐，改动任一侧须同步另一侧
	inline QList<int> curveViewportLayoutSlots()
	{
		return {
			1, 2, 2, 3, 3, 3, 3, 3, 3, 4,      //索引 0~9
			4, 4, 6, 6, 8, 8, 9, 12, 12, 16,   //索引 10~19
		};
	}

	//页布局网格行列（kLayout 的导出侧派生，20261102-R07 报告导出跟随页面
	//布局=该键首个消费方）。rows/cols 为 0 表示"自动"（调用方维持各自现状
	//排布，如报告导出 3 列）；kLayout=0（默认）/负值/越界一律返回 {0,0}。
	//行主序语义：rows=⌈图窗数/cols⌉ 由消费方按数量现算，此处给预设本身的
	//等效规则网格边界。
	//派生依据=主窗口 viewLayoutTable（mViewBrowserWindow.cpp，条带模型）的
	//视觉展开，已与 resource/Icon/viewport/ 各 svg 图标逐个核对（svg 坐标
	//x=列 y=行）：verticalBands=true 条带纵向堆叠（行），rows=条带数、
	//cols=最宽条带格数；false 条带横向排列（列），cols=条带数、rows=最高
	//条带格数。条带格数不全相等的"不规则预设"（3@上1下2/上2下1/左1右2/
	//左2右1，索引 3~6）首期退化为边界等效规则网格 2×2（保留上/下或左/右
	//两段条带结构，3 图按行主序填格后末行/末列留白方位与原图标最接近；
	//逐格不等分的精确复刻待预设几何数据可消费后再对齐——视口平铺渲染
	//仍是下一期，本表仅导出侧消费）。下表注释即 20 预设↔行列对照表：
	//  索引  0  1.svg           1×1（单视口）
	//  索引  1  2@横分          2 行×1 列
	//  索引  2  2@纵分          1 行×2 列
	//  索引  3  3@上1下2        退化 2×2（不规则：上带 1 格/下带 2 格）
	//  索引  4  3@上2下1        退化 2×2（不规则：上带 2 格/下带 1 格）
	//  索引  5  3@左1右2        退化 2×2（不规则：左带 1 格/右带 2 格）
	//  索引  6  3@左2右1        退化 2×2（不规则：左带 2 格/右带 1 格）
	//  索引  7  3@横分          3 行×1 列
	//  索引  8  3@纵分          1 行×3 列
	//  索引  9  4@均分          2×2
	//  索引 10  4@横分          4 行×1 列
	//  索引 11  4@纵分          1 行×4 列
	//  索引 12  6@上3下3        2 行×3 列
	//  索引 13  6@左3右3        3 行×2 列
	//  索引 14  8@上4下4        2 行×4 列
	//  索引 15  8@左4右4        4 行×2 列
	//  索引 16  9@均分          3×3
	//  索引 17  12@上4中4下4    3 行×4 列
	//  索引 18  12@左4中4右4    4 行×3 列
	//  索引 19  16@均分         4×4
	struct CurvePageLayoutGrid
	{
		int rows = 0;   //0=自动（kLayout 未选/越界/不可解析）
		int cols = 0;
	};
	inline CurvePageLayoutGrid curvePageLayoutGrid(int kLayout)
	{
		//{rows, cols}；顺序与 curveViewportIconPaths()/viewLayoutTable 严格同序。
		//kLayout<=0 一律按"未选"处理（需求拍板：默认 0=自动排布，R06 回归线），
		//预设 0（单视口 1×1）因此不可经 kLayout 表达——与"未选布局"共用自动排布，
		//属首期已知限制（页属性网格选首格写 0，与默认值不可区分）
		static const int kGrids[][2] = {
			{ 1,  1},   //0  1.svg
			{ 2,  1},   //1  2@横分
			{ 1,  2},   //2  2@纵分
			{ 2,  2},   //3  3@上1下2（不规则退化）
			{ 2,  2},   //4  3@上2下1（不规则退化）
			{ 2,  2},   //5  3@左1右2（不规则退化）
			{ 2,  2},   //6  3@左2右1（不规则退化）
			{ 3,  1},   //7  3@横分
			{ 1,  3},   //8  3@纵分
			{ 2,  2},   //9  4@均分
			{ 4,  1},   //10 4@横分
			{ 1,  4},   //11 4@纵分
			{ 2,  3},   //12 6@上3下3
			{ 3,  2},   //13 6@左3右3
			{ 2,  4},   //14 8@上4下4
			{ 4,  2},   //15 8@左4右4
			{ 3,  3},   //16 9@均分
			{ 3,  4},   //17 12@上4中4下4
			{ 4,  3},   //18 12@左4中4右4
			{ 4,  4},   //19 16@均分
		};
		if (kLayout <= 0 || kLayout >= int(sizeof(kGrids) / sizeof(kGrids[0])))
			return {};
		return { kGrids[kLayout][0], kGrids[kLayout][1] };
	}
}
