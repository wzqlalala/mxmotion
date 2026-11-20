#include "CurveBarExtract.h"

#include <algorithm>
#include <QtNumeric>

namespace CurveBarExtract
{
	namespace
	{
		//可见段线性插值：base 已按 x 升序；freq 越界取最近端点（防御，正常调用
		//恒在 [fmin, fmax] 内）；退化单点直接取值
		double interpolate(const QVector<QPair<double, double>>& base, double freq)
		{
			if (freq <= base.first().first)
				return base.first().second;
			if (freq >= base.last().first)
				return base.last().second;
			//首个 x >= freq 的基点；freq 与其前方点之间插值
			auto it = std::lower_bound(base.cbegin(), base.cend(), freq,
				[](const QPair<double, double>& p, double v) { return p.first < v; });
			const QPair<double, double>& hi = *it;
			const QPair<double, double>& lo = *(it - 1);
			const double span = hi.first - lo.first;
			if (span <= 0.0)
				return lo.second;
			const double t = (freq - lo.first) / span;
			return lo.second + (hi.second - lo.second) * t;
		}
	}

	QVector<ExtractedPoint> extractUniform(const PostCurve::CurveSeriesData& series, int count)
	{
		if (count < 2)
			return {};
		//可见基点收集（隐藏区间/非有限值剔除），按 x 升序整理
		const int n = qMin(series.xData.size(), series.yData.size());
		QVector<QPair<double, double>> base;
		base.reserve(n);
		for (int i = 0; i < n; ++i)
		{
			if (series.isPointHidden(i))
				continue;
			const double x = series.xData.at(i);
			const double y = series.yData.at(i);
			if (!qIsFinite(x) || !qIsFinite(y))
				continue;
			base.append(qMakePair(x, y));
		}
		if (base.size() < 2)
			return {};
		std::sort(base.begin(), base.end(),
			[](const QPair<double, double>& a, const QPair<double, double>& b)
			{ return a.first < b.first; });

		//退化：全部可见点同一频率（范围零宽），N 点全部落该频率取同值
		const double fMin = base.first().first;
		const double fMax = base.last().first;
		QVector<ExtractedPoint> pts;
		pts.reserve(count);
		if (fMax <= fMin)
		{
			for (int k = 0; k < count; ++k)
				pts.append({ fMin, base.first().second });
			return pts;
		}
		for (int k = 0; k < count; ++k)
		{
			//均匀含首尾：t∈[0,1] 等分，k=0/N-1 精确落在两端（避免浮点漂移）
			const double t = double(k) / double(count - 1);
			const double freq = fMin + (fMax - fMin) * t;
			pts.append({ freq, interpolate(base, freq) });
		}
		return pts;
	}

	QStringList categoryLabels(const QVector<ExtractedPoint>& pts)
	{
		QStringList labels;
		labels.reserve(pts.size());
		for (const ExtractedPoint& p : pts)
			labels.append(QString::number(p.freq));
		return labels;
	}
}
