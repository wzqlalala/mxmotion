
#include "mRunnerOptimizationPanel.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mSetManager.h"

//BasicData
#include "mModelData.h"

//Qt
#include <QLabel>
#include <QComboBox>

using namespace MechPreData;
using namespace MechSetData;
using namespace MxFunctions;
using namespace mxr;
using namespace MechStructOptionData;

namespace MechMainWindow
{
    mRunnerOptimizationPanel::mRunnerOptimizationPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 1. 流道优化参数（6个带状态的参数）==========
        FoldSection* runnerFold = new FoldSection("流道优化", this, QColor("#909399"), true);
        _paramBindings.append(addCheckParam(runnerFold, "最大迭代步"));
        _paramBindings.append(addCheckParam(runnerFold, "目标函数容差"));
        _paramBindings.append(addCheckParam(runnerFold, "收敛离散指数"));
        _paramBindings.append(addCheckParam(runnerFold, "优化步长"));
        _paramBindings.append(addCheckParam(runnerFold, "过滤半径"));
        _paramBindings.append(addCheckParam(runnerFold, "惩罚参数"));
        m_foldWidgets.append(runnerFold);
        _mainLayout->addWidget(runnerFold);

        // ========== 2. 流体属性（2个纯输入参数）==========
        FoldSection* fluidFold = new FoldSection("流体属性", this, QColor("#909399"), true);
        _paramBindings.append(addPlainParam(fluidFold, "密度"));
        _paramBindings.append(addPlainParam(fluidFold, "动力粘度"));
        m_foldWidgets.append(fluidFold);
        _mainLayout->addWidget(fluidFold);

        // ========== 3. 流体边界（3个单元面集合选择）==========
        FoldSection* boundaryFold = new FoldSection("流体边界", this, QColor("#909399"), true);
        addSetComboRow(boundaryFold, "入口", _comboInlet,  MapRelation_Master);
        addSetComboRow(boundaryFold, "出口", _comboOutlet, MapRelation_Slave);
        addSetComboRow(boundaryFold, "壁面", _comboWall,   MapRelation_None);
        m_foldWidgets.append(boundaryFold);
        _mainLayout->addWidget(boundaryFold);

