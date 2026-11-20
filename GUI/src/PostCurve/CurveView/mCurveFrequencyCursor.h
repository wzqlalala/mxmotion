#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 特定频率红线游标图元（QwtPlotItem）
//
// NVH 模态贡献量分析三联动（design D4）：响应曲线/柱状图等本功能产出视口的
// "当前频率"红竖线，跨视口保持同一频率状态。纯视图态：不进文档/命令栈，
// 由 mCurveChartView 经 setFrequencyCursor 下发（值=数据坐标，draw 时经轴
// 映射取像素，缩放/刻度切换随映射保持），rebuild 尾部重放（复数图上下面板
// 各一条，下面板随重建销毁重建重挂）。
// =============================================================================

#include "../PostCurveGlobal.h"

#include <qwt_plot_item.h>

class QwtScaleMap;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveFrequencyCursor : public QwtPlotItem
	{
	public:
		explicit mCurveFrequencyCursor();

		//游标 X（数据坐标）
		void setXValue(double x);
		double xValue() const { return m_x; }

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		double m_x = 0.0;   //游标 X（数据坐标）
	};
}
