#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 极坐标底图图元（QwtPlotItem，design D5）
//
// chartType=3 极坐标画布接管：外圆+半径同心圆网格+角度放射线+角度/半径刻度
// 文字；半径刻度值经现有缩放语义（Linear/Log/dB10/dB20+计权，刻度序列由
// mCurveAxisTickEngine 族在变换空间生成，视图侧换算后下发）映射到 [0,R]
// 像素；角度按均匀分步（X1 轴刻度总数直接生效，如 8/12 条放射线）。
// Y1 轴标题文本与半径刻度标注沿 0° 方向图内绘制（标题位于刻度标注下方、
// 向右延伸，轴 kText 属性驱动）。
//
// 纯视图元素：不进模型/命令栈，由 mCurveChartView 随全量重建重建（策略同
// 基准线标签/柱顶标注：直接 attach 画布，不进引擎注册表，不占图例、不参与
// 命中）。
//
// 几何契约（FrameItem/SeriesItem/游标共用）：画布内切圆（留白给角度标注带），
// 屏幕坐标 y 向下，极角数学正向（0° 沿 +x 逆时针），像素位=中心+R·r̂·(cosθ,
// -sinθ)。半径缩放契约 PolarRadiusScale：计权先加权后变换（weightGain），
// 变换空间域 [domainMin,domainMax] 归一到 [0,1]，越界钳制。
// =============================================================================

#include "../PostCurveGlobal.h"

#include "../CurveCore/mCurveWeighting.h"   //weightGain（计权先加权后变换）

#include <qwt_plot_item.h>
#include <QColor>
#include <QFont>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QVector>
#include <cmath>

class QwtScaleMap;

namespace PostCurve
{
	//MSVC <cmath> 不带 _USE_MATH_DEFINES 不发 M_PI：极坐标图元族共用圆周率
	inline constexpr double kPolarPi = 3.14159265358979323846;

	//极坐标画布几何（FrameItem/SeriesItem/命中查询共用）：内切圆留白给角度
	//标注带，半径取宽高较小者的一半（圆不随画布拉伸变椭圆）
	struct PolarCanvasGeometry
	{
		QPointF center;
		double radius = 0.0;
	};

	inline PolarCanvasGeometry polarCanvasGeometry(const QRectF& canvasRect)
	{
		const qreal margin = 30.0;   //外圆距画布边缘留白（角度标注带）
		PolarCanvasGeometry g;
		g.center = canvasRect.center();
		g.radius = qMax(10.0, qMin(canvasRect.width(), canvasRect.height()) / 2.0 - margin);
		return g;
	}

	//极坐标像素位（θ 数学正向，屏幕 y 翻转）
	inline QPointF polarPixel(const PolarCanvasGeometry& g, double normRadius, double phaseDeg)
	{
		const qreal rad = phaseDeg * kPolarPi / 180.0;
		return QPointF(g.center.x() + g.radius * normRadius * std::cos(rad),
			g.center.y() - g.radius * normRadius * std::sin(rad));
	}

	//半径缩放（Y1 轴节点口径，design D5）：计权先加权后变换；变换空间域
	//[domainMin,domainMax] 归一到 [0,1]（Linear 域=原始值空间，Log/dB 域=
	//变换空间刻度值，与"数据预变换+线性轴"架构同语义）
	struct PolarRadiusScale
	{
		int scaleMode = 0;      //0 Linear/1 Log/2 dB20/3 dB10（CurveAxisData 口径）
		int weightMode = 0;     //0无/1A/2B/3C/4U（U 与无等效）
		double dbRef = 1.0;
		double freqHz = 0.0;    //计权频率（向量各自携带，weightGain 入参）
		double domainMin = 0.0;
		double domainMax = 1.0;

		//变换空间值（mCurveChartView::transformValue 同式：Log/dB 档 v<=0
		//非法返 NaN，Linear 档变换=原值、0/负合法——手动域含 0（M8 线性
		//默认域 [0,10]）依赖此路径，2026-11-12 D25 复核勘正旧全档拒 v<=0）
		double transformed(double magnitude) const
		{
			const double ref = (qIsFinite(dbRef) && dbRef > 0.0) ? dbRef : 1.0;
			const double gain = weightGain(weightMode, freqHz);
			const double v = magnitude * gain;
			const double nan = std::numeric_limits<double>::quiet_NaN();
			if (!qIsFinite(v)
				|| ((scaleMode == 1 || scaleMode == 2 || scaleMode == 3) && !(v > 0.0)))
				return nan;
			switch (scaleMode)
			{
			case 1:  return std::log10(v);
			case 2:  return 20.0 * std::log10(v / ref);
			case 3:  return 10.0 * std::log10(v / ref);
			default: return v;   //Linear：变换=原值（计权仍生效）
			}
		}

