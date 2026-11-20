#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QPointer>

#include "mPanelBase.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostScalarOperationPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostScalarOperationPanel(QWidget* parent = nullptr);

		void newScalarPanel(int caseid);

		//设置当前视图
		void setCurViewName(const QString& viewName);

	private slots:
		//创建视口
		void slot_createScalarViewport(QString viewport);

		//生成标量运算
		void slot_generateScalar();

	signals:

private:
	void setShowMode();
	QString getCloudViewName(const QString& viewName);

	QPointer<mxr::REBUILD_PostRend>			_curPostRend{};				//当前激活窗口

	QVBoxLayout*								_mainLayout;
	QComboBox*								_opCombo;						//运算符
	QLineEdit*								_valueEdit;						//标量数值
	QPushButton*							_buttonGenerate;				//生成按钮

	};
}
