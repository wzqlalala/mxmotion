#include "mWindowTitleBar.h"

#include "UiPalette.h"
#include "UiMetrics.h"

#include <QApplication>
#include <QGuiApplication>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShowEvent>
#include <QStyle>
#include <QWindow>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <windowsx.h>
#endif

using namespace MxFunctions;

namespace MechMainWindow
{
	namespace
	{
		// 规则 2：窗口头按钮 headerControl = H−2（标准 24×24，16px 图形），与顶部快捷按钮同规格；
		// 标题栏关闭/最小化/最大化按钮保持系统习惯的宽条形（宽≈高×1.9），高度=窗口头高
		int captionButtonWidth()
		{
			return (UiMetrics::windowHeaderControl() * 19 + 5) / 10; // 24→46 标准档
		}

		// 规则 2：窗口头高 header = H+2（标准 28），树/参数/自由弹窗/顶部切换条共用
		int titleBarHeight()
		{
			return UiMetrics::windowHeaderHeight();
		}

		QString iconFontFamily()
		{
			// Windows 10+ 的 MDL2 图标字体，字形与原生标题栏按钮一致
			return QStringLiteral("Segoe MDL2 Assets");
		}

		// MDL2 字体可用性（探测一次缓存）：Win7/Linux 未随系统发行该字体，
		// 直接按字形绘制会得到整片豆腐块，必须回退到矢量绘制
		bool hasIconFont()
		{
			static const bool ok = QFontDatabase().families()
				.contains(iconFontFamily(), Qt::CaseInsensitive);
			return ok;
		}

		// 无 MDL2 字体时的矢量回退图形种类（与 mCaptionButton::Role 一一对应）
		enum class CaptionGlyph { Min, Max, Restore, Close };

		/*
		* Win10 原生字形复刻（Segoe MDL2 Assets 的 ChromeMinimize E921 /
		* ChromeMaximize E922 / ChromeRestore E923 / ChromeClose E8BB）：
		* 几何与 Windows Terminal 标题栏的 10×10 基准矢量完全同构——
		*   Min     底边 10×1 横线；Max 10×10 方框描边；
		*   Restore 右上后框(2.5,0.5,7,7)+左下前框(0.5,2.5,7,7)；
		*   Close   对角 X，平端笔帽。
		* 字盒边长与 MDL2 字号同口径（glyphBox×10/16），线宽 1px@10 号等比缩放，
		* Win7/Linux 无该字体时观感与 Win10 字形渲染一致。
		*/
		void drawVectorGlyph(QPainter& p, CaptionGlyph glyph, const QRect& rect,
			int glyphBox, const QColor& fg)
		{
			const qreal f = qMax(8, glyphBox * 10 / 16);
			const qreal t = qMax<qreal>(1.0, f / 10.0);
			const qreal x0 = rect.center().x() - f / 2.0;
			const qreal y0 = rect.center().y() - f / 2.0;
			p.setRenderHint(QPainter::Antialiasing, true);
			const QPen pen(fg, t, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
			p.setBrush(Qt::NoBrush);
			switch (glyph)
			{
			case CaptionGlyph::Min:
				// 底边横线（MDL2 ChromeMinimize 位于字格底部）
				p.setPen(Qt::NoPen);
				p.setBrush(fg);
				p.drawRect(QRectF(x0, y0 + f - t, f, t));
				break;
			case CaptionGlyph::Max:
				// 方框：描边含在 f×f 字盒内
				p.setPen(pen);
				p.drawRect(QRectF(x0 + t / 2.0, y0 + t / 2.0, f - t, f - t));
				break;
			case CaptionGlyph::Restore:
				// 双框：后框右上、前框左下，偏移 1/4 字盒、边长 7/10 字盒
				p.setPen(pen);
				p.drawRect(QRectF(x0 + 0.25 * f, y0 + t / 2.0, 0.7 * f, 0.7 * f));
				p.drawRect(QRectF(x0 + t / 2.0, y0 + 0.25 * f, 0.7 * f, 0.7 * f));
				break;
			case CaptionGlyph::Close:
				p.setPen(pen);
				p.drawLine(QPointF(x0, y0), QPointF(x0 + f, y0 + f));
				p.drawLine(QPointF(x0 + f, y0), QPointF(x0, y0 + f));
				break;
			}
		}
	}

