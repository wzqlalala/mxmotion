#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 坐标轴刻度引擎（"坐标轴刻度设置"的渲染接线）
//
// 主轴/原生副轴的刻度由 Qwt 轴引擎 divideScale 生成（fit/缩放/平移每次重算
// 都会重入，入参携带当前范围），参数栏"刻度生成方式"经 applyAxis 安装的
// 引擎生效（生成方式值=面板下拉索引：linear 0均匀/1固定步长；log/dB 0每十倍
// /1均匀/2固定步长。轴为数据预变换+线性轴，均匀/固定步长引擎与缩放方式无
// 关，log/dB 下即在刻度变换空间等分/取步长）：
//   - 均匀分步法：mCurveUniformTickEngine——范围按"刻度总数"等分，主刻度
//     数字个数恰为刻度总数（含两端）；缩放后引擎重入仍保持个数（数字随
//     范围不再取整，属该生成方式的语义）
//   - 固定步长法：PlotView 的 TecPlotLinearScaleEngine（setFixedStep，步长
//     取绝对对齐倍数）
//   - 每十倍分步法（log/dB 默认）：mCurveDecadeTickEngine——"每十倍刻度数"
//     把每个数量级（log10 宽 1.0 / dB10 宽 10 / dB20 宽 20，见 decadeWidthOf）
//     等分出主刻度，"每十倍网格线数"把每个数量级细分出次刻度/次网格
// 自绘副轴条带（mCurveAxisStripWidget）不经 Qwt 轴引擎，直接调
// mCurveUniformTickDiv / mCurveDecadeTickDiv 保持同一套语义。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT/Qwt
#include <qwt_scale_engine.h>
#include <qwt_scale_div.h>
#include <qwt_interval.h>
#include <QList>
#include <QVector>
#include <QtGlobal>
#include <cmath>
#include <algorithm>
#include <utility>

namespace PostCurve
{
	//均匀分步法刻度划分：[x1,x2] 等分 tickCount-1 段，返回恰好 tickCount 个
	//主刻度（范围非法时退化为 Qwt 自动划分、tickCount<=1 走自动档）；每主刻度段细分
	//出 minorCount 个次刻度（次网格线即画在这些位置；minorCount<=0 时回落
	//Qwt 自动份数 maxMinSteps，观感对齐自动划分）
	inline QwtScaleDiv mCurveUniformTickDiv(double x1, double x2,
		double tickCount, int minorCount, int maxMinSteps)
	{
		if (!(x2 > x1) || qIsNaN(x1) || qIsNaN(x2))
			return QwtLinearScaleEngine().divideScale(x1, x2, 8, maxMinSteps, 0.0);
		//自动档（tickCount<=1，NVH 2D 线图族 X 轴随窗宽自适应，2026-11-16 裁定③）：
		//交 Qwt 自动 nice 步长划分，主刻度上限放宽 12（旧统一 8 档偏疏，原型约
		//10~13 个标签——nvh-log-scale-prototype-align）
		const int n = static_cast<int>(std::llround(tickCount));
		if (n < 2)
			return QwtLinearScaleEngine().divideScale(x1, x2, 12, maxMinSteps, 0.0);

		QList<double> ticks[QwtScaleDiv::NTickTypes];
		const double step = (x2 - x1) / (n - 1);
		for (int k = 0; k < n; ++k)
			ticks[QwtScaleDiv::MajorTick] += x1 + k * step;
		const int minors = minorCount > 0 ? qBound(1, minorCount, 1000) : maxMinSteps;
		for (int i = 0; i + 1 < n; ++i)
			for (int k = 1; k <= minors; ++k)
				ticks[QwtScaleDiv::MinorTick] += x1 + (i + k / (minors + 1.0)) * step;
		return QwtScaleDiv(QwtInterval(x1, x2), ticks);
	}

	//Qwt 轴引擎包装（mCurveChartView::applyAxis 按刻度生成方式安装；
	//QwtPlot 接管引擎所有权）
	class mCurveUniformTickEngine : public QwtLinearScaleEngine
	{
	public:
		mCurveUniformTickEngine(double tickCount, int minorCount)
			: _tickCount(tickCount), _minorCount(minorCount) {}

		QwtScaleDiv divideScale(double x1, double x2,
			int maxMajSteps, int maxMinSteps, double stepSize) const override
		{
			Q_UNUSED(maxMajSteps)
			Q_UNUSED(stepSize)
			return mCurveUniformTickDiv(x1, x2, _tickCount, _minorCount, maxMinSteps);
		}

