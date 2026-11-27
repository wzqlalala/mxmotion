#include "mFramelessWindowHelper.h"

#ifdef Q_OS_WIN

// 本 .cpp 必须在包含任何 Qt 头之前包含 windows.h，避免 Qt5
// qwindowdefs_win.h 的 tagMSG/Q_WINSTRICT 冲突屏蔽 Win32 类型定义
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include "UiPalette.h"
#include "mWindowTitleBar.h"

#include <QAbstractButton>
#include <QAbstractNativeEventFilter>
#include <QApplication>
#include <QBoxLayout>
#include <QOperatingSystemVersion>
#include <QSet>
#include <QTimer>
#include <QVariant>
#include <QWidget>
#include <QWindow>

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

namespace MechMainWindow
{
	namespace FramelessWindowHelper
	{
		namespace
		{
			// 最大化时非客户区清零会把系统预留的顶部遮挡条（≈外扩边框量）一并抹掉，
			// 自绘标题栏内容随之贴到屏幕顶边被裁。遮挡量随 DPI/系统设置变化，
			// 不能写死——用 SM_CYSIZEFRAME + SM_CXPADDEDBORDER 由系统 API 取。
			//
			// 兼容性：GetSystemMetricsForDpi 是 Win10 1607 新增 API，静态导入会让
			// 程序在 Win7 启动即失败（找不到输入点），必须 GetProcAddress 动态解析；
			// Win7 只有系统级 DPI，回退到普通 GetSystemMetrics 返回值即正确。
			// 启用 Qt 官方 Windows OpenGL 全屏/最大化边框保护（QTBUG-41309 / QTBUG-50271）：
			// 当宿主内嵌 QOpenGLWidget（如三维视口）且通过 WM_NCCALCSIZE 清零原生边框时，
			// 部分显卡驱动（Intel/NVIDIA Optimus/AMD）在窗口最大化后会把 WGL 交换链提升为
			// 独占/独立翻转（Fullscreen / Independent Flip）模式，导致所有 WS_POPUP | WS_EX_LAYERED
			// 子弹窗（如“文件”下拉菜单、模型树右键菜单、组合框下拉列表）被压在 OpenGL 图层下方无法显示，
			// 还原窗口后又恢复正常。向 qwindows 平台插件显式开启 HasBorderInFullScreen 可阻断该提升。
			void ensureOpenGlPopupSafeMaximized(QWidget* wnd)
			{
				if (!wnd) return;
				using SetBorderDefaultFn = void (*)(bool);
				static const bool s_defaultApplied = []() {
					if (auto fn = reinterpret_cast<SetBorderDefaultFn>(
							QGuiApplication::platformFunction(QByteArrayLiteral("WindowsSetHasBorderInFullScreenDefault"))))
					{
						fn(true);
					}
					return true;
				}();
				Q_UNUSED(s_defaultApplied);
				if (QWindow* wh = wnd->windowHandle())
				{
					using SetBorderFn = void (*)(QWindow*, bool);
					if (auto fn = reinterpret_cast<SetBorderFn>(
							QGuiApplication::platformFunction(QByteArrayLiteral("WindowsSetHasBorderInFullScreen"))))
					{
						fn(wh, true);
					}
				}
			}

