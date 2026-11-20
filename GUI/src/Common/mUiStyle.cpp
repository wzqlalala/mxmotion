#include "mUiStyle.h"

#include "mRibbonButton.h"
#include "mUiMenu.h"

#include "UiMetrics.h"
#include "UiPalette.h"

#include <QPainter>
#include <QStyleOption>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleOptionMenuItem>
#include <QToolButton>
#include <QTabBar>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QWidget>
#include <QApplication>
#include <QDebug>
#include <QFontMetrics>
#include <QSet>

using namespace MxFunctions;

namespace MechMainWindow
{
	mUiStyle* mUiStyle::s_instance = nullptr;

	namespace
	{
		// 规则 0：边框 1px、圆角 4px、焦点 2px 为固定常量，不随档位缩放
		constexpr int kRadius = 4;
		constexpr int kDropArrowGlyphWidth = 6;
		constexpr qreal kDropArrowPenWidth = 1.2;

		// 受 mUiStyle 自绘接管的 Ribbon 按钮判定：
		// 快捷钮/文件钮/翻页钮/标题栏钮有专属绘制或保持底座样式，不在此列；
		// comboMainBtn/comboDropBtn（双动作全高入口，规则 1.3）反而必须接管——
		// Qt 对 TextBesideIcon 文字居中布局会压缩文字盒宽，导致"扫掠"类文字被箭头区挤出
		bool isRibbonToolButton(const QWidget* w)
		{
			if (!w) return false;
			// QTabBar 内建滚动按钮（页签溢出时的左右箭头 QToolButton 子控件，懒创建、
			// objectName 为 tabBarScrollLeftButton/RightButton）不在 Ribbon 自绘范围：
			// 若按默认 true 拦截，drawComplexControl(CC_ToolButton) 直接 return 会把
			// 整个按钮画成空白——箭头消失。按父控件判定不依赖 objectName（Qt 各版本
			// 懒创建时机/命名有差异）。样式与箭头交回底座绘制，由 ThemeManager 的
			// 页签栏 QSS 后代选择器压制全局 QToolButton 21px 白底规则
			if (qobject_cast<const QTabBar*>(w->parentWidget()))
				return false;
			const QString name = w->objectName();
			static const QSet<QString> kExcluded = {
				QStringLiteral("FileButton"), QStringLiteral("ViewButton"),
				QStringLiteral("m_leftScrollBtn"), QStringLiteral("m_rightScrollBtn"),
				QStringLiteral("QuickNewBtn"), QStringLiteral("QuickSaveBtn"), QStringLiteral("QuickOpenBtn")
			};
			return !kExcluded.contains(name);
		}

		bool isBigRibbonButton(const QWidget* w)
		{
			// 组合按钮子钮按规则 1.3 双动作分区绘制：上图标区（居中纯图标）、下文字区（文字+箭头）
			const QString name = w->objectName();
			if (name == QStringLiteral("comboMainBtn") || name == QStringLiteral("comboDropBtn"))
				return false;
			const QVariant big = w->property("uiBigButton");
			if (big.isValid()) return big.toBool();
			const QVariant isz = w->property("uiIconSize");
			if (isz.isValid()) return isz.toSize().width() > 24;
			if (auto* tb = qobject_cast<const QToolButton*>(w))
				return tb->toolButtonStyle() == Qt::ToolButtonTextUnderIcon;
			return false;
		}
	}

	bool mUiStyle::menuHasLeadingColumn(const QMenu* menu)
	{
		if (!menu)
			return false;
		const auto acts = menu->actions();
		for (const QAction* act : acts)
		{
			if (!act->isVisible() || act->isSeparator() || qobject_cast<const QWidgetAction*>(act))
				continue;
			if (!act->icon().isNull())
				return true;
		}
		return false;
	}

	QPointF mUiStyle::menuRightSlotCenter(const QWidget* menu, const QRectF& actionRect)
	{
		const qreal outerRight = menu ? qreal(menu->width()) - 1.0 : actionRect.right();
		return QPointF(outerRight - UiMetrics::menuRightSlotPaddingF(),
			actionRect.center().y());
	}

	mUiStyle::mUiStyle() : QProxyStyle()
	{
	}

	mUiStyle::~mUiStyle() = default;

	void mUiStyle::install(QWidget* root)
	{
		if (!root) return;
		// instance() 统一创建并注册给公共控件 mUiMenu（挂菜单排版样式）
		(void)instance();
		// 挂到根控件：子控件未显式 setStyle 时沿父链解析到本样式。
		// 已挂过（属性标记）直接跳过——setStyle 会触发样式变更与重抛光，
		// 主题重发路径每次都调 install，重复 setStyle 是纯开销
		if (!root->property("mxUiStyled").toBool())
		{
			root->setProperty("mxUiStyled", true);
			root->setStyle(s_instance);
		}
		const auto children = root->findChildren<QWidget*>();
		for (QWidget* w : children)
		{
			// 只接管 Ribbon 按钮；页签交回 MyTabBar::paintEvent 全量自绘
			//（QTabBar 在 Qt5 下对 Shape/Label/复合元素的自绘路径不可靠，多轮验证均不触发）
			if (qobject_cast<QTabBar*>(w))
				continue;
			if (qobject_cast<QToolButton*>(w) && isRibbonToolButton(w)
				&& !w->property("mxUiStyled").toBool())
			{
				w->setProperty("mxUiStyled", true);
				w->setStyle(s_instance);
			}
		}
	}

