#include "mPostRealtimeAnimWidget.h"
#include "mPostRealtimeAnimSettingsPopup.h"

//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QAction>
#include <QCoreApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QFrame>
#include <QHBoxLayout>
#include <QMenu>
#include <QtMath>
#include <algorithm>
#include "UiPalette.h"
#include "UiMetrics.h"
#include "mUiMenu.h"
#include "mGlobalSignals.h"

//---- 原型 SVG path 图标常量（anim-tab-v13 R4：用户否决 qrc 资产复用裁定 A7——
//原型 proto_anim_script.js icons/animationTypes 里即有全部 path，直接同形渲染，
//口径=原型 svg()：viewBox 0 0 24 24、fill:none、stroke currentColor 1.5、round）----
namespace
{
	const QLatin1String kPathSettings("M4 5h16M4 12h16M4 19h16M8 3v4m8 3v4m-6 3v4");	//设置=三横线+圆点调节杆（原型 settings）
	const QLatin1String kPathFirst("M5 4v16m14-16L9 12l10 8z");		//首帧 |◀（细线媒体图标）
	const QLatin1String kPathPrevious("m16 4-10 8 10 8z");			//上一帧 ◀
	const QLatin1String kPathPlay("m8 4 12 8-12 8z");				//播放 ▶
	const QLatin1String kPathPause("M8 4v16m8-16v16");				//暂停 ⏸
	const QLatin1String kPathNext("m8 4 10 8-10 8z");				//下一帧 ▶
	const QLatin1String kPathLast("M19 4v16M5 4l10 8-10 8z");		//末帧 ▶|
	const QLatin1String kPathReset("M5 8a8 8 0 1 1-1 7M5 3v5h5");	//重置（弹窗步长钮）
	const QLatin1String kPathTransient("M3 19h18M5 16c8 0 10-2 10-12m-3 3 3-3 3 3");	//瞬态=曲线+箭头
	const QLatin1String kPathModal("M3 12c2-13 5-13 9 0s7 13 9 0");	//模态=正弦
	const QLatin1String kPathLinear("M3 19h18M12 19V4m-4 4 4-4 4 4");	//线性=轴+箭头

	//SVG path 令牌化（按命令分组数值——隐式重复参数组只作用于本命令的数值段）：
	//原型 path 全部无空格粘连写法（如"0 1 1-1 7"），按正负号切分防粘连
	struct PathCmd { QChar cmd; QVector<qreal> nums; };
	QVector<PathCmd> groupPath(const QString& d)
	{
		QVector<PathCmd> out;
		QString num;
		const auto flushNum = [&]() {
			if (!num.isEmpty() && !out.isEmpty()) { out.last().nums << num.toDouble(); num.clear(); }
		};
		for (const QChar& c : d)
		{
			if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
			{
				flushNum();
				out.append({ c, {} });
			}
			else if (c.isDigit() || c == '-' || c == '+' || c == '.')
			{
				if (c == '-' || c == '+')
					flushNum();
				num += c;
			}
			else
				flushNum();	//空白/逗号
		}
		flushNum();
		return out;
	}

	//SVG path 子集解析（MmLlHhVvCcSsAaZz+隐式重复参数）——原型 icons/animationTypes
	//仅用该子集；A（弧）走端点参数化→圆心参数化→≤90°分段三次贝塞尔逼近。
	//不支持命令整段丢弃（防御：不画半截路径）
	class PathBuilder
	{
	public:
		explicit PathBuilder(const QString& d) { parse(d); }
		QPainterPath path() const { return _path; }

	private:
		void parse(const QString& d)
		{
			const QVector<PathCmd> cmds = groupPath(d);
			for (const PathCmd& g : cmds)
			{
				char cmd0 = g.cmd.toLatin1();
				if (cmd0 == 0) { fail(); return; }
				const bool rel = (cmd0 >= 'a' && cmd0 <= 'z');
				const char cmd = rel ? char(cmd0 - 'a' + 'A') : cmd0;
				bool firstM = true;
				int ni = 0;
				const QVector<qreal>& v = g.nums;
				if (cmd == 'Z')
				{
					_path.closeSubpath();	//Z 不带参数（残留数值非法，防御忽略）
					_cur = _start; _hasCtrl = false;
					continue;
				}
				while (ni < v.size())
				{
					switch (cmd)
					{
						case 'M':
						{
							if (ni + 2 > v.size()) { fail(); return; }
							const qreal x = v[ni++], y = v[ni++];
							if (firstM)
							{
								_cur = rel ? _cur + QPointF(x, y) : QPointF(x, y);
								_start = _cur;
								_path.moveTo(_cur);
								_hasCtrl = false;
								firstM = false;
							}
							else
								lineTo(x, y, rel);	//M 后续隐式 L/（小写 m 后隐式 l）
							continue;
						}
						case 'L':
							if (ni + 2 > v.size()) { fail(); return; }
							lineTo(v[ni], v[ni + 1], rel); ni += 2; continue;
						case 'H':
							if (ni + 1 > v.size()) { fail(); return; }
							_cur.setX(rel ? _cur.x() + v[ni] : v[ni]);
							_path.lineTo(_cur); ++ni; afterLine(); continue;
						case 'V':
							if (ni + 1 > v.size()) { fail(); return; }
							_cur.setY(rel ? _cur.y() + v[ni] : v[ni]);
							_path.lineTo(_cur); ++ni; afterLine(); continue;
						case 'C':
						{
							if (ni + 6 > v.size()) { fail(); return; }
							const QPointF c1 = pt(v[ni], v[ni + 1], rel);
							const QPointF c2 = pt(v[ni + 2], v[ni + 3], rel);
							const QPointF e = pt(v[ni + 4], v[ni + 5], rel);
							ni += 6;
							_path.cubicTo(c1, c2, e);
							_cur = e; _ctrl = c2; _hasCtrl = true;
							continue;
						}
						case 'S':
						{
							if (ni + 4 > v.size()) { fail(); return; }
							QPointF c1 = _cur;
							if (_hasCtrl)
								c1 = 2 * _cur - _ctrl;	//反射上一次控制点
							const QPointF c2 = pt(v[ni], v[ni + 1], rel);
							const QPointF e = pt(v[ni + 2], v[ni + 3], rel);
							ni += 4;
							_path.cubicTo(c1, c2, e);
							_cur = e; _ctrl = c2; _hasCtrl = true;
							continue;
						}
						case 'A':
						{
							if (ni + 7 > v.size()) { fail(); return; }
							const qreal rx = qAbs(v[ni]), ry = qAbs(v[ni + 1]);
							const qreal rot = v[ni + 2];
							const bool large = v[ni + 3] != 0, sweep = v[ni + 4] != 0;
							const QPointF e = pt(v[ni + 5], v[ni + 6], rel);
							ni += 7;
							arcTo(rx, ry, rot, large, sweep, e);
							continue;
						}
						default:
							fail(); return;
					}
				}
			}
		}
		QPointF pt(qreal x, qreal y, bool rel) const
		{
			const QPointF a(x, y);
			return rel ? _cur + a : a;
		}
		void lineTo(qreal x, qreal y, bool rel)
		{
			_cur = pt(x, y, rel);
			_path.lineTo(_cur);
			afterLine();
		}
		void afterLine() { _hasCtrl = false; }
		void fail() { _path = QPainterPath(); }	//防御：不支持的命令不画半截
		//端点→圆心参数化 + ≤90° 分段三次贝塞尔逼近（W3C SVG 弧标准算法）
		void arcTo(qreal rx, qreal ry, qreal deg, bool large, bool sweep, const QPointF& end)
		{
			if (rx <= 0 || ry <= 0) { lineTo(end.x(), end.y(), false); return; }
			const QPointF p0 = _cur;
			const qreal phi = qDegreesToRadians(deg);
			const qreal cp = qCos(phi), sp = qSin(phi);
			const qreal dx = (p0.x() - end.x()) / 2, dy = (p0.y() - end.y()) / 2;
			const qreal x1 = cp * dx + sp * dy, y1 = -sp * dx + cp * dy;
			const qreal lam = (rx * rx * ry * ry) / (rx * rx * y1 * y1 + ry * ry * x1 * x1);
			if (lam <= 0) { lineTo(end.x(), end.y(), false); return; }
			qreal co = qSqrt(qMax<qreal>(0.0, lam - 1.0));
			if (large == sweep)
				co = -co;
			const qreal cxp = co * rx * y1 / ry, cyp = -co * ry * x1 / rx;
			const qreal cx = cp * cxp - sp * cyp + (p0.x() + end.x()) / 2;
			const qreal cy = sp * cxp + cp * cyp + (p0.y() + end.y()) / 2;
			const auto ang = [](qreal ux, qreal uy) { return qAtan2(uy, ux); };
			qreal th1 = ang((x1 - cxp) / rx, (y1 - cyp) / ry);
			qreal dth = ang((-x1 - cxp) / rx, (-y1 - cyp) / ry) - th1;
			if (!sweep && dth > 0) dth -= 2 * M_PI;
			if (sweep && dth < 0) dth += 2 * M_PI;
			const int n = qCeil(qAbs(dth) / (M_PI / 2));
			const qreal dq = dth / n, t = 4.0 / 3.0 * qTan(dq / 4);
			const auto ptAt = [&](qreal th) {
				return QPointF(cx + rx * qCos(th) * cp - ry * qSin(th) * sp,
					cy + rx * qCos(th) * sp + ry * qSin(th) * cp);
			};
			const auto dAt = [&](qreal th) {
				return dq * QPointF(-rx * qSin(th) * cp - ry * qCos(th) * sp,
					-rx * qSin(th) * sp + ry * qCos(th) * cp);
			};
			for (int i = 0; i < n; ++i)
			{
				const qreal ta = th1 + i * dq, tb = ta + dq;
				_path.cubicTo(ptAt(ta) + t * dAt(ta), ptAt(tb) - t * dAt(tb), ptAt(tb));
			}
			_cur = end; _hasCtrl = false;
		}
		QPainterPath _path;
		QPointF _cur{}, _start{}, _ctrl{};
		bool _hasCtrl = false;
	};