        _mainLayout->addStretch();
    }

    mRunnerOptimizationPanel::~mRunnerOptimizationPanel()
    {
    }

    // ------------------------------------------------------------------
    // 辅助：创建"状态+输入"参数行
    // ------------------------------------------------------------------
    mRunnerOptimizationPanel::ParamBinding
    mRunnerOptimizationPanel::addCheckParam(FoldSection* fold, const QString& label)
    {
        ParamBinding binding;
        binding.checkBox = new QCheckBox(label);
        binding.lineEdit = new mCalcLineEdit("", this, true, false);
        binding.lineEdit->setEnabled(false);

        int idx = _paramBindings.size();

        connect(binding.checkBox, &QCheckBox::toggled, this,
            [this, idx](bool checked) {
                auto& b = _paramBindings[idx];
                b.lineEdit->setEnabled(checked);
                if (b.status) *b.status = checked;
                if (checked && b.value)
                    *b.value = b.lineEdit->text();
            });

        connect(binding.lineEdit, &QLineEdit::textChanged, this,
            [this, idx]() {
                auto& b = _paramBindings[idx];
                if (b.checkBox && b.checkBox->isChecked() && b.value)
                    *b.value = b.lineEdit->text();
            });

        fold->addRow(binding.checkBox, binding.lineEdit);
        return binding;
    }

    // ------------------------------------------------------------------
    // 辅助：创建纯输入参数行（无状态复选框）
    // ------------------------------------------------------------------
    mRunnerOptimizationPanel::ParamBinding
    mRunnerOptimizationPanel::addPlainParam(FoldSection* fold, const QString& label)
    {
        ParamBinding binding;
        binding.lineEdit = new mCalcLineEdit("", this, true, false);

        int idx = _paramBindings.size();

        connect(binding.lineEdit, &QLineEdit::editingFinished, this,
            [this, idx]() {
                auto& b = _paramBindings[idx];
                if (b.value)
                    *b.value = b.lineEdit->text();
            });

        fold->addRow(new QLabel(label), binding.lineEdit);
        return binding;
    }

    // ------------------------------------------------------------------
    // 辅助：创建集合选择下拉框行
    // ------------------------------------------------------------------
    void mRunnerOptimizationPanel::addSetComboRow(FoldSection* fold, const QString& label,
                                                   QComboBox*& combo, MxFunctions::MapRelationType type)
    {
        combo = new QComboBox(this);
        combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        updateComboSetList(combo);

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this, combo, type]() {
                if (!getCVM) return;
                int modelID = getCVM->getModelData()->getModelDataID();
                auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
                int setId = setManager.getIDBySetName(combo->currentText());
                auto& mapRelation = getCVM->getModelData()->getMapRelationManager();

                if (combo->currentText().isEmpty())
                {
                    mapRelation.deleteMapRelation(
                        { Physical_RunnerStructoption, modelID }, { Physical_Set, -1 });
                }
                else
                {
                    mapRelation.appendMapRelation(
                        { Physical_RunnerStructoption, modelID }, { Physical_Set, setId }, type);
                }
            });

        fold->addRow(new QLabel(label), combo);
    }

    // ------------------------------------------------------------------
    // 辅助：更新下拉框可选集合列表
    // ------------------------------------------------------------------
    void mRunnerOptimizationPanel::updateComboSetList(QComboBox* combo)
    {
        if (!getCVM || !combo) return;
        QString currentText = combo->currentText();
        combo->blockSignals(true);
        QStringList setNames = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set)
            .getAllSetNamesByTypes(MxFunctions::SetType::ElementSurface);
        setNames.prepend("");
        combo->clear();
        combo->addItems(setNames);
        combo->setCurrentText(currentText);
        combo->blockSignals(false);
    }

    // ------------------------------------------------------------------
    void mRunnerOptimizationPanel::newRunnerOptimization()
    {
        if (!getCVM)
            return;

        if (getCVM->getModelData()->getRunnerStructOption() == nullptr)
            getCVM->getModelData()->createRunnerStructOption();
        else
            getCVM->getModelData()->getRunnerStructOption();

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateRunnerOptimizationSig(true);
    }

    // ------------------------------------------------------------------
    void mRunnerOptimizationPanel::editRunnerOptimization()
    {
        _runnerOptionBase = getCVM->getModelData()->getRunnerStructOption();
        if (!_runnerOptionBase)
            return;

        mRunnerOptionData& d = _runnerOptionBase->_mRunnerOptionData;

        // 按添加顺序绑定数据指针（必须与构造函数中的添加顺序严格一致）
        int idx = 0;
        // --- 流道优化参数 ---
        _paramBindings[idx].status = &d._isMaxiter;          _paramBindings[idx++].value = &d._maxiter;
        _paramBindings[idx].status = &d._isObjectivetolrance;_paramBindings[idx++].value = &d._objectivetolrance;
        _paramBindings[idx].status = &d._isDiscretetarget;   _paramBindings[idx++].value = &d._discretetarget;
        _paramBindings[idx].status = &d._isTopomovelimit;    _paramBindings[idx++].value = &d._topomovelimit;
        _paramBindings[idx].status = &d._isRmin;             _paramBindings[idx++].value = &d._rmin;
        _paramBindings[idx].status = &d._isPenal;            _paramBindings[idx++].value = &d._penal;
        // --- 流体属性 ---
        _paramBindings[idx].status = nullptr;                _paramBindings[idx++].value = &d._density;
        _paramBindings[idx].status = nullptr;                _paramBindings[idx++].value = &d._dynamicViscosity;

        syncDataToUI();

        // --- 流体边界反显 ---
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int modelID = getCVM->getModelData()->getModelDataID();
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);

        updateComboSetList(_comboInlet);
        int inletId = mapRelation.getMapRelation<int>({ Physical_RunnerStructoption, modelID }, Physical_Set, MapRelation_Master);
        _comboInlet->setCurrentText(setManager.getSetNameByID(inletId));

        updateComboSetList(_comboOutlet);
        int outletId = mapRelation.getMapRelation<int>({ Physical_RunnerStructoption, modelID }, Physical_Set, MapRelation_Slave);
        _comboOutlet->setCurrentText(setManager.getSetNameByID(outletId));

        updateComboSetList(_comboWall);
        int wallId = mapRelation.getMapRelation<int>({ Physical_RunnerStructoption, modelID }, Physical_Set, MapRelation_None);
        _comboWall->setCurrentText(setManager.getSetNameByID(wallId));
    }

    // ------------------------------------------------------------------
    // 将内存数据一次性同步到UI
    // ------------------------------------------------------------------
    void mRunnerOptimizationPanel::syncDataToUI()
    {
        for (auto& b : _paramBindings)
        {
            if (b.checkBox && b.status)
                b.checkBox->setChecked(*b.status);
            if (b.lineEdit)
            {
                bool enabled = (b.checkBox == nullptr) || (b.status && *b.status);
                b.lineEdit->setEnabled(enabled);
                if (b.value)
                    b.lineEdit->setExpression(*b.value);
            }
        }
    }
}
