#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 自绘标题栏（mWindowTitleBar）
*
* 完全自绘、自给自足：颜色/尺寸/图标全部取自 UiPalette/UiPalette 公式，
* 不读全局 QSS、不依赖外部重发。主窗与任何对话框（mFramelessDialog）只要
* new 一个本类实例即自动对齐——悬浮/按下/红底白图标/尺寸全部一致。
*
* 关键设计：
*   - 三按钮用内部自绘按钮 mCaptionButton（QPainter 直接画背景态与字形），
*     从根上摆脱全局 QSS 压盖、WA_Hover 不触发、autoRaise 干扰这一整类问题；
*   - 主题/档位变化由本类自检（showEvent + 宿主 changeEvent 时比对缓存值），
*     不需要 ThemeManager 外部主动调用 refreshTheme/refreshMetrics；
*   - 布局/行为对齐 Windows 原生标题栏：左图标+标题、右最小化/最大化/关闭，
*     中部拖动移动、双击切最大化、拖到屏幕顶部 Aero Snap、图标单击系统菜单。
*/

#include "mCommon_global.h"

#include <QWidget>

class QLabel;

namespace MechMainWindow
{
	class mCaptionButton;

	/*
	* 标题栏拖拽委托：宿主为 ADS 浮动容器等特殊窗口时，标题栏的按下/拖动/
	* 松开不再走默认的窗口 move，而是转发给委托（由委托驱动拖回停靠判定等）。
	* beginCaptionDrag 的 eventHandler 供委托转发给需要 grab 鼠标的机制
	*（如 qtads 的 startDragging 用它接管拖动期间的全部鼠标事件）。
	*/
	class COMMON_EXPORT mTitleBarDragDelegate
	{
	public:
		virtual ~mTitleBarDragDelegate() = default;
		virtual void beginCaptionDrag(const QPoint& globalPos, QWidget* eventHandler) = 0;
		virtual void moveCaptionDrag(const QPoint& globalPos) = 0;
		virtual void endCaptionDrag() = 0;
	};

	class COMMON_EXPORT mWindowTitleBar : public QWidget
	{
		Q_OBJECT
	public:
		explicit mWindowTitleBar(QWidget* hostWindow);
		~mWindowTitleBar() override;

		/*
		* 设置拖拽委托（所有权归调用者，通常以宿主窗口为 parent 随之销毁）。
		* 设置后标题栏拖动/松开全部转发委托；传 nullptr 恢复默认拖动行为。
		*/
		void setDragDelegate(mTitleBarDragDelegate* delegate);

		/*
		* 同步窗口状态：标题文字、图标、最大化/还原按钮图标、激活态。
		* 幂等；通常在宿主窗口状态变化（changeEvent）时由本类内部触发，
		* 也可由外部显式调用（如主题/档位切换后）。
		*/
		void updateWindowState();

		/*
		* 标题栏总高度（含最大化时的 1px 补偿）
		*/
		int barHeight() const;

		/*
		* nativeEvent 的 WM_NCHITTEST 判断该点是否属于标题栏拖动区
		*（标题栏范围内、按钮与图标之外）
		*/
		bool isInCaptionArea(const QPoint& globalPos) const;

		/*
		* 标题栏底部分隔线开关（默认开）：softwareTitleBar 与内容 surface 同值，
		* 普通弹窗靠这条线区分窗头与内容；主窗口（下方是菜单栏自带分界线）与
		* 提示窗 mFramelessMessageBox（提示/警告，无需分界）创建后传 false 关闭
		*/
		void setSeparatorVisible(bool on);

		/*
		* 只留关闭键（默认关，三键）：提示/警告等固定小窗按原型无最小化/最大化，
		* 隐藏两键并禁掉双击标题栏最大化与拖到顶边的 Aero Snap
		*/
		void setCloseOnly(bool on);

		/*
		* 弹窗族关闭键档（默认关）：关闭键改画 close3x 图标钮 24×24（盒 32）、
		* hover 淡灰圆角 2、无红底白图标——对齐已验收白卡弹窗（通道映射/
		* 图层管理）的 ✕ 形态与"图标右缘距窗右 20"落位（2026-11-09 用户
		* 裁定：提示框族 ✕ 用主窗红条档与其它弹窗不一致）。仅影响关闭键，
		* 主窗档不变
		*/
		void setDialogFlatClose(bool on);

		/*
		* 本实例标题栏高度覆盖（0=默认取窗口头档位）：提示框原型形态覆盖 34
		*（外框 400×156 三段式之一，供图系 1.5× 放大稿 ÷1.5 标定），窗口头
		* 档位为 28，依附 mFramelessDialog 的小窗按所属家族覆盖
		*/
		void setBarHeight(int h);

