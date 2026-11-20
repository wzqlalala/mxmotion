#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mExportReportDlg.h"

#include "mPostReportRenderer.h"
#include "mPostExportAPIRegistrar.h"
#include "CurveView/mCurveChartView.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"

//Qt
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QEvent>
#include <QEventLoop>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QPixmap>
#include <QRegularExpression>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "mPostReportContext.h"
#include "../IOFile/mIOFileManager.h"   //计算文件名（与前IOFile版 MechanIOFile 区分）
#include "mGlobalSignals.h"             //outputMessageSig 消息输出

namespace
{
	//主按钮（导出）：产品主蓝（同 mOpenCurveDlg 主操作按钮配色）
	const char* kPrimaryBtnStyle = R"(
		QPushButton{
			background:#1385F6; border:1px solid #1385F6; border-radius:4px;
			color:#FFFFFF; padding:4px 10px;
		}
		QPushButton:hover{ background:#3D9EF8; }
		QPushButton:pressed{ background:#0F6ACB; }
		QPushButton:disabled{ background:#9CC3E5; border:1px solid #9CC3E5; }
	)";
	//次按钮（关闭）：白底描边（同 mEditCurveDlg 底部按钮行）
	const char* kNormalBtnStyle = R"(
		QPushButton{
			background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
			color:#333333; padding:4px 10px;
		}
		QPushButton:hover{ background:#EFF1F2; }
	)";
	//输入控件：白底描边圆角（对齐设计图01.1 的输入行样式）
	const char* kInputStyle = R"(
		QComboBox, QLineEdit{
			background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;
			color:#333333; padding-left:8px; padding-right:8px;
		}
		QComboBox:hover, QLineEdit:hover{ border:1px solid #1385F6; }
		QComboBox:focus, QLineEdit:focus{ border:1px solid #1385F6; }
		QComboBox::drop-down{ border:none; width:24px; }
	)";
	//置灰输入控件（模板路径行在"默认模板"下不可用时的态）
	const char* kInputStyleDisabled = R"(
		QComboBox, QLineEdit{
			background:#F5F6F7; border:1px solid #C9CDD4; border-radius:4px;
			color:#999999; padding-left:8px; padding-right:8px;
		}
		QComboBox::drop-down{ border:none; width:24px; }
	)";

	//PPT 模板整页图插入框实测几何（EMU，design/report-export/报告母版.pptx 页3
	//量取，与 export_pptx.py TPL_PAGEIMG_XYWH 同源）：20261102-R08 起整页拼合
	//画布固定为该框宽高比（567:286，12700=两值最大公约数），PPT 侧整图入框
	//不再二次变形；画布分辨率取该比例整数倍 ×10（5670x2860，两维皆偶，避免
	//奇数尺寸），不再随图窗卡片几何变化（根治导出源图尺寸不一致）
	const int kPageFrameW = 5670;   // = 7200900 EMU / 12700 x 10
	const int kPageFrameH = 2860;   // = 3632200 EMU / 12700 x 10

	//导出源图统一渲染尺寸（20261103-R10）：采集层不再 grab 界面卡片，改由临时
	//离屏视图按此尺寸重渲染每个图表。1515x765（1.98:1）取报告容器整幅尺寸——
	//旧行为中"未显示页"的来源尺寸；全部图表统一按它出图后，同字号文字的像素
	//尺寸与深浅跨页跨图一致（旧行为显示页卡片平铺 755x368 文字占比大、未显示页
	//1515x765 文字占比小，R08 拉伸充满放大了该不一致）。只改截图来源，布局
	//跟随/拉伸充满/compose 画布等既有行为全部不变
	const int kExportRenderW = 1515;
	const int kExportRenderH = 765;

	//把一页的全部图窗截图程序化拼合为单张整页大图（宫格，画布=固定框比例
	//kPageFrameW x kPageFrameH，列数=页面布局派生列数 cols（20261102-R07
	//跟随页面布局选择器；0/负=自动 3 列），行数=⌈N/cols⌉，图窗按页内视口序
	//行主序填格、空格留白，每图**非等比拉伸**充满自己的格子——20261102-R08
	//拉伸充满（取代 R06/R07 等比缩放置中与"单元格取图窗最大宽高"的画布
	//派生方式；格子越接近图窗比例变形越小，用户已知并接受）——确定性产物，
	//不依赖界面平铺状态，效果对齐母版 pptx 页3 插入框比例
	bool composePageImage(const QList<QPixmap>& grabs, const QString& imgPath, int cols = 0)
	{
		if (grabs.isEmpty())
			return false;
		if (cols <= 0)
			cols = 3;
		//单图页独占整画布（20261102-R08 §1"单视图页该图直接拉伸充满插入框"，
		//与三渲染器脚本勾选态 N=1 处理一致；否则 3 列画布左侧 1/3 之外全留白，
		//整图入框后图仍缩在框左——旧"视图页-2 单图缩左上"现象的根因之一）
		if (grabs.size() == 1)
			cols = 1;
		const int rows = (grabs.size() + cols - 1) / cols;
		QImage pageImg(kPageFrameW, kPageFrameH, QImage::Format_RGB32);
		pageImg.fill(Qt::white);
		QPainter painter(&pageImg);
		painter.setRenderHint(QPainter::SmoothPixmapTransform);
		for (int i = 0; i < grabs.size(); ++i)
		{
			const int r = i / cols;
			const int c = i % cols;
			//格子边界按累加分段取整（除不尽时余量归末行/末列），保证格子
			//并集恰为整画布（无缝隙重叠）
			const int x0 = c * kPageFrameW / cols;
			const int x1 = (c + 1) * kPageFrameW / cols;
			const int y0 = r * kPageFrameH / rows;
			const int y1 = (r + 1) * kPageFrameH / rows;
			painter.drawPixmap(QRect(x0, y0, x1 - x0, y1 - y0), grabs[i]);
		}
		painter.end();
		return pageImg.save(imgPath, "PNG");
	}

	//离屏绘制准备（缺陷 20261030-B01-2）：懒物化后从未上屏的视图处于隐藏态，
	//Qt 对隐藏控件的布局激活是推迟的——resize 不级联到引擎 QwtPlot/画布（离屏
	//对照实测画布停在构造期尺寸 100x168），其上宿主自绘的闭合边框覆盖层
	//（mCurveChartView::BorderOverlay）随之停在旧几何：grab 得到"绘图区左上角
	//一只空画布小框（带边框）+整页无画布外框"的残图。置 WA_DontShowOnScreen
	//后 show，可在不闪界面/不抢焦点/不改用户当前激活视口与缩放的前提下，把
	//布局级联、polish 与画布 Resize 挂起的 0ms 延迟同步（边框覆盖层/副轴条带
	//几何，经 QTimer::singleShot(0)）完整走完；已可见视图零操作（已显示页
	//回归基线不变）。离屏对照（链接真实 PostCurve 模块，offscreen 平台）：
	//未显示页修复前与显示页差 116 万像素，修复后逐像素一致
	//
	//B02（20261102）真实界面实证补正：真实容器形态中，attach 物化即触发
	//Resize 级联（视图/引擎/QwtPlot/画布 4 连发，离屏 harness 实测），视图
	//show 后已可见——本守卫的 show+事件循环对其早退（"等到了寂寞"），画布
	//Resize 挂起的 0ms 延迟同步仍压在事件队列里（导出物化→grab 全程同步、
	//不进事件循环），BorderOverlay 停留构造期几何（实测 147x172 vs 画布
	//871x634）。故守卫无论走哪条路径，收尾都显式调用
	//mCurveChartView::syncSelfDrawnGeometry()（幂等：已同步视图为同值写短路，
	//对已显示页零干扰）——不赌 show 级联/事件巧合，见该方法的实现注释
	//
	//R10（20261103）守卫语义迁移：采集层改为临时离屏视图标准尺寸重渲染
	//（见 grabChartStandard），不再借用/物化界面卡片视图——B01/B02 时代
	//"已可见视图零操作"的还原分支在产品路径上不再被走到（临时视图恒为
	//隐藏态构造，恒走 show+事件循环+显式同步全链路），机制原样保留服务
	//临时视图（还原分支对"构造即隐藏"的临时视图亦按原态精确还原，无副作用）
	class OffscreenPaintReady
	{
	public:
		explicit OffscreenPaintReady(QWidget* view)
		{
			_view = view;
			if (view == nullptr)
				return;
			if (!view->isVisible())
			{
				_hidden = true;
				//记录自身隐藏位原态（isHidden 与祖先显隐无关）：析构按原态精确还原。
				//不能无条件 hide()——视图自身可见但祖先隐藏时导出，若退出时 hide()
				//会清掉自身可见位，祖先恢复后该视图不再重现（状态污染）
				_wasSelfHidden = view->isHidden();
				view->setAttribute(Qt::WA_DontShowOnScreen);
				view->show();
				//兑现 show 级联里挂起的 0ms 延迟同步；ExcludeUserInputEvents：
				//导出期间不消费任何用户输入（不抢交互）。
				//两轮=当前已核实延迟链的链深上限：画布 Resize→scheduleAxisStripLayout
				//0ms→updateAxisStrips 同步收尾，右副轴条带几何变更再链式一跳；
				//引擎未来加长 0ms 延迟链需同步调大此处轮数
				for (int i = 0; i < 2; ++i)
				{
					QEventLoop loop;
					QTimer::singleShot(0, &loop, &QEventLoop::quit);
					loop.exec(QEventLoop::ExcludeUserInputEvents);
				}
			}
			//B02：显式几何同步收尾（两种路径都走；幂等见上注）
			if (auto* chartView = qobject_cast<PostCurve::mCurveChartView*>(view))
				chartView->syncSelfDrawnGeometry();
		}
		~OffscreenPaintReady()
		{
			//按 enter 原态精确还原：自身原隐藏才 hide 回去，自身原可见仅还原
			//属性（可见位保持，祖先恢复显隐后自然重现）；属性必还原——离屏
			//语义只在本次截图内有效
			if (!_hidden || _view.isNull())
				return;
			if (_wasSelfHidden)
				_view->hide();
			_view->setAttribute(Qt::WA_DontShowOnScreen, false);
		}
	private:
		QPointer<QWidget> _view;
		bool _hidden = false;
		bool _wasSelfHidden = false;
	};

	//单图表标准尺寸离屏重渲染（20261103-R10 导出截图分辨率统一）：构造临时
	//mCurveChartView 绑定目标图表节点，统一按 kExportRenderW x kExportRenderH
	//重渲染后 grab，随即销毁——采集层与界面卡片彻底解耦。
	//零界面干扰：临时视图**不进视口工厂登记**（无卡片/无 attach/无 LRU 登记，
	//不影响任何现有卡片的大小/显隐/缩放/激活态），不改变文档（构造期 rebuild
	//只读模型），离屏渲染（WA_DontShowOnScreen）后即析构；同一图表可临时视图
	//与界面视图短暂共存（二者都是模型的单向投影，互不通信）。
	//就绪链路复用 B02 机制：临时视图同为"从未显示→布局未激活/自绘层不同步"
	//形态（B01/B02 同源问题），resize→OffscreenPaintReady（WA_DontShowOnScreen
	//+show+事件循环兑现 0ms 延迟链+syncSelfDrawnGeometry 显式收口）→grab，
	//与旧守卫验证过的链路逐句一致。
	//与旧路径（materializeChart 借用界面视图）的差异只在截图来源与分辨率：
	//隐藏图表（kChartVisible=false）在临时视图下不受卡片显隐影响、按完整内容
	//渲染导出（旧路径同渲染导出、尺寸兜底 800x600；是否跳过隐藏图表另行立规）。
	//性能：逐图表构造/销毁临时视图与旧路径 materialize→release 同构（旧路径
	//同样逐图表构建+销毁视图），无架构性开销劣化；实测见 workspace
	//tools/r12_harness 证据
	QPixmap grabChartStandard(int chartId)
	{
		PostCurve::mCurveChartView view(chartId);
		view.resize(kExportRenderW, kExportRenderH);
		OffscreenPaintReady ready(&view);
		return view.grab();
	}
}

namespace PostWindow
{
	void mExportReportDlg::showInstance(QWidget* parent)
	{
		static QPointer<mExportReportDlg> _dlg;
		if (_dlg.isNull())
			_dlg = new mExportReportDlg(parent);
		_dlg->setWindowModality(Qt::ApplicationModal);
		_dlg->show();
		_dlg->raise();
		_dlg->activateWindow();
	}

	mExportReportDlg::mExportReportDlg(QWidget* parent)
		//导出期间独占操作（渲染为同步过程）：应用级模态
		: QWidget(parent, Qt::Window | Qt::FramelessWindowHint)
	{
		setObjectName("exportReportDlg");
		setAttribute(Qt::WA_StyledBackground, true);
		//白底细描边（无边框弹窗轮廓，同 mEditCurveDlg 的处理；objectName 选择器：
		//QSS 裸类名匹配不上带命名空间的 metaObject）
		setStyleSheet("QWidget#exportReportDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
		setFixedSize(480, 300);

		buildUi();
	}

	void mExportReportDlg::buildUi()
	{
		QVBoxLayout* outer = new QVBoxLayout(this);
		outer->setContentsMargins(1, 1, 1, 1);
		outer->setSpacing(0);

		//--- 标题栏：M徽标 + "输出报告" + 关闭按钮（设计图01.1） ---
		_titleBar = new QWidget(this);
		_titleBar->setObjectName("exportReportTitleBar");
		_titleBar->setFixedHeight(44);
		_titleBar->setStyleSheet("QWidget#exportReportTitleBar{background:#FFFFFF;}");
		QHBoxLayout* titleLayout = new QHBoxLayout(_titleBar);
		titleLayout->setContentsMargins(14, 0, 8, 0);
		titleLayout->setSpacing(4);
		QLabel* logoLabel = new QLabel(_titleBar);
		logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
			.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
		QLabel* titleLabel = new QLabel(QStringLiteral("输出报告"), _titleBar);
		titleLabel->setStyleSheet(
			"QLabel{color:#333333; font-size:15px; font-weight:600; background:transparent;}");
		QToolButton* closeBtn = new QToolButton(_titleBar);
		closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
		closeBtn->setIconSize(QSize(24, 24));
		closeBtn->setToolTip(QStringLiteral("关闭"));
		closeBtn->setFixedSize(32, 28);
		closeBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
		)");
		connect(closeBtn, &QToolButton::clicked, this, &QWidget::close);
		titleLayout->addWidget(logoLabel);
		titleLayout->addWidget(titleLabel, 1);
		titleLayout->addWidget(closeBtn);
		outer->addWidget(_titleBar);

		//--- 内容区：类型/模板/路径三行表单 + 图片粒度选项（设计图01.1 四行） ---
		QWidget* body = new QWidget(this);
		body->setStyleSheet("QWidget{background:#FFFFFF;}");
		outer->addWidget(body, 1);
		QVBoxLayout* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(24, 12, 24, 0);
		bodyLayout->setSpacing(14);

		const QString labelStyle =
			"QLabel{color:#333333; font-size:13px; background:transparent;}";

		//导出文件类型：显示名 -> 内置渲染器名（mPostReportRendererFactory 按名创建）
		QHBoxLayout* typeRow = new QHBoxLayout();
		typeRow->setSpacing(10);
		QLabel* typeLabel = new QLabel(QStringLiteral("导出文件类型"), body);
		typeLabel->setStyleSheet(labelStyle);
		typeLabel->setFixedWidth(88);
		_formatCombo = new QComboBox(body);
		_formatCombo->setFixedHeight(30);
		_formatCombo->setStyleSheet(kInputStyle);
		_formatCombo->addItem(QStringLiteral("PPT 演示文稿"), "PowerPoint");
		_formatCombo->addItem(QStringLiteral("HTML 网页"), "HTML");
		_formatCombo->addItem(QStringLiteral("Word 文档"), "Word");
		connect(_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mExportReportDlg::slot_formatChanged);
		typeRow->addWidget(typeLabel);
		typeRow->addWidget(_formatCombo, 1);
		bodyLayout->addLayout(typeRow);

		//导出文件模板：默认模板（随程序发布的报告母版）/自定义（自选同构模板文件）；
		//HTML 未定义自定义模板，仅"默认模板"一项（设计图03）
		QHBoxLayout* templateRow = new QHBoxLayout();
		templateRow->setSpacing(10);
		QLabel* templateLabel = new QLabel(QStringLiteral("导出文件模板"), body);
		templateLabel->setStyleSheet(labelStyle);
		templateLabel->setFixedWidth(88);
		_templateCombo = new QComboBox(body);
		_templateCombo->setFixedHeight(30);
		_templateCombo->setStyleSheet(kInputStyle);
		_templateCombo->addItem(QStringLiteral("默认模板"), "default");
		_templateCombo->addItem(QStringLiteral("自定义"), "custom");
		connect(_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mExportReportDlg::slot_templateModeChanged);
		_templatePathEdit = new QLineEdit(body);
		_templatePathEdit->setFixedHeight(30);
		_templatePathEdit->setStyleSheet(kInputStyle);
		_templatePathEdit->setPlaceholderText(QStringLiteral("选择模板文件"));
		_templateBrowseBtn = new QToolButton(body);
		_templateBrowseBtn->setIcon(QIcon(":/mMainWidgets/Icon/Curve/操作/打开文件.svg"));
		_templateBrowseBtn->setIconSize(QSize(16, 16));
		_templateBrowseBtn->setFixedSize(28, 30);
		_templateBrowseBtn->setToolTip(QStringLiteral("选择模板文件"));
		_templateBrowseBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
		)");
		connect(_templateBrowseBtn, &QToolButton::clicked, this, &mExportReportDlg::slot_browseTemplate);
		templateRow->addWidget(templateLabel);
		templateRow->addWidget(_templateCombo, 1);
		templateRow->addWidget(_templatePathEdit, 2);
		templateRow->addWidget(_templateBrowseBtn);
		bodyLayout->addLayout(templateRow);
		updateTemplateRow();

		//导出文件路径：手输或经浏览按钮弹对话框选择（设计图01.1 红字注释）
		QHBoxLayout* pathRow = new QHBoxLayout();
		pathRow->setSpacing(10);
		QLabel* pathLabel = new QLabel(QStringLiteral("导出文件路径"), body);
		pathLabel->setStyleSheet(labelStyle);
		pathLabel->setFixedWidth(88);
		_pathEdit = new QLineEdit(body);
		_pathEdit->setFixedHeight(30);
		_pathEdit->setStyleSheet(kInputStyle);
		_pathEdit->setPlaceholderText(QStringLiteral("选择导出保存路径并设置文件名"));
		QToolButton* browseBtn = new QToolButton(body);
		browseBtn->setIcon(QIcon(":/mMainWidgets/Icon/Curve/操作/打开文件.svg"));
		browseBtn->setIconSize(QSize(16, 16));
		browseBtn->setFixedSize(28, 30);
		browseBtn->setToolTip(QStringLiteral("选择导出保存路径"));
		browseBtn->setStyleSheet(R"(
			QToolButton{ background:transparent; border:none; }
			QToolButton:hover{ background:#EFF1F2; border-radius:4px; }
		)");
		connect(browseBtn, &QToolButton::clicked, this, &mExportReportDlg::slot_browsePath);
		pathRow->addWidget(pathLabel);
		pathRow->addWidget(_pathEdit, 1);
		pathRow->addWidget(browseBtn);
		bodyLayout->addLayout(pathRow);

		//图片粒度选项：三格式通用（勾选=每图窗一图，不勾=每视图页合成一张整页
		//大图）；默认不勾
		_perImageCheck = new QCheckBox(QStringLiteral("将每个图表导出成单独图片"), body);
		_perImageCheck->setStyleSheet("QCheckBox{color:#333333; font-size:13px; background:transparent;}");
		_perImageCheck->setChecked(false);
		QHBoxLayout* checkRow = new QHBoxLayout();
		checkRow->addWidget(_perImageCheck);
		bodyLayout->addLayout(checkRow);
		bodyLayout->addStretch(1);

		//--- 底部按钮行：关闭（次）+ 导出（主，蓝） ---
		QHBoxLayout* btnRow = new QHBoxLayout();
		btnRow->setContentsMargins(0, 10, 24, 16);
		btnRow->setSpacing(12);
		btnRow->addStretch(1);
		QPushButton* closeAllBtn = new QPushButton(QStringLiteral("关闭"), body);
		closeAllBtn->setFixedSize(64, 30);
		closeAllBtn->setStyleSheet(kNormalBtnStyle);
		connect(closeAllBtn, &QPushButton::clicked, this, &QWidget::close);
		QPushButton* exportBtn = new QPushButton(QStringLiteral("导出"), body);
		exportBtn->setFixedSize(64, 30);
		exportBtn->setStyleSheet(kPrimaryBtnStyle);
		exportBtn->setDefault(true);
		connect(exportBtn, &QPushButton::clicked, this, &mExportReportDlg::slot_export);
		btnRow->addWidget(closeAllBtn);
		btnRow->addWidget(exportBtn);
		outer->addLayout(btnRow);
	}

	void mExportReportDlg::updateTemplateRow()
	{
		if (_templateCombo == nullptr)
			return;
		//HTML 未定义自定义模板（设计图03）：仅"默认模板"一项
		const bool isHtml = _formatCombo->currentData().toString() == QLatin1String("HTML");
		const bool wantCustom = !isHtml
			&& _templateCombo->currentData().toString() == QLatin1String("custom");
		_templateCombo->blockSignals(true);
		_templateCombo->clear();
		_templateCombo->addItem(QStringLiteral("默认模板"), "default");
		if (!isHtml)
			_templateCombo->addItem(QStringLiteral("自定义"), "custom");
		_templateCombo->setCurrentIndex(wantCustom ? 1 : 0);
		_templateCombo->blockSignals(false);

		//自定义模板路径行随选择联动（默认模板下置灰）
		const bool custom = _templateCombo->currentData().toString() == QLatin1String("custom");
		_templatePathEdit->setEnabled(custom);
		_templateBrowseBtn->setEnabled(custom);
		_templatePathEdit->setStyleSheet(custom ? kInputStyle : kInputStyleDisabled);
	}

	QString mExportReportDlg::defaultSuffixForFilter(const QString& filter)
	{
		QRegularExpression re(R"(\*\.(\w+))");
		QRegularExpressionMatch m = re.match(filter);
		return m.hasMatch() ? "." + m.captured(1) : QString();
	}

	void mExportReportDlg::slot_formatChanged(int index)
	{
		Q_UNUSED(index);
		//类型切换联动模板行（HTML 仅默认模板）
		updateTemplateRow();
		//切换类型后原路径的扩展名不再匹配新格式（如 .pptx -> .docx），清空
		//让用户重新选择；路径为空时点"导出"会按当前类型弹保存对话框补选
		_pathEdit->clear();
	}

	void mExportReportDlg::slot_templateModeChanged(int index)
	{
		Q_UNUSED(index);
		//默认模板/自定义切换联动模板路径行
		updateTemplateRow();
	}

	void mExportReportDlg::slot_browseTemplate()
	{
		//按导出类型过滤模板文件（与导出文件格式同构：pptx/docx）
		const QString rendererName = _formatCombo->currentData().toString();
		const QString filter = (rendererName == QLatin1String("Word"))
			? QStringLiteral("DOCX Files(*.docx)")
			: QStringLiteral("PPTX Files(*.pptx)");
		const QString path = QFileDialog::getOpenFileName(this,
			QStringLiteral("选择模板文件"), QString(), filter);
		if (path.isEmpty())
			return;
		_templatePathEdit->setText(path);
	}

	void mExportReportDlg::slot_browsePath()
	{
		PostData::mPostReportRendererFactory* factory = PostData::mPostReportRendererFactory::instance();
		if (factory->availableNames().isEmpty())
			registerPostExportRenderers();
		QSharedPointer<PostData::mPostReportRenderer> renderer =
			factory->createRenderer(_formatCombo->currentData().toString());
		if (renderer.isNull())
			return;

		const QString suffix = defaultSuffixForFilter(renderer->fileFilter());
		QString path = QFileDialog::getSaveFileName(this, QStringLiteral("选择导出路径"),
			QStringLiteral("MxSimReport%1").arg(suffix), renderer->fileFilter());
		if (path.isEmpty())
			return;
		if (QFileInfo(path).suffix().isEmpty())
			path.append(suffix);
		_pathEdit->setText(path);
	}

	bool mExportReportDlg::collectContext(PostData::mPostReportContext& context) const
	{
		PostCurve::CurveDocument* doc = PostCurve::mCurveDocController::getInstance()->document();
		if (doc == nullptr)
			return false;

		//截图落临时目录：渲染器统一按路径读图（PPT/HTML/Word 通用）
		const QString snapDir = QDir::temp().absoluteFilePath("mx_curve_report");
		QDir(snapDir).mkpath(".");

		context.meta.mainTitle = QStringLiteral("MxSim曲线分析报告");
		context.meta.dateTime = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
		const QString solveFile = MechIOFile::mIOFileManager::getInstance()->getReadSolveFileName();
		if (!solveFile.isEmpty())
			context.meta.mxFileName = QFileInfo(solveFile).fileName();

		//按文档树页序遍历（只读访问，经文档公开接口）：节序=文档树页序，
		//无图表的页不出现；页内图窗序=页内视口序（w 序，数全部视口）。
		//20261103-R10 起截图来源=临时离屏视图按统一标准尺寸重渲染
		//（见 grabChartStandard）：不再要求图表打开/物化到界面卡片，与界面
		//卡片状态（显示过/未显示过/从未同步）完全无关，数百图场景内存有界
		int figureNo = 0;   //勾选时的图注全局递增序号（导出顺序）
		for (int pageId : doc->childNodeIds(doc->rootNodeId()))
		{
			if (doc->nodeType(pageId) != PostCurve::CurveNodeType::Page)
				continue;
			const QString pageName = doc->nodeName(pageId);

			//页面布局选择器（kLayout）→ 网格行列（R07 导出跟随页面布局；
			//0/越界=自动 3 列，即 R06 现状——kLayout=0 回归线）
			const PostCurve::CurvePageLayoutGrid grid = PostCurve::curvePageLayoutGrid(
				doc->nodeProperty(pageId, PostCurve::CurveProp::kLayout).toInt());

			//本页全部图窗（保持页内视口序；不按"已打开"过滤——
			//懒加载下未打开的图表截图时按需物化）
			struct PageChart { int windowNo; int chartId; };
			QList<PageChart> pageCharts;
			int windowNo = 0;
			for (int chartId : doc->childNodeIds(pageId))
			{
				if (doc->nodeType(chartId) != PostCurve::CurveNodeType::Chart)
					continue;
				pageCharts.append({ ++windowNo, chartId });
			}
			if (pageCharts.isEmpty())
				continue;

			PostData::ReportSection section;
			section.id = QStringLiteral("page%1").arg(pageId);
			section.title = pageName;
			//节级布局契约字段（0=自动）：三渲染器模板分支按此消费（JSON 仅
			//模板分支写入，3D 老链路不感知）
			section.gridCols = grid.cols;
			section.gridRows = grid.rows;

			if (context.imagePerChart)
			{
				//勾选：每图窗单独一张 PNG（R10 起恒为标准尺寸，与卡片状态
				//无关），caption 由宿主组装为完整图注
				for (const PageChart& pc : pageCharts)
				{
					const QString imgPath = QString("%1/chart%2.png").arg(snapDir).arg(pc.chartId);
					//临时离屏视图按标准尺寸重渲染（不借界面卡片，见
					//grabChartStandard；渲染失败/写盘失败均跳过该图）
					if (!grabChartStandard(pc.chartId).save(imgPath, "PNG"))
						continue;
					PostData::ReportSectionItem item;
					item.type = PostData::ReportSectionItem::Image;
					item.image.imagePath = imgPath;
					item.image.caption = QStringLiteral("Figure %1: %2: Window%3")
						.arg(++figureNo).arg(pageName).arg(pc.windowNo);
					section.items.append(item);
					context.resourceImages << imgPath;
				}
			}
			else
			{
				//不勾选（默认）：本页全部图窗按标准尺寸重渲染后程序化宫格拼合
				//为单张整页大图，一页一图；页大图 caption 留空（页名即节标题）
				QList<QPixmap> grabs;
				for (const PageChart& pc : pageCharts)
					grabs << grabChartStandard(pc.chartId);
				const QString imgPath = QString("%1/page%2.png").arg(snapDir).arg(pageId);
				//按页面布局列数拼合（0=自动 3 列；20261102-R08 拉伸充满新基线）
				if (!composePageImage(grabs, imgPath, grid.cols))
					continue;
				PostData::ReportSectionItem item;
				item.type = PostData::ReportSectionItem::Image;
				item.image.imagePath = imgPath;
				section.items.append(item);
				context.resourceImages << imgPath;
			}

			if (!section.items.isEmpty())
				context.addSection(section);
		}

		return !context.sections.isEmpty();
	}

	void mExportReportDlg::slot_export()
	{
		//渲染器就位（面板可能早于 registerAPI 初始化，就地注册，重复注册安全）
		PostData::mPostReportRendererFactory* factory = PostData::mPostReportRendererFactory::instance();
		if (factory->availableNames().isEmpty())
			registerPostExportRenderers();
		QSharedPointer<PostData::mPostReportRenderer> renderer =
			factory->createRenderer(_formatCombo->currentData().toString());
		if (renderer.isNull())
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("不支持的导出格式: %1").arg(_formatCombo->currentData().toString()), true);
			return;
		}

		//导出路径：未选择时按当前类型弹保存对话框补选
		QString savePath = _pathEdit->text().trimmed();
		const QString suffix = defaultSuffixForFilter(renderer->fileFilter());
		if (savePath.isEmpty())
		{
			savePath = QFileDialog::getSaveFileName(this, QStringLiteral("选择导出路径"),
				QStringLiteral("MxSimReport%1").arg(suffix), renderer->fileFilter());
			if (savePath.isEmpty())
				return;
			_pathEdit->setText(savePath);
		}
		if (QFileInfo(savePath).suffix().isEmpty())
		{
			savePath.append(suffix);
			_pathEdit->setText(savePath);
		}

		//收集当前曲线图表视窗截图（按视图页分组）；模板化分支开关随对话框选择设置
		PostData::mPostReportContext context;
		context.templateMode = true;
		context.imagePerChart = _perImageCheck->isChecked();
		if (_templateCombo->currentData().toString() == QLatin1String("custom"))
		{
			const QString templatePath = _templatePathEdit->text().trimmed();
			if (templatePath.isEmpty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
					QStringLiteral("导出报告失败: 请先选择自定义模板文件"), true);
				return;
			}
			context.templatePath = templatePath;
		}
		if (!collectContext(context))
		{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
					QStringLiteral("导出报告失败: 当前没有可导出的曲线图表"), true);
			return;
		}

		//渲染并保存
		QString renderError;
		if (!renderer->render(context, savePath, renderError))
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
				QStringLiteral("导出报告失败: %1").arg(renderError), true);
			return;
		}
		MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
			QStringLiteral("报告已导出: %1").arg(savePath), true);
	}

	void mExportReportDlg::mousePressEvent(QMouseEvent* event)
	{
		//仅标题栏区域起拖（内容区控件交互不受影响）
		if (event->button() == Qt::LeftButton && _titleBar != nullptr
			&& _titleBar->underMouse())
			_dragOffset = event->globalPos() - frameGeometry().topLeft();
		QWidget::mousePressEvent(event);
	}

	void mExportReportDlg::mouseMoveEvent(QMouseEvent* event)
	{
		if ((event->buttons() & Qt::LeftButton) && !_dragOffset.isNull())
			move(event->globalPos() - _dragOffset);
		QWidget::mouseMoveEvent(event);
	}

	void mExportReportDlg::mouseReleaseEvent(QMouseEvent* event)
	{
		_dragOffset = QPoint();
		QWidget::mouseReleaseEvent(event);
	}
}