	/*
	* 标题栏自绘按钮（mCaptionButton）
	* 彻底绕开 QToolButton/QSS：paintEvent 用 QPainter 直接画背景态与 MDL2 字形，
	* 颜色全部取 UiPalette。因此主窗/对话框任何位置悬浮、按下、红底白图标完全一致，
	* 不受全局 QSS（mApplicationStart.qss 的 QToolButton 白底规则）干扰。
	* 例外：弹窗族档（setDialogFlatStyle）关闭键改画 close3x 资源图标
	*（与已验收弹窗同源同图），hover 淡灰圆角，见该函数注记。
	*/
	class mCaptionButton : public QAbstractButton
	{
	public:
		enum class Role { Min, Max, Close };

		mCaptionButton(Role role, QWidget* parent = nullptr)
			: QAbstractButton(parent), m_role(role)
		{
			setCursor(Qt::ArrowCursor);
			setFocusPolicy(Qt::NoFocus);
			// 关键：开启悬停跟踪 + WA_Hover，否则鼠标移入/移出不触发重绘，
			// 悬浮态要靠按下才出现、且移走后不恢复（表现为"悬浮不变色、按下才变、不还原"）
			setMouseTracking(true);
			setAttribute(Qt::WA_Hover, true);
			// 尺寸由 applyMetrics 统一给
		}

		void setHostMaximized(bool maxd) { m_hostMax = maxd; update(); }

		/*
		* 弹窗族档（白卡约定，2026-11-09 用户裁定对齐）：关闭键=close3x 图标
		* 24×24、透明底、hover 淡灰 #EFF1F2 圆角 2、按下不变色——照抄已验收
		* 先例 mChannelMapDlg/mLayerManageDlg 的 QToolButton 形态（图标右缘距
		* 窗右 20 由 mWindowTitleBar::setDialogFlatClose 布局右衬配合）。
		* 红底白图标是主窗标题栏档，弹窗族不用——此前 46 宽条红底即用户指出
		* 的"与其它弹窗不一致"。#EFF1F2 为先例 QSS 硬编码原值，三主题同源。
		* 尺寸不在此定：由宿主 mWindowTitleBar::applyMetrics 按档重发。
		*/
		void setDialogFlatStyle(bool on)
		{
			if (m_dialogFlat == on)
				return;
			m_dialogFlat = on;
			if (on)
				m_flatIcon = QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png"));
			update();
		}

		void applyMetrics(int barH)
		{
			// 弹窗族档：方盒 32=图标 24+呼吸 4（先例 QToolButton 自然观感）；
			// 主窗档保持系统习惯宽条（宽≈高×1.9 固定 46 档），高度=窗口头高
			setFixedSize(m_dialogFlat ? 32 : captionButtonWidth(), m_dialogFlat ? 32 : barH);
			// 图形盒 = icon 档值（标准 16）
			m_glyphBox = UiMetrics::menuIconSize();
			update();
		}

	protected:
		// 悬停进入/离开强制重绘：underMouse() 值改变后必须显式 update，
		// 否则依赖默认事件派发在部分宿主（尤其对话框）不重画，悬浮态卡住
		void enterEvent(QEvent* event) override
		{
			QAbstractButton::enterEvent(event);
			update();
		}
		void leaveEvent(QEvent* event) override
		{
			QAbstractButton::leaveEvent(event);
			update();
		}

		void paintEvent(QPaintEvent*) override
		{
			QPainter p(this);
			p.setRenderHint(QPainter::TextAntialiasing);

			const bool hover = underMouse() && isEnabled();
			const bool pressed = isDown();
			const bool active = window() ? window()->isActiveWindow() : true;

			if (m_dialogFlat)
			{
				// 弹窗族档：hover 淡灰圆角 2（先例 QSS 仅 hover 无 pressed 规则，
				// 按下不变色）；close3x 图标静止/hover 均原样绘制，无反白
				if (hover)
				{
					p.setRenderHint(QPainter::Antialiasing);
					p.setPen(Qt::NoPen);
					p.setBrush(QColor(QStringLiteral("#EFF1F2")));
					p.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 2, 2);
				}
				m_flatIcon.paint(&p, rect(), Qt::AlignCenter);
				return;
			}

