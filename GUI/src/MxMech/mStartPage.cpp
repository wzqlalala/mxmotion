#include "mStartPage.h"

//QT
#include <QPixmap>
#include <QThread>
#include <QDesktopWidget>
#include <QApplication>

// 声明 Windows API
#ifdef _WIN32
#include <windows.h>
#endif


namespace MxMech
{
	mStartPage::mStartPage()
	{
		//this->setPixmap(QPixmap(":/mMainWidgets/Icon/start.png"));

		// 获取屏幕分辨率
		QDesktopWidget* desktopWidget = QApplication::desktop();
		QRect screenRect = desktopWidget->screenGeometry();
		// 加载图片并调整大小（多体产品用多体版启动图，形态由编译宏决定，见 CMakeLists MX_RIBBON_FORM）
#ifdef MX_RIBBON_FORM_MOTION
		QPixmap pixmap(":/mMainWidgets/Icon/start_Motion.png");
#else
		QPixmap pixmap(":/mMainWidgets/Icon/start.png");
#endif
		QPixmap scaledPixmap = pixmap.scaled(screenRect.width() * 7/24, screenRect.height()* 5 /12, Qt::KeepAspectRatio, Qt::SmoothTransformation);
		this->setPixmap(scaledPixmap);

        // 在显示启动页之前禁用其窗口获取焦点，避免启动时焦点被启动页抢走
#ifdef _WIN32
        // 先创建为隐藏窗口并移除 WS_EX_NOACTIVATE 风格，确保 Qt 接管后不再抢夺焦点
        this->setWindowFlag(Qt::WindowDoesNotAcceptFocus);
        this->setAttribute(Qt::WA_ShowWithoutActivating);
#endif
		this->show();
		QThread::sleep(1);
	}

	mStartPage::~mStartPage()
	{
	}
}