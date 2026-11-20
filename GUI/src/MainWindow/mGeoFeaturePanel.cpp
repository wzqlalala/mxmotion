#include "mGeoFeaturePanel.h"
#include "UiMetrics.h"
#include "ViewportContainer.h"
#include <cmath> //【OCC摘除·S10/P0-1】M_PI 显式自含（原经退役头 OCC 链传递）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "mGeoDevPreviewGate.h" //【GUI 门控】孔特征依赖未推送 API
#include "mMechGlobalSignals.h"
#include "mUiMenu.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
//#include "mGeoCommandSolidOfDragVecOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandSolidOfSpinOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandSolidOfDragLineOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandSolidOfLoftOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandSolidOfDraftOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandSolidOfHollowingOCC.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
#include "mGeoManager.h"
//#include "mGeoCommandList.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头（EditShape 管线为死代码）
#include "PickCommonFunction.h"

#include "mGeoFeatureTypes.h"
#include "mGeoPreviewTypes.h"
#include "mGeoServer.h"

#include <mMeshData.h>

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/DirectGeoModelingTool.h"
#include "PreViewportWidget.h"
 

#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include <QMenu>
#include <memory>

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1】随退役头下线
using namespace mxr;

namespace MechMainWindow
{
	mGeoFeaturePanel::mGeoFeaturePanel(QWidget* parent)
		: mGeoPanelBase(parent)
	{
		stretchDirId = -1;
		rotationAxisId = -1;
		rotationPntId = -1;
		wdpFaceId = -1;

		connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
			clearTableItems(pickObjTable);
			//孔模式保持点选面状态，避免退化为对象拾取
			geoPickWidget->_featPick = (GeoHole == currentFeature) ? HoleFace : FeatureObj;

			//拾取类型切换（如几何面→几何线）后刷新当前特征预览
			if (GeoStretch == currentFeature)            slot_stretchPreview();
			else if (GeoRotation == currentFeature)      slot_rotationPreview();
			else if (GeoSweep == currentFeature)         slot_sweepPreview();
			else if (GeoLoft == currentFeature)          slot_loftPreview();
			else if (GeoShellRemoval == currentFeature)  slot_hollowingPreview();
			else if (GeoWithdrawingPattern == currentFeature) slot_draftPreview();
			else if (GeoChamfering == currentFeature)    slot_chamferPreview();
			});

		connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
			//孔模式保持点选面状态，避免退化为对象拾取
			geoPickWidget->_featPick = (GeoHole == currentFeature) ? HoleFace : FeatureObj;
			});

		geoPickWidget_2 = new GeoPickWidget(this);
		geoPickWidget_2->_featPick = None;
		geoPickWidget_2->pickLabel->setText("扫掠路径");
		geoPickWidget_2->pickObjTypeCombo->clear();
		geoPickWidget_2->pickObjTypeCombo->addItems({"几何线" });
		connect(geoPickWidget_2->pickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget_2->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
			geoPickWidget->strPickType = geoPickWidget_2->strPickType;
			setGeoHighlight(sweepPickLineTable, geoPickWidget_2->pickObjTypeCombo->currentText());
			geoPickWidget->_featPick = SweepLine;
			});

		//按顺序初始化
		initStretchFold();
		initRotationFold();
		initSweepFold();
		initLoftFold();
		initShellRmFold();
		initWDPRmFold();
		initChamferFold();
		initHoleFold();

		_mainLayout->addStretch();

        //全局信号（拾取/单位制/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接

        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(stretchUnitsLabel, stretchHeightEdit));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(chamferUnitsLabel, chamferDimEdit));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(shellRmUnitsLabel, shellRmThicknessEdit));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(holeRadiusUnitsLabel, holeRadiusEdit));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(holeDepthUnitsLabel, holeDepthEdit));

        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(chamferAngleUnit, chamferAngEdit));
        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(wdpUnitsLabel, wdpAngleEdit));
        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(rotationUnitsLabel, rotationAngleEdit));
    }

	void mGeoFeaturePanel::connectGeoSignals()
	{
		//拾取信号
		slot_subWindowActivated();

		// 新架构：直接挂容器激活信号（带 prevID，可区分真正的视口切换）
		if (auto* container = MechPreData::mViewportModelManager::getInstance()->getViewportContainer()) {
			conn_viewportChanged = connect(container, &MechMainWindow::ViewportContainer::sig_activeViewportChanged,
				this, [this](int viewportID, int prevID) {
					Q_UNUSED(viewportID); Q_UNUSED(prevID);
					slot_subWindowActivated();
				});
		} else {
			conn_viewportChanged = connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, [this](int) { slot_subWindowActivated(); });
		}

		//单位制信号
		connectUnitSystemResponse();
	}

	void mGeoFeaturePanel::disconnectGeoSignals()
	{
		if (conn_geo)
		{
			disconnect(conn_geo);
			conn_geo = QMetaObject::Connection();
		}
		if (conn_viewportChanged)
		{
			disconnect(conn_viewportChanged);
			conn_viewportChanged = QMetaObject::Connection();
		}
		disconnectUnitSystemResponse();
	}

	mGeoFeaturePanel::~mGeoFeaturePanel()
	{

	}

	//初始化拉伸操作界面
	void mGeoFeaturePanel::initStretchFold()
	{
		//初始化拉伸所有控件
		QToolButton* stretchReverseBtn = new QToolButton(this);
		stretchReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		stretchReverseBtn->setToolTip("反向");
		connect(stretchReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(stretchVx, stretchVy, stretchVz);
			});

		QToolButton* stretchMousePickBtn = new QToolButton( this);
		stretchMousePickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		stretchMousePickBtn->setToolTip("选择拉伸方向");
		connect(stretchMousePickBtn, &QToolButton::clicked, this, [this, stretchMousePickBtn]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(stretchDirId, "几何线");
			geoPickWidget->_featPick = StretchDirection;
			});

		stretchHeightEdit = new MxFunctions::mCalcLineEdit(this); stretchHeightEdit->setExpression("10");
		stretchUnitsLabel = new QLabel("mm");

		stretchPreviewChkBox = new QCheckBox("预览", this);
		stretchPreviewChkBox->setChecked(false);
		connect(stretchPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoStretch);
			if (checked)
			{
				slot_stretchPreview();
			}
			});

		stretchMergeChkBox = new QCheckBox("是否合并", this);
		stretchMergeChkBox->setChecked(false);

		stretchCancelBtn = new QPushButton("取消", this);
		stretchOkBtn = new QPushButton("确认", this);

		stretchVx = new MxFunctions::mCalcLineEdit(this); stretchVx->setExpression("1"); //stretchVx->setPlaceholderText("0");
		stretchVy = new MxFunctions::mCalcLineEdit(this); stretchVy->setExpression(""); stretchVy->setPlaceholderText("0");
		stretchVz = new MxFunctions::mCalcLineEdit(this); stretchVz->setExpression(""); stretchVz->setPlaceholderText("0");

		connect(stretchCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(stretchOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createStretchBtn);
		
		// ========== 定义拉伸折叠块 ==========
		FoldSection* stretchFold_1 = new FoldSection("拉伸定义", this);
		m_FoldSections.append(stretchFold_1);
		m_foldWidgets.append(stretchFold_1);

		// ========== 拉伸方向折叠块 ==========
		FoldSection* stretchFold_2 = new FoldSection("拉伸方向和高度", this, QColor(0, 0, 0));
		//多行布局
		stretchFold_2->addRow(new QLabel("Vx"), stretchVx, stretchMousePickBtn);
		stretchFold_2->addRow(new QLabel("Vy"), stretchVy, stretchReverseBtn);
		stretchFold_2->addRow(new QLabel("Vz"), stretchVz, 28);
		stretchFold_2->addRow(new QLabel("高度"), stretchHeightEdit, stretchUnitsLabel);

		QHBoxLayout* checkBoxLayout = new QHBoxLayout(stretchFold_2);
		checkBoxLayout->addWidget(stretchPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		checkBoxLayout->addStretch();
		checkBoxLayout->addWidget(stretchMergeChkBox);
		stretchFold_2->addRow(checkBoxLayout);

		QFrame* horizontalLine = createHorizontalLine(stretchFold_2);
		stretchFold_2->addRow(horizontalLine);
		stretchFold_2->addRow(stretchCancelBtn, stretchOkBtn,"Left");
		m_foldWidgets.append(stretchFold_2);
	}

	//初始化旋转操作界面
	void mGeoFeaturePanel::initRotationFold()
	{
		//初始化旋转所有控件
		QToolButton* rotationReverseBtn = new QToolButton(this);
		rotationReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		rotationReverseBtn->setToolTip("反向");
		connect(rotationReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(rotationVx, rotationVy, rotationVz);
			});

		QToolButton* rotationAxisPickBtn = new QToolButton(this);
		rotationAxisPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		rotationAxisPickBtn->setToolTip("选择旋转轴");
		connect(rotationAxisPickBtn, &QToolButton::clicked, this, [this, rotationAxisPickBtn]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(rotationAxisId, "几何线");
			geoPickWidget->_featPick = RotationAxis;
			});

		QToolButton* rotationPointPickBtn = new QToolButton(this);
		rotationPointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		rotationPointPickBtn->setToolTip("选择轴上一点");
		connect(rotationPointPickBtn, &QToolButton::clicked, this, [this, rotationPointPickBtn]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
			Highlight(rotationPntId, "几何点");
			geoPickWidget->_featPick = RotationAxisPnt;
			});

		rotationAngleEdit = new MxFunctions::mCalcLineEdit(); rotationAngleEdit->setExpression("0");
		rotationUnitsLabel = new QLabel("°");

		rotationPreviewChkBox = new QCheckBox("预览", this);
		rotationPreviewChkBox->setChecked(false);
		connect(rotationPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoRotation);
			if (checked)
			{
				slot_rotationPreview();
			}
			});

		rotationMergeChkBox = new QCheckBox("是否合并", this);
		rotationMergeChkBox->setChecked(false);

		rotationCancelBtn = new QPushButton("取消", this);
		rotationOkBtn = new QPushButton("确认", this);

		rotationVx = new MxFunctions::mCalcLineEdit(this); rotationVx->setExpression("1"); 
		rotationVy = new MxFunctions::mCalcLineEdit(this); rotationVy->setExpression(""); rotationVy->setPlaceholderText("0");
		rotationVz = new MxFunctions::mCalcLineEdit(this); rotationVz->setExpression(""); rotationVz->setPlaceholderText("0");

		rotationPntX = new MxFunctions::mCalcLineEdit(this); rotationPntX->setExpression(""); rotationPntX->setPlaceholderText("0");
		rotationPntY = new MxFunctions::mCalcLineEdit(this); rotationPntY->setExpression(""); rotationPntY->setPlaceholderText("0");
		rotationPntZ = new MxFunctions::mCalcLineEdit(this); rotationPntZ->setExpression(""); rotationPntZ->setPlaceholderText("0");

		connect(rotationCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(rotationOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createRotationBtn);

		// ========== 定义旋转折叠块 ==========
		FoldSection* rotationFold_1 = new FoldSection("旋转定义", this);
		m_FoldSections.append(rotationFold_1);
		m_foldWidgets.append(rotationFold_1);

		//// ========== 旋转轴折叠块 ==========
		FoldSection* rotationFold_2 = new FoldSection("旋转轴", this);

		//多行布局
		rotationFold_2->addRow(new QLabel("Vx"), rotationVx, rotationAxisPickBtn);
		rotationFold_2->addRow(new QLabel("Vy"), rotationVy, rotationReverseBtn);
		rotationFold_2->addRow(new QLabel("Vz"), rotationVz,28);
		m_foldWidgets.append(rotationFold_2);
	
		// ========== 轴上一点折叠块 ==========
		FoldSection* rotationFold_3 = new FoldSection("轴上一点", this);
		//多行布局
		rotationFold_3->addRow(new QLabel("X"), rotationPntX, rotationPointPickBtn);
		rotationFold_3->addRow(new QLabel("Y"), rotationPntY, 28);
		rotationFold_3->addRow(new QLabel("Z"), rotationPntZ, 28);

		rotationFold_3->addRow(new QLabel("角度"), rotationAngleEdit, rotationUnitsLabel);
		QHBoxLayout* checkBoxLayout = new QHBoxLayout(rotationFold_3);
		checkBoxLayout->addWidget(rotationPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		checkBoxLayout->addStretch();
		checkBoxLayout->addWidget(rotationMergeChkBox);
		rotationFold_3->addRow(checkBoxLayout);

		QFrame* horizontalLine = createHorizontalLine(rotationFold_3);
		rotationFold_3->addRow(horizontalLine);
		rotationFold_3->addRow(rotationCancelBtn, rotationOkBtn,"Left");
		m_foldWidgets.append(rotationFold_3);
	}

	//初始化扫掠操作界面
	void mGeoFeaturePanel::initSweepFold()
	{
		//初始化扫掠所有控件
		sweepPickLineTable = new QTableWidget(this);
		sweepPickLineTable->setMaximumHeight(100);
		sweepPickLineTable->setMinimumHeight(80);
		sweepPickLineTable->verticalHeader()->setHidden(true);
		sweepPickLineTable->setColumnCount(1);
		QStringList horizontalHeader = { "几何ID" };
		sweepPickLineTable->setHorizontalHeaderLabels(horizontalHeader);
		sweepPickLineTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		sweepPickLineTable->horizontalHeader()->setHighlightSections(false);
		sweepPickLineTable->setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 10));
		sweepPickLineTable->verticalHeader()->setDefaultSectionSize(20);
		sweepPickLineTable->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
		sweepDelAction = new QAction("删除");
		sweepPickLineTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

		sweepPreviewChkBox = new QCheckBox("预览", this);
		sweepPreviewChkBox->setChecked(false);
		connect(sweepPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoSweep);
			if (checked)
			{
				slot_sweepPreview();
			}
			});

		sweepMergeChkBox = new QCheckBox("是否合并", this);
		sweepMergeChkBox->setChecked(false);

		sweepCancelBtn = new QPushButton("取消", this);
		sweepOkBtn = new QPushButton("确认", this);

		connect(sweepPickLineTable, &QTableWidget::customContextMenuRequested, this, &mGeoFeaturePanel::slot_contextMenuTableData);
		connect(sweepCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(sweepOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createSweepBtn);
		// ========== 定义扫掠折叠块 ==========
		FoldSection* sweepFold_1 = new FoldSection("扫掠定义", this);
		m_FoldSections.append(sweepFold_1);
		sweepFold_1->addRow(sweepPickLineTable);
		QHBoxLayout* checkBoxLayout = new QHBoxLayout(sweepFold_1);
		checkBoxLayout->addWidget(sweepPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		checkBoxLayout->addStretch();
		checkBoxLayout->addWidget(sweepMergeChkBox);
		sweepFold_1->addRow(checkBoxLayout);

		QFrame* horizontalLine = createHorizontalLine(sweepFold_1);
		sweepFold_1->addRow(horizontalLine);
		sweepFold_1->addRow(sweepCancelBtn, sweepOkBtn, "Left");
		m_foldWidgets.append(sweepFold_1);
	}

	//初始化放样操作界面
	void mGeoFeaturePanel::initLoftFold()
	{
		//初始化放样所有控件
		loftPreviewChkBox = new QCheckBox("预览", this);
		loftPreviewChkBox->setChecked(false);
		connect(loftPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoLoft);
			if (checked)
			{
				slot_loftPreview();
			}
			});

		loftTypeChkBox = new QCheckBox("是否生成实体", this);
		loftTypeChkBox->setChecked(false);

		loftCancelBtn = new QPushButton("取消", this);
		loftOkBtn = new QPushButton("确认", this);

		connect(loftCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(loftOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createLoftBtn);

		// ========== 定义放样折叠块 ==========
		FoldSection* loftFold_1 = new FoldSection("放样定义", this);
	
		QHBoxLayout* checkBoxLayout = new QHBoxLayout(loftFold_1);
		checkBoxLayout->addWidget(loftPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		checkBoxLayout->addStretch();
		checkBoxLayout->addWidget(loftTypeChkBox);
		loftFold_1->addRow(checkBoxLayout);

		QFrame* horizontalLine = createHorizontalLine(loftFold_1);
		loftFold_1->addRow(horizontalLine);
		loftFold_1->addRow(loftCancelBtn, loftOkBtn , "Left");
		m_FoldSections.append(loftFold_1);
		m_foldWidgets.append(loftFold_1);
	}

	//初始化抽壳操作界面
	void mGeoFeaturePanel::initShellRmFold()
	{
		//初始化抽壳所有控件
		shellRmPreviewChkBox = new QCheckBox("预览", this);
		shellRmPreviewChkBox->setChecked(false);
		connect(shellRmPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoShellRemoval);
			if (checked)
			{
				slot_hollowingPreview();
			}
			});

		shellRmCancelBtn = new QPushButton("取消", this);
		shellRmOkBtn = new QPushButton("确认", this);

		shellRmThicknessEdit = new MxFunctions::mCalcLineEdit(this); shellRmThicknessEdit->setExpression("2");
		shellRmUnitsLabel = new QLabel("mm");

		connect(shellRmCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(shellRmOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createHollowingBtn);

		// ========== 定义抽壳折叠块 ==========
		FoldSection* shellRmFold_1 = new FoldSection("抽壳定义", this);
		shellRmFold_1->addRow(new QLabel("厚度"), shellRmThicknessEdit, shellRmUnitsLabel);
		shellRmFold_1->addRow(shellRmPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		QFrame* horizontalLine = createHorizontalLine(shellRmFold_1);
		shellRmFold_1->addRow(horizontalLine);

		shellRmFold_1->addRow(shellRmCancelBtn, shellRmOkBtn,"Left");
		m_FoldSections.append(shellRmFold_1);
		m_foldWidgets.append(shellRmFold_1);
	}

	//初始化拔模操作界面
	void mGeoFeaturePanel::initWDPRmFold()
	{
		//初始化拔模所有控件
		wdpPickMidEdit = new QLineEdit( this);
		wdpPickMidEdit->setPlaceholderText("请选择中性面");
		wdpPickMidEdit->setAlignment(Qt::AlignCenter);
		wdpPickMidEdit->setReadOnly(true);

		wdpPickMidBtn = new QToolButton(this);
		wdpPickMidBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		wdpPickMidBtn->setToolTip("选择中性面");
		connect(wdpPickMidBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
			Highlight(wdpFaceId, "几何面");
			geoPickWidget->_featPick = DraftNeutralFace;
			});

		wdpPickMidReverseBtn = new QToolButton(this);
		wdpPickMidReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		wdpPickMidReverseBtn->setToolTip("反向");
		connect(wdpPickMidReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(wdpPickMidEdit);
			});
	
		wdpPreviewChkBox = new QCheckBox("预览", this);
		wdpPreviewChkBox->setChecked(false);
		connect(wdpPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoWithdrawingPattern);
			if (checked)
			{
				slot_draftPreview();
			}
			});

		wdpCancelBtn = new QPushButton("取消", this);
		wdpOkBtn = new QPushButton("确认", this);

		wdpAngleEdit = new MxFunctions::mCalcLineEdit(); wdpAngleEdit->setExpression("30");
		wdpUnitsLabel = new QLabel("°");

		connect(wdpCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(wdpOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createDraftBtn);


		// ========== 定义拔模折叠块 ==========
		FoldSection* wdpFold_1 = new FoldSection("拔模定义", this);
		wdpFold_1->addRow(new QLabel("中性面"), wdpPickMidEdit, wdpPickMidReverseBtn, wdpPickMidBtn);
		wdpFold_1->addRow(new QLabel("角度"), wdpAngleEdit, wdpUnitsLabel);
		wdpFold_1->addRow(wdpPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		QFrame* horizontalLine = createHorizontalLine(wdpFold_1);
		wdpFold_1->addRow(horizontalLine);

		wdpFold_1->addRow(wdpCancelBtn, wdpOkBtn, "Left");
		m_FoldSections.append(wdpFold_1);
		m_foldWidgets.append(wdpFold_1);
	}

	//初始化倒角操作界面
	void mGeoFeaturePanel::initChamferFold()
	{
		//初始化倒角所有控件
		chamferComboBox = new QComboBox(this);
		chamferComboBox->addItems({"倒圆角","倒斜角"});

		chamferPreviewChkBox = new QCheckBox("预览", this);
		chamferPreviewChkBox->setChecked(false);
		connect(chamferPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoChamfering);
			if (checked)
			{
				slot_chamferPreview();
			}
			});

		chamferCancelBtn = new QPushButton("取消", this);
		chamferOkBtn = new QPushButton("确认", this);

		chamferDimEdit = new MxFunctions::mCalcLineEdit(this); chamferDimEdit->setExpression("2");
		chamferAngEdit = new MxFunctions::mCalcLineEdit(this); chamferAngEdit->setExpression("45");
		QLabel* chamferDimTypeLabel = new QLabel("半径");
		chamferUnitsLabel = new QLabel("mm");

		QLabel* chamferAngleLabel = new QLabel("角度",this);
		chamferAngleUnit = new QLabel("°",this);

		connect(chamferOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createChamferBtn);
		connect(chamferCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		// ========== 定义倒角折叠块 ==========
		FoldSection* chamferFold_1 = new FoldSection("倒角定义", this);
		chamferFold_1->addRow(chamferComboBox);
		chamferFold_1->addRow(chamferDimTypeLabel, chamferDimEdit, chamferUnitsLabel);
		chamferFold_1->addRow(chamferAngleLabel, chamferAngEdit, chamferAngleUnit);
		chamferFold_1->addRow(chamferPreviewChkBox);  //【预览修复·20261019】合并裁决误注释，恢复源分支挂接
		QFrame* horizontalLine = createHorizontalLine(chamferFold_1);
		chamferFold_1->addRow(horizontalLine);

		chamferFold_1->addRow(chamferCancelBtn, chamferOkBtn, "Left");
		m_FoldSections.append(chamferFold_1);
		m_foldWidgets.append(chamferFold_1);

		connect(chamferComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
			[this, chamferDimTypeLabel, chamferAngleLabel, chamferFold_1](int i) {
				chamferAngleLabel->setVisible(false);
				chamferAngEdit->setVisible(false);
				chamferAngleUnit->setVisible(false);
				if (i)
				{
					chamferDimTypeLabel->setText("长度");
					chamferAngleLabel->setVisible(true);
					chamferAngEdit->setVisible(true);
					chamferAngleUnit->setVisible(true);
				}

				else
				{
					chamferDimTypeLabel->setText("半径");
				}
				chamferFold_1->refreshLayout();
			});


		chamferAngleLabel->setVisible(false);
		chamferAngEdit->setVisible(false);
		chamferAngleUnit->setVisible(false);
	}

	//初始化孔操作界面
	void mGeoFeaturePanel::initHoleFold()
	{
		//初始化孔所有控件
		holeTypeComboBox = new QComboBox(this);
		holeTypeComboBox->addItems({ "圆孔" });//目前只有圆孔，预留其它孔类型扩展

		holeCancelBtn = new QPushButton("取消", this);
		holeOkBtn = new QPushButton("确认", this);

		holeRadiusEdit = new MxFunctions::mCalcLineEdit(this); holeRadiusEdit->setExpression("5");
		holeDepthEdit = new MxFunctions::mCalcLineEdit(this); holeDepthEdit->setExpression("10");
		QLabel* holeTypeLabel = new QLabel("孔类型");
		holeRadiusUnitsLabel = new QLabel("mm");

		QLabel* holeDepthLabel = new QLabel("深度", this);
		holeDepthUnitsLabel = new QLabel("mm", this);

		connect(holeCancelBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_cancelBtn);
		connect(holeOkBtn, &QPushButton::clicked, this, &mGeoFeaturePanel::slot_createHoleBtn);

		// ========== 定义孔折叠块 ==========
		FoldSection* holeFold_1 = new FoldSection("孔定义", this);
		holeFold_1->addRow(holeTypeLabel, holeTypeComboBox);
		holeFold_1->addRow(new QLabel("半径"), holeRadiusEdit, holeRadiusUnitsLabel);
		holeFold_1->addRow(holeDepthLabel, holeDepthEdit, holeDepthUnitsLabel);

		QFrame* horizontalLine = createHorizontalLine(holeFold_1);
		holeFold_1->addRow(horizontalLine);

		holeFold_1->addRow(holeCancelBtn, holeOkBtn, "Left");
		m_FoldSections.append(holeFold_1);
		m_foldWidgets.append(holeFold_1);
	}

	void mGeoFeaturePanel::showFeatureFold(GeoOperationType type)
	{
		// 进入编辑时连接单位制切换信号（退出编辑由mInfoWindow::uneditGeometry统一断开）
		connectUnitSystemResponse();

		hideAllFold();
		currentFeature = type;
		pickObjTable->setVisible(true);
		geoPickWidget->pickLabel->setText("拾取");

		geoPickWidget->pickObjTypeCombo->clear();
		geoPickWidget->pickObjTypeCombo->addItems({ "几何面","几何线" });

		geoPickWidget_2->setVisible(false);

		for (int i = sweepPickLineTable->rowCount() - 1; i >= 0; i--) {
			sweepPickLineTable->removeRow(i);
		}

		geoPickWidget->_featPick = FeatureObj;
		
		if (GeoStretch == type)
		{
			m_FoldSections[0]->insertRow(0, pickObjTable);
			m_FoldSections[0]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 0,1 });

			//进入几何面板编辑时激活直接建模渲染工具，并按勾选框状态挂接预览
			activateDirectGeoModelingTool(true);
			connectPreView(stretchPreviewChkBox->isChecked());
		}
		if (GeoRotation == type)
		{
			m_FoldSections[1]->insertRow(0, pickObjTable);
			m_FoldSections[1]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 2,3,4 });

			activateDirectGeoModelingTool(true);
			connectPreView(rotationPreviewChkBox->isChecked(), GeoRotation);
		}
		if (GeoSweep == type)
		{
			m_FoldSections[2]->insertRow(0, geoPickWidget_2);
			m_FoldSections[2]->insertRow(0, pickObjTable);
			m_FoldSections[2]->insertRow(0, geoPickWidget);
			geoPickWidget_2->setVisible(true);

			displayAfterInsertFolds({ 5 });

			activateDirectGeoModelingTool(true);
			connectPreView(sweepPreviewChkBox->isChecked(), GeoSweep);
		}
		if (GeoLoft == type)
		{
			geoPickWidget->pickLabel->setText("放样对象");
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({"几何线" });

			m_FoldSections[3]->insertRow(0, pickObjTable);
			m_FoldSections[3]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 6 });

			activateDirectGeoModelingTool(true);
			connectPreView(loftPreviewChkBox->isChecked(), GeoLoft);
		}
		if (GeoShellRemoval == type)
		{
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({ "几何面" });
			m_FoldSections[4]->insertRow(0, pickObjTable);
			m_FoldSections[4]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 7 });

			activateDirectGeoModelingTool(true);
			connectPreView(shellRmPreviewChkBox->isChecked(), GeoShellRemoval);
		}
		if (GeoWithdrawingPattern == type)
		{
			geoPickWidget->pickLabel->setText("拔模曲面");
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({ "几何面" });

			m_FoldSections[5]->insertRow(0, pickObjTable);
			m_FoldSections[5]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 8 });

			activateDirectGeoModelingTool(true);
			connectPreView(wdpPreviewChkBox->isChecked(), GeoWithdrawingPattern);
		}
		if (GeoChamfering == type)
		{
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({ "几何线" });
			geoPickWidget->pickLabel->setText("倒角对象");
			m_FoldSections[6]->insertRow(0, pickObjTable);
			m_FoldSections[6]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 9 });

			activateDirectGeoModelingTool(true);
			connectPreView(chamferPreviewChkBox->isChecked(), GeoChamfering);
		}
		if (GeoHole == type)
		{
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({ "几何面" });
			geoPickWidget->pickLabel->setText("孔放置面");
			//同硬点添加：不显示拾取表格，点选位置用点标记反馈
			pickObjTable->setVisible(false);
			m_FoldSections[7]->insertRow(0, pickObjTable);
			m_FoldSections[7]->insertRow(0, geoPickWidget);
			//addItems触发下拉信号会置为FeatureObj，需在其后覆盖
			geoPickWidget->_featPick = HoleFace;

			displayAfterInsertFolds({ 10 });
		}
		if (GeoHole == type)
		{
			geoPickWidget->pickObjTypeCombo->clear();
			geoPickWidget->pickObjTypeCombo->addItems({ "几何面" });
			geoPickWidget->pickLabel->setText("孔放置面");
			//同硬点添加：不显示拾取表格，点选位置用点标记反馈
			pickObjTable->setVisible(false);
			m_FoldSections[7]->insertRow(0, pickObjTable);
			m_FoldSections[7]->insertRow(0, geoPickWidget);
			//addItems触发下拉信号会置为FeatureObj，需在其后覆盖
			geoPickWidget->_featPick = HoleFace;

			displayAfterInsertFolds({ 10 });
		}

		geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
	}

	//取消
	void mGeoFeaturePanel::slot_cancelBtn()
	{
		//取消预览渲染并断开当前特征的预览信号
		clearStretchPreviewRender();
		connectPreView(false, currentFeature);
		//取消编辑几何面板，取消激活渲染工具
		activateDirectGeoModelingTool(false);
		hideAllFold();
	}
	void mGeoFeaturePanel::slot_createStretchBtn()
	{
		try
		{
			//控件值判空提示
			if (pickObjTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取拉伸对象！"));
				return;
			}
			if (stretchVx->text().isNull())
			{
				stretchVx->setText("0");
			}
			if (stretchVy->text().isNull())
			{
				stretchVy->setText("0");
			}
			if (stretchVz->text().isNull())
			{
				stretchVz->setText("0");
			}
			if (stretchHeightEdit->text().isNull())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请输入拉伸高度值！"));
				return;
			}

			std::vector<int> geoLineIds;
			std::vector<int> geoFaceIds;
			//QList<MGeoCommand::EditShape> mEditShapeList; //【OCC摘除·S10/P0-1】孤儿死声明（引擎路径不消费）
			for (int i = 0; i < pickObjTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				if (strId.indexOf("几何线") == 0)
				{
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
					geoLineIds.push_back(id);
				}
				else if (strId.indexOf("几何面") == 0)
				{
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
					geoFaceIds.push_back(id);
				}
				
			}

			if (geoLineIds.empty()&& geoFaceIds.empty())
			{
				return;
			}

			double dis = getDefaultValue(Unit::Length, stretchHeightEdit->text().toDouble());
			double mdir1[3] = { stretchVx->text().toDouble(),stretchVy->text().toDouble(),stretchVz->text().toDouble() };

            EventDispatcher::mGeoFeatureStretchRequest req;
            if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
            {
                req.geoIds = geoLineIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
            }
            else
            {
                req.geoIds = geoFaceIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Face;
            }
            req.dis = dis;
            req.mdir1[0] = mdir1[0];
            req.mdir1[1] = mdir1[1];
            req.mdir1[2] = mdir1[2];
            req.isFuse = stretchMergeChkBox->isChecked();
            req.isReverse = false;
            req.name = "StretchFeature";

            EventDispatcher::mGeoOperationResult res = getGEOServer.features().stretch(req);
            if (res.succeeded())
            {
                if (!stretchMergeChkBox->isChecked() && res.generatedObjects.size() == 2)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                }
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	//创建旋转特征
	void mGeoFeaturePanel::slot_createRotationBtn()
	{
		try
		{
			//控件值判空提示
			if (pickObjTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取旋转对象！"));
				return;
			}
			if (rotationVx->text().isNull())
			{
				rotationVx->setText("0");
			}
			if (rotationVy->text().isNull())
			{
				rotationVy->setText("0");
			}
			if (rotationVz->text().isNull())
			{
				rotationVz->setText("0");
			}
			if (rotationPntX->text().isNull())
			{
				rotationPntX->setText("0");
			}
			if (rotationPntY->text().isNull())
			{
				rotationPntY->setText("0");
			}
			if (rotationPntZ->text().isNull())
			{
				rotationPntZ->setText("0");
			}
			if (rotationAngleEdit->text().isNull())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请输入旋转角度值！"));
				return;
			}

            std::vector<int> geoLineIds;
            std::vector<int> geoFaceIds;
            //QList<MGeoCommand::EditShape> mEditShapeList; //【OCC摘除·S10/P0-1】孤儿死声明（引擎路径不消费）
            for (int i = 0; i < pickObjTable->rowCount(); ++i)
            {
                QTableWidgetItem* item = pickObjTable->item(i, 0);
                if (!item)
                {
                	continue;
                }
                QString strId = item->text();
                if (strId.indexOf("几何线") == 0)
                {
                    strId = strId.mid(strId.lastIndexOf(":") + 1);
                    int id = strId.toInt();
                    geoLineIds.push_back(id);
                }
                else if (strId.indexOf("几何面") == 0)
                {
                    strId = strId.mid(strId.lastIndexOf(":") + 1);
                    int id = strId.toInt();
                    geoFaceIds.push_back(id);
                }

            }

            if (geoLineIds.empty() && geoFaceIds.empty())
            {
                return;
            }

			double angle = getDefaultValue(Unit::Angle, rotationAngleEdit->text().toDouble()) * M_PI / 180.0;
			double mAxis1[3] = { rotationVx->text().toDouble(),rotationVy->text().toDouble(),rotationVz->text().toDouble() };
			double mPnt[3] = { rotationPntX->text().toDouble(),rotationPntY->text().toDouble(),rotationPntZ->text().toDouble() };

            EventDispatcher::mGeoFeatureRotateRequest req;
            if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
            {
                req.geoIds = geoLineIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
            }
            else
            {
                req.geoIds = geoFaceIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Face;
            }
            req.angle = angle;
            req.mAxis1[0] = mAxis1[0];
            req.mAxis1[1] = mAxis1[1];
            req.mAxis1[2] = mAxis1[2];
            req.mPnt[0] = mPnt[0];
            req.mPnt[1] = mPnt[1];
            req.mPnt[2] = mPnt[2];
            req.isFuse = rotationMergeChkBox->isChecked();
            req.isReverse = false;
            req.name = "RotateFeature";

            EventDispatcher::mGeoOperationResult res = getGEOServer.features().rotate(req);
            if (res.succeeded())
            {
                if (!rotationMergeChkBox->isChecked() && res.generatedObjects.size() == 2)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                }
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	//创建扫掠特征
	void mGeoFeaturePanel::slot_createSweepBtn()
	{
		try
		{
			//控件值判空提示
			if (pickObjTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取扫掠对象！"));
				return;
			}
			//控件值判空提示
			if (sweepPickLineTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取扫掠路径！"));
				return;
			}

            std::vector<int> geoLineIds;
            std::vector<int> geoFaceIds;
			for (int i = 0; i < pickObjTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				if (strId.indexOf("几何线") == 0)
				{
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
                    geoLineIds.push_back(id);
				}
				else if (strId.indexOf("几何面") == 0)
				{
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
                    geoFaceIds.push_back(id);
				}

			}

            std::vector<int> m_path;
			for (int i = 0; i < sweepPickLineTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = sweepPickLineTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				m_path.push_back(id);
			}

            if ((geoLineIds.empty() && geoFaceIds.empty()) || m_path.empty())
			{
				return;
			}

            EventDispatcher::mGeoFeatureSweepRequest req;
            if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
            {
                req.geoIds = geoLineIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
            }
            else
            {
                req.geoIds = geoFaceIds;
                req.shapeType = GeoNeutralData::GN_ShapeType::Face;
            }

            req.pathIds = m_path;
            req.isFuse = sweepMergeChkBox->isChecked();
            req.name = "sweepFeature";

            EventDispatcher::mGeoOperationResult res = getGEOServer.features().sweep(req);
            if (res.succeeded())
            {
                if (!sweepMergeChkBox->isChecked() && res.generatedObjects.size() == 2)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                }
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	//创建放样特征
	void mGeoFeaturePanel::slot_createLoftBtn()
	{
		try
		{
			//控件值判空提示
			if (pickObjTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取放样对象！"));
				return;
			}

			std::vector<int> m_shape;
			for (int i = 0; i < pickObjTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				m_shape.push_back(id);
			}
			
			if (m_shape.empty())
			{
				return;
			}

            EventDispatcher::mGeoFeatureLoftRequest req;
            req.geoIds = m_shape;
            req.isSolid = loftTypeChkBox->isChecked();
            req.name = "SolidOfLoft";

            EventDispatcher::mGeoOperationResult res = getGEOServer.features().loft(req);
            if (res.succeeded() && !sweepMergeChkBox->isChecked() && res.generatedObjects.size() == 2)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	//创建抽壳特征
	void mGeoFeaturePanel::slot_createHollowingBtn()
	{
		try
		{
			//控件值判空提示
			if (pickObjTable->rowCount() == 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取抽壳对象！"));
				return;
			}
			//控件值判空提示
			if (shellRmThicknessEdit->text().isEmpty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请设置抽壳厚度！"));
				return;
			}


            std::vector<int> m_shape;
			for (int i = 0; i < pickObjTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				if (strId.indexOf("几何面") == 0)
				{
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
                    m_shape.push_back(id);
				}

			}

			if (m_shape.empty() )
			{
				return;
			}
			double dis = getDefaultValue(Unit::Length, shellRmThicknessEdit->text().toDouble());

            EventDispatcher::mGeoFeatureHollowingRequest req;
            req.geoIds = m_shape;
            req.thickness = dis;
            req.name = "HollowingFeature";
            EventDispatcher::mGeoOperationResult res = getGEOServer.features().hollowing(req);
            if (res.succeeded())
            {
                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑出错！"));
		}
	}

	//创建拔模特征
	void mGeoFeaturePanel::slot_createDraftBtn()
	{
		try
		{
            std::vector<int> m_shape;
            std::vector<int> partIds;
			for (int i = 0; i < pickObjTable->rowCount(); ++i)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				auto part = getGEOServer.context().PartManager().getPartByGeoFaceID(id);
                partIds.push_back(part.metadata().getID());
				m_shape.push_back(id);
			}
			if (partIds.size() != 1)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请确保拔模面位于同一实体！"));
				return;
			}
			if (m_shape.empty())
			{
				return;
			}

			//中性面id
			int neutralFace = wdpFaceId;

			wdpDir = extractVectorFromLineEdit(wdpPickMidEdit->text());
			double mdir[3] = { wdpDir[0],wdpDir[1],wdpDir[2]};

            mGeoFeatureDraftAngleRequest req;
            req.geoIds = m_shape;
            req.faceID = neutralFace;
            req.mdir[0] = mdir[0];
            req.mdir[1] = mdir[1];
            req.mdir[2] = mdir[2];
            req.angle = getDefaultValue(Unit::Angle, wdpAngleEdit->text().toDouble()) * M_PI / 180.0;
            req.isReverse = false;
            req.name = "SolidOfDraft";

            EventDispatcher::mGeoOperationResult res = getGEOServer.features().draftangle(req);
            if (res.succeeded() && !sweepMergeChkBox->isChecked() && res.generatedObjects.size() == 2)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	//创建倒角特征
	void mGeoFeaturePanel::slot_createChamferBtn()
	{
        
		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty() || chamferDimEdit->text().isEmpty())
		{
			return;
		}

        std::vector<int> m_shape;
        for (int t : geoIds)
		{
            m_shape.push_back(t);
		}

		if (chamferComboBox->currentIndex() == 1)
		{
            mGeoFeatureChamferRequest req;
            req.geoIds = m_shape;
            req.dis = getDefaultValue(Unit::Length, chamferDimEdit->text().toDouble());
            req.angle = getDefaultValue(Unit::Angle, chamferAngEdit->text().toDouble());
            EventDispatcher::mGeoOperationResult res = getGEOServer.features().chamfer(req);
            if (res.succeeded())
			{
                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                //倒角/圆角预览会把源部件置透明；正式编辑成功后必须先恢复
                //原 alpha 并删除预览面片，再触发模型刷新，否则结果体残留透明态。
                clearStretchPreviewRender();
				getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败!"));
			}
		}
		else
		{
            mGeoFeatureFilletRequest req;
            req.geoIds = m_shape;
            req.radius = getDefaultValue(Unit::Length, chamferDimEdit->text().toDouble());
            EventDispatcher::mGeoOperationResult res = getGEOServer.features().fillet(req);
            if (res.succeeded())
            {
                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                //圆角与倒角共用预览透明化；正式编辑成功后同样先恢复再刷新。
                clearStretchPreviewRender();
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑失败!"));
            }
		}
	}

	//创建孔特征
	void mGeoFeaturePanel::slot_createHoleBtn()
	{
		try
		{
			//交点信息（孔中心点）
			auto vecSelInfos = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoFace);
			if (vecSelInfos.empty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请在几何面上点选孔位置！"));
				return;
			}

			//控件值判空提示
			if (holeRadiusEdit->text().isEmpty() || holeDepthEdit->text().isEmpty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入孔的半径和深度！"));
				return;
			}
			double radius = getDefaultValue(Unit::Length, holeRadiusEdit->text().toDouble());
			double depth = getDefaultValue(Unit::Length, holeDepthEdit->text().toDouble());
			if (radius <= 0 || depth <= 0)
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("孔的半径和深度必须大于0！"));
				return;
			}

			//面ID以及孔中心点
			QHash<int, QVector<QVector3D>> points{};
			for (auto info : vecSelInfos)
			{
				int faceId = info.id;
				QVector3D pnt = info.intersection_point;
				if (points.contains(faceId))
				{
					points[faceId].append(pnt);
				}
				else
				{
					points.insert(faceId, QVector<QVector3D>{ pnt });
				}
			}
			QList<int> FaceIDList = points.keys();
			//QList<MGeoCommand::EditShape> mEditShapeList; //【OCC摘除·S10/P0-1】孤儿死声明（引擎路径不消费）
			//【OCC摘除·S10/P0-1】EditShape 构建循环整段下线（孤儿：引擎路径不消费，唯一消费者在恒 0 门控宏内）
			//for (int t : FaceIDList)
			//{
				//auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoFaceID(t);
				//if (nullptr == set)
				//{
					//MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何参数输入错误!"));
					//return;
				//}

				//MGeoCommand::EditShape mEditShape;
				//mEditShape.mSet = set;
				//mEditShape.editFaceIDs.insert(t);
				//mEditShapeList.append(mEditShape);
			//}

#if MXGUI_HAS_HOLE_FEATURE
			mGeoCommandSolidEditOfHole* mGeoHole = mGeoCommandFactory::createSolidEditOfHole();
			mGeoHole->setGeoDataBase(getCVM->getModelData()->getGeoData());
			mGeoHole->setModelDataEditShape(mEditShapeList);
			mGeoHole->setPointFaceIds(points);
			mGeoHole->setHoleType(HoleCircle);
			mGeoHole->setRadius(radius);
			mGeoHole->setDepth(depth);
			int modelDataID = getCVM->getModelData()->getModelDataID();
			if (mGeoHole->handleCommand())
			{
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, vecSelInfos);

				geoPickWidget->clearHighlight();
				MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("孔创建成功!"));
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("孔创建失败!"));
			}
#endif // MXGUI_HAS_HOLE_FEATURE
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何编辑出错！"));
		}
	}

	//激活/取消激活直接建模渲染工具（以几何面板编辑/取消编辑为界）
	void mGeoFeaturePanel::activateDirectGeoModelingTool(bool active)
	{
		if (!getCVM || !getCVM->getPreViewportWidget() || !getCVM->getPreViewportWidget()->interaction())
		{
			return;
		}
		auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
		if (!manager)
		{
			return;
		}
		manager->setActive(active ? mxr::InteractionToolType::DirectGeoModeling : mxr::InteractionToolType::None);
	}

	//清除拉伸预览渲染
	void mGeoFeaturePanel::clearStretchPreviewRender()
	{
		//恢复原部件透明度（减量预览结束时）
		setSourcePartTransparent(false, -1);

		if (!getCVM || !getCVM->getPreViewportWidget() || !getCVM->getPreViewportWidget()->interaction())
		{
			return;
		}
		auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
		auto* tool = manager ? manager->getTool<mxr::InteractionDirectGeoModelingTool>(mxr::InteractionToolType::DirectGeoModeling) : nullptr;
		if (!tool)
		{
			return;
		}
		tool->deleteGeoFeatruePoint();
		tool->deleteGeoFeatrueLine();
		tool->deleteGeoFeatrueFace();
	}

	//连接/断开指定特征的预览信号；conn 为 true 时该特征参数控件编辑都会触发对应预览
	void mGeoFeaturePanel::connectPreView(bool conn, GeoOperationType type)
	{
		auto connectEdit = [this, conn](QLineEdit* edit, void (mGeoFeaturePanel::* slot)()) {
			if (!edit)
			{
				return;
			}
			if (conn)
			{
				connect(edit, &QLineEdit::editingFinished, this, slot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(edit, &QLineEdit::editingFinished, this, slot);
			}
			};
		auto connectChk = [this, conn](QCheckBox* chk, void (mGeoFeaturePanel::* slot)()) {
			if (!chk)
			{
				return;
			}
			if (conn)
			{
				connect(chk, &QCheckBox::toggled, this, slot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(chk, &QCheckBox::toggled, this, slot);
			}
			};
		auto connectTable = [this, conn](QTableWidget* table, void (mGeoFeaturePanel::* slot)()) {
			if (!table)
			{
				return;
			}
			if (conn)
			{
				//拾取对象变化触发预览：itemChanged 覆盖内容修改，model 的增删信号覆盖行增删
				connect(table, &QTableWidget::itemChanged, this, slot, Qt::UniqueConnection);
				connect(table->model(), &QAbstractItemModel::rowsInserted, this, slot, Qt::UniqueConnection);
				connect(table->model(), &QAbstractItemModel::rowsRemoved, this, slot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(table, &QTableWidget::itemChanged, this, slot);
				disconnect(table->model(), &QAbstractItemModel::rowsInserted, this, slot);
				disconnect(table->model(), &QAbstractItemModel::rowsRemoved, this, slot);
			}
			};

		void (mGeoFeaturePanel::* previewSlot)() = nullptr;
		switch (type)
		{
		case GeoStretch:             previewSlot = &mGeoFeaturePanel::slot_stretchPreview;    break;
		case GeoRotation:            previewSlot = &mGeoFeaturePanel::slot_rotationPreview;   break;
		case GeoSweep:               previewSlot = &mGeoFeaturePanel::slot_sweepPreview;      break;
		case GeoLoft:                previewSlot = &mGeoFeaturePanel::slot_loftPreview;       break;
		case GeoShellRemoval:        previewSlot = &mGeoFeaturePanel::slot_hollowingPreview;  break;
		case GeoWithdrawingPattern:  previewSlot = &mGeoFeaturePanel::slot_draftPreview;      break;
		case GeoChamfering:          previewSlot = &mGeoFeaturePanel::slot_chamferPreview;    break;
		default: return;
		}

		switch (type)
		{
		case GeoStretch:
			connectEdit(stretchVx, previewSlot);
			connectEdit(stretchVy, previewSlot);
			connectEdit(stretchVz, previewSlot);
			connectEdit(stretchHeightEdit, previewSlot);
			connectChk(stretchMergeChkBox, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoRotation:
			connectEdit(rotationVx, previewSlot);
			connectEdit(rotationVy, previewSlot);
			connectEdit(rotationVz, previewSlot);
			connectEdit(rotationPntX, previewSlot);
			connectEdit(rotationPntY, previewSlot);
			connectEdit(rotationPntZ, previewSlot);
			connectEdit(rotationAngleEdit, previewSlot);
			connectChk(rotationMergeChkBox, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoSweep:
			connectChk(sweepMergeChkBox, previewSlot);
			connectTable(pickObjTable, previewSlot);
			connectTable(sweepPickLineTable, previewSlot);
			break;
		case GeoLoft:
			connectChk(loftTypeChkBox, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoShellRemoval:
			connectEdit(shellRmThicknessEdit, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoWithdrawingPattern:
			connectEdit(wdpAngleEdit, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoChamfering:
			connectEdit(chamferDimEdit, previewSlot);
			connectEdit(chamferAngEdit, previewSlot);
			if (conn)
			{
				connect(chamferComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(chamferComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot);
			}
			connectTable(pickObjTable, previewSlot);
			break;
		default:
			break;
		}

		if (!conn)
		{
			clearStretchPreviewRender();
		}
	}

	//连接/断开拉伸预览信号
	void mGeoFeaturePanel::connectPreView(bool conn)
	{
		connectPreView(conn, GeoStretch);
		if (conn)
		{
			//勾选后立即按当前参数预览一次
			slot_stretchPreview();
		}
	}

	//拉伸预览：构建预览请求包，调用预览服务，把离散化结果交给直接建模渲染工具
	void mGeoFeaturePanel::slot_stretchPreview()
	{
		if (!stretchPreviewChkBox->isChecked() || GeoStretch != currentFeature)
		{
			return;
		}
		if (!getCVM || !getCVM->getPreViewportWidget() || !getCVM->getPreViewportWidget()->interaction())
		{
			return;
		}
		auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
		auto* tool = manager ? manager->getTool<mxr::InteractionDirectGeoModelingTool>(mxr::InteractionToolType::DirectGeoModeling) : nullptr;
		if (!tool)
		{
			return;
		}

		//收集拾取对象
		std::vector<int> geoLineIds;
		std::vector<int> geoFaceIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			if (strId.indexOf("几何线") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoLineIds.push_back(strId.toInt());
			}
			else if (strId.indexOf("几何面") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoFaceIds.push_back(strId.toInt());
			}
		}
		if (geoLineIds.empty() && geoFaceIds.empty())
		{
			clearStretchPreviewRender();
			return;
		}
		if (stretchHeightEdit->text().isEmpty())
		{
			clearStretchPreviewRender();
			return;
		}

		//构建预览请求包（复用正式拉伸请求包）
		EventDispatcher::mGeoFeatureStretchRequest req;
		if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
		{
			req.geoIds = geoLineIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
		}
		else
		{
			req.geoIds = geoFaceIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Face;
		}
		req.dis = getDefaultValue(Unit::Length, stretchHeightEdit->text().toDouble());
		req.mdir1[0] = stretchVx->text().toDouble();
		req.mdir1[1] = stretchVy->text().toDouble();
		req.mdir1[2] = stretchVz->text().toDouble();
		req.isFuse = stretchMergeChkBox->isChecked();
		req.isReverse = false;
		req.name = "StretchFeature";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().stretchPreview(req);

		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}

		renderPreviewResult(res);
	}

	//当前特征的预览勾选框是否勾选
	bool mGeoFeaturePanel::isPreviewChecked(GeoOperationType type) const
	{
		switch (type)
		{
		case GeoStretch:             return stretchPreviewChkBox && stretchPreviewChkBox->isChecked();
		case GeoRotation:            return rotationPreviewChkBox && rotationPreviewChkBox->isChecked();
		case GeoSweep:               return sweepPreviewChkBox && sweepPreviewChkBox->isChecked();
		case GeoLoft:                return loftPreviewChkBox && loftPreviewChkBox->isChecked();
		case GeoShellRemoval:        return shellRmPreviewChkBox && shellRmPreviewChkBox->isChecked();
		case GeoWithdrawingPattern:  return wdpPreviewChkBox && wdpPreviewChkBox->isChecked();
		case GeoChamfering:          return chamferPreviewChkBox && chamferPreviewChkBox->isChecked();
		default:                     return false;
		}
	}

	//减量操作预览时把原部件透明，预览结束时恢复原透明度
	void mGeoFeaturePanel::setSourcePartTransparent(bool transparent, int partID)
	{
		if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
		{
			return;
		}

		//先恢复上一次透明化的部件（恢复颜色与透明度，并关闭渲染透明模式）
		if (m_transparentPartID >= 0)
		{
			mPartData* oldPart = getCVM->getModelData()->getMeshData()->getPartData(m_transparentPartID);
			if (oldPart && m_transparentPartOriginAlpha >= 0)
			{
				oldPart->setPartAlpha(static_cast<ushort>(m_transparentPartOriginAlpha));
				if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
				{
					getCVM->getPreRenderWidget()->getPreRender()->setTransparent(false, 1.0f);
				}
				getCVM->updatePartShowColor(getCVM->getPartType(), { m_transparentPartID });
			}
			m_transparentPartID = -1;
			m_transparentPartOriginAlpha = -1;
		}

		if (!transparent || partID < 0)
		{
			return;
		}

		mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(partID);
		if (!part)
		{
			return;
		}

		//记录原透明度（ushort 刻度），再透明化
		m_transparentPartOriginAlpha = static_cast<int>(part->getPartAlpha());
		m_transparentPartID = partID;
		//减量预览时原部件透明 30%：部件 alpha 约定 0=全透明、255=不透明，30%透明取 76
		part->setPartAlpha(static_cast<ushort>(76));
		//渲染管线只有在透明模式下才会使用部件 alpha
		if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
		{
			getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 1.0f);
		}
		getCVM->updatePartShowColor(getCVM->getPartType(), { partID });
	}

	//把预览离散化结果渲染到视口（颜色取被操作部件内色，线/点同色系加深；
	//增量操作面 30% 透明，减量操作面不透明）
	void mGeoFeaturePanel::renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res, bool reducePreview)
	{
		if (!getCVM || !getCVM->getPreViewportWidget() || !getCVM->getPreViewportWidget()->interaction())
		{
			return;
		}
		auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
		auto* tool = manager ? manager->getTool<mxr::InteractionDirectGeoModelingTool>(mxr::InteractionToolType::DirectGeoModeling) : nullptr;
		if (!tool)
		{
			return;
		}

		//预览颜色：按面所属部件着色（与 mGeoInfoPanel::slot_setPartColor 写入的
		//用户色同源），仅透明度按增量/减量区分。取不到部件时退回 res.partID，再退回默认米色。
		const int faceAlpha = reducePreview ? 0 : 76;
		const QColor kFallbackColor(255, 243, 228);
		QHash<int, QVector4D> partFaceColors;
		QHash<int, QVector3D> partLineColors;
		auto colorOfPart = [&](int partID) -> QColor
		{
			if (partID >= 0 && getCVM->getModelData() && getCVM->getModelData()->getMeshData())
			{
				mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(partID);
				if (part)
				{
					const QColor c = part->getPartInnerColor();
					if (c.isValid()) return c;
				}
			}
			return QColor();
		};
		auto faceColorOfPart = [&](int partID) -> QVector4D
		{
			if (!partFaceColors.contains(partID))
			{
				const QColor c = colorOfPart(partID);
				const QColor used = c.isValid() ? c : kFallbackColor;
				partFaceColors.insert(partID, QVector4D(used.red(), used.green(), used.blue(), faceAlpha));
				partLineColors.insert(partID, QVector3D(used.red(), used.green(), used.blue()));
			}
			return partFaceColors.value(partID);
		};
		auto lineColorOfPart = [&](int partID) -> QVector3D
		{
			faceColorOfPart(partID); // 确保缓存已填
			return partLineColors.value(partID);
		};
		// 主部件色：优先 res.partID，用于线/点及无 facePartIDs 时的回退。
		const QVector4D kPreviewFaceColor = faceColorOfPart(res.partID);
		// 线/点统一用不透明黑色，与部件色区分
		const QVector3D kPreviewLineColor(90.0f, 90.0f, 90.0f);
		const QVector3D kPreviewPointColor = kPreviewLineColor;

		//中性离散数据 → 渲染接口数据（颜色数组按渲染接口要求与点数组等长，由 GUI 统一填充）
		QVector<std::pair<QVector3D, float>> previewPoints;
		previewPoints.reserve(res.mesh.points.size());
		for (const auto& p : res.mesh.points)
		{
			previewPoints.push_back({ QVector3D(p.first.x(), p.first.y(), p.first.z()), p.second });
		}
		QVector<QVector3D> previewPointColors(previewPoints.size(), kPreviewPointColor);

		QVector<QVector3D> previewLines;
		previewLines.reserve(res.mesh.lines.size());
		for (const auto& p : res.mesh.lines)
		{
			previewLines.push_back(QVector3D(p.x(), p.y(), p.z()));
		}
		QVector<QVector3D> previewLineColors(previewLines.size(), kPreviewLineColor);

		QVector<QVector3D> previewFaces;
		previewFaces.reserve(static_cast<int>(res.mesh.faceTriangles.size()));
		for (const auto& p : res.mesh.faceTriangles)
		{
			previewFaces.append(QVector3D(p.x(), p.y(), p.z()));
		}
		// 逐三角形按 facePartIDs 取所属部件色；数据缺失时退回主部件色。
		QVector<QVector4D> previewFaceColors;
		previewFaceColors.reserve(previewFaces.size());
		const int triCount = previewFaces.size() / 3;
		const bool hasPartIDs = static_cast<int>(res.mesh.facePartIDs.size()) == triCount;
		for (int t = 0; t < triCount; ++t)
		{
			const QVector4D c = hasPartIDs ? faceColorOfPart(res.mesh.facePartIDs[t]) : kPreviewFaceColor;
			previewFaceColors.append(c);
			previewFaceColors.append(c);
			previewFaceColors.append(c);
		}

		tool->drawGeoFeatruePoint(previewPoints, previewPointColors);
		tool->drawGeoFeatrueLine(previewLines, previewLineColors, res.mesh.lineWidth);
		tool->drawGeoFeatrueFace(previewFaces, previewFaceColors);

		//强制刷新视口，避免上一帧画面残留
		if (getCVM->getPreRenderWidget())
		{
			getCVM->getPreRenderWidget()->updatePaint();
		}
	}

	//旋转预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_rotationPreview()
	{
		if (!isPreviewChecked(GeoRotation) || GeoRotation != currentFeature)
		{
			return;
		}

		//收集拾取对象
		std::vector<int> geoLineIds;
		std::vector<int> geoFaceIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			if (strId.indexOf("几何线") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoLineIds.push_back(strId.toInt());
			}
			else if (strId.indexOf("几何面") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoFaceIds.push_back(strId.toInt());
			}
		}
		if (geoLineIds.empty() && geoFaceIds.empty())
		{
			clearStretchPreviewRender();
			return;
		}
		if (rotationAngleEdit->text().isEmpty())
		{
			clearStretchPreviewRender();
			return;
		}

		EventDispatcher::mGeoFeatureRotateRequest req;
		if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
		{
			req.geoIds = geoLineIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
		}
		else
		{
			req.geoIds = geoFaceIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Face;
		}
		req.angle = getDefaultValue(Unit::Angle, rotationAngleEdit->text().toDouble()) * M_PI / 180.0;
		req.mAxis1[0] = rotationVx->text().toDouble();
		req.mAxis1[1] = rotationVy->text().toDouble();
		req.mAxis1[2] = rotationVz->text().toDouble();
		req.mPnt[0] = rotationPntX->text().toDouble();
		req.mPnt[1] = rotationPntY->text().toDouble();
		req.mPnt[2] = rotationPntZ->text().toDouble();
		req.isFuse = rotationMergeChkBox->isChecked();
		req.isReverse = false;
		req.name = "RotateFeature";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().rotatePreview(req);
		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//扫掠预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_sweepPreview()
	{
		if (!isPreviewChecked(GeoSweep) || GeoSweep != currentFeature)
		{
			return;
		}

		std::vector<int> geoLineIds;
		std::vector<int> geoFaceIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			if (strId.indexOf("几何线") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoLineIds.push_back(strId.toInt());
			}
			else if (strId.indexOf("几何面") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				geoFaceIds.push_back(strId.toInt());
			}
		}

		std::vector<int> pathIds;
		for (int i = 0; i < sweepPickLineTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = sweepPickLineTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			pathIds.push_back(strId.toInt());
		}

		if ((geoLineIds.empty() && geoFaceIds.empty()) || pathIds.empty())
		{
			clearStretchPreviewRender();
			return;
		}

		EventDispatcher::mGeoFeatureSweepRequest req;
		if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
		{
			req.geoIds = geoLineIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Edge;
		}
		else
		{
			req.geoIds = geoFaceIds;
			req.shapeType = GeoNeutralData::GN_ShapeType::Face;
		}
		req.pathIds = pathIds;
		req.isFuse = sweepMergeChkBox->isChecked();
		req.name = "SweepFeature";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().sweepPreview(req);
		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//放样预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_loftPreview()
	{
		if (!isPreviewChecked(GeoLoft) || GeoLoft != currentFeature)
		{
			return;
		}

		std::vector<int> shapeIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			shapeIds.push_back(strId.toInt());
		}
		if (shapeIds.empty())
		{
			clearStretchPreviewRender();
			return;
		}

		EventDispatcher::mGeoFeatureLoftRequest req;
		req.geoIds = shapeIds;
		req.isSolid = loftTypeChkBox->isChecked();
		req.name = "SolidOfLoft";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().loftPreview(req);
		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//抽壳预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_hollowingPreview()
	{
		if (!isPreviewChecked(GeoShellRemoval) || GeoShellRemoval != currentFeature)
		{
			return;
		}

		std::vector<int> shapeIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			if (strId.indexOf("几何面") == 0)
			{
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				shapeIds.push_back(strId.toInt());
			}
		}
		if (shapeIds.empty() || shellRmThicknessEdit->text().isEmpty())
		{
			clearStretchPreviewRender();
			return;
		}

		EventDispatcher::mGeoFeatureHollowingRequest req;
		req.geoIds = shapeIds;
		req.thickness = getDefaultValue(Unit::Length, shellRmThicknessEdit->text().toDouble());
		req.name = "HollowingFeature";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().hollowingPreview(req);
		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		//减量操作预览：先把原部件透明，再画不透明预览（透明度与增量操作相反）
		setSourcePartTransparent(true, res.partID);
		renderPreviewResult(res, true);
	}

	//拔模预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_draftPreview()
	{
		if (!isPreviewChecked(GeoWithdrawingPattern) || GeoWithdrawingPattern != currentFeature)
		{
			return;
		}

		std::vector<int> shapeIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			shapeIds.push_back(strId.toInt());
		}
		if (shapeIds.empty() || wdpFaceId < 0 || wdpAngleEdit->text().isEmpty())
		{
			clearStretchPreviewRender();
			return;
		}

		wdpDir = extractVectorFromLineEdit(wdpPickMidEdit->text());
		EventDispatcher::mGeoFeatureDraftAngleRequest req;
		req.geoIds = shapeIds;
		req.faceID = wdpFaceId;
		req.mdir[0] = wdpDir[0];
		req.mdir[1] = wdpDir[1];
		req.mdir[2] = wdpDir[2];
		req.angle = getDefaultValue(Unit::Angle, wdpAngleEdit->text().toDouble()) * M_PI / 180.0;
		req.isReverse = false;
		req.name = "SolidOfDraft";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.features().draftanglePreview(req);
		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//倒角/圆角预览：构建预览请求包，调用预览服务渲染
	void mGeoFeaturePanel::slot_chamferPreview()
	{
		if (!isPreviewChecked(GeoChamfering) || GeoChamfering != currentFeature)
		{
			return;
		}

		std::vector<int> shapeIds;
		for (int i = 0; i < pickObjTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = pickObjTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			shapeIds.push_back(strId.toInt());
		}
		if (shapeIds.empty() || chamferDimEdit->text().isEmpty())
		{
			clearStretchPreviewRender();
			return;
		}

		EventDispatcher::mGeoPreviewResult res;
		if (chamferComboBox->currentIndex() == 1)
		{
			EventDispatcher::mGeoFeatureChamferRequest req;
			req.geoIds = shapeIds;
			req.dis = getDefaultValue(Unit::Length, chamferDimEdit->text().toDouble());
			req.angle = getDefaultValue(Unit::Angle, chamferAngEdit->text().toDouble());
			req.name = "ChamferFeature";
			res = getGEOServer.features().chamferPreview(req);
		}
		else
		{
			EventDispatcher::mGeoFeatureFilletRequest req;
			req.geoIds = shapeIds;
			req.radius = getDefaultValue(Unit::Length, chamferDimEdit->text().toDouble());
			req.name = "FilletFeature";
			res = getGEOServer.features().filletPreview(req);
		}

		if (!res.succeeded())
		{
			clearStretchPreviewRender();
			return;
		}
		//减量操作预览：先把原部件透明，再画不透明预览（透明度与增量操作相反）
		setSourcePartTransparent(true, res.partID);
		renderPreviewResult(res, true);
	}

	//创建孔特征

	void mGeoFeaturePanel::slot_highlightGeoData()
	{
		QString type = "几何线";

		int num = 0;
		vector<int> res;
		//获取拾取模式
		PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		switch (pickFilter)
		{
		case mxr::PickFilter::PickGeoSolid:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			type = "几何体";
			break;
		}
		case mxr::PickFilter::PickGeoFace:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			type = "几何面";
			break;
		}
		case mxr::PickFilter::PickGeoLine:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			type = "几何线";
			break;
		}
		case mxr::PickFilter::PickGeoPoint:
		{
			res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			type = "几何点";
			break;
		}
		default:
			break;
		}
		num = res.size();

		if (geoPickWidget->strPickType != type)
		{
			return;
		}

		if (geoPickWidget->_featPick == FeatureObj)//特征形状对象
		{
			for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
			{
				QTableWidgetItem* item = pickObjTable->item(i, 0);
				if (!item)
				{
					continue;
				}
				QString strId = item->text();
				if (strId.indexOf(type) == 0)
				{
					pickObjTable->removeRow(i);
				}
			}

			for (auto id : res)
			{
				pickObjTable->insertRow(pickObjTable->rowCount());
				pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
			}

			//拾取对象变化后刷新当前特征预览
			if (GeoStretch == currentFeature)            slot_stretchPreview();
			else if (GeoRotation == currentFeature)      slot_rotationPreview();
			else if (GeoSweep == currentFeature)         slot_sweepPreview();
			else if (GeoLoft == currentFeature)          slot_loftPreview();
			else if (GeoShellRemoval == currentFeature)  slot_hollowingPreview();
			else if (GeoWithdrawingPattern == currentFeature) slot_draftPreview();
			else if (GeoChamfering == currentFeature)    slot_chamferPreview();
		}

		if (geoPickWidget->_featPick == StretchDirection)
		{
			//几何线
			auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setCoord(QVector3D{}, stretchVx, stretchVy, stretchVz);
				return;
			}
			else
			{
				setCoord(lines[lines.size() - 1].m_dir, stretchVx, stretchVy, stretchVz);
			}

			disHighlight(stretchDirId, "几何线");
			stretchDirId = lines[lines.size() - 1].id;
		}
		if (geoPickWidget->_featPick == RotationAxis)
		{
			//几何线
			auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setCoord(QVector3D{}, rotationVx, rotationVy, rotationVz);
				return;
			}
			else
			{
				setCoord(lines[lines.size() - 1].m_dir, rotationVx, rotationVy, rotationVz);
			}

			disHighlight(rotationAxisId, "几何线");
			rotationAxisId = lines[lines.size() - 1].id;
		}
		if (geoPickWidget->_featPick == RotationAxisPnt)
		{
			auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
			if (points.size() <= 0)
			{
				setCoord(QVector3D{}, rotationPntX, rotationPntY, rotationPntZ);
				return;
			}
			else
			{
				setCoord(points[points.size() - 1].m_coor, rotationPntX, rotationPntY, rotationPntZ);
			}

			disHighlight(rotationPntId, "几何点");
			rotationPntId = points[points.size() - 1].id;
		}

		if (geoPickWidget->_featPick == SweepLine)
		{
			while (sweepPickLineTable->rowCount())
			{
				sweepPickLineTable->removeRow(0);
			}

			for (auto id : res)
			{
				sweepPickLineTable->insertRow(sweepPickLineTable->rowCount());
				sweepPickLineTable->setItem(sweepPickLineTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
			}

			//扫掠路径变化后刷新扫掠预览
			if (GeoSweep == currentFeature)
			{
				slot_sweepPreview();
			}
		}
		if (geoPickWidget->_featPick == DraftNeutralFace)
		{
			auto faces = getGEOServer.geoCalculate().CalculateFaceAttr(res);
			if (faces.size() <= 0)
			{
				setVectorToLineEdit(wdpPickMidEdit, QVector3D{});
				return;
			}
			else
			{
				//获取中性面的法向量
				wdpDir = faces[faces.size() - 1].m_dir;
				setVectorToLineEdit(wdpPickMidEdit, wdpDir);
			}

			disHighlight(wdpFaceId, type);
			wdpFaceId = faces[faces.size() - 1].id;

			//中性面变化后刷新拔模预览
			if (GeoWithdrawingPattern == currentFeature)
			{
				slot_draftPreview();
			}
		}

		if (geoPickWidget->_featPick == HoleFace && type.compare("几何面") == 0)
		{
			//高亮点选的孔中心点（同硬点添加）
			getCVM->getPreRenderWidget()->getHighlightController()
				->updateExtraHighLight(getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData());
		}
	}

	void mGeoFeaturePanel::hideEvent(QHideEvent* event)
	{
		//退出创建孔等功能时复位拾取状态并清掉点击点标记，避免隐藏面板仍响应拾取
		geoPickWidget->_featPick = None;
		geoPickWidget->clearHighlight();
		//取消编辑几何面板：断开当前特征预览信号、清除预览渲染、取消激活渲染工具
		connectPreView(false, currentFeature);
		clearStretchPreviewRender();
		activateDirectGeoModelingTool(false);
		mGeoPanelBase::hideEvent(event);
	}

	void mGeoFeaturePanel::slot_subWindowActivated()
	{
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoFeaturePanel::slot_highlightGeoData);
	}

	void mGeoFeaturePanel::slot_contextMenuTableData(const QPoint& pos)
	{
		mUiMenu menu(sweepPickLineTable);
		menu.addActions({ sweepDelAction });

		QAction* selected = menu.exec(sweepPickLineTable->viewport()->mapToGlobal(pos));
		if (selected == sweepDelAction)
		{
			std::vector<int> Ids;
			QList<QTableWidgetItem*> items = sweepPickLineTable->selectedItems();
			if (!items.isEmpty())
			{
				vector<int> ids;
				for (auto item : items)
				{
					ids.push_back(item->row());
				}

				sort(ids.begin(), ids.end());

				for (int i = ids.size() - 1; i >= 0; --i)
				{
					QTableWidgetItem* item = sweepPickLineTable->item(ids[i], 0);
					if (!item)
					{
						continue;
					}
					QString strId = item->text();
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();
					Ids.push_back(id);

					sweepPickLineTable->removeRow(ids[i]);
				}
				getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoEdge, Ids);
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
	}
}
