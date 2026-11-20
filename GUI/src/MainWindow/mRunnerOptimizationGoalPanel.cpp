
#include "mRunnerOptimizationGoalPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"

//BasicData
#include "mRunnerStructOption.h"
#include "mRunnerOptimizationGoalData.h"

//Qt
#include <QComboBox>
#include <QMessageBox>
#include <QLabel>
using namespace MechStructOptionData;
using namespace MxFunctions;

namespace MechMainWindow
{
    mRunnerOptimizationGoalPanel::mRunnerOptimizationGoalPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _optimizationGoalData = nullptr;

        _mainLayout = new QVBoxLayout(this);

        //优化目标组
        FoldSection* goalFold = new FoldSection("优化目标", this);

        _comboObjectiveType = new QComboBox(); _comboObjectiveType->addItems({ "最小化", "最大化" });
        goalFold->addRow(new PanelLabel("优化方式"), _comboObjectiveType);
        connect(_comboObjectiveType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationGoalPanel::slot_objectiveTypeChanged);

        _comboResponseType = new QComboBox(); _comboResponseType->addItems({ "平均压降", "流量均匀性" });
        goalFold->addRow(new PanelLabel("优化响应"), _comboResponseType);
        connect(_comboResponseType, &QComboBox::currentTextChanged, this, &mRunnerOptimizationGoalPanel::slot_responseTypeChanged);

        m_foldWidgets.append(goalFold);
        _mainLayout->addWidget(goalFold);

        _mainLayout->addStretch();
    }

    mRunnerOptimizationGoalPanel::~mRunnerOptimizationGoalPanel()
    {
        if (_mainLayout != nullptr)
        {
            delete _mainLayout;
            _mainLayout = nullptr;
        }
    }

    void mRunnerOptimizationGoalPanel::editRunnerOptimizationGoalPanel(const QString& responseType)
    {
        if (!getCVM)
        {
            return;
        }
        _optimizationGoalData = getCVM->getModelData()->getRunnerStructOption()->_goalData;
        if (_optimizationGoalData == nullptr)
        {
            return;
        }
        setEditing(true);

        const QSignalBlocker blockerObj(_comboObjectiveType);
        _comboObjectiveType->setCurrentText(_optimizationGoalData->_objectiveType);

        const QSignalBlocker blockerRes(_comboResponseType);
        if (!responseType.isEmpty())
        {
            _comboResponseType->setCurrentText(responseType);
            _optimizationGoalData->_responseType = responseType;
        }
        else
        {
            _comboResponseType->setCurrentText(_optimizationGoalData->_responseType);
        }
    }

    void mRunnerOptimizationGoalPanel::uneditRunnerOptimizationGoalPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        _optimizationGoalData = nullptr;
    }

    void mRunnerOptimizationGoalPanel::slot_objectiveTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_objectiveType = text;
    }

    void mRunnerOptimizationGoalPanel::slot_responseTypeChanged(const QString& text)
    {
        if (!_optimizationGoalData)
        {
            return;
        }
        _optimizationGoalData->_responseType = text;
    }
}
