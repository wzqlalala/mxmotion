#include "ThemeManager.h"

#include "UiMetrics.h"
#include "UiPalette.h"
#include "mRibbonWidget.h"
#include "mRibbonButton.h"
#include "mWindowTitleBar.h"
#include "mUiStyle.h"
#include "mUiMenu.h"
#include "MyTabBar.h"

#include <QCoreApplication>
#include <QApplication>
#include <QMainWindow>
#include <QSettings>
#include <QFile>
#include <QToolButton>
#include <QPushButton>
#include <QTabBar>
#include <QLabel>
#include <QScrollArea>
#include <QStackedWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSpacerItem>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QContextMenuEvent>
#include <QCheckBox>
#include <QRadioButton>
#include <QSlider>
#include <QDialog>
#include <QTreeView>
#include <QTreeWidget>
#include <QTableWidget>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QSet>
#include <QMetaObject>
#include <QFont>
#include <QEvent>
#include <QTimer>
#include <QPointer>

#include <vector>

#ifdef Q_OS_WIN
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif
#include <QFontMetrics>
#include <QStyle>

using namespace MxFunctions;

namespace MechMainWindow
{
	namespace
	{
		// 主题代数：每次外观实际下发自增。控件记录自己被下发到的代数，
		// 隐藏的顶层窗/对话框在 Show 时据此懒补发，切档期间不为不可见控件付费
		quint64 s_themeGeneration = 1;

		// 持久化档位/主题是否已读入：ThemeManager 单例首次创建即读一次，
		// 保证远早于主窗口的窗口（启动前许可/登录弹窗）也按用户设置的
		// 档位/主题渲染，而不是落在代码默认的浅色档
		bool s_settingsLoaded = false;

		// QSS 相等跳过：Qt 的 setStyleSheet 不比较新旧串，同串重设也会
		// 走一遍 QSS 解析 + unpolish/polish + 后代失效，是切档卡顿的大头。
		// 生成串不变（如只切主题时所有纯尺寸 QSS）在这里直接短路。
		inline void setQss(QWidget* w, const QString& qss)
		{
			if (w->styleSheet() != qss)
				w->setStyleSheet(qss);
		}

		/*
		* 字体相等跳过：setFont 同字体也会沿父子链传播 FontChange 并
		* 触发 updateGeometry，功能区数百个按钮逐个重设代价可观
		*/
		inline void setFontIfChanged(QWidget* w, const QFont& f)
		{
			if (w->font() != f)
				w->setFont(f);
		}

		// 有系统边框的顶层窗（Window/Dialog）：菜单、-tooltip 等弹出窗不做 DWM 改色
		bool isFramedTopWindow(const QWidget* w)
		{
			const Qt::WindowFlags flags = w->windowFlags();
			const int type = int(flags & Qt::WindowType_Mask);
			return type == int(Qt::Window) || type == int(Qt::Dialog);
		}

		// 菜单栏总高度收口的 Ribbon 页滚动区高：总高减页签栏高
		int ribbonPageHeight()
		{
			return UiMetrics::menuBarTotalHeight() - UiMetrics::tabBarHeight();
		}

		// 文件按钮与 mRibbonButton 自绘使用同一套档位参数：实际图标宽 + 图文间距
		// + 文字宽 + 左右内留白；高与页签栏平齐。不能再用固定 12px 图标/8px 留白，
		// 否则特大档（20px 图标、G=8）会把末尾文字裁掉。
		void layoutFileButton(QToolButton* fb)
		{
			if (!fb) return;
			QFont f(UiMetrics::uiFontFamily());
			f.setPixelSize(UiMetrics::buttonFontSize());
			const int textW = QFontMetrics(f).horizontalAdvance(fb->text());
			const int iconW = fb->icon().isNull()
				? 0 : UiMetrics::menuIconSize() + UiMetrics::menuContentGap();
			const int arrowW = fb->property("mxNoArrow").toBool() ? 0 : UiMetrics::arrowReserveWidth();
			const int contentW = iconW + textW + arrowW + 2 * UiMetrics::menuInset();
			fb->setFixedSize(qMax(UiMetrics::menuFullMinWidth(), contentW), UiMetrics::tabBarHeight());
		}

		// ThemeManager 直接管理尺寸/样式的特殊按钮：
		// 顶部快捷栏按钮、翻页按钮——这些有专属分支或不走 Ribbon 规则，
		// 普通按钮重算与宽度兜底都必须跳过它们，避免两套口径互相覆盖
			bool isManagedSpecialButton(const QString& name)
			{
				static const QSet<QString> kSpecial = {
					QStringLiteral("FileButton"), QStringLiteral("ViewButton"),
					QStringLiteral("m_leftScrollBtn"), QStringLiteral("m_rightScrollBtn"),
					QStringLiteral("QuickNewBtn"), QStringLiteral("QuickSaveBtn"), QStringLiteral("QuickOpenBtn")
				};
				return kSpecial.contains(name);
			}

			bool isRibbonInput(const QWidget* w)
			{
				for (const QWidget* p = w; p; p = p->parentWidget())
				{
					if (p->objectName() == QStringLiteral("mGroupWidget"))
						return true;
				}
				return false;
			}

			void applyRibbonLineEditStyle(QLineEdit* edit)
			{
				// 使用 Qt 原生行编辑绘制；QSS 只统一颜色/边框，不介入文本绘制。
				// 边框内保留 1px，文字和光标共用同一内容矩形，焦点只换边框色。
				edit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				edit->setTextMargins(0, 0, 0, 0);
				edit->setFixedHeight(UiMetrics::smallButtonHeight());
				setQss(edit, QStringLiteral(
					"QLineEdit { font-family:'%8'; font-size:%1px; color:%2; background:%3; border:1px solid %4; border-radius:2px; padding:0px; }"
					"QLineEdit:hover { border-color:%5; }"
					"QLineEdit:focus { border-color:%5; }"
					"QLineEdit:disabled { color:%6; background:%7; }"
				).arg(UiMetrics::buttonFontSize())
				 .arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex())
				 .arg(UiPalette::focusHex())
				 .arg(UiPalette::disabledHex(), UiPalette::panelHex())
				 .arg(UiMetrics::uiFontFamily()));
				edit->ensurePolished();
			}

