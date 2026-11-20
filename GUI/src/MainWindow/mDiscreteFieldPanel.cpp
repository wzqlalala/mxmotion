#pragma execution_character_set("utf-8")

#include "mDiscreteFieldPanel.h"
#include "mDiscreteField.h"
#include "mFieldManager.h"
#include "mModelData.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDataStatusManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QLabel>
#include <QColorDialog>
#include <QTableWidget>
#include <QHeaderView>
#include <QIntValidator>
#include <QApplication>

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

#include "mDockLayoutManager.h"
#include "mTableDataWindow.h"
#include "mOtherFunction.h"

using namespace MechDiscreteFieldData;
using namespace MechPreData;

namespace MechMainWindow
{
	mDiscreteFieldPanel::mDiscreteFieldPanel(QWidget* parent)
		: mPanelBase(parent)
	{
        _discreteFieldData = nullptr;

		initUI();
	}

	mDiscreteFieldPanel::~mDiscreteFieldPanel()
	{
	}

	void mDiscreteFieldPanel::initUI()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		// 定义位置
		FoldSection* locateFold = new FoldSection("定义位置", this);
		_radioElement = new QRadioButton("单元");
		_radioNode = new QRadioButton("节点");
		QHBoxLayout* locateLayout = new QHBoxLayout();
		locateLayout->addWidget(_radioElement);
        locateLayout->addStretch();
		locateLayout->addWidget(_radioNode);
		locateLayout->addStretch();
		locateFold->addRow(locateLayout);
		m_foldWidgets.append(locateFold);
		mainLayout->addWidget(locateFold);

		// 定义
		defineFold = new FoldSection("定义", this);
		_comboType = new QComboBox();
		_comboType->addItems({ "角度场", "厚度场", "坐标系场" });

		_comboTypeNodeOnly = new QComboBox();
		_comboTypeNodeOnly->addItem("厚度场");
		_comboTypeNodeOnly->setVisible(false);

		defineFold->addRow(new PanelLabel("类型"), _comboType, _comboTypeNodeOnly);
		m_foldWidgets.append(defineFold);
		mainLayout->addWidget(defineFold);

		// 场数据
		FoldSection* dataFold = new FoldSection("场数据", this);
		_stackedWidget = new QStackedWidget(this);

		// 角度场页面
		_pageAngel = new QWidget();
		QFormLayout* angelFormLayout = new QFormLayout(_pageAngel);
		angelFormLayout->setContentsMargins(0, 0, 0, 0);

		      _editDefaultAngel = new QLineEdit();
		      angelFormLayout->addRow(new PanelLabel("默认角度"), _editDefaultAngel);

		      auto dataLabel = new QLabel("离散角度场数据");
		      _btnAngelTable = new QPushButton();
		      _btnAngelTable->setFixedSize(30, 30);
		      QIcon icon(":/mMainWidgets/Icon/table.png");
		      _btnAngelTable->setIcon(icon);
		      _btnAngelTable->setIconSize(_btnAngelTable->size());
		      _btnAngelTable->setStyleSheet(
		          "QPushButton:pressed {"
		          "     background-color: #666666;"
		          "     border: 2px solid #333333;"
		          "}"
		      );
		      angelFormLayout->addRow(dataLabel, _btnAngelTable);

		      angelFormLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
		_stackedWidget->addWidget(_pageAngel);

		// 厚度场页面
		_pageThickness = new QWidget();
		QFormLayout* thicknessFormLayout = new QFormLayout(_pageThickness);
		thicknessFormLayout->setContentsMargins(0, 0, 0, 0);

		      _editDefaultThickness = new QLineEdit();
		      _labelDefaultThickness = new PanelLabel("默认厚度");
		      thicknessFormLayout->addRow(_labelDefaultThickness, _editDefaultThickness);

		      auto thickDataLabel = new QLabel("离散厚度场数据");
		      _btnThicknessTable = new QPushButton();
		      _btnThicknessTable->setFixedSize(30, 30);
		      _btnThicknessTable->setIcon(icon);
		      _btnThicknessTable->setIconSize(_btnThicknessTable->size());
		      _btnThicknessTable->setStyleSheet(
		          "QPushButton:pressed {"
		          "     background-color: #666666;"
		          "     border: 2px solid #333333;"
		          "}"
		      );
		      thicknessFormLayout->addRow(thickDataLabel, _btnThicknessTable);

		      thicknessFormLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
		_stackedWidget->addWidget(_pageThickness);

