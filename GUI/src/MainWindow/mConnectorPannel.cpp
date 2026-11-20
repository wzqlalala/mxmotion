#include "mConnectorPannel.h"
#include "mConnector.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDisplayMenuFunction.h"

#include "mMeshData.h"
//commond
#include "mCommondSystem.h"
#include "mCommonds.h"
#include <mConnectorDisperse.h>
#include "mConnectorRigidbody.h"

#include "mConnectorBolt.h"
#include "mConnectorFastener.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectorBearingUnits.h"
#include "AllRenderSymbolManager.h"

using namespace MechPreData;
using namespace MechFunctionData;
using namespace MechConnectorData;
namespace MechMainWindow
{
	mConnectorPannel::mConnectorPannel(QWidget* parent)
		: mPanelBase(parent)
	{
		_mainLayout = new QVBoxLayout(this);
		_mainLayout->addStretch();
	}

	mConnectorPannel::~mConnectorPannel()
	{}

    void mConnectorPannel::createConnector(MxFunctions::ConnectorType type)
    {
        //数据
        _connectorData = nullptr;
        setEditing(true);
        mConnectorManager& manager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
        switch (type)
        {
        case MxFunctions::Connector_Bind:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorBind>(manager.createID(), Connector_Bind);
        }
        break;
        case MxFunctions::Connector_Rigid:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorRigid>(manager.createID(), Connector_Rigid);
        }
        break;
        case MxFunctions::Connector_Flexible:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorFlexible>(manager.createID(), Connector_Flexible);
        }
        break;
        case MxFunctions::Connector_MPC:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorMPC>(manager.createID(), Connector_MPC);
        }
        break;
        case MxFunctions::Connector_Disperse:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorDisperse>(manager.createID(), Connector_Disperse);
        }
        break;
        case MxFunctions::Connector_Bolt:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorBolt>(manager.createID(), Connector_Bolt);
            //创建初始节点
            int freeNodeId = getCVM->createFreeNode(QVector3D(0, 0, 0));
            std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
            connectorPtr->node_ = freeNodeId;
        }
        break;
        case MxFunctions::Connector_DistributedCoupling:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorDistributedCoupling>(manager.createID(), Connector_DistributedCoupling);
        }
        break;
        case MxFunctions::Connector_BearingUnits:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorBearingUnits>(manager.createID(), Connector_BearingUnits);
        }
        break;
        case MxFunctions::Connector_Rigidbody:
        {
            _connectorData = std::make_shared<MechConnectorData::mConnectorRigidbody>(manager.createID(), Connector_Rigidbody);
        }
        break;
        default:
            return;
        }

        if (_connectorData == nullptr)
        {
            return;
        }

        auto name = getCVM->getModelData()->createNameByType(Physical_Connector, _connectorData->getConnectorTypeToStringEn());
        _connectorData->setConnectorName(name);
        getCVM->getModelData()->addPhysicalFieldP(_connectorData);


        QString Info = QString("成功创建连接 %1").arg(_connectorData->getConnectorName());
        mGlobalSignals::getInstance()->outputMessageSig(Info, true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateConnectorSig(true, _connectorData->getConnectorId());

        mCommondConnector* commond = new mCommondConnector(mCommondBase::CommondType::CONNECTOR_CREATEUNDO);
        commond->_connectorData = _connectorData;
        mCommondSystem::getInstance()->addCommond(commond);
    }

	void mConnectorPannel::editConnector(const int& connId)
	{
		_connectorData = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorData(connId);
		if (_connectorData == nullptr)
		{
			return;
		}
		setEditing(true);
		//数据反显
		MechMainWindow::mConnectorBase* connbase = mConnector::getInstance()->getConnectorByType(_connectorData->getConnectorType());
		connbase->setInputsValue(_connectorData);
		
		//调出对应控件
		connectorPanel(_connectorData->getConnectorType());
	}

	void mConnectorPannel::unEditConnector()
	{
		if (!isEditing())
		{
			return;
		}
		//入口统一复位编辑态：后续各早退分支（数据为空/已删除）无需再单独清标志
		setEditing(false);
		if (_connectorData == nullptr)
		{
			clearFoldWidgets();
			return;
		}
		//_connectorData 为共享指针，数据被删除（树上删除等）后引用仍非空，
		//需向管理器按 ID 复核；数据不在当前模型管理器（已删除或属于其他模型——
		//模型切换后当前管理器按 ID 查不到）时无法走数据提交收尾，
		//但 UI 折叠框仍必须卸载，否则残留折叠框会与后续进入编辑的面板叠加显示
		if (getCVM == nullptr)
		{
			_connectorData = nullptr;
			clearFoldWidgets();
			return;
		}
		auto& connManager = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector);
		if (connManager.getConnectorData(_connectorData->getConnectorId()) == nullptr)
		{
			_connectorData = nullptr;
			clearFoldWidgets();
			return;
		}
		MechMainWindow::mConnectorBase* connbase = mConnector::getInstance()->getConnectorByType(_connectorData->getConnectorType());
        if (connbase == nullptr)
        {
			clearFoldWidgets();
            return;
        }

		//使用新通用拾取控件，切出前统一失活提交，避免未提交数据丢失
        connbase->deactivatePickAndCommit();
		clearFoldWidgets();

        //切面板后清空红蓝高亮
        {
            if (!getCVM)
            {
                return;
            }
            if (!getCVM->getPreRenderWidget())
                return;
            auto widget_ptr = getCVM->getPreRenderWidget();
            auto highlight_controller = widget_ptr->getHighlightController();

            auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"));
            mGeoSelectData* geoData = new mGeoSelectData();
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);

            auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
            MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
            meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            meshHighLightRender->updateHighLightRender(meshData);

            geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), false);
            geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});
            geoHighLightRender->updateHighLightRender(geoData);

            meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"), false);
            meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
            meshHighLightRender->updateHighLightRender(meshData);
        }
    }

	void mConnectorPannel::rendFreeNodes()
	{
		if (_connectorData == nullptr)
		{
			return;
		}
		MechMainWindow::mConnectorBase* connbase = mConnector::getInstance()->getConnectorByType(_connectorData->getConnectorType());
		connbase->rendFreeNodes(true);
	}

	void mConnectorPannel::disRendFreeNodes()
	{
		if (_connectorData == nullptr)
		{
			return;
		}
		MechMainWindow::mConnectorBase* connbase = mConnector::getInstance()->getConnectorByType(_connectorData->getConnectorType());
		connbase->rendFreeNodes(false);
	}

    void mConnectorPannel::connectorPanel(MxFunctions::ConnectorType type)
    {
        auto connector = mConnector::getInstance()->getConnectorByType(type);
        if (connector == nullptr)
        {
            return;
        }
        connector->setConnData(_connectorData);

        // 连接定义折叠框
        QString typeName = getCVM->getModelData()->getManager<MechConnectorData::mConnectorManager>(Physical_Connector).getConnectorTypeNameByType(type);
        FoldSection* propFold0 = new FoldSection("连接定义", this);
        propFold0->addRow(new PanelLabel("连接类型："), new PanelLabel(typeName));
        m_foldWidgets.append(propFold0);

        switch (type)
        {
        case MxFunctions::Connector_Bind:
        {
            FoldSection* propFold1 = new FoldSection("接触对", this);
            FoldSection* propFold2 = new FoldSection("属性", this);
            mConnectorBind* connRigid = dynamic_cast<mConnectorBind*>(connector);
            QVector<QWidget*>wds1 = connRigid->getPosInputs(propFold1);
            QVector<QWidget*>wds2 = connRigid->getValInputs(propFold2);
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
        case MxFunctions::Connector_Rigid:
        {
            FoldSection* propFold1 = new FoldSection("位置", this);
            FoldSection* propFold2 = new FoldSection("自由度", this);
            mConnectorRigid* connRigid = dynamic_cast<mConnectorRigid*>(connector);
            QVector<QWidget*>wds1 = connRigid->getPosInputs(propFold1);
            QVector<QWidget*>wds2 = connRigid->getValInputs(propFold2);
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
        case MxFunctions::Connector_Flexible:
        {
            FoldSection* propFold1 = new FoldSection("位置", this);
            FoldSection* propFold2 = new FoldSection("主节点权重及自由度", this);
            FoldSection* propFold4 = new FoldSection("从节点自由度", this);

            mConnectorFlexible* connFlex = dynamic_cast<mConnectorFlexible*>(connector);
            QVector<QWidget*>wds1 = connFlex->getPosInputs(propFold1);
            QVector<QWidget*>wds2 = connFlex->getWightsAndValsInputs(propFold2);
            QVector<QWidget*>wds4 = connFlex->getSubValInputs(propFold4);

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
            for (int i = 0; i < wds4.size(); ++i)
            {
                propFold4->addRow(wds4[i]);
                wds4[i]->show();
            }
            m_foldWidgets.append(propFold1);
            m_foldWidgets.append(propFold2);
            m_foldWidgets.append(propFold4);

            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
                _mainLayout->addWidget(propFold0);
                _mainLayout->addWidget(propFold1);
                _mainLayout->addWidget(propFold2);
                _mainLayout->addWidget(propFold4);
                _mainLayout->addStretch();
            }
        }
        break;
        case MxFunctions::Connector_MPC:
        {
            FoldSection* propFold1 = new FoldSection("位置", this);
            FoldSection* propFold2 = new FoldSection("主节点自由度及权重", this);
            FoldSection* propFold3 = new FoldSection("从节点自由度及权重", this);
            FoldSection* propFold4 = new FoldSection("参数", this);
            mConnectorMPC* connMPC = dynamic_cast<mConnectorMPC*>(connector);
            QVector<QWidget*>wds1 = connMPC->getPosInputs(propFold1);
            QVector<QWidget*>wds2 = connMPC->getValInputs(propFold2);
            QVector<QWidget*>wds3 = connMPC->getValInputs2(propFold3);
            QVector<QWidget*>wds4 = connMPC->getValInputs3(propFold4);
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
            for (int i = 0; i < wds4.size(); ++i)
            {
                propFold4->addRow(wds4[i]);
                wds4[i]->show();
            }
            m_foldWidgets.append(propFold1);
            m_foldWidgets.append(propFold3);
            m_foldWidgets.append(propFold2);
            m_foldWidgets.append(propFold4);
            QLayoutItem* lastItem = _mainLayout->itemAt(_mainLayout->count() - 1);
            if (lastItem && lastItem->spacerItem())
            {
                _mainLayout->removeItem(lastItem);
                _mainLayout->addWidget(propFold0);
                _mainLayout->addWidget(propFold1);
                _mainLayout->addWidget(propFold3);
                _mainLayout->addWidget(propFold2);
                _mainLayout->addWidget(propFold4);
                _mainLayout->addStretch();
            }
        }
        break;
        case MxFunctions::Connector_Disperse:
        {
            FoldSection* propFold1 = new FoldSection("参考点", this);
            FoldSection* propFold2 = new FoldSection("刚体", this);
            mConnectingDisperse* connDeputy = dynamic_cast<mConnectingDisperse*>(connector);
            QVector<QWidget*>wds1 = connDeputy->getReferencePoint(propFold1);
            QVector<QWidget*>wds2 = connDeputy->getRigidBody(propFold2);
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
        case MxFunctions::Connector_Bolt:
        {
            FoldSection* propFold1 = new FoldSection("类型定义", this);
            FoldSection* propFold2 = new FoldSection("作用对象", this);
            FoldSection* propFold3 = new FoldSection("预紧方向", this);
            // 预紧方向折叠框：标题自带勾选框，未勾选时整个折叠框内容不生效（参考重力局部施加）
            propFold3->setCheckable(true);

            mConnectorBoltPanel* connDeputy = dynamic_cast<mConnectorBoltPanel*>(connector);
            QVector<QWidget*>wds1 = connDeputy->getTypeDefine(propFold1);
            QVector<QWidget*>wds2 = connDeputy->getTargetObject(propFold2);
            QVector<QWidget*>wds3 = connDeputy->getVectorDefine(propFold3);

            // 绑定折叠框勾选状态到数据类（isOpen_），控制预紧方向是否生效
            std::shared_ptr<MechConnectorData::mConnectorBolt> boltPtr =
                std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
            if (boltPtr)
            {
                propFold3->setChecked(boltPtr->isOpen_);
                propFold3->onCheckBoxToggled(boltPtr->isOpen_);
                connect(propFold3, &FoldWidget::enabledChanged, this, [=](bool checked) {
                    if (boltPtr) {
                        boltPtr->isOpen_ = checked;
                    }
                    });
            }

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
        case MxFunctions::Connector_DistributedCoupling:
        {
            FoldSection* propFold1 = new FoldSection("位置", this);
            FoldSection* propFold2 = new FoldSection("耦合点权重及自由度", this);

            mConnectorDistributedCouplingPanel* connDc = dynamic_cast<mConnectorDistributedCouplingPanel*>(connector);
            QVector<QWidget*> wds1 = connDc->getPosInputs(propFold1);
            QVector<QWidget*> wds2 = connDc->getWeightAndDofInputs(propFold2);

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
        case MxFunctions::Connector_BearingUnits:
        {
            FoldSection* propFold1 = new FoldSection("轴承单元定义", this);
            FoldSection* propFold2 = new FoldSection("轴承参数定义", this);

            mConnectorBearingUnits* connBearing = dynamic_cast<mConnectorBearingUnits*>(connector);
            QVector<QWidget*> wds1 = connBearing->getBearingUnits(propFold1);
            QVector<QWidget*> wds2 = connBearing->getBearingPara(propFold2);
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
        case MxFunctions::Connector_Rigidbody:
        {
            FoldSection* propFold1 = new FoldSection("参考点", this);
            FoldSection* propFold2 = new FoldSection("解析表面", this);
            auto connRigidbody = dynamic_cast<mConnectorRigidbodyPanel*>(connector);
            QVector<QWidget*>wds1 = connRigidbody->getReferencePoint(propFold1);
            QVector<QWidget*>wds2 = connRigidbody->getSurfaceAnalysis(propFold2);
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
        default:
            break;
        }
    }

    mMixSet* mConnectorPannel::createSet()
    {
        int setID = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).createID();
        mMixSet* eleset = new mMixSet(setID, SetType::Mix, "contain", true);
        getCVM->getModelData()->addPhysicalField<MechSetData::mSetBase>(eleset);
        return eleset;
    }

   //工具函数
    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>convertPickDataToMap(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickData)
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
}