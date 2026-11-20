#pragma execution_character_set("utf-8")

#include "mNodeCoordinatePanel.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mGlobalSignals.h"
#include "mGlobalVarible.h"
#include "mDataList.h"
#include "mCenterMainWindow.h"
#include "mViewportModel.h"
#include "mViewportModelManager.h"
#include "pickManager.h"
#include "PickCommonFunction.h"
#include "mMeshData.h"

#include <set>

#include <QHeaderView>
#include <QComboBox>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QMouseEvent>

#include "mxr/data/SelectDataStruct.h"
#include "../PreData/LowCPlusDefine.h"
#include "mOtherFunction.h"

using namespace  MechPreData;
using namespace mxr;

namespace MechMainWindow
{

	mNodeCoordinatePanel::mNodeCoordinatePanel(QWidget* parent)
		: mPanelBase(parent)
	{
		initUi();
	}

	mNodeCoordinatePanel::~mNodeCoordinatePanel()
	{
		if (_nodePick)
		{
			_nodePick->clear();
			delete _nodePick;
			_nodePick = nullptr;
		}
		_modeCombo.clear();
		_refCombo.clear();
		_anaCombo.clear();
	}

	void mNodeCoordinatePanel::newNodeCoordinatePanel()
	{
		setEditing(true);
		refreshComboBoxes();

		// 从数据管理类恢复节点拾取
		restoreNodePickFromManager();
	}

