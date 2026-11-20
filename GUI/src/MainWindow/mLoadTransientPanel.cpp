#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "mDataStatusManager.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSurface.h>
#include<mElementSet.h>

#include "mLoadTransientPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadTransientPanel::mLoadTransientPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadTransientPanel::~mLoadTransientPanel()
    {

    }

    void mLoadTransientPanel::initInputs()
    {
        QStringList combonames =
        {
            "载荷名称",
            "载荷类型"
        };

        for (int i = 0; i < 2; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }


        QStringList inpnames =
        {
           "延迟时间",
        };

        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.labelname->setText(inpnames[i]);
            inp.lineeditvalue->setUnit("秒");
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveDelayTime();
                });

        }
        //载荷名称
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadTransientPanel::slot_loadNameChanged);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(0);
        comboBoxs[0].combobox->blockSignals(false);

        //载荷类型
        comboBoxs[1].combobox->addItems({ "力","位移","速度","加速度" });
        connect(comboBoxs[1].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadTransientPanel::slot_loadTypeChanged);
        comboBoxs[1].combobox->blockSignals(true);
        comboBoxs[1].combobox->setCurrentIndex(index);
        comboBoxs[1].combobox->blockSignals(false);
    }

    void mLoadTransientPanel::clearInputs()
    {
        inputWidgets.clear();
    }

    void mLoadTransientPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        //设置延迟时间数据
        std::shared_ptr<MechCaseData::mLoadTransient> pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadTransient>(loadData);
        QVariant padata = pressurePtr->pa_;
        index = pressurePtr->way_;
        QString pastr = "";
        if (!padata.isNull() && padata.isValid())
        {
            pastr = MxFunctions::mNumberToString(padata.value<double>());
        }
        inputWidgets[0].lineeditvalue->setExpression(pastr);

        //填充载荷名称下拉框
        comboBoxs[0].combobox->clear();
        _loadboundtable.clear();

        //获取所有力，重力，力矩，压力的名称
        auto allForceLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_FORCE);
        auto allGravityLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_GRAVITY);
        auto allTorqueLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_TORQUE);
        auto allPressureLoadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_PRESSURE);
        //获取所有位移/转角
        auto allFixedBoundName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(BOUNDARY_IM_DISPLACEMENT);

        //力
        for (auto loadName : allForceLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //重力
        for (auto loadName : allGravityLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //力矩
        for (auto loadName : allTorqueLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //压力
        for (auto loadName : allPressureLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }

        //位移/转角
        for (auto boundName : allFixedBoundName)
        {
            //根据名称获取id
            int boundid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(boundName);
            //映射到载荷哈希表中
            comboBoxs[0].combobox->addItem(boundName);
            _loadboundtable[boundName] = boundid;
        }

        //设置载荷名称下拉框选取目标
        auto& maploadbound = getCVM->getModelData()->getMapRelationManager();

        int loadboundid = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_LoadBoundary);
        if (loadboundid != -1)
        {
            QString loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(loadboundid);
            comboBoxs[0].combobox->setCurrentText(loadName);
        }

        //设置载荷类型下标
        comboBoxs[1].combobox->setCurrentIndex(index);
    }

    QVector<QWidget*> mLoadTransientPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(comboBoxs[0].propwid);
        wds.append(inputWidgets[0].propwid);
        wds.append(comboBoxs[1].propwid);
        return wds;
    }

    QVector<QWidget*> mLoadTransientPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    void mLoadTransientPanel::saveDelayTime()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadTransient>(_loadData);
        if (!load)
            return;

        QString pastr = inputWidgets[0].lineeditvalue->text().trimmed();
        if (!pastr.isEmpty())
        {
            load->pa_ = QVariant::fromValue<double>(pastr.toDouble());
        }
        else
        {
            load->pa_ = QVariant();
        }
        notifyLoadChanged();
    }

    void mLoadTransientPanel::saveLoadBound()
    {
        if (!_loadData)
            return;

        int loadboundid = getLoadBoundId();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        if (loadboundid != -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_LoadBoundary, loadboundid });
        }
        notifyLoadChanged();
    }

    void mLoadTransientPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadTransientPanel::getPickedData()
    {
        return _picks;
    }

    void mLoadTransientPanel::slot_loadTypeChanged(int idx)
    {
        index = idx;
        if (_loadData)
        {
            std::shared_ptr<MechCaseData::mLoadTransient> loadPtr = std::dynamic_pointer_cast<MechCaseData::mLoadTransient>(_loadData);
            if (loadPtr)
            {
                loadPtr->way_ = index;
            }
        }
        notifyLoadChanged();
    }

    void mLoadTransientPanel::slot_loadNameChanged(int idx)
    {
        Q_UNUSED(idx)
        saveLoadBound();
    }

    int mLoadTransientPanel::getLoadBoundId()
    {
        int loadboundid = -1;
        QString loadboundname = comboBoxs[0].combobox->currentText();
        if (_loadboundtable.find(loadboundname) != _loadboundtable.end())
        {
            loadboundid = _loadboundtable[loadboundname];
        }
        return loadboundid;
    }
}