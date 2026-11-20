#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 自绘标题栏窗口框架助手（mFramelessWindowHelper）
* 背景：Qt 原生标题栏无法跟随主题换色，主窗与对话框统一改为自绘标题栏。
* 本助手提供一套可复用的窗口化能力，供主窗口（mMechMainWindow）与对话框
* 基类（mFramelessDialog）共同接入：
*   - installFrameless()：保留原生窗口框架（WS_THICKFRAME/WS_CAPTION），
*     不设置 Qt::FramelessWindowHint，从而 DWM 阴影、边框拖拽动画、
*     Aero Snap、最大化/还原动画全部由系统原生提供
*   - handleNativeEvent()：清零非客户区（去原生标题栏）、提供边缘缩放
*     命中带、最大化几何钳制、首次显示时向 DWM 声明框架延伸以保留阴影
* 标题栏颜色取自 UiPalette，切主题后由 ThemeManager::applyTitleBarTheme
* 重发。
*/

#include "mCommon_global.h"

#include <QWidget>

class QBoxLayout;

namespace MechMainWindow
{
	namespace FramelessWindowHelper
	{
		/*
		* 安装自绘标题栏窗口标志：保留原生框架（不用 FramelessWindowHint），
		* 仅保留 Window/MinMax/Close 能力位。必须在窗口 show 之前调用。
		*/
		COMMON_EXPORT void installFrameless(QWidget* window);

		/*
		* 处理 Windows 原生消息（WM_NCCALCSIZE 清零非客户区、WM_NCHITTEST
		* 边缘缩放、WM_GETMINMAXINFO 最大化钳制、WM_SHOWWINDOW 阴影声明）。
		* 在宿主 nativeEvent 中调用；返回 true 表示消息已消费。
		*/
		COMMON_EXPORT bool handleNativeEvent(QWidget* wnd, void* message, long* result);
	}
}