	mUiStyle* mUiStyle::instance()
	{
		// 首次调用即创建：mUiMenu 等自绘控件可能早于 install() 构造，
		// 需要拿到一个可用实例挂样式，不能返回 null
		if (!s_instance)
			s_instance = new mUiStyle();
		// 注册给公共控件库的 mUiMenu（构造/adopt/每次展开前经它挂到菜单，
		// 保证 QMenu 内部排版与自绘同源）；mUiStyle 在 RebuildGUI、mUiMenu 在
		// MxFunctions，mUiMenu 不能反向依赖本类，只能经该注册口注入
		mUiMenu::setSharedStyle(s_instance);
		return s_instance;
	}

	void mUiStyle::installMenu(QWidget* menu)
	{
		QMenu* qmenu = qobject_cast<QMenu*>(menu);
		if (!qmenu) return;
		// instance() 统一创建并注册给公共控件 mUiMenu
		(void)instance();
		// 菜单是顶层弹出窗口，install() 的"挂根控件让子控件继承"对它不生效，需显式安装；
		// 子菜单可能在 aboutToShow 时才创建（动态填充），故每次展开时递归补挂新出现的子菜单
		qmenu->setStyle(s_instance);
		// QSS 抛光后 Qt 会用 QStyleSheetStyle 包裹本样式，widget->style() 不再是 s_instance，
		// PE_IndicatorArrowRight 等兜底自绘无法靠样式指针识别"已安装的菜单"，故打属性标记
		qmenu->setProperty("mxUiMenu", true);
		// 菜单行 hover 态由 drawMenuItem 自绘，QMenu 默认不跟踪鼠标需显式打开
		// （否则 State_Selected/State_MouseOver 不会随鼠标移动更新，hover 色画不出来）
		qmenu->setMouseTracking(true);
		// 弹出菜单默认 WA_Hover=false，State_MouseOver 永远不会置位（按钮等控件则由
		// QApplication 默认打开）。drawMenuItem/QSS 的悬停分支都依赖该状态，必须显式打开
		qmenu->setAttribute(Qt::WA_Hover, true);
		// aboutToShow 的递归补挂只连一次：lambda 连接无法被 Qt::UniqueConnection 去重，
		// 重复调用 installMenu（主题重发/每次展开）会在同一菜单上无限累积处理器
		if (!qmenu->property("mxUiMenuSynced").toBool())
		{
			qmenu->setProperty("mxUiMenuSynced", true);
			QObject::connect(qmenu, &QMenu::aboutToShow, qmenu, [qmenu]() {
				const auto subs = qmenu->findChildren<QMenu*>();
				for (QMenu* sub : subs)
				{
					if (sub->style() != s_instance)
						installMenu(sub);
					// 子菜单同样开启鼠标跟踪（hover 自绘依赖 State_MouseOver 状态更新）
					sub->setMouseTracking(true);
					sub->setAttribute(Qt::WA_Hover, true);
				}
			});
		}
	}

	void mUiStyle::refresh(QWidget* root)
	{
		if (!root) return;
		const auto children = root->findChildren<QWidget*>();
		for (QWidget* w : children)
		{
			if (w->style() == s_instance)
			{
				w->style()->unpolish(w);
				w->style()->polish(w);
			}
			w->update();
		}
		root->update();
	}

	void mUiStyle::polish(QWidget* w)
	{
		QProxyStyle::polish(w);
		// 自绘按钮不再依赖 QSS 背景：声明 WA_StyledBackground 的控件在深色主题下
		// 容易残留全局 QSS 白底，自绘路径统一用透明底+自绘状态色
		if (qobject_cast<QToolButton*>(w) && isRibbonToolButton(w))
			w->setAttribute(Qt::WA_Hover, true);
		if (auto* menu = qobject_cast<QMenu*>(w); menu && menu->property("mxUiMenu").toBool())
		{
			menu->setAttribute(Qt::WA_TranslucentBackground, true);
			menu->setAutoFillBackground(false);
			// 菜单窗口无内边距供阴影衰减，自绘/Effect 阴影会在圆角外的四角被裁出残影
			//（见 mUiMenu::init）：不挂 QGraphicsDropShadowEffect。
			// 注意：不能在此调用 setWindowFlag——qApp->setStyleSheet 全局抛光会走到这里，
			// 抛光路径里改窗口标志会触发样式系统重入（启动即崩）。菜单投影由系统
			// CS_DROPSHADOW 承担：Popup 菜单只要不设 NoDropShadowWindowHint，窗口类
			// 创建时即带投影（见 mUiMenu::init 注释），无需在此处理。
		}
	}

	int mUiStyle::pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
	{
		switch (metric)
		{
		// 自绘按钮不再使用 Qt 默认框架边距，宽度完全由 UiMetrics 公式控制
		case PM_DefaultFrameWidth:
			if (widget && qobject_cast<const QToolButton*>(widget) && isRibbonToolButton(widget))
				return 0;
			break;
		case PM_ButtonMargin:
			if (widget && qobject_cast<const QToolButton*>(widget) && isRibbonToolButton(widget))
				return 0;
			break;
		// 命令菜单（规则 1.0/基础菜单尺寸表）：按 mxUiMenu 标记识别
		//（QSS 抛光后 style()==this 恒为假，必须同时认属性标记）
		case PM_MenuHMargin:
			if (widget && (widget->property("mxUiMenu").toBool() || widget->style() == this))
				return UiMetrics::menuItemInset();          // 行内左右留白 inset=G+1（标准 7）
			break;
		case PM_MenuVMargin:
			if (widget && (widget->property("mxUiMenu").toBool() || widget->style() == this))
				return qMax(1, UiMetrics::baseG() - 2);     // 容器四边 choiceGap=G−2（标准 4）
			break;
		case PM_SmallIconSize:
			if (widget && (widget->property("mxUiMenu").toBool() || widget->style() == this))
				return UiMetrics::menuIconSize();           // 命令图标 16（标准档）
			break;
		case PM_MenuPanelWidth:
			if (widget && (widget->property("mxUiMenu").toBool() || widget->style() == this))
				return 1;                                    // 边框 1px
			break;
		default:
			break;
		}
		return QProxyStyle::pixelMetric(metric, option, widget);
	}

