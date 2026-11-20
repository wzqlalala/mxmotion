#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLineEdit>
#include "mCheckComboBox.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostAnimationMenuWidget : public QWidget
	{
		Q_OBJECT
	public:
		explicit mPostAnimationMenuWidget(QWidget* parent = nullptr);

	public slots:
		/*
		* 设置动画菜单栏禁用状态
		*/
		void setAnimationMenuStatus(bool isEnable);

		/*
		* 更新动画菜单界面
		*/
		void updateAnimationMenuGUI(QString view);

		/*
		* 暂停动画
		*/
		void pauseAnimation();

		/*
		* 设置失效单元隐藏开关
		*/
		void setHideFailedElement(bool checked);

	signals:

	protected:

	public:
		QToolButton* _btnFirst;
		QToolButton* _btnPrevious;
		QToolButton* _btnNext;
		QToolButton* _btnLast;
		QToolButton* _btnPlay;
        MxFunctions::mCheckComboBox* _comboRange;
		QCheckBox* _checkLoopPlay;
		QSlider* _sliderRate;
		QLineEdit* _lineEditRate;
		QToolButton* _btnExportVideo;
		QToolButton* _btnCustomVideo;
		QCheckBox* _checkHideFailedElement;
	};
}
