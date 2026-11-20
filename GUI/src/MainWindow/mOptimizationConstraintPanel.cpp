
#include "mOptimizationConstraintPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mStructOption.h"
#include "mOptimizationConstraintData.h"
#include "mPropertyManager.h"
#include "mCalcLineEdit.h"
#include "mCaseBase.h"
#include "mCaseManager.h"
#include "mOptimizationGoalPanel.h"

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechPropertyData;
using namespace MechCaseData;
using namespace MechStructOptionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    namespace
    {
        //判断约束类型是否为从优化目标同步过来的响应类型
        bool isResponseConstraintType(const QString& text)
        {
            return text == "柔度" || text == "位移" || text == "频率" || text == "应力"
                || text == "综合柔度" || text == "综合频率" || text == "柔度指数"
                || text == "热柔度" || text == "节点温度" || text == "PSD位移" || text == "频响位移";
        }
    }

    mOptimizationConstraintPanel::mOptimizationConstraintPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _optimizationConstraintData = nullptr;

        _mainLayout = new QVBoxLayout(this);

        //优化约束组
        FoldSection* constraintFold = new FoldSection("优化约束", this);
        // 1. 约束上限
        m_checkUpperBound = new QCheckBox("约束上限", this);
        m_lineUpperBound = new mCalcLineEdit(this);
        m_lineUpperBound->setEnabled(false);
        constraintFold->addRow(m_checkUpperBound, m_lineUpperBound);

        // 2. 约束下限 (修正：原代码误加到了 paramFold)
        m_checkLowerBound = new QCheckBox("约束下限", this);
        m_lineLowerBound = new mCalcLineEdit(this);
        m_lineLowerBound->setEnabled(false);
        constraintFold->addRow(m_checkLowerBound, m_lineLowerBound);

        m_comboConstraintType = new QComboBox(); m_comboConstraintType->addItems({ "质量", "质量分数", "体积", "体积分数",
            "柔度", "位移", "频率", "应力", "综合柔度", "综合频率", "柔度指数", "热柔度", "节点温度", "PSD位移", "频响位移" });
        constraintFold->addRow(new PanelLabel("约束类型", this), m_comboConstraintType);
        connect(m_comboConstraintType, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_constraintTypeChanged);
        // --- 约束上限联动 ---
        connect(m_checkUpperBound, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationConstraintData)
            {
                _optimizationConstraintData->_upperBoundStatus = (state == Qt::Checked);
                m_lineUpperBound->setEnabled(_optimizationConstraintData->_upperBoundStatus);
            }
            });
        connect(m_lineUpperBound, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationConstraintData)
                _optimizationConstraintData->_upperBoundValue = m_lineUpperBound->text();
            });

        // --- 约束下限联动 ---
        connect(m_checkLowerBound, &QCheckBox::stateChanged, this, [this](int state) {
            if (_optimizationConstraintData)
            {
                _optimizationConstraintData->_lowerBoundStatus = (state == Qt::Checked);
                m_lineLowerBound->setEnabled(_optimizationConstraintData->_lowerBoundStatus);
            }
            });
        connect(m_lineLowerBound, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationConstraintData)
                _optimizationConstraintData->_lowerBoundValue = m_lineLowerBound->text();
            });

       
        m_foldWidgets.append(constraintFold);
        _mainLayout->addWidget(constraintFold);

        //结构质量属性组
        FoldSection* designFold = new FoldSection("结构质量", this);
        //m_checkProperty = new QCheckBox("指定区域");
        m_lableProperty = new PanelLabel("指定区域", this);
        m_comboProperty = new QComboBox(); m_comboProperty->setEnabled(true);
        designFold->addRow(m_lableProperty, m_comboProperty);
        //connect(m_checkProperty, &QCheckBox::stateChanged, this, &mOptimizationConstraintPanel::slot_checkPropertyChanged);
        connect(m_comboProperty, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_propertyChanged);

        m_foldWidgets.append(designFold);
        _mainLayout->addWidget(designFold);

        //****************响应类型约束控件(与优化目标面板保持一致)****************
        //工况
        m_comboCase = new QComboBox();
        connect(m_comboCase, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_caseChanged);

        //节点
        m_pickNode = new OptGoalPosPickWidget;
        m_pickNode->initWidget("节点");
        connect(m_pickNode->btnok, &QPushButton::clicked, this, &mOptimizationConstraintPanel::slot_getNode);
        connect(m_pickNode->btnclear, &QPushButton::clicked, this, &mOptimizationConstraintPanel::slot_clearNode);
        connect(m_pickNode->labelpicked, &QCheckBox::stateChanged, this, &mOptimizationConstraintPanel::slot_checkNode);

        //自由度
        m_comboDof = new QComboBox();
        m_comboDof->addItems({ "1", "2", "3", "4", "5", "6", "disp", "rot" });
        connect(m_comboDof, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_dofChanged);

        //频率-目标阶数
        m_orderEdit = new mCalcLineEdit("", nullptr, false, true, true);
        connect(m_orderEdit, &QLineEdit::editingFinished, this, [this]() {
            if (_optimizationConstraintData)
                _optimizationConstraintData->_frequencyOrder = m_orderEdit->text();
            });

        //PSD位移/频响位移公共控件
        m_comboFrequencyTargetType = new QComboBox();
        m_comboFrequencyTargetType->addItems({ "指定频率点", "最大响应点", "最小响应点", "平均", "求和", "RMS" });
        connect(m_comboFrequencyTargetType, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_frequencyTargetTypeChanged);

        m_editSpecifiedFrequency = new mCalcLineEdit("", nullptr, false, true, true);
        connect(m_editSpecifiedFrequency, &QLineEdit::editingFinished, this, &mOptimizationConstraintPanel::slot_specifiedFrequencyChanged);

        m_comboDofDynamic = new QComboBox();
        m_comboDofDynamic->addItems({ "1", "2", "3", "4", "5", "6" });
        connect(m_comboDofDynamic, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_dofChanged);

        //频响位移优化类型
        m_comboFrfValueType = new QComboBox();
        m_comboFrfValueType->addItems({ "幅值", "实部", "虚部" });
        connect(m_comboFrfValueType, &QComboBox::currentTextChanged, this, &mOptimizationConstraintPanel::slot_frfValueTypeChanged);

        //复用标签
        m_labelCase = new PanelLabel("目标工况");
        m_labelFrequency = new PanelLabel("目标频率");
        m_labelNode = new PanelLabel("节点");
        m_labelDof = new PanelLabel("自由度");
        m_labelSpecifiedFrequency = new PanelLabel("指定频率点");
        m_labelFrfValueType = new PanelLabel("优化类型");

        //综合柔度
        m_tableCompliance = new QTableWidget;
        m_tableCompliance->setColumnCount(2);
        m_tableCompliance->verticalHeader()->setHidden(true);
        m_tableCompliance->resizeColumnsToContents();
        m_tableCompliance->setSelectionBehavior(QAbstractItemView::SelectRows);
        QStringList complianceHeaderText;
        complianceHeaderText << QString("工况名") << QString("权重");
        m_tableCompliance->setHorizontalHeaderLabels(complianceHeaderText);
        m_tableCompliance->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_tableCompliance->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableCompliance->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableCompliance->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_tableCompliance->setMinimumHeight(270);

        //综合频率
        initFrequencyTable();

        //柔度指数
        m_complianceCheck = new QCheckBox("柔度工况");
        connect(m_complianceCheck, &QCheckBox::clicked, this, &mOptimizationConstraintPanel::slot_complianceCheckBox);
        m_frequencyCheck = new QCheckBox("模态工况");
        connect(m_frequencyCheck, &QCheckBox::clicked, this, &mOptimizationConstraintPanel::slot_frequencyCheckBox);

        _mainLayout->addStretch();

    }

    mOptimizationConstraintPanel::~mOptimizationConstraintPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
        if (m_pickNode != nullptr)
        {
            m_pickNode->clear();
        }
    }

    void mOptimizationConstraintPanel::createOptimizationConstraint(const QString& type)
    {
        if (!getCVM)
        {
            return;
        }
        auto structOption = getCVM->getModelData()->getStructOptionBase();
        if (structOption == nullptr)
        {
            structOption = getCVM->getModelData()->createStructOption();
            emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationSig(true);
        }
        int ID = structOption->createOptimizationConstraintID();
        auto data = structOption->getOptimizationConstraintData(ID);
        int cnt = 1;
        QString text = "OptiConstraint";
        QString  name = text + "_" + MxFunctions::mNumberToString(cnt++);
        while (structOption->existOptimizationConstraintName(name))
        {
            name = text + "_" + MxFunctions::mNumberToString(cnt++);
        }
        _optimizationConstraintData = new mOptimizationConstraintData(ID, name);
        _optimizationConstraintData->_constraintType = type;
        structOption->addOptimizationConstraintData(_optimizationConstraintData);

        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建优化约束 %1").arg(_optimizationConstraintData->getName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationConstraintSig(true, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::editOptimizationConstraintPanel(int id)
    {
        if (!getCVM)
        {
            return;
        }
        _optimizationConstraintData = getCVM->getModelData()->getStructOptionBase()->getOptimizationConstraintData(id);
        if (_optimizationConstraintData == nullptr)
        {
            return;
        }
        setEditing(true);

        // 屏蔽信号以防回显时触发存值逻辑
        const QSignalBlocker blockerUpperCheck(m_checkUpperBound);
        const QSignalBlocker blockerUpperLine(m_lineUpperBound);
        const QSignalBlocker blockerLowerCheck(m_checkLowerBound);
        const QSignalBlocker blockerLowerLine(m_lineLowerBound);
        const QSignalBlocker blockerCombo(m_comboConstraintType);
        //const QSignalBlocker blockerCheck(m_checkProperty);

        // 1. 回显上限状态
        m_checkUpperBound->setChecked(_optimizationConstraintData->_upperBoundStatus);
        m_lineUpperBound->setExpression(_optimizationConstraintData->_upperBoundValue);
        m_lineUpperBound->setEnabled(_optimizationConstraintData->_upperBoundStatus);

        // 2. 回显下限状态
        m_checkLowerBound->setChecked(_optimizationConstraintData->_lowerBoundStatus);
        m_lineLowerBound->setExpression(_optimizationConstraintData->_lowerBoundValue);
        m_lineLowerBound->setEnabled(_optimizationConstraintData->_lowerBoundStatus);

        // 3. 回显约束类型
        m_comboConstraintType->setCurrentText(_optimizationConstraintData->_constraintType);

        //m_checkProperty->setChecked(_optimizationConstraintData->_isArea);

        slot_constraintTypeChanged(_optimizationConstraintData->_constraintType);
    }

    void mOptimizationConstraintPanel::uneditOptimizationConstraintPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _optimizationConstraintData = nullptr;
    }

    void mOptimizationConstraintPanel::updatePropertyCombobox(QComboBox* combobox)
    {
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int propertyId = mapRelation.getMapRelation<int>({ Physical_OptimationConstraint, _optimizationConstraintData->getId() }, Physical_Property);
        QString currentText = propertyManager.getPropertyNameByID(propertyId);
        combobox->blockSignals(true);
        combobox->clear();
        combobox->addItem("所有设计域");
        combobox->addItems(propertyManager.getAllPropertyNames());

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else if (combobox->model()->rowCount() > 0) {
            combobox->setCurrentIndex(0); // 默认不选
        }
        combobox->blockSignals(false);
    }

    void mOptimizationConstraintPanel::slot_constraintTypeChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        QString oldType = _optimizationConstraintData->_constraintType;
        _optimizationConstraintData->_constraintType = text;
        int id = _optimizationConstraintData->getId();

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

        //质量/质量分数/体积/体积分数：保持原有设计域(指定区域)逻辑不变
        if (!isResponseConstraintType(text))
        {
            fold->setTitle(QString("结构%1").arg(text));
            updatePropertyCombobox(m_comboProperty);
            fold->addRow(m_lableProperty, m_comboProperty);

            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, id);
            QTimer::singleShot(0, fold, [=]() {
                fold->refreshLayout();
            });
            return;
        }

        //响应类型：切换逻辑与优化目标面板保持一致
        m_comboCase->setEnabled(true);
        if (text == "柔度")
        {
            fold->setTitle("柔度响应");
            updateCaseCombobox(m_comboCase);
            fold->addRow(m_labelCase, m_comboCase);
        }
        else if (text == "位移")
        {
            fold->setTitle("位移响应");
            updateCaseCombobox(m_comboCase);
            fold->addRow(m_labelCase, m_comboCase);
            updatePickNodeState();
            fold->addRow(m_labelNode, m_pickNode->propwid);
            m_comboDof->blockSignals(true);
            m_comboDof->setCurrentText(_optimizationConstraintData->_nodeDofs);
            m_comboDof->blockSignals(false);
            fold->addRow(m_labelDof, m_comboDof);
        }
        else if (text == "频率")
        {
            fold->setTitle("频率响应");
            updateCaseCombobox(m_comboCase, { ImplicitModal });
            fold->addRow(m_labelCase, m_comboCase);
            const QSignalBlocker blockerOrderEdit(m_orderEdit);
            m_orderEdit->setExpression(_optimizationConstraintData->_frequencyOrder);
            fold->addRow(new PanelLabel("目标阶数"), m_orderEdit);
        }
        else if (text == "应力")
        {
            fold->setTitle("应力响应");
            updateCaseCombobox(m_comboCase);
            fold->addRow(m_labelCase, m_comboCase);
        }
        else if (text == "综合柔度")
        {
            fold->setTitle("综合柔度响应");
            updateComplianceTable(oldType != text);
            m_tableCompliance->setEnabled(true);
            fold->addRow(m_tableCompliance);
        }
        else if (text == "综合频率")
        {
            fold->setTitle("综合频率响应");
            updateCaseCombobox(m_comboCase, { ImplicitModal });
            fold->addRow(m_labelCase, m_comboCase);
            updateFrequencyTable(oldType != text);
            m_tableFrequency->setEnabled(true);
            m_addBtn->setEnabled(true);
            m_reduceBtn->setEnabled(true);
            m_clearBtn->setEnabled(true);
            fold->addRow(m_tablePara);
            fold->addRow(m_tableFrequency);
        }
        else if (text == "柔度指数")
        {
            fold->setTitle("柔度指数响应");
            m_complianceCheck->blockSignals(true);
            m_complianceCheck->setChecked(_optimizationConstraintData->_isCompliance);
            m_complianceCheck->blockSignals(false);
            slot_complianceCheckBox(_optimizationConstraintData->_isCompliance);
            fold->addRow(m_complianceCheck);
            updateComplianceTable(oldType != text);
            fold->addRow(m_tableCompliance);

            updateCaseCombobox(m_comboCase, { ImplicitModal });
            m_frequencyCheck->blockSignals(true);
            m_frequencyCheck->setChecked(_optimizationConstraintData->_isFrequency);
            m_frequencyCheck->blockSignals(false);
            slot_frequencyCheckBox(_optimizationConstraintData->_isFrequency);
            fold->addRow(m_frequencyCheck, m_comboCase);
            updateFrequencyTable(oldType != text);
            fold->addRow(m_tablePara);
            fold->addRow(m_tableFrequency);
        }
        else if (text == "热柔度")
        {
            fold->setTitle("热柔度响应");
            updateCaseCombobox(m_comboCase, { ImplicitSteadyHeat });
            fold->addRow(m_labelCase, m_comboCase);
        }
        else if (text == "节点温度")
        {
            fold->setTitle("节点温度响应");
            updateCaseCombobox(m_comboCase, { ImplicitSteadyHeat });
            fold->addRow(m_labelCase, m_comboCase);
            updatePickNodeState();
            fold->addRow(m_labelNode, m_pickNode->propwid);
        }
        else if (text == "PSD位移")
        {
            fold->setTitle("PSD位移响应");
            updateCaseCombobox(m_comboCase, { ImplicitRandomVibration });
            fold->addRow(m_labelCase, m_comboCase);

            m_comboFrequencyTargetType->blockSignals(true);
            m_comboFrequencyTargetType->setCurrentText(_optimizationConstraintData->_frequencyTargetType);
            m_comboFrequencyTargetType->blockSignals(false);
            fold->addRow(m_labelFrequency, m_comboFrequencyTargetType);

            const QSignalBlocker blockerFreq(m_editSpecifiedFrequency);
            m_editSpecifiedFrequency->setExpression(_optimizationConstraintData->_specifiedFrequency);
            bool showSpecified = _optimizationConstraintData->_frequencyTargetType == "指定频率点";
            if (showSpecified)
            {
                fold->addRow(m_labelSpecifiedFrequency, m_editSpecifiedFrequency);
            }
            else
            {
                m_labelSpecifiedFrequency->setParent(nullptr);
                m_editSpecifiedFrequency->setParent(nullptr);
            }

            updatePickNodeState();
            fold->addRow(m_labelNode, m_pickNode->propwid);

            m_comboDofDynamic->blockSignals(true);
            m_comboDofDynamic->setCurrentText(_optimizationConstraintData->_nodeDofs);
            m_comboDofDynamic->blockSignals(false);
            fold->addRow(m_labelDof, m_comboDofDynamic);
        }
        else if (text == "频响位移")
        {
            fold->setTitle("频响位移响应");
            updateCaseCombobox(m_comboCase, { ImplicitFrequency });
            fold->addRow(m_labelCase, m_comboCase);

            m_comboFrequencyTargetType->blockSignals(true);
            m_comboFrequencyTargetType->setCurrentText(_optimizationConstraintData->_frequencyTargetType);
            m_comboFrequencyTargetType->blockSignals(false);
            fold->addRow(m_labelFrequency, m_comboFrequencyTargetType);

            const QSignalBlocker blockerFreq(m_editSpecifiedFrequency);
            m_editSpecifiedFrequency->setExpression(_optimizationConstraintData->_specifiedFrequency);
            bool showSpecified = _optimizationConstraintData->_frequencyTargetType == "指定频率点";
            if (showSpecified)
            {
                fold->addRow(m_labelSpecifiedFrequency, m_editSpecifiedFrequency);
            }
            else
            {
                m_labelSpecifiedFrequency->setParent(nullptr);
                m_editSpecifiedFrequency->setParent(nullptr);
            }

            updatePickNodeState();
            fold->addRow(m_labelNode, m_pickNode->propwid);

            m_comboDofDynamic->blockSignals(true);
            m_comboDofDynamic->setCurrentText(_optimizationConstraintData->_nodeDofs);
            m_comboDofDynamic->blockSignals(false);
            fold->addRow(m_labelDof, m_comboDofDynamic);

            m_comboFrfValueType->blockSignals(true);
            m_comboFrfValueType->setCurrentText(_optimizationConstraintData->_frfValueType);
            m_comboFrfValueType->blockSignals(false);
            fold->addRow(m_labelFrfValueType, m_comboFrfValueType);
        }

        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, id);

        QTimer::singleShot(0, fold, [=]() {
            fold->refreshLayout();
        });
    }

    void mOptimizationConstraintPanel::slot_checkPropertyChanged(int state)
    {
        if (_optimizationConstraintData)
            _optimizationConstraintData->_isArea = (state == Qt::Checked);

        m_comboProperty->setEnabled(_optimizationConstraintData->_isArea);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationConstraintSig(false, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::slot_propertyChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        auto& propertyManager = getCVM->getModelData()->getManager<mPropertyManager>(Physical_Property);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int proertyId = propertyManager.getIDByPropertyName(text);
        int id = _optimizationConstraintData->getId();
        mapRelation.appendMapRelation({ Physical_OptimationConstraint, id }, { Physical_Property, proertyId });
        if (proertyId == -1)
        {
            m_comboProperty->setCurrentIndex(0);
        }
        else
        {
            m_comboProperty->setCurrentText(text);
        }

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateOptimizationConstraintSig(false, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::updateCaseCombobox(QComboBox* combobox, QList<CaseType> caseType)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int id = _optimizationConstraintData->getId();
        int caseId = mapRelation.getMapRelation<int>({ Physical_OptimationConstraint, id }, Physical_Case);
        QString currentText = caseManager.getCaseNameByID(caseId);
        combobox->blockSignals(true);
        combobox->clear();
        for (auto caseTypeOne : caseType)
        {
            combobox->addItems(getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(caseTypeOne));
        }

        int idx = combobox->findText(currentText);
        if (idx >= 0) {
            combobox->setCurrentIndex(idx);
        }
        else
        {
            if (combobox->model()->rowCount() > 0) caseId = caseManager.getIDByCaseName(combobox->currentText());
            else caseId = -1;
            mapRelation.appendMapRelation({ Physical_OptimationConstraint, id }, { Physical_Case, caseId });
        }
        combobox->blockSignals(false);
    }

    void mOptimizationConstraintPanel::updateComplianceTable(bool deleteMap, QList<CaseType> caseType)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
        if (deleteMap)
        {
            _optimizationConstraintData->_caseToweight.clear();
        }

        QStringList CaseNames;
        for (auto caseTypeOne : caseType)
        {
            CaseNames += getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getAllCaseNamesByTypes(caseTypeOne);
        }

        m_tableCompliance->setRowCount(CaseNames.size());
        for (int i = 0; i < CaseNames.size(); i++)
        {
            int caseID = caseManager.getIDByCaseName(CaseNames[i]);
            mCalcLineEdit* weight = new mCalcLineEdit("1.0", this, true, false);
            weight->setAlignment(Qt::AlignCenter);
            QCheckBox* caseCheck = new QCheckBox(CaseNames[i]);
            caseCheck->setProperty("row", i);
            weight->setProperty("row", i);
            weight->setEnabled(false);

            if (_optimizationConstraintData->_caseToweight.find(caseID) != _optimizationConstraintData->_caseToweight.cend())
            {
                caseCheck->setChecked(true);
                weight->setEnabled(true);
                weight->setExpression(MxFunctions::mNumberToString(_optimizationConstraintData->_caseToweight[caseID]));
            }

            m_tableCompliance->setCellWidget(i, 0, caseCheck);
            m_tableCompliance->setCellWidget(i, 1, weight);

            connect(caseCheck, &QCheckBox::clicked, this, [this](bool checked) {
                QCheckBox* cb = qobject_cast<QCheckBox*>(sender());
                int caseID = getCVM->getModelData()->getPhyIDByName(Physical_Case, cb->text());
                if (caseID == -1) return;
                int row = cb->property("row").toInt();
                mCalcLineEdit* line = qobject_cast<mCalcLineEdit*>(m_tableCompliance->cellWidget(row, 1));
                line->setEnabled(checked);

                if (checked)
                {
                    _optimizationConstraintData->_caseToweight[caseID] = line->text().toDouble();
                }
                else
                {
                    _optimizationConstraintData->_caseToweight.erase(caseID);
                }
            });

            connect(weight, &QLineEdit::editingFinished, this, [this]() {
                mCalcLineEdit* line = qobject_cast<mCalcLineEdit*>(sender());
                int row = line->property("row").toInt();
                QCheckBox* cb = qobject_cast<QCheckBox*>(m_tableCompliance->cellWidget(row, 0));
                if (!cb->isChecked()) return;

                int caseID = getCVM->getModelData()->getPhyIDByName(Physical_Case, cb->text());
                if (caseID == -1) return;
                _optimizationConstraintData->_caseToweight[caseID] = line->text().toDouble();
                });
        }
    }

    void mOptimizationConstraintPanel::updateFrequencyTable(bool deleteMap)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        if (deleteMap)
        {
            _optimizationConstraintData->_order.clear();
        }

        disconnectFrequencyTableSig();
        m_tableFrequency->setRowCount(_optimizationConstraintData->_order.size());
        for (int i = 0; i < _optimizationConstraintData->_order.size(); i++)
        {
            QTableWidgetItem* order = new QTableWidgetItem(MxFunctions::mNumberToString(_optimizationConstraintData->_order[i].first));
            order->setTextAlignment(Qt::AlignCenter);
            QTableWidgetItem* weight = new QTableWidgetItem(MxFunctions::mNumberToString(_optimizationConstraintData->_order[i].second));
            weight->setTextAlignment(Qt::AlignCenter);

            m_tableFrequency->setItem(i, 0, order);
            m_tableFrequency->setItem(i, 1, weight);
        }
        connectFrequencyTableSig();
    }

    void mOptimizationConstraintPanel::initFrequencyTable()
    {
        m_tablePara = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout;
        m_addBtn = new QPushButton(); m_addBtn->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png")); m_addBtn->setToolTip("增加");
        connect(m_addBtn, &QPushButton::clicked, this, &mOptimizationConstraintPanel::slot_addLine);
        m_reduceBtn = new QPushButton(); m_reduceBtn->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png")); m_reduceBtn->setToolTip("减少");
        connect(m_reduceBtn, &QPushButton::clicked, this, &mOptimizationConstraintPanel::slot_deleteLine);
        m_clearBtn = new QPushButton(); m_clearBtn->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); m_clearBtn->setToolTip("清空");
        connect(m_clearBtn, &QPushButton::clicked, this, &mOptimizationConstraintPanel::slot_clearTable);
        layout->addWidget(m_addBtn);
        layout->addWidget(m_reduceBtn);
        layout->addWidget(m_clearBtn);
        layout->addStretch();
        layout->setContentsMargins(0, 0, 0, 0);
        m_tablePara->setLayout(layout);

        m_tableFrequency = new QTableWidget;
        m_tableFrequency->setColumnCount(2);
        m_tableFrequency->verticalHeader()->setHidden(true);
        m_tableFrequency->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("模态阶数") << QString("权重");
        m_tableFrequency->setHorizontalHeaderLabels(headerText);
        m_tableFrequency->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_tableFrequency->horizontalHeader()->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tableFrequency->setSelectionMode(QAbstractItemView::ExtendedSelection);
        m_tableFrequency->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_tableFrequency->setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed);
        m_tableFrequency->setMinimumHeight(270);

        //设置交替换行
        m_tableFrequency->setAlternatingRowColors(true);

        m_tableChangeConn = connect(m_tableFrequency, &QTableWidget::itemChanged, this, &mOptimizationConstraintPanel::slot_lineChanged);

        // 美化表格样式 - 浅蓝主题(与优化目标面板保持完全一致)
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
            background-color: #F7F8FA;
            border: 1px solid #dcdcdc;
            border-left: 0;

            padding:4px;
            font-weight:normal;
        }
        QHeaderView::section:vertical
        {
            background-color: #F7F8FA;
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

    void mOptimizationConstraintPanel::connectFrequencyTableSig()
    {
        disconnectFrequencyTableSig();
        m_tableChangeConn = connect(m_tableFrequency, &QTableWidget::itemChanged, this, &mOptimizationConstraintPanel::slot_lineChanged);
    }

    void mOptimizationConstraintPanel::disconnectFrequencyTableSig()
    {
        if (m_tableChangeConn)
        {
            disconnect(m_tableChangeConn);
        }
    }

    void mOptimizationConstraintPanel::updatePickNodeState()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        auto node = _optimizationConstraintData->_node;
        bool picked = node.first != MxFunctions::PickDataType::None;

        if (picked)
        {
            m_pickNode->labelpicked->setText(QString("已选择%1: %2").arg(m_pickNode->PickDataEnum2Str(node.first)).arg(node.second.result_id));
        }
        else
        {
            m_pickNode->labelpicked->setText("当前未选择");
        }
        m_pickNode->labelpicked->setDisabled(!picked);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::slot_caseChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        //判断发送信号源
        QComboBox* combo = dynamic_cast<QComboBox*>(QObject::sender());
        if (combo == nullptr)
        {
            return;
        }
        QString type = _optimizationConstraintData->_constraintType;
        int id = _optimizationConstraintData->getId();
        //判断当前类型(与优化目标面板一致的响应类型才有工况映射)
        if (isResponseConstraintType(type))
        {
            //工况一对一映射关系改变
            auto& caseManager = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
            auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
            int caseId = caseManager.getIDByCaseName(text);
            if (caseId == -1)
            {
                combo->setCurrentIndex(0);
                mapRelation.appendMapRelation({ Physical_OptimationConstraint, id }, { Physical_Case, caseId });
                return;
            }
            combo->setCurrentText(text);
            //设置id
            mapRelation.appendMapRelation({ Physical_OptimationConstraint, id }, { Physical_Case, caseId });
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, id);
    }

    void mOptimizationConstraintPanel::slot_dofChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_nodeDofs = text;
    }

    void mOptimizationConstraintPanel::slot_getNode()
    {
        if (!_optimizationConstraintData)
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
            _optimizationConstraintData->_node = { pickedData[0].first , pickedData[0].second };
        }
        updatePickNodeState();
    }

    void mOptimizationConstraintPanel::slot_clearNode()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }

        _optimizationConstraintData->_node = { MxFunctions::PickDataType::None , {} };
        updatePickNodeState();
        ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
        ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
        UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mOptimizationConstraintPanel::slot_checkNode(int state)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }

        m_pickNode->setCheckedState(state == Qt::Checked);
        SelectionOperate highlight = state == Qt::Checked ? SelectionOperate::ADD : SelectionOperate::REDUCE;
        if (_optimizationConstraintData->_node.first == MxFunctions::PickDataType::GeoPoint)
        {
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoPoint, { _optimizationConstraintData->_node.second.result_id });
        }
        else if (_optimizationConstraintData->_node.first == MxFunctions::PickDataType::Node)
        {
            mxr::MeshSelectInfoData n;
            n.data_id.element_id = _optimizationConstraintData->_node.second.result_id;
            std::vector<mxr::MeshSelectInfoData> meshIds { n };
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(highlight, SelectionInfoType::MeshNode, meshIds);
        }
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mOptimizationConstraintPanel::slot_lineChanged(QTableWidgetItem* item)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        if (item->column() == 0)
        {
            _optimizationConstraintData->_order[item->row()].first = item->text().toDouble();
        }
        else
        {
            _optimizationConstraintData->_order[item->row()].second = item->text().toDouble();
        }
    }

    void mOptimizationConstraintPanel::slot_addLine()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        disconnectFrequencyTableSig();
        int row = m_tableFrequency->currentRow() < 0 ? m_tableFrequency->rowCount() : m_tableFrequency->currentRow() + 1;
        m_tableFrequency->insertRow(row);
        QTableWidgetItem* order = new QTableWidgetItem();
        order->setTextAlignment(Qt::AlignCenter);
        QTableWidgetItem* weight = new QTableWidgetItem();
        weight->setTextAlignment(Qt::AlignCenter);

        m_tableFrequency->setItem(row, 0, order);
        m_tableFrequency->setItem(row, 1, weight);

        _optimizationConstraintData->_order.insert(row, { 0,0 });
        connectFrequencyTableSig();
    }

    void mOptimizationConstraintPanel::slot_deleteLine()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        disconnectFrequencyTableSig();
        QList<int> rows;
        for (const auto& idx : m_tableFrequency->selectionModel()->selectedIndexes()) rows << idx.row();
        rows = rows.toSet().values();
        std::sort(rows.begin(), rows.end(), std::greater<int>());

        if (rows.isEmpty())
            rows.append(m_tableFrequency->rowCount() - 1);

        for (auto row : rows)
        {
            if (row >= 0 && row < m_tableFrequency->rowCount())
            {
                _optimizationConstraintData->_order.removeAt(row);
                m_tableFrequency->removeRow(row);
            }
        }
        connectFrequencyTableSig();
    }

    void mOptimizationConstraintPanel::slot_clearTable()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        disconnectFrequencyTableSig();
        m_tableFrequency->setRowCount(0);
        _optimizationConstraintData->_order.clear();
        connectFrequencyTableSig();
    }

    void mOptimizationConstraintPanel::slot_complianceCheckBox(bool isCheck)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_isCompliance = isCheck;
        m_tableCompliance->setEnabled(_optimizationConstraintData->_isCompliance);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::slot_frequencyCheckBox(bool isCheck)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_isFrequency = isCheck;
        m_comboCase->setEnabled(_optimizationConstraintData->_isFrequency);
        m_tableFrequency->setEnabled(_optimizationConstraintData->_isFrequency);
        m_addBtn->setEnabled(_optimizationConstraintData->_isFrequency);
        m_reduceBtn->setEnabled(_optimizationConstraintData->_isFrequency);
        m_clearBtn->setEnabled(_optimizationConstraintData->_isFrequency);
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_OptimationConstraint, _optimizationConstraintData->getId());
    }

    void mOptimizationConstraintPanel::slot_frequencyTargetTypeChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_frequencyTargetType = text;
        // 仅对“指定频率点”行做插入/摘除，避免整体重建布局导致所有行闪烁
        if (_optimizationConstraintData->_constraintType == "PSD位移" || _optimizationConstraintData->_constraintType == "频响位移")
        {
            auto fold = m_foldWidgets[1];
            if (text == "指定频率点")
            {
                // 需要显示时插入到“目标频率”行正下方，切换页签后位置固定
                QFormLayout* formLayout = fold->layout();
                int freqRow = -1;
                QFormLayout::ItemRole role = QFormLayout::LabelRole;
                if (formLayout && m_labelFrequency)
                {
                    formLayout->getWidgetPosition(m_labelFrequency, &freqRow, &role);
                }
                int insertRow = (freqRow >= 0) ? (freqRow + 1) : formLayout->rowCount();
                fold->insertRow(insertRow, m_labelSpecifiedFrequency, m_editSpecifiedFrequency);
                m_labelSpecifiedFrequency->show();
                m_editSpecifiedFrequency->show();
            }
            else
            {
                // 隐藏时设置空的父对象
                m_labelSpecifiedFrequency->setParent(nullptr);
                m_editSpecifiedFrequency->setParent(nullptr);
            }
            QTimer::singleShot(0, fold, [=]() {
                fold->refreshLayout();
            });
        }
    }

    void mOptimizationConstraintPanel::slot_specifiedFrequencyChanged()
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_specifiedFrequency = m_editSpecifiedFrequency->text();
    }

    void mOptimizationConstraintPanel::slot_frfValueTypeChanged(const QString& text)
    {
        if (!_optimizationConstraintData)
        {
            return;
        }
        _optimizationConstraintData->_frfValueType = text;
    }
}