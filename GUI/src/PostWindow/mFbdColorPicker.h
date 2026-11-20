#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QWidget>
#include "mpostwindow_global.h"

class QLineEdit;
class QLabel;
class QPushButton;
class QHideEvent;
class QScreen;
class QTimer;

namespace PostWindow
{
	class FbdSvSquare;
	class FbdHueBar;
	class FbdMagnifier;

	/*
	* 紧凑取色弹窗（可复用控件）：SV渐变方阵 + 吸管屏幕取色/当前颜色预览圆/色相条 +
	* RGB/HSL/HEX三种格式输入（通道名条右侧箭头循环切换）。
	* 以 Qt::Popup 无边框弹窗形态使用：点击弹窗外部即完成取色并自动关闭；用户改变颜色即发出 colorChanged。
	* 用法：new FbdColorPicker(parent) → setCurrentColor() → popupAt(全局坐标)；
	* 连接 colorChanged 实时接收颜色。吸管进入屏幕取色：取色弹窗保持可见，放大镜跟随光标，
	* 左键取色（弹窗保持打开），右键或Esc取消。图标资源（吸管/格式切换箭头）后续接入，当前为文字/空占位。
	*/
	class MPOSTWINDOW_EXPORT FbdColorPicker : public QWidget
	{
		Q_OBJECT

	public:
		explicit FbdColorPicker(QWidget* parent = nullptr);

		/** 当前颜色（取色过程中的实时值） */
		QColor currentColor() const;

		/** 设置当前颜色（程序设置不发出colorChanged） */
		void setCurrentColor(const QColor& color);

		/** 显示在指定全局坐标处（越界自动收回屏内） */
		void popupAt(QPoint globalTopLeft);

	signals:
		/** 用户取色变化（拖动SV/色相、输入分量、吸管确认） */
		void colorChanged(const QColor& color);

	protected:
		/** 屏幕取色模式下的全局事件过滤（移动放大镜/左键取色/右键取消/Esc取消） */
		bool eventFilter(QObject* watched, QEvent* event) override;

		/** 弹窗在取色模式下被外部隐藏（激活切换等，不经过事件过滤器）时结束取色，防止过滤器残留拦截全局点击 */
		void hideEvent(QHideEvent* event) override;

	private:
		/** 应用取色格式（0RGB/1HSL/2HEX）：切换输入框显隐、通道名并刷新数值 */
		void applyColorMode();

		/** 按当前格式刷新分量输入框数值 */
		void refreshColorEditors();

		/** 当前颜色应用：刷新输入框/预览圆并发出colorChanged */
		void applyColor(const QColor& color);

		/** 分量输入完成：按当前格式解析并应用 */
		void slotColorEditorsChanged();

		/** 吸管按钮：进入屏幕取色模式（取色弹窗保持可见，放大镜跟随光标） */
		void startScreenPick();

		/** 屏幕取色结束：移除事件过滤器与放大镜 */
		void endScreenPick();

		/** 取全局坐标处的屏幕颜色（实时抓取光标所在屏，支持多屏/高DPI） */
		QColor colorAtScreen(const QPoint& globalPos) const;

	private:
		FbdSvSquare*					_sv{};				//SV方阵（实现类型在cpp内）
		FbdHueBar*						_hue{};				//色相条
		QPushButton*					_eyedropper{};		//吸管按钮（图标资源后续接入）
		QLabel*							_preview{};			//当前颜色预览圆
		QLineEdit*						_r{};				//第一分量（R或H）
		QLineEdit*						_g{};				//第二分量（G或S）
		QLineEdit*						_b{};				//第三分量（B或L）
		QLineEdit*						_hex{};				//HEX输入（#RRGGBB）
		QLabel*							_lblR{};			//R/H通道名
		QLabel*							_lblG{};			//G/S通道名
		QLabel*							_lblB{};			//B/L通道名
		QLabel*							_hexLabel{};		//HEX通道名
		QPushButton*					_modeBtn{};			//格式循环切换（RGB/HSL/HEX）
		int								_mode{ 0 };			//取色格式：0RGB/1HSL/2HEX
		QColor							_cur{};				//当前颜色
		FbdMagnifier*					_magnifier{};		//屏幕取色放大镜
		QPixmap							_screenPix{};		//取色模式开始时的屏幕快照
		QScreen*						_snapScreen{};		//快照所属屏（跨屏轮询时据此重取快照）
		bool							_picking{ false };	//是否处于屏幕取色模式
		QTimer*							_pickTimer{};		//光标轮询定时器（悬停区域可能无MouseMove事件）
	};
}
