#include "mNVHDisplayOptionsDlg.h"

#include "mDlgGeometry.h"   //PostCurve::DlgGeom（本次运行内几何记忆）

#include "CurveWidgets/mCurveEditorWidgets.h"   //PostCurve::createCurveSciDoubleSpin（dB 参考值家族件）
#include "UiMetrics.h"      //MxFunctions::UiMetrics::zoneRadius（主钮圆角档）
#include "UiPalette.h"      //MxFunctions::UiPalette 语义槽（主钮取色三主题各自成立）
#include <qt_windows.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace PostWindow
{
	namespace
	{
		/*
		* 主按钮形态（蓝底白字圆角 4）：照抄 mFramelessMessageBox::makePrimaryButton
		* 家族先例——取色全走 UiPalette 语义槽，三主题各自成立；hover/pressed 原型
		* 未画按应用先例（primary 提亮/动作色压暗）。mxBtnNoTheme=ThemeManager 按钮
		* 分支豁免约定，运行时主题重发不覆盖（ui-spec §1）
		*/
		void makePrimaryButton(QPushButton* primaryBtn)
		{
			primaryBtn->setProperty("mxBtnNoTheme", true);
			primaryBtn->setStyleSheet(QStringLiteral(
				"QPushButton { background:%1; color:%2; border:none; border-radius:%3px; }"
				"QPushButton:hover { background:%4; }"
				"QPushButton:pressed { background:%5; }")
				.arg(MxFunctions::UiPalette::primaryHex(), MxFunctions::UiPalette::onActionHex())
				.arg(MxFunctions::UiMetrics::zoneRadius())
				.arg(MxFunctions::UiPalette::primary().lighter(108).name())
				.arg(MxFunctions::UiPalette::actionHex()));
		}
	}

	mNVHDisplayOptionsDlg::mNVHDisplayOptionsDlg(NVHDisplayCtx ctx,
		const NVHContextScale& ctxScale, const NVHDisplayOptions& common,
		QWidget* parent)
		: MechMainWindow::mFramelessDialog(parent)
	{
		setWindowTitle(QStringLiteral("显示选项"));

		//白卡弹窗族标题栏（原型 02 实测与转换设置形态同档：带高 62、logo 14×16
		//左缘 20 紧排、标题 13px/600、仅✗关闭钮、无标题栏下分隔线；常数与接线
		//照 mFramelessMessageBox 原型形态块，mxBtnNoTheme 豁免约定见各控件）
		setWindowIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/logo.png")));
		setTitleBarSeparatorVisible(false);
		setTitleBarCloseOnly(true);
		setTitleBarHeight(62);
		setTitleBarIconSize(QSize(14, 16));
		setTitleBarFont(13, 600);
		setTitleBarLayoutCompact(20, 5);
		setTitleBarDialogFlatClose(true);

		//工作副本：确定才回写（取消不改变已存设置，spec）
		_ctxScale = ctxScale;
		_common = common;
		_ctx = ctx;

		buildUi();
		//原型 02 整窗 480×493（标题带 62 + 内容 429）。注意：可缩放白卡弹窗
		//真机可见卡=编程尺寸+16×39（Qt 建窗按"客户区+原生框"预留、WM_NCCALCSIZE
		//清零后膨胀奉为正统——FramelessWindowHelper 首帧对齐注释在案，家族可缩放
		//档同款，如首选项 800×600 显示 816×639），本框真机即 496×532
		resize(480, 493);
		PostCurve::DlgGeom::bind(this, "NVHDisplayOptionsDlg",
			PostCurve::DlgGeom::Mode::Geometry);
	}

	void mNVHDisplayOptionsDlg::onAccept()
	{
		//控件态 -> 工作副本（spec：确定应用到后续生成）
		_common.displayPhase = (_dispCombo->currentIndex() == 1);
		if (_scaleLogRadio->isChecked())
			_ctxScale.scale = NVHScale_Log;
		else if (_scalePercentRadio->isChecked())
			_ctxScale.scale = NVHScale_PercentPlot;
		else if (_scaleDb10Radio->isChecked())
			_ctxScale.scale = NVHScale_dB10;
		else if (_scaleDb20Radio->isChecked())
			_ctxScale.scale = NVHScale_dB20;
		else
			_ctxScale.scale = NVHScale_Linear;
		//dB 参考值：科学计数框范围内恒正（1e-12~1e9 家族口径），value() 直取——
		//非法键入 spinbox 自行保持原值（与旧"非法/空串保留原值"语义等价）
		_ctxScale.db10Ref = _db10RefSpin->value();
		_ctxScale.db20Ref = _db20RefSpin->value();
		_common.weightingOn = _weightCheck->isChecked();
		_common.weightMode = _weightCombo->currentIndex() + 1;
		_common.weightingGlobal = _weightGlobalCheck->isChecked();
		_common.unitsOn = _unitsCheck->isChecked();
		_common.unitTime = _timeCombo->currentIndex();
		_common.unitLength = _lengthCombo->currentIndex();
		_common.unitForce = _forceCombo->currentIndex();
		//绘图布局下拉直映 layout（0 当前视图页新窗口/1 当前激活窗口/2 新视图页，
		//与 NVHDisplayOptions::layout 口径一致）
		_common.layout = _layoutCombo->currentIndex();
		_common.layoutGlobal = _layoutGlobalCheck->isChecked();
		accept();
	}

	void mNVHDisplayOptionsDlg::buildUi()
	{
		//行结构（原型 02 实测 480×493，折算 480×489；行控件统一高 24——白卡
		//家族常数；主行距 18、缩放带节距 31=24+7、底钮行前气口 13）：显示行 /
		//缩放带（带级标题"缩放:"独立整行 + 五单选竖排，dB10/dB20 行内参考值
		//框）/ 加权行 / 单位行 / 绘图布局行 / 底钮行；标签与控件同排自然流排
		//（原型控件左缘 56/90/132/80，非固定标签列），行内 spacing 12 承担缝差
		auto* lay = new QVBoxLayout(this);
		lay->setContentsMargins(20, 10, 20, 20);
		lay->setSpacing(18);

		//—— 行1 显示：原型=下拉框 184×24（旧两单选撤销）；两态随图表类型联动
		//（柱状/2D=相位形态；极坐标=首尾相接，读值同为 displayPhase） ——
		//语境判据（D2 规格表）：极坐标=仅 Linear；柱状=档集 {Linear,%Plot}；
		//加权置灰=柱状+极坐标两语境；显示行文案=极坐标语境首尾相接族
		const bool polar = (_ctx == NVHCtx_ShowPolar);
		const bool bar = (_ctx == NVHCtx_ShowBar);
		auto* dispRow = new QHBoxLayout();
		dispRow->setSpacing(12);
		auto* dispLabel = new QLabel(QStringLiteral("显示"));
		_dispCombo = new QComboBox();
		_dispCombo->insertItem(0, polar ? QStringLiteral("非首尾相接")
			: QStringLiteral("不显示相位"));
		_dispCombo->insertItem(1, polar ? QStringLiteral("首尾相接")
			: QStringLiteral("显示相位"));
		_dispCombo->setCurrentIndex(_common.displayPhase ? 1 : 0);
		_dispCombo->setFixedSize(184, 24);
		dispRow->addWidget(dispLabel);
		dispRow->addWidget(_dispCombo);
		dispRow->addStretch(1);
		lay->addLayout(dispRow);

		//—— 行2 缩放：带级标题"缩放:"独立整行 + 五单选竖排（原型节距 31=24+7），
		//dB10/dB20 行内参考值框 151×24（原型左缘 rel 89=20+50+19）；极坐标语境
		//后四档置灰仅 Linear 可选（spec 置灰语义） ——
		auto* scaleBand = new QVBoxLayout();
		scaleBand->setSpacing(7);
		//带级标题档（ui-spec-mxmech §4）：13px/600、全角冒号、独立整行——
		//构造期 QFont 设定（非 QSS，运行时主题重发不触碰；色随主题 QLabel 规则）
		auto* scaleLabel = new QLabel(QStringLiteral("缩放："));
		{
			QFont bandFont = scaleLabel->font();
			bandFont.setPixelSize(13);
			bandFont.setWeight(QFont::DemiBold);
			scaleLabel->setFont(bandFont);
		}
		scaleLabel->setFixedHeight(24);
		scaleBand->addWidget(scaleLabel);
		_scaleLinearRadio = new QRadioButton(QStringLiteral("Linear"));
		_scaleLinearRadio->setFixedHeight(24);
		_scaleLogRadio = new QRadioButton(QStringLiteral("Logarithmic"));
		_scaleLogRadio->setFixedHeight(24);
		_scalePercentRadio = new QRadioButton(QStringLiteral("% Plot"));
		_scalePercentRadio->setFixedHeight(24);
		_scaleDb10Radio = new QRadioButton(QStringLiteral("dB10"));
		_scaleDb10Radio->setFixedHeight(24);
		_scaleDb20Radio = new QRadioButton(QStringLiteral("dB20"));
		_scaleDb20Radio->setFixedHeight(24);
		_scaleGroup = new QButtonGroup(this);
		for (QRadioButton* radio : { _scaleLinearRadio, _scaleLogRadio,
			_scalePercentRadio, _scaleDb10Radio, _scaleDb20Radio })
		{
			_scaleGroup->addButton(radio);
		}
		_db10RefSpin = PostCurve::createCurveSciDoubleSpin(1, 1e-12, 1e9, 1.0, this);
		_db10RefSpin->setFixedSize(151, 24);
		//回显当前参考值（原型默认态 dB10/dB20 框显示 1.0——2026-11-11 标注稿
		//真机对照修：初版只建框不回显，开框两框全空）；displayDecimals=1 整数
		//形态一位小数呈现（"1.0"与 NVHContextScale 轴后缀的 Ref 文本
		//口径一致；极小值走科学计数法分支自动带量级）
		_db10RefSpin->setValue(_ctxScale.db10Ref);
		auto* db10Row = new QHBoxLayout();
		db10Row->setSpacing(18);
		db10Row->addWidget(_scaleDb10Radio);
		db10Row->addWidget(_db10RefSpin);
		db10Row->addStretch(1);
		_db20RefSpin = PostCurve::createCurveSciDoubleSpin(1, 1e-12, 1e9, 1.0, this);
		_db20RefSpin->setFixedSize(151, 24);
		_db20RefSpin->setValue(_ctxScale.db20Ref);
		auto* db20Row = new QHBoxLayout();
		db20Row->setSpacing(18);
		db20Row->addWidget(_scaleDb20Radio);
		db20Row->addWidget(_db20RefSpin);
		db20Row->addStretch(1);
		scaleBand->addWidget(_scaleLinearRadio);
		scaleBand->addWidget(_scaleLogRadio);
		scaleBand->addWidget(_scalePercentRadio);
		scaleBand->addLayout(db10Row);
		scaleBand->addLayout(db20Row);
		const auto echoScale = [&]()
		{
			//语境存量档原样回显（context-display-options-state：折算删除——
			//每语境存量恒在域内）
			switch (_ctxScale.scale)
			{
			case NVHScale_Log:          _scaleLogRadio->setChecked(true); break;
			case NVHScale_PercentPlot:  _scalePercentRadio->setChecked(true); break;
			case NVHScale_dB10:         _scaleDb10Radio->setChecked(true); break;
			case NVHScale_dB20:         _scaleDb20Radio->setChecked(true); break;
			default:                    _scaleLinearRadio->setChecked(true); break;
			}
		};
		echoScale();
		if (polar)
		{
			//spec 置灰语义：仅 Linear 可选（design 69 维持；参考值框随行可用性
			//在下方向步设定——design D30③）
			_scaleLogRadio->setEnabled(false);
			_scalePercentRadio->setEnabled(false);
			_scaleDb10Radio->setEnabled(false);
			_scaleDb20Radio->setEnabled(false);
		}
		else if (bar)
		{
			//spec 置灰语义：柱状图仅 Linear/%Plot 可选（design D12 维持——
			//大图 02 02段+2026-11-17 柱状语境原型：Logarithmic/dB10/dB20 三档置灰）
			_scaleLogRadio->setEnabled(false);
			_scaleDb10Radio->setEnabled(false);
			_scaleDb20Radio->setEnabled(false);
		}
		//dB 参考值框可用性=所在行单选钮可用性（build 期一次设定，不随选中态
		//切换——大图 01/02/08 与极坐标原型四图一致形态，design D30③；未绑定
		//语境与 2D 语境全行可用=恒可编辑）
		_db10RefSpin->setEnabled(_scaleDb10Radio->isEnabled());
		_db20RefSpin->setEnabled(_scaleDb20Radio->isEnabled());
		lay->addLayout(scaleBand);

		//—— 行3 加权：原型=勾选框自带行首文案（无独立标签），A/B/C/U 下拉
		//282×24（原型实测宽），设为全局右贴内容区右缘 ——
		auto* weightRow = new QHBoxLayout();
		weightRow->setSpacing(12);
		_weightCheck = new QCheckBox(QStringLiteral("加权"));
		_weightCheck->setChecked(_common.weightingOn);
		_weightCombo = new QComboBox();
		_weightCombo->addItems({ QStringLiteral("A"), QStringLiteral("B"),
			QStringLiteral("C"), QStringLiteral("U") });
		_weightCombo->setFixedSize(282, 24);
		_weightCombo->setCurrentIndex(qBound(0, _common.weightMode - 1, 3));
		_weightGlobalCheck = new QCheckBox(QStringLiteral("设为全局"));
		_weightGlobalCheck->setChecked(_common.weightingGlobal);
		if (polar || bar)
		{
			//柱状/极坐标语境加权行置灰（2026-11-13 用户下发极坐标原型+大图 02
			//一致画灰，用户裁定跟原型——design D30②）：灰显=只读呈现，回显值
			//照常随确定回写；极坐标 Linear 产出不消费计权（2026-11-16 原型逐字
			//实证+用户裁定，渲染契约处屏蔽），柱状维持原语义；设为全局豁免保持可用
			_weightCheck->setEnabled(false);
			_weightCombo->setEnabled(false);
		}
		weightRow->addWidget(_weightCheck);
		weightRow->addWidget(_weightCombo);
		weightRow->addStretch(1);
		weightRow->addWidget(_weightGlobalCheck);
		lay->addLayout(weightRow);

		//—— 行4 单位：原型=勾选框"单位:"行首（原"标签+勾选框"双文案撤销），
		//时间/长度/力三下拉 72/72/84×24（一期仅存档，换算语义随真机复核） ——
		auto* unitsRow = new QHBoxLayout();
		unitsRow->setSpacing(12);
		_unitsCheck = new QCheckBox(QStringLiteral("单位："));
		_unitsCheck->setChecked(_common.unitsOn);
		unitsRow->addWidget(_unitsCheck);
		const auto addUnitCombo = [&unitsRow](QComboBox** combo,
			const QString& label, const QStringList& items, int w)
		{
			auto* lab = new QLabel(label);
			unitsRow->addWidget(lab);
			*combo = new QComboBox();
			(*combo)->addItems(items);
			(*combo)->setFixedSize(w, 24);   //行控件统一高 24（白卡家族常数）
			unitsRow->addWidget(*combo);
		};
		addUnitCombo(&_timeCombo, QStringLiteral("时间"),
			{ QStringLiteral("s"), QStringLiteral("ms") }, 72);
		addUnitCombo(&_lengthCombo, QStringLiteral("长度"),
			{ QStringLiteral("mm"), QStringLiteral("m") }, 72);
		addUnitCombo(&_forceCombo, QStringLiteral("力"),
			{ QStringLiteral("N"), QStringLiteral("kN") }, 84);
		_timeCombo->setCurrentIndex(_common.unitTime);
		_lengthCombo->setCurrentIndex(_common.unitLength);
		_forceCombo->setCurrentIndex(_common.unitForce);
		unitsRow->addStretch(1);
		lay->addLayout(unitsRow);

		//—— 行5 绘图布局：原型=下拉框 292×24（旧三单选撤销），设为全局右贴
		//内容区右缘；下拉直映 layout 0/1/2 与 NVHDisplayOptions 口径一致 ——
		auto* layoutRow = new QHBoxLayout();
		layoutRow->setSpacing(12);
		auto* layoutLabel = new QLabel(QStringLiteral("绘图布局"));
		_layoutCombo = new QComboBox();
		_layoutCombo->addItems({ QStringLiteral("在当前视图页上打开新窗口"),
			QStringLiteral("在当前激活的窗口打开"),
			QStringLiteral("在新视图页上打开新窗口") });
		_layoutCombo->setCurrentIndex(_common.layout);
		_layoutCombo->setFixedSize(292, 24);
		_layoutGlobalCheck = new QCheckBox(QStringLiteral("设为全局"));
		_layoutGlobalCheck->setChecked(_common.layoutGlobal);
		layoutRow->addWidget(layoutLabel);
		layoutRow->addWidget(_layoutCombo);
		layoutRow->addStretch(1);
		layoutRow->addWidget(_layoutGlobalCheck);
		lay->addLayout(layoutRow);

		//—— 底钮行：取消/确定 44×26（原型实测；右对齐、右边距 20=布局边距；
		//确定=主钮形态 makePrimaryButton 家族先例） ——
		auto* btnRow = new QHBoxLayout();
		btnRow->setSpacing(13);
		btnRow->addStretch(1);
		_cancelBtn = new QPushButton(QStringLiteral("取消"));
		_cancelBtn->setFixedSize(44, 26);
		_okBtn = new QPushButton(QStringLiteral("确定"));
		_okBtn->setFixedSize(44, 26);
		_okBtn->setDefault(true);
		makePrimaryButton(_okBtn);
		btnRow->addWidget(_cancelBtn);
		btnRow->addWidget(_okBtn);
		lay->addLayout(btnRow);

		setExternalLayout(lay, true);   //内容区按当前主题上色（家族同链路）
		connect(_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
		connect(_okBtn, &QPushButton::clicked, this,
			&mNVHDisplayOptionsDlg::onAccept);
	}
}