	QSize mUiStyle::sizeFromContents(ContentsType type, const QStyleOption* option,
		const QSize& size, const QWidget* widget) const
	{
		// Ribbon 按钮宽度 = 内容宽 + 2×menuInset（规则 1.3 固定左右留白 G−2），
		// 高度固定（大 ribbonFull / 小 ribbonHalf 或 H），由 ThemeManager 的 setFixedHeight 收口，
		// 这里只管内容→外框的增量，保证 Qt 不再私自加边距把文字挤出
		if (type == CT_ToolButton && widget && isRibbonToolButton(widget))
		{
			const int inset = UiMetrics::menuInset();
			return QSize(size.width() + 2 * inset, size.height());
		}
		// 菜单行外高 = control = H（规则 1.0：选项外高引用 control，相邻行距 0）；
		// 分隔线占位高 = 2×choiceGap+1（标准 9，上下各 choiceGap 留白），横线本身由 drawMenuItem 画
		// 备用普通 QMenu 的 sizeHint 也消费同一份内容宽/最小宽，不能只覆盖行高。
		if (type == CT_MenuItem && widget && widget->property("mxUiMenu").toBool())
		{
			const auto* menu = qobject_cast<const QMenu*>(widget);
			if (!menu)
				return QProxyStyle::sizeFromContents(type, option, size, widget);
			const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
			if (mi && mi->menuItemType == QStyleOptionMenuItem::Separator)
				return QSize(size.width(), UiMetrics::menuSeparatorHeight());
			QFont font = mi ? mi->font : menu->font();
			font.setPixelSize(UiMetrics::bodyFontSize());
			const QFontMetrics fm(font);
			int contentWidth = 0;
			for (QAction* action : menu->actions())
			{
				if (!action->isVisible() || action->isSeparator())
					continue;
				const auto* widgetAction = qobject_cast<const QWidgetAction*>(action);
				if (widgetAction && widgetAction->defaultWidget())
				{
					contentWidth = qMax(contentWidth,
						widgetAction->defaultWidget()->sizeHint().width());
					continue;
				}
				QString text = action->text();
				const int tabIndex = text.indexOf(QLatin1Char('\t'));
				if (tabIndex >= 0)
					text = text.left(tabIndex);
				int width = fm.horizontalAdvance(text);
				const QString shortcut = action->shortcut().toString(QKeySequence::NativeText);
				if (!shortcut.isEmpty())
					width += fm.horizontalAdvance(shortcut);
				if (menuHasLeadingColumn(menu))
					width += UiMetrics::menuIconSize() + UiMetrics::menuIconTextGap();
				contentWidth = qMax(contentWidth, width);
			}
			return QSize(qMax(UiMetrics::menuMinWidth(), contentWidth
				+ 4 * UiMetrics::baseG() + UiMetrics::menuIconTextGap()
				+ UiMetrics::dropdownSlot()), UiMetrics::menuItemHeight());
		}
		return QProxyStyle::sizeFromContents(type, option, size, widget);
	}

