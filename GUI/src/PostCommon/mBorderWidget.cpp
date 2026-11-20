#include "mBorderWidget.h"
#include <QPainter>
#include <QEvent>

namespace PostCommon
{
	mBorderWidget::mBorderWidget(QWidget* parent)
		: QWidget(parent),highlightBorder(false)
	{
		// 移除窗口装饰
		setWindowFlags(Qt::FramelessWindowHint);
		// 允许透明背景
		setAttribute(Qt::WA_TranslucentBackground);
		_widget = parent;
	}

	mBorderWidget::~mBorderWidget()
	{
	}
	void mBorderWidget::mousePressEvent(QMouseEvent *event) {
		QWidget::mousePressEvent(event);
		emit borderWidgetClicked(this); // 发射信号
	}
	void mBorderWidget::setHighlightBorder(bool highlight)
	{
		highlightBorder = highlight;
		update(); // 请求重绘
	}

	void mBorderWidget::onRendereEvent(QEvent* event)
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			emit borderSelected(_widget);
		}
	}

	void mBorderWidget::paintEvent(QPaintEvent * event)
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		if (highlightBorder) {
			//QColor customColor(41, 157, 255);
			//painter.setPen(QPen(customColor, 2));
			//painter.drawRect(rect().adjusted(0, 0, 0, 0)); 
		}
		else {
			painter.setPen(QPen(Qt::gray, 1)); 
			painter.drawRect(rect().adjusted(0, 0, 0, 0)); 
		}
	}



}