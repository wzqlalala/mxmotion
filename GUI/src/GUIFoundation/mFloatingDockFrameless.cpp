// windows.h 引入顺序与 mFramelessWindowHelper.cpp 一致（Q_OS_WIN 由 Qt 头定义，
// WIN32_LEAN_AND_MEAN 避免 Qt5 qwindowdefs_win.h 的 tagMSG 冲突）
#include "mFloatingDockFrameless.h"
#include "mFramelessWindowHelper.h"
#include "mWindowTitleBar.h"

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include "DockManager.h"
#include "FloatingDockContainer.h"

#include <QApplication>
#include <QBoxLayout>
#include <QPointer>

namespace MechMainWindow
{
	namespace
	{
		/*
		* 浮动容器标题栏拖拽委托：把 mWindowTitleBar 的按下/拖动/松开转发到
		* qtads 公开的拖拽状态机（startDragging/moveFloating/finishDragging）。
		* moveFloating/finishDragging 在 CFloatingDockContainer 里是 protected，
		* 经公开接口 IFloatingWidget 调用（与 qtads Linux 自绘标题栏同一条链路，
		* moveFloating 内部自带拖回停靠 drop overlay 的更新）。
		*/
		class mFloatingDragDelegate
			: public QObject
			, public mTitleBarDragDelegate
		{
		public:
			mFloatingDragDelegate(ads::CFloatingDockContainer* floating, QObject* parent)
				: QObject(parent), m_floating(floating)
			{
			}

			void beginCaptionDrag(const QPoint& globalPos, QWidget* eventHandler) override
			{
				if (!m_floating)
					return;
				// 按下即进入 DraggingFloatingWidget 态：qtads 会 grab eventHandler
				// 的鼠标，拖动期间移出标题栏也能持续收到 move/release
				const QPoint offset = globalPos - m_floating->frameGeometry().topLeft();
				m_floating->startDragging(offset, m_floating->size(), eventHandler);
			}

			void moveCaptionDrag(const QPoint&) override
			{
				if (!m_floating)
					return;
				ads::IFloatingWidget* fw = m_floating.data();
				fw->moveFloating();
			}

			void endCaptionDrag() override
			{
				if (!m_floating)
					return;
				ads::IFloatingWidget* fw = m_floating.data();
				// 拖回停靠成功时 qtads 会隐藏并延迟销毁浮动容器（标题栏随之析构），
				// 之后不再触碰本委托
				fw->finishDragging();
			}

		private:
			QPointer<ads::CFloatingDockContainer> m_floating;
		};
	}

	mFloatingDockFrameless* mFloatingDockFrameless::_instance = nullptr;

	void mFloatingDockFrameless::attach(ads::CDockManager* manager)
	{
		if (!manager)
			return;
		if (!_instance)
		{
			_instance = new mFloatingDockFrameless;
			// 应用级 native 过滤：浮动容器是 qtads 的类，nativeEvent 无法覆写，
			// 只能在 Qt 分发前拦截框架消息转发给 FramelessWindowHelper
			qApp->installNativeEventFilter(_instance);
		}

		// 此后拖出/恢复布局产生的浮动容器统一注入（floatingWidgetCreated 发出时
		// 窗口尚未 show，改窗口标志无句柄重建闪烁）
		QObject::connect(manager, &ads::CDockManager::floatingWidgetCreated,
			_instance, [inst = _instance](ads::CFloatingDockContainer* floating)
		{
			inst->applyTo(floating);
		});

		// attach 晚于首个浮动容器时兜底补注入（幂等）
		for (ads::CFloatingDockContainer* floating : manager->floatingWidgets())
			_instance->applyTo(floating);
	}

	mFloatingDockFrameless::~mFloatingDockFrameless()
	{
		if (_instance == this)
			_instance = nullptr;
	}

	void mFloatingDockFrameless::applyTo(ads::CFloatingDockContainer* floating)
	{
		if (!floating || _windows.contains(floating))
			return;

		// 保留原生框架（WS_THICKFRAME/WS_CAPTION），非客户区由 native filter 清零
		FramelessWindowHelper::installFrameless(floating);

		// 自绘标题栏插到浮动容器布局首行（qtads 构造时建立的 QBoxLayout）
		auto* bar = new mWindowTitleBar(floating);
		if (auto* box = qobject_cast<QBoxLayout*>(floating->layout()))
			box->insertWidget(0, bar);
		else
			bar->setParent(floating);	// 兜底：非盒布局时不悬空（正常不会发生）

		// 拖动转发 qtads 拖拽状态机（拖回停靠可用）；委托以浮动容器为 parent 随之销毁
		auto* delegate = new mFloatingDragDelegate(floating, floating);
		bar->setDragDelegate(delegate);

		// 标题/图标/激活态同步：浮动容器是 qtads 的类，changeEvent 不会调
		// updateWindowState，这里经事件过滤器代发；show 时标题栏也会自检主题
		floating->installEventFilter(this);
		bar->updateWindowState();

		_windows.insert(floating);
		QObject::connect(floating, &QObject::destroyed, this, [this, floating]()
		{
			_windows.remove(floating);
		});
	}

	bool mFloatingDockFrameless::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
	{
#ifdef Q_OS_WIN
		if (eventType != QByteArrayLiteral("windows_generic_MSG"))
			return false;
		if (_windows.isEmpty())
			return false;

		// 只转发框架相关消息；qtads 自身依赖的 WM_NCLBUTTONDOWN/WM_MOVING/
		// WM_EXITSIZEMOVE 等不拦截，其 nativeEvent 照常收到
		MSG* msg = static_cast<MSG*>(message);
		switch (msg->message)
		{
		case WM_NCCALCSIZE:
		case WM_NCHITTEST:
		case WM_GETMINMAXINFO:
		case WM_SHOWWINDOW:
		case WM_SIZE:
			break;
		default:
			return false;
		}

		QWidget* wnd = QWidget::find(reinterpret_cast<WId>(msg->hwnd));
		if (!wnd || !_windows.contains(wnd))
			return false;
		return FramelessWindowHelper::handleNativeEvent(wnd, message, result);
#else
		Q_UNUSED(eventType)
		Q_UNUSED(message)
		Q_UNUSED(result)
		return false;
#endif
	}

	bool mFloatingDockFrameless::eventFilter(QObject* watched, QEvent* event)
	{
		auto* wnd = qobject_cast<QWidget*>(watched);
		if (wnd && _windows.contains(wnd))
		{
			switch (event->type())
			{
			case QEvent::WindowTitleChange:
			case QEvent::WindowIconChange:
			case QEvent::ActivationChange:
			case QEvent::WindowStateChange:
			case QEvent::Show:
				if (mWindowTitleBar* bar = wnd->findChild<mWindowTitleBar*>())
					bar->updateWindowState();
				break;
			default:
				break;
			}
		}
		return QObject::eventFilter(watched, event);
	}
}
