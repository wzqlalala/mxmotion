#include "mModalContriPanel.h"

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
    mModalContriPanel::mModalContriPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        data = nullptr;
        /**********************************************模态贡献量**********************************************************/

        FoldSection* modalContriFold = new FoldSection("模态贡献量参数", this);

        _combOutPutType = new QComboBox();
        modalContriFold->addRow(new PanelLabel("输出类型"), _combOutPutType);
        _combOutPutType->addItems({ "structure","fluid" });
        connect(_combOutPutType, &QComboBox::currentTextChanged, [this](QString txt) {
            //todo
            data->_outPutType = txt;
            _combOutPutSet->blockSignals(true);
            //清空
            _combOutPutSet->clear();
            //空白行：峰值筛选开启时输出集合可不选
            _combOutPutSet->addItem("");
            //切换时，影响输出集合下拉框内容填充
            if (txt == "structure")
            {
                //获取所有自由度集合填充
                QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::DOFNode);
                _combOutPutSet->addItems(names);
            }
            else if (txt == "fluid")
            {
                //获取所有节点集合填充
                QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
                _combOutPutSet->addItems(names);
            }
            _combOutPutSet->blockSignals(false);
            });

        _combSortType = new QComboBox();
        modalContriFold->addRow(new PanelLabel("排序方式"), _combSortType);
        _combSortType->addItems({ "mag","percentage" });
        connect(_combSortType, &QComboBox::currentTextChanged, [this](QString txt) {
            //todo
            data->_SortType = txt;
            });

        _combOutPutSet = new QComboBox();
        modalContriFold->addRow(new PanelLabel("输出集合"), _combOutPutSet);
        connect(_combOutPutSet, &QComboBox::currentTextChanged, [this](QString txt) {
            //根据名称获取集合id
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_ModalContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_OutSet);

            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModalContri, data->getId());
            });

        _combOutPutFreqSet = new QComboBox();
        modalContriFold->addRow(new PanelLabel("频率点集合"), _combOutPutFreqSet);
        _combOutPutFreqSet->addItem("");
        connect(_combOutPutFreqSet, &QComboBox::currentTextChanged, [this](QString txt) {
            int setId = -1;
            if (!txt.isEmpty())
            {
                setId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(txt);
            }
            //映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_ModalContri ,data->getId() }, { Physical_Set,setId }, MxFunctions::MapRelation_FrequencySet);
            });

        _combResultType = new QComboBox();
        modalContriFold->addRow(new PanelLabel("结果类型"), _combResultType);
        _combResultType->addItems({ "disp","vel","acc" });
        connect(_combResultType, &QComboBox::currentTextChanged, [this](QString txt) {
            //todo
            data->_ResultType = txt;
            });

        _fltScoreInput.initWidget();
        _fltScoreInput.checkbox->setText("过滤分数");
        _fltScoreInput.lineeditvalue->setExpression("0.001");
        modalContriFold->addRow(_fltScoreInput.propwid);
        connect(_fltScoreInput.checkbox, &QCheckBox::stateChanged, this, [this](int state) {
            bool checked = (state == Qt::Checked);
            data->_fltScoreChecked = checked;
            _fltScoreInput.lineeditvalue->setEnabled(checked);
            });
        connect(_fltScoreInput.lineeditvalue, &QLineEdit::editingFinished, this, [this]() {
            data->_fltScore = _fltScoreInput.lineeditvalue->text();
            });

        _OutOrdInput.initWidget();
        _OutOrdInput.checkbox->setText("输出阶数");
        _OutOrdInput.lineeditvalue->setExpression("3");
        modalContriFold->addRow(_OutOrdInput.propwid);
        connect(_OutOrdInput.checkbox, &QCheckBox::stateChanged, this, [this](int state) {
            bool checked = (state == Qt::Checked);
            data->_OutOrdChecked = checked;
            _OutOrdInput.lineeditvalue->setEnabled(checked);
            });
        connect(_OutOrdInput.lineeditvalue, &QLineEdit::editingFinished, this, [this]() {
            data->_OutOrd = _OutOrdInput.lineeditvalue->text();
            });

        _checkBoxPkScr = new QCheckBox();
        modalContriFold->addRow(new PanelLabel("峰值筛选"), _checkBoxPkScr);
        connect(_checkBoxPkScr, &QCheckBox::stateChanged, this, [=](int state) {
            //峰值筛选可与输出集合共存，不再禁用输出集合下拉框
            data->_checkBoxPkScr = (state == Qt::Checked);
            });

        m_foldWidgets.append(modalContriFold);
        _mainLayout->addWidget(modalContriFold);
        _mainLayout->addStretch();
    }
    mModalContriPanel::~mModalContriPanel()
    {
    }

    void mModalContriPanel::createModalContri(int id)
    {
        if (!getCVM) return;

        if (id == 0) //id等于0，从模型树创建
        {
            auto globalParam = getCVM->getModelData()->getGlobalParam();
            if (globalParam->getModalContriData())
            {
                mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个模态贡献量！"), true);
                return;
            }
            data = globalParam->createModalContriData();
        }
        else//从工况创建
        {
            //根据id获取工况数据
            mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(id);
            if (caseBase == nullptr) return;
            //转为频响分析或者声固耦合
            if (caseBase->getCaseType() == ImplicitFrequency)
            {
                mCaseImplicitFrequency* analysis = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
                if (analysis == nullptr) return;

                if (analysis->getModalContriData())
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个模态贡献量！"), true);
                    return;
                }
                data = analysis->createModalContriData();
            }
            else if (caseBase->getCaseType() == ASCoupling)
            {
                mCaseImplASCoupling* analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                if (analysis->getModalContriData())
                {
                    mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个模态贡献量！"), true);
                    return;
                }
                data = analysis->createModalContriData();
            }

        }

        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建模态贡献量"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateContriSig(true, id, "模态贡献量");
    }

    void mModalContriPanel::editModalContri(int contriID)
    {
        //获取正确的data
        if (contriID == 0) //0为模型树上的
        {
            if (!getCVM) return;
            data = getCVM->getModelData()->getGlobalParam()->getModalContriData();
            if (data == nullptr) return;

        }
        else //根据工况id获取工况，再根据工况获取data
        {
            //根据id获取工况数据
            mCaseBase* caseBase = getCVM->getModelData()->getManager<MechCaseData::mCaseManager>(Physical_Case).getCaseData(contriID);
            if (caseBase == nullptr) return;
            //转为频响分析或者声固耦合
            if (caseBase->getCaseType() == ImplicitFrequency)
            {
                mCaseImplicitFrequency* analysis = dynamic_cast<mCaseImplicitFrequency*>(caseBase);
                if (analysis == nullptr) return;
                data = analysis->getModalContriData();

            }
            else if (caseBase->getCaseType() == ASCoupling)
            {
                mCaseImplASCoupling* analysis = dynamic_cast<mCaseImplASCoupling*>(caseBase);
                if (analysis == nullptr) return;
                data = analysis->getModalContriData();
            }
        }

        //填充面板
        //输出类型下拉框 structure，fluid
        _combOutPutType->blockSignals(true);
        _combOutPutType->setCurrentText(data->_outPutType);
        _combOutPutType->blockSignals(false);

        //排序方式下拉框 mag，percentage
        _combSortType->blockSignals(true);
        _combSortType->setCurrentText(data->_SortType);
        _combSortType->blockSignals(false);

        //输出集合
        _combOutPutSet->blockSignals(true);
        _combOutPutSet->clear();
        //空白行：峰值筛选开启时输出集合可不选
        _combOutPutSet->addItem("");
        //判断当前是结构还是声腔
        if (_combOutPutType->currentText() == "structure")
        {
            //获取所有自由度集合填充
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::DOFNode);
            _combOutPutSet->addItems(names);
        }
        else if (_combOutPutType->currentText() == "fluid")
        {
            //获取所有节点集合填充
            QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::Node);
            _combOutPutSet->addItems(names);
        }
        //获取映射设置当前下拉框内容
        int setId = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_ModalContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_OutSet);
        if (setId != -1)
        {
            //根据集合id获取集合名称
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId);
            //设置下拉框内容
            _combOutPutSet->setCurrentText(setName);
        }
        else if (!data->_checkBoxPkScr)
        {
            // 未开启峰值筛选时输出集合为必选，默认选第一个集合并手动添加映射
            _combOutPutSet->setCurrentIndex(1);
            QString setName = _combOutPutSet->currentText();
            int firstSetId = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getIDBySetName(setName);
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_ModalContri ,data->getId() }, { Physical_Set,firstSetId }, MxFunctions::MapRelation_OutSet);
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_ModalContri, data->getId());
        }
        else
        {
            // 开启峰值筛选时输出集合可不选，默认选空白行，不做映射
            _combOutPutSet->setCurrentIndex(0);
        }
        _combOutPutSet->blockSignals(false);

        //输出频率点集合
        _combOutPutFreqSet->blockSignals(true);
        _combOutPutFreqSet->clear();
        //获取所有频率点集合填充
        QStringList names = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getAllSetNamesByTypes(MxFunctions::SetType::FrequencyPoint);
        _combOutPutFreqSet->addItem("");
        _combOutPutFreqSet->addItems(names);
        //获取映射设置当前下拉框内容
        int setId2 = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_ModalContri ,data->getId() }, Physical_Set, MxFunctions::MapRelation_FrequencySet);
        if (setId2 != -1)
        {
            //根据集合id获取集合名称
            QString setName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(setId2);
            //设置下拉框内容
            _combOutPutFreqSet->setCurrentText(setName);
        }
        else
        {
            _combOutPutFreqSet->setCurrentIndex(0);
            // 默认选择空白行，不做映射
        }
        _combOutPutFreqSet->blockSignals(false);

        //贡献量结果类型
        _combResultType->blockSignals(true);
        _combResultType->setCurrentText(data->_ResultType);
        _combResultType->blockSignals(false);

        //过滤分数
        _fltScoreInput.checkbox->setChecked(data->_fltScoreChecked);
        _fltScoreInput.lineeditvalue->setExpression(data->_fltScore);
        _fltScoreInput.lineeditvalue->setEnabled(data->_fltScoreChecked);

        //输出阶数
        _OutOrdInput.checkbox->setChecked(data->_OutOrdChecked);
        _OutOrdInput.lineeditvalue->setExpression(data->_OutOrd);
        _OutOrdInput.lineeditvalue->setEnabled(data->_OutOrdChecked);
        //峰值筛选
        _checkBoxPkScr->setChecked(data->_checkBoxPkScr);
    }
}
