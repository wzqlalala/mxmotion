#pragma once
//自绘标题栏对话框基类：标题栏/阴影/边框缩放/最大化动画由基类提供
#include "mFramelessDialog.h"
#include "ColorBarWidget.h"
#include <QPointer>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

namespace Ui
{
	class mCustomColorBar;
}

namespace PostWindow
{
	class mCustomColorBar : public MechMainWindow::mFramelessDialog
	{
		Q_OBJECT

	public:
		/** 默认构造函数 */
		mCustomColorBar(QWidget *parent = Q_NULLPTR);

		/** 重载构造函数 */
		mCustomColorBar(const QStringList& posnames, QWidget* parent = Q_NULLPTR);

		/** 析构函数 */
		~mCustomColorBar();

        void getColorAndValue();
        void getDefaultColorAndValue(QVector<QColor> colors, QVector<QString> values);
        void getColorBarNum();

	private slots:
		void slot_okButton();
		void slot_defaultButton();

	signals:
        void setDiscrete(int);
	protected:
		void mousePressEvent(QMouseEvent *event);


	private:
		Ui::mCustomColorBar*		_ui;		//UI
		ColorBarWidget*				colorBar_{};
        int                         colorBarNum_;//颜色条阶数
	};
}
