#include "mCurveBaselineLabel.h"

#include <QFontMetrics>
#include <QPainter>
#include <qwt_scale_map.h>
#include <qwt_text.h>

namespace PostCurve
{
	mCurveBaselineLabel::mCurveBaselineLabel()
		: QwtPlotItem(QwtText(QStringLiteral("BaselineLabel")))
	{
		//紧贴所属基准线（引擎 markerLine Z50）之上，裁剪线(Z60)/悬停读数(998)之下
		setZ(51.0);
		setItemAttribute(QwtPlotItem::Legend, false);
	}

	void mCurveBaselineLabel::setLabelText(const QString& text)
	{
		if (m_text == text)
			return;
		m_text = text;
		itemChanged();
	}

	void mCurveBaselineLabel::setLabelFont(const QFont& font)
	{
		if (m_font == font)
			return;
		m_font = font;
		itemChanged();
	}

	void mCurveBaselineLabel::setLabelColor(const QColor& color)
	{
		if (m_color == color)
			return;
		m_color = color;
		itemChanged();
	}

	void mCurveBaselineLabel::setLabelPos(int pos)
	{
		pos = qBound(0, pos, 8);
		if (m_labelPos == pos)
			return;
		m_labelPos = pos;
		itemChanged();
	}

	void mCurveBaselineLabel::setLineHorizontal(bool horizontal)
	{
		if (m_horizontal == horizontal)
			return;
		m_horizontal = horizontal;
		itemChanged();
	}

	void mCurveBaselineLabel::setLineValue(double value)
	{
		if (qFuzzyCompare(m_value, value))
			return;
		m_value = value;
		itemChanged();
	}

	void mCurveBaselineLabel::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		if (m_text.isEmpty())
			return;
		//所属线像素位置（与线同值同映射，标签始终贴线）；线越出画布不绘制
		//（线本身越界也不画）
		const double linePx = m_horizontal ? yMap.transform(m_value)
			: xMap.transform(m_value);
		if (m_horizontal
			&& (linePx < canvasRect.top() || linePx > canvasRect.bottom()))
			return;
		if (!m_horizontal
			&& (linePx < canvasRect.left() || linePx > canvasRect.right()))
			return;

		painter->save();
		const QFont font = m_font.family().isEmpty() ? QFont() : m_font;
		painter->setFont(font);
		const QFontMetricsF fm(font);
		const qreal textW = fm.horizontalAdvance(m_text);
		const qreal textH = fm.height();

		//9宫格 -> 文字矩形：沿线方向分量贴画布边缘摆（纵向线上/中/下，横向线
		//左/中/右），垂直线分量决定文字在线的哪一侧（中=骑线）；贴边放不下
		//整行文字时跳绘不绘（2026-11-17 第六批②A：钳入改跳绘）
		const int hIdx = m_labelPos % 3;   //0左 1中 2右
		const int vIdx = m_labelPos / 3;   //0上 1中 2下
		const qreal gap = 5.0;             //文字与线的间距
		const qreal margin = 6.0;          //贴画布边缘的留白

		QRectF rect;
		int align = Qt::TextSingleLine;
		if (m_horizontal)
		{
			rect = canvasRect.adjusted(margin, 0, -margin, 0);
			align |= (hIdx == 0) ? Qt::AlignLeft
				: (hIdx == 1) ? Qt::AlignHCenter : Qt::AlignRight;
			if (vIdx == 0)
			{
				//线上方放不下整行文字即跳绘（2026-11-17 第六批②A 裁定：出界钳入
				//改跳绘，回归面=模板/属性基准线贴边标签从钳入变不绘）
				if (linePx - gap - rect.top() < textH)
					return;
				rect.setBottom(linePx - gap);
				align |= Qt::AlignBottom;   //线上方
			}
			else if (vIdx == 1)
			{
				rect = QRectF(rect.left(), linePx - textH / 2.0, rect.width(), textH);
				align |= Qt::AlignVCenter;  //骑线
			}
			else
			{
				//线下方放不下整行文字即跳绘（同上裁定；NVH 柱状图 Out-of-Phase
				//沉底隐藏语义由本分支自然达成）
				if (rect.bottom() - (linePx + gap) < textH)
					return;
				rect.setTop(linePx + gap);
				align |= Qt::AlignTop;      //线下方
			}
		}
		else
		{
			rect = canvasRect.adjusted(0, margin, 0, -margin);
			align |= (vIdx == 0) ? Qt::AlignTop
				: (vIdx == 1) ? Qt::AlignVCenter : Qt::AlignBottom;
			if (hIdx == 0)
			{
				//线左侧放不下整行文字即跳绘（同上裁定）
				if (linePx - gap - rect.left() < textW)
					return;
				rect.setRight(linePx - gap);
				align |= Qt::AlignRight;    //线左侧
			}
			else if (hIdx == 1)
			{
				rect = QRectF(linePx - textW / 2.0, rect.top(), textW, rect.height());
				align |= Qt::AlignHCenter;  //骑线
			}
			else
			{
				//线右侧放不下整行文字即跳绘（同上裁定）
				if (rect.right() - (linePx + gap) < textW)
					return;
				rect.setLeft(linePx + gap);
				align |= Qt::AlignLeft;     //线右侧
			}
		}
		painter->setPen(m_color);
		painter->drawText(rect, align, m_text);
		painter->restore();
	}
}