	private:
		double _tickCount;   //刻度总数（主刻度数字个数，含两端）
		int _minorCount;     //每主刻度网格数（<=0=自动）
	};

	//列表标注法刻度划分（linear 轴 tickGenMode 2；2026-11-17 第六批③/C2 裁定：
	//贡献族 X 轴按频率点列表标注主刻度——主刻度=值列表界内成员，升序近重复
	//去重（相对容差 1e-9），无次刻度；值列表空/界内无成员/范围非法时回落
	//Qwt 自动划分（与均匀档回落口径一致）
	inline QwtScaleDiv mCurveListTickDiv(double x1, double x2,
		const QVector<double>& tickValues, int maxMinSteps)
	{
		if (!(x2 > x1) || qIsNaN(x1) || qIsNaN(x2) || tickValues.isEmpty())
			return QwtLinearScaleEngine().divideScale(x1, x2, 8, maxMinSteps, 0.0);
		QVector<double> sorted = tickValues;
		std::sort(sorted.begin(), sorted.end());
		QList<double> majors;
		double lastKept = 0.0;
		for (double v : sorted) {
			if (!qIsFinite(v))
				continue;
			if (!majors.isEmpty() && qAbs(v - lastKept) <= 1e-9 * qMax(1.0, qAbs(lastKept)))
				continue;   //近重复去重（相对容差，浮点频率列表防噪）
			lastKept = v;
			if (v >= x1 && v <= x2)
				majors += v;
		}
		if (majors.isEmpty())
			return QwtLinearScaleEngine().divideScale(x1, x2, 8, maxMinSteps, 0.0);
		QList<double> ticks[QwtScaleDiv::NTickTypes];
		ticks[QwtScaleDiv::MajorTick] = majors;
		return QwtScaleDiv(QwtInterval(x1, x2), ticks);
	}

	//Qwt 轴引擎包装（列表标注法；QwtPlot 接管所有权）
	class mCurveListTickEngine : public QwtLinearScaleEngine
	{
	public:
		explicit mCurveListTickEngine(QVector<double> tickValues)
			: _tickValues(std::move(tickValues)) {}

		QwtScaleDiv divideScale(double x1, double x2,
			int maxMajSteps, int maxMinSteps, double stepSize) const override
		{
			Q_UNUSED(maxMajSteps)
			Q_UNUSED(stepSize)
			return mCurveListTickDiv(x1, x2, _tickValues, maxMinSteps);
		}

	private:
		QVector<double> _tickValues;   //频率点列表（界内成员落主刻度）
	};

	//每十倍宽度：刻度变换空间中 HyperGraph 一个"十倍段"的宽度（对齐
	//HyperGraph2024 视觉：dB10 轴 0~20、每十倍刻度数 10 → 刻度步长 1）。
	//轴采用"数据预变换+线性轴"，log10 轴一个数量级宽 1.0；dB10/dB20 分别
	//按其名义系数取 10/20
	inline double decadeWidthOf(int scaleMode)
	{
		switch (scaleMode)
		{
		case 1:  return 1.0;    //log10
		case 2:  return 20.0;   //dB20
		case 3:  return 10.0;   //dB10
		default: return 1.0;
		}
	}