	//按原型 svg() 口径渲染单枚图标位图：viewBox 0 0 24 24 等比缩到 px、
	//fill:none、stroke 1.5（随缩放）round cap/join
	QPixmap renderProtoIcon(const QString& pathData, const QColor& color, int px)
	{
		QPixmap pm(px, px);
		pm.fill(Qt::transparent);
		QPainter p(&pm);
		p.setRenderHint(QPainter::Antialiasing);
		const qreal k = px / 24.0;
		p.scale(k, k);
		p.setPen(QPen(color, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
		p.setBrush(Qt::NoBrush);
		p.drawPath(PathBuilder(pathData).path());
		p.end();
		return pm;
	}
}


namespace PostWindow
{
	const int mPostRealtimeAnimWidget::kParamStepCount;

	//---- 原型 SVG path 图标工厂静态实现（口径见文件头匿名段注释）----

	QPixmap mPostRealtimeAnimWidget::protoIconPixmap(const QString& pathData, const QColor& color, int px)
	{
		return renderProtoIcon(pathData, color, px);
	}

	QIcon mPostRealtimeAnimWidget::protoIcon(const QString& pathData, int px)
	{
		//三态随色：常态=text、勾选态(播放中/类型菜单选中)=selectedText、禁用=disabled
		//（原型 .vp-button aria-pressed selected 底+selectedText 与 disabled 灰的色板语义）
		QIcon ic;
		ic.addPixmap(renderProtoIcon(pathData, MxFunctions::UiPalette::text(), px), QIcon::Normal, QIcon::Off);
		ic.addPixmap(renderProtoIcon(pathData, MxFunctions::UiPalette::selectedText(), px), QIcon::Normal, QIcon::On);
		ic.addPixmap(renderProtoIcon(pathData, MxFunctions::UiPalette::selectedText(), px), QIcon::Active, QIcon::On);
		ic.addPixmap(renderProtoIcon(pathData, MxFunctions::UiPalette::disabled(), px), QIcon::Disabled);
		return ic;
	}

	//实时动画播放控件条 v1.4（QToolBar 化，anim-tab-v13 形态反转）：主窗口工具栏壳内容件
	//（壳=initAnimToolBar 预创建，面板经 animToolBarProvider 回挂；前处理隐藏/后处理常显）。
	//帧形态：QTimer 逐帧发 frameIndexChanged → 面板驱动 _comboFrame 切帧全链
	//（slot_chooseFrame→slot_showCloud→seekFrame+渐进装载兜底）。
	//线性/模态形态：插值档经 interpolationStepChanged → 面板互斥判定+静态定位路由
	//（unify-anim-playback：播放推进已合一会话播放器，本类 tick 自驱仅帧形态混合档保留）。
	//未装载帧由既有兜底语义处理（保持上一帧+frameReady 补绘），此处不做时序补丁。
	//v1.4 控件带（原型 V1.3 口径）：设置入口（懒弹窗锚点）/类型钮（瞬态/线性/模态更名，
	//D-01/D-02）/分隔线/走带五钮（首帧承担原停止钮回起点 D-03）/滑条/读数——
	//fps 下拉与循环钮退役（D-04，设置弹窗速度滑条与播放方式承接）；
	//尺寸对齐视图页控件组：按钮 min(H, H−2) 档（UiMetrics 派生，五档密度随动——04 J-09），
	//图标 qrc post 组既有资产，子控件交全局 QSS+工具栏主题链
	mPostRealtimeAnimWidget::mPostRealtimeAnimWidget(int frameCount, int curIndex, QWidget* parent)
		: QWidget(parent)
	{
		setObjectName("animRealtimeContent");	//ui-shot 工具栏状态驱动解析名（invokeChildSlot allWidgets 兜底）
		_frameCount = frameCount > 0 ? frameCount : 1;
		_frameIndex = qBound(0, curIndex, _frameCount - 1);
		_contextFrameCount = frameCount > 0 ? frameCount : 0;

		//固定高度紧凑单行（视图页控件组口径）：条高=baseH+6（标准档 32px），与 v1.3 一致
		setFixedHeight(MxFunctions::UiMetrics::baseH() + 6);

		_playTimer = new QTimer(this);
		applyFps(10);	//默认 10 帧/秒（D-04 默认口径，经会话+定时器同步）
		connect(_playTimer, &QTimer::timeout, this, &mPostRealtimeAnimWidget::slot_tick);

		QHBoxLayout* lay = new QHBoxLayout(this);
		lay->setContentsMargins(MxFunctions::UiMetrics::fieldInset(), 4,
			MxFunctions::UiMetrics::fieldInset(), 4);
		lay->setSpacing(MxFunctions::UiMetrics::baseG());

		//按钮尺寸对齐视图页帧导航行（min H×(H−2)，标准档 26×24 为 UiMetrics 派生值非固定值
		//——04 J-09：五档密度下随档；全局 QSS 的 QToolButton width/height=controlIn 被显式
		//最小值覆盖）；图形盒取规则 icon（标准 16px）
		const int kBtnW = MxFunctions::UiMetrics::baseH();
		const int kBtnH = MxFunctions::UiMetrics::baseH() - 2;
		//R4：图标档改原型 --std-icon 口径（controlIconSize，标准档 20px——壳
		//applyToolbarTheme setIconSize 同源，原 iconSize()=16 与原型图形盒不符）
		const int kIconPx = MxFunctions::UiMetrics::controlIconSize();
		const QSize kIcon = QSize(kIconPx, kIconPx);

		//1. 设置入口钮（v1.4 新增）：懒弹窗锚点；图标=R4 原型 settings path 同形渲染
		//（三横线+圆点调节杆，废弃 qrc shezhi.svg 齿轮）；checkable 仅作"弹窗开态"
		//可感知标识（FR-16）
		_btnSettings = new QToolButton(this);
		_btnSettings->setObjectName("animBarBtn");
		_btnSettings->setIcon(protoIcon(kPathSettings, kIconPx));
		_btnSettings->setIconSize(kIcon);
		_btnSettings->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnSettings->setCheckable(true);
		_btnSettings->setToolTip(tr("动画设置"));
		connect(_btnSettings, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::slot_settingsClicked);

		//2. 类型钮（v1.4）：当前类型图标+下箭头复合钮（InstantPopup 原生 menu-indicator
		//承担原型下箭头），菜单=纵向三项（图标+名称，选中项高亮 checkable）；
		//三项更名映射既有引擎链（D-01/D-02 纯更名）：瞬态=帧动画/线性=线性动画/模态=正弦动画
		_btnMode = new QToolButton(this);
		_btnMode->setObjectName("animBarBtn");
		_btnMode->setToolButtonStyle(Qt::ToolButtonIconOnly);
		_btnMode->setAutoRaise(true);
		_btnMode->setIconSize(kIcon);
		//D-08 补遗：合成图标（icon+▾）的渲染尺寸在 refreshModeMenuUi 里按位图实际尺寸重设
		_btnMode->setMinimumSize(QSize(kBtnW + 12, kBtnH));	//D-08：类型钮宽=control+body=38px（原型规则 18，容纳 icon+▾ 合成图标）
		//D-08：▾ 下箭头由 refreshModeMenuUi 合成进图标（QToolButton setArrowType 会以箭头取代图标渲染——实证）
		_btnMode->setToolTip(tr("动画类型"));
		ensureTypeMenu();	//菜单顺序=瞬态/模态/线性（原型 animationTypes 序，BUG-03）；D-08/D-18：icon-only 纵列+selected 底（R3-D18 终案，收口在 ensureTypeMenu）
		connect(_btnMode, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::showTypeMenuForShot);

		//3. 分隔线（类型组/走带组之间，原型分隔线；单内容件方案 QToolBar 原生 separator
		//需多项 addWidget 才存在，QFrame 等价承担——J-08 裁定留痕；配色入 StyleChange 链）
		_separator = new QWidget(this);
		_separator->setObjectName("animBarSeparator");
		_separator->setAttribute(Qt::WA_StyledBackground, true);
		_separator->setFixedSize(1, MxFunctions::UiMetrics::controlIconSize());	//D-13：高=icon 档（原型规则 26 height:var(--std-icon)）

		//4. 走带五钮（首帧/上一帧/播放暂停/下一帧/末帧）：R4 图标=原型 first/previous/
		//play/pause/next/last path 同形渲染（细线媒体图标，废弃 qrc diyizheng 等箭头
		//资产与蓝圆 kaishi/zanting）；播放/暂停随态切换在 setPlayingUi（selectedText
		//按压色由 protoIcon 的 On 态位图承担）
		_btnFirst = new QToolButton(this);
		_btnFirst->setObjectName("animBarBtn");
		_btnFirst->setIcon(protoIcon(kPathFirst, kIconPx));
		_btnFirst->setIconSize(kIcon);
		_btnFirst->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnFirst->setToolTip(tr("首帧"));
		_btnPlay = new QToolButton(this);
		_btnPlay->setObjectName("animBarBtn");
		_btnPlay->setIcon(protoIcon(kPathPlay, kIconPx));
		_btnPlay->setCheckable(true);
		_btnPlay->setIconSize(kIcon);
		_btnPlay->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnPlay->setToolTip(tr("播放"));
		_btnPrev = new QToolButton(this);
		_btnPrev->setObjectName("animBarBtn");
		_btnPrev->setIcon(protoIcon(kPathPrevious, kIconPx));
		_btnPrev->setIconSize(kIcon);
		_btnPrev->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnPrev->setToolTip(tr("上一帧"));
		_btnNext = new QToolButton(this);
		_btnNext->setObjectName("animBarBtn");
		_btnNext->setIcon(protoIcon(kPathNext, kIconPx));
		_btnNext->setIconSize(kIcon);
		_btnNext->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnNext->setToolTip(tr("下一帧"));
		_btnLast = new QToolButton(this);
		_btnLast->setObjectName("animBarBtn");
		_btnLast->setIcon(protoIcon(kPathLast, kIconPx));
		_btnLast->setIconSize(kIcon);
		_btnLast->setMinimumSize(QSize(kBtnW, kBtnH));
		_btnLast->setToolTip(tr("末帧"));

		//5. 进度滑条+读数（域随形态：帧 0..N-1 / 档位 1..20；松手提交 P5-1 保留；
		//R4 渲染真值口径：轨道 6px、已填充段/拇指=primary、拇指 15px——见 applySliderStyle）
		_slider = new QSlider(Qt::Horizontal, this);
		_slider->setObjectName("animProgress");
		_slider->setRange(0, _frameCount - 1);
		_slider->setValue(_frameIndex);
		//D-20：滑条改弹性（原型 #animationProgress flex:1 0 calc(2*--std-control) 口径）——
		//最小 160、水平 Expanding：工具栏被拉宽时富余空间由滑条伸长吸收（原 setFixedWidth(160)
		//使富余落到读数前成为空白带）
		_slider->setMinimumWidth(160);
		_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		_slider->setToolTip(tr("帧进度"));
		applySliderStyle();	//工具栏壳内滑条槽/手柄+分隔线配色本地补发（UiPalette 令牌）

		//D-20+D-21：读数定宽预留+默认左对齐（原型 <output> auto 宽观感——其条总宽恒定、
		//播放中文本变化不挤动任何控件）。D-20 的纯 hug 内容使读数 QLabel sizeHint 随每次
		//读数文本变化，QMainWindow 工具栏行随之重分配空间、挤动行内前面所有控件
		//（"9.000 / 10.000"→"10.000 / 10.000"复现路径）。D-21 改按当前显示格式最坏情况
		//字符串预留定宽（updateReadoutReservedWidth，经 updateReadout 在状态变化事件
		//收口重算），AlignLeft 文本只短不长；播放 tick 路径宽度不变零几何变化
		//（setFrameLabelText 同值跳过节流照常）
		_frameLabel = new QLabel(this);
		updateReadout();

		lay->addWidget(_btnSettings);
		lay->addWidget(_btnMode);
		lay->addWidget(_separator);
		lay->addWidget(_btnFirst);
		lay->addWidget(_btnPrev);
		lay->addWidget(_btnPlay);
		lay->addWidget(_btnNext);
		lay->addWidget(_btnLast);
		lay->addWidget(_slider);
		lay->addWidget(_frameLabel);
		//D-20：去掉尾部 stretch——布局 stretch 因子优先于 Expanding 尺寸策略，保留会让
		//富余宽度全被空 stretch 吃掉、滑条永不伸长；改由滑条吸收（原型 flex:1 口径，
		//读数即贴滑条尾缘）

		//走带接线（首帧承担原停止钮回起点语义 D-03；步进先停播=原型口径，经 pausePlayback
		//发 playbackToggled(false)→面板停机点补刷链照常走）
		connect(_btnPlay, &QToolButton::toggled, this, &mPostRealtimeAnimWidget::slot_playToggled);
		connect(_btnFirst, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::slot_firstClicked);
		connect(_btnPrev, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::slot_prevClicked);
		connect(_btnNext, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::slot_nextClicked);
		connect(_btnLast, &QToolButton::clicked, this, &mPostRealtimeAnimWidget::slot_lastClicked);
		connect(_slider, &QSlider::sliderMoved, this, &mPostRealtimeAnimWidget::slot_sliderMoved);
		connect(_slider, &QSlider::sliderPressed, this, [this]() {
			if (isPlaying()) _playTimer->stop();	//混合档自驱表停走（现状保留）
			//【阻塞-2·unify-anim-playback】引擎档推进停役：按下即挂起会话推进（宿主
			//会话 pause；不经 pausePlayback——不清 _enginePlayback、不复位按钮），拖动
			//本地预览不被逐 tick 镜像抢写（02 §3.1"按下即暂停推进预览"）
			if (_enginePlayback)
				emit engineAdvanceSuspendRequested(true);
		});
		connect(_slider, &QSlider::sliderReleased, this, [this]() {
			//P5-1 审计修复：松手提交（值变化才发一次全链；未拖动不重复触发）。
			//播放中拖动=拖动预览+松手一次提交+续播（tick 自提交帧推进）；
			//录制中拖动的逐条"已暂停"提示随逐值触发消除（P6-4 自然消解）
			//【阻塞-2·unify-anim-playback】引擎档松手先恢复推进：帧模式后续 seek 续播时
			//镜像通道以 Playing 判停机，须先回 Playing；参数化提交路由首步 pause 对
			//Playing 幂等重暂停后定位+续推，结果等价。无值变化时恢复 play=原位松手续播
			//（否则停在 Paused 而按钮呈播放态）
			if (_enginePlayback && _btnPlay->isChecked())
				emit engineAdvanceSuspendRequested(false);
			if (_mode == AnimMode::Frame)
			{
				if (_slider->value() != _frameIndex)
					applyIndex(_slider->value(), true);
			}
			else
			{
				if (_slider->value() != _step)
					applyStep(_slider->value(), true);
			}
			//【F-2 修复·unify-anim-playback】引擎门：引擎态续推由会话承担（按下侧挂起/
			//松手恢复+宿主定位路由），自驱表仅混合档重启——否则豁免后引擎档松手重启
			//自驱表=GUI 定时器与会话定时器双驱动
			if (_btnPlay->isChecked() && !_enginePlayback)
				_playTimer->start();
		});
		applyEnabledState();	//按初始上下文（构造参数可能=单帧/0）落禁用矩阵

		//语言切换（F-2/J-06：权威通道=MxFunctions::mGlobalSignals::updateTranslator，
		//mPostNotesPanel.cpp:197 先例同款）；设置弹窗懒创建未创建时跳过（§9 契约）
		connect(MxFunctions::mGlobalSignals::getInstance(),
			&MxFunctions::mGlobalSignals::updateTranslator,
			this, &mPostRealtimeAnimWidget::retranslateUi);
	}

	void mPostRealtimeAnimWidget::changeEvent(QEvent* event)
	{
		//v1.3 主题懒补发（ThemeManager 同款口径）：切明暗/护眼主题或切尺寸档时全局 QSS
		//重发会向所有控件投递 StyleChange，此时现场重发滑条+分隔线配色（04 J-08②：
		//分隔线不随主题会冻结）；其余子控件由全局 QSS+工具栏主题链直接覆盖
		if (event->type() == QEvent::StyleChange)
			applySliderStyle();
		QWidget::changeEvent(event);
	}

	bool mPostRealtimeAnimWidget::eventFilter(QObject* watched, QEvent* event)
	{
		//v1.4：工具栏壳被拖动/重停/拖出悬浮（拖拽交互对壳连续发 Move）→ 关闭设置弹窗
		//（原型：拖动随手柄关闭；02 §7 风险 1 定案）。壳由面板挂载时 installEventFilter
		if (watched == parentWidget() && event->type() == QEvent::Move
			&& _settingsPopup != nullptr && _settingsPopup->isVisible())
			closeSettingsPopup();
		return QWidget::eventFilter(watched, event);
	}

	void mPostRealtimeAnimWidget::slot_playToggled(bool playing)
	{
		if (playing)
		{
			//禁用域防御（按钮已在矩阵禁用；程序性 toggled 路径兜底回退，不发信号）
			if (_contextFrameCount == 0 || !_sessionValid
				|| (_mode == AnimMode::Frame && _contextFrameCount == 1))
			{
				_btnPlay->blockSignals(true);
				_btnPlay->setChecked(false);
				_btnPlay->blockSignals(false);
				return;
			}
			//与 v1.3 时序一致：先启表再发 playbackToggled(true)（面板正向闸：录制拒启/
			//预生成反向→pausePlayback 停表+复位，既有链）。尾端跳对端（FR-08）必须在
			//放行确认之后执行——若先跳再发，录制期 applyPlayStart 的 frameIndexChanged
			//会先落帧闸产生一条冗余提示（时序审计 2026-11-11）
			_playTimer->start();
			setPlayingUi(true);
			emit playbackToggled(true);
			if (!_playTimer->isActive())
				return;		//面板拒启/反向闸已停表：尾端跳转不执行
			applyPlayStart();	//FR-08：位于方向尾端→先跳对端起点再播（走既有帧变化链）
		}
		else
		{
			_playTimer->stop();
			_enginePlayback = false;	//引擎合一：暂停路径同步清引擎态
		}
		setPlayingUi(playing);
		emit playbackToggled(playing);
	}

	void mPostRealtimeAnimWidget::slot_firstClicked()
	{
		pausePlayback();	//停播（FR-04：首帧钮=停播+跳域起点，与原停止钮 stop-to-start 一致）
		jumpToEdge(true);
	}

	void mPostRealtimeAnimWidget::slot_lastClicked()
	{
		pausePlayback();
		jumpToEdge(false);
	}

	void mPostRealtimeAnimWidget::slot_prevClicked()
	{
		pausePlayback();	//先停播（原型 PF-02 口径）；stepBy 越界钳位停（BUG-02，不改方向）
		stepBy(-1);
	}

	void mPostRealtimeAnimWidget::slot_nextClicked()
	{
		pausePlayback();
		stepBy(1);
	}

	void mPostRealtimeAnimWidget::slot_sliderMoved(int value)
	{
		//P5-1 审计修复保留：拖动仅本地预览（滑条原生位移+标签跟随），不再逐值触发切帧
		//全链；提交统一收口 sliderReleased。预览文案随显示方式（帧档/时间档）且与读数
		//同源换算；分母=归一化域末恒 1.000×倍率（D-22；末帧<=0/缺失回退帧序文案）
		if (_mode == AnimMode::Frame)
		{
			const FrameDomain dom = frameDomain();
			const int idx = qBound(dom.valid ? dom.lo : 0, value, dom.valid ? dom.hi : _frameCount - 1);
			const int hiIdx = dom.valid ? dom.hi : qMax(0, _frameCount - 1);
			//D-22 归一化显示域：预览口径=读数同源（分子=当前帧归一化×倍率、分母=1.000×倍率；
			//末帧<=0/缺失回退帧序文案）
			const double tEnd = timeAxisEnd();
			if (_session.display == DisplayStyle::Time && _timeAxisValid && tEnd > 0.0)
				setFrameLabelText(QString("%1 / %2")
					.arg(formatAnimTime(normalizedTime(frameTimeAt(idx))),
						 formatAnimTime(normalizedTime(tEnd))));
			else
				setFrameLabelText(tr("帧 %1 / %2").arg(idx + 1).arg(hiIdx + 1));
		}
		else
			setFrameLabelText(tr("档位 %1 / %2").arg(qBound(1, value, kParamStepCount)).arg(kParamStepCount));
	}

	void mPostRealtimeAnimWidget::slot_settingsClicked()
	{
		//PF-06 弹窗开合：再点入口/外点/Esc 关闭且保留已输入值（值在会话不在控件）
		if (_settingsPopup != nullptr && _settingsPopup->isVisible())
		{
			closeSettingsPopup();
			return;
		}
		ensureSettingsPopup();
		if (_settingsPopup != nullptr)
		{
			//D-22 时间域归一化：弹窗 W2 界值/校验界/文案/有帧扫描全在归一化域工作——
			//首末界与帧表在此统一÷末帧时间（tEnd<=0/缺失原样下发，timeAxisValid=false
			//时弹窗只校验有限性，界值不被消费）
			const double tEnd = timeAxisEnd();
			const bool normOk = tEnd > 0.0;
			const double tFirst = normOk ? frameTimeAt(0) / tEnd : frameTimeAt(0);
			const double tLast = normOk ? tEnd / tEnd
				: (_frameTimes.isEmpty() ? 1.0 : _frameTimes.last());
			QVector<double> normTimes;
			if (normOk)
			{
				normTimes.reserve(_frameTimes.size());
				for (double t : _frameTimes)
					normTimes.append(t / tEnd);
			}
			_settingsPopup->setSession(_session, _timeAxisValid,
				tFirst, tLast, normOk ? normTimes : _frameTimes);
			_settingsPopup->popup();
			_btnSettings->setChecked(true);	//入口展开态可感知（FR-16）
		}
	}

	void mPostRealtimeAnimWidget::slot_tick()
	{
		if (_contextFrameCount == 0)
		{
			//防御：帧表清空期间（无结果态）tick 不推进
			quietStop();
			return;
		}
		if (_mode == AnimMode::Frame && !frameDomain().valid)
		{
			//防御：W2 范围域映射失败（弹窗校验态未及回报的窗口期）tick 不推进
			quietStop();
			return;
		}
		advance(_direction);
	}

	void mPostRealtimeAnimWidget::ensureTypeMenu()
	{
		//R3-D18（终案）：类型菜单=自绘 QWidget 弹层（Qt::Popup，36px 图标列，原型规则
		//21-25 直达）——QMenu 的 ::item:checked 伪态不受 QSS 支持（选中项回退原生深色
		//高亮，截图实证），mUiMenu::adopt 又会清 QSS 并按"裸对勾"设计语言自绘，均与原型
		//type-menu（selected 底+无指示列）不符；QToolButton 三枚 checkable+手动互斥，
		//clicked→applyModeSwitch 真实链。懒创建：弹层被 uishot"用后即焚"deleteLater 后
		//经 QPointer 判空重建（tooltip 按当前语言重挂）
		if (_menuMode != nullptr)
			return;
		const int ctlH = MxFunctions::UiMetrics::baseH();
		const int iconPx = MxFunctions::UiMetrics::controlIconSize();
		const QSize iconSz(iconPx, iconPx);
		auto* menu = new QWidget(_btnMode, Qt::Popup | Qt::FramelessWindowHint);
		menu->setObjectName("animTypeMenu");
		menu->setAttribute(Qt::WA_StyledBackground, true);
		//R4 复核对位原型规则 21-25 computed（protoaudit4：菜单 36×88）：圆角=--std-radius
		//4px（旧 2px 系误读 --ui-radius）、内距=choice-gap 4、项间距 0、项 26×26（规则 22
		//flex/width/height=control）、hover=hover 底、选中=selected 底+selectedText
		//（无描边——规则 24 仅 background/color，焦点圈=focus outline 仅键盘态）
		menu->setStyleSheet(QStringLiteral(
			"QWidget#animTypeMenu{background:%1;border:1px solid %2;border-radius:4px;}"
			"QToolButton#animTypeItem{background:transparent;border:none;border-radius:4px;padding:0px;}"
			"QToolButton#animTypeItem:hover{background:%3;}"
			"QToolButton#animTypeItem:checked{background:%4;}")
			.arg(MxFunctions::UiPalette::surfaceHex(), MxFunctions::UiPalette::borderHex(),
				 MxFunctions::UiPalette::hoverHex(), MxFunctions::UiPalette::selectedHex()));
		QVBoxLayout* menuLay = new QVBoxLayout(menu);
		//R4 实证：普通 QWidget（WA_StyledBackground）的 QSS padding 不参与布局尺寸——
		//内距改由布局边距承担（边线 1+内距 4=5，菜单 36×88=原型实测；QSS 只留底/边/
		//圆角）
		menuLay->setContentsMargins(5, 5, 5, 5);
		menuLay->setSpacing(0);	//R4：原型规则 21 gap:0（纵列三项紧贴）
		auto makeItem = [&](const QString& pathData) {
			auto* btn = new QToolButton(menu);
			btn->setObjectName("animTypeItem");
			btn->setIcon(protoIcon(pathData, iconPx));	//R4：原型 path 同形（On 态=selectedText 随勾选）
			btn->setIconSize(iconSz);
			btn->setCheckable(true);
			btn->setAutoRaise(true);
			btn->setFixedSize(QSize(ctlH, ctlH));	//R4：26×26（原型规则 22，旧 ctlH+2=28 与 36×88 菜单实测不符）
			btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			menuLay->addWidget(btn);
			return btn;
		};
		_btnTypeFrame = makeItem(kPathTransient);	//R4：图标=原型 animationTypes path（瞬态曲线），废弃 qrc donghua.png
		_btnTypeFrame->setToolTip(tr("瞬态动画"));
		_btnTypeSine = makeItem(kPathModal);		//R4：模态=原型 modal 正弦 path，废弃 qrc modalContributionAmount.png
		_btnTypeSine->setToolTip(tr("模态动画：当前帧变形正弦插值（原正弦动画）"));
		_btnTypeLinear = makeItem(kPathLinear);	//R4：线性=原型 linear 线性箭头 path，废弃 qrc xianxingdiejia.png
		_btnTypeLinear->setToolTip(tr("线性动画：当前帧变形线性插值"));
		connect(_btnTypeFrame, &QToolButton::clicked, this, [this]() { applyModeSwitch(static_cast<int>(AnimMode::Frame)); if (_menuMode != nullptr) _menuMode->close(); });
		connect(_btnTypeSine, &QToolButton::clicked, this, [this]() { applyModeSwitch(static_cast<int>(AnimMode::Sine)); if (_menuMode != nullptr) _menuMode->close(); });
		connect(_btnTypeLinear, &QToolButton::clicked, this, [this]() { applyModeSwitch(static_cast<int>(AnimMode::Linear)); if (_menuMode != nullptr) _menuMode->close(); });
		_menuMode = menu;
		refreshModeMenuUi();
	}

	void mPostRealtimeAnimWidget::showTypeMenuForShot()
	{
		//R3 盲区闭合（T12）：类型菜单非模态弹出——QWidget::popup 不进事件循环、不阻塞
		//uishot 后续 grab；锚定类型钮下方（原型 positionTypes 同语义，边缘翻转由
		//QMenu 原生承担）
		ensureTypeMenu();
		if (_menuMode == nullptr || _btnMode == nullptr)
			return;
		_menuMode->move(_btnMode->mapToGlobal(QPoint(0, _btnMode->height())));
		_menuMode->show();	//Qt::Popup 子类无 QMenu::popup——QWidget 用 move+show 定位弹出
	}

	void mPostRealtimeAnimWidget::refreshTypeMenuForShot()
	{
		ensureTypeMenu();	//uishot dump/截图前确保菜单已建（可对 dump 目标取证结构）
	}

	void mPostRealtimeAnimWidget::sendHoverForShot(int target)
	{
		//R3 盲区闭合：synthetic QEvent::Enter 悬停态取证（QSS :hover 由 WA_UnderMouse
		//驱动，Enter 事件置位后 update 现场重绘）——0=设置 1=类型 2=播放 3=首帧 4=末帧
		QWidget* w = nullptr;
		switch (target)
		{
			case 0: w = _btnSettings; break;
			case 1: w = _btnMode; break;
			case 2: w = _btnPlay; break;
			case 3: w = _btnFirst; break;
			case 4: w = _btnLast; break;
			default: return;
		}
		if (w == nullptr || !w->isEnabled())
			return;
		QEvent enter(QEvent::Enter);
		QCoreApplication::sendEvent(w, &enter);
		w->update();
	}

	void mPostRealtimeAnimWidget::applyModeSwitch(int idx)
	{
		//形态切换协议（保留，原 _comboMode currentIndexChanged 等价；菜单槽与 ForShot
		//注入槽共用）：停当前模式播放（静默，不发 playbackToggled——切换是用户主动语义
		//换挡，面板在 modeChanged 里做渲染侧收尾）→ 控件语义换挡 → 不开自动播放
		if (idx < 0 || idx > static_cast<int>(AnimMode::Sine) || idx == static_cast<int>(_mode))
			return;
		quietStop();
		_mode = static_cast<AnimMode>(idx);
		if (_mode != AnimMode::Frame)
			_step = 1;	//进参数化形态档位回起点（拖动/播放再定位）
		_direction = _session.reverse ? -1 : 1;	//换挡方向基线复位（FR-08 反向基线）
		refreshModeMenuUi();
		refreshSessionUi();
		emit modeChanged(idx);
	}

	void mPostRealtimeAnimWidget::setFrameForShot(int index)
	{
		//显示级切帧（emit=false：不触发切帧全链，仅落滑条/读数/禁用矩阵——TC 边界态截图）
		setFrameIndex(index);
	}

	void mPostRealtimeAnimWidget::setStepForShot(int step)
	{
		applyStep(qBound(1, step, kParamStepCount), false);
	}

	void mPostRealtimeAnimWidget::setModeForShot(int mode)
	{
		applyModeSwitch(mode);	//真实链（emit modeChanged→面板渲染侧收尾，TC-05/06 用例）
	}

	void mPostRealtimeAnimWidget::setPlayForShot(bool playing)
	{
		//真实链（toggled→slot_playToggled→面板正向闸），TC-07 播放按压态用例
		if (_btnPlay->isChecked() != playing)
			_btnPlay->setChecked(playing);
	}

	void mPostRealtimeAnimWidget::applyIndex(int index, bool emitChange)
	{
		_frameIndex = qBound(0, index, _frameCount - 1);
		_slider->blockSignals(true);
		_slider->setValue(_frameIndex);
		_slider->blockSignals(false);
		//D-23 回归修复：帧序推进即重落禁用矩阵——此前矩阵只在加载/换挡/范围/校验事件
		//重落，推进路径（步进/滑条/跳端点/tick/面板回同步）不刷新，加载起点态的
		//"首帧/上一帧禁"永不解除（="彻底点不了"），终点态"下一帧/末帧禁"同因反向失效。
		//域源不变=frameDomain()（与走带推进同一 bounds，W2 收窄域边界态自动成立）
		applyEnabledState();
		updateReadout();
		if (emitChange)
			emit frameIndexChanged(_frameIndex);
	}

	void mPostRealtimeAnimWidget::applyStep(int step, bool emitChange)
	{
		_step = qBound(1, step, kParamStepCount);
		_slider->blockSignals(true);
		_slider->setValue(_step);
		_slider->blockSignals(false);
		applyEnabledState();	//D-23：档位推进同刷新（1..20 档边界态随档走，applyIndex 同因）
		updateReadout();
		if (emitChange)
			emit interpolationStepChanged(_step);
	}

	double mPostRealtimeAnimWidget::frameTimeAt(int idx) const
	{
		//越界返 0（无帧时间数据时 timeAxisValid=false，时间档本就不走此路）
		return (idx >= 0 && idx < _frameTimes.size()) ? _frameTimes.at(idx) : 0.0;
	}

	mPostRealtimeAnimWidget::FrameDomain mPostRealtimeAnimWidget::frameDomain() const
	{
		//FR-11 指定范围有效帧域（D-05 升格真接入，对齐原型 bounds() 语义）：
		//未勾选/帧时间数据缺失→全帧表 [0, N-1]；勾选且时间轴可得→范围端点时间按
		//帧时间表二分映射帧序——lo=首个 >=起始时间（lower_bound）、hi=末个 <=结束时间
		//（upper_bound-1，对齐原型 findIndex/findLastIndex）；范围内无帧（lo 越界或
		//hi<lo）→valid=false（走带禁——弹窗"此范围内没有动画帧"校验先行，本处防御）。
		//D-22 时间域归一化：W2 端点=归一化域 0~1（弹窗输入/校验/文案同域），二分前先
		//×末帧时间还原原始时间域（等价于原域÷末帧时间，逐帧除法省去）；末帧<=0 或
		//缺失→范围域回退全帧表（与读数回退口径一致）
		FrameDomain d;
		if (_frameCount <= 0)
		{
			d.lo = 0; d.hi = 0; d.valid = false;
			return d;
		}
		d.lo = 0;
		d.hi = _frameCount - 1;
		d.valid = true;
		const double tEnd = timeAxisEnd();
		const bool rangeActive = _session.rangeEnabled && _timeAxisValid
			&& tEnd > 0.0
			&& _frameTimes.size() >= _frameCount;
		if (!rangeActive)
			return d;
		const double start = _session.rangeStart * tEnd;
		const double end = _session.rangeEnd * tEnd;
		const int loIdx = static_cast<int>(
			std::lower_bound(_frameTimes.cbegin(), _frameTimes.cend(), start)
			- _frameTimes.cbegin());
		const int hiIdx = static_cast<int>(
			std::upper_bound(_frameTimes.cbegin(), _frameTimes.cend(), end)
			- _frameTimes.cbegin()) - 1;
		if (loIdx < _frameTimes.size() && hiIdx >= loIdx)
		{
			d.lo = loIdx;
			d.hi = hiIdx;
		}
		else
			d.valid = false;	//范围内无帧：保留全表边界值，走带禁由 valid 承担
		return d;
	}

	void mPostRealtimeAnimWidget::advance(int direction)
	{
		//v1.4 推进核（仅 tick 自动推进，BUG-02：步进钮已分离至 stepBy——步进越界钳位停
		//不改方向、不发停机信号）：R3-D15 改 proto-literal 结构（逐分支对齐原型 advance
		//:29-39）——next 界内=seek(next)；越界且未在边界=先落边界帧（步长>1 时边界帧
		//可见一拍）；已在边界=按播放方式（单次停/循环跳对端/往返换向+反向步进）。
		//long long 过渡防步长超大整数越界（01 边界 8；D-16 后步长域=2^53-1）。
		//FR-11 真接入：帧形态域=frameDomain()（W2 生效时限范围内帧序，原型 bounds()），
		//参数化形态域不变（1..20 档与帧时间域无关）
		const bool modeIsFrame = (_mode == AnimMode::Frame);
		const FrameDomain dom = modeIsFrame ? frameDomain() : FrameDomain{ 1, kParamStepCount, true };
		if (modeIsFrame && !dom.valid)
		{
			stopAtNaturalEnd();	//防御：范围域失效（弹窗校验态未及回报）自然停机
			return;
		}
		const long long lo = dom.lo;
		const long long hi = dom.hi;
		const long long cur = modeIsFrame ? _frameIndex : _step;
		const long long next = cur + static_cast<long long>(direction) * _session.stepSize;
		const long long edge = (direction > 0) ? hi : lo;
		auto applyPos = [this, modeIsFrame, lo, hi](long long v) {
			v = qBound(lo, v, hi);
			if (modeIsFrame)
				applyIndex(static_cast<int>(v), true);
			else
				applyStep(static_cast<int>(v), true);
		};
		if (next >= lo && next <= hi)
			applyPos(next);
		else if (cur != edge)
			applyPos(edge);	//越界先落边界帧（原型 seek(edge)，方向不变；单次经尾判停）
		else if (_session.playStyle == PlayStyle::Once)
			stopAtNaturalEnd();
		else if (_session.playStyle == PlayStyle::Loop)
			applyPos(direction > 0 ? lo : hi);	//循环：跳对端续播
		else
		{
			_direction = -direction;	//往返：换向
			applyPos(cur + static_cast<long long>(_direction) * _session.stepSize);
		}
		if (_session.playStyle == PlayStyle::Once)
		{
			const long long onceEdge = (_direction > 0) ? hi : lo;
			if ((modeIsFrame ? _frameIndex : _step) == onceEdge)
				stopAtNaturalEnd();	//原型 ：37 单次落边界帧即停（含越界落边界路径）
		}
	}

	void mPostRealtimeAnimWidget::stepBy(int direction)
	{
		//BUG-02：步进钮专用推进（与 tick 推进核分离）——越界一律钳位停（不改 _direction、
		//不发停机信号，暂停态步进不再触发 Once 分支多发 playbackToggled(false)→面板
		//停机点补刷不重复）；Q-02 口径裁定修订版 02：手动步进边界一律钳位停，含循环模式
		//不回绕（依据=原型 step()→seek() 钳位行为）；tick 推进核边界行为不变（FR-07 域：
		//自动播放仍按播放方式 单次停/循环回绕/往返换向）。FR-11 真接入：帧形态钳位域=
		//frameDomain()（W2 生效时范围帧序域）
		const bool modeIsFrame = (_mode == AnimMode::Frame);
		const FrameDomain dom = modeIsFrame ? frameDomain() : FrameDomain{ 1, kParamStepCount, true };
		if (modeIsFrame && !dom.valid)
			return;	//防御：范围域失效（走带禁由禁用矩阵/校验态承担）
		const long long lo = dom.lo;
		const long long hi = dom.hi;
		long long next = (modeIsFrame ? _frameIndex : _step)
			+ static_cast<long long>(direction) * _session.stepSize;
		next = qBound(lo, next, hi);
		const int v = static_cast<int>(next);
		if (modeIsFrame)
			applyIndex(v, true);
		else
			applyStep(v, true);
	}

	void mPostRealtimeAnimWidget::stopAtNaturalEnd()
	{
		//BUG-02③：单次自然结束停表——仅播放态发 playbackToggled(false)（暂停态不再
		//到达本路径，停机点补刷不重复）；复位播放钮+图标/提示归位
		_playTimer->stop();
		if (!_btnPlay->isChecked())
			return;
		_btnPlay->blockSignals(true);
		_btnPlay->setChecked(false);
		_btnPlay->blockSignals(false);
		setPlayingUi(false);
		emit playbackToggled(false);
	}

	void mPostRealtimeAnimWidget::jumpToEdge(bool toStart)
	{
		//v1.4 首帧/末帧统一（FR-04）：停播后跳域端点并停（首帧承担原停止钮 stop-to-start）。
		//FR-11 真接入：帧形态端点=frameDomain() 域端（W2 生效时=范围首/末帧，原型
		//First/Last→seek(bounds[0/1]) 同语义）；参数化形态端点不变
		if (_mode == AnimMode::Frame)
		{
			const FrameDomain dom = frameDomain();
			applyIndex(dom.valid ? (toStart ? dom.lo : dom.hi)
								 : (toStart ? 0 : qMax(0, _frameCount - 1)), true);
		}
		else
			applyStep(toStart ? 1 : kParamStepCount, true);
	}

	void mPostRealtimeAnimWidget::applyPlayStart()
	{
		//FR-08 起播方向尾端检查：正向在终点/反向在起点→先跳对端起点再播
		//（跳转走既有帧变化链，随后从对端推进；单帧帧形态被禁用矩阵拦截不达此路）。
		//FR-11 真接入：帧形态尾端判据=frameDomain() 域端
		if (_mode == AnimMode::Frame)
		{
			const FrameDomain dom = frameDomain();
			if (!dom.valid)
				return;	//防御：范围域失效（播放闸/校验态兜底拦停）
			if (_direction > 0 && _frameIndex >= dom.hi)
				applyIndex(dom.lo, true);
			else if (_direction < 0 && _frameIndex <= dom.lo)
				applyIndex(dom.hi, true);
		}
		else
		{
			if (_direction > 0 && _step >= kParamStepCount)
				applyStep(1, true);
			else if (_direction < 0 && _step <= 1)
				applyStep(kParamStepCount, true);
		}
	}

	void mPostRealtimeAnimWidget::setFrameIndex(int index)
	{
		//外部（面板帧下拉等）切帧同步条上显示，不回发信号防环
		applyIndex(index, false);
	}

	void mPostRealtimeAnimWidget::setFrameCount(int count, int curIndex)
	{
		//v1.1 观察项 A：帧范围动态刷新（切工况重建帧表时调用，不重建不闪动）。
		//v1.4：帧数同为禁用域权威源——帧形态域刷新+读数+禁用矩阵一并收口
		//（01 边界 2：播放中帧表变化→当前帧钳位、播放态维持）
		_frameCount = count > 0 ? count : 1;
		_frameIndex = qBound(0, curIndex, _frameCount - 1);
		_contextFrameCount = count > 0 ? count : 0;
		if (_mode == AnimMode::Frame)
		{
			_slider->blockSignals(true);
			_slider->setRange(0, _frameCount - 1);
			_slider->setValue(_frameIndex);
			_slider->blockSignals(false);
		}
		applyEnabledState();
		updateReadout();
	}

	void mPostRealtimeAnimWidget::setEnabledContext(int frameCount)
	{
		//v1.4 常显形态禁用域入口（D-08）：无结果=0（停播+全禁，读数"暂无动画帧"）/
		//单帧=1（帧形态走带禁、参数化按边界）/多帧=N（全使能按边界）
		_contextFrameCount = qMax(0, frameCount);
		if (_contextFrameCount == 0)
			pausePlayback();	//无结果先停播（FR-01；isPlaying 门 no-op 不重复补发）
		applyEnabledState();
		updateReadout();
	}

	void mPostRealtimeAnimWidget::setFrameTimes(const QVector<double>& times)
	{
		//FR-09/11/14 真接入数据源（D-05 升格）：面板经 mPostDataBase::getFrameTime 批读
		//当前结果帧时间表随 setFrameCount 同步下发；全帧<=0=数据源无帧时间（未装载/
		//静态结果）→ _timeAxisValid=false，时间档读数回退帧序显示、W2 时间域映射回退
		//全帧表（不崩溃、不提示）
		_frameTimes = times;
		_timeAxisValid = false;
		for (double t : _frameTimes)
		{
			if (t > 0.0) { _timeAxisValid = true; break; }
		}
		refreshSessionUi();	//读数/W2 域/禁用矩阵随帧时间轴可用性刷新（时间档+范围+倍率联动）
	}

	void mPostRealtimeAnimWidget::pausePlayback()
	{
		if (!isPlaying()) return;
		_playTimer->stop();
		_enginePlayback = false;	//引擎合一：会话驱动态一并清除（面板 toggled(false) 分支 session pause）
		_btnPlay->blockSignals(true);
		_btnPlay->setChecked(false);
		_btnPlay->blockSignals(false);
		setPlayingUi(false);
		emit playbackToggled(false);
	}

	void mPostRealtimeAnimWidget::enterEnginePlayback()
	{
		//引擎合一（2026-11-16）：面板值就绪门通过后，帧模式播放改由会话播放器驱动——
		//自驱表静默停（playbackToggled 已由播放钮发出，此处不再发），推进事件由
		//playTick 补发的 PlayAdvance frameChanged 单通道承担
		if (_playTimer)
			_playTimer->stop();
		_enginePlayback = true;
	}

	void mPostRealtimeAnimWidget::mirrorEngineStep(int step, int direction)
	{
		//引擎档播放推进镜像·参数化（unify-anim-playback 2026-11-17）：档位+方向回摆同步
		//——滑条/档位读数/禁用矩阵跟随（applyStep emit=false 不回发防环：宿主镜像通道
		//单向回流，不触发面板定位路由）+方向基线落账（_direction，供 applyReverse/
		//engineSettingsChanged 下发消费）。仅由宿主参数化镜像通道调用（playTick 每 tick
		//interpolationAdvanced 回流；方向含 PingPong 换向回摆，读数随回摆递减）
		_direction = direction;
		applyStep(qBound(1, step, kParamStepCount), false);
	}

	void mPostRealtimeAnimWidget::mirrorEngineDirection(int direction)
	{
		//引擎档方向基线同步（帧/参数化共用，unify-anim-playback）：playbackDirection
		//Changed 回流（PingPong 端点内建换向）——纯基线落账零 UI 动作。防后续 reconfigure
		//下发过时方向=方向回跳（FR-05/FR-06 验收③）
		_direction = direction;
	}

	void mPostRealtimeAnimWidget::stopPlayback()
	{
		//停止（v1 口径 stop-to-start，按形态回各自起点）：停表+复位播放态+定位起点。
		//v1.4 用户入口已由首帧钮承担（D-03），本函数保留为内部停机链出口
		_playTimer->stop();
		_enginePlayback = false;	//引擎合一：内部停机链出口同步清引擎态
		if (_btnPlay->isChecked())
		{
			_btnPlay->blockSignals(true);
			_btnPlay->setChecked(false);
			_btnPlay->blockSignals(false);
			setPlayingUi(false);
			emit playbackToggled(false);
		}
		if (_mode == AnimMode::Frame)
			applyIndex(0, true);					//帧形态回第 1 帧（HyperView stop-to-start）
		else
			applyStep(1, true);						//参数化形态回第 1 档
	}

	void mPostRealtimeAnimWidget::yieldForRecording()
	{
		//P6-1 审计修复：录制让路专用停机——仅停表+视觉归位（applyIndex/applyStep
		//emit=false，不发 frameIndexChanged/interpolationStepChanged——不触发面板切帧全链）。
		//播放态复位静默（quietStop 不发 playbackToggled）；语义与 v1.3 完全一致
		quietStop();
		if (_mode == AnimMode::Frame)
			applyIndex(0, false);
		else
			applyStep(1, false);
	}

	void mPostRealtimeAnimWidget::quietStop()
	{
		//静默停止：形态切换内部收尾用（不发 playbackToggled，避免面板侧互斥判定介入）
		if (!isPlaying() && !_btnPlay->isChecked())
			return;
		const bool wasEngine = _enginePlayback;
		_playTimer->stop();
		_enginePlayback = false;	//引擎合一：静默停止（形态换挡/录制让路）同步清引擎态
		if (wasEngine)
			emit playbackToggled(false);	//引擎合一：会话播放器须随停（面板 toggle(false) 分支 session pause；录制期补发被早退拦=P6-1 口径保持）
		_btnPlay->blockSignals(true);
		_btnPlay->setChecked(false);
		_btnPlay->blockSignals(false);
		setPlayingUi(false);
	}

	void mPostRealtimeAnimWidget::refreshSessionUi()
	{
		//按当前形态刷新滑条范围/步进提示/读数语义+禁用矩阵（v1.3 refreshModeUi 扩展：
		//换挡三件套——滑条域换挡/读数语义/禁用域）。FR-11 真接入：帧形态滑条域=
		//frameDomain()（W2 生效时限范围内帧序，滑条 min/max 即范围端点帧序）
		if (_mode == AnimMode::Frame)
		{
			const FrameDomain dom = frameDomain();
			_slider->blockSignals(true);
			_slider->setRange(dom.valid ? dom.lo : 0, dom.valid ? dom.hi : qMax(0, _frameCount - 1));
			_slider->setValue(_frameIndex);
			_slider->blockSignals(false);
			_slider->setToolTip(tr("帧进度"));
			_btnPrev->setToolTip(tr("上一帧"));
			_btnNext->setToolTip(tr("下一帧"));
		}
		else
		{
			_slider->blockSignals(true);
			_slider->setRange(1, kParamStepCount);
			_slider->setValue(_step);
			_slider->blockSignals(false);
			_slider->setToolTip(tr("插值进度（拖动直接设档）"));
			_btnPrev->setToolTip(tr("上一档"));
			_btnNext->setToolTip(tr("下一档"));
		}
		applyEnabledState();
		updateReadout();
	}

	void mPostRealtimeAnimWidget::refreshModeMenuUi()
	{
		//类型入口图标随选中项切换+菜单选中项高亮（FR-03）+D-08：类型钮 tooltip 动态含
		//当前类型名（原型 title='动画类型：'+label）；R4：图标=原型 animationTypes path
		//同形渲染（瞬态曲线/正弦/线性箭头，废弃 qrc 语义无关资产），复合=类型图形
		//（stroke 1.5 round）+右下实心 ▾（原型 .vp-animation-type-arrow fill:currentColor）
		//按下标=AnimMode 映射（Frame/Linear/Sine，非原型 animationTypes 序）
		static const QString kModePaths[3] = {
			QString(kPathTransient),	//Frame=瞬态曲线
			QString(kPathLinear),		//Linear=线性箭头
			QString(kPathModal) };		//Sine=模态正弦
		const int idx = static_cast<int>(_mode);
		if (idx >= 0 && idx < 3 && _btnMode != nullptr)
		{
			//D-08 补遗（自绘弹层后无 menu-indicator）：合成图标=20px 类型图形+choice-gap
			//4px 隙+12px 槽内实心 ▾（R4 对位原型规则 18/20 computed：类型钮宽=control+body
			//38、▾=vp-animation-type-arrow path 'm3 4 3 4 3-4z' 12 盒→6×4 实心三角、
			//垂直几何中心；位图 36 宽入 38 钮居中→图形 x1..21/▾ 中心 x30，与原型逐位一致）
			const int iconPx = MxFunctions::UiMetrics::controlIconSize();
			const int arrowSlot = qMax(10, MxFunctions::UiMetrics::bodyFontSize());	//▾ 槽宽=--std-body 档
			const int choiceGap = MxFunctions::UiMetrics::choiceGap();	//图形与 ▾ 隙=4（原型 gap 规则）
			QPixmap pm(iconPx + choiceGap + arrowSlot, iconPx);
			pm.fill(Qt::transparent);
			QPainter p(&pm);
			p.setRenderHint(QPainter::Antialiasing);
			const QColor color = MxFunctions::UiPalette::text();
			p.drawPixmap(0, 0, renderProtoIcon(kModePaths[idx], color, iconPx));
			p.setPen(Qt::NoPen);
			p.setBrush(color);
			const qreal cx = iconPx + choiceGap + arrowSlot / 2.0;
			const qreal cy = iconPx / 2.0;	//▾ 垂直几何中心（原型 ▾ 盒垂直居中，path 4..8 中心 6=盒中心）
			QPolygonF arrow;
			arrow << QPointF(cx - 3.0, cy - 2.0) << QPointF(cx + 3.0, cy - 2.0) << QPointF(cx, cy + 2.0);
			p.drawPolygon(arrow);
			p.end();
			_btnMode->setIcon(QIcon(pm));
			_btnMode->setIconSize(pm.size());	//合成图标按位图尺寸渲染（16px 档会压扁叠画）
			static const char* kModeTrContext[3] = { QT_TR_NOOP("瞬态动画"), QT_TR_NOOP("线性动画"), QT_TR_NOOP("模态动画") };
			_btnMode->setToolTip(tr("动画类型：%1").arg(tr(kModeTrContext[idx])));
		}
		if (_btnTypeFrame != nullptr)
			_btnTypeFrame->setChecked(_mode == AnimMode::Frame);
		if (_btnTypeLinear != nullptr)
			_btnTypeLinear->setChecked(_mode == AnimMode::Linear);
		if (_btnTypeSine != nullptr)
			_btnTypeSine->setChecked(_mode == AnimMode::Sine);
	}

	void mPostRealtimeAnimWidget::applyEnabledState()
	{
		//v1.4 禁用矩阵（02 §3.3）：无结果=全禁（读数"暂无动画帧"）；单帧×帧形态=走带禁
		//（帧域 0..0）；单帧×参数化=按边界（开放问题 7 结论：generateAnimation 参数化
		//分支仅查 boundProxy，帧表旁路，单帧可用）；多帧=按边界（起点禁首帧/上一帧、
		//终点禁下一帧/末帧，FR-04）；校验错误=走带全禁（FR-15）。录制/预生成态不在此层
		//禁用（面板闸拦截+提示，保留现 UX——02 §3.3 矩阵既定）。FR-11 真接入：帧形态
		//边界判据与滑条域=frameDomain()（W2 生效时范围内首末帧；范围域失效=走带禁）
		const bool modeIsFrame = (_mode == AnimMode::Frame);
		const bool hasResult = _contextFrameCount > 0;
		const FrameDomain dom = modeIsFrame ? frameDomain()
			: FrameDomain{ 1, kParamStepCount, true };
		const bool playable = _sessionValid && hasResult && dom.valid
			&& (modeIsFrame ? _contextFrameCount > 1 : true);
		const long long lo = dom.lo;
		const long long hi = dom.hi;
		const long long cur = modeIsFrame ? _frameIndex : _step;
		const bool atStart = playable && cur <= lo;
		const bool atEnd = playable && cur >= hi;

		_btnPlay->setEnabled(playable);
		_btnFirst->setEnabled(playable && !atStart);
		_btnPrev->setEnabled(playable && !atStart);
		_btnNext->setEnabled(playable && !atEnd);
		_btnLast->setEnabled(playable && !atEnd);
		_slider->setEnabled(playable && hi > lo);	//滑条域（单帧×帧形态 0..0 → 禁）
		_btnSettings->setEnabled(hasResult);		//设置/类型：无结果禁，单帧/多帧使
		_btnMode->setEnabled(hasResult);
	}

	void mPostRealtimeAnimWidget::updateReadout()
	{
		//v1.4 读数（FR-05）：无帧="暂无动画帧"；帧档="帧 n / m"（1 基）；时间档=
		//"0.000 / 1.000"（3 位小数，原型 format 口径）。D-05 升格真接入：时间档依赖帧
		//时间轴（FR-09）。D-22 时间域归一化（对齐原型"最大值永远是 1.000"）：显示值=
		//帧时间÷末帧时间（折算 0~1）×倍率——分子=当前帧归一化值×倍率、分母=归一化域末
		//（恒 1.000）×倍率（倍率≠1 时最大值=倍率值，原型行为不变）；末帧<=0 或缺失
		//回退按帧序显示（不崩溃、不提示）；分母不再随 W2 范围域收窄（归一化域最大值恒
		//1.000，W2 只限走带帧序域）。FR-14 真接入：倍率仅缩放时间读数。
		//D-09：读数 tooltip=原型 title（'当前 / 总计'，无帧='请载入结果帧'）
		if (_contextFrameCount <= 0)
		{
			setFrameLabelText(tr("暂无动画帧"));
			if (_frameLabel != nullptr)
				_frameLabel->setToolTip(tr("请载入结果帧"));
			updateReadoutReservedWidth();	//D-21：无帧态最坏文本宽（语言切换等事件联动）
			return;
		}
		if (_frameLabel != nullptr && _frameLabel->toolTip() != tr("当前 / 总计"))
			_frameLabel->setToolTip(tr("当前 / 总计"));
		if (_mode == AnimMode::Frame)
		{
			const FrameDomain dom = frameDomain();
			const int hiIdx = dom.valid ? dom.hi : qMax(0, _frameCount - 1);
			const double tEnd = timeAxisEnd();
			if (_session.display == DisplayStyle::Time && _timeAxisValid && tEnd > 0.0)
			{
				setFrameLabelText(QString("%1 / %2")
					.arg(formatAnimTime(normalizedTime(frameTimeAt(_frameIndex))),
						 formatAnimTime(normalizedTime(tEnd))));
			}
			else
				setFrameLabelText(tr("帧 %1 / %2").arg(_frameIndex + 1).arg(hiIdx + 1));
		}
		else
			setFrameLabelText(tr("档位 %1 / %2").arg(_step).arg(kParamStepCount));
		updateReadoutReservedWidth();	//D-21：当前格式最坏文本预留定宽（宽不变则零几何变化）
	}

	QString mPostRealtimeAnimWidget::formatAnimTime(double t) const
	{
		//D-21：时间档读数格式化独立 helper（原内联 arg(...,'f',3) 于读数与拖动预览两处，
		//宽度预留新增第三处消费——收口单点防格式漂移）
		return QString::number(t, 'f', 3);
	}

	double mPostRealtimeAnimWidget::timeAxisEnd() const
	{
		//D-22 归一化域分母（单点收口）：末帧时间；缺失或<=0 返 0——调用方以 tEnd>0
		//判据回退帧序显示/全帧表域（任务口径 3：回退不变）
		return (_frameTimes.isEmpty() || _frameTimes.last() <= 0.0) ? 0.0 : _frameTimes.last();
	}

	double mPostRealtimeAnimWidget::normalizedTime(double t) const
	{
		//D-22 归一化显示换算单点（读数/拖动预览/宽度预留三处共用——D-21 同款防口径
		//漂移）：t/末帧时间×倍率（折算 0~1 后乘倍率；调用方须先验 tEnd>0）
		return t / timeAxisEnd() * _session.timeScale;
	}

	void mPostRealtimeAnimWidget::updateReadoutReservedWidth()
	{
		//D-21 读数宽度抖动修复：按当前显示格式的最坏情况文本预留定宽——D-20 读数 hug
		//内容使控件条 sizeHint 随每次读数文本微变，QMainWindow 工具栏行随之重分配空间、
		//挤动行内前面所有控件（"9.000 / 10.000"→"10.000 / 10.000"复现路径）。原型
		//output 虽 auto 宽但其条总宽恒定（定宽+滑条 flex 吸收富余），本修达成同等观感。
		//重算时机=最坏文本可能变化的事件路径（setFrameTimes/setFrameCount/显示方式/
		//语言/倍率/范围切换，均经 updateReadout 收口+StyleChange 密度档字体联动）：
		//播放 tick 路径最坏文本不变→宽度守卫短路不触碰几何→零挤动（读数 setText 照常，
		//AlignLeft 下预留宽≥文本宽，只短不长）
		if (_frameLabel == nullptr)
			return;
		const QFontMetrics fm = _frameLabel->fontMetrics();
		QString worst;
		if (_contextFrameCount <= 0)
			worst = tr("暂无动画帧");	//无帧态
		else if (_mode != AnimMode::Frame)
			worst = tr("档位 %1 / %1").arg(kParamStepCount);	//参数化档：两侧最大档位
		else if (_session.display == DisplayStyle::Time && _timeAxisValid)
		{
			//时间档（D-22 归一化域）：归一化域末恒 1.000×倍率=最长形态（当前读数≤域末
			//恒成立，W2 范围收窄不影响分母；末帧<=0/缺失时该分支不走，守卫照常回退帧档）
			const double tEnd = timeAxisEnd();
			if (tEnd > 0.0)
			{
				const QString endText = formatAnimTime(normalizedTime(tEnd));
				worst = endText + QStringLiteral(" / ") + endText;
			}
			else
				worst = tr("帧 %1 / %1").arg(_frameCount);
		}
		else
			worst = tr("帧 %1 / %1").arg(_frameCount);	//帧档：两侧取最大帧号（覆盖 W2 域末）
		const int w = fm.horizontalAdvance(worst) + 6;	//6px 余量
		if (_frameLabel->minimumWidth() != w)	//守卫：同宽不触碰几何（播放 tick 零几何变化的关键）
			_frameLabel->setFixedWidth(w);
	}

	void mPostRealtimeAnimWidget::applyPlayStyle(int style)
	{
		_session.playStyle = static_cast<PlayStyle>(qBound(0, style, 2));
		//引擎合一：引擎态下变更即时下发会话。【unify-anim-playback】原"改往返自动暂停
		//降档"分支随 Q2=B 移除——setPingPong 参数下发即时生效（不暂停不降档无提示，
		//推进至端点即换向 FR-05 验收③）；非引擎态零动作（自驱 advance 直读
		//_session.playStyle，混合档 PingPong 即时生效由此承担，现状已成立）
		if (_enginePlayback)
			emit engineSettingsChanged();
	}

	void mPostRealtimeAnimWidget::applyReverse(bool on)
	{
		_session.reverse = on;
		//FR-08：播放中切换即时掉头；取消恢复正向（往返换向态取消时归位正向）
		if (on)
		{
			if (_direction > 0)
				_direction = -1;
		}
		else
			_direction = 1;
		if (_enginePlayback)
			emit engineSettingsChanged();	//引擎合一：方向即时下发会话（playTick 按 _aniDirection 推进）
	}

	void mPostRealtimeAnimWidget::applyDisplayStyle(int style)
	{
		const auto v = static_cast<DisplayStyle>(qBound(0, style, 1));
		_session.display = v;
		updateReadout();	//切显示方式不打断播放（FR-09；帧时间数据缺失时时间档读数自动回退帧序）
	}

	void mPostRealtimeAnimWidget::applyStepSize(long long step)
	{
		_session.stepSize = qMax<long long>(1, step);	//步进钮与自动播放 tick 共用步长（FR-10）；
		//D-16：long long 承载（对齐原型 safe-integer 2^53-1 域；validate 侧已限 ≤2^53-1），
		//advance 推进本就以 long long 过渡，超大步长越界由边界分支钳位
		if (_enginePlayback)
			emit engineSettingsChanged();	//引擎合一：步长即时下发会话（playTick 步进粒度）
	}

	void mPostRealtimeAnimWidget::applyRange(bool enabled, double start, double end)
	{
		//FR-11 指定范围真接入（D-05 升格）：W2 勾选后起始/结束时间按帧时间表二分映射为
		//帧序域（frameDomain，原型 bounds() 语义）——播放/走带/滑条限定范围内；未勾选=
		//全帧表。D-22：端点=归一化时间域 0~1（弹窗输入/校验/文案同域；弹窗下发已归一化
		//见 slot_settingsClicked），frameDomain 内 ×末帧时间还原原始域二分，读数分母不随
		//范围收窄（归一化域最大值恒 1.000）。FR-15 校验照常在弹窗（时间须在首末之间/
		//结束≥起始/范围内无帧→走带禁）。暂停态且当前帧落在域外时按原型 validate() 口径
		//钳位进域（emit=走切帧全链；播放态不钳——下一 tick 经 advance 域边界分支自然归位，
		//避免播放中逐键输入触发切帧风暴）
		const bool changed = (enabled != _session.rangeEnabled)
			|| start != _session.rangeStart || end != _session.rangeEnd;
		_session.rangeEnabled = enabled;
		_session.rangeStart = start;
		_session.rangeEnd = end;
		refreshSessionUi();	//滑条域/读数分母/走带禁用矩阵随域即时刷新
		if (_mode == AnimMode::Frame && !_btnPlay->isChecked())
		{
			const FrameDomain dom = frameDomain();
			if (dom.valid && (_frameIndex < dom.lo || _frameIndex > dom.hi))
				applyIndex(start < end ? qBound(dom.lo, _frameIndex, dom.hi) : dom.lo, true);
		}
		Q_UNUSED(changed);
		if (_enginePlayback)
			emit engineSettingsChanged();	//引擎合一：范围即时下发会话（playTick 回绕按新域）
	}

	void mPostRealtimeAnimWidget::applyFps(int fps)
	{
		_session.fps = qBound(2, fps, 20);
		if (_playTimer != nullptr)
			_playTimer->setInterval(1000 / _session.fps);	//播放中调整即时生效（FR-12）
		if (_enginePlayback)
			emit engineSettingsChanged();	//引擎合一：速度即时下发会话（setFrameRate 播放中重启定时器）
	}

	void mPostRealtimeAnimWidget::applyShape(int mode)
	{
		//FR-13 模型构形真接入（D-05 升格）：选"未变形"→面板 shapeChanged 槽写
		//REBUILD_PostRender::setDeformationScale(0,0,0)（置零先例 mPostDeformation.cpp:92-93，
		//deformationChanged 广播由渲染门面照常触发，测量标注等消费方自动跟随）；切回
		//"变形后"/退出工具栏/结果切换由面板槽恢复既定系数（变形面板 guidata 权威值）。
		//本槽只记会话+发变化沿，不触碰渲染（渲染收口在面板，与四路协议同构）；
		//纯状态写入不触播放链（录制 playbackState=Stopped 期/预生成互斥组合均不误触）
		const auto v = static_cast<ShapeMode>(qBound(0, mode, 1));
		const bool changed = (v != _session.shape);
		_session.shape = v;
		if (changed)
			emit shapeChanged(static_cast<int>(v));
	}

	void mPostRealtimeAnimWidget::applyTimeScale(double scale)
	{
		_session.timeScale = scale;
		//D-04（原型 TimeScale 变更 emit→paintTransport 同源行为）+FR-14 真接入：倍率仅
		//缩放时间读数（_timeAxisValid 才有差异；帧档下 updateReadout 幂等），不改播放
		//速度/原始时间域
		updateReadout();
	}

	void mPostRealtimeAnimWidget::resetShapeToDeformed()
	{
		//FR-13：面板侧上下文切换（结果切换/退出后处理/面板析构）配套——构形会话回
		//"变形后"，不发 shapeChanged（恢复动作由面板槽自做，避免恢复写与复位竞态）；
		//设置弹窗懒创建，下次 popup 时经 setSession 镜像呈现
		_session.shape = ShapeMode::Deformed;
	}

	void mPostRealtimeAnimWidget::applySessionValid(bool valid)
	{
		if (_sessionValid == valid)
			return;
		_sessionValid = valid;
		if (!valid)
			pausePlayback();	//校验不通过→播放自动暂停（FR-15；停机点补刷经面板 !playing 分支）
		applyEnabledState();	//走带禁用域随校验态解除/恢复（修正后禁用解除，不自动续播）
	}

	void mPostRealtimeAnimWidget::setFrameLabelText(const QString& text)
	{
		//播放黑闪诊断修复（2026-11-10，防御加固，保留）：帧号 label 与滑条是播放期仅有的
		//逐 tick 自重绘源。QSlider 同值 setValue 自身零重绘，QLabel 同文本 setText
		//仍走一次重绘提交——值不变时直接跳过，消除回绕/停帧档位上的无谓重绘
		if (_frameLabel != nullptr && _frameLabel->text() != text)
			_frameLabel->setText(text);
	}

	//D-24 关闭崩溃修复：弹窗 parent=设置钮，按钮析构（~QObject 阶段）连带删弹窗时，
	//弹窗 destroyed 回调对半析构的按钮 setChecked → 空虚调用（VS 断点栈实证：
	//~Widget→删设置钮→删弹窗→destroyed→ensureSettingsPopup lambda→setChecked→0 地址）。
	//显式析构先删弹窗层：此刻按钮链仍完好，destroyed 回调 setChecked 安全；QPointer 随置空
	mPostRealtimeAnimWidget::~mPostRealtimeAnimWidget()
	{
		if (_settingsPopup != nullptr) {
			delete _settingsPopup;	//按钮仍存活，destroyed 回调 setChecked 安全
			_settingsPopup = nullptr;
		}
		if (_menuMode != nullptr) {
			delete _menuMode;	//类型菜单弹层同口径先删（无 destroyed 回调，纯析构序整洁）
			_menuMode = nullptr;
		}
	}

	void mPostRealtimeAnimWidget::ensureSettingsPopup()
	{
		if (_settingsPopup != nullptr)
			return;
		_settingsPopup = new mPostRealtimeAnimSettingsPopup(_btnSettings);
		//八类会话字段+校验态回接（成员函数指针直连，Qt5 新语法免 slots 声明）
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::playStyleChanged,
			this, [this](int s) { applyPlayStyle(s); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::reverseChanged,
			this, [this](bool on) { applyReverse(on); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::displayChanged,
			this, [this](int s) { applyDisplayStyle(s); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::stepSizeChanged,
			this, [this](long long s) { applyStepSize(s); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::rangeChanged,
			this, [this](bool en, double s, double e) { applyRange(en, s, e); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::fpsChanged,
			this, [this](int f) { applyFps(f); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::shapeChanged,
			this, [this](int m) { applyShape(m); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::timeScaleChanged,
			this, [this](double v) { applyTimeScale(v); });
		connect(_settingsPopup, &mPostRealtimeAnimSettingsPopup::sessionValidated,
			this, &mPostRealtimeAnimWidget::applySessionValid);
		connect(_settingsPopup, &QObject::destroyed, this, [this]() {
			//ui-shot 弹窗"用后即焚"（deleteLater）等外部销毁路径：入口展开态复位
			if (_btnSettings != nullptr)
				_btnSettings->setChecked(false);
		});
	}

	void mPostRealtimeAnimWidget::openSettingsPopup()
	{
		//显式打开（ui-shot 弹窗截图驱动经面板槽转发）；已开则幂等
		if (_settingsPopup != nullptr && _settingsPopup->isVisible())
			return;
		slot_settingsClicked();
	}

	void mPostRealtimeAnimWidget::closeSettingsPopup()
	{
		//宿主退出链调用（切视口/切面板/壳拖动，§9 契约：退出瞬时态）；值保留在会话
		if (_settingsPopup != nullptr && _settingsPopup->isVisible())
			_settingsPopup->close();
		if (_btnSettings != nullptr)
			_btnSettings->setChecked(false);
	}

	void mPostRealtimeAnimWidget::retranslateUi()
	{
		//语言切换全量重译（F-2）：控件创建时已带 tr() 初始文本，此处运行时刷新
		//（踩坑 16 尾）；设置弹窗懒创建未创建时跳过（§9 契约）
		_btnSettings->setToolTip(tr("动画设置"));
		_btnMode->setToolTip(tr("动画类型"));
		_btnFirst->setToolTip(tr("首帧"));
		_btnPlay->setToolTip(isPlaying() ? tr("暂停") : tr("播放"));
		_btnPrev->setToolTip(_mode == AnimMode::Frame ? tr("上一帧") : tr("上一档"));
		_btnNext->setToolTip(_mode == AnimMode::Frame ? tr("下一帧") : tr("下一档"));
		_btnLast->setToolTip(tr("末帧"));
		//D-08：菜单项 icon-only（无文字），语言切换只刷新 ToolTip；类型钮 tooltip
		//动态名经 refreshModeMenuUi 收口
		if (_btnTypeFrame != nullptr)
			_btnTypeFrame->setToolTip(tr("瞬态动画"));
		if (_btnTypeLinear != nullptr)
			_btnTypeLinear->setToolTip(tr("线性动画：当前帧变形线性插值"));
		if (_btnTypeSine != nullptr)
			_btnTypeSine->setToolTip(tr("模态动画：当前帧变形正弦插值（原正弦动画）"));
		refreshModeMenuUi();	//D-08：类型钮 tooltip 动态名随语言刷新
		if (_slider != nullptr)
			_slider->setToolTip(_mode == AnimMode::Frame ? tr("帧进度") : tr("插值进度（拖动直接设档）"));
		updateReadout();
		if (_settingsPopup != nullptr)
			_settingsPopup->retranslateUi();
	}

	void mPostRealtimeAnimWidget::setPlayingUi(bool playing)
	{
		//R4：播放/暂停随态切换=原型 play/pause path 同形渲染（废弃 qrc kaishi/zanting
		//蓝圆资产）；按压底/selectedText 色由壳 QSS checked 底+protoIcon On 态位图承担
		const int px = MxFunctions::UiMetrics::controlIconSize();
		_btnPlay->setIcon(protoIcon(playing ? kPathPause : kPathPlay, px));
		_btnPlay->setToolTip(playing ? tr("暂停") : tr("播放"));
	}

	void mPostRealtimeAnimWidget::applyIconTheme()
	{
		//R4：全部图标按当前 UiPalette 重渲染（主题切换链=applySliderStyle 的 StyleChange
		//现场重发沿用，滑条机制同一通道）
		const int px = MxFunctions::UiMetrics::controlIconSize();
		if (_btnSettings != nullptr)
			_btnSettings->setIcon(protoIcon(kPathSettings, px));
		if (_btnFirst != nullptr)
			_btnFirst->setIcon(protoIcon(kPathFirst, px));
		if (_btnPrev != nullptr)
			_btnPrev->setIcon(protoIcon(kPathPrevious, px));
		if (_btnNext != nullptr)
			_btnNext->setIcon(protoIcon(kPathNext, px));
		if (_btnLast != nullptr)
			_btnLast->setIcon(protoIcon(kPathLast, px));
		if (_btnPlay != nullptr)
			setPlayingUi(_btnPlay->isChecked());
		if (_menuMode != nullptr)
		{
			//菜单三项随主题重渲染（选中项 On 态=selectedText）
			const QSize iconSz(px, px);
			if (_btnTypeFrame != nullptr) { _btnTypeFrame->setIcon(protoIcon(kPathTransient, px)); _btnTypeFrame->setIconSize(iconSz); }
			if (_btnTypeSine != nullptr) { _btnTypeSine->setIcon(protoIcon(kPathModal, px)); _btnTypeSine->setIconSize(iconSz); }
			if (_btnTypeLinear != nullptr) { _btnTypeLinear->setIcon(protoIcon(kPathLinear, px)); _btnTypeLinear->setIconSize(iconSz); }
		}
		refreshModeMenuUi();	//类型钮合成图标随主题色
	}

	void mPostRealtimeAnimWidget::applySliderStyle()
	{
		//R4 渲染真值口径（proto_shot --eval+裁图放大实测，shots\protoaudit3\slider_proto*.png）：
		//#animationProgress 高 26（含）、accent=primary #1890FF——Chromium 原生渲染实测
		//轨道粗 6px 圆头、色 #3B3B3B（深灰，非边线浅灰）、已填充段=primary、拇指直径 15px
		//圆形 primary（原 4px 细槽+13px 小拇指与原型明显不符——用户指认项）。速度滑条
		//真值（--std-tab-underline 2px+13px 拇指）已在弹窗侧既有 QSS 对齐，无需改。
		//深色主题轨道色随主题（原生渲染在暗底下为浅灰语义，取 border 档）；切主题经
		//changeEvent(StyleChange) 现场重发；图标同场重渲染（applyIconTheme）
		QString track = QStringLiteral("#3B3B3B");	//实测渲染真值（浅色/护眼主题）
		if (MxFunctions::UiPalette::theme() == MxFunctions::UiPalette::Theme::Dark)
			track = MxFunctions::UiPalette::borderHex();
		const QString qss = QString(
			"QSlider#animProgress{min-height:26px;}"	//原型 input 高 26（拇指 15px 完整容纳）
			"QSlider#animProgress::groove:horizontal{height:6px;background:%1;border-radius:3px;}"
			"QSlider#animProgress::sub-page:horizontal{background:%2;border-radius:3px;}"
			"QSlider#animProgress::handle:horizontal{width:15px;height:15px;background:%2;border-radius:7px;margin:-4px 0;}"
			"QWidget#animBarSeparator{background:%3;}")
			.arg(track,
				 MxFunctions::UiPalette::primaryHex(),	//已填充段与手柄=主题强调色（原型 accent-color）
				 MxFunctions::UiPalette::borderHex());	//分隔线=边线色
		if (styleSheet() != qss)
			setStyleSheet(qss);
		applyIconTheme();
		updateReadoutReservedWidth();	//D-21：主题/密度档字体随 StyleChange 变化→最坏文本 advance 重算（构造期 _frameLabel 未建由判空跳过）
	}
}