			/*
			* Win7：保留 WS_CAPTION 时，DWM 会在鼠标悬停窗口右上角把原生的
			* 最小化/最大化/关闭按钮重绘出来——WM_NCCALCSIZE 清零只隐藏常态
			* 外观，按钮悬停热区仍由系统命中，悬停/激活即浮现原生按钮
			*（Win8+ 的 DWM 尊重清零结果，无此问题）。
			* 根治：Win7 上把 WS_CAPTION 从原生窗口样式剥掉，原生按钮随样式
			* 消失、不可能再被重绘；保留 WS_THICKFRAME/WS_SYSMENU/
			* WS_MINIMIZEBOX/WS_MAXIMIZEBOX——边缘缩放、系统菜单（Alt+Space/
			* 图标单击）、任务栏最小化与 DWM 阴影（配合已声明的
			* DwmExtendFrameIntoClientArea）全部不变。
			* 原生样式随 HWND 创建时按 Qt flags 生成（setWindowFlags 触发句柄
			* 重建后会回到带 CAPTION 的样式），故在 NCCALCSIZE 里幂等补剥一次。
			*/
			void stripNativeCaptionForWin7(HWND hwnd)
			{
				if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8)
					return;
				const LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
				if (style & WS_CAPTION)
					SetWindowLongPtrW(hwnd, GWL_STYLE, style & ~LONG_PTR(WS_CAPTION));
			}
		}
		void installFrameless(QWidget* window)
		{
			if (!window) return;
			// 保留原生窗口框架（WS_THICKFRAME/WS_CAPTION），仅经 nativeEvent 的
			// WM_NCCALCSIZE 把非客户区清零去掉原生标题栏。框架仍在，DWM 阴影、
			// 平滑边框缩放与 Aero Snap/最大化/还原动画都由系统原生提供。
			// 必须在窗口 show 之前调用：窗口标志在显示后动态切换会触发句柄重建/闪烁。
			window->setWindowFlags(Qt::Window
				| Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
		}

		bool handleNativeEvent(QWidget* wnd, void* message, long* result)
		{
			MSG* msg = static_cast<MSG*>(message);

		// 非客户区清零：标题栏/边框全部客户区自绘（Qt 部件绘制，与内容
		// 同一渲染管线，无 DWM 合成冲突，拖拽不闪不掉线）。
		// 但最大化时必须保留系统预留的顶部遮挡条（≈外扩边框量）：Windows 最大化
		// 会把窗口向屏幕外扩出边框宽，顶部那几像素被屏幕边裁掉。若不清零后
		// 仍把 rgrc[0] 四边各内缩相同像素，自绘标题栏与状态栏会贴到屏幕边被裁掉。
		// Win7 在此顺带剥掉 WS_CAPTION（见 stripNativeCaptionForWin7）：
		// 否则悬停右上角时 DWM 会把原生最小化/最大化/关闭按钮重绘出来。
		if (msg->message == WM_NCCALCSIZE)
		{
			stripNativeCaptionForWin7(msg->hwnd);
			if (msg->wParam != TRUE)
				return false;
			NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(msg->lParam);
			// 注意：必须以 Win32 实时状态 IsZoomed(msg->hwnd) 判定最大化，而不能仅看 wnd->isMaximized()
			//（Win+Up / 双击标题栏 / ShowWindow(SW_MAXIMIZE) 触发的首个 WM_NCCALCSIZE 早于 WM_SIZE，
			//  此时 Qt 的 isMaximized() 可能尚未同步）。
			// 最大化时直接将客户区矩形对齐到所属显示器的工作区 mi.rcWork（彻底消除高 DPI 与多屏下
			// WM_GETMINMAXINFO + overlap 双重扣减造成的坐标偏移与边缘死区），并在底边预留 1 物理像素，
			// 确保即使在“自动隐藏任务栏”（rcWork == rcMonitor）或不同显卡 WDDM 驱动下，
			// 最大化 OpenGL 窗口也绝不会被驱动误判为全屏独占表面而吞掉 QMenu 菜单弹窗。
			if (::IsZoomed(msg->hwnd))
			{
				if (const HMONITOR hMon = MonitorFromWindow(msg->hwnd, MONITOR_DEFAULTTONEAREST))
				{
					MONITORINFO mi{};
					mi.cbSize = sizeof(mi);
					if (GetMonitorInfoW(hMon, &mi))
					{
						params->rgrc[0] = mi.rcWork;
						if (params->rgrc[0].bottom > params->rgrc[0].top + 2)
							params->rgrc[0].bottom -= 1;
					}
				}
			}
			*result = 0;
			return true;
		}

		// 窗口每次显示/最大化还原时都向 DWM 声明框架延伸：非客户区清零后
		// DWM 默认不画阴影，MARGINS{0,0,0,1} 只触发阴影不产生可见玻璃条。
		// 注意：不能用 static 一次性标志——多个窗口各自都要声明，且最大化
		// 还原后 DWM 可能重置阴影状态，需重复声明
		if (msg->message == WM_SHOWWINDOW && msg->wParam == TRUE)
		{
			ensureOpenGlPopupSafeMaximized(wnd);
			MARGINS m{ 0, 0, 0, 1 };
			DwmExtendFrameIntoClientArea(msg->hwnd, &m);

			if (wnd && !wnd->isMaximized() && !wnd->isFullScreen() && !::IsZoomed(msg->hwnd))
			{
				const QSize declared = wnd->minimumSize() == wnd->maximumSize()
					? wnd->minimumSize() : QSize();
				const auto resyncClientSize = [wnd, declared]()
				{
					if (!wnd || !wnd->isVisible() || wnd->isMaximized() || wnd->isFullScreen())
						return;
					const HWND hwnd = reinterpret_cast<HWND>(wnd->winId());
					if (::IsZoomed(hwnd))
						return;
					const qreal dpr = wnd->devicePixelRatioF();
					const int targetW = qRound((declared.isValid() ? declared.width()
						: wnd->width()) * dpr);
					const int targetH = qRound((declared.isValid() ? declared.height()
						: wnd->height()) * dpr);
					RECT crc{};
					if (!GetClientRect(hwnd, &crc))
						return;
					if ((crc.right - crc.left) == targetW && (crc.bottom - crc.top) == targetH)
						return;
					RECT wrc{};
					GetWindowRect(hwnd, &wrc);
					SetWindowPos(hwnd, nullptr, wrc.left, wrc.top, targetW, targetH,
						SWP_NOZORDER | SWP_NOACTIVATE);
				};

				resyncClientSize();
				QTimer::singleShot(0, wnd, resyncClientSize);
			}
		}
			if (msg->message == WM_SIZE && (msg->wParam == SIZE_RESTORED || msg->wParam == SIZE_MAXIMIZED))
			{
				ensureOpenGlPopupSafeMaximized(wnd);
				MARGINS m{ 0, 0, 0, 1 };
				DwmExtendFrameIntoClientArea(msg->hwnd, &m);
			}

		if (msg->message == WM_NCHITTEST && wnd && !wnd->isMaximized() && !wnd->isFullScreen() && !::IsZoomed(msg->hwnd))
		{
			// 固定尺寸窗（min==max，提示框等）：无缩放形态，不提供边缘缩放热区
			const bool fixedForm = wnd->minimumSize() == wnd->maximumSize()
				&& !wnd->minimumSize().isNull();
			if (fixedForm)
				return false;
			// lParam 是物理像素，窗口矩形必须同样取物理像素的 GetWindowRect；
			// 混用 QWidget::frameGeometry（逻辑像素）在高 DPI 缩放下命中带会错位
			RECT rc{};
			GetWindowRect(msg->hwnd, &rc);
			const long x = GET_X_LPARAM(msg->lParam);
			const long y = GET_Y_LPARAM(msg->lParam);
			const int border = qMax(4, GetSystemMetrics(SM_CXSIZEFRAME));
			const bool left = x < rc.left + border;
			const bool right = x >= rc.right - border;
			const bool top = y < rc.top + border;
			const bool bottom = y >= rc.bottom - border;
			if (left || right || top || bottom)
			{
				// 控件命中查询走 Qt 逻辑坐标：物理像素按窗口所在屏 DPR 折算
				const qreal dpr = wnd->devicePixelRatioF();
				const QPoint pos(qRound(x / dpr), qRound(y / dpr));
				// 标题栏贴着上边缘：压到按钮/图标区（isInCaptionArea 的补集）时不抢，
				// 图标单击是系统菜单、按钮是 min/max/close，都必须可点
				if (mWindowTitleBar* bar = wnd->findChild<mWindowTitleBar*>())
				{
					if (bar->rect().contains(bar->mapFromGlobal(pos)) && !bar->isInCaptionArea(pos))
						return false;
				}
				// 通用兜底：命中带压在任何按钮上都交还客户区（状态栏/停靠区边缘按钮）
				for (QWidget* w = wnd->childAt(wnd->mapFromGlobal(pos)); w && w != wnd; w = w->parentWidget())
				{
					if (qobject_cast<QAbstractButton*>(w))
						return false;
				}
				if (left && top) { *result = HTTOPLEFT; return true; }
				if (right && top) { *result = HTTOPRIGHT; return true; }
				if (left && bottom) { *result = HTBOTTOMLEFT; return true; }
				if (right && bottom) { *result = HTBOTTOMRIGHT; return true; }
				if (left) { *result = HTLEFT; return true; }
				if (right) { *result = HTRIGHT; return true; }
				if (top) { *result = HTTOP; return true; }
				if (bottom) { *result = HTBOTTOM; return true; }
			}
		}

			// 固定尺寸弹窗（min==max，如提示框/固定对话框）：解除 Qt 默认附加原生边框后的
			// ptMinTrackSize 钳制，使 WM_SHOWWINDOW 中的 resyncClientSize 能将窗口精准收拢到声明尺寸。
			// 可缩放主窗口不在此拦截 WM_GETMINMAXINFO：由系统与 Qt 原生维护最大化外框与高 DPI 最小尺寸，
			// 再由上方 WM_NCCALCSIZE(IsZoomed) 一步把最大化客户区精准映射到 mi.rcWork，避免双重扣减与全屏误判。
			if (msg->message == WM_GETMINMAXINFO && wnd)
			{
				const bool fixedForm = wnd->minimumSize() == wnd->maximumSize()
					&& !wnd->minimumSize().isNull();
				if (fixedForm)
				{
					MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(msg->lParam);
					const qreal dpr = wnd->devicePixelRatioF();
					mmi->ptMinTrackSize.x = qRound(wnd->minimumWidth() * dpr);
					mmi->ptMinTrackSize.y = qRound(wnd->minimumHeight() * dpr);
					*result = 0;
					return true;
				}
			}

			return false;
		}

		/*
		* 动态标题栏装配（installTitleBarChrome/uninstallTitleBarChrome）已删除
		* （2026-11-14）：应用级 native filter 依赖 QWidget::find 反查窗口，而
		* WM_NCCALCSIZE 在 CreateWindowEx 期间同步发出时 HWND 尚未进入 Qt 映射表，
		* filter 全部落空——原生标题栏残留与自绘栏并存成"双标题栏"。动态弹出的
		* 面板请装进 mFramelessDialog（类级 nativeEvent，与首选项同链路）。
		*/
	}
}

