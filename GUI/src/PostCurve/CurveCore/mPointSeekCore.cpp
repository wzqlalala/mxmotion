#include "mPointSeekCore.h"

//QT
#include <QVector>
//STD
#include <algorithm>
#include <cmath>
#include <limits>

namespace PostCurve
{
	int mPointSeekCore::pointCount(const QVector<double>& xs, const QVector<double>& ys,
		const QVector<double>& zs)
	{
		//三列等长口径的最小值：任一列缺失按最短列计（调用方模型数据三列恒
		//等长，此处防御不齐数据不越界）
		return qMin(xs.size(), qMin(ys.size(), zs.size()));
	}

	bool mPointSeekCore::isValidPoint(int i, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//下标有效且三坐标均 finite（NaN/±inf 任一即 NaN 断口点，不可寻——
		//对齐 R40/修订四非有限值口径）
		if (i < 0 || i >= pointCount(xs, ys, zs))
			return false;
		return qIsFinite(xs.at(i)) && qIsFinite(ys.at(i)) && qIsFinite(zs.at(i));
	}

	double mPointSeekCore::coordinateAt(int column, int i, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		if (i < 0 || i >= pointCount(xs, ys, zs))
			return std::numeric_limits<double>::quiet_NaN();
		switch (column)
		{
		case ColX:
			return xs.at(i);
		case ColY:
			return ys.at(i);
		case ColValue:
		default:   //column 越界按极值列兜底（防御）
			return zs.at(i);
		}
	}

	QVector<int> mPointSeekCore::validIndexOrder(const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//有效点下标升序表（NaN 断口点不入序——步进不可停驻）；seekOrder 全点
		//序与 X 列曲线类型步进序共用（T47-3）
		QVector<int> order;
		const int n = pointCount(xs, ys, zs);
		order.reserve(n);
		for (int i = 0; i < n; ++i)
		{
			if (isValidPoint(i, xs, ys, zs))
				order.append(i);
		}
		return order;
	}

	QVector<int> mPointSeekCore::seekOrder(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//有效点下标集合（NaN 断口点不入序——步进不可停驻）
		QVector<int> order = validIndexOrder(xs, ys, zs);
		//X/Y 列：该坐标升序**稳定**排序（std::stable_sort 等值保原下标序，
		//对齐 HG"等值保序号序"口径）；极值列=下标序不排序
		if (column == ColX || column == ColY)
		{
			std::stable_sort(order.begin(), order.end(),
				[column, &xs, &ys, &zs](int a, int b)
			{
				return coordinateAt(column, a, xs, ys, zs)
					< coordinateAt(column, b, xs, ys, zs);
			});
		}
		return order;
	}

	int mPointSeekCore::globalMaxIndex(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//该坐标在有效点中的最大点（并列取首个=下标最小；NaN 断口点不入候选
		//——落点必须可寻/可渲染）
		const int n = pointCount(xs, ys, zs);
		int best = -1;
		double bestV = 0.0;
		for (int i = 0; i < n; ++i)
		{
			if (!isValidPoint(i, xs, ys, zs))
				continue;
			const double v = coordinateAt(column, i, xs, ys, zs);
			if (best < 0 || v > bestV)
			{
				best = i;
				bestV = v;
			}
		}
		return best;
	}

	int mPointSeekCore::globalMinIndex(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//同 globalMaxIndex 口径取最小
		const int n = pointCount(xs, ys, zs);
		int best = -1;
		double bestV = 0.0;
		for (int i = 0; i < n; ++i)
		{
			if (!isValidPoint(i, xs, ys, zs))
				continue;
			const double v = coordinateAt(column, i, xs, ys, zs);
			if (best < 0 || v < bestV)
			{
				best = i;
				bestV = v;
			}
		}
		return best;
	}

	QVector<int> mPointSeekCore::localPeakIndices(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//局部峰：沿下标序扫描，峰=该坐标严格大于两邻（端点单侧判定；邻点该
		//坐标非 finite 按该侧不存在处理；平台值不判峰——严格大于口径）。候选
		//点恒为有效点（NaN 断口点不判峰谷）
		QVector<int> peaks;
		const int n = pointCount(xs, ys, zs);
		for (int i = 0; i < n; ++i)
		{
			if (!isValidPoint(i, xs, ys, zs))
				continue;
			const double v = coordinateAt(column, i, xs, ys, zs);
			bool isPeak = true;
			//左邻（i-1）：存在且该坐标 finite 才参与比较，否则该侧不存在
			if (i - 1 >= 0)
			{
				const double lv = coordinateAt(column, i - 1, xs, ys, zs);
				if (qIsFinite(lv) && !(v > lv))
					isPeak = false;
			}
			//右邻（i+1）：同口径
			if (isPeak && i + 1 < n)
			{
				const double rv = coordinateAt(column, i + 1, xs, ys, zs);
				if (qIsFinite(rv) && !(v > rv))
					isPeak = false;
			}
			if (isPeak)
				peaks.append(i);
		}
		return peaks;
	}

