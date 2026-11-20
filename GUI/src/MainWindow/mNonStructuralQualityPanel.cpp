#include "mNonStructuralQualityPanel.h"
#include "mAppConfiguration.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mCalcLineEdit.h"
#include "mGlobalSignals.h"
#include "mInfoWindow.h"
#include "mGeoDataBase.h"

#include "mPythonAgent.h"
#include "mPyScriptWriter.h"
#include "mPyScriptReader.h"
#include "PickCommonFunction.h"
#include "mDataStatusManager.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/basic/CameraController.h"
#include "mxr/gizmo/GizmoManager.h"


//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QCompleter>
#include <QTimer>
#include <QFormLayout>
#include <QDateTime>
#include <QInputDialog>

//BasicData
#include "mMapRelationManager.h"
#include "mPropertyBase.h"
#include "mNonStructuralQuality.h"
#include "mPropertyManager.h"
#include "mTorsionBar.h"
#include "mTube.h"
#include "mSolidHomo.h"
#include "mShellHomo.h"
#include "mConstantBeam.h"
#include "mCrossBeam.h"
#include "mShearPlate.h"
#include "mModelData.h"
#include "mMeshData.h"
#include "mMaterialManager.h"
#include "mMaterialBase.h"
#include "mSetBase.h"
#include "mPartSet.h"
#include "mElementBase.h"
#include "mDataElement.h"
#include "mSectionShapeManager.h"
#include "mSectionShapeBase.h"
#include "mSpring.h"
#include "mViscousdampsection.h"
#include "mCbush.h"
#include "mPlyLaminate.h"
//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

using namespace MechPreData;
using namespace MechSetData;
using namespace MechPropertyData;
using namespace MechElementData;
using namespace MechSectionShapeData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechMapRelation;
using namespace MechCaseData;

