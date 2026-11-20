#include "mFramelessMessageBox.h"

#include "ThemeManager.h"
#include "UiMetrics.h"
#include "UiPalette.h"

#include <QAbstractButton>
#include <QApplication>
#include <QCursor>
#include <QDialogButtonBox>
#include <QFont>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

namespace MechMainWindow
{
	namespace
	{
		/*
		* 原型形态常数（仅 prototypeStyle=true 时使用——2026-11-05 用户圈定：
		* 原型三段式只给曲线切换确认框，其余调用点保持通用形态）。
		* 原型实测（提示框原型供图系 1.5× 放大稿——用户 2026-11-04 确认真机
		* 目标 400×156；以下常数＝原型实测 ÷1.5 取整，与白卡家族常数互证：
		* logo 14×16、✕ 墨迹 12、正文 13px 全部咬合）：
		* 对话框外框 400×156 = 标题带 34 + 灰带 85 + 按钮带 35 + 外框线 2；
		* 灰带左右内边距 20、上下 13、图标 25、图标-文字间距 13；
		* 按钮带右距 9、上 4 下 5；按钮 44×26、钮距 11
		*/
		constexpr int kContentWidth = 398;   // 对话框外框宽 400 = 398 + 外框线 2
		constexpr int kBannerHeight = 85;
		constexpr int kBannerPadL = 20;
		constexpr int kBannerPadV = 13;
		constexpr int kIconTextGap = 13;
		constexpr int kIconEdge = 25;
		constexpr int kBtnW = 44;
		constexpr int kBtnH = 26;
		constexpr int kBtnGap = 11;
		constexpr int kFooterPadR = 9;
		constexpr int kFooterPadT = 4;
		constexpr int kFooterPadB = 5;

		/*
		* 转换设置形态常数（仅 convertInputStyle=true；2026-11-06 用户供新原型，
		* 原生 1:1 设计稿——蓝钮实测恰 44×26 与家族常数同尺，内衬全落整数档，
		* 非 1.5×/1.2× 放大稿）：卡内框 480×172（外框 482，比例 2.79 与旧稿
		* 一致）；20px 内衬系统——标题带 62（logo/✕ 墨迹中心实测 31 自洽），
		* 带下衬 10（输入行顶 72），输入行 24，行→钮距 30，底钮带下衬 20；
		* 输入框 320×24（2026-11-06 像素复核原型实测 320：左缘 141/右缘 460，
		* 旧 300/302 系测量偏差——20+label114+6+320+20=480 恰收整；label 为
		* 弹性格吃距、spin 依右缘钉住）；钮 44×26 钮距 11 沿用家族常数（原型
		* 实测咬合）。按钮序=取消左/继续右（QDialogButtonBox 平台 Windows 序
		* 主键在左，与原型相反，装配处换位）。底钮带右衬不在此处叠加：根布局
		* 右衬 kBannerPadL=20 已是原型档（按钮/输入框右缘同落 460），页脚再
		* 叠 20 成双重衬 40——真机按钮右缘 440 偏差 20 的根因（11-06 复核）。
		* 各随自家原型、不与确认框 34 带强统一（ui-spec-mxmech §5 M6 先例）。
		*/
		constexpr int kConvertContentWidth = 480;
		constexpr int kConvertTitleBarHeight = 62;
		constexpr int kConvertPadTop = 10;
		constexpr int kConvertRowBtnGap = 30;
		constexpr int kConvertFooterPadB = 20;
		constexpr int kConvertSpinW = 320;

		QStyle::StandardPixmap iconToStylePixmap(QMessageBox::Icon icon)
		{
			switch (icon)
			{
			case QMessageBox::Information: return QStyle::SP_MessageBoxInformation;
			case QMessageBox::Warning:     return QStyle::SP_MessageBoxWarning;
			case QMessageBox::Critical:    return QStyle::SP_MessageBoxCritical;
			case QMessageBox::Question:    return QStyle::SP_MessageBoxQuestion;
			default:                       return QStyle::SP_MessageBoxInformation;
			}
		}

