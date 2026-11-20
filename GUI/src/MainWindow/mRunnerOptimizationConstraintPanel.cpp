
#include "mRunnerOptimizationConstraintPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "PickCommonFunction.h"

//BasicData
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationConstraintData.h"
#include "mPropertyManager.h"
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
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MechPropertyData;
using namespace MechStructOptionData;
using namespace MxFunctions;
using namespace mxr;

namespace MechMainWindow
{
    mRunnerOptimizationConstraintPanel::mRunnerOptimizationConstraintPanel(QWidget* parent)
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

        // 2. 约束下限
        m_checkLowerBound = new QCheckBox("约束下限", this);
        m_lineLowerBound = new mCalcLineEdit(this);
        m_lineLowerBound->setEnabled(false);
        constraintFold->addRow(m_checkLowerBound, m_lineLowerBound);

        // 3. 约束类型
        m_comboConstraintType = new QComboBox(); m_comboConstraintType->addItems({ "质量", "质量分数", "体积", "体积分数" });
        constraintFold->addRow(new PanelLabel("约束类型", this), m_comboConstraintType);
        connect(m_comboConstraintType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationConstraintPanel::slot_constraintTypeChanged);

        // 4. 约束方式（流道优化新增：固体/流体）
        m_comboConstraintMode = new QComboBox(); m_comboConstraintMode->addItems({ "固体", "流体" });
        constraintFold->addRow(new PanelLabel("约束方式", this), m_comboConstraintMode);
        connect(m_comboConstraintMode, &QComboBox::currentTextChanged, this, &mRunnerOptimizationConstraintPanel::slot_constraintModeChanged);

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
        m_lableProperty = new PanelLabel("指定区域", this);
        m_comboProperty = new QComboBox(); m_comboProperty->setEnabled(true);
        designFold->addRow(m_lableProperty, m_comboProperty);
        connect(m_comboProperty, &QComboBox::currentTextChanged, this, &mRunnerOptimizationConstraintPanel::slot_propertyChanged);

        m_foldWidgets.append(designFold);
        _mainLayout->addWidget(designFold);

        _mainLayout->addStretch();
    }

    mRunnerOptimizationConstraintPanel::~mRunnerOptimizationConstraintPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mRunnerOptimizationConstraintPanel::createRunnerOptimizationConstraint(const QString& type)
    {
        if (!getCVM)
        {
            return;
        }
        auto runnerOption = getCVM->getModelData()->getRunnerStructOption();
        int ID = runnerOption->createOptimizationConstraintID();
        auto data = runnerOption->getOptimizationConstraintData(ID);
        int cnt = 1;
        QString text = "约束";
        QString  name = text + "_" + MxFunctions::mNumberToString(cnt++);
        while (runnerOption->existOptimizationConstraintName(name))
        {
            name = text + "_" + MxFunctions::mNumberToString(cnt++);
        }
        _optimizationConstraintData = new mRunnerOptimizationConstraintData(ID, name);
        _optimizationConstraintData->_constraintType = type;
        runnerOption->addOptimizationConstraintData(_optimizationConstraintData);

        //创建成功发送消息
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建流道优化约束 %1").arg(_optimizationConstraintData->getName()), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationConstraintSig(true, _optimizationConstraintData->getId());
    }

    void mRunnerOptimizationConstraintPanel::editRunnerOptimizationConstraintPanel(int id)
    {
        if (!getCVM)
        {
            return;
        }
        _optimizationConstraintData = getCVM->getModelData()->getRunnerStructOption()->getOptimizationConstraintData(id);
        if (_optimizationConstraintData == nullptr)
        {
            return;
        }
        setEditing(true);
        //更新截面属性
        updatePropertyCombobox(m_comboProperty);

        // 屏蔽信号以防回显时触发存值逻辑
        const QSignalBlocker blockerUpperCheck(m_checkUpperBound);
        const QSignalBlocker blockerUpperLine(m_lineUpperBound);
        const QSignalBlocker blockerLowerCheck(m_checkLowerBound);
        const QSignalBlocker blockerLowerLine(m_lineLowerBound);
        const QSignalBlocker blockerComboType(m_comboConstraintType);
        const QSignalBlocker blockerComboMode(m_comboConstraintMode);

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

        // 4. 回显约束方式（流道优化新增）
        m_comboConstraintMode->setCurrentText(_optimizationConstraintData->_constraintMode);

        slot_constraintTypeChanged(_optimizationConstraintData->_constraintType);
    }

    void mRunnerOptimizationConstraintPanel::uneditRunnerOptimizationConstraintPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _optimizationConstraintData = nullptr;
    }

    void mRunnerOptimizationConstraintPanel::updatePropertyCombobox(QComboBox* combobox)
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

    void mRunnerOptimizationConstraintPanel::slot_constraintTypeChanged(const QString& text)
    {
        if (_optimizationConstraintData)
            _optimizationConstraintData->_constraintType = text;

        auto fold = m_foldWidgets[1];
        fold->setTitle(QString("结构%1").arg(text));
    }

    void mRunnerOptimizationConstraintPanel::slot_constraintModeChanged(const QString& text)
    {
        if (_optimizationConstraintData)
            _optimizationConstraintData->_constraintMode = text;
    }

    void mRunnerOptimizationConstraintPanel::slot_checkPropertyChanged(int state)
    {
        if (_optimizationConstraintData)
            _optimizationConstraintData->_isArea = (state == Qt::Checked);

        m_comboProperty->setEnabled(_optimizationConstraintData->_isArea);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationConstraintSig(false, _optimizationConstraintData->getId());
    }

    void mRunnerOptimizationConstraintPanel::slot_propertyChanged(const QString& text)
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

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationConstraintSig(false, _optimizationConstraintData->getId());
    }
}