		/*
		* 标题图标尺寸覆盖（无效 QSize=默认按窗口头档方盒＝头高−2×max(2,G−2)，
		* 标准 44）：提示窗按白卡家族常数 logo.png 缩 14×16——窗口图标是满幅
		* M 标，默认大盒会渲染成 44px 巨标；14×16 与提示框原型 21×22÷1.5 咬合
		*（原型供图系 1.5× 放大稿，真机目标 400×156，2026-11-04 确认）
		*/
		void setTitleIconSize(const QSize& size);

		/*
		* 标题字号/字重覆盖（0=默认窗口头字号 12 常规）：白卡对话框家族常数
		* 为标题 13px/600（通道映射/模板曲线/图层三框已验收口径）
		*/
		void setTitleFont(int pixelSize, int weight);

		/*
		* 紧凑标题排布覆盖（负值=默认档）：转换设置形态（带高 62）原型要求
		* logo 左缘贴 20 内衬（默认=带高方盒居中，左缘 24）、标题格紧跟 logo
		*（默认=方盒格 62+windowInset，文字格 70 墨迹 72）。iconLeft=图标格
		* 左缘；iconTextGap=图标格→标题格间距。须在 setTitleIconSize 之后
		* 调用（紧凑图标格宽=图标宽）；2026-11-06 原型像素复核标定 20/5。
		* 关闭键不在此覆写：其形态归 setDialogFlatClose（2026-11-09 用户裁定
		* 弹窗族对齐白卡家族图标钮，原"抄家族 46 宽条"档被取代；主窗档仍为
		* 46 宽条红条）
		*/
		void setTitleLayoutCompact(int iconLeft, int iconTextGap);

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		void changeEvent(QEvent* event) override;
		void showEvent(QShowEvent* event) override;
		void paintEvent(QPaintEvent* event) override;

	private slots:
		void onMinClicked();
		void onMaxRestoreClicked();
		void onCloseClicked();

	private:
		void initUi();
		void initConnections();
		void showSystemMenu(const QPoint& globalPos);
		// 自检主题/档位是否变化，变了就刷新（自给自足，不靠外部喂）
		void refreshIfStale();
		void applyColors();
		void applyMetrics();
		QWidget* host() const { return m_host; }

	private:
		QWidget* m_host = nullptr;         // 宿主窗口（QMainWindow/QDialog）
		QLabel* m_iconLabel = nullptr;
		QLabel* m_titleLabel = nullptr;
		mCaptionButton* m_btnMin = nullptr;
		mCaptionButton* m_btnMax = nullptr;
		mCaptionButton* m_btnClose = nullptr;

		// 缓存已应用的档位/主题，用于自检是否需要刷新
		int m_appliedLevel = -1;
		int m_appliedTheme = -1;

		// 拖动状态：记录按下时的全局坐标与宿主窗口位置（最大化时拖动先还原再跟随）
		bool m_pressOnCaption = false;
		QPoint m_pressGlobalPos;
		QPoint m_pressWindowPos;
		bool m_dragMoved = false;

		// 拖拽委托：非空时标题栏拖动转发委托（ADS 浮动容器拖回停靠），
		// m_delegateDragging 标记委托拖动进行中
		mTitleBarDragDelegate* m_dragDelegate = nullptr;
		bool m_delegateDragging = false;

		// 底部分隔线是否绘制（默认 true；主窗口与提示窗 mFramelessMessageBox 关闭）
		bool m_separatorVisible = true;

		// 只留关闭键（提示窗等固定小窗）：隐藏 min/max，禁双击最大化/顶边吸附
		bool m_closeOnly = false;

		// 弹窗族关闭键档（提示框族启用），见 setDialogFlatClose
		bool m_dialogFlatClose = false;

		// 本实例标题栏高覆盖（0=随窗口头档位），见 setBarHeight
		int m_heightOverride = 0;

		// 标题图标尺寸/标题字号/字重覆盖（无效 QSize、0=家族默认），见
		// setTitleIconSize/setTitleFont
		QSize m_iconOverride;
		int m_titlePx = 0;
		int m_titleWeight = 0;

		// 紧凑标题排布覆盖（负值=默认方盒+windowInset 档），见 setTitleLayoutCompact
		int m_iconLeftOverride = -1;
		int m_textGapOverride = -1;
		QWidget* m_titleGap = nullptr;   // 图标格→标题格定宽垫块（宽随档/覆盖重算）
	};
}
