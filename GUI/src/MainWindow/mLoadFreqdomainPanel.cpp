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

#include "mLoadFreqdomainPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadFreqdomainPanel::mLoadFreqdomainPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadFreqdomainPanel::~mLoadFreqdomainPanel()
    {

    }

    void mLoadFreqdomainPanel::initInputs()
    {
        QStringList combonames =
        {
            "频域载荷类型",
            "载荷名称",
            "载荷类型"
        };

        for (int i = 0; i < 3; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }


        QStringList inpnames =
        {
           "延迟时间",
           "相位"
        };


        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.labelname->setText(inpnames[i]);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveDelayAndPhase();
                });
        }

        _units = QStringList
        {
           "度",
           "弧度"
        };
        QVector<int> unitto = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = _units[unitto[1]];
        inputWidgets[0].lineeditvalue->setUnit("秒");
        inputWidgets[1].lineeditvalue->setUnit(unit);

        //频域载荷类型
        comboBoxs[0].combobox->addItems({ "rload","rload1" });
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadFreqdomainPanel::slot_freqLoadTypeChanged);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(freqTypeIndex);
        comboBoxs[0].combobox->blockSignals(false);


        //载荷名称
        connect(comboBoxs[1].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadFreqdomainPanel::slot_loadNameChanged);
        comboBoxs[1].combobox->setCurrentIndex(0);

        //载荷类型
        comboBoxs[2].combobox->addItems({ "力","位移","速度","加速度" });
        connect(comboBoxs[2].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadFreqdomainPanel::slot_loadTypeChanged);
        comboBoxs[2].combobox->blockSignals(true);
        comboBoxs[2].combobox->setCurrentIndex(loadTypeIndex);
        comboBoxs[2].combobox->blockSignals(false);


        //函数
        combofunc1.initWidget("幅值曲线");
        combofunc1.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        combofunc2.initWidget("相位曲线");
        combofunc2.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        //获取所有函数id
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        _functable[""] = -1;
        combofunc2.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            //通过id获取函数名，并插入下拉框中
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc1.combobox->addItem(funcname);
            combofunc2.combobox->addItem(funcname);
            //放到哈希表中
            _functable[funcname] = funcid;
        }
        //每次点击函数下拉框，局部保存函数映射
        connect(combofunc1.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadFreqdomainPanel::saveFunc1);
        connect(combofunc2.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadFreqdomainPanel::saveFunc2);

        //每次点击函数1按钮添加函数做的事
        connect(combofunc1.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            QString curFunc1 = combofunc1.combobox->currentText();
            QString curFunc2 = combofunc2.combobox->currentText();

            combofunc1.combobox->clear();
            combofunc2.combobox->clear();
            _functable.clear();
            _functable[""] = -1;
            combofunc2.combobox->addItem("");

            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc1.combobox->addItem(funcname);
                combofunc2.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }

            if (combofunc1.combobox->findText(curFunc1) >= 0)
                combofunc1.combobox->setCurrentText(curFunc1);
            else
                combofunc1.combobox->setCurrentIndex(allFunc.size() - 1);

            if (combofunc2.combobox->findText(curFunc2) >= 0)
                combofunc2.combobox->setCurrentText(curFunc2);

            std::shared_ptr<MechCaseData::mLoadFreqdomain> loadPtr = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(_loadData);
            saveFunc1();
            });

        //每次点击函数2按钮添加函数做的事
        connect(combofunc2.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            QString curFunc1 = combofunc1.combobox->currentText();
            QString curFunc2 = combofunc2.combobox->currentText();

            combofunc1.combobox->clear();
            combofunc2.combobox->clear();
            _functable.clear();
            _functable[""] = -1;
            combofunc2.combobox->addItem("");

            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc1.combobox->addItem(funcname);
                combofunc2.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }

            if (combofunc1.combobox->findText(curFunc1) >= 0)
                combofunc1.combobox->setCurrentText(curFunc1);

            if (combofunc2.combobox->findText(curFunc2) >= 0)
                combofunc2.combobox->setCurrentText(curFunc2);
            else
                combofunc2.combobox->setCurrentIndex(allFunc.size() - 1);

            std::shared_ptr<MechCaseData::mLoadFreqdomain> loadPtr = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(_loadData);
            saveFunc2();
            });
    }

    void mLoadFreqdomainPanel::clearInputs()
    {
        inputWidgets.clear();
    }

    void mLoadFreqdomainPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        //设置系统单位
        QVector<int> unitto = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Angle, { 3,0,0,0 }, unitto);

        QString unit = _units[unitto[1]];

        //设置延迟时间,相位数据 
        std::shared_ptr<MechCaseData::mLoadFreqdomain> pressurePtr = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(loadData);

        QVariant padata1 = pressurePtr->delay_;
        QVariant padata2 = pressurePtr->phase_;

        freqTypeIndex = pressurePtr->way2_;
        loadTypeIndex = pressurePtr->way_;

        QString pastr1 = "";
        QString pastr2 = "";
        if (!padata1.isNull() && padata1.isValid())
        {
            pastr1 = MxFunctions::mNumberToString(padata1.value<double>());
        }
        if (!padata2.isNull() && padata2.isValid())
        {
            pastr2 = MxFunctions::mNumberToString(padata2.value<double>() * ratio);
        }
        inputWidgets[0].lineeditvalue->setExpression(pastr1);
        inputWidgets[1].lineeditvalue->setExpression(pastr2);
        inputWidgets[1].lineeditvalue->setUnit(unit);

        //填充载荷名称下拉框
        comboBoxs[1].combobox->clear();
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
            // 跳过被抑制的载荷
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[1].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //重力
        for (auto loadName : allGravityLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            // 跳过被抑制的载荷
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[1].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //力矩
        for (auto loadName : allTorqueLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            // 跳过被抑制的载荷
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[1].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }
        //压力
        for (auto loadName : allPressureLoadName)
        {
            //根据名称获取id
            int loadid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(loadName);
            // 跳过被抑制的载荷
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(loadid);
            if (refLoad && refLoad->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[1].combobox->addItem(loadName);
            _loadboundtable[loadName] = loadid;
        }

        //位移转角
        for (auto boundName : allFixedBoundName)
        {
            //根据名称获取id
            int boundid = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(boundName);
            // 跳过被抑制的边界
            auto refBc = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(boundid);
            if (refBc && refBc->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[1].combobox->addItem(boundName);
            _loadboundtable[boundName] = boundid;
        }


        auto& maploadbound = getCVM->getModelData()->getMapRelationManager();

        //设置载荷名称下拉框选取目标
        int loadboundid = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_LoadBoundary);
        if (loadboundid != -1)
        {
            QString loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(loadboundid);
            comboBoxs[1].combobox->setCurrentText(loadName);
        }

        //设置函数
        combofunc1.combobox->clear();
        combofunc2.combobox->clear();
        _functable[""] = -1;
        combofunc2.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc1.combobox->addItem(funcname);
            combofunc2.combobox->addItem(funcname);

            _functable[funcname] = funcid;
        }

        int funcid1 = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_AmplitudeFunction);
        if (funcid1 != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid1);
            combofunc1.combobox->setCurrentText(funcname);
        }

        int funcid2 = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_PhaseFunction);
        if (funcid2 != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid2);
            combofunc2.combobox->setCurrentText(funcname);
        }

        //设置载荷类型下标
        comboBoxs[0].combobox->setCurrentIndex(freqTypeIndex);
        comboBoxs[2].combobox->setCurrentIndex(loadTypeIndex);

        // 若数据尚无对应映射，仅补充建立缺失的映射关系
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int curLoadBoundId = getLoadBoundId();
        if (curLoadBoundId != -1 &&
            mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_LoadBoundary) == -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,loadData->getLoadBoundaryId() }, { Physical_LoadBoundary, curLoadBoundId });
        }
        int curFuncId1 = getFuncId1();
        if (curFuncId1 != -1 &&
            mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_AmplitudeFunction) == -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,loadData->getLoadBoundaryId() }, { Physical_Function, curFuncId1 }, MapRelationType::MapRelation_AmplitudeFunction);
        }
        int curFuncId2 = getFuncId2();
        if (curFuncId2 != -1 &&
            mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_PhaseFunction) == -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,loadData->getLoadBoundaryId() }, { Physical_Function, curFuncId2 }, MapRelationType::MapRelation_PhaseFunction);
        }
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, pressurePtr->getLoadBoundaryId());
    }

    QVector<QWidget*> mLoadFreqdomainPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(comboBoxs[0].propwid);
        wds.append(comboBoxs[1].propwid);
        wds.append(inputWidgets[0].propwid);
        wds.append(inputWidgets[1].propwid);
        wds.append(combofunc1.propwid);
        wds.append(combofunc2.propwid);
        wds.append(comboBoxs[2].propwid);
        return wds;
    }

    QVector<QWidget*> mLoadFreqdomainPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    void mLoadFreqdomainPanel::saveDelayAndPhase()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(_loadData);
        if (!load)
            return;

        QVector<int>curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Angle, curidx, { 3,0,0,0 });

        QString pastr1 = inputWidgets[0].lineeditvalue->text().trimmed();
        QString pastr2 = inputWidgets[1].lineeditvalue->text().trimmed();
        if (!pastr1.isEmpty())
        {
            load->delay_ = QVariant::fromValue<double>(pastr1.toDouble());
        }
        else
        {
            load->delay_ = QVariant();
        }

        if (!pastr2.isEmpty())
        {
            load->phase_ = QVariant::fromValue<double>(pastr2.toDouble() * ratio);
        }
        else
        {
            load->phase_ = QVariant();
        }

        notifyLoadChanged();
    }

    void mLoadFreqdomainPanel::saveLoadBound()
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

    void mLoadFreqdomainPanel::saveFunc1()
    {
        if (!_loadData)
            return;

        int funcid1 = getFuncId1();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        if (funcid1 != -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function, funcid1 }, MapRelationType::MapRelation_AmplitudeFunction);
        }
        notifyLoadChanged();
    }

    void mLoadFreqdomainPanel::saveFunc2()
    {
        if (!_loadData)
            return;

        int funcid2 = getFuncId2();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        if (funcid2 != -1)
        {
            mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function, funcid2 }, MapRelationType::MapRelation_PhaseFunction);
        }
        notifyLoadChanged();
    }

    void mLoadFreqdomainPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadFreqdomainPanel::getPickedData()
    {
        return _picks;
    }

    void mLoadFreqdomainPanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadFreqdomainPanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadFreqdomainPanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadFreqdomainPanel::slot_unitSystemResponse);
    }

    void mLoadFreqdomainPanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        //频域载荷量纲为角度，只响应角度单位/全量变化
        if (!(type & AngleUnit)) return;
        //单位切换
        inputWidgets[1].lineeditvalue->setUnit(_units[idx[1]]);
        QString valuestr = inputWidgets[1].lineeditvalue->text().trimmed();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getSwitchAngleUnitRatio();
        if (!valuestr.isEmpty())
        {
            double value = valuestr.toDouble();
            value *= ratio;
            inputWidgets[1].lineeditvalue->setExpression(MxFunctions::mNumberToString(value));
        }
    }
    void mLoadFreqdomainPanel::slot_loadTypeChanged(int idx)
    {
        loadTypeIndex = idx;
        if (_loadData)
        {
            std::shared_ptr<MechCaseData::mLoadFreqdomain> loadPtr = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(_loadData);
            if (loadPtr)
            {
                loadPtr->way_ = loadTypeIndex;
            }
        }
        notifyLoadChanged();
    }

    void mLoadFreqdomainPanel::slot_freqLoadTypeChanged(int idx)
    {
        freqTypeIndex = idx;
        if (_loadData)
        {
            std::shared_ptr<MechCaseData::mLoadFreqdomain> loadPtr = std::dynamic_pointer_cast<MechCaseData::mLoadFreqdomain>(_loadData);
            if (loadPtr)
            {
                loadPtr->way2_ = freqTypeIndex;
            }
        }

        if (freqTypeIndex == 0)
        {
            combofunc1.labelname->clear();
            combofunc2.labelname->clear();

            combofunc1.labelname->setText("幅值曲线");
            combofunc2.labelname->setText("相位曲线");
        }
        else if (freqTypeIndex == 1)
        {
            combofunc1.labelname->clear();
            combofunc2.labelname->clear();

            combofunc1.labelname->setText("实部曲线");
            combofunc2.labelname->setText("虚部曲线");
        }

        notifyLoadChanged();
    }

    void mLoadFreqdomainPanel::slot_funcChanged(int index)
    {
        Q_UNUSED(index)
    }

    void mLoadFreqdomainPanel::slot_loadNameChanged(int idx)
    {
        Q_UNUSED(idx)
        saveLoadBound();
    }

    int mLoadFreqdomainPanel::getLoadBoundId()
    {
        int loadboundid = -1;
        QString loadboundname = comboBoxs[1].combobox->currentText();
        if (_loadboundtable.find(loadboundname) != _loadboundtable.end())
        {
            loadboundid = _loadboundtable[loadboundname];
        }
        return loadboundid;
    }

    int mLoadFreqdomainPanel::getFuncId1()
    {
        int funcid = -1;
        QString funcname = combofunc1.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    int mLoadFreqdomainPanel::getFuncId2()
    {
        int funcid = -1;
        QString funcname = combofunc2.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }
}