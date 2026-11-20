#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QWidget>
#include <QTimer>
#include <QToolButton>
#include <QSlider>
#include <QLabel>
#include <QPointer>
#include <QVector>
#include "mpostwindow_global.h"

class QAction;
class QMenu;

namespace PostWindow
{
	class mPostRealtimeAnimSettingsPopup;

	//实时动画播放控件条 v1.4（QToolBar 化，anim-tab-v13 形态反转：原 v1.3 ADS dock 内容件
	//改为主窗口工具栏壳的内容件——壳由 mCenterMainWindow::initAnimToolBar 预创建
	//(animToolBarProvider 通道)，面板 ensureRealtimeAnimMounted 经 provider 取壳回挂；
	//前处理隐藏/后处理常显（D-08"总是显示"），v1.3 dock 包裹机制随 ADS 形态退役）：
	//控件带按原型 V1.3 重排：设置入口(shezhi.svg，懒弹窗锚点)→动画类型钮(图标+菜单，
	//瞬态=帧动画/线性=线性动画/模态=正弦动画，D-01/D-02 纯更名映射引擎链零改动)→
	//分隔线(QFrame 单内容件方案裁定，J-08)→走带五钮(首帧/上一帧/播放暂停/下一帧/末帧，
//停止钮退役=首帧钮承担回起点 D-03)→滑条→读数(fps 下拉/循环钮退役=设置弹窗播放方式/
//速度承接 D-04)。帧形态=QTimer 逐帧经 frameIndexChanged 驱动面板 _comboFrame 切帧
//全链（渐进装载兜底）；线性/模态播放推进已合一会话播放器（见尾注 unify-anim-playback）；
//互斥判定点全部收口在面板
	//侧（本类只发信号，四路协议不变）；会话状态（播放方式/反向/步长/范围/速度/构形/倍率）
	//本类持有（面板实例内存活期，关闭弹窗不清值），逐字段校验呈现收口在设置弹窗
//（红边+文案，S1 错误保留原值）；滑条槽/手柄/分隔线配色取 UiPalette 语义色
//（StyleChange 现场重发）
//【unify-anim-playback】线性/模态播放推进已合一会话播放器（宿主分诊
//ensureParametricEnginePlayback 承载，恒引擎档）：插值步进/循环回绕/往返换向全在会话
//playTick，本类 tick 自驱表仅帧形态混合档保留；档位跟随经宿主镜像通道 mirrorEngineStep
//回流（方向感知），滑条按下/松手经 engineAdvanceSuspendRequested 停役/恢复会话推进
	class MPOSTWINDOW_EXPORT mPostRealtimeAnimWidget : public QWidget
	{
		Q_OBJECT
	public:
		//动画形态（条首类型钮）：瞬态=逐帧走帧表（D-01 更名，原"帧动画"）；
		//线性/模态=参数化变形插值档推进（D-01/D-02，原"线性动画/正弦动画"，模态=正弦同物异名）
		enum class AnimMode { Frame = 0, Linear = 1, Sine = 2 };
		//播放方式（设置弹窗）：单次=达边界停表复位；循环=边界回绕；往返=边界换向（FR-07）
		enum class PlayStyle { Once = 0, Loop = 1, PingPong = 2 };
		//显示方式（设置弹窗）：帧=读数"帧 n / m"（1 基）；时间="0.000 / 1.000"×倍率
		//（FR-09 D-05 升格真接入：读数=当前/域末帧时间×倍率 3 位小数；D-22 时间域归一化：
		//显示值=帧时间÷末帧时间，最大值恒 1.000×倍率=原型口径；帧时间数据缺失或末帧
		//<=0 回退按帧序显示，不提示不崩溃）
		enum class DisplayStyle { Frame = 0, Time = 1 };
		//模型构形（设置弹窗）：变形后=现状观感；未变形=渲染侧置零变形系数档
		//（FR-13 D-05 升格真接入：经面板 shapeChanged 槽写 setDeformationScale(0,0,0)）
		enum class ShapeMode { Undeformed = 0, Deformed = 1 };
		//插值档位数（与动画面板 totalf/渲染会话 _aniLinearCount 同值=20，插值公式消费）
		static const int kParamStepCount = 20;

