#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mConnectorManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"

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

#include "mLoadAcousticFrequencyDomainPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadAcousticFrequencyDomainPanel::mLoadAcousticFrequencyDomainPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadAcousticFrequencyDomainPanel::~mLoadAcousticFrequencyDomainPanel()
    {
        clearInputs();
    }

    void mLoadAcousticFrequencyDomainPanel::initInputs()
    {
        QStringList combonames =
        {
            "调用载荷名称",
        };

        for (int i = 0; i < 1; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }

        QStringList inpnames =
        {
           "声场密度",
           "体积模量",
           "延迟时间",
           "相位"
        };

        for (int i = 0; i < 4; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.labelname->setText(inpnames[i]);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveValues();
                });
        }

        //选择声学载荷
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadAcousticFrequencyDomainPanel::saveAcousticLoad);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(0);
        comboBoxs[0].combobox->blockSignals(false);


        //函数
        combofunc.initWidget("曲线");
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));

        //获取所有函数id
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            //通过id获取函数名，并插入下拉框中
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            //放到哈希表中
            _functable[funcname] = funcid;
        }
        //每次点击函数下拉框，局部保存函数映射
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadAcousticFrequencyDomainPanel::saveFunc);

        //每次点击函数1按钮添加函数做的事
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->clear();
            _functable.clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() - 1);
            saveFunc();
            });
    }

    void mLoadAcousticFrequencyDomainPanel::clearInputs()
    {
        for (auto inp : inputWidgets)
        {
            inp.clear();
        }
        inputWidgets.clear();
        for (auto combo : comboBoxs)
        {
            combo.clear();
        }
        comboBoxs.clear();
    }

    void mLoadAcousticFrequencyDomainPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        //设置值
        std::shared_ptr<MechCaseData::mLoadAcousticFrequencyDomain> acousticPtr = std::dynamic_pointer_cast<MechCaseData::mLoadAcousticFrequencyDomain>(loadData);

        QVariant padata1 = acousticPtr->rho_;
        QVariant padata2 = acousticPtr->B_;
        QVariant padata3 = acousticPtr->delay_;
        QVariant padata4 = acousticPtr->xita_;

        QString pastr1 = "";
        QString pastr2 = "";
        QString pastr3 = "";
        QString pastr4 = "";

        if (!padata1.isNull() && padata1.isValid())
        {
            pastr1 = MxFunctions::mNumberToString(padata1.value<double>());
        }
        inputWidgets[0].lineeditvalue->setExpression(pastr1);

        if (!padata2.isNull() && padata2.isValid())
        {
            pastr2 = MxFunctions::mNumberToString(padata2.value<double>());
        }
        inputWidgets[1].lineeditvalue->setExpression(pastr2);

        if (!padata3.isNull() && padata3.isValid())
        {
            pastr3 = MxFunctions::mNumberToString(padata3.value<double>());
        }
        inputWidgets[2].lineeditvalue->setExpression(pastr3);

        if (!padata4.isNull() && padata4.isValid())
        {
            pastr4 = MxFunctions::mNumberToString(padata4.value<double>());
        }
        inputWidgets[3].lineeditvalue->setExpression(pastr4);


        //填充螺栓名称下拉框
        comboBoxs[0].combobox->clear();
        _acousticloadtable.clear();

        //获取所有声学载荷（目前仅有声学载荷）
        auto allAcousticConcentratedForceName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getAllLoadBoundaryNamesByTypes(LOAD_IM_ACOCONCFORCE);
        //声学载荷
        for (auto Name : allAcousticConcentratedForceName)
        {
            //根据名称获取id
            int id = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getIDByLoadBoundaryName(Name);
            // 跳过被抑制的声学载荷
            auto refLoad = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryData(id);
            if (refLoad && refLoad->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(Name);
            _acousticloadtable[Name] = id;
        }

        auto& maploadbound = getCVM->getModelData()->getMapRelationManager();

        //设置载荷名称下拉框选取目标
        int id = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_LoadBoundary);
        if (id != -1)
        {
            QString loadName = getCVM->getModelData()->getManager<MechCaseData::mLoadBoundaryManager>(Physical_LoadBoundary).getLoadBoundaryNameByID(id);
            comboBoxs[0].combobox->setCurrentText(loadName);
        }
        else
        {
            comboBoxs[0].combobox->setCurrentIndex(0);
            int acousticid = getAcousticLoad();
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_LoadBoundary,acousticid });
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
        }

        //设置函数
        combofunc.combobox->blockSignals(true);

        combofunc.combobox->clear();
        _functable.clear();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }

        int funcid = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }
        else
        {
            combofunc.combobox->setCurrentIndex(0);
        }
        combofunc.combobox->blockSignals(false);
    }

    QVector<QWidget*> mLoadAcousticFrequencyDomainPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        return wds;
    }

    QVector<QWidget*> mLoadAcousticFrequencyDomainPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[0].propwid);
        wds.append(inputWidgets[0].propwid);
        wds.append(inputWidgets[1].propwid);
        wds.append(inputWidgets[2].propwid);
        wds.append(inputWidgets[3].propwid);
        wds.append(combofunc.propwid);
        return wds;
    }

    int mLoadAcousticFrequencyDomainPanel::getFuncId()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    int mLoadAcousticFrequencyDomainPanel::getAcousticLoad()
    {
        int AcousticLoadid = -1;
        QString AcousticLoadname = comboBoxs[0].combobox->currentText();
        if (_acousticloadtable.find(AcousticLoadname) != _acousticloadtable.end())
        {
            AcousticLoadid = _acousticloadtable[AcousticLoadname];
        }
        return AcousticLoadid;
    }

    void mLoadAcousticFrequencyDomainPanel::slot_funcChanged(int idx)
    {
        saveFunc();
    }

    void mLoadAcousticFrequencyDomainPanel::slot_selectAcousticLoadChanged(int idx)
    {
        saveAcousticLoad();
    }

    void mLoadAcousticFrequencyDomainPanel::saveValues()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadAcousticFrequencyDomain>(_loadData);
        if (!load)
            return;

        auto readDouble = [&](int idx) -> QVariant {
            if (idx < 0 || idx >= inputWidgets.size())
                return QVariant();
            QString txt = inputWidgets[idx].lineeditvalue->text().trimmed();
            if (!txt.isEmpty())
                return QVariant::fromValue<double>(txt.toDouble());
            return QVariant();
        };

        load->rho_ = readDouble(0);
        load->B_ = readDouble(1);
        load->delay_ = readDouble(2);
        load->xita_ = readDouble(3);
        notifyLoadChanged();
    }

    void mLoadAcousticFrequencyDomainPanel::saveAcousticLoad()
    {
        if (!_loadData)
            return;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int acousticid = getAcousticLoad();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_LoadBoundary,acousticid });
        notifyLoadChanged();
    }

    void mLoadAcousticFrequencyDomainPanel::saveFunc()
    {
        if (!_loadData)
            return;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int funcid = getFuncId();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function,funcid });
        notifyLoadChanged();
    }

    void mLoadAcousticFrequencyDomainPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }
}

