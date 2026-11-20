#include "mGeoEditPanel.h"
#include "UiMetrics.h"
#include "ViewportContainer.h"
#include <cmath> //【OCC摘除·S10/P0-1 终局2】M_PI 显式自含（原经 OCC 链头传递）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
#include "mMechGlobalSignals.h"
#include "mUiMenu.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
//#include "mGeoCommandSolidOfDragVecOCC.h" //【OCC摘除·S10/P0-1 终局2】退役头
#include "mGeoManager.h"
#include "mMechGlobalSignals.h"
#include "PickCommonFunction.h"
#include "mMeshData.h"
//#include "mGeoInfoCalculate.h" //【OCC摘除·S10/P0-1】死 include（S9 已退役，零使用）

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/GeometryDividerTool.h"
#include "mxr/view/HighlightController.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
//【OCC摘除·S10/P0-1 终局2】退役头批量下线（调用体在恒0门控/孤儿声明内）：
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandSurfaceEditOfSplitByGeoPoints.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandSurfaceEditOfSplitByLinesOCC.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandSolidEditOfSplitByLines.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandSolidEditOfSplitBySweepLines.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandCommFunctionOCC.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandOperateOfProject.h" //【OCC摘除·S10/P0-1 终局2】退役头
//#include "mGeoCommandSurfaceEditOfSplitByGeoPoints.h"
//#include "mGeoCommandSurfaceEditOfSplitByLinesOCC.h"
//#include "mGeoCommandSolidEditOfSplitByLines.h"
//#include //#include "mGeoCommandSolidOfSketchOCC.h" //【OCC摘除·S10/P0-1 终局2】退役头（调用体在恒0门控/孤儿声明内）.h"
//#include "mGeoCommandCommFunctionOCC.h"
//#include "mGeoCommandOperateOfProject.h"
//#include "mGeoSketchAlgorithm.h" //【OCC摘除·S10/P0-1 终局2】死 include（零使用；OCC 链头）

#include <QMenu>
#include <QHash>