		//会话状态（FR-06 八类设置项，默认值=原型/D-04/D-06 拍板口径；FR-09/11/13/14
		//D-05 升格真接入后全部字段实际生效）
		struct AnimSettings {
			PlayStyle   playStyle = PlayStyle::Loop;	//播放方式（默认循环）
			bool        reverse = false;				//反向播放（默认关；播放中切换即时掉头）
			DisplayStyle display = DisplayStyle::Time;	//显示方式（默认时间=原型口径；01 派生默认值
														//说明：时间轴接入后恢复原型默认"时间"档；
														//帧时间数据缺失读数回退帧序，档位不回切）
			long long   stepSize = 1;					//步长（正整数；D-16 对齐原型 safe-integer 域，long long 承载）
			bool        rangeEnabled = false;			//指定范围 W2（默认关=全帧表；FR-11 真接入：勾选后
														//起始/结束时间经帧时间表二分映射为帧序域；D-22：
														//端点=归一化时间域 0~1（帧时间/末帧时间），弹窗输入/
														//校验/文案同域）
			double      rangeStart = 0.0;
			double      rangeEnd = 1.0;
			int         fps = 10;						//播放速度 2~20 帧/秒步进 2（默认 10）
			ShapeMode   shape = ShapeMode::Deformed;	//模型构形（默认变形后）
			double      timeScale = 1.0;				//时间显示倍率（默认 1；FR-14 真接入：仅缩放时间读数）
		};

		explicit mPostRealtimeAnimWidget(int frameCount, int curIndex, QWidget* parent = nullptr);
		~mPostRealtimeAnimWidget();	//D-24：弹窗层先于按钮链显式删除（关闭崩溃修复，见 cpp）

		//---- 原型 SVG path 图标工厂（R4：用户否决 qrc 复用裁定 A7，改原型 icons/
		//animationTypes path 同形渲染——viewBox 24、stroke 1.5 round、fill:none）----
		//按给定色渲染单枚 path 位图（设置弹窗重置钮等外部消费点复用）
		static QPixmap protoIconPixmap(const QString& pathData, const QColor& color, int px);
		//三态图标（Off=text/On=selectedText/Disabled=disabled，UiPalette 现值）；
		//主题切换后由 applyIconTheme 全量重渲染
		static QIcon protoIcon(const QString& pathData, int px);

		void setFrameIndex(int index);          //外部切帧（面板帧下拉等）同步条上显示（仅帧形态语义）
		void setFrameCount(int count, int curIndex);	//v1.1 观察项 A：帧范围动态刷新（切工况重建帧表时调用）
		int  frameIndex() const { return _frameIndex; }
		int  step() const { return _step; }		//参数化形态当前插值档位（unify-anim-playback：宿主档位域尾端检查用，1..kParamStepCount）
		AnimMode mode() const { return _mode; }
		bool isPlaying() const { return _enginePlayback || (_playTimer != nullptr && _playTimer->isActive()); }
		bool enginePlayback() const { return _enginePlayback; }	//引擎合一：会话播放器驱动态（真值=自驱表停，推进由面板配置的会话承担）
		void enterEnginePlayback();					//引擎合一：自驱表静默停+引擎态落账（面板配置会话后调用）
		void exitEnginePlayback() { _enginePlayback = false; }	//引擎态清除（兜底口；常规停走 pausePlayback/quietStop）
		int  direction() const { return _direction; }			//播放方向（+1/-1，面板配置会话用）
		void pausePlayback();                    //外部请求暂停（预生成动画/录制互斥）
		void stopPlayback();                     //外部请求停止（内部停机链：停表+回起点）
		void yieldForRecording();                //P6-1 审计修复：录制让路专用停机（仅停表+视觉归位，不发 frameIndexChanged/interpolationStepChanged——不触发面板切帧全链）
		//引擎档播放推进镜像·参数化（unify-anim-playback）：档位+方向回摆同步——滑条/档位
		//读数跟随（applyStep emit=false 不回发防环）+方向基线落账（_direction，供 applyReverse/
		//engineSettingsChanged 下发消费）；仅由宿主参数化镜像通道调用
		void mirrorEngineStep(int step, int direction);
		//引擎档方向基线同步（帧/参数化共用，unify-anim-playback）：playbackDirectionChanged
		//回流（PingPong 端点内建换向）；纯基线落账零 UI 动作
		void mirrorEngineDirection(int direction);

