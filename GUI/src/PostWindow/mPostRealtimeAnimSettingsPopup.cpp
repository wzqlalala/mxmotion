#include "mPostRealtimeAnimSettingsPopup.h"

//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QCheckBox>
#include <QCursor>
#include <QDebug>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QScreen>
#include <QScrollArea>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include "UiPalette.h"
#include "UiMetrics.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace PostWindow
{
	namespace
	{
		//弹窗超高内部滚动上限（FR-06：主体超高内部滚动；9 行字段正常态不触顶）
		const int kMaxPopupBodyHeight = 480;

		//Win11 DWM 对顶层窗默认施加圆角+描边+投影（SKILL 踩坑 30，0.21 轮定案）：
		//与窗口类样式无关，按窗口属性关闭——DWMWA_WINDOW_CORNER_PREFERENCE=DONOTROUND(33,1)、
		//DWMWA_BORDER_COLOR=DWMWA_COLOR_NONE(34,0xFFFFFFFE)、DWMWA_NCRENDERING_POLICY=
		//DISABLED(2,1)。dwmapi 经 GetProcAddress 动态取（ThemeManager.cpp:506 同款，免链接依赖）；
		//配套纪律：轻投影禁 QGraphicsDropShadowEffect（Popup 上为实心剪影）、禁 WA_NativeWindow
		void applyPopupDwmAttributes(QWidget* w)
		{
#ifdef _WIN32
			using DwmSetWindowAttributeFn = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
			static DwmSetWindowAttributeFn fn = []() {
				HMODULE mod = ::GetModuleHandleW(L"dwmapi.dll");
				return mod != nullptr
					? reinterpret_cast<DwmSetWindowAttributeFn>(::GetProcAddress(mod, "DwmSetWindowAttribute"))
					: DwmSetWindowAttributeFn(nullptr);
			}();
			if (fn == nullptr || w == nullptr)
				return;
			HWND hwnd = reinterpret_cast<HWND>(w->winId());
			if (hwnd == nullptr)
				return;
			const DWORD cornerPref = 1;					//DWMWCP_DONOTROUND
			fn(hwnd, 33, &cornerPref, sizeof(cornerPref));
			const COLORREF colorNone = 0xFFFFFFFE;		//DWMWA_COLOR_NONE
			fn(hwnd, 34, &colorNone, sizeof(colorNone));
			const DWORD ncDisabled = 1;					//DWMWCP_DISABLE
			fn(hwnd, 2, &ncDisabled, sizeof(ncDisabled));
#else
			Q_UNUSED(w);
#endif
		}

		//字段行标签（标签列令牌：文字区 84px+左内距 6px=固定宽 90px——R4-A 网格对位：
		//外层表单边距 6+标签左内距 6 → 文字 x=13、控件列 x=105，与原型实测坐标逐位
		//一致（真身 computed：fields padding-inline 12=6 容器边距+6 内距两段承担，
		//范围框 margin-inline 6 与字段边距错层）；SKILL §2.3 label-width；控件创建即设
		//初始文本，语言切换只做刷新——踩坑 16 尾）。D-11：色=label 次文字色、左对齐+
		//长标签折行（proto 规则 41：--std-label 色 flex 左对齐）
		QLabel* makeFieldLabel(const QString& text, int leftInset = 6)
		{
			QLabel* lbl = new QLabel(text);
			lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
			lbl->setFixedWidth(84 + leftInset);
			lbl->setContentsMargins(leftInset, 0, 0, 0);
			//R4-A：行高 26 档定高（原型规则 41 min-height:control）。定高而非仅 min——
			//Qt wordWrap 标签 sizeHint 按可折两行启发式取 28px（"时间显示倍率"一行
			//72px 实际可容纳于 84px 列），行盒被抬高 2px → 弹窗总高 293≠原型 291；
			//标准密度下全部标签均单行（最长 6 字 72px≤84px），定高无截断副作用
			lbl->setFixedHeight(MxFunctions::UiMetrics::baseH());
			lbl->setWordWrap(true);
			lbl->setStyleSheet(QStringLiteral("QLabel{color:%1;background:transparent;border:none;}")
				.arg(MxFunctions::UiPalette::labelHex()));
			return lbl;
		}

		//字段下方错误文案槽（16px 高；R2-D17 对齐原型规则 66 hidden 语义——无错时隐藏
		//不占高，显隐经 applyFieldError + refreshPopupHeight 收敛高度；文案色=error 令牌；
		//R4-A 字号=UiMetrics 辅助字号像素档（标准 11px=原型 --std-helper computed 实测），
		//不再写死 11——密度档位随全局体系缩放；margin-top 4=规则 66 margin）
		QLabel* makeErrorSlot()
		{
			QLabel* err = new QLabel(QString());
			err->setFixedHeight(16);
			err->setContentsMargins(0, 4, 0, 0);
			err->setVisible(false);
			err->setStyleSheet(QStringLiteral("QLabel{color:%1;font-size:%2px;background:transparent;border:none;}")
				.arg(MxFunctions::UiPalette::error().name(),
					 QString::number(MxFunctions::UiMetrics::helperFontSize())));
			return err;
		}

		//弹窗关闭钮图形（R4-E 同形：原型 icons.close 'm3 3 6 6m0-6-6 6' 于 12×12 盒、
		//stroke 1px 平头帽直绘两线段——proto_anim_script.js icons.close+规则 35 真值；
		//原"✕"文本字形为近似替代，两斜线几何完全一致且线宽/帽型与原型像素级同形）
		QPixmap headerCloseIcon(const QColor& color)
		{
			QPixmap pm(12, 12);
			pm.fill(Qt::transparent);
			QPainter p(&pm);
			p.setRenderHint(QPainter::Antialiasing);
			p.setPen(QPen(color, 1.0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
			p.drawLine(3, 3, 9, 9);
			p.drawLine(9, 3, 3, 9);
			p.end();
			return pm;
		}

		//输入框基础样式（surface 底/border 边/2px 圆角/焦点 primary/error 态红边，令牌 §2.3；
		//disabled=panel 底 disabled 字（原型规则 44）；invalid 聚焦保持 error 边（规则 47）；
		//QSS 字面量单行不断开——踩坑 4；实参与占位符一一对应——踩坑 35）
		QString editStyle()
		{
			return QStringLiteral(
				"QLineEdit{background:%1;color:%2;border:1px solid %3;border-radius:4px;padding:1px 4px;}"
				"QLineEdit:focus{border-color:%4;}"
				"QLineEdit[animFieldError=\"true\"]{border:1px solid %5;}"
				"QLineEdit[animFieldError=\"true\"]:focus{border-color:%5;}"
				"QLineEdit:disabled{background:%6;color:%7;}")
				.arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::textHex(),
					 MxFunctions::UiPalette::borderHex(), MxFunctions::UiPalette::primaryHex(),
					 MxFunctions::UiPalette::error().name(), MxFunctions::UiPalette::panelHex(),
					 MxFunctions::UiPalette::disabledHex());
		}

		//数值默认文本（BUG-06：整数不带小数，与原型默认 0/1/1 一致；非整数量值自然带小数）
		QString numText(double v)
		{
			return QString::number(v);
		}

		//D-19 段选中底属性同步（QSS 属性选择器 [segChecked]/[segHover]）：Qt 无父子
		//状态选择器，段容器视觉随 radio checked 经动态属性+unpolish/polish 即时生效
		//（applyFieldError 同款口径）；setSession 的 QSignalBlocker 屏蔽 toggled，
		//程序性同步由其尾部循环调本函数补齐
		void syncSegmentChecked(QRadioButton* rb)
		{
			QWidget* cell = rb->parentWidget();
			if (cell == nullptr || cell->property("segChecked").toBool() == rb->isChecked())
				return;
			cell->setProperty("segChecked", rb->isChecked());
			cell->style()->unpolish(cell);
			cell->style()->polish(cell);
		}

		//D-19 段容器事件桥（原型 choices>label=整段可点/hover 的 <label> 语义）：radio
		//退为内容单元后设鼠标透明，容器接管整段鼠标——①Enter/Leave 同步 segHover
		//（悬停底铺满段宽，含内容单元上方区域）；②按下/释放/移动/双击按坐标平移+
		//钳入 radio 矩形后直发 radio（段内任意位置点击语义=点击该选项，与原实现
		//"段=radio"可点范围逐位同域）；容器自身不消费鼠标
		class SegmentCellRouter : public QObject
		{
		public:
			SegmentCellRouter(QWidget* cell, QRadioButton* rb)
				: QObject(cell), _cell(cell), _rb(rb) {}

		protected:
			bool eventFilter(QObject* watched, QEvent* event) override
			{
				if (watched != _cell)
					return QObject::eventFilter(watched, event);
				switch (event->type())
				{
					case QEvent::Enter:
					case QEvent::Leave:
					{
						const bool hover = event->type() == QEvent::Enter;
						if (_cell->property("segHover").toBool() != hover)
						{
							_cell->setProperty("segHover", hover);
							_cell->style()->unpolish(_cell);
							_cell->style()->polish(_cell);
						}
						break;
					}
					case QEvent::MouseButtonPress:
					case QEvent::MouseButtonRelease:
					case QEvent::MouseMove:
					case QEvent::MouseButtonDblClick:
					{
						QMouseEvent* me = static_cast<QMouseEvent*>(event);
						QPoint p = me->pos() - _rb->pos();
						p.setX(qBound(0, p.x(), _rb->width() - 1));
						p.setY(qBound(0, p.y(), _rb->height() - 1));
						QMouseEvent fwd(me->type(), p, me->button(), me->buttons(), me->modifiers());
						QCoreApplication::sendEvent(_rb, &fwd);
						return true;
					}
					default:
						break;
				}
				return QObject::eventFilter(watched, event);
			}

		private:
			QWidget* _cell;
			QRadioButton* _rb;
		};

		//D-19 分段连体段容器（原型规则 54：choices>label display:flex+align-items/
		//justify-content:center——「radio 13px+隙 4+文字」作为整体内容单元在段内
		//水平垂直居中；规则 53-61 的边框/圆角/选中底/hover 属于整段 label，铺满段宽
		//不随内容缩窄）：animRole 边框圆角/segChecked 选中底/segHover 悬停底全部落
		//容器（属性选择器），radio 退为透明内容单元按 sizeHint 宽居中（两侧 stretch
		//等分 slack=justify-content:center 同效）；段高仍 26 档（规则 54 height:control）
		QWidget* makeSegmentCell(QRadioButton* rb, const char* role, int segHeight)
		{
			QWidget* cell = new QWidget(rb->parentWidget());
			cell->setAttribute(Qt::WA_StyledBackground, true);	//属性选择器底色需自绘背景
			cell->setProperty("animRole", QString::fromLatin1(role));
			cell->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			cell->setMinimumHeight(segHeight);
			QHBoxLayout* lay = new QHBoxLayout(cell);
			lay->setContentsMargins(0, 0, 0, 0);
			lay->setSpacing(0);
			lay->addStretch(1);		//两侧 stretch 等分富余→内容单元水平居中
			lay->addWidget(rb);		//radio 改挂容器（sizeHint 宽=13+隙 4+文字，Fixed 不参与分宽）
			lay->addStretch(1);
			rb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			rb->setMinimumHeight(segHeight);	//R4-A：段选行高 26 档
			rb->setAttribute(Qt::WA_TransparentForMouseEvents);	//整段鼠标归容器（转发桥见 SegmentCellRouter）
			cell->installEventFilter(new SegmentCellRouter(cell, rb));
			QObject::connect(rb, &QRadioButton::toggled, cell, [rb]() { syncSegmentChecked(rb); });
			syncSegmentChecked(rb);	//构造默认选中段底色即时备好（polish 前属性已就位）
			return cell;
		}
	}

	//动画设置上弹窗（构造即建全量控件并置初始文本，retranslateUi 只刷新——踩坑 16 尾；
	//布局：28px 头（标题+关闭钮）+QScrollArea 主体（超高内部滚动，FR-06）+字段网格
	//（QFormLayout 标签列 84px）+W2 指定范围激活框）
	mPostRealtimeAnimSettingsPopup::mPostRealtimeAnimSettingsPopup(QWidget* anchor)
		: QWidget(anchor, Qt::Popup | Qt::FramelessWindowHint)
		, _anchor(anchor)
	{
		setObjectName("mAnimSettingsPopup");	//ui-shot 弹窗注册名（mUiShotController::dialogSlots）
		setAttribute(Qt::WA_StyledBackground, true);
		//弹窗外壳（D-07：原型规则 31 border-radius:0=直角弹窗，surface 底+border 1px；
		//布局边距仍须大于描边弧段——直角无弧段，2px 边距足够，踩坑 16）
		setStyleSheet(QStringLiteral("#mAnimSettingsPopup{background:%1;border:1px solid %2;border-radius:0;}")
			.arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::borderHex()));
		//R4-A 弹窗字号钉桩（用户两轮否决项①——字号实测差）：根控件显式 setFont=
		//UiMetrics 正文字号像素档（标准档 12px=真身页面 .cae-ui 根字体 12px/1.5 雅黑
		//computed 实测，弹窗 CSS font:inherit 即继承该值）。不依赖全局 QSS @{font.body}
		//逐控件覆盖：全局 QSS 只声明了 QLabel/QLineEdit 等部分类的 font-size，
		//QRadioButton/QGroupBox 仅声明 font-family——字号回落应用默认字体
		//（QFont(族) 默认点阵≈16px）→ 段选/指定范围标题渲染成大字，与原型 12px
		//"明显不一样"（用户截图指认项根因）。setFont 沿父子链传播，全部子控件
		//归位 12px；密度档位切换按 UiMetrics 标准档公式随动（标准档恒=原型 12px），
		//错误 11px 档同源（makeErrorSlot）。真值证据：protoaudit4 原型 computed
		//title/label/radio/chk/inp/speedVal/legend 全 12px/400 + 实现 QFontInfo dump 对表
		QFont popupFont = font();
		popupFont.setFamily(MxFunctions::UiMetrics::uiFontFamily());
		popupFont.setPixelSize(MxFunctions::UiMetrics::bodyFontSize());
		setFont(popupFont);
		setFixedWidth(280);		//宽=panel-min（02 §3.2：宽 280 标准档；R4-A 复核=原型
		//--std-panel-min computed 280 与真身 offsetWidth 实测 280 逐位一致，sizeHint
		//不放宽——字段列 90+8+162+6+6+2=274<280 余量 6px，字体钉桩后无撑宽源）

		QVBoxLayout* rootLay = new QVBoxLayout(this);
		rootLay->setContentsMargins(1, 1, 1, 1);	//弹窗自身描边由外壳承担，头/主体贴边
		rootLay->setSpacing(0);

		//28px 标准头：标题+关闭钮（SKILL §3.2——头 28px、标题左对齐、关闭钮 24×24 最右
		//垂直居中；R4-E 对位原型 computed：头高 28（含底边线，box-sizing border-box）、
		//标题 12px/字重 400（原型规则 32 header font-weight:400 显式覆盖公共头 600——
		//以原型 CSS 真值为准）、padL=window-inset 8、padR=window-end-inset 4）
		QWidget* head = new QWidget(this);
		head->setFixedHeight(28);
		head->setStyleSheet(QStringLiteral("QWidget{background:%1;border-bottom:1px solid %2;}")
			.arg(MxFunctions::UiPalette::headerHex(), MxFunctions::UiPalette::borderHex()));
		QHBoxLayout* headLay = new QHBoxLayout(head);
		headLay->setContentsMargins(MxFunctions::UiMetrics::windowInset(), 0, 4, 0);
		_lblTitle = new QLabel(tr("动画设置"), head);
		headLay->addWidget(_lblTitle);
		headLay->addStretch(1);
		_btnClose = new QToolButton(head);
		_btnClose->setIcon(headerCloseIcon(MxFunctions::UiPalette::text()));	//R4-E：原型 close path 同形
		_btnClose->setIconSize(QSize(12, 12));
		_btnClose->setToolTip(tr("关闭动画设置"));
		_btnClose->setAutoRaise(true);
		_btnClose->setFixedSize(QSize(24, 24));
		headLay->addWidget(_btnClose);
		rootLay->addWidget(head);

		//主体（超高内部滚动，FR-06；_bodyScroll 供 W2 高度收敛时重建布局 item——踩坑 34；
		//注意 _bodyHost 是滚动区内层 widget，不在弹窗根布局里，重建目标必须是 _bodyScroll）
		QScrollArea* body = new QScrollArea(this);
		body->setWidgetResizable(true);
		body->setFrameShape(QFrame::NoFrame);
		body->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		_bodyHost = new QWidget(body);
		//主体底=surface；D-01 分段连体单选组（原型规则 53-61+popup 实测：选项为连体
		//边框段、选中=selected 底 selectedText 字、hover=hover、首末段圆角=--std-radius
		//4px（R4 勘误：原型 computed border-radius 4px 0 0 4px，旧 2px 系误读
		//--ui-radius——真值 4px 与输入框/范围框/菜单同源）、相邻段 border-left:0；
		//D-19 起段视觉落段容器（makeSegmentCell）：animRole 边框圆角铺满段宽+
		//segChecked/segHover 属性选择器承担选中底/悬停底，radio 退为透明内容单元
		//按 sizeHint 居中——Qt 无父子状态选择器，容器属性经 SegmentCellRouter/
		//syncSegmentChecked 与 radio 状态同步）；D-02 速度滑条（原型规则 70-71 computed：
		//2px 轨道 border 色+13px 圆形 primary 手柄、margin-top=(2−13)/2=-5.5 垂直居中）；
		//全局 QSS indicator 14+2px 使三项单选行超宽的压缩规则保留
		//（13px+padding0 贴 SKILL §3.0 choice 档）；QSS 单行——踩坑 4
		_bodyHost->setStyleSheet(QStringLiteral(
			"QWidget{background:%1;}"
			"QRadioButton{color:%2;background:transparent;padding:0;spacing:4px;font-size:%8px;}"
			"QRadioButton::indicator{width:13px;height:13px;padding:0px;}"
			"QRadioButton:checked{color:%5;}"
			"QWidget[animRole=\"first\"]{border:1px solid %3;border-top-left-radius:4px;border-bottom-left-radius:4px;}"
			"QWidget[animRole=\"mid\"]{border:1px solid %3;border-left:none;}"
			"QWidget[animRole=\"last\"]{border:1px solid %3;border-left:none;border-top-right-radius:4px;border-bottom-right-radius:4px;}"
			"QWidget[animRole][segHover=\"true\"]{background:%6;}"
			"QWidget[animRole][segChecked=\"true\"]{background:%4;}"
			"QCheckBox{color:%2;background:transparent;padding:0;spacing:4px;font-size:%8px;}"
			"QCheckBox::indicator{width:13px;height:13px;padding:0px;}"
			"QSlider{min-height:26px;}"
			"QSlider::groove:horizontal{height:2px;background:%3;}"
			"QSlider::handle:horizontal{width:13px;height:13px;background:%7;border-radius:6px;margin:-5.5px 0;}")
			.arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::textHex(),
				 MxFunctions::UiPalette::borderHex(), MxFunctions::UiPalette::selectedHex(),
				 MxFunctions::UiPalette::selectedTextHex(), MxFunctions::UiPalette::hoverHex(),
				 MxFunctions::UiPalette::primaryHex(),
				 QString::number(MxFunctions::UiMetrics::bodyFontSize())));
		//R4-A 字号钉桩补遗（dump QFontInfo 实证）：全局 QSS 的 QRadioButton 规则只声明
		//font-family——QSS 字体声明存在时未声明属性回落应用默认字体（≈16px）而非父子链
		//继承字体，弹窗根 setFont 对 radio 不生效（T8 dump：radio fontPx=16/label=12）。
		//本主体 QSS 显式补 font-size=%8px 后归位 12px（原型 computed radioSpan 12px）
		QFormLayout* form = new QFormLayout(_bodyHost);
		//R4-A 网格对位（真身坐标复刻）：表单容器边距 6（=原型 container-edge，范围框
		//margin-inline 6 的错层承担者），字段级 12 由"标签左内距 6（makeFieldLabel）+
		//控件行右内距 6（各行 margins）"两段承担——标签文字 x=13/控件列 x=105/
		//控件右缘 267 与原型 computed 逐位一致；行距 6=--std-gap、列距 8=--std-column-gap
		form->setContentsMargins(6, 6, 6, 6);
		form->setHorizontalSpacing(8);
		form->setVerticalSpacing(6);
		const int ctlH = MxFunctions::UiMetrics::baseH();
		//控件行统一右内距 6（R4-A：控件右缘=280−1−6−6=267=原型实测）
		const int kCellRightInset = 6;

		//1. 播放方式（三选分段连体单选组 D-01，默认循环；方向键/Home/End 由 QRadioButton
		//分组原生承担，FR-16；等宽填满控件列=原型 choices grid 规则 53；D-19 段内
		//内容单元居中=原型规则 54 flex center）
		QWidget* playStyleRow = new QWidget(_bodyHost);
		QHBoxLayout* playStyleLay = new QHBoxLayout(playStyleRow);
		playStyleLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6
		playStyleLay->setSpacing(0);	//D-01 分段连体：段间 0 共边线
		_rbOnce = new QRadioButton(tr("单次"), playStyleRow);
		_rbLoop = new QRadioButton(tr("循环"), playStyleRow);
		_rbPingPong = new QRadioButton(tr("往返"), playStyleRow);
		_rbLoop->setChecked(true);	//默认循环（FR-06）
		playStyleLay->addWidget(makeSegmentCell(_rbOnce, "first", ctlH));
		playStyleLay->addWidget(makeSegmentCell(_rbLoop, "mid", ctlH));
		playStyleLay->addWidget(makeSegmentCell(_rbPingPong, "last", ctlH));
		form->addRow(makeFieldLabel(tr("播放方式")), playStyleRow);

		//2. 反向播放（默认关；空标签占位保持三线对齐）
		QWidget* reverseRow = new QWidget(_bodyHost);
		QHBoxLayout* reverseLay = new QHBoxLayout(reverseRow);
		reverseLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6
		reverseLay->setSpacing(8);
		_chkReverse = new QCheckBox(tr("反向播放"), reverseRow);
		_chkReverse->setChecked(false);
		_chkReverse->setMinimumHeight(ctlH);	//R4-A：行高 26 档（原型规则 48 min-height:control）
		reverseLay->addWidget(_chkReverse);
		reverseLay->addStretch(1);
		form->addRow(makeFieldLabel(QString()), reverseRow);

		//3. 显示方式（两选分段连体单选组，默认时间=原型口径——FR-09 D-05 升格真接入，
		//01 派生默认值说明：帧时间轴接入后恢复原型默认"时间"档；帧时间数据缺失时读数
		//由控件条回退帧序显示，档位不回切）
		QWidget* displayRow = new QWidget(_bodyHost);
		QHBoxLayout* displayLay = new QHBoxLayout(displayRow);
		displayLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6
		displayLay->setSpacing(0);
		_rbTime = new QRadioButton(tr("时间"), displayRow);
		_rbFrame = new QRadioButton(tr("帧"), displayRow);
		_rbTime->setChecked(true);
		displayLay->addWidget(makeSegmentCell(_rbTime, "first", ctlH));
		displayLay->addWidget(makeSegmentCell(_rbFrame, "last", ctlH));
		form->addRow(makeFieldLabel(tr("显示方式")), displayRow);

		//4. 步长+重置钮（默认 1；重置=瞬时动作钮一键回 1，FR-10；D-14 输入框填满控件列、
		//D-10 重置=图标钮 proto reset 资产）+字段下方错误槽
		QWidget* stepRow = new QWidget(_bodyHost);
		QHBoxLayout* stepLay = new QHBoxLayout(stepRow);
		stepLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6（输入 130+隙 6+重置钮 26=162 控件列逐位对位）
		stepLay->setSpacing(6);
		_editStep = new QLineEdit(QString::number(1), stepRow);
		_editStep->setStyleSheet(editStyle());
		_editStep->setMinimumHeight(ctlH);
		_editStep->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		stepLay->addWidget(_editStep, 1);
		_btnStepReset = new QToolButton(stepRow);
		//R4：重置图标=原型 reset path 同形渲染（环形箭头，废弃 qrc reset.png——A7 否决
		//qrc 复用，工厂静态方法复用控件条 SVG path 渲染口径）
		_btnStepReset->setIcon(mPostRealtimeAnimWidget::protoIconPixmap(
			QLatin1String("M5 8a8 8 0 1 1-1 7M5 3v5h5"),
			MxFunctions::UiPalette::text(), 16));
		_btnStepReset->setIconSize(QSize(16, 16));
		_btnStepReset->setToolTip(tr("重置步长为 1"));
		_btnStepReset->setAutoRaise(true);
		_btnStepReset->setFixedSize(QSize(ctlH, ctlH));
		stepLay->addWidget(_btnStepReset);
		_errStep = makeErrorSlot();
		QWidget* stepCell = new QWidget(_bodyHost);
		QVBoxLayout* stepCellLay = new QVBoxLayout(stepCell);
		stepCellLay->setContentsMargins(0, 0, 0, 0);
		stepCellLay->setSpacing(0);
		stepCellLay->addWidget(stepRow);
		stepCellLay->addWidget(_errStep);
		form->addRow(makeFieldLabel(tr("步长")), stepCell);

		//5. W2 指定范围激活框（D-06：checkable QGroupBox=fieldset——标题嵌于边框线上
		//（原型放大实测：legend 文字竖直中心压在框顶边线上），勾选展开字段/取消勾选
		//隐藏回收高度；再启用恢复暂存值）
		_rangeFrame = new QGroupBox(tr("指定范围"), _bodyHost);
		_rangeFrame->setObjectName("animRangeFrame");
		_rangeFrame->setCheckable(true);
		_rangeFrame->setChecked(false);
		//R4-A fieldset 对位（原型 computed+放大实测，protoaudit4）：
		//①圆角 4px（computed border-radius 4px，旧 2px 系误读 --ui-radius）；
		//②标题字号显式钉 UiMetrics 正文像素档（全局 QSS QGroupBox 无 font-size——
		//旧版回落应用默认≈16px 大字=用户指认"弹窗字体明显不一样"第二处）；
		//③标题嵌线几何：margin-top 9=标题带高度（标题行 18px 自 y0 起、边线过其
		//竖直中心 y9——原型 legend 压线同款；旧 margin-top 17 使标题悬空框外、折叠
		//41px 高的"标题+空盒"两段式与原型不符）；padding-top 12=展开字段起始避开
		//标题带（展开态框高 9+1+12+内容 58+5+1=86≈原型 89，复验微调）；
		//④折叠定高 25px=原型规则 65 calc(line-height 18+gap 6+1 边线)，展开交布局
		//自然收敛（toggle 槽解固定）
		_rangeFrame->setStyleSheet(QStringLiteral(
			"QGroupBox#animRangeFrame{border:1px solid %1;border-radius:4px;margin-top:9px;margin-left:0;margin-right:0;padding:11px 5px 5px 5px;color:%2;font-size:%4px;font-weight:400;}"
			"QGroupBox#animRangeFrame::title{subcontrol-origin:margin;subcontrol-position:top left;left:8px;padding:0 4px;background:%3;color:%2;}"
			"QGroupBox#animRangeFrame::indicator{width:13px;height:13px;}")
			.arg(MxFunctions::UiPalette::borderHex(), MxFunctions::UiPalette::labelHex(),
				 MxFunctions::UiPalette::surfaceHex(),
				 QString::number(MxFunctions::UiMetrics::bodyFontSize())));
		_rangeFrame->setFixedHeight(MxFunctions::UiMetrics::bodyLineHeight()
			+ MxFunctions::UiMetrics::baseG() + 1);	//R4-A：折叠 25px（原型规则 65 calc）
		QVBoxLayout* frameLay = new QVBoxLayout(_rangeFrame);
		frameLay->setContentsMargins(0, 0, 0, 0);
		_rangeBoxInner = new QWidget(_rangeFrame);
		QFormLayout* rangeForm = new QFormLayout(_rangeBoxInner);
		rangeForm->setContentsMargins(0, 2, 0, 0);
		frameLay->addWidget(_rangeBoxInner);
		rangeForm->setHorizontalSpacing(8);
		rangeForm->setVerticalSpacing(4);
		//R4-A：框内网格=原型 frame padding 5 内缩后与外层字段同线（文字 x=13/控件 x=105
		//右缘 267），内层标签用 leftInset=0 变体（外层 6 容器边距由框自身承担）
		_editRangeStart = new QLineEdit(numText(0.0), _rangeBoxInner);
		_editRangeStart->setStyleSheet(editStyle());
		_editRangeStart->setMinimumHeight(ctlH);
		_editRangeStart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		_errRangeStart = makeErrorSlot();
		QWidget* startCell = new QWidget(_rangeBoxInner);
		QVBoxLayout* startCellLay = new QVBoxLayout(startCell);
		startCellLay->setContentsMargins(0, 0, 0, 0);
		startCellLay->setSpacing(0);
		startCellLay->addWidget(_editRangeStart);
		startCellLay->addWidget(_errRangeStart);
		rangeForm->addRow(makeFieldLabel(tr("起始时间"), 0), startCell);
		_editRangeEnd = new QLineEdit(numText(1.0), _rangeBoxInner);
		_editRangeEnd->setStyleSheet(editStyle());
		_editRangeEnd->setMinimumHeight(ctlH);
		_editRangeEnd->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		_errRangeEnd = makeErrorSlot();
		QWidget* endCell = new QWidget(_rangeBoxInner);
		QVBoxLayout* endCellLay = new QVBoxLayout(endCell);
		endCellLay->setContentsMargins(0, 0, 0, 0);
		endCellLay->setSpacing(0);
		endCellLay->addWidget(_editRangeEnd);
		endCellLay->addWidget(_errRangeEnd);
		rangeForm->addRow(makeFieldLabel(tr("结束时间"), 0), endCell);
		_rangeBoxInner->setVisible(false);	//默认关=字段隐藏回收高度（标题行=groupbox 标题保留）
		form->addRow(_rangeFrame);

		//6. 播放速度（滑条 2~20 帧/秒步进 2 默认 10，D-04；播放中调整即时生效；D-09 tooltip）
		QWidget* fpsRow = new QWidget(_bodyHost);
		QHBoxLayout* fpsLay = new QHBoxLayout(fpsRow);
		fpsLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6
		fpsLay->setSpacing(6);	//R4-A：滑条与读数隙=--std-gap 6（原型规则 67 computed）
		_sliderFps = new QSlider(Qt::Horizontal, fpsRow);
		_sliderFps->setRange(2, 20);
		_sliderFps->setValue(10);
		_sliderFps->setSingleStep(2);
		_sliderFps->setPageStep(2);
		_sliderFps->setMinimumWidth(80);	//R4-A：原型 #animationSpeed flex:1+min-width:0（宽度=列宽-读数自然宽-隙 6≈94，实测）；旧 min 110 会挤压读数列
		_sliderFps->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		_sliderFps->setToolTip(tr("左慢右快，2～20帧/秒"));
		_lblFpsReadout = new QLabel(tr("%1 帧/秒").arg(10), fpsRow);
		fpsLay->addWidget(_sliderFps, 1);
		fpsLay->addWidget(_lblFpsReadout);
		form->addRow(makeFieldLabel(tr("播放速度")), fpsRow);

		//7. 模型构形（两选分段连体单选组，默认变形后 D-06；未变形=FR-13 真接入档，
		//经控件条 shapeChanged→面板写 setDeformationScale）
		QWidget* shapeRow = new QWidget(_bodyHost);
		QHBoxLayout* shapeLay = new QHBoxLayout(shapeRow);
		shapeLay->setContentsMargins(0, 0, kCellRightInset, 0);	//R4-A：控件行右内距 6
		shapeLay->setSpacing(0);
		_rbUndeformed = new QRadioButton(tr("未变形"), shapeRow);
		_rbDeformed = new QRadioButton(tr("变形后"), shapeRow);
		_rbDeformed->setChecked(true);
		shapeLay->addWidget(makeSegmentCell(_rbUndeformed, "first", ctlH));
		shapeLay->addWidget(makeSegmentCell(_rbDeformed, "last", ctlH));
		form->addRow(makeFieldLabel(tr("模型构形")), shapeRow);

		//8. 时间显示倍率（默认 1；FR-14 真接入档——仅缩放时间读数；D-14 输入框填满控件列）+字段下方错误槽
		_editTimeScale = new QLineEdit(numText(1.0), _bodyHost);
		_editTimeScale->setStyleSheet(editStyle());
		_editTimeScale->setMinimumHeight(ctlH);
		_editTimeScale->setMaximumHeight(ctlH);	//R4-A：行盒钳 26——VBox 直挂 QLineEdit 时行 hint 实测 28（步长行经 stepRow 包裹=26），钳后弹窗总高 293→291=原型；错误态增高由错误槽承担
		_editTimeScale->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		_editTimeScale->setToolTip(tr("仅缩放时间读数，不改变播放速度或原始时间范围"));
		_errTimeScale = makeErrorSlot();
		QWidget* scaleCell = new QWidget(_bodyHost);
		QVBoxLayout* scaleCellLay = new QVBoxLayout(scaleCell);
		scaleCellLay->setContentsMargins(0, 0, 0, 0);
		scaleCellLay->setSpacing(0);
		scaleCellLay->addWidget(_editTimeScale);
		scaleCellLay->addWidget(_errTimeScale);
		form->addRow(makeFieldLabel(tr("时间显示倍率")), scaleCell);

		body->setWidget(_bodyHost);
		rootLay->addWidget(body, 1);
		_bodyScroll = body;

		//展开态高度收敛（首建一拍后按内容 sizeHint 定型；W2 折叠/展开再入 refreshPopupHeight）
		QTimer::singleShot(0, this, &mPostRealtimeAnimSettingsPopup::refreshPopupHeight);

		//---- ui-shot 字段驱动（TC-28）：关键控件 objectName（mUiShotController 点分调用
		//经 findChild 解析，radio/checkbox 可直接 setChecked 驱动原生信号链；弹窗级注入槽
		//见 setXxxForShot 系）
		_rbOnce->setObjectName("rbAnimOnce");
		_rbLoop->setObjectName("rbAnimLoop");
		_rbPingPong->setObjectName("rbAnimPingPong");
		_chkReverse->setObjectName("chkAnimReverse");
		_rbTime->setObjectName("rbAnimTime");
		_rbFrame->setObjectName("rbAnimFrame");
		_editStep->setObjectName("editAnimStep");
		_btnStepReset->setObjectName("btnAnimStepReset");
		_editRangeStart->setObjectName("editAnimRangeStart");
		_editRangeEnd->setObjectName("editAnimRangeEnd");
		_sliderFps->setObjectName("sliderAnimFps");
		_rbUndeformed->setObjectName("rbAnimUndeformed");
		_rbDeformed->setObjectName("rbAnimDeformed");
		_editTimeScale->setObjectName("editAnimTimeScale");

		//---- 交互接线（值即时入会话+即时校验，PF-06）----
		connect(_rbOnce, &QRadioButton::toggled, this, [this](bool on) { if (on) emit playStyleChanged(0); });
		connect(_rbLoop, &QRadioButton::toggled, this, [this](bool on) { if (on) emit playStyleChanged(1); });
		connect(_rbPingPong, &QRadioButton::toggled, this, [this](bool on) { if (on) emit playStyleChanged(2); });
		connect(_chkReverse, &QCheckBox::toggled, this, &mPostRealtimeAnimSettingsPopup::reverseChanged);
		//BUG-07：displayChanged 载体值=控件条 DisplayStyle 枚举同源（Frame=0/Time=1）——
		//曾按"0=时间 1=帧"倒置发射，切"时间"档被控件条解释为 Frame（读数档位静默不动）；
		//同类自查：playStyle（单次0/循环1/往返2）/shape
		//（未变形0/变形后1）/reverse(bool) 与控件条枚举逐项核对同序无倒置
		connect(_rbTime, &QRadioButton::toggled, this, [this](bool on) { if (on) emit displayChanged(1); });
		connect(_rbFrame, &QRadioButton::toggled, this, [this](bool on) { if (on) emit displayChanged(0); });
		connect(_editStep, &QLineEdit::textChanged, this, [this]() {
			if (validateStepSize())
				emit stepSizeChanged(_editStep->text().trimmed().toLongLong());	//D-16：long long（对齐原型 safe-integer 域）
			refreshSessionValid();
		});
		connect(_btnStepReset, &QToolButton::clicked, this, [this]() {
			//重置=瞬时动作（FR-10）：文本回 1，经 textChanged 既有链完成校验+回发
			_editStep->setText(QString::number(1));
		});
		connect(_rangeFrame, &QGroupBox::toggled, this, [this](bool on) {
			//W2 条款：勾选展开/取消勾选隐藏字段回收高度、标题行保留；再启用恢复暂存值
			_rangeBoxInner->setVisible(on);
			//R4-A：折叠定高 25/展开解固定交布局自然收敛（fieldset 嵌线几何配套）
			if (on)
			{
				_rangeFrame->setMinimumHeight(0);
				_rangeFrame->setMaximumHeight(QWIDGETSIZE_MAX);
			}
			else
				_rangeFrame->setFixedHeight(MxFunctions::UiMetrics::bodyLineHeight()
					+ MxFunctions::UiMetrics::baseG() + 1);
			if (!on)
			{
				//取消勾选=失效字段随隐藏收敛：清两处红边文案（S1 错误态只对生效字段呈现）
				applyFieldError(_editRangeStart, _errRangeStart, QString());
				applyFieldError(_editRangeEnd, _errRangeEnd, QString());
			}
			//踩坑 34：隐藏/展开改变子树 hint 后，顶层窗高度须延迟一拍破互锁收敛
			QTimer::singleShot(0, this, &mPostRealtimeAnimSettingsPopup::refreshPopupHeight);
			if (on)
				validateRangeFields();	//展开即按暂存值校验（界校验 timeAxisValid 守卫在函数内）
			refreshSessionValid();
			emit rangeChanged(on, _editRangeStart->text().toDouble(), _editRangeEnd->text().toDouble());
		});
		connect(_editRangeStart, &QLineEdit::textChanged, this, [this]() {
			const bool ok = validateRangeFields();
			refreshSessionValid();
			if (ok && _rangeFrame->isChecked())
				emit rangeChanged(true, _editRangeStart->text().toDouble(), _editRangeEnd->text().toDouble());
		});
		connect(_editRangeEnd, &QLineEdit::textChanged, this, [this]() {
			const bool ok = validateRangeFields();
			refreshSessionValid();
			if (ok && _rangeFrame->isChecked())
				emit rangeChanged(true, _editRangeStart->text().toDouble(), _editRangeEnd->text().toDouble());
		});
		connect(_sliderFps, &QSlider::valueChanged, this, &mPostRealtimeAnimSettingsPopup::snapFps);
		connect(_rbUndeformed, &QRadioButton::toggled, this, [this](bool on) { if (on) emit shapeChanged(0); });
		connect(_rbDeformed, &QRadioButton::toggled, this, [this](bool on) { if (on) emit shapeChanged(1); });
		connect(_editTimeScale, &QLineEdit::textChanged, this, [this]() {
			if (validateTimeScale())
				emit timeScaleChanged(_editTimeScale->text().trimmed().toDouble());
			refreshSessionValid();
		});
		//提交档已随 FR-18 提示机制退役（原 editingFinished→控件条提示链拆除）：
		//倍率合法值经 textChanged→timeScaleChanged 即时入会话+刷读数（FR-14 真接入）
		connect(_btnClose, &QToolButton::clicked, this, &QWidget::close);

		//创建后调一次 DWM 三件套（踩坑 30：Show 后在 showEvent 再调一次，两时机缺一不可）
		applyPopupDwmAttributes(this);
	}

	void mPostRealtimeAnimSettingsPopup::setSession(const mPostRealtimeAnimWidget::AnimSettings& s,
		bool timeAxisValid, double tFirst, double tLast, const QVector<double>& frameTimes)
	{
		_timeAxisValid = timeAxisValid;
		_tFirst = tFirst;
		_tLast = tLast;
		_frameTimes = frameTimes;
		//程序性同步不回发信号（会话权威源在控件条，此处只镜像编辑态）
		{
			const QSignalBlocker b1(_rbOnce); const QSignalBlocker b2(_rbLoop); const QSignalBlocker b3(_rbPingPong);
			const QSignalBlocker b4(_chkReverse); const QSignalBlocker b5(_rbTime); const QSignalBlocker b6(_rbFrame);
			const QSignalBlocker b7(_editStep); const QSignalBlocker b8(_rangeFrame);
			const QSignalBlocker b9(_editRangeStart); const QSignalBlocker b10(_editRangeEnd);
			const QSignalBlocker b11(_sliderFps); const QSignalBlocker b12(_rbUndeformed); const QSignalBlocker b13(_rbDeformed);
			const QSignalBlocker b14(_editTimeScale);
			_rbOnce->setChecked(s.playStyle == mPostRealtimeAnimWidget::PlayStyle::Once);
			_rbLoop->setChecked(s.playStyle == mPostRealtimeAnimWidget::PlayStyle::Loop);
			_rbPingPong->setChecked(s.playStyle == mPostRealtimeAnimWidget::PlayStyle::PingPong);
			_chkReverse->setChecked(s.reverse);
			_rbTime->setChecked(s.display == mPostRealtimeAnimWidget::DisplayStyle::Time);
			_rbFrame->setChecked(s.display == mPostRealtimeAnimWidget::DisplayStyle::Frame);
			_editStep->setText(QString::number(s.stepSize));
			_rangeFrame->setChecked(s.rangeEnabled);
			_rangeBoxInner->setVisible(s.rangeEnabled);
			//R4-A：折叠定高 25/展开解固定（setChecked 经 QSignalBlocker 屏蔽 toggled，
			//高度切换在此补一份——与 toggled 槽同款口径）
			if (s.rangeEnabled)
			{
				_rangeFrame->setMinimumHeight(0);
				_rangeFrame->setMaximumHeight(QWIDGETSIZE_MAX);
			}
			else
				_rangeFrame->setFixedHeight(MxFunctions::UiMetrics::bodyLineHeight()
					+ MxFunctions::UiMetrics::baseG() + 1);
			_editRangeStart->setText(numText(s.rangeStart));
			_editRangeEnd->setText(numText(s.rangeEnd));
			_sliderFps->setValue(s.fps);
			_rbUndeformed->setChecked(s.shape == mPostRealtimeAnimWidget::ShapeMode::Undeformed);
			_rbDeformed->setChecked(s.shape == mPostRealtimeAnimWidget::ShapeMode::Deformed);
			_editTimeScale->setText(numText(s.timeScale));
		}
		//D-19：QSignalBlocker 屏蔽 toggled→三组七段选中底属性此处补同步
		for (QRadioButton* rb : { _rbOnce, _rbLoop, _rbPingPong,
			_rbTime, _rbFrame, _rbUndeformed, _rbDeformed })
			syncSegmentChecked(rb);
		QTimer::singleShot(0, this, &mPostRealtimeAnimSettingsPopup::refreshPopupHeight);
		updateFpsReadout();
		validateStepSize();
		validateTimeScale();
		validateRangeFields();
	}

	void mPostRealtimeAnimSettingsPopup::popup()
	{
		//先定型尺寸再锚定（J-07 裁定：字面基准"上弹"+顶部空间不足翻转下弹——宿主为
		//窗口顶部工具栏上方无空间，实际观感即下弹；视口左右各留 12px，规范 §3.8 D 通用）
		adjustSize();
		const QRect anchorG = _anchor != nullptr
			? QRect(_anchor->mapToGlobal(QPoint(0, 0)), _anchor->size())
			: QRect(QCursor::pos(), QSize(0, 0));
		QScreen* scr = QGuiApplication::screenAt(anchorG.center());
		if (scr == nullptr)
			scr = QGuiApplication::primaryScreen();
		const QRect scrRect = (scr != nullptr) ? scr->availableGeometry() : QRect();
		QPoint pos;
		const QPoint above = anchorG.topLeft() - QPoint(0, height());
		if (scrRect.isValid() && above.y() >= scrRect.top() + 12)
			pos = above;	//上方有空间→按原型字面上弹（FR-06/01 §1.3 字面口径）
		else
			pos = QPoint(anchorG.left(), anchorG.bottom() + 1);	//翻转下弹（窗口顶部工具栏宿主常态）
		if (scrRect.isValid())
		{
			pos.setX(qBound(scrRect.left() + 12, pos.x(), scrRect.right() - width() - 12));
			pos.setY(qMin(pos.y(), scrRect.bottom() - height() - 12));
		}
		move(pos);
		show();
	}

	void mPostRealtimeAnimSettingsPopup::showEvent(QShowEvent* event)
	{
		//DWM 三件套第二次（创建后+Show 后各调一次，踩坑 30）+焦点入首控件（FR-16：
		//D-05 对齐原型——焦点落播放方式组"当前选中项"（settings 打开 focus checked radio））
		applyPopupDwmAttributes(this);
		QWidget::showEvent(event);
		(_rbLoop->isChecked() ? _rbLoop : _rbOnce)->setFocus();
	}

	void mPostRealtimeAnimSettingsPopup::keyPressEvent(QKeyEvent* event)
	{
		//Esc 关弹窗并回焦点到入口（FR-16）；Qt::Popup 外点关闭原生（重开值保留=会话在控件条）
		if (event->key() == Qt::Key_Escape)
		{
			close();
			if (_anchor != nullptr)
				_anchor->setFocus();
			event->accept();
			return;
		}
		QWidget::keyPressEvent(event);
	}

	bool mPostRealtimeAnimSettingsPopup::validateStepSize()
	{
		//步长须为正整数（FR-15 文案逐字；错误保留原值=不回写文本，仅红边+文案+不发值）。
		//BUG-04：放宽规范化比对——"03"/"+3" 数学上=正整数 3 接受（原型 number 输入口径）；
		//toLongLong 防超大整数溢出；D-16 对齐原型 safe-integer 域（2^53-1=9007199254740991，
		//Number.isSafeInteger 口径），越界拒
		bool ok = false;
		const long long v = _editStep->text().trimmed().toLongLong(&ok);
		const bool valid = ok && v >= 1 && v <= 9007199254740991LL;
		if (!valid)
			applyFieldError(_editStep, _errStep, tr("步长须为正整数"));
		else
			applyFieldError(_editStep, _errStep, QString());
		return valid;
	}

	bool mPostRealtimeAnimSettingsPopup::validateTimeScale()
	{
		//时间显示倍率须为有限正数（FR-15；1e308 合法=有限正数，0/负数/非数字不合法）
		bool ok = false;
		const double v = _editTimeScale->text().trimmed().toDouble(&ok);
		const bool valid = ok && qIsFinite(v) && v > 0.0;
		if (!valid)
			applyFieldError(_editTimeScale, _errTimeScale, tr("时间显示倍率须为有限正数"));
		else
			applyFieldError(_editTimeScale, _errTimeScale, QString());
		return valid;
	}

	bool mPostRealtimeAnimSettingsPopup::validateRangeFields()
	{
		//起始/结束时间（FR-15 全组文案逐字）：有限数值→帧表首末之间→结束≥起始→
		//范围内须存在动画帧；timeAxisValid=false（帧时间数据缺失，FR-11 边界）时仅
		//校验有限性，界校验/有帧校验激活（控件条侧 W2 域映射同步回退全帧表）
		bool okS = false, okE = false;
		const double s = _editRangeStart->text().trimmed().toDouble(&okS);
		const double e = _editRangeEnd->text().trimmed().toDouble(&okE);
		const bool startValid = okS && qIsFinite(s)
			&& (!_timeAxisValid || (s >= _tFirst && s <= _tLast));
		bool endValid = okE && qIsFinite(e)
			&& (!_timeAxisValid || (e >= _tFirst && e <= _tLast));

		if (!startValid)
		{
			if (!okS || !qIsFinite(s))
				applyFieldError(_editRangeStart, _errRangeStart, tr("请输入有限的起始时间"));
			else
				applyFieldError(_editRangeStart, _errRangeStart,
					tr("起始时间须在 %1～%2 之间").arg(QString::number(_tFirst, 'f', 3), QString::number(_tLast, 'f', 3)));
		}
		else
			applyFieldError(_editRangeStart, _errRangeStart, QString());

		if (!endValid)
		{
			if (!okE || !qIsFinite(e))
				applyFieldError(_editRangeEnd, _errRangeEnd, tr("请输入有限的结束时间"));
			else
				applyFieldError(_editRangeEnd, _errRangeEnd,
					tr("结束时间须在 %1～%2 之间").arg(QString::number(_tFirst, 'f', 3), QString::number(_tLast, 'f', 3)));
		}
		else if (startValid && e < s)
		{
			applyFieldError(_editRangeEnd, _errRangeEnd, tr("结束时间不能小于起始时间"));
			endValid = false;
		}
		else
			applyFieldError(_editRangeEnd, _errRangeEnd, QString());

		//范围内须存在动画帧（timeAxisValid 才可判；缺失时控件条域映射回退全帧表）
		if (startValid && endValid && _timeAxisValid && e >= s)
		{
			bool hasFrame = false;
			for (double t : _frameTimes)
			{
				if (t >= s && t <= e) { hasFrame = true; break; }
			}
			if (!hasFrame)
			{
				applyFieldError(_editRangeEnd, _errRangeEnd, tr("此范围内没有动画帧，请调整端点"));
				endValid = false;
			}
		}
		return startValid && endValid;
	}

	void mPostRealtimeAnimSettingsPopup::refreshSessionValid()
	{
		//会话整体校验态（步长/倍率/范围 W2 生效时三组全过→有效）；幂等去重：
		//仅态变化才发 sessionValidated（无效→控件条自动暂停+走带禁，FR-15 联动）
		bool valid = validateStepSize() && validateTimeScale();
		if (_rangeFrame->isChecked())
			valid = valid && validateRangeFields();
		if (valid != _sessionValid)
		{
			_sessionValid = valid;
			emit sessionValidated(valid);
		}
		QTimer::singleShot(0, this, &mPostRealtimeAnimSettingsPopup::refreshPopupHeight);	//R2-D17：错误显隐→高度收敛
	}

	void mPostRealtimeAnimSettingsPopup::applyFieldError(QLineEdit* field, QLabel* errSlot, const QString& text)
	{
		//S1：错误保留原值（不回写文本）+红边（动态属性命中 QSS，unpolish/polish 即时生效）
		//+字段下方错误文案；D-09：输入框 title 同步错误文案（原型 error() 的 input.title
		//语义），清错后恢复原 tooltip（倍率=帮助文本，其余为空）
		if (field != nullptr)
		{
			field->setProperty("animFieldError", !text.isEmpty());
			field->style()->unpolish(field);
			field->style()->polish(field);
			field->setToolTip(text);
		}
		if (errSlot != nullptr)
		{
			if (errSlot->text() != text)
				errSlot->setText(text);
			errSlot->setVisible(!text.isEmpty());	//R2-D17：无错隐藏回收高度（规则 66）
		}
	}

	void mPostRealtimeAnimSettingsPopup::snapFps(int value)
	{
		//2..20 步进 2 吸附（D-04）：键盘单击步长 2 原生偶数，鼠标拖动任意值向下取偶
		const int v = qBound(2, value - (value % 2), 20);
		if (v != _sliderFps->value())
		{
			const QSignalBlocker b(_sliderFps);
			_sliderFps->setValue(v);
		}
		updateFpsReadout();
		emit fpsChanged(v);
	}

	void mPostRealtimeAnimSettingsPopup::updateFpsReadout()
	{
		if (_lblFpsReadout != nullptr)
			_lblFpsReadout->setText(tr("%1 帧/秒").arg(_sliderFps->value()));
	}

	void mPostRealtimeAnimSettingsPopup::refreshPopupHeight()
	{
		//W2 折叠/展开高度收敛（FR-06）×踩坑 34 三层互锁破除：①Qt 顶层窗不随子控件
		//sizeHint 自动缩放；②QLayout::invalidate 不清 item 缓存，子树 hint 新鲜而顶层
		//adjustSize 仍读旧值；③旧 hint 已写进窗口 minimumSize 钳死 resize——解法=
		//重建根布局 item（removeWidget+insertWidget 强制按最新子树重算）+解除高度钳制
		//+adjustSize 按内容收敛（超高内部滚动上限 480px 兜底，FR-06）。
		//BUG-07 轮全状态检查 R1-F1：重建目标必须是根布局里的 _bodyScroll（QScrollArea），
		//曾误用其内层 _bodyHost——indexOf 恒 -1 跳过重建、旧 hint 钳高不破，W2 展开后
		//弹窗高度不增长（内容被 282px 视口裁切出滚动条）
		QVBoxLayout* lay = qobject_cast<QVBoxLayout*>(layout());
		if (lay == nullptr || _bodyScroll == nullptr)
			return;
		const int idx = lay->indexOf(_bodyScroll);
		if (idx >= 0)
		{
			lay->removeWidget(_bodyScroll);
			lay->insertWidget(idx, _bodyScroll, 1);
		}
		setMinimumHeight(0);
		setMaximumHeight(QWIDGETSIZE_MAX);
		//显式定高（BUG-07 轮 R1-F1 实证）：QAbstractScrollArea 的 sizeHint 是缓存值
		//（不随内容 hint 增长刷新——内容变高时 viewport 未变、缓存死锁在旧值），依赖
		//sizeHint/adjustSize 收敛无效；改用内层内容 _bodyHost 的 sizeHint（每次查询
		//新鲜）+ 头部/边距常量直接定高（28px 头+根边距 2+边框 2≈30）
		setFixedHeight(qMin(_bodyHost->sizeHint().height() + 30, kMaxPopupBodyHeight));
	}

	void mPostRealtimeAnimSettingsPopup::retranslateUi()
	{
		//语言切换全量重译（F-2：updateTranslator → 控件条 → 本函数；objectName 查找名
		//不受语言影响）。控件创建时已带初始文本，此处只刷新（踩坑 16 尾）
		_lblTitle->setText(tr("动画设置"));
		_btnClose->setIcon(headerCloseIcon(MxFunctions::UiPalette::text()));	//R4-E：主题色重取（重译链顺带刷新图形）
		_btnClose->setToolTip(tr("关闭动画设置"));
		_rbOnce->setText(tr("单次"));
		_rbLoop->setText(tr("循环"));
		_rbPingPong->setText(tr("往返"));
		_chkReverse->setText(tr("反向播放"));
		_rbTime->setText(tr("时间"));
		_rbFrame->setText(tr("帧"));
		_btnStepReset->setToolTip(tr("重置步长为 1"));	//D-10：重置=图标钮（无文字）
		_rangeFrame->setTitle(tr("指定范围"));
		updateFpsReadout();
		_rbUndeformed->setText(tr("未变形"));
		_rbDeformed->setText(tr("变形后"));
		//错误文案重译：触发一轮重校验（无效态文案随语言刷新）
		validateStepSize();
		validateTimeScale();
		if (_rangeFrame->isChecked())
			validateRangeFields();
	}

	//---- ui-shot 字段驱动注入槽（SKILL §40 契约，TC-28 补）----
	//实现口径：一律走控件原生信号链（setChecked→toggled→changed 信号→控件条 applyXxx
	//应用），与用户点击完全同路径——注入即真实行为；仅"已是目标态"时零动作防无谓信号

	void mPostRealtimeAnimSettingsPopup::setPlayStyleForShot(int style)
	{
		if (style == 0 && !_rbOnce->isChecked())
			_rbOnce->setChecked(true);
		else if (style == 1 && !_rbLoop->isChecked())
			_rbLoop->setChecked(true);
		else if (style == 2 && !_rbPingPong->isChecked())
			_rbPingPong->setChecked(true);
	}

	void mPostRealtimeAnimSettingsPopup::setDisplayStyleForShot(int style)
	{
		//TC-28 主用例：切"时间"档（0）→ _rbTime checked → displayChanged(1=Time 枚举值)
		//→ 控件条 applyDisplayStyle（读数换挡走真实链，BUG-07 对齐后）
		if (style == 0 && !_rbTime->isChecked())
			_rbTime->setChecked(true);
		else if (style == 1 && !_rbFrame->isChecked())
			_rbFrame->setChecked(true);
	}

	void mPostRealtimeAnimSettingsPopup::setShapeForShot(int mode)
	{
		if (mode == 0 && !_rbUndeformed->isChecked())
			_rbUndeformed->setChecked(true);
		else if (mode == 1 && !_rbDeformed->isChecked())
			_rbDeformed->setChecked(true);
	}

	void mPostRealtimeAnimSettingsPopup::setStepSizeForShot(int step)
	{
		//文本注入经 textChanged 既有链（校验+stepSizeChanged 回发）；非法值（≤0）不钳位、
		//直达 S1 错误态（T10 校验错误用例驱动）
		_editStep->setText(QString::number(step));
	}

	void mPostRealtimeAnimSettingsPopup::setTimeScaleForShot(int value)
	{
		//倍率文本注入（TC-28 同构）：经 textChanged 既有链（校验+timeScaleChanged 回发+
		//控件条读数刷新）；非法值（≤0）不钳位、直达 S1 错误态
		_editTimeScale->setText(QString::number(value));
	}

	void mPostRealtimeAnimSettingsPopup::setRangeForShot(int startCenti, int endCenti)
	{
		//W2 勾选+端点文本注入（invokeChildSlot 仅 bool/int 参，端点以百分比×100 承载
		//=归一化域两位小数，D-22 后弹窗端点本就是 0~1 归一化值）：先勾选激活框
		//（setChecked→toggled→展开+rangeChanged 既有链），再注入端点文本
		//（textChanged→validateRangeFields+rangeChanged 回发→控件条域映射），全链与用户
		//输入同路径
		if (!_rangeFrame->isChecked())
			_rangeFrame->setChecked(true);
		_editRangeStart->setText(QString::number(startCenti / 100.0, 'f', 2));
		_editRangeEnd->setText(QString::number(endCenti / 100.0, 'f', 2));
	}

	void mPostRealtimeAnimSettingsPopup::setReverseForShot(bool on)
	{
		if (_chkReverse->isChecked() != on)
			_chkReverse->setChecked(on);
	}

	//R3 盲区闭合：synthetic QEvent::Enter 悬停态取证（QSS :hover 由 WA_UnderMouse 驱动，
	//Enter 置位后 update 现场重绘；不禁用控件不发——原型 :hover:not(:disabled) 语义）
	void mPostRealtimeAnimSettingsPopup::sendHoverForShot(int target)
	{
		QWidget* w = nullptr;
		switch (target)
		{
			case 0: w = _rbOnce; break;
			case 1: w = _rbLoop; break;
			case 2: w = _rbPingPong; break;
			case 3: w = _chkReverse; break;
			case 4: w = _rbTime; break;
			case 5: w = _rbFrame; break;
			case 6: w = _btnStepReset; break;
			case 7: w = _rbUndeformed; break;
			case 8: w = _rbDeformed; break;
			case 9: w = _editTimeScale; break;
			default: return;
		}
		if (w == nullptr || !w->isEnabled())
			return;
		//D-19：段视觉（悬停底）在段容器上、radio 鼠标透明——分段 radio 的悬停取证
		//目标改落其所在段容器（parentWidget 带 animRole 属性者）；反向播放等非分段
		//控件目标不变
		if (qobject_cast<QRadioButton*>(w) != nullptr && w->parentWidget() != nullptr
			&& !w->parentWidget()->property("animRole").isNull())
			w = w->parentWidget();
		QEvent enter(QEvent::Enter);
		QCoreApplication::sendEvent(w, &enter);
		w->update();
	}

	//R3 盲区闭合：synthetic 方向键注入焦点控件（单选组 ←→↑↓/Home/End 原生导航），键盘
	//导航后选中态迁移+连体段视觉取证（press+release 成对，与真实按键同路径）
	void mPostRealtimeAnimSettingsPopup::sendKeyForShot(int key)
	{
		QWidget* fw = focusWidget();
		if (fw == nullptr)
			return;
		const Qt::Key k = (key == 0) ? Qt::Key_Left
			: (key == 1) ? Qt::Key_Right
			: (key == 2) ? Qt::Key_Up
			: (key == 3) ? Qt::Key_Down
			: (key == 4) ? Qt::Key_Home
			: Qt::Key_End;
		QKeyEvent press(QEvent::KeyPress, k, Qt::NoModifier);
		QKeyEvent release(QEvent::KeyRelease, k, Qt::NoModifier);
		QCoreApplication::sendEvent(fw, &press);
		QCoreApplication::sendEvent(fw, &release);
	}
}
