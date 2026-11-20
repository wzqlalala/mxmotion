#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

/*
* 自绘标题栏对话框基类（mFramelessDialog）
* 用法与 QDialog 一致：把内容部件经 setContentWidget() 装入即可，
* 标题栏（图标/标题/最小化/最大化/关闭）、DWM 阴影、
* 边框缩放、最大化/还原动画全部由本基类与 mFramelessWindowHelper 提供。
*
* 示例：
*   auto* dlg = new mFramelessDialog(parent);
*   dlg->setWindowTitle(QStringLiteral("首选项"));
*   auto* content = new QWidget;
*   ... 组装 content 布局 ...
*   dlg->setContentWidget(content);
*   dlg->show();
*/

#include "mCommon_global.h"

#include <QDialog>

class QLayout;
class QVBoxLayout;

namespace MechMainWindow
{
	class mWindowTitleBar;

	class COMMON_EXPORT mFramelessDialog : public QDialog
	{
		Q_OBJECT
	public:
		explicit mFramelessDialog(QWidget* parent = nullptr);
		~mFramelessDialog() override;

	/*
	* 装入内容部件（标题栏下方区域），所有权移交给本对话框
	*/
	void setContentWidget(QWidget* content);

	/*
	* 装入已组装好的外部布局（裸 new、尚未挂到任何部件）：包进内容容器再交给
	* 本对话框，用于旧代码 QDialog->setLayout(...) 形态的迁移。applyTheme 为
	* true 时内容容器按当前主题上色（ThemeManager::applyDialogTheme，与首选项
	* 同链路）。返回内容容器（null 布局返回 nullptr）。
	*/
	QWidget* setExternalLayout(QLayout* lay, bool applyTheme = true);

	protected:
		bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
		void changeEvent(QEvent* event) override;
		/*
		* 窗口外框 1px 边线（UiPalette::border，实时取色）：softwareTitleBar 与
		* surface 同值，无边线时整窗是一块同色平板；四周留 1px 边距不被子控件覆盖
		*/
		void paintEvent(QPaintEvent* event) override;

		/*
		* 标题栏下分隔线开关转发：普通弹窗默认保留分界线；提示窗子类
		*（mFramelessMessageBox，各种提示/警告）构造时传 false 去掉横线
		*/
		void setTitleBarSeparatorVisible(bool on);

		/*
		* 标题栏只留关闭键转发：提示窗子类（mFramelessMessageBox）构造时传
		* true，隐藏最小化/最大化并禁双击最大化与顶边吸附（原型固定小窗形态）
		*/
		void setTitleBarCloseOnly(bool on);

		/*
		* 标题栏高度覆盖转发（0=随窗口头档位）：提示窗原型形态按 34 覆盖
		*（外框 400×156 三段式之一；供图系 1.5× 放大稿，÷1.5 实测标定）
		*/
		void setTitleBarHeight(int h);

		/*
		* 标题图标尺寸覆盖转发（无效 QSize=默认窗口头档方盒 44）：提示窗按
		* 白卡家族常数 logo.png 缩 14×16 覆盖——logo.png 满幅 M 标在默认大盒
		* 会渲染成 44px 巨标（现状截图实锤）；14×16 与原型 ÷1.5 咬合
		*/
		void setTitleBarIconSize(const QSize& size);

		/*
		* 标题字号/字重覆盖转发（0=默认窗口头字号 12 常规；字重 CSS 口径
		* 600→DemiBold）：提示窗按白卡对话框家族常数取 13px/600
		*/
		void setTitleBarFont(int pixelSize, int weight);

		/*
		* 紧凑标题排布覆盖转发（负值=默认方盒+windowInset 档）：转换设置
		* 形态（带高 62）按 2026-11-06 原型像素复核取 20/5——logo 左缘 20、
		* 标题格紧跟 logo；须在 setTitleBarIconSize 之后调用（紧凑图标格宽
		* =图标宽）。关闭键形态归 setTitleBarDialogFlatClose（2026-11-09
		* 用户裁定弹窗族对齐白卡家族图标钮，原"46 宽条"档被取代）
		*/
		void setTitleBarLayoutCompact(int iconLeft, int iconTextGap);

		/*
		* 弹窗族关闭键档转发（默认关）：关闭键改画 close3x 图标钮 24×24、
		* hover 淡灰圆角 2、无红底——对齐已验收白卡弹窗（通道映射/图层管理）
		* 的 ✕ 形态与"图标右缘距窗右 20"落位（2026-11-09 用户裁定：提示框
		* 族 ✕ 用主窗红条档与其它弹窗不一致）。提示框族原型形态启用
		*/
		void setTitleBarDialogFlatClose(bool on);

	private:
		mWindowTitleBar* m_titleBar = nullptr;
		QVBoxLayout* m_contentLay = nullptr;
	};
}
