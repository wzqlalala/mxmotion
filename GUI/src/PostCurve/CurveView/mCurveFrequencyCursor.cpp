#include "mCurveFrequencyCursor.h"

#include <QPainter>
#include <qwt_scale_map.h>
#include <qwt_text.h>

namespace PostCurve
{
	mCurveFrequencyCursor::mCurveFrequencyCursor()
		: QwtPlotItem(QwtText(QStringLiteral("FrequencyCursor")))
	{
		//readout/crosshair 顶层带（悬停读数 Z998 之下、注释/裁剪线之上）：
		//游标是持续态参考线，压在全部内容之上但不遮蔽读数
		setZ(990.0);
		setItemAttribute(QwtPlotItem::Legend, false);
	}

	void mCurveFrequencyCursor::setXValue(double x)
	{
		if (qFuzzyCompare(m_x, x))
			return;
		m_x = x;
		itemChanged();
	}

	void mCurveFrequencyCursor::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		const double px = xMap.transform(m_x);
		//游标越出画布不绘制（与基准线越界不画一致）
		if (px < canvasRect.left() || px > canvasRect.right())
			return;
		painter->save();
		painter->setPen(QPen(Qt::red, 1.0));
		painter->drawLine(qreal(px), canvasRect.top(), qreal(px), canvasRect.bottom());
		painter->restore();
	}
}