#else // !Q_OS_WIN —— Linux/macOS：Qt 通用无边框实现
/*
* 非 Windows 平台没有 WM_NCCALCSIZE/DWM 机制：去掉系统标题栏只能整体
* FramelessWindowHint，代价是 WM 不再提供阴影与边缘缩放热区。
* 阴影缺失与 Win7 Basic 主题同状，由 mUiMenu/mFramelessDialog 的 1px
* border() 描边兜底；边缘缩放由 LinuxEdgeResizer 手动拖缩补齐。
* 对外接口与 Win 分支同名同语义，调用方（mFramelessDialog/
* mFloatingDockFrameless/mMechMainWindow）无需平台判断。
*/

#include "mWindowTitleBar.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBoxLayout>
#include <QEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QSet>
#include <QVariant>
#include <QWidget>

namespace MechMainWindow
{
	namespace FramelessWindowHelper
	{
		namespace
		{
			/*
			* Linux 边缘缩放器：FramelessWindowHint 下窗口管理器不再提供边框
			* 缩放热区（Qt5.13 也无 QWidget::startSystemResize），用应用级事件
			* 过滤器在按下点落入登记窗口边缘带时接管，grabMouse 后手动拖缩。
			* 让位规则与 Win 分支 WM_NCHITTEST 同口径：
			*   - 命中带 5px，压到按钮（QAbstractButton：状态栏/停靠区/标题栏
			*     按钮都贴窗边）或自绘标题栏（caption 是拖动/双击最大化区）时
			*     直接不抢，点击仍归客户区控件；
			*   - 接入窗口顶部都有 mWindowTitleBar，顶边实际只在其外才生效；
			*   - 最大化/全屏不允许缩放（与原生一致）。
			*/
			class LinuxEdgeResizer : public QObject
			{
			public:
				static LinuxEdgeResizer* instance()
				{
					static LinuxEdgeResizer s_filter;
					return &s_filter;
				}