		//归一化半径（D35 勘误，2026-11-16 商软缩放五样张实证：超域向量不钳
		//满径——域 8.0E-05 < Response 8.01e-05 时箭头穿出最外环 ~5% 半径、深档
		//直线冲出画布缘由视口自然裁剪，旧 qBound(0,r,1) 满径系 D23 自研口径
		//作废；下钳 0 保留：域恒 0 起无负半径，NaN/退化域极点兜底不变）
		double normalizedRadius(double magnitude) const
		{
			const double t = transformed(magnitude);
			if (!qIsFinite(t))
				return 0.0;
			if (!(domainMax > domainMin))
				return 0.0;
			return qMax(0.0, (t - domainMin) / (domainMax - domainMin));
		}
	};

	class POSTCURVE_EXPORT mCurvePolarFrameItem : public QwtPlotItem
	{
	public:
		explicit mCurvePolarFrameItem();

		//半径刻度（归一化半径 [0,1]，视图侧经 PolarRadiusScale 归一后下发；
		//majors=主刻度圆+数字，minors=次网格细圆无数字）
		void setRadiusTicks(const QVector<double>& majorValues,
			const QStringList& majorLabels, const QVector<double>& minorValues);
		//角度放射线数（X1 刻度总数直接生效，如 8/12；<3 钳 3，D25/L13）
		void setAngleTickCount(int count);
		//Y1 轴标题（沿 0° 方向图内近极点、刻度标注下方、向右延伸）
		void setTitle(const QString& title);
		void setTitleFont(const QFont& font);
		void setTitleColor(const QColor& color);
		//刻度数字字体/网格色（网格色随图表 gridColor，同直角轴 applyAxis 口径）
		void setLabelFont(const QFont& font);
		void setGridColor(const QColor& color);
		//角度标签独立通道（D25/M6）：字体/颜色/文本由 X1 面板下发（文本视图侧
		//按 X1 格式/精度预构建，个数与放射线数不符回退整数度）——撤 Y1
		//labelFont 劫持与黑硬编码
		void setAngleLabelFont(const QFont& font);
		void setAngleLabelColor(const QColor& color);
		void setAngleLabels(const QStringList& labels);
		//轴可见性（D25/M7）：Y1 kVisible=环网+径向数字+标题，X1 kVisible=辐条
		//+角度度数（外圆恒画，框定画布）
		void setRadialVisible(bool on);
		void setAngularVisible(bool on);

	protected:
		void draw(QPainter* painter,
			const QwtScaleMap& xMap,
			const QwtScaleMap& yMap,
			const QRectF& canvasRect) const override;

	private:
		QVector<double> m_majorTicks;   //主刻度圆（归一化半径 0..1）
		QStringList m_majorLabels;      //主刻度数字（与 majorTicks 一一对应）
		QVector<double> m_minorTicks;   //次网格细圆（归一化半径 0..1）
		int m_angleCount = 8;           //放射线数（含 0°，沿圆均布）
		QString m_title;                //Y1 轴标题（空不绘制）
		QFont m_titleFont;
		QColor m_titleColor = Qt::black;
		QFont m_labelFont;              //刻度数字字体（族名空=默认；径向数字，Y1 labelFont）
		QColor m_gridColor = QColor(220, 220, 220);
		QFont m_angleLabelFont;         //角度度数字体（族名空=默认；X1 labelFont）
		QColor m_angleLabelColor = Qt::black;   //角度度数颜色（X1 titleColor，默认黑）
		QStringList m_angleLabels;      //角度度数文本（视图侧预构建；空=整数度回退）
		bool m_radialVisible = true;    //Y1 轴可见（环网+径向数字+标题）
		bool m_angularVisible = true;   //X1 轴可见（辐条+角度度数）
	};
}
