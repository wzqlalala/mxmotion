#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 柱状图柱顶数值标注图元（QwtPlotItem）
//
// 柱状图（chartType=1）每根柱顶的数值标注（模态贡献量分析产出，设计图15）：
// 文本=该柱原始 Y 值（参与量量值），位置=柱顶（与引擎曲线同参变换后的数据
// 坐标，draw 时经轴映射取像素，随缩放/刻度切换联动），正值柱标在柱顶上方、
// 负值柱标在柱底下方。
//
// 纯视图元素：不进模型/命令栈，由 mCurveChartView 随全量重建重建
// （策略同基准线标签 _baselineLabels：直接 attach 画布，不进引擎注册表，
// 不占图例、不参与命中）。
// =============================================================================

#include "../PostCurveGlobal.h"

#include <qwt_plot_item.h>
#include <QColor>
#include <QString>
#include <QVector>

class QwtScaleMap;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveBarLabelItem : public QwtPlotItem
	{
	public:
		explicit mCurveBarLabelItem();

		//整批柱标注（三数组等长一一对应；texts 与 xs/ys 数目不一致时按较小者截齐）
		void setBarData(const QVector<double>& xs, const QVector<double>& ys,
			const QStringList& texts);
		//清空全部标注（空批=不绘制）
		void clearBars();
		//标注文字颜色（默认黑）
		void setLabelColor(const QColor& color);

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		QVector<double> m_xs;      //柱中心 X（数据坐标，与引擎曲线同参）
		QVector<double> m_ys;      //柱顶 Y（数据坐标，正值柱顶/负值柱底）
		QStringList m_texts;       //标注文本（原始量值）
		QColor m_color = Qt::black; //文字色
	};
}
