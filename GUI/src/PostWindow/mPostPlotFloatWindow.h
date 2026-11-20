#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QShowEvent>
#include <QResizeEvent>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PlotView
{
	class mPlotView;
}

namespace PostWindow
{

class mPostPlotFloatWindow : public QWidget
{
	Q_OBJECT
public:
	explicit mPostPlotFloatWindow(QWidget* parent = nullptr);
	~mPostPlotFloatWindow();

	void setPlotView(PlotView::mPlotView* plotView);
	PlotView::mPlotView* getPlotView() const;

	void setWindowTitleText(const QString& title);

protected:
	void showEvent(QShowEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void scheduleReplot();

	QVBoxLayout* _layout;
	PlotView::mPlotView* _plotView;
	QString _name;
};

} // namespace PostWindow
