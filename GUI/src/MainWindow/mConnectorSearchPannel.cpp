#include "mConnectorSearchPannel.h"
#include "mConnector.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDisplayMenuFunction.h"
#include "PickCommonFunction.h"
#include "mCommonds.h"
#include "mCommondSystem.h"
#include "mMeshData.h"
#include "mDockLayoutManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"
#include "mOtherFunction.h"


using namespace MechPreData;
using namespace MechFunctionData;
using namespace MechConnectorData;
namespace MechMainWindow
{
	ConnectorSearchTable::ConnectorSearchTable(QWidget* parent)
		: QTableWidget(parent)
	{
		setColumnCount(4);
		setHorizontalHeaderLabels({
			"连接名",
			"主节点ID",
			"从节点ID",
			"删除"
			});

		horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
		verticalHeader()->setVisible(false);
		// 允许多行选择
		setSelectionMode(QAbstractItemView::ExtendedSelection);
		// 行选择模式（选中整行）
		setSelectionBehavior(QAbstractItemView::SelectRows);
		setFocusPolicy(Qt::NoFocus); // 避免 cellWidget 抢焦点导致表格重置 selection
		createWidgetUI();
	}

	ConnectorSearchTable::~ConnectorSearchTable() {}

	void ConnectorSearchTable::createWidgetUI()
	{
		_widget = new QWidget;
		_widget->hide();
		auto* layout = new QVBoxLayout(_widget);

		auto* toolLayout = new QHBoxLayout;

		_filterCol = new QComboBox; _filterCol->addItems({ "连接名","主节点ID","从节点ID"});
		_filterText = new QLineEdit;

		_sortCol = new QComboBox; _sortCol->addItems({ "连接名","主节点ID","从节点ID" });
		_sortOrder = new QComboBox; _sortOrder->addItems({ "升序","降序" });

		toolLayout->addSpacing(20);
		toolLayout->addWidget(new QLabel("过滤:"));
		toolLayout->addWidget(_filterCol);
		toolLayout->addWidget(_filterText);
		// toolLayout->addWidget(btnFilter);
		toolLayout->addSpacing(20);
		toolLayout->addWidget(new QLabel("排序:"));
		toolLayout->addWidget(_sortCol);
		toolLayout->addWidget(_sortOrder);
		// toolLayout->addWidget(btnSort);

		auto space = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);
		toolLayout->addSpacerItem(space);
		auto createButton = new QPushButton("创建");
		toolLayout->addWidget(createButton);
		connect(createButton, &QPushButton::clicked, this, &ConnectorSearchTable::onCreateClicked);

		layout->addLayout(toolLayout);
		layout->addWidget(this);

		// ====== 实时过滤 ======
		connect(_filterText, &QLineEdit::textChanged, this, [=](const QString&) {
			filterByColumn(_filterCol->currentIndex(), _filterText->text());
			});
		connect(_filterCol, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [=](int) {
			filterByColumn(_filterCol->currentIndex(), _filterText->text());
			});

		// ====== 实时排序 ======
		auto sortFunc = [=]() {
			Qt::SortOrder order = (_sortOrder->currentIndex() == 0) ? Qt::AscendingOrder : Qt::DescendingOrder;
			sortByColumnAndOrder(_sortCol->currentIndex(), order);
			};

		connect(_sortCol, QOverload<int>::of(&QComboBox::currentIndexChanged), this, sortFunc);
		connect(_sortOrder, QOverload<int>::of(&QComboBox::currentIndexChanged), this, sortFunc);

