#include "mCurvePolarFrameItem.h"

#include <QFontMetrics>
#include <QPainter>
#include <qwt_scale_map.h>
#include <qwt_text.h>

namespace PostCurve
{
	mCurvePolarFrameItem::mCurvePolarFrameItem()
		: QwtPlotItem(QwtText(QStringLiteral("PolarFrameItem")))
	{
		//画布底图：直角网格随轴带隐藏消失，底图在内容之下（SeriesItem 20/
		//柱顶标注 30 之下）
		setZ(5.0);
		setItemAttribute(QwtPlotItem::Legend, false);
	}

	void mCurvePolarFrameItem::setRadiusTicks(const QVector<double>& majorValues,
		const QStringList& majorLabels, const QVector<double>& minorValues)
	{
		m_majorTicks = majorValues;
		m_majorLabels = majorLabels;
		m_minorTicks = minorValues;
		itemChanged();
	}

	void mCurvePolarFrameItem::setAngleTickCount(int count)
	{
		count = qMax(3, count);   //D25/L13：钳 3（旧 <3 退化 8 会静默换成别的个数）
		if (m_angleCount == count)
			return;
		m_angleCount = count;
		itemChanged();
	}

	void mCurvePolarFrameItem::setTitle(const QString& title)
	{
		if (m_title == title)
			return;
		m_title = title;
		itemChanged();
	}

	void mCurvePolarFrameItem::setTitleFont(const QFont& font)
	{
		if (m_titleFont == font)
			return;
		m_titleFont = font;
		itemChanged();
	}

	void mCurvePolarFrameItem::setTitleColor(const QColor& color)
	{
		if (m_titleColor == color)
			return;
		m_titleColor = color;
		itemChanged();
	}

	void mCurvePolarFrameItem::setLabelFont(const QFont& font)
	{
		if (m_labelFont == font)
			return;
		m_labelFont = font;
		itemChanged();
	}

	void mCurvePolarFrameItem::setGridColor(const QColor& color)
	{
		if (m_gridColor == color)
			return;
		m_gridColor = color;
		itemChanged();
	}

	void mCurvePolarFrameItem::setAngleLabelFont(const QFont& font)
	{
		if (m_angleLabelFont == font)
			return;
		m_angleLabelFont = font;
		itemChanged();
	}

	void mCurvePolarFrameItem::setAngleLabelColor(const QColor& color)
	{
		if (m_angleLabelColor == color)
			return;
		m_angleLabelColor = color;
		itemChanged();
	}

	void mCurvePolarFrameItem::setAngleLabels(const QStringList& labels)
	{
		if (m_angleLabels == labels)
			return;
		m_angleLabels = labels;
		itemChanged();
	}

	void mCurvePolarFrameItem::setRadialVisible(bool on)
	{
		if (m_radialVisible == on)
			return;
		m_radialVisible = on;
		itemChanged();
	}

	void mCurvePolarFrameItem::setAngularVisible(bool on)
	{
		if (m_angularVisible == on)
			return;
		m_angularVisible = on;
		itemChanged();
	}

	void mCurvePolarFrameItem::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		Q_UNUSED(xMap)
		Q_UNUSED(yMap)
		const PolarCanvasGeometry g = polarCanvasGeometry(canvasRect);

		painter->save();
		const QFont labelFont = m_labelFont.family().isEmpty() ? QFont() : m_labelFont;
		const QFont titleFont = m_titleFont.family().isEmpty() ? QFont() : m_titleFont;
		const QFontMetricsF labelFm(labelFont);

		//—— 半径网格：次网格细圆（浅）先画，主刻度圆（深）后画，外圆最深。
		//刻度值口径：视图侧经 PolarRadiusScale 归一后下发 [0,1] 归一化半径，
		//本图元不做域换算（域归一收口在 applyPolarDecor 一处）——
		painter->setRenderHint(QPainter::Antialiasing, true);
		const QPen minorPen(m_gridColor, 1.0);
		const QPen majorPen(m_gridColor.darker(130), 1.0);

