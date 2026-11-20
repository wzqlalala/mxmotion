#include "mCurveTrimMarker.h"

#include <QBrush>
#include <QPainter>
#include <QPainterPath>
#include <QPen>

//Qwt：坐标轴映射（transform 数据值 -> 像素）
#include <qwt_scale_map.h>

namespace PostCurve
{
	namespace
	{
		const QColor kTrimLineColor(0, 0, 0);   //黑色界线（UI设计图16）
		const double kTrimTriangleHalf = 5.0;   //三角垂直于界线方向的半宽
		const double kTrimTriangleLen = 9.0;    //三角沿界线法向（朝内侧）的长度
	}

	mCurveTrimMarker::mCurveTrimMarker(Role role)
		: QwtPlotItem()
		, _role(role)
	{
		//置于普通曲线(10)与引擎基准线(50)之上、十字光标(999)之下
		setZ(60.0);
	}

	void mCurveTrimMarker::setOrientation(Orientation orient)
	{
		_orientation = orient;
	}

	void mCurveTrimMarker::setValue(double value)
	{
		_value = value;
	}

	void mCurveTrimMarker::setRole(Role role)
	{
		_role = role;
	}

	void mCurveTrimMarker::setHighlighted(bool on)
	{
		_highlighted = on;
	}

	void mCurveTrimMarker::draw(QPainter* painter,
		const QwtScaleMap& xMap,
		const QwtScaleMap& yMap,
		const QRectF& canvasRect) const
	{
		painter->save();

		QPen linePen(kTrimLineColor);
		linePen.setWidthF(_highlighted ? 3.0 : 2.0);
		painter->setPen(linePen);

		QPainterPath triangle;
		if (_orientation == Vertical)
		{
			//竖直界线（"水平"方向截取）：三角在竖线中部，起始朝右/终止朝左
			const double px = xMap.transform(_value);
			painter->drawLine(QPointF(px, canvasRect.top()), QPointF(px, canvasRect.bottom()));
			const double cy = canvasRect.center().y();
			const double dir = (_role == Start) ? 1.0 : -1.0;
			triangle.moveTo(px, cy - kTrimTriangleHalf);
			triangle.lineTo(px, cy + kTrimTriangleHalf);
			triangle.lineTo(px + dir * kTrimTriangleLen, cy);
		}
		else
		{
			//水平界线（"垂直"方向截取）：屏幕 y 向下，数值增大朝上——
			//起始（数值下界）的内侧在上方 -> 三角朝上，终止朝下
			const double py = yMap.transform(_value);
			painter->drawLine(QPointF(canvasRect.left(), py), QPointF(canvasRect.right(), py));
			const double cx = canvasRect.center().x();
			const double dir = (_role == Start) ? -1.0 : 1.0;
			triangle.moveTo(cx - kTrimTriangleHalf, py);
			triangle.lineTo(cx + kTrimTriangleHalf, py);
			triangle.lineTo(cx, py + dir * kTrimTriangleLen);
		}
		triangle.closeSubpath();

		painter->setPen(Qt::NoPen);
		painter->setBrush(kTrimLineColor);
		painter->drawPath(triangle);

		painter->restore();
	}
}