		//v1.4 会话与上下文（面板/宿主驱动）
		void setEnabledContext(int frameCount);		//无结果=0/单帧=1/多帧=N：常显形态禁用域入口（D-08 停播+禁用替代隐藏）
		void setFrameTimes(const QVector<double>& times);	//帧时间轴下发（FR-09/11/14 真接入数据源，随 setFrameCount
															//同步刷新；全<=0=无帧时间数据→时间档回退帧序显示）
		bool timeAxisValid() const { return _timeAxisValid; }
		const AnimSettings& settings() const { return _session; }	//会话只读（面板 params.loop 映射消费，04 T-1）

		//FR-11 帧形态有效帧域（原型 bounds() 语义，引擎合一 2026-11-16 转公开：面板
		//起播分诊按域解析真实帧号）。W2 生效且帧时间轴可得时按帧时间表二分映射范围
		//端点→帧序；范围内无帧/映射失败=valid=false；W2 未勾选或帧时间数据缺失=全帧表
		struct FrameDomain { int lo = 0; int hi = 0; bool valid = false; };
		FrameDomain frameDomain() const;
		void resetShapeToDeformed();				//FR-13：面板侧上下文切换（结果切换/退出后处理）调用——构形会话
													//回"变形后"（恢复动作由面板槽自做，此处只复位会话不回发信号）
		void closeSettingsPopup();					//宿主退出链调用（切上下文关弹窗，§9 契约）
		void openSettingsPopup();					//显式打开（ui-shot 弹窗截图驱动经面板槽转发）
		void retranslateUi();						//语言切换全量重译（弹窗懒创建未创建时跳过）

		//---- ui-shot 工具栏状态驱动（SKILL §40，TC 状态矩阵补；objectName=animRealtimeContent，
		//经 mUiShotController invokeChildSlot allWidgets 兜底解析——工具栏子树不在面板内）----
		//帧/档为显示级驱动（applyIndex/applyStep emit=false：不触发切帧全链/懒生成，仅落
		//禁用矩阵与读数态——TC 边界态截图用）；模式/播放走真实链（modeChanged/playbackToggled）
		Q_INVOKABLE void setFrameForShot(int index);	//帧形态显示级切帧（0 基，applyIndex emit=false）
		Q_INVOKABLE void setStepForShot(int step);		//参数化档位显示级切换（1 基，applyStep emit=false）
		Q_INVOKABLE void setModeForShot(int mode);		//形态切换真实链（quietStop→换挡→modeChanged）
		Q_INVOKABLE void setPlayForShot(bool playing);	//播放钮真实链（toggled→slot_playToggled→面板闸）
		Q_INVOKABLE void showTypeMenuForShot();			//R3 盲区闭合：类型菜单非模态弹出（QMenu::popup，不阻塞事件循环，供 uishot grab）
		Q_INVOKABLE void sendHoverForShot(int target);	//R3 盲区闭合：synthetic QEvent::Enter 悬停态取证（0=设置 1=类型 2=播放 3=首帧 4=末帧）
		Q_INVOKABLE void refreshTypeMenuForShot();		//R3：确保类型菜单已建（懒重建入口，刷新选中态）