		// 坐标系场页面
		_coorWidget = new QWidget();
		_coorTypeLabel = new PanelLabel("坐标系类型");
		_comboCoorType = new QComboBox();
		_comboCoorType->addItems({ "直角坐标系", "柱坐标系", "球坐标系" });
		QFormLayout* coorWidgetFormLayout = new QFormLayout(_coorWidget);
		coorWidgetFormLayout->setContentsMargins(0, 0, 0, 0);
		QHBoxLayout* comboCoorLayout = new QHBoxLayout();
		comboCoorLayout->addSpacing(5);
		comboCoorLayout->addWidget(_comboCoorType);
		coorWidgetFormLayout->addRow(_coorTypeLabel, comboCoorLayout);

		_coorWidget->setParent(nullptr);
		_pageCoor = new QWidget();
		QFormLayout* coorPageFormLayout = new QFormLayout(_pageCoor);
		coorPageFormLayout->setContentsMargins(0, 0, 0, 0);

		// 方向点1 行
		_directPoint1Widget = new mPointPickerWidget(QStringLiteral("方向点1"), this);
		_directPoint1Widget->setParent(nullptr);
		connect(_directPoint1Widget, &mPointPickerWidget::coordinateChanged, this, [this]() {
			if (!_discreteFieldData) return;
			QVector3D pt = _directPoint1Widget->coordinate();
			if (pt == _discreteFieldData->getDirectPoint1()) return;
			_discreteFieldData->setDirectPoint1(pt);
			this->updatePointRender("direction1", pt, QVector3D(1, 0, 1));
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
			});
		coorPageFormLayout->addRow(_directPoint1Widget);

		// 方向点2 行
		_directPoint2Widget = new mPointPickerWidget(QStringLiteral("方向点2"), this);
		_directPoint2Widget->setParent(nullptr);
		connect(_directPoint2Widget, &mPointPickerWidget::coordinateChanged, this, [this]() {
			if (!_discreteFieldData) return;
			QVector3D pt = _directPoint2Widget->coordinate();
			if (pt == _discreteFieldData->getDirectPoint2()) return;
			_discreteFieldData->setDirectPoint2(pt);
			this->updatePointRender("direction2", pt, QVector3D(213 / 255.000, 104 / 255.000, 25 / 255.000));
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
			});
		coorPageFormLayout->addRow(_directPoint2Widget);

		// 方向点3 行
		_directPoint3Widget = new mPointPickerWidget(QStringLiteral("方向点3"), this);
		_directPoint3Widget->setParent(nullptr);
		connect(_directPoint3Widget, &mPointPickerWidget::coordinateChanged, this, [this]() {
			if (!_discreteFieldData) return;
			QVector3D pt = _directPoint3Widget->coordinate();
			if (pt == _discreteFieldData->getDirectPoint3()) return;
			_discreteFieldData->setDirectPoint3(pt);
			this->updatePointRender("direction3", pt, QVector3D(127 / 255.000, 1, 0));
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
			});
		coorPageFormLayout->addRow(_directPoint3Widget);

		// 离散坐标系场数据行
		auto coorLabel = new QLabel("离散坐标系场数据");
		_btnCoorTable = new QPushButton();
		_btnCoorTable->setFixedSize(30, 30);
		_btnCoorTable->setIcon(icon);
		_btnCoorTable->setIconSize(_btnCoorTable->size());
		_btnCoorTable->setStyleSheet(
			"QPushButton:pressed {"
			"	 background-color: #666666;"
			"	 border: 2px solid #333333;"
			"}"
		);
		coorPageFormLayout->addRow(coorLabel, _btnCoorTable);

		coorPageFormLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

		_stackedWidget->addWidget(_pageCoor);
		dataFold->addRow(_stackedWidget);
		m_foldWidgets.append(dataFold);
		mainLayout->addWidget(dataFold);

		mainLayout->addStretch();

        table = new mDiscreteFieldTableWidget();
        connect(_btnAngelTable, &QPushButton::clicked, this, &mDiscreteFieldPanel::slot_angelDataTable);
        connect(_btnThicknessTable, &QPushButton::clicked, this,
            &mDiscreteFieldPanel::slot_thickDataTable);
        connect(_btnCoorTable, &QPushButton::clicked, this,
            &mDiscreteFieldPanel::slot_coorDataTable);

