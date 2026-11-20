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
#include "mPostDataBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostLogPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostLogPanel(QWidget* parent = nullptr);

		void newLogPanel(int caseid);

		//设置当前视图
		void setCurViewName(const QString& viewName);

        void updatePanel();

	private slots:
        //选择文件
        void slot_chooseFile();
        // 切换注释模式： 编辑模式，普通模式
        void slot_changeType();
        //设置颜色
        void slot_setColor();

        // 设置透明度
        void slot_setSlider(int value);
        void slot_setSpinBox(int value);
    signals:

    protected:

	private:
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};					//当前激活窗口
        QString                                 _curFileContent{};

		QVBoxLayout*							_mainLayout;
        QLineEdit*                              _lineFile;
        QPushButton*                            _btnFile;
        QToolButton*                            _btnColor;
        QSlider*                                _sliderTransparency;
        QSpinBox*                               _spinTransparency;
        QPushButton* _currentStateButton;        // 显示并切换当前编辑模式
    };
}
