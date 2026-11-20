#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 菜单栏主题/尺寸统一下发入口（ThemeManager）
* 切档或切主题都只调用 applyAll()：把 UiMetrics/UiPalette 的最新值
* 重新下发到顶部菜单栏控件链（mMenuWidget → RibbonPage → mGroupWidget → 内部按钮），
* 并重读/写入 QSettings。要求幂等、可重复调用。
*
* 该类只依赖 Qt 与 UiMetrics/UiPalette，不依赖具体业务控件；
* 通过 QObject 父子树 + objectName 定位菜单栏内受控控件。
*/

#include "mCommon_global.h"

#include <QObject>
#include <QWidget>

#include <functional>

class QMenu;
class QToolButton;
class QComboBox;
class QLineEdit;
class QLabel;
class QToolBar;
class QContextMenuEvent;

namespace MechMainWindow
{
	class mGroupWidget;
}

namespace MechMainWindow
{
	class COMMON_EXPORT ThemeManager : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 自样式弹窗豁免标记（bool 属性）：弹窗构造时自带完整设计稿 QSS 的自绘弹窗
		* 置 true，ThemeManager 对其跳过整窗 applyDialogTheme（QSS 整表替换会冲掉
		* 弹窗自设样式）；未标记的通用表单对话框仍随主题。
		* 跨模块引用统一用本常量，不散落字符串字面量
		*/
		static constexpr char kSelfStyledProp[] = "mxSelfStyled";

		static ThemeManager* instance();

		/*
		* 统一应用入口：把当前 UiMetrics/UiPalette 下发到以 root 为根的菜单栏控件树。
		* 幂等，可重复调用。root 通常为 mMenuWidget（QMenuBar 宿主）。
		*/
		void applyAll(QWidget* root);

		/*
		* 切换尺寸档位(1-5)并 applyAll
		*/
		void applyLevel(int level, QWidget* root);

		/*
		* 切换主题并 applyAll
		*/
		void applyTheme(int themeIndex, QWidget* root);

	/*
	* 档位+主题一次切换并只跑一轮 applyAll（首选项"应用"用；
	* 分别调 applyLevel/applyTheme 会全量重发两遍，切档卡顿且中途闪旧主题）。
	* 运行期切档/切主题按"实际变化的维度"掩码下发：只切主题不做尺寸/布局重排，
	* 只切档不重发未变化的 QSS，档位与主题都没变时直接跳过
	*/
	void applyAppearance(int level, int themeIndex, QWidget* root);

		/*
		* 从 QSettings 读取持久化的档位+主题（启动时调用，applyAll 前）
		*/
		void loadSettings();

		/*
		* 把当前档位+主题写入 QSettings
		*/
		void saveSettings();

		/*
		* 组分隔线与底色：mGroupWidget 自身及组内非按钮子控件。
		* 供 applyPalette 与 mGroupWidget::updateLayout（按钮点击后列宽重排）复用，
		* 保证组样式不被退回创建时的旧主题色。
		*/
		void applyGroupWidgetPalette(QWidget* groupWidget);

		/*
		* 组内普通 Ribbon 按钮的主题样式（文字/hover/pressed/disabled 全量取自色板）。
		* 供 applyMetrics 统一调用；后注册按钮（后处理页等）在宿主侧也可直接调用，
		* 避免按钮创建时的样式表在全局 QSS 加载后被 21px 白底规则压掉。
		*/
		void applyRibbonButtonTheme(QToolButton* btn);

		/*
		* 通用表单对话框的主题样式（首选项等）：
		* 对话框/树/堆叠页/输入/下拉/表格/按钮/标签全部按 UiPalette 语义色重发，
		* 使切主题后各控件背景、文字、悬浮与选中状态与 UI 规则 0.2 色板一致。
		* 幂等，可重复调用。
		*/
		void applyDialogTheme(QWidget* dialog);

		/*
		* 顶层窗口标题栏（Windows 非客户区）按主题改色。
		* QSS/QPalette 管不到原生标题栏，这里通过 DwmSetWindowAttribute
		* 把 caption 底色设为 UiPalette::header、文字设为 UiPalette::text；
		* dwmapi 运行时加载，不引入新链接依赖，Win10 1809 以下自动跳过。
		*/
		static void applyTitleBarTheme(QWidget* window);

		/*
		* Ribbon 按钮最小宽度计算（文字单行完整显示为下限），对齐规则 1.3 宽度公式：
		* 独立全高 = max(共用最小宽, max(H, T+箭头占宽) + 2(G−2))；
		* 上下二分 = max(共用最小宽, icon + menuContentGap + T + 箭头占宽 + 2(G−2))。
		* 仅当按钮"确实带下拉菜单"（menu() 非空或调用方通过 hasMenu 提前声明）
		* 才预留箭头占宽（entryArrow + menuContentGap），避免无菜单按钮右侧凭空留白。
		*/
		static int ribbonButtonMinWidth(QToolButton* btn, bool hasMenu = false);

		/*
		* Ribbon 下拉菜单（QMenu）的一站式装配（规则 1.0）：清空 QSS 退出 QStyleSheetStyle
		* + 挂 mUiStyle 自绘（行内容/勾选线/子菜单箭头/容器圆角），创建后调用一次即可；
		* 子菜单在 aboutToShow 时自动递归补挂
		*/
		static void applyMenuStyle(QWidget* menu);