        connect(table, &mDiscreteFieldTableWidget::rowAdded,
            this, &mDiscreteFieldPanel::syncTableDataToField);
        connect(table, &mDiscreteFieldTableWidget::rowRemoved,
            this, &mDiscreteFieldPanel::syncTableDataToField);
        connect(table, &mDiscreteFieldTableWidget::tableCleared,
            this, &mDiscreteFieldPanel::syncTableDataToField);
        connect(table, &mDiscreteFieldTableWidget::rowCountSetByInput,
            this, &mDiscreteFieldPanel::syncTableDataToField);
        connect(table, &mDiscreteFieldTableWidget::cellTextChanged,
            this, &mDiscreteFieldPanel::syncTableDataToField);

		// 连接类型切换信号
		connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &mDiscreteFieldPanel::onTypeChanged);

		// 连接位置切换信号
		connect(_radioElement, &QRadioButton::toggled,
			this, &mDiscreteFieldPanel::onLocateChanged);
		connect(_radioNode, &QRadioButton::toggled,
			this, &mDiscreteFieldPanel::onLocateChanged);

		// 初始化默认厚度控件可见性
		updateDefaultThicknessVisibility();

		// 连接数据变更信号——角度场
		connect(_editDefaultAngel, &QLineEdit::textChanged, this, [=](const QString& text) {
            if (_discreteFieldData)
            {
                _discreteFieldData->setDefaultAngel(text);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
            }
			});

		// 连接数据变更信号——厚度场
		connect(_editDefaultThickness, &QLineEdit::textChanged, this, [=](const QString& text) {
            if (_discreteFieldData)
            {
                _discreteFieldData->setDefaultThickness(text);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
            }
			});

		// 连接数据变更信号——坐标系场
		connect(_comboCoorType, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, [=](int index) {
				if (!_discreteFieldData) return;
				MxFunctions::CoordinateType type = MxFunctions::Cartesian;
				if (index == 1)
					type = MxFunctions::Cylindrical;
				else if (index == 2)
					type = MxFunctions::Spherical;
				_discreteFieldData->setCoorType(type);
				if (_currentTableType == TableDataType::Coor)
					slot_coorDataTable();
			});

		//updateTypeComboByLocate();
	}

	void mDiscreteFieldPanel::newDiscreteFieldPanel(MxFunctions::DiscreteSpace spaceType)
	{
        // 重置界面到默认状态
        uneditDiscreteFieldPanel();

		if (!getCVM)
			return;

		int fieldID = getCVM->getModelData()
			->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
			.createID();

		_discreteFieldData = new MechDiscreteFieldData::mDiscreteField(fieldID);
		QString name = getCVM->getModelData()->createNameByType(Physical_Field, "DiscreteField");
		_discreteFieldData->setDiscreteSpaceName(name);
        _discreteFieldData->setSpaceType(spaceType);
        //默认选择单元
        _discreteFieldData->setLocate(MxFunctions::PickDataType::Element);

        _directPoint1Widget->setCoordinate(QVector3D(0, 0, 0));
        _directPoint2Widget->setCoordinate(QVector3D(1, 0, 0));
        _directPoint3Widget->setCoordinate(QVector3D(0, 1, 0));

		getCVM->getModelData()->addPhysicalField<MechDiscreteFieldData::mDiscreteField>(_discreteFieldData);
		_discreteFieldID = fieldID;
		setEditing(true);

		

		// 默认节点+厚度场，直接显示厚度表格
		if (_discreteFieldData)
		{
			_currentTableType = TableDataType::Thick;
			slot_thickDataTable();
		}

		// 进入面板后按当前页面自动激活/取消激活计算点控件
		updatePickerActivationByPage();

		// 更新树节点状态
		emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFieldSig(true, fieldID);
	}

	void mDiscreteFieldPanel::editDiscreteFieldPanel(int fieldID)
	{
		if (!getCVM)
			return;

		_discreteFieldData = getCVM->getModelData()
			->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field)
			.getDiscreteFieldData(fieldID);
		_discreteFieldID = fieldID;

		if (!_discreteFieldData)
			return;
		setEditing(true);

        _directPoint1Widget->setCoordinate(_discreteFieldData->getDirectPoint1());
        _directPoint2Widget->setCoordinate(_discreteFieldData->getDirectPoint2());
        _directPoint3Widget->setCoordinate(_discreteFieldData->getDirectPoint3());

		// 阻塞信号，防止递归触发
		bool blockType = _comboType->blockSignals(true);
		bool blockElement = _radioElement->blockSignals(true);
		bool blockNode = _radioNode->blockSignals(true);
		bool blockCoor = _comboCoorType->blockSignals(true);

		// 设置位置
		if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Element)
			_radioElement->setChecked(true);
		else
			_radioNode->setChecked(true);

		updateTypeComboByLocate();

		// 设置场类型
		int typeIndex = 0;
		MxFunctions::DiscreteSpace spaceType = _discreteFieldData->getSpaceType();
		if (spaceType == MxFunctions::AngelSpace)
			typeIndex = 0;
		else if (spaceType == MxFunctions::ThicknessSpace)
			typeIndex = 1;
		else if (spaceType == MxFunctions::CoorSpace)
			typeIndex = 2;

		if (_radioNode->isChecked())
			typeIndex = 1; // 节点模式强制厚度场

		_comboType->setCurrentIndex(typeIndex);
		_stackedWidget->setCurrentIndex(typeIndex);

		if (typeIndex == 2)
			defineFold->insertRow(1, _coorWidget);
		else
			_coorWidget->setParent(nullptr);
		if (m_foldWidgets.size() > 1)
			m_foldWidgets[1]->refreshLayout();

		// 角度场数据
		_editDefaultAngel->setText(_discreteFieldData->getDefaultAngel());

		// 厚度场数据
		_editDefaultThickness->setText(_discreteFieldData->getDefaultThickness());

		// 坐标系场数据
		int coorIndex = 0;
		MxFunctions::CoordinateType coorType = _discreteFieldData->getCoorType();
		if (coorType == MxFunctions::Cartesian)
			coorIndex = 0;
		else if (coorType == MxFunctions::Cylindrical)
			coorIndex = 1;
		else if (coorType == MxFunctions::Spherical)
			coorIndex = 2;
		_comboCoorType->setCurrentIndex(coorIndex);


		// 恢复信号
		_comboType->blockSignals(blockType);
		_radioElement->blockSignals(blockElement);
		_radioNode->blockSignals(blockNode);
		_comboCoorType->blockSignals(blockCoor);

		// 直接刷新表格显示，不调用 onLocateChanged 避免 sync 空数据
		switch (typeIndex)
		{
		case 0:
			_currentTableType = TableDataType::Angel;
			slot_angelDataTable();
			break;
		case 1:
			_currentTableType = TableDataType::Thick;
			slot_thickDataTable();
			break;
		case 2:
			_currentTableType = TableDataType::Coor;
			slot_coorDataTable();
			break;
		}

		// 进入面板后按当前页面自动激活/取消激活计算点控件
		updatePickerActivationByPage();

		// 更新树节点状态
		emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFieldSig(false, fieldID);
	}

	void mDiscreteFieldPanel::uneditDiscreteFieldPanel()
	{
		if (!isEditing())
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支无需再单独清标志
		setEditing(false);
		_discreteFieldData = nullptr;
		_discreteFieldID = -1;

		bool blockType = _comboType->blockSignals(true);
		bool blockElement = _radioElement->blockSignals(true);
		bool blockNode = _radioNode->blockSignals(true);
		bool blockCoor = _comboCoorType->blockSignals(true);

		_radioNode->setChecked(true);
		_radioElement->setChecked(false);

		updateTypeComboByLocate();

		_comboType->setCurrentIndex(0);
		_stackedWidget->setCurrentIndex(0);
		_coorWidget->setParent(nullptr);
		if (m_foldWidgets.size() > 1)
			m_foldWidgets[1]->refreshLayout();

		_editDefaultAngel->clear();
		_editDefaultThickness->clear();
		_comboCoorType->setCurrentIndex(0);

		resetDirectPointPickers();

		_comboType->blockSignals(blockType);
		_radioElement->blockSignals(blockElement);
		_radioNode->blockSignals(blockNode);
		_comboCoorType->blockSignals(blockCoor);
	}

    void mDiscreteFieldPanel::resetDirectPointPickers()
    {
        //退出编辑时失活拾取并清空高亮与残留数据，避免占用全局拾取状态
        for (mPointPickerWidget* picker : { _directPoint1Widget, _directPoint2Widget, _directPoint3Widget })
        {
            if (!picker) continue;
            if (picker->pickController())
                picker->pickController()->setPickActive(false);
            picker->clearAllHighlight();
            picker->resetToDefault();
        }

        if (getCVM && getCVM->getPreRenderWidget())
        {
            auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
            render->deletePoint("direction1");
            render->deletePoint("direction2");
            render->deletePoint("direction3");
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mDiscreteFieldPanel::updatePickerActivationByPage()
    {
        // 坐标系场页面包含计算点控件，进入该页面激活首个计算点控件，
        // 其他页面则取消激活全部计算点控件（控件内部全局互斥，激活新的会自动失活其他）
        bool isCoorPage = (_stackedWidget && _stackedWidget->currentWidget() == _pageCoor);
        if (isCoorPage && _directPoint1Widget && _directPoint1Widget->pickController())
            _directPoint1Widget->pickController()->setPickActive(true);
        else
        {
            for (mPointPickerWidget* picker : { _directPoint1Widget, _directPoint2Widget, _directPoint3Widget })
            {
                if (picker && picker->pickController())
                {
                    picker->pickController()->setPickActive(false);
                    picker->clearAllHighlight();
                }
            }
        }
    }

	void mDiscreteFieldPanel::onTypeChanged(int index)
	{
		_stackedWidget->setCurrentIndex(index);
		if (index == 2)
			defineFold->insertRow(1, _coorWidget);
		else
	           _coorWidget->setParent(nullptr);
	       m_foldWidgets[1]->refreshLayout();

		if (_discreteFieldData)
		{
			syncTableDataToField();

			MxFunctions::DiscreteSpace type = MxFunctions::AngelSpace;
			if (index == 1)
				type = MxFunctions::ThicknessSpace;
	       else if (index == 2)
	       {
	           type = MxFunctions::CoorSpace;
	       }
				
			_discreteFieldData->setSpaceType(type);

			switch (type)
			{
			case MxFunctions::AngelSpace:
				_currentTableType = TableDataType::Angel;
				slot_angelDataTable();
				break;
			case MxFunctions::ThicknessSpace:
				_currentTableType = TableDataType::Thick;
				slot_thickDataTable();
				break;
			case MxFunctions::CoorSpace:
				_currentTableType = TableDataType::Coor;
				slot_coorDataTable();
				break;
			default:
				break;
			}
		}

		// 切换类型后按页面自动激活/取消激活计算点控件
		updatePickerActivationByPage();

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
	}

	void mDiscreteFieldPanel::updateDefaultThicknessVisibility()
	{
		bool isNode = _radioNode->isChecked();
		if (_labelDefaultThickness)
			_labelDefaultThickness->setVisible(!isNode);
		if (_editDefaultThickness)
			_editDefaultThickness->setVisible(!isNode);
	}

	void mDiscreteFieldPanel::onLocateChanged()
	{
		updateDefaultThicknessVisibility();

		if (_discreteFieldData)
		{
			syncTableDataToField();

			if (_radioElement->isChecked())
				_discreteFieldData->setLocate(MxFunctions::PickDataType::Element);
			else
				_discreteFieldData->setLocate(MxFunctions::PickDataType::Node);

			updateTypeComboByLocate();

			if (_radioNode->isChecked())
			{
				// 节点模式：强制切换到厚度场，不通过 _comboType 的 currentIndexChanged
				_stackedWidget->setCurrentIndex(1);
				_coorWidget->setParent(nullptr);
				if (m_foldWidgets.size() > 1)
					m_foldWidgets[1]->refreshLayout();

				_discreteFieldData->setSpaceType(MxFunctions::ThicknessSpace);
				_currentTableType = TableDataType::Thick;
				slot_thickDataTable();
			}
			else
			{
				// 单元模式：根据 _comboType 当前索引刷新
				int idx = _comboType->currentIndex();
				_stackedWidget->setCurrentIndex(idx);
				if (idx == 2)
					defineFold->insertRow(1, _coorWidget);
				else
					_coorWidget->setParent(nullptr);
				if (m_foldWidgets.size() > 1)
					m_foldWidgets[1]->refreshLayout();

				switch (idx)
				{
				case 0:
					_currentTableType = TableDataType::Angel;
	                   _discreteFieldData->setSpaceType(MxFunctions::AngelSpace);
					slot_angelDataTable();
					break;
				case 1:
					_currentTableType = TableDataType::Thick;
	                   _discreteFieldData->setSpaceType(MxFunctions::ThicknessSpace);
					slot_thickDataTable();
					break;
				case 2:
					_currentTableType = TableDataType::Coor;
	                   _discreteFieldData->setSpaceType(MxFunctions::CoorSpace);
					slot_coorDataTable();
					break;
				}
			}

			// 切换位置后按页面自动激活/取消激活计算点控件
			updatePickerActivationByPage();

	           getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Field, _discreteFieldData->getId());
		}
	}

    void mDiscreteFieldPanel::slot_angelDataTable()
    {
        table->clearTableData();
        _currentTableType = TableDataType::Angel;

        bool isNode = false;
        QStringList headers;
        if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Element)
            headers << "单元编号" << "角度";
        else if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Node)
        {
            headers << "节点编号" << "厚度";
            isNode = true;
        }

        table->setHeadList(headers, 220);

        if (isNode)
            table->setAngelData(std::map<int, QString>());
        else
            table->setAngelData(_discreteFieldData->getElementAngelMap());

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mDiscreteFieldPanel::slot_thickDataTable()
    {
        table->clearTableData();
        _currentTableType = TableDataType::Thick;

        bool isNode = false;
        QStringList headers;
        if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Element)
            headers << "单元编号" << "厚度";
        else if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Node)
        {
            headers << "节点编号" << "厚度";
            isNode = true;
        }

        table->setHeadList(headers, 220);

        if (isNode)
            table->setThickData(_discreteFieldData->getNodeThicknessMap());
        else
            table->setThickData(_discreteFieldData->getElementThicknessMap());

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mDiscreteFieldPanel::slot_coorDataTable()
    {
        table->clearTableData();
        _currentTableType = TableDataType::Coor;

        bool isNode = false;
        QStringList headers;
        if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Element)
            headers << "单元编号";
        else if (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Node)
        {
            headers << "节点编号";
            isNode = true;
        }

        headers << "方向点1坐标" << "方向点2坐标" << "方向点3坐标";

        table->setHeadList(headers, 220);

        if (isNode)
            table->setCoorData(std::map<int, QStringList>());
        else
            table->setCoorData(_discreteFieldData->getElementCoorMap());

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setCustomWidget(table);
        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mDiscreteFieldPanel::updateTypeComboByLocate()
    {
        bool isNode = _radioNode->isChecked();
        _comboType->setVisible(!isNode);
        _comboTypeNodeOnly->setVisible(isNode);
        if (_discreteFieldData && isNode)
            _discreteFieldData->setSpaceType(MxFunctions::ThicknessSpace);
    }

    void mDiscreteFieldPanel::updatePointRender(QString str, QVector3D point, QVector3D color)
    {
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->getCommonShapeRender()->appendPoint(str, { point }, color);
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mDiscreteFieldPanel::syncTableDataToField()
    {
        if (!_discreteFieldData)
            return;

        bool isNode = (_discreteFieldData->getLocate() == MxFunctions::PickDataType::Node);

        switch (_currentTableType)
        {
        case TableDataType::Angel:
            if (isNode)
                ;
            else
                _discreteFieldData->setElementAngelMap(table->getAngelData());
            break;

        case TableDataType::Thick:
            if (isNode)
                _discreteFieldData->setNodeThicknessMap(table->getThickData());
            else
                _discreteFieldData->setElementThicknessMap(table->getThickData());
            break;

        case TableDataType::Coor:
            if (isNode)
                ;
            else
                _discreteFieldData->setElementCoorMap(table->getCoorData());
            break;

        default:
            break;
        }

        //更新节点状态
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateFieldSig(false, _discreteFieldID);
    }

	// ================== mDiscreteFieldTableWidget ==================

	mDiscreteFieldTableWidget::mDiscreteFieldTableWidget(QWidget* parent)
		: QWidget(parent)
	{
		initUI();
	}

	void mDiscreteFieldTableWidget::initUI()
	{
		QVBoxLayout* mainLayout = new QVBoxLayout(this);

		QHBoxLayout* btnLayout = new QHBoxLayout();
		m_btnAdd = new QPushButton();
		m_btnRemove = new QPushButton();
		m_btnClear = new QPushButton();

        m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        m_btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        m_btnClear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));

        //m_btnAdd->setFixedSize(100, 35);
        //m_btnRemove->setFixedSize(100, );
        //m_btnClear->setFixedSize(24, 24);

        QString btnStyle = R"(
            QPushButton {
                background-color: #FFFFFF;
                border: 1px solid #DCDCDC;
                border-radius: 3px;
            }
            QPushButton::hover {
                background-color: #299DFF;
            }
        )";
        m_btnAdd->setStyleSheet(btnStyle);
        m_btnRemove->setStyleSheet(btnStyle);
        m_btnClear->setStyleSheet(btnStyle);

  m_editRowNum = new QLineEdit();
  m_editRowNum->setPlaceholderText("行数");
        m_editRowNum->setFixedWidth(40);
        m_editRowNum->setFixedHeight(24);
  m_editRowNum->setValidator(new QIntValidator(0, 9999, this));
        m_editRowNum->setStyleSheet(R"(
            QLineEdit {
                background-color: #FFFFFF;
                border: 1px solid #DCDCDC;
                border-radius: 3px;
                padding: 2px 4px;
            }
        )");

        //btnLayout->addSpacing(10);
		btnLayout->addWidget(m_btnAdd);
		btnLayout->addWidget(m_btnRemove);
		btnLayout->addWidget(m_btnClear);
		btnLayout->addWidget(m_editRowNum);
		btnLayout->addStretch();

		mainLayout->addLayout(btnLayout);

		m_table = new mDiscreteFieldTableEdit(this);
		m_delegate = new mDiscreteFieldItemDelegate(this);
		m_table->setItemDelegate(m_delegate);
		m_table->setAlternatingRowColors(true);
		m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
		m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);

        m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        m_table->horizontalHeader()->setStretchLastSection(false);
        m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
        

		mainLayout->addWidget(m_table);

		connect(m_btnAdd, &QPushButton::clicked, this, &mDiscreteFieldTableWidget::addRow);
		connect(m_btnRemove, &QPushButton::clicked, this, &mDiscreteFieldTableWidget::removeRow);
		connect(m_btnClear, &QPushButton::clicked, this, &mDiscreteFieldTableWidget::clearTable);
		connect(m_editRowNum, &QLineEdit::editingFinished, this, &mDiscreteFieldTableWidget::setRowByInput);
		connect(m_table, &mDiscreteFieldTableEdit::requestAddRow, this, &mDiscreteFieldTableWidget::addRow);

		connect(m_table, &QTableWidget::itemChanged, this, [=](QTableWidgetItem* item) {
		    if (!item) return;
		    // 批量同步已经在 mDiscreteFieldItemDelegate::setModelData 中完成，
		    // 这里只负责通知外部数据变化
		    emit cellTextChanged();
		});

        // ===================== 标准表格样式（完全一致） =====================
        m_table->setStyleSheet(R"(
            QTableWidget {
                background-color: #FFFFFF;
                gridline-color: #E8E8E8;
                alternate-background-color: #F0F7FF;
                selection-background-color: #D0E6F7;
                selection-color: black;
                border: 1px solid #D0D0D0;
            }
            QHeaderView::section:horizontal {
                background: #F5F5F5;
                color: #333333;
                border: 1px solid #E0E0E0;
                border-left: 0;
                padding: 4px;
                font-weight: normal;
            }
            QHeaderView::section:vertical {
                background: #F5F5F5;
                color: #333333;
                border: 1px solid #E0E0E0;
                border-top: 0;
                padding: 4px;
                font-weight: normal;
            }
            QTableWidget::item {
                color: #333333;
                padding: 4px;
            }
        )");
	}

	void mDiscreteFieldTableWidget::setHeadList(const QStringList& heads, int columnWidth)
	{
		m_table->setColumnCount(heads.size());
		m_table->setHorizontalHeaderLabels(heads);

	   m_table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	   m_table->horizontalHeader()->setStretchLastSection(false);
	   m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

	      // 循环设置固定列宽（Fixed 模式下生效）
	      for (int i = 0; i < m_table->columnCount(); i++)
	      {
	          m_table->setColumnWidth(i, columnWidth);
	      }
	}

	void mDiscreteFieldTableWidget::setAngelData(std::map<int, QString> twoColData)
	{
		bool blocked = m_table->blockSignals(true);
		_angelMapData = twoColData;
		m_table->setRowCount(static_cast<int>(_angelMapData.size()));
		int row = 0;
		for (const auto& pair : _angelMapData)
		{
			m_table->setItem(row, 0, new QTableWidgetItem(MxFunctions::mNumberToString(pair.first)));
			m_table->setItem(row, 1, new QTableWidgetItem(pair.second));
			row++;
		}
		m_table->blockSignals(blocked);
	}

	std::map<int, QString> mDiscreteFieldTableWidget::getAngelData()
	{
		_angelMapData.clear();
		for (int row = 0; row < m_table->rowCount(); ++row)
		{
			QTableWidgetItem* keyItem = m_table->item(row, 0);
			QTableWidgetItem* valItem = m_table->item(row, 1);
			if (keyItem && valItem)
			{
				bool ok = false;
				int key = keyItem->text().toInt(&ok);
				if (ok)
					_angelMapData[key] = valItem->text();
			}
		}
		return _angelMapData;
	}

	void mDiscreteFieldTableWidget::setThickData(std::map<int, QString> twoColData)
	{
		bool blocked = m_table->blockSignals(true);
		_thickMapData = twoColData;
		m_table->setRowCount(static_cast<int>(_thickMapData.size()));
		int row = 0;
		for (const auto& pair : _thickMapData)
		{
			m_table->setItem(row, 0, new QTableWidgetItem(MxFunctions::mNumberToString(pair.first)));
			m_table->setItem(row, 1, new QTableWidgetItem(pair.second));
			row++;
		}
		m_table->blockSignals(blocked);
	}

	std::map<int, QString> mDiscreteFieldTableWidget::getThickData()
	{
		_thickMapData.clear();
		for (int row = 0; row < m_table->rowCount(); ++row)
		{
			QTableWidgetItem* keyItem = m_table->item(row, 0);
			QTableWidgetItem* valItem = m_table->item(row, 1);
			if (keyItem && valItem)
			{
				bool ok = false;
				int key = keyItem->text().toInt(&ok);
				if (ok)
					_thickMapData[key] = valItem->text();
			}
		}
		return _thickMapData;
	}

	void mDiscreteFieldTableWidget::setCoorData(std::map<int, QStringList> multColData)
	{
		bool blocked = m_table->blockSignals(true);
		_coorMapData = multColData;
		m_table->setRowCount(static_cast<int>(_coorMapData.size()));
		int row = 0;
		for (const auto& pair : _coorMapData)
		{
			m_table->setItem(row, 0, new QTableWidgetItem(MxFunctions::mNumberToString(pair.first)));
			for (int col = 0; col < pair.second.size(); ++col)
			{
				m_table->setItem(row, col + 1, new QTableWidgetItem(pair.second[col]));
			}
			row++;
		}
		m_table->blockSignals(blocked);
	}

	std::map<int, QStringList> mDiscreteFieldTableWidget::getCoorData()
	{
		_coorMapData.clear();
		for (int row = 0; row < m_table->rowCount(); ++row)
		{
			QTableWidgetItem* keyItem = m_table->item(row, 0);
			if (!keyItem) continue;
			bool ok = false;
			int key = keyItem->text().toInt(&ok);
			if (!ok) continue;

			QStringList values;
			for (int col = 1; col < m_table->columnCount(); ++col)
			{
				QTableWidgetItem* valItem = m_table->item(row, col);
				values.append(valItem ? valItem->text() : QString());
			}
			_coorMapData[key] = values;
		}
		return _coorMapData;
	}

	bool mDiscreteFieldTableWidget::setCurrentText(int row, int col, QString& text)
	{
		if (row < 0 || row >= m_table->rowCount() || col < 0 || col >= m_table->columnCount())
			return false;
		m_table->setItem(row, col, new QTableWidgetItem(text));
		return true;
	}

	bool mDiscreteFieldTableWidget::setCurrentIndex(int row, int index)
	{
		if (row < 0 || row >= m_table->rowCount() || index < 0 || index >= m_table->columnCount())
			return false;
		m_table->setCurrentCell(row, index);
		return true;
	}

	void mDiscreteFieldTableWidget::clearTableData()
	{
		bool blocked = m_table->blockSignals(true);
		m_table->clearContents();
		m_table->setRowCount(0);
		m_table->blockSignals(blocked);
		_angelMapData.clear();
		_thickMapData.clear();
		_coorMapData.clear();
	}

	void mDiscreteFieldTableWidget::addRow()
	{
		int row = m_table->rowCount();
		m_table->insertRow(row);
		for (int col = 0; col < m_table->columnCount(); ++col)
		{
			m_table->setItem(row, col, new QTableWidgetItem());
		}
		emit rowAdded(row);
	}

	void mDiscreteFieldTableWidget::removeRow()
	{
		int row = m_table->currentRow();
		if (row >= 0)
		{
			m_table->removeRow(row);
			emit rowRemoved(row);
		}
	}

	void mDiscreteFieldTableWidget::clearTable()
	{
	    clearTableData();
	    emit tableCleared();
	}

	void mDiscreteFieldTableWidget::setRowByInput()
	{
		bool ok;
		int count = m_editRowNum->text().toInt(&ok);
		if (!ok || count < 0)
			return;

		int currentCount = m_table->rowCount();
		if (count == currentCount)
			return;

		if (count > currentCount)
		{
		    for (int i = currentCount; i < count; ++i)
		    {
		        addRow();
		    }
		}
		else
		{
		    for (int i = currentCount - 1; i >= count; --i)
		    {
		        m_table->removeRow(i);
		    }
		}
		emit rowCountSetByInput();
}
}
