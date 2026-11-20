#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QVector>
#include "mPostRealtimeAnimWidget.h"
#include "mpostwindow_global.h"

class QCheckBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QRadioButton;
class QScrollArea;
class QSlider;
class QToolButton;

namespace PostWindow
{
	//动画设置上弹窗 v1.4（anim-tab-v13，原型《前处理原型-V1.3》动画设置面板）：
	//设置入口钮锚定的 Qt::Popup|FramelessWindowHint 顶层（吸附式弹窗先例：
	//mPostExplodePanel.cpp:514/mPostMeasureTool.cpp:725 同构；点外/Esc 自动关闭，
	//不抢主窗口焦点链）——28px 标准头（标题+关闭钮）+主体字段网格（标签列 84px 令牌）
	//+W2 指定范围激活框（勾选展开/取消勾选隐藏字段回收高度、标题行保留，§3.6 条款）。
	//本类只做"编辑呈现+逐字段校验"（S1：错误保留原值+红边+字段下方错误文案），会话
	//权威源在 mPostRealtimeAnimWidget::AnimSettings（关闭弹窗不清值=值在会话）；
	//逐字段 change 信号回控件条应用（播放中调整即时生效项：速度/反向/显示方式）。
	//Win11 DWM 三件套（DWMWA_WINDOW_CORNER_PREFERENCE/BORDER_COLOR/NCRENDERING_POLICY，
	//创建后与 Show 后各调一次，SKILL 踩坑 30 定案）；禁 WA_NativeWindow（黑闪纪律）；
	//W2 折叠高度回收走踩坑 34 三层互锁破除（顶层窗+布局 item 缓存+minimumSize 钳制）
	class MPOSTWINDOW_EXPORT mPostRealtimeAnimSettingsPopup : public QWidget
	{
		Q_OBJECT
	public:
		//anchor=设置入口钮（锚定上弹基准，Esc 关闭回焦点）
		explicit mPostRealtimeAnimSettingsPopup(QWidget* anchor);

		//会话镜像（控件条下发；QSignalBlocker 同步控件不回发信号）+校验上下文：
		//timeAxisValid=帧时间轴可得；tFirst/tLast=帧表首末时间（端点界校验）；
		//frameTimes=全帧时间（"范围内须存在动画帧"校验，timeAxisValid 才可达）。
		//D-22 时间域归一化：三者由控件条统一÷末帧时间后下发（0~1 域），W2 输入框/
		//校验界/文案/有帧扫描同域工作，本弹窗逻辑零改动（缺帧/末帧<=0 时原样下发，
		//timeAxisValid=false 只校验有限性）
		void setSession(const mPostRealtimeAnimWidget::AnimSettings& s,
			bool timeAxisValid, double tFirst, double tLast,
			const QVector<double>& frameTimes);

		//锚定上弹（J-07 裁定：上弹优先+顶部空间不足翻转下弹——宿主为窗口顶部工具栏，
		//上方无翻转空间，实际观感即下弹；视口左右各留 12px，规范 §3.8 D 通用）
		void popup();

		void retranslateUi();	//语言切换全量重译（全局 updateTranslator → 控件条转发）

		//---- ui-shot 字段驱动注入槽（SKILL §40 契约，TC-28 补；mUiShotController
		//dialogSlots 已登记本弹窗，弹窗打开后经点分调用 mAnimSettingsPopup.setXxxForShot
		//驱动——invokeChildSlot 仅支持 bool/int 参）----
		//走控件原生信号链（radio setChecked→toggled→changed 信号→控件条 applyXxx 应用），
		//与用户点击同路径；Q_INVOKABLE 按名调用需元对象可见（踩坑 39）
		Q_INVOKABLE void setPlayStyleForShot(int style);	//0=单次 1=循环 2=往返
		Q_INVOKABLE void setDisplayStyleForShot(int style);	//0=时间 1=帧（TC-28 主用例；内部映射 displayChanged 枚举值 1/0）
		Q_INVOKABLE void setShapeForShot(int mode);			//0=未变形 1=变形后
		Q_INVOKABLE void setStepSizeForShot(int step);		//步长文本注入（经 textChanged 校验链，非法值走 S1 错误态）
		Q_INVOKABLE void setTimeScaleForShot(int value);	//倍率文本注入（经 textChanged 校验链+读数刷新，FR-14 真接入驱动）
		Q_INVOKABLE void setRangeForShot(int startCenti, int endCenti);	//W2 勾选+端点文本注入（百分比/100→两位小数，经既有校验/回发链）
		Q_INVOKABLE void setReverseForShot(bool on);		//反向播放勾选注入
		Q_INVOKABLE void sendHoverForShot(int target);		//R3 盲区闭合：synthetic Enter 悬停态取证（0单次 1循环 2往返 3反向 4时间 5帧 6重置钮 7未变形 8变形后 9倍率）
		Q_INVOKABLE void sendKeyForShot(int key);			//R3 盲区闭合：synthetic 方向键（0=Left 1=Right 2=Up 3=Down 4=Home 5=End）→ 焦点控件（单选组导航）

