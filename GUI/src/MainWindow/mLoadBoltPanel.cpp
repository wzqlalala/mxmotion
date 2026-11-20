#include "mMechGlobalSignals.h"
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

#include "mLoadBoltPanel.h"
#include "mDataStatusManager.h"
#include "mConnectorBase.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
using namespace MechConnectorData;
namespace MechMainWindow
{
    mLoadBoltPanel::mLoadBoltPanel(QObject* parent)
    {
        initInputs();
    }

    mLoadBoltPanel::~mLoadBoltPanel()
    {
        clearInputs();
    }

    void mLoadBoltPanel::initInputs()
    {
        QStringList combonames =
        {
            "螺栓",
            "方式",
        };

        for (int i = 0; i < 2; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }


        QStringList inpnames =
        {
           "值"
        };


        for (int i = 0; i < 1; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.labelname->setText(inpnames[i]);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveValue();
                });
        }

        //选择螺栓约束
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadBoltPanel::saveBolt);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(0);
        comboBoxs[0].combobox->blockSignals(false);


        //载荷名称
        comboBoxs[1].combobox->addItems({ "力预紧","位移预紧","螺栓锁止"});
        connect(comboBoxs[1].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadBoltPanel::slot_wayChanged);
        comboBoxs[1].combobox->blockSignals(true);
        comboBoxs[1].combobox->setCurrentIndex(type);
        comboBoxs[1].combobox->blockSignals(false);


        //函数
        combofunc.initWidget("函数");
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
            this, &mLoadBoltPanel::saveFunc);

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

        //缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mLoadBoltPanel::clearInputs()
    {
        inputWidgets.clear();
        comboBoxs.clear();
    }

    QVector<QWidget*> mLoadBoltPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    void mLoadBoltPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        //设置值
        std::shared_ptr<MechCaseData::mLoadBolt> boltPtr = std::dynamic_pointer_cast<MechCaseData::mLoadBolt>(loadData);

        QVariant padata1 = boltPtr->value_;

        QString pastr1 = "";
        if (!padata1.isNull() && padata1.isValid())
        {
            pastr1 = MxFunctions::mNumberToString(padata1.value<double>());
        }

        inputWidgets[0].lineeditvalue->setExpression(pastr1);

        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[1].combobox->blockSignals(true);

        //填充螺栓名称下拉框
        comboBoxs[0].combobox->clear();
        _bolttable.clear();

        //获取所有螺栓约束
        auto allConnectorBoltName = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getAllConnectorNamesByTypes(Connector_Bolt);
        //螺栓约束
        for (auto boltName : allConnectorBoltName)
        {
            //根据名称获取id
            int boltid = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getIDByConnectorName(boltName);
            // 跳过被抑制的螺栓约束
            auto boltData = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(boltid);
            if (boltData && boltData->isSuppressed())
                continue;
            //映射到哈希表中
            comboBoxs[0].combobox->addItem(boltName);
            _bolttable[boltName] = boltid;
        }

        auto& maploadbound = getCVM->getModelData()->getMapRelationManager();

        //设置载荷名称下拉框选取目标
        int loadboundid = maploadbound.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Connector);
        if (loadboundid != -1)
        {
            QString loadName = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorNameByID(loadboundid);
            comboBoxs[0].combobox->setCurrentText(loadName);
        }
        else
        {
            comboBoxs[0].combobox->setCurrentIndex(0);
            int connectorboltid = getConnectorBoltId();
            //添加载荷边界映射
            getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Connector,connectorboltid });
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
        }

        //设置方式下标
        type = boltPtr->way_;
        comboBoxs[1].combobox->setCurrentIndex(type);

        comboBoxs[0].combobox->blockSignals(false);
        comboBoxs[1].combobox->blockSignals(false);

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
        combofunc.combobox->blockSignals(false);

        //缩放因子
        double scale = maploadbound.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));
    }

    QVector<QWidget*> mLoadBoltPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[1].propwid);
        if (comboBoxs[1].combobox->currentIndex() == 2) //螺栓锁止没有值和函数
        {
            wds.append(comboBoxs[1].propwid);
        }
        else
        {
            wds.append(comboBoxs[1].propwid);
            wds.append(inputWidgets[0].propwid);
            wds.append(combofunc.propwid);
        }
        return wds;
    }

    QVector<QWidget*> mLoadBoltPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[0].propwid);
        return wds;
    }

    int mLoadBoltPanel::getConnectorBoltId()
    {
        int connectorboltid = -1;
        QString connectorboltname = comboBoxs[0].combobox->currentText();
        if (_bolttable.find(connectorboltname) != _bolttable.end())
        {
            connectorboltid = _bolttable[connectorboltname];
        }
        return connectorboltid;
    }

    int mLoadBoltPanel::getFuncId()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadBoltPanel::slot_selectBoltChanged(int idx)
    {
        saveBolt();
    }

    void mLoadBoltPanel::slot_wayChanged(int idx)
    {
        type = idx;
        if (!faval_->isVisible()) return;
        faval_->removeAll();
        faval_->addRow(comboBoxs[1].propwid);
        comboBoxs[1].propwid->show();

        if (type != 2)
        {
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }

        faval_->refreshLayout();
        saveWay();
    }

    void mLoadBoltPanel::slot_funcChanged(int idx)
    {
        saveFunc();
    }

    void mLoadBoltPanel::saveValue()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadBolt>(_loadData);
        if (!load)
            return;

        QString pastr = inputWidgets[0].lineeditvalue->text().trimmed();
        if (!pastr.isEmpty())
        {
            load->value_ = QVariant::fromValue<double>(pastr.toDouble());
        }
        else
        {
            load->value_ = QVariant();
        }
        notifyLoadChanged();
    }

    void mLoadBoltPanel::saveBolt()
    {
        if (!_loadData)
            return;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int connectorboltid = getConnectorBoltId();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Connector, connectorboltid });
        notifyLoadChanged();
    }

    void mLoadBoltPanel::saveWay()
    {
        if (!_loadData)
            return;
        auto load = std::dynamic_pointer_cast<MechCaseData::mLoadBolt>(_loadData);
        if (!load)
            return;
        load->way_ = type;
        notifyLoadChanged();
    }

    void mLoadBoltPanel::saveFunc()
    {
        if (!_loadData)
            return;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int funcid = getFuncId();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyLoadChanged();
    }

    void mLoadBoltPanel::saveScale()
    {
        if (!_loadData)
            return;
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadBoltPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }
}