#include "mEleSetCtrlPanel.h"

#include "mMechGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"

//通用拾取控件
#include "mGeneralPickUp.h"
#include "mGetSetFunction.h"
#include "mGetSetTypeTemplate.h"
#include "IPickPersistenceStrategy.h"

//BasicData
#include "mSetBase.h"
#include "mSetManager.h"
#include "mMixSet.h"
#include "mModelData.h"
#include "mModelFixManager.h"
#include "mMapRelationManager.h"

#include "mDataStatusManager.h"

//Qt
#include <QHBoxLayout>

using namespace MechPreData;
using namespace MechSetData;
using namespace MechCommon;

namespace MechMainWindow
{
    mEleSetCtrlPanel::mEleSetCtrlPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _eleSetCtrl = nullptr;
        initUI();
    }

    mEleSetCtrlPanel::~mEleSetCtrlPanel()
    {
        // 失活拾取控件：触发 commit 持久化并清理高亮（控件本身随 parent 销毁）
        if (_pickWidgetController)
        {
            _pickWidgetController->setKeepHighlightOnDeactivate(false);
            _pickWidgetController->setPickActive(false);
            _pickWidgetController->setKeepHighlightOnDeactivate(true);
        }
    }

    void mEleSetCtrlPanel::initUI()
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(0, 0, 0, 0);
        _mainLayout->setSpacing(5);

        // 类型折叠面板
        FoldSection* defineFold = new FoldSection("类型", this);

        // 模型修改类型
        PanelLabel* labelModelFixType = new PanelLabel("模型修改类型");
        _comboModelFixType = new QComboBox();
        _comboModelFixType->addItems({ "单元生死" });
        defineFold->addRow(labelModelFixType, _comboModelFixType);

        // 激活/移除单选
        QHBoxLayout* radioLayout = new QHBoxLayout();
        radioLayout->setContentsMargins(0, 0, 0, 0);
        radioLayout->setSpacing(20);
        _btnGroupActiveOrRemove = new QButtonGroup(this);
        _radioActive = new QRadioButton("激活单元");
        _radioRemove = new QRadioButton("移除单元");
        _radioRemove->setChecked(true);
        _btnGroupActiveOrRemove->addButton(_radioActive, 0);
        _btnGroupActiveOrRemove->addButton(_radioRemove, 1);
        radioLayout->addWidget(_radioActive);
        radioLayout->addWidget(_radioRemove);
        radioLayout->addStretch();
        defineFold->addRow(radioLayout);

        m_foldWidgets.append(defineFold);
        _mainLayout->addWidget(defineFold);

        // 作用对象折叠面板：通用拾取控件（拾取单元 / 选择单元集合）
        FoldSection* targetFold = new FoldSection("作用对象", this);

        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        _pickWidgetController->setDefultTip("选择对象");

        // 激活时切换到单元拾取模式
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(8);
            }
            });

        // 拾取/集合变化：同步数据状态
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            if (_eleSetCtrl && getCVM)
            {
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModelFix, _eleSetCtrl->getId());
            }
            });

        targetFold->addRow(_pickWidgetController->widget());

        m_foldWidgets.append(targetFold);
        _mainLayout->addWidget(targetFold);

        _mainLayout->addStretch();

        connect(_btnGroupActiveOrRemove, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &mEleSetCtrlPanel::slot_activeOrRemoveChanged);
    }

    void mEleSetCtrlPanel::setupPickController()
    {
        if (!_pickWidgetController || !_eleSetCtrl)
        {
            return;
        }

        // 持久化：拾取/集合 -> 内部 mMixSet + Physical_ModelFix 映射（失活 commit 时写入）
        auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
            MxFunctions::PhysicalFieldType::Physical_ModelFix, _eleSetCtrl->getId());
        _pickWidgetController->setPersistenceStrategy(strategy);

        // 集合解析器 + 可选集合列表（仅单元集合）
        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setSelectSetList(getSetNamesList({ SetType::Element }));

        // 仅允许拾取单元
        _pickWidgetController->setAllowedTypes(&MechCommon::pickOnlyElement);
        _pickWidgetController->setPickType();
    }

    int mEleSetCtrlPanel::newEleSetCtrlPanel()
    {
        if (!getCVM)
        {
            return -1;
        }

        // 切换编辑对象前先失活提交上一对象的拾取数据
        deactivatePickAndCommit();

        //创建数据
        int id = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).createID();
        _eleSetCtrl = std::make_shared<mEleSetCtrl>(id);
        getCVM->getModelData()->addPhysicalFieldP(_eleSetCtrl);
        setEditing(true);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateModelFixSig(true, id);

        // 默认移除单元
        _radioRemove->setChecked(true);

        // 绑定持久化策略并清空拾取控件，进入拾取状态
        setupPickController();
        _pickWidgetController->displayData(PickWidgetData{});
        _pickWidgetController->setPickActive(true);

        return id;
    }

    void mEleSetCtrlPanel::editEleSetCtrlPanel(int modelFixId)
    {
        // 切换编辑对象前先失活提交上一对象的拾取数据
        deactivatePickAndCommit();

        if (modelFixId >= 0)
        {
            auto modelFix = getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(modelFixId);
            _eleSetCtrl = std::dynamic_pointer_cast<mEleSetCtrl>(modelFix);
        }

        if (!_eleSetCtrl)
        {
            return;
        }
        setEditing(true);

        updateFromData();
    }

    void mEleSetCtrlPanel::uneditEleSetCtrlPanel()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支无需再单独清标志
        setEditing(false);
        //_eleSetCtrl 为共享指针，数据被删除（树上删除等）后引用仍非空，
        //需向管理器按 ID 复核，已删除则不再走切出收尾逻辑
        if (_eleSetCtrl
            && getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix).getModelFixData(_eleSetCtrl->getId()) == nullptr)
        {
            _eleSetCtrl = nullptr;
            return;
        }
        deactivatePickAndCommit();
        _eleSetCtrl = nullptr;
    }

    void mEleSetCtrlPanel::updateFromData()
    {
        if (!_eleSetCtrl || !_pickWidgetController)
        {
            return;
        }

        // 激活/移除
        if (_eleSetCtrl->isActiveOrRemove())
        {
            _radioActive->setChecked(true);
        }
        else
        {
            _radioRemove->setChecked(true);
        }

        setupPickController();

        // 反显作用对象：从映射关系读取
        PickWidgetData displayData;
        int modelFixId = _eleSetCtrl->getId();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int setId = mapload.getMapRelation<int>(
            { MxFunctions::PhysicalFieldType::Physical_ModelFix, modelFixId },
            MxFunctions::PhysicalFieldType::Physical_Set);

        if (setId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
            if (set && set->getInternal())
            {
                // 内部 mMixSet：展开拾取数据回显
                if (auto* mixSet = dynamic_cast<mMixSet*>(set))
                {
                    displayData.pickedData = mixSet->getMixSetData();
                }
            }
            else if (set)
            {
                // 外部集合：按名称回显
                displayData.selectedSetNames = { set->getSetName() };
            }
        }

        _pickWidgetController->displayData(displayData);
        _pickWidgetController->setPickActive(true);
    }

    void mEleSetCtrlPanel::deactivatePickAndCommit()
    {
        if (!_pickWidgetController)
        {
            return;
        }

        // 失活提交：保证已拾取数据持久化；不保留临时高亮
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        _pickWidgetController->setKeepHighlightOnDeactivate(false);
        const bool wasActive = _pickWidgetController->isPickActive();
        if (wasActive)
        {
            _pickWidgetController->setPickActive(false);
        }
        // 兜底：清理失活临时高亮残留
        if (renderValid && !wasActive)
        {
            _pickWidgetController->clearAllHighlight();
        }
        _pickWidgetController->setKeepHighlightOnDeactivate(true);

        // 切出面板：恢复拾取工具栏全部按钮
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    void mEleSetCtrlPanel::slot_activeOrRemoveChanged(int id)
    {
        if (!_eleSetCtrl)
        {
            return;
        }
        _eleSetCtrl->setActiveOrRemove(id == 0);
    }
}