	QVector<int> mPointSeekCore::localValleyIndices(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//局部谷：与 localPeakIndices 镜像（严格小于判定，其余口径逐条同）
		QVector<int> valleys;
		const int n = pointCount(xs, ys, zs);
		for (int i = 0; i < n; ++i)
		{
			if (!isValidPoint(i, xs, ys, zs))
				continue;
			const double v = coordinateAt(column, i, xs, ys, zs);
			bool isValley = true;
			if (i - 1 >= 0)
			{
				const double lv = coordinateAt(column, i - 1, xs, ys, zs);
				if (qIsFinite(lv) && !(v < lv))
					isValley = false;
			}
			if (isValley && i + 1 < n)
			{
				const double rv = coordinateAt(column, i + 1, xs, ys, zs);
				if (qIsFinite(rv) && !(v < rv))
					isValley = false;
			}
			if (isValley)
				valleys.append(i);
		}
		return valleys;
	}

	QVector<int> mPointSeekCore::extremumPointSet(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//极值点集 E=局部峰∪局部谷（T47-2 20261117 用户拍板 ◄► 切换极值；
		//判定口径与 localPeak/ValleyIndices 一致，两表各自按下标升序产出），
		//并集去重后按升序排列。单点曲线该点同判峰与谷（端点双侧皆不存在），
		//去重避免同下标重复入集
		QVector<int> set = localPeakIndices(column, xs, ys, zs);
		const QVector<int> valleys = localValleyIndices(column, xs, ys, zs);
		for (int v : valleys)
		{
			if (!set.contains(v))
				set.append(v);
		}
		std::sort(set.begin(), set.end());
		return set;
	}

	int mPointSeekCore::stepSeek(int plot3dType, int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs,
		int fromIndex, int dir)
	{
		//首行 ◄► 步进（T47-2/T47-3 20261117 用户对齐修正：极值列=**极值点集
		//切换**——在极值点集 E（局部峰∪谷，下标升序）上移动；X 列=曲线类型
		//按序号步进/其余类型坐标升序；Y 列口径不变）
		if (column == ColValue)
		{
			//◄=E 中小于当前下标的最大者、►=E 中大于当前下标的最小者（当前
			//点∈E 即取前后邻居）；方向无后继止于端点不环绕（原位不动）；
			//NaN 断口点不在 E 内（不可寻不可停驻口径不变）
			const QVector<int> extrema = extremumPointSet(ColValue, xs, ys, zs);
			if (extrema.isEmpty())
				return -1;   //无极值点（空数据/全 NaN/全平台）→ 无点可寻
			if (!isValidPoint(fromIndex, xs, ys, zs))
			{
				//当前下标无效（未寻点/越界/NaN 断口）：向右=取 E 首个、向左=取 E 末个
				return dir >= 0 ? extrema.first() : extrema.last();
			}
			if (dir >= 0)
			{
				for (int e : extrema)
				{
					if (e > fromIndex)
						return e;
				}
				return fromIndex;   //后方无极值：止于端点不环绕
			}
			for (int k = extrema.size() - 1; k >= 0; --k)
			{
				const int e = extrema.at(k);
				if (e < fromIndex)
					return e;
			}
			return fromIndex;   //前方无极值：止于端点不环绕
		}
		//X/Y 列步进序（T47-3 20261117 用户拍板：曲线类型 plot3dType==1 的
		//X 列 ◄►=按序号[有效点下标升序]步进，跳过 NaN 断口点；瀑布/曲面/
		//柱状与其余列维持该坐标升序稳定排序）
		const QVector<int> order = (plot3dType == 1 && column == ColX)
			? validIndexOrder(xs, ys, zs)
			: seekOrder(column, xs, ys, zs);
		if (order.isEmpty())
			return -1;
		const int pos = order.indexOf(fromIndex);
		if (pos < 0)
		{
			//当前点不在序内（未寻点/该点已变 NaN 断口）：向右=落序首、向左=落序尾
			return dir >= 0 ? order.first() : order.last();
		}
		const int next = pos + (dir >= 0 ? 1 : -1);
		if (next < 0 || next >= order.size())
			return fromIndex;   //止于端点不环绕（原位不动）
		return order.at(next);
	}