				void add(QWidget* w)
				{
					if (!w) return;
					if (m_windows.isEmpty())
						qApp->installEventFilter(this);
					m_windows.insert(w);
					// 窗口销毁兜底：拖缩中途窗口被删（如浮动容器拖回停靠即销毁）
					QObject::connect(w, &QObject::destroyed, this, [this, w]() {
						m_windows.remove(w);
						m_resizing = nullptr;
						m_edge = 0;
					});
				}

				void remove(QWidget* w)
				{
					if (!w) return;
					if (m_resizing == w)
					{
						w->releaseMouse();
						m_resizing = nullptr;
						m_edge = 0;
					}
					m_windows.remove(w);
					if (m_windows.isEmpty())
						qApp->removeEventFilter(this);
				}

				bool eventFilter(QObject* obj, QEvent* ev) override
				{
					if (m_windows.isEmpty())
						return QObject::eventFilter(obj, ev);

					// 拖缩中：吞掉鼠标移动/释放，按位移改窗口几何
					//（grabMouse 保证移出窗口后事件仍投递到本窗口）
					if (m_resizing)
					{
						if (ev->type() == QEvent::MouseMove)
						{
							applyResize(m_resizing.data(), static_cast<QMouseEvent*>(ev)->globalPos());
							return true;
						}
						if (ev->type() == QEvent::MouseButtonRelease
							&& static_cast<QMouseEvent*>(ev)->button() == Qt::LeftButton)
						{
							m_resizing->releaseMouse();
							m_resizing = nullptr;
							m_edge = 0;
							return true;
						}
						return QObject::eventFilter(obj, ev);
					}

					if (ev->type() != QEvent::MouseButtonPress && ev->type() != QEvent::MouseMove)
						return QObject::eventFilter(obj, ev);
					QWidget* wnd = qobject_cast<QWidget*>(obj);
					if (!wnd) return QObject::eventFilter(obj, ev);
					QWidget* top = wnd->window();
					if (!top || !m_windows.contains(top))
						return QObject::eventFilter(obj, ev);

					auto* me = static_cast<QMouseEvent*>(ev);
					if (me->button() != Qt::NoButton && me->button() != Qt::LeftButton)
						return QObject::eventFilter(obj, ev);

					const int edge = edgeHit(top, me->globalPos());
					if (ev->type() == QEvent::MouseMove)
					{
						// 悬停光标提示：命中带内给缩放光标，离开还原。
						// 只设窗口级光标，子控件自带光标优先级更高不受影响
						if (edge)
							top->setCursor(cursorFor(edge));
						else if (top->cursor().shape() != Qt::ArrowCursor)
							top->unsetCursor();
						return QObject::eventFilter(obj, ev);
					}
					if (edge && !top->isMaximized() && !top->isFullScreen())
					{
						m_resizing = top;
						m_edge = edge;
						m_pressGlobal = me->globalPos();
						m_pressGeo = top->frameGeometry();
						top->grabMouse();
						return true;   // 吞掉按下，开始手动拖缩
					}
					return QObject::eventFilter(obj, ev);
				}