	void mNodeCoordinatePanel::restoreNodePickFromManager()
	{
		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		auto nodeData = manager.getNodeCoordinateNameData();

		// 没有已保存数据时，保持统一设置并清空拾取
		if (nodeData.empty())
		{
			_modeCombo.combobox->setCurrentIndex(0);
			_currentMode = 0;
			switchMode(0);
			_nodePick->setState(false);
			return;
		}

		// 判断是否为统一设置：所有节点参考/分析坐标系都相同
		auto it = nodeData.begin();
		QString firstRef = it->second.first;
		QString firstAna = it->second.second;
		bool isUnified = true;
		for (const auto& pair : nodeData)
		{
			if (pair.second.first != firstRef || pair.second.second != firstAna)
			{
				isUnified = false;
				break;
			}
		}

		if (isUnified)
		{
			_modeCombo.combobox->setCurrentIndex(0);
			_currentMode = 0;
			switchMode(0);

			// 恢复统一设置的下拉框
			if (!firstRef.isEmpty() && _refCombo.combobox->findText(firstRef) >= 0)
				_refCombo.combobox->setCurrentText(firstRef);
			if (!firstAna.isEmpty() && _anaCombo.combobox->findText(firstAna) >= 0)
				_anaCombo.combobox->setCurrentText(firstAna);

			// 恢复节点拾取
			_nodePick->nodeIds.clear();
			_nodePick->pickedData.clear();
			for (const auto& pair : nodeData)
			{
				_nodePick->nodeIds.push_back(pair.first);
				RenderPickResultData data;
				data.result_id = pair.first;
				_nodePick->pickedData.emplace_back(MxFunctions::PickDataType::Node, data);
			}
			_nodePick->setState(true, false);
			if (_nodePick->labelpicked->isChecked())
			{
				// 恢复时若勾选则高亮，否则清除渲染选择
				highlightPickedNodes(true);
			}
			else if (getCVM->getPreRenderWidget())
			{
				ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
		else
		{
			_modeCombo.combobox->setCurrentIndex(1);
			_currentMode = 1;
			switchMode(1);

			// 分别设置仅恢复下拉框为首个可用值
			if (_refCombo.combobox->count() > 0)
				_refCombo.combobox->setCurrentIndex(0);
			if (_anaCombo.combobox->count() > 0)
				_anaCombo.combobox->setCurrentIndex(0);

			// 恢复节点拾取状态（管理类中保存的节点即为当前已选）
			_nodePick->nodeIds.clear();
			_nodePick->pickedData.clear();
			for (const auto& pair : nodeData)
			{
				_nodePick->nodeIds.push_back(pair.first);
				RenderPickResultData data;
				data.result_id = pair.first;
				_nodePick->pickedData.emplace_back(MxFunctions::PickDataType::Node, data);
			}
			_nodePick->setState(true, false);
			// 恢复时若勾选则高亮，否则清除渲染选择
			if (_nodePick->labelpicked->isChecked())
			{
				highlightPickedNodes(true);
			}
			else if (getCVM->getPreRenderWidget())
			{
				ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
				MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
	}

	void mNodeCoordinatePanel::uneditNodeCoordinatePanel()
	{
		if (!isEditing())
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支无需再单独清标志
		setEditing(false);
		if (_currentTable)
		{
			mTableDataWindow::getInstance()->resetInitialTable();
			_currentTable.reset();
		}
	}

	void mNodeCoordinatePanel::initUi()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);
		mainLayout->setContentsMargins(0, 0, 0, 0);
		mainLayout->setSpacing(5);

		_foldSection = new FoldSection("节点坐标系", this);

		// 节点拾取
		_nodePick = new NCNodePickWidget();
        _nodePick->initWidget(this, "节点", true);
		connect(_nodePick->btnok, &QPushButton::clicked, this, &mNodeCoordinatePanel::onPickNode);
		connect(_nodePick->btnadd, &QPushButton::clicked, this, &mNodeCoordinatePanel::onAddNodePick);
		connect(_nodePick->btnclear, &QPushButton::clicked, this, &mNodeCoordinatePanel::onClearNodePick);
		connect(_nodePick->labelpicked, &QCheckBox::stateChanged, this, &mNodeCoordinatePanel::onHighlightPicked);
		_foldSection->addRow(_nodePick->propwid);

		// 设置方式
		_modeCombo.initWidget("设置方式");
		_modeCombo.combobox->addItem("统一设置");
		_modeCombo.combobox->addItem("分别设置");
		connect(_modeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mNodeCoordinatePanel::switchMode);
		_foldSection->addRow(_modeCombo.propwid);

		// 参考坐标系
		_refCombo.initWidget("参考坐标系");

		// 分析坐标系
		_anaCombo.initWidget("分析坐标系");

		// 按钮行
		_btnOk = new QPushButton("确定", this);
		_btnReset = new QPushButton("重置", this);
		_btnResetSep = new QPushButton("重置", this);
		_btnEditTable = new QPushButton("编辑", this);
		connect(_btnOk, &QPushButton::clicked, this, &mNodeCoordinatePanel::onOkClicked);
		connect(_btnReset, &QPushButton::clicked, this, &mNodeCoordinatePanel::onResetClicked);
		connect(_btnResetSep, &QPushButton::clicked, this, &mNodeCoordinatePanel::onResetClicked);
		connect(_btnEditTable, &QPushButton::clicked, this, &mNodeCoordinatePanel::onEditTableClicked);

		// 统一设置按钮行：重置 确定
		_btnUnifiedWidget = new QWidget(this);
		QHBoxLayout* unifiedLayout = new QHBoxLayout(_btnUnifiedWidget);
		unifiedLayout->setContentsMargins(0, 0, 0, 0);
		unifiedLayout->addStretch();
		unifiedLayout->addWidget(_btnReset);
		unifiedLayout->addWidget(_btnOk);

		// 分别设置按钮行：重置 编辑
		_btnSeparateWidget = new QWidget(this);
		QHBoxLayout* separateLayout = new QHBoxLayout(_btnSeparateWidget);
		separateLayout->setContentsMargins(0, 0, 0, 0);
		separateLayout->addStretch();
		separateLayout->addWidget(_btnResetSep);
		separateLayout->addWidget(_btnEditTable);

		mainLayout->addWidget(_foldSection);
		mainLayout->addStretch();
	}

	void mNodeCoordinatePanel::refreshComboBoxes()
	{
		QStringList names = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate).getAllCoordinateNames();
		QString oldRef = _refCombo.combobox->currentText();
		QString oldAna = _anaCombo.combobox->currentText();

		_refCombo.combobox->clear();
		_anaCombo.combobox->clear();
		_refCombo.combobox->addItems(names);
		_anaCombo.combobox->addItems(names);

		if (!oldRef.isEmpty() && names.contains(oldRef))
			_refCombo.combobox->setCurrentText(oldRef);
		else if (!names.isEmpty())
			_refCombo.combobox->setCurrentIndex(0);

		if (!oldAna.isEmpty() && names.contains(oldAna))
			_anaCombo.combobox->setCurrentText(oldAna);
		else if (!names.isEmpty())
			_anaCombo.combobox->setCurrentIndex(0);
	}

	void mNodeCoordinatePanel::switchMode(int modeIndex)
	{
		_currentMode = modeIndex;
		mTableDataWindow::getInstance()->resetInitialTable();
		updateModeWidgets(_currentMode);
	}

	void mNodeCoordinatePanel::updateModeWidgets(int modeIndex)
	{
        _refCombo.propwid->setParent(nullptr);
        _anaCombo.propwid->setParent(nullptr);
        _btnUnifiedWidget->setParent(nullptr);
        _btnSeparateWidget->setParent(nullptr);


		QFormLayout* formLayout = _foldSection->layout();
		if (formLayout)
		{
			while (formLayout->rowCount() > 2)
			{
				formLayout->removeRow(2);
			}
		}

		if (modeIndex == 0)
		{
			_foldSection->addRow(_refCombo.propwid);
			_foldSection->addRow(_anaCombo.propwid);
			_foldSection->addRow(_btnUnifiedWidget);
		}
		else
		{
			_foldSection->addRow(_btnSeparateWidget);
		}

		QTimer::singleShot(0, _foldSection, [=]() {
			_foldSection->refreshLayout();
			});
	}

	void mNodeCoordinatePanel::onOkClicked()
	{
		QString refName = _refCombo.combobox->currentText();
		QString anaName = _anaCombo.combobox->currentText();
		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		int refId = manager.getIDByCoordinateName(refName);
		int anaId = manager.getIDByCoordinateName(anaName);
		if (refName.isEmpty() || anaName.isEmpty())
			return;

		if (_currentMode == 0)
		{
			// 统一设置：对拾取节点设置相同的参考/分析坐标系
			manager.setUnifiedNodeCoordinate(_nodePick->nodeIds, refId, anaId);
		}
	}

	void mNodeCoordinatePanel::onResetClicked()
	{
		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		manager.clearNodeCoordinateData();

		// 取消节点高亮并清空拾取
		highlightPickedNodes(false);
		_nodePick->setState(false);
		if (getCVM->getPreRenderWidget())
		{
			ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
			getCVM->getPreRenderWidget()->updatePaint();
		}

		// 恢复统一设置模式，并将参考/分析坐标系下拉框重置为默认第一项
		_modeCombo.combobox->setCurrentIndex(0);
		_currentMode = 0;
		switchMode(0);
		if (_refCombo.combobox->count() > 0)
			_refCombo.combobox->setCurrentIndex(0);
		if (_anaCombo.combobox->count() > 0)
			_anaCombo.combobox->setCurrentIndex(0);

		// 清空表格
		if (_currentTable)
		{
			mTableDataWindow::getInstance()->resetInitialTable();
			_currentTable.reset();
		}
	}

	void mNodeCoordinatePanel::onEditTableClicked()
	{
		openNodeCoordinateTable();
	}

	void mNodeCoordinatePanel::openNodeCoordinateTable()
	{
		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		QStringList names = manager.getAllCoordinateNames();
		_currentTable = std::make_unique<mNodeCoordinateTableWidget>();
		_currentTable->setCoordinateNames(names);
		_currentTable->setNodeData(manager.getNodeCoordinateNameData());

		connect(_currentTable.get(), &mNodeCoordinateTableWidget::itemChanged,
			this, &mNodeCoordinatePanel::onTableItemChanged, Qt::UniqueConnection);

		mTableDataWindow::getInstance()->setCustomWidget(_currentTable.get());
		mDockLayoutManager::getInstance()->activateDock("Table");
	}

	void mNodeCoordinatePanel::onTableItemChanged()
	{
		if (!_currentTable)
			return;
		if (_currentMode != 1)
			return;

		auto data = _currentTable->getNodeData();
		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		manager.clearNodeCoordinateData();
		for (const auto& pair : data)
		{
			int refId = manager.getIDByCoordinateName(pair.second.first);
			int anaId = manager.getIDByCoordinateName(pair.second.second);
			if (pair.second.first.isEmpty() || pair.second.second.isEmpty())
				continue;
			manager.addNodeCoordinate(pair.first, refId, anaId);
		}
	}

	void mNodeCoordinatePanel::onClearTableClicked()
	{
		if (_currentTable)
		{
			_currentTable->clearContents();
			_currentTable->setRowCount(0);
		}
	}

	void mNodeCoordinatePanel::updateNodePickFromSelected(bool append)
	{
		if (!getCVM->getPreRenderWidget())
			return;

		int objNum = 0;
		MxFunctions::PickDataType pickDataType = getCVM->getCurrentPickDataType(objNum);
		if (objNum == 0 || pickDataType != MxFunctions::PickDataType::Node)
			return;

		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> selectedData;
		SaveSelectMemoryData(getCVM->getPreRenderWidget(), pickDataType, selectedData);
		if (selectedData.empty())
			return;

		if (!append)
		{
			_nodePick->nodeIds.clear();
			_nodePick->pickedData.clear();
		}

		// 去重并追加
		std::set<int> existingIds(_nodePick->nodeIds.begin(), _nodePick->nodeIds.end());
		for (const auto& pair : selectedData)
		{
			if (pair.first != MxFunctions::PickDataType::Node)
				continue;
			int nodeId = pair.second.result_id;
			if (existingIds.insert(nodeId).second)
			{
				_nodePick->nodeIds.push_back(nodeId);
				_nodePick->pickedData.push_back(pair);
			}
		}

		// 仅更新文本和 enabled 状态，不控制复选框勾选
		_nodePick->setState(true, false);
		// 选择/追加后清除渲染选择，避免节点一直高亮
		if (getCVM->getPreRenderWidget())
		{
			ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
			MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
			getCVM->getPreRenderWidget()->updatePaint();
		}
	}

	void mNodeCoordinatePanel::onPickNode()
	{
		if (_currentMode == 0)
		{
			updateNodePickFromSelected(false);
		}
		else
		{
			// 分别设置：选择节点，覆盖已有节点坐标数据
			pickNodesToTable(false);
		}
	}

	void mNodeCoordinatePanel::onAddNodePick()
	{
		if (_currentMode == 0)
		{
			// 统一设置：追加选中节点到拾取列表
			updateNodePickFromSelected(true);
		}
		else
		{
			// 分别设置：追加节点到表格
			pickNodesToTable(true);
		}
	}

	void mNodeCoordinatePanel::pickNodesToTable(bool append)
	{
		if (!getCVM->getPreRenderWidget())
			return;

		int objNum = 0;
		MxFunctions::PickDataType pickDataType = getCVM->getCurrentPickDataType(objNum);
		if (objNum == 0 || pickDataType != MxFunctions::PickDataType::Node)
			return;

		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> selectedData;
		SaveSelectMemoryData(getCVM->getPreRenderWidget(), pickDataType, selectedData);
		if (selectedData.empty())
			return;

		auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
		QStringList names = manager.getAllCoordinateNames();
		QString defaultName = names.isEmpty() ? QString() : names.first();

		auto nodeData = manager.getNodeCoordinateNameData();
		if (!append)
		{
			nodeData.clear();
		}

		for (const auto& pair : selectedData)
		{
			if (pair.first != MxFunctions::PickDataType::Node)
				continue;
			int nodeId = pair.second.result_id;
			// 追加时跳过已存在节点；选择时直接覆盖
			if (append && nodeData.find(nodeId) != nodeData.end())
				continue;
			nodeData[nodeId] = std::make_pair(defaultName, defaultName);
		}

		// 保存到管理类
		manager.clearNodeCoordinateData();
		for (const auto& pair : nodeData)
		{
			int refId = manager.getIDByCoordinateName(pair.second.first);
			int anaId = manager.getIDByCoordinateName(pair.second.second);
			if (pair.second.first.isEmpty() || pair.second.second.isEmpty())
				continue;
			manager.addNodeCoordinate(pair.first, refId, anaId);
		}

		// 更新节点拾取状态
		_nodePick->nodeIds.clear();
		_nodePick->pickedData.clear();
		for (const auto& pair : nodeData)
		{
			_nodePick->nodeIds.push_back(pair.first);
			RenderPickResultData data;
			data.result_id = pair.first;
			_nodePick->pickedData.emplace_back(MxFunctions::PickDataType::Node, data);
		}
		_nodePick->setState(true, false);
		// 选择/追加后清除渲染选择，避免节点一直高亮
		if (getCVM->getPreRenderWidget())
		{
			ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
			MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
			getCVM->getPreRenderWidget()->updatePaint();
		}

		// 重新打开表格显示
		openNodeCoordinateTable();
	}

	void mNodeCoordinatePanel::onClearNodePick()
	{
		if (_currentMode == 0)
		{
			// 取消高亮
			highlightPickedNodes(false);
			_nodePick->setState(false);
			if (getCVM->getPreRenderWidget())
			{
				ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
				getCVM->getPreRenderWidget()->updatePaint();
			}
		}
		else
		{
			// 分别设置：清除所有节点坐标数据
			auto& manager = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(MxFunctions::Physical_Coordinate);
			manager.clearNodeCoordinateData();
			_nodePick->setState(false);
			highlightPickedNodes(false);
			if (_currentTable)
			{
				mTableDataWindow::getInstance()->resetInitialTable();
				_currentTable.reset();
			}
		}
	}

	void mNodeCoordinatePanel::highlightPickedNodes(bool highlight)
	{
		if (!getCVM->getPreRenderWidget())
			return;

		auto* meshSelectData = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData();
		if (!meshSelectData)
			return;

		// 使用 ADD/REDUCE 控制高亮，与 MPC 保持一致
		SelectionOperate op = highlight ? SelectionOperate::ADD : SelectionOperate::REDUCE;
		std::vector<MeshSelectInfoData> meshIds;
		for (const auto& pair : _nodePick->pickedData)
		{
			if (pair.first != MxFunctions::PickDataType::Node)
				continue;
			MeshSelectInfoData info;
			info.part_id = pair.second.part_id;
			info.data_id.node_id = pair.second.result_id;
			meshIds.push_back(info);
		}
		if (!meshIds.empty())
		{
			meshSelectData->UpdateSelectDataByType(op, SelectionInfoType::MeshNode, meshIds);
		}

		UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mNodeCoordinatePanel::onHighlightPicked(int state)
	{
		_nodePick->setCheckedState(state == Qt::Checked);
		_nodePick->labelpicked->setChecked(state == Qt::Checked);
		highlightPickedNodes(state == Qt::Checked);
	}

	//////////////////////////////////////////////////////////////////////////
	mNodeCoordinateTableWidget::mNodeCoordinateTableWidget(QWidget* parent)
		: QTableWidget(parent)
	{
		initUi();
	}

	void mNodeCoordinateTableWidget::mousePressEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			QModelIndex idx = indexAt(event->pos());
			if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
				&& !(event->modifiers() & Qt::ControlModifier)
				&& !(event->modifiers() & Qt::ShiftModifier))
			{
				m_lastPressedIndex = idx;
				return;
			}
		}
		m_lastPressedIndex = QModelIndex();
		QTableWidget::mousePressEvent(event);
	}

	void mNodeCoordinateTableWidget::mouseReleaseEvent(QMouseEvent* event)
	{
		if (m_lastPressedIndex.isValid())
		{
			QModelIndex idx = indexAt(event->pos());
			if (idx == m_lastPressedIndex)
			{
				edit(idx);
			}
			m_lastPressedIndex = QModelIndex();
			return;
		}
		QTableWidget::mouseReleaseEvent(event);
	}

	void mNodeCoordinateTableWidget::mouseDoubleClickEvent(QMouseEvent* event)
	{
		if (event->button() == Qt::LeftButton)
		{
			QModelIndex idx = indexAt(event->pos());
			if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
				&& !(event->modifiers() & Qt::ControlModifier)
				&& !(event->modifiers() & Qt::ShiftModifier))
			{
				edit(idx);
				return;
			}
		}
		QTableWidget::mouseDoubleClickEvent(event);
	}

	void mNodeCoordinateTableWidget::setCoordinateNames(const QStringList& names)
	{
		_coordinateNames = names;
		if (!names.isEmpty())
		{
			auto callback = [this](int col, const QString& value) {
				this->batchSetColumn(col, value);
			};
			setItemDelegateForColumn(2, new mNodeCoordinateComboDelegate(names, callback, this));
			setItemDelegateForColumn(3, new mNodeCoordinateComboDelegate(names, callback, this));
		}
	}

	void mNodeCoordinateTableWidget::batchSetColumn(int col, const QString& value)
	{
		if (!selectionModel()) return;

		QModelIndexList selectedIndexes = selectionModel()->selectedIndexes();
		bool blocked = signalsBlocked();
		blockSignals(true);
		for (const QModelIndex& idx : selectedIndexes)
		{
			if (idx.column() != col) continue;
			QTableWidgetItem* cellItem = item(idx.row(), idx.column());
			if (cellItem) cellItem->setText(value);
		}
		blockSignals(blocked);
		for (const QModelIndex& idx : selectedIndexes)
		{
			if (idx.column() != col) continue;
			emit itemChanged(item(idx.row(), idx.column()));
		}
	}

	void mNodeCoordinateTableWidget::setNodeData(const std::map<int, std::pair<QString, QString>>& data)
	{
		auto* meshData = getCVM->getModelData()->getMeshData();
		setRowCount(static_cast<int>(data.size()));
		int row = 0;
		for (const auto& pair : data)
		{
			QTableWidgetItem* idItem = new QTableWidgetItem(MxFunctions::mNumberToString(pair.first));
			idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
			setItem(row, 0, idItem);

			QVector3D pos = meshData ? meshData->getVectorById(pair.first) : QVector3D();
			QTableWidgetItem* coordItem = new QTableWidgetItem(
				QString("%1,%2,%3").arg(pos.x()).arg(pos.y()).arg(pos.z()));
			coordItem->setFlags(coordItem->flags() & ~Qt::ItemIsEditable);
			setItem(row, 1, coordItem);

			setItem(row, 2, new QTableWidgetItem(pair.second.first));
			setItem(row, 3, new QTableWidgetItem(pair.second.second));
			row++;
		}
	}

	std::map<int, std::pair<QString, QString>> mNodeCoordinateTableWidget::getNodeData() const
	{
		std::map<int, std::pair<QString, QString>> data;
		for (int row = 0; row < rowCount(); ++row)
		{
			QTableWidgetItem* idItem = item(row, 0);
			QTableWidgetItem* refItem = item(row, 2);
			QTableWidgetItem* anaItem = item(row, 3);
			if (!idItem || !refItem || !anaItem)
				continue;
			int nodeId = idItem->text().toInt();
			data[nodeId] = std::make_pair(refItem->text(), anaItem->text());
		}
		return data;
	}

	void mNodeCoordinateTableWidget::initUi()
	{
		setColumnCount(4);
		QStringList headers;
		headers << "节点ID" << "节点坐标" << "参考坐标系" << "分析坐标系";
		setHorizontalHeaderLabels(headers);
		horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		setSelectionBehavior(QAbstractItemView::SelectItems);
		setSelectionMode(QAbstractItemView::ExtendedSelection);
		setAlternatingRowColors(true);
		setStyleSheet(R"(
			QHeaderView::section {
				background: #F7F8FA;
				border: 1px solid #dcdcdc;
				border-left: 0;
				padding: 4px;
				font-weight: normal;
			}
			QHeaderView::section:vertical {
				background: #F7F8FA;
				border: 1px solid #dcdcdc;
				border-top: 0;
				padding: 4px;
				font-weight: normal;
			}
			QTableWidget {
				gridline-color: #d0e6f7;
				alternate-background-color: #ECF5FF;
				selection-background-color: #add8e6;
				selection-color: black;
			}
			QTableWidget::item {
				background: transparent;
			}
		)");
	}

	//////////////////////////////////////////////////////////////////////////
	mNodeCoordinateComboDelegate::mNodeCoordinateComboDelegate(const QStringList& items, BatchCallback callback, QObject* parent)
		: QStyledItemDelegate(parent)
		, _items(items)
		, _callback(callback)
	{
	}

	QWidget* mNodeCoordinateComboDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& /*option*/,
		const QModelIndex& index) const
	{
		QComboBox* editor = new QComboBox(parent);
		editor->setFrame(false);
		editor->addItems(_items);
		if (_callback)
		{
			connect(editor, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
				this, [this, index](const QString& text) {
					if (_callback) _callback(index.column(), text);
				});
		}
		return editor;
	}

	void mNodeCoordinateComboDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
	{
		QString text = index.model()->data(index, Qt::EditRole).toString();
		QComboBox* comboBox = static_cast<QComboBox*>(editor);
		int tIndex = comboBox->findText(text);
		if (tIndex >= 0)
			comboBox->setCurrentIndex(tIndex);
	}

	void mNodeCoordinateComboDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const
	{
		QComboBox* comboBox = static_cast<QComboBox*>(editor);
		model->setData(index, comboBox->currentText(), Qt::EditRole);
	}
}
