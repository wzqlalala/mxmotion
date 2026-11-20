#pragma execution_character_set("utf-8")

// =============================================================================
// mTemplateCurveDlg —— 模板曲线应用对话框（实现）
// 结构与入口语义见 mTemplateCurveDlg.h 头注（openspec add-template-curve 3.1~3.3）
// =============================================================================

#include "mTemplateCurveDlg.h"
#include "mCurveTemplateManager.h"
#include "mTemplateApplyPipeline.h"
#include "mChannelMapDlg.h"
#include "mTemplateStartPageDlg.h"
#include "mLayerManageDlg.h"
#include "mCurveLayerManager.h"
#include "CurveCore/mCurveDocController.h"
#include "mDlgGeometry.h"

//QT
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include "mFramelessMessageBox.h"
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSizeGrip>
#include <QToolButton>
#include <QVBoxLayout>

namespace PostCurve
{
	mTemplateCurveDlg* mTemplateCurveDlg::_instance = nullptr;

	mTemplateCurveDlg::mTemplateCurveDlg(QWidget* parent)
		: QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
	{
		buildUi();
		resize(1000, 620);   //需求图：宽体三段式（列表+参数区 / 选项带 / 按钮）
		setMinimumSize(900, 540);
		DlgGeom::bind(this, "TemplateCurveDlg", DlgGeom::Mode::Geometry, QSize(1000, 620));
		_needCenter = !DlgGeom::hasSessionGeometry("TemplateCurveDlg");
	}

	mTemplateCurveDlg::~mTemplateCurveDlg()
	{
		if (_instance == this)
			_instance = nullptr;
	}

	void mTemplateCurveDlg::showInstance(QWidget* parent)
	{
		//父窗口沿链走到顶层主窗口（QMdiSubWindow 亦带 Window 标志，直接作父
		//会被 MDI 收编，对齐 mApplyStyleDlg 同款处理）
		QWidget* host = parent;
		while (host && host->parentWidget())
			host = host->parentWidget();
		if (!host)
			host = parent;

		if (!_instance)
			_instance = new mTemplateCurveDlg(host);
		_instance->reloadTemplateList();   //已打开则刷新列表并保留勾选状态

		if (_instance->_needCenter)
		{
			_instance->_needCenter = false;
			if (QWidget* pw = _instance->parentWidget())
				_instance->move(pw->frameGeometry().center()
					- QPoint(_instance->width() / 2, _instance->height() / 2));
		}
		_instance->show();
		_instance->raise();
		_instance->activateWindow();
	}

	/*************************** UI 构建 ***************************/