		//半径环网（D25/M7：Y1 轴可见性门控）
		if (m_radialVisible)
		{
			painter->setPen(minorPen);
			for (double rNorm : m_minorTicks)
			{
				if (rNorm <= 0.0 || rNorm >= 1.0)
					continue;
				painter->drawEllipse(g.center, g.radius * rNorm, g.radius * rNorm);
			}
			painter->setPen(majorPen);
			for (double rNorm : m_majorTicks)
			{
				if (rNorm <= 0.0 || rNorm >= 1.0)
					continue;
				painter->drawEllipse(g.center, g.radius * rNorm, g.radius * rNorm);
			}
		}
		//外圆（边界框定，恒画）
		painter->setPen(QPen(m_gridColor.darker(160), 1.0));
		painter->drawEllipse(g.center, g.radius, g.radius);

		//—— 角度放射线 + 度数标注（0° 沿 +x 逆时针，标注在圆外沿径向）。
		//X1 独立通道（D25/M6：字体=刻度字体设置/颜色=显示颜色/文本=格式精度
		//管线预构建，个数不符回退整数度——撤 Y1 labelFont 劫持与黑硬编码）
		//+ M7：X1 轴可见性门控辐条与度数 ——
		const int n = qMax(3, m_angleCount);
		const bool customAngleLabels = m_angleLabels.size() == n;
		if (m_angularVisible)
		{
			painter->setPen(majorPen);
			for (int k = 0; k < n; ++k)
			{
				const double ang = 360.0 * k / n;
				const QPointF outer = polarPixel(g, 1.0, ang);
				painter->drawLine(g.center, outer);
			}
		}
		if (m_angularVisible)
		{
			const QFont angleFont = m_angleLabelFont.family().isEmpty()
				? QFont() : m_angleLabelFont;
			const QFontMetricsF angleFm(angleFont);
			painter->setFont(angleFont);
			painter->setPen(m_angleLabelColor);
			for (int k = 0; k < n; ++k)
			{
				const double ang = 360.0 * k / n;
				const QString text = customAngleLabels
					? m_angleLabels.at(k) : QString::number(static_cast<int>(ang));
				const qreal rad = ang * kPolarPi / 180.0;
				const qreal lr = g.radius + 12.0;
				const QPointF p(g.center.x() + lr * std::cos(rad),
					g.center.y() - lr * std::sin(rad));
				const qreal tw = angleFm.horizontalAdvance(text);
				const qreal th = angleFm.height();
				painter->drawText(QPointF(p.x() - tw / 2.0, p.y() + th / 3.0), text);
			}
		}

		//—— 半径刻度数字沿 0° 方向左对齐连排（各标签左缘落在其刻度半径上，
		//需求 04 原型形态："0.0E+00" 起于极点向右连排——旧居中形态作废）；
		//0 值标签照画（2026-11-12 复核：原图实际显示 0.0E+00，旧"对齐参考
		//软件跳 0"口径作废）。位置=0° 辐条下方 2px（2026-11-12 原型 05 轮，
		//旧"轴线上方"形态作废，顺带消除与 0° 角度标注的叠字）+ Y1 标题
		//图内近极点（刻度行下方、0° 轴线下缘，
		//向右延伸出圆前省略号截断——.h 契约+需求 04 原型，圆外旧形态系与
		//契约自相矛盾的实现，作废）
		const int labelCount = qMin(m_majorTicks.size(), m_majorLabels.size());
		if (m_radialVisible)
		{
			//D25/M6 复核勘正：径向刻度数字须显式设字体/画笔——角度块先行时
			//继承其字体颜色、X1 隐藏时继承网格灰笔（恢复 HEAD 无条件设置语义）
			painter->setFont(labelFont);
			painter->setPen(QColor(Qt::black));
			for (int i = 0; i < labelCount; ++i)
			{
				const double rNorm = qBound(0.0, m_majorTicks.at(i), 1.0);
				const QString& text = m_majorLabels.at(i);
				painter->drawText(QPointF(g.center.x() + g.radius * rNorm,
					g.center.y() + labelFm.ascent() + 2.0), text);
			}
		}
		if (m_radialVisible && !m_title.isEmpty())
		{
			painter->setFont(titleFont);
			painter->setPen(m_titleColor);
			const QFontMetricsF titleFm(titleFont);
			const qreal avail = qMax(10.0, g.radius - 6.0);
			//标题基线让位刻度行（2026-11-12 原型 05 轮）：刻度数字移至辐条
			//下方后，标题基线下移一档、顶缘落在刻度行底缘之下，不再叠字
			painter->drawText(
				QPointF(g.center.x(),
					g.center.y() + labelFm.height() + titleFm.height() + 1.0),
				titleFm.elidedText(m_title, Qt::ElideRight, avail));
		}
		painter->restore();
	}
}
