#include "mGeoExtractPanel.h"
#include "UiMetrics.h"
#include "ViewportContainer.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"
#include "mMeshData.h"
#include "mGeoManager.h"
#include "mGeoManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
 

#include "mViewportModelManager.h"
#include "mViewportModel.h"

#include <QMenu>
#include "mUiMenu.h"
#include <QTimer>
#include <qsignalmapper.h>
//#include <mGeoCommandFactory.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
#include "PickCommonFunction.h"
#include "mOtherFunction.h"

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】随死 include 下线
using namespace mxr;
namespace MechMainWindow
{
	mGeoExtractPanel::mGeoExtractPanel(QWidget* parent)
		: mGeoPanelBase(parent)
	{
        geoPickWidget_2 = new GeoPickWidget(this);
        geoPickWidget_2->_featPick = None;

		connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
            clearTableItems(pickObjTable);
			geoPickWidget->_featPick = FeatureObj;
			});

		connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
			geoPickWidget->_featPick = FeatureObj;
			});

        connect(geoPickWidget_2->pickBtn, &QToolButton::clicked, this, [this]() {
            geoPickWidget->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
            setGeoHighlight(pickObjTable_2, geoPickWidget_2->pickObjTypeCombo->currentText());
            geoPickWidget->_featPick = FeaturePick::Fluid;
            });

       
        connect(geoPickWidget_2->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            geoPickWidget_2->_featPick = None;
            geoPickWidget->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
            clearTableItems(pickObjTable_2);
            geoPickWidget->_featPick = FeaturePick::Fluid;
            });


        pickObjTable_2 = new QTableWidget(this);
        pickObjTable_2->setMaximumHeight(100);
        pickObjTable_2->setMinimumHeight(80);
        pickObjTable_2->verticalHeader()->setHidden(true);
        pickObjTable_2->setColumnCount(1);
        QStringList horizontalHeader = { "几何ID" };
        pickObjTable_2->setHorizontalHeaderLabels(horizontalHeader);
        pickObjTable_2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        pickObjTable_2->horizontalHeader()->setHighlightSections(false);
        pickObjTable_2->setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 10));
        pickObjTable_2->verticalHeader()->setDefaultSectionSize(20);
        pickObjTable_2->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
        pickObjTable_2->setEditTriggers(QAbstractItemView::NoEditTriggers);
        connect(pickObjTable_2, &QTableWidget::customContextMenuRequested, this, &mGeoExtractPanel::slot_contextMenuTableData);


        initMidFacePanel();
        initFluidPanel();
        initMidLinePanel();
        initBoundaryFacePanel();


        _mainLayout->addStretch();

        //全局信号（拾取/单位制/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接

        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(midDisUnit, midFaceOffsetDis));
        m_LengthLabels.append(QPair<QLabel*, mCalcLineEdit*>(midLimitValUnit, midFaceFillMax));

        m_AngleLabels.append(QPair<QLabel*, mCalcLineEdit*>(midAngleUnit, midFaceAngle));
	}

	void mGeoExtractPanel::connectGeoSignals()
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

	void mGeoExtractPanel::disconnectGeoSignals()
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

	mGeoExtractPanel::~mGeoExtractPanel()
	{

	}


	void mGeoExtractPanel::showFold(GeoOperationType type)
	{
		// 进入编辑时连接单位制切换信号（退出编辑由mInfoWindow::uneditGeometry统一断开）
		connectUnitSystemResponse();

		hideAllFold();
		
		geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget_2->setVisible(false);
        pickObjTable_2->setVisible(false);
		if (GeoMidFace == type)
		{
			geoPickWidget->pickObjTypeCombo->addItems({ "几何体","几何面" });
			geoPickWidget->_featPick = FeatureObj;
			m_FoldSections[0]->insertRow(0, pickObjTable);
			m_FoldSections[0]->insertRow(0, geoPickWidget);
			displayAfterInsertFolds({ 0 });
		}
        if (GeoFluid == type)
        {
            geoPickWidget_2->setVisible(true);
            pickObjTable_2->setVisible(true);
            geoPickWidget->pickObjTypeCombo->addItems({ "封闭区域面","封闭区域边"/*, "封顶剖面"*/});
            geoPickWidget->_featPick = FeatureObj;
            geoPickWidget_2->pickObjTypeCombo->clear();
            geoPickWidget_2->pickObjTypeCombo->addItems({ "封闭体积内部面" });
            m_FoldSections[1]->insertRow(0, pickObjTable);
            m_FoldSections[1]->insertRow(0, geoPickWidget);
            m_FoldSections[1]->insertRow(0, pickObjTable_2);
            m_FoldSections[1]->insertRow(0, geoPickWidget_2);
            displayAfterInsertFolds({ 1 });
        }
        if (GeoMidLine == type)
        {
            geoPickWidget_2->setVisible(true);
            pickObjTable_2->setVisible(true);
            geoPickWidget->pickObjTypeCombo->addItems({ "几何线" });
            geoPickWidget->_featPick = FeatureObj;
            geoPickWidget_2->pickObjTypeCombo->clear();
            geoPickWidget_2->pickObjTypeCombo->addItems({ "几何线" });
            m_FoldSections[2]->insertRow(0, pickObjTable_2);
            m_FoldSections[2]->insertRow(0, geoPickWidget_2);
            m_FoldSections[2]->insertRow(0, pickObjTable);
            m_FoldSections[2]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 2 });
        }
        if (GeoBoundaryFace == type)
        {
            geoPickWidget->pickObjTypeCombo->addItems({ "几何体"});
            geoPickWidget->_featPick = FeatureObj;

            m_FoldSections[3]->insertRow(0, pickObjTable);
            m_FoldSections[3]->insertRow(0, geoPickWidget);
            displayAfterInsertFolds({ 3 });
        }
		geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
	}

	void MechMainWindow::mGeoExtractPanel::initMidFacePanel()
	{
		//中面
		midFaceFuncComb = new QComboBox(this);
		midFaceFuncComb->addItems({ "表面偏置"/*,"生成网格"*/ });
		offsetDisComb = new QComboBox(this);
		offsetDisComb->addItems({ "自动","手动" });
		midFaceMeshSize = new MxFunctions::mCalcLineEdit(this); midFaceMeshSize->setExpression("2.00");
		midFaceOffsetDis = new MxFunctions::mCalcLineEdit(this); midFaceOffsetDis->setExpression("0.50");
		midFaceAngle = new MxFunctions::mCalcLineEdit(this); midFaceAngle->setExpression("170.00");
		midFaceFillMax = new MxFunctions::mCalcLineEdit(this); midFaceFillMax->setExpression("0.10");
		midFaceSecBar = new QCheckBox("型材抽中面", this);

		QLabel* meshLabe = new PanelLabel("网格尺寸", this);
		QLabel* offsetLabe = new PanelLabel("偏置距离", this);
		QLabel* offsetLabe2 = new PanelLabel("偏置距离", this);

        midDisUnit = new QLabel("mm", this);
        midAngleUnit = new QLabel("°", this);
        midLimitValUnit = new QLabel("mm", this);

		QPushButton* extractMidFaceCancelBtn = new QPushButton("取消", this);
		extractMidFaceeOkBtn = new QPushButton("确认", this);
		connect(extractMidFaceCancelBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onCancel);
		connect(extractMidFaceeOkBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onMidFaceActive);

		FoldSection* extractMidFaceFold = new FoldSection("提取中面", this);
		extractMidFaceFold->addRow(new PanelLabel("中面方法"), midFaceFuncComb);
		extractMidFaceFold->addRow(meshLabe, midFaceMeshSize);
		extractMidFaceFold->addRow(offsetLabe, offsetDisComb);
		extractMidFaceFold->addRow(offsetLabe2, midFaceOffsetDis, midDisUnit);

		extractMidFaceFold->addRow(new PanelLabel("端面角度"), midFaceAngle, midAngleUnit);
		extractMidFaceFold->addRow(new PanelLabel("面缝合阈值"), midFaceFillMax, midLimitValUnit);
		extractMidFaceFold->addRow(midFaceSecBar);
		QFrame* hLine4 = createHorizontalLine(extractMidFaceFold);
		extractMidFaceFold->addRow(hLine4);
		extractMidFaceFold->addRow(extractMidFaceCancelBtn, extractMidFaceeOkBtn, "Left");

		
		meshLabe->setVisible(false);
		offsetLabe2->setVisible(false);
		midFaceMeshSize->setVisible(false);
		midFaceOffsetDis->setVisible(false);
        midDisUnit->setVisible(false);

		m_FoldSections.append(extractMidFaceFold);
		m_foldWidgets.append(extractMidFaceFold);

		connect(midFaceFuncComb, QOverload<int>::of(&QComboBox::activated), [this, extractMidFaceFold, offsetLabe, meshLabe, offsetLabe2](int index) {
			meshLabe->setVisible(false);
			midFaceMeshSize->setVisible(false);
			getCVM->getModelData()->getMeshData()->setMeshMidface(false);
			if (index == 1) 
			{
				meshLabe->setVisible(true);
				midFaceMeshSize->setVisible(true);
				getCVM->getModelData()->getMeshData()->setMeshMidface(true);
			}
			extractMidFaceFold->refreshLayout();
			});

		connect(offsetDisComb, QOverload<int>::of(&QComboBox::activated), [this, extractMidFaceFold, offsetLabe2](int index) {
			offsetLabe2->setVisible(false);
			midFaceOffsetDis->setVisible(false);
            midDisUnit->setVisible(false);
			if (index == 0) {
				
			}
			else {
				offsetLabe2->setVisible(true);
				midFaceOffsetDis->setVisible(true);
                midDisUnit->setVisible(true);
			}
			extractMidFaceFold->refreshLayout();
			});
	}

    void mGeoExtractPanel::initMidLinePanel()
    {
        
        subDelAction = new QAction("删除");
        QPushButton* extractMidLineCancelBtn = new QPushButton("取消", this);
        QPushButton* extractMidLineOkBtn = new QPushButton("确认", this);
        connect(extractMidLineCancelBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onCancel);
        connect(extractMidLineOkBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onMidLineActive);

        FoldSection* extractMidLineFold = new FoldSection("提取中线", this);
        extractMidLineFold->addRow(extractMidLineCancelBtn, extractMidLineOkBtn, "Left");
        m_FoldSections.append(extractMidLineFold);
        m_foldWidgets.append(extractMidLineFold);
    }

    void mGeoExtractPanel::initBoundaryFacePanel()
    {
        QPushButton* extractBoundaryFaceCancelBtn = new QPushButton("取消", this);
        QPushButton* extractBoundaryFaceOkBtn = new QPushButton("确认", this);
        connect(extractBoundaryFaceCancelBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onCancel);
        connect(extractBoundaryFaceOkBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onBoundaryFaceActive);

        FoldSection* extractBoundaryFaceFold = new FoldSection("提取边界面", this);
        extractBoundaryFaceFold->addRow(extractBoundaryFaceCancelBtn, extractBoundaryFaceOkBtn, "Left");
        m_FoldSections.append(extractBoundaryFaceFold);
        m_foldWidgets.append(extractBoundaryFaceFold);

    }

    void MechMainWindow::mGeoExtractPanel::initFluidPanel()
    {
        
        //流体域
        subDelAction = new QAction("删除");
        QPushButton* extractFluidCancelBtn = new QPushButton("取消", this);
        QPushButton* extractFluideOkBtn = new QPushButton("确认", this);
        connect(extractFluidCancelBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onCancel);
        connect(extractFluideOkBtn, &QPushButton::clicked, this, &mGeoExtractPanel::slot_onFluidActive);

        FoldSection* extractFluidFold = new FoldSection("提取流体域", this);
        extractFluidFold->addRow(extractFluidCancelBtn, extractFluideOkBtn, "Left");
        m_FoldSections.append(extractFluidFold);
        m_foldWidgets.append(extractFluidFold);
    }

    void mGeoExtractPanel::slot_contextMenuTableData(const QPoint& pos)
    {
        mUiMenu menu(pickObjTable_2);
        menu.addActions({ subDelAction });
        QAction* selected = menu.exec(pickObjTable_2->viewport()->mapToGlobal(pos));
        if (selected == subDelAction)
        {
            QList<QTableWidgetItem*> items = pickObjTable_2->selectedItems();
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
                    QString strId = pickObjTable_2->item(ids[i], 0)->text();
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
                    pickObjTable_2->removeRow(ids[i]);
                }
                MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
                getCVM->getPreRenderWidget()->updatePaint();
            }
        }
    }
	

	void mGeoExtractPanel::slot_highlightGeoData()
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

		if (geoPickWidget->strPickType != type )
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
		}
        if (geoPickWidget->_featPick == Fluid)
        {
            for (int i = pickObjTable_2->rowCount() - 1; i >= 0; i--)
            {
                QString strId = pickObjTable_2->item(i, 0)->text();
                if (strId.indexOf(type) == 0)
                {
                    pickObjTable_2->removeRow(i);
                }
            }

            for (auto id : res)
            {
                pickObjTable_2->insertRow(pickObjTable_2->rowCount());
                pickObjTable_2->setItem(pickObjTable_2->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
            }
        }
	}

	void mGeoExtractPanel::slotMidSurfaceThreadFinished()
	{
        disconnect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::geoAsyncTaskFinished, this, &mGeoExtractPanel::slotMidSurfaceThreadFinished);

        extractMidFaceeOkBtn->setEnabled(true);
        geoPickWidget->clearHighlight();
        clearTableItems(pickObjTable);

        //从结果池里面拿出结果
        EventDispatcher::mGeoOperationResult outResult;
        if (getGEOServer.geoExtract().takeResult(outResult) && outResult.generatedObjects.size() == 2)
		{
            // 撤消收口（成功）：前态快照入撤消栈（整个中面提取 = 一个撤消条目）。
            if (m_midFaceHistoryToken != nullptr)
            {
                getGEOServer.historyOperationSucceeded(
                    m_midFaceHistoryToken, EventDispatcher::mGeoOperationKind::Extract);
                m_midFaceHistoryToken = nullptr;
            }
            getCVM->getModelData()->getGeoManager()->registerCreatedPart(outResult.generatedObjects[1].id.id);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
			}
		else
		{
            // 撤消收口（失败/无结果）：丢弃前态快照，不入栈。
            if (m_midFaceHistoryToken != nullptr)
            {
                getGEOServer.historyOperationFailed(m_midFaceHistoryToken);
                m_midFaceHistoryToken = nullptr;
            }
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建失败！"));
		}
	}

	void mGeoExtractPanel::slot_subWindowActivated()
	{
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoExtractPanel::slot_highlightGeoData);
	}

	void mGeoExtractPanel::slot_onCancel()
	{
		hideAllFold();
	}

	void mGeoExtractPanel::slot_onMidFaceActive()
	{
		try
		{
			if (midFaceFuncComb->currentIndex() == 1 && midFaceMeshSize->text().isEmpty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请输入网格尺寸!"));
				return;
			}
			if (offsetDisComb->currentIndex() == 1 && midFaceOffsetDis->text().isEmpty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请输入偏置距离!"));
				return;
			}
			std::set<int> geoIds = GetTableIdsUseSet();
			if (geoIds.empty())
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1,tr("请拾取几何!"));
				return;
			}
            EventDispatcher::mGeoExtractMidFaceRequest req;
				for (int t : geoIds)
				{
                req.geoIds.push_back(t);
				}
			if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
			{
                req.objtype = GeoNeutralData::GN_ShapeType::Face;
			}
			else
			{
                req.objtype = GeoNeutralData::GN_ShapeType::Solid;
				}

			if (midFaceFuncComb->currentIndex() == 0)
			{
                req.midFaceType = EventDispatcher::MidSurfaceType::SkinOffset;
			}
			else if (midFaceFuncComb->currentIndex() == 1)
			{
                req.midFaceType = EventDispatcher::MidSurfaceType::MeshOffset;
                req.meshSize = midFaceMeshSize->text().toDouble();
				QString str = QString("中面网格尺寸：") + QString::number(req.meshSize);
			}
            req.endFaceAngle = midFaceAngle->text().toDouble();
            req.sewingTolerance = midFaceFillMax->text().toDouble();

			if (offsetDisComb->currentIndex() == 0)
			{
                req.disType = EventDispatcher::DisType::AutoDis;
			}
			else
			{
                req.disType = EventDispatcher::DisType::ManuDis;
                req.distance = midFaceOffsetDis->text().toDouble();
			}

            // 撤消接入（异步三段式）：提交前抓前态快照（提交受理后 worker 才可能
            // 提交写库，先抓无竞态）；终态在 slotMidSurfaceThreadFinished 按成败收口，
            // 受理失败/异常在本函数内收口。busy 期间库侧拒绝撤消。
            m_midFaceHistoryToken = getGEOServer.historyOperationBegin();
            EventDispatcher::mGeoAsyncSubmitResult res = getGEOServer.geoExtract().extractMidFace(req);
            if (res.accepted())//受理成功
			{
                connect(MxFunctions::mGlobalSignals::getInstance(), &MxFunctions::mGlobalSignals::geoAsyncTaskFinished, this, &mGeoExtractPanel::slotMidSurfaceThreadFinished);
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("正在进行中面抽取..."));
			}
			else
			{
                getGEOServer.historyOperationFailed(m_midFaceHistoryToken);
                m_midFaceHistoryToken = nullptr;
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("中面抽取失败!"));
			}
		}
		catch (...)
		{
            // 异常路径撤消收口：丢弃未终态的令牌，避免 busy 卡死。
            if (m_midFaceHistoryToken != nullptr)
            {
                getGEOServer.historyOperationFailed(m_midFaceHistoryToken);
                m_midFaceHistoryToken = nullptr;
            }
			MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("中面抽取可能已经发生错误，请进行几何清理之后重试!"));
		}
	}

    void mGeoExtractPanel::slot_onFluidActive()
    {

        std::set<int> geoAIds = GetTableIdsUseSet();
        std::vector<int> SeedFaces;
        std::vector<int> Faces;
        std::vector<int> LoopEdges;
        for (int i = 0; i < pickObjTable_2->rowCount(); ++i)
        {
            QString strId = pickObjTable_2->item(i, 0)->text();
            strId = strId.mid(strId.lastIndexOf(":") + 1);
            int id = strId.toInt();
            SeedFaces.push_back(id);
        }

        for (auto geoId : geoAIds)
        {
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                Faces.push_back(geoId);
            }
            else if (geoPickWidget->pickObjTypeCombo->currentIndex() == 1)
            {
                LoopEdges.push_back(geoId);
            }
        }

        if (SeedFaces.empty() || (Faces.empty() && LoopEdges.empty()))
            {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(1, tr("请拾取几何!"));
            return;
            }
        mGeoExtractFluidRequest req;
        req.SeedFaces = SeedFaces;
        req.Faces = Faces;
        req.LoopEdges = LoopEdges;


        EventDispatcher::mGeoOperationResult res = getGEOServer.geoExtract().extractFluid(req);
        if (res.succeeded() && res.generatedObjects.size() == 2)
        {
            getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
        }

    }
    void mGeoExtractPanel::slot_onMidLineActive()
    {
        std::set<int> geoAIds = GetTableIdsUseSet();
        std::set<int> geoBIds;
        for (int i = 0; i < pickObjTable_2->rowCount(); ++i)
        {
            QString strId = pickObjTable_2->item(i, 0)->text();
            strId = strId.mid(strId.lastIndexOf(":") + 1);
            int id = strId.toInt();
            geoBIds.insert(id);
        }

         if (geoAIds.empty() || geoBIds.empty())
         {
             return;
         }
        std::vector<int> masterlines;
        std::vector<int> targetlines;
        for (auto mId : geoAIds)
        {
            masterlines.push_back(mId);
        }
        for (auto tId : geoBIds)
        {
            targetlines.push_back(tId);
        }

        mGeoExtractMidLineRequest req;
        req.masterlines = masterlines;
        req.targetlines = targetlines;

        EventDispatcher::mGeoOperationResult res = getGEOServer.geoExtract().extractMidLine(req);

        if (res.succeeded() && res.generatedObjects.size() == 2)
        {
            getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
        }

    }
    void mGeoExtractPanel::slot_onBoundaryFaceActive()
    {
        std::set<int> geoAIds = GetTableIdsUseSet();
         if (geoAIds.empty())
         {
             MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("请选择几何！"));
             return;
         }

        std::vector<int> solids;
        for (auto geoId : geoAIds)
        {
            solids.push_back(geoId);
        }

        EventDispatcher::mGeoExtractBoundFaceRequest req;
        req.solids = solids;

        EventDispatcher::mGeoOperationResult res = getGEOServer.geoExtract().extractBoundFace(req);

        if (res.succeeded() && res.generatedObjects.size() == 2)
        {
            getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
        }
        else
        {
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建失败！"));
        }

    }
}