namespace MechMainWindow
{
    mNonStructuralQualityPanel::mNonStructuralQualityPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);

        //定义组
        FoldSection* defineFold = new FoldSection("定义", this);
        _comboType = new QComboBox();
        _comboType->addItems({"单位非结构质量（1对1）",
            "单位非结构质量（1对多）",
            "总非结构质量（1对1）",
            "总非结构质量（1对多）"});

        defineFold->addRow(new PanelLabel("类型"), _comboType);

        _comboProperty = new QComboBox();
        _comboProperty->addItems({"截面属性",
            "单元", "单元集合"});

        defineFold->addRow(new PanelLabel("属性类型"), _comboProperty);

        // 单位非结构质量值标签和输入框（用于1对多+截面属性，默认隐藏）
        _valueLabel = new QLabel("单位非结构质量值", this);
        _valueLineEdit = new QLineEdit(this);
        _valueLabel->setVisible(false);
        _valueLineEdit->setVisible(false);
        connect(_valueLineEdit, &QLineEdit::textChanged,
            this, &mNonStructuralQualityPanel::slot_nonStructuralQualityChanged);


        defineFold->addRow(_valueLabel, _valueLineEdit);

        // 连接下拉框变化信号
        connect(_comboType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mNonStructuralQualityPanel::onComboChanged);
        connect(_comboProperty, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &mNonStructuralQualityPanel::onComboChanged);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        // 1）第一种组合
        _tableUnionToCrossPropety = new QTableWidget;
        _tableUnionToCrossPropety->setColumnCount(2);
        _tableUnionToCrossPropety->verticalHeader()->setHidden(true);
        _tableUnionToCrossPropety->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("名称") << QString("单位非结构质量值");
        _tableUnionToCrossPropety->setHorizontalHeaderLabels(headerText);
        _tableUnionToCrossPropety->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableUnionToCrossPropety->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableUnionToCrossPropety->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableUnionToCrossPropety->setSelectionBehavior(QAbstractItemView::SelectItems);
        _tableUnionToCrossPropety->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        _tableUnionToCrossPropety->setMinimumHeight(270);
        //设置交替换行
        _tableUnionToCrossPropety->setAlternatingRowColors(true);
        _tableUnionToCrossPropety->setSelectionBehavior(QAbstractItemView::SelectRows);

        QGridLayout* assignLayout = new QGridLayout();
        _allEleBtn = new QToolButton();
        _allEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/xuanzequanbu.png"));
        _allEleBtn->setToolTip("全部单元集");
        _addEleBtn = new QToolButton();
        _addEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
        _addEleBtn->setToolTip("增加单元集");
        _reduceEleBtn = new QToolButton();
        _reduceEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
        _reduceEleBtn->setToolTip("减少单元集");
        _deleteEleBtn = new QToolButton();
        _deleteEleBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
        _deleteEleBtn->setToolTip("清空单元集");

        connect(_allEleBtn, &QToolButton::clicked, this, &mNonStructuralQualityPanel::onAllEleBtnClicked);
        connect(_addEleBtn, &QToolButton::clicked, this, &mNonStructuralQualityPanel::onAddEleBtnClicked);
        connect(_reduceEleBtn, &QToolButton::clicked, this, &mNonStructuralQualityPanel::onReduceEleBtnClicked);
        connect(_deleteEleBtn, &QToolButton::clicked, this, &mNonStructuralQualityPanel::onDeleteEleBtnClicked);

        connect(_tableUnionToCrossPropety, &QTableWidget::itemChanged,
            this, &mNonStructuralQualityPanel::slot_tableItemChanged);
        connect(_tableUnionToCrossPropety, &QTableWidget::itemSelectionChanged, this,
            [this]() {
            int currentRow = _tableUnionToCrossPropety->currentRow();
            _clickRow = currentRow;
            if (_propIndex == Physical_Element)
            {
                highlightRowFirstColumn(currentRow);
            }
        });
        
        connect(_tableUnionToCrossPropety, &QTableWidget::currentItemChanged, this,
            [this](QTableWidgetItem* current, QTableWidgetItem* previous) {
            if (_propIndex == Physical_Element && current && current->column() == 0)
                _pickTargetRow = current->row();
        });
        connect(_tableUnionToCrossPropety, &QTableWidget::cellClicked, this,
            [this](int row, int column) {
            if (_propIndex != Physical_Element)
                return;
            if (column == 0)
                _pickTargetRow = row;
            highlightRowFirstColumn(row);
        });

        // 将按钮放入垂直布局容器中，隐藏时自动消除空白
        QVBoxLayout* btnLayout = new QVBoxLayout();
        btnLayout->setSpacing(8);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->addWidget(_allEleBtn);
        btnLayout->addWidget(_addEleBtn);
        btnLayout->addWidget(_reduceEleBtn);
        btnLayout->addWidget(_deleteEleBtn);
        btnLayout->addStretch();
        QWidget* btnWidget = new QWidget(this);
        btnWidget->setLayout(btnLayout);

        assignLayout->addWidget(_tableUnionToCrossPropety, 0, 0, 8, 7);
        assignLayout->addWidget(btnWidget, 0, 7, 8, 1);

        defineFold->addRow(assignLayout);

        _elementModel = new QStringListModel(this);

        _mainLayout->addStretch();

        // 美化表格样式 - 浅蓝主题
        this->setStyleSheet(R"(
        QHeaderView::section {
            background-color: #87cefa;  /* 浅蓝色表头 */
            color: #2c3e50;
            font-weight: bold;
            padding: 6px;
            border: 1px solid #b0d4f1;
        }
        QTableWidget {
            gridline-color: #d0e6f7;
            alternate-background-color:#ECF5FF;
            selection-background-color: #add8e6; /* 选中浅蓝 */
            selection-color: black;
        }
        QTableWidget::item {
            background: transparent;
        }
        QTableWidget:disabled {
            background: #F2F2F2;
        }
        QTableWidget::item:disabled {
            color: #9A9A9A;
        }
        QHeaderView::section:horizontal
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-left: 0;

	        padding:4px;
	        font-weight:normal;
        }
        QHeaderView::section:vertical
        {
            background: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-top: 0;

	        padding:4px;
	        font-weight:normal;
        }
        QPushButton:disabled {
            background-color: #E6E6E6;
            color: #9A9A9A;
            border: 1px solid #C8C8C8;
        }
        QLineEdit:disabled, QComboBox:disabled {
            background-color: #F3F3F3;
            color: #888;
        }
        )");
    }

 

    mNonStructuralQualityPanel::~mNonStructuralQualityPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }

    }

    void mNonStructuralQualityPanel::newNonStructuralQualityPanel()
    {
        int propertyID;
        int nonStructuralQualityID;
        nonStructuralQualityID = getCVM->getModelData()->getManager<MechCaseData::mNonStructQualityManager>(Physical_NonStructuralQuality).createID();
       _nonStructuralQualityData = new mNonStructuralQuality(nonStructuralQualityID);

       //_nonStructuralQualityData->setNonType(_iNonType);

        _nonStructuralQualityData->setNsmName(getCVM->getModelData()->createNameByType(Physical_NonStructuralQuality, "NSM"));

      
        getCVM->getModelData()->addPhysicalField<mNonStructuralQuality>(_nonStructuralQualityData);
        setEditing(true);

        //先不映射
        ////添加默认映射关系
        //auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        //mapRelation.appendTableMapRelation({ Physical_Property, _nonStructuralQualityData->getNonStructuralQualityId() }, { Physical_PropertyShape, -1 }, 0);
        //mapRelation.appendTableMapRelation({ Physical_Property, _nonStructuralQualityData->getNonStructuralQualityId() }, { Physical_PropertyShape, -1 }, 1);

        //updatePanel();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(true, _nonStructuralQualityData->getNonStructuralQualityId());
    }

    void mNonStructuralQualityPanel::editPropertyPanel(int propertyID)
    {
        if (!getCVM)
            return;
        if (!getCVM->getPreRenderWidget())
            return;
        _nonStructuralQualityData = getCVM->getModelData()
            ->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getNonStructuralQualityData(propertyID);
        _nonStructuralQualityDataID = propertyID;
        setEditing(true);

        bool state = _comboType->blockSignals(true);
        bool state1 = _comboProperty->blockSignals(true);
        int typeIndex = static_cast<int>(_nonStructuralQualityData->getTypeIndex());
        _comboType->setCurrentIndex(typeIndex);


        auto field = _nonStructuralQualityData->getPropIndex();
        int propIndex = 0;
        if (field == Physical_Property)
            propIndex = 0;
        else if (field == Physical_Element)
            propIndex = 1;
        else if (field == Physical_Set)
            propIndex = 2;

        _comboProperty->setCurrentIndex(propIndex);
        _comboType->blockSignals(state);
        _comboProperty->blockSignals(state1);

        _valueLineEdit->setText(_nonStructuralQualityData->getUnionValue());
        updateTableAndLayout();

        //更新树节点状态
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig
        (false, _nonStructuralQualityData->getNonStructuralQualityId());

    }

    void mNonStructuralQualityPanel::uneditNonStructuralQualityPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        stopElementPick();
    }

    void mNonStructuralQualityPanel::hideEvent(QHideEvent* event)
    {
        stopElementPick();
        mPanelBase::hideEvent(event);
    }

    void mNonStructuralQualityPanel::onComboChanged()
    {
        updateTableAndLayout();
    }

    void mNonStructuralQualityPanel::updateTableAndLayout()
    {
        //模式可能变化：先结束视口单元拾取回填
        stopElementPick();

        int typeIndex = _comboType->currentIndex();
        int propIndex = _comboProperty->currentIndex();

        // 尝试获取非结构质量数据
        if (getCVM && _nonStructuralQualityDataID >= 0)
        {
            _nonStructuralQualityData = getCVM->getModelData()->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
                .getNonStructuralQualityData(_nonStructuralQualityDataID);
        }

        // 0 = 单位非结构质量（1对1）
        // 1 = 单位非结构质量（1对多）
        // 2 = 总非结构质量（1对1）
        // 3 = 总非结构质量（1对多）

        // propIndex: 0 = 截面属性, 1 = 单元, 2 = 单元集合

        if (typeIndex == 0)
        {
            _typeIndex = Non::Union;
            if (propIndex == 0)
                _propIndex = Physical_Property;
            else if (propIndex == 1)
                _propIndex = Physical_Element;
            else if (propIndex == 2)
                _propIndex = Physical_Set;
        }
        else if (typeIndex == 1)
        {
            _typeIndex = Non::MultUnion;
            if (propIndex == 0)
                _propIndex = Physical_Property;
            else if (propIndex == 1)
                _propIndex = Physical_Element;
            else if (propIndex == 2)
                _propIndex = Physical_Set;
        }
        else if (typeIndex == 2)
        {
            _typeIndex = Non::ALL;
            if (propIndex == 0)
                _propIndex = Physical_Property;
            else if (propIndex == 1)
                _propIndex = Physical_Element;
            else if (propIndex == 2)
                _propIndex = Physical_Set;
        }
        else if (typeIndex == 3)
        {
            _typeIndex = Non::MultAll;
            if (propIndex == 0)
                _propIndex = Physical_Property;
            else if (propIndex == 1)
                _propIndex = Physical_Element;
            else if (propIndex == 2)
                _propIndex = Physical_Set;
        }

        bool oldBlockState = _tableUnionToCrossPropety->blockSignals(true);

        _allEleBtn->setVisible(true);



        if (_propIndex == Physical_Property)
            {
                _valueLabel->setVisible(false);
                _valueLineEdit->setVisible(false);
                
                if (_typeIndex == Non::Union)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称") << QString("单位非结构质量值"));
                }
                else if (_typeIndex == Non::ALL)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称") << QString("总非结构质量值"));
                }
                else if (_typeIndex == Non::MultAll || _typeIndex == Non::MultUnion)
                {
                    _tableUnionToCrossPropety->setColumnCount(1);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称"));
                    _valueLabel->setVisible(true);
                    _valueLineEdit->setVisible(true);
                }
                    
                _tableUnionToCrossPropety->setRowCount(0);
                if (_nonStructuralQualityData)
                    _crossPropertyData = _nonStructuralQualityData->getCrossPropertyData();
                {
                    int row = 0;
                    for (const auto& pair : _crossPropertyData)
                    {
                        //加判断 实际的截面属性是否存在 、存在的时候在添加到界面  并将实际情况维护在数据部分
                        auto propertyData = getCVM->getModelData()->getManager<MechPropertyData::mPropertyManager>(Physical_Property)
                            .getPropertyData(pair.first);
                        if (propertyData)
                        {
                            _tableUnionToCrossPropety->insertRow(row);
                            auto item0 = new QTableWidgetItem(pair.first);
                            item0->setTextAlignment(Qt::AlignCenter);
                            item0->setFlags(item0->flags() & ~Qt::ItemIsEditable);
                            _tableUnionToCrossPropety->setItem(row, 0, item0);

                            if (_tableUnionToCrossPropety->columnCount() == 2)
                            {
                                QTableWidgetItem* item = new QTableWidgetItem(pair.second);
                                item->setTextAlignment(Qt::AlignCenter);
                                _tableUnionToCrossPropety->setItem(row, 1, item);
                            }
                            ++row;
                        }
                        
                    }
                }
            }
        else if (_propIndex == Physical_Element)
            {
                _valueLabel->setVisible(false);
                _valueLineEdit->setVisible(false);
                _allEleBtn->setVisible(false);
                if (_typeIndex == Non::Union)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("ID") << QString("单位非结构质量值"));
                }
                else if (_typeIndex == Non::MultUnion || _typeIndex == Non::MultAll)
                {
                    _tableUnionToCrossPropety->setColumnCount(1);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("ID"));
                    _valueLabel->setVisible(true);
                    _valueLineEdit->setVisible(true);
                }
                else if (_typeIndex == Non::ALL)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("ID") << QString("总非结构质量值"));
                }
                
                _tableUnionToCrossPropety->setRowCount(0);
                if (_nonStructuralQualityData)
                    _elementData = _nonStructuralQualityData->getElementData();
                {
                    int row = 0;
                    for (const auto& pair : _elementData)
                    {
                        _tableUnionToCrossPropety->insertRow(row);

                        QTableWidgetItem* item = new QTableWidgetItem(pair.first);
                        item->setTextAlignment(Qt::AlignCenter);
                        //记录编辑前值，供ID列非法输入还原
                        item->setData(Qt::UserRole, pair.first);
                        _tableUnionToCrossPropety->setItem(row, 0, item);

                        if (_tableUnionToCrossPropety->columnCount() == 2)
                        {
                            QTableWidgetItem* item1 = new QTableWidgetItem(pair.second);
                            item1->setTextAlignment(Qt::AlignCenter);
                            _tableUnionToCrossPropety->setItem(row, 1, item1);
                        }
                        ++row;
                    }
                }
            }
        else if (_propIndex == Physical_Set)
            {
                _valueLabel->setVisible(false);
                _valueLineEdit->setVisible(false);

                if (_typeIndex == Non::Union)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称") << QString("单位非结构质量值"));
                }
                else if (_typeIndex == Non::MultUnion || _typeIndex == Non::MultAll)
                {
                    _tableUnionToCrossPropety->setColumnCount(1);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称"));
                    _valueLabel->setVisible(true);
                    _valueLineEdit->setVisible(true);
                }
                else if (_typeIndex == Non::ALL)
                {
                    _tableUnionToCrossPropety->setColumnCount(2);
                    _tableUnionToCrossPropety->setHorizontalHeaderLabels(QStringList()
                        << QString("名称") << QString("总非结构质量值"));
                }

                _tableUnionToCrossPropety->setRowCount(0);
                if (_nonStructuralQualityData)
                    _elementSetData = _nonStructuralQualityData->getElementSetData();
                {
                    int row = 0;
                    for (const auto& pair : _elementSetData)
                    {
                        MechSetData::mSetBase* setBase = getCVM->getModelData()
                            ->getManager<mSetManager>(Physical_Set).getSetData(pair.first);

                        if (setBase)
                        {
                            _tableUnionToCrossPropety->insertRow(row);
                            QTableWidgetItem* item = new QTableWidgetItem(pair.first);
                            item->setTextAlignment(Qt::AlignCenter);
                            item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                            _tableUnionToCrossPropety->setItem(row, 0, item);

                            if (_tableUnionToCrossPropety->columnCount() == 2)
                            {
                                QTableWidgetItem* item1 = new QTableWidgetItem(pair.second);
                                item1->setTextAlignment(Qt::AlignCenter);
                                _tableUnionToCrossPropety->setItem(row, 1, item1);
                            }
                            
                            ++row;
                        }
                        
                    }
                }
            }
    


       
        _tableUnionToCrossPropety->blockSignals(oldBlockState);


        _nonStructuralQualityData->setTypeIndex(_typeIndex);
        _nonStructuralQualityData->setPropIndex(_propIndex);

        _tableUnionToCrossPropety->resizeColumnsToContents();

        m_foldWidgets[0]->refreshLayout();

        //单元模式下开启视口拾取回填：拾取到的单元ID自动填入当前行ID列
        if (_propIndex == Physical_Element)
            startElementPick();
    }

    void mNonStructuralQualityPanel::slot_nonStructuralQualityChanged(QString value)
    {
        _unionValue = value;
        _nonStructuralQualityData->setUnionValue(_unionValue);
    }

    void mNonStructuralQualityPanel::onAllEleBtnClicked()
    {
        bool oldBlockState = _tableUnionToCrossPropety->blockSignals(true);

        _tableUnionToCrossPropety->setRowCount(0);
        _nonStructuralQualityData = getCVM->getModelData()->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getNonStructuralQualityData(_nonStructuralQualityDataID);

        if (_propIndex == Physical_Property)
            {
                _crossPropertyData.clear();
                auto target = _nonStructuralQualityData->getCrossPropertyData();
                //获取所有的截面属性名称，和对应的单位非结构质量值
                vector<int> alls = getCVM->getModelData()->getAllPhyID(Physical_Property);
                for (auto& id : alls)
                {
                    mPropertyBase* proBase = getCVM->getModelData()
                        ->getManager<mPropertyManager>(Physical_Property).getPropertyData(id);

                    if (proBase)
                    {
                        QString propName = proBase->getPropertyName();
                        QString value = target[propName].isEmpty() ? "1" : target[propName];
                        _crossPropertyData[propName] = value;
                    }

                }
                _nonStructuralQualityData->setCrossPropertyData(_crossPropertyData);

                int row = 0;
                for (const auto& pair : _crossPropertyData)
                {
                    _tableUnionToCrossPropety->insertRow(row);
                    QTableWidgetItem* item = new QTableWidgetItem(pair.first);
                    item->setTextAlignment(Qt::AlignCenter);
                    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                    _tableUnionToCrossPropety->setItem(row, 0, item);

                    if (_tableUnionToCrossPropety->columnCount() == 2)
                    {
                        QTableWidgetItem* item1 = new QTableWidgetItem(pair.second);
                        item1->setTextAlignment(Qt::AlignCenter);
                        _tableUnionToCrossPropety->setItem(row, 1, item1);
                    }
                    
                    ++row;
                }
            }
        else if (_propIndex == Physical_Element)
            {
                _elementData.clear();
                if (_nonStructuralQualityData)
                    _elementData = _nonStructuralQualityData->getElementData();

                _tableUnionToCrossPropety->setRowCount(0);
                int row = 0;
                for (const auto& pair : _elementData)
                {
                    _tableUnionToCrossPropety->insertRow(row);

                    QTableWidgetItem* item = new QTableWidgetItem(pair.first);
                    item->setTextAlignment(Qt::AlignCenter);
                    item->setData(Qt::UserRole, pair.first);
                    _tableUnionToCrossPropety->setItem(row, 0, item);

                    if (_tableUnionToCrossPropety->columnCount() == 2)
                    {
                        QTableWidgetItem* item1 = new QTableWidgetItem(pair.second);
                        item1->setTextAlignment(Qt::AlignCenter);
                        _tableUnionToCrossPropety->setItem(row, 1, item1);
                    }
                    
                    ++row;
                }
                if (_nonStructuralQualityData)
                    _nonStructuralQualityData->setElementData(_elementData);
                emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);
            }
        else if (_propIndex == Physical_Set)
            {
                _elementSetData.clear();
                auto target = _nonStructuralQualityData->getElementSetData();
                //获取所有单元集名称
                auto elementLists = getCVM->getModelData()
                    ->getManager<MechSetData::mSetManager>(Physical_Set)
                    .getAllSetNamesByTypes(Element);
                for (auto eleStr : elementLists)
                {
                    QString vlaue = target[eleStr].isEmpty() ? "1" : target[eleStr];
                    _elementSetData[eleStr] = vlaue;
                   
                }
                _nonStructuralQualityData->setElementSetData(_elementSetData);

                int row = 0;
                for (const auto& pair : _elementSetData)
                {
                    _tableUnionToCrossPropety->insertRow(row);
                    QTableWidgetItem* item = new QTableWidgetItem(pair.first);
                    item->setTextAlignment(Qt::AlignCenter);
                    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
                    _tableUnionToCrossPropety->setItem(row, 0, item);

                    if (_tableUnionToCrossPropety->columnCount() == 2)
                    {
                        QTableWidgetItem* item1 = new QTableWidgetItem(pair.second);
                        item1->setTextAlignment(Qt::AlignCenter);
                        _tableUnionToCrossPropety->setItem(row, 1, item1);
                    }
                    
                    ++row;
                }
            }
        
        _tableUnionToCrossPropety->blockSignals(oldBlockState);

        //更新树节点状态
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityData->getNonStructuralQualityId());

    }

    void mNonStructuralQualityPanel::onAddEleBtnClicked()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        _nonStructuralQualityData = getCVM->getModelData()->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getNonStructuralQualityData(_nonStructuralQualityDataID);
        if (!_nonStructuralQualityData)
            return;

        // 根据当前类型确定候选列表
        QStringList candidates;
        bool needCompleter = false;

      
        if (_propIndex == Physical_Property)
        {
            vector<int> ids = getCVM->getModelData()->getAllPhyID(Physical_Property);
            for (auto& id : ids)
            {
                mPropertyBase* proBase = getCVM->getModelData()
                    ->getManager<mPropertyManager>(Physical_Property).getPropertyData(id);
                if (proBase)
                    candidates.append(proBase->getPropertyName());
            }
            needCompleter = true;
        }
        else if (_propIndex == Physical_Set)
        {
            candidates = getCVM->getModelData()
                ->getManager<MechSetData::mSetManager>(Physical_Set)
                .getAllSetNamesByTypes(Element);
            needCompleter = true;

        }

        int row = _tableUnionToCrossPropety->rowCount();
        _tableUnionToCrossPropety->insertRow(row);

        if (!needCompleter)
        {
            int colCount = _tableUnionToCrossPropety->columnCount();
            for (int col = 0; col < colCount; ++col)
            {
                QTableWidgetItem* item = new QTableWidgetItem("");
                item->setTextAlignment(Qt::AlignCenter);
                _tableUnionToCrossPropety->setItem(row, col, item);
            }

            //当前选中行移到新增行（同时经currentItemChanged锁定该行ID列，可直接拾取填入）
            _tableUnionToCrossPropety->setCurrentCell(row, 0);
            _tableUnionToCrossPropety->selectRow(row);

            // 同步到数据模型
            if (_propIndex == Physical_Element)
            {
                _nonStructuralQualityData->setElementData(_elementData);

            }

            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);
            return;
        }

        _elementModel->setStringList(candidates);

        // 第一列使用 QLineEdit + QCompleter
        QLineEdit* editor = new QLineEdit(_tableUnionToCrossPropety);
        _tableUnionToCrossPropety->setCellWidget(row, 0, editor);
        editor->setFocus();

        // 2列表格时，第二列给默认值
        if (_tableUnionToCrossPropety->columnCount() == 2)
        {
            QTableWidgetItem* item1 = new QTableWidgetItem("1");
            item1->setTextAlignment(Qt::AlignCenter);
            _tableUnionToCrossPropety->setItem(row, 1, item1);
        }

        // 创建 completer
        QCompleter* completer = new QCompleter(_elementModel, editor);
        completer->setCaseSensitivity(Qt::CaseInsensitive);
        completer->setFilterMode(Qt::MatchContains);
        completer->setCompletionMode(QCompleter::PopupCompletion);
        editor->setCompleter(completer);
        completer->complete();

        QPointer<QLineEdit> editorGuard(editor);
        auto finishFlag = std::make_shared<bool>(false);

        std::function<void()> finishEditing = [=]() mutable
        {
            if (*finishFlag) return;
            *finishFlag = true;

            if (!editorGuard)
            {
                if (_tableUnionToCrossPropety->rowCount() > row)
                    _tableUnionToCrossPropety->removeRow(row);
                return;
            }

            // 检查行是否还存在
            if (_tableUnionToCrossPropety->rowCount() <= row)
            {
                editorGuard->deleteLater();
                return;
            }

            QString text = editorGuard->text().trimmed();

            // 1) 空文本 -> 删除该行
            if (text.isEmpty())
            {
                _tableUnionToCrossPropety->removeRow(row);
                editorGuard->deleteLater();
                return;
            }

            // 2) 非候选 -> 删除
            QStringList candidateList = _elementModel->stringList();
            if (!candidateList.contains(text))
            {
                _tableUnionToCrossPropety->removeRow(row);
                editorGuard->deleteLater();
                return;
            }

            // 3) 去重
            bool exists = false;
            for (int i = 0; i < _tableUnionToCrossPropety->rowCount(); ++i)
            {
                if (i == row) continue;
                QTableWidgetItem* item = _tableUnionToCrossPropety->item(i, 0);
                if (item && item->text() == text)
                {
                    exists = true;
                    break;
                }
            }
            if (exists)
            {
                _tableUnionToCrossPropety->removeRow(row);
                editorGuard->deleteLater();
                return;
            }

            // 通过校验 -> 写入 item 文本，移除 cell widget
            _tableUnionToCrossPropety->removeCellWidget(row, 0);

            QTableWidgetItem* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
           
            if (_propIndex == Physical_Element)
            {
                item->setFlags(item->flags()& Qt::ItemIsEditable);
            }
            else
            {
                item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            }

            _tableUnionToCrossPropety->setItem(row, 0, item);
            editorGuard->deleteLater();

            // 同步到数据模型

            if (_propIndex == Physical_Property)
                {
                    _nonStructuralQualityData->setCrossPropertyData(_crossPropertyData);
                }
            else if (_propIndex == Physical_Set)
                {
                    _nonStructuralQualityData->setElementSetData(_elementSetData);
                }
            
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);
        };

        connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
            editor,
            [=](const QString& choice)
            {
                if (!editorGuard) return;
                editorGuard->setText(choice);
                finishEditing();
            }, Qt::UniqueConnection);

        connect(editor, &QLineEdit::editingFinished, editor, [=]() {
            finishEditing();
            }, Qt::UniqueConnection);

        //更新树节点状态
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityData->getNonStructuralQualityId());
    }

    void mNonStructuralQualityPanel::onReduceEleBtnClicked()
    {
        
        if (getCVM == nullptr)
        {
            return;
        }

        _nonStructuralQualityData = getCVM->getModelData()->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getNonStructuralQualityData(_nonStructuralQualityDataID);
        if (!_nonStructuralQualityData)
            return;

        QItemSelectionModel* select = _tableUnionToCrossPropety->selectionModel();
        QModelIndexList selectedRows = select->selectedRows();

        if (selectedRows.isEmpty() && _clickRow < 0)
            return;

        QList<int> rows;
        for (const QModelIndex& idx : selectedRows)
            rows.append(idx.row());

        if (!rows.contains(_clickRow))
            rows.append(_clickRow);

        std::sort(rows.begin(), rows.end(), std::greater<int>());

        for (int row : rows)
        {
            QTableWidgetItem* item = _tableUnionToCrossPropety->item(row, 0);
            QString key = item ? item->text().trimmed() : QString();

            if (_propIndex == Physical_Property)
                {
                    if (!key.isEmpty())
                        _crossPropertyData.erase(key);
                    _nonStructuralQualityData->setCrossPropertyData(_crossPropertyData);
                }
            else if (_propIndex == Physical_Element)
                {
                    if (!key.isEmpty())
                        _elementData.erase(key);
                    _nonStructuralQualityData->setElementData(_elementData);
                }
            else if (_propIndex == Physical_Set)
                {
                    if (!key.isEmpty())
                        _elementSetData.erase(key);
                    _nonStructuralQualityData->setElementSetData(_elementSetData);
                }
            
            _tableUnionToCrossPropety->removeRow(row);
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);

        //更新树节点状态
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityData->getNonStructuralQualityId());
        clearHighlight();
    }

    void mNonStructuralQualityPanel::onDeleteEleBtnClicked()
    {
        if (getCVM == nullptr)
        {
            return;
        }

        _nonStructuralQualityData = getCVM->getModelData()->getManager<mNonStructQualityManager>(Physical_NonStructuralQuality)
            .getNonStructuralQualityData(_nonStructuralQualityDataID);
        if (!_nonStructuralQualityData)
            return;

        _tableUnionToCrossPropety->setRowCount(0);

        if (_propIndex == Physical_Property)
                {
                    _crossPropertyData.clear();
                    _nonStructuralQualityData->setCrossPropertyData(_crossPropertyData);
                }
        else if (_propIndex == Physical_Element)
                {
                    _elementData.clear();
                    _nonStructuralQualityData->setElementData(_elementData);
                }
        else if (_propIndex == Physical_Set)
                {
                    _elementSetData.clear();
                    _nonStructuralQualityData->setElementSetData(_elementSetData);
                }

        clearHighlight();
        emit MechCommon::mMechGlobalSignals::getInstance()
            ->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);
    }

    void mNonStructuralQualityPanel::slot_tableItemChanged(QTableWidgetItem* item)
    {
        if (!item || !_nonStructuralQualityData)
            return;

        if (_propIndex == Physical_Element && item->column() == 0)
        {
            if (!validateElementIdItem(item))
                return;
        }

        mMapRelationManager& mapManager = getCVM->getModelData()->getMapRelationManager();
        mapManager.deleteMapRelation({ Physical_NonStructuralQuality, _nonStructuralQualityDataID });
        QList<int> propertyIds;
        QList<int> elementSetIds;

        // Rebuild data structure from entire table to ensure consistency

        if (_propIndex == Physical_Property)
            {
                _crossPropertyData.clear();
                for (int r = 0; r < _tableUnionToCrossPropety->rowCount(); ++r)
                {
                    QTableWidgetItem* k = _tableUnionToCrossPropety->item(r, 0);
                    QTableWidgetItem* v;
                    if(_tableUnionToCrossPropety->columnCount() == 2)
                        v = _tableUnionToCrossPropety->item(r, 1);
                    if (k && !k->text().trimmed().isEmpty())
                    {
                        if (_tableUnionToCrossPropety->columnCount() == 2)
                            _crossPropertyData[k->text().trimmed()] = (v && !v->text().trimmed().isEmpty()) ? v->text().trimmed() : "0";
                        else
                            _crossPropertyData[k->text().trimmed()] = "";

                        int propertyId = getCVM->getModelData()->getPhyIDByName(Physical_Property, k->text().trimmed());
                        mapManager.appendTableMapRelation({ Physical_NonStructuralQuality, _nonStructuralQualityDataID },
                            { Physical_Property, propertyId }, r);
                    }

                }
                _nonStructuralQualityData->setCrossPropertyData(_crossPropertyData);
            }
        else if (_propIndex == Physical_Element)
            {
                _elementData.clear();
                for (int r = 0; r < _tableUnionToCrossPropety->rowCount(); ++r)
                {
                    QTableWidgetItem* k = _tableUnionToCrossPropety->item(r, 0);
                    QTableWidgetItem* v;
                    if (_tableUnionToCrossPropety->columnCount() == 2)
                        v = _tableUnionToCrossPropety->item(r, 1);
                    if (k && !k->text().trimmed().isEmpty())
                    {
                        if (_tableUnionToCrossPropety->columnCount() == 2)
                            _elementData[k->text().trimmed()] = (v && !v->text().trimmed().isEmpty()) ? v->text().trimmed() : "0";
                        else
                            _elementData[k->text().trimmed()] = "";
                    }
                        
                }
                _nonStructuralQualityData->setElementData(_elementData);
            }
        else if (_propIndex == Physical_Set)
            {
                _elementSetData.clear();
                for (int r = 0; r < _tableUnionToCrossPropety->rowCount(); ++r)
                {
                    QTableWidgetItem* k = _tableUnionToCrossPropety->item(r, 0);
                    QTableWidgetItem* v;
                    if(_tableUnionToCrossPropety->columnCount() == 2)
                         v = _tableUnionToCrossPropety->item(r, 1);
                    if (k && !k->text().trimmed().isEmpty())
                    {
                        if (_tableUnionToCrossPropety->columnCount() == 2)
                            _elementSetData[k->text().trimmed()] = (v && !v->text().trimmed().isEmpty()) ? v->text().trimmed() : "0";
                        else
                            _elementSetData[k->text().trimmed()] = "";

                        int eleSetId = getCVM->getModelData()->getPhyIDByName(Physical_Set, k->text().trimmed());
                        mapManager.appendTableMapRelation({ Physical_NonStructuralQuality, _nonStructuralQualityDataID },
                            { Physical_Set, eleSetId }, r);
                    }

                }
                _nonStructuralQualityData->setElementSetData(_elementSetData);
            }
  
        // _Element 枚举类型下，编辑完成后触发当前行第一列单元选择
        if (_propIndex == Physical_Element)
        {
            highlightRowFirstColumn(item->row());
        }
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateNonStructuralQualitySig(false, _nonStructuralQualityDataID);

    }

    bool mNonStructuralQualityPanel::validateElementIdItem(QTableWidgetItem* item)
    {
        if (item == nullptr)
            return true;

        QString text = item->text().trimmed();

        //空ID（尚未填写的行）允许通过，编辑前值同步为空
        if (text.isEmpty())
        {
            bool old = _tableUnionToCrossPropety->blockSignals(true);
            item->setData(Qt::UserRole, QString());
            _tableUnionToCrossPropety->blockSignals(old);
            return true;
        }

        //必须为整数且为模型中存在的单元ID
        bool ok = false;
        int eleId = text.toInt(&ok);
        bool valid = ok && eleId >= 0 && getCVM != nullptr
            && getCVM->getModelData()->getMeshData()->getPartIdByEleId(eleId) >= 0;

        //不与其他行重复
        if (valid)
        {
            for (int r = 0; r < _tableUnionToCrossPropety->rowCount(); ++r)
            {
                if (r == item->row())
                    continue;
                QTableWidgetItem* other = _tableUnionToCrossPropety->item(r, 0);
                if (other && other->text().trimmed() == text)
                {
                    valid = false;
                    break;
                }
            }
        }

        if (valid)
        {
            //记录编辑前合法值，供后续非法输入还原
            bool old = _tableUnionToCrossPropety->blockSignals(true);
            item->setData(Qt::UserRole, text);
            _tableUnionToCrossPropety->blockSignals(old);
            return true;
        }

        //非法输入：还原为编辑前的值并提示
        mGlobalSignals::getInstance()->outputMessageSig(1,
            QString("单元ID无效或与其他行重复：%1，已还原").arg(text));
        QString backup = item->data(Qt::UserRole).toString();
        bool old = _tableUnionToCrossPropety->blockSignals(true);
        item->setText(backup);
        item->setData(Qt::UserRole, backup);
        _tableUnionToCrossPropety->blockSignals(old);
        return false;
    }

    void mNonStructuralQualityPanel::clearHighlight()
    {
        if (!getCVM)
        {
            return;
        }
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        //将清空后的选集同步到渲染高亮（参照mViewportModel::clearPick的清空序列），
        //否则高亮控制器仍保留旧单元，重绘后视口显示的还是旧高亮
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->updateRender();
    }

    void mNonStructuralQualityPanel::highlightRowFirstColumn(int row)
    {
        if (_propIndex != Physical_Element || row < 0)
            return;

        QTableWidgetItem* firstColItem = _tableUnionToCrossPropety->item(row, 0);
        if (firstColItem && !firstColItem->text().trimmed().isEmpty())
        {
            slot_eleIDSelect(firstColItem->text().trimmed());
        }
        else
        {
            //空ID行无单元可高亮：清除当前高亮，避免残留上一个ID的高亮
            clearHighlight();
        }
    }

    void mNonStructuralQualityPanel::slot_eleIDSelect(const QString& id)
    {
        if (getCVM->getPreRenderWidget() == nullptr)
            return;
        if (!getCVM)
        {
            return;
        }
        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
        auto meshData = getCVM->getModelData()->getMeshData();
        std::vector<int> eleIds;
        eleIds.push_back(id.toInt());
        std::vector<MeshSelectInfoData> eles;
        for (auto ele : eleIds)
        {
            MeshSelectInfoData n;
            n.data_id.element_id = ele;
            auto partId = meshData->getPartIdByEleId(ele);
            if (partId < 0)
            {
                continue;
            }
            n.part_id = meshData->getPartIdByEleId(ele);
            eles.push_back(n);
        }
        clearHighlight();
        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());


        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mNonStructuralQualityPanel::startElementPick()
    {
        if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
            return;

        stopElementPick();

        //拾取工具栏仅启用"拾取单元"，其余置灰
        QVector<bool> enabled(10, false);
        enabled[8] = true;
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(enabled);

        MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);

        //监听视口拾取完成信号
        _pickConnection = connect(getCVM->getPreRenderWidget()->getSelectionContainer(),
            &mxr::SelectionDataContainer::ReadyToReadContainer,
            this, &mNonStructuralQualityPanel::slot_pickedElementFinished);
    }

    void mNonStructuralQualityPanel::stopElementPick()
    {
        if (_pickConnection)
        {
            disconnect(_pickConnection);
            _pickConnection = QMetaObject::Connection();
            //恢复拾取工具栏全部按钮，避免常驻置灰影响其它面板
            emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
        }
        _pickTargetRow = -1;
    }

    void mNonStructuralQualityPanel::slot_pickedElementFinished()
    {
        if (_propIndex != Physical_Element || _applyingPick || !_pickConnection)
            return;
        if (getCVM == nullptr || getCVM->getPreRenderWidget() == nullptr)
        {
            stopElementPick();
            return;
        }

        //仅当点击过ID格且该行仍是当前行时才回填；否则视为未锁定，拾取不生效
        if (_pickTargetRow < 0 || _pickTargetRow != _tableUnionToCrossPropety->currentRow())
        {
            _pickTargetRow = -1;
            return;
        }

        const auto& selected = getCVM->getPreRenderWidget()->getSelectionContainer()
            ->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
        if (selected.empty())
            return;

        //一次性拾取多个单元时取最后一个
        int eleId = selected.back().data_id.element_id;
        QString newText = QString::number(eleId);

        QTableWidgetItem* item = _tableUnionToCrossPropety->item(_pickTargetRow, 0);
        if (item == nullptr)
        {
            item = new QTableWidgetItem();
            item->setTextAlignment(Qt::AlignCenter);
            _tableUnionToCrossPropety->setItem(_pickTargetRow, 0, item);
        }

        if (item->text().trimmed() != newText)
            _pickTargetRow = -1;

        //setText触发slot_tableItemChanged：按原有链路同步数据并高亮该单元
        _applyingPick = true;
        item->setText(newText);
        _applyingPick = false;
    }
}

