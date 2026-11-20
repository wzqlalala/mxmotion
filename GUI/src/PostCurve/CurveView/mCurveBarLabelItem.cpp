#include "mCurveBarLabelItem.h"

#include <QFontMetrics>
#include <QPainter>
#include <qwt_scale_map.h>
#include <qwt_text.h>

namespace PostCurve
{
	mCurveBarLabelItem::mCurveBarLabelItem()
		: QwtPlotItem(QwtText(QStringLiteral("BarLabelItem")))
	{
		//曲线上层（引擎曲线 Z0）、基准线族（markerLine Z50/线上标签 Z51）之下；
		//柱顶标注须压在柱形之上但不得遮蔽基准线与注释
		setZ(30.0);
		setItemAttribute(QwtPlotItem::Legend, false);
	}

	void mCurveBarLabelItem::setBarData(const QVector<double>& xs, const QVector<double>& ys,
		const QStringList& texts)
	{
		//三数组等长截齐（防御调用方传参不齐，标注与柱一一对应不可错位）
		const int count = qMin(xs.size(), qMin(ys.size(), texts.size()));
		m_xs = xs.mid(0, count);
		m_ys = ys.mid(0, count);
		m_texts = texts.mid(0, count);
		itemChanged();
	}

	void mCurveBarLabelItem::clearBars()
	{
		if (m_xs.isEmpty() && m_ys.isEmpty() && m_texts.isEmpty())
			return;
		m_xs.clear();
		m_ys.clear();
		m_texts.clear();
		itemChanged();
	}

	void mCurveBarLabelItem::setLabelColor(const QColor& color)
	{
		if (m_color == color)
			return;
		m_color = color;
		itemChanged();
	}

	void mCurveBarLabelItem::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		if (m_texts.isEmpty())
			return;
		painter->save();
		//越出画布的标注由裁剪兜底（柱心 X 越界仍可能有半截文本可见，裁剪防外溢）
		painter->setClipRect(canvasRect);
		painter->setPen(m_color);
		const QFontMetricsF fm(painter->font());
		const qreal textH = fm.height();
		const qreal gap = 3.0;     //文本与柱顶/柱底的间距
		const qreal margin = 2.0;  //贴画布左右边缘的收进留白
		const int count = m_texts.size();
		for (int i = 0; i < count; ++i)
		{
			const qreal px = xMap.transform(m_xs.at(i));
			const qreal py = yMap.transform(m_ys.at(i));
			//柱心 X 越出画布不绘制（柱本身越界也不画，语义一致）
			if (px < canvasRect.left() || px > canvasRect.right())
				continue;
			const qreal textW = fm.horizontalAdvance(m_texts.at(i));
			//正值柱标在柱顶上方、负值柱标在柱底下方（符号=原始值符号，与 D5
			//文本口径一致；变换域位置由 ys 承载）
			const qreal top = (m_ys.at(i) >= 0.0)
				? py - gap - textH
				: py + gap;
			QRectF rect(px - textW / 2.0, top, textW, textH);
			//近画布上/下缘防裁剪（任务 3.2/D5）：下缘出界翻转到数据点上方
			//（柱内侧），仍出界再钳制边界内——标注恒可见优先
			if (rect.bottom() > canvasRect.bottom() - margin)
				rect.moveTop(py - gap - textH);
			if (rect.top() < canvasRect.top() + margin)
				rect.moveTop(canvasRect.top() + margin);
			if (rect.bottom() > canvasRect.bottom() - margin)
				rect.moveBottom(canvasRect.bottom() - margin);
			//贴画布左右边缘时收进画布内（柱顶标注整体可见优先）
			if (rect.left() < canvasRect.left() + margin)
				rect.moveLeft(canvasRect.left() + margin);
			if (rect.right() > canvasRect.right() - margin)
				rect.moveRight(canvasRect.right() - margin);
			painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextSingleLine, m_texts.at(i));
		}
		painter->restore();
	}
}