	int mPointSeekCore::nearestExtremum(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs,
		int fromIndex, bool peak, int dir)
	{
		//◄∩/►∩/◄U/►U：沿下标序自 fromIndex 前/后最近一个局部峰/谷。
		//候选表按下标升序，方向扫描线性复杂度
		const QVector<int> extrema = peak
			? localPeakIndices(column, xs, ys, zs)
			: localValleyIndices(column, xs, ys, zs);
		if (extrema.isEmpty())
			return -1;
		if (dir >= 0)
		{
			//向后：第一个下标严格大于 fromIndex 的峰/谷（fromIndex<0=取首个）
			for (int e : extrema)
			{
				if (fromIndex < 0 || e > fromIndex)
					return e;
			}
			return -1;   //后方无峰/谷：原地不动不环绕
		}
		//向前：最后一个下标严格小于 fromIndex 的峰/谷（fromIndex<0=取末个）
		for (int k = extrema.size() - 1; k >= 0; --k)
		{
			const int e = extrema.at(k);
			if (fromIndex < 0 || e < fromIndex)
				return e;
		}
		return -1;
	}

	bool mPointSeekCore::coordinateVaries(int column, const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//列可用性：点数<2 → false；该坐标全部 finite 值不足两个或极差==0
		//（全程恒值，如用户截图曲线 y=-0.8 恒值 → Y 列整列灰）→ false
		const int n = pointCount(xs, ys, zs);
		if (n < 2)
			return false;
		double lo = 0.0, hi = 0.0;
		bool any = false;
		for (int i = 0; i < n; ++i)
		{
			const double v = coordinateAt(column, i, xs, ys, zs);
			if (!qIsFinite(v))
				continue;
			if (!any)
			{
				any = true;
				lo = hi = v;
				continue;
			}
			lo = qMin(lo, v);
			hi = qMax(hi, v);
		}
		if (!any)
			return false;
		//恒值判定=极差恰为 0（double 直接比较：全部相等值极差恒精确 0，
		//无需容差）
		return (hi - lo) != 0.0;
	}

	bool mPointSeekCore::columnEnabled(int plot3dType, int column,
		const QVector<double>& xs, const QVector<double>& ys, const QVector<double>& zs)
	{
		//三列启停统一口径（20261117 用户拍板：曲线类型 Y 列固定禁用，替代
		//恒值推断口径——设计稿与截图中的曲线 Y 列灰即此口径）：plot3dType==1
		//（三维曲线）→ Y 轴列固定整列禁用（首行 ◄► 与 ∩/U 组一并灰，即便
		//该列数值全程变化）；plot3dType 0/2/3（瀑布/曲面/柱状）与其余列
		//（极值/X轴）维持恒值兜底判定
		if (plot3dType == 1 && column == ColY)
			return false;
		return coordinateVaries(column, xs, ys, zs);
	}

	QString mPointSeekCore::exportTableCsv(const QVector<double>& xs,
		const QVector<double>& ys, const QVector<double>& zs)
	{
		//导出表格 CSV 文本装配（T47-4 20261117 用户拍板）：表头行+全部点按
		//下标序（坐标数据表行序——refreshTable 同款最短列防御），三列 'f',4
		//（与表格显示一致；无序号列）；非有限值导空串（定义曲线弹窗数值表
		//非有限行空串口径）；逗号分隔+CRLF 行尾（CSV 惯例/mDynStiffnessMapping
		//先例同款换行）；BOM 由写文件侧统一加（见头注释）
		QString csv = QStringLiteral("横坐标,纵坐标,高度\r\n");
		const int n = pointCount(xs, ys, zs);
		for (int i = 0; i < n; ++i)
		{
			const double x = xs.at(i), y = ys.at(i), z = zs.at(i);
			csv += QStringLiteral("%1,%2,%3\r\n")
				.arg(qIsFinite(x) ? QString::number(x, 'f', 4) : QString(),
					qIsFinite(y) ? QString::number(y, 'f', 4) : QString(),
					qIsFinite(z) ? QString::number(z, 'f', 4) : QString());
		}
		return csv;
	}
}