			// 背景态（规则 0.2/2）：min/max 悬停 hover、按下 selected；
			// close 悬停/按下 error 红底（红底白图标），与主窗完全一致。
			if (m_role == Role::Close)
			{
				if (pressed)
					p.fillRect(rect(), UiPalette::error().darker(115));
				else if (hover)
					p.fillRect(rect(), UiPalette::error());
			}
			else
			{
				if (pressed)
					p.fillRect(rect(), UiPalette::selected());
				else if (hover)
					p.fillRect(rect(), UiPalette::hover());
			}

			// 字形颜色：常态主文字色（失活转 muted）；关闭钮悬停/按下红底反白
			QColor fg;
			if (m_role == Role::Close && (hover || pressed))
				fg = UiPalette::onAction();
			else
				fg = active ? UiPalette::text() : UiPalette::muted();

			// 字形按图形盒 10/16 比例缩放（与原生视觉一致）。
			// Win7/Linux 无 Segoe MDL2 Assets：按字形画会整片豆腐块，
			// 回退矢量绘制（drawVectorGlyph，与 MDL2 字形视觉等效）
			if (hasIconFont())
			{
				QString glyph;
				switch (m_role)
				{
				case Role::Min:   glyph = QStringLiteral("\uE921"); break;
				case Role::Max:   glyph = m_hostMax ? QStringLiteral("\uE923") : QStringLiteral("\uE922"); break;
				case Role::Close: glyph = QStringLiteral("\uE8BB"); break;
				}
				QFont f(iconFontFamily());
				f.setPixelSize(qMax(8, m_glyphBox * 10 / 16));
				p.setFont(f);
				p.setPen(fg);
				p.drawText(rect(), Qt::AlignCenter, glyph);
			}
			else
			{
				CaptionGlyph glyph = (m_role == Role::Min) ? CaptionGlyph::Min
					: (m_role == Role::Close) ? CaptionGlyph::Close
					: (m_hostMax ? CaptionGlyph::Restore : CaptionGlyph::Max);
				drawVectorGlyph(p, glyph, rect(), m_glyphBox, fg);
			}
		}

	private:
		Role m_role;
		bool m_hostMax = false;
		int m_glyphBox = 16;
		// 弹窗族档（setDialogFlatStyle，仅关闭键启用）
		bool m_dialogFlat = false;
		QIcon m_flatIcon;
	};
	/* ==================== mWindowTitleBar ==================== */

	mWindowTitleBar::mWindowTitleBar(QWidget* hostWindow)
		: QWidget(hostWindow)
		, m_host(hostWindow)
	{
		setObjectName(QStringLiteral("mWindowTitleBar"));
		initUi();
		initConnections();
		// 构造即完整：配色 + 尺寸 + 窗口状态一次性就位，不依赖外部重发
		applyColors();
		applyMetrics();
		updateWindowState();
		// 记录本次应用的档位/主题，供自检比对
		m_appliedLevel = UiMetrics::level();
		m_appliedTheme = static_cast<int>(UiPalette::theme());
	}

	mWindowTitleBar::~mWindowTitleBar() = default;

	int mWindowTitleBar::barHeight() const
	{
		// 最大化时多 1px 补偿原生最大化遮挡工作区顶部的问题
		const int base = m_heightOverride > 0 ? m_heightOverride : titleBarHeight();
		return base + (m_host && m_host->isMaximized() ? 1 : 0);
	}

	void mWindowTitleBar::initUi()
	{
		m_iconLabel = new QLabel(this);
		m_iconLabel->setObjectName(QStringLiteral("titleBarIcon"));
		m_iconLabel->setAlignment(Qt::AlignCenter);
		m_iconLabel->setCursor(Qt::ArrowCursor);
		m_iconLabel->installEventFilter(this);

		m_titleLabel = new QLabel(this);
		m_titleLabel->setObjectName(QStringLiteral("titleBarTitle"));
		m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		m_titleLabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

		m_btnMin = new mCaptionButton(mCaptionButton::Role::Min, this);
		m_btnMax = new mCaptionButton(mCaptionButton::Role::Max, this);
		m_btnClose = new mCaptionButton(mCaptionButton::Role::Close, this);

		QHBoxLayout* lay = new QHBoxLayout(this);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(0);
		lay->addWidget(m_iconLabel);
		// 规则 2：头部动作与标题间距至少 windowInset=G+2（标准 8）。
		// 垫块用定宽控件承载：紧凑覆盖档（setTitleLayoutCompact）要在
		// applyMetrics 里改宽，addSpacing 死项做不到
		m_titleGap = new QWidget(this);
		m_titleGap->setFixedWidth(UiMetrics::windowInset());
		lay->addWidget(m_titleGap);
		lay->addWidget(m_titleLabel);
		lay->addWidget(m_btnMin);
		lay->addWidget(m_btnMax);
		lay->addWidget(m_btnClose);
		setLayout(lay);

		setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	}

