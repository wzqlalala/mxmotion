#include "mTPContriPanel.h"

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

#include <QCompleter>
#include <QTimer>
#include <QPointer>
#include <functional>
#include <algorithm>
#include <QStringListModel>

#include "mGlobalParam.h"
#include "mDataStatusManager.h"

using namespace MechCommon;
using namespace MechSetData;
using namespace MechCaseData;

namespace MechMainWindow
{
    mTPContriPanel::mTPContriPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        data = nullptr;
        /**********************************************传递路径贡献量**********************************************************/

        FoldSection* tpContriFold = new FoldSection("传递路径贡献量参数", this);

        _combTPRespType = new QComboBox();
        tpContriFold->addRow(new PanelLabel("响应类型"), _combTPRespType);
        _combTPRespType->addItems({ "disp","vel","acc" });
        connect(_combTPRespType, &QComboBox::currentTextChanged, [this](QString txt)
            {
                if (data)
                    data->_respType = txt;
            });

        _combTPCnnPtSet = new QComboBox();
        tpContriFold->addRow(new PanelLabel("连接点集合"), _combTPCnnPtSet);
        connect(_combTPCnnPtSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConptSet);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
            });

        _combTPRespPtSet = new QComboBox();
        tpContriFold->addRow(new PanelLabel("响应点集合"), _combTPRespPtSet);
        connect(_combTPRespPtSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ResponseGridSet);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
            });

        _combTPCnnElemSet = new QComboBox();
        tpContriFold->addRow(new PanelLabel("连接单元集合"), _combTPCnnElemSet);
        connect(_combTPCnnElemSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConelSet);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
            });

        _combTPHardLinkElemSet = new QComboBox();
        tpContriFold->addRow(new PanelLabel("硬链接单元集合"), _combTPHardLinkElemSet);
        _combTPHardLinkElemSet->addItem("");
        connect(_combTPHardLinkElemSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = -1;
                if (!txt.isEmpty())
                {
                    setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                }
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConrelSet);
            });

        _combTPPassiveNodeSet = new QComboBox();
        tpContriFold->addRow(new PanelLabel("被动侧节点集合"), _combTPPassiveNodeSet);
        _combTPPassiveNodeSet->addItem("");
        connect(_combTPPassiveNodeSet, &QComboBox::currentTextChanged, [this](QString txt)
            {
                int setId = -1;
                if (!txt.isEmpty())
                {
                    setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
                }
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConvolSet);
            });

        m_foldWidgets.append(tpContriFold);
        _mainLayout->addWidget(tpContriFold);
        _mainLayout->addStretch();
    }

    mTPContriPanel::~mTPContriPanel()
    {
    }

    void mTPContriPanel::createTPContri(int id)
    {
        if (!getCVM) return;

        // 传递路径贡献量只在模型树创建（id=0）
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getTPContriData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个传递路径贡献量！"), true);
            return;
        }
        data = globalParam->createTPContriData();

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建传递路径贡献量"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContriSig(true, id, "传递路径贡献量");
    }

    void mTPContriPanel::editTPContri(int contriID)
    {
        // 传递路径贡献量只在模型树（id=0）
        if (!getCVM) return;
        data = getCVM->getModelData()->getGlobalParam()->getTPContriData();
        if (data == nullptr) return;

        //填充面板
        //响应类型
        _combTPRespType->blockSignals(true);
        _combTPRespType->setCurrentText(data->_respType);
        _combTPRespType->blockSignals(false);

        //连接点集合
        _combTPCnnPtSet->blockSignals(true);
        _combTPCnnPtSet->clear();
        QStringList nodeNames = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
        _combTPCnnPtSet->addItems(nodeNames);
        int cnnPtSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_TPContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ConptSet);
        if (cnnPtSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(cnnPtSetId);
            _combTPCnnPtSet->setCurrentText(setName);
        }
        else
        {
            _combTPCnnPtSet->setCurrentIndex(0);
            // 默认选择第一个时，手动添加映射
            QString setName = _combTPCnnPtSet->currentText();
            int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(setName);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConptSet);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
        }
        _combTPCnnPtSet->blockSignals(false);

        //响应点集合
        _combTPRespPtSet->blockSignals(true);
        _combTPRespPtSet->clear();
        _combTPRespPtSet->addItems(nodeNames);
        int respPtSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_TPContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ResponseGridSet);
        if (respPtSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(respPtSetId);
            _combTPRespPtSet->setCurrentText(setName);
        }
        else
        {
            _combTPRespPtSet->setCurrentIndex(0);
            // 默认选择第一个时，手动添加映射
            QString setName = _combTPRespPtSet->currentText();
            int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(setName);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ResponseGridSet);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
        }
        _combTPRespPtSet->blockSignals(false);

        //连接单元集合
        _combTPCnnElemSet->blockSignals(true);
        _combTPCnnElemSet->clear();
        QStringList elemNames = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Element);
        _combTPCnnElemSet->addItems(elemNames);
        int cnnElemSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_TPContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ConelSet);
        if (cnnElemSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(cnnElemSetId);
            _combTPCnnElemSet->setCurrentText(setName);
        }
        else
        {
            _combTPCnnElemSet->setCurrentIndex(0);
            // 默认选择第一个时，手动添加映射
            QString setName = _combTPCnnElemSet->currentText();
            int setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(setName);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_TPContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ConelSet);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_TPContri, data->getId());
        }
        _combTPCnnElemSet->blockSignals(false);

        //硬链接单元集合
        _combTPHardLinkElemSet->blockSignals(true);
        _combTPHardLinkElemSet->clear();
        _combTPHardLinkElemSet->addItem("");
        _combTPHardLinkElemSet->addItems(elemNames);
        int hardLinkElemSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_TPContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ConrelSet);
        if (hardLinkElemSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(hardLinkElemSetId);
            _combTPHardLinkElemSet->setCurrentText(setName);
        }
        else
        {
            _combTPHardLinkElemSet->setCurrentIndex(0);
            // 默认选择空白行，不做映射
        }
        _combTPHardLinkElemSet->blockSignals(false);

        //被动侧节点集合
        _combTPPassiveNodeSet->blockSignals(true);
        _combTPPassiveNodeSet->clear();
        _combTPPassiveNodeSet->addItem("");
        _combTPPassiveNodeSet->addItems(nodeNames);
        int passiveNodeSetId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_TPContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ConvolSet);
        if (passiveNodeSetId != -1)
        {
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(passiveNodeSetId);
            _combTPPassiveNodeSet->setCurrentText(setName);
        }
        else
        {
            _combTPPassiveNodeSet->setCurrentIndex(0);
            // 默认选择空白行，不做映射
        }
        _combTPPassiveNodeSet->blockSignals(false);
    }
}
