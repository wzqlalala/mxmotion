#include "mPostPlotFloatWindow.h"
#include "mPlotView.h"
#include <qwt_plot.h>
#include <QTimer>

using namespace PlotView;

namespace PostWindow {

mPostPlotFloatWindow::mPostPlotFloatWindow(QWidget* parent)
	: QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
	_layout = new QVBoxLayout(this);
	_layout->setContentsMargins(0, 0, 0, 0);
	_plotView = nullptr;
	setMinimumSize(400, 300);
	resize(400, 300); // 确保初始尺寸不为0，避免子widget在首次布局时获得0x0尺寸
}

mPostPlotFloatWindow::~mPostPlotFloatWindow()
{
	if (_plotView) {
		_layout->removeWidget(_plotView);
		_plotView->setParent(nullptr);
		delete _plotView;
		_plotView = nullptr;
	}
}

void mPostPlotFloatWindow::setPlotView(PlotView::mPlotView* plotView)
{
	if (_plotView) {
		_layout->removeWidget(_plotView);
		_plotView->setParent(nullptr);
		delete _plotView;
		_plotView = nullptr;
	}
	_plotView = plotView;
	if (_plotView) {
		_layout->addWidget(_plotView);
		_layout->setStretchFactor(_plotView, 1);
		_plotView->setMinimumSize(200, 150); // 确保plotView本身不会获得0x0尺寸

		// 使用scheduleReplot轮询直到canvas获得有效尺寸，确保曲线一定能绘制
		scheduleReplot();
	}
}

PlotView::mPlotView* mPostPlotFloatWindow::getPlotView() const
{
	return _plotView;
}

void mPostPlotFloatWindow::setWindowTitleText(const QString& title)
{
	_name = title;
}

void mPostPlotFloatWindow::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	// QwtPlot 需要显式 replot 才能正确显示曲线
	// 使用 scheduleReplot 轮询直到 canvas 获得有效尺寸后再绘制
	if (_plotView) {
		scheduleReplot();
	}
}

void mPostPlotFloatWindow::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	// resizeEvent 中直接同步 replot 时，QwtPlot 的 canvas 可能还未获得最终尺寸
	// 改为延迟到当前事件循环结束后执行，确保布局引擎先完成尺寸计算
	if (_plotView) {
		QTimer::singleShot(0, this, [this]() {
			if (_plotView) {
				QwtPlot* qwtPlot = _plotView->getQwtPlot();
				if (qwtPlot) {
					qwtPlot->replot();
				}
			}
		});
	}
}

void mPostPlotFloatWindow::scheduleReplot()
{
	if (!_plotView) return;
	QwtPlot* qwtPlot = _plotView->getQwtPlot();
	if (!qwtPlot) return;

	QSize canvasSize = qwtPlot->canvas()->size();
	if (canvasSize.width() > 0 && canvasSize.height() > 0) {
		// canvas已有有效尺寸，执行replot
		qwtPlot->replot();
	} else {
		// canvas尺寸尚未就绪，延迟50ms后重试
		QTimer::singleShot(50, this, &mPostPlotFloatWindow::scheduleReplot);
	}
}

} // namespace PostWindow
