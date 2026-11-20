
#include "mOptimizationGoalPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mCommonFunctions.h"
#include "mDataStatusManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mStructOption.h"
#include "mOptimizationGoalData.h"
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mCalcLineEdit.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QFormLayout>
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechCaseData;
using namespace MechStructOptionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mOptimizationGoalPanel::mOptimizationGoalPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _optimizationGoalData = nullptr;

        _mainLayout = new QVBoxLayout(this);

        //优化目标组
        FoldSection* goalFold = new FoldSection("优化目标", this);
        _comboObjectiveType = new QComboBox();
        _comboObjectiveType->addItems({ "最小化", "最大化" });
        goalFold->addRow(new PanelLabel("优化方式"), _comboObjectiveType);
        connect(_comboObjectiveType, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_objectiveTypeChanged);

        _comboResponseType = new QComboBox(); _comboResponseType->addItems({ "柔度", "位移", "频率", "应力", "综合柔度", "综合频率", "柔度指数", "热柔度", "节点温度", "PSD位移", "频响位移" });
        goalFold->addRow(new PanelLabel("优化响应"), _comboResponseType);
        connect(_comboResponseType, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_responseTypeChanged);

        m_foldWidgets.append(goalFold);
        _mainLayout->addWidget(goalFold);

        //柔度响应组
       FoldSection* flexFold = new FoldSection("柔度响应", this);
        _comboCase = new QComboBox();
       connect(_comboCase, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_caseChanged);

        m_foldWidgets.append(flexFold);
        _mainLayout->addWidget(flexFold);

       //节点位移
        _pickNode = new OptGoalPosPickWidget;
        _pickNode->initWidget("节点");
        connect(_pickNode->btnok, &QPushButton::clicked, this, &mOptimizationGoalPanel::slot_getNode);
        connect(_pickNode->btnclear, &QPushButton::clicked, this, &mOptimizationGoalPanel::slot_clearNode);
        connect(_pickNode->labelpicked, &QCheckBox::stateChanged, this, &mOptimizationGoalPanel::slot_checkNode);

        _comboDof = new QComboBox();
        _comboDof->addItems({ "1", "2", "3", "4", "5", "6", "disp", "rot"});
        connect(_comboDof, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_dofChanged);

        //频率
        _orderEdit = new mCalcLineEdit("",nullptr, false, true, true);
        connect(_orderEdit, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationGoalData)
                _optimizationGoalData->_frequencyOrder = _orderEdit->text();
            });

        //PSD位移/频响位移公共控件
        _comboFrequencyTargetType = new QComboBox();
        _comboFrequencyTargetType->addItems({ "指定频率点", "最大响应点", "最小响应点", "平均", "求和", "RMS" });
        connect(_comboFrequencyTargetType, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_frequencyTargetTypeChanged);

        _editSpecifiedFrequency = new mCalcLineEdit("", nullptr, false, true, true);
        connect(_editSpecifiedFrequency, &QLineEdit::editingFinished, this, &mOptimizationGoalPanel::slot_specifiedFrequencyChanged);

        _comboDofDynamic = new QComboBox();
        _comboDofDynamic->addItems({ "1", "2", "3", "4", "5", "6" });
        connect(_comboDofDynamic, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_dofChanged);

        // 复用标签
        _labelCase = new PanelLabel("目标工况");
        _labelFrequency = new PanelLabel("目标频率");
        _labelNode = new PanelLabel("节点");
        _labelDof = new PanelLabel("自由度");
        _labelSpecifiedFrequency = new PanelLabel("指定频率点");
        _labelFrfValueType = new PanelLabel("优化类型");

        _comboFrfValueType = new QComboBox();
        _comboFrfValueType->addItems({ "幅值", "实部", "虚部" });
        connect(_comboFrfValueType, &QComboBox::currentTextChanged, this, &mOptimizationGoalPanel::slot_frfValueTypeChanged);

        //综合柔度
        _tableCompliance = new QTableWidget;
        _tableCompliance->setColumnCount(2);
        _tableCompliance->verticalHeader()->setHidden(true);
        _tableCompliance->resizeColumnsToContents();
        _tableCompliance->setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选中
        QStringList headerText;
        headerText << QString("工况名") << QString("权重");
        _tableCompliance->setHorizontalHeaderLabels(headerText);
        _tableCompliance->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableCompliance->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableCompliance->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableCompliance->setSelectionBehavior(QAbstractItemView::SelectItems);
        _tableCompliance->setMinimumHeight(270);

        //综合频率
        initFrequencyTable();

        //柔度指数
        _complianceCheck = new QCheckBox("柔度工况");
        connect(_complianceCheck, &QCheckBox::clicked, this, &mOptimizationGoalPanel::slot_complianceCheckBox);
        _frequencyCheck = new QCheckBox("模态工况");
        connect(_frequencyCheck, &QCheckBox::clicked, this, &mOptimizationGoalPanel::slot_frequencyCheckBox);

       _mainLayout->addStretch();
    }

    mOptimizationGoalPanel::~mOptimizationGoalPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
        _pickNode->clear();
    }

    void mOptimizationGoalPanel::editOptimizationGoalPanel(const QString& type)
    {
        if (!getCVM)
        {
            return;
        }
        auto structOptionBase = getCVM->getModelData()->getStructOptionBase();
        if (structOptionBase == nullptr)
        {
            structOptionBase = getCVM->getModelData()->createStructOption();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationSig(true);
        }
        _optimizationGoalData = structOptionBase->_goalData;
        if (_optimizationGoalData == nullptr)
        {
            return;
        }
        setEditing(true);

        _comboObjectiveType->setCurrentText(_optimizationGoalData->_objectiveType);
        _comboResponseType->blockSignals(true);
        type != "" ? _comboResponseType->setCurrentText(type) : _comboResponseType->setCurrentText(_optimizationGoalData->_responseType);
        _comboResponseType->blockSignals(false);
        QString currentType = _comboResponseType->currentText();

        slot_responseTypeChanged(currentType);
    }

    void mOptimizationGoalPanel::uneditOptimizationGoalPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _optimizationGoalData = nullptr;
    }

    void mOptimizationGoalPanel::updateCaseCombobox(QComboBox* combobox, QList<CaseType> caseType)
    {
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = mapRelation.getMapRelation<int>({ Physical_OptimationGoal, getCVM->getModelData()->getModelDataID() }, Physical_Case);
        QString currentText = caseManager.getCaseNameByID(id);
        combobox->blockSignals(true);
        combobox->clear();
        for (auto caseType : caseType)
        {
            combobox->addItems(getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(caseType));
        }

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else
        {
            int GoalID = getCVM->getModelData()->getModelDataID();
            if (combobox->model()->rowCount() > 0) id = caseManager.getIDByCaseName(combobox->currentText());
            else id = -1;
            mapRelation.appendMapRelation({ Physical_OptimationGoal, GoalID }, { Physical_Case, id });
        }
        combobox->blockSignals(false);
    }

    void mOptimizationGoalPanel::updateComplianceTable(bool deleteMap, QList<CaseType> caseType)
    {
        int GoalID = getCVM->getModelData()->getModelDataID();
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        if (deleteMap)
        {
            _optimizationGoalData->_caseToweight.clear();
        }

        QStringList CaseNames;
        for (auto caseType : caseType)
        {
            CaseNames += getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(caseType);
        }

        _tableCompliance->setRowCount(CaseNames.size());
        for (int i = 0; i < CaseNames.size(); i++)
        {
            int caseID = caseManager.getIDByCaseName(CaseNames[i]);
            mCalcLineEdit* weight = new mCalcLineEdit("1.0", this, true, false);
            weight->setAlignment(Qt::AlignCenter);
            QCheckBox* caseCheck = new QCheckBox(CaseNames[i]);
            caseCheck->setProperty("row", i);
            weight->setProperty("row", i);
            weight->setEnabled(false);

            if(_optimizationGoalData->_caseToweight.find(caseID) != _optimizationGoalData->_caseToweight.cend())
            {
                caseCheck->setChecked(true);
                weight->setEnabled(true);
                weight->setExpression(MxFunctions::mNumberToString(_optimizationGoalData->_caseToweight[caseID]));
            }

            _tableCompliance->setCellWidget(i, 0, caseCheck);
            _tableCompliance->setCellWidget(i, 1, weight);

            connect(caseCheck, &QCheckBox::clicked, this, [this](bool checked) {
                QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
                int caseID = getCVM->getModelData()->getPhyIDByName(Physical_Case,cb->text());
                if (caseID == -1) return;
                int row = cb->property("row").toInt();
                mCalcLineEdit* line = qobject_cast<mCalcLineEdit*>(_tableCompliance->cellWidget(row, 1));
                line->setEnabled(checked);

                if (checked)
                {
                    _optimizationGoalData->_caseToweight[caseID] = line->text().toDouble();
                }
                else
                {
                    _optimizationGoalData->_caseToweight.erase(caseID);
                }
            });

            connect(weight, &QLineEdit::editingFinished, this, [this]() {
                mCalcLineEdit* line = qobject_cast<mCalcLineEdit*>(sender());
                int row = line->property("row").toInt();
                QCheckBox* cb = qobject_cast<QCheckBox*>(_tableCompliance->cellWidget(row, 0));
                if (!cb->isChecked()) return;

                int caseID = getCVM->getModelData()->getPhyIDByName(Physical_Case, cb->text());
                if (caseID == -1) return;
                _optimizationGoalData->_caseToweight[caseID] = line->text().toDouble();
                });
        }
    }

    void mOptimizationGoalPanel::updateFrequencyLaout(const QString& text, FoldSection* fold)
    {

        QGridLayout* crossBeamLayout = new QGridLayout();
        crossBeamLayout->addWidget(_tableFrequency, 0, 0, 8, 7);
        crossBeamLayout->addWidget(_addBtn, 0, 7);
        crossBeamLayout->addWidget(_reduceBtn, 1, 7);
        crossBeamLayout->addWidget(_clearBtn, 2, 7);

        fold->addRow(crossBeamLayout);
    }

    void mOptimizationGoalPanel::updateFrequencyTable(bool deleteMap)
    {
        if (deleteMap)
        {
            _optimizationGoalData->_order.clear();
        }

        disconnectFrequencyTableSig();
        _tableFrequency->setRowCount(_optimizationGoalData->_order.size());
        for (int i = 0; i < _optimizationGoalData->_order.size(); i++)
        {
            QTableWidgetItem* order = new QTableWidgetItem(MxFunctions::mNumberToString(_optimizationGoalData->_order[i].first));
            order->setTextAlignment(Qt::AlignCenter);
            QTableWidgetItem* weight = new QTableWidgetItem(MxFunctions::mNumberToString(_optimizationGoalData->_order[i].second));
            weight->setTextAlignment(Qt::AlignCenter);

            _tableFrequency->setItem(i, 0, order);
            _tableFrequency->setItem(i, 1, weight);
        }
        connectFrequencyTableSig();
    }

    void mOptimizationGoalPanel::initFrequencyTable()
    {
        _tablePara = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout;
        _addBtn = new QPushButton(); _addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); _addBtn->setToolTip("增加");
        connect(_addBtn, &QPushButton::clicked, this, &mOptimizationGoalPanel::slot_addLine);
        _reduceBtn = new QPushButton(); _reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); _reduceBtn->setToolTip("减少");
        connect(_reduceBtn, &QPushButton::clicked, this, &mOptimizationGoalPanel::slot_deleteLine);
        _clearBtn = new QPushButton(); _clearBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); _clearBtn->setToolTip("清空");
        connect(_clearBtn, &QPushButton::clicked, this, &mOptimizationGoalPanel::slot_clearTable);
        layout->addWidget(_addBtn);
        layout->addWidget(_reduceBtn);
        layout->addWidget(_clearBtn);
        layout->addStretch();
        layout->setContentsMargins(0, 0, 0, 0);
        _tablePara->setLayout(layout);

        _tableFrequency = new QTableWidget;
        _tableFrequency->setColumnCount(2);
        _tableFrequency->verticalHeader()->setHidden(true);
        _tableFrequency->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("模态阶数") << QString("权重");
        _tableFrequency->setHorizontalHeaderLabels(headerText);
        _tableFrequency->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        _tableFrequency->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        _tableFrequency->setSelectionMode(QAbstractItemView::ExtendedSelection);
        _tableFrequency->setSelectionBehavior(QAbstractItemView::SelectItems);
        _tableFrequency->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        _tableFrequency->setMinimumHeight(270);

        //设置交替换行
        _tableFrequency->setAlternatingRowColors(true);

        m_tableChangeConn = connect(_tableFrequency, &QTableWidget::itemChanged, this, &mOptimizationGoalPanel::slot_lineChanged);

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

    void mOptimizationGoalPanel::connectFrequencyTableSig()
    {
        disconnectFrequencyTableSig();
        m_tableChangeConn = connect(_tableFrequency, &QTableWidget::itemChanged, this, &mOptimizationGoalPanel::slot_lineChanged);
    }

    void mOptimizationGoalPanel::disconnectFrequencyTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
    }

    void mOptimizationGoalPanel::slot_objectiveTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_objectiveType = text;
    }

    void mOptimizationGoalPanel::slot_responseTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        QString oldType = _optimizationGoalData->_responseType;
        _optimizationGoalData->_responseType = text;
       
        auto fold = m_foldWidgets[1];
        QLayoutItem* item;
        while ((item = fold->layout()->takeAt(0)) != nullptr)
        {
            if (item->widget())
            {
                item->widget()->setParent(nullptr);  // 移除旧 widget
            }
            delete item;
        }

        _comboCase->setEnabled(true);
        //切换页面
        if (text == "柔度")
        {
            fold->setTitle("柔度响应");
            updateCaseCombobox(_comboCase);
            fold->addRow(_labelCase, _comboCase);
        }
        else if (text == "位移")
        {
            fold->setTitle("位移响应");
            updateCaseCombobox(_comboCase);
            fold->addRow(_labelCase, _comboCase);
            _pickNode->setState(_optimizationGoalData->_node);
            fold->addRow(_labelNode, _pickNode->propwid);
            _comboDof->blockSignals(true);
            _comboDof->setCurrentText(_optimizationGoalData->_nodeDofs);
            _comboDof->blockSignals(false);
            fold->addRow(_labelDof, _comboDof);
        }
        else if (text == "频率")
        {
            fold->setTitle("频率响应");
            updateCaseCombobox(_comboCase, { ImplicitModal });
            fold->addRow(_labelCase, _comboCase);
            const QSignalBlocker blockerMinCheck(_orderEdit);
            _orderEdit->setExpression(_optimizationGoalData->_frequencyOrder);
            fold->addRow(new PanelLabel("目标阶数"), _orderEdit);
        }
        else if (text == "应力")
        {
            fold->setTitle("应力响应");
            updateCaseCombobox(_comboCase);
            fold->addRow(_labelCase, _comboCase);
        }
        else if (text == "综合柔度")
        {
            fold->setTitle("综合柔度响应");
            updateComplianceTable(oldType != text);
            _tableCompliance->setEnabled(true);
            fold->addRow(_tableCompliance);
        }
        else if (text == "综合频率")
        {
            fold->setTitle("综合频率响应");
            updateCaseCombobox(_comboCase,{ ImplicitModal });
            fold->addRow(_labelCase, _comboCase);
            updateFrequencyTable(oldType != text);
            //updateFrequencyLaout(text, fold);
            _tableFrequency->setEnabled(true);
            _addBtn->setEnabled(true);
            _reduceBtn->setEnabled(true);
            _clearBtn->setEnabled(true);
            fold->addRow(_tablePara);
            fold->addRow(_tableFrequency);
        }
        else if (text == "柔度指数")
        {
            fold->setTitle("柔度指数响应");
            _complianceCheck->blockSignals(true);
            _complianceCheck->setChecked(_optimizationGoalData->_isCompliance);
            _complianceCheck->blockSignals(false);
            slot_complianceCheckBox(_optimizationGoalData->_isCompliance);
            fold->addRow(_complianceCheck);
            updateComplianceTable(oldType != text);
            fold->addRow(_tableCompliance);

            updateCaseCombobox(_comboCase, { ImplicitModal });
            _frequencyCheck->blockSignals(true);
            _frequencyCheck->setChecked(_optimizationGoalData->_isFrequency);
            _frequencyCheck->blockSignals(false);
            slot_frequencyCheckBox(_optimizationGoalData->_isFrequency);
            fold->addRow(_frequencyCheck, _comboCase);
            updateFrequencyTable(oldType != text);
            //updateFrequencyLaout(text, fold);
            fold->addRow(_tablePara);
            fold->addRow(_tableFrequency);
        }  
        else if (text == "热柔度")
        {
            fold->setTitle("热柔度响应");
            updateCaseCombobox(_comboCase, { ImplicitSteadyHeat });
            fold->addRow(_labelCase, _comboCase);
        }
        else if (text == "节点温度")
        {
            fold->setTitle("节点温度响应");
            updateCaseCombobox(_comboCase, { ImplicitSteadyHeat });
            fold->addRow(_labelCase, _comboCase);
            _pickNode->setState(_optimizationGoalData->_node);
            fold->addRow(_labelNode, _pickNode->propwid);
        }
        else if (text == "PSD位移")
        {
            fold->setTitle("PSD位移响应");
            updateCaseCombobox(_comboCase, { ImplicitRandomVibration });
            fold->addRow(_labelCase, _comboCase);

            _comboFrequencyTargetType->blockSignals(true);
            _comboFrequencyTargetType->setCurrentText(_optimizationGoalData->_frequencyTargetType);
            _comboFrequencyTargetType->blockSignals(false);
            fold->addRow(_labelFrequency, _comboFrequencyTargetType);

            const QSignalBlocker blockerFreq(_editSpecifiedFrequency);
            _editSpecifiedFrequency->setExpression(_optimizationGoalData->_specifiedFrequency);
            bool showSpecified = _optimizationGoalData->_frequencyTargetType == "指定频率点";
            if (showSpecified)
            {
                // 需要显示时重新加入布局
                fold->addRow(_labelSpecifiedFrequency, _editSpecifiedFrequency);
            }
            else
            {
                // 隐藏时设置空的父对象
                _labelSpecifiedFrequency->setParent(nullptr);
                _editSpecifiedFrequency->setParent(nullptr);
            }

            _pickNode->setState(_optimizationGoalData->_node);
            fold->addRow(_labelNode, _pickNode->propwid);

            _comboDofDynamic->blockSignals(true);
            _comboDofDynamic->setCurrentText(_optimizationGoalData->_nodeDofs);
            _comboDofDynamic->blockSignals(false);
            fold->addRow(_labelDof, _comboDofDynamic);
        }
        else if (text == "频响位移")
        {
            fold->setTitle("频响位移响应");
            updateCaseCombobox(_comboCase, { ImplicitFrequency });
            fold->addRow(_labelCase, _comboCase);

            _comboFrequencyTargetType->blockSignals(true);
            _comboFrequencyTargetType->setCurrentText(_optimizationGoalData->_frequencyTargetType);
            _comboFrequencyTargetType->blockSignals(false);
            fold->addRow(_labelFrequency, _comboFrequencyTargetType);

            const QSignalBlocker blockerFreq(_editSpecifiedFrequency);
            _editSpecifiedFrequency->setExpression(_optimizationGoalData->_specifiedFrequency);
            bool showSpecified = _optimizationGoalData->_frequencyTargetType == "指定频率点";
            if (showSpecified)
            {
                // 需要显示时重新加入布局
                fold->addRow(_labelSpecifiedFrequency, _editSpecifiedFrequency);
            }
            else
            {
                // 隐藏时设置空的父对象
                _labelSpecifiedFrequency->setParent(nullptr);
                _editSpecifiedFrequency->setParent(nullptr);
            }

            _pickNode->setState(_optimizationGoalData->_node);
            fold->addRow(_labelNode, _pickNode->propwid);

            _comboDofDynamic->blockSignals(true);
            _comboDofDynamic->setCurrentText(_optimizationGoalData->_nodeDofs);
            _comboDofDynamic->blockSignals(false);
            fold->addRow(_labelDof, _comboDofDynamic);

            _comboFrfValueType->blockSignals(true);
            _comboFrfValueType->setCurrentText(_optimizationGoalData->_frfValueType);
            _comboFrfValueType->blockSignals(false);
            fold->addRow(_labelFrfValueType, _comboFrfValueType);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, getCVM->getModelData()->getModelDataID());

        //TODO：其他类型
        QTimer::singleShot(0, fold, [=](){
            fold->refreshLayout();
        });
    }





    void mOptimizationGoalPanel::slot_caseChanged(const QString& text)
    {
        //判断发送信号源
        QComboBox* combo = dynamic_cast<QComboBox*>(QObject::sender());
        if (combo == nullptr)
        {
            return;
        }
        QString type = _optimizationGoalData->_responseType;
        int id = getCVM->getModelData()->getModelDataID();
        //判断当前类型
        if (type == "柔度" || type == "位移" || type == "频率" || type == "应力" || type == "综合柔度" || type == "柔度指数" || type == "热柔度" || type == "节点温度" || type == "PSD位移" || type == "频响位移")
        {
            //工况一对一映射关系改变
            auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            int caseId = caseManager.getIDByCaseName(text);        
            if (caseId == -1)
            {
                combo->setCurrentIndex(0);
                mapRelation.appendMapRelation({ Physical_OptimationGoal, id }, { Physical_Case, caseId });
                return;
            }
            combo->setCurrentText(text);
            //设置id
            mapRelation.appendMapRelation({ Physical_OptimationGoal, id }, { Physical_Case, caseId });
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, id);
    }
    void mOptimizationGoalPanel::slot_dofChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_nodeDofs = text;
    }
    void mOptimizationGoalPanel::slot_getNode()
    {
        if (!_optimizationGoalData)
        {
            return;
        }

        int objNum = 0;
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> pickedData;
        SaveSelectMemoryData(getCVM->getPreRenderWidget(), getCVM->getCurrentPickDataType(objNum), pickedData);
        if (pickedData.empty())
            return;

        if (pickedData[0].first == PickDataType::GeoPoint || pickedData[0].first == PickDataType::Node)
        {
            _optimizationGoalData->_node = { pickedData[0].first , pickedData[0].second };
        }
        _pickNode->setState(_optimizationGoalData->_node);
    }

    void mOptimizationGoalPanel::slot_clearNode()
    {
        if (!_optimizationGoalData)
        {
            return;
        }

        _optimizationGoalData->_node = { MxFunctions::PickDataType::None , {} };
        _pickNode->setState(_optimizationGoalData->_node);
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mOptimizationGoalPanel::slot_checkNode(int state)
    {
        if (!_optimizationGoalData)
        {
            return;
        }

        _pickNode->setCheckedState(state == Qt::Checked);
        SelectionOperate highlight = state == Qt::Checked ? SelectionOperate::ADD : SelectionOperate::REDUCE;
        if (_optimizationGoalData->_node.first == MxFunctions::PickDataType::GeoPoint)
        {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoPoint, { _optimizationGoalData->_node.second.result_id });
        }
        else if (_optimizationGoalData->_node.first == MxFunctions::PickDataType::Node)
        {
            mxr::MeshSelectInfoData n;
            n.data_id.element_id = _optimizationGoalData->_node.second.result_id;
            std::vector<mxr::MeshSelectInfoData> meshIds { n };
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(highlight, SelectionInfoType::MeshNode, meshIds);
        }
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }
    void mOptimizationGoalPanel::slot_lineChanged(QTableWidgetItem* item)
    {
        if (item->column() == 0)
        {
            _optimizationGoalData->_order[item->row()].first = item->text().toDouble();
        }
        else
        {
            _optimizationGoalData->_order[item->row()].second = item->text().toDouble();
        }
    }
    void mOptimizationGoalPanel::slot_addLine()
    {
        disconnectFrequencyTableSig();
        int row = _tableFrequency->currentRow() < 0 ? _tableFrequency->rowCount() : _tableFrequency->currentRow() + 1;
        _tableFrequency->insertRow(row);
        QTableWidgetItem* order = new QTableWidgetItem();
        order->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem* weight = new QTableWidgetItem();
        weight->setTextAlignment(Qt::AlignCenter);

        _tableFrequency->setItem(row, 0, order);
        _tableFrequency->setItem(row, 1, weight);

        _optimizationGoalData->_order.insert(row, { 0,0 });
        connectFrequencyTableSig();
    }
    void mOptimizationGoalPanel::slot_deleteLine()
    {
        disconnectFrequencyTableSig();
        QList<int> rows;
        for (const auto& idx : _tableFrequency->selectionModel()->selectedIndexes()) rows << idx.row();
        rows = rows.toSet().values();
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        if (rows.isEmpty())
            rows.append(_tableFrequency->rowCount() - 1);

        for (auto row : rows)
        {
            if (row >= 0 && row < _tableFrequency->rowCount())
            {
                _optimizationGoalData->_order.removeAt(row);
                _tableFrequency->removeRow(row);
            }
        }
        connectFrequencyTableSig();
    }
    void mOptimizationGoalPanel::slot_clearTable()
    {
        disconnectFrequencyTableSig();
        _tableFrequency->setRowCount(0);
        _optimizationGoalData->_order.clear();
        connectFrequencyTableSig();
    }
    void mOptimizationGoalPanel::slot_complianceCheckBox(bool isCheck)
    {
        _optimizationGoalData->_isCompliance = isCheck;
        _tableCompliance->setEnabled(_optimizationGoalData->_isCompliance);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, getCVM->getModelData()->getModelDataID());
    }
    void mOptimizationGoalPanel::slot_frequencyCheckBox(bool isCheck)
    {
        _optimizationGoalData->_isFrequency = isCheck;
        _comboCase->setEnabled(_optimizationGoalData->_isFrequency);
        _tableFrequency->setEnabled(_optimizationGoalData->_isFrequency);
        _addBtn->setEnabled(_optimizationGoalData->_isFrequency);
        _reduceBtn->setEnabled(_optimizationGoalData->_isFrequency);
        _clearBtn->setEnabled(_optimizationGoalData->_isFrequency);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, getCVM->getModelData()->getModelDataID());
    }

    void mOptimizationGoalPanel::slot_frequencyTargetTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_frequencyTargetType = text;
        // 仅对“指定频率点”行做插入/摘除，避免整体重建布局导致所有行闪烁
        if (_optimizationGoalData->_responseType == "PSD位移" || _optimizationGoalData->_responseType == "频响位移")
        {
            auto fold = m_foldWidgets[1];
            if (text == "指定频率点")
            {
                // 需要显示时插入到“目标频率”行正下方，切换页签后位置固定
                QFormLayout* formLayout = fold->layout();
                int freqRow = -1;
                QFormLayout::ItemRole role = QFormLayout::LabelRole;
                if (formLayout && _labelFrequency)
                {
                    formLayout->getWidgetPosition(_labelFrequency, &freqRow, &role);
                }
                int insertRow = (freqRow >= 0) ? (freqRow + 1) : formLayout->rowCount();
                fold->insertRow(insertRow, _labelSpecifiedFrequency, _editSpecifiedFrequency);
                _labelSpecifiedFrequency->show();
                _editSpecifiedFrequency->show();
            }
            else
            {
                // 隐藏时设置空的父对象
                _labelSpecifiedFrequency->setParent(nullptr);
                _editSpecifiedFrequency->setParent(nullptr);
            }
            QTimer::singleShot(0, fold, [=]() {
                fold->refreshLayout();
            });
        }
    }

    void mOptimizationGoalPanel::slot_specifiedFrequencyChanged()
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_specifiedFrequency = _editSpecifiedFrequency->text();
    }

    void mOptimizationGoalPanel::slot_frfValueTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_frfValueType = text;
    }

    void OptGoalPosPickWidget::setState(std::pair<MxFunctions::PickDataType, RenderPickResultData> node)
    {
        bool picked = false;
        if (node.first != MxFunctions::PickDataType::None)
        {
            picked = true;
        }

        if (picked)
        {
            labelpicked->setText(QString("已选择%1: %2").arg(PickDataEnum2Str(node.first)).arg(node.second.result_id));
        }
        else
        {
            labelpicked->setText("当前未选择");
        }
        labelpicked->setDisabled(!picked);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationGoal, getCVM->getModelData()->getModelDataID());
    }
}