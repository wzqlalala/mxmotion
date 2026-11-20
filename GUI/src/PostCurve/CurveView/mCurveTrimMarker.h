#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 裁剪线图元（"编辑曲线"会话的范围界线，UI设计图16）
//
// 一条贯穿画布的黑色界线 + 线中部朝范围内侧的实心三角把手（起始线三角
// 朝内侧、终止线三角朝外侧反向），随拖拽/步进移动，拖拽中加粗反馈。
// 直接 attach 到 QwtPlot，不进 PlotView 引擎注册表——不占图例、不参与
// 引擎曲线命中，生命周期由 mCurveChartView 编辑会话自管（策略同注释标记
// _annotationMarkers / 悬停预览 _hoverItem）。
// 命中与拖拽不在图元内处理：由 mCurveChartView 的事件过滤器按 ±5px
// 像素带判定（水平方向=竖直带，垂直方向=水平带）。
// =============================================================================

#include "../PostCurveGlobal.h"

//Qwt
#include <qwt_plot_item.h>

class QPainter;
class QwtScaleMap;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveTrimMarker : public QwtPlotItem
	{
	public:
		//界线方向：竖线（固定X值，"水平"裁剪方向用）/ 横线（固定Y值，"垂直"方向用）
		enum Orientation
		{
			Vertical = 0,
			Horizontal = 1,
		};

		//界线角色：起始（范围下界，三角朝内侧）/ 终止（范围上界）
		enum Role
		{
			Start = 0,
			End = 1,
		};

		explicit mCurveTrimMarker(Role role);

		void setOrientation(Orientation orient);
		Orientation orientation() const { return _orientation; }

		//界线固定值（Vertical 为 X 值 / Horizontal 为 Y 值，渲染空间）
		void setValue(double value);
		double value() const { return _value; }

		void setRole(Role role);
		Role role() const { return _role; }

		//拖拽/悬停反馈：线宽 2px -> 3px（不主动重绘，由调用方 replot）
		void setHighlighted(bool on);
		bool isHighlighted() const { return _highlighted; }

		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		Role _role;
		Orientation _orientation = Vertical;
		double _value = 0.0;
		bool _highlighted = false;
	};
}