			private:
				enum Hit
				{
					kLeft = 1, kRight = 2, kTop = 4, kBottom = 8
				};

				// 边缘命中：返回方向位组合，0=不在命中带或需让位给交互控件
				static int edgeHit(QWidget* top, const QPoint& globalPos)
				{
					const QPoint pos = top->mapFromGlobal(globalPos);
					constexpr int band = 5;
					const bool left = pos.x() < band;
					const bool right = pos.x() >= top->width() - band;
					const bool bottom = pos.y() >= top->height() - band;
					// 顶边让位给自绘标题栏（caption 拖动/双击最大化），无标题栏的窗口才开放
					const bool topEdge = pos.y() < band
						&& top->findChild<mWindowTitleBar*>() == nullptr;
					if (!left && !right && !bottom && !topEdge)
						return 0;
					for (QWidget* w = top->childAt(pos); w && w != top; w = w->parentWidget())
					{
						if (qobject_cast<QAbstractButton*>(w))
							return 0;
						if (w->objectName() == QStringLiteral("mWindowTitleBar"))
							return 0;
					}
					int e = 0;
					if (left) e |= kLeft;
					if (right) e |= kRight;
					if (topEdge) e |= kTop;
					if (bottom) e |= kBottom;
					return e;
				}

				static QCursor cursorFor(int edge)
				{
					const bool h = edge & (kLeft | kRight);
					const bool v = edge & (kTop | kBottom);
					if (h && v)
						return QCursor((edge & (kLeft | kBottom)) ? Qt::SizeBDiagCursor : Qt::SizeFDiagCursor);
					if (h) return QCursor(Qt::SizeHorCursor);
					return QCursor(Qt::SizeVerCursor);
				}