	void mWindowTitleBar::initConnections()
	{
		connect(m_btnMin, &QAbstractButton::clicked, this, &mWindowTitleBar::onMinClicked);
		connect(m_btnMax, &QAbstractButton::clicked, this, &mWindowTitleBar::onMaxRestoreClicked);
		connect(m_btnClose, &QAbstractButton::clicked, this, &mWindowTitleBar::onCloseClicked);
	}

	void mWindowTitleBar::applyColors()
	{
		// 标题栏自身底色/下边线走 paintEvent 自绘（见下），这里只设标题/图标文字色。
		// 按钮背景态已由 mCaptionButton 自绘，无需 QSS。
		const bool active = m_host && m_host->isActiveWindow();
		m_titleLabel->setStyleSheet(QStringLiteral("color:%1; background:transparent; border:none;")
			.arg(active ? UiPalette::textHex() : UiPalette::mutedHex()));
		m_iconLabel->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
		update();
		// 宿主外框线与标题栏下边线同源取色，主题变化时一并重画
		if (m_host)
			m_host->update();
	}

	void mWindowTitleBar::applyMetrics()
	{
		// 尺寸随档位重算：标题栏高（含实例覆盖）、按钮宽（≈高×1.9）、图标盒、标题字号
		const int barH = barHeight();
		// 图标格：默认=带高方盒（图标居中）；紧凑档=图标宽格+左缘覆盖
		//（转换设置原型：logo 左 20、格宽即图标宽 14，墨迹左 21 咬合）
		const bool compact = m_iconLeftOverride >= 0;
		const int iconCellW = compact && m_iconOverride.isValid() ? m_iconOverride.width() : barH;
		m_iconLabel->setFixedSize(iconCellW, barH);
		if (m_titleGap)
			m_titleGap->setFixedWidth(m_textGapOverride >= 0 ? m_textGapOverride : UiMetrics::windowInset());
		if (QHBoxLayout* lay = qobject_cast<QHBoxLayout*>(layout()))
		{
			// 右衬 16 仅弹窗族档：close3x 图标盒 32 内图标 24 居中，图标右缘
			// 呼吸 4+右衬 16=20 —— 咬合已验收弹窗"✕ 图标右缘距窗右 20"先例
			//（mChannelMapDlg 标题布局右衬 20）；主窗档关闭键贴边，右衬 0
			lay->setContentsMargins(compact ? m_iconLeftOverride : 0, 0,
				m_dialogFlatClose ? 16 : 0, 0);
		}
		m_btnMin->applyMetrics(barH);
		m_btnMax->applyMetrics(barH);
		// 关闭键形态随档：弹窗族档=setDialogFlatClose 切 close3x 图标钮
		//（2026-11-09 用户裁定对齐白卡家族）；主窗档=46 宽条悬停红条骨架
		// 默认（曾按原型 ✕ 墨迹反推 62 方盒致悬停红块放大翻车，11-06 回退）
		m_btnClose->applyMetrics(barH);
		QFont f(UiMetrics::uiFontFamily());
		f.setPixelSize(m_titlePx > 0 ? m_titlePx : UiMetrics::windowTitleFontSize());
		if (m_titleWeight > 0)
		{
			// 字重语义取 CSS 口径（白卡家族常数“13px/600”），折算 Qt5 枚举：
			// ≥700→Bold、≥550→DemiBold、其余 Normal
			f.setWeight(m_titleWeight >= 700 ? QFont::Bold
				: (m_titleWeight >= 550 ? QFont::DemiBold : QFont::Normal));
		}
		m_titleLabel->setFont(f);
		setFixedHeight(barH);
	}

