#include "mCurveReadoutItem.h"

#include "../CurveCore/CurveDefine.h"

#include <QPainter>
#include <QFontMetrics>
#include <QStringList>
#include <qwt_scale_map.h>
#include <qwt_text.h>

namespace PostCurve
{
	mCurveReadoutItem::mCurveReadoutItem()
		: QwtPlotItem(QwtText(QStringLiteral("CurveReadout")))
	{
		setZ(998.0);   //曲线/注释/基准线(Z50)之上，十字光标(999)之下
		setItemAttribute(QwtPlotItem::Legend, false);
		setVisible(false);
	}

	void mCurveReadoutItem::showAt(const QPoint& canvasPos, const QString& text)
	{
		m_pos = canvasPos;
		m_text = text;
		setVisible(true);
		itemChanged();   //通知 Qwt 重绘
	}

	void mCurveReadoutItem::hideReadout()
	{
		setVisible(false);
		m_pos = QPoint(-1, -1);
		m_text.clear();
	}

	void mCurveReadoutItem::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		Q_UNUSED(xMap);
		Q_UNUSED(yMap);
		if (m_pos.x() < 0 || m_pos.y() < 0 || m_text.isEmpty())
			return;

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing, true);

		//吸附点空心圆圈（同引擎 mPlotSnapper 悬停标记：空心不遮曲线；
		//描边取模块主题蓝，同曲线 hover 高亮语言）
		painter->setPen(QPen(curveHighlightBlue(), 2.0));
		painter->setBrush(Qt::NoBrush);
		painter->drawEllipse(QPointF(m_pos), 5.0, 5.0);

		QFont font(QStringLiteral("Arial"), 10);
		painter->setFont(font);
		const QFontMetrics fm(font);

		//文本块尺寸（逐行取最大宽）+ 内边距
		qreal textW = 0.0;
		qreal textH = 0.0;
		const QStringList lines = m_text.split(QLatin1Char('\n'));
		for (const QString& line : lines)
		{
			textW = qMax(textW, qreal(fm.horizontalAdvance(line)));
			textH += fm.height();
		}
		const qreal pad = 4.0;
		const QSizeF chip(textW + pad * 2.0, textH + pad * 2.0);

		//默认摆在锚点右下方避让光标，右侧/下侧越界翻向左上，最终收回画布内
		QPointF topLeft(m_pos.x() + 14.0, m_pos.y() + 14.0);
		if (topLeft.x() + chip.width() > canvasRect.right())
			topLeft.setX(m_pos.x() - chip.width() - 14.0);
		if (topLeft.y() + chip.height() > canvasRect.bottom())
			topLeft.setY(m_pos.y() - chip.height() - 14.0);
		topLeft.setX(qBound(canvasRect.left(), topLeft.x(),
			qMax(canvasRect.left(), canvasRect.right() - chip.width())));
		topLeft.setY(qBound(canvasRect.top(), topLeft.y(),
			qMax(canvasRect.top(), canvasRect.bottom() - chip.height())));

		//白底黑边黑字文本框（同引擎 mPlotSnapper 悬停标签，直角矩形）
		painter->setPen(QPen(Qt::black, 1.0));
		painter->setBrush(QColor(255, 255, 255, 220));
		painter->drawRect(QRectF(topLeft, chip));
		painter->setPen(Qt::black);
		painter->drawText(QRectF(topLeft.x() + pad, topLeft.y() + pad, textW, textH),
			Qt::AlignLeft | Qt::AlignTop, m_text);

		painter->restore();
	}
}
