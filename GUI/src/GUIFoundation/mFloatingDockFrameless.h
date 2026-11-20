#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* ADS 浮动容器自绘标题栏注入器（mFloatingDockFrameless）
*
* 背景：qtads 4.x 的自绘浮动标题栏（CFloatingWidgetTitleBar + 拖拽状态机转发）
* 只在 Linux 分支编译，Windows 分支浮动容器固定使用原生标题栏——拖出 dock 后
* 回到系统白标题栏，与主窗/首选项的自绘标题栏风格割裂，切主题也不跟随。
*
* 本类经 CDockManager::floatingWidgetCreated 给每个浮动容器统一接入：
*   - FramelessWindowHelper::installFrameless 保留原生框架，
*     经应用级 native filter 调 handleNativeEvent 清零非客户区
*     （浮动容器是 qtads 的类，nativeEvent 无法覆写，只能全局过滤转发）；
*   - 插入与主窗/首选项完全同款的 mWindowTitleBar（配色/尺寸/三键自绘）；
*   - 标题栏拖动经 mTitleBarDragDelegate 转发 startDragging/moveFloating/
*     finishDragging——qtads 公开的拖拽状态机，拖回停靠（drop overlay）
*     行为与原生标题栏拖动一致。
*
* native filter 只消费 WM_NCCALCSIZE/WM_NCHITTEST/WM_GETMINMAXINFO 等
* 框架消息；qtads 自身依赖的 WM_NCLBUTTONDOWN/WM_MOVING 等不拦截。
*/

#include "guifoundation_global.h"

#include <QAbstractNativeEventFilter>
#include <QEvent>
#include <QObject>
#include <QSet>

class QWidget;

namespace ads
{
	class CDockManager;
	class CFloatingDockContainer;
}

namespace MechMainWindow
{
	class GUIFOUNDATION_EXPORT mFloatingDockFrameless
		: public QObject
		, public QAbstractNativeEventFilter
	{
		Q_OBJECT
	public:
		/*
		* 接入一个 dock 管理器：连接 floatingWidgetCreated（此后所有拖出/
		* 恢复布局产生的浮动容器自动注入自绘标题栏），并安装 native filter。
		* 幂等；须在首个浮动容器出现前调用（mDockLayoutManager::setCDockManager）。
		*/
		static void attach(ads::CDockManager* manager);

		~mFloatingDockFrameless() override;

		bool nativeEventFilter(const QByteArray& eventType, void* message, long* result) override;

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		mFloatingDockFrameless() = default;

		/*
		* 给单个浮动容器注入自绘标题栏（防重复）
		*/
		void applyTo(ads::CFloatingDockContainer* floating);

	private:
		static mFloatingDockFrameless* _instance;

		// 已注入自绘标题栏的浮动容器（native filter 反查 + 防重复注入）
		QSet<QWidget*> _windows;
	};
}