	void mWindowTitleBar::refreshIfStale()
	{
		// 自检档位/主题是否变化，变了就刷新（自给自足，不靠外部喂）。
		// showEvent 与宿主 changeEvent 都会调用，保证任何时候打开都按当前值。
		const int lv = UiMetrics::level();
		const int th = static_cast<int>(UiPalette::theme());
		if (lv == m_appliedLevel && th == m_appliedTheme)
			return;
		m_appliedLevel = lv;
		m_appliedTheme = th;
		applyMetrics();
		applyColors();
		updateWindowState();
	}

	void mWindowTitleBar::updateWindowState()
	{
		if (!m_host) return;
		m_titleLabel->setText(m_host->windowTitle());
		if (!m_host->windowIcon().isNull())
		{
			// 窗口图标：窗口头内垂直居中；默认=窗口头档方盒（头高−2×max(2,G−2)），
			// 提示窗等按白卡家族常数覆盖（logo 14×16），防止 logo.png 满盒渲染成巨标
			QSize iconSize(m_iconOverride);
			if (!iconSize.isValid())
			{
				const int edge = barHeight() - 2 * qMax(2, UiMetrics::baseG() - 2);
				iconSize = QSize(edge, edge);
			}
			m_iconLabel->setPixmap(m_host->windowIcon().pixmap(iconSize));
		}
		// 最大化/还原图标切换 + 工具提示（对齐原生）
		m_btnMax->setHostMaximized(m_host->isMaximized());
		m_btnMax->setToolTip(m_host->isMaximized() ? QStringLiteral("还原") : QStringLiteral("最大化"));
		m_btnMin->setToolTip(QStringLiteral("最小化"));
		m_btnClose->setToolTip(QStringLiteral("关闭"));
		applyColors();   // 激活态决定标题/图标文字色
		setFixedHeight(barHeight());
	}

	bool mWindowTitleBar::isInCaptionArea(const QPoint& globalPos) const
	{
		if (!rect().contains(mapFromGlobal(globalPos)))
			return false;
		// 排除按钮区与图标区（图标区走系统菜单，不算拖动区）；
		// 隐藏的键（提示窗 closeOnly 下的 min/max）几何已失效，一并跳过防拖动死区
		for (QWidget* w : { static_cast<QWidget*>(m_iconLabel), static_cast<QWidget*>(m_btnMin),
							static_cast<QWidget*>(m_btnMax), static_cast<QWidget*>(m_btnClose) })
		{
			if (w->isVisible() && w->rect().contains(w->mapFromGlobal(globalPos)))
				return false;
		}
		return true;
	}

	bool mWindowTitleBar::eventFilter(QObject* watched, QEvent* event)
	{
		if (watched == m_iconLabel)
		{
			if (event->type() == QEvent::MouseButtonPress)
			{
				auto* me = static_cast<QMouseEvent*>(event);
				if (me->button() == Qt::LeftButton)
				{
					showSystemMenu(m_iconLabel->mapToGlobal(QPoint(0, m_iconLabel->height())));
					return true;
				}
			}
			else if (event->type() == QEvent::MouseButtonDblClick)
			{
				// 对齐原生：双击标题栏图标关闭窗口
				m_host->close();
				return true;
			}
		}
		return QWidget::eventFilter(watched, event);
	}

	void mWindowTitleBar::setSeparatorVisible(bool on)
	{
		if (m_separatorVisible == on) return;
		m_separatorVisible = on;
		update();
	}

	void mWindowTitleBar::setCloseOnly(bool on)
	{
		if (m_closeOnly == on) return;
		m_closeOnly = on;
		m_btnMin->setVisible(!on);
		m_btnMax->setVisible(!on);
		update();
	}

	void mWindowTitleBar::setDialogFlatClose(bool on)
	{
		if (m_dialogFlatClose == on) return;
		m_dialogFlatClose = on;
		m_btnClose->setDialogFlatStyle(on);
		// 右衬与关闭键盒都随档变，重跑一次布局度量（applyMetrics 对重入幂等，
		// refreshIfStale/showEvent 再触发也按当前档一致）
		applyMetrics();
	}

