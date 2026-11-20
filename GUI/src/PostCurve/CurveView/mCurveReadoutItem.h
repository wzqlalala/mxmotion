#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 悬停读数标签（QwtPlotItem）
//
// 基准线模式内悬停曲线时的探针读数：吸附点空心
// 主题蓝圆圈 + 单行"y= …  @ x= …"白底黑边黑字文本框（样式同引擎 mPlotSnapper
// 悬停标签），默认摆在锚点右下方避让光标，越界自动翻向/收回画布内；
// Z 取次顶（十字光标之下、曲线/注释/基准线之上）。
// 纯视图临时元素：不进模型/命令栈，由交互层（mCurveChartView）直接显隐。
// =============================================================================

#include "../PostCurveGlobal.h"

#include <qwt_plot_item.h>
#include <QString>
#include <QPoint>

class QwtPlot;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveReadoutItem : public QwtPlotItem
	{
	public:
		explicit mCurveReadoutItem();
		~mCurveReadoutItem() override = default;

		//设置读数文本与锚点（画布像素）并显示；text 可多行（'\n' 分行）
		void showAt(const QPoint& canvasPos, const QString& text);
		//隐藏并复位（不在本类内 replot，由调用方统一触发）
		void hideReadout();

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		QPoint m_pos{ -1, -1 };   //锚点（画布像素；负值=无效不绘制）
		QString m_text;           //读数文本（多行）
	};
}