			void applyRibbonSpinBoxStyle(QDoubleSpinBox* spin)
			{
				spin->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				spin->setFixedHeight(UiMetrics::smallButtonHeight());
				// 完整声明上下按钮的位置/尺寸，避免 Qt 默认子控件矩形挤压编辑区。
				setQss(spin, QStringLiteral(
					"QDoubleSpinBox { font-family:'%8'; font-size:%1px; color:%2; background:%3; border:1px solid %4; border-radius:2px; padding:0px; }"
					"QDoubleSpinBox:hover { border-color:%5; }"
					"QDoubleSpinBox:focus { border-color:%5; }"
					"QDoubleSpinBox:disabled { color:%6; background:%7; }"
					"QDoubleSpinBox::up-button, QDoubleSpinBox::down-button { subcontrol-origin:border; width:14px; height:12px; border:none; background:transparent; }"
					"QDoubleSpinBox::up-button { subcontrol-position:top right; }"
					"QDoubleSpinBox::down-button { subcontrol-position:bottom right; }"
				).arg(UiMetrics::buttonFontSize())
				 .arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex())
				 .arg(UiPalette::focusHex())
				 .arg(UiPalette::disabledHex(), UiPalette::panelHex())
				 .arg(UiMetrics::uiFontFamily()));
				if (auto* edit = spin->findChild<QLineEdit*>())
				{
					// 外框只画一次；完整覆盖全局裸 QLineEdit 的字体、高度和边框，
					// 防止内部编辑器再次继承一套内容矩形。
					setQss(edit, QStringLiteral(
						"QLineEdit { font-family:'%4'; font-size:%1px; height:auto; color:%2; border:none; background:transparent; padding:0px; margin:0px; }"
						"QLineEdit:disabled { color:%3; background:transparent; }"
					).arg(UiMetrics::buttonFontSize()).arg(UiPalette::textHex(), UiPalette::disabledHex())
					 .arg(UiMetrics::uiFontFamily()));
					edit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
					edit->setTextMargins(0, 0, 0, 0);
					edit->ensurePolished();
				}
			}

		// 大/小按钮判定：优先 uiBigButton 标记（setQToolButton 打标），
		// 否则按创建时记录的意图图标尺寸 uiIconSize（<=24 为小按钮），兜底用布局方式
		bool isBigButton(const QToolButton* btn)
		{
			const QVariant big = btn->property("uiBigButton");
			if (big.isValid())
				return big.toBool();
			const QVariant isz = btn->property("uiIconSize");
			if (isz.isValid())
				return isz.toSize().width() > 24;
			return btn->toolButtonStyle() == Qt::ToolButtonTextUnderIcon;
		}
	}

	ThemeManager::ThemeManager(QObject* parent) : QObject(parent)
	{
		// 全局 Show 过滤：隐藏对话框/顶层窗显示时按主题代数懒补发（见 eventFilter）
		qApp->installEventFilter(this);
	}

	ThemeManager* ThemeManager::instance()
	{
		static ThemeManager* s = new ThemeManager(qApp);
		// 首次创建可能远早于 mMenuWidget（启动前的许可弹窗在此刻构造取色）：
		// 先读一次持久化档位/主题，否则弹窗按默认浅色渲染（用户设的护眼/深色失效）
		if (!s_settingsLoaded)
			s->loadSettings();
		return s;
	}

	void ThemeManager::loadSettings()
	{
		const auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings settings(path, QSettings::IniFormat);
		const int lv = settings.value("menubar/uiSizeLevel", 3).toInt();
		const int th = settings.value("menubar/uiTheme", 0).toInt();
		UiMetrics::setLevel(lv);
		UiPalette::setTheme(static_cast<UiPalette::Theme>(th < 0 || th > 2 ? 0 : th));
		s_settingsLoaded = true;
	}

	void ThemeManager::saveSettings()
	{
		const auto path = QCoreApplication::applicationDirPath() + "/../Setting" + "/Settings.ini";
		QSettings settings(path, QSettings::IniFormat);
		settings.setValue("menubar/uiSizeLevel", UiMetrics::level());
		settings.setValue("menubar/uiTheme", static_cast<int>(UiPalette::theme()));
	}

	void ThemeManager::applyLevel(int level, QWidget* root)
	{
		if (level < 1) level = 1;
		if (level > 5) level = 5;
		const bool metricsChanged = (UiMetrics::level() != level);
		UiMetrics::setLevel(level);
		saveSettings();
		// 档位没变就不再全量重发一遍样式（此前同值重设也要整树重抛光）
		if (metricsChanged)
			applyAllInternal(root, MaskMetrics);
	}

	void ThemeManager::applyTheme(int themeIndex, QWidget* root)
	{
		if (themeIndex < 0 || themeIndex > 2) themeIndex = 0;
		const bool paletteChanged = (static_cast<int>(UiPalette::theme()) != themeIndex);
		UiPalette::setTheme(static_cast<UiPalette::Theme>(themeIndex));
		saveSettings();
		if (paletteChanged)
			applyAllInternal(root, MaskPalette);
	}

	void ThemeManager::applyAppearance(int level, int themeIndex, QWidget* root)
	{
		// 档位+主题一次生效：避免先 applyLevel 再 applyTheme 触发两轮全量 applyAll
		//（整树样式重发 ×2，切档明显卡顿且中途闪一帧旧主题）
		if (level < 1) level = 1;
		if (level > 5) level = 5;
		if (themeIndex < 0 || themeIndex > 2) themeIndex = 0;
		const bool metricsChanged = (UiMetrics::level() != level);
		const bool paletteChanged = (static_cast<int>(UiPalette::theme()) != themeIndex);
		UiMetrics::setLevel(level);
		UiPalette::setTheme(static_cast<UiPalette::Theme>(themeIndex));
		saveSettings();
		if (metricsChanged || paletteChanged)
			applyAllInternal(root,
				(metricsChanged ? MaskMetrics : 0) | (paletteChanged ? MaskPalette : 0));
	}

	void ThemeManager::applyAll(QWidget* root)
	{
		// 启动路径（mMenuWidget 构造/mAppInitializer）：维度未知，全量下发
		applyAllInternal(root, MaskAll);
	}

	void ThemeManager::applyAllInternal(QWidget* root, int mask)
	{
		if (!root) return;
		++s_themeGeneration;
		// 规则自绘样式接管 Ribbon 按钮/页签/菜单行的绘制（幂等）：
		// 尺寸与颜色只由 UiMetrics/UiPalette 决定，QSS 仅保留非关键容器底色
		mUiStyle::install(root);
		// QSS 多数同时嵌字号与颜色（任一维度变化串都会变），
		// 相等跳过保证未变化的串零成本
		applyPalette(root);

		// 保持应用级默认字体始终跟随规范字体族（覆盖未走 QSS 的自绘控件、Qwt 坐标轴等）
		if (qApp)
		{
			QFont appFont = qApp->font();
			if (appFont.family() != UiMetrics::uiFontFamily())
			{
				appFont.setFamily(UiMetrics::uiFontFamily());
				qApp->setFont(appFont);
			}
		}

		// 全局基础控件样式（原静态 qss）：按当前令牌填空后全局重发，
		// 与菜单栏树同代数生效——不再出现"启动后全局串压顶令牌"的时序
		applyGlobalQss();
		// 尺寸/字号/布局只在档位变化时重做：纯切主题不再触发
		// 数百个按钮的 unpolish/polish、多轮布局激活与主窗全量重排
		if (mask & MaskMetrics)
			applyMetrics(root);
		mUiStyle::refresh(root);
		// 全局补发（首选项对话框/树/状态栏/工具栏/标题栏/dock）延迟一拍：
		// 菜单栏先画出来，用户点击"应用"后立即看到新样式，
		// 外围窗口在下一轮事件循环补齐（视觉终态与同步执行完全一致）
		QPointer<QWidget> guard(root);
		QTimer::singleShot(0, this, [this, guard]()
		{
			if (guard)
				applyGlobalRound();
		});
	}

	void ThemeManager::applyGlobalRound()
	{
		// 多棵树共享的宿主（dock 容器等）只补发一次：
		// 此前每棵树都把全部祖先重发一遍，共享祖先被反复 setStyleSheet
		QSet<QWidget*> treeHostsDone;
		for (QWidget* top : QApplication::topLevelWidgets())
		{
			// 首选项等通用表单对话框同步换肤：只处理当前可见的；
			// 隐藏对话框在 Show 时按主题代数懒补发（eventFilter），
			// 此前所有隐藏 QDialog 也整树重发样式，是切档卡顿的大头之一
			if (qobject_cast<QDialog*>(top))
			{
				// 自样式弹窗（mxSelfStyled 标记）：无论可见与否整体跳过——
				// 构造自带完整设计稿 QSS 的自绘弹窗，整窗 applyDialogTheme 的
				// QSS 整表替换会冲掉自设样式；且均为无边框自绘，无原生标题栏随主题需求
				if (top->property(kSelfStyledProp).toBool())
					continue;
				if (top->isVisible())
				{
					applyDialogTheme(top);
					if (top->windowHandle())
						applyTitleBarTheme(top);
				}
				continue;
			}
			// 顶层窗标题栏（Windows 非客户区）改色：只处理已可见窗口——
			// winId() 会给隐藏窗强制创建原生窗口（隐藏对话框首次被切档牵连
			// 建 HWND 非常贵）；隐藏窗显示时由 eventFilter 补发
			if (top->isWindow() && top->isVisible() && top->windowHandle()
				&& isFramedTopWindow(top))
				applyTitleBarTheme(top);
			// 视口工具栏（附 A：视图工具栏/视图显示栏随主题）：主窗 QMainWindow 的工具栏
			// 不在菜单栏控件树内，实现侧经 Q_INVOKABLE applyToolbarTheme 元方法重发
			//（Common 不反向链接 MainWindow 类型，与 refreshToolBarTheme 同一解耦手法）
			if (qobject_cast<QMainWindow*>(top))
				QMetaObject::invokeMethod(top, "applyToolbarTheme", Qt::DirectConnection);
			// 状态栏（附 A：状态栏随主题/档位）：header 底 + text 字 + 顶边 1px border，
			// 内部富文本链接色也在其 applyTheme 里一并重生成
			if (top->isWindow())
			{
				const auto statusBars = top->findChildren<QStatusBar*>();
				for (QStatusBar* sb : statusBars)
					QMetaObject::invokeMethod(sb, "applyTheme", Qt::DirectConnection);
			}
			// 树视图（规则 2.1：模型树/视图树/后处理树等）仅窗口层随主题：
			// 树区底色 surface、文字 text；树结构（行高/缩进/字号/选中样式）按最新规则不随档位，保持现状。
			// 同一趟里顺带处理树所在的 dock 容器（ADS CDockWidget 及其内部的树窗口宿主）底色：
			// 否则 registerDock 创建时的 surface 色在切主题后残留（浅色条带）；
			// 并通知树窗口自身重发工具条样式（refreshToolBarTheme，存在的树窗口均有该方法）
			const auto trees = top->findChildren<QTreeView*>();
			for (QTreeView* tv : trees)
			{
				applyTreeTheme(tv);
				for (QWidget* p = tv->parentWidget(); p; p = p->parentWidget())
				{
					if (treeHostsDone.contains(p))
						continue;
					treeHostsDone.insert(p);
					QMetaObject::invokeMethod(p, "refreshToolBarTheme", Qt::DirectConnection);
					if (!p->styleSheet().isEmpty())
						setQss(p, QStringLiteral("background-color: %1;").arg(UiPalette::surfaceHex()));
					if (p->autoFillBackground())
					{
						QPalette pp = p->palette();
						pp.setColor(QPalette::Window, UiPalette::surface());
						pp.setColor(QPalette::Base, UiPalette::surface());
						p->setPalette(pp);
					}
				}
			}

			// 参数栏面板（mPanelBase 系三方参数面板）：控件状态样式是面板级
			// 样式表（宿主祖先样式表会盖掉应用级 QSS，见 mParamBarStyle），值取自
			// UiPalette/UiMetrics，切主题/切档后须逐面板重发；Common 不反向链接
			// CommonWidgets，经 mxParamPanel 标记 + applyTheme 元方法解耦
			//（与状态栏 applyTheme/树宿主 refreshToolBarTheme 同一手法）。
			// 面板本体也可能是顶层：mInfoWindow::setMainWidget 切换面板时对非当前
			// 面板 setParent(nullptr) 摘挂缓存（ensurePanel 复用），切主题时它们
			// 不在任何容器子树内，只查各顶层窗的后代必漏——停留面板能刷新、
			// 摘挂面板残留旧主题的根因。顶层自身与后代都要查
			auto applyPanelTheme = [](QWidget* w)
			{
				if (w->property("mxParamPanel").toBool())
					QMetaObject::invokeMethod(w, "applyTheme", Qt::DirectConnection);
			};
			applyPanelTheme(top);
			for (QWidget* p : top->findChildren<QWidget*>())
				applyPanelTheme(p);
		}
		// 停靠系统（ADS 页签/标题栏/关闭按钮）随主题/档位重发（规则 0.2/1.2）；
		// 页签栏高、关闭钮尺寸也随档位变化，与树重发同一时机
		applyDockTheme();
	}

	bool ThemeManager::eventFilter(QObject* obj, QEvent* ev)
	{
		// 通用编辑控件右键菜单拦截：QLineEdit/QTextEdit 等的 createStandardContextMenu
		// 产出裸 QMenu，悬浮行回退 Qt 底座样式（高亮底 + 白字），这里统一接管为
		// mUiMenu 自绘菜单。注意 mChineseTextEdit 自身重写 contextMenuEvent 时已用
		// mUiMenu 并 accept 事件，本过滤器不会再命中（事件已被消费）。
		if (ev->type() == QEvent::ContextMenu)
		{
			if (interceptEditContextMenu(obj, ev))
				return true;
		}
		// 顶层窗显示时补发换肤：隐藏期间错过主题下发的对话框/窗口在这里补齐。
		// 只拦 Show 且只看窗口级控件，子控件的 Show 事件一概快速放行。
		// 实际补发排队到显示流程完成后：Show 事件内部同步改样式会打断
		// 首次布局/绘制，弹窗出现"样式没加载上"的残破首帧
		if (ev->type() == QEvent::Show)
		{
			if (QWidget* w = qobject_cast<QWidget*>(obj); w && w->isWindow()
				&& w->property("mxThemeGen").toULongLong() != s_themeGeneration)
			{
				QPointer<QWidget> win(w);
				QMetaObject::invokeMethod(this, [this, win]()
				{
					if (win)
						themeWindowLazy(win);
				}, Qt::QueuedConnection);
			}
		}
		return QObject::eventFilter(obj, ev);
	}

	bool ThemeManager::interceptEditContextMenu(QObject* obj, QEvent* ev)
	{
		// 只接管通用可编辑控件的标准编辑菜单；业务自定义 contextMenuEvent 的控件
		//（树/表格/图形画布等）不在此列。只读/禁用的编辑框 Qt 仍会弹"复制/全选"
		// 菜单，语义保持一致，不按可写状态过滤。
		QLineEdit* edit = qobject_cast<QLineEdit*>(obj);
		QTextEdit* textEdit = edit ? nullptr : qobject_cast<QTextEdit*>(obj);
		if (!edit && !textEdit)
			return false;
		auto* ce = static_cast<QContextMenuEvent*>(ev);
		QWidget* owner = edit ? static_cast<QWidget*>(edit) : static_cast<QWidget*>(textEdit);

		mUiMenu menu(owner);
		applyMenuStyle(&menu);

		auto addEditAction = [&menu](const QString& text, bool enabled, std::function<void()> fn)
		{
			QAction* act = menu.addAction(text);
			act->setEnabled(enabled);
			QObject::connect(act, &QAction::triggered, &menu, std::move(fn));
			return act;
		};

		if (edit)
		{
			QLineEdit* e = edit;   // 拷贝进 lambda，菜单 exec 期间 owner 必活（父子关系）
			const bool writable = !e->isReadOnly() && e->isEnabled();
			addEditAction(QStringLiteral("撤销(U)"), writable && e->isUndoAvailable(), [e]() { e->undo(); });
			addEditAction(QStringLiteral("恢复(R)"), writable && e->isRedoAvailable(), [e]() { e->redo(); });
			menu.addSeparator();
			addEditAction(QStringLiteral("剪切(T)"), writable && e->hasSelectedText(), [e]() { e->cut(); });
			addEditAction(QStringLiteral("复制(C)"), e->hasSelectedText(), [e]() { e->copy(); });
			addEditAction(QStringLiteral("粘贴(P)"), writable, [e]() { e->paste(); });
			addEditAction(QStringLiteral("删除"), writable && e->hasSelectedText(), [e]() {
				// 无公共 del() 槽：backspace 在光标处退格；有选区时删除选区（QLineEdit 语义）
				e->backspace();
			});
			menu.addSeparator();
			addEditAction(QStringLiteral("选择全部(A)"), !e->text().isEmpty(), [e]() { e->selectAll(); });
		}
		else
		{
			QTextEdit* t = textEdit;
			const bool writable = !t->isReadOnly() && t->isEnabled();
			addEditAction(QStringLiteral("撤销(U)"), writable && t->document()->isUndoAvailable(), [t]() { t->undo(); });
			addEditAction(QStringLiteral("恢复(R)"), writable && t->document()->isRedoAvailable(), [t]() { t->redo(); });
			menu.addSeparator();
			addEditAction(QStringLiteral("剪切(T)"), writable && t->textCursor().hasSelection(), [t]() { t->cut(); });
			addEditAction(QStringLiteral("复制(C)"), t->textCursor().hasSelection(), [t]() { t->copy(); });
			addEditAction(QStringLiteral("粘贴(P)"), writable && t->canPaste(), [t]() { t->paste(); });
			addEditAction(QStringLiteral("删除"), writable && t->textCursor().hasSelection(), [t]() {
				t->textCursor().removeSelectedText();
			});
			menu.addSeparator();
			addEditAction(QStringLiteral("选择全部(A)"), !t->document()->isEmpty(), [t]() { t->selectAll(); });
		}

		ce->accept();
		menu.exec(ce->globalPos());
		return true;
	}

	void ThemeManager::themeWindowLazy(QWidget* w)
	{
		if (!w || !w->isWindow() || !w->isVisible()) return;
		if (w->property("mxThemeGen").toULongLong() == s_themeGeneration) return;

		if (qobject_cast<QDialog*>(w))
		{
			// 自样式弹窗（mxSelfStyled 标记）：构造自带完整设计稿 QSS 的自绘弹窗，
			// 豁免下方 selfThemed 探测与整窗补发——applyDialogTheme 的 QSS 整表
			// 替换会冲掉自设样式；盖当前代数避免每次 Show 都排队一次空跑
			//（标记伴随弹窗终生，代数不会再次落后，不会被"补发"）
			if (w->property(kSelfStyledProp).toBool())
			{
				w->setProperty("mxThemeGen", s_themeGeneration);
			}
			else
			{
				// 自绘标题栏对话框（mFramelessDialog/mFramelessMessageBox 等）：
				// 构造时已对内容区控件调用过 applyDialogTheme（代数打在内容控件上），
				// 只补发过期的内容区，不整窗重发——整窗 QSS 会波及自绘标题栏；
				// 瞬时弹窗（构造即显示、其间无切换）代数一致，天然零开销
				bool selfThemed = false;
				const auto children = w->findChildren<QWidget*>();
				for (QWidget* c : children)
				{
					if (!c->property("mxThemeGen").isValid())
						continue;
					selfThemed = true;
					if (c->property("mxThemeGen").toULongLong() != s_themeGeneration)
						applyDialogTheme(c);
				}
				// 普通对话框（无自管内容区标记）：整窗补发，与切档时的行为一致
				if (!selfThemed)
					applyDialogTheme(w);
			}
		}
		if (w->isVisible() && w->windowHandle() && isFramedTopWindow(w))
			applyTitleBarTheme(w);
	}

	void ThemeManager::applyTitleBarTheme(QWidget* window)
	{
		if (!window) return;

		// 自绘标题栏（主窗与 mFramelessDialog 对话框）：mWindowTitleBar 已自给自足
		//（构造即完整，且在 showEvent/changeEvent 里自检档位/主题变化自动刷新），
		// 这里无需再喂。只需跳过它，不落入下面的原生 DWM 改色分支。
		if (window->findChild<mWindowTitleBar*>(QStringLiteral("mWindowTitleBar")))
			return;

#ifdef Q_OS_WIN
		if (!window->isWindow()) return;
		// winId() 确保 HWND 已创建（未 show 的窗也先拿到句柄）
		const HWND hwnd = reinterpret_cast<HWND>(window->winId());
		if (!hwnd) return;

		// 运行时加载 dwmapi，避免新增链接依赖；失败（非 Win / 过老系统）直接跳过
		using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
		static DwmSetWindowAttributeFn pDwmSetWindowAttribute = []() {
			if (HMODULE h = LoadLibraryW(L"dwmapi.dll"))
				return reinterpret_cast<DwmSetWindowAttributeFn>(
					reinterpret_cast<void*>(GetProcAddress(h, "DwmSetWindowAttribute")));
			return DwmSetWindowAttributeFn(nullptr);
		}();
		if (!pDwmSetWindowAttribute) return;

		// 深色主题切暗色框架（DWMWA_USE_IMMERSIVE_DARK_MODE=20, Win10 1809+），
		// 深色下必须开它标题栏才会真正变深；浅/护眼关闭以恢复亮框
		const BOOL dark = (UiPalette::theme() == UiPalette::Theme::Dark) ? TRUE : FALSE;
		pDwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));

		// caption 底色 / 文字色（DWMWA_CAPTION_COLOR=35, DWMWA_TEXT_COLOR=36, Win11 22000+；
		// 在不支持的系统上返回 E_INVALIDARG，无害跳过）
		const COLORREF caption = RGB(UiPalette::header().red(), UiPalette::header().green(), UiPalette::header().blue());
		const COLORREF captionText = RGB(UiPalette::text().red(), UiPalette::text().green(), UiPalette::text().blue());
		pDwmSetWindowAttribute(hwnd, 35, &caption, sizeof(caption));
		pDwmSetWindowAttribute(hwnd, 36, &captionText, sizeof(captionText));
