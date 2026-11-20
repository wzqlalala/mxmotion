#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 规则自绘样式（mUiStyle，QProxyStyle）
*
* 《通用结构软件界面UI设计规则》的 Qt 落地样式层：所有尺寸取自 UiMetrics（规则 0.1 的
* H/G 派生），所有颜色取自 UiPalette（规则 0.2 的三主题语义色板），不引入任何业务头。
*
* 背景：菜单栏控件链此前依赖大段 QSS 字符串拼接下发样式，Qt 的 QSS 在以下场景不可靠——
*   1) QToolButton 的 sizeHint/图标文字布局不受 QSS 精确控制，基线对齐（规则 1.3：
*      全高文字与二分下行文字共用基线）无法保证；
*   2) 全局 QSS（mApplicationStart.qss 的 21px 白底 QToolButton 规则）会反向覆盖
*      按钮局部样式，文字被挤出可视区；
*   3) 切档/切主题后 QSS 缓存（polish）滞后，出现"文字丢失、重启后正常"。
* 因此把功能区按钮、页签、快捷钮、命令菜单行等关键控件的绘制收口到本类自绘，
* QSS 只保留非关键容器底色，绘制结果只由 UiMetrics/UiPalette 决定，可预测、可验收。
*
* 安装方式：mUiStyle::install(widget) 挂到菜单栏根控件，子控件自动继承；
* 自绘覆盖的控件按 objectName/类型识别（见 drawControl/drawComplexControl 实现）。
*/

#include "mCommon_global.h"

#include <QProxyStyle>

class QMenu;

namespace MechMainWindow
{
	class COMMON_EXPORT mUiStyle : public QProxyStyle
	{
		Q_OBJECT
	public:
			mUiStyle();
		~mUiStyle() override;

		/*
		* 把规则样式挂到控件子树（菜单栏根控件调用一次即可）
		*/
		static void install(QWidget* root);

		/*
		* 给单个弹出菜单（QMenu，含将来 addMenu 添加的子菜单）安装命令菜单自绘样式（规则 1.0）。
		* 与 install() 的区别：install 只挂已有子控件且跳过 QMenu；菜单是顶层弹出窗口、
		* 子菜单在 aboutToShow 时才创建，需要 aboutToShow 时递归补挂。
		*/
		static void installMenu(QWidget* menu);

		/*
		* 当前实例（install 后全局唯一，供绘制辅助函数外部调用）
		*/
		static mUiStyle* instance();

		/*
		* 切档/切主题后通知重绘（清缓存+全树 update）
		*/
		static void refresh(QWidget* root);

		/*
		* 菜单左列模式：任一可见项有图标或勾选框时保留对齐列；纯文字菜单不占图标位
		*/
		static bool menuHasLeadingColumn(const QMenu* menu);

		// 按菜单外框定位右槽；actionRect 与绘制/命中使用同一坐标系。
		static QPointF menuRightSlotCenter(const QWidget* menu, const QRectF& actionRect);

		/* ---------- QStyle 接口 ---------- */
		void polish(QWidget* w) override;
		int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
			const QWidget* widget = nullptr) const override;
		QSize sizeFromContents(ContentsType type, const QStyleOption* option,
			const QSize& size, const QWidget* widget) const override;
		void drawPrimitive(PrimitiveElement element, const QStyleOption* option,
			QPainter* painter, const QWidget* widget = nullptr) const override;
		void drawControl(ControlElement element, const QStyleOption* option,
			QPainter* painter, const QWidget* widget = nullptr) const override;
		void drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
			QPainter* painter, const QWidget* widget = nullptr) const override;
		QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option,
			SubControl subControl, const QWidget* widget = nullptr) const override;
		QRect subElementRect(SubElement element, const QStyleOption* option,
			const QWidget* widget = nullptr) const override;

		/* ---------- 自绘辅助（供 MyTabBar/mComboButton 等自定义 paintEvent 复用） ---------- */

		/*
		* 绘制功能区按钮内容：图标+文字按规则 1.3 布局
		*   全高：大图标(menuLargeIcon=H+4)在上，文字行盒底距按钮底 bigButtonTextBottomPadding()，
		*         图文间距 menuContentGap，整体水平居中；
		*   二分：小图标(icon=16)在左，文字在右，横向图文间距 menuContentGap，整体左对齐垂直居中
		* 状态色：常态 text / 悬停 hover 底 / 选中 selected 底 / 禁用 disabled，圆角 radius=4
		*/
		static void drawRibbonButton(QPainter* p, const QRect& rect, const QString& text,
			const QIcon& icon, const QSize& iconSize, bool big, bool hasArrow,
			bool hover, bool pressed, bool checked, bool enabled, const QFont& font,
			const QWidget* widget = nullptr);

		/*
		* 绘制按钮悬停/选中/焦点背景（圆角 4，规则 0：radius 固定 4 不随档位）
		*/
		static void drawButtonState(QPainter* p, const QRect& rect,
			bool hover, bool pressed, bool checked, bool focused);

		/*
		* 附属下拉箭头绘制复用 mRibbonButton.h 的自由函数
		* drawDropArrow / drawDropArrowBesideText（单一实现，跨自绘控件共用）
		*/

		/*
		* 绘制子菜单右箭头（规则 1.0 右槽标识）：实心小三角，形状与 drawMenuItem 自绘一致。
		* 菜单在 QSS 之下由 QStyleSheetStyle 的 PE_IndicatorArrowRight 兜底走这里，
		* 保证"行内容自绘被 QSS 接管"与"QSS 被移除走纯自绘"两条路径箭头一致可见。
		*/
		static void drawSubMenuMark(QPainter* p, const QRect& rect, const QColor& color);

		/*
		* 绘制菜单勾选标识（规则 1.0 左槽）：干净打勾线（无方框、无凹槽底座）。
		* 勾选框 PNG（check1x/uncheck1x）与底座样式的 sunken 面板叠加会显"凹槽"，
		* 故勾选统一由此自绘，QSS 的 QMenu::indicator 被压成 0 尺寸
		*/
		static void drawMenuCheckMark(QPainter* p, const QRect& rect, bool checked,
			bool enabled, bool exclusive = false);

	private:
		void drawTabBarTab(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
		void drawTabBarTabShape(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
		void drawToolButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;
		void drawMenuItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const;

		static mUiStyle* s_instance;
	};
}
