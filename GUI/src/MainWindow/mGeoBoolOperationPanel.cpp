#include "mGeoBoolOperationPanel.h"
#include "UiMetrics.h"
#include "ViewportContainer.h"
#include "mMechGlobalSignals.h"
#include "mUiMenu.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDockLayoutManager.h"
#include "mGlobalSignals.h"
#include "mGeoManager.h"
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1】退役头死 include（零引用）

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"


#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"

#include "mGeoPreviewTypes.h"
#include "mGeoServer.h"
#include <mMeshData.h>

#include "InteractionCore/Controller/InteractionToolManager.h"
#include "InteractionCore/Tool/DirectGeoModelingTool.h"
#include "PreViewportWidget.h"

#include <QMenu>
#include <algorithm>
#include <set>

//using namespace MGeoCommand; //【OCC摘除·S10/P0-1】随退役头下线
using namespace mxr;

namespace MechMainWindow
{
	mGeoBoolOperationPanel::mGeoBoolOperationPanel(QWidget* parent)
		: mGeoPanelBase(parent)
	{
		geoPickWidget_2 = new GeoPickWidget(this);
		geoPickWidget_2->_featPick = None;

		connect(geoPickWidget->pickObjTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			clearTableItems(pickObjTable);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
			if (geoPickWidget_2->pickObjTypeCombo->currentIndex() != index)
			{
				geoPickWidget_2->pickObjTypeCombo->setCurrentIndex(index);
			}
			geoPickWidget->_featPick = FeatureObj;
			});