#else
		Q_UNUSED(window);
#endif
	}

	void ThemeManager::applyDialogTheme(QWidget* dialog)
	{
		if (!dialog) return;

		// 记录本次已下发的主题代数：隐藏期间再次切主题不会被本轮命中，
		// Show 事件过滤器按代数差懒补发
		dialog->setProperty("mxThemeGen", s_themeGeneration);

		// 对话框本体与所有容器底：surface（内容底色），统一绑定规范字体族与正文字号
		const QString fontFam = UiMetrics::uiFontFamily();
		const int bodyPx = UiMetrics::bodyFontSize();
		setQss(dialog, QStringLiteral(
			"QDialog, QWidget { background-color:%1; color:%2; font-family:'%3'; font-size:%4px; }"
		).arg(UiPalette::surfaceHex(), UiPalette::textHex(), fontFam)
		 .arg(bodyPx));

		const auto children = dialog->findChildren<QWidget*>();
		for (QWidget* w : children)
		{
			// 自绘标题栏及其内部控件（图标/标题/最小化/最大化/关闭按钮）自给自足，
			// 不受对话框通用表单样式覆盖
			if (w->objectName() == QStringLiteral("mWindowTitleBar"))
				continue;
			if (w->parentWidget() && w->parentWidget()->objectName() == QStringLiteral("mWindowTitleBar"))
				continue;

			// 树（首选项左侧目录）
			if (auto* tree = qobject_cast<QTreeWidget*>(w))
			{
				// 需求明确的形态树（mLayerManageDlg 图层树等）按属性豁免：
				// setQss 为整体替换语义，主题一经下发会抹掉控件自带的
				// 行高/勾选指示器/选中样式，豁免须在下发前短路（同 mxComboNoTheme）
				if (tree->property("mxTreeNoTheme").toBool())
					continue;
				setQss(tree, QStringLiteral(
					"QTreeWidget { font-family:'%7'; font-size:%8px; background:%1; color:%2; border:1px solid %3; }"
					"QTreeWidget::item { height:26px; background:transparent; color:%2; }"
					"QTreeWidget::item:hover { background:%4; }"
					"QTreeWidget::item:selected { background:%5; color:%6; }"
				).arg(UiPalette::surfaceHex(), UiPalette::textHex(), UiPalette::borderHex(),
					  UiPalette::hoverHex(), UiPalette::selectedHex(), UiPalette::selectedTextHex(),
					  fontFam).arg(bodyPx));
				continue;
			}
			// 下拉框及其弹出列表
			if (auto* combo = qobject_cast<QComboBox*>(w))
			{
				// 需求明确的纯文本观感下拉（mChannelMapDlg 映射四列等）按属性豁免：
				// setQss 为整体替换语义，主题一经下发会抹掉控件自带的
				// "无边框无箭头" QSS，豁免须在下发前短路
				if (combo->property("mxComboNoTheme").toBool())
					continue;
				setQss(combo, QStringLiteral(
					"QComboBox { font-family:'%9'; font-size:%10px; color:%1; background:%2; border:1px solid %3; border-radius:2px; padding:1px 4px; }"
					"QComboBox:hover { border-color:%4; }"
					"QComboBox:focus { border-color:%4; }"
					"QComboBox:disabled { color:%5; background:%6; }"
					"QComboBox QAbstractItemView { font-family:'%9'; font-size:%10px; background:%2; color:%1; border:1px solid %3;"
					" selection-background-color:%7; selection-color:%8; }"
				).arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex(),
					  UiPalette::focusHex(), UiPalette::disabledHex(), UiPalette::panelHex(),
					  UiPalette::selectedHex(), UiPalette::selectedTextHex(), fontFam).arg(bodyPx));
				continue;
			}
			// 输入框 / 计算输入框
			if (auto* edit = qobject_cast<QLineEdit*>(w))
			{
				// 可编辑下拉/数字框的内嵌编辑行不单独套边框：父控件已完整下发样式，
				// 再叠一层 QLineEdit 边框会出现双重边框与双重内边距
				if (qobject_cast<QComboBox*>(edit->parentWidget())
					|| qobject_cast<QAbstractSpinBox*>(edit->parentWidget()))
					continue;
				setQss(edit, QStringLiteral(
					"QLineEdit { font-family:'%7'; font-size:%8px; color:%1; background:%2; border:1px solid %3; border-radius:2px; padding:1px 4px; }"
					"QLineEdit:hover { border-color:%4; }"
					"QLineEdit:focus { border-color:%4; }"
					"QLineEdit:disabled { color:%5; background:%6; }"
				).arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex(),
					  UiPalette::focusHex(), UiPalette::disabledHex(), UiPalette::panelHex(),
					  fontFam).arg(bodyPx));
				continue;
			}
			// 数字输入框
			if (auto* spin = qobject_cast<QDoubleSpinBox*>(w))
			{
				// 文字贴左：默认右对齐，且被样式表接管后上下箭头 subcontrol 回退
				// 默认宽度会挤压左侧文本，这里把文字钉在左缘、上下按钮收窄到 16px
				setQss(spin, QStringLiteral(
					"QDoubleSpinBox { font-family:'%6'; font-size:%7px; color:%1; background:%2; border:1px solid %3; border-radius:2px; padding:1px 4px; text-align:left; }"
					"QDoubleSpinBox:disabled { color:%4; background:%5; }"
					"QDoubleSpinBox::up-button { width:16px; border:none; }"
					"QDoubleSpinBox::down-button { width:16px; border:none; }"
				).arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex(),
					  UiPalette::disabledHex(), UiPalette::panelHex(), fontFam).arg(bodyPx));
				if (auto* le = spin->findChild<QLineEdit*>())
					le->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
				continue;
			}
			// 表格（最近打开列表等）
			if (auto* table = qobject_cast<QTableWidget*>(w))
			{
				setQss(table, QStringLiteral(
					"QTableWidget { font-family:'%7'; font-size:%8px; background:%1; color:%2; border:1px solid %3; gridline-color:%3; }"
					"QTableWidget::item:selected { background:%4; color:%5; }"
					"QHeaderView::section { font-family:'%7'; font-size:%8px; background:%6; color:%2; border:1px solid %3; font-weight:normal; }"
					"QTableCornerButton::section { background:%6; border:1px solid %3; }"
				).arg(UiPalette::surfaceHex(), UiPalette::textHex(), UiPalette::borderHex(),
					  UiPalette::selectedHex(), UiPalette::selectedTextHex(), UiPalette::headerHex(),
					  fontFam).arg(bodyPx));
				continue;
			}
			// 按钮
			if (auto* pbtn = qobject_cast<QPushButton*>(w))
			{
				// 主按钮豁免（约定名 mxBtnNoTheme）：自带完整配色 QSS 的默认/主按钮
				//（mFramelessMessageBox 默认键等），主题重发不整表替换
				if (pbtn->property("mxBtnNoTheme").toBool())
					continue;
				setQss(pbtn, QStringLiteral(
				"QPushButton { font-family:'%9'; font-size:%10px; color:%1;"
				" background:%2; border:1px solid %3; border-radius:3px; padding:3px 10px; }"
				"QPushButton:hover { background:%4; }"
				"QPushButton:pressed, QPushButton:checked { background:%5; color:%6; }"
				"QPushButton:disabled { color:%7; background:%8; }"
			).arg(UiPalette::textHex())
			 .arg(UiPalette::surfaceHex())
			 .arg(UiPalette::borderHex())
			 .arg(UiPalette::hoverHex())
			 .arg(UiPalette::selectedHex())
			 .arg(UiPalette::selectedTextHex())
			 .arg(UiPalette::disabledHex())
			 .arg(UiPalette::panelHex())
			 .arg(fontFam)
			 .arg(bodyPx));
				continue;
			}
			if (auto* tbtn = qobject_cast<QToolButton*>(w))
			{
				setQss(tbtn, QStringLiteral(
					"QToolButton { font-family:'%8'; font-size:%9px; color:%1; background:%2; border:1px solid %3; border-radius:3px; padding:1px 4px; }"
					"QToolButton:hover { background:%4; }"
					"QToolButton:pressed, QToolButton:checked { background:%5; }"
					"QToolButton:disabled { color:%6; background:%7; }"
				).arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex(),
					  UiPalette::hoverHex(), UiPalette::selectedHex(),
					  UiPalette::disabledHex(), UiPalette::panelHex(), fontFam).arg(bodyPx));
				continue;
			}
			// 标签
			if (auto* lbl = qobject_cast<QLabel*>(w))
			{
				if (lbl->property("mxLblNoTheme").toBool())
					continue;
				setQss(lbl, QStringLiteral(
					"QLabel { font-family:'%2'; font-size:%3px; color:%1; background:transparent; border:none; }"
				).arg(UiPalette::textHex(), fontFam).arg(bodyPx));
				continue;
			}
			if (auto* chk = qobject_cast<QCheckBox*>(w))
			{
				setQss(chk, QStringLiteral(
					"QCheckBox { font-family:'%3'; font-size:%4px; color:%1; background:transparent; }"
					"QCheckBox:disabled { color:%2; }"
				).arg(UiPalette::textHex(), UiPalette::disabledHex(), fontFam).arg(bodyPx));
				continue;
			}
			if (auto* radio = qobject_cast<QRadioButton*>(w))
			{
				setQss(radio, QStringLiteral(
					"QRadioButton { font-family:'%3'; font-size:%4px; color:%1; background:transparent; }"
					"QRadioButton:disabled { color:%2; }"
				).arg(UiPalette::textHex(), UiPalette::disabledHex(), fontFam).arg(bodyPx));
				continue;
			}
			// 分割条
			if (auto* split = qobject_cast<QSplitter*>(w))
			{
				setQss(split, QStringLiteral(
					"QSplitter::handle { background:%1; }"
				).arg(UiPalette::borderHex()));
				continue;
			}
			// 堆叠页与空白容器保持 surface 底（被 QDialog,QWidget 规则覆盖，无需单独处理）
		}
	}

	void ThemeManager::applyPalette(QWidget* root)
	{
		if (!root) return;

			// 菜单栏宿主底色
		setQss(root, QStringLiteral(
			"QMenuBar#mMenuWidget { background: %1; color: %2; border: none; }"
			"QMenuBar#mMenuWidget::item { background: transparent; color: %2; }"
		).arg(UiPalette::surfaceHex(), UiPalette::textHex()));

		// 递归处理菜单栏控件树
		const auto children = root->findChildren<QWidget*>();
		for (QWidget* w : children)
		{
			const QString name = w->objectName();

			// 页签栏：行内绘制已由 mUiStyle 自绘（规则 1.2），此处只发容器底色
			if (auto* tabBar = qobject_cast<QTabBar*>(w))
			{
				// 页签溢出时 Qt 内建的左右滚动按钮（QTabBar 的 QToolButton 子控件）
				// 会被全局 QSS 的 QToolButton 21px 白底/边框规则命中（应用级 QSS 在
				// applyAll 之后加载）。控件级样式表优先级更高，用后代选择器压掉：
				// 箭头本身由底座样式绘制（黑色实心箭头）。按钮底为不透明实色，
				// 与功能区翻页按钮（m_leftScrollBtn）同一口径：常态 border 灰底、
				// 悬停 hover、按压 selected、禁用 panel 灰底
				const int scrollerW = qMax(14, UiMetrics::tabBarHeight() / 2);
				setQss(tabBar, QStringLiteral(
					"QTabBar { qproperty-drawBase:false; border:0px; background:%1; }"
					"QTabBar QToolButton { background:%2; border:none; border-radius:3px; width:%3px; }"
					"QTabBar QToolButton:hover { background:%4; border:none; }"
					"QTabBar QToolButton:pressed { background:%5; border:none; }"
					"QTabBar QToolButton:disabled { background:%6; border:none; }"
				).arg(UiPalette::surfaceHex())
				 .arg(UiPalette::borderHex())
				 .arg(scrollerW)
				 .arg(UiPalette::hoverHex())
				 .arg(UiPalette::selectedHex())
				 .arg(UiPalette::panelHex()));
				tabBar->update();
				continue;
			}

			// Ribbon 页（QScrollArea 宿主）
			if (auto* sa = qobject_cast<QScrollArea*>(w))
			{
				// 注意：这里不能给 QScrollArea 声明任何 QSS border——QSS 边框会把
				// viewport 缩进边框宽度（上下各 1px），而内容最小高按准则恰为
				// ribbonHeight（=页最小高），viewport 被压掉 2px 后内容比可视区高
				// 2px，产生 2px 竖向滚动范围，滑轮可上下小幅滚动菜单栏。
				// 功能区上下分界线改由 mRibbonSeparatorOverlay 覆盖层自绘（不影响几何）。
				setQss(sa, QStringLiteral(
					"QScrollArea { border:none; background-color:%1; }"
				).arg(UiPalette::surfaceHex()));
				if (sa->widget())
				{
					// 内容容器只声明类选择器（无 background）：一旦写了 background-color，
					// Qt 会把它提升为对 content 及全部后代 QWidgets 生效，mGroupWidget
					// 组就会被 QStyleSheetStyle 接管绘制，paintEvent 里自绘的组间
					// 分隔线整段丢失（此前"分割线完全消失"的根因）。
					// content 本体透明，露出 QScrollArea 的 surface 底色即可
					setQss(sa->widget(), QStringLiteral(
						".QWidget{border:none;}"
					));
				}
				continue;
			}

			// 组分隔线与底色：mGroupWidget（objectName 已设为 mGroupWidget）
			if (name == QStringLiteral("mGroupWidget"))
			{
				applyGroupWidgetPalette(w);
				w->update(); // paintEvent 分隔线用 UiPalette
			}

			// 组对象会在 applyGroupWidgetPalette 后被遍历到，但普通输入控件不会；
			// 必须单独进入这两个分支，不能在处理完组后 continue 跳过整棵子树。
			if (auto* edit = qobject_cast<QLineEdit*>(w); edit && isRibbonInput(edit) &&
				!qobject_cast<QAbstractSpinBox*>(edit->parentWidget()))
			{
				applyRibbonLineEditStyle(edit);
				edit->setMinimumWidth(0);
				edit->setMaximumWidth(QWIDGETSIZE_MAX);
				edit->setFixedWidth(edit->sizeHint().width());
				continue;
			}
			if (auto* spin = qobject_cast<QDoubleSpinBox*>(w); spin && isRibbonInput(spin))
			{
				applyRibbonSpinBoxStyle(spin);
				continue;
			}
			if (name == QStringLiteral("mGroupWidget"))
				continue;

			// 左右翻页按钮
			if (name == QStringLiteral("m_leftScrollBtn"))
			{
				setQss(w, QStringLiteral(
					"QToolButton#m_leftScrollBtn{background-color:%1;border-right:1px solid %2;border-left:none;border-top:none;border-bottom:none;}"
					"QToolButton#m_leftScrollBtn:hover{background-color:%3;}"
				).arg(UiPalette::borderHex(), UiPalette::surfaceHex(), UiPalette::hoverHex()));
				continue;
			}
			if (name == QStringLiteral("m_rightScrollBtn"))
			{
				setQss(w, QStringLiteral(
					"QToolButton#m_rightScrollBtn{background-color:%1;border-left:1px solid %2;border-right:none;border-top:none;border-bottom:none;}"
					"QToolButton#m_rightScrollBtn:hover{background-color:%3;}"
				).arg(UiPalette::borderHex(), UiPalette::surfaceHex(), UiPalette::hoverHex()));
				continue;
			}

			// 文件按钮
			if (name == QStringLiteral("FileButton"))
			{
				// 文件按钮已是全自绘 mRibbonButton（无 QSS，箭头/文字/图标与基线对齐
				// 由 paintEvent 保证），这里只管布局尺寸，颜色全部实时读 UiPalette
				if (auto* btn = qobject_cast<QToolButton*>(w))
					layoutFileButton(btn);
				continue;
			}

			// 视图按钮 / 快捷按钮（已是全自绘 mRibbonButton 纯图标模式：
			// 图标居中、悬停/按压底色自绘，无需任何 QSS；清历史残留即可）
			if (name == QStringLiteral("ViewButton") || name == QStringLiteral("QuickNewBtn") ||
				name == QStringLiteral("QuickSaveBtn") || name == QStringLiteral("QuickOpenBtn"))
			{
				setQss(w, QString());
				continue;
			}

			if (name == QStringLiteral("comboButton"))
			{
				// mComboButton 是单个全自绘 mRibbonButton（Mode::Split），
				// 构造起不再挂任何 QSS（挂 QSS 会引入 QStyleSheetStyle 绕过自绘）。
				// 主题重发只清历史残留样式表并重挂自绘样式（幂等）
				setQss(w, QString());
				if (mUiStyle* s = mUiStyle::instance())
				{
					if (!w->property("mxUiStyled").toBool())
					{
						w->setProperty("mxUiStyled", true);
						w->setStyle(s);
					}
				}
				continue;
			}

			// 文件/视图下拉菜单（含子菜单，findChildren<QMenu*> 本身递归整棵树，
			// 子菜单会在循环中走到本分支，无需单独再写一遍子菜单样式）
			if (auto* menu = qobject_cast<QMenu*>(w))
			{
				// 全自绘菜单（mUiMenu / mxUiMenu 标记）不再下发任何 QSS——一旦挂 QSS，
				// Qt 会用 QStyleSheetStyle 接管整个 QMenu::item/indicator 绘制，
				// 自绘的勾选框/文字/行高全部失效；主题重发时只需清空，避免把 QSS 重新挂上
				setQss(menu, QString());
				// 菜单内自定义 QWidgetAction 的子控件也跟随主题
				const auto menuChildren = menu->findChildren<QWidget*>();
				for (QWidget* mw : menuChildren)
				{
					if (auto* lbl = qobject_cast<QLabel*>(mw))
					{
						// 禁用的 QLabel 是分组标题行（QWidgetAction 宿主，如单位系统弹窗的
						// "单位系统/角度/转速/温度"）：需 label 字 + panel 灰底，不能被
						// 通用行内标签规则（text 字 + 透明底）覆盖，否则标题底色丢失
						if (!lbl->isEnabled())
						{
							setQss(lbl, QStringLiteral(
								"QLabel{ font-family:'%3'; color:%1; background:%2; padding:2px 0; }"
							).arg(UiPalette::labelHex(), UiPalette::panelHex())
							 .arg(UiMetrics::uiFontFamily()));
							continue;
						}
						setQss(lbl, QStringLiteral(
							"QLabel { color:%1; background:transparent; border:none; }"
						).arg(UiPalette::textHex()));
					}
					else if (auto* chk = qobject_cast<QCheckBox*>(mw))
					{
						setQss(chk, QStringLiteral(
							"QCheckBox { color:%1; background:transparent; }"
							"QCheckBox:disabled { color:%2; }"
						).arg(UiPalette::textHex(), UiPalette::disabledHex()));
					}
				}
				continue;
			}
		}
	}

	void ThemeManager::applyMenuStyle(QWidget* menu)
	{
		if (!menu) return;
		// 彻底重构版：菜单行内容/勾选/箭头/文字全部自绘（mUiMenu 或 mUiStyle 兜底），
		// 容器底色/边框自绘。关键：清空 QSS——只要菜单挂 QSS，Qt 就用 QStyleSheetStyle
		// 接管 QMenu::item/indicator 的绘制，任何 QProxyStyle 自绘都轮不到执行，
		// 这就是"勾选框一直是底座凹槽样式、文字丢失、改了多次无效"的根因。
		setQss(menu, QString());   // 清空 QSS，退出 QStyleSheetStyle
		menu->setProperty("mxUiMenu", true);
		// 挂 mUiStyle：QMenu 内部排版（行高 sizeFromContents/PM_Menu*）由 mUiStyle 按
		// mxUiMenu 标记下发，与自绘 sizeHint/paintAll 同源——不挂则排版走应用默认样式，
		// 行高与自绘不一致 → 行错位、选项消失
		if (mUiStyle* s = mUiStyle::instance())
			menu->setStyle(s);
		if (QMenu* qm = qobject_cast<QMenu*>(menu))
		{
			qm->setMouseTracking(true);
			qm->setAttribute(Qt::WA_Hover, true);
			// 纳管菜单同样不设 NoDropShadowWindowHint：Popup 窗口类在创建时自带
			// CS_DROPSHADOW 系统投影（与 mUiMenu 构造同口径）。此处不能 setWindowFlag：
			// 类样式按创建时 flags 注册，事后开关不会换类，反而在 aboutToShow 递归里
			// 触发窗口重建闪烁
			// 子菜单（addMenu 动态创建）在 aboutToShow 时递归补挂。
			// 只连一次：lambda 连接无法被 Qt::UniqueConnection 去重，
			// 本函数会被主题重发/菜单每次展开反复调用，重复连接会无限累积处理器
			if (!qm->property("mxUiMenuSynced").toBool())
			{
				qm->setProperty("mxUiMenuSynced", true);
				QObject::connect(qm, &QMenu::aboutToShow, qm, [qm]() {
					for (QMenu* sub : qm->findChildren<QMenu*>())
						applyMenuStyle(sub);
				});
			}
		}
	}

	void ThemeManager::applyMetrics(QWidget* root)
	{
		if (!root) return;

		// 页签栏 + 顶部行（文件/视图/快捷图标 + 页签栏）的留白与竖分隔线随档位重发：
		// 这些值在 initMenuBtn 里按创建时档位写死，切档不重发会保持旧档间距
		if (auto* tabBar = root->findChild<QTabBar*>(QStringLiteral("MenuTabBar")))
		{
			tabBar->setFixedHeight(UiMetrics::tabBarHeight());
			QFont f(UiMetrics::uiFontFamily());
			f.setPixelSize(UiMetrics::tabFontSize());
			setFontIfChanged(tabBar, f);
			if (auto* tabs = qobject_cast<MyTabBar*>(tabBar))
				tabs->refreshGeometry();
			tabBar->updateGeometry();
			for (QMenu* menu : root->findChildren<QMenu*>())
			{
				if (auto* m = qobject_cast<mUiMenu*>(menu))
					m->applyMetrics();
			}

			if (auto* hLay = qobject_cast<QHBoxLayout*>(tabBar->parentWidget()->layout()))
			{
				const int inset = UiMetrics::windowInset();
				hLay->setContentsMargins(inset, 0, inset, 0);
				for (int i = 0; i < hLay->count(); ++i)
				{
					QLayoutItem* item = hLay->itemAt(i);
					if (item->spacerItem())
					{
						item->spacerItem()->changeSize(inset, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
						continue;
					}
					if (QFrame* sep = qobject_cast<QFrame*>(item->widget());
						sep && sep->objectName() == QStringLiteral("menuBarSeparator"))
					{
						sep->setFixedHeight(UiMetrics::separatorLineHeight());
						setQss(sep, QString("color:%1;border:none;border-left:1px solid %1;")
							.arg(UiPalette::borderHex()));
					}
				}
				hLay->activate();
				if (QHBoxLayout* quick = hLay->findChild<QHBoxLayout*>(QStringLiteral("quickButtonLayout")))
				{
					quick->setSpacing(UiMetrics::quickGap());
					quick->activate();
				}
			}
		}

		// 文件按钮
		if (auto* fb = root->findChild<QToolButton*>(QStringLiteral("FileButton")))
		{
			layoutFileButton(fb);
			// 图标随档位缩放：与行内分隔线同一规格 icon = max(H/2+3,G+10)（标准 16）
			const int edge = UiMetrics::separatorLineHeight();
			fb->setIconSize(QSize(edge, edge));
		}

		// 命令菜单（文件/视图下拉）行高/行宽/字号全部按 UiMetrics 自算（mUiMenu），
		// 但 Qt 会缓存 actionGeometry/sizeHint；切档后需抛光重算并丢缓存，
		// 否则下一次弹出的菜单仍按旧档行高排版（表现为"菜单不随档位变化"）
		{
			const auto menus = root->findChildren<QMenu*>();
			for (QMenu* m : menus)
			{
				if (!m->property("mxUiMenu").toBool())
					continue;
				if (QStyle* st = m->style())
				{
					st->unpolish(m);
					st->polish(m);
				}
				m->updateGeometry();
				m->update();
			}
		}

		// 视图按钮
		if (auto* vb = root->findChild<QToolButton*>(QStringLiteral("ViewButton")))
		{
			vb->setFixedSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());
			vb->setIconSize(UiMetrics::quickIconSize());
		}

		// 快捷按钮
		const char* quickNames[] = { "QuickNewBtn", "QuickSaveBtn", "QuickOpenBtn" };
		for (const char* n : quickNames)
		{
			if (auto* b = root->findChild<QToolButton*>(QString::fromLatin1(n)))
			{
				b->setFixedSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());
				b->setIconSize(UiMetrics::quickIconSize());
			}
		}

		// Ribbon 页滚动区（视口）高度收口：总高 - 页签栏高。
		// 顶部菜单行（页签栏）在 RibbonMenuStackeWidget 之外的兄弟分支，用 name 过滤只处理页，
		// 避免把"页签栏+功能区总高"再下发给页本身导致功能区整体超高
		const auto rootPages = root->findChildren<QScrollArea*>();
		for (QScrollArea* page : rootPages)
		{
			page->setMinimumHeight(ribbonPageHeight());
			page->updateGeometry();
			if (auto* ribbonPage = qobject_cast<MechMainWindow::RibbonPage*>(page))
				ribbonPage->applyMetrics();
			else if (page->widget())
				page->widget()->updateGeometry();
			page->update();
		}
		if (auto* stacked = root->findChild<QStackedWidget*>(QStringLiteral("RibbonMenuStackeWidget")))
		{
			stacked->setMinimumHeight(ribbonPageHeight());
			stacked->updateGeometry();
			if (stacked->layout())
				stacked->layout()->activate();
		}

		// 遍历整棵树：重设按钮尺寸/字号，组合按钮、组内容高
		const auto children = root->findChildren<QWidget*>();
		for (QWidget* w : children)
		{
			const QString name = w->objectName();

			if (name == QStringLiteral("mGroupWidget"))
			{
				// 尺寸/间距/高度经 mGroupWidget::applyMetrics 统一下发（内部含内容区与
				// 标题高度，不再重复设一遍）；布局激活推迟到本函数末尾统一一趟，
				// 避免每组 activate 一次 ×（首趟+兜底趟）的多轮全量重排
				if (auto* group = qobject_cast<MechMainWindow::mGroupWidget*>(w))
					group->applyMetrics(false);
				// 组底部间距随档位缩放
				if (auto* vlay = qobject_cast<QVBoxLayout*>(w->layout()))
				{
					if (vlay->count() >= 3)
					{
						if (auto* spacer = vlay->itemAt(2))
						{
							if (spacer->spacerItem())
								spacer->spacerItem()->changeSize(0, qMax(4, UiMetrics::baseG()), QSizePolicy::Fixed, QSizePolicy::Fixed);
						}
					}
				}
				continue;
			}

			// 普通 Ribbon 按钮（含组合按钮 comboButton——Split 模式与普通全高按钮
			// 同一尺寸/主题口径；快捷按钮/文件按钮/翻页按钮除外）
			if (auto* btn = qobject_cast<QToolButton*>(w))
			{
				if (isManagedSpecialButton(name))
					continue;

				// QTabBar 内建滚动按钮（页签溢出左右箭头）：功能区按钮的高度/宽度
				// 公式对它无意义（会被定宽成整钮宽），且 applyRibbonButtonTheme
				// 会显式重挂 mUiStyle——CC_ToolButton 被拦截后箭头消失。
				// 尺寸交回 QTabBar 自身布局，样式由 applyPalette 的页签栏 QSS 下发
				if (qobject_cast<QTabBar*>(w->parentWidget()))
					continue;

				// 大/小判定：优先 uiBigButton 标记（setQToolButton 打标），
				// 否则按创建时记录的意图图标尺寸 uiIconSize 判断（addToolBtn 打标，
				// 意图尺寸 <=24 为小按钮，>24 为大按钮），兜底用布局方式。
				// 统一写回 uiBigButton，保证幂等且与 updateLayout 列对齐判定一致
				const bool isBig = isBigButton(btn);
				const bool isRibbonHalf = !isBig && btn->toolButtonStyle() == Qt::ToolButtonTextBesideIcon;
				if (!btn->property("uiBigButton").isValid())
					btn->setProperty("uiBigButton", isBig);

				// 高度按档位的大/小按钮高；图标尺寸按规则 1.3：
				// 全高大图标保留 H+4；二分图标盒保留 max(H/2+7,G+10)，不按比例换算
				const int btnH = isBig ? UiMetrics::bigButtonHeight()
					: isRibbonHalf ? UiMetrics::halfButtonHeight() : UiMetrics::smallButtonHeight();
				btn->setFixedHeight(btnH);
				int iconEdge;
				if (isBig)
				{
					iconEdge = UiMetrics::menuLargeIconSize(); // 大图标 menuLargeIcon = H+4（标准 30）
				}
				else if (btn->toolButtonStyle() == Qt::ToolButtonTextBesideIcon)
				{
					iconEdge = UiMetrics::menuIconSize(); // 二分小图标 icon = 16（标准档）
				}
				else
				{
					// 无文字纯图标小钮：图标贴按钮内缘
					iconEdge = btnH - 2 * UiMetrics::menuInset();
				}
				const QSize iconSz(iconEdge, iconEdge);
				if (btn->iconSize() != iconSz)
					btn->setIconSize(iconSz);
				QFont f(UiMetrics::uiFontFamily());
				f.setPixelSize(UiMetrics::buttonFontSize());
				// 字体没变（相邻档位功能区字号可能相同）时跳过 setFont 与抛光：
				// 二者都会触发样式失效与几何重算，数百个按钮累计是切档可感知的停顿
				if (btn->font() != f)
				{
					btn->setFont(f);
					btn->style()->unpolish(btn);
					btn->style()->polish(btn);
					btn->updateGeometry();
				}
				// 宽度走规则 1.3 公式（见 ribbonButtonMinWidth），保证文字单行完整显示不折行。
				// 这也是唯一一趟公式宽兜底：原先主遍历后再整树扫一遍重复 setFixedWidth，
				// 结果完全相同（同公式同输入），纯耗时
				btn->setFixedWidth(ribbonButtonMinWidth(btn));

				// 主题重应用时同步刷新按钮自身的样式表颜色。
				// 不能只在创建时写色：① 切主题后创建时的旧色（如浅色 #424b57）会残留在
				// 按钮样式表里，深色底上近乎黑字=文字"丢失"；② 无色样式表会触发全局 QSS
				// QToolButton 规则（width/height:21px 白底方块），把文字挤出可视区。
				// 这里按按钮形态重写完整样式：文字/悬停/选中/禁用颜色全部取自当前色板，
				// 并显式声明 border/background，压过全局 QSS 的 21px 白底规则。
				applyRibbonButtonTheme(btn);
			}
		}

		// 公式宽度兜底后再做组内同列小按钮列宽对齐（updateLayout 是按钮宽度的
		// 最终下发者）。这里只改宽度不逐组 activate：布局激活统一收口到
		// 本函数末尾（root + 各主窗菜单栏各一趟），整树只做一次同步重排，
		// 否则每组 activate × 两轮的全量重排是切档卡顿的主要来源之一
		const auto groups = root->findChildren<MechMainWindow::mGroupWidget*>();
		for (MechMainWindow::mGroupWidget* g : groups)
			g->updateLayout();

		// 重算菜单栏最小高并触发布局
		root->setMinimumHeight(UiMetrics::menuBarTotalHeight());
		if (root->layout())
			root->layout()->activate();
		root->updateGeometry();

		// applyLevel/applyTheme 的 root 是首选项对话框（切档/切主题的入口在首选项里），
		// 此时必须跨到主窗经 menuWidget() 找真正的菜单栏补发高度：
		// 仅 setMinimumHeight 不够——setMenuWidget 之后 QMainWindow 布局已缓存菜单栏
		// 旧的 sizeHint，必须 invalidate 主窗布局丢弃缓存立即重排（重启后正常正是因为
		// 装配时重新读了高度）。root 是菜单栏自身时本轮已在上文完成，这里会再执行一遍，
		// 全部为幂等操作。
		for (QWidget* top : QApplication::topLevelWidgets())
		{
			auto* mw = qobject_cast<QMainWindow*>(top);
			if (!mw || !mw->menuWidget())
				continue;
			QWidget* menu = mw->menuWidget();
			menu->setMinimumHeight(UiMetrics::menuBarTotalHeight());
			if (menu->layout())
				menu->layout()->activate();
			menu->updateGeometry();
			if (mw->layout())
			{
				mw->layout()->invalidate();
				mw->layout()->activate();
			}
			menu->adjustSize();
		}
	}

	int ThemeManager::ribbonButtonMinWidth(QToolButton* btn, bool hasMenu)
	{
		if (!btn) return 0;
		// 规则 1.3 宽度公式（先量文字，再算内容，最后与最小宽取较大值）：
		//   独立全高 = max(共用最小宽, max(H, T＋箭头占宽) ＋ 2(G−2))     —— 图标 H 与文字取较宽项
		//   上下二分 = max(共用最小宽, icon ＋ menuContentGap ＋ T ＋ 箭头占宽 ＋ 2(G−2))
		// 箭头占宽 = entryArrow(body) ＋ menuContentGap，只有确实带菜单的按钮才加（规则 1.3 计一次）。
		const bool isBig = isBigButton(btn);
		// 字号必须按功能区字号量文字（不能信 btn->font()：调用方常未设字体，
		// 用默认 9px 量"管理布局"得 ~40px，而实际渲染 menuFont=14px 要 ~68px，宽度必然不够）
		QFont f(UiMetrics::uiFontFamily());
		f.setPixelSize(UiMetrics::buttonFontSize());
		const int T = QFontMetrics(f).horizontalAdvance(btn->text());
			const bool withMenu = !btn->property("mxNoArrow").toBool()
				&& (hasMenu || btn->menu() != nullptr
					|| btn->objectName() == QStringLiteral("wideButton")
					|| btn->property("mxHasMenu").toBool());
		const int arrowW = withMenu ? UiMetrics::arrowReserveWidth() : 0;
		int need;
		if (isBig)
		{
			// 全高：大图标(menuLargeIconSize)与文字取较宽项，纵向图文间距不计入宽度；
			// 双动作全高入口（mComboButton，Split 模式）：文字取按钮文字与文字区
			// 文字 dropText 的最宽项，文字区恒带箭头（规则 1.3 双动作仍按一个全高按钮计算）
			int textMax = T;
			if (const QVariant dt = btn->property("dropText"); dt.isValid())
			{
				const int dropT = QFontMetrics(f).horizontalAdvance(dt.toString());
				textMax = qMax(textMax, dropT);
			}
			need = qMax(UiMetrics::menuLargeIconSize(), textMax + arrowW);
		}
		else
		{
			// 二分：小图标在左 + 图文间距 + 文字 + 箭头；无图标时图标占宽为 0
				const int iconW = (btn->toolButtonStyle() == Qt::ToolButtonTextBesideIcon && !btn->icon().isNull())
				? UiMetrics::menuIconSize() + UiMetrics::menuContentGap() : 0;
			need = iconW + T + arrowW;
		}
		return qMax(UiMetrics::menuFullMinWidth(), need + 2 * UiMetrics::menuInset());
	}

	namespace
	{
		// 停靠系统主题重发入口（GUIFoundation 侧 mDockLayoutManager 启动时注册）
		ThemeManager::DockThemeApplier s_dockThemeApplier = nullptr;
		// 追加型 applier：dock 主题重发后补发调用方自定义样式（如主窗窗口边线）
		std::vector<std::function<void()>> s_dockThemeExtraAppliers;
	}

	void ThemeManager::applyGlobalQss()
	{
		QFile f(QStringLiteral(":/mMainWidgets/mApplicationStart.qss"));
		if (!f.exists() || !f.open(QIODevice::ReadOnly | QIODevice::Text))
			return;
		QString qss = QString::fromUtf8(f.readAll());
		f.close();

		/*
		* 占位符表：@{token.name} -> 当前取值。
		* 颜色取 UiPalette（随三主题），字号/尺寸取 UiMetrics（随五档）。
		* 表键即文件内占位符全集；文件里出现表外占位符时原样保留（便于渐进迁移）
		*/
		const QMap<QString, QString> tokens = {
			// 颜色（UiPalette）
			{ "@{color.text}", UiPalette::textHex() },
			{ "@{color.label}", UiPalette::labelHex() },
			{ "@{color.muted}", UiPalette::mutedHex() },
			{ "@{color.disabled}", UiPalette::disabledHex() },
			{ "@{color.border}", UiPalette::borderHex() },
			{ "@{color.surface}", UiPalette::surfaceHex() },
			{ "@{color.panel}", UiPalette::panelHex() },
			{ "@{color.header}", UiPalette::headerHex() },
			{ "@{color.workspace}", UiPalette::workspaceHex() },
			{ "@{color.primary}", UiPalette::primaryHex() },
			{ "@{color.selected}", UiPalette::selectedHex() },
			{ "@{color.selectedText}", UiPalette::selectedTextHex() },
			{ "@{color.hover}", UiPalette::hoverHex() },
			{ "@{color.focus}", UiPalette::focusHex() },
			{ "@{color.error}", UiPalette::errorHex() },
			// 字号（UiMetrics）
			{ "@{font.body}", QString::number(UiMetrics::bodyFontSize()) + "px" },
			{ "@{font.helper}", QString::number(UiMetrics::helperFontSize()) + "px" },
			{ "@{font.button}", QString::number(UiMetrics::bodyFontSize()) + "px" },
			{ "'@{font.family}'", QStringLiteral("'%1'").arg(UiMetrics::uiFontFamily()) },
			// 尺寸（UiMetrics）
			// control = 控件外高 H（无边框控件直接用）；controlIn = 带 1px 边框
			// 控件的 QSS 内容区高（H−2）：QSS 的 width/height 指内容区尺寸，
			// 加上下各 1px 边框后控件外高恰为 H（规则 0.1"控件外高"语义）
			{ "@{size.control}", QString::number(UiMetrics::baseH()) + "px" },
			{ "@{size.controlIn}", QString::number(UiMetrics::baseH() - 2) + "px" },
			{ "@{size.small}", QString::number(UiMetrics::smallButtonHeight()) + "px" },
			{ "@{size.header}", QString::number(UiMetrics::windowHeaderHeight()) + "px" },
			{ "@{size.radius}", QString::number(UiMetrics::zoneRadius()) + "px" },
			{ "@{size.choice}", QString::number(UiMetrics::choiceSize()) + "px" },
			// 原生输入框内建左衬约 2px，补 G−1 后与下拉框 inset(G+1) 对齐
			{ "@{size.inset}", QString::number(UiMetrics::menuItemInset() - 2) + "px" },
		};
		for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
			qss.replace(it.key(), it.value());

		if (qApp->styleSheet() != qss)
			qApp->setStyleSheet(qss);
	}

	void ThemeManager::applyDockTheme()
	{
		if (s_dockThemeApplier)
			s_dockThemeApplier();
		for (const auto& fn : s_dockThemeExtraAppliers)
			fn();
	}

	void ThemeManager::registerDockThemeApplier(DockThemeApplier fn)
	{
		s_dockThemeApplier = fn;
	}

	void ThemeManager::addDockThemeExtraApplier(std::function<void()> fn)
	{
		s_dockThemeExtraAppliers.push_back(std::move(fn));
	}

	void ThemeManager::applyTreeTheme(QWidget* tree)
	{
		auto* tv = qobject_cast<QTreeView*>(tree);
		if (!tv) return;

		// 仅窗口层面随主题（规则 0.2/2.1：树区底色 surface、文字 text）；
		// 树结构本身（行高/缩进/字号/箭头/选中样式）按最新规则不随档位，保持 Qt 原生/委托现状。
		// 只走 QSS：style sheet 规则优先于 QPalette，同时 setPalette 属于被覆盖的死设置
		setQss(tv, QStringLiteral(
			"QTreeView { font-family:'%3'; font-size:%4px; background-color:%1; color:%2; border:none; }"
		).arg(UiPalette::surfaceHex(), UiPalette::textHex(), UiMetrics::uiFontFamily())
		 .arg(UiMetrics::bodyFontSize()));
		tv->update();
	}

	void ThemeManager::applyTreeFilterBarTheme(const TreeFilterBar& bar)
	{
		// 树窗口过滤/排序工具条（前处理 mBrowserWindow 与后处理 mPostBrowserWindow 共用）：
		// 行高 = control = H、字号 = body、边线/悬停/选中取语义色板（规则 0.1/0.2/2.1）。
		// 两处窗口各持有同构控件组，统一下发避免两份 QSS 手写漂移。幂等。
		if (!bar.paletteHost)
			return;
		const QString fontFam = UiMetrics::uiFontFamily();
		const QString comboBoxQss = QStringLiteral(
			"QComboBox { border:1px solid %1; border-radius:4px; height:%2px;"
			" font-family:'%8'; font-size:%3px; color:%4; background:%5; padding-left:6px; }"
			"QComboBox QAbstractItemView { font-family:'%8'; font-size:%3px; background:%5; color:%4; border:1px solid %1;"
			" selection-background-color:%6; selection-color:%7; outline:none; }"
		).arg(UiPalette::borderHex())
		 .arg(UiMetrics::controlHeight())
		 .arg(UiMetrics::bodyFontSize())
		 .arg(UiPalette::textHex())
		 .arg(UiPalette::surfaceHex())
		 .arg(UiPalette::selectedHex())
		 .arg(UiPalette::selectedTextHex())
		 .arg(fontFam);
		const QString lineEditQss = QStringLiteral(
			"QLineEdit { border:1px solid %1; border-radius:4px; height:%2px;"
			" font-family:'%6'; font-size:%3px; color:%4; background:%5; padding-left:6px; }"
		).arg(UiPalette::borderHex())
		 .arg(UiMetrics::controlHeight())
		 .arg(UiMetrics::bodyFontSize())
		 .arg(UiPalette::textHex())
		 .arg(UiPalette::surfaceHex())
		 .arg(fontFam);
		const QString labelQss = QStringLiteral("font-family:'%3'; color:%1; font-size:%2px;")
			.arg(UiPalette::labelHex()).arg(UiMetrics::bodyFontSize()).arg(fontFam);

		if (bar.nameCombo) setQss(bar.nameCombo, comboBoxQss);
		if (bar.statusCombo) setQss(bar.statusCombo, comboBoxQss);
		if (bar.sortTypeCombo) setQss(bar.sortTypeCombo, comboBoxQss);
		if (bar.findEdit) setQss(bar.findEdit, lineEditQss);
		if (bar.filterLabel) setQss(bar.filterLabel, labelQss);
		if (bar.sortLabel) setQss(bar.sortLabel, labelQss);
		if (bar.sortTool)
		{
			bar.sortTool->setIconSize(QSize(UiMetrics::controlIconSize(), UiMetrics::controlIconSize()));
			bar.sortTool->setFixedHeight(UiMetrics::controlHeight());
			setQss(bar.sortTool, QStringLiteral(
				"QToolBar { background:%1; border:none; }"
				"QToolButton { background:transparent; border:none; }"
				"QToolButton:hover { background:%2; border-radius:4px; }"
			).arg(UiPalette::surfaceHex(), UiPalette::hoverHex()));
		}
		// 窗口本体底色随主题（构造期 palette 只在创建时取色）
		QPalette pal = bar.paletteHost->palette();
		pal.setColor(QPalette::Window, UiPalette::surface());
		bar.paletteHost->setPalette(pal);
	}

	void ThemeManager::applyRibbonButtonTheme(QToolButton* btn)
	{
		if (!btn) return;
		// 绘制已由 mUiStyle/mRibbonButton 自绘接管（背景/图标/文字/箭头全部
		// 按规则 1.3 与文字基线对齐）。这里绝不能再挂 QSS：一旦挂 QSS，
		// Qt 用 QStyleSheetStyle 包裹样式，CE_ToolButtonLabel 自绘被绕过，
		// 底座样式的 menu-indicator 槽回来把箭头钉到控件右上/右下角，
		// 这正是"箭头与文字顶部对齐"修多次无效的根因。
		// 全局 QSS（mApplicationStart.qss 的 21px 白底规则）不影响本按钮：
		// 尺寸由 setFixedWidth/setFixedHeight 收口，背景/前景全部自绘。
		setQss(btn, QString());
		if (mUiStyle* s = mUiStyle::instance())
		{
			// 只挂一次：setStyle 触发样式变更与整控件重抛光，且按钮无 QSS 后
			// 有效样式本就回落到已设置的 mUiStyle，重复重挂是纯开销。
			// 属性标记与 mUiStyle::install 共用同一约定
			if (!btn->property("mxUiStyled").toBool())
			{
				btn->setProperty("mxUiStyled", true);
				btn->setStyle(s);
			}
		}
		btn->update();
	}

	void ThemeManager::applyGroupWidgetPalette(QWidget* w)
	{
		if (!w) return;
		// 组底色用 palette 下发（QSS 的 background 会触发 QStyleSheetStyle 覆盖
		// paintEvent 自绘的分隔线，导致线被截短/截断）
		QPalette pal = w->palette();
		pal.setColor(QPalette::Window, UiPalette::surface());
		w->setPalette(pal);
		w->setAutoFillBackground(true);
		// 只保留字体/颜色，背景色不再走 QSS
		setQss(w, QStringLiteral(
			"QWidget#mGroupWidget { font-size:%1px; font-family:'%3'; color:%2; }"
		).arg(UiMetrics::buttonFontSize()).arg(UiPalette::textHex())
		 .arg(UiMetrics::uiFontFamily()));
			// 组内所有子控件（输入框/下拉框/标签等）跟随主题
			const auto subWidgets = w->findChildren<QWidget*>();
			for (QWidget* sub : subWidgets)
			{
				// 组内 QPushButton（addPushButton 创建）统一刷新主题色。
				//（addDropDownButton 已重构为全自绘 mRibbonButton，不再产出 QPushButton，
				// 原"QPushButton::menu-indicator 归零"补丁随之一并删除）
				if (auto* pbtn = qobject_cast<QPushButton*>(sub))
				{
					setQss(pbtn, QStringLiteral(
						"QPushButton { font-family:'%8'; font-size:%1px; color:%2;"
						" background:%3; border:1px solid %4; border-radius:3px; padding:1px 6px; }"
						"QPushButton:hover { background:%5; }"
						"QPushButton:pressed, QPushButton:checked { background:%6; }"
						"QPushButton:disabled { color:%7; }"
					).arg(UiMetrics::buttonFontSize())
					 .arg(UiPalette::textHex())
					 .arg(UiPalette::surfaceHex())
					 .arg(UiPalette::borderHex())
						 .arg(UiPalette::hoverHex())
						 .arg(UiPalette::selectedHex())
						 .arg(UiPalette::disabledHex())
						 .arg(UiMetrics::uiFontFamily()));
					continue;
				}
				// 组内普通 Ribbon 按钮（QToolButton）刷新主题样式表：
				// 切主题后按钮底色已随组变深，但文字色仍停留在创建时写入的旧主题色，
				// 深色底上深灰文字=文字"丢失"；无色样式表还会触发全局 QSS 的
				// QToolButton 21px 白底规则把文字挤出。这里统一重发全量状态色
				if (auto* tbtn = qobject_cast<QToolButton*>(sub))
				{
					// 跳过已单独处理的特殊按钮（comboMainBtn/comboDropBtn/翻页钮
					// 由 applyPalette/applyMetrics 的专属分支处理）
					const QString n = tbtn->objectName();
					if (n == QStringLiteral("comboMainBtn") || n == QStringLiteral("comboDropBtn") ||
						n == QStringLiteral("m_leftScrollBtn") || n == QStringLiteral("m_rightScrollBtn"))
						continue;
					// 纯图标按钮（addIconToolButton）：无文字，仅刷新 hover 背景
					if (tbtn->toolButtonStyle() == Qt::ToolButtonIconOnly)
					{
						setQss(tbtn, QStringLiteral(
							"QToolButton { background:transparent; border:none; border-radius:3px; }"
							"QToolButton:hover { background:%1; }"
							"QToolButton:pressed, QToolButton:checked { background:%2; }"
						).arg(UiPalette::hoverHex(), UiPalette::selectedHex()));
						continue;
					}
					// 其余（含 toolbtnFilter 等"开关"按钮）统一走自绘主题：
					// 选中底色由 mUiStyle::drawRibbonButton 的 checked→selected 自绘分支覆盖，
					// 不追加 QSS（追加式样式表会让按钮进入 QStyleSheetStyle，自绘被绕过、
					// 底座 menu-indicator 槽回归，箭头被钉到角落——历史病灶）
					applyRibbonButtonTheme(tbtn);
					continue;
				}
			if (auto* lbl = qobject_cast<QLabel*>(sub))
			{
				const bool groupTitle = lbl->objectName() == QStringLiteral("mGroupTitleLabel");
				setQss(lbl, QStringLiteral(
					"QLabel { font-family:'%3'; font-size:%1px; color:%2; background:transparent; border:none; }"
				).arg(groupTitle ? UiMetrics::groupTitleFontSize() : UiMetrics::buttonFontSize())
				 .arg(groupTitle ? UiPalette::mutedHex() : UiPalette::textHex())
				 .arg(UiMetrics::uiFontFamily()));
				continue;
			}
			if (auto* edit = qobject_cast<QLineEdit*>(sub))
			{
				// SpinBox 内置编辑区的外框/焦点色由 SpinBox 自己负责，不能套一层
				// 独立 QLineEdit 边框，否则父框与内框叠加形成双重内边距。
				if (qobject_cast<QAbstractSpinBox*>(edit->parentWidget()))
					continue;
				applyRibbonLineEditStyle(edit);
				// 宽度锁随档位重钉：新样式表会改变 sizeHint，先解锁再按新值重钉，
				// 保证焦点/失焦只改变边框颜色、不改变控件宽度。
				edit->setMinimumWidth(0);
				edit->setMaximumWidth(QWIDGETSIZE_MAX);
				edit->setFixedWidth(edit->sizeHint().width());
				continue;
			}
			if (auto* combo = qobject_cast<QComboBox*>(sub))
			{
				combo->setFixedHeight(UiMetrics::smallButtonHeight());
				// 下拉按钮宽度过宽（全局 QSS 30px）会吃掉窄下拉的可见文字，这里收到 18px；
				// 箭头/背景/列表项选中全部走色板
				setQss(combo, QStringLiteral(
					"QComboBox { font-family:'%10'; font-size:%1px; color:%2; background:%3; border:1px solid %4; border-radius:2px; padding:1px 4px; }"
					"QComboBox:hover { border-color:%5; }"
					"QComboBox:focus { border-color:%5; }"
					"QComboBox:disabled { color:%6; background:%7; }"
					"QComboBox::drop-down { subcontrol-origin:padding; subcontrol-position:top right; width:18px; border:none; }"
					"QComboBox QAbstractItemView { background:%3; color:%2; border:1px solid %4;"
					" selection-background-color:%8; selection-color:%9; }"
				).arg(UiMetrics::buttonFontSize())
				 .arg(UiPalette::textHex(), UiPalette::surfaceHex(), UiPalette::borderHex())
				 .arg(UiPalette::focusHex())
				 .arg(UiPalette::disabledHex(), UiPalette::panelHex())
				 .arg(UiPalette::selectedHex(), UiPalette::selectedTextHex())
				 .arg(UiMetrics::uiFontFamily()));
				continue;
			}
			if (auto* spin = qobject_cast<QDoubleSpinBox*>(sub))
			{
				applyRibbonSpinBoxStyle(spin);
				continue;
			}
			if (auto* chk = qobject_cast<QCheckBox*>(sub))
			{
				setQss(chk, QStringLiteral(
					"QCheckBox { color:%1; background:transparent; }"
					"QCheckBox:disabled { color:%2; }"
				).arg(UiPalette::textHex(), UiPalette::disabledHex()));
				continue;
			}
			if (auto* radio = qobject_cast<QRadioButton*>(sub))
			{
				setQss(radio, QStringLiteral(
					"QRadioButton { color:%1; background:transparent; }"
					"QRadioButton:disabled { color:%2; }"
				).arg(UiPalette::textHex(), UiPalette::disabledHex()));
				continue;
			}
			if (auto* slider = qobject_cast<QSlider*>(sub))
			{
				setQss(slider, QStringLiteral(
					"QSlider::groove:horizontal { height:4px; background:%1; border-radius:2px; }"
					"QSlider::handle:horizontal { width:12px; margin:-5px 0; border-radius:6px; background:%2; }"
					"QSlider::sub-page:horizontal { background:%3; border-radius:2px; }"
				).arg(UiPalette::borderHex(), UiPalette::primaryHex(), UiPalette::selectedHex()));
				continue;
			}
		}
	}
}