	void mUiStyle::drawPrimitive(PrimitiveElement element, const QStyleOption* option,
		QPainter* painter, const QWidget* widget) const
	{
		// 接管面板类绘制：Ribbon 按钮/菜单的背景由 drawControl 统一自绘，此处跳过底座默认
		switch (element)
		{
		case PE_PanelButtonTool:
			if (widget && isRibbonToolButton(widget))
				return; // 背景在 drawControl(CE_ToolButtonLabel) 里统一画
			break;
		case PE_IndicatorArrowDown:
			// 底座样式（Fusion/Windows）在 QSS 管不到的自绘路径下，会用 PE_IndicatorArrowDown
			// 给带菜单的按钮画默认小三角——与 mUiStyle 在 CE_ToolButtonLabel 里自绘的箭头
			// 叠加成双三角（单位/剖面/管理布局/重置布局的两个三角就是这么来的）。
			// Ribbon 按钮的箭头统一由 drawToolButton 自绘，这里拦截底座的默认箭头
			if (widget && isRibbonToolButton(widget))
				return;
			break;
		case PE_IndicatorArrowRight:
			// 子菜单右箭头兜底：菜单被 QSS 抛光后由 QStyleSheetStyle 绘制行内容，
			// 自绘的 drawMenuItem 被绕过；子菜单箭头走 PE_IndicatorArrowRight 经
			// baseStyle 链回到这里。这里统一用 drawSubMenuMark 画与规则 1.0 右槽
			// 一致的实心小三角，保证自绘/QSS 两条路径下箭头都稳定可见。
			// 已安装本样式的菜单由 installMenu 打上 mxUiMenu 属性（QSS 抛光后
			// widget->style() 已变成 QStyleSheetStyle，无法再按样式指针识别）
			if (widget && widget->property("mxUiMenu").toBool())
			{
				drawSubMenuMark(painter, option->rect,
					(option->state & State_Enabled) ? UiPalette::text() : UiPalette::disabled());
				return;
			}
			break;
		case PE_PanelMenu:
		{
			// 菜单容器：surface 底 + 圆角 4，不画边框（规则 1.0：radius 固定 4，
			// 与 mUiMenu::paintAll 容器底同源）。必须同时认 mxUiMenu 属性标记：
			// 应用级 QSS 抛光后 widget->style() 是 QStyleSheetStyle 包裹（!= this），
			// 只认样式指针会把纳管菜单落回底座样式，底座自带 1px 边框的容器底
			if (widget && (widget->style() == this || widget->property("mxUiMenu").toBool()))
			{
				painter->save();
				painter->setRenderHint(QPainter::Antialiasing);
				painter->setPen(Qt::NoPen);
				painter->setBrush(UiPalette::surface());
				// 整 rect 填充不留 -1：与 mUiMenu::paintAll 同理，mask/窗口内未绘制
				// 的 1px 条带会渲染成黑色边线
				painter->drawRoundedRect(option->rect, kRadius, kRadius);
				painter->restore();
				return;
			}
			break;
		}
		case PE_IndicatorMenuCheckMark:
		{
			// 勾选标识统一在 drawMenuItem 左列自绘（打勾线，无方框底座）。
			// 这里必须拦截底座样式：Fusion/QSS 底座会把勾选槽画成 sunken 凹面板
			//（BevelPanel 底 + 方框/圆点），视觉上就是"勾选框底下的凹槽"
			if (widget && (widget->style() == this || widget->property("mxUiMenu").toBool()))
				return;
			break;
		}
		case PE_IndicatorCheckBox:
		case PE_IndicatorRadioButton:
		{
			// 菜单勾选槽兜底拦截：Fusion 会把菜单勾选槽当普通 CheckBox/Radio 指示器
			// 画（带 Bevel 凹槽），菜单内统一拦截，勾选只由 drawMenuItem 左列自绘
			if (widget && (widget->property("mxUiMenu").toBool()
				|| qobject_cast<const QMenu*>(widget) || widget->style() == this))
				return;
			break;
		}
		case PE_PanelItemViewRow:
		case PE_PanelItemViewItem:
		{
			// 菜单行/格面板：拦截底座样式给行槽/勾选槽画的 sunken 面板（凹槽）。
			// 行 hover 底由 drawMenuItem / QSS 的 ::item:selected 负责
			if (widget && (widget->style() == this || widget->property("mxUiMenu").toBool()))
				return;
			break;
		}
		default:
			break;
		}
		QProxyStyle::drawPrimitive(element, option, painter, widget);
	}

	void mUiStyle::drawControl(ControlElement element, const QStyleOption* option,
		QPainter* painter, const QWidget* widget) const
	{
		switch (element)
		{
		case CE_TabBarTabShape:
			// QTabBar 的 Shape/Label 分阶段绘制：Shape 只负责背景（规则 1.2 常态 surface、悬停 hover）
			drawTabBarTabShape(option, painter, widget);
			return;
		case CE_TabBarTab:
		case CE_TabBarTabLabel:
			// 复合与 Label 路径都画"文字+底线"（规则 1.2 选中 600 字重+蓝字+2px 底线）；
			// 背景已在 Shape 阶段画过，这里不再重复填充以免盖掉悬停态
			drawTabBarTab(option, painter, widget);
			return;
		case CE_ToolButtonLabel:
			if (widget && isRibbonToolButton(widget))
			{
				drawToolButton(option, painter, widget);
				return;
			}
			break;
		case CE_ToolBoxTabShape:
			break;
		case CE_MenuItem:
			// 接管命令菜单行：凡带 mxUiMenu 标记的菜单（applyMenuStyle 打过标，
			// 含被 QSS 抛光后样式指针已变为 QStyleSheetStyle 的）都走自绘，
			// 不再按"style()==this"判断（QSS 抛光后该判断恒为假，自绘被绕过，
			// 勾选框退回底座默认凹槽样式——这正是改多次无效的根因）
			if (widget && widget->property("mxUiMenu").toBool())
				drawMenuItem(option, painter, widget);
			else
				QProxyStyle::drawControl(element, option, painter, widget);
			return;
		case CE_MenuEmptyArea:
		{
			if (widget && (widget->property("mxUiMenu").toBool() || widget->style() == this))
				painter->fillRect(option->rect, UiPalette::surface());
			else
				QProxyStyle::drawControl(element, option, painter, widget);
			return;
		}
		default:
			break;
		}
		QProxyStyle::drawControl(element, option, painter, widget);
	}