	void mWindowTitleBar::setBarHeight(int h)
	{
		if (m_heightOverride == h) return;
		m_heightOverride = h;
		applyMetrics();
		updateWindowState();
	}

	void mWindowTitleBar::setTitleIconSize(const QSize& size)
	{
		if (m_iconOverride == size) return;
		m_iconOverride = size;
		// 图标尺寸只影响取图盒，重取一次即可（盒在标题栏内居中不变）
		updateWindowState();
	}

	void mWindowTitleBar::setTitleFont(int pixelSize, int weight)
	{
		if (m_titlePx == pixelSize && m_titleWeight == weight) return;
		m_titlePx = pixelSize;
		m_titleWeight = weight;
		applyMetrics();
	}

	void mWindowTitleBar::setTitleLayoutCompact(int iconLeft, int iconTextGap)
	{
		if (m_iconLeftOverride == iconLeft && m_textGapOverride == iconTextGap)
			return;
		m_iconLeftOverride = iconLeft;
		m_textGapOverride = iconTextGap;
		applyMetrics();
	}

	void mWindowTitleBar::paintEvent(QPaintEvent*)
	{
		// 软件标题栏底色，自绘避免全局 QSS 干扰
		QPainter p(this);
		p.fillRect(rect(), UiPalette::softwareTitleBar());
		// 窗口头下边线（规则 2）：与内容区分隔。softwareTitleBar 与 surface 同值
		//（三主题刻意一致），不画这条线时无边框窗是一整块同色平板；
		// 主窗口与提示窗（mFramelessMessageBox）经 setSeparatorVisible(false) 关闭
		if (m_separatorVisible)
		{
			p.setPen(UiPalette::border());
			p.drawLine(0, height() - 1, width() - 1, height() - 1);
		}
	}

	void mWindowTitleBar::showSystemMenu(const QPoint& globalPos)
	{
#ifdef Q_OS_WIN
		if (!m_host) return;
		const HWND hwnd = reinterpret_cast<HWND>(m_host->winId());
		if (!hwnd) return;
		HMENU menu = GetSystemMenu(hwnd, FALSE);
		if (!menu) return;
		// 按当前窗口状态刷新菜单项可用性
		const UINT maximized = m_host->isMaximized() ? (MF_BYCOMMAND | MF_GRAYED) : (MF_BYCOMMAND | MF_ENABLED);
		const UINT restored = m_host->isMaximized() ? (MF_BYCOMMAND | MF_ENABLED) : (MF_BYCOMMAND | MF_GRAYED);
		EnableMenuItem(menu, SC_MAXIMIZE, maximized);
		EnableMenuItem(menu, SC_RESTORE, restored);
		EnableMenuItem(menu, SC_MOVE, restored);
		EnableMenuItem(menu, SC_SIZE, restored);
		EnableMenuItem(menu, SC_MINIMIZE, MF_BYCOMMAND | MF_ENABLED);
		EnableMenuItem(menu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED);
		SetMenuDefaultItem(menu, UINT_MAX, FALSE);
		const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD, globalPos.x(), globalPos.y(), 0, hwnd, nullptr);
		if (cmd != 0)
			PostMessageW(hwnd, WM_SYSCOMMAND, cmd, 0);
#else
		Q_UNUSED(globalPos);
#endif
	}

	void mWindowTitleBar::onMinClicked()
	{
		if (m_host) m_host->showMinimized();
	}

	void mWindowTitleBar::onMaxRestoreClicked()
	{
		if (!m_host) return;
		if (m_host->isMaximized())
		{
			// 最大化→还原：不能用 QPropertyAnimation 直接驱动 geometry——
			// 窗口仍处最大化状态时 Windows 的 WM_GETMINMAXINFO 会把 setGeometry
			// 钳制回最大化矩形，动画帧被吞掉，且状态机错乱。原生还原动画由系统提供。
			m_host->showNormal();
		}
		else
		{
			m_host->showMaximized();
		}
		updateWindowState();
	}

	void mWindowTitleBar::onCloseClicked()
	{
		if (m_host) m_host->close();
	}

	void mWindowTitleBar::setDragDelegate(mTitleBarDragDelegate* delegate)
	{
		m_dragDelegate = delegate;
	}

	void mWindowTitleBar::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			// 委托模式（ADS 浮动容器）：按下即进入委托的拖拽状态机
			//（对齐 qtads Linux 自绘标题栏：按下 startFloating，
			// 由 MouseEventHandler grab 鼠标接管拖动期间的全部事件）
			if (m_dragDelegate)
			{
				m_delegateDragging = true;
				m_dragDelegate->beginCaptionDrag(event->globalPos(), this);
				return;
			}
			m_pressOnCaption = true;
			m_dragMoved = false;
			m_pressGlobalPos = event->globalPos();
			m_pressWindowPos = m_host ? m_host->frameGeometry().topLeft() : QPoint();
		}
		QWidget::mousePressEvent(event);
	}

	void mWindowTitleBar::mouseMoveEvent(QMouseEvent* event)
	{
		if (m_delegateDragging && (event->buttons() & Qt::LeftButton) && m_dragDelegate)
		{
			m_dragDelegate->moveCaptionDrag(event->globalPos());
			return;
		}
		if (m_pressOnCaption && (event->buttons() & Qt::LeftButton) && m_host)
		{
			const QPoint delta = event->globalPos() - m_pressGlobalPos;
			if (!m_dragMoved && (qAbs(delta.x()) + qAbs(delta.y()) >= QApplication::startDragDistance()))
				m_dragMoved = true;
			if (m_dragMoved)
			{
				// 最大化拖动：先还原，窗口宽度按原还原尺寸，光标保持在标题栏上的相对水平位置
				if (m_host->isMaximized())
				{
					const QRect normalGeo = m_host->normalGeometry();
					const int relX = event->globalPos().x() - m_host->frameGeometry().x();
					const double ratio = m_host->frameGeometry().width() > 0
						? double(relX) / m_host->frameGeometry().width() : 0.5;
					m_host->showNormal();
					const int newX = event->globalPos().x() - int(normalGeo.width() * ratio);
					m_pressWindowPos = QPoint(newX, event->globalPos().y() - titleBarHeight() / 2);
					m_pressGlobalPos = event->globalPos();
				}
				m_host->move(m_pressWindowPos + (event->globalPos() - m_pressGlobalPos));
			}
		}
		QWidget::mouseMoveEvent(event);
	}

	void mWindowTitleBar::mouseReleaseEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			// 委托模式：松开结束拖拽（委托内部完成拖回停靠判定/窗口复位）
			if (m_delegateDragging)
			{
				m_delegateDragging = false;
				if (m_dragDelegate)
					m_dragDelegate->endCaptionDrag();
				return;
			}
			m_pressOnCaption = false;
