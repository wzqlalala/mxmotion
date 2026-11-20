#include "mGeoRepairPanel.h"
#include "ViewportContainer.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"
//【L2 句柄适配·自持】上游公开面清 include 后传递型缺失，消费方自持完整型
#include "mMeshData.h"

//#include "mGeoSketchAlgorithm.h" //【OCC摘除·S10/P0-1 终局4】死 include（零使用；编译期 OCC 最后一厘米）
#include "mGeoInfoPanel.h"
#include "mInfoWindow.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"


#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"

#include <QMenu>
#include <QDebug>
#include <QtMath>

#include <algorithm>
#include <map>

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】
using namespace mxr;

namespace MechMainWindow
{
    mGeoRepairPanel::mGeoRepairPanel(QWidget* parent)
        : mGeoPanelBase(parent)
    {
        connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            clearTableItems(pickObjTable);
            setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeatureObj;
            geoPickWidget->strPickType = geoPickWidget->pickObjTypeCombo->currentText();
            //if (!InterferName.isEmpty())
            //{
            //    getCVM->getPreRenderWidget()->GetBeamSystem()->deleteSetSymbol(InterferName);
            //    InterferName = "";
            //}
            });

        initRepairSplitEdgeFold();
        initAdditionalEdgeFold();
        initRepeatFaceFold();
        initHardPointFold();
        initDuplicateLinesFold();
        // 拟合曲线功能暂时隐藏；算法与面板实现保留，后续启用时恢复初始化。
        // initFitCurvesFold();
        _mainLayout->addStretch();

        //全局信号（拾取/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接
	}

    void mGeoRepairPanel::connectGeoSignals()
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
    }

    void mGeoRepairPanel::disconnectGeoSignals()
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
    }

    mGeoRepairPanel::~mGeoRepairPanel()
    {

    }

    void mGeoRepairPanel::showFold(GeoOperationType type)
    {
        hideAllFold();
        currentFeature = type;
        geoPickWidget->pickLabel->setText("对象");

        geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->_featPick = FeatureObj;
        pickObjTable->setVisible(true);
        geoPickWidget->setVisible(true);

        if (GeoRepairSplitEdge == type)
        {
            displayAfterInsertFolds({ 0 });
            pickObjTable->setVisible(false);
            //隐藏拾取部件，避免其以独立弹窗形式弹出
            geoPickWidget->setVisible(false);
            //更改拾取模式为符号
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->_featPick = RepairSplitEdge;
            geoPickWidget->strPickType = "分割边";
            autoSplitEdgeSearchAction();
        }
        if (GeoRepairExtraEdge == type)
        {
            displayAfterInsertFolds({ 1 });
            pickObjTable->setVisible(false);
            //隐藏拾取部件，避免其以独立弹窗形式弹出
            geoPickWidget->setVisible(false);
            //更改拾取模式为符号
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->_featPick = ExtraEdges;
            geoPickWidget->strPickType = "额外边";
            autoAdditionalEdgeSearchAction();
        }
        if (GeoRemoveRepeatFace == type)
        {
            displayAfterInsertFolds({ 2 });
            pickObjTable->setVisible(false);
            //隐藏拾取部件，避免其以独立弹窗形式弹出
            geoPickWidget->setVisible(false);
            //更改拾取模式为符号
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->_featPick = DuplicateFaces;
            geoPickWidget->strPickType = "重复面";
            autoRepeatFaceSearchAction();
        }
        if (GeoHardPoint == type)
        {
            hardPointDelete->setCurrentIndex(0);
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(true);
            geoPickWidget->pickObjTypeCombo->clear();
            geoPickWidget->pickObjTypeCombo->addItems({ "几何面" });
            geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
            geoPickWidget->_featPick = AddHardPoint;
            geoPickWidget->strPickType = "几何面";
            m_FoldSections[3]->insertRow(1, pickObjTable);
            m_FoldSections[3]->insertRow(1, geoPickWidget);
            displayAfterInsertFolds({ 3 });
            m_FoldSections[3]->refreshLayout();
        }
        if (GeoRemoveDuplicateLines == type)
        {
            displayAfterInsertFolds({ 4 });
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
            auto viewportPrivate = VMManager->getCurrentViewportPrivate();
            auto render = viewportPrivate->getRendWidget();
            render->setPickFilter(PickFilter::PickSymbol);
            geoPickWidget->_featPick = DuplicateLines;
            geoPickWidget->strPickType = "重复曲线";
            autoDuplicateLinesSearchAction();
        }
        if (GeoFitCurves == type)
        {
            // 拟合曲线暂未开放：隐藏入口后仍保留兜底，避免旧调用路径索引未创建的折叠区。
            clearFitCurveSymbols(true);
            pickObjTable->setVisible(false);
            geoPickWidget->setVisible(false);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("拟合曲线功能暂未开放!"), true);
            return;
        }
        if (GeoInterferenceCheck == type)
        {
            displayAfterInsertFolds({ 4 });
            pickObjTable->setVisible(true);
            geoPickWidget->pickObjTypeCombo->clear();
            geoPickWidget->pickObjTypeCombo->addItems({ "几何面", "几何体" });
            m_FoldSections[4]->insertRow(0, pickObjTable);
            m_FoldSections[4]->insertRow(0, geoPickWidget);
            m_FoldSections[4]->refreshLayout();

            //【GUI 门控】interferenceRemoveBtn 属 QUALITY_REPAIR_V2（未推送），块暂闭
            //if (interferenceRemoveBtn)
            //{
            //    interferenceRemoveBtn->setVisible(geoPickWidget->pickObjTypeCombo->currentText() == "几何体");
            //}
        }
    }
    //初始化分割边界面
    void mGeoRepairPanel::initRepairSplitEdgeFold()
    {
        maxLength = new  MxFunctions::mCalcLineEdit();
        maxLength->setPlaceholderText("不限制");
        minAngle = new  MxFunctions::mCalcLineEdit();
        minAngle->setPlaceholderText("150");
        maxLengthUnit = new QLabel("mm", this);
        minAngleUnit = new QLabel("°", this);
        excludeChkBoxSplitEdge = new QCheckBox("排除", this);
        QPushButton* repairSplitEdgeCancelBtn = new QPushButton("取消", this);
        QPushButton* repairSplitEdgeOkBtn = new QPushButton("确认", this);

        connect(repairSplitEdgeCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(repairSplitEdgeOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_repairSplitEdgeAction);
        FoldSection* repairSplitEdgeFold_1 = new FoldSection("修复分割边", this);

        repairSplitEdgeFold_1->addRow(new PanelLabel("最大长度"), maxLength, maxLengthUnit);
        repairSplitEdgeFold_1->addRow(new PanelLabel("最小角度"), minAngle, minAngleUnit);
        repairSplitEdgeFold_1->addRow(new QLabel(""), excludeChkBoxSplitEdge, "Left");
		QFrame* horizontalLine = createHorizontalLine(repairSplitEdgeFold_1);
		repairSplitEdgeFold_1->addRow(horizontalLine);
		repairSplitEdgeFold_1->addRow(repairSplitEdgeCancelBtn, repairSplitEdgeOkBtn,"Left");

        m_FoldSections.append(repairSplitEdgeFold_1);
        m_foldWidgets.append(repairSplitEdgeFold_1);
        //_mainLayout->addWidget(repairSplitEdgeFold_1);
    }

	//初始化额外边界面
	void mGeoRepairPanel::initAdditionalEdgeFold()
	{
        excludeChkBoxExtraEdge = new QCheckBox("排除", this);
        QPushButton* additionalEdgeCancelBtn = new QPushButton("取消", this);
        QPushButton* additionalEdgeOkBtn = new QPushButton("确认", this);

        connect(additionalEdgeCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(additionalEdgeOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_extraEdgesActionAll);
        FoldSection* additionalEdgeFold_1 = new FoldSection("去除额外边", this);
        additionalEdgeFold_1->addRow(new QLabel(""), excludeChkBoxExtraEdge, "Left");
        QFrame* horizontalLine = createHorizontalLine(additionalEdgeFold_1);
        additionalEdgeFold_1->addRow(horizontalLine);
        additionalEdgeFold_1->addRow(additionalEdgeCancelBtn, additionalEdgeOkBtn, "Left");

        m_FoldSections.append(additionalEdgeFold_1);
        m_foldWidgets.append(additionalEdgeFold_1);
        //_mainLayout->addWidget(additionalEdgeFold_1);
    }

	//初始化重复面界面
	void mGeoRepairPanel::initRepeatFaceFold()
	{
        deleteSolidFace = new QCheckBox("允许从实体中删除面", this);
        excludeChkBoxDuplicateFaces = new QCheckBox("排除", this);
        QPushButton* repeatFaceCancelBtn = new QPushButton("取消", this);
        QPushButton* repeatFaceOkBtn = new QPushButton("确认", this);

        connect(repeatFaceCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(repeatFaceOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_duplicateFacesActionAll);
        FoldSection* repeatFaceFold_1 = new FoldSection("去除重复面", this);
        repeatFaceFold_1->addRow(new QLabel(""), deleteSolidFace, "Left");
        repeatFaceFold_1->addRow(new QLabel(""), excludeChkBoxDuplicateFaces, "Left");
        QFrame* horizontalLine = createHorizontalLine(repeatFaceFold_1);
        repeatFaceFold_1->addRow(horizontalLine);
        repeatFaceFold_1->addRow(repeatFaceCancelBtn, repeatFaceOkBtn, "Left");

        m_FoldSections.append(repeatFaceFold_1);
        m_foldWidgets.append(repeatFaceFold_1);
       // _mainLayout->addWidget(repeatFaceFold_1);
    }

    void mGeoRepairPanel::initHardPointFold()
    {
        QPushButton* hardPointCancelBtn = new QPushButton("取消", this);
        QPushButton* hardPointOkBtn = new QPushButton("确认", this);
        connect(hardPointCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(hardPointOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_hardPointAction);


        hardPointDelete = new QComboBox(this);
        hardPointDelete->addItems({ "点选添加硬点","点选删除硬点","选择几何面删除硬点","删除全部硬点" });
        connect(hardPointDelete, QOverload<int>::of(&QComboBox::activated), [this](int index) {
            geoPickWidget->pickObjTypeCombo->clear();
            if (0 == index)
            {
                geoPickWidget->pickObjTypeCombo->clear();
                geoPickWidget->pickObjTypeCombo->addItem("几何面");
                geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
                geoPickWidget->setVisible(true);
                pickObjTable->setVisible(false);
                geoPickWidget->_featPick = AddHardPoint;
                clearHardPointSymbols(true);
            }
            else if (1 == index)
            {
                geoPickWidget->pickObjTypeCombo->clear();
                geoPickWidget->pickObjTypeCombo->addItem("几何点");
                geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
                geoPickWidget->setVisible(true);
                pickObjTable->setVisible(true);
                geoPickWidget->_featPick = FeatureObj;
                autoHardPointSearchAction();
            }
            else if (2 == index)
            {
                geoPickWidget->pickObjTypeCombo->clear();
                geoPickWidget->pickObjTypeCombo->addItem("几何面");
                geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
                geoPickWidget->setVisible(true);
                pickObjTable->setVisible(true);
                geoPickWidget->_featPick = FeatureObj;
                autoHardPointSearchAction();
            }
            else if (3 == index)
            {
                geoPickWidget->setVisible(false);
                pickObjTable->setVisible(false);
                geoPickWidget->_featPick = FeatureObj;
                autoHardPointSearchAction();
            }
            // 显隐切换后重新计算折叠块高度，避免内容被裁剪/挤压
            m_FoldSections[3]->refreshLayout();
            });

        FoldSection* hardPointFold = new FoldSection("硬点增删", this);
        hardPointFold->addRow(new PanelLabel("硬点增删方式"), hardPointDelete);
        hardPointFold->addRow(hardPointCancelBtn, hardPointOkBtn, "Left");
        m_FoldSections.append(hardPointFold);
        m_foldWidgets.append(hardPointFold);
    }

    //初始化重复线操作界面
    void mGeoRepairPanel::initDuplicateLinesFold()
    {
        QPushButton* duplicateLinesCancelBtn = new QPushButton("取消", this);
        QPushButton* duplicateLinesOkBtn = new QPushButton("确认", this);
        excludeChkBox = new QCheckBox("排除", this);
        excludeChkBox->setChecked(false);
        connect(duplicateLinesCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(duplicateLinesOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_duplicateLinesActionAll);
        FoldSection* duplicateLinesFold = new FoldSection("去除重复线", this);
        duplicateLinesFold->addRow(new QLabel(""), excludeChkBox, "Left");
        duplicateLinesFold->addRow(duplicateLinesCancelBtn, duplicateLinesOkBtn, "Left");

        m_FoldSections.append(duplicateLinesFold);
        m_foldWidgets.append(duplicateLinesFold);
    }

    //初始化拟合线操作界面
    void mGeoRepairPanel::initFitCurvesFold()
    {

        QPushButton* duplicateLinesCancelBtn = new QPushButton("取消", this);
        QPushButton* duplicateLinesOkBtn = new QPushButton("确认", this);

        maxDisFitCurves = new MxFunctions::mCalcLineEdit(this);
        maxDisFitCurves->setExpression("1");
        maxDisFitCurvesUnit = new QLabel("mm", this);

        linesFitCurvesChkBox = new QCheckBox("直线", this);
        linesFitCurvesChkBox->setChecked(true);
        arcsFitCurvesChkBox = new QCheckBox("圆弧", this);
        arcsFitCurvesChkBox->setChecked(true);
        splinesFitCurvesChkBox = new QCheckBox("样条曲线", this);
        splinesFitCurvesChkBox->setChecked(true);
        tangencyFitCurvesChkBox = new QCheckBox("正确相切", this);
        tangencyFitCurvesChkBox->setChecked(true);
        excludeChkBoxFitCurves = new QCheckBox("排除", this);
        excludeChkBoxFitCurves->setChecked(false);

        connect(duplicateLinesCancelBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_onCancel);
        connect(duplicateLinesOkBtn, &QPushButton::clicked, this, &mGeoRepairPanel::slot_fitCurvesActionAll);
        FoldSection* duplicateLinesFold = new FoldSection("拟合曲线", this);
        duplicateLinesFold->addRow(new QLabel("最大距离"), maxDisFitCurves, maxDisFitCurvesUnit);
        duplicateLinesFold->addRow(new QLabel(""), linesFitCurvesChkBox, "Right");
        duplicateLinesFold->addRow(new QLabel(""), arcsFitCurvesChkBox, "Right");
        duplicateLinesFold->addRow(new QLabel(""), splinesFitCurvesChkBox, "Right");
        duplicateLinesFold->addRow(new QLabel(""), tangencyFitCurvesChkBox, "Right");
        duplicateLinesFold->addRow(new QLabel(""), excludeChkBoxFitCurves, "Left");
        duplicateLinesFold->addRow(duplicateLinesCancelBtn, duplicateLinesOkBtn, "Left");

        m_FoldSections.append(duplicateLinesFold);
        m_foldWidgets.append(duplicateLinesFold);
    }

	void mGeoRepairPanel::slot_highlightGeoData()
	{
		QString type = "几何线";
        std::vector<SymbolSelectInfoData> result;
        mxr::UniversalSymbolFont* symbolFont = nullptr;
		int num = 0;
		vector<int> res;
		//获取拾取模式
		PickFilter pickFilter = getCVM->getPreRenderWidget()->getPickFilter();
		auto geoManager = getCVM->getModelData()->getGeoManager();
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
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
            symbolFont = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(name);

            QStringList nameList = name.split("_");
            res.push_back(nameList.last().toInt());
            QString symbolStr = nameList.first();
            if (symbolStr == "DuplicateFacesGroup")
            {
                type = "重复面";
            }
            else if (symbolStr == "SplitEdgeGroup")
            {
                type = "分割边";
            }
            else if (symbolStr == "ExtraEdgeGroup")
            {
                type = "额外边";
            }
            else if (symbolStr == "DuplicateLinesGroup")
            {
                type = "重复曲线";
            }
            else if (symbolStr == "FitCurvesGroup")
            {
                type = "拟合曲线";
            }
            else if (symbolStr == "SplitEdgeGroup")
            {
                type = "分割边";
            }
            else if (symbolStr == "ExtraEdgeGroup")
            {
                type = "额外边";
            }
            break;
        }
		default:
			break;
		}

        if (geoPickWidget->strPickType != type)
        {
            return;
        }

		num = res.size();
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
		}
        else if (geoPickWidget->_featPick == AddHardPoint && type.compare("几何面") == 0)
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
        else if (geoPickWidget->_featPick == DuplicateFaces)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxDuplicateFaces->isChecked())
            {
                if (symbolFont->_faceColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_faceColor = { 0,255,0 };
                }
                else if (symbolFont->_faceColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_faceColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_duplicateFacesActionSingle(res[res.size() - 1]);

            }
        }
        else if (geoPickWidget->_featPick == RepairSplitEdge)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxSplitEdge->isChecked())
            {
                if (symbolFont->_pointColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_pointColor = { 0,255,0 };
                }
                else if (symbolFont->_pointColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_pointColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_repairSplitEdgeSingle(res[res.size() - 1]);

            }
        }
        else if (geoPickWidget->_featPick == ExtraEdges)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxExtraEdge->isChecked())
            {
                if (symbolFont->_lineColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_lineColor = { 0,255,0 };
                }
                else if (symbolFont->_lineColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_lineColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_extraEdgesActionSingle(res[res.size() - 1]);

            }
        }
        else if (geoPickWidget->_featPick == DuplicateLines)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBox->isChecked())
            {
                if (symbolFont->_lineColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_lineColor = { 0,255,0 };
                }
                else if (symbolFont->_lineColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_lineColor = { 255,0,0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_duplicateLinesActionSingle(res[res.size() - 1], 0);
            }
        }
        else if (geoPickWidget->_featPick == FitCurves)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxFitCurves->isChecked())
            {
                if (symbolFont->_lineColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_lineColor = { 0,255,0 };
                }
                else if (symbolFont->_lineColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_lineColor = { 255,0,0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_fitCurvesActionSingle(res[res.size() - 1]);
            }
        }
        else if (geoPickWidget->_featPick == RepairSplitEdge)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxSplitEdge->isChecked())
            {
                if (symbolFont->_pointColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_pointColor = { 0,255,0 };
                }
                else if (symbolFont->_pointColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_pointColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_repairSplitEdgeSingle(res[res.size() - 1]);

            }
        }
        else if (geoPickWidget->_featPick == ExtraEdges)
        {
            if (symbolFont == nullptr)
            {
                return;
            }
            if (excludeChkBoxExtraEdge->isChecked())
            {
                if (symbolFont->_lineColor == QVector3D(255, 0, 0))
                {
                    symbolFont->_lineColor = { 0,255,0 };
                }
                else if (symbolFont->_lineColor == QVector3D(0, 255, 0))
                {
                    symbolFont->_lineColor = { 255, 0, 0 };
                }
                disHighlightSymbol(result.at(0));
            }
            else
            {
                slot_extraEdgesActionSingle(res[res.size() - 1]);

            }
       }
	}

	void mGeoRepairPanel::slot_subWindowActivated()
	{
		//disconnect(getCVM->getPreRenderWidget()->getGeoSelectData());
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoRepairPanel::slot_highlightGeoData);
	}

	
	void mGeoRepairPanel::slot_onCancel()
	{
        std::vector<int> partIds = getCVM->getModelData()->getMeshData()->getAllShowPartIds();
        mInfoWindow::getInstance()->getGeoInfoPanel()->setPartAlpha(partIds, 0.0);
        clearRepairSymbols();
        hideAllFold();
	}

    void mGeoRepairPanel::clearSplitEdgeSymbols(bool updateRender)
    {
        if (splitEdgeGroups.empty())
        {
            return;
        }
        if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
        {
            splitEdgeGroups.clear();
            return;
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController != nullptr)
        {
            vector<QString> symbolNames;
            for (const auto& group : splitEdgeGroups)
            {
                symbolNames.push_back(group.symbolName);
            }
            symbolController->deleteSymbol(symbolNames);
            symbolController->UpdateAllSymbol();
        }
        splitEdgeGroups.clear();
        if (updateRender)
        {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    bool mGeoRepairPanel::executeSplitEdgeRepair(const std::vector<int>& pointIds)
    {
        if (pointIds.empty())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到分割边数据!"));
            return false;
        }

        std::set<int> affectedPartIds;
        for (int pointId : pointIds) {
            GeoNeutralData::GN_Part pointPart =
                getGEOServer.context().PartManager().getPartByGeoPointID(pointId);
            const int partId = pointPart.metadata().getID();
            if (partId > 0) {
                affectedPartIds.insert(partId);
            }
        }

        EventDispatcher::mGeoRepairRequest request;
        request.repairType = EventDispatcher::RepairSplitPoint;
        request.pointIds.insert(pointIds.begin(), pointIds.end());
        request.name = QStringLiteral("RepairSplitEdge");

        // RepairSplitPoint 使用并查集把共享边的分割点连成整条链：
        // “一条边分成三段”的两个内点共享中间边，必须一次合并为一个分量；
        // 专用 repairSplitEdge 仅支持互不相交的两边一组，不能处理该场景。
        EventDispatcher::mGeoOperationResult result =
            getGEOServer.geoEdit().repairOCC(request);
        if (!result.succeeded())
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("几何编辑失败：%1").arg(QString::fromStdString(result.message)));
            return false;
        }

        std::set<int> modifiedPartIds;
        for (const auto& editedPart : result.modifiedObjectIds)
        {
            getGEOServer.context().PartManager()._editPart.insert(editedPart.id);
            modifiedPartIds.insert(editedPart.id);
        }
        if (modifiedPartIds.empty()) {
            modifiedPartIds = affectedPartIds;
        }

        clearSplitEdgeSymbols(true);
        geoPickWidget->clearHighlight();
        clearTableItems(pickObjTable);
        getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
            0, tr("几何编辑成功!"));
        return true;
    }

    void mGeoRepairPanel::slot_repairSplitEdgeAction()
    {
        try
        {
            std::vector<int> pointIds;
            if (!splitEdgeGroups.empty())
            {
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController == nullptr)
                {
                    return;
                }
                for (const auto& group : splitEdgeGroups)
                {
                    const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                        group.symbolName);
                    // 红色=待修复；“排除”勾选后点击符号可切绿，绿色不参与本次修复
                    if (symbol->_pointColor == QVector3D(255, 0, 0))
                    {
                        pointIds.push_back(group.pointId);
                    }
                }
            }
            else
            {
                // 兼容旧入口：符号未建立时直接使用拾取表格中的几何点
                const std::set<int> tableIds = GetTableIdsUseSet();
                pointIds.assign(tableIds.begin(), tableIds.end());
            }

            if (!executeSplitEdgeRepair(pointIds))
            {
                return;
            }
            autoSplitEdgeSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_repairSplitEdgeSingle(int pointId)
    {
        try
        {
            if (pointId <= 0)
            {
                return;
            }
            auto symbolController = getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr
                ? nullptr
                : getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr)
            {
                auto it = std::find_if(splitEdgeGroups.begin(), splitEdgeGroups.end(),
                    [pointId](const SplitEdgeGroupInfo& group) {
                        return group.pointId == pointId;
                    });
                if (it != splitEdgeGroups.end())
                {
                    const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                        it->symbolName);
                    if (symbol->_pointColor != QVector3D(255, 0, 0))
                    {
                        return;
                    }
                }
            }

            if (!executeSplitEdgeRepair({ pointId }))
            {
                return;
            }
            autoSplitEdgeSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::clearAdditionalEdgeSymbols(bool updateRender)
    {
        if (additionalEdgeGroups.empty()) {
            return;
        }
        if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                vector<QString> symbolNames;
                for (const auto& group : additionalEdgeGroups) {
                    symbolNames.push_back(group.symbolName);
                }
                symbolController->deleteSymbol(symbolNames);
                symbolController->UpdateAllSymbol();
            }
        }
        additionalEdgeGroups.clear();
        if (updateRender && getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mGeoRepairPanel::clearDuplicateFaceSymbols(bool updateRender)
    {
        if (duplicateFaceGroups.empty()) {
            return;
        }
        if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                vector<QString> symbolNames;
                for (const auto& group : duplicateFaceGroups) {
                    symbolNames.push_back(group.symbolName);
                }
                symbolController->deleteSymbol(symbolNames);
                symbolController->UpdateAllSymbol();
            }
        }
        duplicateFaceGroups.clear();
        if (updateRender && getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mGeoRepairPanel::clearDuplicateLineSymbols(bool updateRender)
    {
        if (overlapGroups.empty()) {
            return;
        }
        if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                vector<QString> symbolNames;
                for (const auto& group : overlapGroups) {
                    symbolNames.push_back(group.DuplicateLinesGroupName);
                }
                symbolController->deleteSymbol(symbolNames);
                symbolController->UpdateAllSymbol();
            }
        }
        overlapGroups.clear();
        if (updateRender && getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mGeoRepairPanel::clearFitCurveSymbols(bool updateRender)
    {
        if (fitCurveGroups.empty()) {
            return;
        }
        if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                vector<QString> symbolNames;
                for (const auto& group : fitCurveGroups) {
                    symbolNames.push_back(group.symbolName);
                }
                symbolController->deleteSymbol(symbolNames);
                symbolController->UpdateAllSymbol();
            }
        }
        fitCurveGroups.clear();
        if (updateRender && getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mGeoRepairPanel::clearHardPointSymbols(bool updateRender)
    {
        if (hardPointGroups.empty()) {
            return;
        }
        if (getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                vector<QString> symbolNames;
                for (const auto& group : hardPointGroups) {
                    symbolNames.push_back(group.symbolName);
                }
                symbolController->deleteSymbol(symbolNames);
                symbolController->UpdateAllSymbol();
            }
        }
        hardPointGroups.clear();
        if (updateRender && getCVM != nullptr && getCVM->getPreRenderWidget() != nullptr) {
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    bool mGeoRepairPanel::executeAdditionalEdgeRepair(
        const std::vector<int>& edgeIds, const std::set<int>& keepEdgeIds)
    {
        if (edgeIds.empty()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到额外边数据!"));
            return false;
        }

        EventDispatcher::mGeoRepairRequest request;
        request.repairType = EventDispatcher::RepairAdditionalEdge;
        request.edgeIds.insert(edgeIds.begin(), edgeIds.end());
        request.keepEdgeIds = keepEdgeIds;
        request.name = QStringLiteral("RepairAdditionalEdge");

        EventDispatcher::mGeoOperationResult result =
            getGEOServer.geoEdit().repairOCC(request);
        if (!result.succeeded()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("几何编辑失败：%1").arg(QString::fromStdString(result.message)));
            return false;
        }

        for (const auto& editedPart : result.modifiedObjectIds) {
            getGEOServer.context().PartManager()._editPart.insert(editedPart.id);
        }
        std::set<int> deletedPartIds;
        for (const auto& removedPart : result.removedObjectIds) {
            deletedPartIds.insert(removedPart.id);
        }
        if (!deletedPartIds.empty()) {
            getCVM->getModelData()->getMeshData()->deleteGeoPart(deletedPartIds);
            MxFunctions::mGlobalSignals::getInstance()->deletePartSig(deletedPartIds);
        }

        clearAdditionalEdgeSymbols(true);
        geoPickWidget->clearHighlight();
        clearTableItems(pickObjTable);
        if (!result.modifiedObjectIds.empty()) {
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
        return true;
    }

    bool mGeoRepairPanel::executeDuplicateFaceRepair(const std::vector<int>& faceIds)
    {
        if (faceIds.empty()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到重复面数据!"));
            return false;
        }

        EventDispatcher::mGeoRepairRequest request;
        request.repairType = EventDispatcher::RepairDuplicateFace;
        request.faceIds.insert(faceIds.begin(), faceIds.end());
        request.allowRemoveFromSolid = deleteSolidFace != nullptr
            && deleteSolidFace->isChecked();
        request.name = QStringLiteral("RepairDuplicateFace");

        EventDispatcher::mGeoOperationResult result =
            getGEOServer.geoEdit().repairOCC(request);
        if (!result.succeeded()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("几何编辑失败：%1").arg(QString::fromStdString(result.message)));
            return false;
        }

        for (const auto& editedPart : result.modifiedObjectIds) {
            getGEOServer.context().PartManager()._editPart.insert(editedPart.id);
        }
        std::set<int> deletedPartIds;
        for (const auto& removedPart : result.removedObjectIds) {
            deletedPartIds.insert(removedPart.id);
        }
        if (!deletedPartIds.empty()) {
            getCVM->getModelData()->getMeshData()->deleteGeoPart(deletedPartIds);
            MxFunctions::mGlobalSignals::getInstance()->deletePartSig(deletedPartIds);
        }

        clearDuplicateFaceSymbols(true);
        geoPickWidget->clearHighlight();
        clearTableItems(pickObjTable);
        if (!result.modifiedObjectIds.empty()) {
            getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        }
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
        return true;
    }

    bool mGeoRepairPanel::executeFitCurveRepair(const std::vector<int>& groupIds)
    {
        if (groupIds.empty()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到拟合曲线数据!"));
            return false;
        }

        EventDispatcher::mGeoFitCurveFixRequest request;
        request.groupIds = groupIds;
        EventDispatcher::mGeoOperationResult result =
            getGEOServer.geoEdit().fitCurveFixOCC(request);
        if (!result.succeeded()) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                2, tr("几何编辑失败：%1").arg(QString::fromStdString(result.message)));
            return false;
        }

        for (const auto& editedPart : result.modifiedObjectIds) {
            getGEOServer.context().PartManager()._editPart.insert(editedPart.id);
        }
        clearFitCurveSymbols(true);
        geoPickWidget->clearHighlight();
        getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
        MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
        return true;
    }

    void mGeoRepairPanel::clearRepairSymbols()
    {
        clearSplitEdgeSymbols(true);
        clearAdditionalEdgeSymbols(true);
        clearDuplicateFaceSymbols(true);
        clearDuplicateLineSymbols(true);
        clearFitCurveSymbols(true);
        clearHardPointSymbols(true);
    }

    void mGeoRepairPanel::hideEvent(QHideEvent* event)
    {
        clearRepairSymbols();
        mGeoPanelBase::hideEvent(event);
    }

    void mGeoRepairPanel::slot_additionalEdgeAction()
    {
        try {
            if (additionalEdgeGroups.empty()) {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到额外边数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr) {
                return;
            }

            std::vector<int> edgeIds;
            std::set<int> keepEdgeIds;
            for (const auto& group : additionalEdgeGroups) {
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                if (symbol->_lineColor == QVector3D(255, 0, 0)) {
                    edgeIds.push_back(group.itemId);
                }
                else {
                    keepEdgeIds.insert(group.itemId);
                }
            }
            if (!executeAdditionalEdgeRepair(edgeIds, keepEdgeIds)) {
                return;
            }
            autoAdditionalEdgeSearchAction();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_repeatFaceAction()
    {
        try {
            if (duplicateFaceGroups.empty()) {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到重复面数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr) {
                return;
            }

            std::vector<int> faceIds;
            for (const auto& group : duplicateFaceGroups) {
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                if (symbol->_faceColor == QVector3D(255, 0, 0)) {
                    faceIds.push_back(group.itemId);
                }
            }
            if (!executeDuplicateFaceRepair(faceIds)) {
                return;
            }
            autoRepeatFaceSearchAction();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_extraEdgesActionAll()
    {
        slot_additionalEdgeAction();
    }

    void mGeoRepairPanel::slot_extraEdgesActionSingle(int edgeId)
    {
        try {
            if (edgeId <= 0) {
                return;
            }
            auto symbolController = getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr
                ? nullptr
                : getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                const auto it = std::find_if(
                    additionalEdgeGroups.begin(), additionalEdgeGroups.end(),
                    [edgeId](const RepairSymbolInfo& group) {
                        return group.itemId == edgeId;
                    });
                if (it != additionalEdgeGroups.end()) {
                    const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                        it->symbolName);
                    if (symbol->_lineColor != QVector3D(255, 0, 0)) {
                        return;
                    }
                }
            }
            if (!executeAdditionalEdgeRepair({ edgeId }, {})) {
                return;
            }
            autoAdditionalEdgeSearchAction();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_duplicateFacesActionAll()
    {
        slot_repeatFaceAction();
    }

    void mGeoRepairPanel::slot_duplicateFacesActionSingle(int faceId)
    {
        try {
            if (faceId <= 0) {
                return;
            }
            auto symbolController = getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr
                ? nullptr
                : getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr) {
                const auto it = std::find_if(
                    duplicateFaceGroups.begin(), duplicateFaceGroups.end(),
                    [faceId](const RepairSymbolInfo& group) {
                        return group.itemId == faceId;
                    });
                if (it != duplicateFaceGroups.end()) {
                    const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                        it->symbolName);
                    if (symbol->_faceColor != QVector3D(255, 0, 0)) {
                        return;
                    }
                }
            }
            if (!executeDuplicateFaceRepair({ faceId })) {
                return;
            }
            autoRepeatFaceSearchAction();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_duplicateLinesActionAll()
    {
        try
        {
            if (overlapGroups.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到重复线数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            std::vector<int> groupIDs;
            for (auto overlapGroup : overlapGroups)
            {
                const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(overlapGroup.DuplicateLinesGroupName);
                QVector3D lineColor = un->_lineColor;
                if (lineColor == QVector3D(255, 0, 0))
                {
                    groupIDs.push_back(overlapGroup.ID);
                }
            }
            if (groupIDs.size() == 0)
            {
                return;
            }

            EventDispatcher::mGeoDuplicateLineFixRequest req;
            req.groupIds = groupIDs;
            req.actionType = 0;
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().duplicateLineFixOCC(req);
            if (res.succeeded())
            {
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                // 被清空删除的 Part：同步删除模型树/网格/渲染
                std::set<int> deletedPartIds;
                for (auto removedObj : res.removedObjectIds)
                {
                    deletedPartIds.insert(removedObj.id);
                }
                if (!deletedPartIds.empty())
                {
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(deletedPartIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(deletedPartIds);
                }
                geoPickWidget->clearHighlight();
                vector<QString> showSymbol;
                for (auto overlapGroup : overlapGroups)
                {
                    showSymbol.push_back(overlapGroup.DuplicateLinesGroupName);
                }
                symbolController->deleteSymbol(showSymbol);
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();

                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
            autoDuplicateLinesSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_duplicateLinesActionSingle(int groupID, int actionType)
    {
        try
        {
            if (overlapGroups.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到重复线数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            // 通过组ID找到符号名并检查颜色
            QString symbolStr;
            for (auto overlapGroup : overlapGroups)
            {
                if (overlapGroup.ID == groupID)
                {
                    symbolStr = overlapGroup.DuplicateLinesGroupName;
                    break;
                }
            }
            if (symbolStr.isEmpty())
            {
                return;
            }
            const auto& un = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(symbolStr);
            QVector3D lineColor = un->_lineColor;
            if (lineColor != QVector3D(255, 0, 0))
            {
                return;
            }

            EventDispatcher::mGeoDuplicateLineFixRequest req;
            req.groupIds = { groupID };
            req.actionType = 0;
            EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().duplicateLineFixOCC(req);
            if (res.succeeded())
            {
                for (auto editpart : res.modifiedObjectIds)
                {
                    getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                }
                // 被清空删除的 Part：同步删除模型树/网格/渲染
                std::set<int> deletedPartIds;
                for (auto removedObj : res.removedObjectIds)
                {
                    deletedPartIds.insert(removedObj.id);
                }
                if (!deletedPartIds.empty())
                {
                    getCVM->getModelData()->getMeshData()->deleteGeoPart(deletedPartIds);
                    MxFunctions::mGlobalSignals::getInstance()->deletePartSig(deletedPartIds);
                }
                geoPickWidget->clearHighlight();
                vector<QString> showSymbol;
                for (auto overlapGroup : overlapGroups)
                {
                    showSymbol.push_back(overlapGroup.DuplicateLinesGroupName);
                }
                symbolController->deleteSymbol(showSymbol);
                symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();

                getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败！"));
            }
            autoDuplicateLinesSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_fitCurvesActionAll()
    {
        try
        {
            if (fitCurveGroups.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到拟合曲线数据!"));
                return;
            }
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr)
            {
                return;
            }

            std::vector<int> groupIds;
            for (const auto& group : fitCurveGroups)
            {
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                if (symbol->_lineColor == QVector3D(255, 0, 0))
                {
                    groupIds.push_back(group.itemId);
                }
            }
            if (!executeFitCurveRepair(groupIds))
            {
                return;
            }
            autoFitCurvesSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_fitCurvesActionSingle(int groupId)
    {
        try
        {
            if (groupId <= 0)
            {
                return;
            }
            auto symbolController = getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr
                ? nullptr
                : getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController != nullptr)
            {
                const auto it = std::find_if(
                    fitCurveGroups.begin(), fitCurveGroups.end(),
                    [groupId](const RepairSymbolInfo& group) {
                        return group.itemId == groupId;
                    });
                if (it != fitCurveGroups.end())
                {
                    const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                        it->symbolName);
                    if (symbol->_lineColor != QVector3D(255, 0, 0))
                    {
                        return;
                    }
                }
            }
            if (!executeFitCurveRepair({ groupId }))
            {
                return;
            }
            autoFitCurvesSearchAction();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    void mGeoRepairPanel::slot_hardPointAction()
    {

        try
        {
            const int actionType = hardPointDelete->currentIndex();
            std::set<int> geoIds = GetTableIdsUseSet();

            if (actionType == 0)
            {
                //交点信息
                auto vecSelInfos = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getMoreSelectDataByType(SelectionInfoType::GeoFace);

                //源面
                auto geoFaceIds = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
                if (vecSelInfos.empty() || geoFaceIds.empty())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何参数输入错误!"));
                    return;
                }

                // 收集面ID -> 硬点坐标
                std::map<int, std::vector<QVector3D>> facePoints;
                for (auto info : vecSelInfos)
                {
                    QVector3D pnt = info.intersection_point;
                    facePoints[info.id].push_back(pnt);
                }

                EventDispatcher::mGeoHardPointRequest req;
                req.opType = EventDispatcher::HardPointAdd;
                req.facePoints = facePoints;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().hardPointOCC(req);
                if (res.succeeded())
                {
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdateMorePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, vecSelInfos);
                    geoPickWidget->clearHighlight();
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            else if (actionType == 1)
            {
                // 按点删除硬点
                if (geoIds.empty())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何点!"), true);
                    return;
                }
                EventDispatcher::mGeoHardPointRequest req;
                req.opType = EventDispatcher::HardPointRemoveByPoint;
                req.pointIds = geoIds;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().hardPointOCC(req);
                if (res.succeeded())
                {
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    geoPickWidget->clearHighlight();
                    clearTableItems(pickObjTable);
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            else if (actionType == 2)
            {
                // 按面删除硬点
                if (geoIds.empty())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请选择几何面!"), true);
                    return;
                }
                EventDispatcher::mGeoHardPointRequest req;
                req.opType = EventDispatcher::HardPointRemoveByFace;
                req.faceIds = geoIds;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().hardPointOCC(req);
                if (res.succeeded())
                {
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    geoPickWidget->clearHighlight();
                    clearTableItems(pickObjTable);
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            else if (actionType == 3)
            {
                // 删除全部硬点
                EventDispatcher::mGeoHardPointRequest req;
                req.opType = EventDispatcher::HardPointRemoveAll;

                EventDispatcher::mGeoOperationResult res = getGEOServer.geoEdit().hardPointOCC(req);
                if (res.succeeded())
                {
                    for (auto editpart : res.modifiedObjectIds)
                    {
                        getGEOServer.context().PartManager()._editPart.insert(editpart.id);
                    }
                    geoPickWidget->clearHighlight();
                    clearTableItems(pickObjTable);
                    getCVM->getModelData()->getGeoManager()->syncAfterShapeEdit();
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                else
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何编辑失败!"));
                }
            }
            if (actionType != 0)
            {
                autoHardPointSearchAction();
            }
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
        }
    }

    bool mGeoRepairPanel::autoSplitEdgeSearchAction()
    {
        try
        {
            clearSplitEdgeSymbols(true);
            geoPickWidget->clearHighlight();

            auto inputValue = [](MxFunctions::mCalcLineEdit* edit, double fallback) -> double {
                const QString text = edit->text().trimmed();
                bool ok = false;
                const double value = text.toDouble(&ok);
                return text.isEmpty() || !ok ? fallback : value;
            };

            EventDispatcher::mGeoRepairSearchRequest searchRequest;
            searchRequest.searchSplitPoints = true;

            // 空值不套用 0.1 placeholder：盒体边均分后的段长通常远大于 0.1，
            // 误把 placeholder 当阈值会把手工分割点全部过滤掉。空值=不限制长度。
            const double maxLengthInput = inputValue(maxLength, -1.0);
            searchRequest.maxEdgeLength = maxLengthInput > 0.0
                ? getDefaultValue(Unit::Length, maxLengthInput)
                : -1.0;

            // 本面板角度控件固定为“°”；OCC checkSplitPoints 的 minAngle 参数为弧度
            searchRequest.minAngle = qDegreesToRadians(inputValue(minAngle, 150.0));

            EventDispatcher::mGeoRepairSearchResult searchResult =
                getGEOServer.geoEdit().repairSearchOCC(searchRequest);
            if (!searchResult.success || searchResult.pointIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的几何!"));
                return false;
            }

            std::vector<int> pointIds(searchResult.pointIds.begin(), searchResult.pointIds.end());
            const std::vector<GeoNeutralData::GeoVertexAttr> vertexAttrs =
                getGEOServer.geoCalculate().CalculateVertexAttr(pointIds);
            std::map<int, QVector3D> coordinates;
            for (const auto& attr : vertexAttrs)
            {
                if (attr.id > 0)
                {
                    coordinates[attr.id] = attr.m_coor;
                }
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr)
            {
                return false;
            }

            for (int pointId : pointIds)
            {
                const auto coordIt = coordinates.find(pointId);
                if (coordIt == coordinates.end())
                {
                    continue;
                }

                SplitEdgeGroupInfo group;
                group.pointId = pointId;
                group.symbolName = QString("SplitEdgeGroup_%1").arg(pointId);

                QVector<QVector3D> points;
                points.push_back(coordIt->second);
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                symbol->_vexter = points;
                symbol->_pointIndex_size = { { 0, 0.5 } };
                symbol->_pointLight = true;
                symbol->_pointColor = { 255, 0, 0 };
                symbol->_size = 1.0f;
                symbol->_pointDepthState = false;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);

                splitEdgeGroups.push_back(group);
            }

            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !splitEdgeGroups.empty();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
            return false;
        }
    }
    bool mGeoRepairPanel::autoAdditionalEdgeSearchAction()
    {
        try {
            clearAdditionalEdgeSymbols(true);
            geoPickWidget->clearHighlight();

            EventDispatcher::mGeoRepairSearchRequest searchRequest;
            searchRequest.searchAdditionalEdges = true;
            EventDispatcher::mGeoRepairSearchResult searchResult =
                getGEOServer.geoEdit().repairSearchOCC(searchRequest);
            if (!searchResult.success || searchResult.edgeIds.empty()) {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的几何!"));
                return false;
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr) {
                return false;
            }

            for (int edgeId : searchResult.edgeIds) {
                GeoNeutralData::GN_Part part =
                    getGEOServer.context().PartManager().getPartByGeoLineID(edgeId);
                const auto& mesh = part.getGeoDiscreteData();
                QVector<QVector3D> points;
                std::vector<unsigned int> lineIndex;
                const auto edgeIt = mesh.edgePoints.find(edgeId);
                if (edgeIt != mesh.edgePoints.end()) {
                    std::map<int, int> globalToLocal;
                    auto appendPoint = [&](int globalIndex) -> int {
                        const auto pointIt = mesh.globalPoints.find(globalIndex);
                        if (pointIt == mesh.globalPoints.end()) {
                            return -1;
                        }
                        const auto localIt = globalToLocal.find(globalIndex);
                        if (localIt != globalToLocal.end()) {
                            return localIt->second;
                        }
                        const int localIndex = points.size();
                        points.push_back(QVector3D(
                            pointIt->second.x(), pointIt->second.y(), pointIt->second.z()));
                        globalToLocal[globalIndex] = localIndex;
                        return localIndex;
                    };
                    for (int globalIndex : edgeIt->second) {
                        appendPoint(globalIndex);
                    }
                    for (int index = 0; index + 1 < points.size(); ++index) {
                        lineIndex.push_back(static_cast<unsigned int>(index));
                        lineIndex.push_back(static_cast<unsigned int>(index + 1));
                    }
                }

                // 离散数据缺失时退化为端点直线，保证识别结果仍可交互。
                if (points.size() < 2) {
                    const std::vector<GeoNeutralData::GeoEdgeAttr> attrs =
                        getGEOServer.geoCalculate().CalculateEdgeAttr({ edgeId });
                    if (attrs.size() == 1) {
                        points.clear();
                        lineIndex.clear();
                        points.push_back(attrs.front().m_startPoint);
                        points.push_back(attrs.front().m_endPoint);
                        lineIndex.push_back(0);
                        lineIndex.push_back(1);
                    }
                }
                if (points.size() < 2) {
                    continue;
                }

                RepairSymbolInfo group;
                group.itemId = edgeId;
                group.symbolName = QString("ExtraEdgeGroup_%1").arg(edgeId);
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                symbol->_vexter = points;
                symbol->_lineIndex = lineIndex;
                symbol->_lineColor = { 255, 0, 0 };
                symbol->_size = 1.0f;
                symbol->_lineIsFixedPixel = false;
                symbol->_lineDepthState = false;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);
                additionalEdgeGroups.push_back(group);
            }

            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !additionalEdgeGroups.empty();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
            return false;
        }
    }

    bool mGeoRepairPanel::autoRepeatFaceSearchAction()
    {
        try {
            clearDuplicateFaceSymbols(true);
            geoPickWidget->clearHighlight();

            EventDispatcher::mGeoRepairSearchRequest searchRequest;
            searchRequest.searchDuplicateFaces = true;
            EventDispatcher::mGeoRepairSearchResult searchResult =
                getGEOServer.geoEdit().repairSearchOCC(searchRequest);
            if (!searchResult.success || searchResult.faceIds.empty()) {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的几何!"));
                return false;
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr) {
                return false;
            }

            // repairSearchOCC 返回成对重复面的扁平 ID。GUI 默认每组保留一个：
            // 首个红色提交删除，其余绿色保留；勾选“排除”后可点击切换红绿。
            std::map<QString, int> seenDuplicateGroups;
            for (int faceId : searchResult.faceIds) {
                GeoNeutralData::GN_Part part =
                    getGEOServer.context().PartManager().getPartByGeoFaceID(faceId);
                const auto& mesh = part.getGeoDiscreteData();
                QVector<QVector3D> points;
                std::vector<unsigned int> faceIndex;
                const auto faceIt = mesh.facePoints.find(faceId);
                if (faceIt != mesh.facePoints.end()) {
                    std::map<int, int> globalToLocal;
                    auto appendPoint = [&](int globalIndex) -> int {
                        const auto pointIt = mesh.globalPoints.find(globalIndex);
                        if (pointIt == mesh.globalPoints.end()) {
                            return -1;
                        }
                        const auto localIt = globalToLocal.find(globalIndex);
                        if (localIt != globalToLocal.end()) {
                            return localIt->second;
                        }
                        const int localIndex = points.size();
                        points.push_back(QVector3D(
                            pointIt->second.x(), pointIt->second.y(), pointIt->second.z()));
                        globalToLocal[globalIndex] = localIndex;
                        return localIndex;
                    };
                    for (int globalIndex : faceIt->second) {
                        const int localIndex = appendPoint(globalIndex);
                        if (localIndex >= 0) {
                            faceIndex.push_back(static_cast<unsigned int>(localIndex));
                        }
                    }
                }
                if (points.empty() || faceIndex.size() < 3) {
                    continue;
                }

                const std::vector<GeoNeutralData::GeoFaceAttr> attrs =
                    getGEOServer.geoCalculate().CalculateFaceAttr({ faceId });
                QString groupKey;
                if (attrs.size() == 1) {
                    const auto quantize = [](double value) {
                        return QString::number(std::round(value * 1000.0), 'f', 0);
                    };
                    groupKey = quantize(attrs.front().m_coor.x()) + QLatin1Char(',')
                        + quantize(attrs.front().m_coor.y()) + QLatin1Char(',')
                        + quantize(attrs.front().m_coor.z()) + QLatin1Char(',')
                        + quantize(attrs.front().m_dir.x()) + QLatin1Char(',')
                        + quantize(attrs.front().m_dir.y()) + QLatin1Char(',')
                        + quantize(attrs.front().m_dir.z()) + QLatin1Char(',')
                        + quantize(attrs.front().m_area);
                }
                const bool primaryFace = groupKey.isEmpty()
                    || seenDuplicateGroups.insert({ groupKey, faceId }).second;

                RepairSymbolInfo group;
                group.itemId = faceId;
                group.symbolName = QString("DuplicateFacesGroup_%1").arg(faceId);
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    group.symbolName);
                symbol->_vexter = points;
                symbol->_faceIndex = faceIndex;
                symbol->_faceColor = primaryFace
                    ? QVector3D(255, 0, 0) : QVector3D(0, 255, 0);
                symbol->_size = 1.0f;
                symbol->_faceIsFixedPixel = false;
                symbol->_faceDepthState = true;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);
                duplicateFaceGroups.push_back(group);
            }

            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !duplicateFaceGroups.empty();
        }
        catch (...) {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("未找到符合条件的几何！"));
            return false;
        }
    }

    bool mGeoRepairPanel::autoDuplicateLinesSearchAction()
    {
        try
        {
            clearDuplicateLineSymbols(true);
            geoPickWidget->clearHighlight();

            EventDispatcher::mGeoDuplicateLineSearchResult searchResult =
                getGEOServer.geoEdit().duplicateLineSearchOCC();
            if (!searchResult.success || searchResult.groups.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的几何!"));
                return false;
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr)
            {
                return false;
            }
            for (const auto& group : searchResult.groups)
            {
                if (group.highlightPoints.size() < 2)
                {
                    continue;
                }

                OverlapGroupInfo info;
                info.ID = group.groupId;
                info.DuplicateLinesGroupName =
                    QString("DuplicateLinesGroup_%1").arg(group.groupId);
                std::vector<unsigned int> lineIndex;
                for (size_t index = 0; index + 1 < group.highlightPoints.size(); ++index)
                {
                    lineIndex.push_back(static_cast<unsigned int>(index));
                    lineIndex.push_back(static_cast<unsigned int>(index + 1));
                }
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    info.DuplicateLinesGroupName);
                symbol->_vexter = QVector<QVector3D>::fromStdVector(group.highlightPoints);
                symbol->_lineIndex = lineIndex;
                symbol->_lineColor = { 255, 0, 0 };
                symbol->_size = 1.0f;
                symbol->_lineIsFixedPixel = false;
                symbol->_lineDepthState = false;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);
                overlapGroups.push_back(info);
            }
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !overlapGroups.empty();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("未找到符合条件的几何！"));
            return false;
        }
    }

    bool mGeoRepairPanel::autoFitCurvesSearchAction()
    {
        try
        {
            clearFitCurveSymbols(true);
            geoPickWidget->clearHighlight();

            EventDispatcher::mGeoFitCurveSearchRequest request;
            request.maxDistance = getDefaultValue(
                Unit::Length, maxDisFitCurves->text().toDouble());
            request.allowLines = linesFitCurvesChkBox != nullptr
                && linesFitCurvesChkBox->isChecked();
            request.allowArcs = arcsFitCurvesChkBox != nullptr
                && arcsFitCurvesChkBox->isChecked();
            request.allowSplines = splinesFitCurvesChkBox != nullptr
                && splinesFitCurvesChkBox->isChecked();
            request.requireTangency = tangencyFitCurvesChkBox != nullptr
                && tangencyFitCurvesChkBox->isChecked();

            EventDispatcher::mGeoFitCurveSearchResult searchResult =
                getGEOServer.geoEdit().fitCurveSearchOCC(request);
            if (!searchResult.success || searchResult.groups.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到符合条件的几何!"));
                return false;
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr)
            {
                return false;
            }
            for (const auto& group : searchResult.groups)
            {
                if (group.highlightPoints.size() < 2)
                {
                    continue;
                }

                RepairSymbolInfo info;
                info.itemId = group.groupId;
                info.symbolName = QString("FitCurvesGroup_%1").arg(group.groupId);
                std::vector<unsigned int> lineIndex;
                for (size_t index = 0; index + 1 < group.highlightPoints.size(); ++index)
                {
                    lineIndex.push_back(static_cast<unsigned int>(index));
                    lineIndex.push_back(static_cast<unsigned int>(index + 1));
                }
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    info.symbolName);
                symbol->_vexter = QVector<QVector3D>::fromStdVector(group.highlightPoints);
                symbol->_lineIndex = lineIndex;
                symbol->_lineColor = { 255, 0, 0 };
                symbol->_size = 1.0f;
                symbol->_lineIsFixedPixel = false;
                symbol->_lineDepthState = false;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);
                fitCurveGroups.push_back(info);
            }
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !fitCurveGroups.empty();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("未找到符合条件的几何！"));
            return false;
        }
    }

    bool mGeoRepairPanel::autoHardPointSearchAction()
    {
        try
        {
            clearHardPointSymbols(true);
            EventDispatcher::mGeoRepairSearchRequest request;
            request.searchHardPoints = true;
            EventDispatcher::mGeoRepairSearchResult searchResult =
                getGEOServer.geoEdit().repairSearchOCC(request);
            if (!searchResult.success || searchResult.pointIds.empty())
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                    1, tr("未找到硬点!"));
                return false;
            }

            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController == nullptr)
            {
                return false;
            }
            std::vector<int> pointIds(
                searchResult.pointIds.begin(), searchResult.pointIds.end());
            const std::vector<GeoNeutralData::GeoVertexAttr> attributes =
                getGEOServer.geoCalculate().CalculateVertexAttr(pointIds);
            for (const GeoNeutralData::GeoVertexAttr& attribute : attributes)
            {
                if (attribute.id <= 0)
                {
                    continue;
                }
                RepairSymbolInfo info;
                info.itemId = attribute.id;
                info.symbolName = QString("HardPointGroup_%1").arg(attribute.id);
                QVector<QVector3D> points;
                points.push_back(attribute.m_coor);
                const auto& symbol = symbolController->getOrCreateSymbolData<UniversalSymbolFont>(
                    info.symbolName);
                symbol->_vexter = points;
                symbol->_pointIndex_size = { { 0, 0.5 } };
                symbol->_pointLight = true;
                symbol->_pointColor = { 255, 0, 0 };
                symbol->_size = 1.0f;
                symbol->_pointDepthState = false;
                symbol->_showState = true;
                symbolController->createSymbol(symbol);
                hardPointGroups.push_back(info);
            }
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
            return !hardPointGroups.empty();
        }
        catch (...)
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(
                1, tr("未找到硬点！"));
            return false;
        }
    }
}
