#include "mPostMenuContributor.h"
#include "mUiMenu.h"
#include "mpostwindow_global.h"

//QT
#include <QToolButton>
#include <QMenu>
#include <QAction>

#include <tuple>

#include "IRibbonHost.h"
#include "mRibbonWidget.h"
#include "ThemeManager.h"
#include "UiPalette.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mPostCurveData.h"
#include "mPostDataManager.h"
#include "mOpenCurveDlg.h"
#include "mCurveStatisticsDlg.h"
#include "mCreate3DPlotDlg.h"
#include "mDefine3DCurveDlg.h"
#include "mTemplateCurveDlg.h"
#include "mCurveTemplateManager.h"
#include "CurveView/mCurveViewportFactory.h"
// （R46 注释入口改模式开关，queryAxis3DDisplayRange 消费随旧直接创建入口退役，
//   m3DChartView.h include 一并移除）
#include "CurveCore/ChartTypeProfile.h"
#include "mDefineCurveDlg.h"
#include "mEditCurveDlg.h"
#include "mTemplateConfigDlg.h"
#include "mDynStiffnessDlg.h"
#include "mVtfCurveDlg.h"
#include "mNtfCurveDlg.h"
#include "CurveCore/mCurveDocController.h"
#include "CurveCore/CurveDocument.h"   //CurveNode/CurveDocument 完整定义（3D注释按钮查表 chartType 与启停联动用）
#include <QUndoStack>
#include "CurvePages/mWaterfall3DPage.h"      //3D曲线工具页五件（R37，Ribbon 点击挂载）
#include "CurvePages/mCurveStyle3DPage.h"
#include "CurvePages/mPointSeek3DPage.h"
#include "CurvePages/mSectionQuery3DPage.h"
#include "CurvePages/mDatum3DPage.h"
#include "mPanelHostProvider.h"   //Info dock 容器经 IPanelHost 接口解耦（mPostInfoHub 同款）
#include "mDockLayoutManager.h"   //activateDock("Info")
#include "mExportReportDlg.h"
#include "mStrengthReportDlg.h"
#include "mPostAnimationMenuWidget.h"
#include "mPostCloudMenuWidget.h"
#include "mPostDisplayMenuFunction.h"
#include "mPostSelectMenuFunction.h"
#include "mPostIdLineEdit.h"
#include "mOtherFunction.h"
#include "mCalcLineEdit.h"
#include "mAppConfiguration.h"
#include "UiMetrics.h"
#include "mxr/view/REBUILD_PostRend.h"

//QT
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QLabel>
#include <QMouseEvent>
#include <QRegularExpression>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>

#include <functional>

#include <cstring>
using namespace MxFunctions;

/*
* 融合按钮左右键分离过滤器（自前处理 mMenuWidget.cpp 的 DualClickFilter 复制：
* PostWindow 不依赖 MainWindow；用于"显示"页"步骤/旋转/缩放"左键正向/右键反向）
*/
class PostDualClickFilter : public QObject
{
public:
	explicit PostDualClickFilter(QObject* parent = nullptr) : QObject(parent) {}
	std::function<void()> onRightClick;
protected:
	bool eventFilter(QObject* watched, QEvent* event) override
	{
		if (event->type() == QEvent::MouseButtonPress)
		{
			auto* mouseEvent = static_cast<QMouseEvent*>(event);
			if (mouseEvent->button() == Qt::RightButton && onRightClick)
			{
				onRightClick();
				return true;
			}
		}
		return QObject::eventFilter(watched, event);
	}
};

namespace PostWindow
{
	mPostMenuContributor* mPostMenuContributor::getInstance()
	{
		static mPostMenuContributor instance;
		return &instance;
	}

	void mPostMenuContributor::registerMenus(IRibbonHost* host)
	{
		if (host == nullptr)
			return;

		buildCloudPage(host);
		buildResultPage(host);
		buildAnimationPage(host);
		buildCurvePage(host);
		buildNVHCurvePage(host);
		build3DCurvePage(host);
		buildReportPage(host);
		buildShellPages(host);
		buildNotePage(host);
		buildDisplayPage(host);
		buildSelectPage(host);
		buildModelPage(host);
		//TODO 其余按迁移地图迁入后在此追加（postMenu1后处理文件入口）
	}

