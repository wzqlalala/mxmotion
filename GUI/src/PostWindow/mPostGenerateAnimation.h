#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostGenerateAnimationPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostGenerateAnimationPanel(QWidget* parent = nullptr);

		void newGenerateAniPanel(int caseid);

		//获取最接近输入时间的帧号
		int getClosestFrame(double time);

		//更新动画生成参数页界面
		void updateGenerateAnimationGUI(QString ani, QString var, QString start, QString end);

		//更新面板
		void updatePanel();

	private slots:
		//设置动画类型
		void slot_chooseAniType(const QString& type);

		//设置控制变量类型
		void slot_chooseVarType(const QString& type);

		//编辑起始输入框
		void slot_setStartEdit();

		//编辑结束输入框
		void slot_setEndEdit();
		
		//生成动画
		void slot_generateAnimation();

	signals:

	private:
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};					//当前激活窗口
		QMap<int, double>						_frameTimes{};					// 时间、帧号
		int										_startFrame;					//起始帧
		int										_endFrame;						//结束帧
		double									_startFrameTime;				//起始时间
		double									_endFrameTime;					//结束时间

		QVBoxLayout*							_mainLayout;
		QComboBox*								_comboCase;
		QComboBox*								_comboAniType;
		QComboBox*								_comboVarType;
		QLineEdit*								_lineEditStart;
		QLineEdit*								_lineEditEnd;
		QPushButton*							_buttonGenerate;
	};
}