		connect(this, &QTableWidget::itemSelectionChanged,
			this, &ConnectorSearchTable::onItemSelectionChanged);
	}

	void ConnectorSearchTable::refreshTable()
	{
		clearContents();
		setRowCount(0);
		if (!_connRigid.isEmpty())
		{
			//填入表格
			for (int i = 0; i < _connRigid.size(); ++i)
			{
				insertRow(i);
				fillRowFromRigidData(i, _connRigid[i]);
			}
		}
		if (!_connFlexible.isEmpty())
		{
			for (int i = 0; i < _connFlexible.size(); ++i)
			{
				insertRow(i);
				fillRowFromFlexibleData(i, _connFlexible[i]);
			}
		}
	}

	void ConnectorSearchTable::fillRowFromRigidData(int row, std::shared_ptr<MechConnectorData::mConnectorRigid>connRigid)
	{
		// 0 连接名
		QLineEdit* nameEdit = new QLineEdit(QString("%1").arg(connRigid->getConnectorName()));
		setCellWidget(row, 0, nameEdit);
		// 连接信号，修改数据层
		connect(nameEdit, &QLineEdit::textChanged, this, [this, connRigid](const QString& text) {
			//先删除原来的渲染
			deleteConnRend({ connRigid->getConnectorName() });
			connRigid->setConnectorName(text);
			createRigidConnRend({ connRigid });
			});

		// ==== 主节点 ====
		QString showMain = MxFunctions::mNumberToString(connRigid->mainnodes_.second.result_id);

		auto* item1 = new QTableWidgetItem(showMain);
		item1->setData(Qt::UserRole, showMain);  
		item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
		setItem(row, 1, item1);

		// ==== 从节点 ====
		QString showSecond = "";
		auto subnodes = connRigid->subnodes_[MxFunctions::PickDataType::Node];
		if (subnodes.size() > 0)
		{
			if (subnodes.size() <=2 )
			{
				for (int i = 0 ; i < subnodes.size(); ++i)
				{
					showSecond += MxFunctions::mNumberToString(subnodes[i].result_id)+" ";
				}
			}
			else
			{
				showSecond = QString("%1 %2 等").arg(subnodes[0].result_id).arg(subnodes[1].result_id);
			}
		}
		QString tipSecond = "";
		int cnt = 0;
		for (int i = 0; i < subnodes.size(); ++i)
		{
			tipSecond += MxFunctions::mNumberToString(subnodes[i].result_id) + " ";
			++cnt;
			if (cnt % 6 == 0)
				tipSecond += "\n";
		}

		auto* item2 = new QTableWidgetItem(showSecond);
		item2->setToolTip(tipSecond);
		item2->setData(Qt::UserRole, showSecond);
		item2->setFlags(item1->flags() & ~Qt::ItemIsEditable);
		setItem(row, 2, item2);

		// ==== 删除 ====
		QPushButton* btnDel = new QPushButton("删除");
		btnDel->setFocusPolicy(Qt::NoFocus);
		setCellWidget(row, 3, btnDel);
		connect(btnDel, &QPushButton::clicked, this, &ConnectorSearchTable::onDeleteClicked);
	}
	
	void ConnectorSearchTable::fillRowFromFlexibleData(int row, std::shared_ptr<MechConnectorData::mConnectorFlexible>connFlexible)
	{
		// 0 连接名
		QLineEdit* nameEdit = new QLineEdit(QString("%1").arg(connFlexible->getConnectorName()));
		setCellWidget(row, 0, nameEdit);
		// 连接信号，修改数据层
		connect(nameEdit, &QLineEdit::textChanged, this, [this, connFlexible](const QString& text) {
			deleteConnRend({ connFlexible->getConnectorName()});
			connFlexible->setConnectorName(text);
			createFlexibleConnRend({ connFlexible });
			});

		// ==== 从节点 ====
		QString showSecond = MxFunctions::mNumberToString(connFlexible->subnodes_.second.result_id);

		auto* item1 = new QTableWidgetItem(showSecond);
		item1->setData(Qt::UserRole, showSecond);
		item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
		setItem(row, 2, item1);

		// ==== 主节点 ====
		QString showMain = "";
		auto subnodes = connFlexible->mainnodes_[MxFunctions::PickDataType::Node];
		if (subnodes.size() > 0)
		{
			if (subnodes.size() <= 2)
			{
				for (int i = 0; i < subnodes.size(); ++i)
				{
					showMain += MxFunctions::mNumberToString(subnodes[i].result_id) + " ";
				}
			}
			else
			{
				showMain = QString("%1 %2 等").arg(subnodes[0].result_id).arg(subnodes[1].result_id);
			}
		}
		QString tipMain = "";
		int cnt = 0;
		for (int i = 0; i < subnodes.size(); ++i)
		{
			tipMain += MxFunctions::mNumberToString(subnodes[i].result_id) + " ";
			++cnt;
			if (cnt % 6 == 0)
				tipMain += "\n";
		}

		auto* item2 = new QTableWidgetItem(showMain);
		item2->setToolTip(tipMain);
		item2->setData(Qt::UserRole, showMain);
		item2->setFlags(item1->flags() & ~Qt::ItemIsEditable);
		setItem(row, 1, item2);

		// ==== 删除 ====
		QPushButton* btnDel = new QPushButton("删除");
		btnDel->setFocusPolicy(Qt::NoFocus);
		setCellWidget(row, 3, btnDel);
		connect(btnDel, &QPushButton::clicked, this, &ConnectorSearchTable::onDeleteClicked);
	}

	// 根据单元格 widget 找到其所在行（线性查找，表行数一般不多）
	int ConnectorSearchTable::findRowOfWidget(QWidget* w)
	{
		if (!w) return -1;
		for (int r = 0; r < rowCount(); ++r) {
			for (int c = 0; c < columnCount(); ++c) {
				QWidget* cw = cellWidget(r, c);
				if (cw == w) return r;
				// 某些情况下 nameEdit 是 QLineEdit 放在 cellWidget
			}
		}
		return -1;
	}

	void ConnectorSearchTable::setRigidConnData(const QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>& rigid)
	{
		_connRigid = rigid;
		_connFlexible.clear();
		refreshTable();
	}
	
	void ConnectorSearchTable::setFlexibleConnData(const QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>& flexible)
	{
		_connFlexible = flexible;
		_connRigid.clear();
		refreshTable();
	}

	void ConnectorSearchTable::sortByColumnAndOrder(int col, Qt::SortOrder order)
	{
		struct RowInfo {
			int row;
			QString key;
		};

		QVector<RowInfo> rowsInfo;

		for (int r = 0; r < rowCount(); ++r)
		{
			QString key;

			if (col == 0)
			{
				// 0 列是 QLineEdit
				QLineEdit* le = qobject_cast<QLineEdit*>(cellWidget(r, 0));
				key = le ? le->text() : "";
			}
			else
			{
				// 1、2 列是 QTableWidgetItem，key 在 UserRole 中
				QTableWidgetItem* it = item(r, col);
				key = it ? it->data(Qt::UserRole).toString() : "";
			}

			rowsInfo.push_back({ r, key });
		}

		// 排序
		std::sort(rowsInfo.begin(), rowsInfo.end(), [=](RowInfo& a, RowInfo& b) {
			return order == Qt::AscendingOrder ? (a.key < b.key) : (a.key > b.key);
			});

		// ===== 重排数据层 =====
		if (!_connRigid.isEmpty())
		{
			QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>newConnRigid;
			for (auto info : rowsInfo)
			{
				newConnRigid.append(_connRigid[info.row]);
			}
			_connRigid = newConnRigid;
		}
		if (!_connFlexible.isEmpty())
		{
			QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>newConnFlexible;
			for (auto info : rowsInfo)
			{
				newConnFlexible.append(_connFlexible[info.row]);
			}
			_connFlexible = newConnFlexible;
		}

		// ===== 重建表格 =====
		refreshTable();
	}
	
	void ConnectorSearchTable::filterByColumn(int col, const QString& text)
	{
		QString key = text.trimmed();

		if (key.isEmpty())
		{
			for (int r = 0; r < rowCount(); ++r)
				setRowHidden(r, false);
			return;
		}

		for (int r = 0; r < rowCount(); ++r)
		{
			bool match = false;

			if (col == 0)
			{
				QLineEdit* le = qobject_cast<QLineEdit*>(cellWidget(r, 0));
				if (le) match = le->text().contains(key);
			}
			else if (col == 1 || col == 2)
			{
				QTableWidgetItem* it = item(r, col);
				QString fullKey = it->data(Qt::UserRole).toString();
				match = fullKey.contains(key);
			}

			setRowHidden(r, !match);
		}
	}

	void ConnectorSearchTable::onTypeChanged(const QString& text)
	{

	}
	
	void ConnectorSearchTable::onDeleteClicked()
	{
		QList<QTableWidgetSelectionRange> ranges = selectedRanges();
		QVector<int> delRows;

		if (!ranges.isEmpty()) {
			for (auto range : ranges)
			{
				for (int r = range.topRow(); r <= range.bottomRow(); ++r)
					delRows.push_back(r);
			}
		}
		else {
			QWidget* senderW = qobject_cast<QWidget*>(sender());
			int row = findRowOfWidget(senderW);
			if (row < 0) return;
			delRows.push_back(row);
		}

		// 从后往前删除
		std::sort(delRows.begin(), delRows.end(), std::greater<int>());
		if (!_connRigid.isEmpty())
		{
			for (int r : delRows)
				if (r < _connRigid.size()) _connRigid.erase(_connRigid.begin() + r);
		}
		if (!_connFlexible.isEmpty())
		{
			for (int r : delRows)
				if (r < _connFlexible.size()) _connFlexible.erase(_connFlexible.begin() + r);
		}

		refreshTable();
	}
	
	void ConnectorSearchTable::onCreateClicked()
	{
		if (!_connRigid.isEmpty())
		{
			for (auto connData : _connRigid)
			{
				getCVM->getModelData()->addPhysicalFieldP(connData);
				mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建连接 %1").arg(connData->getConnectorName()), true);
				emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorSig(true, connData->getConnectorId());

				//添加该连接的主从节点
				std::vector<int>nodesid;
				nodesid.push_back(connData->mainnodes_.second.result_id);
				auto subnodes = connData->subnodes_[MxFunctions::PickDataType::Node];
				for (auto node : subnodes)
				{
					nodesid.push_back(node.result_id);
				}
				getCVM->getModelData()->getMeshData()->appendConnectNodes(nodesid);

				mCommondConnector* commond = new mCommondConnector(mCommondBase::CommondType::CONNECTOR_CREATEUNDO);
				commond->_connectorData = connData;
				mCommondSystem::getInstance()->addCommond(commond);
			}
		}
		if(!_connFlexible.isEmpty())
		{
			for (auto connData : _connFlexible)
			{
				getCVM->getModelData()->addPhysicalFieldP(connData);
				mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建连接 %1").arg(connData->getConnectorName()), true);
				emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorSig(true, connData->getConnectorId());

				//添加该连接的主从节点
				std::vector<int>nodesid;
				nodesid.push_back(connData->subnodes_.second.result_id);
				auto subnodes = connData->mainnodes_[MxFunctions::PickDataType::Node];
				for (auto node : subnodes)
				{
					nodesid.push_back(node.result_id);
				}
				getCVM->getModelData()->getMeshData()->appendConnectNodes(nodesid);

				mCommondConnector* commond = new mCommondConnector(mCommondBase::CommondType::CONNECTOR_CREATEUNDO);
				commond->_connectorData = connData;
				mCommondSystem::getInstance()->addCommond(commond);
			}
		}
	}

	void ConnectorSearchTable::onItemSelectionChanged()
	{
		QSet<QString>connRendNames;
		QModelIndexList list = this->selectionModel()->selectedRows();
		bool isrigid = !_connRigid.isEmpty();
		QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>rigidconns;
		QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>flexibleconns;
		for (QModelIndex l : list)
		{
			int row = l.row();
			// 获取第一列的QLineEdit
			QLineEdit* nameEdit = qobject_cast<QLineEdit*>(this->cellWidget(row, 0));
			QString connectorName = nameEdit->text();
			connRendNames.insert(connectorName);
			//添加渲染
			if (isrigid)
			{
				rigidconns.append(_connRigid[row]);
			}
			else
			{
				flexibleconns.append(_connFlexible[row]);
			}
		}
		//移除之前的渲染
		std::vector<QString> deleteRendNames;
		for (auto name : _connRendNames)
		{
			deleteRendNames.push_back(name);
		}
		deleteConnRend(deleteRendNames);
		
		_connRendNames.clear();

		//渲染
		if (isrigid)
		{
			createRigidConnRend(rigidconns);
		}
		else
		{
			createFlexibleConnRend(flexibleconns);
		}

	}

	void ConnectorSearchTable::assignConnector(const QString& newsymbol, const QVector3D& masterpos, const QVector<QVector3D>& slavepos, const ConnectorType& conntype)
	{
		auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();

		if (slavepos.isEmpty() || masterpos.isNull())
		{
			return;
		}
		switch (conntype)
		{
		case MxFunctions::Connector_Rigid:
		{
			const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
			arrow->_masterPos = masterpos;
			arrow->_slavePos = slavepos;
			arrow->_txt = "";
			arrow->_color = QVector3D(74.0f, 160.0f, 67.0f);
			symbolController->createSymbol(arrow);
			symbolController->UpdateAllSymbol();
			break;
		}
		case MxFunctions::Connector_Flexible:
		{
			const auto& arrow = symbolController->getOrCreateSymbolData<Line_Connect_Implicit>(newsymbol);
			arrow->_masterPos = masterpos;
			arrow->_slavePos = slavepos;
			arrow->_txt = "";
			arrow->_color = QVector3D(66.0f, 128.0f, 220.0f);
			symbolController->createSymbol(arrow);
			symbolController->UpdateAllSymbol();
			break;
		}
		default:
			break;
		}
		_connRendNames.insert(newsymbol);
		getCVM->getPreRenderWidget()->updatePaint();
	}

	void ConnectorSearchTable::createRigidConnRend(const QVector<std::shared_ptr<MechConnectorData::mConnectorRigid>>& rigid)
	{
		for (auto conn : rigid)
		{
			QVector3D masterpos;
			masterpos = getCVM->getModelData()->getMeshData()->getVectorById(conn->mainnodes_.second);

			QVector<QVector3D>slaveposs;
			auto subnodes = conn->subnodes_[PickDataType::Node];
			for (int i = 0; i < subnodes.size(); ++i)
			{
				QVector3D slavepos;
				slavepos = getCVM->getModelData()->getMeshData()->getVectorById(subnodes[i].result_id);
				slaveposs.append(slavepos);
			}

			QString newsymbol = QString("connector_%1").arg(conn->getConnectorId());
			assignConnector(newsymbol, masterpos, slaveposs, ConnectorType::Connector_Rigid);
		}
	}

	void ConnectorSearchTable::createFlexibleConnRend(const QVector<std::shared_ptr<MechConnectorData::mConnectorFlexible>>& flexible)
	{
		for (auto conn : flexible)
		{
			QVector3D masterpos;
			masterpos = getCVM->getModelData()->getMeshData()->getVectorById(conn->subnodes_.second);

			QVector<QVector3D>slaveposs;
			auto subnodes = conn->mainnodes_[PickDataType::Node];
			for (int i = 0; i < subnodes.size(); ++i)
			{
				QVector3D slavepos;
				slavepos = getCVM->getModelData()->getMeshData()->getVectorById(subnodes[i].result_id);
				slaveposs.append(slavepos);
			}

			QString newsymbol = QString("connector_%1").arg(conn->getConnectorId());
			assignConnector(newsymbol, masterpos, slaveposs, ConnectorType::Connector_Flexible);
		}
	}

	void ConnectorSearchTable::deleteConnRend(const std::vector<QString>& rendNames)
	{
		//删除原有符号
		auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
		symbolController->setSymbolShowState(rendNames, false);
		symbolController->deleteSymbol(rendNames);
		symbolController->UpdateAllSymbol();
		getCVM->getPreRenderWidget()->updatePaint();
	}








	mConnectorSearchPannel::mConnectorSearchPannel(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		
		_table = new ConnectorSearchTable();

		//连接类型
		propFold1 = new FoldSection("类型", this);
		_connType.initWidget("连接类型");
		_connType.combobox->addItem("刚性连接");
		_connType.combobox->addItem("柔性连接");
		_connType.combobox->setCurrentIndex(0);
		connect(_connType.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mConnectorSearchPannel::slot_swicthConnType);

		_connWay.initWidget("连接方式");
		_connWay.combobox->addItem("一对一");
		_connWay.combobox->addItem("一对多");
		_connWay.combobox->setCurrentIndex(0);
		connect(_connWay.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mConnectorSearchPannel::slot_swicthConnWay);
		
		propFold1->addRow(_connType.propwid);
		propFold1->addRow(_connWay.propwid);

		//拾取：通用拾取控件（参考 mLoadForcePanel 用法；连接搜索无需计算主节点按钮）
		propFold2 = new FoldSection("区域", this);
		_areaPickController = mGeneralPickUp::createPickWidget(CBBlue);
		_areaPickController->setDefultTip("主节点区域");
		_areaPickController->setAllowedTypes(&MechCommon::pickOnlyNode);
		_areaPickController->setSelectSetBtnVisible(false);
		connect(_areaPickController, &mPickWidgetController::pickActiveChanged, this, &mConnectorSearchPannel::slot_pickActiveChanged);
		connect(_areaPickController, &mPickWidgetController::dataChanged, this, &mConnectorSearchPannel::slot_areaDataChanged);

		_partPickController = mGeneralPickUp::createPickWidget(CBRed);
		_partPickController->setDefultTip("从节点部件");
		_partPickController->setAllowedTypes([]() {
			QSet<MxFunctions::PickDataType> types;
			types.insert(MxFunctions::PickDataType::Part);
			return types;
			});
		_partPickController->setSelectSetBtnVisible(false);
		connect(_partPickController, &mPickWidgetController::pickActiveChanged, this, &mConnectorSearchPannel::slot_pickActiveChanged);
		connect(_partPickController, &mPickWidgetController::dataChanged, this, &mConnectorSearchPannel::slot_partDataChanged);

		propFold2->addRow(_areaPickController->widget());
		propFold2->addRow(_partPickController->widget());

		//参数
		propFold3 = new FoldSection("参数", this);
		_allowrance.initWidget("容差", false);
		connect(_allowrance.lineeditvalue, &mCalcLineEdit::editingFinished,
			this, &mConnectorSearchPannel::slot_editFinished);
		_searchbtn.initWidget("开始搜索", true);
		_searchbtn.tablebtn->setFixedWidth(100);
		connect(_searchbtn.tablebtn, &QPushButton::clicked, this, &mConnectorSearchPannel::slot_startSearch);

		propFold3->addRow(_allowrance.propwid);
		
		slot_swicthConnWay(0);

		m_foldWidgets.append(propFold1);
		m_foldWidgets.append(propFold2);
		m_foldWidgets.append(propFold3);
		_mainLayout->addWidget(propFold1);
		_mainLayout->addWidget(propFold2);
		_mainLayout->addWidget(propFold3);
		_mainLayout->addWidget(_searchbtn.propwid);

		_mainLayout->addStretch();
	}

	mConnectorSearchPannel::~mConnectorSearchPannel()
	{
		_connType.clear();
		_connWay.clear();
		_allowrance.clear();
		if (_mainLayout)
		{
			delete _mainLayout;
			_mainLayout = nullptr;
		}
	}

	void mConnectorSearchPannel::slot_swicthConnType(int i)
	{
		int connway = _connWay.combobox->currentIndex();
		_connWay.combobox->clear();
		if (i == 0)
		{
			_connWay.combobox->addItem("一对一");
			_connWay.combobox->addItem("一对多");
			_areaPickController->setDefultTip("主节点区域");
			_partPickController->setDefultTip("从节点部件");
		}
		else
		{
			_connWay.combobox->addItem("一对一");
			_connWay.combobox->addItem("多对一");
			_areaPickController->setDefultTip("从节点区域");
			_partPickController->setDefultTip("主节点部件");
		}
		_connWay.combobox->setCurrentIndex(connway);
	}

	void mConnectorSearchPannel::slot_swicthConnWay(int i)
	{
		propFold3->setVisible(i != 0);
	}

	void mConnectorSearchPannel::slot_editFinished()
	{

	}

	std::vector<int> mConnectorSearchPannel::getPickedNodeIds()
	{
		std::vector<int>nodeIDs;
		if (!_areaPickController)
		{
			return nodeIDs;
		}
		const auto& areaData = _areaPickController->getAllData().pickedData;
		auto itNode = areaData.find(MxFunctions::PickDataType::Node);
		if (itNode != areaData.end())
		{
			for (const auto& d : itNode->second)
			{
				nodeIDs.push_back(d.result_id);
			}
		}
		return nodeIDs;
	}

	std::vector<int> mConnectorSearchPannel::getPickedPartIds()
	{
		std::vector<int>partIDs;
		if (!_partPickController)
		{
			return partIDs;
		}
		const auto& partData = _partPickController->getAllData().pickedData;
		auto itPart = partData.find(MxFunctions::PickDataType::Part);
		if (itPart == partData.end())
		{
			return partIDs;
		}
		//过滤非网格部件（沿用原 parttype >= 2 校验）
		auto meshData = getCVM->getModelData()->getMeshData();
		for (const auto& d : itPart->second)
		{
			auto partDataPtr = meshData->getPartData(static_cast<uint64_t>(d.result_id));
			if (partDataPtr && partDataPtr->getPartType() >= 2)
			{
				partIDs.push_back(d.result_id);
			}
		}
		return partIDs;
	}

	void mConnectorSearchPannel::slot_pickActiveChanged(bool isActive)
	{
		if (!isActive)
		{
			return;
		}
		//按控件区分拾取模式：区域=节点拾取(5)，部件=部件拾取(9)
		if (sender() == _partPickController)
		{
			emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
		}
		else
		{
			emit MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
		}
	}

	void mConnectorSearchPannel::slot_areaDataChanged()
	{
		//数据由通用拾取控件内部管理，此处仅做数据变化后的联动占位
	}

	void mConnectorSearchPannel::slot_partDataChanged()
	{
		//数据由通用拾取控件内部管理，此处仅做数据变化后的联动占位
	}

	void mConnectorSearchPannel::editSearchConnector()
	{
		setEditing(true);
		//激活区域拾取控件（激活后自动连接视口拾取信号）；
		//部件控件由用户点击时自行激活（全局互斥，同时仅一个激活）
		if (_areaPickController)
		{
			_areaPickController->setPickActive(true);
		}
		mTableDataWindow::getInstance()->resetInitialTable();
		mTableDataWindow::getInstance()->setCustomWidget(_table->getWidget());
	}

	void mConnectorSearchPannel::unEditSearchConnector()
	{
		if (!isEditing())
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支无需再单独清标志
		setEditing(false);
		//失活并清理通用拾取控件（失活时自动断开视口拾取信号并清理高亮）
		if (_areaPickController)
		{
			_areaPickController->setKeepHighlightOnDeactivate(false);
			_areaPickController->setPickActive(false);
			_areaPickController->setKeepHighlightOnDeactivate(true);
			_areaPickController->clearAllHighlight();
		}
		if (_partPickController)
		{
			_partPickController->setKeepHighlightOnDeactivate(false);
			_partPickController->setPickActive(false);
			_partPickController->setKeepHighlightOnDeactivate(true);
			_partPickController->clearAllHighlight();
		}
		mTableDataWindow::getInstance()->resetInitialTable();
	}

	void mConnectorSearchPannel::constructConnectorData()
	{
		//从通用拾取控件获取数据
		auto nodeIDs = getPickedNodeIds();
		auto partIDs = getPickedPartIds();

		if (nodeIDs.empty() || partIDs.empty())
		{
			return;
		}

		_connRigid.clear();
		_connFlexible.clear();

		_allConnNames.clear();
		//连接类型
		int typeidx = _connType.combobox->currentIndex();
		QString name = typeidx == 0 ? "刚性连接_" : "柔性连接_";

		mConnectorManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
		if (_connWay.combobox->currentIndex() == 0)
		{
			//计算一对一节点
			auto main2sub = calMain2Sub();

			for (auto it = main2sub.begin(); it != main2sub.end(); it++)
			{
				if (it.value())
				{

				}
				//创建连接
				QString connname = createConnName(name);
				_allConnNames.insert(connname);
				if (typeidx == 0)
				{
					auto connData = std::make_shared<MechConnectorData::mConnectorRigid>(manager.createID(), Connector_Rigid, connname);
					connData->way_ = 0;
					//对象
					connData->mainnodes_.first = MxFunctions::PickDataType::Node;
					RenderPickResultData data;
					data.result_id = it.key();
					connData->mainnodes_.second = data;
					connData->subnodes_[MxFunctions::PickDataType::Node] .push_back(it.value());
					
					_connRigid.append(connData);
				}
				else
				{
					auto connData = std::make_shared<MechConnectorData::mConnectorFlexible>(manager.createID(), Connector_Flexible, connname);
					connData->way_ = 0;
					//对象
					connData->subnodes_.first = MxFunctions::PickDataType::Node;
					RenderPickResultData data;
					data.result_id = it.key();
					connData->subnodes_.second = data;
					connData->mainnodes_[MxFunctions::PickDataType::Node].push_back(it.value());
					
					_connFlexible.append(connData);
				}
			}
		}
		else
		{
			//计算一对多节点
			auto main2subs = calMain2Subs();

			for (auto it = main2subs.begin(); it != main2subs.end(); it++)
			{
				if (it.value().empty())
				{
					continue;
				}
				//创建连接
				QString connname = createConnName(name);
				_allConnNames.insert(connname);
				if (typeidx == 0)
				{
					auto connData = std::make_shared<MechConnectorData::mConnectorRigid>(manager.createID(), Connector_Rigid, connname);
					//一对一
					connData->way_ = 1;
					//对象
					connData->mainnodes_.first = MxFunctions::PickDataType::Node;
					RenderPickResultData data;
					data.result_id = it.key();
					connData->mainnodes_.second = data;
					auto nodes = it.value();
					for (int i = 0; i < nodes.size(); ++i)
					{
						RenderPickResultData node;
						node.result_id = nodes[i];
						connData->subnodes_[MxFunctions::PickDataType::Node].push_back(node);
					}

					_connRigid.append(connData);
				}
				else
				{
					auto connData = std::make_shared<MechConnectorData::mConnectorFlexible>(manager.createID(), Connector_Flexible, connname);
					//一对多
					connData->way_ = 1;
					//对象
					connData->subnodes_.first = MxFunctions::PickDataType::Node;
					RenderPickResultData data;
					data.result_id = it.key();
					connData->subnodes_.second = data;
					auto nodes = it.value();
					for (int i = 0; i < nodes.size(); ++i)
					{
						RenderPickResultData node;
						node.result_id = nodes[i];
						connData->mainnodes_[MxFunctions::PickDataType::Node].push_back(node);
					}

					_connFlexible.append(connData);
				}
			}
		}
	}

	QString mConnectorSearchPannel::createConnName(const QString& name)
	{
		mConnectorManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
		if (_allConnNames.isEmpty())
			_allConnNames = manager.getAllConnectorNamesDup();
		int i = 1;
		QString connname = name + MxFunctions::mNumberToString(i);
		while (_allConnNames.find(connname) != _allConnNames.end())
		{
			++i;
			connname = name + MxFunctions::mNumberToString(i);
		}
		_allConnNames.insert(connname);
		return connname;
	}

	QHash<int, int> mConnectorSearchPannel::calMain2Sub()
	{
		QHash<int, int> main2sub;
		//从通用拾取控件获取数据
		auto nodeIDs = getPickedNodeIds();
		auto partIDs = getPickedPartIds();
		if (nodeIDs.empty() || partIDs.empty())
		{
			return main2sub;
		}
		main2sub = getCVM->getModelData()->getMeshData()->getNodeByPartNearestNode(partIDs, nodeIDs);
		return main2sub;
	}

	QHash<int, std::vector<int>> mConnectorSearchPannel::calMain2Subs()
	{
		QHash<int, std::vector<int>> main2subs;
		if (_allowrance.lineeditvalue->text().isEmpty())
		{
			return main2subs;
		}
		//从通用拾取控件获取数据
		auto nodeIDs = getPickedNodeIds();
		auto partIDs = getPickedPartIds();
		if (nodeIDs.empty() || partIDs.empty())
		{
			return main2subs;
		}
		double tolerance = _allowrance.lineeditvalue->text().trimmed().toDouble();
		main2subs = getCVM->getModelData()->getMeshData()->getNodeByPartNearestNode(partIDs, nodeIDs, tolerance);
		return main2subs;
	}

	void mConnectorSearchPannel::slot_startSearch()
	{
		//构造数据
		constructConnectorData();

		//传入表格
		if (_connType.combobox->currentIndex() == 0)
		{
			_table->setRigidConnData(_connRigid);
		}
		else
		{
			_table->setFlexibleConnData(_connFlexible);
		}

		mDockLayoutManager::getInstance()->activateDock("Table");
	}
}