	void mPostMenuContributor::buildCloudPage(IRibbonHost* host)
	{
		//挂Ribbon宿主控件下，随主窗口在qApp存活期内销毁（见成员注释）；
		//hide：控件包仅作信号接收器，防宿主show时连带显示为空白块
		_cloudMW = new mPostCloudMenuWidget(dynamic_cast<QWidget*>(host));
		_cloudMW->hide();

		host->createRibbonPage("云图");

		mGroupWidget* threeNewGW = host->addGroup("云图", "新建");
		//切换组（工况/变量/复数/帧/分量/层六个下拉）已按要求整组移除（2026-11-11）：
		//云图选择由结果树与云图参数面板承载，mPostCloudMenuWidget 对应下拉不再创建
		//mGroupWidget* threeChangeGW = host->addGroup("云图", "切换");
		//mGroupWidget* threeRangeGW = host->addGroup("云图", "范围");（原范围组整块为废弃注释，见下）
		mGroupWidget* threeFreeNodeGW = host->addGroup("云图", "自由节点");
		mGroupWidget* threeQueryGW = host->addGroup("云图", "查询");
		mGroupWidget* threeFilterGW = host->addGroup("云图", "过滤");
		//（原 _postCloudGW 选项卡列表只写不读，随迁移移除）

		auto* bus = MechCommon::mMechGlobalSignals::getInstance();

		if (threeNewGW)
		{
			//线性叠加/等值面图/后处理-集合（2026-11-10）与矢量图（2026-11-11，选择
			//入口并归结果树/结果页向量图）已按要求从菜单栏移除（信号接收端原样保留），
			//剩余按钮列号顺次重排避免中间空列
			QToolButton* scalarBtn = threeNewGW->addToolBtn("标量运算", ":/mMainWidgets/Icon/post/svg/biaoliangyunshuan.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(scalarBtn, "标量运算", ":/mMainWidgets/Icon/post/svg/biaoliangyunshuan.svg", "对云图颜色表进行标量运算。");
			QObject::connect(scalarBtn, &QToolButton::clicked, scalarBtn, [bus]() {
				emit bus->clickScalarOperationSig();
			});

			QToolButton* animationBtn = threeNewGW->addToolBtn("动画", ":/mMainWidgets/Icon/post/svg/donghua.svg", 0, 1, 2, 1, QSize(30, 30));
			animationBtn->setProperty("mxHasMenu", true);
			host->setQToolButton(animationBtn, "动画", ":/mMainWidgets/Icon/post/svg/donghua.svg", "查看动画。");
			animationBtn->setPopupMode(QToolButton::InstantPopup); // 点击就下拉

			QMenu* animationMenu = new MechMainWindow::mUiMenu();
			// 一键生成
			QAction* generateAction = animationMenu->addAction("一键生成");
			QObject::connect(generateAction, &QAction::triggered, generateAction, [bus]() {
				emit bus->clickAnimationSig(1);
				});

			// 自定义
			QAction* customAction = animationMenu->addAction("自定义");
			QObject::connect(customAction, &QAction::triggered, customAction, [bus]() {
				emit bus->clickAnimationSig(2);
				});
			animationBtn->setMenu(animationMenu);

			QToolButton* sectionBtn = threeNewGW->addToolBtn("切面图", ":/mMainWidgets/Icon/post/svg/qiemiantu.svg", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(sectionBtn, "切面图", ":/mMainWidgets/Icon/post/svg/qiemiantu.svg", "查看切面图。");
			QObject::connect(sectionBtn, &QToolButton::clicked, sectionBtn, [bus]() {
				emit bus->clickSectionSig();
				});

			QToolButton* streamlineBtn = threeNewGW->addToolBtn("流线图", ":/mMainWidgets/Icon/post/svg/liuxiantu.svg", 0, 3, 2, 1, QSize(30, 30));
			host->setQToolButton(streamlineBtn, "流线图", ":/mMainWidgets/Icon/post/svg/liuxiantu.svg", "查看流线图。");
			QObject::connect(streamlineBtn, &QToolButton::clicked, streamlineBtn, [bus]() {
				emit bus->clickStreamLineSig();
			});

			QToolButton* subareaBtn = threeNewGW->addToolBtn("分区", ":/mMainWidgets/Icon/post/svg/fenqu.svg", 0, 4, 2, 1, QSize(30, 30));
			host->setQToolButton(subareaBtn, "分区", ":/mMainWidgets/Icon/post/svg/fenqu.svg", "查看分区。");
			QObject::connect(subareaBtn, &QToolButton::clicked, subareaBtn, [bus]() {
				emit bus->clickPartitionSig();
				});
		}

		//（原"范围"组整块为废弃注释代码，引用已删除的旧控件接口，随迁移不保留）

		if (threeFreeNodeGW)
		{
			//自由节点组整饬（用户 2026-11-05 确认模拟图）：左侧"显示"全高按钮（图标上
			//文字下）跨两行锚定，右侧两行紧凑网格——行0"大小[标签32+数字框70]"、
			//行1"关联云图+删除"两行左缘对齐；此前标签跨2列与数字框隔着空列、"删除"
			//钉在 col5，行内参差
			_cloudMW->_toolbtnFreeNode = threeFreeNodeGW->addToolBtn("显示", ":/mMainWidgets/Icon/post/display.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(_cloudMW->_toolbtnFreeNode, "显示", ":/mMainWidgets/Icon/post/display.png", "显示。");
			QObject::connect(_cloudMW->_toolbtnFreeNode, &QToolButton::clicked, _cloudMW, &mPostCloudMenuWidget::showOrHideFreeNode);

			QLabel* sizeLabel = new QLabel("大小");
			sizeLabel->setFixedWidth(32);
			_cloudMW->_sizeSpinBox = new QDoubleSpinBox();
			_cloudMW->_sizeSpinBox->setFixedWidth(70);
			QHBoxLayout* sizeRow = new QHBoxLayout();
			sizeRow->setSpacing(4);
			sizeRow->setContentsMargins(0, 0, 0, 0);
			sizeRow->addWidget(sizeLabel);
			sizeRow->addWidget(_cloudMW->_sizeSpinBox);
			threeFreeNodeGW->addBoxLayout(sizeRow, 0, 1, 1, 1);

			_cloudMW->_sizeSpinBox->setMinimum(1.000000);
			_cloudMW->_sizeSpinBox->setMaximum(99.000000);
			_cloudMW->_sizeSpinBox->setSingleStep(1.000000);
			_cloudMW->_sizeSpinBox->setValue(10.000000);
			QObject::connect(_cloudMW->_sizeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), _cloudMW, &mPostCloudMenuWidget::slot_setPointSize);

			//行1"关联云图+删除"合为一个整体单元格：删除紧跟复选框（模拟图定稿），
			//若按两列摆，col1 宽由行0"大小+数字框"（106px）撑宽，复选框（69px）
			//右侧会留 37px 空隙再接删除
			_cloudMW->_checkboxColud = new QCheckBox("关联云图");
			_cloudMW->_deleteBtn = new QPushButton("删除");
			QHBoxLayout* checkRow = new QHBoxLayout();
			checkRow->setSpacing(8);
			checkRow->setContentsMargins(0, 0, 0, 0);
			checkRow->addWidget(_cloudMW->_checkboxColud);
			checkRow->addWidget(_cloudMW->_deleteBtn);
			threeFreeNodeGW->addBoxLayout(checkRow, 1, 1, 1, 1);
			QObject::connect(_cloudMW->_checkboxColud, &QCheckBox::toggled, _cloudMW, &mPostCloudMenuWidget::relatedCloudFreeNode);
			QObject::connect(_cloudMW->_deleteBtn, &QPushButton::clicked, _cloudMW, &mPostCloudMenuWidget::deleteFreeNode);
		}

		if (threeQueryGW)
		{
			//查询组整饬（用户 2026-11-05 确认模拟图）："显示值"从左侧自由区入网格，
			//三段式 [显示值全高][隐藏值/隐藏全部 等宽堆叠小钮][高级查询全高] 同网格对齐
			QToolButton* toolbtnShow = threeQueryGW->addToolBtn("显示值", ":/mMainWidgets/Icon/post/xianshizhi.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(toolbtnShow, "显示值", ":/mMainWidgets/Icon/post/xianshizhi.png", "显示拾取节点或单元的结果值。");
			QObject::connect(toolbtnShow, &QToolButton::clicked, toolbtnShow, [bus]() {
				emit bus->clickCloudShowValueSig();
				});

			QToolButton* toolbtnHide = threeQueryGW->addToolBtn("隐藏值", ":/mMainWidgets/Icon/post/yingcangzhi.png", 0, 1, 1, 1);
			host->setQToolButton(toolbtnHide, "隐藏值", ":/mMainWidgets/Icon/post/yingcangzhi.png", "隐藏拾取节点或单元的结果值。", Qt::ToolButtonTextBesideIcon);
			QObject::connect(toolbtnHide, &QToolButton::clicked, toolbtnHide, [bus]() {
				emit bus->clickCloudHideValueSig();
				});

			QToolButton* toolbtnHideAll = threeQueryGW->addToolBtn("隐藏全部", ":/mMainWidgets/Icon/post/yingcangquanbu.png", 1, 1, 1, 1);
			host->setQToolButton(toolbtnHideAll, "隐藏全部", ":/mMainWidgets/Icon/post/yingcangquanbu.png", "隐藏全部已经显示的结果值。", Qt::ToolButtonTextBesideIcon);
			QObject::connect(toolbtnHideAll, &QToolButton::clicked, toolbtnHideAll, [bus]() {
				emit bus->clickCloudHideAllSig();
				});

			QToolButton* toolbtnAdvanced = threeQueryGW->addToolBtn("高级查询", ":/mMainWidgets/Icon/post/gaojichaxun.png", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(toolbtnAdvanced, "高级查询", ":/mMainWidgets/Icon/post/gaojichaxun.png", "高级查询。");
			QObject::connect(toolbtnAdvanced, &QToolButton::clicked, toolbtnAdvanced, [bus]() {
				emit bus->clickAdvancedQuerySig();
				});
		}

		if (threeFilterGW)
		{
			// 清除默认的列拉伸
			for (int i = 0; i < 7; ++i)
			{
				threeFilterGW->m_R_gridLayout_->setColumnStretch(i, 0);
			}

			threeFilterGW->m_R_gridLayout_->setHorizontalSpacing(4);

			_cloudMW->_toolbtnFilter = threeFilterGW->addToolBtn("开关", ":/mMainWidgets/Icon/post/guan.png", 0, 0, 2, 1, QSize(30, 30));
			_cloudMW->_toolbtnFilter->setObjectName("toolbtnFilter");
			host->setQToolButton(_cloudMW->_toolbtnFilter, "开关", ":/mMainWidgets/Icon/post/kaiguan.png", "选中时开启过滤,关闭时清空过滤");
			// 选中底色随主题（原硬编码 #EBF2FA 浅色底在深色主题下是一块突兀亮斑）；
			// 文字/hover/disabled 由 ThemeManager::applyGroupWidgetPalette 按 objectName 统一下发

			QLabel* labelRule = new QLabel("规则");
			labelRule->setFixedWidth(32);
			_cloudMW->_comboRule = new QComboBox();
			_cloudMW->_comboRule->setObjectName("规则");
			QHBoxLayout* h = new QHBoxLayout();
			h->setSpacing(0);
			h->setContentsMargins(0, 0, 0, 0);
			h->addWidget(labelRule);
			h->addWidget(_cloudMW->_comboRule);
			threeFilterGW->addBoxLayout(h, 0, 1, 1, 1);
			QStringList rules = { "之间", "大于", "小于", "等于" };
			_cloudMW->_comboRule->addItems(rules);
			_cloudMW->_comboRule->setFixedWidth(80);


			QLabel* labelRange = new QLabel("范围");
			labelRange->setFixedWidth(32);
			_cloudMW->_lineEditRangeMin = new MxFunctions::mCalcLineEdit();
			_cloudMW->_lineEditRangeMin->setFixedHeight(25);
			_cloudMW->_lineEditRangeMin->setFixedWidth(80);
			h->setSpacing(0);
			h->setContentsMargins(0, 0, 0, 0);
			h = new QHBoxLayout();
			h->addWidget(labelRange);
			h->addWidget(_cloudMW->_lineEditRangeMin);
			h->setSpacing(0);
			threeFilterGW->addBoxLayout(h, 1, 1, 1, 1);
			QLabel* label = threeFilterGW->addLabel("—", 1, 2);

			_cloudMW->_lineEditRangeMax = new MxFunctions::mCalcLineEdit();
			_cloudMW->_lineEditRangeMax->setFixedHeight(25);
			_cloudMW->_lineEditRangeMax->setFixedWidth(80);
			threeFilterGW->addCalcLineEdit(_cloudMW->_lineEditRangeMax, 1, 3);

			_cloudMW->_m_slider = threeFilterGW->addSlider("hslider", 1, 4);//htx 添加滑块
			_cloudMW->_m_slider->setRange(1, 100);//设置范围
			_cloudMW->_m_slider->setValue(50);
			_cloudMW->_m_slider->setSingleStep(1);

			//导出过滤后模型的STL（过滤开启时可用，随过滤开关启停）
			//过滤组列整体左移（用户 2026-11-05 确认模拟图）：导出STL 从 col6 紧贴
			//范围输入右侧（col5），消除行1"规则"下拉到导出STL 之间约150px的空列
			_cloudMW->_toolbtnExportSTL = threeFilterGW->addToolBtn("导出STL", ":/mMainWidgets/Icon/homepage/daochu3x", 0, 5, 2, 1, QSize(30, 30));
			host->setQToolButton(_cloudMW->_toolbtnExportSTL, "导出STL", ":/mMainWidgets/Icon/homepage/daochu3x.png", "导出过滤后模型的STL文件");
			_cloudMW->_toolbtnExportSTL->setEnabled(false);
			QObject::connect(_cloudMW->_toolbtnExportSTL, &QToolButton::clicked, _cloudMW->_toolbtnExportSTL, [this]() {
				exportFilterSTL();
			});

			threeFilterGW->m_R_gridLayout_->setColumnStretch(5, 0);
			threeFilterGW->m_R_gridLayout_->setColumnStretch(6, 1);
			threeFilterGW->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
			_cloudMW->_comboRule->setEnabled(false);
			_cloudMW->_lineEditRangeMin->setEnabled(false);
			_cloudMW->_lineEditRangeMax->setEnabled(false);
			_cloudMW->_m_slider->setHidden(true);

			QObject::connect(_cloudMW->_m_slider, &QSlider::valueChanged, _cloudMW->_m_slider, [this, bus]() {
				if (_cloudMW->_isModify)//防止循环修改
				{
					_cloudMW->_isModify = false;
					return;
				}

				if (_cloudMW->_isGreater)//大于
				{
					int value = _cloudMW->_m_slider->value();//获取到滑块值
					double dx = (_cloudMW->_filterMin.toDouble() + _cloudMW->_filterMax.toDouble()) / 100;

					double position = 0;//位置值

					if (value == 1)
					{
						position = _cloudMW->_filterMin.toDouble();
					}
					else if (value == 100)
					{
						position = _cloudMW->_filterMax.toDouble();
					}
					else
					{
						double dx = (_cloudMW->_filterMin.toDouble() + _cloudMW->_filterMax.toDouble()) / 100;

						position = dx * value + _cloudMW->_filterMin.toDouble();//得到位置
					}

					_cloudMW->_lineEditRangeMin->setText(MxFunctions::mNumberToString(position));

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());

				}
				else
				{
					int value = _cloudMW->_m_slider->value();//获取到滑块值
					double position = 0;//位置值

					if (value == 1)
					{
						position = _cloudMW->_filterMin.toDouble();
					}
					else if (value == 100)
					{
						position = _cloudMW->_filterMax.toDouble();
					}
					else
					{
						double dx = (_cloudMW->_filterMin.toDouble() + _cloudMW->_filterMax.toDouble()) / 100;

						position = dx * value + _cloudMW->_filterMin.toDouble();//得到位置
					}

					_cloudMW->_lineEditRangeMax->setText(MxFunctions::mNumberToString(position));

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				}

				});

			QObject::connect(_cloudMW->_toolbtnFilter, &QToolButton::clicked, _cloudMW->_toolbtnFilter, [this, bus]() {
				_cloudMW->_filterOpen = !_cloudMW->_filterOpen;
				bool start = _cloudMW->_filterOpen;

				//设置禁用状态
				_cloudMW->_comboRule->setEnabled(start);
				_cloudMW->_lineEditRangeMin->setEnabled(start);
				_cloudMW->_lineEditRangeMax->setEnabled(start);
				if (_cloudMW->_toolbtnExportSTL)
					_cloudMW->_toolbtnExportSTL->setEnabled(start);
				_cloudMW->_m_slider->setHidden(true);

				if (!start)
				{
					_cloudMW->_toolbtnFilter->setIcon(QIcon(":/mMainWidgets/Icon/post/guan.png"));

					//清空过滤
					_cloudMW->_comboRule->blockSignals(true);
					_cloudMW->_lineEditRangeMin->blockSignals(true);
					_cloudMW->_lineEditRangeMax->blockSignals(true);

					_cloudMW->_comboRule->setCurrentText("之间");
					_cloudMW->_lineEditRangeMin->setText(_cloudMW->_filterMin);
					_cloudMW->_lineEditRangeMax->setText(_cloudMW->_filterMax);

					_cloudMW->_comboRule->blockSignals(false);
					_cloudMW->_lineEditRangeMin->blockSignals(false);
					_cloudMW->_lineEditRangeMax->blockSignals(false);

					emit bus->clickCloudClearFilterSig();
				}
				else
				{
					_cloudMW->_toolbtnFilter->setIcon(QIcon(":/mMainWidgets/Icon/post/kai.png"));

					_cloudMW->_lineEditRangeMin->setEnabled(true);
					_cloudMW->_lineEditRangeMax->setEnabled(true);
				}
				});

			QObject::connect(_cloudMW->_comboRule, QOverload<const QString&>::of(&QComboBox::currentTextChanged), _cloudMW->_comboRule, [this, bus](const QString& type) {
				if (type == "之间")
				{
					_cloudMW->_lineEditRangeMin->setEnabled(true);
					_cloudMW->_lineEditRangeMax->setEnabled(true);
					_cloudMW->_m_slider->setHidden(true);

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				}
				else if (type == "大于")
				{
					_cloudMW->_lineEditRangeMin->setEnabled(true);
					_cloudMW->_lineEditRangeMax->setEnabled(false);
					_cloudMW->_lineEditRangeMax->setText(_cloudMW->_filterMax);
					_cloudMW->_m_slider->setHidden(false);
					_cloudMW->_isGreater = true;
					_cloudMW->_m_slider->setValue(1);

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				}
				else if (type == "小于")
				{
					_cloudMW->_lineEditRangeMin->setEnabled(false);
					_cloudMW->_lineEditRangeMax->setEnabled(true);
					_cloudMW->_lineEditRangeMin->setText(_cloudMW->_filterMin);
					_cloudMW->_m_slider->setHidden(false);
					_cloudMW->_isGreater = false;
					_cloudMW->_m_slider->setValue(100);

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				}
				else if (type == "等于")
				{
					_cloudMW->_lineEditRangeMin->setEnabled(true);
					_cloudMW->_lineEditRangeMax->setEnabled(true);
					_cloudMW->_m_slider->setHidden(true);
					QString str = _cloudMW->_lineEditRangeMin->text();
					_cloudMW->_lineEditRangeMax->setText(str);

					emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				}
				});

			QObject::connect(_cloudMW->_lineEditRangeMin, &QLineEdit::editingFinished, _cloudMW->_lineEditRangeMin, [this, bus]() {
				double value = _cloudMW->_lineEditRangeMin->text().toDouble();
				if (value < _cloudMW->_filterMin.toDouble())
				{
					_cloudMW->_lineEditRangeMin->setText(_cloudMW->_filterMin);
				}

				if (_cloudMW->_lineEditRangeMin->text().isEmpty())//如果输入框为空
				{
					_cloudMW->_lineEditRangeMin->setText(_cloudMW->_filterMin);
				}

				if (_cloudMW->_comboRule->currentText() == "等于")
				{
					QString str = _cloudMW->_lineEditRangeMin->text();
					_cloudMW->_lineEditRangeMax->setText(str);
				}
				if (_cloudMW->_comboRule->currentText() == "大于")
				{
					int value = _cloudMW->_m_slider->value();//获取到滑块值
					double dx = (_cloudMW->_filterMin.toDouble() + _cloudMW->_filterMax.toDouble()) / 100;

					double position = dx * value + _cloudMW->_filterMin.toDouble();

					double nowvalue = _cloudMW->_lineEditRangeMin->text().toDouble();

					if (std::abs(position - nowvalue) > dx)//当值大于dx时，
					{
						int num = nowvalue / dx;
						if (num > 100)//如果超过最大值，则取最大值
						{
							num = 100;
						}
						_cloudMW->_m_slider->setValue(num);
						_cloudMW->_isModify = true;
					}

				}


				emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				});

			QObject::connect(_cloudMW->_lineEditRangeMax, &QLineEdit::editingFinished, _cloudMW->_lineEditRangeMax, [this, bus]() {
				double value = _cloudMW->_lineEditRangeMax->text().toDouble();
				if (value > _cloudMW->_filterMax.toDouble())
				{
					_cloudMW->_lineEditRangeMax->setText(_cloudMW->_filterMax);
				}

				if (_cloudMW->_lineEditRangeMax->text().isEmpty())//如果输入框为空
				{
					_cloudMW->_lineEditRangeMax->setText(_cloudMW->_filterMax);
				}

				if (_cloudMW->_comboRule->currentText() == "等于")
				{
					QString str = _cloudMW->_lineEditRangeMax->text();
					_cloudMW->_lineEditRangeMin->setText(str);
				}

				if (_cloudMW->_comboRule->currentText() == "小于")
				{
					int value = _cloudMW->_m_slider->value();//获取到滑块值
					double dx = (_cloudMW->_filterMin.toDouble() + _cloudMW->_filterMax.toDouble()) / 100;

					double position = dx * value + _cloudMW->_filterMin.toDouble();

					double nowvalue = _cloudMW->_lineEditRangeMax->text().toDouble();

					if (std::abs(position - nowvalue) > dx)//当值大于dx时，
					{
						int num = nowvalue / dx;
						if (num > 100)//如果超过最大值，则取最大值
						{
							num = 100;
						}
						_cloudMW->_m_slider->setValue(num);
						_cloudMW->_isModify = true;
					}

				}

				emit bus->clickCloudStartFilterSig(_cloudMW->_lineEditRangeMin->text(), _cloudMW->_lineEditRangeMax->text(), _cloudMW->_comboRule->currentText());
				});

		}
	}

	void mPostMenuContributor::exportFilterSTL()
	{
		auto* vpMgr = MechPreData::mViewportModelManager::getInstance();
		auto* curVM = vpMgr->getCurrentViewportModel();

		//仅后处理云图支持
		if (curVM == nullptr || !curVM->isPostModel())
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(QString("无法导出该模型，请选择后处理模型！"));
			return;
		}

		//默认目录
		QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_recentDir;
		QFileInfo info(defaultDir);
		if (defaultDir.isEmpty() || !info.exists())
		{
			defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
		}

		//默认文件名：数据库名_当前云图变量名（去除文件名非法字符）
		QString defaultName;
		mxr::REBUILD_PostRend* postRend = curVM->getPostRenderWidget();
		if (postRend != nullptr)
		{
			MechPreData::mPostGUIData* guidata = curVM->getGUIDataByPostRender(postRend);
			if (guidata != nullptr && !guidata->_cloudVarName.isEmpty())
			{
				defaultName = QString("%1_%2").arg(postRend->getPostDataName()).arg(guidata->_cloudVarName);
				defaultName.replace(QRegularExpression("[\\/:*?\"<>|()]"), "_");
			}
		}
		if (defaultName.isEmpty())
			defaultName = QStringLiteral("filter_model");
		QString defaultPath = QDir(defaultDir).filePath(defaultName + ".stl");

		QString fileName = QFileDialog::getSaveFileName(
			nullptr,
			QObject::tr("导出STL"),
			defaultPath,
			QStringLiteral("STL(*.stl)")
		);

		if (fileName.isEmpty())
			return;
		if (!fileName.endsWith(".stl", Qt::CaseInsensitive))
			fileName += ".stl";

		MAppConfiguration::mAppConfiguration::getInstance()->_recentDir = QFileInfo(fileName).absolutePath();
		emit MechCommon::mMechGlobalSignals::getInstance()->clickExportFilterSTLSig(fileName);
	}

	void mPostMenuContributor::buildResultPage(IRibbonHost* host)
	{
		host->createRibbonPage("结果");

		//结果页按原型 V1.2 最终态整理（2026-11-09）：六个原型分组按序构建；同功能
		//不同名的对齐原型改名（功能阵列=阵列与回转；向量图/线性叠加/查询接云图页
		//既有同名功能信号）。原型没有的既有功能（曲线/路径结果显示/标注/模态贡献量/
		//面板贡献量/传递路径贡献量，原"待定"组）已按要求从菜单栏移除（2026-11-10），
		//信号接收端在 mPostBrowserWindow 原样保留
		mGroupWidget* graphicsGW = host->addGroup("结果", "结果图形");
		mGroupWidget* deformGW = host->addGroup("结果", "变形控制");
		mGroupWidget* spatialGW = host->addGroup("结果", "空间显示");
		mGroupWidget* analysisGW = host->addGroup("结果", "结果分析");
		mGroupWidget* outputGW = host->addGroup("结果", "报告与输出");
		mGroupWidget* casesGW = host->addGroup("结果", "工况");

		auto* bus = MechCommon::mMechGlobalSignals::getInstance();

		if (graphicsGW)
		{
			QToolButton* threeDBtn = graphicsGW->addToolBtn("云图", ":/mMainWidgets/Icon/post/svg/yuntu.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(threeDBtn, "云图", ":/mMainWidgets/Icon/post/svg/yuntu.svg", "查看云图。");
			QObject::connect(threeDBtn, &QToolButton::clicked, threeDBtn, [bus]() { emit bus->click3DResultSig(); });

			//向量图=V1.3 向量参数面板（结果页直连，原型 #vectorParameters）；
			//旧云图派生"矢量图"接收端（clickVectorSig→slot_createVector）原样保留；
			//张量图已按要求从菜单栏移除（2026-11-11，未实现功能，原点击仅提示待接入）
			QToolButton* vectorBtn = graphicsGW->addToolBtn("向量图", ":/mMainWidgets/Icon/post/svg/xiangliangtu.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(vectorBtn, "向量图", ":/mMainWidgets/Icon/post/svg/xiangliangtu.svg", "查看向量图。");
			QObject::connect(vectorBtn, &QToolButton::clicked, vectorBtn, [bus]() { emit bus->clickPostVectorPlotSig(); });
		}

		if (deformGW)
		{
			//变形控制：打开变形控制参数面板（云图面板"变形系数"齿轮按钮共用同一信号）；
			//图标取自原型 #icon-deformation（虚线网格+蓝色变形曲线+箭头）
			QToolButton* deformationBtn = deformGW->addToolBtn("变形控制", ":/mMainWidgets/Icon/post/bianxingkongzhi.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(deformationBtn, "变形控制", ":/mMainWidgets/Icon/post/bianxingkongzhi", "设置变形系数与初始构型显示。");
			QObject::connect(deformationBtn, &QToolButton::clicked, deformationBtn, [bus]() { emit bus->clickDeformationSig(); });
		}

		if (spatialGW)
		{
			//等值面图：打开等值面参数面板（作用于当前激活云图视口，不建树节点；图标与树侧等值面节点同源）
			QToolButton* isosurfaceBtn = spatialGW->addToolBtn("等值面图", ":/mMainWidgets/Icon/post/svg/dengzhimian.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(isosurfaceBtn, "等值面图", ":/mMainWidgets/Icon/post/svg/dengzhimian.svg", "查看等值面图。");
			QObject::connect(isosurfaceBtn, &QToolButton::clicked, isosurfaceBtn, [bus]() { emit bus->clickIsosurfacePanelSig(); });

			//阵列与回转：原"功能阵列"改名对齐原型（面板即镜像与阵列+轴向回转），信号不变
			QToolButton* parrayMimageBtn = spatialGW->addToolBtn("阵列与回转", ":/mMainWidgets/Icon/post/gongnengzhenlie.png", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(parrayMimageBtn, "阵列与回转", ":/mMainWidgets/Icon/post/gongnengzhenlie", "镜像与阵列、轴向回转显示。");
			QObject::connect(parrayMimageBtn, &QToolButton::clicked, parrayMimageBtn, [bus]() { emit bus->clickPostParrayMimageSig(); });

			QToolButton* ExplodeBtn = spatialGW->addToolBtn("爆炸视图", ":/mMainWidgets/Icon/post/baozhashitu.png", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(ExplodeBtn, "爆炸视图", ":/mMainWidgets/Icon/post/baozhashitu", "使用爆炸视图。");
			QObject::connect(ExplodeBtn, &QToolButton::clicked, ExplodeBtn, [bus]() { emit bus->clickPostExplodeSig(); });

			//截面剖切：打开截面剖切参数面板（V1.3 原型 #sectionCutParameters，作用当前
			//结果模型；图标取自原型 openSectionCut（立方体+切面））
			QToolButton* sectionCutBtn = spatialGW->addToolBtn("截面剖切", ":/mMainWidgets/Icon/post/jiemianpoqie.png", 0, 3, 2, 1, QSize(30, 30));
			host->setQToolButton(sectionCutBtn, "截面剖切", ":/mMainWidgets/Icon/post/jiemianpoqie", "定义剖切平面并裁剪显示模型。");
			QObject::connect(sectionCutBtn, &QToolButton::clicked, sectionCutBtn, [bus]() { emit bus->clickPostSectionCutSig(); });
		}

		if (analysisGW)
		{
			//截面力图标取自原型 #icon-fbd（立方体+三支蓝色力箭头），不再与测量工具共用
			QToolButton* SectionForceBtn = analysisGW->addToolBtn("截面力", ":/mMainWidgets/Icon/post/jiemianli.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(SectionForceBtn, "截面力", ":/mMainWidgets/Icon/post/jiemianli", "截面力。");
			QObject::connect(SectionForceBtn, &QToolButton::clicked, SectionForceBtn, [bus]() { emit bus->clickPostSectionForceSig(); });

			QToolButton* MeasureToolBtn = analysisGW->addToolBtn("测量工具", ":/mMainWidgets/Icon/post/svg/celianggongju.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(MeasureToolBtn, "测量工具", ":/mMainWidgets/Icon/post/svg/celianggongju.svg", "使用测量工具。");
			QObject::connect(MeasureToolBtn, &QToolButton::clicked, MeasureToolBtn, [bus]() { emit bus->clickPostMeasureToolSig(); });

			//结果标签面板（≠既有"标注"）；图标与树侧"标签"分支同源（biaoqian.svg 吊牌）
			QToolButton* labelBtn = analysisGW->addToolBtn("标签", ":/mMainWidgets/Icon/post/svg/biaoqian.svg", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(labelBtn, "标签", ":/mMainWidgets/Icon/post/svg/biaoqian.svg", "创建并管理结果标签。");
			QObject::connect(labelBtn, &QToolButton::clicked, labelBtn, [bus]() { emit bus->clickPostTagSig(); });

			//查询：V1.3 查询参数面板（查询对象及坐标系/查询值/查询结果；原型 resultQuery）
			//——旧"高级查询"面板仍由云图页同名按钮（clickAdvancedQuerySig）承载
			QToolButton* queryBtn = analysisGW->addToolBtn("查询", ":/mMainWidgets/Icon/post/gaojichaxun.png", 0, 3, 2, 1, QSize(30, 30));
			host->setQToolButton(queryBtn, "查询", ":/mMainWidgets/Icon/post/gaojichaxun", "结果查询。");
			QObject::connect(queryBtn, &QToolButton::clicked, queryBtn, [bus]() { emit bus->clickPostQuerySig(); });

			//曲线提取：打开参数面板（提取/绘制引擎待接入，面板按原型出全界面）；
			//图标取自原型 resultCurveExtract（坐标轴+曲线）
			QToolButton* curveExtractBtn = analysisGW->addToolBtn("曲线提取", ":/mMainWidgets/Icon/post/quxiantiqu.png", 0, 4, 2, 1, QSize(30, 30));
			host->setQToolButton(curveExtractBtn, "曲线提取", ":/mMainWidgets/Icon/post/quxiantiqu", "从结果中提取曲线。");
			QObject::connect(curveExtractBtn, &QToolButton::clicked, curveExtractBtn, [bus]() { emit bus->clickPostCurveExtractSig(); });
		}

		if (outputGW)
		{
			QToolButton* reportBtn = outputGW->addToolBtn("分析报告", ":/mMainWidgets/Icon/post/svg/fenxibaogao.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(reportBtn, "分析报告", ":/mMainWidgets/Icon/post/svg/fenxibaogao.svg", "输出结果分析报告。");
			QObject::connect(reportBtn, &QToolButton::clicked, reportBtn, [bus]() { emit bus->clickPostExportWordSig(); });

			QToolButton* VideoResultToolBtn = outputGW->addToolBtn("对比视频", ":/mMainWidgets/Icon/post/icon_对比视频（30x30）@3x.png", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(VideoResultToolBtn, "对比视频", ":/mMainWidgets/Icon/post/icon_对比视频（30x30）@3x", "对比视频");
			QObject::connect(VideoResultToolBtn, &QToolButton::clicked, VideoResultToolBtn, [bus]() { emit bus->clickPostCompVideoSig(); });

			//强度计算报告导出（规则模板容器一键导出；真机发现"报告"页在强度上下文不可达，
			//补挂结果页——与曲线模块"报告"页入口同一 showInstance 单例）
			QToolButton* strengthReportBtn = outputGW->addToolBtn("强度报告导出", ":/mMainWidgets/Icon/post/svg/qiangdubaogao.svg", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(strengthReportBtn, "强度报告导出", ":/mMainWidgets/Icon/post/svg/qiangdubaogao.svg", "按规则模板一键导出强度计算报告");
			QObject::connect(strengthReportBtn, &QToolButton::clicked, strengthReportBtn, [strengthReportBtn]() {
				StrengthReport::mStrengthReportDlg::showInstance(strengthReportBtn->window());
			});
		}

		if (casesGW)
		{
			//工况组对齐原型（步序列/线性叠加/包络）：步序列/包络接 V1.3 派生工况参数面板
			//（clickPostStepSequenceSig/clickPostEnvelopeSig，每次点击新建一条"步序列-N/包络-N"
			//定义）；线性叠加接云图页同名功能；三按钮图标均取自原型派生载荷
			//entries（步序列=阶梯条/线性叠加=加号/包络=包络线），不再共用同一张
			QToolButton* stepsBtn = casesGW->addToolBtn("步序列", ":/mMainWidgets/Icon/post/svg/buxulie.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(stepsBtn, "步序列", ":/mMainWidgets/Icon/post/svg/buxulie.svg", "按步序列定义派生工况。");
			QObject::connect(stepsBtn, &QToolButton::clicked, stepsBtn, [bus]() { emit bus->clickPostStepSequenceSig(); });

			QToolButton* linearBtn = casesGW->addToolBtn("线性叠加", ":/mMainWidgets/Icon/post/svg/xianxingdiejia.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(linearBtn, "线性叠加", ":/mMainWidgets/Icon/post/svg/xianxingdiejia.svg", "查看线性叠加后的云图。");
			QObject::connect(linearBtn, &QToolButton::clicked, linearBtn, [bus]() { emit bus->clickLinearSuperpositionSig(); });

			//包络接 V1.3 包络派生工况参数面板（clickPostEnvelopeSig，每次点击新建一条
			//"包络-N"定义）；定义组另含包络类型下拉（Min/Max/Extreme/Range，默认 Max）
			QToolButton* envelopeBtn = casesGW->addToolBtn("包络", ":/mMainWidgets/Icon/post/svg/baoluo.svg", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(envelopeBtn, "包络", ":/mMainWidgets/Icon/post/svg/baoluo.svg", "按包络类型定义派生工况。");
			QObject::connect(envelopeBtn, &QToolButton::clicked, envelopeBtn, [bus]() { emit bus->clickPostEnvelopeSig(); });
		}
	}

	void mPostMenuContributor::buildAnimationPage(IRibbonHost* host)
	{
		//同buildCloudPage：挂宿主控件下托管生命周期，退出期不再析构QWidget
		_animationMW = new mPostAnimationMenuWidget(dynamic_cast<QWidget*>(host));
		_animationMW->hide();

		host->createRibbonPage("动画");

		mGroupWidget* aniFrameGW = host->addGroup("动画", "帧数控制");
		mGroupWidget* aniControlGW = host->addGroup("动画", "动画控制");
		mGroupWidget* aniOutputGW = host->addGroup("动画", "输出");

		auto* bus = MechCommon::mMechGlobalSignals::getInstance();

		if (aniFrameGW)
		{
			//帧数控制组整饬（用户 2026-11-05 截图反馈"乱、不整齐"）：五个帧控钮
			//此前是半高小按钮（图标左文字右）单排悬在组内容区顶部、下方留出约
			//2/3 的空洞，且宽度随文字长短参差；改与云图页"新建"组一致的全高
			//按钮（图标上文字下，高=组内容区高，正好填满），并开同排等宽
			aniFrameGW->setUniformRowButtons(true);

			_animationMW->_btnFirst = aniFrameGW->addToolBtn("第一帧", ":/mMainWidgets/Icon/post/diyizheng.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(_animationMW->_btnFirst, "第一帧", ":/mMainWidgets/Icon/post/diyizheng.png", "切换至第一帧。");
			QObject::connect(_animationMW->_btnFirst, &QToolButton::clicked, _animationMW->_btnFirst, [bus]() {
				emit bus->clickAniFirstFrameSig();
				});

			_animationMW->_btnPrevious = aniFrameGW->addToolBtn("上一帧", ":/mMainWidgets/Icon/post/shangyizheng.png", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(_animationMW->_btnPrevious, "上一帧", ":/mMainWidgets/Icon/post/shangyizheng.png", "切换至上一帧。");
			QObject::connect(_animationMW->_btnPrevious, &QToolButton::clicked, _animationMW->_btnPrevious, [bus]() {
				emit bus->clickAniPrevFrameSig();
				});

			_animationMW->_btnNext = aniFrameGW->addToolBtn("下一帧", ":/mMainWidgets/Icon/post/xiayizheng.png", 0, 3, 2, 1, QSize(30, 30));
			host->setQToolButton(_animationMW->_btnNext, "下一帧", ":/mMainWidgets/Icon/post/xiayizheng.png", "切换至下一帧。");
			QObject::connect(_animationMW->_btnNext, &QToolButton::clicked, _animationMW->_btnNext, [bus]() {
				emit bus->clickAniNextFrameSig();
				});

			_animationMW->_btnLast = aniFrameGW->addToolBtn("最后一帧", ":/mMainWidgets/Icon/post/zuihouyizheng.png", 0, 4, 2, 1, QSize(30, 30));
			host->setQToolButton(_animationMW->_btnLast, "最后一帧", ":/mMainWidgets/Icon/post/zuihouyizheng.png", "切换至最后一帧。");
			QObject::connect(_animationMW->_btnLast, &QToolButton::clicked, _animationMW->_btnLast, [bus]() {
				emit bus->clickAniLastFrameSig();
				});

			//播放按钮初始按“未播放”构建（开始图标+未按下）：页面建于启动注册期，早于任何动画
			//生成；此前默认建成播放中外观，勾选“动画”页签调出页面时会误显示为正在播放
			_animationMW->_btnPlay = aniFrameGW->addToolBtn("开始", ":/mMainWidgets/Icon/post/kaishi.png", 0, 2, 2, 1, QSize(30, 30));
			host->setQToolButton(_animationMW->_btnPlay, "开始", ":/mMainWidgets/Icon/post/kaishi.png", "开始播放动画。");
			_animationMW->_btnPlay->setCheckable(true);
			_animationMW->_btnPlay->setChecked(false);
			QObject::connect(_animationMW->_btnPlay, &QToolButton::clicked, _animationMW->_btnPlay, [this, bus, host](bool play) {
				emit bus->clickAniPlaySig(play);

				_animationMW->_btnFirst->setEnabled(!play);
				_animationMW->_btnPrevious->setEnabled(!play);
				_animationMW->_btnNext->setEnabled(!play);
				_animationMW->_btnLast->setEnabled(!play);
				_animationMW->_sliderRate->setEnabled(!play);
				_animationMW->_lineEditRate->setEnabled(!play);
				_animationMW->_comboRange->setEnabled(!play);
				_animationMW->_checkLoopPlay->setEnabled(!play);
				if (_animationMW->_checkHideFailedElement != nullptr)
					_animationMW->_checkHideFailedElement->setEnabled(!play);

				//禁用动画参数帧控件
				emit bus->clickAniSetStatusSig(play);

				// 播放/暂停切换：仅换图标/文字/提示，尺寸不固化——
				// 切档后由 ThemeManager::applyAll 统一按档位重设，
				// 固化 60x60 会导致切档后播放钮与其它帧控钮高度不一致
				if (play)
				{
					_animationMW->_btnPlay->setIcon(QIcon(":/mMainWidgets/Icon/post/zanting.png"));
					_animationMW->_btnPlay->setText(QString("暂停"));
					host->installTooltip(_animationMW->_btnPlay, "暂停", ":/mMainWidgets/Icon/post/zanting.png", "暂停播放动画。", 200);
				}
				else
				{
					_animationMW->_btnPlay->setIcon(QIcon(":/mMainWidgets/Icon/post/kaishi.png"));
					_animationMW->_btnPlay->setText(QString("开始"));
					host->installTooltip(_animationMW->_btnPlay, "开始", ":/mMainWidgets/Icon/post/kaishi.png", "开始播放动画。", 200);
				}
				});

			_animationMW->_btnFirst->setEnabled(false);
			_animationMW->_btnPrevious->setEnabled(false);
			_animationMW->_btnNext->setEnabled(false);
			_animationMW->_btnLast->setEnabled(false);

			//禁用动画参数帧控件
			emit bus->clickAniSetStatusSig(false);
		}

		if (aniControlGW)
		{
			QLabel* labelRange = aniControlGW->addLabel("渲染范围", 0, 0, 1, 1);
			labelRange->setFixedWidth(80);
			_animationMW->_comboRange = aniControlGW->addCombobox("渲染范围", 0, 1, 1, 1);
			QStringList frameRange = { "当前帧范围","所有帧最大范围" };
			_animationMW->_comboRange->addItems(frameRange);
			_animationMW->_comboRange->setFixedWidth(120);
			QObject::connect(_animationMW->_comboRange, QOverload<const QString&>::of(&MxFunctions::mCheckComboBox::currentTextChanged), _animationMW->_comboRange, [bus](const QString& type) {
				emit bus->clickAniSetRenderRangeSig(type);
				});

			_animationMW->_checkLoopPlay = aniControlGW->addCheckBox("循环播放", 0, 2, 1, 2);
			_animationMW->_checkLoopPlay->setFixedWidth(100);
			_animationMW->_checkLoopPlay->setCheckable(true);
			_animationMW->_checkLoopPlay->setChecked(true);
			QObject::connect(_animationMW->_checkLoopPlay, &QCheckBox::toggled, _animationMW->_checkLoopPlay,
				[bus](bool checked) {
					emit bus->clickAniSetLoopPlaySig(checked);
				});

			_animationMW->_checkHideFailedElement = aniControlGW->addCheckBox("失效单元隐藏", 0, 3, 1, 2);
			_animationMW->_checkHideFailedElement->setFixedWidth(100);
			_animationMW->_checkHideFailedElement->setCheckable(true);
			_animationMW->_checkHideFailedElement->setChecked(true);
			QObject::connect(_animationMW->_checkHideFailedElement, &QCheckBox::toggled, _animationMW->_checkHideFailedElement,
				[bus](bool checked) {
					emit bus->clickAniSetHideFailedElementSig(checked);
				});


			QLabel* labelRate = aniControlGW->addLabel("动画速率", 1, 0, 1, 1);
			labelRate->setFixedWidth(80);

			_animationMW->_sliderRate = aniControlGW->addSlider("动画速率", 1, 1, 1, 1);
			_animationMW->_sliderRate->setFixedWidth(120);
			_animationMW->_sliderRate->setMinimum(1);
			_animationMW->_sliderRate->setMaximum(99);
			_animationMW->_sliderRate->setValue(10);

			_animationMW->_lineEditRate = new QLineEdit();
			aniControlGW->addLineEdit(_animationMW->_lineEditRate, 1, 2, 1, 1);
			_animationMW->_lineEditRate->setFixedWidth(60);
			_animationMW->_lineEditRate->setFixedHeight(25);
			_animationMW->_lineEditRate->setValidator(new QIntValidator(1, 99, _animationMW->_lineEditRate));
			_animationMW->_lineEditRate->setText(MxFunctions::mNumberToString(_animationMW->_sliderRate->value()));

			_animationMW->_sliderRate->setEnabled(false);
			_animationMW->_lineEditRate->setEnabled(false);

			QObject::connect(_animationMW->_sliderRate, &QSlider::valueChanged, _animationMW->_sliderRate, [this, bus](int value) {
				if (_animationMW->_lineEditRate->text().toInt() != value)
				{
					_animationMW->_lineEditRate->setText(MxFunctions::mNumberToString(value));
					emit bus->clickAniSetFrameRateSig(value);
				}
				});

			QObject::connect(_animationMW->_lineEditRate, &QLineEdit::editingFinished, _animationMW->_lineEditRate, [this, bus]() {
				int value = _animationMW->_lineEditRate->text().toInt();
				if (value >= 1 && value <= 99 && _animationMW->_sliderRate->value() != value)
				{
					_animationMW->_sliderRate->setValue(value);
					emit bus->clickAniSetFrameRateSig(value);
				}
				});

			QLabel* labelFps = aniControlGW->addLabel("fps", 1, 3, 1, 1);
			labelFps->setFixedWidth(30);
			labelFps->setAlignment(Qt::AlignLeft);
		}

		if (aniOutputGW)
		{
			_animationMW->_btnExportVideo = aniOutputGW->addToolBtn("导出视频", ":/mMainWidgets/Icon/post/daochushiping.png", 0, 0, 1, 1, QSize(16, 16), 10);
			// 高度不固化：随档位由 ThemeManager::applyAll 统一按小按钮高下发（固化26切档后不缩放）
			_animationMW->_btnExportVideo->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			host->installTooltip(_animationMW->_btnExportVideo, "导出视频", ":/mMainWidgets/Icon/post/daochushiping.png", "导出当前动画的全部帧。", 200);

			_animationMW->_btnCustomVideo = aniOutputGW->addToolBtn("自定义视频", ":/mMainWidgets/Icon/post/zidingyishiping", 1, 0, 1, 1, QSize(16, 16), 10);
			_animationMW->_btnCustomVideo->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			host->installTooltip(_animationMW->_btnCustomVideo, "自定义视频", ":/mMainWidgets/Icon/post/zidingyishiping", "自定义视频。", 200);

			QObject::connect(_animationMW->_btnExportVideo, &QToolButton::clicked, _animationMW->_btnExportVideo, [bus]() {
				emit bus->clickAniExportVideoSig();
				});

			QObject::connect(_animationMW->_btnCustomVideo, &QToolButton::clicked, _animationMW->_btnCustomVideo, [bus]() {
				emit bus->clickCreateCustomVideoSig();
				});
		}

		//初始整体禁用（构建期无动画上下文）：页面启停由宿主侧按激活视口控制，
		//控件状态随后续视口激活/动画生成事件同步
		_animationMW->setAnimationMenuStatus(false);

		//激活视口变化时按当前视口同步动画页控件状态：动画视口按其guidata的播放/停止态
		//同步，未生成动画的全禁用；非后处理视口时updateAnimationMenuGUI内部直接返回，
		//此时页面已由宿主侧refreshPostMenuEnabledState整体禁用，不依赖此处
		QObject::connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged,
			_animationMW, [this](int) {
				auto* viewportPrivate = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportPrivate();
				if (viewportPrivate != nullptr)
					_animationMW->updateAnimationMenuGUI(viewportPrivate->getViewportName());
			});
	}

	void mPostMenuContributor::buildNVHCurvePage(IRibbonHost* host)
	{
		host->createRibbonPage("曲线");

		PostCurve::mCurveViewportFactory* factory = PostCurve::mCurveViewportFactory::getInstance();

		//文件组：新建（导入csv/pch生成曲线，新建入口，已开发完成）
		mGroupWidget* openCurve = host->addGroup("曲线", "文件");
		if (openCurve)
		{
			//与主页「新建/打开」同款大按钮：构造传 QSize(30,30)（>24 判为 Mode::Big，图标在上/文字在下），
			//高度/图标/宽度由 setQToolButton 与 ThemeManager::applyAll 随档位统一下发，勿事后 setIconSize 固化
			QToolButton* openCurveBtn = openCurve->addToolBtn("新建", ":/mMainWidgets/Icon/Curve/工具栏/新建_大.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(openCurveBtn, "新建", ":/mMainWidgets/Icon/Curve/工具栏/新建_大.svg", "导入csv文件生成曲线");
			QObject::connect(openCurveBtn, &QToolButton::clicked, openCurveBtn, [openCurveBtn]() {
				PostCurve::mOpenCurveDlg::showInstance(openCurveBtn->window());      //原 mMenuWidget::slot_NVHCurve
				});
			//按激活图表类型显隐（声明表 ChartTypeProfile：柱状图隐藏，未声明类型/
			//无激活图表回退 XY=显示）。"新建"是文件组唯一入口，整组一并显隐
			auto syncNewBtn = [openCurveBtn, openCurve](int chartType)
			{
				const bool hidden = PostCurve::ribbonActionHidden(
					PostCurve::chartTypeProfile(chartType),
					PostCurve::ChartTypeProfile::RibbonAction::NewCurve);
				openCurveBtn->setVisible(!hidden);
				openCurve->setVisible(!hidden);
			};
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_currentChartTypeChanged,
				openCurveBtn, syncNewBtn);
			syncNewBtn(factory->currentChartType());
		}

		//曲线组：定义曲线/编辑曲线已接入（设计图10/16，模式开关：向当前激活视窗
		//的图表批量添加数据集通道 / 裁剪线交互+浮动窗口）
		mGroupWidget* editGroup = host->addGroup("曲线", "曲线");
		if (editGroup)
		{
			QToolButton* defineBtn = editGroup->addToolBtn("定义曲线", ":/mMainWidgets/Icon/Curve/工具栏/定义曲线_大.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(defineBtn, "定义曲线", ":/mMainWidgets/Icon/Curve/工具栏/定义曲线_大.svg", "新增/复制/表达式定义曲线");
			//默认禁用，当前激活视口为曲线视窗时启用（工厂的当前图表变化广播，
			//含非曲线视口的 -1）
			defineBtn->setEnabled(false);
			//定义曲线模式开关（checkable，保持按下直至再次点击/浮动窗口关闭）：
			//与编辑曲线/基准线/注释互斥——互斥在视口工厂收口，开启任一功能自动
			//退出其余，各按钮按下态经工厂信号回写
			defineBtn->setCheckable(true);
			QObject::connect(defineBtn, &QToolButton::clicked, defineBtn, [defineBtn](bool checked)
				{
					//浮动窗口单例先就位（ctor 已连工厂模式信号，显隐随模式）
					(void)PostCurve::mDefineCurveDlg::getInstance();
					PostCurve::mCurveViewportFactory::getInstance()->setDefineCurveMode(checked);
				});
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_defineCurveStarted,
				defineBtn, [defineBtn]() { defineBtn->setChecked(true); });
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_defineCurveStopped,
				defineBtn, [defineBtn]() { defineBtn->setChecked(false); });
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_currentChartChanged,
				defineBtn, [defineBtn](int chartId)
				{
					defineBtn->setEnabled(chartId >= 0);
				});

			QToolButton* editCurveBtn = editGroup->addToolBtn("编辑曲线", ":/mMainWidgets/Icon/Curve/工具栏/编辑曲线_大.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(editCurveBtn, "编辑曲线", ":/mMainWidgets/Icon/Curve/工具栏/编辑曲线_大.svg", "拖动裁剪线截取编辑曲线");
			//编辑曲线模式开关：保持按下直至再次点击（期间激活视窗显示可拖拽
			//裁剪线并弹出"编辑曲线"浮动窗口，设计图16）；会话状态经视口工厂
			//持有并广播。开启失败（无激活曲线视窗）/浮动窗口关闭时经
			//sig_editCurveStopped 回弹按钮
			editCurveBtn->setCheckable(true);
			QObject::connect(editCurveBtn, &QToolButton::clicked, editCurveBtn, [editCurveBtn](bool checked)
				{
					//浮动窗口单例先就位（ctor 已连工厂会话信号，显隐随会话）
					(void)PostCurve::mEditCurveDlg::getInstance();
					PostCurve::mCurveViewportFactory::getInstance()->setEditCurveMode(checked);
				});
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_editCurveStarted,
				editCurveBtn, [editCurveBtn]() { editCurveBtn->setChecked(true); });
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_editCurveStopped,
				editCurveBtn, [editCurveBtn]() { editCurveBtn->setChecked(false); });
			//按激活图表类型显隐（声明表 ChartTypeProfile：柱状图/复数图隐藏，
			//未声明类型/无激活图表回退 XY=显示；切到不适用的类型时会话已在
			//工厂侧随之结束，此处仅做整项隐藏）
			auto syncEditBtn = [editCurveBtn](int chartType)
			{
				editCurveBtn->setVisible(!PostCurve::ribbonActionHidden(
					PostCurve::chartTypeProfile(chartType),
					PostCurve::ChartTypeProfile::RibbonAction::EditCurve));
			};
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_currentChartTypeChanged,
				editCurveBtn, syncEditBtn);
			syncEditBtn(factory->currentChartType());

			//模板配置/动刚度曲线/VTF曲线/NTF曲线（设计图：曲线组中部四项，
            //模板配置/动刚度曲线/VTF曲线/NTF曲线已接入（模板配置生成动刚度/VTF/NTF
            //CSV 映射模板，各曲线对话框导入 pch+CSV 批量生成实测+目标值曲线）；
            //曲线统计亦已接入（见下方 statBtn）。图标暂借用既有曲线图标，
			//正式切图后期替换（同名SVG入qrc后改路径即可）
			const std::initializer_list<std::tuple<const char*, const char*, const char*>> scaffoldBtns = {
				std::make_tuple("模板配置", ":/mMainWidgets/Icon/Curve/工具栏/保存项目_大.svg", "配置动刚度/VTF曲线模板（pch 提取生成 CSV 映射文件）"),
				std::make_tuple("动刚度曲线", ":/mMainWidgets/Icon/Curve/工具栏/定义曲线_大.svg", "生成动刚度响应曲线"),
				std::make_tuple("VTF曲线", ":/mMainWidgets/Icon/Curve/工具栏/编辑曲线_大.svg", "生成振动传递率曲线"),
				std::make_tuple("NTF曲线", ":/mMainWidgets/Icon/Curve/工具栏/新建_大.svg", "生成噪声传递函数曲线"),
			};
			int scaffoldCol = 2;   //编辑曲线占 (0,1)，其后顺延
			for (const auto& scaffoldBtn : scaffoldBtns)
			{
				QToolButton* scaffoldBtnPtr = editGroup->addToolBtn(std::get<0>(scaffoldBtn), std::get<1>(scaffoldBtn), 0, scaffoldCol++, 2, 1, QSize(30, 30));
				host->setQToolButton(scaffoldBtnPtr, std::get<0>(scaffoldBtn), std::get<1>(scaffoldBtn), std::get<2>(scaffoldBtn));
				//已接入功能的按钮接线（唤起式对话框，同"新建"按钮模式）。
				//名字用同文件窄字面量 strcmp 比较（两侧同一执行字符集，字节级
				//一致；勿用 QLatin1String 比较中文——Latin1 域装不下中文，恒不等）
				if (std::strcmp(std::get<0>(scaffoldBtn), "模板配置") == 0)
				{
					QObject::connect(scaffoldBtnPtr, &QToolButton::clicked, scaffoldBtnPtr, [scaffoldBtnPtr]()
						{
							PostCurve::mTemplateConfigDlg::showInstance(scaffoldBtnPtr->window());
						});
				}
				else if (std::strcmp(std::get<0>(scaffoldBtn), "动刚度曲线") == 0)
				{
					QObject::connect(scaffoldBtnPtr, &QToolButton::clicked, scaffoldBtnPtr, [scaffoldBtnPtr]()
						{
							PostCurve::mDynStiffnessDlg::showInstance(scaffoldBtnPtr->window());
						});
				}
				else if (std::strcmp(std::get<0>(scaffoldBtn), "VTF曲线") == 0)
				{
					QObject::connect(scaffoldBtnPtr, &QToolButton::clicked, scaffoldBtnPtr, [scaffoldBtnPtr]()
						{
							PostCurve::mVtfCurveDlg::showInstance(scaffoldBtnPtr->window());
						});
				}
				else if (std::strcmp(std::get<0>(scaffoldBtn), "NTF曲线") == 0)
				{
					QObject::connect(scaffoldBtnPtr, &QToolButton::clicked, scaffoldBtnPtr, [scaffoldBtnPtr]()
						{
							PostCurve::mNtfCurveDlg::showInstance(scaffoldBtnPtr->window());
						});
				}
			}

			//曲线统计（设计图：点击弹出"曲线统计"弹窗——曲线形式单选/频率范围/
			//关闭/保存，保存经文件对话框导出 xlsx/pptx，见 mCurveStatisticsDlg）
			QToolButton* statBtn = editGroup->addToolBtn("曲线统计", ":/mMainWidgets/Icon/Curve/工具栏/基准线_大.svg", 0, scaffoldCol++, 2, 1, QSize(30, 30));
			host->setQToolButton(statBtn, "曲线统计", ":/mMainWidgets/Icon/Curve/工具栏/基准线_大.svg", "统计动刚度/VTF/NTF 实测曲线相对目标值的超限频段并导出 Excel");
			QObject::connect(statBtn, &QToolButton::clicked, statBtn, [statBtn]() {
				PostCurve::mCurveStatisticsDlg::showInstance(statBtn->window());
				});
		}
		//
		mGroupWidget* annotationGroup = host->addGroup("曲线", "标注");
		if (annotationGroup)
		{
			//基准线下拉按钮（设计图14，图6）：纵向/横向两个互斥项，点击进入对应
			//方向的基准线模式（保持激活，画布左键连续创建基准线，悬停曲线显示
			//X/Y 读数），再点同项退出；与注释模式互斥（按钮按下态经工厂信号同步）
			QToolButton* baselineBtn = annotationGroup->addToolBtn("基准线", ":/mMainWidgets/Icon/Curve/工具栏/基准线_大.svg", 0, 0, 2, 1, QSize(30, 30));
			baselineBtn->setProperty("mxHasMenu", true);
			host->setQToolButton(baselineBtn, "基准线", ":/mMainWidgets/Icon/Curve/工具栏/基准线_大.svg", "添加纵向/横向基准线");
			baselineBtn->setCheckable(true);
			baselineBtn->setPopupMode(QToolButton::InstantPopup);

			QMenu* baselineMenu = new MechMainWindow::mUiMenu(baselineBtn);
			// 勾选标识/行内容统一由 mUiStyle 自绘（干净打勾线），无底座凹槽
			MechMainWindow::ThemeManager::applyMenuStyle(baselineMenu);
			QAction* baselineVerticalAct = baselineMenu->addAction(
				QIcon(":/mMainWidgets/Icon/Curve/结构树/纵向基准.svg"), QStringLiteral("纵向基准线"));
			QAction* baselineHorizontalAct = baselineMenu->addAction(
				QIcon(":/mMainWidgets/Icon/Curve/结构树/横向基准.svg"), QStringLiteral("横向基准线"));
			baselineVerticalAct->setCheckable(true);
			baselineHorizontalAct->setCheckable(true);
			QActionGroup* baselineGroup = new QActionGroup(baselineMenu);
			baselineGroup->setExclusive(true);
			baselineGroup->addAction(baselineVerticalAct);
			baselineGroup->addAction(baselineHorizontalAct);
			baselineBtn->setMenu(baselineMenu);

			//再点同项=退出当前模式；点另一项=切向（工厂保证与注释模式互斥）
			auto toggleBaseline = [](int orientation)
			{
				PostCurve::mCurveViewportFactory* f = PostCurve::mCurveViewportFactory::getInstance();
				if (f->baselineMode() && f->baselineOrientation() == orientation)
					f->setBaselineMode(false, orientation);
				else
					f->setBaselineMode(true, orientation);
			};
			QObject::connect(baselineVerticalAct, &QAction::triggered, baselineBtn, [toggleBaseline]()
				{
					toggleBaseline(1);   //纵向（同 CurveBaselineData：1 纵向 0 横向）
				});
			QObject::connect(baselineHorizontalAct, &QAction::triggered, baselineBtn, [toggleBaseline]()
				{
					toggleBaseline(0);
				});

			QToolButton* annotationBtn = annotationGroup->addToolBtn("注释", ":/mMainWidgets/Icon/Curve/工具栏/注释_大.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(annotationBtn, "注释", ":/mMainWidgets/Icon/Curve/工具栏/注释_大.svg", "添加曲线注释");

			//注释模式开关：保持按下直至再次点击（期间左键在曲线视窗打点注释、
			//悬停曲线实时预览注释内容）；经视口工厂广播给全部已打开图表视图
			annotationBtn->setCheckable(true);
			QObject::connect(annotationBtn, &QToolButton::clicked, annotationBtn, [](bool checked)
				{
					PostCurve::mCurveViewportFactory::getInstance()->setAnnotationMode(checked);
				});

			//与基准线模式互斥的按钮态同步：开启基准线会联动退出注释模式（反之
			//亦然），两个按钮的按下态各自跟随工厂广播回写（用户点击已置位的除外）
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_baselineModeChanged,
				baselineBtn, [baselineBtn, baselineVerticalAct, baselineHorizontalAct](bool on, int orientation)
				{
					baselineBtn->setChecked(on);
					baselineVerticalAct->setChecked(on && orientation == 1);
					baselineHorizontalAct->setChecked(on && orientation == 0);
				});
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_annotationModeChanged,
				baselineBtn, [baselineBtn, annotationBtn](bool on)
				{
					annotationBtn->setChecked(on);
					if (on)
						baselineBtn->setChecked(false);   //互斥回写（菜单项勾选随基准线信号更新）
				});
		}

		//诊断组（同页组序：文件/曲线/标注/诊断，design D1）：模态贡献量分析唯一
		//入口——唤醒 NVHToolSet 独立窗口页（总线信号→mPostInfoHub::
		//createModalContribution→activateDock，openspec add-nvh-toolset-window-page）。
		//门控=出现层单层（2026-11-16 用户二次裁定"曲线模式即可点"）：本页属曲线
		//工作模式专属页签，按钮只在曲线模式可见（曲线窗口页显示⟹后处理界面已开），
		//可见即可点，无附加 dock 置灰链（dockOpenChanged/isDockOpen 链已撤）
		mGroupWidget* diagnoseGroup = host->addGroup("曲线", "诊断");
		if (diagnoseGroup)
		{
			//需求供图 模态贡献量分析_大@3x.png 入库（Icon/Curve/工具栏/，qrc 已注册），
			//与同页大按钮同款 30×30 档（>24 判为 Mode::Big，尺寸随 ThemeManager 统一下发）
			QToolButton* nvhContributionBtn = diagnoseGroup->addToolBtn("模态贡献量分析",
				":/mMainWidgets/Icon/Curve/工具栏/模态贡献量分析_大@3x.png", 0, 0, 1, 1, QSize(30, 30));
			host->setQToolButton(nvhContributionBtn, "模态贡献量分析",
				":/mMainWidgets/Icon/Curve/工具栏/模态贡献量分析_大@3x.png", "唤醒 NVH工具集窗口页进行模态贡献量分析");
			auto* bus = MechCommon::mMechGlobalSignals::getInstance();
			QObject::connect(nvhContributionBtn, &QToolButton::clicked, nvhContributionBtn, [bus]() {
				emit bus->clickPostModalContributionSig();
				});
		}
	}

	void mPostMenuContributor::build3DCurvePage(IRibbonHost* host)
	{
		host->createRibbonPage("3D曲线");

		//五组九钮（R37 重组，序=设计稿 design/3d-curve/ref-ribbon-3dcurve-tab-
		//full.png）：文件[新建]/曲线[定义曲线, 瀑布图]/格式[曲线样式, 坐标轴]/
		//查询[数值, 数据查询]/标注[基准, 注释]

		//文件组：新建（从文件新建三维图，Plot3D 渲染）——行为不变
		mGroupWidget* fileGroup = host->addGroup("3D曲线", "文件");
		if (fileGroup)
		{
			//与"曲线"页"新建"同款大按钮：构造传 QSize(30,30)（>24 判为 Big，
			//图标在上/文字在下），高度/图标/宽度由 setQToolButton 与
			//ThemeManager::applyAll 统一下发，勿事后 setIconSize 固化
			QToolButton* createBtn = fileGroup->addToolBtn("新建", ":/mMainWidgets/Icon/Curve/工具栏/新建_大.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(createBtn, "新建", ":/mMainWidgets/Icon/Curve/工具栏/新建_大.svg", "从文件新建三维图");
			QObject::connect(createBtn, &QToolButton::clicked, createBtn, [createBtn]() {
				PostCurve::mCreate3DPlotDlg::showInstance(createBtn->window());
				});
		}

		//曲线组：定义曲线（自文件组移入——图标/tooltip/showInstance 行为原样，
		//现无启停联动，维持）+ 瀑布图（新钮，工具页挂载）
		mGroupWidget* curveGroup = host->addGroup("3D曲线", "曲线");
		QToolButton* waterfallBtn = nullptr;
		if (curveGroup)
		{
			//"定义曲线"：同款大按钮（图标与 2D 定义曲线一致），点击唤出三维定义
			//曲线弹窗单例（仅组别布局调整，连接代码原样）
			QToolButton* define3DBtn = curveGroup->addToolBtn("定义曲线", ":/mMainWidgets/Icon/Curve/工具栏/定义曲线_大.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(define3DBtn, "定义曲线", ":/mMainWidgets/Icon/Curve/工具栏/定义曲线_大.svg", "按文件/数学运算/数值定义三维曲线");
			QObject::connect(define3DBtn, &QToolButton::clicked, define3DBtn, [define3DBtn]() {
				PostCurve::mDefine3DCurveDlg::showInstance(define3DBtn->window());
				});

			//"瀑布图"：大按钮（图标复用结构树筛选/绘图.svg，不新增 qrc 资源），
			//点击挂载瀑布图工具页（纯展示占位页）
			waterfallBtn = curveGroup->addToolBtn("瀑布图", ":/mMainWidgets/Icon/Curve/结构树筛选/绘图.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(waterfallBtn, "瀑布图", ":/mMainWidgets/Icon/Curve/结构树筛选/绘图.svg", "瀑布图工具页");
			QObject::connect(waterfallBtn, &QToolButton::clicked, waterfallBtn, [this](bool)
				{
					if (_waterfall3DPage.isNull())
						_waterfall3DPage = new PostCurve::mWaterfall3DPage;
					mount3DToolPage(_waterfall3DPage.data());
				});
		}

		//格式组：曲线样式（图表级工具页，挂载前刷新上下文）+ 坐标轴（选中首
		//曲线 x 壳走既有轴页链路）
		mGroupWidget* formatGroup = host->addGroup("3D曲线", "格式");
		QToolButton* styleBtn = nullptr;
		QToolButton* axisBtn = nullptr;
		if (formatGroup)
		{
			styleBtn = formatGroup->addToolBtn("曲线样式", ":/mMainWidgets/Icon/Curve/操作/应用样式.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(styleBtn, "曲线样式", ":/mMainWidgets/Icon/Curve/操作/应用样式.svg", "曲线样式工具页");
			QObject::connect(styleBtn, &QToolButton::clicked, styleBtn, [this](bool)
				{
					PostCurve::mCurveViewportFactory* f = PostCurve::mCurveViewportFactory::getInstance();
					if (_curveStyle3DPage.isNull())
						_curveStyle3DPage = new PostCurve::mCurveStyle3DPage;
					static_cast<PostCurve::mCurveStyle3DPage*>(_curveStyle3DPage.data())
						->setChart(f->currentChartId());   //挂载前刷新上下文（换图表重绑）
					mount3DToolPage(_curveStyle3DPage.data());
				});

			axisBtn = formatGroup->addToolBtn("坐标轴", ":/mMainWidgets/Icon/Curve/结构树筛选/坐标轴.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(axisBtn, "坐标轴", ":/mMainWidgets/Icon/Curve/结构树筛选/坐标轴.svg", "当前三维图首条曲线的 X 轴");
			//点击：选中当前激活三维图下第一条曲线（树序）的 AxisX3DData 固定子
			//——走既有 sig_currentNodeChanged→参数栏 editCurveNode 链路挂
			//mAxis3DPropPage，零新页面；无曲线/壳查不到时静默返回（防御）
			QObject::connect(axisBtn, &QToolButton::clicked, axisBtn, [](bool)
				{
					PostCurve::mCurveDocController* controller = PostCurve::mCurveDocController::getInstance();
					PostCurve::CurveDocument* doc = controller->document();
					PostCurve::mCurveViewportFactory* f = PostCurve::mCurveViewportFactory::getInstance();
					const int chartId = f->currentChartId();
					const PostCurve::CurveNode* chart = (chartId > 0) ? doc->node(chartId) : nullptr;
					if (!chart || chart->type != PostCurve::CurveNodeType::Chart
						|| chart->chart.chartType != 5)
						return;
					for (int childId : doc->childNodeIds(chartId))
					{
						const PostCurve::CurveNode* c = doc->node(childId);
						if (!c || c->type != PostCurve::CurveNodeType::Curve)
							continue;   //树序第一条曲线
						for (int shellId : doc->childNodeIds(childId))
						{
							const PostCurve::CurveNode* shell = doc->node(shellId);
							if (shell && shell->type == PostCurve::CurveNodeType::AxisX3DData)
							{
								controller->setCurrentNodeId(shellId);
								return;
							}
						}
						return;   //首条曲线无 x 壳（异常骨架）：静默返回，不落到后续曲线
					}
				});
		}

		//查询组：数值（寻点占位页）+ 数据查询（截面查询占位页）
		mGroupWidget* queryGroup = host->addGroup("3D曲线", "查询");
		QToolButton* seekBtn = nullptr;
		QToolButton* queryBtn = nullptr;
		if (queryGroup)
		{
			seekBtn = queryGroup->addToolBtn("数值", ":/mMainWidgets/Icon/Curve/操作/选取.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(seekBtn, "数值", ":/mMainWidgets/Icon/Curve/操作/选取.svg", "数值工具页");
			QObject::connect(seekBtn, &QToolButton::clicked, seekBtn, [this](bool)
				{
					if (_pointSeek3DPage.isNull())
						_pointSeek3DPage = new PostCurve::mPointSeek3DPage;
					mount3DToolPage(_pointSeek3DPage.data());
				});

			queryBtn = queryGroup->addToolBtn("数据查询", ":/mMainWidgets/Icon/Curve/操作/后处理.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(queryBtn, "数据查询", ":/mMainWidgets/Icon/Curve/操作/后处理.svg", "数据查询工具页");
			QObject::connect(queryBtn, &QToolButton::clicked, queryBtn, [this](bool)
				{
					if (_sectionQuery3DPage.isNull())
						_sectionQuery3DPage = new PostCurve::mSectionQuery3DPage;
					mount3DToolPage(_sectionQuery3DPage.data());
				});
		}

		//标注组（R34 三维图注释 + R37 基准新钮）：基准在既有"注释"左侧；
		//注释钮及其联动零改动
		mGroupWidget* annoGroup = host->addGroup("3D曲线", "标注");
		QToolButton* datumBtn = nullptr;
		if (annoGroup)
		{
			datumBtn = annoGroup->addToolBtn("基准", ":/mMainWidgets/Icon/Curve/结构树筛选/基准.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(datumBtn, "基准", ":/mMainWidgets/Icon/Curve/结构树筛选/基准.svg", "基准工具页");
			QObject::connect(datumBtn, &QToolButton::clicked, datumBtn, [this](bool)
				{
					if (_datum3DPage.isNull())
						_datum3DPage = new PostCurve::mDatum3DPage;
					mount3DToolPage(_datum3DPage.data());
				});

			QToolButton* anno3DBtn = annoGroup->addToolBtn("注释", ":/mMainWidgets/Icon/Curve/工具栏/注释_大.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(anno3DBtn, "注释", ":/mMainWidgets/Icon/Curve/工具栏/注释_大.svg", "三维图注释：激活后在图内容点击创建");

			//启停联动（对齐"定义曲线"按钮先例）：经工厂 sig_currentChartChanged
			//广播的当前激活图表 id + 文档查表（chartType==5）判定——当前激活图表
			//为三维图时启用，否则置灰（含无激活图表的 -1）
			PostCurve::mCurveViewportFactory* factory = PostCurve::mCurveViewportFactory::getInstance();
			auto syncAnnoBtn = [anno3DBtn](int chartId)
			{
				PostCurve::CurveDocument* doc = PostCurve::mCurveDocController::getInstance()->document();
				const PostCurve::CurveNode* chart = (chartId > 0) ? doc->node(chartId) : nullptr;
				anno3DBtn->setEnabled(chart && chart->type == PostCurve::CurveNodeType::Chart
					&& chart->chart.chartType == 5);
			};
			anno3DBtn->setEnabled(false);
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_currentChartChanged,
				anno3DBtn, syncAnnoBtn);
			syncAnnoBtn(factory->currentChartId());

			//点击（R46 对齐二维曲线注释交互）：按钮检出态=进入三维注释模式——
			//模式内画布左键单击曲线/曲面/柱状等内容点创建牵引线注释（锚点=拾取
			//点），内容外点击不创建且模式保持；再次点击按钮退出。经视口工厂
			//广播给全部已打开三维图视图（工厂持有模式状态，新物化视图自动承接）
			anno3DBtn->setCheckable(true);
			QObject::connect(anno3DBtn, &QToolButton::clicked, anno3DBtn, [](bool checked)
				{
					PostCurve::mCurveViewportFactory::getInstance()->setAnnotation3DMode(checked);
				});
			//按下态随工厂信号同步（开启被拒/互斥踢出/末视口关闭自动退出时回弹）
			QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_annotation3DModeChanged,
				anno3DBtn, [anno3DBtn](bool on)
				{
					anno3DBtn->setChecked(on);
				});
		}

		//六新钮统一启停联动（R34 注释钮同款，与注释钮各自连接并存）：共用一个
		//判定——当前激活图表为三维图（chartType==5）时启用，否则置灰（含 -1）。
		//连接上下文对象用按钮自身防悬垂（按钮随 Ribbon 宿主销毁时连接自动断开）；
		//构造时按 currentChartId() 初同步一次。按钮列表存 QPointer（评审建议）：
		//任一存活连接触发时会对列表里全部按钮 setEnabled，裸指针在 Ribbon 页
		//部分析构的窗口期有悬垂面，QPointer 自动塌缩为 nullptr
		PostCurve::mCurveViewportFactory* factory = PostCurve::mCurveViewportFactory::getInstance();
		const QList<QPointer<QToolButton>> toolBtns = {
			waterfallBtn, styleBtn, axisBtn, seekBtn, queryBtn, datumBtn };
		auto syncToolBtns = [toolBtns](int chartId)
		{
			PostCurve::CurveDocument* doc = PostCurve::mCurveDocController::getInstance()->document();
			const PostCurve::CurveNode* chart = (chartId > 0) ? doc->node(chartId) : nullptr;
			const bool ok = chart && chart->type == PostCurve::CurveNodeType::Chart
				&& chart->chart.chartType == 5;
			for (const QPointer<QToolButton>& b : toolBtns)
			{
				if (b)
					b->setEnabled(ok);
			}
		};
		for (const QPointer<QToolButton>& b : toolBtns)
		{
			if (b)
			{
				b->setEnabled(false);
				QObject::connect(factory, &PostCurve::mCurveViewportFactory::sig_currentChartChanged,
					b, syncToolBtns);
			}
		}
		syncToolBtns(factory->currentChartId());
	}

	void mPostMenuContributor::mount3DToolPage(QWidget* page)
	{
		if (page == nullptr)
			return;
		//挂载序列照 mPostInfoHub::createCurve（unEdit → activateDock → 挂载）
		MechCommon::panelHostRef()->unEditInfoWindow();
		mDockLayoutManager::getInstance()->activateDock("Info");
		//窄宽压缩（mPostInfoHub::mountToHost 同款，"只降不升"）：页面自身已按
		//B01/B02 自适配（行按钮 minimumWidth(1) 等），此处仅对受全局 QSS
		//width:80px 支配的按钮/最长条目下拉再降下限，幂等可重复挂载
		const auto effectiveMin = [](QWidget* w) { return qMax(w->minimumWidth(), w->minimumSizeHint().width()); };
		for (QComboBox* combo : page->findChildren<QComboBox*>())
			combo->setMinimumWidth(qMin(50, effectiveMin(combo)));
		for (QPushButton* btn : page->findChildren<QPushButton*>())
			btn->setMinimumWidth(qMin(qMin(52, effectiveMin(btn)), btn->maximumWidth()));
		for (QLineEdit* edit : page->findChildren<QLineEdit*>())
		{
			QWidget* pw = edit->parentWidget();
			if (qobject_cast<QAbstractSpinBox*>(pw) != nullptr || qobject_cast<QComboBox*>(pw) != nullptr)
				continue;   //微调框/下拉内部输入框由父控件自管几何
			edit->setMinimumWidth(qMin(40, effectiveMin(edit)));
		}
		for (QCheckBox* check : page->findChildren<QCheckBox*>())
		{
			check->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
			check->setMinimumWidth(check->minimumSizeHint().width());
		}
		for (QRadioButton* radio : page->findChildren<QRadioButton*>())
		{
			radio->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
			radio->setMinimumWidth(radio->minimumSizeHint().width());
		}
		MechCommon::panelHostRef()->setMainWidget(page);
	}

	void mPostMenuContributor::buildReportPage(IRibbonHost* host)
	{
		host->createRibbonPage("报告");

		//模板组：保存模板/模板曲线（设计图：报告页·模板组；add-template-curve D6）
		mGroupWidget* templateGroup = host->addGroup("报告", "模板");
		if (templateGroup)
		{
			QToolButton* saveTemplateBtn = templateGroup->addToolBtn("保存模板", ":/mMainWidgets/Icon/Curve/工具栏/保存项目_大.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(saveTemplateBtn, "保存模板", ":/mMainWidgets/Icon/Curve/工具栏/保存项目_大.svg", "将当前曲线页面保存为模板文件（*.mxtpl）");
			//整文档保存链路：另存为对话框→落盘→登记 recents（任务 2.1/2.2）
			QObject::connect(saveTemplateBtn, &QToolButton::clicked, saveTemplateBtn, []() {
				PostCurve::mCurveTemplateManager::getInstance()->saveCurrentAsTemplate(
					PostCurve::mCurveDocController::getInstance()->document(),
					nullptr);
			});

			//"模板编辑"脚手架更名"模板曲线"（入口名对齐设计图），接线打开应用对话框单例
			QToolButton* editTemplateBtn = templateGroup->addToolBtn("模板曲线", ":/mMainWidgets/Icon/Curve/工具栏/编辑曲线_大.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(editTemplateBtn, "模板曲线", ":/mMainWidgets/Icon/Curve/工具栏/编辑曲线_大.svg", "应用模板生成曲线（追加/叠加/替换）");
			QObject::connect(editTemplateBtn, &QToolButton::clicked, editTemplateBtn, []() {
				PostCurve::mTemplateCurveDlg::showInstance(nullptr);
			});
		}

		//输出组：报告导出（弹"输出报告"对话框，曲线图表视窗截图导出 PPT/HTML/Word）
		mGroupWidget* outputGroup = host->addGroup("报告", "输出");
		if (outputGroup)
		{
			QToolButton* exportReportBtn = outputGroup->addToolBtn("报告导出", ":/mMainWidgets/Icon/post/svg/baogao.svg", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(exportReportBtn, "报告导出", ":/mMainWidgets/Icon/post/svg/baogao.svg", "按所选格式导出报告");
			QObject::connect(exportReportBtn, &QToolButton::clicked, exportReportBtn, [exportReportBtn]()
				{
					mExportReportDlg::showInstance(exportReportBtn->window());
				});
		}

		//输出组并列：强度计算报告导出（规则模板容器一键导出目标 docx；R10 独立对话框+新入口；
		//03 §1.1-C 唯一既有代码触点，纯追加不改既有行）。图标=资源库独立图标 fenxibaogao.png，
		//与同组既有"报告导出"（icon_导出报告@3x.png）区分防误点（B9）；非模态 showInstance
		//（T-23/U-01=A：页签③"切变量↔回抓取"交替流要求）
		//强度上下文入口另见 buildResultPage 报告与输出组（真机发现本页强度上下文不可达，两入口同一单例）
		if (outputGroup)
		{
			QToolButton* strengthReportBtn = outputGroup->addToolBtn("强度报告导出", ":/mMainWidgets/Icon/post/svg/qiangdubaogao.svg", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(strengthReportBtn, "强度报告导出", ":/mMainWidgets/Icon/post/svg/qiangdubaogao.svg", "按规则模板一键导出强度计算报告");
			QObject::connect(strengthReportBtn, &QToolButton::clicked, strengthReportBtn, [strengthReportBtn]()
				{
					StrengthReport::mStrengthReportDlg::showInstance(strengthReportBtn->window());
				});
		}
	}

	void mPostMenuContributor::buildShellPages(IRibbonHost* host)
	{
		//空壳页：无内容，仅为动态页签切换注册页名（页名与树侧switchMenuBarWindow发送的名字一致）
		static const char* shellPageNames[] = {
			"路径", "流线图", "切面图", "等值面图", "矢量图",
			"分区显示", "后处理-集合", "线性叠加", "分析报告"
		};
		for (const char* name : shellPageNames)
		{
			host->createRibbonPage(QString::fromUtf8(name));
		}
	}

	void mPostMenuContributor::buildCurvePage(IRibbonHost* host)
	{
		//旧后处理曲线页（后处理树点曲线节点时 switchMenuBarWindow 落此页；无页签显示）。
		//原页名"曲线"让位给曲线工作台的"曲线"页（buildNVHCurvePage），改名避免
		//createRibbonPage 同名复用把两组内容并进一页、以及候选名单误判切工作模式
		host->createRibbonPage("后处理-曲线");

		mGroupWidget* dataGW = host->addGroup("后处理-曲线", "数据");
		mGroupWidget* styleGW = host->addGroup("后处理-曲线", "样式");
		mGroupWidget* toolGW = host->addGroup("后处理-曲线", "工具");

		if (dataGW)
		{
			_curveBtn = dataGW->addToolBtn("曲线", ":/mMainWidgets/Icon/post/svg/quxian.svg", 0, 0);
			// 尺寸不固化：addToolBtn 已按意图图标 30 打标为大按钮并给规则宽度兜底，
			// 高度/宽度/图标由 ThemeManager::applyAll 随档位统一重设（固化 60x45 切档后错位）
			host->installTooltip(_curveBtn, "曲线", ":/mMainWidgets/Icon/post/svg/quxian.svg", "设置曲线显隐。", 200);
			// 该按钮绕过 setQToolButton，主题样式需显式下发一次（applyAll 重放时也会再下发）
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(_curveBtn);

			QMenu* menu = new MechMainWindow::mUiMenu(_curveBtn);
			// 动态填充的曲线显隐勾选也走 mUiStyle 自绘打勾（无底座凹槽）
			MechMainWindow::ThemeManager::applyMenuStyle(menu);
			_curveBtn->setMenu(menu);
			// 保证主按钮点击即弹出
			_curveBtn->setPopupMode(QToolButton::InstantPopup);

			// 在菜单 aboutToShow 时刷新内容
			QObject::connect(menu, &QMenu::aboutToShow, menu, [=] {
				menu->clear();

				auto* vpMgr = MechPreData::mViewportModelManager::getInstance();
				auto items = vpMgr->getCurrentViewportPrivate()->getTreeSelectItems();
				if (items.size() == 0) return;
				auto item = items.at(0);
				QString viewport = vpMgr->getCurrentViewportModel()->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getViewNameByID(PostData::CurveItem, item._phyID);
				PostData::mPostCurveData* curveData = vpMgr->getCurrentViewportModel()->getModelData()->getManager<PostData::mPostDataManager>(Physical_Post).getCurveData(viewport);
				if (curveData == nullptr)	return;

				// 动态添加
				for (int i = 0; i < curveData->_curveList.size(); ++i)
				{
					auto c = curveData->_curveList.at(i);
					QAction* act = menu->addAction(c.name);
					act->setCheckable(true);
					act->setChecked(c.visible);
					act->setData(i);
					QObject::connect(act, &QAction::toggled, curveData,
						[curveData, i](bool checked)
						{
							curveData->setCurveVisible(i, checked);
							//更新曲线显示
							emit MechCommon::mMechGlobalSignals::getInstance()->clickUpdateCurveDisplaySig();
						}
					);
				}
				});

			QToolButton* barBtn = dataGW->addToolBtn("柱状图", ":/mMainWidgets/Icon/post/zhuzhuangtu.png", 0, 1);
			host->installTooltip(barBtn, "柱状图", ":/mMainWidgets/Icon/post/zhuzhuangtu.png", "查看柱状图。", 200);
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(barBtn);
			QObject::connect(barBtn, &QToolButton::clicked, barBtn, [=]() {
				//TODO 柱状图功能未实现（原实现即为空）
				});
			barBtn->hide();//htx add 无功能实现，暂时隐藏

			QToolButton* tableBtn = dataGW->addToolBtn("表格", ":/mMainWidgets/Icon/post/biaoge.png", 0, 2);
			host->installTooltip(tableBtn, "表格", ":/mMainWidgets/Icon/post/biaoge.png", "查看表格数据。", 200);
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(tableBtn);
			QObject::connect(tableBtn, &QToolButton::clicked, tableBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickShowTableDataSig();
				});
		}

		if (styleGW)
		{
			QToolButton* curveStyleBtn = styleGW->addToolBtn("曲线样式", ":/mMainWidgets/Icon/post/quxianyangshi.png", 0, 1);
			host->installTooltip(curveStyleBtn, "曲线样式", ":/mMainWidgets/Icon/post/quxianyangshi.png", "设置曲线样式。", 200);
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(curveStyleBtn);
			QObject::connect(curveStyleBtn, &QToolButton::clicked, curveStyleBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickCurveStyleSig();
				});

			QToolButton* viewStyleBtn = styleGW->addToolBtn("视图样式", ":/mMainWidgets/Icon/post/shituyangshi.png", 0, 2);
			host->installTooltip(viewStyleBtn, "视图样式", ":/mMainWidgets/Icon/post/shituyangshi.png", "设置视图样式。", 200);
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(viewStyleBtn);
			QObject::connect(viewStyleBtn, &QToolButton::clicked, viewStyleBtn, [=]() {
				//TODO 视图样式功能未实现（原实现即为空）
				});
			viewStyleBtn->hide();//htx add 无功能实现先隐藏
		}

		if (toolGW)
		{
			QToolButton* pathBtn = toolGW->addToolBtn("路径", ":/mMainWidgets/Icon/post/svg/lujing.svg", 0, 0);
			host->installTooltip(pathBtn, "路径", ":/mMainWidgets/Icon/post/svg/lujing.svg", "创建路径。", 200);
			MechMainWindow::ThemeManager::instance()->applyRibbonButtonTheme(pathBtn);
			QObject::connect(pathBtn, &QToolButton::clicked, pathBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickPathSig();
				});
		}
	}

	void mPostMenuContributor::buildNotePage(IRibbonHost* host)
	{
		host->createRibbonPage("注释");

		mGroupWidget* dataGW = host->addGroup("注释", "新建");

		if (dataGW)
		{
			QToolButton* lineBtn = dataGW->addToolBtn("二维线", ":/mMainWidgets/Icon/post/svg/erweixian.svg", 0, 0);
			host->setQToolButton(lineBtn, "二维线", ":/mMainWidgets/Icon/post/svg/erweixian.svg", "创建二维线。");
			lineBtn->setIconSize(QSize(30, 30));
			QObject::connect(lineBtn, &QToolButton::clicked, lineBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickLineSig();
				});

			QToolButton* arrowBtn = dataGW->addToolBtn("箭头", ":/mMainWidgets/Icon/post/svg/jiantou.svg", 0, 1);
			host->setQToolButton(arrowBtn, "箭头", ":/mMainWidgets/Icon/post/svg/jiantou.svg", "创建箭头。");
			arrowBtn->setIconSize(QSize(30, 30));
			QObject::connect(arrowBtn, &QToolButton::clicked, arrowBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickArrowSig();
				});

			QToolButton* textBtn = dataGW->addToolBtn("文本", ":/mMainWidgets/Icon/post/svg/wenben.svg", 0, 2);
			host->setQToolButton(textBtn, "文本", ":/mMainWidgets/Icon/post/svg/wenben.svg", "创建文本。");
			textBtn->setIconSize(QSize(30, 30));
			QObject::connect(textBtn, &QToolButton::clicked, textBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickTextSig();
				});

			QToolButton* logBtn = dataGW->addToolBtn("Log", ":/mMainWidgets/Icon/post/svg/log.svg", 0, 3);
			host->setQToolButton(logBtn, "Log", ":/mMainWidgets/Icon/post/svg/log.svg", "导入Log文件。");
			logBtn->setIconSize(QSize(30, 30));
			QObject::connect(logBtn, &QToolButton::clicked, logBtn, [=]() {
				emit MechCommon::mMechGlobalSignals::getInstance()->clickLogSig();
				});
		}
	}

	/*
	* "显示"页（后处理独立页）：布局自前处理 mMenuWidget::initDisplayMenu 移植，
	* 功能槽在 mPostDisplayMenuFunction（视图组为前后处理视口通用的相机操作）
	*/
	void mPostMenuContributor::buildDisplayPage(IRibbonHost* host)
	{
		auto* fn = mPostDisplayMenuFunction::getInstance();

		//页键带#Post后缀与前处理同名页分离，页签文本仍为"显示"（见mMenuWidget::pageForTabText）
		host->createRibbonPage(MechMainWindow::kPostDisplayPageKey);

		mGroupWidget* orientGW = host->addGroup(MechMainWindow::kPostDisplayPageKey, "视图");
		mGroupWidget* entityGW = host->addGroup(MechMainWindow::kPostDisplayPageKey, "实体化");
		mGroupWidget* modelGW = host->addGroup(MechMainWindow::kPostDisplayPageKey, "模型");

		if (orientGW)
		{
			// ===== 步骤 / 旋转Sx/Sy/Sz：左键正向、右键反向 =====
			QToolButton* btn = orientGW->addToolBtn("步骤", ":/mMainWidgets/Icon/display/buzhou.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(btn, "步骤", ":/mMainWidgets/Icon/display/buzhou.png", "左键：上一个视角\n右键：下一个视角");
			auto* stepFilter = new PostDualClickFilter(btn);
			stepFilter->onRightClick = [fn]() { fn->slot_changeViewList(true); };
			btn->installEventFilter(stepFilter);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn]() {
				fn->slot_changeViewList(false);
				});

			// 度数输入框（旋转角度）
			auto* angleEdit = new QDoubleSpinBox(orientGW);
			angleEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
			angleEdit->setRange(0.1, 359);
			angleEdit->setDecimals(1);
			angleEdit->setValue(30);
			angleEdit->setMinimumWidth(46);
			angleEdit->setFixedHeight(UiMetrics::menuItemHeight());

			const QString rotNames[3] = { "旋转Sx", "旋转Sy", "旋转Sz" };
			const QString rotIcons[3] = { ":/mMainWidgets/Icon/display/xuanzhuanSx.png",
				":/mMainWidgets/Icon/display/xuanzhuanSy.png",
				":/mMainWidgets/Icon/display/xuanzhuanSz.png" };
			for (int i = 0; i < 3; ++i)
			{
				const QString name = rotNames[i];
				const QString icon = rotIcons[i];
				const QString axisPos = QString("旋转+S%1").arg(QChar('x' + i));
				const QString axisNeg = QString("旋转-S%1").arg(QChar('x' + i));
				const QString tip = QString("左键：绕屏幕%1轴正向旋转\n右键：绕屏幕%1轴反向旋转").arg(QChar('x' + i));

				btn = orientGW->addToolBtn(name, icon, 0, 1 + i, 2, 1, QSize(30, 30));
				host->setQToolButton(btn, name, icon, tip);
				auto* rotFilter = new PostDualClickFilter(btn);
				rotFilter->onRightClick = [fn, axisNeg, angleEdit]() {
					fn->slot_angleRotation(axisNeg, angleEdit->text().toDouble());
				};
				btn->installEventFilter(rotFilter);
				QObject::connect(btn, &QToolButton::pressed, btn, [fn, axisPos, angleEdit]() {
					fn->slot_angleRotation(axisPos, angleEdit->text().toDouble());
					});
			}

			// 度数框+“度”标签，垂直居中于两行
			{
				auto* hbox = new QHBoxLayout();
				hbox->setContentsMargins(0, 0, 0, 0);
				hbox->addWidget(angleEdit);
				auto* label = new QLabel("度");
				label->setFixedWidth(15);
				hbox->addWidget(label);
				auto* vbox = new QVBoxLayout();
				vbox->setContentsMargins(0, 0, 0, 0);
				vbox->addStretch();
				vbox->addLayout(hbox);
				orientGW->addBoxLayout(vbox, 0, 4, 2, 1);
			}

			// ===== 方向键T字形布局 + 平移百分比输入框 =====
			auto* panEdit = new QDoubleSpinBox(orientGW);
			panEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
			panEdit->setRange(1, 100);
			panEdit->setDecimals(0);
			panEdit->setValue(10);
			panEdit->setMinimumWidth(40);
			panEdit->setFixedHeight(UiMetrics::menuItemHeight());

			btn = orientGW->addToolBtn("向上", ":/mMainWidgets/Icon/display/xiangshang.png", 0, 6, 1, 1, QSize(16, 16));
			host->setQToolButton(btn, "向上", ":/mMainWidgets/Icon/display/xiangshang.png", "向上平移", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn, panEdit]() {
				fn->slot_directionPan("up", panEdit->text().toDouble());
				});

			btn = orientGW->addToolBtn("向下", ":/mMainWidgets/Icon/display/xiangxia.png", 1, 6, 1, 1, QSize(16, 16));
			host->setQToolButton(btn, "向下", ":/mMainWidgets/Icon/display/xiangxia.png", "向下平移", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn, panEdit]() {
				fn->slot_directionPan("down", panEdit->text().toDouble());
				});

			btn = orientGW->addToolBtn("向左", ":/mMainWidgets/Icon/display/xiangzuo.png", 1, 5, 1, 1, QSize(16, 16));
			host->setQToolButton(btn, "向左", ":/mMainWidgets/Icon/display/xiangzuo.png", "向左平移", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn, panEdit]() {
				fn->slot_directionPan("left", panEdit->text().toDouble());
				});

			btn = orientGW->addToolBtn("向右", ":/mMainWidgets/Icon/display/xiangyou.png", 1, 7, 1, 1, QSize(16, 16));
			host->setQToolButton(btn, "向右", ":/mMainWidgets/Icon/display/xiangyou.png", "向右平移", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn, panEdit]() {
				fn->slot_directionPan("right", panEdit->text().toDouble());
				});

			// 百分比输入框+“%”标签
			{
				auto* hbox = new QHBoxLayout();
				hbox->setContentsMargins(0, 0, 0, 0);
				hbox->addWidget(panEdit);
				auto* label = new QLabel("%");
				label->setFixedWidth(15);
				hbox->addWidget(label);
				auto* vbox = new QVBoxLayout();
				vbox->setContentsMargins(0, 0, 0, 0);
				vbox->addStretch();
				vbox->addLayout(hbox);
				orientGW->addBoxLayout(vbox, 0, 8, 2, 1);
			}

			// ===== 缩放（左键放大、右键缩小）+ 倍数输入框 =====
			auto* zoomEdit = new QDoubleSpinBox(orientGW);
			zoomEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
			zoomEdit->setRange(0.1, 100);
			zoomEdit->setDecimals(1);
			zoomEdit->setValue(2);
			zoomEdit->setMinimumWidth(42);
			zoomEdit->setFixedHeight(UiMetrics::menuItemHeight());

			btn = orientGW->addToolBtn("缩放", ":/mMainWidgets/Icon/display/suofang.png", 0, 9, 2, 1, QSize(30, 30));
			host->setQToolButton(btn, "缩放", ":/mMainWidgets/Icon/display/suofang.png", "左键：放大\n右键：缩小");
			auto* zoomFilter = new PostDualClickFilter(btn);
			zoomFilter->onRightClick = [fn, zoomEdit]() {
				fn->slot_valueZoom(false, zoomEdit->text().toDouble());
			};
			btn->installEventFilter(zoomFilter);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn, zoomEdit]() {
				fn->slot_valueZoom(true, zoomEdit->text().toDouble());
				});

			// 倍数输入框+“倍”标签
			{
				auto* hbox = new QHBoxLayout();
				hbox->setContentsMargins(0, 0, 0, 0);
				hbox->addWidget(zoomEdit);
				auto* label = new QLabel("倍");
				label->setFixedWidth(15);
				hbox->addWidget(label);
				auto* vbox = new QVBoxLayout();
				vbox->setContentsMargins(0, 0, 0, 0);
				vbox->addStretch();
				vbox->addLayout(hbox);
				orientGW->addBoxLayout(vbox, 0, 10, 2, 1);
			}
		}

		if (entityGW)
		{
			// 梁/杆实体化切换大按钮：按下显示实体化，弹起隐藏
			auto* btn = entityGW->addToolBtn("梁", ":/mMainWidgets/Icon/display/xianshiquanbuliang.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(btn, "梁", ":/mMainWidgets/Icon/display/xianshiquanbuliang.png", "按下：显示全部梁实体化\n弹起：隐藏全部梁实体化");
			btn->setCheckable(true);
			QObject::connect(btn, &QToolButton::toggled, btn, [fn](bool checked) {
				fn->slot_setAllBeamVolume(checked);
				});

			btn = entityGW->addToolBtn("杆", ":/mMainWidgets/Icon/display/xianshiquanbugan.png", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(btn, "杆", ":/mMainWidgets/Icon/display/xianshiquanbugan.png", "按下：显示全部杆实体化\n弹起：隐藏全部杆实体化");
			btn->setCheckable(true);
			QObject::connect(btn, &QToolButton::toggled, btn, [fn](bool checked) {
				fn->slot_setAllRodVolume(checked);
				});
		}

		if (modelGW)
		{
			auto* btn = modelGW->addToolBtn("隐藏当前", ":/mMainWidgets/Icon/display/yincangdangqian.png", 0, 1, 1, 1, QSize(20, 20));
			host->setQToolButton(btn, "隐藏当前", ":/mMainWidgets/Icon/display/yincangdangqian.png", "隐藏当前拾取的单元", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn]() {
				fn->slot_maskModel();
				});

			btn = modelGW->addToolBtn("隐藏其他", ":/mMainWidgets/Icon/display/yincangqita.png", 1, 1, 1, 1, QSize(20, 20));
			host->setQToolButton(btn, "隐藏其他", ":/mMainWidgets/Icon/display/yincangqita.png", "隐藏当前拾取以外的全部单元", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn]() {
				fn->slot_maskOtherModel();
				});

			btn = modelGW->addToolBtn("反转显示", ":/mMainWidgets/Icon/display/fanzhuanxianshi.png", 0, 2, 1, 1, QSize(20, 20));
			host->setQToolButton(btn, "反转显示", ":/mMainWidgets/Icon/display/fanzhuanxianshi.png", "已显示/已隐藏的单元互换", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn]() {
				fn->slot_reverseMaskModel();
				});

			btn = modelGW->addToolBtn("显示全部", ":/mMainWidgets/Icon/display/xianshiquanbu.png", 1, 2, 1, 1, QSize(20, 20));
			host->setQToolButton(btn, "显示全部", ":/mMainWidgets/Icon/display/xianshiquanbu.png", "显示全部单元", Qt::ToolButtonTextBesideIcon);
			QObject::connect(btn, &QToolButton::pressed, btn, [fn]() {
				fn->slot_showAllModel();
				});
		}
	}

	/*
	* "选择"页（后处理独立页）：布局自前处理 mMenuWidget::initSelectMenu 的
	* 网格/批量操作两组移植，功能槽在 mPostSelectMenuFunction
	*/
	void mPostMenuContributor::buildSelectPage(IRibbonHost* host)
	{
		auto* fn = mPostSelectMenuFunction::getInstance();

		//页键带#Post后缀与前处理同名页分离，页签文本仍为"选择"（见mMenuWidget::pageForTabText）
		host->createRibbonPage(MechMainWindow::kPostSelectPageKey);

		mGroupWidget* griddingGW = host->addGroup(MechMainWindow::kPostSelectPageKey, "网格");
		mGroupWidget* batchGW = host->addGroup(MechMainWindow::kPostSelectPageKey, "批量操作");

		if (griddingGW)
		{
			auto* nodeIdBtn = griddingGW->addToolBtn("按照Id选择节点", ":/mMainWidgets/Icon/choice/IdChoicePoint3x.png", 0, 0, 1, 1, QSize(20, 20));
			host->setQToolButton(nodeIdBtn, "按照Id选择节点", ":/mMainWidgets/Icon/choice/IdChoicePoint3x.png", "通过节点ID选中节点。", Qt::ToolButtonTextBesideIcon);
			QObject::connect(nodeIdBtn, &QToolButton::pressed, nodeIdBtn, [fn]() {
				fn->slot_nodeIDSelect();
				});

			fn->getNodeInputLineEdit()->setParent(griddingGW);
			griddingGW->addLineEdit(fn->getNodeInputLineEdit(), 0, 1);
			fn->getNodeInputLineEdit()->setMinimumWidth(110);
			fn->getNodeInputLineEdit()->setFixedHeight(UiMetrics::menuItemHeight());

			auto* eleIdBtn = griddingGW->addToolBtn("按照Id选择单元", ":/mMainWidgets/Icon/choice/IdChoiceUnit3x.png", 1, 0, 1, 1, QSize(20, 20));
			host->setQToolButton(eleIdBtn, "按照Id选择单元", ":/mMainWidgets/Icon/choice/IdChoiceUnit3x.png", "通过单元ID选中单元。", Qt::ToolButtonTextBesideIcon);
			QObject::connect(eleIdBtn, &QToolButton::pressed, eleIdBtn, [fn]() {
				fn->slot_eleIDSelect();
				});

			fn->getEleInputLineEdit()->setParent(griddingGW);
			griddingGW->addLineEdit(fn->getEleInputLineEdit(), 1, 1);
			fn->getEleInputLineEdit()->setMinimumWidth(110);
			fn->getEleInputLineEdit()->setFixedHeight(UiMetrics::menuItemHeight());
		}

		if (batchGW)
		{
			auto* invertBtn = batchGW->addToolBtn("反向", ":/mMainWidgets/Icon/choice/fanxiang3x.png", 0, 0, 2, 1, QSize(30, 30));
			host->setQToolButton(invertBtn, "反向", ":/mMainWidgets/Icon/choice/fanxiang3x.png", "选中当前未选定的相同类\n型的实体(单元或节点)。");
			QObject::connect(invertBtn, &QToolButton::pressed, invertBtn, [fn]() {
				fn->slot_reverseSelect();
				});

			auto* clearBtn = batchGW->addToolBtn("清空", ":/mMainWidgets/Icon/choice/clear.png", 0, 1, 2, 1, QSize(30, 30));
			host->setQToolButton(clearBtn, "清空", ":/mMainWidgets/Icon/choice/clear.png", "清空所有拾取对象。");
			QObject::connect(clearBtn, &QToolButton::pressed, clearBtn, [fn]() {
				fn->slot_clearPick();
				});
		}
	}

	void mPostMenuContributor::buildModelPage(IRibbonHost* host)
	{
		//页键带#Post后缀与前处理同名页分离，页签文本仍为"模型"（见mMenuWidget::pageForTabText）；
		//V1.2原型：集合（部件集/节点集/单元边集/单元面集/单元集）+坐标系（直角/柱/球）两组创建钮，
		//按钮只发全局信号，功能在树侧槽实现（mPostTreeView::slot_createPostSetTyped/slot_createPostCoordinate）
		host->createRibbonPage(MechMainWindow::kPostModelPageKey);

		mGroupWidget* setGW = host->addGroup(MechMainWindow::kPostModelPageKey, "集合");
		mGroupWidget* coordGW = host->addGroup(MechMainWindow::kPostModelPageKey, "坐标系");

		auto* bus = MechCommon::mMechGlobalSignals::getInstance();

		if (setGW)
		{
			//按钮序即clickPostSetCreateSig的setType口径：0部件集/1节点集/2单元边集/3单元面集/4单元集
			const QString setNames[5] = { "部件集", "节点集", "单元边集", "单元面集", "单元集" };
			const QString setIcons[5] = {
				":/mMainWidgets/Icon/post/svg/bujianji.svg",
				":/mMainWidgets/Icon/post/svg/jiedianji.svg",
				":/mMainWidgets/Icon/post/svg/danbianji.svg",
				":/mMainWidgets/Icon/post/svg/danmianji.svg",
				":/mMainWidgets/Icon/post/svg/danyuanji.svg"
			};
			for (int i = 0; i < 5; ++i)
			{
				QToolButton* btn = setGW->addToolBtn(setNames[i], setIcons[i], 0, i, 2, 1, QSize(30, 30));
				host->setQToolButton(btn, setNames[i], setIcons[i], "创建" + setNames[i] + "。");
				QObject::connect(btn, &QToolButton::clicked, btn, [bus, i]() {
					emit bus->clickPostSetCreateSig(i);
					});
			}
		}

		if (coordGW)
		{
			//coordType：0直角/1柱/2球，对应MxFunctions::CoordinateType，供后续接线
			const QString coordNames[3] = { "直角坐标系", "柱坐标系", "球坐标系" };
			const QString coordIcons[3] = {
				":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg",
				":/mMainWidgets/Icon/post/svg/zuobiaoxi_zhu.svg",
				":/mMainWidgets/Icon/post/svg/zuobiaoxi_qiu.svg"
			};
			for (int i = 0; i < 3; ++i)
			{
				QToolButton* btn = coordGW->addToolBtn(coordNames[i], coordIcons[i], 0, i, 2, 1, QSize(30, 30));
				host->setQToolButton(btn, coordNames[i], coordIcons[i], "创建" + coordNames[i] + "。");
				QObject::connect(btn, &QToolButton::clicked, btn, [bus, i]() {
					emit bus->clickPostCoordinateCreateSig(i);
					});
			}
		}
	}
}