				void applyResize(QWidget* top, const QPoint& globalPos)
				{
					const QPoint delta = globalPos - m_pressGlobal;
					QRect g = m_pressGeo;
					const int minW = qMax(1, top->minimumWidth());
					const int minH = qMax(1, top->minimumHeight());
					if (m_edge & kLeft)
						g.setLeft(qMin(g.left() + delta.x(), g.right() + 1 - minW));
					if (m_edge & kRight)
						g.setRight(qMax(g.right() + delta.x(), g.left() + minW - 1));
					if (m_edge & kTop)
						g.setTop(qMin(g.top() + delta.y(), g.bottom() + 1 - minH));
					if (m_edge & kBottom)
						g.setBottom(qMax(g.bottom() + delta.y(), g.top() + minH - 1));
					if (top->maximumWidth() < QWIDGETSIZE_MAX)
						g.setWidth(qMin(g.width(), top->maximumWidth()));
					if (top->maximumHeight() < QWIDGETSIZE_MAX)
						g.setHeight(qMin(g.height(), top->maximumHeight()));
					top->setGeometry(g);
				}

				QSet<QWidget*> m_windows;
				QPointer<QWidget> m_resizing;
				int m_edge = 0;
				QPoint m_pressGlobal;
				QRect m_pressGeo;
			};
		}

		void installFrameless(QWidget* window)
		{
			if (!window) return;
			// 必须在窗口 show 之前调用（与 Win 分支同约定：显示后切窗口标志会
			// 触发句柄重建/闪烁）。FramelessWindowHint 去掉系统标题栏后，
			// 自绘 mWindowTitleBar 是唯一标题栏；边缘缩放由 LinuxEdgeResizer 补齐
			Qt::WindowFlags flags = window->windowFlags();
			flags |= Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint
				| Qt::WindowCloseButtonHint;
			window->setWindowFlags(flags | Qt::FramelessWindowHint);
			LinuxEdgeResizer::instance()->add(window);
		}

		bool handleNativeEvent(QWidget*, void*, long*)
		{
			// 非 Windows 无 Win32 消息，窗口框架行为全部由 Qt/WM 处理
			return false;
		}
	}
}

#endif // Q_OS_WIN