		/*
		* 主按钮形态（原型"继续"：蓝底白字圆角 4）：取色全走 UiPalette 语义槽，
		* 三主题各自成立；hover/pressed 原型未画，按应用先例取 primary 提亮/
		* 动作色压暗。mxBtnNoTheme=ThemeManager 按钮分支豁免约定，运行时主题
		* 重发不覆盖。仅原型形态使用（通用形态默认键保持系统样）
		*/
		void makePrimaryButton(QPushButton* defaultBtn)
		{
			if (!defaultBtn)
				return;
			defaultBtn->setProperty("mxBtnNoTheme", true);
			defaultBtn->setStyleSheet(QStringLiteral(
				"QPushButton { background:%1; color:%2; border:none; border-radius:%3px; }"
				"QPushButton:hover { background:%4; }"
				"QPushButton:pressed { background:%5; }")
				.arg(MxFunctions::UiPalette::primaryHex(), MxFunctions::UiPalette::onActionHex())
				.arg(MxFunctions::UiMetrics::zoneRadius())
				.arg(MxFunctions::UiPalette::primary().lighter(108).name())
				.arg(MxFunctions::UiPalette::actionHex()));
		}

		/*
		* Qt4 字符串按钮形态装配：button0→确定位、button1→取消位、button2→应用位。
		* 返回实际按钮数；每个按钮按序号挂动态属性，点击时经属性还原返回值；
		* defaultNumber 指向的按钮设为默认（回车触发）；makeDefaultPrimary
		* 仅原型形态为 true（默认键再做主按钮形态）
		*/
		int applyStringButtons(QDialogButtonBox* buttonBox,
			const QString& button0Text, const QString& button1Text, const QString& button2Text,
			int defaultNumber, bool makeDefaultPrimary)
		{
			int count = 0;
			QPushButton* btns[3] = {
			buttonBox->button(QDialogButtonBox::Ok),
			buttonBox->button(QDialogButtonBox::Cancel),
			buttonBox->button(QDialogButtonBox::Apply) };
			const QString* texts[3] = { &button0Text, &button1Text, &button2Text };
			for (int i = 0; i < 3; ++i)
			{
				QPushButton* b = btns[i];
				if (b == nullptr || texts[i]->isEmpty())
					continue;
				b->setText(*texts[i]);
				b->setProperty("mx_strbtn_idx", QVariant(i));
				if (i == defaultNumber)
				{
					b->setDefault(true);
					// 默认键=主按钮形态；静态接口在构造（含主题下发）之后调本函数，覆盖安全
					if (makeDefaultPrimary)
						makePrimaryButton(b);
				}
				count = count + 1;
			}
			return count;
		}
	}

	mFramelessMessageBox::mFramelessMessageBox(QWidget* parent, QMessageBox::Icon icon,
		const QString& title, const QString& text, QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton, bool prototypeStyle, bool convertInputStyle)
		: mFramelessDialog(parent)
		, _prototypeStyle(prototypeStyle)
		, _convertInputStyle(convertInputStyle)
	{
		setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
		setWindowTitle(title);
		// 提示/警告窗不画标题栏下分隔线（普通弹窗保留）——三形态一致
		setTitleBarSeparatorVisible(false);
		if (_prototypeStyle || _convertInputStyle)
		{
			// 原型形态（当前仅曲线切换确认框启用）：标题栏 M 标 + 只留关闭键 +
			// 高 34（原型标题带 51÷1.5——供图系 1.5× 放大稿，真机目标 400×156，
			// 用户 2026-11-04 确认）+ 白卡家族常数（logo 14×16 防满盒巨标、
			// 标题 13px/600）。转换设置形态（getConvertCount）同族常数、标题带
			// 自带档 62（2026-11-06 新原型实测，见 kConvert* 常数组注）。
			setWindowIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/logo.png")));
			setTitleBarCloseOnly(true);
			setTitleBarHeight(_convertInputStyle ? kConvertTitleBarHeight : 34);
			setTitleBarIconSize(QSize(14, 16));
			setTitleBarFont(13, 600);
			// 转换设置形态标题排布覆写（2026-11-06 原型像素复核）：logo 左缘 20
			//（方盒档居中→左缘 24，差 4）、标题格紧随 logo（方盒档文字格 70/
			// 墨迹 72，原型墨迹 41）。关闭键=白卡弹窗族档 close3x 图标钮
			//（2026-11-09 用户裁定对齐已验收弹窗：46 宽条悬停红底系主窗档，
			// 与其它弹窗不一致，真机截图点出；此前曾按原型 ✕ 墨迹反推 62
			// 方盒+12px 字形亦翻车回退——自绘骨架勿按墨迹反推，形态照家族先例）
			if (_convertInputStyle)
				setTitleBarLayoutCompact(kBannerPadL, 5);
			setTitleBarDialogFlatClose(true);
		}
		// 通用形态（默认，全工程调用点）：标题栏随窗口头默认档（高 28、三键、
		// 无窗口图标）＝改造前样貌
		if (_convertInputStyle)
			buildConvertContent(text, buttons, defaultButton);
		else if (_prototypeStyle)
			buildPrototypeContent(icon, text, buttons, defaultButton);
		else
			buildPlainContent(icon, text, buttons, defaultButton);
	}