#include <mViewportModelManager.h>
#include "mGeoManager.h"
#include "mModelData.h"
#include "mBrowserWindow.h"
#include "mTreeView.h"
#include "mMeshData.h"
//#include "mGeoCommandSolidOfSketchOCC.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "PreViewportWidget.h"
#include "InteractionCore/Controller/InteractionToolManager.h"
#include <InteractionCore/Tool/CreateSketch2DGeometryTool.h>
#include <InteractionCore/Tool/CreateSketchGeometryTool.h>
//#include <mOccProject.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用；OCC 链头）
#include <mGeoEditService.h>
//#include <mOccCancelCutFace.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用；OCC 链头）

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】
using namespace mxr;
using namespace MechCommon;
namespace MechMainWindow
{
    mGeoEditPanel::mGeoEditPanel(QWidget* parent)
        : mGeoPanelBase(parent)
    {
        geoPickWidget_2 = new GeoPickWidget(this);
        geoPickWidget_2->_featPick = None;

        connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
            if (geoPickWidget_2->pickLabel->text() == "绘制曲线")
            {
                closeDrawCurve();
            }
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            });

        connect(geoPickWidget_2->pickBtn, &QToolButton::clicked, this, [this]() {
            if (geoPickWidget_2->pickLabel->text() == "绘制曲线")
            {
                isPntMode = true;
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::GeometryDividerTool);
                getCVM->getPreViewportWidget()->interaction()->getToolManager()->
                    active<mxr::InteractionGeometryDividerTool>();
                connect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(), &mxr::SignalEmitter::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
                //connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
                //getCVM->getPreRenderWidget()->getStateMachineManager()->setGeoDividerStateEnabled(true);
                return;
            }
            geoPickWidget->_featPick = None;
            geoPickWidget_2->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
            geoPickWidget->strPickType = geoPickWidget_2->strPickType;
            setGeoHighlight(spPickTable, geoPickWidget_2->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = getPickType(splitTypeCombo->currentText());
            });


        spliTypeLabel = new QLabel("分割类型", this);
        splitTypeCombo = new QComboBox(this);
        connect(splitTypeCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
            showChildFold(splitTypeCombo->currentText());
            });

        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            if (index == 0 && GeoFeatureProjection == currentFeature)
            {
                geoPickWidget_2->pickObjTypeCombo->clear();
                geoPickWidget_2->pickObjTypeCombo->addItems({ "几何线","几何面" });
            }
            else if (index == 1 && GeoFeatureProjection == currentFeature)
            {
                geoPickWidget_2->pickObjTypeCombo->clear();
                geoPickWidget_2->pickObjTypeCombo->addItems({ "几何面" });
            }
            // 干涉检查功能下，仅当选择类型为"几何体"时显示"移除"按钮
            if (GeoInterferenceCheck == currentFeature && interferenceRemoveBtn)
            {
                interferenceRemoveBtn->setVisible(geoPickWidget->pickObjTypeCombo->currentText() == "几何体");
            }
            geoPickWidget->_featPick = FeatureObj;
            });

        spPickTable = new QTableWidget(this);
        spPickTable->setMaximumHeight(100);
        spPickTable->setMinimumHeight(80);
        spPickTable->verticalHeader()->setHidden(true);
        spPickTable->setColumnCount(1);
        QStringList horizontalHeader = { "几何ID" };
        spPickTable->setHorizontalHeaderLabels(horizontalHeader);
        spPickTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        spPickTable->horizontalHeader()->setHighlightSections(false);
        spPickTable->setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 10));
        spPickTable->verticalHeader()->setDefaultSectionSize(20);
        spPickTable->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
        spPickTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        subDelAction = new QAction("删除");

        connect(spPickTable, &QTableWidget::customContextMenuRequested, this, &mGeoEditPanel::slot_contextMenuTableData);

        //第一个折叠块为类型
        FoldSection* spTypeFold = new FoldSection("选择分割类型", this);
        spTypeFold->addRow(spliTypeLabel, splitTypeCombo);
        m_FoldSections.append(spTypeFold);
        m_foldWidgets.append(spTypeFold);
        _mainLayout->addWidget(spTypeFold);

        //按顺序初始化
        initSplitFaceFold();
        initSplitSolidFold();
        initMergeFold();
        initCancelTrimFaceFold();
        initRepairCracksFold();
        iniDeleteFold();
        initSplitLineFold();
        initLineLengthenFold();
        initFaceOffsetFold();
        initFeatureProjectionFold();
        initFaceExtendFold();
        initLineCombineFold();
        initCheckEdgeAndFaceFold();
        initInterferenceCheckFold();
        //微小实体检测折叠块必须最后追加：其showFold索引为27，
        //插在前面会使干涉检查(26)等既有索引失效
        initTinySolidFold();
        initFillSurfaceFold();
        //删除线折叠块同规则追加在末尾（索引29），插在前面会使既有索引失效
        initDeleteLineFold();

        //初始化Id
        projectionAxisId =
            spSolidSwpLastLineID =
            spFaceDtmLastLineID =
            spSolidDtmLastLineID =
            spFaceLastLineID =
            spFacePntAId =
            spFacePntBId =
            spLineAId =
            spLineBId = -1;

        _mainLayout->addStretch();


        //全局信号（拾取/单位制/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接

        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(spFaceDisUnit, spFaceDistance));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(spFaceSizeUnit, spFaceSize));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(spSolidDragDisUnit, spSolidDragDis));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(spSolidSizeUnit, spSolidSize));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(spSolidFaceExSizeUnit, spSolidFaceExSize));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(maxRepairFaceDisUnit, maxRepairFaceDis));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(minRepairFaceDisUnit, minRepairFaceDis));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(lengthSizeUnit, lengthSize));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(faceOffsetDisUnit, faceOffsetDis));


        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(shortEdgeMaxLenUnit, shortEdgeMaxLen));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(sharpEdgeMaxAngleUnit, sharpEdgeMaxAngle));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(smallFaceMaxAreaUnit, smallFaceMaxArea));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(smallFaceMaxWidthUnit, smallFaceMaxWidth));
    }

    void mGeoEditPanel::connectGeoSignals()
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

    void mGeoEditPanel::disconnectGeoSignals()
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

	mGeoEditPanel::~mGeoEditPanel()
	{

	}

	FeaturePick mGeoEditPanel::getPickType(QString type)
	{
		if (GeoCancelTrimFace == currentFeature)
		{
			return FeaturePick::ExternalCutEdge;
		}
		if (type.compare("几何点分割") == 0)
		{

		}
		if (type.compare("剪切线分割") == 0)
		{

		}
		if (type.compare("几何线分割") == 0)
		{
			return FeaturePick::SpFaceGeoLine;
		}
		if (type.compare("基准面分割") == 0 && GeoSplitFace == currentFeature)
		{

		}
		if (type.compare("曲线分割") == 0)
		{


		}
		if (type.compare("边界线分割") == 0)
		{
			return FeaturePick::SpSolidBndLine;
		}
		if (type.compare("扫掠线分割") == 0)
		{
			return FeaturePick::SpSolidSwLine;
		}
        if (type.compare("基准面分割") == 0 && GeoSplitSolid == currentFeature)
		{

		}
		if (type.compare("几何面分割") == 0)
		{
			return FeaturePick::SpSolidGeoFace;
		}
        if (GeoFeatureProjection == currentFeature)
        {
            return FeaturePick::ProjectionObj;
        }
        if (GeoFaceExtend == currentFeature)
        {
            return FeaturePick::ProjectionObj;
        }
		return FeaturePick::None;
	}

    void mGeoEditPanel::showFold(GeoOperationType type)
    {
        hideAllFold();

        spPickTable->setVisible(false);
        geoPickWidget_2->setVisible(false);
        pickObjTable->setVisible(true);
        //部分分支（开放边、微小实体等）会隐藏拾取行，切换功能时统一恢复，需要的分支会再自行隐藏
        geoPickWidget->setVisible(true);

        currentFeature = type;
        geoPickWidget->pickLabel->setText("拾取");
        geoPickWidget_2->pickLabel->setText("拾取");

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget_2->pickObjTypeCombo->clear();
        geoPickWidget->_featPick = FeatureObj;
        point3dCoords.clear();

        for (int i = spPickTable->rowCount() - 1; i >= 0; i--) {
            spPickTable->removeRow(i);
        }
        for (int i = pickObjTable->rowCount() - 1; i >= 0; i--) {
            pickObjTable->removeRow(i);
        }

        if (GeoSplitFace == type)
        {
            splitTypeCombo->clear();
            splitTypeCombo->addItems({ "几何点分割","剪切线分割","几何线分割","基准面分割" });
            splitTypeCombo->setCurrentIndex(0);

            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);

            displayAfterInsertFolds({ 0, 1 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoSplitSolid == type)
        {
            splitTypeCombo->clear();
            splitTypeCombo->addItems({
                "曲线分割","边界线分割","扫掠线分割","基准面分割","几何面分割"
                });
            splitTypeCombo->setCurrentIndex(0);

            m_FoldSections[5]->insertRow(0, geoPickWidget_2);
            m_FoldSections[5]->insertRow(0, pickObjTable);
            m_FoldSections[5]->insertRow(0, geoPickWidget);
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            geoPickWidget_2->setVisible(true);
            geoPickWidget_2->pickObjTypeCombo->addItem("几何线");
            geoPickWidget_2->pickLabel->setText("绘制曲线");

            displayAfterInsertFolds({ 0, 5 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoMergeLine == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            m_FoldSections[10]->insertRow(0, pickObjTable);
            m_FoldSections[10]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 10 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoMergeFace == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            m_FoldSections[11]->insertRow(0, pickObjTable);
            m_FoldSections[11]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 11 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoMergeSolid == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[12]->insertRow(0, pickObjTable);
            m_FoldSections[12]->insertRow(0, geoPickWidget);

            displayAfterInsertFolds({ 12 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoCancelTrimFace == type)
        {
            geoPickWidget->pickLabel->setText("外部剪裁边");
            geoPickWidget_2->pickLabel->setText("内部剪裁边");

            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            geoPickWidget_2->pickObjTypeCombo->addItem("几何线");

            m_FoldSections[13]->insertRow(0, spPickTable);
            m_FoldSections[13]->insertRow(0, geoPickWidget_2);
            m_FoldSections[13]->insertRow(0, pickObjTable);
            m_FoldSections[13]->insertRow(0, geoPickWidget);
            spPickTable->setVisible(true);
            geoPickWidget_2->setVisible(true);

            displayAfterInsertFolds({ 13 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoRepairCracks == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            m_FoldSections[14]->insertRow(0, pickObjTable);
            m_FoldSections[14]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 14 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoDeleteFace == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            m_FoldSections[15]->insertRow(0, pickObjTable);
            m_FoldSections[15]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 15 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoDeleteSolid == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[16]->insertRow(0, pickObjTable);
            m_FoldSections[16]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 16 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoDeleteLine == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            m_FoldSections[29]->insertRow(0, pickObjTable);
            m_FoldSections[29]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 29 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoSplitLine == type)
        {
            displayAfterInsertFolds({ 17 });
            pickObjTable->setVisible(false);
            geoPickWidget_2->setVisible(false);
            geoPickWidget->setVisible(false);
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            geoPickWidget->_featPick = SpLine;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoLineLengthen == type)
        {
            displayAfterInsertFolds({ 18 });
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoFaceOffset == type)
        {
            geoPickWidget->setVisible(false);
            displayAfterInsertFolds({ 19 });
            pickObjTable->setVisible(false);
            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoFeatureProjection == type)
        {
            geoPickWidget->pickLabel->setText("投影对象");
            geoPickWidget_2->pickLabel->setText("投影到");

            geoPickWidget->pickObjTypeCombo->clear();
            geoPickWidget->pickObjTypeCombo->addItems({ "几何点", "几何线" });
            geoPickWidget_2->pickObjTypeCombo->clear();
            geoPickWidget_2->pickObjTypeCombo->addItems({ "几何线" ,"几何面" });

            m_FoldSections[20]->insertRow(0, spPickTable);
            m_FoldSections[20]->insertRow(0, geoPickWidget_2);
            m_FoldSections[20]->insertRow(0, pickObjTable);
            m_FoldSections[20]->insertRow(0, geoPickWidget);
            spPickTable->setVisible(true);
            geoPickWidget_2->setVisible(true);

            displayAfterInsertFolds({ 20 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoFaceExtend == type)
        {
            geoPickWidget->pickLabel->setText("延伸面");
            geoPickWidget_2->pickLabel->setText("目标面");

            geoPickWidget->pickObjTypeCombo->addItem("几何面");
            geoPickWidget_2->pickObjTypeCombo->addItem("几何面");
            geoPickWidget->setVisible(false);
            m_FoldSections[21]->insertRow(0, spPickTable);
            m_FoldSections[21]->insertRow(0, geoPickWidget_2);
            m_FoldSections[21]->insertRow(0, pickObjTable);
            m_FoldSections[21]->insertRow(0, geoPickWidget);
            spPickTable->setVisible(true);
            geoPickWidget_2->setVisible(true);

            displayAfterInsertFolds({ 21 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoLineCombine == type)
        {
            geoPickWidget->setVisible(false);
            pickObjTable->setVisible(false);
            spPickTable->setVisible(false);
            displayAfterInsertFolds({ 22 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoCheckShortEdge == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[23]->insertRow(0, pickObjTable);
            m_FoldSections[23]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 23 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }

        if (GeoCheckSharpEdge == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[24]->insertRow(0, pickObjTable);
            m_FoldSections[24]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 24 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            slot_checkSharpEdge();
        }

        if (GeoCheckOpenEdge == type)
        {
            //开放边为纯拓扑检查：无参数、无拾取，直接检查全部部件并高亮
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
            slot_checkOpenEdge();
        }

        if (GeoCheckSmallFace == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何体");
            m_FoldSections[25]->insertRow(0, pickObjTable);
            m_FoldSections[25]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 25 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }
        if (GeoCheckTinySolid == type)
        {
            //微小实体检测遍历全部几何体，不需要拾取对象
            displayAfterInsertFolds({ 27 });
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
        }
        if (GeoInterferenceCheck == type)
        {
            displayAfterInsertFolds({ 26 });
            pickObjTable->setVisible(true);
            geoPickWidget->pickObjTypeCombo->clear();
            geoPickWidget->pickObjTypeCombo->addItems({ "几何面", "几何体" });
            m_FoldSections[26]->insertRow(0, pickObjTable);
            m_FoldSections[26]->insertRow(0, geoPickWidget);
            m_FoldSections[26]->refreshLayout();
            // 根据当前选择的拾取类型更新"移除"按钮显隐（仅"几何体"时显示）
            if (interferenceRemoveBtn)
            {
                interferenceRemoveBtn->setVisible(geoPickWidget->pickObjTypeCombo->currentText() == "几何体");
            }
        }
        if (GeoFillSurface == type)
        {
            geoPickWidget->pickObjTypeCombo->addItem("几何线");
            m_FoldSections[28]->insertRow(0, pickObjTable);
            m_FoldSections[28]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 28 });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
        }


    }

	void mGeoEditPanel::showChildFold(QString type)
	{
		hideAllFold();
		spPickTable->setVisible(false);
		geoPickWidget_2->setVisible(false);
		pickObjTable->setVisible(true);
		geoPickWidget->setVisible(true);

		geoPickWidget->pickLabel->setText("拾取");
		geoPickWidget_2->pickLabel->setText("拾取");
		geoPickWidget_2->pickObjTypeCombo->clear();
		geoPickWidget->_featPick = FeatureObj;

		closeDrawCurve();

		if (type != currentType)
		{
			for (int i = spPickTable->rowCount() - 1; i >= 0; i--) {
				spPickTable->removeRow(i);
			}
			for (int i = pickObjTable->rowCount() - 1; i >= 0; i--) {
				pickObjTable->removeRow(i);
			}
		}


		currentType = type;

		if (type.compare("几何点分割") == 0)
		{
			showFold(GeoSplitFace);
		}
		if (type.compare("剪切线分割") == 0)
		{
			m_FoldSections[2]->insertRow(0, geoPickWidget_2);
			m_FoldSections[2]->insertRow(0, pickObjTable);
			m_FoldSections[2]->insertRow(0, geoPickWidget);
			geoPickWidget_2->setVisible(true);
			geoPickWidget_2->pickLabel->setText("绘制曲线");
			geoPickWidget_2->pickObjTypeCombo->addItem("几何线");

			displayAfterInsertFolds({ 0, 2 });
		}
		if (type.compare("几何线分割") == 0)
		{
			m_FoldSections[3]->insertRow(0, spPickTable);
			m_FoldSections[3]->insertRow(0, geoPickWidget_2);
			m_FoldSections[3]->insertRow(0, pickObjTable);
			m_FoldSections[3]->insertRow(0, geoPickWidget);
			spPickTable->setVisible(true);
			geoPickWidget_2->setVisible(true);
			geoPickWidget_2->pickObjTypeCombo->addItem("几何线");

			displayAfterInsertFolds({ 0, 3 });
		}
		if (type.compare("基准面分割") == 0 && GeoSplitFace == currentFeature)
		{
			m_FoldSections[4]->insertRow(0, pickObjTable);
			m_FoldSections[4]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 0, 4 });
		}
		if (type.compare("曲线分割") == 0)
		{
			showFold(GeoSplitSolid);
		}
		if (type.compare("边界线分割") == 0)
		{
			m_FoldSections[6]->insertRow(0, spPickTable);
			m_FoldSections[6]->insertRow(0, geoPickWidget_2);
			m_FoldSections[6]->insertRow(0, pickObjTable);
			m_FoldSections[6]->insertRow(0, geoPickWidget);
			spPickTable->setVisible(true);
			geoPickWidget_2->setVisible(true);
			geoPickWidget_2->pickObjTypeCombo->addItem("几何线");

			displayAfterInsertFolds({ 0, 6 });
		}
		if (type.compare("扫掠线分割") == 0)
		{
			m_FoldSections[7]->insertRow(0, spPickTable);
			m_FoldSections[7]->insertRow(0, geoPickWidget_2);
			m_FoldSections[7]->insertRow(0, pickObjTable);
			m_FoldSections[7]->insertRow(0, geoPickWidget);
			spPickTable->setVisible(true);
			geoPickWidget_2->setVisible(true);
			geoPickWidget_2->pickObjTypeCombo->addItem("几何线");

			displayAfterInsertFolds({ 0, 7 });
		}
		if (type.compare("基准面分割") == 0 && GeoSplitSolid == currentFeature)
		{
			m_FoldSections[8]->insertRow(0, pickObjTable);
			m_FoldSections[8]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 0, 8 });
		}
		if (type.compare("几何面分割") == 0)
		{
			m_FoldSections[9]->insertRow(0, spPickTable);
			m_FoldSections[9]->insertRow(0, geoPickWidget_2);
			m_FoldSections[9]->insertRow(0, pickObjTable);
			m_FoldSections[9]->insertRow(0, geoPickWidget);
			spPickTable->setVisible(true);
			geoPickWidget_2->setVisible(true);
			geoPickWidget_2->pickObjTypeCombo->addItem("几何面");

			displayAfterInsertFolds({ 0, 9 });
		}
	}

	void mGeoEditPanel::closeDrawCurve()
	{
		if (!getCVM->getPreRenderWidget()) return;
		//getCVM->getPreRenderWidget()->getStateMachineManager()->setGeoDividerStateEnabled(false);
		point3dCoords.clear();
		//disconnect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
        disconnect(getCVM->getPreViewportWidget()->interaction()->getSignalEmitter(), &mxr::SignalEmitter::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
        if (isPntMode)
        {
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
            isPntMode = false;
        }

        //清空干涉结果的渲染
        if (!InterferName.isEmpty())
        {
            //几何体
            getCVM->getPreRenderWidget()->GetBeamSystem()->deleteSetSymbol(InterferName);

            //几何面
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            symbolController->deleteSymbol({ InterferName });

            InterferName = "";
        }



    }

	//初始化分割面界面控件
	void mGeoEditPanel::initSplitFaceFold()
	{
		//初始化所有控件
		QToolButton* spFaceMousePickBtn = new QToolButton(this);
		spFaceMousePickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(spFaceMousePickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(spFaceLastLineID, "几何线");
			geoPickWidget->_featPick = SpFaceCustomVec;
			});

		QToolButton* spFaceReverseBtn = new QToolButton(this);
		spFaceReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		connect(spFaceReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(spFaceCustomLineVec);
			});

		//---通过点
		spFacePntA = new QLineEdit(this);
		spFacePntB = new QLineEdit(this);
		spFacePntA->setReadOnly(true);
		spFacePntB->setReadOnly(true);

		QToolButton* pickA = new QToolButton(this);
		pickA->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(pickA, &QToolButton::clicked, this, [this, pickA]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
			Highlight(spFacePntAId, "几何点");
			geoPickWidget->_featPick = SpFacePntA;
			});

		QToolButton* pickB = new QToolButton(this);
		pickB->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(pickB, &QToolButton::clicked, this, [this, pickB]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
			Highlight(spFacePntBId, "几何点");
			geoPickWidget->_featPick = SpFacePntB;
			});


		QPushButton* spFaceByPntCancelBtn = new QPushButton("取消", this);
		//QPushButton* spFaceByPntOkBtn = new QPushButton("确认", this);
		connect(spFaceByPntCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		//connect(spFaceByPntOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitFaceByPnt);

		FoldSection* spFaceByPntFold = new FoldSection("几何点分割", this);
		spFaceByPntFold->addRow(new QLabel("几何点A"), spFacePntA, pickA);
		spFaceByPntFold->addRow(new QLabel("几何点B"), spFacePntB, pickB);
		QFrame* hLine1 = createHorizontalLine(spFaceByPntFold);
		spFaceByPntFold->addRow(hLine1);
		spFaceByPntFold->addRow(new QLabel(""), spFaceByPntCancelBtn, "Left");

		m_FoldSections.append(spFaceByPntFold);
		m_foldWidgets.append(spFaceByPntFold);

		//--通过剪切线
		spFaceLSmooth = new QCheckBox("分割线光滑", this);
		spFaceLSmooth->setChecked(true);
		spFaceLClose = new QCheckBox("闭合分割线", this);
		QPushButton* spFaceByCutLineCancelBtn = new QPushButton("取消", this);
		QPushButton* spFaceByCutLineOkBtn = new QPushButton("确认", this);
		connect(spFaceByCutLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spFaceByCutLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitFaceByCutLine);

		FoldSection* spFaceByCutLineFold = new FoldSection("剪切线分割", this);
		spFaceByCutLineFold->addRow(spFaceLSmooth, spFaceLClose);
		QFrame* hLine2 = createHorizontalLine(spFaceByCutLineFold);
		spFaceByCutLineFold->addRow(hLine2);
		spFaceByCutLineFold->addRow(spFaceByCutLineCancelBtn, spFaceByCutLineOkBtn, "Left");

		m_FoldSections.append(spFaceByCutLineFold);
		m_foldWidgets.append(spFaceByCutLineFold);

		//--通过几何线
        QLabel* customLabel = new QLabel("自定义向量", this);
		QLabel* distanceLabel = new QLabel("距离", this);
        spFaceDisUnit = new QLabel("mm", this);
		spFaceDir = new QComboBox(this);
		spFaceDir->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义","面法向" });
		connect(spFaceDir, QOverload<int>::of(&QComboBox::activated), [this, customLabel, spFaceMousePickBtn, spFaceReverseBtn](int index) {
			spFaceMousePickBtn->setVisible(false);
			spFaceReverseBtn->setVisible(false);
			if (index == 5) {
				geoPickWidget->_featPick = None;
				customLabel->setVisible(true);
				spFaceMousePickBtn->setVisible(true);
				spFaceReverseBtn->setVisible(true);
				spFaceCustomLineVec->setVisible(true);
                m_FoldSections[3]->refreshLayout();
            }
            else {
				customLabel->setVisible(false);
                spFaceCustomLineVec->setVisible(false);
            }
			});

		spFaceMode = new QComboBox(this);
		spFaceMode->addItems({ "完整切割","距离" });
		connect(spFaceMode, QOverload<int>::of(&QComboBox::activated), [this, distanceLabel](int index) {
			if (index == 1) {
				geoPickWidget->_featPick = None;
				distanceLabel->setVisible(true);
				spFaceDistance->setVisible(true);
                spFaceDisUnit->setVisible(true);
                m_FoldSections[3]->refreshLayout();
            }
			else {
				distanceLabel->setVisible(false);
				spFaceDistance->setVisible(false);
                spFaceDisUnit->setVisible(false);
            }
			});

		spFaceCustomLineVec = new QLineEdit(this);
		spFaceCustomLineVec->setPlaceholderText("1,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spFaceCustomLineVec);
		spFaceDistance = new MxFunctions::mCalcLineEdit(this); spFaceDistance->setExpression("10");
		QPushButton* spFaceByGeoLineCancelBtn = new QPushButton("取消", this);
		QPushButton* spFaceByGeoLineOkBtn = new QPushButton("确认", this);
		connect(spFaceByGeoLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spFaceByGeoLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitFaceByGeoLine);

		FoldSection* spFaceByGeoLineFold = new FoldSection("几何线分割", this);
		spFaceByGeoLineFold->addRow(new QLabel("分割方向"), spFaceDir, spFaceMousePickBtn);
		spFaceByGeoLineFold->addRow(customLabel, spFaceCustomLineVec, spFaceReverseBtn);
		spFaceByGeoLineFold->addRow(new QLabel("切割方式"), spFaceMode);
		spFaceByGeoLineFold->addRow(distanceLabel, spFaceDistance, spFaceDisUnit);
		QFrame* hLine3 = createHorizontalLine(spFaceByGeoLineFold);
		spFaceByGeoLineFold->addRow(hLine3);
		spFaceByGeoLineFold->addRow(spFaceByGeoLineCancelBtn, spFaceByGeoLineOkBtn, "Left");


		customLabel->setVisible(false);
		spFaceCustomLineVec->setVisible(false);
		spFaceMousePickBtn->setVisible(false);
		spFaceReverseBtn->setVisible(false);
		distanceLabel->setVisible(false);
		spFaceDistance->setVisible(false);
        spFaceDisUnit->setVisible(false);

		m_FoldSections.append(spFaceByGeoLineFold);
		m_foldWidgets.append(spFaceByGeoLineFold);

		//--通过基准面
		QToolButton* spFaceDtmPickBtn = new QToolButton(this);
		spFaceDtmPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(spFaceDtmPickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(spFaceDtmLastLineID, "几何线");
			geoPickWidget->_featPick = SpFaceDtmCustomVec;
			});

		QToolButton* spFaceDtmReverseBtn = new QToolButton(this);
		spFaceDtmReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		connect(spFaceDtmReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(spFaceCustomFaceVec);
			});

		spFaceDtmPickBtn->setVisible(false);
		spFaceDtmReverseBtn->setVisible(false);

		spFaceSize = new MxFunctions::mCalcLineEdit(this); spFaceSize->setExpression("10000.00");;
		spFaceStdPnt = new QLineEdit(this);
		spFaceStdPnt->setPlaceholderText("0,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spFaceStdPnt);
		spFaceCustomFaceVec = new QLineEdit(this);
		spFaceCustomFaceVec->setPlaceholderText("1,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spFaceCustomFaceVec);
        spFaceSizeUnit = new QLabel("mm", this);
		QLabel* customVecLabel = new QLabel("自定义向量", this);
		spFaceVector = new QComboBox(this);
        spFaceVector->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义" });
		connect(spFaceVector, QOverload<int>::of(&QComboBox::activated), [this, customVecLabel, spFaceDtmPickBtn, spFaceDtmReverseBtn](int index) {
			spFaceDtmPickBtn->setVisible(false);
			spFaceDtmReverseBtn->setVisible(false);
			if (index == 5) {
				geoPickWidget->_featPick = None;
				customVecLabel->setVisible(true);
				spFaceDtmPickBtn->setVisible(true);
				spFaceDtmReverseBtn->setVisible(true);
				spFaceCustomFaceVec->setVisible(true);
                m_FoldSections[4]->refreshLayout();
            }
			else {
				customVecLabel->setVisible(false);
                spFaceCustomFaceVec->setVisible(false);
            }
			});

		QPushButton* spFaceByStdFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* spFaceByStdFaceOkBtn = new QPushButton("确认", this);
		connect(spFaceByStdFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spFaceByStdFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitFaceByStdFace);

		FoldSection* spFaceByStdFaceFold = new FoldSection("基准面分割", this);
		spFaceByStdFaceFold->addRow(new QLabel("平面法向"), spFaceVector, spFaceDtmPickBtn);
		spFaceByStdFaceFold->addRow(customVecLabel, spFaceCustomFaceVec, spFaceDtmReverseBtn);
		spFaceByStdFaceFold->addRow(new QLabel("平面尺寸"), spFaceSize, spFaceSizeUnit);
		spFaceByStdFaceFold->addRow(new QLabel("基准点"), spFaceStdPnt);
		QFrame* hLine4 = createHorizontalLine(spFaceByStdFaceFold);
		spFaceByStdFaceFold->addRow(hLine4);
		spFaceByStdFaceFold->addRow(spFaceByStdFaceCancelBtn, spFaceByStdFaceOkBtn, "Left");

		customVecLabel->setVisible(false);
		spFaceCustomFaceVec->setVisible(false);

		m_FoldSections.append(spFaceByStdFaceFold);
		m_foldWidgets.append(spFaceByStdFaceFold);
	}

	//初始化分割体操作界面
	void mGeoEditPanel::initSplitSolidFold()
	{
		//--通过曲线分割
		spSolidLSmooth = new QCheckBox("分割线光滑", this);
		spSolidLSmooth->setChecked(true);
		spSolidLClose = new QCheckBox("闭合分割线", this);
		QPushButton* spSolidByCurveCancelBtn = new QPushButton("取消", this);
		QPushButton* spSolidByCurveOkBtn = new QPushButton("确认", this);
		connect(spSolidByCurveCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spSolidByCurveOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitSolidByCurve);

		FoldSection* spSolidByCurveFold = new FoldSection("曲线分割", this);
		spSolidByCurveFold->addRow(spSolidLSmooth, spSolidLClose);
		QFrame* hLine5 = createHorizontalLine(spSolidByCurveFold);
		spSolidByCurveFold->addRow(hLine5);
		spSolidByCurveFold->addRow(spSolidByCurveCancelBtn, spSolidByCurveOkBtn, "Left");

		m_FoldSections.append(spSolidByCurveFold);
		m_foldWidgets.append(spSolidByCurveFold);


		//--通过边界线分割
		QLabel* expLine = new QLabel("扩大面系数");
		QLabel* expNumLine = new QLabel("扩大系数");
		spSolidFaceExpand = new QComboBox(this);
		spSolidFaceExpand->addItems({ "自动","手动" });
		connect(spSolidFaceExpand, QOverload<int>::of(&QComboBox::activated), [this, expNumLine](int index) {
			if (index == 1) {
				geoPickWidget->_featPick = None;
				expNumLine->setVisible(true);
				spSolidFaceExNum->setVisible(true);
                showChildFold("边界线分割");
            }
			else {
				expNumLine->setVisible(false);
                spSolidFaceExNum->setVisible(false);
            }
			});

		spSolidFaceExNum = new MxFunctions::mCalcLineEdit(this); spSolidFaceExNum->setExpression("2");
        spSolidExpFace = new QCheckBox("扩大面", this);
		connect(spSolidExpFace, &QCheckBox::stateChanged, this, [this, expLine]() {
			if (spSolidExpFace->checkState()) {
				expLine->setVisible(true);
				spSolidFaceExpand->setVisible(true);
			}
			else {
				expLine->setVisible(false);
				spSolidFaceExpand->setVisible(false);
			}
			});

		expLine->setVisible(false);
		spSolidFaceExpand->setVisible(false);

		QPushButton* spSolidByBdLineCancelBtn = new QPushButton("取消", this);
		QPushButton* spSolidByBdLineOkBtn = new QPushButton("确认", this);
		connect(spSolidByBdLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spSolidByBdLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitSolidByBndLine);

		FoldSection* spSolidByBdLineFold = new FoldSection("边界线分割", this);
		spSolidByBdLineFold->addRow(expLine, spSolidFaceExpand);
		spSolidByBdLineFold->addRow(expNumLine, spSolidFaceExNum);
		spSolidByBdLineFold->addRow(spSolidExpFace);
		QFrame* hLine6 = createHorizontalLine(spSolidByBdLineFold);
		spSolidByBdLineFold->addRow(hLine6);
		spSolidByBdLineFold->addRow(spSolidByBdLineCancelBtn, spSolidByBdLineOkBtn, "Left");

		expNumLine->setVisible(false);
		spSolidFaceExNum->setVisible(false);

		m_FoldSections.append(spSolidByBdLineFold);
		m_foldWidgets.append(spSolidByBdLineFold);

		//--通过扫掠线分割
		QToolButton* spSolidSwpPickBtn = new QToolButton(this);
		spSolidSwpPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(spSolidSwpPickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(spSolidSwpLastLineID, "几何线");
			geoPickWidget->_featPick = SpSolidSweepCustomVec;
			});

		QToolButton* spSolidSwpReverseBtn = new QToolButton(this);
		spSolidSwpReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		connect(spSolidSwpReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(spSolidCustomVec);
			});
		spSolidSwpPickBtn->setVisible(false);
		spSolidSwpReverseBtn->setVisible(false);

		QLabel* customLabel = new QLabel("自定义向量", this);
		QLabel* distanceLabel = new QLabel("距离", this);
		spSolidDragDir = new QComboBox(this);
        spSolidDragDir->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义" });
        connect(spSolidDragDir, QOverload<int>::of(&QComboBox::activated),
			[this, customLabel, spSolidSwpPickBtn, spSolidSwpReverseBtn](int index) {
			if (index == 5) {
				geoPickWidget->_featPick = None;
				customLabel->setVisible(true);
				spSolidCustomVec->setVisible(true);
				spSolidSwpPickBtn->setVisible(true);
				spSolidSwpReverseBtn->setVisible(true);
                    showChildFold("扫掠线分割");
                }
			else {
				spSolidSwpPickBtn->setVisible(false);
				spSolidSwpReverseBtn->setVisible(false);
				customLabel->setVisible(false);
                    spSolidCustomVec->setVisible(false);
                }
			});

		spSolidDragMode = new QComboBox(this);
		spSolidDragMode->addItems({ "完整切割","距离" });
		connect(spSolidDragMode, QOverload<int>::of(&QComboBox::activated), [this, distanceLabel](int index) {
			if (index == 1) {
				geoPickWidget->_featPick = None;
				distanceLabel->setVisible(true);
				spSolidDragDis->setVisible(true);
                spSolidDragDisUnit->setVisible(true);
                showChildFold("扫掠线分割");
            }
			else {
				distanceLabel->setVisible(false);
				spSolidDragDis->setVisible(false);
                spSolidDragDisUnit->setVisible(false);
            }
			});

		spSolidCustomVec = new QLineEdit(this);
		spSolidCustomVec->setPlaceholderText("1,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spSolidCustomVec);
		spSolidDragDis = new MxFunctions::mCalcLineEdit(this); spSolidDragDis->setExpression("10");
        spSolidDragDisUnit = new QLabel("mm", this);
		QPushButton* spSolidBySwLineCancelBtn = new QPushButton("取消", this);
		QPushButton* spSolidBySwLineOkBtn = new QPushButton("确认", this);
		connect(spSolidBySwLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spSolidBySwLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitSolidBySwLine);

		FoldSection* spSolidBySwLineFold = new FoldSection("扫掠线分割", this);
		spSolidBySwLineFold->addRow(new QLabel("拉伸方向"), spSolidDragDir, spSolidSwpPickBtn);
		spSolidBySwLineFold->addRow(customLabel, spSolidCustomVec, spSolidSwpReverseBtn);
		spSolidBySwLineFold->addRow(new QLabel("拉伸方式"), spSolidDragMode);
		spSolidBySwLineFold->addRow(distanceLabel, spSolidDragDis, spSolidDragDisUnit);
		QFrame* hLine7 = createHorizontalLine(spSolidBySwLineFold);
		spSolidBySwLineFold->addRow(hLine7);
		spSolidBySwLineFold->addRow(spSolidBySwLineCancelBtn, spSolidBySwLineOkBtn, "Left");

		customLabel->setVisible(false);
		spSolidCustomVec->setVisible(false);
		distanceLabel->setVisible(false);
		spSolidDragDis->setVisible(false);
        spSolidDragDisUnit->setVisible(false);

		m_FoldSections.append(spSolidBySwLineFold);
		m_foldWidgets.append(spSolidBySwLineFold);

		//--通过基准面分割
		QToolButton* spSolidDtmPickBtn = new QToolButton(this);
		spSolidDtmPickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
		connect(spSolidDtmPickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->clearHighlight();
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			Highlight(spSolidSwpLastLineID, "几何线");
			geoPickWidget->_featPick = SpSolidDatumCustomVec;
			});

		QToolButton* spSolidDtmReverseBtn = new QToolButton(this);
		spSolidDtmReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
		connect(spSolidDtmReverseBtn, &QToolButton::clicked, this, [this]() {
			reverseVecAction(spSolidCustomFaceVec);
			});
		spSolidDtmPickBtn->setVisible(false);
		spSolidDtmReverseBtn->setVisible(false);

		spSolidSize = new MxFunctions::mCalcLineEdit(this); spSolidSize->setExpression("10000.00");
        spSolidSizeUnit = new QLabel("mm", this);
		spSolidStdPnt = new QLineEdit(this);
		spSolidStdPnt->setPlaceholderText("0,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spSolidStdPnt);
		spSolidCustomFaceVec = new QLineEdit(this);
		spSolidCustomFaceVec->setPlaceholderText("1,0,0");
		MxFunctions::mEditDiagnosis::setOnlyVec3(spSolidCustomFaceVec);
		//QLabel* customVecLabel = new QLabel("自定义向量", this);
		spSolidVector = new QComboBox(this);
		spSolidVector->addItems({ "X轴","Y轴","Z轴","垂直于屏幕","平行于屏幕","自定义" });

		QPushButton* spSolidByStdFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* spSolidByStdFaceOkBtn = new QPushButton("确认", this);
		connect(spSolidByStdFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spSolidByStdFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitSolidByStdFace);

		FoldSection* spSolidByStdFaceFold = new FoldSection("基准面分割", this);
        QFrame* hLine8 = createHorizontalLine(spSolidByStdFaceFold);
        QStackedWidget* stackWidget = new QStackedWidget();
        QWidget* widget1 = new QWidget;
        QWidget* widget2 = new QWidget;
        stackWidget->addWidget(widget1);
        stackWidget->addWidget(widget2);
        stackWidget->setCurrentIndex(0);
        spSolidByStdFaceFold->addRow(stackWidget);

        QFormLayout* m_formLayout = new QFormLayout(widget1);
        m_formLayout->setContentsMargins(0, 0, 0, 0);
        m_formLayout->setSpacing(10);
        QFormLayout* m_formLayout2 = new QFormLayout(widget2);
        m_formLayout2->setContentsMargins(0, 0, 0, 0);
        m_formLayout2->setSpacing(10);

        QHBoxLayout* h1 = new QHBoxLayout();
        h1->addWidget(new QLabel("平面法向")); h1->addWidget(spSolidVector); h1->addWidget(spSolidDtmPickBtn);

        QHBoxLayout* h2 = new QHBoxLayout();
        h2->addWidget(new QLabel("自定义向量")); h2->addWidget(spSolidCustomFaceVec); h2->addWidget(spSolidDtmReverseBtn);

        QHBoxLayout* h3 = new QHBoxLayout();
        h3->addWidget(new QLabel("平面尺寸")); h3->addWidget(spSolidSize); h3->addWidget(spSolidSizeUnit);

        QHBoxLayout* h4 = new QHBoxLayout();
        h4->addWidget(new QLabel("基准点")); h4->addWidget(spSolidStdPnt);

        m_formLayout->addRow(h1);
        m_formLayout2->addRow(h2);
        m_formLayout->addRow(h3);
        m_formLayout->addRow(h4);

		spSolidByStdFaceFold->addRow(hLine8);
		spSolidByStdFaceFold->addRow(spSolidByStdFaceCancelBtn, spSolidByStdFaceOkBtn, "Left");

		m_FoldSections.append(spSolidByStdFaceFold);
		m_foldWidgets.append(spSolidByStdFaceFold);

        connect(spSolidVector, QOverload<int>::of(&QComboBox::activated), [this, m_formLayout, m_formLayout2,
            h1, h2, h3, h4, stackWidget](int index) {
            if (index == 5) {
                geoPickWidget->_featPick = None;
                h1->setParent(nullptr);
                h2->setParent(nullptr);
                h3->setParent(nullptr);
                h4->setParent(nullptr);
                m_formLayout2->addRow(h1);
                m_formLayout2->addRow(h2);
                m_formLayout2->addRow(h3);
                m_formLayout2->addRow(h4);
                stackWidget->setCurrentIndex(1);
                showChildFold("基准面分割");
            }
            else {
                h1->setParent(nullptr);
                h3->setParent(nullptr);
                h4->setParent(nullptr);
                m_formLayout->addRow(h1);
                m_formLayout->addRow(h3);
                m_formLayout->addRow(h4);
                stackWidget->setCurrentIndex(0);
                showChildFold("基准面分割");
            }
            });

		//--通过几何面分割
		spSolidFaceExSize = new MxFunctions::mCalcLineEdit(this); spSolidFaceExSize->setExpression("10000.00");
        spSolidFaceExSizeUnit = new QLabel("mm", this);
		QPushButton* spSolidByGeoFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* spSolidByGeoFaceOkBtn = new QPushButton("确认", this);
		connect(spSolidByGeoFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spSolidByGeoFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitSolidByGeoFace);

		FoldSection* spSolidByGeoFaceFold = new FoldSection("几何面分割", this);
		spSolidByGeoFaceFold->addRow(new QLabel("面尺寸扩大值"), spSolidFaceExSize, spSolidFaceExSizeUnit);
		QFrame* hLine9 = createHorizontalLine(spSolidByGeoFaceFold);
		spSolidByGeoFaceFold->addRow(hLine9);
		spSolidByGeoFaceFold->addRow(spSolidByGeoFaceCancelBtn, spSolidByGeoFaceOkBtn, "Left");

		m_FoldSections.append(spSolidByGeoFaceFold);
		m_foldWidgets.append(spSolidByGeoFaceFold);
	}

	//初始化合并界面
	void MechMainWindow::mGeoEditPanel::initMergeFold()
	{
		//--合并线
		QPushButton* mergeLineCancelBtn = new QPushButton("取消", this);
		QPushButton* mergeLineOkBtn = new QPushButton("确认", this);
		connect(mergeLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(mergeLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_mergeAction);
		FoldSection* mergeLineFold = new FoldSection("合并线", this);
		mergeLineFold->addRow(mergeLineCancelBtn, mergeLineOkBtn, "Left");

		m_FoldSections.append(mergeLineFold);
		m_foldWidgets.append(mergeLineFold);

		//--合并面
		QPushButton* mergeFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* mergeFaceOkBtn = new QPushButton("确认", this);
		connect(mergeFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(mergeFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_mergeAction);
		FoldSection* mergeFaceFold = new FoldSection("合并面", this);
		mergeFaceFold->addRow(mergeFaceCancelBtn, mergeFaceOkBtn, "Left");

		m_FoldSections.append(mergeFaceFold);
		m_foldWidgets.append(mergeFaceFold);

		//--合并体
		mergeRemoveGap = new QCheckBox("消除接缝", this);
		mergeRemoveGap->setCheckState(Qt::Checked);
		QPushButton* mergeSolidCancelBtn = new QPushButton("取消", this);
		QPushButton* mergeSolidOkBtn = new QPushButton("确认", this);
		connect(mergeSolidCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(mergeSolidOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_mergeAction);
		FoldSection* mergeSolidFold = new FoldSection("合并体", this);
		mergeSolidFold->addRow(mergeRemoveGap);
		QFrame* hLine12 = createHorizontalLine(mergeSolidFold);
		mergeSolidFold->addRow(hLine12);
		mergeSolidFold->addRow(mergeSolidCancelBtn, mergeSolidOkBtn, "Left");

		m_FoldSections.append(mergeSolidFold);
		m_foldWidgets.append(mergeSolidFold);
	}

	//初始化取消裁剪面操作界面
	void mGeoEditPanel::initCancelTrimFaceFold()
	{
		keepVertexChkBox = new QCheckBox("保持点", this);
		extendChkBox = new QCheckBox("延伸", this);

		QPushButton* cancelTrimCancelBtn = new QPushButton("取消", this);
		QPushButton* cancelTrimOkBtn = new QPushButton("确认", this);
		connect(cancelTrimCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(cancelTrimOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_cancelCutFaceAction);

		FoldSection* cancelTrimFold = new FoldSection("取消裁剪面", this);
		cancelTrimFold->addRow(extendChkBox, keepVertexChkBox);
		QFrame* hLine13 = createHorizontalLine(cancelTrimFold);
		cancelTrimFold->addRow(hLine13);
		cancelTrimFold->addRow(cancelTrimCancelBtn, cancelTrimOkBtn, "Left");

		m_FoldSections.append(cancelTrimFold);
		m_foldWidgets.append(cancelTrimFold);

	}

    void mGeoEditPanel::initFaceExtendFold()
    {

        QPushButton* cancelBtn = new QPushButton("取消", this);
        QPushButton* okBtn = new QPushButton("确认", this);
        connect(cancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(okBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_faceExtendAction);

        FoldSection* cancelTrimFold = new FoldSection("面延伸", this);
        QFrame* hLine13 = createHorizontalLine(cancelTrimFold);
        cancelTrimFold->addRow(hLine13);
        cancelTrimFold->addRow(cancelBtn, okBtn, "Left");

        m_FoldSections.append(cancelTrimFold);
        m_foldWidgets.append(cancelTrimFold);
    }

    void mGeoEditPanel::initLineCombineFold()
    {

        spLineA = new QLineEdit(this);
        spLineB = new QLineEdit(this);
        spLineA->setReadOnly(true);
        spLineB->setReadOnly(true);

        QToolButton* pickA = new QToolButton(this);
        pickA->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickA, &QToolButton::clicked, this, [this, pickA]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(spLineAId, "几何线");
            geoPickWidget->_featPick = SplineCombineA;
            });

        QToolButton* pickB = new QToolButton(this);
        pickB->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        connect(pickB, &QToolButton::clicked, this, [this, pickB]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(spLineBId, "几何线");
            geoPickWidget->_featPick = SplineCombineB;
            });

        isSmooth = new QCheckBox("平滑", this);
        QPushButton* spFaceByPntCancelBtn = new QPushButton("取消", this);
        //QPushButton* spFaceByPntOkBtn = new QPushButton("确认", this);
        connect(spFaceByPntCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        //connect(spFaceByPntOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_lineCombineAction);

        FoldSection* spFaceByPntFold = new FoldSection("线连接", this);
        spFaceByPntFold->addRow(new QLabel("几何线A"), spLineA, pickA);
        spFaceByPntFold->addRow(new QLabel("几何线B"), spLineB, pickB);
        spFaceByPntFold->addRow(isSmooth);
        QFrame* hLine1 = createHorizontalLine(spFaceByPntFold);
        spFaceByPntFold->addRow(hLine1);
        spFaceByPntFold->addRow(new QLabel(""), spFaceByPntCancelBtn, "Left");

        m_FoldSections.append(spFaceByPntFold);
        m_foldWidgets.append(spFaceByPntFold);
    }

	//初始化修补缝隙操作界面
	void mGeoEditPanel::initRepairCracksFold()
	{
		maxRepairFaceDis = maxRepairFaceDis = new MxFunctions::mCalcLineEdit(this); maxRepairFaceDis->setExpression("5");
		minRepairFaceDis = minRepairFaceDis = new MxFunctions::mCalcLineEdit(this); minRepairFaceDis->setExpression("0");

		QPushButton* repairCracksCancelBtn = new QPushButton("取消", this);
		QPushButton* repairCracksOkBtn = new QPushButton("确认", this);
		connect(repairCracksCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(repairCracksOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_repairFaceAction);
        minRepairFaceDisUnit = new QLabel("mm", this);
        maxRepairFaceDisUnit = new QLabel("mm", this);

		FoldSection* repairCracksFold = new FoldSection("修补缝隙", this);
		repairCracksFold->addRow(new QLabel("最小距离"), minRepairFaceDis, minRepairFaceDisUnit);
		repairCracksFold->addRow(new QLabel("最大距离"), maxRepairFaceDis, maxRepairFaceDisUnit);
		QFrame* hLine14 = createHorizontalLine(repairCracksFold);
		repairCracksFold->addRow(hLine14);
		repairCracksFold->addRow(repairCracksCancelBtn, repairCracksOkBtn, "Left");

		m_FoldSections.append(repairCracksFold);
		m_foldWidgets.append(repairCracksFold);

	}

    void mGeoEditPanel::initFillSurfaceFold()
    {
        QPushButton* repairCracksCancelBtn = new QPushButton("取消", this);
        QPushButton* repairCracksOkBtn = new QPushButton("确认", this);
        connect(repairCracksCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(repairCracksOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_fillSurfaceAction);

        FoldSection* repairCracksFold = new FoldSection("填充曲面", this);
        QFrame* hLine14 = createHorizontalLine(repairCracksFold);
        repairCracksFold->addRow(hLine14);
        repairCracksFold->addRow(repairCracksCancelBtn, repairCracksOkBtn, "Left");

        m_FoldSections.append(repairCracksFold);
        m_foldWidgets.append(repairCracksFold);
    }

	//初始化删除操作界面
	void mGeoEditPanel::iniDeleteFold()
	{
		//删除面
		delFaceEqualArea = new QCheckBox("等面积", this);
		connect(delFaceEqualArea, &QCheckBox::stateChanged, this, [this]() {
            if (delFaceEqualArea->isChecked())
			{
				CollectFaceIsEqualArea();
			}
			else {

			}
			});

		delFaceAssociatedSolid = new QCheckBox("删除关联实体", this);
		QPushButton* deleteFaceCancelBtn = new QPushButton("取消", this);
		QPushButton* deleteFaceOkBtn = new QPushButton("确认", this);
		connect(deleteFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(deleteFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_deleteFaceAction);

		FoldSection* deleteFaceFold = new FoldSection("删除面", this);
		QLabel* DelFaceLabel = new QLabel("", this);
		DelFaceLabel->setVisible(false);
		deleteFaceFold->addRow(DelFaceLabel, delFaceEqualArea, delFaceAssociatedSolid);
		QFrame* hLine15 = createHorizontalLine(deleteFaceFold);
		deleteFaceFold->addRow(hLine15);
		deleteFaceFold->addRow(deleteFaceCancelBtn, deleteFaceOkBtn, "Left");

		m_FoldSections.append(deleteFaceFold);
		m_foldWidgets.append(deleteFaceFold);

		//删除体
		delSolidEqualVolume = new QCheckBox("等体积", this);
		connect(delSolidEqualVolume, &QCheckBox::stateChanged, this, [this]() {
			if (delSolidEqualVolume->isChecked())
			{
				CollectSolidIsEqualVolume();
			}
			else {

			}
			});

		delSolidKeepFace = new QCheckBox("保留表面", this);

		QPushButton* deleteSolidCancelBtn = new QPushButton("取消", this);
		QPushButton* deleteSolidOkBtn = new QPushButton("确认", this);
		connect(deleteSolidCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(deleteSolidOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_deleteSolidAction);

		FoldSection* deleteSolidFold = new FoldSection("删除体", this);
		QLabel* DelSolidLabel = new QLabel("", this);
		DelSolidLabel->setVisible(false);
        deleteSolidFold->addRow(DelSolidLabel, delSolidEqualVolume, delSolidKeepFace);
		QFrame* hLine16 = createHorizontalLine(deleteSolidFold);
		deleteSolidFold->addRow(hLine16);
		deleteSolidFold->addRow(deleteSolidCancelBtn, deleteSolidOkBtn, "Left");

		m_FoldSections.append(deleteSolidFold);
		m_foldWidgets.append(deleteSolidFold);
	}

	//初始化删除线操作界面（仅独立线，无附加选项）
	void mGeoEditPanel::initDeleteLineFold()
	{
		QLabel* deleteLineTip = new QLabel("仅支持删除独立线", this);
		QPushButton* deleteLineCancelBtn = new QPushButton("取消", this);
		QPushButton* deleteLineOkBtn = new QPushButton("确认", this);
		connect(deleteLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(deleteLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_deleteLineAction);

		FoldSection* deleteLineFold = new FoldSection("删除线", this);
		deleteLineFold->addRow(deleteLineTip);
		QFrame* hLine17 = createHorizontalLine(deleteLineFold);
		deleteLineFold->addRow(hLine17);
		deleteLineFold->addRow(deleteLineCancelBtn, deleteLineOkBtn, "Left");

		m_FoldSections.append(deleteLineFold);
		m_foldWidgets.append(deleteLineFold);
	}

	void mGeoEditPanel::initSplitLineFold()
	{
		QComboBox* spMod = new QComboBox(this);
		spMod->addItem("点选分割");
		QPushButton* spLineCancelBtn = new QPushButton("取消", this);
		QPushButton* spLineOkBtn = new QPushButton("确认", this);
		connect(spLineCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
		connect(spLineOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_splitLineAction);

		FoldSection* spLineFold = new FoldSection("分割线", this);
		spLineFold->addRow(new QLabel("分割方式"), spMod);
		spLineFold->addRow(spLineCancelBtn, spLineOkBtn, "Left");
		m_FoldSections.append(spLineFold);
		m_foldWidgets.append(spLineFold);
	}

    void mGeoEditPanel::initLineLengthenFold()
    {
        QPushButton* LineLengthenCancelBtn = new QPushButton("取消", this);
        QPushButton* LineLengthenOkBtn = new QPushButton("确认", this);
        connect(LineLengthenCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(LineLengthenOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_lineLengthenAction);

        lengthSize = new MxFunctions::mCalcLineEdit(this);
        lengthSize->setExpression("10");
        reverseLengthen = new QCheckBox("反向", this);
        lengthSizeUnit = new QLabel("mm", this);

        FoldSection* LineLengthenFold = new FoldSection("线延长", this);
        LineLengthenFold->addRow(new QLabel("延伸长度"), lengthSize, lengthSizeUnit);
        LineLengthenFold->addRow(reverseLengthen);
        LineLengthenFold->addRow(LineLengthenCancelBtn, LineLengthenOkBtn, "Left");
        m_FoldSections.append(LineLengthenFold);
        m_foldWidgets.append(LineLengthenFold);
    }

    void mGeoEditPanel::initFaceOffsetFold()
    {
        QPushButton* faceOffsetCancelBtn = new QPushButton("取消", this);
        QPushButton* faceOffsetOkBtn = new QPushButton("确认", this);
        connect(faceOffsetCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(faceOffsetOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_faceOffsetAction);

        faceOffsetDis = new MxFunctions::mCalcLineEdit(this);
        faceOffsetDis->setExpression("10");
        faceOffsetDisUnit = new QLabel("mm", this);

        FoldSection* faceOffsetFold = new FoldSection("面偏置", this);
        faceOffsetFold->addRow(new QLabel("偏置距离"), faceOffsetDis, faceOffsetDisUnit);
        faceOffsetFold->addRow(faceOffsetCancelBtn, faceOffsetOkBtn, "Left");
        m_FoldSections.append(faceOffsetFold);
        m_foldWidgets.append(faceOffsetFold);
    }

    void mGeoEditPanel::initFeatureProjectionFold()
    {
        QPushButton* projectionCancelBtn = new QPushButton("取消", this);
        QPushButton* projectionOkBtn = new QPushButton("确认", this);
        connect(projectionCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(projectionOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_featureProjectionAction);

        projectionVec = new QLineEdit(this);
        projectionVec->setPlaceholderText("1,0,0");
        MxFunctions::mEditDiagnosis::setOnlyVec3(projectionVec);
        isSplitProjection = new QCheckBox("分割", this);

        QToolButton* projectionReverseBtn = new QToolButton(this);
        projectionReverseBtn->setIcon(QIcon(":/mMainWidgets/Icon/choice/fanxiang3x.png"));
        projectionReverseBtn->setToolTip("反向");
        connect(projectionReverseBtn, &QToolButton::clicked, this, [this]() {
            reverseVecAction(projectionVec);
            });

        QToolButton* projectionMousePickBtn = new QToolButton(this);
        projectionMousePickBtn->setIcon(QIcon(":/mMainWidgets/Icon/Pick any.png"));
        projectionMousePickBtn->setToolTip("选择投影方向");
        connect(projectionMousePickBtn, &QToolButton::clicked, this, [this, projectionMousePickBtn]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->clearHighlight();
            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            Highlight(projectionAxisId, "几何线");
            geoPickWidget->_featPick = ProjectionVec;
            });

        FoldSection* projectionFold = new FoldSection("特征投影", this);
        projectionFold->addRow(new QLabel("投影方向"), projectionVec, projectionReverseBtn, projectionMousePickBtn);
        projectionFold->addRow(isSplitProjection);
        projectionFold->addRow(projectionCancelBtn, projectionOkBtn, "Left");
        m_FoldSections.append(projectionFold);
        m_foldWidgets.append(projectionFold);
    }

    void mGeoEditPanel::initCheckEdgeAndFaceFold()
    {
        //短边
        QPushButton* shortEdgeCancelBtn = new QPushButton("取消", this);
        QPushButton* shortEdgeOkBtn = new QPushButton("检查", this);
        connect(shortEdgeCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(shortEdgeOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_checkShortEdge);


        shortEdgeMaxLen = new MxFunctions::mCalcLineEdit(this);
        shortEdgeMaxLen->setExpression("0.1");
        shortEdgeMaxLenUnit = new QLabel("mm", this);

        FoldSection* shortEdgeFold = new FoldSection("短边检测", this);
        shortEdgeFold->addRow(new QLabel("最大长度"), shortEdgeMaxLen, shortEdgeMaxLenUnit);
        shortEdgeFold->addRow(shortEdgeCancelBtn, shortEdgeOkBtn, "Left");
        m_FoldSections.append(shortEdgeFold);
        m_foldWidgets.append(shortEdgeFold);
        //-----------------------------


        //锐边
        QPushButton* sharpEdgeCancelBtn = new QPushButton("取消", this);
        QPushButton* sharpEdgeOkBtn = new QPushButton("检查", this);
        connect(sharpEdgeCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(sharpEdgeOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_checkSharpEdge);


        sharpEdgeMaxAngle = new MxFunctions::mCalcLineEdit(this);
        sharpEdgeMaxAngle->setExpression("120");
        sharpEdgeMaxAngleUnit = new QLabel("°", this);

        concave = new QCheckBox("凹陷"); concave->setChecked(true);
        convex = new QCheckBox("凸起"); convex->setChecked(false);

        FoldSection* sharpEdgeFold = new FoldSection("锐边检测", this);
        sharpEdgeFold->addRow(new QLabel("最大棱角"), sharpEdgeMaxAngle, sharpEdgeMaxAngleUnit);
        sharpEdgeFold->addRow(concave);
        sharpEdgeFold->addRow(convex);
        sharpEdgeFold->addRow(sharpEdgeCancelBtn, sharpEdgeOkBtn, "Left");
        m_FoldSections.append(sharpEdgeFold);
        m_foldWidgets.append(sharpEdgeFold);
        //-----------------------------

        //小型面
        QPushButton* smallFaceCancelBtn = new QPushButton("取消", this);
        QPushButton* smallFaceOkBtn = new QPushButton("检查", this);
        QPushButton* smallFaceRepairBtn = new QPushButton("修复", this);
        connect(smallFaceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(smallFaceOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_checkSmallFace);
        connect(smallFaceRepairBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_checkSmallFace);

        smallFaceMaxArea = new MxFunctions::mCalcLineEdit(this);
        smallFaceMaxArea->setExpression("0.0000001");
        smallFaceMaxAreaUnit = new QLabel("mm²", this);

        smallFaceMaxWidth = new MxFunctions::mCalcLineEdit(this);
        smallFaceMaxWidth->setExpression("0.1");
        smallFaceMaxWidthUnit = new QLabel("mm", this);

        //smallFaceMinRatio = new MxFunctions::mCalcLineEdit(this);
        //smallFaceMinRatio->setExpression("5");

        useAreaCheckFace = new QCheckBox("按面积查找面"); useAreaCheckFace->setChecked(true); 
        useWidthCheckFace = new QCheckBox("按短边宽度查找面"); useWidthCheckFace->setChecked(true);
        //useRatioCheckFace = new QCheckBox("按长宽比查找面"); useRatioCheckFace->setChecked(true);

        FoldSection* smallFaceFold = new FoldSection("小型面检测", this);
        smallFaceFold->addRow(useAreaCheckFace);
        smallFaceFold->addRow(new QLabel("最大面积"), smallFaceMaxArea, smallFaceMaxAreaUnit);
        smallFaceFold->addRow(useWidthCheckFace);
        smallFaceFold->addRow(new QLabel("最大宽度"), smallFaceMaxWidth, smallFaceMaxWidthUnit);
        //smallFaceFold->addRow(useRatioCheckFace);
        //smallFaceFold->addRow(new QLabel("最小长宽比"), smallFaceMinRatio);
        smallFaceFold->addRow(smallFaceCancelBtn, smallFaceOkBtn, smallFaceRepairBtn, "Left");
        m_FoldSections.append(smallFaceFold);
        m_foldWidgets.append(smallFaceFold);
        //-----------------------------

    }

    void mGeoEditPanel::initInterferenceCheckFold()
    {
        // 创建三个按钮（移除、取消、检查）
        interferenceRemoveBtn = new QPushButton("移除", this);
        interferenceRemoveBtn->setVisible(false);   // 默认隐藏，仅在选择"几何体"时显示
        QPushButton* interferenceCancelBtn = new QPushButton("取消", this);
        QPushButton* interferenceCheckBtn = new QPushButton("检查", this);

        // 连接按钮信号
        connect(interferenceRemoveBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_RemovalInterfer);
        connect(interferenceCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(interferenceCheckBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_interferenceCheckAction);

        // 创建折叠区域
        FoldSection* interferenceCheckFold = new FoldSection("几何干涉检查", this);
        interferenceCheckFold->addRow(interferenceRemoveBtn, interferenceCancelBtn, interferenceCheckBtn, "Left");
        m_FoldSections.append(interferenceCheckFold);
        m_foldWidgets.append(interferenceCheckFold);
    }

    //初始化微小实体检测界面
    void mGeoEditPanel::initTinySolidFold()
    {
        QPushButton* tinySolidCancelBtn = new QPushButton("取消", this);
        QPushButton* tinySolidOkBtn = new QPushButton("检查", this);
        connect(tinySolidCancelBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_onCancel);
        connect(tinySolidOkBtn, &QPushButton::clicked, this, &mGeoEditPanel::slot_checkTinySolid);

        tinySolidMaxVolume = new MxFunctions::mCalcLineEdit(this);
        tinySolidMaxVolume->setExpression("0.000001");
        tinySolidMaxVolumeUnit = new QLabel("mm³", this);

        FoldSection* tinySolidFold = new FoldSection("微小实体检测", this);
        tinySolidFold->addRow(new QLabel("最大体积"), tinySolidMaxVolume, tinySolidMaxVolumeUnit);
        tinySolidFold->addRow(tinySolidCancelBtn, tinySolidOkBtn, "Left");
        m_FoldSections.append(tinySolidFold);
        m_foldWidgets.append(tinySolidFold);
    }

	void mGeoEditPanel::slot_highlightGeoData()
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

		if (geoPickWidget->strPickType != type || geoPickWidget->_featPick == None)
		{
			return;
		}

		if (geoPickWidget->_featPick == FeatureObj)
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

			if (delFaceEqualArea->isChecked() && GeoDeleteFace == currentFeature && type == "几何面")
			{
				CollectFaceIsEqualArea();
			}
			if (delSolidEqualVolume->isChecked() && GeoDeleteSolid == currentFeature && type == "几何体")
			{
				CollectSolidIsEqualVolume();
			}

		}
		else if (geoPickWidget->_featPick == SpFacePntA)
		{

			disHighlight(spFacePntAId, "几何点");
			
			if (res.size() <= 0 || spFacePntAId == -2)
			{
				spFacePntA->setText("");
				spFacePntAId = -1;
			}
			else
			{
                if (spFacePntBId == res[res.size() - 1])
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("几何点%1已被拾取!").arg(spFacePntBId));
                    return;
                }
				spFacePntA->setText(QString(type + ": %1").arg(res[res.size() - 1]));
				spFacePntAId = res[res.size() - 1];
				if (spFacePntBId <= -1)
				{
					geoPickWidget->_featPick = SpFacePntB;
				}
				else
				{
					slot_splitFaceByPnt();
				}
			}
		}
		else if (geoPickWidget->_featPick == SpFacePntB)
		{
			disHighlight(spFacePntBId, "几何点");
            if (res.size() <= 0 || spFacePntBId == -2)
			{
				spFacePntB->setText("");
				spFacePntBId = -1;
			}
            else
			{
                if (spFacePntAId == res[res.size() - 1])
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("几何点%1已被拾取!").arg(spFacePntAId));
                    return;
                }
				spFacePntB->setText(QString(type + ": %1").arg(res[res.size() - 1]));
				spFacePntBId = res[res.size() - 1];
				if (spFacePntAId <= -1)
				{
					geoPickWidget->_featPick = SpFacePntA;
				}
				else
				{
					slot_splitFaceByPnt();
				}
			}
		}
        else if (geoPickWidget->_featPick == SplineCombineA)
        {

            disHighlight(spLineAId, "几何线");

            if (res.size() <= 0)
            {
                spLineA->setText("");
                spLineAId = -1;
            }
            else
            {
                if (spLineBId == res[res.size() - 1])
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("几何线%1已被拾取!").arg(spLineBId));
                    return;
                }
                spLineA->setText(QString(type + ": %1").arg(res[res.size() - 1]));
                spLineAId = res[res.size() - 1];
                if (spLineBId == -1)
                {
                    geoPickWidget->_featPick = SplineCombineB;
                }
                else
                {
                    slot_lineCombineAction();
                }
            }
        }
        else if (geoPickWidget->_featPick == SplineCombineB)
        {
            disHighlight(spLineBId, "几何线");
            if (res.size() <= 0)
            {
                spLineB->setText("");
                spLineBId = -1;
            }
            else
            {
                if (spLineAId == res[res.size() - 1])
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, QString("几何线%1已被拾取!").arg(spLineAId));
                    return;
                }
                spLineB->setText(QString(type + ": %1").arg(res[res.size() - 1]));
                spLineBId = res[res.size() - 1];
                QString text = spLineA->text();
                if (spLineAId == -1)
                {
                    geoPickWidget->_featPick = SplineCombineA;
                }
                else
                {
                    slot_lineCombineAction();
                }
            }
        }
		else if (geoPickWidget->_featPick == SpFaceGeoLine ||
			geoPickWidget->_featPick == SpSolidBndLine ||
			geoPickWidget->_featPick == SpSolidSwLine ||
			geoPickWidget->_featPick == SpSolidGeoFace ||
			geoPickWidget->_featPick == ExternalCutEdge ||
            geoPickWidget->_featPick == ProjectionObj)
		{
			if (geoPickWidget_2->strPickType != type)
			{
				return;
			}

			for (int i = spPickTable->rowCount() - 1; i >= 0; i--)
			{
				QString strId = spPickTable->item(i, 0)->text();
				if (strId.indexOf(type) == 0)
				{
					spPickTable->removeRow(i);
				}
			}

			for (auto id : res)
			{
				spPickTable->insertRow(spPickTable->rowCount());
				spPickTable->setItem(spPickTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
			}
		}
		else if (geoPickWidget->_featPick == SpSolidSweepCustomVec)
		{
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setVectorToLineEdit(spSolidCustomVec, QVector3D{});
				return;
			}
			else
			{
				setVectorToLineEdit(spSolidCustomVec, lines[lines.size() - 1].m_dir);
			}

			disHighlight(spSolidSwpLastLineID, type);
			spSolidSwpLastLineID = lines[lines.size() - 1].id;
		}
		else if (geoPickWidget->_featPick == SpFaceCustomVec)
		{
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setVectorToLineEdit(spFaceCustomLineVec, QVector3D{});
				return;
			}
			else
			{
				setVectorToLineEdit(spFaceCustomLineVec, lines[lines.size() - 1].m_dir);
			}

			disHighlight(spFaceLastLineID, type);
			spFaceLastLineID = lines[lines.size() - 1].id;
		}
		else if (geoPickWidget->_featPick == SpFaceDtmCustomVec)
		{
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setVectorToLineEdit(spFaceCustomFaceVec, QVector3D{});
				return;
			}
			else
			{
				setVectorToLineEdit(spFaceCustomFaceVec, lines[lines.size() - 1].m_dir);
			}

			disHighlight(spFaceDtmLastLineID, type);
			spFaceDtmLastLineID = lines[lines.size() - 1].id;
		}
		else if (geoPickWidget->_featPick == SpSolidDatumCustomVec)
		{
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
			if (lines.size() <= 0)
			{
				setVectorToLineEdit(spSolidCustomFaceVec, QVector3D{});
				return;
			}
			else
			{
				setVectorToLineEdit(spSolidCustomFaceVec, lines[lines.size() - 1].m_dir);
			}

			disHighlight(spSolidDtmLastLineID, type);
			spSolidDtmLastLineID = lines[lines.size() - 1].id;
		}
        else if (geoPickWidget->_featPick == ProjectionVec)
        {
            auto lines = getGEOServer.geoCalculate().CalculateEdgeAttr(res);
            if (lines.size() <= 0)
            {
                setVectorToLineEdit(projectionVec, QVector3D{});
                return;
            }
            else
            {
                setVectorToLineEdit(projectionVec, lines[lines.size() - 1].m_dir);
            }

            disHighlight(projectionAxisId, type);
            projectionAxisId = lines[lines.size() - 1].id;
            }
        else if (geoPickWidget->_featPick == SpLine && type.compare("几何线") == 0)
		{
			//交点信息
			//auto vecSelInfos = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoEdge);
			//vecSelInfos[0].id = vecIntersectionPnt.size();
			//vecIntersectionPnt.insert(vecIntersectionPnt.end(), vecSelInfos.begin(), vecSelInfos.end());

			//添加原有交点
			//getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, vecSelInfos);

			//高亮交点

			getCVM->getPreRenderWidget()->getHighlightController()->updateExtraHighLight(getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData());

		}

	}

	void mGeoEditPanel::slot_subWindowActivated()
	{
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoEditPanel::slot_highlightGeoData);
	}


	void mGeoEditPanel::slot_contextMenuTableData(const QPoint& pos)
	{
		mUiMenu menu(spPickTable);
		menu.addActions({ subDelAction });
		QAction* selected = menu.exec(spPickTable->viewport()->mapToGlobal(pos));
		if (selected == subDelAction)
		{
			QList<QTableWidgetItem*> items = spPickTable->selectedItems();
			if (!items.isEmpty())
			{
				vector<int> ids;
				for (auto item : items)
				{
					ids.push_back(item->row());
				}

				sort(ids.rbegin(), ids.rend());

				for (int i = ids.size() - 1; i >= 0; --i)
				{
					QString strId = spPickTable->item(ids[i], 0)->text();
					QString strGeoType = strId;
					strId = strId.mid(strId.lastIndexOf(":") + 1);
					int id = strId.toInt();

					std::vector<int> Ids;
					Ids.push_back(id);

					if (strGeoType.indexOf("几何线") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoEdge, Ids);
					}
					else if (strGeoType.indexOf("几何面") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoFace, Ids);
					}
					else if (strGeoType.indexOf("几何体") == 0)
					{
						getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::REDUCE, SelectionInfoType::GeoSolid, Ids);
					}
					spPickTable->removeRow(ids[i]);
				}
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
	}


	void mGeoEditPanel::slot_onCancel()
	{
		//getCVM->getPreRenderWidget()->getStateMachineManager()->setGeoDividerStateEnabled(false);
        spFacePntA->setText("");
        spFacePntB->setText("");
        spFacePntAId = -1;
        spFacePntBId = -1;

		disconnect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
        getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
        hideAllFold();
        closeDrawCurve();
	}

	void mGeoEditPanel::slot_splitFaceByPnt()
	{
        try
        {
            if (spFacePntAId == -1 || spFacePntBId == -1)
            {
                return;
            }

            std::set<int> geoIds({ spFacePntAId , spFacePntBId });
            std::vector<uint64_t> targets;
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            //发送分割线请求
            EventDispatcher::mGeoSurfaceEditOfSplitByPointsRequest req;
            req.objtype = GeoNeutralData::GN_ShapeType::Vertex;
            req.targets = targets;

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().surfaceEditOfSplitByPoints(req);

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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
            }
        }
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitFaceByCutLine()
	{
		try
		{
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
			//getCVM->getPreRenderWidget()->getStateMachineManager()->setGeoDividerStateEnabled(false);
			if (point3dCoords.isEmpty())
			{
				return;
			}

            std::vector<int> m_shape;
			std::set<int> geoIds = GetTableIdsUseSet();;
			if (geoIds.empty())
			{
				return;
			}
            for (auto id : geoIds)
			{
                m_shape.push_back(id);
				}

			glm::vec3 screenVec = getCVM->getPreRenderWidget()->getCameraController()->getCameraState().eye_dir;
			double mdir1[3] = { screenVec[0],screenVec[1],screenVec[2] };


            mGeoFaceEditOfSplitByLinesRequest req;
            req.geoIds = m_shape;
            req.points = point3dCoords;
            req.mdir1[0] = mdir1[0];
            req.mdir1[1] = mdir1[1];
            req.mdir1[2] = mdir1[2];
            req.splitType = EventDispatcher::CutLines;
            req.isClosed = spFaceLClose->isChecked();
            req.isSmooth = spFaceLSmooth->isChecked();
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitFacesByCutLine(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}

		point3dCoords.clear();
	}

	void mGeoEditPanel::slot_splitFaceByGeoLine()
	{
		try
		{
            std::vector<int> m_shape;
            std::vector<int> lineIds;
			std::set<int> geoFaceIds = GetTableIdsUseSet();
			std::set<int> geoLineIds;
			for (int i = 0; i < spPickTable->rowCount(); ++i)
			{
				QString strId = spPickTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoLineIds.insert(id);
                lineIds.push_back(id);
			}
            for (auto id : geoFaceIds)
			{
                m_shape.push_back(id);
			}

            if (geoFaceIds.empty() || geoLineIds.empty())
			{
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
					return;
				}

            

			double spVec[3] = { 1,0,0 };
            EventDispatcher::NormalType mType;
			if (spFaceDir->currentIndex() != 6)
			{
                mType = EventDispatcher::UserDefine;
                double* mdir[3];
				getDir(mdir, spFaceDir->currentIndex(), spFaceCustomLineVec);
				spVec[0] = *mdir[0];
				spVec[1] = *mdir[1];
				spVec[2] = *mdir[2];
			}
			else
			{
                mType = EventDispatcher::NormalToSurface;
			}

			double dis = -1.0;
            if (spFaceMode->currentIndex() == 1)
            {
                dis = getDefaultValue(Unit::Length, spFaceDistance->text().toDouble());
            }

            mGeoFaceEditOfSplitByLinesRequest req;
            req.geoIds = m_shape;
            req.splitedges = lineIds;
            req.dis = dis;
            req.normalType = mType;
            req.mdir1[0] = spVec[0];
            req.mdir1[1] = spVec[1];
            req.mdir1[2] = spVec[2];
            req.splitType = EventDispatcher::GeoLines;
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitFacesByCutLine(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitFaceByStdFace()
	{
		try
		{
			std::set<int> geoFaceIds = GetTableIdsUseSet();
			if (geoFaceIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
            for (auto id : geoFaceIds)
            {
                m_shape.push_back(id);
            }

			//QList<MGeoCommand::EditShape> mEditShapeList; //【OCC摘除·S10/P0-1 终局2】孤儿死声明（同特征面板模式）
			double spVec[3] = { 1,0,0 };
			double* mdir[3];
			getDir(mdir, spFaceVector->currentIndex(), spFaceCustomFaceVec);
			spVec[0] = *mdir[0];
			spVec[1] = *mdir[1];
			spVec[2] = *mdir[2];

			double mPos[3];
			QVector3D customPos = extractVectorFromLineEdit(spFaceStdPnt->text());
			mPos[0] = customPos[0];
			mPos[1] = customPos[1];
			mPos[2] = customPos[2];

			double size = getDefaultValue(Unit::Length, spFaceSize->text().toDouble());

            mGeoFaceEditOfSplitByPlnRequest req;
            req.geoIds = m_shape;
            req.spVec[0] = spVec[0];
            req.spVec[1] = spVec[1];
            req.spVec[2] = spVec[2];
            req.mPos[0] = mPos[0];
            req.mPos[1] = mPos[1];
            req.mPos[2] = mPos[2];
            req.size = size;

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitFacesByPln(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitSolidByCurve()
	{
		try
		{
            getCVM->getPreViewportWidget()->interaction()->getToolManager()->setActive(mxr::InteractionToolType::None);
			//getCVM->getPreRenderWidget()->getStateMachineManager()->setGeoDividerStateEnabled(false);
			if (point3dCoords.isEmpty())
			{
				return;
			}

			std::set<int> geoIds = GetTableIdsUseSet();;
			if (geoIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
            for (auto id : geoIds)
			{
                m_shape.push_back(id);
				}

			glm::vec3 screenVec = getCVM->getPreRenderWidget()->getCameraController()->getCameraState().eye_dir;
			double mdir1[3] = { -screenVec[0],-screenVec[1],-screenVec[2] };

            mGeoSolidEditOfSplitByLinesRequest req;
            req.geoIds = m_shape;
            req.points = point3dCoords;
            req.mdir1[0] = mdir1[0];
            req.mdir1[1] = mdir1[1];
            req.mdir1[2] = mdir1[2];
            req.isClosed = spSolidLClose->isChecked();
            req.isSmooth = spSolidLSmooth->isChecked();
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitSolidsByLines(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
			point3dCoords.clear();

		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitSolidByBndLine()
	{
		try
		{
            std::vector<int> m_shape;
            std::vector<int> lineIds;
			std::set<int> geoSolidIds = GetTableIdsUseSet();
			std::set<int> geoLineIds;
			for (int i = 0; i < spPickTable->rowCount(); ++i)
			{
				QString strId = spPickTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoLineIds.insert(id);
                lineIds.push_back(id);
			}
            for (auto id : geoSolidIds)
            {
                m_shape.push_back(id);
            }

			if (geoSolidIds.empty() || geoLineIds.empty())
			{
				return;
				}

            mGeoSolidEditOfSplitByBoundLinesRequest req;
            if (spSolidFaceExpand->currentIndex() == 0)
			{
                req._ExtendValue = 0;
			}
			else
			{
                req._ExtendValue = spSolidFaceExNum->text().toDouble();
			}
            req.geoIds = m_shape;
            req.splitedges = lineIds;
            req._isExtend = spSolidExpFace->isChecked();
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitSolidsByBoundLines(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitSolidBySwLine()
	{
		try
		{
            std::vector<int> m_shape;
            std::vector<int> m_lines;
			std::set<int> geoSolidIds = GetTableIdsUseSet();
			std::set<int> geoLineIds;
			for (int i = 0; i < spPickTable->rowCount(); ++i)
			{
				QString strId = spPickTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoLineIds.insert(id);
                m_lines.push_back(id);
			}
            for (auto id : geoSolidIds)
			{
                m_shape.push_back(id);
			}

            if (geoSolidIds.empty() || geoLineIds.empty())
			{
					return;
				}

			double spVec[3];
			double* mdir[3];
			getDir(mdir, spSolidDragDir->currentIndex(), spSolidCustomVec);
			spVec[0] = *mdir[0];
			spVec[1] = *mdir[1];
			spVec[2] = *mdir[2];

            mGeoSolidEditOfSplitBySweepLinesRequest req;
            req.geoIds = m_shape;
            req.splitedges = m_lines;
			if (spSolidDragMode->currentIndex() == 0)
			{
                req.SplitValue = -1.0; //完全分割
			}
			else
			{
                req.SplitValue = getDefaultValue(Unit::Length, spSolidDragDis->text().toDouble());
			}
            req.spVec[0] = spVec[0];
            req.spVec[1] = spVec[1];
            req.spVec[2] = spVec[2];
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitSolidsBySweepLines(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitSolidByStdFace()
	{
		try
		{
            std::vector<int> m_shape;
			std::set<int> geoSolidIds = GetTableIdsUseSet();
			if (geoSolidIds.empty())
			{
				return;
			}
            for (auto id : geoSolidIds)
			{
                m_shape.push_back(id);
				}

			double spVec[3];
			double* mdir[3];
			getDir(mdir, spSolidVector->currentIndex(), spSolidCustomFaceVec);
			spVec[0] = *mdir[0];
			spVec[1] = *mdir[1];
			spVec[2] = *mdir[2];

			double mPos[3];
			QVector3D pos = extractVectorFromLineEdit(spSolidStdPnt->text());
			mPos[0] = pos[0];
			mPos[1] = pos[1];
			mPos[2] = pos[2];

            mGeoSolidEditOfSplitByStdFaceRequest req;
            req.geoIds = m_shape;
            req.spVec[0] = spVec[0];
            req.spVec[1] = spVec[1];
            req.spVec[2] = spVec[2];
            req.mPos[0] = mPos[0];
            req.mPos[1] = mPos[1];
            req.mPos[2] = mPos[2];
            req.PlnLength = getDefaultValue(Unit::Length, spSolidSize->text().toDouble());
          
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitSolidsByStdFace(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_splitSolidByGeoFace()
	{
		try
		{
            std::vector<int> m_shape;
            std::vector<int> geoface;
			std::set<int> geoSolidIds = GetTableIdsUseSet();
			std::set<int> geoPlaneIds;
			for (int i = 0; i < spPickTable->rowCount(); ++i)
			{
				QString strId = spPickTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoPlaneIds.insert(id);
                geoface.push_back(id);
			}
            for (auto id : geoSolidIds)
            {
                m_shape.push_back(id);
            }

			if (geoSolidIds.empty() || geoPlaneIds.empty())
			{
				return;
			}

            mGeoSolidEditOfSplitByGeoFaceRequest req;
            req.geoIds = m_shape;
            req.faceIds = geoface;
            req.ExtendValve = getDefaultValue(Unit::Length, spSolidFaceExSize->text().toDouble());
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfSplitSolidsByGeoFace(req);
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
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_mergeAction()
	{
		try
		{
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
            for (int t : geoIds)
            {
                m_shape.push_back(t);
            }
			
			if (GeoMergeLine == currentFeature)
			{
                mGeoLineEditOfMergeRequest req;
                req.name = "lineMerge";
                req.geoIds = m_shape;
                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfMergeEdges(req);
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
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
				}
			}
			else if (GeoMergeFace == currentFeature)
			{
                mGeoFaceEditOfMergeRequest req;
                req.name = "faceMerge";
                req.geoIds = m_shape;
                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfMergeFaces(req);
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
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
				}
			}
			else if (GeoMergeSolid == currentFeature)
			{
                mGeoSolidEditOfMergeRequest req;
                req.name = "solidMerge";
                req.geoIds = m_shape;
                req.removeGap = mergeRemoveGap->isChecked();
                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfMergeSolids(req);
                if (res.succeeded())
                {
                    //跨部件合并会删除工具体 Part；先发 DeleteOnePart 清掉渲染/
                    //模型树中的旧节点，再同步保留部件的新形状，避免悬空 partData。
                    std::set<int> removedPartIds;
                    for (const auto& removedPart : res.removedObjectIds)
                    {
                        if (removedPart.type == GeoNeutralData::GN_ShapeType::Part)
                        {
                            removedPartIds.insert(removedPart.id);
                        }
                    }
                    if (!removedPartIds.empty())
                    {
                        getCVM->getModelData()->getGeoManager()->deletePart(removedPartIds);
                    }
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
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
				}
			}
		}
		catch (...)
		{
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_cancelCutFaceAction()
	{
		try
		{
            // 外部裁剪边
            std::vector<uint64_t> geoExternalIds;
            // 内部裁剪边
            std::vector<uint64_t> geoInternalIds;

            std::set<int> externalIds = GetTableIdsUseSet();
            geoExternalIds.reserve(externalIds.size());
            for (int id : externalIds) {
                geoExternalIds.push_back(static_cast<uint64_t>(id));
            }
            geoInternalIds.reserve(spPickTable->rowCount());
			for (int i = 0; i < spPickTable->rowCount(); ++i)
			{
				QString strId = spPickTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
                geoInternalIds.push_back(static_cast<uint64_t>(id));
			}
			if (geoExternalIds.empty() && geoInternalIds.empty())
			{
				return;
			}

            EventDispatcher::mGeoCancelTrimFaceRequest req;

			if (geoExternalIds.size() > 0)
			{
                req.geoIds = geoExternalIds;
                req.isExtend = extendChkBox->isChecked();
                req.isKeepVer = keepVertexChkBox->isChecked();
                req.mUntrimType = EventDispatcher::ByTrimLines;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().cancelCutFace(req);
                if (res.succeeded())
				{
                    int modelDataID = getCVM->getModelData()->getModelDataID();

                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
					{
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
					}
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
					geoPickWidget->clearHighlight();
					clearTableItems(pickObjTable);
					MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("[外部裁剪面]几何编辑成功!"));
				}
				else
				{
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("[外部裁剪面]几何编辑失败!"));
				}
			}

			if (geoInternalIds.size() > 0)
			{
                req.geoIds = geoInternalIds;
                req.isExtend = extendChkBox->isChecked();
                req.isKeepVer = keepVertexChkBox->isChecked();
                req.mUntrimType = EventDispatcher::ByInternalLines;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().cancelCutFace(req);
                if (res.succeeded())
				{
                    int modelDataID = getCVM->getModelData()->getModelDataID();

                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
					{
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
					}
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
					geoPickWidget->clearHighlight();
                    clearTableItems(pickObjTable);
					MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("[内部裁剪面]几何编辑成功!"));
				}
				else
				{
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("[内部裁剪面]几何编辑失败!"));
				}
               
			}

		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_repairFaceAction()
	{
		try
		{
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				return;
			}

			double dMax = getDefaultValue(Unit::Length, maxRepairFaceDis->text().toDouble());
			double dMin = getDefaultValue(Unit::Length, minRepairFaceDis->text().toDouble());

            EventDispatcher::mGeoLineEditOfSweRequest req;
            req.edgeIds = geoIds;
            req.minDistance = dMin;
            req.maxDistance = dMax;

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().lineEditOfSweOCC(req);
            if (res.succeeded())
			{
                for (auto editpart : res.modifiedObjectIds)
				{
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
				}
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
				geoPickWidget->clearHighlight();
				clearTableItems(pickObjTable);
			}
			else
			{
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

    void mGeoEditPanel::slot_fillSurfaceAction()
    {
            //【OCC摘除·S9·旧命令链下线】填充面命令（createSurfaceOfFill）随冻结岛退役；
            // 迁移引擎（OCCAdapter executeFeature*）后恢复。
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("填充面已切换引擎管线（迁移中）。"));
    }

	void mGeoEditPanel::slot_GeoDividerLineData(QVector<QVector3D> vec3Ddata)
	{
		point3dCoords.clear();
		point3dCoords = vec3Ddata;
		disconnect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::signal_GeoDividerLineData, this, &mGeoEditPanel::slot_GeoDividerLineData);
	}

	void mGeoEditPanel::slot_deleteFaceAction()
	{
		try
		{
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
			for (int t : geoIds)
			{
                m_shape.push_back(t);
				}

                mGeoFaceEditOfDeleteRequest req;
                req.name = "faceDelete";
                req.delAssociatedSolid = delFaceAssociatedSolid->isChecked();
                req.geoIds = m_shape;
                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteFaces(req);
                if (res.succeeded())
			{
                    //将编辑成功的partID记录
                    std::set<int> partIds;
                    for (auto editpart : res.modifiedObjectIds)
                {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    if (res.removedObjectIds.size() > 0)
                    {
                        for (int i = 0; i < res.removedObjectIds.size(); ++i)
                        {
                            partIds.insert(res.removedObjectIds[i].id);
                        }
                        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                }
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    geoPickWidget->clearHighlight();
                    clearTableItems(pickObjTable);
			}
			else
			{
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_deleteSolidAction()
	{
		try
		{
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
			for (int t : geoIds)
			{
                m_shape.push_back(t);
				}

            mGeoSolidEditOfDeleteRequest req;
            req.name = "solidDelete";
            req.reserveFace = delSolidKeepFace->isChecked();
            req.reserveMesh = false;
            req.geoIds = m_shape;
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteSolids(req);
			int modelDataID = getCVM->getModelData()->getModelDataID();
            if (res.succeeded())
			{
                //将编辑成功的partID记录
                std::set<int> partIds;
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                if (res.removedObjectIds.size() > 0)
                {
                    for (int i = 0; i < res.removedObjectIds.size(); ++i)
                    {
                        partIds.insert(res.removedObjectIds[i].id);
                    }
                    getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
			}
			else
			{
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
			}
            ///*std::set<int> geoIds = GetTableIdsUseSet();
            //if (geoIds.empty())
            //{
            //    return;
            //}

            //QList<MGeoCommand::EditShape> mEditShapeList;
            //for (int t : geoIds)
            //{
            //    auto set = getCVM->getModelData()->getGeoData()->getPartSetByGeoSolidID(t);
            //    if (nullptr == set)
            //    {
            //        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
            //        return;
            //    }
            //    MGeoCommand::EditShape mEditShape;
            //    mEditShape.mSet = set;
            //    mEditShape.editSolidIDs.insert(t);
            //    mEditShapeList.append(mEditShape);
            //}
            //std::set<int> partIds;
            //mGeoCommandSolidEditOfRemoveSolid* mGeoDelSolid = mGeoCommandFactory::createSolidEditOfRemoveSolid();
            //mGeoDelSolid->setGeoDataBase(getCVM->getModelData()->getGeoData());
            //mGeoDelSolid->reserveFace(delSolidKeepFace->isChecked());
            //mGeoDelSolid->reserveMesh(false);
            //mGeoDelSolid->setModelDataEditShape(mEditShapeList);
            //int modelDataID = getCVM->getModelData()->getModelDataID();
            //if (mGeoDelSolid->handleCommand())
            //{

            //    geoPickWidget->clearHighlight();
            //    clearTableItems(pickObjTable);
            //    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
            //    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);

            //    mGeoDelSolid->getDeletePartIds(partIds);
            //    if (partIds.size() > 0)
            //    {
            //        getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet(partIds);
            //        getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
            //        MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
            //    }
            //    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            //}
            //else
            //{
            //    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
            //}*/

		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::slot_deleteLineAction()
	{
		try
		{
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				return;
			}
            std::vector<int> m_shape;
			for (int t : geoIds)
			{
                m_shape.push_back(t);
			}

            mGeoLineEditOfDeleteRequest req;
            req.name = "lineDelete";
            req.geoIds = m_shape;
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editOfDeleteEdges(req);
            if (res.succeeded())
			{
                //将编辑成功的partID记录
                std::set<int> partIds;
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                if (res.removedObjectIds.size() > 0)
                {
                    for (int i = 0; i < res.removedObjectIds.size(); ++i)
                    {
                        partIds.insert(res.removedObjectIds[i].id);
                    }
                    getCVM->getModelData()->getGeoManager()->deleteGeoDataModelSet_2(partIds);
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(partIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(partIds);
                }
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
			}
			else
			{
                //拓扑边等被引擎拒绝时透出具体原因
                QString errMsg = res.message.empty()
                    ? tr("几何编辑失败!")
                    : QString::fromStdString(res.message);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, errMsg);
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoEditPanel::CollectFaceIsEqualArea()
	{
		std::set<int> selFaceIds = GetTableIdsUseSet();
		if (selFaceIds.empty()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何面!"), true);
			return;
		}

        auto ids = getGEOServer.geoCalculate().getEqualAreaFaceIdByIds(selFaceIds);
		set<int> ret{};
        for (auto i : ids)
		{
			QString name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 2, i); //【OCC摘除·S9】GN 部件名
			bool isVisible = !getCVM->getModelData()->getMeshData()->getGeoPartMask(name); // True是隐藏还是False
			if (isVisible)
				ret.insert(i);
		}
		//std::set<int> highLightElems = mGeoCommandCommFunctionOCC::removeEqualFaces(selFaceIds, ret, getCVM->getModelData()->getGeoData());
		std::vector<int> highLightFaces;
		highLightFaces.insert(highLightFaces.end(), ret.begin(), ret.end());

		clearTableItems(pickObjTable);
		for (auto id : highLightFaces)
		{
			pickObjTable->insertRow(pickObjTable->rowCount());
			pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString("几何面: %1").arg(id)));
		}

		geoPickWidget->_featPick = None;
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, highLightFaces);
		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
		geoPickWidget->_featPick = FeatureObj;
	}

	void mGeoEditPanel::CollectSolidIsEqualVolume()
	{
		std::set<int> selSolidIds = GetTableIdsUseSet();
		if (selSolidIds.empty()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何体!"), true);
			return;
		}

        auto ids = getGEOServer.geoCalculate().getEqualVolumeSolidByIds(selSolidIds);
		set<int> ret{};
		for (auto i : ids)
		{
			QString name = MGeoNeutralQuery::partNameByGeoId((getCVM ? getCVM->getModelData()->getGeoManager() : nullptr), 3, i); //【OCC摘除·S9】GN 部件名
			bool isVisible = !getCVM->getModelData()->getMeshData()->getGeoPartMask(name); // True是隐藏还是False
			if (isVisible)
				ret.insert(i);
		}
		//std::set<int> highLightElems = mGeoCommandCommFunctionOCC::removeEqualShapes(selSolidIds, ret, getCVM->getModelData()->getGeoData());
		std::vector<int> highLightFaces;
		highLightFaces.insert(highLightFaces.end(), ret.begin(), ret.end());

		clearTableItems(pickObjTable);
		for (auto id : highLightFaces)
		{
			pickObjTable->insertRow(pickObjTable->rowCount());
			pickObjTable->setItem(pickObjTable->rowCount() - 1, 0, new QTableWidgetItem(QString("几何体: %1").arg(id)));
		}

		geoPickWidget->_featPick = None;
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, highLightFaces);
		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
		geoPickWidget->_featPick = FeatureObj;
	}

	void mGeoEditPanel::slot_splitLineAction()
	{
		try
		{
			//交点信息
			auto vecSelInfos = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoEdge);

			//源线
			auto geoIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			if (vecSelInfos.empty() || geoIds.empty())
			{
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
				return;
			}

			std::set<int> edgeIds(geoIds.begin(), geoIds.end());

			QHash<int, QVector<QVector3D>> points{};
			for (auto info : vecSelInfos)
			{
				int edgeId = info.id;
				QVector3D pnt = info.intersection_point;
                if (points.contains(edgeId))
				{
					points[edgeId].append(pnt);
				}
				else {
					points.insert(edgeId, QVector<QVector3D>{ pnt });
				}
			}

            std::vector<uint64_t> targets;
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }

            //发送分割线请求
            EventDispatcher::mGeoLineEditOfSplitRequest req;
            req.objtype = GeoNeutralData::GN_ShapeType::Edge;
            req.targets = targets;
            req.splitType = 0;
            req._isOneWaySplit = true;
            req.edgePointHash = points;


            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().lineEditOfSplit(req);

            if (res.succeeded())
            {

                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
			{
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));

				geoPickWidget->clearHighlight();
                getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, vecSelInfos);
			}
			else
			{
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("分割线操作执行失败！"));
			}
		}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

    void mGeoEditPanel::slot_lineLengthenAction()
    {
        try
        {
            std::vector<int> geoIds = GetTableIdsUseVector();
            if (geoIds.empty())
            {
                return;
            }

            double dis = getDefaultValue(Unit::Length, lengthSize->text().toDouble());
            if (fabs(dis) < 1e-10)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
                return;
            }

            //发送线延长请求
            EventDispatcher::mGeoLineEditOfLineLengthenRequest req;
            req.geoId = *geoIds.begin();
            req.length = dis;
            req.isReserve = reverseLengthen->isChecked();
            


            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().lineLengthen(req);

            if (res.succeeded())
            {

                //将编辑成功的partID记录
                for (auto editpart : res.modifiedObjectIds)
            {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));

                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
             
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }
         
        }

        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoEditPanel::slot_faceOffsetAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何面!"), true);
                return;
            }

            double dis = getDefaultValue(Unit::Length, faceOffsetDis->text().toDouble());
            if (fabs(dis) < 1e-10)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
                return;
            }

            //发送线延长请求
            EventDispatcher::mGeoSurfaceEditOfOffsetRequest req;
            req.faceIds = geoIds;
            req.distance = dis;
            req.name = "Model";

            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editSurfaceOfOffsetOCC(req);

            if (res.succeeded())
            {
                if (res.generatedObjects.size() == 2)
                {
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            }
            else
            {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }


         
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoEditPanel::slot_featureProjectionAction()
    {
        try
        {
            // 被操作对象。
            std::vector<uint64_t> targets;
            // 工具
            std::vector<uint64_t> tools;


            std::set<int> geo1Ids = GetTableIdsUseSet();
            tools.reserve(geo1Ids.size());
            for (int id : geo1Ids) {
                tools.push_back(static_cast<uint64_t>(id));
            }
            targets.reserve(spPickTable->rowCount());
            for (int i = 0; i < spPickTable->rowCount(); ++i)
            {
                QString strId = spPickTable->item(i, 0)->text();
                strId = strId.mid(strId.lastIndexOf(":") + 1);
                int id = strId.toInt();
                targets.push_back(static_cast<uint64_t>(id));
            }

            if (targets.empty() || tools.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入不完整!"), true);
                return;
            }

            //发送分割线请求
            EventDispatcher::mGeoProjectRequest req;
            req.targets = targets;
            req.tools = tools;
            if (geoPickWidget->pickObjTypeCombo->currentText() == "几何点")
            {
                if (geoPickWidget_2->pickObjTypeCombo->currentText() == "几何线")
                {
                    req.m_projectType = 0;
                }
                else if (geoPickWidget_2->pickObjTypeCombo->currentText() == "几何面")
                {
                    req.m_projectType = 1;
                }
            }
            else if (geoPickWidget->pickObjTypeCombo->currentText() == "几何线")
            {
                req.m_projectType = 2;
            }
            QVector3D projectVec = extractVectorFromLineEdit(projectionVec->text());
            if (projectVec.isNull())
            {
                req.m_projectDirType = 0;
            }
            else
            {
                req.m_projectDirType = 1;
                for (int i = 0; i < 3; ++i) {
                    req.m_Direction[i] = static_cast<double>(projectVec[i]);
                }

            }
            req.m_isSplit = isSplitProjection->isChecked();
            req.name = "OperateOfProject";
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().project(req);

            if (res.succeeded())
            {

                if (isSplitProjection->isChecked())
                {
                    //将编辑成功的partID记录
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    if (res.generatedObjects.size() == 2)
                    {

                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                        getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                }
                    else
                    {
                        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }
                }

            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }
    void mGeoEditPanel::slot_lineCombineAction()
    {
        try
        {
            if (spLineAId == -1 || spLineBId == -1)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数缺失!"));
                return;
            }


            EventDispatcher::mGeoLineCombineRequest req;

            req.lineId1 = spLineAId;
            req.lineId2 = spLineBId;
            req.isSmooth = isSmooth->isChecked();
            req.name = "LineCombine";


            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().lineCombine(req);
            if (res.succeeded())
            {
                if (res.generatedObjects.size() == 2)
                {
                spLineA->setText("");
                spLineB->setText("");
                geoPickWidget->clearHighlight();
                spLineAId = spLineBId = -1;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功！"));
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            }
            else
            {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }
        }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }
    void mGeoEditPanel::slot_faceExtendAction()
    {
        std::set<int> geoAIds = GetTableIdsUseSet();
        std::set<int> geoBIds;
        std::set<int> toolFaces;
        for (int i = 0; i < spPickTable->rowCount(); ++i)
        {
            QString strId = spPickTable->item(i, 0)->text();
            strId = strId.mid(strId.lastIndexOf(":") + 1);
            int id = strId.toInt();
            geoBIds.insert(id);
            toolFaces.insert(id);
        }
         if (geoAIds.empty() || geoBIds.empty())
         {
             return;
         }

        EventDispatcher::mGeoSurfaceEditOfExtendFaceRequest req;
        req.extendFaceIds = geoAIds;
        req.targetFaceIds = toolFaces;


        EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().editSurfaceOExtendOCC(req);
        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
        {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            geoPickWidget->clearHighlight();
            clearTableItems(pickObjTable);
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }

    }

    void mGeoEditPanel::slot_checkShortEdge()
    {
        double maxLen = getDefaultValue(Unit::Length, shortEdgeMaxLen->text().toDouble());

        if (maxLen <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入正确的参数！"));
            return;
        }

        //获取选择的实体
        std::set<int> ids = GetTableIdsUseSet();

        //获取实体对应的边（新栈几何关系查询）
        std::set<int> edgeIdSet;
        for (const auto id : ids)
        {
            auto related = getGEOServer.geoCalculate().getShapeRelated(id, 3);
            edgeIdSet.insert(related.Edges.begin(), related.Edges.end());
                }
        std::vector<int> edgeIds(edgeIdSet.begin(), edgeIdSet.end());

        //判断边的长度是否达到maxLength
        auto edgeAttr = getGEOServer.geoCalculate().getEdgeAttr(edgeIds);

        std::vector<int> highLightEdges;
        for (const auto attr : edgeAttr)
        {
            if (attr.m_length <= maxLen)
            {
                highLightEdges.push_back(attr.id);
            }
        }

        if (highLightEdges.size() <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的对象！"));
            return;
        }

        getCVM->clearPick();

        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
            SelectionOperate::ADD, SelectionInfoType::GeoEdge, highLightEdges);

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

    }

    void mGeoEditPanel::slot_checkOpenEdge()
    {
        try
        {
            // 开放边 = 全文档中只被一个面引用的边（纯拓扑检查，两后端同语义）。
            std::set<int> geoIds;
            for (int partId : getGEOServer.context().PartManager().getPartList())
            {
                GeoNeutralData::GN_Part part =
                    getGEOServer.context().PartManager().getPartById(partId);
                if (part.metadata().getID() <= 0)
                {
                    continue;
                }
                const auto& edgeFaceMap = part.getEdgeFaceMap();
                for (const auto& entry : edgeFaceMap)
                {
                    if (entry.second.size() == 1 && part.existShape(1, entry.first))
                    {
                        geoIds.insert(entry.first);
                    }
                }
            }

            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的对象！"));
                return;
            }

            MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
            std::vector<int> ids;
            ids.insert(ids.end(), geoIds.begin(), geoIds.end());
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
                SelectionOperate::ADD, SelectionInfoType::GeoEdge, ids);
            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
        }
    }

    void mGeoEditPanel::slot_checkSharpEdge()
    {
       try
       {
           double maxAngle = getDefaultValue(Unit::Angle, sharpEdgeMaxAngle->text().toDouble());
            EventDispatcher::mGeoDetectOfSharpEdgesRequest req;
            req.angleMax = maxAngle / 180 * M_PI;
            req.checkConvex = convex->isChecked();
            req.checkConcave = concave->isChecked();

            EventDispatcher::mGeoQueryResult<EventDispatcher::mGeoDetectOfSharpEdgesResult> res =
                getGEOServer.geoEdit().detectOfSharpEdges(req);

            if (!res.succeeded())
           {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("锐边检测失败：%1").arg(QString::fromStdString(res.message)));
                return;
           }

            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                tr("%1个尖锐边!").arg(res.value.sharpEdgeIds.size()));

            if (res.value.sharpEdgeIds.empty())
           {
                return;
            }

            SelectionInfoType displayType = SelectionInfoType::GeoEdge;
               MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
               std::vector<int> ids;
            ids.insert(ids.end(), res.value.sharpEdgeIds.begin(), res.value.sharpEdgeIds.end());
               getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, displayType, ids);
               MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
               getCVM->getPreRenderWidget()->updatePaint();
           }
       catch (...)
       {
           MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
       }

    }

    void mGeoEditPanel::slot_checkSmallFace()
    {
        bool useArea = useAreaCheckFace->checkState();
        bool useWidth = useWidthCheckFace->checkState();

        double maxArea = smallFaceMaxArea->text().toDouble();
        double maxWidth = getDefaultValue(Unit::Length, smallFaceMaxWidth->text().toDouble());


        if (useArea && maxArea <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入正确的参数！"));
            return;
        }


        if (useWidth && maxWidth <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入正确的参数！"));
            return;
        }


        //获取选择的实体
        std::set<int> ids = GetTableIdsUseSet();

        //获取实体对应的面（新栈几何关系查询）
        std::set<int> faceIdSet;
        for (const auto id : ids)
        {
            auto related = getGEOServer.geoCalculate().getShapeRelated(id, 3);
            faceIdSet.insert(related.Faces.begin(), related.Faces.end());
                }
        std::vector<int> faceIds(faceIdSet.begin(), faceIdSet.end());

        //判断边的长度或者面的面积是否达到要求
        std::vector<int> highLightFaces;
        if (useArea)
        {
            auto faceAttr = getGEOServer.geoCalculate().getFaceAttr(faceIds);

            for (const auto attr : faceAttr)
            {
                if (attr.m_area <= maxArea)
                {
                    highLightFaces.push_back(attr.id);
                }
            }
        }

        if (useWidth)
        {
            //获取面对应的边（新栈几何关系查询）
            for (const auto id : faceIds)
            {
                auto edgeRelated = getGEOServer.geoCalculate().getShapeRelated(id, 2);
                if (edgeRelated.Edges.size() < 3)
                    continue;

                //判断边的长度是否达到maxLength
                auto edgeAttr = getGEOServer.geoCalculate().getEdgeAttr(edgeRelated.Edges);

                for (const auto attr : edgeAttr)
                {
                    if (attr.m_length <= maxWidth)
                    {
                        highLightFaces.push_back(id);
                        break;
                    }
                }
            }
        }

        if (highLightFaces.size() <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的对象！"));
            return;
        }

        getCVM->clearPick();

        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
            SelectionOperate::ADD, SelectionInfoType::GeoFace, highLightFaces);

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

    }

    void mGeoEditPanel::slot_checkTinySolid()
    {
        const double maxVolume = tinySolidMaxVolume->text().toDouble();
        if (maxVolume <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请输入正确的参数！"));
            return;
        }

        //【空窗收口·G5】老链（getPartSet 遍历 + mGeoInfoCalculate 体积缓存）随冻结岛退役，
        // 改 GN 部件枚举（getPartList + geoIdsByPartId）+ 引擎 geoCalculate().getSolidAttr 同步查询
        auto* geoMgr = (getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getGeoManager() : nullptr;
        auto* partMgr = (geoMgr != nullptr) ? geoMgr->getGeoPartMgr() : nullptr;
        std::vector<int> solidIds;
        if (partMgr != nullptr)
        {
            for (int partId : partMgr->getPartList())
            {
                for (int sid : MGeoNeutralQuery::geoIdsByPartId(geoMgr, 3, partId))
                {
                    solidIds.push_back(sid);
                }
            }
        }

        std::vector<int> highLightSolids;
        if (!solidIds.empty())
        {
            const auto solidAttrs = getGEOServer.geoCalculate().getSolidAttr(solidIds);
            for (const auto& attr : solidAttrs)
            {
                if (attr.m_volume > 0 && attr.m_volume <= maxVolume)
                {
                    highLightSolids.push_back(attr.id);
                }
            }
        }

        if (highLightSolids.size() <= 0)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的对象！"));
            return;
        }

        getCVM->clearPick();

        getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(
            SelectionOperate::ADD, SelectionInfoType::GeoSolid, highLightSolids);

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mGeoEditPanel::slot_interferenceCheckAction()
    {
        try
        {
            std::set<int> geoIds = GetTableIdsUseSet();
            if (geoIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何对象!"), true);
                return;
            }

            QString checkType = geoPickWidget->pickObjTypeCombo->currentText();

            EventDispatcher::mGeoInterferenceCheckRequest req;
            req.geoIds.assign(geoIds.begin(), geoIds.end());
            if (checkType == "几何面")
            {
                req.mode = EventDispatcher::mGeoInterferenceCheckMode::FaceVsFace;
            }
            else
            {
                req.mode = EventDispatcher::mGeoInterferenceCheckMode::SolidVsSolid;
            }

            EventDispatcher::mGeoQueryResult<EventDispatcher::mGeoInterferenceCheckResult> res =
                getGEOServer.geoEdit().interferenceCheck(req);

            if (!res.succeeded())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,
                    tr("干涉检查出错：%1").arg(QString::fromStdString(res.message)));
                return;
            }

            //记录干涉对，供移除干涉使用
            m_interferencePairs = res.value.entries;

            if (!res.value.hasInterference)
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0,
                    QString::fromStdString(res.message));
                return;
            }

                geoPickWidget->clearHighlight();
                getCVM->clearPick();

                    if (checkType == "几何体")
                    {
                //红色三角网格高亮干涉体积
                if (!res.value.trianglePoints.isEmpty())
                        {
                    QVector3D color = { 255, 0, 0 };
                    InterferName = QString::fromStdString(getCVM->getPreRenderWidget()->GetBeamSystem()->createTriangleMeshes(res.value.trianglePoints, color));
                        }
                    }
                    else
                    {
                //红色线符号高亮干涉交线
                if (!res.value.segmentPoints.isEmpty())
                {
                        InterferName = "GeoFaceInterferenceCheck";
                    QVector<QVector3D> points = res.value.segmentPoints;
                    std::vector<unsigned int> _lineIndex;
                    _lineIndex.reserve(points.size());
                    for (int i = 0; i < points.size(); ++i)
                        {
                        _lineIndex.push_back(static_cast<unsigned int>(i));
                        }

                        //*******************************通用符号+文字***********************//
                        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                        const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(InterferName);
                        un->_vexter = points;
                        un->_lineIndex = _lineIndex;
                        un->_lineColor = { 255,0,0 };
                        un->_size = 1.0f;
                        un->_lineIsFixedPixel = false;
                        un->_lineDepthState = false;
                        un->_showState = true;
                        symbolController->createSymbol(un);

                        symbolController->UpdateAllSymbol();
                        getCVM->getPreRenderWidget()->updatePaint();
                    }
                }
            }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("干涉检查出错！"));
        }
    }

    void mGeoEditPanel::slot_RemovalInterfer()
    {
        if (!InterferName.isEmpty())
        {
            getCVM->getPreRenderWidget()->GetBeamSystem()->deleteSetSymbol(InterferName);
        }

        if (m_interferencePairs.empty())
            {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("没有需要移除的干涉！"), true);
            return;
        }

        EventDispatcher::mGeoInterferenceRemoveRequest req;
        req.pairs = m_interferencePairs;
        EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().interferenceRemove(req);
        if (res.succeeded())
        {
            //将编辑成功的partID记录
            for (auto editpart : res.modifiedObjectIds)
            {
                getGEOServer.context().PartManager()._editPart.insert(editpart.id);
            }
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            m_interferencePairs.clear();
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
            }
        }
}