	signals:
		void playStyleChanged(int style);			//0=单次 1=循环 2=往返（PlayStyle）
		void reverseChanged(bool on);
		void displayChanged(int style);				//0=帧 1=时间（控件条 DisplayStyle 枚举同源，BUG-07 对齐）
		void stepSizeChanged(qint64 step);			//仅合法值回发（错误保留原值 S1；D-16 对齐原型 safe-integer 域）
		void rangeChanged(bool enabled, double start, double end);
		void fpsChanged(int fps);					//2..20 偶数
		void shapeChanged(int mode);				//0=未变形 1=变形后（ShapeMode）
		void timeScaleChanged(double scale);		//仅合法值回发（textChanged 逐值同步会话，FR-14 真接入：仅缩放时间读数）
		void sessionValidated(bool valid);			//会话整体校验态变化（无效→控件条走带禁+暂停）

	protected:
		void showEvent(QShowEvent* event) override;	//DWM 三件套第二次+焦点入首控件（FR-16）
		void keyPressEvent(QKeyEvent* event) override;	//Esc 关闭回焦点入口（FR-16）

	private:
		//逐字段校验（FR-15 文案逐字）：错误=红边+字段下方错误文案+错误保留原值；
		//timeAxisValid=false 时端点仅校验有限性（界校验随帧时间轴接入激活）
		bool validateStepSize();
		bool validateTimeScale();
		bool validateRangeFields();
		void refreshSessionValid();					//汇总校验态→sessionValidated（幂等去重）
		void applyFieldError(QLineEdit* field, QLabel* errSlot, const QString& text);
		void updateFpsReadout();					//"N 帧/秒"随动读数
		void refreshPopupHeight();					//W2 折叠/展开高度收敛（踩坑 34 破互锁）
		void snapFps(int value);					//2..20 步进 2 吸附（鼠标拖动取偶）

		QWidget* _anchor{};							//设置入口钮（锚点）
		QScrollArea* _bodyScroll{};					//主体滚动区（高度收敛重建布局 item 的对象，踩坑 34）
		QWidget* _bodyHost{};						//滚动区内容宿主（字段网格）
		QLabel* _lblTitle{};						//28px 头标题（重译）
		QToolButton* _btnClose{};					//28px 头关闭钮
		QRadioButton* _rbOnce{}; QRadioButton* _rbLoop{}; QRadioButton* _rbPingPong{};
		QCheckBox* _chkReverse{};
		QRadioButton* _rbTime{}; QRadioButton* _rbFrame{};
		QLineEdit* _editStep{}; QToolButton* _btnStepReset{};
		QGroupBox* _rangeFrame{}; QWidget* _rangeBoxInner{};	//W2 激活框（checkable QGroupBox=fieldset legend 断口样式，D-06）
		QLineEdit* _editRangeStart{}; QLineEdit* _editRangeEnd{};
		QSlider* _sliderFps{}; QLabel* _lblFpsReadout{};
		QRadioButton* _rbUndeformed{}; QRadioButton* _rbDeformed{};
		QLineEdit* _editTimeScale{};
		QLabel* _errStep{}; QLabel* _errTimeScale{};
		QLabel* _errRangeStart{}; QLabel* _errRangeEnd{};
		bool _timeAxisValid{ false };
		double _tFirst{ 0.0 };
		double _tLast{ 1.0 };
		QVector<double> _frameTimes;				//全帧时间（1 基序对齐，有帧校验用）
		bool _sessionValid{ true };
	};
}