		/*
		* 通用右键菜单事件拦截（编辑框/参数面板等标准编辑菜单的最终防线）：
		* 本类构造时已向 qApp 安装事件过滤，此处处理 QEvent::ContextMenu——
		* 目标为 QLineEdit/QTextEdit 等可编辑控件时，改由 mUiMenu 自绘菜单
		* 弹出对应的标准编辑动作（撤销/恢复/剪切/复制/粘贴/删除/全选），
		* 彻底避开 Qt createStandardContextMenu 生成的裸 QMenu
		*（原生高亮底色上画白色文字，悬浮时文字不可见的根因之一）。
		* 返回 true 表示已接管（事件被消费，不再走控件默认 contextMenuEvent）。
		*/
		bool interceptEditContextMenu(QObject* obj, QEvent* ev);

		/*
		* 树视图的主题下发（规则 0.2/2.1）：仅树区底色 surface、文字 text 随主题。
		* 树结构本身（行高/缩进/字号/箭头/选中样式）按最新规则不随档位，保持现状。
		* 切主题后由 applyAll 遍历顶层窗口统一调用（幂等）。
		*/
		void applyTreeTheme(QWidget* tree);

		/*
		* 树窗口过滤/排序工具条控件组（前处理 mBrowserWindow 与后处理 mPostBrowserWindow
		* 同构持有），统一下发主题/档位样式，避免两处各写一份 QSS 漂移。幂等。
		* paletteHost 为底色随主题刷新的宿主窗口（通常传 this）。
		*/
		struct TreeFilterBar
		{
			QComboBox* nameCombo = nullptr;
			QComboBox* statusCombo = nullptr;
			QComboBox* sortTypeCombo = nullptr;
			QLineEdit* findEdit = nullptr;
			QLabel* filterLabel = nullptr;
			QLabel* sortLabel = nullptr;
			QToolBar* sortTool = nullptr;
			QWidget* paletteHost = nullptr;
		};
		static void applyTreeFilterBarTheme(const TreeFilterBar& bar);

		/*
		* 停靠系统（ADS dock 页签/标题栏/关闭按钮）按当前主题/档位重发。
		* 实现转发到 GUIFoundation 的 mDockLayoutManager::applyTheme()（经全局函数指针注册解耦，
		* Common 不反向链接 GUIFoundation）；未注册时为空操作。幂等。
		*/
		/*
		* 全局基础控件样式（原 mApplicationStart.qss）的令牌化下发：
		* 读取 qss 资源文件，把文件内的命名占位符（@{...}）替换为当前
		* UiPalette/UiMetrics 取值后经 qApp->setStyleSheet 全局应用。
		* 切主题/切档时随 applyAllInternal 重发（全局串与令牌系统同源，
		* 不再是启动后一次性压顶的静态样式）；文件中未使用占位符的字面值
		* 原样保留。幂等（内容相同不重设）。
		*/
		void applyGlobalQss();

		void applyDockTheme();

		/* GUIFoundation 侧 mDockLayoutManager 向本类注册的重发入口（启动时注册一次） */
		typedef void (*DockThemeApplier)();
		static void registerDockThemeApplier(DockThemeApplier fn);

		/* 追加型 applier：applyDockTheme 重发 dock 样式后补发调用方自定义样式。
		*  与 registerDockThemeApplier 不互斥（多个并存，按注册序调用），
		*  用于主窗等在 dock 主题重发后补发窗口边线等场景 */
		static void addDockThemeExtraApplier(std::function<void()> fn);

private:
	explicit ThemeManager(QObject* parent = nullptr);

	// 下发维度掩码：切档只做 Metrics，切主题只做 Palette，
	// 两者都变（或启动全量）才走 MaskAll
	enum ChangeMask
	{
		MaskMetrics = 1,
		MaskPalette = 2,
		MaskAll = MaskMetrics | MaskPalette
	};

	// 掩码版下发：菜单栏控件树同步处理，全局补发（对话框/树/dock 等）延迟一拍
	void applyAllInternal(QWidget* root, int mask);

	// 全局补发一轮：顶层对话框/标题栏/状态栏/工具栏/树宿主/dock。
	// 由 applyAllInternal 经 0ms 定时器延迟调用，让菜单栏先画出来
	void applyGlobalRound();

	// 隐藏对话框/隐藏顶层窗的懒补发：显示时按"主题代数"判断是否落后，
	// 落后则现场补发（对话框换肤 + DWM 标题栏改色），
	// 切档期间不再为不可见控件付费（此前所有隐藏 QDialog 都全量重发样式）
	bool eventFilter(QObject* obj, QEvent* ev) override;

	// 懒补发实际执行体（经排队连接在显示流程完成后调用）：
	// 在 Show 事件内部同步改样式会打断首次布局/绘制，弹窗出现
	// "样式没加载上"的残破首帧（许可验证警告框即此症状）；
	// 自绘标题栏对话框（mFramelessDialog 系）构造时已对内容区下发主题，
	// 这里只补发过期的内容区，不整窗重发
	void themeWindowLazy(QWidget* w);

	// 颜色下发：重 set QSS / 自绘控件 update()
	void applyPalette(QWidget* root);

	// 尺寸下发：重 set 尺寸/iconSize/字号，并触发布局更新
	void applyMetrics(QWidget* root);
	};
}
