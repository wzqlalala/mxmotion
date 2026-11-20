#include "mDeputyPannel.h"
#include "mDeputy.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDisplayMenuFunction.h"
#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"

#include "mMeshData.h"
//commond
#include "mCommondSystem.h"
#include "mCommonds.h"
#include "AllRenderSymbolManager.h"

using namespace MechPreData;
using namespace MechFunctionData;
using namespace MechConnectorData;
namespace MechMainWindow
{
    mDeputyPannel::mDeputyPannel(QWidget* parent)
        : mPanelBase(parent)
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->addStretch();
    }

    mDeputyPannel::~mDeputyPannel()
    {}

    void mDeputyPannel::createDeputy(MxFunctions::DeputyType type)
    {
        _deputyData = nullptr;
        setEditing(true);
        //数据
        mDeputyManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
        switch (type)
        {
        case MxFunctions::Deputy_Connector:
        {
            _deputyData = std::make_shared<MechConnectorData::mConnectingDeputy>(manager.createID(), Deputy_Connector);
        }
        break;
        case MxFunctions::Deputy_Fastener:
        {
            _deputyData = std::make_shared<MechConnectorData::mConnectorFastener>(manager.createID(), Deputy_Fastener);
        }
        break;
        default:
            return;
        }

        if (_deputyData == nullptr)
        {
            return;
        }
        auto name = getCVM->getModelData()->createNameByType(Physical_Deputy, _deputyData->getDeputyTypeToStringEn());
        _deputyData->setDeputyName(name);

        getCVM->getModelData()->addPhysicalFieldP(_deputyData);

        QString Info = QString("成功创建连接副 %1").arg(_deputyData->getDeputyName());
        mGlobalSignals::getInstance()->outputMessageSig(Info, true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorDeputySig(true, _deputyData->getDeputyId());
    }

    void mDeputyPannel::editDeputy(const int& depId)
    {
        _deputyData = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyData(depId);
        if (_deputyData == nullptr)
        {
            return;
        }
        setEditing(true);

        // 先调出对应控件（确保 FoldSection 等 UI 已创建并赋给 panel 的成员）
        deputyPanel(_deputyData->getDeputyType());

        // 数据反显（setInputsValue 可能触发槽函数访问 fastenerDefine_ 等指针）
        MechMainWindow::mDeputyPanelBase* panel = mDeputyPanelManager::getInstance()->getPanelByType(_deputyData->getDeputyType());
        if (panel)
        {
            panel->setInputsValue(_deputyData);
        }

        // 重新渲染已有的连接副数据
        if (_deputyData)
            AllRenderSymbolManager::getInstance()->rendDeputy(_deputyData, true);

        connectSig(panel);
    }

    void mDeputyPannel::unEditDeputy()
    {
        if (!isEditing())
        {
            return;
        }
        //入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
        setEditing(false);
        //数据不在当前模型管理器（已删除或属于其他模型——模型切换后当前管理器
        //按 ID 查不到）时无法走数据提交收尾，但 UI 折叠框仍必须卸载，
        //否则残留折叠框会与后续进入编辑的面板叠加显示
        if (getCVM == nullptr)
        {
            _deputyData = nullptr;
            clearFoldWidgets();
            return;
        }
        if (_deputyData != nullptr)
        {
            auto& deputyManager = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
            if (deputyManager.getDeputyData(_deputyData->getDeputyId()) == nullptr)
            {
                _deputyData = nullptr;
                clearFoldWidgets();
                return;
            }
        }
        // 清除连接器面板的临时预览点
        if (getCVM->getPreRenderWidget())
        {
            auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
            if (render)
            {
                render->deletePoint("deputy_ref_preview");
                render->deletePoint("deputy_mov_preview");
            }
            getCVM->getPreRenderWidget()->updatePaint();
        }

        if (_deputyData != nullptr)
        {
            MechMainWindow::mDeputyPanelBase* panel = mDeputyPanelManager::getInstance()->getPanelByType(_deputyData->getDeputyType());
            if (panel == nullptr)
            {
                clearFoldWidgets();
                return;
            }
            disconnectSig(panel);
        }
        clearFoldWidgets();
    }

    void mDeputyPannel::deputyPanel(MxFunctions::DeputyType type)
    {
        auto panel = mDeputyPanelManager::getInstance()->getPanelByType(type);
        if (panel == nullptr)
        {
            return;
        }
        panel->setDeputyData(_deputyData);

        // 连接副定义折叠框
        QString typeName = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy).getDeputyTypeNameByType(type);
        FoldSection* propFold0 = new FoldSection("连接副定义", this);
        propFold0->addRow(new PanelLabel("连接副类型："), new PanelLabel(typeName));
        m_foldWidgets.append(propFold0);

        switch (type)
        {
        case MxFunctions::Deputy_Connector:
        {
            FoldSection* propFold1 = new FoldSection("定义", this);
            FoldSection* propFold2 = new FoldSection("参数", this);
            mDeputyConnectorPanel* connDeputy = dynamic_cast<mDeputyConnectorPanel*>(panel);
            QVector<QWidget*>wds1 = connDeputy->getPosInputs(propFold1);
            QVector<QWidget*>wds2 = connDeputy->getValInputs(propFold2);
            for (int i = 0; i < wds1.size(); ++i)
            {
                propFold1->addRow(wds1[i]);
                wds1[i]->show();
            }
            for (int i = 0; i < wds2.size(); ++i)
            {
                propFold2->addRow(wds2[i]);
                wds2[i]->show();
            }
            m_foldWidgets.append(propFold1);
            m_foldWidgets.append(propFold2);
            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
                _mainLayout->addWidget(propFold0);
                _mainLayout->addWidget(propFold1);
                _mainLayout->addWidget(propFold2);
                _mainLayout->addStretch();
            }
        }
        break;
        case MxFunctions::Deputy_Fastener:
        {
            FoldSection* propFold1 = new FoldSection("焊接点定义", this);
            FoldSection* propFold2 = new FoldSection("连接部件", this);
            FoldSection* propFold3 = new FoldSection("属性", this);

            mDeputyFastenerPanel* connFastener = dynamic_cast<mDeputyFastenerPanel*>(panel);
            QVector<QWidget*>wds1 = connFastener->getFastenerDefine(propFold1);
            QVector<QWidget*>wds2 = connFastener->getConnectParts(propFold2);
            QVector<QWidget*>wds3 = connFastener->getProperties(propFold3);

            for (int i = 0; i < wds1.size(); ++i)
            {
                propFold1->addRow(wds1[i]);
                wds1[i]->show();
            }
            for (int i = 0; i < wds2.size(); ++i)
            {
                propFold2->addRow(wds2[i]);
                wds2[i]->show();
            }
            for (int i = 0; i < wds3.size(); ++i)
            {
                propFold3->addRow(wds3[i]);
                wds3[i]->show();
            }
            m_foldWidgets.append(propFold1);
            m_foldWidgets.append(propFold2);
            m_foldWidgets.append(propFold3);

            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
                _mainLayout->addWidget(propFold0);
                _mainLayout->addWidget(propFold1);
                _mainLayout->addWidget(propFold2);
                _mainLayout->addWidget(propFold3);
                _mainLayout->addStretch();
            }
        }
        break;
        default:
            break;
        }
    }

    void mDeputyPannel::connectSig(mDeputyPanelBase* panel)
    {
        connect(panel, &mDeputyPanelBase::sg_editFinished, this, &mDeputyPannel::slot_finishDeputyEdit);
        connect(panel, &mDeputyPanelBase::sg_pickFinished, this, &mDeputyPannel::slot_finishDeputyPick);
        panel->connectPickSig();
    }

    void mDeputyPannel::disconnectSig(mDeputyPanelBase* panel)
    {
        disconnect(panel, &mDeputyPanelBase::sg_editFinished, this, &mDeputyPannel::slot_finishDeputyEdit);
        disconnect(panel, &mDeputyPanelBase::sg_pickFinished, this, &mDeputyPannel::slot_finishDeputyPick);
        panel->setConnectTbaleSig(false);
        panel->disconnectPickSig();
    }

    mMixSet* mDeputyPannel::createSet()
    {
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* eleset = new mMixSet(setID, SetType::Mix, "contain", true);
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
        return eleset;
    }

    void mDeputyPannel::slot_finishDeputyEdit()
    {
        if (_deputyData == nullptr)
        {
            return;
        }

        switch (_deputyData->getDeputyType())
        {
        case MxFunctions::Deputy_Connector:
        {
            auto panel = dynamic_cast<mDeputyConnectorPanel*>(mDeputyPanelManager::getInstance()->getPanelByType(_deputyData->getDeputyType()));
            panel->storeDeputyValue(_deputyData);
        }
        break;
        case MxFunctions::Deputy_Fastener:
        {
            auto panel = dynamic_cast<mDeputyFastenerPanel*>(mDeputyPanelManager::getInstance()->getPanelByType(_deputyData->getDeputyType()));
            panel->storeDeputyValue(_deputyData);
        }
        break;
        default:
            break;
        }

        auto render = AllRenderSymbolManager::getInstance();
        render->rendDeputy(_deputyData, true);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorDeputySig(false, _deputyData->getDeputyId());
    }

    //工具函数
    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mDeputyPannel::convertPickDataToMap(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickData)
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> resultMap;
        for (const auto& pair : pickData)
        {
            MxFunctions::PickDataType pickType = pair.first;
            const RenderPickResultData& data = pair.second;
            resultMap[pickType].push_back(data);
        }
        return resultMap;
    }

    void mDeputyPannel::slot_finishDeputyPick()
    {
        if (_deputyData == nullptr)
        {
            return;
        }
        //获取映射
        auto& mapload = getCVM->getModelData()->getMapRelationManager();

        //更新集合
        mMixSet* set = createSet();
        auto panel = mDeputyPanelManager::getInstance()->getPanelByType(_deputyData->getDeputyType());
        auto setdata = panel->getPickedData();
        unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data = convertPickDataToMap(setdata);
        set->setMixSetData(data);

        //更新映射
        mapload.appendMapRelation({ Physical_Deputy ,_deputyData->getDeputyId() },
            { Physical_Set,(setdata.empty()) ? -1 : set->getSetId() });

        //渲染
        auto render = AllRenderSymbolManager::getInstance();
        render->rendDeputy(_deputyData, true);

        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorDeputySig(false, _deputyData->getDeputyId());

    }

}
