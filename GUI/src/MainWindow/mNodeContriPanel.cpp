#include "mNodeContriPanel.h"

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
#include "mCaseImplicitFrequency.h"
#include "mCaseImplASCoupling.h"

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
    mNodeContriPanel::mNodeContriPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        data = nullptr;
        /**********************************************节点贡献量**********************************************************/

        FoldSection* nodeContriFold = new FoldSection("节点贡献量参数", this);

        _combNodeRespSet = new QComboBox();
        nodeContriFold->addRow(new PanelLabel("响应点集合"), _combNodeRespSet);
        connect(_combNodeRespSet, &QComboBox::currentTextChanged, [this](QString txt) {
            //根据名称获取集合id
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_NodeContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_ResponseGridSet);

            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_NodeContri, data->getId());
            });

        _combStructNodeSet = new QComboBox();
        nodeContriFold->addRow(new PanelLabel("结构节点"), _combStructNodeSet);
        _combStructNodeSet->addItem("");
        connect(_combStructNodeSet, &QComboBox::currentTextChanged, [this](QString txt) {
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_NodeContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_GridSet);
            });

        _combCavityNodeSet = new QComboBox();
        nodeContriFold->addRow(new PanelLabel("声腔节点"), _combCavityNodeSet);
        _combCavityNodeSet->addItem("");
        connect(_combCavityNodeSet, &QComboBox::currentTextChanged, [this](QString txt) {
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_NodeContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_GridfSet);
            });

        _combNodeFreqSet = new QComboBox();
        nodeContriFold->addRow(new PanelLabel("频率点集合"), _combNodeFreqSet);
        _combNodeFreqSet->addItem("");
        connect(_combNodeFreqSet, &QComboBox::currentTextChanged, [this](QString txt) {
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_NodeContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_FrequencySet);
            });

        _truncTypeCombo = new QComboBox();
        _truncTypeCombo->addItem("帕斯卡");
        _truncTypeCombo->addItem("分贝计权");
        nodeContriFold->addRow(new PanelLabel("响应截断类型"), _truncTypeCombo);
        connect(_truncTypeCombo, &QComboBox::currentTextChanged, this, [this](QString text) {
            if (data)
            {
                data->_rpTruncType = text;
                data->_rpTruncValue = "";
            }
            _truncValueLineEdit->setExpression("");
            });

        _truncValueLineEdit = std::make_shared<mCalcLineEdit>("", this, false, false);
        nodeContriFold->addRow(new PanelLabel("响应截断值"), _truncValueLineEdit.get());
        connect(_truncValueLineEdit.get(), &QLineEdit::editingFinished, this, [this]() {
            if (!data) return;
            data->_rpTruncType = _truncTypeCombo->currentText();
            data->_rpTruncValue = _truncValueLineEdit->text();
            });

        _checkBoxCavityProj = new QCheckBox();
        nodeContriFold->addRow(new QLabel("声腔网格投影值输出"), _checkBoxCavityProj);
        connect(_checkBoxCavityProj, &QCheckBox::stateChanged, this, [=](int state) {
            bool isChecked = (state == Qt::Checked);

            data->_checkBoxCavityProj = isChecked;

            if (isChecked) {
                // 勾选
            }
            else {
                // 取消
            }
            });

        _checkBoxNodePkScr = new QCheckBox();
        nodeContriFold->addRow(new PanelLabel("峰值筛选"), _checkBoxNodePkScr);
        connect(_checkBoxNodePkScr, &QCheckBox::stateChanged, this, [=](int state)
            {
            //峰值筛选可与响应点集合共存，不再禁用响应点集合下拉框
            data->_checkBoxPkScr = (state == Qt::Checked);
            });

        m_foldWidgets.append(nodeContriFold);
        _mainLayout->addWidget(nodeContriFold);
        _mainLayout->addStretch();
    }

    mNodeContriPanel::~mNodeContriPanel()
    {
    }

    void mNodeContriPanel::createNodeContri(int id)
    {
        if (!getCVM) return;

        if (id == 0) //id等于0，从模型树创建
        {
            auto globalParam = getCVM->getModelData()->getGlobalParam();
            if (globalParam->getNodeContriData())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个节点贡献量！"), true);
                return;
            }
            data = globalParam->createNodeContriData();
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
                if (analysis->getNodeContriData())
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个节点贡献量！"), true);
                    return;
                }
                data = analysis->createNodeContriData();
            }

        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建节点贡献量"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContriSig(true, id, "节点贡献量");
    }

    void mNodeContriPanel::editNodeContri(int contriID)
    {
        //获取正确的data
        if (contriID == 0) //0为模型树上的
        {
            if (!getCVM) return;
            data = getCVM->getModelData()->getGlobalParam()->getNodeContriData();
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
                data = analysis->getNodeContriData();
            }
        }

        //填充面板
        //响应点集合
        _combNodeRespSet->blockSignals(true);
        _combNodeRespSet->clear();
        //获取所有节点集合填充
        {
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
            _combNodeRespSet->addItem("");
            _combNodeRespSet->addItems(names);
        }
        //获取映射设置当前下拉框内容
        {
            int setId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_ResponseGridSet);
            if (setId != -1)
            {
                QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId);
                _combNodeRespSet->setCurrentText(setName);
            }
            else if (!data->_checkBoxPkScr)
            {
                // 未开启峰值筛选时响应点集合为必选，默认选第一个集合并手动添加映射
                _combNodeRespSet->setCurrentIndex(1);
                QString setName = _combNodeRespSet->currentText();
                int firstSetId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(setName);
                getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_NodeContri ,data->getId() }, { Physical_Set,firstSetId }, MxFunctions::MapRelation_ResponseGridSet);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_NodeContri, data->getId());
            }
            else
            {
                // 开启峰值筛选时响应点集合可不选，默认选空白行，不做映射
                _combNodeRespSet->setCurrentIndex(0);
            }
        }
        _combNodeRespSet->blockSignals(false);

        //结构节点输出集合
        _combStructNodeSet->blockSignals(true);
        _combStructNodeSet->clear();
        //获取所有节点集合填充
        {
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
            _combStructNodeSet->addItem("");
            _combStructNodeSet->addItems(names);
        }
        //获取映射设置当前下拉框内容
        {
            int setId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_GridSet);
            if (setId != -1)
            {
                QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId);
                _combStructNodeSet->setCurrentText(setName);
            }
            else
            {
                _combStructNodeSet->setCurrentIndex(0);
                // 默认选择空白行，不做映射
            }
        }
        _combStructNodeSet->blockSignals(false);

        //声腔节点输出集合
        _combCavityNodeSet->blockSignals(true);
        _combCavityNodeSet->clear();
        //获取所有节点集合填充
        {
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
            _combCavityNodeSet->addItem("");
            _combCavityNodeSet->addItems(names);
        }
        //获取映射设置当前下拉框内容
        {
            int setId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_GridfSet);
            if (setId != -1)
            {
                QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId);
                _combCavityNodeSet->setCurrentText(setName);
            }
            else
            {
                _combCavityNodeSet->setCurrentIndex(0);
                // 默认选择空白行，不做映射
            }
        }
        _combCavityNodeSet->blockSignals(false);

        //输出频率点
        _combNodeFreqSet->blockSignals(true);
        _combNodeFreqSet->clear();
        //获取所有频率点集合填充
        {
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::FrequencyPoint);
            _combNodeFreqSet->addItem("");
            _combNodeFreqSet->addItems(names);
        }
        //获取映射设置当前下拉框内容
        {
            int setId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_NodeContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_FrequencySet);
            if (setId != -1)
            {
                QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId);
                _combNodeFreqSet->setCurrentText(setName);
            }
            else
            {
                _combNodeFreqSet->setCurrentIndex(0);
                // 默认选择空白行，不做映射
            }
        }
        _combNodeFreqSet->blockSignals(false);

        //响应截断值
        _truncTypeCombo->blockSignals(true);
        int typeIndex = _truncTypeCombo->findText(data->_rpTruncType);
        _truncTypeCombo->setCurrentIndex(typeIndex >= 0 ? typeIndex : 0);
        _truncValueLineEdit->setExpression(data->_rpTruncValue);
        _truncTypeCombo->blockSignals(false);

        //声腔网格投影值输出
        _checkBoxCavityProj->setChecked(data->_checkBoxCavityProj);

        //峰值筛选
        _checkBoxNodePkScr->setChecked(data->_checkBoxPkScr);
    }
}
