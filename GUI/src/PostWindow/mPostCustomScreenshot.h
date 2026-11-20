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
#include "mAnimationRecordWidget.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostCustomScreenshotPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostCustomScreenshotPanel(QWidget* parent = nullptr);

		//快速截图
		void slot_aniQuickScreenshot();

        //快速截图(带路径)
        void aniQuickScreenshot(QString path);

		//更新面板数据
		void updateScreenshotPanel();

		//更新截图文件名
		QString getNextScreenshotFilename(const QString& dirPath, const QString& prefix = "截图-", const QString& ext = "png");

		//重新绘制录制窗口
		void rePainterRecordWindow();

		//录制进行中查询（实时动画 v1.1 录制互斥权威簿记）：引擎链
		//RecordScreen 发起成功处置位，signal_EndAnimation/CaptureFinished 回执复位
		bool isRecording() const { return _isStart; }

	private slots:
		//设置视口名称
		void setCurViewName(QString viewname);

		//设置分辨率
		void slot_setResolution(int f);

		//设置背景颜色
		void slot_setBackGroundColor();

		//完成信号
		void slot_EndAnimation();

		//设置路径
		void slot_setScreenshotPath();

		//自定义截图
		void slot_customScreenshot();

		//设置选择截屏范围
		void slot_chooseRange(const QString&);

		//绘制录制窗口
		void getRecordWindow();

		//获取录制窗口起始坐标以及宽和高
		void getRecordWindowInfo(QPoint& startPos, int& width, int& height);

		//获取截屏框位置显示
		void slot_setPos(QPoint startPos, QPoint endPos);

		//输入坐标设置截屏框位置
		void slot_setRecordCoord();

	signals:

	protected:
		void hideEvent(QHideEvent* event) override;

	private:
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};					//当前激活窗口
		QColor									_color{ 255,255,255 };
		bool									_isStart{ false };				//判断是否为截图功能
		QString									_filePath;						//截图文件保存路径
		QString									_fileName;						//截图文件保存名称
		mAnimationRecordWidget*					_recordWidget{};
		QPoint									_posF;
		QPoint									_posS;
		bool									_recordFlag { false };			//标记截图框是否显示

		QVBoxLayout*							_mainLayout;
		QSpinBox*								_spinResolution;
		QPushButton*							_btnBackground;
		QComboBox*								_comboRange;
		QSpinBox*								_spinLeftx;
		QSpinBox*								_spinLefty;
		QSpinBox*								_spinRightx;
		QSpinBox*								_spinRighty;
		QLineEdit*								_lineEditFlieName;
		QLineEdit*								_lineEditFilePath;
	};
}