	void mTemplateCurveDlg::buildUi()
	{
		setAutoFillBackground(true);
		//须用 objectName 选择器：QSS 裸类名匹配不上 PostCurve:: 命名空间类
		//（对齐 mApplyStyleDlg 注释）
		setObjectName("templateCurveDlg");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet("QWidget#templateCurveDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");

		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(1, 1, 1, 1);
		outer->setSpacing(0);

		//标题栏（可拖动）：白底 + M徽标 + 深灰标题 + 深色关闭按钮
		_titleBar = new QWidget(this);
		//"mWindowTitleBar" 为 ui-spec §2 约定名：ThemeManager 整树补发按此名跳过
		//自绘标题栏及其直接子件（logo/标题 13px/600/✗ 的 QSS 不被运行时抹除；
		//工程内按此名查找均为类型锁定 findChild，普通容器挂名无副作用）
		_titleBar->setObjectName("mWindowTitleBar");
		_titleBar->setFixedHeight(52);
		_titleBar->setStyleSheet("QWidget#mWindowTitleBar{background:#FFFFFF;}");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(20, 20, 20, 10);
		titleLayout->setSpacing(3);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("模板曲线"), _titleBar);
		titleLabel->setStyleSheet(
			"QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
		QToolButton* closeBtn = new QToolButton(_titleBar);
		closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
		closeBtn->setIconSize(QSize(24, 24));
		closeBtn->setToolTip(QStringLiteral("关闭"));
		closeBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:2px; }
		)");
		//close() 而非 hide()：对齐既有弹窗关闭语义（单例实例存活，勾选状态随之会话内记忆）
		connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(closeBtn);

		//主体三段（需求图全貌）：上=左模板列表+右绘图文件区；中=选项带
		//（选项/显示、通道匹配检查、模式设置三组横排，组间竖分隔线，全宽）；
		//底=关闭/应用（右下）
		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:#FFFFFF;}");
		QHBoxLayout* bodyLayout = new QHBoxLayout(body);
		//纵向气口与左右列宽均为原型实测（原型内框 1040×640 → 对话框 1000×620，
		//×0.9615 折算）：标题栏下留白 16、面板间 16；列表卡:参数盒 = 201:783
		//→ 193:752（验收标准为等比一致，非绝对像素）
		bodyLayout->setContentsMargins(20, 16, 20, 0);
		bodyLayout->setSpacing(16);
		bodyLayout->addWidget(buildTemplatePanel(), 193);
		bodyLayout->addWidget(buildParamPanel(), 752);

		QHBoxLayout* optionRow = new QHBoxLayout();
		//原型实测：参数卡底边 y411→"选项:"墨迹顶 y430 白 18 行 ≈17px，标签行盒
		//上部余 ~4 → margin 13（旧值 8 偏紧）
		optionRow->setContentsMargins(20, 13, 20, 0);
		optionRow->addWidget(buildOptionPanel(), 1);

		QHBoxLayout* buttonRow = new QHBoxLayout();
		//原型实测：选项末行底 y574→按钮顶 y607 白 32 行 ≈31px
		buttonRow->setContentsMargins(20, 30, 20, 16);
		buttonRow->setSpacing(12);
		QPushButton* applyBtn = new QPushButton(QStringLiteral("应用"), this);
		QPushButton* closeDlgBtn = new QPushButton(QStringLiteral("关闭"), this);
		applyBtn->setFixedSize(54, 26);
		closeDlgBtn->setFixedSize(54, 26);
		closeDlgBtn->setStyleSheet(R"(
			QPushButton{
				background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
				color:#333333; padding:2px 14px;
			}
			QPushButton:hover{ background:#EFF1F2; }
		)");
		applyBtn->setStyleSheet(R"(
			QPushButton{
				background:#0066CC; border:1px solid #0066CC; border-radius:4px;
				color:#FFFFFF; padding:2px 14px;
			}
			QPushButton:hover{ background:#0073E0; }
		)");
		connect(applyBtn, &QPushButton::clicked, this, &mTemplateCurveDlg::onApplyClicked);
		connect(closeDlgBtn, &QPushButton::clicked, this, &QWidget::close);
		buttonRow->addStretch(1);
		buttonRow->addWidget(closeDlgBtn);
		buttonRow->addWidget(applyBtn);

		outer->addWidget(_titleBar);
		outer->addWidget(body, 1);
		outer->addLayout(optionRow);
		outer->addLayout(buttonRow);

		//右下角缩放手柄：不占布局行——覆盖子件贴角，resizeEvent 跟随（对齐 mApplyStyleDlg）
		_grip = new QSizeGrip(this);
		_grip->setFixedSize(_grip->sizeHint());
	}

	QWidget* mTemplateCurveDlg::buildTemplatePanel()
	{
		QWidget* panel = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(12);   //原型实测：工具行底→列表卡顶 ≈15（气口与主体留白同族）

		//工具行：打开模板 / 删除模板 / 勾(通道映射) / 图层——供图图标已到位
		//（需求图形态：纯图标小按钮，悬停沿用仓内浅灰惯例）
		QHBoxLayout* tools = new QHBoxLayout();
		tools->setSpacing(6);
		const QString toolBtnStyle = R"(
			QToolButton{
				background:transparent; border:none; border-radius:3px;
			}
			QToolButton:hover{ background:#EFF1F2; }
		)";
		auto makeToolButton = [this, &toolBtnStyle](const QString& iconRes,
			const QString& tip) {
			QToolButton* btn = new QToolButton(this);
			btn->setFixedSize(24, 24);
			btn->setIcon(QIcon(iconRes));
			btn->setIconSize(QSize(18, 18));   //供图按 18×18 设计（PNG 为 @3x 54×54）
			btn->setToolTip(tip);
			btn->setStyleSheet(toolBtnStyle);
			return btn;
		};
		QToolButton* openBtn = makeToolButton(
			QStringLiteral(":/mMainWidgets/Icon/PostCurve/dakaixiangmu3x.png"),
			QStringLiteral("打开模板：选择外部模板文件（*.mxtpl）加入列表"));
		QToolButton* delBtn = makeToolButton(
			QStringLiteral(":/mMainWidgets/Icon/PostCurve/shanchu3x.png"),
			QStringLiteral("删除模板：从列表移除选中的模板（不删除文件）"));
		//"勾"：为当前模板预开通道映射对话框（校验通道策略的应用也会自动弹）
		//（供图文件名"图例"，图形实为深灰勾，即通道映射入口图标）
		QToolButton* chanBtn = makeToolButton(
			QStringLiteral(":/mMainWidgets/Icon/PostCurve/tuli.svg"),
			QStringLiteral("勾：配置当前模板的通道映射（校验通道）"));
		//"图层"：曲线图层管理对话框（非模态）
		QToolButton* layerBtn = makeToolButton(
			QStringLiteral(":/mMainWidgets/Icon/PostCurve/tuceng.svg"),
			QStringLiteral("图层：管理模板应用生成的曲线图层"));
		connect(openBtn, &QToolButton::clicked, this,
			&mTemplateCurveDlg::onOpenTemplateClicked);
		connect(delBtn, &QToolButton::clicked, this,
			&mTemplateCurveDlg::onDeleteTemplateClicked);
		connect(chanBtn, &QToolButton::clicked, this,
			&mTemplateCurveDlg::onChannelMapClicked);
		connect(layerBtn, &QToolButton::clicked, this, [this]() {
			mLayerManageDlg::showInstance(this);
		});
		tools->addWidget(openBtn);
		tools->addWidget(delBtn);
		tools->addWidget(chanBtn);
		tools->addWidget(layerBtn);
		tools->addStretch(1);

		//模板列表：单选；时间序（旧→新，最新在底=需求图列表形态）；
		//选中态=浅蓝底+深色文字（对齐需求图高亮样式）
		_templateList = new QListWidget(panel);
		_templateList->setSelectionMode(QAbstractItemView::SingleSelection);
		//条目高 23：原型列表节距 24（行顶 121/145/169、文字偏移 8 全节距自洽，
		//选中带=整行 169..192）×0.9688（内框 1040×640→对话框 1000×620）≈23。
		//条目高 23：原型列表节距 24（行顶 121/145/169、文字偏移 8 全节距自洽，
		//选中带=整行 169..192）×0.9688（内框 1040×640→对话框 1000×620）≈23。
		//margin/height 必须三态显式钉死：全局 mApplicationStart.qss 的 item
		//26px+margin 8/8/5 会漏入（现状 31 节距带 5px 缝即其产物），:hover/:selected
		//在全局还各带 height:26，不钉则悬停/选中行反弹回 26。
		//选中项"灰边框"系已知未决问题（2026-10-29 用户裁定搁置），见 ui-spec §3
		_templateList->setStyleSheet(R"(
			QListWidget{
				background:#FFFFFF; border:1px solid #D9DCE0; border-radius:4px;
				font-size:12px; color:#333333;
			}
			QListWidget::item{ height:23px; margin:0; padding:0 8px; }
			QListWidget::item:hover{ height:23px; }
			QListWidget::item:selected{ height:23px; background:#CCE8FF; color:#333333; }
		)");
		connect(_templateList, &QListWidget::itemSelectionChanged,
			this, &mTemplateCurveDlg::onTemplateSelected);

		layout->addLayout(tools);
		layout->addWidget(_templateList, 1);
		return panel;
	}

	QWidget* mTemplateCurveDlg::buildParamPanel()
	{
		QWidget* panel = new QWidget(this);
		QVBoxLayout* layout = new QVBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(0);

		//绘图文件区（需求图行式）：带边框容器，行由 reloadParamTable 动态重建；
		//行= 绘图文件N： ☑当前参数值[路径框][浏览] 默认参数值[只读路径框]
		QWidget* box = new QWidget(panel);
		box->setObjectName("paramBox");
		//须用 objectName 选择器，避免边框样式渗到行内子件
		box->setStyleSheet("QWidget#paramBox{background:#FFFFFF;"
			"border:1px solid #D9DCE0; border-radius:4px;}");
		_paramBoxLayout = new QVBoxLayout(box);
		//内边距原型实测 7：参数盒顶边框(原型 y84/85)→行1 路径框顶(原型 y93)，
		//间隔 7 白行（×0.9615 折算 ≈6.7，取 7；四边对称取同值）
		_paramBoxLayout->setContentsMargins(7, 7, 7, 7);
		_paramBoxLayout->setSpacing(0);

		layout->addWidget(box, 1);
		return panel;
	}

	QWidget* mTemplateCurveDlg::buildOptionPanel()
	{
		QWidget* panel = new QWidget(this);
		//原型结构（2026-10-29 实测）："选项:"为独立整行标题（y430..442），三列标题
		//"显示:"/"通道匹配检查:"/"模式设置:"同排（y460..470）——旧实现把"选项:"
		//放进第一列，致中/右列标题整体抬高一行与原型错排
		QVBoxLayout* outer = new QVBoxLayout(panel);
		outer->setContentsMargins(0, 0, 0, 0);
		outer->setSpacing(10);   //"选项:"顶→列标题顶 30 行 ≈29px（标题行盒 ~19+10）

		QLabel* optCaption = new QLabel(QStringLiteral("选项:"), panel);
		optCaption->setStyleSheet(
			"QLabel{color:#333333; font-size:13px; font-weight:600; background:transparent;}");
		outer->addWidget(optCaption);

		QHBoxLayout* layout = new QHBoxLayout(panel);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(12);   //原型列内容→分隔线净隙 13 行 ≈12px

		//组间竖分隔线（需求图：三组之间各一条浅灰竖线，纵向=列标题顶→末行底
		//y456..576；取色 (220,223,230)）
		auto makeVLine = [this]() {
			QFrame* line = new QFrame(this);
			line->setFrameShape(QFrame::VLine);
			line->setStyleSheet("QFrame{color:#DCDFE6;}");
			return line;
		};

		//行节距 24（原型节距 25×0.9688）：列标题与选项行一律 setFixedHeight(24)、
		//列内 spacing 0——旧"原生控件 ~20 高 + spacing 8"组合节距 ~30 偏疏

		//第一组：显示（需求图竖排三项，默认全勾；
		//"自动放置注释"需求图无此项，不建 UI，管线侧保持 v1 默认勾行为）
		QVBoxLayout* dispCol = new QVBoxLayout();
		dispCol->setSpacing(0);
		QLabel* dispCaption = new QLabel(QStringLiteral("显示:"), panel);
		dispCaption->setFixedHeight(24);
		_autoScaleXCheck = new QCheckBox(QStringLiteral("自动缩放X轴"), panel);
		_autoScaleYCheck = new QCheckBox(QStringLiteral("自动缩放Y轴"), panel);
		_reportColorCheck = new QCheckBox(QStringLiteral("使用报告颜色"), panel);
		for (QCheckBox* c : { _autoScaleXCheck, _autoScaleYCheck, _reportColorCheck })
		{
			c->setChecked(true);   //需求图三项默认全勾（样式继承父窗口，不逐控件设）
			c->setFixedHeight(24);
		}
		dispCol->addWidget(dispCaption);
		dispCol->addWidget(_autoScaleXCheck);
		dispCol->addWidget(_autoScaleYCheck);
		dispCol->addWidget(_reportColorCheck);
		dispCol->addStretch(1);

		//第二组：通道匹配检查（需求图竖排，顺序：校验通道/选择最佳匹配/跳过验证；
		//"自动填充"需求图无此控件，不建 UI，取值恒"无"）
		QVBoxLayout* chanCol = new QVBoxLayout();
		chanCol->setSpacing(0);
		QLabel* chanCaption = new QLabel(QStringLiteral("通道匹配检查:"), panel);
		chanCaption->setFixedHeight(24);
		_checkChannelRadio = new QRadioButton(QStringLiteral("校验通道"), panel);
		_bestMatchRadio = new QRadioButton(QStringLiteral("选择最佳匹配"), panel);
		_skipVerifyRadio = new QRadioButton(QStringLiteral("跳过验证"), panel);
		//两列 radio 显式各自成组（QButtonGroup 默认互斥）：radio 同 parent 会
		//autoExclusive 串成一组，六颗只能选一、两处默认值相互顶掉——分组后
		//"跳过验证"与"追加"才可同时成立（各自组内默认）
		QButtonGroup* chanGroup = new QButtonGroup(this);
		chanGroup->addButton(_checkChannelRadio);
		chanGroup->addButton(_bestMatchRadio);
		chanGroup->addButton(_skipVerifyRadio);
		_skipVerifyRadio->setChecked(true);
		for (QRadioButton* r : { _checkChannelRadio, _bestMatchRadio, _skipVerifyRadio })
			r->setFixedHeight(24);
		chanCol->addWidget(chanCaption);
		chanCol->addWidget(_checkChannelRadio);
		chanCol->addWidget(_bestMatchRadio);
		chanCol->addWidget(_skipVerifyRadio);
		chanCol->addStretch(1);

		//第三组：模式设置（需求图竖排：叠加/追加(默认)/替换；
		//子项默认勾+灰显，仅叠加模式可用）
		QVBoxLayout* modeCol = new QVBoxLayout();
		modeCol->setSpacing(0);
		QLabel* modeCaption = new QLabel(QStringLiteral("模式设置:"), panel);
		modeCaption->setFixedHeight(24);
		_overlayRadio = new QRadioButton(QStringLiteral("叠加"), panel);
		_appendRadio = new QRadioButton(QStringLiteral("追加"), panel);
		_replaceRadio = new QRadioButton(QStringLiteral("替换"), panel);
		QButtonGroup* modeGroup = new QButtonGroup(this);
		modeGroup->addButton(_overlayRadio);
		modeGroup->addButton(_appendRadio);
		modeGroup->addButton(_replaceRadio);
		_appendRadio->setChecked(true);
		for (QRadioButton* r : { _overlayRadio, _appendRadio, _replaceRadio })
			r->setFixedHeight(24);
		_skipBaselineCheck = new QCheckBox(
			QStringLiteral("不叠加基准线/参考曲线"), panel);
		_skipBaselineCheck->setFixedHeight(24);
		_skipBaselineCheck->setChecked(true);   //需求图子项默认勾
		//子项仅叠加模式可用（需求图为灰显联动），默认追加 → 灰显
		connect(_overlayRadio, &QRadioButton::toggled,
			_skipBaselineCheck, &QCheckBox::setEnabled);
		_skipBaselineCheck->setEnabled(false);
		modeCol->addWidget(modeCaption);
		modeCol->addWidget(_overlayRadio);
		QHBoxLayout* baselineRow = new QHBoxLayout();
		baselineRow->setContentsMargins(20, 0, 0, 0);
		baselineRow->addWidget(_skipBaselineCheck);
		modeCol->addLayout(baselineRow);
		modeCol->addWidget(_appendRadio);
		modeCol->addWidget(_replaceRadio);
		modeCol->addStretch(1);

		//列宽比按原型分隔线落点折算：x278/x534 → 三段 253:246:461，扣分隔线两侧
		//净隙得 24:22:45（落点 ≈252/497 对齐原型 253/499）——旧 5:6:6+尾 stretch 4
		//把右列起点推到 ~54% 宽处，原型 ~50%
		layout->addLayout(dispCol, 24);
		layout->addWidget(makeVLine());
		layout->addLayout(chanCol, 22);
		layout->addWidget(makeVLine());
		layout->addLayout(modeCol, 45);
		outer->addLayout(layout);
		return panel;
	}

	/*************************** 模板列表 ***************************/

	void mTemplateCurveDlg::reloadTemplateList()
	{
		const QStringList recents =
			mCurveTemplateManager::getInstance()->templateRecents();

		//保留仍存在路径的选中；否则默认选最新（尾行）
		QString keep;
		if (QListWidgetItem* cur = _templateList->currentItem())
			keep = cur->data(Qt::UserRole).toString();

		_templateList->blockSignals(true);   //重建期间的 selectionChanged 不驱动参数表
		_templateList->clear();
		for (const QString& p : recents)
		{
			QListWidgetItem* item = new QListWidgetItem(QFileInfo(p).fileName(), _templateList);
			item->setData(Qt::UserRole, p);
			item->setToolTip(p);
		}
		int selRow = _templateList->count() - 1;   //默认最新（尾）
		for (int i = 0; i < _templateList->count(); ++i)
		{
			if (_templateList->item(i)->data(Qt::UserRole).toString() == keep)
			{
				selRow = i;
				break;
			}
		}
		if (selRow >= 0)
			_templateList->item(selRow)->setSelected(true);
		_templateList->setCurrentRow(selRow);
		_templateList->blockSignals(false);

		onTemplateSelected();   //手动驱动首刷（含列表为空的清空分支）
	}

	void mTemplateCurveDlg::onTemplateSelected()
	{
		_paramFiles.clear();
		_currentParamValues.clear();
		clearParamRows();
		_channelRows.clear();          //通道映射缓存按模板对齐：切模板即失效
		_channelRowsTemplate.clear();

		QListWidgetItem* cur = _templateList->currentItem();
		if (!cur)
			return;
		const QString path = cur->data(Qt::UserRole).toString();

		QStringList files;
		if (!mCurveTemplateManager::readTemplateMeta(path, nullptr, &files))
			return;   //文件失效（被外部删除等）：参数表保持空，应用时再报错
		reloadParamTable(files);
	}

	void mTemplateCurveDlg::onOpenTemplateClicked()
	{
		const QString filePath = QFileDialog::getOpenFileName(this,
			QStringLiteral("打开模板"), QString(),
			QStringLiteral("模板文件 (*.mxtpl)"));
		if (filePath.isEmpty())
			return;
		//"打开模板"把外部选择路径插入列表尾并选中（D2）
		mCurveTemplateManager::getInstance()->addTemplateRecent(
			QDir::toNativeSeparators(filePath));
		reloadTemplateList();
	}

	void mTemplateCurveDlg::onDeleteTemplateClicked()
	{
		QListWidgetItem* cur = _templateList->currentItem();
		if (!cur)
			return;
		//仅移除列表项，文件本身不删（对齐设计"仅删除当前选中的模板"）
		mCurveTemplateManager::getInstance()->removeTemplateRecent(
			cur->data(Qt::UserRole).toString());
		reloadTemplateList();
	}

	/*************************** 参数表（3.2） ***************************/

	void mTemplateCurveDlg::reloadParamTable(const QStringList& paramFiles)
	{
		_paramFiles = paramFiles;
		_currentParamValues = paramFiles;   //当前参数值初始=默认参数值
		clearParamRows();
		if (!_paramBoxLayout)
			return;
		QWidget* host = new QWidget(this);
		QVBoxLayout* rows = new QVBoxLayout(host);
		rows->setContentsMargins(0, 0, 0, 0);
		rows->setSpacing(4);   //原型实测行距 4：行1 框底(原型 y116)→行2 框顶(原型 y121)间隔 4 白行
		for (int i = 0; i < paramFiles.size(); ++i)
		{
			QHBoxLayout* rowLay = new QHBoxLayout();
			rowLay->setSpacing(6);

			QLabel* nameLbl = new QLabel(
				QStringLiteral("绘图文件%1:").arg(i + 1), host);

			//☑当前参数值（默认全勾=该行以当前参数值参与替换）
			QCheckBox* check = new QCheckBox(QStringLiteral("当前参数值"), host);
			check->setChecked(true);

			//当前参数值路径框（只读，经右侧浏览按钮换文件）
			//高度定 24px：原型实测路径框 24px（经"应用"钮 26px、勾选框 14px 双标尺 1:1 校准）；
			//sizeHint 自然值约 30px，行内与勾选框/标签高差过大，观感参差
			QLineEdit* curEdit = new QLineEdit(_currentParamValues.at(i), host);
			curEdit->setReadOnly(true);
			curEdit->setToolTip(_paramFiles.at(i));
			curEdit->setFixedHeight(24);
			curEdit->setStyleSheet(R"(
				QLineEdit{
					background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
					font-size:12px; color:#333333; padding:2px 6px;
				}
			)");

			QToolButton* browseBtn = new QToolButton(host);
			browseBtn->setIcon(QIcon(
				QStringLiteral(":/mMainWidgets/Icon/Curve/操作/打开文件.svg")));
			browseBtn->setIconSize(QSize(16, 16));
			browseBtn->setToolTip(QStringLiteral("选择替换的绘图文件"));
			browseBtn->setStyleSheet(R"(
				QToolButton{ background:transparent; border:none; }
				QToolButton:hover{ background:#EFF1F2; border-radius:3px; }
			)");
			const int idx = i;
			connect(browseBtn, &QToolButton::clicked, this,
				[this, idx]() { browseParamFile(idx); });

			QLabel* defLbl = new QLabel(QStringLiteral("默认参数值"), host);

			//默认参数值：只读灰底（模板保存时的原始路径）；高度与当前参数值框一致（原型 24px）
			QLineEdit* defEdit = new QLineEdit(paramFiles.at(i), host);
			defEdit->setReadOnly(true);
			defEdit->setToolTip(paramFiles.at(i));
			defEdit->setFixedHeight(24);
			defEdit->setStyleSheet(R"(
				QLineEdit{
					background:#F5F6F7; border:1px solid #C9CDD4; border-radius:4px;
					font-size:12px; color:#333333; padding:2px 6px;
				}
			)");

			rowLay->addWidget(nameLbl);
			rowLay->addWidget(check);
			//当前参数值框与浏览钮 0 间距贴合成一组（原型：浏览钮紧贴路径框右缘，
			//与行内其余 6px 间隙不同）；行内其它间隙仍走 rowLay 统一 spacing 6
			QHBoxLayout* curGroup = new QHBoxLayout();
			curGroup->setContentsMargins(0, 0, 0, 0);
			curGroup->setSpacing(0);
			curGroup->addWidget(curEdit, 1);
			curGroup->addWidget(browseBtn);
			rowLay->addLayout(curGroup, 5);
			rowLay->addWidget(defLbl);
			//两路径框宽度比原型实测 330:130 ≈ 2.5:1（当前参数值框为主，默认框窄）
			rowLay->addWidget(defEdit, 2);
			rows->addLayout(rowLay);

			_paramRows.append({ check, curEdit });
		}
		_paramBoxLayout->addWidget(host);
		_paramBoxLayout->addStretch(1);
	}

	//清空参数行（宿主 Widget 整体延迟删，布局项即取即删）
	void mTemplateCurveDlg::clearParamRows()
	{
		_paramRows.clear();
		if (!_paramBoxLayout)
			return;
		while (QLayoutItem* item = _paramBoxLayout->takeAt(0))
		{
			if (QWidget* w = item->widget())
			{
				w->hide();
				w->deleteLater();
			}
			delete item;
		}
	}

	void mTemplateCurveDlg::browseParamFile(int row)
	{
		if (row < 0 || row >= _currentParamValues.size() || row >= _paramRows.size())
			return;
		const QString filePath = QFileDialog::getOpenFileName(this,
			QStringLiteral("选择绘图文件"), QString(),
			QStringLiteral("绘图文件 (*.pch *.op2 *.rst *.d3plot *.h3d *.odb);;所有文件 (*.*)"));
		if (filePath.isEmpty())
			return;
		_currentParamValues[row] = QDir::toNativeSeparators(filePath);
		if (_paramRows.at(row).curEdit)
			_paramRows.at(row).curEdit->setText(_currentParamValues.at(row));
		//替换当前参数值时确保该行勾选（换文件即想让它参与替换）
		if (_paramRows.at(row).check)
			_paramRows.at(row).check->setChecked(true);
	}

	/*************************** 选项收集与应用 ***************************/

	mTemplateCurveDlg::ApplyOptions mTemplateCurveDlg::collectOptions() const
	{
		ApplyOptions opt;
		QListWidgetItem* cur = _templateList->currentItem();
		opt.templatePath = cur ? cur->data(Qt::UserRole).toString() : QString();
		//逐行当前参数值：仅勾选行参与替换（未勾行交空串=回退默认参数值语义）
		for (int i = 0; i < _paramRows.size() && i < _currentParamValues.size(); ++i)
		{
			const bool checked = _paramRows.at(i).check
				&& _paramRows.at(i).check->isChecked();
			opt.paramOverride.append(checked ? _currentParamValues.at(i) : QString());
		}
		opt.autoScaleX = _autoScaleXCheck->isChecked();
		opt.autoScaleY = _autoScaleYCheck->isChecked();
		opt.useReportColors = _reportColorCheck->isChecked();
		opt.autoAnnotation = true;   //需求图无"自动放置注释"UI 项，恒走 v1 默认勾
		opt.channelStrategy = _checkChannelRadio->isChecked() ? 1
			: (_bestMatchRadio->isChecked() ? 2 : 0);
		opt.autoFillMode = 0;   //需求图无自动填充 UI，恒"无"
		opt.applyMode = _overlayRadio->isChecked() ? 0
			: (_replaceRadio->isChecked() ? 2 : 1);
		opt.skipBaselines = _skipBaselineCheck->isChecked();
		return opt;
	}

	void mTemplateCurveDlg::onChannelMapClicked()
	{
		QListWidgetItem* cur = _templateList->currentItem();
		if (!cur)
		{
			QMessageBox::warning(this, QStringLiteral("模板曲线"),
				QStringLiteral("请先选择模板"));
			return;
		}
		const QString path = cur->data(Qt::UserRole).toString();
		//预开映射对话框：结果按模板缓存，应用（校验通道策略）直接复用；
		//自动填充档恒"无"（需求图无此 UI，恢复时在此接下拉框取值）
		QList<mChannelMapCore::Row> rows;
		QString err;
		if (!mChannelMapDlg::resolveMapping(this, path, _currentParamValues,
			0, &rows, &err))
			return;   //行提取失败/无文件来源曲线（对话框已告警）
		_channelRows = rows;
		_channelRowsTemplate = path;
	}

	void mTemplateCurveDlg::onApplyClicked()
	{
		const ApplyOptions opt = collectOptions();
		if (opt.templatePath.isEmpty())
		{
			MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("模板曲线"),
				QStringLiteral("请先选择模板"));
			return;
		}

		mTemplateApplyPipeline::Options popt;
		popt.autoScaleX = opt.autoScaleX;
		popt.autoScaleY = opt.autoScaleY;
		popt.useReportColors = opt.useReportColors;
		popt.autoAnnotation = opt.autoAnnotation;
		popt.skipBaselines = opt.skipBaselines;
		popt.channelStrategy = opt.channelStrategy;

		//叠加/替换：先选作用目标=既有模板页面组（无组告警终止，任务 4.2/4.3/4.4）
		QVector<int> targetPages;
		if (opt.applyMode != 1)
		{
			const QList<mCurveLayerManager::GroupInfo> groups =
				mCurveLayerManager::getInstance()->validGroups();
			if (groups.isEmpty())
			{
				QMessageBox::warning(this, QStringLiteral("模板曲线"),
					QStringLiteral("叠加/替换不可用：当前会话还没有模板页面组，"
						"请先用追加模式应用一次模板"));
				return;
			}
			int layerId = groups.first().layerId;
			if (groups.size() >= 2)
			{
				layerId = mTemplateStartPageDlg::pickGroup(this, groups);
				if (layerId <= 0)
					return;   //用户关闭选择框=放弃应用
			}
			for (const mCurveLayerManager::LayerInfo& info :
				mCurveLayerManager::getInstance()->layers())
			{
				if (info.id == layerId)
				{
					targetPages = info.pageIds;   //层涉及页即作用页（失效页已剔除）
					break;
				}
			}
			if (targetPages.isEmpty())
			{
				QMessageBox::warning(this, QStringLiteral("模板曲线"),
					QStringLiteral("所选页面组已失效，请刷新后重试"));
				return;
			}
		}

		//校验通道策略：优先复用"勾"预开的缓存映射，否则现场弹对话框（任务 4.6~4.8）
		if (opt.channelStrategy == 1)
		{
			if (_channelRowsTemplate != opt.templatePath || _channelRows.isEmpty())
			{
				QList<mChannelMapCore::Row> rows;
				QString err;
				if (!mChannelMapDlg::resolveMapping(this, opt.templatePath,
					opt.paramOverride, opt.autoFillMode, &rows, &err))
					return;   //行提取失败/无文件来源曲线/用户放弃
				_channelRows = rows;
				_channelRowsTemplate = opt.templatePath;
			}
			for (const mChannelMapCore::Row& row : _channelRows)
			{
				if (!row.mapped.complete)
					continue;   //未匹配行=跳过不产假数据（管线侧同防）
				popt.channelMap.insert(
					mChannelMapCore::channelKey(row.subcase, row.schema,
						row.object, row.component),
					row.mapped);
			}
		}

		//三模式分派（追加/叠加/替换，任务 4.1/4.3/4.4）
		mTemplateApplyPipeline::Result r;
		if (opt.applyMode == 0)
			r = mTemplateApplyPipeline::overlayTemplate(opt.templatePath,
				opt.paramOverride, popt, targetPages);
		else if (opt.applyMode == 2)
			r = mTemplateApplyPipeline::replaceTemplate(opt.templatePath,
				opt.paramOverride, popt, targetPages);
		else
			r = mTemplateApplyPipeline::appendTemplate(opt.templatePath,
				opt.paramOverride, popt);
		if (!r.ok)
		{
			MechMainWindow::mFramelessMessageBox::warning(this, QStringLiteral("模板曲线"),
				QStringLiteral("模板应用失败：%1").arg(r.err));
			return;
		}

		static const QStringList modeNames = {
			QStringLiteral("叠加"), QStringLiteral("追加"), QStringLiteral("替换") };
		mCurveDocController::getInstance()->reportInfo(
			QStringLiteral("[模板曲线] 已%1应用模板[%2]：涉及 %3 页 / 新建 %4 条曲线%5%6")
			.arg(modeNames.value(opt.applyMode))
			.arg(QFileInfo(opt.templatePath).completeBaseName())
			.arg(r.pageIds.size())
			.arg(r.createdCurves)
			.arg(r.skippedCurves > 0
				? QStringLiteral("（跳过 %1 条）").arg(r.skippedCurves) : QString())
			.arg(r.warnings.isEmpty() ? QString()
				: QStringLiteral("\n%1").arg(r.warnings.join(QStringLiteral("\n")))));
		close();   //帧less QWidget 弹窗：应用成功即收起
	}

	/*************************** 拖动/缩放（对齐 mApplyStyleDlg） ***************************/

	void mTemplateCurveDlg::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton && _titleBar->underMouse())
			_pressPos = event->globalPos();
		QWidget::mousePressEvent(event);
	}

	void mTemplateCurveDlg::mouseMoveEvent(QMouseEvent* event)
	{
		if ((event->buttons() & Qt::LeftButton) && !_dragging
			&& _titleBar->underMouse()
			&& (event->globalPos() - _pressPos).manhattanLength()
				>= QApplication::startDragDistance())
		{
			_dragging = true;
			_dragOffset = event->globalPos() - frameGeometry().topLeft();
		}
		if (_dragging && (event->buttons() & Qt::LeftButton))
			move(event->globalPos() - _dragOffset);
		QWidget::mouseMoveEvent(event);
	}

	void mTemplateCurveDlg::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragging = false;
		QWidget::mouseReleaseEvent(event);
	}

	void mTemplateCurveDlg::mouseDoubleClickEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
			_pressPos = event->globalPos();
	}

	void mTemplateCurveDlg::resizeEvent(QResizeEvent* event)
	{
		if (_grip)
			_grip->move(width() - _grip->width(), height() - _grip->height());
		QWidget::resizeEvent(event);
	}
}