	void mUiStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex* option,
		QPainter* painter, const QWidget* widget) const
	{
		// QToolButton 的完整绘制走 CC_ToolButton（底座样式会在这里面再画一次箭头/背景），
		// Ribbon 按钮已由 CE_ToolButtonLabel 自绘，这里拦截 CC_ToolButton 防止底座再叠加
		if (control == CC_ToolButton && widget && isRibbonToolButton(widget))
			return;
		QProxyStyle::drawComplexControl(control, option, painter, widget);
	}

	QRect mUiStyle::subControlRect(ComplexControl control, const QStyleOptionComplex* option,
		SubControl subControl, const QWidget* widget) const
	{
		return QProxyStyle::subControlRect(control, option, subControl, widget);
	}

	QRect mUiStyle::subElementRect(SubElement element, const QStyleOption* option,
		const QWidget* widget) const
	{
		return QProxyStyle::subElementRect(element, option, widget);
	}

	/* ==================== 页签（规则 1.2） ==================== */

	void mUiStyle::drawTabBarTabShape(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
	{
		Q_UNUSED(widget);
		const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
		if (!tab) return;
		// 背景：常态 surface；悬停 hover；选中不换底（规则 1.2 只改字重/字色/底线）
		const bool hover = (tab->state & State_MouseOver);
		painter->fillRect(tab->rect, hover && (tab->state & State_Enabled)
			? UiPalette::hover() : UiPalette::surface());
	}

	void mUiStyle::drawTabBarTab(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
	{
		const QStyleOptionTab* tab = qstyleoption_cast<const QStyleOptionTab*>(option);
		if (!tab) return;
		const bool enabled = (tab->state & State_Enabled);
		const bool selected = (tab->state & State_Selected);
		const bool focused = (tab->state & State_HasFocus);
		const bool hover = (tab->state & State_MouseOver);

		painter->save();
		painter->setRenderHint(QPainter::Antialiasing, false);

		// 文字：选中 600 字重 + selectedText 蓝字；未选中 400 字重 + text；
		// Label/复合阶段只画文字/底线，背景由 Shape 阶段（drawTabBarTabShape）绘制。
		// 注意像素字号只在字体未设像素号时才覆盖：ThemeManager 已按档位下过 pixelSize，
		// 无条件再设会把同一份 QFont 重解析，部分 Qt5 版本下 painter 丢字（页签整排不显）
		QFont f = widget ? widget->font() : QApplication::font();
		if (f.pixelSize() <= 0)
			f.setPixelSize(UiMetrics::tabFontSize());
		f.setBold(selected && enabled);
		painter->setFont(f);
		painter->setPen(!enabled ? UiPalette::disabled()
			: selected ? UiPalette::selectedText() : UiPalette::text());
		// 对齐用 tightBoundingRect：drawText 的 AlignCenter 在 QTabBar 的 drawControl 里
		// 会叠加 Qt 内部偏移，直接用 painter 画笔按矩形中心画最稳
		const QFontMetrics fm(f);
		const int textW = fm.horizontalAdvance(tab->text);
		const int textX = tab->rect.x() + (tab->rect.width() - textW) / 2;
		const int textY = tab->rect.y() + (tab->rect.height() - fm.height()) / 2 + fm.ascent();
		painter->drawText(textX, textY, tab->text);

		// 选中标识：2px 强调蓝底线，左右与文字边界一致，计入高度（规则 1.2）
		if (selected && enabled)
			painter->fillRect(textX, tab->rect.bottom() - 1, textW, 2, UiPalette::primary());
		if (focused && enabled)
		{
			painter->setPen(QPen(UiPalette::focus(), 2));
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(tab->rect.adjusted(1, 1, -2, -2));
		}
		painter->restore();
	}

	/* ==================== Ribbon 按钮（规则 1.3） ==================== */

	void mUiStyle::drawToolButton(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
	{
		const QStyleOptionToolButton* tb = qstyleoption_cast<const QStyleOptionToolButton*>(option);
		if (!tb) return;

		const bool hover = (tb->state & State_MouseOver) && (tb->state & State_Enabled);
		const bool pressed = (tb->state & State_Sunken);
		const bool checked = (tb->state & State_On) != 0;
		const bool enabled = (tb->state & State_Enabled);
		const bool big = isBigRibbonButton(widget);
		// 附属箭头：由按钮的 mxHasMenu 属性（创建时声明）、QStyleOptionToolButton 的
		// MenuButtonPopup 特征、或按钮确实挂了菜单决定。规则 1.3：有下拉选项才画箭头，
		// 不能再只看 InstantPopup——Qt5 此时不画指示器，但业务上确实需要箭头提示
		const QString wname = widget ? widget->objectName() : QString();
		const QToolButton* tbtn = qobject_cast<const QToolButton*>(widget);
		const bool declaredMenu = widget && widget->property("mxHasMenu").toBool();
		const bool isSplitPopup = (tb->features & QStyleOptionToolButton::MenuButtonPopup);
		const bool realMenu = tbtn && tbtn->menu() != nullptr;
		const bool hasArrow = (!widget || !widget->property("mxNoArrow").toBool())
			&& (declaredMenu || isSplitPopup || realMenu || wname == QStringLiteral("comboDropBtn"));

		QFont f = widget ? widget->font() : tb->font;
		f.setPixelSize(UiMetrics::buttonFontSize());

		drawRibbonButton(painter, tb->rect, tb->text, tb->icon, tb->iconSize, big, hasArrow,
			hover, pressed, checked, enabled, f, widget);
	}

	void mUiStyle::drawButtonState(QPainter* p, const QRect& rect,
		bool hover, bool pressed, bool checked, bool focused)
	{
		p->save();
		p->setRenderHint(QPainter::Antialiasing);
		// 状态在原文点击范围内绘制，不增大宽高（规则 1.3）；常态无外围边框、圆角 4
		if (checked || pressed)
		{
			p->setPen(Qt::NoPen);
			p->setBrush(UiPalette::selected());
			p->drawRoundedRect(rect, kRadius, kRadius);
		}
		else if (hover)
		{
			p->setPen(Qt::NoPen);
			p->setBrush(UiPalette::hover());
			p->drawRoundedRect(rect, kRadius, kRadius);
		}
		if (focused)
		{
			// 键盘焦点：focus 色 2px 内描边（规则 0.2/1.3：向内绘制以免截断，不改变尺寸）
			p->setPen(QPen(UiPalette::focus(), 2));
			p->setBrush(Qt::NoBrush);
			p->drawRoundedRect(rect.adjusted(1, 1, -1, -1), kRadius, kRadius);
		}
		p->restore();
	}

	// 附属箭头绘制复用 mRibbonButton.h 的自由函数 drawDropArrow/drawDropArrowBesideText
	//（同名实现与 MechMainWindow::dropArrowCenterX 同源，不再各存一份）
	void mUiStyle::drawSubMenuMark(QPainter* p, const QRect& rect, const QColor& color)
	{
		// 子菜单右箭头：实心小三角（5×9），以 rect 中心落位。
		// 自绘路径（drawMenuItem）传右槽中心矩形；QSS 兜底路径传 Qt 给出的箭头子矩形，
		// 两者都按中心对齐，形状一致
		p->save();
		p->setRenderHint(QPainter::Antialiasing);
		p->setPen(Qt::NoPen);
		p->setBrush(color);
		const qreal cx = rect.center().x();
		const qreal cy = rect.center().y();
		const qreal w = 5.0, h = 9.0;
		const QVector<QPointF> tri{
			QPointF(cx - w / 2, cy - h / 2), QPointF(cx - w / 2, cy + h / 2),
			QPointF(cx + w / 2, cy) };
		p->drawPolygon(tri);
		p->restore();
	}

	void mUiStyle::drawMenuCheckMark(QPainter* p, const QRect& rect, bool checked,
		bool enabled, bool exclusive)
	{
		// 互斥组未选中项不画空盒：单选语义由选中项的裸对勾表达，画空盒会与复选混淆
		if (!checked && exclusive)
			return;
		p->save();
		p->setRenderHint(QPainter::Antialiasing);
		const qreal box = UiMetrics::choiceSize();
		const QRectF boxRect(rect.center().x() - box / 2.0,
			rect.center().y() - box / 2.0, box, box);
		if (!exclusive)
		{
			if (checked)
			{
				p->setPen(Qt::NoPen);
				p->setBrush(enabled ? UiPalette::primary() : UiPalette::disabled());
				p->drawRoundedRect(boxRect, 2, 2);
			}
			else
			{
				// 未勾选空盒描边不用 border()（浅 #dce2e9/护眼 #b1c6a8，在菜单底上几乎不可见），
				// 用 muted()：浅 #788391/深 #a5b2c3/护眼 #617860，三主题下与 surface/hover 底对比均 ≥3:1；
				// 与 mUiMenu::paintCheck 的未勾选空盒严格同源
				QPen pen(enabled ? UiPalette::muted() : UiPalette::disabled(), 1.2,
					Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
				p->setPen(pen);
				p->setBrush(Qt::NoBrush);
				p->drawRoundedRect(boxRect, 2, 2);
			}
		}
		if (checked)
		{
			const QColor color = enabled ? (exclusive ? UiPalette::selectedText() : UiPalette::onAction())
				: UiPalette::disabled();
			QPen pen(color, 1.6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
			p->setPen(pen);
			p->setBrush(Qt::NoBrush);
			const qreal cx = rect.center().x();
			const qreal cy = rect.center().y();
			const qreal w = qMin<qreal>(10.0, rect.width() * 0.75);
			const qreal h = w * 0.55;
			// 打勾线：左下拐点 → 底部 → 右上
			p->drawPolyline(QVector<QPointF>{
				QPointF(cx - w / 2, cy), QPointF(cx - w * 0.12, cy + h / 2), QPointF(cx + w / 2, cy - h / 2) });
		}
		p->restore();
	}

	void mUiStyle::drawRibbonButton(QPainter* p, const QRect& rect, const QString& text,
		const QIcon& icon, const QSize& iconSize, bool big, bool hasArrow,
		bool hover, bool pressed, bool checked, bool enabled, const QFont& font, const QWidget* widget)
	{
		p->save();

		// 状态背景（常态透明，圆角 4）
		drawButtonState(p, rect, hover, pressed, checked, false);

		const QColor fg = enabled ? UiPalette::text() : UiPalette::disabled();
		const int inset = UiMetrics::menuInset();          // 左右内留白 G−2
		const int contentGap = UiMetrics::menuContentGap(); // 内部图文间距 max(1,G−4)
		const QRect content = rect.adjusted(inset, 0, -inset, 0);

		p->setFont(font);
		const QFontMetrics fm(font);
		const int textW = text.isEmpty() ? 0 : fm.horizontalAdvance(text);

		if (big)
		{
			// 全高按钮：大图标在上，文字行盒底距按钮底 bigButtonTextBottomPadding（规则 1.3 基线）
			const qreal bottomPad = UiMetrics::bigButtonTextBottomPaddingF();
			const int iconEdge = UiMetrics::menuLargeIconSize(); // menuLargeIcon = H+4（标准 30）
			const int lineH = UiMetrics::menuLineHeight();

			// 文字行盒（单行，不折行；宽度不足由 ThemeManager 宽度公式兜底）
			const QRectF textBox(content.x(), rect.bottom() + 1.0 - bottomPad - lineH,
				content.width(), lineH);
			// 图标盒：文字行盒顶 - 图文间距，向上放图标；剩余高度自然留在顶部
			const int iconTop = qRound(textBox.top() - contentGap - iconEdge);
			const int textLayoutW = textW + (hasArrow ? UiMetrics::dropArrowTextGap() + kDropArrowGlyphWidth : 0);
			const int textX = content.x() + (content.width() - textLayoutW) / 2;

			if (!icon.isNull())
			{
				const QPixmap pm = icon.pixmap(QSize(iconEdge, iconEdge),
					enabled ? QIcon::Normal : QIcon::Disabled);
				const int ix = textX + (textLayoutW - pm.width() / pm.devicePixelRatio()) / 2;
				p->drawPixmap(ix, iconTop, pm);
			}
			if (!text.isEmpty())
			{
				p->setPen(fg);
				// 文字基线锚定行盒；箭头中心取实际渲染字形（tightBoundingRect）
				// 的垂直中心——fm.ascent 不含字体内置行距，用它推算视觉中心
				// 会把箭头推到字形上方（"与文字顶部齐平"的根因）
				const qreal baseY = textBox.bottom() - fm.descent();
				p->drawText(QPointF(textX, baseY), text);
				if (hasArrow)
				{
					QRect tight = fm.tightBoundingRect(text);
					tight.setX(0);
					tight.setWidth(textW);
					tight.translate(textX, baseY);
					drawDropArrowBesideText(p, tight, UiMetrics::dropArrowTextGap(), fg);
				}
			}
			else if (hasArrow)
			{
				// 无文字纯箭头（兜底）：行盒视觉中心
				const int ax = content.center().x();
				const int cy = textBox.center().y() - (fm.ascent() - fm.descent()) / 2;
				drawDropArrow(p, QRect(ax - 5, cy - (fm.ascent() + fm.descent()) / 2,
					11, fm.ascent() + fm.descent()), fg);
			}
		}
		else
		{
			// 二分按钮/组合钮文字区：图标在左、文字在右，整体左对齐垂直居中（规则 1.3）；
			// 组合钮子钮（comboMainBtn 纯图标居中 / comboDropBtn 文字居中+箭头紧随）
			// 走专门的居中分支，避免 Qt 居中布局把文字盒宽压没
			const QString name = widget ? widget->objectName() : QString();
			if (name == QStringLiteral("comboMainBtn"))
			{
				// 图标区：纯大图标整体居中（规则 1.3：图标与普通全高按钮同大同位）
				if (!icon.isNull())
				{
					const int iconEdge = UiMetrics::comboIconSize().width();
					const QPixmap pm = icon.pixmap(QSize(iconEdge, iconEdge),
						enabled ? QIcon::Normal : QIcon::Disabled);
					const int iw = int(pm.width() / pm.devicePixelRatio());
					const int ih = int(pm.height() / pm.devicePixelRatio());
					p->drawPixmap(rect.x() + (rect.width() - iw) / 2, rect.y() + (rect.height() - ih) / 2, pm);
				}
			}
		else if (name == QStringLiteral("comboDropBtn"))
		{
			// 文字区：文字+箭头作为整体水平居中、与文字共用基线（规则 1.3 双动作文字区）。
			// 箭头按实际笔画宽度计入总宽；无箭头按钮不参与这项偏移。
			const int totalW = textW + (hasArrow ? UiMetrics::dropArrowTextGap() + kDropArrowGlyphWidth : 0);
			const int x = rect.x() + qMax(0, (rect.width() - totalW) / 2);
			if (!text.isEmpty())
			{
				p->setPen(fg);
				const int baseY = rect.y() + (rect.height() + UiMetrics::menuLineHeight()) / 2 - fm.descent();
				p->drawText(QPoint(x, baseY), text);
				if (hasArrow)
				{
					QRect tight = fm.tightBoundingRect(text);
					tight.setX(0);
					tight.setWidth(textW);
					tight.translate(x, baseY);
					drawDropArrowBesideText(p, tight, UiMetrics::dropArrowTextGap(), fg);
				}
			}
			else if (hasArrow)
			{
				const int gh = fm.ascent() + fm.descent();
				const int cy = rect.center().y() - (fm.ascent() - fm.descent()) / 2;
				drawDropArrow(p, QRect(x - 5, cy - gh / 2, 11, gh), fg);
			}
		}
			else
			{
				const int iconEdge = UiMetrics::menuIconSize(); // 图标盒按产品保留值，标准 20
				int x = content.x();
				if (!icon.isNull())
				{
					const QPixmap pm = icon.pixmap(QSize(iconEdge, iconEdge),
						enabled ? QIcon::Normal : QIcon::Disabled);
					const int iy = rect.y() + (rect.height() - int(pm.height() / pm.devicePixelRatio())) / 2;
					p->drawPixmap(x, iy, pm);
					x += iconEdge + contentGap;
				}
				if (!text.isEmpty())
				{
					p->setPen(fg);
					// 文字行垂直居中锚定基线；箭头中心取实际渲染字形
					//（tightBoundingRect）的垂直中心，与字号/字体解耦
					const int baseY = rect.y() + (rect.height() + UiMetrics::menuLineHeight()) / 2 - fm.descent();
					p->drawText(QPoint(x, baseY), text);
					if (hasArrow)
					{
						QRect tight = fm.tightBoundingRect(text);
						tight.setX(0);
						tight.setWidth(textW);
						tight.translate(x, baseY);
						drawDropArrowBesideText(p, tight, UiMetrics::dropArrowTextGap(), fg);
					}
				}
				else if (hasArrow)
				{
					const int gh = fm.ascent() + fm.descent();
					const int cy = rect.center().y() - (fm.ascent() - fm.descent()) / 2;
					drawDropArrow(p, QRect(x - 5, cy - gh / 2, 11, gh), fg);
				}
			}
		}
		p->restore();
	}

	/* ==================== 命令菜单行（规则 1.0） ==================== */

	void mUiStyle::drawMenuItem(const QStyleOption* option, QPainter* painter, const QWidget* widget) const
	{
		const QStyleOptionMenuItem* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(option);
		if (!mi) return;

		painter->save();
		const bool enabled = (mi->state & State_Enabled);
		const bool selected = (mi->state & State_Selected) && enabled;
		const bool hover = (mi->state & State_MouseOver) && enabled && !selected;
		const bool checked = mi->checked;

		// 行背景：常态 surface；悬停/键盘选中铺 hover 底，再描 1px focus 内缩边框
		//（对齐原型 ribbon-create-option:hover 的 outline:1px solid focus;
		// outline-offset:-1px，鼠标悬浮与键盘焦点同款，与 mUiMenu::paintItem 同口径）。
		// 勾选当前项不铺任何底色：打勾线已足以标识选中态，
		// 整块底色与"干净打勾"的视觉语言冲突
		if (selected || hover)
		{
			painter->fillRect(mi->rect, UiPalette::hover());
			painter->setPen(QPen(UiPalette::focus(), 1));
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(mi->rect.adjusted(0, 0, -1, -1));
		}

		// 分隔线：border 色 1px，端点距外边框 2G（规则 1.0：与行内容边界一致，菜单内容距外边框 2G）
		if (mi->menuItemType == QStyleOptionMenuItem::Separator)
		{
			const int endInset = UiMetrics::menuItemInset() + qMax(1, UiMetrics::baseG() - 2) + 1;
			const int y = mi->rect.center().y();
			painter->setPen(QPen(UiPalette::border(), 1));
			painter->drawLine(mi->rect.left() + endInset, y, mi->rect.right() - endInset, y);
			painter->restore();
			return;
		}

		// 行内容端点距外边框 2G，始终从菜单外框计算，不能把行局部坐标当作外框坐标。
		const int contentX = 2 * UiMetrics::baseG();
		const int iconEdge = UiMetrics::menuIconSize();
		const int iconTextGap = UiMetrics::menuIconTextGap();
		int x = contentX;

		// 勾选标识与子菜单箭头共用右槽中心；图标列仅由真实图标决定。
		const bool hasLeadingColumn = menuHasLeadingColumn(qobject_cast<const QMenu*>(widget));
		if (hasLeadingColumn)
		{
			if (!mi->icon.isNull())
			{
				const QPixmap pm = mi->icon.pixmap(QSize(iconEdge, iconEdge),
					enabled ? QIcon::Normal : QIcon::Disabled, checked ? QIcon::On : QIcon::Off);
				const int iy = mi->rect.y() + (mi->rect.height() - pm.height() / pm.devicePixelRatio()) / 2;
				painter->drawPixmap(x, iy, pm);
			}
			x += iconEdge + iconTextGap;
		}

		// 文字：正文 body（标准 12px）/ 400 字重（规则 0：弹出菜单选项取正文参数）
		QFont f = mi->font;
		f.setPixelSize(UiMetrics::bodyFontSize());
		f.setBold(false);
		painter->setFont(f);
		painter->setPen(enabled ? UiPalette::text() : UiPalette::disabled());

		// 右槽预留：文字不进入右槽；右槽中心距外边框 2G+Q/2（已由 menuRightSlotPadding 给出）
		const int slotReserve = 2 * UiMetrics::baseG() + UiMetrics::dropdownSlot()
			+ UiMetrics::menuIconTextGap();
		QString text = mi->text;
		QString shortcut;
		const int tabIdx = text.indexOf(QLatin1Char('\t'));
		if (tabIdx >= 0)
		{
			shortcut = text.mid(tabIdx + 1);
			text = text.left(tabIdx);
		}
		const QRect textRect(x, mi->rect.y(), mi->rect.right() - slotReserve - x, mi->rect.height());
		painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, text);
		if (!shortcut.isEmpty())
		{
			painter->setPen(enabled ? UiPalette::muted() : UiPalette::disabled());
			const QRect scRect(x, mi->rect.y(), mi->rect.right() - slotReserve - x, mi->rect.height());
			painter->drawText(scRect, Qt::AlignRight | Qt::AlignVCenter | Qt::TextSingleLine, shortcut);
		}

		// 右槽标识：子菜单箭头、单选裸勾、多选方框共用同一中心线。
		const QPointF center = menuRightSlotCenter(widget, mi->rect);
		const QRectF slot(center.x() - UiMetrics::dropdownSlot() / 2.0, mi->rect.y(),
			UiMetrics::dropdownSlot(), mi->rect.height());
		if (mi->checkType != QStyleOptionMenuItem::NotCheckable)
			drawMenuCheckMark(painter, slot.toAlignedRect(), checked, enabled,
				mi->checkType == QStyleOptionMenuItem::Exclusive);
		if (mi->menuItemType == QStyleOptionMenuItem::SubMenu)
		{
			// 右箭头：与 QSS 兜底路径（PE_IndicatorArrowRight）同形同色
			const QColor markColor = enabled ? UiPalette::text() : UiPalette::disabled();
			drawSubMenuMark(painter, QRectF(slot.x(), slot.y(), slot.width(), slot.height()).toRect(), markColor);
		}
		painter->restore();
	}
}