	//每十倍刻度划分（log/dB 轴，HyperGraph2024 语义）：以"每十倍刻度数"把
	//每个数量级等分出主刻度（对齐数量级边界，缩放/平移后重入保持每十倍
	//个数）；以"每十倍网格线数"把每个数量级细分出次刻度（次网格线即画在
	//这些位置）。刻度数<1 且网格数<=0 时退化为 Qwt 自动划分
	inline QwtScaleDiv mCurveDecadeTickDiv(double x1, double x2,
		double decadeWidth, int decadeTickCount, int decadeMinorCount, int maxMinSteps)
	{
		const bool autoMajors = decadeTickCount < 1;
		const bool autoMinors = decadeMinorCount <= 0;
		if ((autoMajors && autoMinors) || !(x2 > x1) || qIsNaN(x1) || qIsNaN(x2))
			return QwtLinearScaleEngine().divideScale(x1, x2, 8, maxMinSteps, 0.0);

		const double w = (qIsFinite(decadeWidth) && decadeWidth > 0) ? decadeWidth : 1.0;
		QList<double> ticks[QwtScaleDiv::NTickTypes];

		//防御：步长与区间宽度失配时（如对数预变换轴的区间仍是原始值空间，
		//见 applyAxis 轴范围对数化之前的中间态），按 w/decadeTickCount 步长
		//会生成数十万级刻度（轴控件绘制百万刻度卡死）。超限回落 Qwt 自动
		//划分（均匀刻度，数量受 maxMajSteps 约束）
		{
			const int majorsGuard = autoMajors ? 0 : qBound(1, decadeTickCount, 100);
			const double stepGuard = w / majorsGuard;
			if ((x2 - x1) / stepGuard > 200.0)
				return QwtLinearScaleEngine().divideScale(x1, x2, 8, maxMinSteps, 0.0);
		}

		//主刻度：每数量级等分 decadeTickCount 个（步长=w/个数，绝对对齐数量
		//级边界；两端向外多扫一个整步防浮点漏刻）
		if (!autoMajors)
		{
			const int majors = qBound(1, decadeTickCount, 100);
			const double step = w / majors;
			const int kFirst = static_cast<int>(std::floor(x1 / step - 1e-9));
			const int kLast = static_cast<int>(std::ceil(x2 / step + 1e-9));
			for (int k = kFirst; k <= kLast; ++k)
			{
				const double t = k * step;
				if (t >= x1 && t <= x2)
					ticks[QwtScaleDiv::MajorTick] += t;
			}
		}
		else
		{
			//主刻度交 Qwt 自动，次刻度仍按每十倍细分
			const QwtScaleDiv autoDiv = QwtLinearScaleEngine().divideScale(x1, x2, 8, 0, 0.0);
			ticks[QwtScaleDiv::MajorTick] = autoDiv.ticks(QwtScaleDiv::MajorTick);
		}

		//次刻度（=每十倍网格线数，画布网格线即画在这些位置）：每个数量级段
		//内细分 decadeMinorCount 条（段宽=w/count，最后一条落在下一数量级边
		//界上，与 HyperGraph 一致——10 条/十倍即逐 1/10 段均布）；自动时按
		//Qwt 自动份数细分每个主刻度段
		const QList<double> majors = ticks[QwtScaleDiv::MajorTick];
		if (!majors.isEmpty())
		{
			if (!autoMinors)
			{
				const int minors = qBound(1, decadeMinorCount, 1000);
				const double sub = w / minors;
				const int dFirst = static_cast<int>(std::floor(x1 / w - 1e-9));
				const int dLast = static_cast<int>(std::ceil(x2 / w + 1e-9));
				for (int d = dFirst; d <= dLast; ++d)
					for (int k = 1; k <= minors; ++k)
					{
						const double t = d * w + k * sub;
						if (t > x1 && t <= x2)
							ticks[QwtScaleDiv::MinorTick] += t;
					}
			}
			else
			{
				const int minors = qBound(1, maxMinSteps, 100);
				for (int i = 0; i + 1 < majors.size(); ++i)
					for (int k = 1; k <= minors; ++k)
					{
						const double t = majors.at(i)
							+ (majors.at(i + 1) - majors.at(i)) * k / (minors + 1.0);
						ticks[QwtScaleDiv::MinorTick] += t;
					}
			}
		}
		return QwtScaleDiv(QwtInterval(x1, x2), ticks);
	}

	//Qwt 轴引擎包装（log/dB 轴的每十倍刻度/网格；QwtPlot 接管所有权）
	class mCurveDecadeTickEngine : public QwtLinearScaleEngine
	{
	public:
		mCurveDecadeTickEngine(double decadeWidth, int decadeTickCount, int decadeMinorCount)
			: _decadeWidth(decadeWidth), _decadeTickCount(decadeTickCount)
			, _decadeMinorCount(decadeMinorCount) {}

		QwtScaleDiv divideScale(double x1, double x2,
			int maxMajSteps, int maxMinSteps, double stepSize) const override
		{
			Q_UNUSED(maxMajSteps)
			Q_UNUSED(stepSize)
			return mCurveDecadeTickDiv(x1, x2, _decadeWidth,
				_decadeTickCount, _decadeMinorCount, maxMinSteps);
		}

	private:
		double _decadeWidth;      //一个"十倍段"在变换空间的宽度（log=1/dB10=10/dB20=20）
		int _decadeTickCount;     //每十倍刻度数（<1=主刻度自动）
		int _decadeMinorCount;    //每十倍网格线数（<=0=次刻度随主刻度段自动细分）
	};
}