	void mFramelessMessageBox::buildPlainContent(QMessageBox::Icon icon, const QString& text,
		QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
	{
		// 通用形态＝改造前（git HEAD）布局原样：图标+文字一行（图标顶对齐）、
		// 按钮盒右下；边距/间距取 UI 档位公式；无主按钮形态、无固定按钮尺寸
		QWidget* content = new QWidget(this);
		QVBoxLayout* rootLay = new QVBoxLayout(content);
		const int inset = MxFunctions::UiMetrics::windowInset() * 2;
		rootLay->setContentsMargins(inset, inset / 2, inset, inset / 2);
		rootLay->setSpacing(MxFunctions::UiMetrics::baseG());

		QHBoxLayout* bodyLay = new QHBoxLayout();
		bodyLay->setSpacing(MxFunctions::UiMetrics::windowInset() * 2);

		_iconLabel = new QLabel(content);
		// 标准消息图标 32px，QStyle 按主题取
		_iconLabel->setPixmap(QApplication::style()->standardIcon(
			iconToStylePixmap(icon)).pixmap(32, 32));
		_iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
		bodyLay->addWidget(_iconLabel);

		_textLabel = new QLabel(text, content);
		_textLabel->setWordWrap(true);
		_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
		// 文字最小宽：长提示换行成窄高条很难看，与系统消息框量级一致
		_textLabel->setMinimumWidth(280);
		bodyLay->addWidget(_textLabel, 1);
		_bodyLay = bodyLay;

		rootLay->addLayout(bodyLay);

		_buttonBox = new QDialogButtonBox(content);
		_buttonBox->setStandardButtons(static_cast<QDialogButtonBox::StandardButtons>(
			static_cast<int>(buttons)));
		if (defaultButton != QMessageBox::NoButton)
		{
			if (QPushButton* db = _buttonBox->button(mapStandardButton(defaultButton)))
				db->setDefault(true);
		}
		connect(_buttonBox, &QDialogButtonBox::clicked, this, &mFramelessMessageBox::onButtonClicked);
		rootLay->addWidget(_buttonBox, 0, Qt::AlignRight);

		setContentWidget(content);
		// 内容底色/文字/按钮按当前主题上色（与首选项同链路；瞬时弹窗构造时取当前值即可）
		ThemeManager::instance()->applyDialogTheme(content);
	}

	void mFramelessMessageBox::buildPrototypeContent(QMessageBox::Icon icon, const QString& text,
		QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
	{
		// 三段式（原型）：白标题栏由基类提供 / 正文灰带（图标+文字） / 白按钮带（右下）
		QWidget* content = new QWidget(this);
		content->setMinimumWidth(kContentWidth);
		QVBoxLayout* rootLay = new QVBoxLayout(content);
		rootLay->setContentsMargins(0, 0, 0, 0);
		rootLay->setSpacing(0);

		// 正文灰带：浅色取原型实测 #F0F3F6，深色/护眼取 panel 档随主题。
		// 样式按规范写在下层容器级一次（objectName 限定 + WA_StyledBackground）：
		// 主题重发只打在本体与类型化子控件上，普通 QWidget 容器无分支、不受触碰
		QWidget* banner = new QWidget(content);
		banner->setObjectName(QStringLiteral("mxMsgBanner"));
		banner->setAttribute(Qt::WA_StyledBackground, true);
		banner->setStyleSheet(QStringLiteral("QWidget#mxMsgBanner { background:%1; }")
			.arg(MxFunctions::UiPalette::theme() == MxFunctions::UiPalette::Theme::Light
				? QStringLiteral("#F0F3F6") : MxFunctions::UiPalette::panelHex()));
		banner->setMinimumHeight(kBannerHeight);
		QHBoxLayout* bodyLay = new QHBoxLayout(banner);
		bodyLay->setContentsMargins(kBannerPadL, kBannerPadV, kBannerPadL, kBannerPadV);
		bodyLay->setSpacing(kIconTextGap);

		_iconLabel = new QLabel(banner);
		if (icon == QMessageBox::Warning)
		{
			// 警告：入库的橙色圆感叹号（原型供图裁切，圆形透明遮罩，三主题通用）
			_iconLabel->setPixmap(QIcon(QStringLiteral(":/mMainWidgets/Icon/promptWarning.png"))
				.pixmap(kIconEdge, kIconEdge));
		}
		else
		{
			// 其余类型无供图：按图标查用次序回退 QStyle 标准消息图标
			_iconLabel->setPixmap(QApplication::style()->standardIcon(
				iconToStylePixmap(icon)).pixmap(kIconEdge, kIconEdge));
		}
		_iconLabel->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
		bodyLay->addWidget(_iconLabel, 0, Qt::AlignVCenter);

		_textLabel = new QLabel(text, banner);
		_textLabel->setWordWrap(true);
		// wordWrap 标签的 sizeHint 会气球式膨胀（本框实测 hint 高被撑到 ~126，
		// 顶得灰带 166、execute 定高 269）。Ignored 策略让布局无视其 hint、按
		// 实际盒宽折行；文字在自身盒内垂直居中。（整窗宽 +16 是另一根因：Qt
		// 显示管线按"客户区+边框"下发的首帧膨胀，已在 mFramelessWindowHelper
		// 于 WM_SHOWWINDOW 后二次收拢，与本标签无关）
		_textLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
		_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		_textLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
		// 文字最小宽：长提示换行成窄高条很难看，与系统消息框量级一致
		_textLabel->setMinimumWidth(280);
		// 消息正文 13px（原型墨迹偏大；取已验收对话框内容 13px 档，色随主题文字）
		QFont bodyFont(MxFunctions::UiMetrics::uiFontFamily());
		bodyFont.setPixelSize(13);
		_textLabel->setFont(bodyFont);
		bodyLay->addWidget(_textLabel, 1);
		_bodyLay = bodyLay;

		// 灰带吸收竖向余量：高度固定 127（原型实测），超长文本/输入框时随之长高
		rootLay->addWidget(banner, 1);

		// 按钮带（底随主题 surface）：按钮右下角，44×26、间距 11、右距 9、上 4 下 5
		QWidget* footer = new QWidget(content);
		QHBoxLayout* footLay = new QHBoxLayout(footer);
		footLay->setContentsMargins(0, kFooterPadT, kFooterPadR, kFooterPadB);
		footLay->setSpacing(kBtnGap);

		_buttonBox = new QDialogButtonBox(footer);
		_buttonBox->setStandardButtons(static_cast<QDialogButtonBox::StandardButtons>(
			static_cast<int>(buttons)));
		// 钮距由按钮盒内建布局决定（实测 ~6px），外层 footLay 的 spacing 管不到
		// 它——原型钮距 16÷1.5=11（现状截图实测 6 即内建 spacing 根因），
		// 直接改盒内布局间距
		if (QBoxLayout* boxLay = qobject_cast<QBoxLayout*>(_buttonBox->layout()))
			boxLay->setSpacing(kBtnGap);
		const auto qbtns = _buttonBox->findChildren<QPushButton*>();
		for (QPushButton* b : qbtns)
			b->setFixedSize(kBtnW, kBtnH);
		if (defaultButton != QMessageBox::NoButton)
		{
			if (QPushButton* db = _buttonBox->button(mapStandardButton(defaultButton)))
				db->setDefault(true);
		}
		connect(_buttonBox, &QDialogButtonBox::clicked, this, &mFramelessMessageBox::onButtonClicked);
		footLay->addStretch(1);
		footLay->addWidget(_buttonBox);
		rootLay->addWidget(footer);

		setContentWidget(content);
		// 内容底色/文字/按钮按当前主题上色（与首选项同链路；瞬时弹窗构造时取当前值即可）
		ThemeManager::instance()->applyDialogTheme(content);
		// 默认键=主按钮形态：必须在 applyDialogTheme 之后设（主题下发整表替换按钮 QSS）
		if (defaultButton != QMessageBox::NoButton)
			makePrimaryButton(_buttonBox->button(mapStandardButton(defaultButton)));
	}

	/*
	* 转换设置形态（getConvertCount 专用）：原型标题栏族已由构造器装配（带高
	* 62 自带档+紧凑排布覆写 20/5 两参+关闭键白卡族档 close3x 图标钮）；正文纯白一行——label 13px 左、
	* QSpinBox 右侧定宽 320×24（2026-11-06 像素复核原型实测 320：左缘 141/
	* 右缘 460，旧 302 系测量偏差；20+label114+6+320+20=480 恰收整，label
	* 弹性格吃距、spin 依右缘钉住；旧 262 稿在 398 档下行溢出 390>358，一并
	* 消除）；框内无上下箭头（全局 QSS 的 up/down 子控件 NoButtons 一并不
	* 生成）、数字左衬 8（内嵌行 setTextMargins 4+内建侧衬 2；原型墨迹 150/内缘
	* 142 复测，QSS ::lineEdit 选择器无效——控件处有注）；行距按
	* 新原型（标题带下衬 10、行→钮行 30）；底部下衬 20（新原型原生实测），
	* 右衬 20 由根布局统一提供、页脚不叠加（双重衬 40 系真机按钮右缘 440
	* 偏差根因，11-06 复核）；钮 44×26 钮距 11 同家族常数咬合，继续=蓝底主键。
	* 按钮序=取消左/继续右（见常数组注）。外框 482×174（内框 480×172）。
	* 数值范围/初值由 getConvertCount 构造后经 _spinBox 设置。
	*/
	void mFramelessMessageBox::buildConvertContent(const QString& label,
		QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton)
	{
		QWidget* content = new QWidget(this);
		content->setMinimumWidth(kConvertContentWidth);
		QVBoxLayout* rootLay = new QVBoxLayout(content);
		rootLay->setContentsMargins(kBannerPadL, kConvertPadTop, kBannerPadL, 0);
		rootLay->setSpacing(kConvertRowBtnGap);

		QHBoxLayout* rowLay = new QHBoxLayout();
		rowLay->setSpacing(6);

		_textLabel = new QLabel(label, content);
		QFont bodyFont(MxFunctions::UiMetrics::uiFontFamily());
		bodyFont.setPixelSize(13);
		_textLabel->setFont(bodyFont);
		rowLay->addWidget(_textLabel);

		_spinBox = new QSpinBox(content);
		_spinBox->setFixedWidth(kConvertSpinW);
		_spinBox->setFixedHeight(24);   // 工具行控件统一高 24（白卡家族常数）
		// 原型输入框内部干净：无上下箭头；数字左衬 8（原型墨迹 150/内缘 142）。
		// 衬垫不走 QSS——::lineEdit 子控件选择器对 QSpinBox 无效（QAbstractSpinBox
		// 无此子控件，规则被静默丢弃；全局表 QSpinbox::lineEdit 另叠拼写大小写
		// 死亡，双重从未生效——11-06 真机墨迹 146 实证本表 padding 6 未起效），
		// 改走内嵌行 setTextMargins API（级联免疫；QLineEdit 内建侧衬 2+外加 4=8）。
		// 豁免注记（rules/ui-spec-mxmech §1 整树补发机制）：ThemeManager 现无
		// QSpinBox 分支，边框/焦点色由全局 QSS 级联供给；将来若仿照 QDoubleSpinBox
		// 增设 QSpinBox 分支，margins/NoButtons 属 C++ 属性不被抹，惟边框形态随新分支。
		_spinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
		if (auto* le = _spinBox->findChild<QLineEdit*>())
			le->setTextMargins(4, 0, 4, 0);
		rowLay->addWidget(_spinBox, 0, Qt::AlignVCenter);
		rootLay->addLayout(rowLay);

		// 按钮带：右下角——右衬 20 已由根布局提供（此处叠加会双重衬成 40，
		// 偏离原型按钮右缘 460），下衬 20 为本框原型档（不与确认框 9/5 通用）
		QWidget* footer = new QWidget(content);
		QHBoxLayout* footLay = new QHBoxLayout(footer);
		footLay->setContentsMargins(0, 0, 0, kConvertFooterPadB);
		footLay->setSpacing(kBtnGap);

		_buttonBox = new QDialogButtonBox(footer);
		_buttonBox->setStandardButtons(static_cast<QDialogButtonBox::StandardButtons>(
			static_cast<int>(buttons)));
		// 原型按钮序：取消在左、继续在右。QDialogButtonBox 按平台 Windows 序
		// 装配（主键在左），与原型相反——对内部盒布局换位即可，standardButton
		// 语义/默认键/主题豁免均按按钮对象定位不受影响；换位须在 add 类操作
		// 之后一次完成（盒仅在按钮增删/方向变更时重排，此处时序安全）
		if (QBoxLayout* boxLay = qobject_cast<QBoxLayout*>(_buttonBox->layout()))
		{
			boxLay->setSpacing(kBtnGap);
			if (QPushButton* cancelBtn = _buttonBox->button(QDialogButtonBox::Cancel))
				if (boxLay->indexOf(cancelBtn) > 0)
				{
					boxLay->removeWidget(cancelBtn);
					boxLay->insertWidget(0, cancelBtn);
				}
		}
		const auto qbtns = _buttonBox->findChildren<QPushButton*>();
		for (QPushButton* b : qbtns)
			b->setFixedSize(kBtnW, kBtnH);
		if (defaultButton != QMessageBox::NoButton)
		{
			if (QPushButton* db = _buttonBox->button(mapStandardButton(defaultButton)))
				db->setDefault(true);
		}
		connect(_buttonBox, &QDialogButtonBox::clicked, this, &mFramelessMessageBox::onButtonClicked);
		footLay->addStretch(1);
		footLay->addWidget(_buttonBox);
		rootLay->addWidget(footer);

		setContentWidget(content);
		ThemeManager::instance()->applyDialogTheme(content);
		// 继续=主按钮形态：必须在 applyDialogTheme 之后设（主题下发整表替换按钮 QSS）
		if (defaultButton != QMessageBox::NoButton)
			makePrimaryButton(_buttonBox->button(mapStandardButton(defaultButton)));
	}

	QDialogButtonBox::StandardButton mFramelessMessageBox::mapStandardButton(QMessageBox::StandardButton b)
	{
		// 两套枚举按 Qt 文档设计大量同值（Ok/Save/Cancel/Yes/No/...），
		// 但不承诺全覆盖（如 Abort），这里显式映射，未覆盖项退化为 Cancel
		switch (b)
		{
		case QMessageBox::Ok:          return QDialogButtonBox::Ok;
		case QMessageBox::Save:        return QDialogButtonBox::Save;
		case QMessageBox::SaveAll:     return QDialogButtonBox::SaveAll;
		case QMessageBox::Open:        return QDialogButtonBox::Open;
		case QMessageBox::Yes:         return QDialogButtonBox::Yes;
		case QMessageBox::YesToAll:    return QDialogButtonBox::YesToAll;
		case QMessageBox::No:          return QDialogButtonBox::No;
		case QMessageBox::NoToAll:     return QDialogButtonBox::NoToAll;
		case QMessageBox::Abort:       return QDialogButtonBox::No;        // 退化为否定语义
		case QMessageBox::Retry:       return QDialogButtonBox::Retry;
		case QMessageBox::Ignore:      return QDialogButtonBox::Ignore;
		case QMessageBox::Close:       return QDialogButtonBox::Close;
		case QMessageBox::Cancel:      return QDialogButtonBox::Cancel;
		case QMessageBox::Discard:     return QDialogButtonBox::Discard;
		case QMessageBox::Help:        return QDialogButtonBox::Help;
		case QMessageBox::Apply:       return QDialogButtonBox::Apply;
		case QMessageBox::Reset:       return QDialogButtonBox::Reset;
		case QMessageBox::RestoreDefaults: return QDialogButtonBox::RestoreDefaults;
		default:                       return QDialogButtonBox::Cancel;
		}
	}

	void mFramelessMessageBox::onButtonClicked(QAbstractButton* button)
	{
		// Qt4 字符串按钮形态：点击序号挂在按钮属性上（按钮盒顺序不可依赖）
		const QVariant idx = button->property("mx_strbtn_idx");
		if (idx.isValid())
		{
			_result = static_cast<QMessageBox::StandardButton>(idx.toInt());
			accept();
			return;
		}
		_result = static_cast<QMessageBox::StandardButton>(
			static_cast<int>(_buttonBox->standardButton(button)));
		accept();
	}

	QMessageBox::StandardButton mFramelessMessageBox::execute(int stringButtonCount)
	{
		_stringButtons = stringButtonCount;
		if (_prototypeStyle || _convertInputStyle)
		{
			// 原型/转换设置形态固定尺寸：宽=各自外框（确认框 400、转换设置 482，
			// 各随自家原型 2026-11-04/2026-11-06 供图），高按最终内容 sizeHint
			// （getInt/转换设置 的输入框在构造后才接入或设值，统一在此定死）；
			// 配合 Helper 的固定窗判定，窗口边缘无缩放热区。通用形态不定死尺寸
			// （沿用改造前由布局自然撑出的大小）
			const int outerW = (_convertInputStyle ? kConvertContentWidth : kContentWidth) + 2;
			setFixedSize(QSize(outerW, sizeHint().height()));
		}
		// 模态期间置顶（两形态共用）：PostCurve 弹窗族（打开曲线/模板曲线/
		// 图层管理等）统一带 WindowStaysOnTopHint 常驻置顶，而应用模态只禁
		// 输入不改层级——提示框会被其覆盖成"盖着点不了"的假死（2026-11-05
		// 用户实报：切换确认框被打开曲线弹窗盖住）。置顶仅本框存活期（静态
		// 接口栈上构造，exec 返回即析构，无需复原）；show 前置 flag，免句柄重建
		setWindowFlag(Qt::WindowStaysOnTopHint, true);
		// 相对父窗居中；父窗不可见（如临时 new 的孤儿 parent）时按鼠标所在屏居中
		QWidget* host = parentWidget() ? parentWidget()->window() : nullptr;
		if (host && host->isVisible())
			move(host->geometry().center() - QPoint(width() / 2, height() / 2));
		else if (QScreen* scr = QGuiApplication::screenAt(QCursor::pos()))
			move(scr->availableGeometry().center() - QPoint(width() / 2, height() / 2));

		show();
		raise();
		activateWindow();
		exec();
		return _result;
	}

	QMessageBox::StandardButton mFramelessMessageBox::information(QWidget* parent, const QString& title,
		const QString& text, QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton, bool prototypeStyle)
	{
		mFramelessMessageBox box(parent, QMessageBox::Information, title, text, buttons, defaultButton,
			prototypeStyle);
		return box.execute(0);
	}

	QMessageBox::StandardButton mFramelessMessageBox::warning(QWidget* parent, const QString& title,
		const QString& text, QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton, bool prototypeStyle)
	{
		mFramelessMessageBox box(parent, QMessageBox::Warning, title, text, buttons, defaultButton,
			prototypeStyle);
		return box.execute(0);
	}

	QMessageBox::StandardButton mFramelessMessageBox::critical(QWidget* parent, const QString& title,
		const QString& text, QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton, bool prototypeStyle)
	{
		mFramelessMessageBox box(parent, QMessageBox::Critical, title, text, buttons, defaultButton,
			prototypeStyle);
		return box.execute(0);
	}

	QMessageBox::StandardButton mFramelessMessageBox::question(QWidget* parent, const QString& title,
		const QString& text, QMessageBox::StandardButtons buttons,
		QMessageBox::StandardButton defaultButton, bool prototypeStyle)
	{
		mFramelessMessageBox box(parent, QMessageBox::Question, title, text, buttons, defaultButton,
			prototypeStyle);
		return box.execute(0);
	}

	void mFramelessMessageBox::about(QWidget* parent, const QString& title, const QString& text,
		bool prototypeStyle)
	{
		mFramelessMessageBox box(parent, QMessageBox::Information, title, text, QMessageBox::Ok,
			QMessageBox::Ok, prototypeStyle);
		box.execute(0);
	}

	int mFramelessMessageBox::information(QWidget* parent, const QString& title, const QString& text,
		const QString& button0Text, const QString& button1Text, const QString& button2Text,
		int defaultButtonNumber, int escapeButtonNumber, bool prototypeStyle)
	{
		QMessageBox::StandardButtons buttons = QMessageBox::Ok;
		if (!button1Text.isEmpty())
			buttons |= QMessageBox::Cancel;
		if (!button2Text.isEmpty())
			buttons |= QMessageBox::Apply;
		mFramelessMessageBox box(parent, QMessageBox::Information, title, text, buttons,
			QMessageBox::NoButton, prototypeStyle);
		const int count = applyStringButtons(box._buttonBox, button0Text, button1Text, button2Text,
			defaultButtonNumber, prototypeStyle);
		// Esc 未点任何按钮即退出：返回值取 escape 序号（与 QMessageBox 语义一致）
		box._result = static_cast<QMessageBox::StandardButton>(escapeButtonNumber);
		return static_cast<int>(box.execute(count));
	}

	int mFramelessMessageBox::warning(QWidget* parent, const QString& title, const QString& text,
		const QString& button0Text, const QString& button1Text, const QString& button2Text,
		int defaultButtonNumber, int escapeButtonNumber, bool prototypeStyle)
	{
		QMessageBox::StandardButtons buttons = QMessageBox::Ok;
		if (!button1Text.isEmpty())
			buttons |= QMessageBox::Cancel;
		if (!button2Text.isEmpty())
			buttons |= QMessageBox::Apply;
		mFramelessMessageBox box(parent, QMessageBox::Warning, title, text, buttons,
			QMessageBox::NoButton, prototypeStyle);
		const int count = applyStringButtons(box._buttonBox, button0Text, button1Text, button2Text,
			defaultButtonNumber, prototypeStyle);
		// Esc 未点任何按钮即退出：返回值取 escape 序号（与 QMessageBox 语义一致）
		box._result = static_cast<QMessageBox::StandardButton>(escapeButtonNumber);
		return static_cast<int>(box.execute(count));
	}

	int mFramelessMessageBox::critical(QWidget* parent, const QString& title, const QString& text,
		const QString& button0Text, const QString& button1Text, const QString& button2Text,
		int defaultButtonNumber, int escapeButtonNumber, bool prototypeStyle)
	{
		QMessageBox::StandardButtons buttons = QMessageBox::Ok;
		if (!button1Text.isEmpty())
			buttons |= QMessageBox::Cancel;
		if (!button2Text.isEmpty())
			buttons |= QMessageBox::Apply;
		mFramelessMessageBox box(parent, QMessageBox::Critical, title, text, buttons,
			QMessageBox::NoButton, prototypeStyle);
		const int count = applyStringButtons(box._buttonBox, button0Text, button1Text, button2Text,
			defaultButtonNumber, prototypeStyle);
		// Esc 未点任何按钮即退出：返回值取 escape 序号（与 QMessageBox 语义一致）
		box._result = static_cast<QMessageBox::StandardButton>(escapeButtonNumber);
		return static_cast<int>(box.execute(count));
	}

	int mFramelessMessageBox::getInt(QWidget* parent, const QString& title, const QString& label,
		int value, int minValue, int maxValue, int step, bool* ok)
	{
		mFramelessMessageBox box(parent, QMessageBox::Question, title, label,
			QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
		// 说明文字下方接整数输入（同一装配骨架，图标/文字/按钮盒复用）
		QWidget* bodyHost = box._textLabel->parentWidget();
		QVBoxLayout* textCol = new QVBoxLayout();
		textCol->setContentsMargins(0, 0, 0, 0);
		textCol->setSpacing(MxFunctions::UiMetrics::baseG());
		box._textLabel->setParent(nullptr);
		textCol->addWidget(box._textLabel);
		QSpinBox* spin = new QSpinBox(bodyHost);
		spin->setRange(minValue, maxValue);
		spin->setSingleStep(step);
		spin->setValue(value);
		textCol->addWidget(spin);
		box._bodyLay->addLayout(textCol, 1);

		if (ok) *ok = false;
		if (box.execute(0) == QMessageBox::Ok)
		{
			if (ok) *ok = true;
			return spin->value();
		}
		return value;
	}

	int mFramelessMessageBox::getConvertCount(QWidget* parent, const QString& title, const QString& label,
		int value, int minValue, int maxValue, bool* ok)
	{
		// 转换设置形态：text 参数位承载 label；继续=默认键（主按钮形态）
		mFramelessMessageBox box(parent, QMessageBox::NoIcon, title, label,
			QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok, false, true);
		// 原型按钮文本：取消/继续（主键"继续"已在构造中设为主按钮形态，
		// 改文本不覆盖 QSS）
		if (QPushButton* okBtn = box._buttonBox->button(QDialogButtonBox::Ok))
			okBtn->setText(QStringLiteral("继续"));
		if (QPushButton* cancelBtn = box._buttonBox->button(QDialogButtonBox::Cancel))
			cancelBtn->setText(QStringLiteral("取消"));
		box._spinBox->setRange(minValue, maxValue);
		box._spinBox->setValue(value);

		if (ok) *ok = false;
		if (box.execute(0) == QMessageBox::Ok)
		{
			if (ok) *ok = true;
			return box._spinBox->value();
		}
		return value;
	}
}
