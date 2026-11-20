#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QMainWindow>
#include <QToolBar>
#include <QAbstractItemModel>
#include <functional>

#include "mCommon_global.h"

/*
* 中心窗口/工具栏提供者（间接通道）
* mCenterMainWindow（居MainWindow）构造时自注册；mDockLayoutManager（居GUIFoundation）
* 预设重排后钉正工具栏顺序时经此取 QMainWindow* 与两个视图工具栏，
* 消除GUIFoundation对MainWindow的类型依赖——链接方向保持 GUIFoundation->Common 单向。
*/
namespace MechCommon
{
	//主窗口（QMainWindow 级即可：addToolBar 等属基类接口）
	COMMON_EXPORT std::function<QMainWindow*()>& centerMainWindowProvider();
	//视图工具栏 / 视图显示工具栏
	COMMON_EXPORT std::function<QToolBar*()>& viewToolBarProvider();
	COMMON_EXPORT std::function<QToolBar*()>& viewDisplayBarProvider();
	//动画工具栏壳（anim-tab-v13：mCenterMainWindow::initAnimToolBar 预创建空壳，
	//内容件由 mPost3DResultPanel 经此回挂；PostWindow 不反向链接 MainWindow 类型，
	//通道与上三者的解耦口径一致）
	COMMON_EXPORT std::function<QToolBar*()>& animToolBarProvider();
}