	signals:
		//帧形态：帧序变化（播放 tick/滑条/步进/首末帧统一出口）→ 面板驱动 _comboFrame 切帧全链
		void frameIndexChanged(int index);
		//v1.1 参数化形态：插值档变化（滑条/步进统一出口，1..kParamStepCount）
		//→ 面板互斥判定+静态定位路由（unify-anim-playback：播放推进承载已退役）
		void interpolationStepChanged(int step);
		void playbackToggled(bool playing);
		//引擎合一：引擎态下弹窗设置（速度/步长/方向/循环/范围）变更 → 面板即时下发会话
		//（FR-12"播放中调整即时生效"延伸到引擎档）
		void engineSettingsChanged();
		//【阻塞-2·unify-anim-playback】引擎档滑条按下推进停役/松手恢复请求：按下=true
		//（宿主会话 pause——拖动本地预览不被逐 tick 镜像抢写；不经 pausePlayback，
		//不清 _enginePlayback 不复位按钮）；松手=false（宿主仅 Paused 态回 play 恢复
		//推进——帧模式 seek 续播/参数化定位三连后均需续推）。仅引擎态发射
		void engineAdvanceSuspendRequested(bool suspended);
		//v1.1 形态切换（值为 AnimMode）→ 面板做渲染侧收尾（回帧模式退出本条自生成参数化形态）
		void modeChanged(int mode);
		//D-05 升格 FR-13：模型构形切换（值为 ShapeMode，仅变化沿发射）→ 面板渲染侧写
		//setDeformationScale（未变形=置零/变形后=恢复既定系数；deformationChanged
		//广播由渲染门面照常触发，测量标注等消费方自动跟随）。四路协议不变，本信号为
		//构形真接入新增下发通道
		void shapeChanged(int mode);

	protected:
		void changeEvent(QEvent* event) override;	//全局 QSS 重发（切主题/切档）→ 滑条/分隔线配色现场重发
		bool eventFilter(QObject* watched, QEvent* event) override;	//v1.4：工具栏壳拖动/脱停→关弹窗（原型拖动随手柄关闭）

	private slots:
		void slot_playToggled(bool playing);
		void slot_firstClicked();		//v1.4 首帧（停播+跳域起点，承担原停止钮回起点语义 D-03）
		void slot_lastClicked();		//v1.4 末帧（停播+跳域终点）
		void slot_prevClicked();
		void slot_nextClicked();
		void slot_sliderMoved(int value);
		void slot_settingsClicked();	//v1.4 设置入口（弹窗 toggle，关闭不清值）
		void slot_tick();

	private:
		void applyIndex(int index, bool emitChange);	//帧形态推进（界内钳位；D-23：推进即重落禁用矩阵）
		void applyStep(int step, bool emitChange);		//参数化形态推进（1..kParamStepCount；D-23 同刷新）
		double frameTimeAt(int idx) const;				//v1.4 帧时间取值（越界返 0，读数消费）
		double timeAxisEnd() const;						//D-22：归一化域分母=末帧时间（缺失/<=0 返 0→调用方回退帧序）
		double normalizedTime(double t) const;			//D-22：归一化显示值=t/末帧时间×倍率（读数/预览/宽度共用，调前须 tEnd>0）
		void stepBy(int direction);						//BUG-02：步进钮专用推进（钳位停不改方向、不发停机信号）
		void stopAtNaturalEnd();						//BUG-02：单次自然结束停表（先落边界帧再停，仅播放态发停机信号）
		void applyModeSwitch(int idx);					//形态切换共用核（菜单槽/ForShot 注入槽共用）
		void ensureTypeMenu();							//R3：类型菜单懒创建（菜单被 uishot 用后即焚后可重建）

