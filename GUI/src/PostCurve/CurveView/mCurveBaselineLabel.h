#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 基准线标签图元（QwtPlotItem）
//
// 基准线（引擎 mPlotMarkerLine）的线上文字（设计图09/14）：文本=基准线
// "标签"（创建时初值=基准线名称，清空即不显示），字体与 9 宫格标签位置
// 随属性页"显示"组的字体设置（kLabelFont）/标签位置（kLabelPos），
// 文字颜色跟随基准线线色（显示组未提供文字色，与线同色区分归属）。
//
// 定位语义（9宫格 idx：0左上 1中上 2右上 3左中 4居中 5右中 6左下 7中下 8右下）：
//   纵向线（固定X值）：上/中/下=沿画布顶/垂直居中/底摆，左/中/右=文字在线
//   左侧/骑线/线右侧；横向线（固定Y值）：左/中/右=沿画布左/水平居中/右摆，
//   上/中/下=文字在线上方/骑线/线下方。
// 线值越出画布不绘制（与线本身越界不画一致）；文字贴边时收进画布内。
//
// 纯视图元素：不进模型/命令栈，由 mCurveChartView 随基准线增量维护
// （策略同注释标记 _annotationMarkers：直接 attach 画布，不进引擎注册表，
// 不占图例、不参与命中）。
// =============================================================================

#include "../PostCurveGlobal.h"

#include <qwt_plot_item.h>
#include <QColor>
#include <QFont>
#include <QString>

class QwtScaleMap;

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveBaselineLabel : public QwtPlotItem
	{
	public:
		explicit mCurveBaselineLabel();

		//标签文本（空=不绘制）
		void setLabelText(const QString& text);
		//标签字体（族名为空=出厂"默认"，按应用默认字体渲染）
		void setLabelFont(const QFont& font);
		//标签颜色（跟随基准线线色）
		void setLabelColor(const QColor& color);
		//标签位置：0左上 1中上 2右上 3左中 4居中 5右中 6左下 7中下 8右下（越界钳制）
		void setLabelPos(int pos);
		//所属线方向：true 横向（固定Y值）/ false 纵向（固定X值）
		//（同 CurveBaselineData：orientation==0 为横向）
		void setLineHorizontal(bool horizontal);
		//所属线固定值（数据坐标：横向=Y值/纵向=X值；与线同值同映射保证贴线）
		void setLineValue(double value);

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		QString m_text;            //标签文本（空=不绘制）
		QFont m_font;              //标签字体（族名空=默认）
		QColor m_color = Qt::black; //文字色（跟随线色）
		int m_labelPos = 4;        //标签位置 9宫格，默认居中（骑线）
		bool m_horizontal = false; //所属线方向（false=纵向）
		double m_value = 0.0;      //所属线固定值（数据坐标）
	};
}
