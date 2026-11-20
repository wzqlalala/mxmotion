#include "mFramelessDialog.h"

#include "mFramelessWindowHelper.h"
#include "mWindowTitleBar.h"
#include "ThemeManager.h"
#include "UiPalette.h"

#include <QPainter>
#include <QVBoxLayout>
#include <QEvent>

namespace MechMainWindow
{
	mFramelessDialog::mFramelessDialog(QWidget* parent)
		: QDialog(parent)
	{
		// 保留原生框架（阴影/拖拽动画/Snap 由系统提供），非客户区清零见
		// FramelessWindowHelper::handleNativeEvent
		FramelessWindowHelper::installFrameless(this);

		m_titleBar = new mWindowTitleBar(this);
		// 标题栏自给自足：构造即完整（配色/尺寸/悬浮态），并自检档位/主题变化，
		// 无需本对话框或 ThemeManager 再补发

		m_contentLay = new QVBoxLayout(this);
		// 四周留 1px 给 paintEvent 的窗口外框线，子控件不覆盖边线
		m_contentLay->setContentsMargins(1, 1, 1, 1);
		m_contentLay->setSpacing(0);
		m_contentLay->addWidget(m_titleBar);
		setLayout(m_contentLay);
	}
	mFramelessDialog::~mFramelessDialog() = default;

	void mFramelessDialog::setTitleBarSeparatorVisible(bool on)
	{
		if (m_titleBar)
			m_titleBar->setSeparatorVisible(on);
	}

	void mFramelessDialog::setTitleBarCloseOnly(bool on)
	{
		if (m_titleBar)
			m_titleBar->setCloseOnly(on);
	}

	void mFramelessDialog::setTitleBarHeight(int h)
	{
		if (m_titleBar)
			m_titleBar->setBarHeight(h);
	}

	void mFramelessDialog::setTitleBarIconSize(const QSize& size)
	{
		if (m_titleBar)
			m_titleBar->setTitleIconSize(size);
	}

	void mFramelessDialog::setTitleBarFont(int pixelSize, int weight)
	{
		if (m_titleBar)
			m_titleBar->setTitleFont(pixelSize, weight);
	}

	void mFramelessDialog::setTitleBarLayoutCompact(int iconLeft, int iconTextGap)
	{
		if (m_titleBar)
			m_titleBar->setTitleLayoutCompact(iconLeft, iconTextGap);
	}

	void mFramelessDialog::setTitleBarDialogFlatClose(bool on)
	{
		if (m_titleBar)
			m_titleBar->setDialogFlatClose(on);
	}

	void mFramelessDialog::paintEvent(QPaintEvent* event)
	{
		QDialog::paintEvent(event);
		// 窗口外框 1px（规则 2 边线）：标题栏底色 softwareTitleBar 与内容 surface
		// 同值，无边线时整窗是一块同色平板（浅色主题下即"悬浮白板"）；
		// 主题切换时由 mWindowTitleBar::applyColors 触发本窗重画换色
		QPainter p(this);
		p.setPen(MxFunctions::UiPalette::border());
		p.drawRect(rect().adjusted(0, 0, -1, -1));
	}

	void mFramelessDialog::setContentWidget(QWidget* content)
	{
		if (!content) return;
		m_contentLay->addWidget(content);
	}

	QWidget* mFramelessDialog::setExternalLayout(QLayout* lay, bool applyTheme)
	{
		if (!lay) return nullptr;
		// 外部布局必须尚未挂到部件上（裸 new），否则 setLayout 会被 Qt 拒绝
		QWidget* content = new QWidget(this);
		content->setLayout(lay);
		setContentWidget(content);
		if (applyTheme)
		{
			// 内容底色/文字/按钮按当前主题（与首选项同链路；标题栏自绘不受影响）
			ThemeManager::instance()->applyDialogTheme(content);
		}
		return content;
	}

	bool mFramelessDialog::nativeEvent(const QByteArray& eventType, void* message, long* result)
	{
#ifdef Q_OS_WIN
		if (FramelessWindowHelper::handleNativeEvent(this, message, result))
			return true;
#endif
		return QDialog::nativeEvent(eventType, message, result);
	}

	void mFramelessDialog::changeEvent(QEvent* event)
	{
		// 窗口状态/标题/图标变化 → 同步自绘标题栏（最大化/还原图标、
		// 激活态颜色、标题文字）
		if (event->type() == QEvent::WindowStateChange
			|| event->type() == QEvent::ActivationChange
			|| event->type() == QEvent::WindowTitleChange
			|| event->type() == QEvent::WindowIconChange)
		{
			if (m_titleBar)
				m_titleBar->updateWindowState();
		}
		QDialog::changeEvent(event);
	}
}
