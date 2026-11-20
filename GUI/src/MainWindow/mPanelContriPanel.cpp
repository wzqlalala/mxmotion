#include "mPanelContriPanel.h"

#include "mCalcLineEdit.h"
#include "mInfoWindow.h"
#include "mMechGlobalSignals.h"
#include "mMaterialManager.h"
#include "mSetManager.h"
#include "mMapRelationManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetBase.h"
#include "mCaseBase.h"
#include "mCaseImplASCoupling.h"

#include <QCompleter>
#include <QTimer>
#include <QPointer>
#include <functional>
#include <algorithm>
#include <QStringListModel>

#include "mGlobalParam.h"

using namespace MechCommon;
using namespace MechSetData;
using namespace MechCaseData;

namespace MechMainWindow
{
    mPanelContriPanel::mPanelContriPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        data = nullptr;
        /**********************************************面板贡献量**********************************************************/

        FoldSection* panelContriFold = new FoldSection("面板贡献量参数", this);

        _combPnlFreqSet = new QComboBox();
        panelContriFold->addRow(new PanelLabel("输出频率点集合"), _combPnlFreqSet);
        _combPnlFreqSet->addItem("");
        connect(_combPnlFreqSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = -1;
                if (!txt.isEmpty())
                {
                    setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                }
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_PanelContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_FrequencySet);
            });

        _combPnlOutType = new QComboBox();
        panelContriFold->addRow(new PanelLabel("输出类型"), _combPnlOutType);
        _combPnlOutType->addItems({ "幅值","实部","虚部" });
        connect(_combPnlOutType, &QComboBox::currentTextChanged, [this](QString txt)
            {
                if (data)
                    data->_outPutType = txt;
            });

        _combPnlRespSet = new QComboBox();
        panelContriFold->addRow(new PanelLabel("响应点集合"), _combPnlRespSet);
        _combPnlRespSet->addItem("");
        connect(_combPnlRespSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = -1;
                if (!txt.isEmpty())
                {
                    setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                }
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_PanelContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ResponseGridSet);
            });

        _checkBoxPnlPkScr = new QCheckBox();
        panelContriFold->addRow(new PanelLabel("峰值筛选"), _checkBoxPnlPkScr);
        connect(_checkBoxPnlPkScr, &QCheckBox::stateChanged, this, [=](int state)
            {
                if (data)
                    //峰值筛选可与响应点集合共存，不再禁用响应点集合下拉框
                    data->_checkBoxPkScr = (state == Qt::Checked);
            });

        m_foldWidgets.append(panelContriFold);
        _mainLayout->addWidget(panelContriFold);
        _mainLayout->addStretch();
    }

    mPanelContriPanel::~mPanelContriPanel()
    {
    }

    void mPanelContriPanel::createPanelContri(int id)
    {
        if (!getCVM) return;

        if (id == 0) //id等于0，从模型树创建
        {
            auto globalParam = getCVM->getModelData()->getGlobalParam();
            if (globalParam->getPanelContriData())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个面板贡献量！"), true);
                return;
            }
            data = globalParam->createPanelContriData();
        }
        else//从工况创建
        {
            //根据id获取工况数据
            mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(id);
            if (caseBase == nullptr) return;
            //转为声固耦合
            if (caseBase->getCaseType() == ASCoupling)
            {
                mCaseImplASCoupling* analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                if (analysis->getPanelContriData())
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个面板贡献量！"), true);
                    return;
                }
                data = analysis->createPanelContriData();
            }

        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建面板贡献量"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContriSig(true, id, "面板贡献量");
    }

    void mPanelContriPanel::editPanelContri(int contriID)
    {
        //获取正确的data
        if (contriID == 0) //0为模型树上的
        {
            if (!getCVM) return;
            data = getCVM->getModelData()->getGlobalParam()->getPanelContriData();
            if (data == nullptr) return;
        }
        else //根据工况id获取工况，再根据工况获取data
        {
            //根据id获取工况数据
            mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(contriID);
            if (caseBase == nullptr) return;
            //转为声固耦合
            if (caseBase->getCaseType() == ASCoupling)
            {
                mCaseImplASCoupling* analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                if (analysis == nullptr) return;
                data = analysis->getPanelContriData();
            }
        }

        //填充面板
        //输出频率点集合
        _combPnlFreqSet->blockSignals(true);
        _combPnlFreqSet->clear();
        QStringList freqNames = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::FrequencyPoint);
        _combPnlFreqSet->addItem("");
        _combPnlFreqSet->addItems(freqNames);
        int freqSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_PanelContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_FrequencySet);
        if (freqSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(freqSetId);
            _combPnlFreqSet->setCurrentText(setName);
        }
        else
        {
            _combPnlFreqSet->setCurrentIndex(0);
            // 默认选择空白行，不做映射
        }
        _combPnlFreqSet->blockSignals(false);

        //输出类型
        _combPnlOutType->blockSignals(true);
        _combPnlOutType->setCurrentText(data->_outPutType);
        _combPnlOutType->blockSignals(false);

        //响应点集合
        _combPnlRespSet->blockSignals(true);
        _combPnlRespSet->clear();
        QStringList nodeNames = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
        _combPnlRespSet->addItem("");
        _combPnlRespSet->addItems(nodeNames);
        int respSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_PanelContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ResponseGridSet);
        if (respSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(respSetId);
            _combPnlRespSet->setCurrentText(setName);
        }
        else
        {
            _combPnlRespSet->setCurrentIndex(0);
            // 默认选择空白行，不做映射
        }
        _combPnlRespSet->blockSignals(false);

        //峰值筛选
        _checkBoxPnlPkScr->setChecked(data->_checkBoxPkScr);
    }
}
