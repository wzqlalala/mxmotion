#pragma once

#include <QWidget>

namespace PostCommon
{
	class mBorderWidget : public QWidget
	{
		Q_OBJECT

	public:
		mBorderWidget(QWidget* parent);
		mBorderWidget() {};

		~mBorderWidget();
		void setHighlightBorder(bool highlight);
	protected:
	signals:
		void borderWidgetClicked(mBorderWidget* borderWidget); 
		void borderSelected(QWidget* subWindow);
	public slots:
		void onRendereEvent(QEvent* event);
	protected:
		void mousePressEvent(QMouseEvent *event) override;
		void paintEvent(QPaintEvent *event) override;
	private:
		QWidget* _widget;
		bool highlightBorder;   // 用于确定是否需要绘制高亮边框
	};
}