		//v1.4 会话应用槽（设置弹窗信号直连成员函数指针，Qt5 新语法免 slots 声明）
		void applyPlayStyle(int style);
		void applyReverse(bool on);
		void applyDisplayStyle(int style);
		void applyStepSize(long long step);
		void applyRange(bool enabled, double start, double end);
		void applyFps(int fps);
		void applyShape(int mode);
		void applyTimeScale(double scale);
		void applySessionValid(bool valid);
		void advance(int direction);					//v1.4 推进核：方向×步长+边界按播放方式（tick/步进共用）
		void jumpToEdge(bool toStart);					//v1.4 首帧/末帧统一（停播+跳端点）
		void applyPlayStart();							//v1.4 起播方向尾端检查（FR-08：尾端→先跳对端起点）
		void setPlayingUi(bool playing);
		void quietStop();								//停表+复位播放钮（不发 playbackToggled，形态切换内部收尾用）
		void refreshSessionUi();						//按形态+会话刷新滑条域/步进提示/读数（原 refreshModeUi 扩展）
		void applyEnabledState();						//v1.4 禁用矩阵（02 §3.3：无结果/单帧/边界/校验错误；
														//D-23：刷新点收口 applyIndex/applyStep 推进咽喉——
														//边界态随帧序/档位即时翻转，域源=frameDomain 单点）
		void updateReadout();							//v1.4 读数按显示方式分派（帧档/时间档/无帧）
		QString formatAnimTime(double t) const;			//D-21：时间档读数格式化 helper（读数/拖动预览/宽度预留共用，防格式漂移）
		void updateReadoutReservedWidth();				//D-21：按当前显示格式最坏文本预留读数定宽（播放 tick 零几何变化）
		void setFrameLabelText(const QString& text);	//帧号/档位标签同值跳过（播放期逐 tick 自重绘节流）
		void applySliderStyle();						//滑条槽/手柄+分隔线按当前 UiPalette 重发（R4：渲染真值口径——轨道 6px/填充段 primary/拇指 15px）
		void applyIconTheme();						//R4：全部图标按当前 UiPalette 重渲染（StyleChange 现场重发）
		void ensureSettingsPopup();						//设置弹窗懒创建
		void refreshModeMenuUi();						//v1.4 类型入口图标/菜单勾选随选中项切换

		QTimer* _playTimer{};
		QToolButton* _btnSettings{};	//设置入口（弹窗锚点，checked=弹窗开态可感知 FR-16）
		QToolButton* _btnMode{};		//类型钮（图标+原生下箭头，点击弹自绘类型菜单层）
		QWidget* _separator{};			//类型组/走带组分隔（单内容件方案 QFrame 等价承担，J-08 裁定）
		QToolButton* _btnFirst{};
		QToolButton* _btnPlay{};
		QToolButton* _btnPrev{};
		QToolButton* _btnNext{};
		QToolButton* _btnLast{};
		QSlider* _slider{};
		QLabel* _frameLabel{};
		QPointer<QWidget> _menuMode{};			//类型菜单弹层（R3-D18：自绘 QWidget 列——QMenu 的 :checked 伪态不受 QSS 支持且 mUiMenu 自绘与原型不符）
		QPointer<QToolButton> _btnTypeFrame{}; QPointer<QToolButton> _btnTypeSine{}; QPointer<QToolButton> _btnTypeLinear{};	//菜单三项（选中=checked 底，原型 selected 底）
		QPointer<class mPostRealtimeAnimSettingsPopup> _settingsPopup;	//懒创建缓存（关闭不清值=值在 _session）
		AnimMode _mode{ AnimMode::Frame };
		int _frameIndex{ 0 };
		int _frameCount{ 0 };
		int _step{ 1 };					//参数化形态当前插值档位（1..kParamStepCount，与会话 playTick 域一致）
		int _contextFrameCount{ 0 };	//v1.4 宿主上下文帧数（0=无结果；禁用域权威源）
		int _direction{ 1 };			//v1.4 运行方向（+1/-1：反向播放基线/往返换向共用）
		bool _enginePlayback{ false };	//引擎合一：会话播放器驱动态（面板 play/pause 分诊落账）
		bool _sessionValid{ true };		//v1.4 会话校验态（弹窗回报；无效→播放暂停+走带禁）
		bool _timeAxisValid{ false };	//FR-09/11/14 真接入判据（全帧 getFrameTime>0=帧时间轴可得；
										//缺失→时间档读数/W2 时间域映射回退帧序/全表，不提示不崩溃）
		AnimSettings _session;
		QVector<double> _frameTimes;	//1 基序帧时间（面板批量下发缓存）
	};
}