		connect(geoPickWidget->pickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
			setGeoHighlight(pickObjTable, geoPickWidget->pickObjTypeCombo->currentText());
			geoPickWidget->_featPick = FeatureObj;
			});


		geoPickWidget_2->pickLabel->setText("对象B");
		geoPickWidget_2->pickObjTypeCombo->clear();
        geoPickWidget_2->pickObjTypeCombo->addItems({ "几何面","几何体" });
        connect(geoPickWidget_2->pickObjTypeCombo, QOverload<int>::of(&QComboBox::activated), [this](int index) {
			geoPickWidget_2->_featPick = None;
			geoPickWidget_2->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
			clearTableItems(targetBTable);
			setGeoHighlight(pickObjTable, geoPickWidget_2->pickObjTypeCombo->currentText());
			if (geoPickWidget->pickObjTypeCombo->currentIndex() != index)
			{
				geoPickWidget->pickObjTypeCombo->setCurrentIndex(index);
			}
			geoPickWidget_2->_featPick = SubPickObjB;
			});

		connect(geoPickWidget_2->pickBtn, &QToolButton::clicked, this, [this]() {
			geoPickWidget->_featPick = None;
			geoPickWidget_2->setPickModeByCombo(geoPickWidget_2->pickObjTypeCombo);
			geoPickWidget->strPickType = geoPickWidget_2->strPickType;
			setGeoHighlight(targetBTable, geoPickWidget_2->pickObjTypeCombo->currentText());
			geoPickWidget->_featPick = SubPickObjB;
			});

		//按顺序初始化
		initInterseFold();
		initSummationFold();
		initSusubtracFold();

		_mainLayout->addStretch();

        //全局信号（拾取/视口切换）统一改由 mInfoWindow 进入面板后 connectGeoSignals 连接
	}

	void mGeoBoolOperationPanel::connectGeoSignals()
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

	void mGeoBoolOperationPanel::disconnectGeoSignals()
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

	mGeoBoolOperationPanel::~mGeoBoolOperationPanel()
	{

	}

	void mGeoBoolOperationPanel::showFold(GeoOperationType type)
	{
		hideAllFold();
		currentFeature = type;
		geoPickWidget->pickLabel->setText("对象A");

		geoPickWidget->pickObjTypeCombo->clear();
        geoPickWidget->pickObjTypeCombo->addItems({ "几何面","几何体" });
		geoPickWidget->_featPick = FeatureObj;

		for (int i = targetBTable->rowCount() - 1; i >= 0; i--) {
			targetBTable->removeRow(i);
		}

		if (GeoInterse == type)
		{
			m_FoldSections[0]->insertRow(0, targetBTable);
			m_FoldSections[0]->insertRow(0, geoPickWidget_2);
			m_FoldSections[0]->insertRow(0, pickObjTable);
			m_FoldSections[0]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 0 });

			activateDirectGeoModelingTool(true);
			connectPreView(intersePreviewChkBox->isChecked());
		}
		if (GeoSummation == type)
		{
			m_FoldSections[1]->insertRow(0, targetBTable);
			m_FoldSections[1]->insertRow(0, geoPickWidget_2);
			m_FoldSections[1]->insertRow(0, pickObjTable);
			m_FoldSections[1]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 1 });

			activateDirectGeoModelingTool(true);
			connectPreView(summationPreviewChkBox->isChecked());
		}
		if (GeoSusubtrac == type)
		{

			m_FoldSections[2]->insertRow(0, targetBTable);
			m_FoldSections[2]->insertRow(0, geoPickWidget_2);
			m_FoldSections[2]->insertRow(0, pickObjTable);
			m_FoldSections[2]->insertRow(0, geoPickWidget);

			displayAfterInsertFolds({ 2 });

			activateDirectGeoModelingTool(true);
			connectPreView(subtracPreviewChkBox->isChecked());
		}

		geoPickWidget->setPickModeByCombo(geoPickWidget->pickObjTypeCombo);
	}

	//初始化布尔求交操作界面
	void mGeoBoolOperationPanel::initInterseFold()
	{
		intersePreviewChkBox = new QCheckBox("预览", this);
		intersePreviewChkBox->setChecked(false);
		connect(intersePreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked);
			if (checked)
			{
				slot_booleanPreview();
			}
			else
			{
				clearPreviewRender();
			}
			});

        interseCopyChkBox = new QCheckBox("复制", this);

		QPushButton* interseCancelBtn = new QPushButton("取消", this);
		QPushButton* interseOkBtn = new QPushButton("确认", this);

		connect(interseCancelBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_onCancel);
		connect(interseOkBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_interseAction);

		FoldSection* interseFold_1 = new FoldSection("布尔运算-求交", this);

        interseFold_1->addRow(interseCopyChkBox);
		interseFold_1->addRow(intersePreviewChkBox);
		QFrame* horizontalLine = createHorizontalLine(interseFold_1);
		interseFold_1->addRow(horizontalLine);
        interseFold_1->addRow(interseCancelBtn, interseOkBtn, "Left");

		m_FoldSections.append(interseFold_1);
		m_foldWidgets.append(interseFold_1);
		_mainLayout->addWidget(interseFold_1);
	}

	//初始化布尔求和操作界面
	void mGeoBoolOperationPanel::initSummationFold()
	{
		summationRetainChkBox = new QCheckBox("是否简化", this);
		summationRetainChkBox->setChecked(false);

        summationCopyChkBox = new QCheckBox("复制", this);

		summationPreviewChkBox = new QCheckBox("预览", this);
		summationPreviewChkBox->setChecked(false);
		connect(summationPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked);
			if (checked)
			{
				slot_booleanPreview();
			}
			else
			{
				clearPreviewRender();
			}
			});

		QPushButton* summationCancelBtn = new QPushButton("取消", this);
		QPushButton* summationOkBtn = new QPushButton("确认", this);

		connect(summationCancelBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_onCancel);
		connect(summationOkBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_summationAction);

		FoldSection* summationFold_1 = new FoldSection("布尔运算-求和", this);
		summationFold_1->addRow(summationRetainChkBox, summationCopyChkBox);
		summationFold_1->addRow(summationPreviewChkBox);
		QFrame* horizontalLine = createHorizontalLine(summationFold_1);
		summationFold_1->addRow(horizontalLine);
		summationFold_1->addRow(summationCancelBtn, summationOkBtn, "Left");

		m_FoldSections.append(summationFold_1);
		m_foldWidgets.append(summationFold_1);
		_mainLayout->addWidget(summationFold_1);
	}

	//初始化布尔求差操作界面
	void mGeoBoolOperationPanel::initSusubtracFold()
	{
		subtracPreviewChkBox = new QCheckBox("预览", this);
		subtracPreviewChkBox->setChecked(false);
		connect(subtracPreviewChkBox, &QCheckBox::toggled, this, [this](bool checked) {
			connectPreView(checked);
			if (checked)
			{
				slot_booleanPreview();
			}
			else
			{
				clearPreviewRender();
			}
			});

        subtracCopyChkBox = new QCheckBox("复制", this);

		QPushButton* susubtracCancelBtn = new QPushButton("取消", this);
		QPushButton* susubtracOkBtn = new QPushButton("确认", this);

		targetBTable = new QTableWidget(this);
		targetBTable->setMaximumHeight(100);
		targetBTable->setMinimumHeight(80);
		targetBTable->verticalHeader()->setHidden(true);
		targetBTable->setColumnCount(1);
		QStringList horizontalHeader = { "几何ID" };
		targetBTable->setHorizontalHeaderLabels(horizontalHeader);
		targetBTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		targetBTable->horizontalHeader()->setHighlightSections(false);
		targetBTable->setFont(QFont(MxFunctions::UiMetrics::uiFontFamily(), 10));
		targetBTable->verticalHeader()->setDefaultSectionSize(20);
		targetBTable->setContextMenuPolicy(Qt::CustomContextMenu);//右键菜单
		targetBTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

		subDelAction = new QAction("删除");

		connect(susubtracCancelBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_onCancel);
		connect(susubtracOkBtn, &QPushButton::clicked, this, &mGeoBoolOperationPanel::slot_susubtracAction);
		connect(targetBTable, &QTableWidget::customContextMenuRequested, this, &mGeoBoolOperationPanel::slot_contextMenuTableData);

		FoldSection* susubtracFold_1 = new FoldSection("布尔运算-求差", this);
		susubtracFold_1->addRow(targetBTable);
        susubtracFold_1->addRow(subtracCopyChkBox);
		susubtracFold_1->addRow(subtracPreviewChkBox);
		QFrame* horizontalLine = createHorizontalLine(susubtracFold_1);
		susubtracFold_1->addRow(horizontalLine);
		susubtracFold_1->addRow(susubtracCancelBtn, susubtracOkBtn, "Left");

		m_FoldSections.append(susubtracFold_1);
		m_foldWidgets.append(susubtracFold_1);
		_mainLayout->addWidget(susubtracFold_1);

	}


	//面板隐藏时取消预览并取消激活渲染工具
	void mGeoBoolOperationPanel::hideEvent(QHideEvent* event)
	{
		connectPreView(false);
		clearPreviewRender();
		activateDirectGeoModelingTool(false);
		mGeoPanelBase::hideEvent(event);
	}

	//激活/取消激活直接建模渲染工具
	void mGeoBoolOperationPanel::activateDirectGeoModelingTool(bool active)
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
	void mGeoBoolOperationPanel::clearPreviewRender()
	{
		//恢复被透明化的原部件
		restoreSourcePartAlpha();

		if (!getCVM || !getCVM->getPreViewportWidget() || !getCVM->getPreViewportWidget()->interaction())
		{
			return;
		}
		auto* manager = getCVM->getPreViewportWidget()->interaction()->getToolManager();
		auto* tool = manager ? manager->getTool<mxr::InteractionDirectGeoModelingTool>(mxr::InteractionToolType::DirectGeoModeling) : nullptr;
		if (tool)
		{
			tool->deleteGeoFeatruePoint();
			tool->deleteGeoFeatrueLine();
			tool->deleteGeoFeatrueFace();
		}
		if (getCVM->getPreRenderWidget())
		{
			getCVM->getPreRenderWidget()->updatePaint();
		}
	}

	//恢复上一次预览透明化的所有部件
	void mGeoBoolOperationPanel::restoreSourcePartAlpha()
	{
		if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
		{
			return;
		}
		if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
		{
			getCVM->getPreRenderWidget()->getPreRender()->setTransparent(false, 1.0f);
		}
		//同一部件可能因之前的刷新被重复记录：以首次记录（原始值）为准，逐部件恢复一次。
		std::vector<int> restoredIDs;
		std::set<int> seen;
		for (size_t i = 0; i < m_transparentPartIDs.size(); ++i)
		{
			const int partID = m_transparentPartIDs[i];
			if (seen.count(partID) > 0)
			{
				continue;
			}
			seen.insert(partID);
			mPartData* oldPart = getCVM->getModelData()->getMeshData()->getPartData(partID);
			if (oldPart && m_transparentPartOriginAlphas[i] >= 0)
			{
				oldPart->setPartAlpha(static_cast<ushort>(m_transparentPartOriginAlphas[i]));
				restoredIDs.push_back(partID);
			}
		}
		if (!restoredIDs.empty())
		{
			getCVM->updatePartShowColor(getCVM->getPartType(), restoredIDs);
		}
		m_transparentPartIDs.clear();
		m_transparentPartOriginAlphas.clear();
	}

	//按布尔类型设置参与部件的透明度：
	// 求差：对象A完全透明(255)；求交：A和B都30%透明(76)；求和：A和B都完全透明(255)
	void mGeoBoolOperationPanel::setSourcePartTransparent(const std::vector<int>& partIDs, ushort alpha)
	{
		if (!getCVM || !getCVM->getModelData() || !getCVM->getModelData()->getMeshData())
		{
			return;
		}
		//渲染管线只有在透明模式下才会使用部件 alpha（见 mPreGeoModelRender::ColorOnePart 分支）
		if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
		{
			getCVM->getPreRenderWidget()->getPreRender()->setTransparent(true, 1.0f);
		}
		for (int partID : partIDs)
		{
			mPartData* part = getCVM->getModelData()->getMeshData()->getPartData(partID);
			if (!part)
			{
				continue;
			}
			//预览刷新会重复调用：已在列表中的部件只更新目标透明度，
			//不再记录（否则原始透明度会被覆盖成透明值，退出时无法恢复）。
			const bool alreadyRecorded =
				std::find(m_transparentPartIDs.begin(), m_transparentPartIDs.end(), partID)
				!= m_transparentPartIDs.end();
			if (!alreadyRecorded)
			{
				m_transparentPartOriginAlphas.push_back(static_cast<int>(part->getPartAlpha()));
				m_transparentPartIDs.push_back(partID);
			}
			part->setPartAlpha(alpha);
			//与几何特征减量预览一致：逐部件刷新显示颜色，使透明度立即生效
			getCVM->updatePartShowColor(getCVM->getPartType(), { partID });
		}
	}

	//连接/断开布尔预览信号；conn 为 true 时拾取表格变化都会触发预览
	void mGeoBoolOperationPanel::connectPreView(bool conn)
	{
		auto connectTable = [this, conn](QTableWidget* table) {
			if (!table)
			{
				return;
			}
			if (conn)
			{
				connect(table, &QTableWidget::itemChanged, this, &mGeoBoolOperationPanel::slot_booleanPreview, Qt::UniqueConnection);
				connect(table->model(), &QAbstractItemModel::rowsInserted, this, &mGeoBoolOperationPanel::slot_booleanPreview, Qt::UniqueConnection);
				connect(table->model(), &QAbstractItemModel::rowsRemoved, this, &mGeoBoolOperationPanel::slot_booleanPreview, Qt::UniqueConnection);
			}
			else
			{
				disconnect(table, &QTableWidget::itemChanged, this, &mGeoBoolOperationPanel::slot_booleanPreview);
				disconnect(table->model(), &QAbstractItemModel::rowsInserted, this, &mGeoBoolOperationPanel::slot_booleanPreview);
				disconnect(table->model(), &QAbstractItemModel::rowsRemoved, this, &mGeoBoolOperationPanel::slot_booleanPreview);
			}
			};

		connectTable(pickObjTable);
		connectTable(targetBTable);

		if (!conn)
		{
			clearPreviewRender();
		}
	}

	//把预览离散化结果渲染到视口（颜色按面所属部件着色；面透明度由 faceAlpha 控制：0=不透明,76=30%透明）
	void mGeoBoolOperationPanel::renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res, int faceAlpha)
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

		//预览颜色：按面所属部件着色，布尔结果不透明（w=0）；取不到部件时退回 res.partID，再退回默认米色。
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

	//布尔预览：构建预览请求包，调用预览服务渲染
	void mGeoBoolOperationPanel::slot_booleanPreview()
	{
		bool checked = false;
		EventDispatcher::mGeoBooleanKind kind = EventDispatcher::mGeoBooleanKind::Fuse;
		if (GeoInterse == currentFeature)
		{
			checked = intersePreviewChkBox && intersePreviewChkBox->isChecked();
			kind = EventDispatcher::mGeoBooleanKind::Common;
		}
		else if (GeoSummation == currentFeature)
		{
			checked = summationPreviewChkBox && summationPreviewChkBox->isChecked();
			kind = EventDispatcher::mGeoBooleanKind::Fuse;
		}
		else if (GeoSusubtrac == currentFeature)
		{
			checked = subtracPreviewChkBox && subtracPreviewChkBox->isChecked();
			kind = EventDispatcher::mGeoBooleanKind::Cut;
		}
		if (!checked)
		{
			return;
		}

		std::set<int> geoIds = GetTableIdsUseSet();
		std::set<int> geoBIds;
		for (int i = 0; i < targetBTable->rowCount(); ++i)
		{
			QTableWidgetItem* item = targetBTable->item(i, 0);
			if (!item)
			{
				continue;
			}
			QString strId = item->text();
			strId = strId.mid(strId.lastIndexOf(":") + 1);
			geoBIds.insert(strId.toInt());
		}
		if (geoIds.empty() || geoBIds.empty())
		{
			clearPreviewRender();
			return;
		}

		std::vector<uint64_t> targets;
		std::vector<uint64_t> tools;
		for (auto id : geoIds)
		{
			targets.push_back(id);
		}
		for (auto bid : geoBIds)
		{
			tools.push_back(bid);
		}

		GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Face;
		if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
		{
			type = GeoNeutralData::GN_ShapeType::Face;
		}
		else
		{
			type = GeoNeutralData::GN_ShapeType::Solid;
		}

		EventDispatcher::mGeoBooleanPreviewRequest req;
		req.kind = kind;
		req.objtype = type;
		req.targets = targets;
		req.tools = tools;
		req.simplify = true;
		req.keepTarget = false;
		req.name = "BoolPreview";

		EventDispatcher::mGeoPreviewResult res = getGEOServer.booleans().executePreview(req);
		if (!res.succeeded())
		{
			clearPreviewRender();
			return;
		}

		//按布尔类型分别设置参与部件透明度与预览面透明度（部件 alpha：0=全透明，255=不透明）：
		// 求差：A完全透明(0)，预览30%透明(w=76)；
		// 求交：A/B都30%透明(76)，预览不透明(w=0)；
		// 求和：A/B都完全透明(0)，预览30%透明(w=76)。
		auto partIdOfGeo = [&](int geoId) -> int
		{
			const int pid = (type == GeoNeutralData::GN_ShapeType::Face)
				? getGEOServer.context().PartManager().getPartByGeoFaceID(geoId).metadata().getID()
				: getGEOServer.context().PartManager().getPartByGeoSolidID(geoId).metadata().getID();
			return pid;
		};
		std::set<int> partAIds, partBIds;
		for (int id : geoIds) partAIds.insert(partIdOfGeo(id));
		for (int id : geoBIds) partBIds.insert(partIdOfGeo(id));

		int faceAlpha = 0;
		if (kind == EventDispatcher::mGeoBooleanKind::Cut)
		{
			faceAlpha = 76;
			setSourcePartTransparent(std::vector<int>(partAIds.begin(), partAIds.end()), 0);
		}
		else if (kind == EventDispatcher::mGeoBooleanKind::Common)
		{
			faceAlpha = 0;
			std::set<int> all = partAIds;
			all.insert(partBIds.begin(), partBIds.end());
			setSourcePartTransparent(std::vector<int>(all.begin(), all.end()), 76);
		}
		else // Fuse
		{
			faceAlpha = 76;
			std::set<int> all = partAIds;
			all.insert(partBIds.begin(), partBIds.end());
			setSourcePartTransparent(std::vector<int>(all.begin(), all.end()), 0);
		}
		renderPreviewResult(res, faceAlpha);
	}
	void mGeoBoolOperationPanel::slot_highlightGeoData()
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

			//拾取对象 A 变化后刷新预览
			{
				const bool on = (GeoInterse == currentFeature && intersePreviewChkBox && intersePreviewChkBox->isChecked())
					|| (GeoSummation == currentFeature && summationPreviewChkBox && summationPreviewChkBox->isChecked())
					|| (GeoSusubtrac == currentFeature && subtracPreviewChkBox && subtracPreviewChkBox->isChecked());
				if (on) slot_booleanPreview();
			}
		}
		if (geoPickWidget->_featPick == SubPickObjB)
		{
			for (int i = targetBTable->rowCount() - 1; i >= 0; i--)
			{
				QString strId = targetBTable->item(i, 0)->text();
				if (strId.indexOf(type) == 0)
				{
					targetBTable->removeRow(i);
				}
			}

			for (auto id : res)
			{
				targetBTable->insertRow(targetBTable->rowCount());
				targetBTable->setItem(targetBTable->rowCount() - 1, 0, new QTableWidgetItem(QString(type + ": %1").arg(id)));
			}

			//拾取对象 B 变化后刷新预览
			{
				const bool on = (GeoInterse == currentFeature && intersePreviewChkBox && intersePreviewChkBox->isChecked())
					|| (GeoSummation == currentFeature && summationPreviewChkBox && summationPreviewChkBox->isChecked())
					|| (GeoSusubtrac == currentFeature && subtracPreviewChkBox && subtracPreviewChkBox->isChecked());
				if (on) slot_booleanPreview();
			}
		}
	}

	void mGeoBoolOperationPanel::slot_subWindowActivated()
	{
		//disconnect(getCVM->getPreRenderWidget()->getGeoSelectData());
		if (conn_geo)
			disconnect(conn_geo);
		if (!getCVM || !getCVM->getPreRenderWidget())
		{
			return;
		}
		conn_geo = connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mGeoBoolOperationPanel::slot_highlightGeoData);
	}

	void mGeoBoolOperationPanel::slot_contextMenuTableData(const QPoint& pos)
	{
		mUiMenu menu(targetBTable);
		menu.addActions({ subDelAction });
		QAction* selected = menu.exec(targetBTable->viewport()->mapToGlobal(pos));
		if (selected == subDelAction)
		{
			QList<QTableWidgetItem*> items = targetBTable->selectedItems();
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
					QString strId = targetBTable->item(ids[i], 0)->text();
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
					targetBTable->removeRow(ids[i]);
				}
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
	}


	void mGeoBoolOperationPanel::slot_onCancel()
	{
		connectPreView(false);
		clearPreviewRender();
		activateDirectGeoModelingTool(false);
		hideAllFold();
	}

	void mGeoBoolOperationPanel::slot_summationAction()
	{
		try
		{
            std::set<int> geoIds = GetTableIdsUseSet();


            std::set<int> geoBIds;
            for (int i = 0; i < targetBTable->rowCount(); ++i)
            {
                QString strId = targetBTable->item(i, 0)->text();
                strId = strId.mid(strId.lastIndexOf(":") + 1);
                int id = strId.toInt();
                geoBIds.insert(id);
            }

            if (geoIds.empty() || geoBIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            std::vector<uint64_t> tools;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }
            for (auto bid : geoBIds)
            {
                tools.push_back(bid);
            }

            //执行布尔操作
            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Face;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }
            EventDispatcher::mGeoBooleanRequest req;
            req.kind = mGeoBooleanKind::Fuse;
            req.objtype = type;
            req.targets = targets;
            req.tools = tools;
            req.simplify = summationRetainChkBox->checkState();
            req.keepTarget = summationCopyChkBox->isChecked();
            req.name = "BoolShape";
            EventDispatcher::mGeoOperationResult res = getGEOServer.booleans().execute(req);
            if (res.succeeded())
            {
                // 正式结果同步/建件前先恢复预览透明度并删除预览层，
                // 避免重建网格继承预览 alpha 导致镂空/半透明残留。
                clearPreviewRender();
                if (summationCopyChkBox->isChecked() && res.generatedObjects.size() == 2)
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
                clearTableItems(pickObjTable);
                clearTableItems(targetBTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("布尔操作执行失败！"));
            }
            /*std::set<int> geoIds = GetTableIdsUseSet();;
			if (geoIds.empty())
			{
				return;
			}

			std::set<int> geoBIds;
			for (int i = 0; i < targetBTable->rowCount(); ++i)
			{
				QString strId = targetBTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoBIds.insert(id);
			}

			if (geoIds.empty() || geoBIds.empty())
			{
				return;
			}

			bool isKeepLine = summationRetainChkBox->checkState();


			mGeoCommandBooleanOperator* mGeoBoolean = mGeoCommandFactory::createBooleanOperator();
			mGeoBoolean->setGeoDataBase(getCVM->getModelData()->getGeoData());
			if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
			{
				for (auto face : geoIds)
				{
					mGeoBoolean->addGeoFace1(face);
				}
				for (auto face : geoBIds)
				{
					mGeoBoolean->addGeoFace2(face);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::ByFace); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			else
			{
				for (auto solid : geoIds)
				{
					mGeoBoolean->addGeoSolid1(solid);
				}
				for (auto solid : geoBIds)
				{
					mGeoBoolean->addGeoSolid2(solid);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::BySolid); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			//mGeoBoolean->setBoolType(MGeoCommand::BoolType(BoolFuse)); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			mGeoBoolean->setModelName("BoolShape");
			mGeoBoolean->isSimplify(isKeepLine);
            mGeoBoolean->isCopy(summationCopyChkBox->isChecked());
			bool isCreate = mGeoBoolean->handleCommand();
			int modelDataID = getCVM->getModelData()->getModelDataID();
			if (isCreate)
			{
                if (summationCopyChkBox->isChecked())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(mGeoBoolean->getGeoDataSet()->getPartID(), modelDataID);
                }
                else
                {
                    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
                    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }

                geoPickWidget->clearHighlight();
				clearTableItems(pickObjTable);
				clearTableItems(targetBTable);
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建失败！"));
            }*/
			}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoBoolOperationPanel::slot_interseAction()
	{
		try
		{
            std::set<int> geoIds = GetTableIdsUseSet();


            std::set<int> geoBIds;
            for (int i = 0; i < targetBTable->rowCount(); ++i)
            {
                QString strId = targetBTable->item(i, 0)->text();
                strId = strId.mid(strId.lastIndexOf(":") + 1);
                int id = strId.toInt();
                geoBIds.insert(id);
            }

            if (geoIds.empty() || geoBIds.empty())
            {
                return;
            }

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            std::vector<uint64_t> tools;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }
            for (auto bid : geoBIds)
            {
                tools.push_back(bid);
            }

            //执行布尔操作
            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Face;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }
            EventDispatcher::mGeoBooleanRequest req;
            req.kind = mGeoBooleanKind::Common;
            req.objtype = type;
            req.targets = targets;
            req.tools = tools;
            req.simplify = true;
            req.keepTarget = interseCopyChkBox->isChecked();
            req.name = "BoolShape";
            EventDispatcher::mGeoOperationResult res = getGEOServer.booleans().execute(req);
            if (res.succeeded())
            {
                // 正式结果同步/建件前先恢复预览透明度并删除预览层，
                // 避免重建网格继承预览 alpha 导致镂空/半透明残留。
                clearPreviewRender();
                if (interseCopyChkBox->isChecked() && res.generatedObjects.size() == 2)
                {
                    getCVM->getModelData()->getGeoManager()->registerCreatedPart(res.generatedObjects[1].id.id);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                }
                else if (res.modifiedObjectIds.size() > 0)
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
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("布尔操作执行失败！"));
                }
                geoPickWidget->clearHighlight();
                clearTableItems(pickObjTable);
                clearTableItems(targetBTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("布尔操作执行失败！"));
            }
            /*std::set<int> geoIds = GetTableIdsUseSet();;
			if (geoIds.empty())
			{
				return;
			}

			std::set<int> geoBIds;
			for (int i = 0; i < targetBTable->rowCount(); ++i)
			{
				QString strId = targetBTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoBIds.insert(id);
			}

			if (geoIds.empty() || geoBIds.empty())
			{
				return;
			}

			mGeoCommandBooleanOperator* mGeoBoolean = mGeoCommandFactory::createBooleanOperator();
			mGeoBoolean->setGeoDataBase(getCVM->getModelData()->getGeoData());
			if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
			{
				for (auto face : geoIds)
				{
					mGeoBoolean->addGeoFace1(face);
				}
				for (auto face : geoBIds)
				{
					mGeoBoolean->addGeoFace2(face);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::ByFace); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			else
			{
				for (auto solid : geoIds)
				{
					mGeoBoolean->addGeoSolid1(solid);
				}
				for (auto solid : geoBIds)
				{
					mGeoBoolean->addGeoSolid2(solid);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::BySolid); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			//mGeoBoolean->setBoolType(MGeoCommand::BoolType(BoolCommon)); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			mGeoBoolean->setModelName("BoolShape");
			mGeoBoolean->isSimplify(true);
            mGeoBoolean->isCopy(interseCopyChkBox->isChecked());
			bool isCreate = mGeoBoolean->handleCommand();
			int modelDataID = getCVM->getModelData()->getModelDataID();
			if (isCreate)
			{
                if (interseCopyChkBox->isChecked())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(mGeoBoolean->getGeoDataSet()->getPartID(), modelDataID);
                }
                else
                {
                    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
                    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }geoPickWidget->clearHighlight();
				clearTableItems(pickObjTable);
				clearTableItems(targetBTable);
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建失败！"));
            }*/
			}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}

	void mGeoBoolOperationPanel::slot_susubtracAction()
	{
		try
		{
            std::set<int> geoIds = GetTableIdsUseSet();


			std::set<int> geoBIds;
			for (int i = 0; i < targetBTable->rowCount(); ++i)
			{
				QString strId = targetBTable->item(i, 0)->text();
				strId = strId.mid(strId.lastIndexOf(":") + 1);
				int id = strId.toInt();
				geoBIds.insert(id);
			}

			if (geoIds.empty() || geoBIds.empty())
			{
				return;
			}

            //拿到操作对象的中性数据
            std::vector<uint64_t> targets;
            std::vector<uint64_t> tools;
            auto geoBase = getCVM->getModelData()->getGeoData();
            for (auto id : geoIds)
            {
                targets.push_back(id);
            }
            for (auto bid : geoBIds)
            {
                tools.push_back(bid);
            }

            //执行布尔操作
            GeoNeutralData::GN_ShapeType type = GeoNeutralData::GN_ShapeType::Face;
            if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
            {
                type = GeoNeutralData::GN_ShapeType::Face;
            }
            else
            {
                type = GeoNeutralData::GN_ShapeType::Solid;
            }
            EventDispatcher::mGeoBooleanRequest req;
            req.kind = mGeoBooleanKind::Cut;
            req.objtype = type;
            req.targets = targets;
            req.tools = tools;
            req.simplify = true;
            req.keepTarget = subtracCopyChkBox->isChecked();
            req.name = "BoolShape";
            EventDispatcher::mGeoOperationResult res = getGEOServer.booleans().execute(req);
            if (res.succeeded())
            {
                // 正式结果同步/建件前先恢复预览透明度并删除预览层，
                // 避免重建网格继承预览 alpha 导致镂空/半透明残留。
                clearPreviewRender();
                if (subtracCopyChkBox->isChecked() && res.generatedObjects.size() == 2)
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
                clearTableItems(pickObjTable);
                clearTableItems(targetBTable);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(tr("布尔操作执行失败！"));
            }

            /*mGeoCommandBooleanOperator* mGeoBoolean = mGeoCommandFactory::createBooleanOperator();
			mGeoBoolean->setGeoDataBase(getCVM->getModelData()->getGeoData());
			if (geoPickWidget->pickObjTypeCombo->currentIndex() == 0)
			{
				for (auto face : geoIds)
				{
					mGeoBoolean->addGeoFace1(face);
				}
				for (auto face : geoBIds)
				{
					mGeoBoolean->addGeoFace2(face);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::ByFace); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			else
			{
				for (auto solid : geoIds)
				{
					mGeoBoolean->addGeoSolid1(solid);
				}
				for (auto solid : geoBIds)
				{
					mGeoBoolean->addGeoSolid2(solid);
				}
				//mGeoBoolean->setCreateThrType(MGeoCommand::BySolid); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			}
			//mGeoBoolean->setBoolType(MGeoCommand::BoolType(BoolCut)); //【OCC摘除·S10/P0-1 终局2】退役类型/using（操作在恒0门控或死代码内）
			mGeoBoolean->setModelName("BoolShape");
			mGeoBoolean->isSimplify(true);
			mGeoBoolean->isCopy(subtracCopyChkBox->isChecked());
			bool isCreate = mGeoBoolean->handleCommand();
			int modelDataID = getCVM->getModelData()->getModelDataID();
            if (isCreate)
            {
                if (subtracCopyChkBox->isChecked())
                {
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何创建成功！"));
                    MxFunctions::mGlobalSignals::getInstance()->createGeoModelSignal(mGeoBoolean->getGeoDataSet()->getPartID(), modelDataID);
                }
                else
                {
                    MGeoData::mGeoGlobalSignals::getInstance()->updateNewShape();
                    MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal(modelDataID);
                    MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(0, tr("几何编辑成功!"));
                }
                geoPickWidget->clearHighlight();
				clearTableItems(pickObjTable);
				clearTableItems(targetBTable);
			}
			else
			{
				MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2,tr("几何创建失败！"));
            }*/
			}
		catch (...)
		{
            MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(2, tr("几何创建出错！"));
		}
	}
}