#ifdef Q_OS_WIN
			// 对齐原生：拖到屏幕顶部自动最大化（Aero Snap 顶边吸附）；
			// 只留关闭键的固定小窗（提示框）不可最大化，禁用吸附
			if (m_dragMoved && m_host && !m_closeOnly && !m_host->isMaximized())
			{
				if (QScreen* scr = QGuiApplication::screenAt(event->globalPos()))
				{
					const QRect avail = scr->availableGeometry();
					if (event->globalPos().y() <= avail.top() + 2)
					{
						m_host->showMaximized();
						updateWindowState();
					}
				}
			}
#endif
			m_dragMoved = false;
		}
		QWidget::mouseReleaseEvent(event);
	}

	void mWindowTitleBar::mouseDoubleClickEvent(QMouseEvent* event)
	{
		// 只留关闭键的小窗（提示框）：双击不做最大化/还原
		if (m_closeOnly)
			return QWidget::mouseDoubleClickEvent(event);
		if (event->button() == Qt::LeftButton)
		{
			onMaxRestoreClicked();
			m_pressOnCaption = false;
			m_dragMoved = false;
			m_delegateDragging = false;
			return;
		}
		QWidget::mouseDoubleClickEvent(event);
	}

	void mWindowTitleBar::changeEvent(QEvent* event)
	{
		// 激活态切换（原生标题栏失活变灰）：刷新标题/图标文字色与按钮图标色
		if (event->type() == QEvent::ActivationChange)
			applyColors();
		QWidget::changeEvent(event);
	}

	void mWindowTitleBar::showEvent(QShowEvent* event)
	{
		// 每次显示前自检档位/主题，保证任何时候打开都与当前设置一致
		refreshIfStale();
		QWidget::showEvent(event);
	}
}
