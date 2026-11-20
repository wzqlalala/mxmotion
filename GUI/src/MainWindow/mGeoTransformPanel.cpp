#include "mGeoTransformPanel.h"
#include "ViewportContainer.h"
#include <cmath> //【OCC摘除·S10/P0-1】M_PI 显式自含（原经退役头 OCC 链传递）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
//#include "mGeoCommandSolidOfDragVecOCC.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）
//#include "mGeoCommandOperateOfRotateOCC.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）

#include "mGeoManager.h"
//#include <TopoDS_Shape.hxx> //【OCC迁移·§8】零调用死 include 注释
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/view/HighlightController.h"
#include "mxr/view/mFontRender.h"

#include "InteractionCore/Event/Action.h"
#include "InteractionCore/InteractionSystem.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/DirectGeoModelingTool.h"
#include "PreViewportWidget.h"

#include "mGeoPreviewTypes.h"
#include "mGeoServer.h"
#include <mMeshData.h>

#include "mViewportModelManager.h"
#include "mViewportModel.h"

//BasicData
#include "mPhysicalFieldEnum.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mCoordinateManager.h"

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1】随退役头下线
using namespace mxr;
namespace MechMainWindow
{
    mGeoTransformPanel::mGeoTransformPanel(QWidget* parent)
        : mGeoPanelBase(parent)
    {
        offsetLastDirID = -1;
        offsetLastStartID = -1;
        offsetLastEndID = -1;
        rotateLastPntID = -1;
        rotateLastAxisID = -1;
        mirrorLastAxisID = -1;
        mirrorLastPntID = -1;
        zoomLastPntID = -1;
        lineArrayLastDirID = -1;
        ringArrayLastAxisID = -1;
        ringArrayLastPntID = -1;

        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        //按顺序初始化
        initOffsetFold();
        initRotateFold();
        initMirrorimageFold();
        initZoomFold();
        initLineArrayFold();
        initRingArrayFold();
        initLocationFold();


        locationSourcePntAId =
            locationSourcePntBId =
            locationSourcePntCId =
            locationTargetPntAId =
            locationTargetPntBId =
            locationTargetPntCId
            = -1;
        coordSystemAId =
            coordSystemBId = -2;
        _mainLayout->addStretch();

        //全局信号（拾取/单位制/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接

        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(offsetUnitsLabel, offsetDisEdit));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(lineArrayUnitsLabel, lineArrayDisEdit));

        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(ringArrayUnitsLabel, ringArrayAngleEdit));
        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(rotateUnitsLabel, rotateAngleEdit));
    }

    void mGeoTransformPanel::connectGeoSignals()
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

    void mGeoTransformPanel::disconnectGeoSignals()
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

    mGeoTransformPanel::~mGeoTransformPanel()
    {

    }

    void mGeoTransformPanel::showFold(GeoOperationType type)
    {
        hideAllFold();
        currentFeature = type;
        geoPickWidget->pickLabel->setText("拾取");

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->pickObjTypeCombo->addItems({ "几何点","几何线","几何面","几何体" });
        geoPickWidget->_featPick = FeatureObj;

        if (GeoOffset == type)
        {
            m_FoldSections[0]->insertRow(0, pickObjTable);
            m_FoldSections[0]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 0,1 });

            //进入编辑时激活直接建模渲染工具，并按勾选框状态挂接预览
            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoOffset), GeoOffset);
        }
        if (GeoRotate == type)
        {
            m_FoldSections[1]->insertRow(0, pickObjTable);
            m_FoldSections[1]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 2,3,4 });

            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoRotate), GeoRotate);
        }
        if (GeoMirrorimage == type)
        {
            m_FoldSections[2]->insertRow(0, pickObjTable);
            m_FoldSections[2]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 5,6,7 });

            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoMirrorimage), GeoMirrorimage);
        }
        if (GeoZoom == type)
        {
            geoPickWidget->pickObjTypeCombo->clear();
            geoPickWidget->pickObjTypeCombo->addItems({ "几何线","几何面","几何体" });

            m_FoldSections[3]->insertRow(0, pickObjTable);
            m_FoldSections[3]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 8,9,10 });

            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoZoom), GeoZoom);
        }
        if (GeoLinearArray == type)
        {
            m_FoldSections[4]->insertRow(0, pickObjTable);
            m_FoldSections[4]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 11,12 });

            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoLinearArray), GeoLinearArray);
        }
        if (GeoRingArray == type)
        {
            m_FoldSections[5]->insertRow(0, pickObjTable);
            m_FoldSections[5]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 13,14,15 });

            activateDirectGeoModelingTool(true);
            connectPreView(isPreviewChecked(GeoRingArray), GeoRingArray);
        }
        if (GeoLocation == type)
        {
            m_FoldSections[6]->insertRow(0, pickObjTable);
            m_FoldSections[6]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 16,17 });

            vector<QString> showSymbol = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoorinateIDStr();
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            symbolController->setAllSymbolShowState(false);
            symbolController->setSymbolShowState(showSymbol, true);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }

        geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
    }

    //初始化平移操作界面
    void mGeoTransformPanel::initOffsetFold()
    {
        //初始化平移所有控件
        offsetMoveRadio = new QRadioButton("移动", this);
        offsetMoveRadio->setChecked(true);
        offsetCopyRadio = new QRadioButton("复制", this);

        QToolButton* offsetReverseBtn = new QToolButton(this);
        offsetReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        offsetReverseBtn->setToolTip("反向");
        connect(offsetReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(offsetVx, offsetVy, offsetVz);
            });
        QToolButton* offsetMousePickBtn = new QToolButton(this);
        offsetMousePickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        offsetMousePickBtn->setToolTip("选择平移方向");
        connect(offsetMousePickBtn, &QToolButton::clicked, this, [this, offsetMousePickBtn]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(offsetLastDirID, "几何线");
            geoPickWidget->_featPick = OffsetDirection;
            });

        QToolButton* offsetPickStartBtn = new QToolButton(this);
        offsetPickStartBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        offsetPickStartBtn->setToolTip("选择起始点");
        connect(offsetPickStartBtn, &QToolButton::clicked, this, [this, offsetPickStartBtn]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(offsetLastStartID, "几何点");
            geoPickWidget->_featPick = OffsetStartPnt;
            });

        QToolButton* offsetPickEndBtn = new QToolButton(this);
        offsetPickEndBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        offsetPickEndBtn->setToolTip("选择终止点");
        connect(offsetPickEndBtn, &QToolButton::clicked, this, [this, offsetPickEndBtn]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(offsetLastEndID, "几何点");
            geoPickWidget->_featPick = OffsetEndPnt;
            });


        offsetDisEdit = new MxFunctions::mCalcLineEdit(this); offsetDisEdit->setExpression("10");
        offsetUnitsLabel = new QLabel("mm");

		offsetPreviewChkBox = new QCheckBox("预览", this);
		offsetPreviewChkBox->setChecked(false);
		connect(offsetPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoOffset);
			if (checked)
			{
				slot_offsetPreview();
			}
			});


        offsetCancelBtn = new QPushButton("取消", this);
        offsetOkBtn = new QPushButton("确认", this);

        offsetVx = new MxFunctions::mCalcLineEdit(this); offsetVx->setExpression("1");
        offsetVy = new MxFunctions::mCalcLineEdit(this); offsetVy->setExpression(""); offsetVy->setPlaceholderText("0");
        offsetVz = new MxFunctions::mCalcLineEdit(this); offsetVz->setExpression(""); offsetVz->setPlaceholderText("0");

        offsetEVx = new MxFunctions::mCalcLineEdit(this); offsetEVx->setExpression("1");
        offsetEVy = new MxFunctions::mCalcLineEdit(this); offsetEVy->setExpression(""); offsetEVy->setPlaceholderText("0");
        offsetEVz = new MxFunctions::mCalcLineEdit(this); offsetEVz->setExpression(""); offsetEVz->setPlaceholderText("0");

        offsetSXLabel = new PanelLabel("Vx", this);
        offsetSYLabel = new PanelLabel("Vy", this);
        offsetSZLabel = new PanelLabel("Vz", this);

        offsetEXLabel = new PanelLabel("End Vx", this);
        offsetEYLabel = new PanelLabel("End Vy", this);
        offsetEZLabel = new PanelLabel("End Vz", this);

        connect(offsetCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(offsetOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_offsetAction);

        // ========== 定义平移折叠块 ==========
        FoldSection* offsetFold_1 = new FoldSection("平移定义", this);
        m_FoldSections.append(offsetFold_1);
        m_foldWidgets.append(offsetFold_1);

        // ========== 平移方向折叠块 ==========
        offsetCustomDis = new QCheckBox("自定义距离", this);
        connect(offsetCustomDis, &QCheckBox::stateChanged, this, [this]() {
            offsetDisLabel->setVisible(false);
            offsetDisEdit->setVisible(false);
            offsetUnitsLabel->setVisible(false);
            offsetExLabel->setVisible(false);
            offsetExEdit->setVisible(false);
            if (offsetCustomDis->checkState())
            {
                offsetDisLabel->setVisible(true);
                offsetDisEdit->setVisible(true);
                offsetUnitsLabel->setVisible(true);
                offsetExLabel->setVisible(false);
                offsetExEdit->setVisible(false);
            }
            else
            {
                offsetDisLabel->setVisible(false);
                offsetDisEdit->setVisible(false);
                offsetUnitsLabel->setVisible(false);
                offsetExLabel->setVisible(true);
                offsetExEdit->setVisible(true);
            }
            });

        //给offsetReverseBtn占位
        QStackedWidget* stacked = new QStackedWidget(this);
        stacked->setMaximumWidth(26);
        stacked->addWidget(offsetReverseBtn);
        stacked->addWidget(new QWidget(this));

        offsetExLabel = new QLabel("放大系数", this);
        offsetDisLabel = new QLabel("平移距离", this);
        offsetExEdit = new MxFunctions::mCalcLineEdit(this); offsetExEdit->setExpression("");
        offsetExEdit->setPlaceholderText("1.00");
        offsetVec = new QComboBox(this);
        offsetVec->addItems({ "方向向量","两点向量" });


        offsetCustomDis->setVisible(false);
        offsetPickStartBtn->setVisible(false);
        offsetPickEndBtn->setVisible(false);

        FoldSection* offsetFold_2 = new FoldSection("平移方向和距离", this);
        QStackedWidget* stackWidget = new QStackedWidget();
        QWidget* widget1 = new QWidget;
        QWidget* widget2 = new QWidget;
        stackWidget->addWidget(widget1);
        stackWidget->addWidget(widget2);
        stackWidget->setCurrentIndex(0);
        offsetFold_2->addRow(stackWidget);

        QFormLayout* m_formLayout = new QFormLayout(widget1);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        QFormLayout* m_formLayout2 = new QFormLayout(widget2);
        m_formLayout2->setContentsMargins(0, 0, 0, 0);
        m_formLayout2->setSpacing(10);

        QHBoxLayout* h1 = new QHBoxLayout();
        h1->addWidget(new PanelLabel("平移方向类型")); h1->addWidget(offsetVec);

        QHBoxLayout* h2 = new QHBoxLayout();
        h2->addWidget(offsetSXLabel); h2->addWidget(offsetVx); h2->addWidget(offsetPickStartBtn); h2->addWidget(offsetMousePickBtn);

        QHBoxLayout* h3 = new QHBoxLayout();
        h3->addWidget(offsetSYLabel); h3->addWidget(offsetVy); h3->addWidget(stacked);

        QHBoxLayout* h4 = new QHBoxLayout();
        h4->addWidget(offsetSZLabel); h4->addWidget(offsetVz); h4->addSpacing(28);

        QHBoxLayout* h5 = new QHBoxLayout();
        h5->addWidget(offsetEXLabel); h5->addWidget(offsetEVx); h5->addWidget(offsetPickEndBtn);

        QHBoxLayout* h6 = new QHBoxLayout();
        h6->addWidget(offsetEYLabel); h6->addWidget(offsetEVy); h6->addSpacing(28);

        QHBoxLayout* h7 = new QHBoxLayout();
        h7->addWidget(offsetEZLabel); h7->addWidget(offsetEVz); h7->addSpacing(28);

        QHBoxLayout* h8 = new QHBoxLayout();
        h8->addWidget(offsetDisLabel); h8->addStretch(); h8->addWidget(offsetDisEdit); h8->addWidget(offsetUnitsLabel);

        QHBoxLayout* h9 = new QHBoxLayout();
        h9->addWidget(offsetExLabel); h9->addStretch(); h9->addWidget(offsetExEdit);

        QHBoxLayout* h10 = new QHBoxLayout();
        h10->addWidget(new PanelLabel("类型")); h10->addWidget(offsetMoveRadio); h10->addWidget(offsetCopyRadio);

        QHBoxLayout* h11 = new QHBoxLayout();
		h11->addWidget(offsetPreviewChkBox);
        h11->addWidget(offsetCustomDis);


        m_formLayout->addRow(h1);
        m_formLayout->addRow(h2);
        m_formLayout->addRow(h3);
        m_formLayout->addRow(h4);
        m_formLayout2->addRow(h5);
        m_formLayout2->addRow(h6);
        m_formLayout2->addRow(h7);
        m_formLayout->addRow(h8);
        m_formLayout2->addRow(h9);
        m_formLayout->addRow(h10);
        m_formLayout->addRow(h11);


        //多行布局
        //offsetFold_2->addRow(new PanelLabel("平移方向类型"), offsetVec);
        //offsetFold_2->addRow(offsetSXLabel, offsetVx, offsetPickStartBtn, offsetMousePickBtn);
        //offsetFold_2->addRow(offsetSYLabel, offsetVy, stacked);
        //offsetFold_2->addRow(offsetSZLabel, offsetVz,28);
        //offsetFold_2->addRow(offsetEXLabel, offsetEVx, offsetPickEndBtn);
        //offsetFold_2->addRow(offsetEYLabel, offsetEVy,28);
        //offsetFold_2->addRow(offsetEZLabel, offsetEVz,28);
        //offsetFold_2->addRow(offsetDisLabel, offsetDisEdit, offsetUnitsLabel);
        //offsetFold_2->addRow(offsetExLabel, offsetExEdit);
        //offsetFold_2->addRow(new PanelLabel("类型"), offsetMoveRadio, offsetCopyRadio);
        //offsetFold_2->addRow(offsetPreviewChkBox, offsetCustomDis);

        QFrame* horizontalLine = createHorizontalLine(offsetFold_2);
        offsetFold_2->addRow(horizontalLine);

        offsetFold_2->addRow(offsetCancelBtn, offsetOkBtn, "Left");
        m_foldWidgets.append(offsetFold_2);

        connect(offsetVec, QOverload<int>::of(&QComboBox::activated),
            [this, offsetReverseBtn, offsetMousePickBtn, offsetPickStartBtn, offsetPickEndBtn, stacked, stackWidget,
            m_formLayout, m_formLayout2, h1, h2, h3, h4, h5, h6, h7, h8, h9, h10, h11](int index) {

                offsetCustomDis->setChecked(false);

                if (index == 0) {
                    geoPickWidget->_featPick = None;
                    offsetSXLabel->setText("Vx");
                    offsetSYLabel->setText("Vy");
                    offsetSZLabel->setText("Vz");

                    h1->setParent(nullptr);
                    h2->setParent(nullptr);
                    h3->setParent(nullptr);
                    h4->setParent(nullptr);
                    h8->setParent(nullptr);
                    h10->setParent(nullptr);
                    h11->setParent(nullptr);
                    m_formLayout->addRow(h1);
                    m_formLayout->addRow(h2);
                    m_formLayout->addRow(h3);
                    m_formLayout->addRow(h4);
                    m_formLayout->addRow(h8);
                    m_formLayout->addRow(h10);
                    m_formLayout->addRow(h11);


                    offsetDisLabel->setVisible(true);
                    offsetDisEdit->setVisible(true);
                    offsetUnitsLabel->setVisible(true);
                    stacked->setCurrentIndex(0);
                    offsetMousePickBtn->setVisible(true);
                    offsetPickStartBtn->setVisible(false);
                    offsetCustomDis->setVisible(false);
                    stackWidget->setCurrentIndex(0);
                    for (int i = 0; i < m_foldWidgets.size(); i++)
                    {
                        _mainLayout->removeWidget(m_foldWidgets[i]);
                        m_foldWidgets[i]->hide();
                    }
                    m_FoldSections[0]->insertRow(0, pickObjTable);
                    m_FoldSections[0]->insertRow(0, geoPickWidget);
                    displayAfterInsertFolds({ 0,1 });
                }
                else if (index == 1)
                {
                    geoPickWidget->_featPick = None;
                    offsetSXLabel->setText("起始Vx");
                    offsetSYLabel->setText("起始Vy");
                    offsetSZLabel->setText("起始Vz");
                    offsetEXLabel->setText("终止Vx");
                    offsetEYLabel->setText("终止Vy");
                    offsetEZLabel->setText("终止Vz");

                    h1->setParent(nullptr);
                    h2->setParent(nullptr);
                    h3->setParent(nullptr);
                    h4->setParent(nullptr);
                    h5->setParent(nullptr);
                    h6->setParent(nullptr);
                    h7->setParent(nullptr);
                    h8->setParent(nullptr);
                    h9->setParent(nullptr);
                    h10->setParent(nullptr);
                    h11->setParent(nullptr);
                    m_formLayout2->addRow(h1);
                    m_formLayout2->addRow(h2);
                    m_formLayout2->addRow(h3);
                    m_formLayout2->addRow(h4);
                    m_formLayout2->addRow(h5);
                    m_formLayout2->addRow(h6);
                    m_formLayout2->addRow(h7);
                    m_formLayout2->addRow(h8);
                    m_formLayout2->addRow(h9);
                    m_formLayout2->addRow(h10);
                    m_formLayout2->addRow(h11);


                    offsetDisLabel->setVisible(false);
                    offsetDisEdit->setVisible(false);
                    offsetUnitsLabel->setVisible(false);
                    offsetMousePickBtn->setVisible(false);
                    offsetCustomDis->setVisible(true);
                    offsetPickStartBtn->setVisible(true);
                    offsetPickEndBtn->setVisible(true);
                    offsetCustomDis->setCheckState(Qt::Unchecked);
                    stackWidget->setCurrentIndex(1);
                    for (int i = 0; i < m_foldWidgets.size(); i++)
                    {
                        _mainLayout->removeWidget(m_foldWidgets[i]);
                        m_foldWidgets[i]->hide();
                    }
                    m_FoldSections[0]->insertRow(0, pickObjTable);
                    m_FoldSections[0]->insertRow(0, geoPickWidget);
                    displayAfterInsertFolds({ 0,1 });
                }
            });
    }

    //初始化定位操作界面
    void mGeoTransformPanel::initLocationFold()
    {
        locationMoveRadio = new QRadioButton("移动", this);
        locationMoveRadio->setChecked(true);
        locationCopyRadio = new QRadioButton("复制", this);
        locationRefTypeCombo = new QComboBox(this);
        locationRefTypeCombo->addItems({ "坐标系","几何点" });
        QLabel* locationRefTypeLabel = new QLabel("定位基准类型");

        coordSystemALabel = new PanelLabel("初始坐标系", this);
        coordSystemBLabel = new PanelLabel("目标坐标系", this);
        locationSourcePntALabel = new PanelLabel("初始几何点A", this);
        locationSourcePntBLabel = new PanelLabel("初始几何点B", this);
        locationSourcePntCLabel = new PanelLabel("初始几何点C", this);
        locationTargetPntALabel = new PanelLabel("目标几何点A", this);
        locationTargetPntBLabel = new PanelLabel("目标几何点B", this);
        locationTargetPntCLabel = new PanelLabel("目标几何点C", this);
        coordSystemA = new QLineEdit(this);
        coordSystemB = new QLineEdit(this);
        locationSourcePntA = new QLineEdit(this);
        locationSourcePntB = new QLineEdit(this);
        locationSourcePntC = new QLineEdit(this);
        locationTargetPntA = new QLineEdit(this);
        locationTargetPntB = new QLineEdit(this);
        locationTargetPntC = new QLineEdit(this);
        coordSystemA->setReadOnly(true);
        coordSystemB->setReadOnly(true);
        locationSourcePntA->setReadOnly(true);
        locationSourcePntB->setReadOnly(true);
        locationSourcePntC->setReadOnly(true);
        locationTargetPntA->setReadOnly(true);
        locationTargetPntB->setReadOnly(true);
        locationTargetPntC->setReadOnly(true);
        QToolButton* pickCoordSystemA = new QToolButton(this);
        pickCoordSystemA->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickCoordSystemA, &QToolButton::clicked, this, [this, pickCoordSystemA]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            //MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->strPickType = "坐标系";
            geoPickWidget->_featPick = LocationSourceCoordinate;
            if (coordSystemAId == 0 || coordSystemAId < -1)
            {
                //return;
            }
            else
            {
                HighlightSymbol(coordSystemAId, "coordinate");
            }
            HighlightSymbol(coordSystemAId, "coordinate");
            
            });

        QToolButton* pickCoordSystemB = new QToolButton(this);
        pickCoordSystemB->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickCoordSystemB, &QToolButton::clicked, this, [this, pickCoordSystemB]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            //MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->strPickType = "坐标系";
            geoPickWidget->_featPick = LocationTargetCoordinate;
            if (coordSystemBId == 0 || coordSystemBId < -1)
            {
                //return;
            }
            else
            {
                HighlightSymbol(coordSystemBId, "coordinate");
            }
            HighlightSymbol(coordSystemBId, "coordinate");
           
            });

        QToolButton* pickSourcePntA = new QToolButton(this);
        pickSourcePntA->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickSourcePntA, &QToolButton::clicked, this, [this, pickSourcePntA]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationSourcePntAId, "几何点");
            geoPickWidget->_featPick = LocationSourcePntA;
            });

        QToolButton* pickSourcePntB = new QToolButton(this);
        pickSourcePntB->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickSourcePntB, &QToolButton::clicked, this, [this, pickSourcePntB]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationSourcePntBId, "几何点");
            geoPickWidget->_featPick = LocationSourcePntB;
            });

        QToolButton* pickSourcePntC = new QToolButton(this);
        pickSourcePntC->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickSourcePntC, &QToolButton::clicked, this, [this, pickSourcePntC]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationSourcePntCId, "几何点");
            geoPickWidget->_featPick = LocationSourcePntC;
            });

        QToolButton* pickTargetPntA = new QToolButton(this);
        pickTargetPntA->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickTargetPntA, &QToolButton::clicked, this, [this, pickTargetPntA]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationTargetPntAId, "几何点");
            geoPickWidget->_featPick = LocationTargetPntA;
            });

        QToolButton* pickTargetPntB = new QToolButton(this);
        pickTargetPntB->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickTargetPntB, &QToolButton::clicked, this, [this, pickTargetPntB]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationTargetPntBId, "几何点");
            geoPickWidget->_featPick = LocationTargetPntB;
            });

        QToolButton* pickTargetPntC = new QToolButton(this);
        pickTargetPntC->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickTargetPntC, &QToolButton::clicked, this, [this, pickTargetPntC]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(0);
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(locationTargetPntCId, "几何点");
            geoPickWidget->_featPick = LocationTargetPntC;
            });


        QPushButton* spFaceByPntOkBtn = new QPushButton("确认", this);
        connect(spFaceByPntOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_locationAction);
        QPushButton* spFaceByPntCancelBtn = new QPushButton("取消", this);
        connect(spFaceByPntCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);




        FoldSection* offsetFold_1 = new FoldSection("几何定位", this);
        m_FoldSections.append(offsetFold_1);
        m_foldWidgets.append(offsetFold_1);



        FoldSection* offsetFold_2 = new FoldSection("定位基准", this);
        offsetFold_2->addRow(locationRefTypeLabel, locationRefTypeCombo);
        offsetFold_2->addRow(coordSystemALabel, coordSystemA, pickCoordSystemA);
        offsetFold_2->addRow(coordSystemBLabel, coordSystemB, pickCoordSystemB);
        offsetFold_2->addRow(locationSourcePntALabel, locationSourcePntA, pickSourcePntA);
        offsetFold_2->addRow(locationSourcePntBLabel, locationSourcePntB, pickSourcePntB);
        offsetFold_2->addRow(locationSourcePntCLabel, locationSourcePntC, pickSourcePntC);
        offsetFold_2->addRow(locationTargetPntALabel, locationTargetPntA, pickTargetPntA);
        offsetFold_2->addRow(locationTargetPntBLabel, locationTargetPntB, pickTargetPntB);
        offsetFold_2->addRow(locationTargetPntCLabel, locationTargetPntC, pickTargetPntC);
        offsetFold_2->addRow(new PanelLabel("类型"), locationMoveRadio, locationCopyRadio);


        QFrame* horizontalLine = createHorizontalLine(offsetFold_2);
        offsetFold_2->addRow(horizontalLine);

        offsetFold_2->addRow(spFaceByPntOkBtn, spFaceByPntCancelBtn, "Left");
        m_foldWidgets.append(offsetFold_2);


        connect(locationRefTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, offsetFold_2, pickSourcePntA, pickSourcePntB, pickSourcePntC, pickCoordSystemA, pickCoordSystemB,
            pickTargetPntA, pickTargetPntB, pickTargetPntC](int i) {
                if (i)
                {
                    locationSourcePntALabel->setVisible(true);
                    locationSourcePntA->setVisible(true);
                    pickSourcePntA->setVisible(true);
                    locationSourcePntBLabel->setVisible(true);
                    locationSourcePntB->setVisible(true);
                    pickSourcePntB->setVisible(true);
                    locationSourcePntCLabel->setVisible(true);
                    locationSourcePntC->setVisible(true);
                    pickSourcePntC->setVisible(true);
                    locationTargetPntALabel->setVisible(true);
                    locationTargetPntA->setVisible(true);
                    pickTargetPntA->setVisible(true);
                    locationTargetPntBLabel->setVisible(true);
                    locationTargetPntB->setVisible(true);
                    pickTargetPntB->setVisible(true);
                    locationTargetPntCLabel->setVisible(true);
                    locationTargetPntC->setVisible(true);
                    pickTargetPntC->setVisible(true);
                    coordSystemALabel->setVisible(false);
                    coordSystemA->setVisible(false);
                    pickCoordSystemA->setVisible(false);
                    coordSystemBLabel->setVisible(false);
                    coordSystemB->setVisible(false);
                    pickCoordSystemB->setVisible(false);
                    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    symbolController->setAllSymbolShowState(false);
                    symbolController->UpdateAllSymbol();
                    getCVM->getPreRenderWidget()->updatePaint();
                }

                else
                {
                    locationSourcePntALabel->setVisible(false);
                    locationSourcePntA->setVisible(false);
                    pickSourcePntA->setVisible(false);
                    locationSourcePntBLabel->setVisible(false);
                    locationSourcePntB->setVisible(false);
                    pickSourcePntB->setVisible(false);
                    locationSourcePntCLabel->setVisible(false);
                    locationSourcePntC->setVisible(false);
                    pickSourcePntC->setVisible(false);
                    locationTargetPntALabel->setVisible(false);
                    locationTargetPntA->setVisible(false);
                    pickTargetPntA->setVisible(false);
                    locationTargetPntBLabel->setVisible(false);
                    locationTargetPntB->setVisible(false);
                    pickTargetPntB->setVisible(false);
                    locationTargetPntCLabel->setVisible(false);
                    locationTargetPntC->setVisible(false);
                    pickTargetPntC->setVisible(false);
                    coordSystemALabel->setVisible(true);
                    coordSystemA->setVisible(true);
                    pickCoordSystemA->setVisible(true);
                    coordSystemBLabel->setVisible(true);
                    coordSystemB->setVisible(true);
                    pickCoordSystemB->setVisible(true);

                    vector<QString> showSymbol = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoorinateIDStr();
                    auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                    symbolController->setAllSymbolShowState(false);
                    symbolController->setSymbolShowState(showSymbol, true);
                    symbolController->UpdateAllSymbol();
                    getCVM->getPreRenderWidget()->updatePaint();
                }
                offsetFold_2->refreshLayout();
            });


        locationSourcePntALabel->setVisible(false);
        locationSourcePntA->setVisible(false);
        pickSourcePntA->setVisible(false);
        locationSourcePntBLabel->setVisible(false);
        locationSourcePntB->setVisible(false);
        pickSourcePntB->setVisible(false);
        locationSourcePntCLabel->setVisible(false);
        locationSourcePntC->setVisible(false);
        pickSourcePntC->setVisible(false);
        locationTargetPntALabel->setVisible(false);
        locationTargetPntA->setVisible(false);
        pickTargetPntA->setVisible(false);
        locationTargetPntBLabel->setVisible(false);
        locationTargetPntB->setVisible(false);
        pickTargetPntB->setVisible(false);
        locationTargetPntCLabel->setVisible(false);
        locationTargetPntC->setVisible(false);
        pickTargetPntC->setVisible(false);

    }

    //初始化旋转操作界面
    void mGeoTransformPanel::initRotateFold()
    {
        //初始化旋转所有控件
        QToolButton* rotateReverseBtn = new QToolButton(this);
        rotateReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        rotateReverseBtn->setToolTip("反向");
        connect(rotateReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(rotateVx, rotateVy, rotateVz);
            });

        rotateMoveRadio = new QRadioButton("移动", this);
        rotateMoveRadio->setChecked(true);
        rotateCopyRadio = new QRadioButton("复制", this);

        QToolButton* rotateAxisPickBtn = new QToolButton(this);
        rotateAxisPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        rotateAxisPickBtn->setToolTip("选择旋转轴");
        connect(rotateAxisPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(rotateLastAxisID, "几何线");
            geoPickWidget->_featPick = RotateAxis;
            });

        QToolButton* rotatePointPickBtn = new QToolButton(this);
        rotatePointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        rotatePointPickBtn->setToolTip("选择轴上一点");
        connect(rotatePointPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(rotateLastPntID, "几何点");
            geoPickWidget->_featPick = RotateAxisPnt;
            });

        rotateAngleEdit = new MxFunctions::mCalcLineEdit(); rotateAngleEdit->setExpression("0");
        rotateUnitsLabel = new QLabel("°");

		rotatePreviewChkBox = new QCheckBox("预览", this);
		rotatePreviewChkBox->setChecked(false);
		connect(rotatePreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoRotate);
			if (checked)
			{
				slot_rotatePreview();
			}
			});

        rotateCancelBtn = new QPushButton("取消", this);
        rotateOkBtn = new QPushButton("确认", this);

        rotateVx = new MxFunctions::mCalcLineEdit(this); rotateVx->setExpression("1");
        rotateVy = new MxFunctions::mCalcLineEdit(this); rotateVy->setExpression(""); rotateVy->setPlaceholderText("0");
        rotateVz = new MxFunctions::mCalcLineEdit(this); rotateVz->setExpression(""); rotateVz->setPlaceholderText("0");

        rotatePntX = new MxFunctions::mCalcLineEdit(this); rotatePntX->setExpression(""); rotatePntX->setPlaceholderText("0");
        rotatePntY = new MxFunctions::mCalcLineEdit(this); rotatePntY->setExpression(""); rotatePntY->setPlaceholderText("0");
        rotatePntZ = new MxFunctions::mCalcLineEdit(this); rotatePntZ->setExpression(""); rotatePntZ->setPlaceholderText("0");

        connect(rotateCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(rotateOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_rotateAction);
        // ========== 定义旋转折叠块 ==========
        FoldSection* rotateFold_1 = new FoldSection("旋转定义", this);
        m_FoldSections.append(rotateFold_1);
        m_foldWidgets.append(rotateFold_1);

        //// ========== 旋转轴折叠块 ==========
        FoldSection* rotateFold_2 = new FoldSection("旋转轴", this);

        //多行布局
        rotateFold_2->addRow(new PanelLabel("Vx"), rotateVx, rotateAxisPickBtn);
        rotateFold_2->addRow(new PanelLabel("Vy"), rotateVy, rotateReverseBtn);
        rotateFold_2->addRow(new PanelLabel("Vz"), rotateVz, 28);

        m_foldWidgets.append(rotateFold_2);

        // ========== 轴上一点折叠块 ==========
        FoldSection* rotateFold_3 = new FoldSection("轴上一点", this);
        //多行布局
        rotateFold_3->addRow(new PanelLabel("X"), rotatePntX, rotatePointPickBtn);
        rotateFold_3->addRow(new PanelLabel("Y"), rotatePntY, 28);
        rotateFold_3->addRow(new PanelLabel("Z"), rotatePntZ, 28);

        rotateFold_3->addRow(new PanelLabel("角度"), rotateAngleEdit, rotateUnitsLabel);
        rotateFold_3->addRow(new PanelLabel("类型"), rotateMoveRadio, rotateCopyRadio);
		rotateFold_3->addRow(rotatePreviewChkBox);

        QFrame* horizontalLine = createHorizontalLine(rotateFold_3);
        rotateFold_3->addRow(horizontalLine);

        rotateFold_3->addRow(rotateCancelBtn, rotateOkBtn, "Left");
        m_foldWidgets.append(rotateFold_3);
    }

    //初始化镜像操作界面
    void mGeoTransformPanel::initMirrorimageFold()
    {
        //初始化镜像所有控件
        mirrorimageMoveRadio = new QRadioButton("移动", this);
        mirrorimageMoveRadio->setChecked(true);
        mirrorimageCopyRadio = new QRadioButton("复制", this);

        QToolButton* mirrorReverseBtn = new QToolButton(this);
        mirrorReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        mirrorReverseBtn->setToolTip("反向");
        connect(mirrorReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(mirrorVx, mirrorVy, mirrorVz);
            });

        QToolButton* mirrorimageVecPickBtn = new QToolButton(this);
        mirrorimageVecPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        mirrorimageVecPickBtn->setToolTip("选择镜像平面");
        connect(mirrorimageVecPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
            Highlight(mirrorLastAxisID, "几何面");
            geoPickWidget->_featPick = MirrorVec;
            });

        QToolButton* mirrorimagePointPickBtn = new QToolButton(this);
        mirrorimagePointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        mirrorimagePointPickBtn->setToolTip("选择平面上一点");
        connect(mirrorimagePointPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(mirrorLastPntID, "几何点");
            geoPickWidget->_featPick = MirrorVecPnt;
            });

        mirrorimagePreviewChkBox = new QCheckBox("预览", this);
        mirrorimagePreviewChkBox->setChecked(false);
		connect(mirrorimagePreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoMirrorimage);
			if (checked)
			{
				slot_mirrorimagePreview();
			}
			});

        mirrorimageCancelBtn = new QPushButton("取消", this);
        mirrorimageOkBtn = new QPushButton("确认", this);

        mirrorVx = new MxFunctions::mCalcLineEdit(); mirrorVx->setExpression("1");
        mirrorVy = new MxFunctions::mCalcLineEdit(); mirrorVy->setExpression(""); mirrorVy->setPlaceholderText("0");
        mirrorVz = new MxFunctions::mCalcLineEdit(); mirrorVz->setExpression(""); mirrorVz->setPlaceholderText("0");

        mirrorimagePntX = new MxFunctions::mCalcLineEdit(this); mirrorimagePntX->setExpression(""); mirrorimagePntX->setPlaceholderText("0");
        mirrorimagePntY = new MxFunctions::mCalcLineEdit(this); mirrorimagePntY->setExpression(""); mirrorimagePntY->setPlaceholderText("0");
        mirrorimagePntZ = new MxFunctions::mCalcLineEdit(this); mirrorimagePntZ->setExpression(""); mirrorimagePntZ->setPlaceholderText("0");

        connect(mirrorimageCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(mirrorimageOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_mirrorimageAction);

        // ========== 定义镜像折叠块 ==========
        FoldSection* mirrorimageFold_1 = new FoldSection("镜像定义", this);
        m_FoldSections.append(mirrorimageFold_1);
        m_foldWidgets.append(mirrorimageFold_1);

        //// ========== 平面法向折叠块 ==========
        FoldSection* mirrorimageFold_2 = new FoldSection("平面法向", this);

        //多行布局
        mirrorimageFold_2->addRow(new PanelLabel("Vx"), mirrorVx, mirrorimageVecPickBtn);
        mirrorimageFold_2->addRow(new PanelLabel("Vy"), mirrorVy, mirrorReverseBtn);
        mirrorimageFold_2->addRow(new PanelLabel("Vz"), mirrorVz, 28);

        m_foldWidgets.append(mirrorimageFold_2);

        // ========== 平面上一点折叠块 ==========
        FoldSection* mirrorimageFold_3 = new FoldSection("平面上一点", this);
        //多行布局
        mirrorimageFold_3->addRow(new PanelLabel("X"), mirrorimagePntX, mirrorimagePointPickBtn);
        mirrorimageFold_3->addRow(new PanelLabel("Y"), mirrorimagePntY, 28);
        mirrorimageFold_3->addRow(new PanelLabel("Z"), mirrorimagePntZ, 28);

        mirrorimageFold_3->addRow(new PanelLabel("类型"), mirrorimageMoveRadio, mirrorimageCopyRadio);
        mirrorimageFold_3->addRow(mirrorimagePreviewChkBox);

        QFrame* horizontalLine = createHorizontalLine(mirrorimageFold_3);
        mirrorimageFold_3->addRow(horizontalLine);

        mirrorimageFold_3->addRow(mirrorimageCancelBtn, mirrorimageOkBtn, "Left");
        m_foldWidgets.append(mirrorimageFold_3);
    }

    //初始化缩放操作界面
    void mGeoTransformPanel::initZoomFold()
    {
        //初始化缩放所有控件
        zoomMoveRadio = new QRadioButton("移动", this);
        zoomMoveRadio->setChecked(true);
        zoomCopyRadio = new QRadioButton("复制", this);

        QToolButton* zoomPointPickBtn = new QToolButton(this);
        zoomPointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        zoomPointPickBtn->setToolTip("选择基准点");
        connect(zoomPointPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(zoomLastPntID, "几何点");
            geoPickWidget->_featPick = ZoomBasePnt;
            });

        QCheckBox* zoomEquelChkBox = new QCheckBox("等比缩放", this);
        zoomEquelChkBox->setChecked(false);
        zoomEqualChkBox = zoomEquelChkBox;

		zoomPreviewChkBox = new QCheckBox("预览", this);
		zoomPreviewChkBox->setChecked(false);
		connect(zoomPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoZoom);
			if (checked)
			{
				slot_zoomPreview();
			}
			});

        zoomCancelBtn = new QPushButton("取消", this);
        zoomOkBtn = new QPushButton("确认", this);

        zoomSx = new MxFunctions::mCalcLineEdit(this); zoomSx->setExpression(""); zoomSx->setPlaceholderText("1");
        zoomSy = new MxFunctions::mCalcLineEdit(this); zoomSy->setExpression(""); zoomSy->setPlaceholderText("1");
        zoomSz = new MxFunctions::mCalcLineEdit(this); zoomSz->setExpression(""); zoomSz->setPlaceholderText("1");

        zoomPntX = new MxFunctions::mCalcLineEdit(this); zoomPntX->setExpression(""); zoomPntX->setPlaceholderText("0");
        zoomPntY = new MxFunctions::mCalcLineEdit(this); zoomPntY->setExpression(""); zoomPntY->setPlaceholderText("0");
        zoomPntZ = new MxFunctions::mCalcLineEdit(this); zoomPntZ->setExpression(""); zoomPntZ->setPlaceholderText("0");

        connect(zoomCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(zoomOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_zoomAction);

        // ========== 定义缩放折叠块 ==========
        FoldSection* zoomFold_1 = new FoldSection("缩放定义", this);
        m_FoldSections.append(zoomFold_1);
        m_foldWidgets.append(zoomFold_1);
        _mainLayout->addWidget(zoomFold_1);

        //// ========== 基准点折叠块 ==========
        FoldSection* zoomFold_2 = new FoldSection("基准点", this);
        //多行布局
        zoomFold_2->addRow(new PanelLabel("X"), zoomPntX, zoomPointPickBtn);
        zoomFold_2->addRow(new PanelLabel("Y"), zoomPntY, 28);
        zoomFold_2->addRow(new PanelLabel("Z"), zoomPntZ, 28);

        m_foldWidgets.append(zoomFold_2);

        // ========== 缩放比例折叠块 ==========
        FoldSection* zoomFold_3 = new FoldSection("缩放比例", this);
        //多行布局
        zoomFold_3->addRow(new PanelLabel("X"), zoomSx);
        zoomFold_3->addRow(new PanelLabel("Y"), zoomSy);
        zoomFold_3->addRow(new PanelLabel("Z"), zoomSz);

        zoomFold_3->addRow(new PanelLabel("类型"), zoomMoveRadio, zoomCopyRadio, zoomEquelChkBox);
		zoomFold_3->addRow(zoomPreviewChkBox);

        QFrame* horizontalLine = createHorizontalLine(zoomFold_3);
        zoomFold_3->addRow(horizontalLine);

        zoomFold_3->addRow(zoomCancelBtn, zoomOkBtn, "Left");
        m_foldWidgets.append(zoomFold_3);
    }

    //初始化线性阵列操作界面
    void mGeoTransformPanel::initLineArrayFold()
    {
        //初始化线性阵列所有控件
        QToolButton* lineArrayReverseBtn = new QToolButton(this);
        lineArrayReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        lineArrayReverseBtn->setToolTip("反向");
        connect(lineArrayReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(lineArrayVx, lineArrayVy, lineArrayVz);
            });

        QToolButton* lineArrayMousePickBtn = new QToolButton(this);
        lineArrayMousePickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        lineArrayMousePickBtn->setToolTip("选择阵列方向");
        connect(lineArrayMousePickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(lineArrayLastDirID, "几何线");
            geoPickWidget->_featPick = LineArrayDirection;
            });

        lineArrayTypeCombo = new QComboBox(this);
        lineArrayTypeCombo->addItems(QStringList({ "总距离","间隔距离" }));
        lineArrayTypeCombo->setCurrentIndex(0);

        lineArrayCountEdit = new MxFunctions::mCalcLineEdit(mCalcLineEdit::CalcMode::StrictPositive, this); lineArrayCountEdit->setExpression("3");
        lineArrayDisEdit = new MxFunctions::mCalcLineEdit(mCalcLineEdit::CalcMode::StrictPositive, this); lineArrayDisEdit->setExpression("10");
        lineArrayUnitsLabel = new QLabel("mm");

		lineArrayPreviewChkBox = new QCheckBox("预览", this);
		lineArrayPreviewChkBox->setChecked(false);
		connect(lineArrayPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoLinearArray);
			if (checked)
			{
				slot_lineArrayPreview();
			}
			});

        lineArrayCancelBtn = new QPushButton("取消", this);
        lineArrayOkBtn = new QPushButton("确认", this);

        lineArrayVx = new MxFunctions::mCalcLineEdit(this); lineArrayVx->setExpression("1");
        lineArrayVy = new MxFunctions::mCalcLineEdit(this); lineArrayVy->setExpression(""); lineArrayVy->setPlaceholderText("0");
        lineArrayVz = new MxFunctions::mCalcLineEdit(this); lineArrayVz->setExpression(""); lineArrayVz->setPlaceholderText("0");

        connect(lineArrayCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(lineArrayOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_lineArrayAction);
        // ========== 定义线性阵列折叠块 ==========
        FoldSection* lineArrayFold_1 = new FoldSection("线性阵列定义", this);
        m_FoldSections.append(lineArrayFold_1);
        m_foldWidgets.append(lineArrayFold_1);

        // ========== 平移方向折叠块 ==========
        FoldSection* lineArrayFold_2 = new FoldSection("线性阵列方向和距离", this);
        //多行布局
        lineArrayFold_2->addRow(new PanelLabel("Vx"), lineArrayVx, lineArrayMousePickBtn);
        lineArrayFold_2->addRow(new PanelLabel("Vy"), lineArrayVy, lineArrayReverseBtn);
        lineArrayFold_2->addRow(new PanelLabel("Vz"), lineArrayVz);
        lineArrayFold_2->addRow(new PanelLabel("数量"), lineArrayCountEdit);
        lineArrayFold_2->addRow(new PanelLabel("数值"), lineArrayTypeCombo, lineArrayDisEdit, lineArrayUnitsLabel);
		lineArrayFold_2->addRow(lineArrayPreviewChkBox);

        QFrame* horizontalLine = createHorizontalLine(lineArrayFold_2);
        lineArrayFold_2->addRow(horizontalLine);

        lineArrayFold_2->addRow(lineArrayCancelBtn, lineArrayOkBtn, "Left");
        m_foldWidgets.append(lineArrayFold_2);
    }

    //初始化环形阵列操作界面
    void mGeoTransformPanel::initRingArrayFold()
    {
        //初始化环形阵列所有控件
        QToolButton* ringArrayReverseBtn = new QToolButton(this);
        ringArrayReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        ringArrayReverseBtn->setToolTip("反向");
        connect(ringArrayReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(ringArrayVx, ringArrayVy, ringArrayVz);
            });

        QToolButton* ringArrayAxisPickBtn = new QToolButton(this);
        ringArrayAxisPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        ringArrayAxisPickBtn->setToolTip("选择阵列旋转轴");
        connect(ringArrayAxisPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(ringArrayLastAxisID, "几何线");
            geoPickWidget->_featPick = RingArrayAxis;
            });

        QToolButton* ringArrayPointPickBtn = new QToolButton(this);
        ringArrayPointPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        ringArrayPointPickBtn->setToolTip("选择轴上一点");
        connect(ringArrayPointPickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
            Highlight(ringArrayLastPntID, "几何点");
            geoPickWidget->_featPick = RingArrayAxisPnt;
            });

        ringArrayTypeCombo = new QComboBox(this);
        ringArrayTypeCombo->addItems(QStringList({ "总角度","间隔角度" }));
        ringArrayTypeCombo->setCurrentIndex(0);

        ringArrayCountEdit = new MxFunctions::mCalcLineEdit(mCalcLineEdit::CalcMode::StrictPositive, this); ringArrayCountEdit->setExpression("4");
        ringArrayAngleEdit = new MxFunctions::mCalcLineEdit(this); ringArrayAngleEdit->setExpression("360");
        ringArrayUnitsLabel = new QLabel("°");

		ringArrayPreviewChkBox = new QCheckBox("预览", this);
		ringArrayPreviewChkBox->setChecked(false);
		connect(ringArrayPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked, GeoRingArray);
			if (checked)
			{
				slot_ringArrayPreview();
			}
			});

        ringArrayCancelBtn = new QPushButton("取消", this);
        ringArrayOkBtn = new QPushButton("确认", this);

        ringArrayVx = new MxFunctions::mCalcLineEdit(this); ringArrayVx->setExpression("1");
        ringArrayVy = new MxFunctions::mCalcLineEdit(this); ringArrayVy->setExpression(""); ringArrayVy->setPlaceholderText("0");
        ringArrayVz = new MxFunctions::mCalcLineEdit(this); ringArrayVz->setExpression(""); ringArrayVz->setPlaceholderText("0");

        ringArrayPntX = new MxFunctions::mCalcLineEdit(this); ringArrayPntX->setExpression(""); ringArrayPntX->setPlaceholderText("0");
        ringArrayPntY = new MxFunctions::mCalcLineEdit(this); ringArrayPntY->setExpression(""); ringArrayPntY->setPlaceholderText("0");
        ringArrayPntZ = new MxFunctions::mCalcLineEdit(this); ringArrayPntZ->setExpression(""); ringArrayPntZ->setPlaceholderText("0");

        connect(ringArrayCancelBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_onCancel);
        connect(ringArrayOkBtn, &QPushButton::clicked, this, &mGeoTransformPanel::slot_ringArrayAction);
        // ========== 定义环形阵列折叠块 ==========
        FoldSection* ringArrayFold_1 = new FoldSection("环形阵列定义", this);
        m_FoldSections.append(ringArrayFold_1);
        m_foldWidgets.append(ringArrayFold_1);

        //// ========== 旋转轴折叠块 ==========
        FoldSection* ringArrayFold_2 = new FoldSection("旋转轴", this);

        //多行布局
        ringArrayFold_2->addRow(new PanelLabel("Vx"), ringArrayVx, ringArrayAxisPickBtn);
        ringArrayFold_2->addRow(new PanelLabel("Vy"), ringArrayVy, ringArrayReverseBtn);
        ringArrayFold_2->addRow(new PanelLabel("Vz"), ringArrayVz);
        m_foldWidgets.append(ringArrayFold_2);

        // ========== 轴上一点折叠块 ==========
        FoldSection* ringArrayFold_3 = new FoldSection("轴上一点", this);
        //多行布局
        ringArrayFold_3->addRow(new PanelLabel("X"), ringArrayPntX, ringArrayPointPickBtn);
        ringArrayFold_3->addRow(new PanelLabel("Y"), ringArrayPntY, 28);
        ringArrayFold_3->addRow(new PanelLabel("Z"), ringArrayPntZ, 28);

        ringArrayFold_3->addRow(new PanelLabel("数量"), ringArrayCountEdit);
        ringArrayFold_3->addRow(new PanelLabel("数值"), ringArrayTypeCombo, ringArrayAngleEdit, ringArrayUnitsLabel);
		ringArrayFold_3->addRow(ringArrayPreviewChkBox);

        QFrame* horizontalLine = createHorizontalLine(ringArrayFold_3);
        ringArrayFold_3->addRow(horizontalLine);

        ringArrayFold_3->addRow(ringArrayCancelBtn, ringArrayOkBtn, "Left");
        m_foldWidgets.append(ringArrayFold_3);
    }

    //取消
    void mGeoTransformPanel::slot_onCancel()
    {
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (geoPickWidget->_featPick == LocationSourceCoordinate|| geoPickWidget->_featPick == LocationTargetCoordinate)
        {
            vector<QString> showSymbol = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoorinateIDStr();
            symbolController->setSymbolShowState(showSymbol, false);
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }
        //取消预览渲染并断开当前操作的预览信号、取消激活渲染工具
        connectPreView(false, currentFeature);
        clearPreviewRender();
        activateDirectGeoModelingTool(false);
        hideAllFold();
    }


    //执行平移操作
    void mGeoTransformPanel::slot_offsetAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }

            float dis;
            float mdir[3];
            if (offsetVec->currentIndex() == 0)
            {
                dis = getDefaultValue(Unit::Length, offsetDisEdit->text().toFloat());
                mdir[0] = offsetVx->text().toFloat();
                mdir[1] = offsetVy->text().toFloat();
                mdir[2] = offsetVz->text().toFloat();
            }
            else
            {

                mdir[0] = offsetEVx->text().toFloat() - offsetVx->text().toFloat();
                mdir[1] = offsetEVy->text().toFloat() - offsetVy->text().toFloat();
                mdir[2] = offsetEVz->text().toFloat() - offsetVz->text().toFloat();
                if (offsetCustomDis->isChecked())
                {
                    dis = getDefaultValue(Unit::Length, offsetDisEdit->text().toFloat());

                }
                else
                {
                    //【OCC迁移·§8零成本清理】gp_Pnt.Distance → QVector3D::distanceToPoint（去 OCC 依赖）
                    const QVector3D point1(offsetVx->text().toFloat(), offsetVy->text().toFloat(), offsetVz->text().toFloat());
                    const QVector3D point2(offsetEVx->text().toFloat(), offsetEVy->text().toFloat(), offsetEVz->text().toFloat());
                    dis = point1.distanceToPoint(point2) * offsetExEdit->text().toFloat();
                    //gp_Pnt point1(offsetVx->text().toFloat(), offsetVy->text().toFloat(), offsetVz->text().toFloat());
                    //gp_Pnt point2(offsetEVx->text().toFloat(), offsetEVy->text().toFloat(), offsetEVz->text().toFloat());
                    //dis = point1.Distance(point2) * offsetExEdit->text().toFloat();
                }
            }


            //发送平移请求
            EventDispatcher::mGeoTranslationRequest req;
            req.objtype = type;
            req.isCopy = offsetCopyRadio->isChecked();
            req.targets = targets;
            req.distance = dis;
            req.name = "Translation";
            req.direction[0] = mdir[0];
            req.direction[1] = mdir[1];
            req.direction[2] = mdir[2];


            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().translate(req);

            if (res.succeeded())
            {
                if (offsetCopyRadio->isChecked() && res.generatedObjects.size() > 0)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("平移操作执行失败！"));
            }

        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行定位操作
    void mGeoTransformPanel::slot_locationAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }

            //发送定位请求
            EventDispatcher::mGeoOperateOfPositionRequest req;

            if (locationRefTypeCombo->currentIndex() == 0)
            {
                if (coordSystemAId == -2 || coordSystemBId == -2)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("坐标系参数缺失!"));
                    return;
                }
                auto coorDataA = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordSystemAId);
                auto coorDataB = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coordSystemBId);
                QVector3D originA, originB;
                bool istrue; QString msg;
                QVector<QVector3D>xyzAxis = coorDataA->getXYZAxis(istrue, msg, originA);
                QVector<QVector3D>xyzBxis = coorDataB->getXYZAxis(istrue, msg, originB);

                req.originA = originA;
                req.originB = originB;
                req.xyzAxis = xyzAxis.toStdVector();
                req.xyzBxis = xyzBxis.toStdVector();
                req.positionType = EventDispatcher::mGeoPositionType::ByCsys;
            }
            else
            {
                if (locationSourcePntAId == -1 || locationSourcePntBId == -1 || locationSourcePntCId == -1
                    || locationTargetPntAId == -1 || locationTargetPntBId == -1 || locationTargetPntCId == -1)
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数缺失!"));
                    return;
                }

                req.tools.push_back(locationSourcePntAId);
                req.tools.push_back(locationSourcePntBId);
                req.tools.push_back(locationSourcePntCId);
                req.tools.push_back(locationTargetPntAId);
                req.tools.push_back(locationTargetPntBId);
                req.tools.push_back(locationTargetPntCId);
                req.positionType = EventDispatcher::mGeoPositionType::ByPoint;
            }
            req.objtype = type;
            req.isCopy = locationCopyRadio->isChecked();
            req.targets = targets;
            req.name = "OperateOfPosition";

            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfPosition(req);

            if (res.succeeded())
            {
                coordSystemA->setText("");
                coordSystemB->setText("");
                locationSourcePntA->setText("");
                locationSourcePntB->setText("");
                locationSourcePntC->setText("");
                locationTargetPntA->setText("");
                locationTargetPntB->setText("");
                locationTargetPntC->setText("");
                locationSourcePntAId =
                    locationSourcePntBId =
                    locationSourcePntCId =
                    locationTargetPntAId =
                    locationTargetPntBId =
                    locationTargetPntCId
                    = -1;
                coordSystemAId =
                    coordSystemBId = -2;
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);

                if (locationCopyRadio->isChecked() && res.generatedObjects.size() > 0)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("平移操作执行失败！"));
            }    
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行旋转操作
    void mGeoTransformPanel::slot_rotateAction()
    {
        try
        {
            //控件值判空提示
            if (pickObjTable->rowCount() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取旋转对象！"));
                return;
            }
            if (rotateVx->text().isNull())
            {
                rotateVx->setText("0");
            }
            if (rotateVy->text().isNull())
            {
                rotateVy->setText("0");
            }
            if (rotateVz->text().isNull())
            {
                rotateVz->setText("0");
            }
            if (rotatePntX->text().isNull())
            {
                rotatePntX->setText("0");
            }
            if (rotatePntY->text().isNull())
            {
                rotatePntY->setText("0");
            }
            if (rotatePntZ->text().isNull())
            {
                rotatePntZ->setText("0");
            }
            if (rotateAngleEdit->text().isNull())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入旋转角度值！"));
                return;
            }


            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }


            double angle = getDefaultValue(Unit::Angle, rotateAngleEdit->text().toDouble());
            double mdir[3] = { rotateVx->text().toDouble(),rotateVy->text().toDouble(),rotateVz->text().toDouble() };
            double mpnt[3] = { rotatePntX->text().toDouble(),rotatePntY->text().toDouble(),rotatePntZ->text().toDouble() };

            //发送旋转请求
            EventDispatcher::mGeoOperateOfRotateRequest req;
            req.objtype = type;
            req.isCopy = rotateCopyRadio->isChecked();
            req.targets = targets;
            req.angle = angle;
            req.name = "OperateOfRotate";
            req.direction[0] = mdir[0];
            req.direction[1] = mdir[1];
            req.direction[2] = mdir[2];
            req.modelLocation[0] = mpnt[0];
            req.modelLocation[1] = mpnt[1];
            req.modelLocation[2] = mpnt[2];


            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfRotate(req);

            if (res.succeeded())
            {
                if (rotateCopyRadio->isChecked() && res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("旋转操作执行失败！"));
            }

        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行镜像操作
    void mGeoTransformPanel::slot_mirrorimageAction()
    {
        try
        {
            //控件值判空提示
            if (pickObjTable->rowCount() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取镜像对象！"));
                return;
            }
            if (mirrorVx->text().isNull())
            {
                mirrorVx->setText("0");
            }
            if (mirrorVy->text().isNull())
            {
                mirrorVy->setText("0");
            }
            if (mirrorVz->text().isNull())
            {
                mirrorVz->setText("0");
            }
            if (mirrorimagePntX->text().isNull())
            {
                mirrorimagePntX->setText("0");
            }
            if (mirrorimagePntY->text().isNull())
            {
                mirrorimagePntY->setText("0");
            }
            if (mirrorimagePntZ->text().isNull())
            {
                mirrorimagePntZ->setText("0");
            }

            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }


            double mdir[3] = { mirrorVx->text().toDouble(),mirrorVy->text().toDouble(),mirrorVz->text().toDouble() };
            double mpnt[3] = { mirrorimagePntX->text().toDouble(),mirrorimagePntY->text().toDouble(),mirrorimagePntZ->text().toDouble() };

            //发送镜像请求
            EventDispatcher::mGeoOperateOfMirrorRequest req;
            req.objtype = type;
            req.isCopy = mirrorimageCopyRadio->isChecked();
            req.targets = targets;
            req.name = "OperateOfMirror";
            req.direction[0] = mdir[0];
            req.direction[1] = mdir[1];
            req.direction[2] = mdir[2];
            req.modelLocation[0] = mpnt[0];
            req.modelLocation[1] = mpnt[1];
            req.modelLocation[2] = mpnt[2];

            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfMirror(req);

            if (res.succeeded())
            {
                //【修复】复制判定误用旋转面板的 rotateCopyRadio（复制粘贴
                // 笔误）——镜像复制从不进 registerCreatedPart，副本未注册
                // 进文档树，退出面板后消失；改为本面板的 mirrorimageCopyRadio
                if (mirrorimageCopyRadio->isChecked() && res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("镜像操作执行失败！"));
            }

            /*mGeoCommandOperateOfMirror* mGeoRotate = mGeoCommandFactory::createOperateOfMirror();
            mGeoRotate->setGeoDataBase(getCVM->getModelData()->getGeoData());
            mGeoRotate->setDirection(mdir);
            mGeoRotate->setModelLocation(mpnt);
            mGeoRotate->setModelDataEditShape(mEditShapeList);
            mGeoRotate->isCopy(mirrorimageCopyRadio->isChecked());
            mGeoRotate->setModelName("OperateOfMirror");
            bool isCreate = mGeoRotate->handleCommand();
            int modelDataID = getCVM->getModelData()->getModelDataID();
            if (isCreate)
            {
                if (!mirrorimageCopyRadio->isChecked())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(mGeoRotate->getGeoDataSet()->getPartID(), modelDataID);
                }
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
            }*/


        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行缩放操作
    void mGeoTransformPanel::slot_zoomAction()
    {
        try
        {
            //控件值判空提示
            if (pickObjTable->rowCount() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取缩放对象！"));
                return;
            }
            if (zoomPntX->text().isNull())
            {
                zoomPntX->setText("0");
            }
            if (zoomPntY->text().isNull())
            {
                zoomPntY->setText("0");
            }
            if (zoomPntZ->text().isNull())
            {
                zoomPntZ->setText("0");
            }
            if (zoomSx->text().isNull())
            {
                zoomSx->setText("1");
            }
            if (zoomSy->text().isNull())
            {
                zoomSy->setText("1");
            }
            if (zoomSz->text().isNull())
            {
                zoomSz->setText("1");
            }


            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }

            double mpnt[3] = { zoomPntX->text().toDouble(),zoomPntY->text().toDouble(),zoomPntZ->text().toDouble() };
            double mscale[3] = { zoomSx->text().toDouble(),zoomSy->text().toDouble(),zoomSz->text().toDouble() };

            //发送缩放请求
            EventDispatcher::mGeoOperateOfScaleRequest req;
            req.objtype = type;
            req.isCopy = zoomCopyRadio->isChecked();
            req.targets = targets;
            req.name = "OperateOfScale";
            req.mScalePara[0] = mscale[0];
            req.mScalePara[1] = mscale[1];
            req.mScalePara[2] = mscale[2];
            req.modelLocation[0] = mpnt[0];
            req.modelLocation[1] = mpnt[1];
            req.modelLocation[2] = mpnt[2];

            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfScale(req);

            if (res.succeeded())
            {
                //【修复】复制判定误用旋转面板的 rotateCopyRadio（复制粘贴
                // 笔误）——缩放复制从不进 registerCreatedPart，副本未注册
                // 进文档树，退出面板后消失；改为本面板的 zoomCopyRadio
                if (zoomCopyRadio->isChecked() && res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("缩放操作执行失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行线性阵列操作
    void mGeoTransformPanel::slot_lineArrayAction()
    {
        try
        {
            //控件值判空提示
            if (pickObjTable->rowCount() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取线性阵列对象！"));
                return;
            }
            if (lineArrayVx->text().isNull())
            {
                lineArrayVx->setText("0");
            }
            if (lineArrayVy->text().isNull())
            {
                lineArrayVy->setText("0");
            }
            if (lineArrayVz->text().isNull())
            {
                lineArrayVz->setText("0");
            }

            if (lineArrayCountEdit->text().isNull())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入线性阵列数量值！"));
                return;
            }
            if (lineArrayDisEdit->text().isNull())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入线性阵列距离值！"));
                return;
            }

            if (lineArrayCountEdit->text().toDouble() > 10000)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("线性阵列数量不能大于10000！"));
                return;
            }

            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }

            double dis = getDefaultValue(Unit::Length, lineArrayDisEdit->text().toDouble());
            double count = lineArrayCountEdit->text().toDouble();
            double mdir[3] = { lineArrayVx->text().toDouble(),lineArrayVy->text().toDouble(),lineArrayVz->text().toDouble() };

            if (lineArrayTypeCombo->currentText() == "总距离")
            {
                dis = dis / (count - 1);
            }

            //发送线性阵列请求
            EventDispatcher::mGeoOperateOfArrayRequest req;
            req.objtype = type;
            req.targets = targets;
            req.name = "OperateOfArray";
            req.distance = dis;
            req._Num = count;
            req.direction[0] = mdir[0];
            req.direction[1] = mdir[1];
            req.direction[2] = mdir[2];
            req.type = EventDispatcher::mGeoArrayType::Line;

            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfArray(req);

            if (res.succeeded())
            {
                if (res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("线性阵列操作执行失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    //执行环形阵列操作
    void mGeoTransformPanel::slot_ringArrayAction()
    {
        try
        {
            //控件值判空提示
            if (pickObjTable->rowCount() == 0)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取环形阵列对象！"));
                return;
            }
            if (ringArrayVx->text().isNull())
            {
                ringArrayVx->setText("0");
            }
            if (ringArrayVy->text().isNull())
            {
                ringArrayVy->setText("0");
            }
            if (ringArrayVz->text().isNull())
            {
                ringArrayVz->setText("0");
            }
            if (ringArrayPntX->text().isNull())
            {
                ringArrayPntX->setText("0");
            }
            if (ringArrayPntY->text().isNull())
            {
                ringArrayPntY->setText("0");
            }
            if (ringArrayPntZ->text().isNull())
            {
                ringArrayPntZ->setText("0");
            }
            if (ringArrayCountEdit->text().isNull())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入环形阵列数量值！"));
                return;
            }
            if (ringArrayAngleEdit->text().isNull())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入环形阵列角度值！"));
                return;
            }

            if (ringArrayCountEdit->text().toDouble() > 10000)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("环形阵列数量不能大于10000！"));
                return;
            }

            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Vertex;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                type = GeoNeutralData::GN_ShapeType::Edge;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3)
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }

            double angle = getDefaultValue(Unit::Angle, ringArrayAngleEdit->text().toDouble());
            double count = ringArrayCountEdit->text().toDouble();
            double mdir[3] = { ringArrayVx->text().toDouble(),ringArrayVy->text().toDouble(),ringArrayVz->text().toDouble() };
            double mpnt[3] = { ringArrayPntX->text().toDouble(),ringArrayPntY->text().toDouble(),ringArrayPntZ->text().toDouble() };

            if (ringArrayTypeCombo->currentText() == "总角度")
            {
                angle = angle / count;
            }

            //发送环形阵列请求
            EventDispatcher::mGeoOperateOfArrayRequest req;
            req.objtype = type;
            req.targets = targets;
            req.name = "OperateOfArray";
            req.angle = angle * M_PI / 180.0;;
            req._Num = count;
            req.direction[0] = mdir[0];
            req.direction[1] = mdir[1];
            req.direction[2] = mdir[2];
            req.modelLocation[0] = mpnt[0];
            req.modelLocation[1] = mpnt[1];
            req.modelLocation[2] = mpnt[2];
            req.type = EventDispatcher::mGeoArrayType::Circle;

            EventDispatcher::mGeoOperationResult res = getGEOServer.transforms().OperateOfArray(req);

            if (res.succeeded())
            {
                if (res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                geoPickWidget->clearHighlight();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("环形阵列操作执行失败！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

	//面板隐藏时取消预览并取消激活渲染工具
	void mGeoTransformPanel::hideEvent(QHideEvent* event)
	{
		connectPreView(false, currentFeature);
		clearPreviewRender();
		activateDirectGeoModelingTool(false);
		mGeoPanelBase::hideEvent(event);
	}

	//激活/取消激活直接建模渲染工具
	void mGeoTransformPanel::activateDirectGeoModelingTool(bool active)
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

	//清除预览渲染
	void mGeoTransformPanel::clearPreviewRender()
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
		tool->deleteGeoFeatruePoint();
		tool->deleteGeoFeatrueLine();
		tool->deleteGeoFeatrueFace();
	}

	//当前操作的预览勾选框是否勾选
	bool mGeoTransformPanel::isPreviewChecked(GeoOperationType type) const
	{
		switch (type)
		{
		case GeoOffset:      return offsetPreviewChkBox && offsetPreviewChkBox->isChecked();
		case GeoRotate:      return rotatePreviewChkBox && rotatePreviewChkBox->isChecked();
		case GeoMirrorimage: return mirrorimagePreviewChkBox && mirrorimagePreviewChkBox->isChecked();
		case GeoZoom:        return zoomPreviewChkBox && zoomPreviewChkBox->isChecked();
		case GeoLinearArray: return lineArrayPreviewChkBox && lineArrayPreviewChkBox->isChecked();
		case GeoRingArray:   return ringArrayPreviewChkBox && ringArrayPreviewChkBox->isChecked();
		default:             return false;
		}
	}

	//连接/断开指定操作的预览信号
	void mGeoTransformPanel::connectPreView(bool conn, GeoOperationType type)
	{
		auto connectEdit = [this, conn](QLineEdit* edit, void (mGeoTransformPanel::* slot)()) {
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
		auto connectChk = [this, conn](QCheckBox* chk, void (mGeoTransformPanel::* slot)()) {
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
		auto connectTable = [this, conn](QTableWidget* table, void (mGeoTransformPanel::* slot)()) {
			if (!table)
			{
				return;
			}
			if (conn)
			{
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

		void (mGeoTransformPanel::* previewSlot)() = nullptr;
		switch (type)
		{
		case GeoOffset:      previewSlot = &mGeoTransformPanel::slot_offsetPreview;      break;
		case GeoRotate:      previewSlot = &mGeoTransformPanel::slot_rotatePreview;      break;
		case GeoMirrorimage: previewSlot = &mGeoTransformPanel::slot_mirrorimagePreview; break;
		case GeoZoom:        previewSlot = &mGeoTransformPanel::slot_zoomPreview;        break;
		case GeoLinearArray: previewSlot = &mGeoTransformPanel::slot_lineArrayPreview;   break;
		case GeoRingArray:   previewSlot = &mGeoTransformPanel::slot_ringArrayPreview;   break;
		default: return;
		}

		switch (type)
		{
		case GeoOffset:
			connectEdit(offsetVx, previewSlot);
			connectEdit(offsetVy, previewSlot);
			connectEdit(offsetVz, previewSlot);
			connectEdit(offsetEVx, previewSlot);
			connectEdit(offsetEVy, previewSlot);
			connectEdit(offsetEVz, previewSlot);
			connectEdit(offsetDisEdit, previewSlot);
			connectEdit(offsetExEdit, previewSlot);
			connectChk(offsetCustomDis, previewSlot);
			connectTable(pickObjTable, previewSlot);
			if (conn)
			{
				connect(offsetVec, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(offsetVec, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot);
			}
			break;
		case GeoRotate:
			connectEdit(rotateVx, previewSlot);
			connectEdit(rotateVy, previewSlot);
			connectEdit(rotateVz, previewSlot);
			connectEdit(rotatePntX, previewSlot);
			connectEdit(rotatePntY, previewSlot);
			connectEdit(rotatePntZ, previewSlot);
			connectEdit(rotateAngleEdit, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoMirrorimage:
			connectEdit(mirrorVx, previewSlot);
			connectEdit(mirrorVy, previewSlot);
			connectEdit(mirrorVz, previewSlot);
			connectEdit(mirrorimagePntX, previewSlot);
			connectEdit(mirrorimagePntY, previewSlot);
			connectEdit(mirrorimagePntZ, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoZoom:
			connectEdit(zoomPntX, previewSlot);
			connectEdit(zoomPntY, previewSlot);
			connectEdit(zoomPntZ, previewSlot);
			connectEdit(zoomSx, previewSlot);
			connectEdit(zoomSy, previewSlot);
			connectEdit(zoomSz, previewSlot);
			connectChk(zoomEqualChkBox, previewSlot);
			connectTable(pickObjTable, previewSlot);
			break;
		case GeoLinearArray:
			connectEdit(lineArrayVx, previewSlot);
			connectEdit(lineArrayVy, previewSlot);
			connectEdit(lineArrayVz, previewSlot);
			connectEdit(lineArrayCountEdit, previewSlot);
			connectEdit(lineArrayDisEdit, previewSlot);
			connectTable(pickObjTable, previewSlot);
			if (conn)
			{
				connect(lineArrayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(lineArrayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot);
			}
			break;
		case GeoRingArray:
			connectEdit(ringArrayVx, previewSlot);
			connectEdit(ringArrayVy, previewSlot);
			connectEdit(ringArrayVz, previewSlot);
			connectEdit(ringArrayPntX, previewSlot);
			connectEdit(ringArrayPntY, previewSlot);
			connectEdit(ringArrayPntZ, previewSlot);
			connectEdit(ringArrayCountEdit, previewSlot);
			connectEdit(ringArrayAngleEdit, previewSlot);
			connectTable(pickObjTable, previewSlot);
			if (conn)
			{
				connect(ringArrayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot, Qt::UniqueConnection);
			}
			else
			{
				disconnect(ringArrayTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, previewSlot);
			}
			break;
		default:
			break;
		}

		if (!conn)
		{
			clearPreviewRender();
		}
	}

	//把预览离散化结果渲染到视口（颜色取被操作部件内色，面/线/点一致）
	void mGeoTransformPanel::renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res)
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

		//预览颜色：按面所属部件着色（与 mGeoInfoPanel::slot_setPartColor 写入的用户色同源），
		//仅透明度调整。取不到部件时退回 res.partID，再退回默认米色。
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
				partFaceColors.insert(partID, QVector4D(used.red(), used.green(), used.blue(), 76));
				partLineColors.insert(partID, QVector3D(used.red(), used.green(), used.blue()));
			}
			return partFaceColors.value(partID);
		};
		auto lineColorOfPart = [&](int partID) -> QVector3D
		{
			faceColorOfPart(partID); // 确保缓存已填
			return partLineColors.value(partID);
		};
		// 面 30% 透明（w=76），线/点不透明；主部件色用于线/点及无 facePartIDs 时的回退。
		const QVector4D kPreviewFaceColor = faceColorOfPart(res.partID);
		// 线/点统一用不透明黑色，与部件色区分
		const QVector3D kPreviewLineColor(90.0f, 90.0f, 90.0f);
		const QVector3D kPreviewPointColor = kPreviewLineColor;

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

		if (getCVM->getPreRenderWidget())
		{
			getCVM->getPreRenderWidget()->updatePaint();
		}
	}

	//平移预览
	void mGeoTransformPanel::slot_offsetPreview()
	{
		if (!isPreviewChecked(GeoOffset) || GeoOffset != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Vertex;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3) type = GeoNeutralData::GN_ShapeType::Solid;

		float dis;
		float mdir[3];
		if (offsetVec->currentIndex() == 0)
		{
			dis = getDefaultValue(Unit::Length, offsetDisEdit->text().toFloat());
			mdir[0] = offsetVx->text().toFloat();
			mdir[1] = offsetVy->text().toFloat();
			mdir[2] = offsetVz->text().toFloat();
		}
		else
		{
			mdir[0] = offsetEVx->text().toFloat() - offsetVx->text().toFloat();
			mdir[1] = offsetEVy->text().toFloat() - offsetVy->text().toFloat();
			mdir[2] = offsetEVz->text().toFloat() - offsetVz->text().toFloat();
			if (offsetCustomDis->isChecked())
			{
				dis = getDefaultValue(Unit::Length, offsetDisEdit->text().toFloat());
			}
			else
			{
				//【对齐 xiaozhigang 推送·2026-10-09】原 gp_Pnt::Distance 链注释退役
				//（GUI 零 OCC：本文件 OCC include 已随 S10 摘除，gp_Pnt 无完整类型），
				// 以 std::sqrt 等价替换，数值语义不变
				//gp_Pnt point1(offsetVx->text().toFloat(), offsetVy->text().toFloat(), offsetVz->text().toFloat());
				//gp_Pnt point2(offsetEVx->text().toFloat(), offsetEVy->text().toFloat(), offsetEVz->text().toFloat());
				//dis = point1.Distance(point2) * offsetExEdit->text().toFloat();
				const float ddx = offsetEVx->text().toFloat() - offsetVx->text().toFloat();
				const float ddy = offsetEVy->text().toFloat() - offsetVy->text().toFloat();
				const float ddz = offsetEVz->text().toFloat() - offsetVz->text().toFloat();
				dis = std::sqrt(ddx * ddx + ddy * ddy + ddz * ddz) * offsetExEdit->text().toFloat();
			}
		}

		EventDispatcher::mGeoTranslationRequest req;
		req.objtype = type;
		req.isCopy = offsetCopyRadio->isChecked(); // 预览类型跟随界面"移动/复制"选择
		req.targets = targets;
		req.distance = dis;
		req.name = "TranslationPreview";
		req.direction[0] = mdir[0];
		req.direction[1] = mdir[1];
		req.direction[2] = mdir[2];

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().translatePreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//旋转预览
	void mGeoTransformPanel::slot_rotatePreview()
	{
		if (!isPreviewChecked(GeoRotate) || GeoRotate != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty() || rotateAngleEdit->text().isEmpty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Vertex;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3) type = GeoNeutralData::GN_ShapeType::Solid;

		EventDispatcher::mGeoOperateOfRotateRequest req;
		req.objtype = type;
		req.isCopy = rotateCopyRadio->isChecked();
		req.targets = targets;
		req.angle = getDefaultValue(Unit::Angle, rotateAngleEdit->text().toDouble());
		req.name = "RotatePreview";
		req.direction[0] = rotateVx->text().toDouble();
		req.direction[1] = rotateVy->text().toDouble();
		req.direction[2] = rotateVz->text().toDouble();
		req.modelLocation[0] = rotatePntX->text().toDouble();
		req.modelLocation[1] = rotatePntY->text().toDouble();
		req.modelLocation[2] = rotatePntZ->text().toDouble();

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().OperateOfRotatePreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//镜像预览
	void mGeoTransformPanel::slot_mirrorimagePreview()
	{
		if (!isPreviewChecked(GeoMirrorimage) || GeoMirrorimage != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Vertex;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3) type = GeoNeutralData::GN_ShapeType::Solid;

		EventDispatcher::mGeoOperateOfMirrorRequest req;
		req.objtype = type;
		req.isCopy = mirrorimageCopyRadio->isChecked();
		req.targets = targets;
		req.name = "MirrorPreview";
		req.direction[0] = mirrorVx->text().toDouble();
		req.direction[1] = mirrorVy->text().toDouble();
		req.direction[2] = mirrorVz->text().toDouble();
		req.modelLocation[0] = mirrorimagePntX->text().toDouble();
		req.modelLocation[1] = mirrorimagePntY->text().toDouble();
		req.modelLocation[2] = mirrorimagePntZ->text().toDouble();

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().OperateOfMirrorPreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//缩放预览
	void mGeoTransformPanel::slot_zoomPreview()
	{
		if (!isPreviewChecked(GeoZoom) || GeoZoom != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Solid;

		EventDispatcher::mGeoOperateOfScaleRequest req;
		req.objtype = type;
		//预览类型跟随界面"移动/复制"选择，与正式操作保持一致
		req.isCopy = zoomCopyRadio->isChecked();
		req.targets = targets;
		req.name = "ScalePreview";
		req.mScalePara[0] = zoomSx->text().toDouble();
		req.mScalePara[1] = zoomSy->text().toDouble();
		req.mScalePara[2] = zoomSz->text().toDouble();
		req.modelLocation[0] = zoomPntX->text().toDouble();
		req.modelLocation[1] = zoomPntY->text().toDouble();
		req.modelLocation[2] = zoomPntZ->text().toDouble();

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().OperateOfScalePreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//线性阵列预览
	void mGeoTransformPanel::slot_lineArrayPreview()
	{
		if (!isPreviewChecked(GeoLinearArray) || GeoLinearArray != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty() || lineArrayCountEdit->text().isEmpty() || lineArrayDisEdit->text().isEmpty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Vertex;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3) type = GeoNeutralData::GN_ShapeType::Solid;

		double dis = getDefaultValue(Unit::Length, lineArrayDisEdit->text().toDouble());
		double count = lineArrayCountEdit->text().toDouble();
		if (lineArrayTypeCombo->currentText() == "总距离")
		{
			dis = dis / (count - 1);
		}

		EventDispatcher::mGeoOperateOfArrayRequest req;
		req.objtype = type;
		req.targets = targets;
		req.name = "ArrayPreview";
		req.distance = dis;
		req._Num = count;
		req.direction[0] = lineArrayVx->text().toDouble();
		req.direction[1] = lineArrayVy->text().toDouble();
		req.direction[2] = lineArrayVz->text().toDouble();
		req.type = EventDispatcher::mGeoArrayType::Line;

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().OperateOfArrayPreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

	//环形阵列预览
	void mGeoTransformPanel::slot_ringArrayPreview()
	{
		if (!isPreviewChecked(GeoRingArray) || GeoRingArray != currentFeature)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		if (geoIds.empty() || ringArrayCountEdit->text().isEmpty() || ringArrayAngleEdit->text().isEmpty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Vertex;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)      type = GeoNeutralData::GN_ShapeType::Vertex;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1) type = GeoNeutralData::GN_ShapeType::Edge;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 2) type = GeoNeutralData::GN_ShapeType::Face;
		else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 3) type = GeoNeutralData::GN_ShapeType::Solid;

		double angle = getDefaultValue(Unit::Angle, ringArrayAngleEdit->text().toDouble());
		double count = ringArrayCountEdit->text().toDouble();
		if (ringArrayTypeCombo->currentText() == "总角度")
		{
			angle = angle / count;
		}

		EventDispatcher::mGeoOperateOfArrayRequest req;
		req.objtype = type;
		req.targets = targets;
		req.name = "ArrayPreview";
		req.angle = angle * M_PI / 180.0;
		req._Num = count;
		req.direction[0] = ringArrayVx->text().toDouble();
		req.direction[1] = ringArrayVy->text().toDouble();
		req.direction[2] = ringArrayVz->text().toDouble();
		req.modelLocation[0] = ringArrayPntX->text().toDouble();
		req.modelLocation[1] = ringArrayPntY->text().toDouble();
		req.modelLocation[2] = ringArrayPntZ->text().toDouble();
		req.type = EventDispatcher::mGeoArrayType::Circle;

		EventDispatcher::mGeoPreviewResult res = getGEOServer.transforms().OperateOfArrayPreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}
		renderPreviewResult(res);
	}

    void mGeoTransformPanel::slot_highlightGeoData()
    {
        QString type = "几何线";

        int num = 0;
        vector<int> res;
        std::vector<SymbolSelectInfoData> result;
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
        case mxr::PickFilter::PickSymbol:
        {
            result = getCVM->getPreRenderWidget()->getSelectionContainer()->getSymbolSelectData()->getSelectDataByType(SymbolSelectionInfoType::Symbol_All);
            if (result.size() == 0)
            {
                return;
            }
            QString name = result[result.size() - 1].name;
            QStringList nameList = name.split("_");
            res.push_back(nameList.last().toInt());
            QString symbolStr = nameList.first();
            if (symbolStr == "coordinate")
            {
                type = "坐标系";
            }
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

        //【崩溃修复·定位复制】定位基准点/坐标系拾取分支直接取 res.back()
        // 无空检查——复制确认后 clearPick 清空选中集，新部件渲染完成的
        // 异步高亮回调（appendOneGeoModelRenderFinished→树高亮→本槽）带
        // 空 res 进入这些分支即 res[-1] 越界崩溃；空选中时这些分支直接返回。
        //（FeatureObj 分支自带 rowCount 遍历、Offset/Rotate 等分支有
        //  size() 空检查，均不受影响）
        if (res.empty()
            && (geoPickWidget->_featPick == LocationSourceCoordinate
                || geoPickWidget->_featPick == LocationTargetCoordinate
                || geoPickWidget->_featPick == LocationSourcePntA
                || geoPickWidget->_featPick == LocationSourcePntB
                || geoPickWidget->_featPick == LocationSourcePntC
                || geoPickWidget->_featPick == LocationTargetPntA
                || geoPickWidget->_featPick == LocationTargetPntB
                || geoPickWidget->_featPick == LocationTargetPntC))
        {
            return;
        }

        if (geoPickWidget->_featPick == FeatureObj)//特征形状对象
        {
            for (int i = pickObjTable->rowCount() - 1; i >= 0; i--)
            {
                QString strId = pickObjTable->item(i, 0)->text();
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

            //拾取对象变化后刷新当前操作预览
            if (GeoOffset == currentFeature)           slot_offsetPreview();
            else if (GeoRotate == currentFeature)      slot_rotatePreview();
            else if (GeoMirrorimage == currentFeature) slot_mirrorimagePreview();
            else if (GeoZoom == currentFeature)        slot_zoomPreview();
            else if (GeoLinearArray == currentFeature) slot_lineArrayPreview();
            else if (GeoRingArray == currentFeature)   slot_ringArrayPreview();
        }
        else if (geoPickWidget->_featPick == OffsetDirection)
        {
            //几何线
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
            if (lines.size() <= 0)
            {
                setCoord(QVector3D{}, offsetVx, offsetVy, offsetVz);
                return;
            }
            else
            {
                setCoord(lines[lines.size() - 1].m_dir, offsetVx, offsetVy, offsetVz);
            }

            disHighlight(offsetLastDirID, "几何线");
            offsetLastDirID = lines[lines.size() - 1].id;

        }
        else if (geoPickWidget->_featPick == OffsetStartPnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, offsetVx, offsetVy, offsetVz);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, offsetVx, offsetVy, offsetVz);
            }

            disHighlight(offsetLastStartID, "几何点");
            offsetLastStartID = points[points.size() - 1].id;

        }
        else if (geoPickWidget->_featPick == OffsetEndPnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, offsetEVx, offsetEVy, offsetEVz);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, offsetEVx, offsetEVy, offsetEVz);
            }

            disHighlight(offsetLastEndID, "几何点");
            offsetLastEndID = points[points.size() - 1].id;

        }
        else if (geoPickWidget->_featPick == RotateAxis)
        {
            //几何线
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
            if (lines.size() <= 0)
            {
                setCoord(QVector3D{}, rotateVx, rotateVy, rotateVz);
                return;
            }
            else
            {
                setCoord(lines[lines.size() - 1].m_dir, rotateVx, rotateVy, rotateVz);
            }

            disHighlight(rotateLastAxisID, "几何线");
            rotateLastAxisID = lines[lines.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == RotateAxisPnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, rotatePntX, rotatePntY, rotatePntZ);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, rotatePntX, rotatePntY, rotatePntZ);
            }

            disHighlight(rotateLastPntID, "几何点");
            rotateLastPntID = points[points.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == MirrorVec)
        {
            //几何线
            auto faces = getGEOServer.geoCalculate().CalculateFaceAttr(res);
            if (faces.size() <= 0)
            {
                setCoord(QVector3D{}, mirrorVx, mirrorVy, mirrorVz);
                return;
            }
            else
            {
                setCoord(faces[faces.size() - 1].m_dir, mirrorVx, mirrorVy, mirrorVz);
            }

            disHighlight(mirrorLastAxisID, "几何面");
            mirrorLastAxisID = faces[faces.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == MirrorVecPnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, mirrorimagePntX, mirrorimagePntY, mirrorimagePntZ);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, mirrorimagePntX, mirrorimagePntY, mirrorimagePntZ);
            }

            disHighlight(mirrorLastPntID, "几何点");
            mirrorLastPntID = points[points.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == ZoomBasePnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, zoomPntX, zoomPntY, zoomPntZ);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, zoomPntX, zoomPntY, zoomPntZ);
            }

            disHighlight(zoomLastPntID, "几何点");
            zoomLastPntID = points[points.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == LineArrayDirection)
        {
            //几何线
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
            if (lines.size() <= 0)
            {
                setCoord(QVector3D{}, lineArrayVx, lineArrayVy, lineArrayVz);
                return;
            }
            else
            {
                setCoord(lines[lines.size() - 1].m_dir, lineArrayVx, lineArrayVy, lineArrayVz);
            }

            disHighlight(lineArrayLastDirID, "几何线");
            lineArrayLastDirID = lines[lines.size() - 1].id;

        }
        else if (geoPickWidget->_featPick == RingArrayAxis)
        {
            //几何线
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
            if (lines.size() <= 0)
            {
                setCoord(QVector3D{}, ringArrayVx, ringArrayVy, ringArrayVz);
                return;
            }
            else
            {
                setCoord(lines[lines.size() - 1].m_dir, ringArrayVx, ringArrayVy, ringArrayVz);
            }

            disHighlight(ringArrayLastAxisID, "几何线");
            ringArrayLastAxisID = lines[lines.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == RingArrayAxisPnt)
        {
            //几何点
            auto points = getGEOServer.geoCalculate().CalculateVertexAttr(res);
            if (points.size() <= 0)
            {
                setCoord(QVector3D{}, ringArrayPntX, ringArrayPntY, ringArrayPntZ);
                return;
            }
            else
            {
                setCoord(points[points.size() - 1].m_coor, ringArrayPntX, ringArrayPntY, ringArrayPntZ);
            }

            disHighlight(ringArrayLastPntID, "几何点");
            ringArrayLastPntID = points[points.size() - 1].id;
        }
        else if (geoPickWidget->_featPick == LocationSourceCoordinate)
        {
            if (coordSystemBId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(coordSystemBId));
                return;
            }
            if (result.size() == 1)
            {
                //点击重复不做任何处理
                if (coordSystemAId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (result.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlightSymbol(result.at(0));

            }
            coordSystemA->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            coordSystemAId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationTargetCoordinate;
        }
        else if (geoPickWidget->_featPick == LocationTargetCoordinate)
        {
            if (coordSystemAId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(coordSystemAId));
                return;
            }
            if (result.size() == 1)
            {
                //点击重复不做任何处理
                if (coordSystemBId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (result.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlightSymbol(result.at(0));

            }
            coordSystemB->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            coordSystemBId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationSourceCoordinate;

        }
        else if (geoPickWidget->_featPick == LocationSourcePntA)
        {
            if (locationSourcePntBId == res[res.size() - 1] || locationSourcePntCId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationSourcePntAId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationSourcePntA->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationSourcePntAId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationSourcePntB;

        }
        else if (geoPickWidget->_featPick == LocationSourcePntB)
        {
            if (locationSourcePntAId == res[res.size() - 1] || locationSourcePntCId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationSourcePntBId == res[res.size() - 1])
                {
                    return;
                }

            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationSourcePntB->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationSourcePntBId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationSourcePntC;

        }
        else if (geoPickWidget->_featPick == LocationSourcePntC)
        {
            if (locationSourcePntAId == res[res.size() - 1] || locationSourcePntBId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationSourcePntCId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationSourcePntC->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationSourcePntCId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationTargetPntA;

        }
        else if (geoPickWidget->_featPick == LocationTargetPntA)
        {
            if (locationTargetPntBId == res[res.size() - 1] || locationTargetPntCId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationTargetPntAId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationTargetPntA->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationTargetPntAId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationTargetPntB;

        }
        else if (geoPickWidget->_featPick == LocationTargetPntB)
        {
            if (locationTargetPntAId == res[res.size() - 1] || locationTargetPntCId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationTargetPntBId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationTargetPntB->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationTargetPntBId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationTargetPntC;

        }
        else if (geoPickWidget->_featPick == LocationTargetPntC)
        {
            if (locationTargetPntAId == res[res.size() - 1] || locationTargetPntBId == res[res.size() - 1])
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("坐标系%1已被拾取!").arg(res[res.size() - 1]));
                return;
            }
            if (res.size() == 1)
            {
                //点击重复不做任何处理
                if (locationTargetPntCId == res[res.size() - 1])
                {
                    return;
                }
            }
            else if (res.size() == 2)
            {
                //若不与其他重复则去除初始值，只保留当前值，若与其他重复则报错

                disHighlight(res.at(0), "几何点");

            }
            locationTargetPntC->setText(QString(type + ": %1").arg(res[res.size() - 1]));
            locationTargetPntCId = res[res.size() - 1];
            geoPickWidget->_featPick = LocationSourcePntA;

        }
    }

    void mGeoTransformPanel::slot_subWindowActivated()
    {
        if (conn_geo)
            disconnect(conn_geo);
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoTransformPanel::slot_highlightGeoData);
    }
}
