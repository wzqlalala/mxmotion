#include "mConnector.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "mDockLayoutManager.h"
#include "mDisplayMenuFunction.h"
#include "AllRenderSymbolManager.h"
#include "PickCommonFunction.h"
#include "mDataStatusManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "IPickPersistenceStrategy.h"
//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//MechModelData
#include "mSetBase.h"
#include "mMixSet.h"
#include "mMeshData.h"
#include "mDataElement.h"
#include "mSetManager.h"


#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"
#include "mConnectorMPC.h"
#include "mConnectorBind.h"
#include "mConnectorDistributedCoupling.h"
#include "mConnectorBearingUnits.h"
#include "mConnectorRigidbody.h"

#include "mConnectingDeputy.h"
#include "mConnectorDisperse.h"
#include "mConnectorBolt.h"
#include "mConnectorFastener.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include "mGeoSet.h"
#include "mElementSurface.h"
#include <qmessagebox.h>
#include <mNode.h>
#include <QStringListModel>
#include <QCompleter>
#include "mOtherFunction.h"
#include "mDirectionSelectorWidget.h"

//#include <TopoDS_Edge.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//include <TopExp.hxx> //【OCC摘除·S10】毒头（拉 OCC 链/冻结岛/出列件）死引用清剿
//#include <TopoDS_Vertex.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <BRep_Tool.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <gp_Pnt.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役

//#include <BRepGProp.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <GProp_GProps.hxx> //【OCC摘除·S10/P0-1】OCC 头随旧栈冻结岛退役
//#include <TopoDS_Face.hxx> //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

using namespace MechCommon;
using namespace MechPreData;
using namespace MechElementData;
using namespace mxr;
using namespace MechSetData;
using namespace MxFunctions;
namespace MechMainWindow
{
    /*************************************************连接类***************************************************************/
    mConnector::mConnector(QObject* parent)
        : QObject(parent)
    {
        m_ConnectorRigid = new mConnectorRigid();
        m_ConnectorFlexible = new mConnectorFlexible();
        m_ConnectorMPC = new mConnectorMPC();
        m_ConnectorBind = new mConnectorBind();
        m_ConnectingDisperse = new mConnectingDisperse();
        m_ConnectorBolt = new mConnectorBoltPanel();
        m_ConnectorDistributedCoupling = new mConnectorDistributedCouplingPanel();
        m_ConnectorBearingUnits = new mConnectorBearingUnits();
        m_ConnectorRigidbodyPanel = new mConnectorRigidbodyPanel();
    }

    mConnector::~mConnector()
    {
        delete m_ConnectorRigid;
        delete m_ConnectorFlexible;
        delete m_ConnectorMPC;
        delete m_ConnectorBind;
        delete m_ConnectingDisperse;
        delete m_ConnectorBolt;
        delete m_ConnectorDistributedCoupling;
        delete m_ConnectorBearingUnits;
        delete m_ConnectorRigidbodyPanel;
    }

    mConnector* mConnector::_instance = nullptr;

    mConnector* mConnector::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mConnector;
        }
        return _instance;
    }

    mConnectorBase* mConnector::getConnectorByType(MxFunctions::ConnectorType type)
    {
        m_ConnectorBase = nullptr;
        switch (type)
        {
        case MxFunctions::Connector_Bind:
            m_ConnectorBase = m_ConnectorBind;
            break;
        case MxFunctions::Connector_Rigid:
            m_ConnectorBase = m_ConnectorRigid;
            break;
        case MxFunctions::Connector_Flexible:
            m_ConnectorBase = m_ConnectorFlexible;
            break;
        case MxFunctions::Connector_MPC:
            m_ConnectorBase = m_ConnectorMPC;
            break;
        case MxFunctions::Connector_Disperse://离散刚体
            m_ConnectorBase = m_ConnectingDisperse;
            break;
        case MxFunctions::Connector_Bolt://螺栓约束
            m_ConnectorBase = m_ConnectorBolt;
            break;
        case MxFunctions::Connector_DistributedCoupling://分布式耦合
            m_ConnectorBase = m_ConnectorDistributedCoupling;
            break;
        case MxFunctions::Connector_BearingUnits://轴承单元
            m_ConnectorBase = m_ConnectorBearingUnits;
            break;
        case MxFunctions::Connector_Rigidbody:
            m_ConnectorBase = m_ConnectorRigidbodyPanel;
            break;
        default:
            break;
        }
        return m_ConnectorBase;
    }

    /*************************************************连接基类***************************************************************/
    mConnectorBase::mConnectorBase(QWidget* parent)
        : mPanelBase(parent)
    {}

    mConnectorBase::~mConnectorBase()
    {}

    bool mConnectorBase::getPickVecData(QVector3D& vec)
    {
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
            double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;

            vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
            };
        auto lines = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
        if (lines.size() == 1)
        {
            auto res = geoManager->CalculateEdgeAttr(lines);
            vec = res.at(0).m_dir;
            return true;
        }
        auto faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
        if (faces.size() == 1)
        {
            auto res = geoManager->CalculateFaceAttr(faces);
            vec = res.at(0).m_dir;
            return true;
        }

        return false;
    }

    bool mConnectorBase::getPickPointData(QVector3D& point)
    {
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto meshData = getCVM->getModelData()->getMeshData();
        auto getNodeVertexByID = [&](mMeshData* meshData, int id, QVector3D& vertex3D) {
            double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[id] * 3;

            vertex3D = QVector3D(vertex[0], vertex[1], vertex[2]);
            };
        auto nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
        if (nodes.size() == 1)
        {
            getNodeVertexByID(meshData, nodes.at(0).data_id.node_id, point);
            return true;
        }
        auto points = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPoint);
        if (points.size() == 1)
        {
            auto res = geoManager->CalculateVertexAttr(points);
            point = res.at(0).m_coor;
            return true;
        }
        auto lines = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
        if (lines.size() == 1)
        {
            auto res = geoManager->CalculateEdgeAttr(lines);
            point = res.at(0).m_coor;
            return true;
        }
        auto faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
        if (faces.size() == 1)
        {
            auto res = geoManager->CalculateFaceAttr(faces);
            point = res.at(0).m_coor;
            return true;
        }
        auto solids = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoSolid);
        if (solids.size() == 1)
        {
            auto res = geoManager->CalculateSolidAttr(solids);
            point = res.at(0).m_coor;
            return true;
        }

        return false;
    }

    void mConnectorBase::setConnData(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        _connectorData = connectorData;
    }

    void mConnectorBase::notifyConnectorChanged()
    {
        if (!_connectorData)
        {
            return;
        }
        // 完整性检查读取数据类字段，而拾取数据仅在控件失活时 commit 落盘；
        // 多选控件（如MPC主节点）拾取期间不失活，检查会一直读到旧数据，
        // 误报"未选择"，直到其他操作使其失活提交。刷新状态前先提交
        // 当前激活拾取控件，保证检查结果与界面拾取一致。
        mPickWidgetController::commitActivePickWidget();
        if (getCVM && getCVM->getDataStatusManager())
        {
            getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_Connector, _connectorData->getConnectorId());
        }
    }

    QString mConnectorBase::PickDataEnum2Str(MxFunctions::PickDataType type)
    {
        switch (type)
        {
        case MxFunctions::PickDataType::None:
            return "";
        case MxFunctions::PickDataType::GeoPoint:
            return "几何点";
        case MxFunctions::PickDataType::GeoEdge:
            return "几何边";
        case MxFunctions::PickDataType::GeoFace:
            return "几何面";
        case MxFunctions::PickDataType::GeoSolid:
            return "几何实体";
        case MxFunctions::PickDataType::Node:
            return "节点";
        case MxFunctions::PickDataType::ElementEdge:
            return "单元边";
        case MxFunctions::PickDataType::ElementFace:
            return "单元面";
        case MxFunctions::PickDataType::Element:
            return "单元";
        case MxFunctions::PickDataType::Part:
            return "部件";
        default:
            break;
        }
        return "";
    }

    int mConnectorBase::calculateMasterNode(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickedData)
    {
        //先将这些对象转为他们的形心点
        std::vector<std::pair<MxFunctions::PickDataType, int>>nodes;
        for (int i = 0; i < pickedData.size(); ++i)
        {
            nodes.push_back({ pickedData[i].first,pickedData[i].second.result_id });
        }
        QVector<QVector3D> pos = getNodesCenter(nodes);
        QVector3D coords;
        for (int i = 0; i < pos.size(); ++i)
        {
            coords += pos[i];
        }
        coords = coords / (nodes.size());

        //int nodeId = getCVM->createFreeNode(coords);
        int nodeId = getCVM->createFreeNode(coords);
        mGlobalSignals::getInstance()->UpDateView();

        return nodeId;
    }

    QVector<QVector3D> mConnectorBase::getNodesCenter(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
    {
        QVector<QVector3D>nodes;
        for (int i = 0; i < nodes_id.size(); ++i)
        {
            auto pickedtype = nodes_id[i].first;
            switch (pickedtype)
            {
            case MxFunctions::PickDataType::Node:
            {
                nodes.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodes_id[i].second));
            }
            break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ nodes_id[i].second });
                nodes.push_back(points[0].m_coor);
            }
            break;
            case MxFunctions::PickDataType::GeoEdge:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr({ nodes_id[i].second });
                nodes.push_back(points[0].m_coor);
            }
            break;
            case MxFunctions::PickDataType::GeoFace:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr({ nodes_id[i].second });
                nodes.push_back(points[0].m_coor);
            }
            break;
            case MxFunctions::PickDataType::GeoSolid:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateSolidAttr({ nodes_id[i].second });
                nodes.push_back(points[0].m_coor);
            }
            break;
            case MxFunctions::PickDataType::Part:
            {
                QVector3D partcoor;
                double v = 0.0;
                if (getCVM->getModelData()->getMeshData()->getPartWeight(nodes_id[i].second, v, partcoor))
                {
                    nodes.push_back(partcoor);
                }
            }
            break;
            default:
                break;
            }
        }
        return nodes;
    }

    void mConnectorBase::transPickedToNode(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>>trans_res;
        for (int i = 0; i < nodes_id.size(); ++i)
        {
            auto pickedtype = nodes_id[i].first;
            switch (pickedtype)
            {
            case MxFunctions::PickDataType::Node:
            {
                trans_res.push_back(nodes_id[i]);
            }
            break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ nodes_id[i].second });
                int createnode = getCVM->createFreeNode(points[0].m_coor);
                trans_res.push_back({ MxFunctions::PickDataType::GeoPoint , createnode });
            }
            break;
            case MxFunctions::PickDataType::GeoEdge:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateEdgeAttr({ nodes_id[i].second });
                int createnode = getCVM->createFreeNode(points[0].m_coor);
                trans_res.push_back({ MxFunctions::PickDataType::Node , createnode });
            }
            break;
            case MxFunctions::PickDataType::GeoFace:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateFaceAttr({ nodes_id[i].second });
                int createnode = getCVM->createFreeNode(points[0].m_coor);
                trans_res.push_back({ MxFunctions::PickDataType::Node , createnode });
            }
            break;
            case MxFunctions::PickDataType::GeoSolid:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateSolidAttr({ nodes_id[i].second });
                int createnode = getCVM->createFreeNode(points[0].m_coor);
                trans_res.push_back({ MxFunctions::PickDataType::Node , createnode });

            }
            break;
            case MxFunctions::PickDataType::Part:
            {
                QVector3D partcoor;
                double v = 0.0;
                if (getCVM->getModelData()->getMeshData()->getPartWeight(nodes_id[i].second, v, partcoor))
                {
                    int createnode = getCVM->createFreeNode(partcoor);
                    trans_res.push_back({ MxFunctions::PickDataType::Node , createnode });
                }
            }
            break;
            default:
                break;
            }
        }
        nodes_id = trans_res;
    }

    QVector<QVector3D> mConnectorBase::transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
    {
        QVector<QVector3D> rendpos;
        for (int i = 0; i < nodes_id.size(); ++i)
        {
            auto pickedtype = nodes_id[i].first;
            switch (pickedtype)
            {
            case MxFunctions::PickDataType::Node:
            {
                rendpos.push_back(getCVM->getModelData()->getMeshData()->getVectorById(nodes_id[i].second));
            }
            break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = getCVM->getModelData()->getGeoManager()->CalculateVertexAttr({ nodes_id[i].second });
                rendpos.push_back(points[0].m_coor);
            }
            break;
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, { nodes_id[i].second });
                rendpos.append(coords);
            }
            break;
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, { nodes_id[i].second });
                rendpos.append(coords);
            }
            break;
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, { nodes_id[i].second });
                rendpos.append(coords);
            }
            break;
            case MxFunctions::PickDataType::Part:
            {
                QVector<QVector3D>coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(4, { nodes_id[i].second });
                rendpos.append(coords);
            }
            break;
            default:
                break;
            }
        }
        return rendpos;
    }

    void mConnectorBase::getNodesFromPicked(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>>trans_res;
        for (int i = 0; i < nodes_id.size(); ++i)
        {
            auto pickedtype = nodes_id[i].first;
            switch (pickedtype)
            {
            case MxFunctions::PickDataType::Node:
            {
                trans_res.push_back(nodes_id[i]);
            }
            break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto p2n = getCVM->getModelData()->getMeshData()->getNodeByGeoPoint(nodes_id[i].second);
                if (p2n.result_id != -1)
                {
                    trans_res.push_back({ PickDataType::Node ,p2n.result_id });
                }
            }
            break;
            case MxFunctions::PickDataType::GeoEdge:
            {
                auto e2n = getCVM->getModelData()->getMeshData()->getNodeByGeoEdge(nodes_id[i].second);
                for (int i = 0; i < e2n.size(); ++i)
                {
                    if (e2n[i].result_id != -1)
                    {
                        trans_res.push_back({ PickDataType::Node ,e2n[i].result_id });
                    }
                }
            }
            break;
            case MxFunctions::PickDataType::GeoFace:
            {
                auto f2n = getCVM->getModelData()->getMeshData()->getNodeByGeoFace(nodes_id[i].second);
                for (int i = 0; i < f2n.size(); ++i)
                {
                    if (f2n[i].result_id != -1)
                    {
                        trans_res.push_back({ PickDataType::Node ,f2n[i].result_id });
                    }
                }
            }
            break;
            case MxFunctions::PickDataType::GeoSolid:
            {

            }
            break;
            case MxFunctions::PickDataType::Part:
            {
                auto part2n = getCVM->getModelData()->getMeshData()->getEleIdsByPartId(nodes_id[i].second);
                for (int i = 0; i < part2n.size(); ++i)
                {
                    if (part2n[i] != -1)
                    {
                        trans_res.push_back({ PickDataType::Node ,part2n[i] });
                    }
                }
            }
            break;
            default:
                break;
            }
        }
        nodes_id = trans_res;
    }

    /*************************************************绑定***************************************************************/
    mConnectorBind::mConnectorBind(QWidget* parent)
        :mConnectorBase(parent)
    {
        connectorPtr = nullptr;
        initInputs();
    }

    mConnectorBind::~mConnectorBind()
    {
        clearInputs();
    }

    void mConnectorBind::initInputs()
    {
        // 接触对：使用通用拾取控件（参考接触面板）
        _masterPickController = mGeneralPickUp::createPickWidgetNoTempHight(CBRed);
        _masterPickController->setDefultTip("主面");
        _masterPickController->setAllowedTypes(&pickGeoFaceAndElementSurface);
        connect(_masterPickController, &mPickWidgetController::pickActiveChanged, this, &mConnectorBind::slot_masterPickActiveChanged);
        connect(_masterPickController, &mPickWidgetController::dataChanged, this, &mConnectorBind::slot_commitMasterPickData);

        _slavePickController = mGeneralPickUp::createPickWidgetNoTempHight(CBBlue);
        _slavePickController->setDefultTip("从面");
        _slavePickController->setAllowedTypes(&pickGeoFaceAndElementSurface);
        connect(_slavePickController, &mPickWidgetController::pickActiveChanged, this, &mConnectorBind::slot_slavePickActiveChanged);
        connect(_slavePickController, &mPickWidgetController::dataChanged, this, &mConnectorBind::slot_commitSlavePickData);

        switchbtn = new QPushButton;
        switchbtn->setFixedWidth(30);
        switchbtn->setIcon(QIcon(":/mMainWidgets/Icon/jiaohuan3x.png"));
        connect(switchbtn, &QPushButton::clicked, this, &mConnectorBind::slot_switchMainAndSubs);
        widinside = new QWidget();
        vlayout = new QVBoxLayout(widinside);
        vlayout->addWidget(_masterPickController->widget());
        vlayout->addWidget(_slavePickController->widget());
        hlayout = new QHBoxLayout();
        hlayout->addWidget(widinside);
        hlayout->addWidget(switchbtn);
        vlayout->setContentsMargins(0, 0, 0, 0);
        hlayout->setContentsMargins(0, 0, 0, 0);
        widoutside = new QWidget();
        widoutside->setLayout(hlayout);
        widoutside->setWindowFlags(Qt::Widget);
        widoutside->setContentsMargins(0, 0, 0, 0);
        widinside->setWindowFlags(Qt::Widget);
        widinside->setContentsMargins(0, 0, 0, 0);
        //类型
        valtypes_ = QStringList{
            "点面绑定",
            "面面绑定",
        };
        typewid.initWidget("类型");
        for (auto t : valtypes_)
        {
            typewid.combobox->addItem(t);
        }
        typewid.combobox->setCurrentIndex(0);
        connect(typewid.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorBind::saveType);

        //参数
        allowrancetype_.initWidget("容差类型");
        allowrancetype_.combobox->addItem("自定义容差");
        allowrancetype_.combobox->addItem("abaqus");
        allowrancetype_.combobox->addItem("optistruct");
        allowrancetype_.combobox->setCurrentIndex(0);
        connect(allowrancetype_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorBind::saveAllowranceType);
        allowrance_.initWidget("容差", false);
        allowrance_.labelname->setFixedWidth(90);
        connect(allowrance_.lineeditvalue, &mCalcLineEdit::editingFinished,
            this, &mConnectorBind::saveAllowrance);

        adjust_.initWidget("调整");
        adjust_.btn->hide();
        connect(adjust_.labelname, &QCheckBox::stateChanged, this, &mConnectorBind::saveAdjust);
    }


    void mConnectorBind::slot_switchMainAndSubs()
    {
        if (!connectorPtr) return;

        // 先提交当前激活控件数据，确保底层映射最新
        mPickWidgetController::commitActivePickWidget();

        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int masterSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Master);
        int slaveSetId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Slave);

        if (masterSetId != -1 || slaveSetId != -1)
        {
            mapRelationManager.appendMapRelation(
                { PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() },
                { Physical_Set, slaveSetId }, MapRelation_Master);
            mapRelationManager.appendMapRelation(
                { PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() },
                { Physical_Set, masterSetId }, MapRelation_Slave);
        }

        loadBindDataToPickWidget(MapRelation_Master, _masterPickController);
        loadBindDataToPickWidget(MapRelation_Slave, _slavePickController);

        notifyConnectorChanged();
    }


    void mConnectorBind::slot_commitMasterPickData()
    {
        if (!getCVM)
            return;
        auto widget_ptr = getCVM->getPreRenderWidget();
        if (!widget_ptr)
            return;
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("red"), true, -2.0f);
        geoHighLightRender->setHighLightColor(255, 0, 0);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("red"));
        meshHighLightRender->setHighLightColor(255, 0, 0);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});

        std::vector<RenderPickResultData> meshRenderData;
        std::vector<int> geoRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(_masterPickController->getAllData(), meshRenderData, geoRenderData);

        if (geoHighLightRender && geoRenderData.size() > 0)
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
        }
        if (meshHighLightRender && meshRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshRenderData.size());
            for (const auto& d : meshRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshFace, meshIds);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        delete geoData;
        delete meshData;
        connectorPtr->setPickStateMaster(_masterPickController->getAllData().isEmpty() ? 1 : 2);
        notifyConnectorChanged();
    }

    void mConnectorBind::slot_commitSlavePickData()
    {
        if (!getCVM)
            return;
        auto widget_ptr = getCVM->getPreRenderWidget();
        if (!widget_ptr)
            return;
        auto highlight_controller = widget_ptr->getHighlightController();

        auto geoHighLightRender = highlight_controller->getExtraPreGeoHighLightRender(std::string("blue"), true, -2.0f);
        geoHighLightRender->setHighLightColor(0, 0, 255);
        mGeoSelectData* geoData = new mGeoSelectData();
        geoData->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, {});

        auto meshHighLightRender = highlight_controller->getExtraPreMeshHighLightRender(std::string("blue"));
        meshHighLightRender->setHighLightColor(0, 0, 255);
        MeshSelectMemoryData* meshData = new MeshSelectMemoryData();
        meshData->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});

        std::vector<RenderPickResultData> meshRenderData;
        std::vector<int> geoRenderData;
        AllRenderSymbolManager::getInstance()->rendContact(_slavePickController->getAllData(), meshRenderData, geoRenderData);

        if (geoHighLightRender && geoRenderData.size() > 0)
        {
            geoData->UpdatePickData(mxr::SelectionOperate::ADD, SelectionInfoType::GeoFace, geoRenderData);
        }
        if (meshHighLightRender && meshRenderData.size() > 0)
        {
            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(meshRenderData.size());
            for (const auto& d : meshRenderData)
            {
                mxr::MeshSelectInfoData n;
                n.data_id.element_id = d.result_id;
                n.data_index.line_index = d.result_id2;
                n.part_id = d.part_id;
                meshIds.push_back(n);
            }
            meshData->UpdateSelectDataByType(mxr::SelectionOperate::ADD, SelectionInfoType::MeshFace, meshIds);
        }

        geoHighLightRender->updateHighLightRender(geoData);
        meshHighLightRender->updateHighLightRender(meshData);
        delete geoData;
        delete meshData;
        connectorPtr->setPickStateSlave(_slavePickController->getAllData().isEmpty() ? 1 : 2);
        notifyConnectorChanged();
    }

    void mConnectorBind::slot_masterPickActiveChanged(bool active)
    {
        if (active)
        {
            emit mMechGlobalSignals::getInstance()->activePickSig(7);
        }
        else
        {
            connectorPtr->setPickStateMaster(0);
        }
    }

    void mConnectorBind::slot_slavePickActiveChanged(bool active)
    {
        if (active)
        {
            emit mMechGlobalSignals::getInstance()->activePickSig(7);
        }
        else
        {
            connectorPtr->setPickStateSlave(0);
        }
    }

    void mConnectorBind::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _masterPickController, _slavePickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    void mConnectorBind::clearFaceHighlight()
    {
        if (!getCVM)
        {
            return;
        }
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

    void mConnectorBind::slot_clearMainNode()
    {
        if (_masterPickController)
            _masterPickController->replacePickedDataAndClearSets({});
        if (connectorPtr)
        {
            auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
            int masterSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Master);
            if (masterSetId != -1)
            {
                mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(masterSetId);
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (mixset)
                    mixset->setMixSetData({});
            }
        }
        slot_commitMasterPickData();
    }

    void mConnectorBind::slot_clearSubNode()
    {
        if (_slavePickController)
            _slavePickController->replacePickedDataAndClearSets({});
        if (connectorPtr)
        {
            auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
            int slaveSetId = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Slave);
            if (slaveSetId != -1)
            {
                mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(slaveSetId);
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (mixset)
                    mixset->setMixSetData({});
            }
        }
        slot_commitSlavePickData();
    }

    void mConnectorBind::loadBindDataToPickWidget(int mapTypeRaw, mPickWidgetController* picker)
    {
        if (!picker || !connectorPtr) return;
        MxFunctions::MapRelationType mapType = static_cast<MxFunctions::MapRelationType>(mapTypeRaw);

        PickWidgetData widgetData;
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        int setId = mapRelationManager.getMapRelation<int>(
            { PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, mapType);
        if (setId != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setId);
            if (set)
            {
                if (set->getSetType() == Mix)
                {
                    auto mixset = dynamic_cast<mMixSet*>(set);
                    if (mixset)
                        widgetData.pickedData = mixset->getMixSetData();
                }
                else if (set->getSetType() == GeoFace || set->getSetType() == ElementSurface)
                {
                    widgetData.selectedSetNames.insert(set->getSetName());
                }
            }
        }

        auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
            PhysicalFieldType::Physical_Connector, connectorPtr->getConnectorId(),
            mapType == MapRelation_Master ? MapRelation_Master : MapRelation_Slave);
        picker->setPersistenceStrategy(strategy);
        picker->setSetResolver(&resolveSetToTypedData);
        picker->setSelectSetList(getSetNamesList({ SetType::GeoFace, SetType::ElementSurface }));
        picker->displayData(widgetData, true);

        // 反显后主动刷新一次渲染高亮，避免旧高亮残留
        if (mapType == MapRelation_Master)
            slot_commitMasterPickData();
        else
            slot_commitSlavePickData();
    }

    void mConnectorBind::refreshValueSection()
    {
        if (!faval_)
        {
            return;
        }
        faval_->removeAll();
        faval_->addRow(typewid.propwid);
        typewid.propwid->show();
        faval_->addRow(allowrancetype_.propwid);
        allowrancetype_.propwid->show();
        if (allowrancetype_.combobox->currentIndex() == 0)
        {
            faval_->addRow(allowrance_.propwid);
            allowrance_.propwid->show();
        }
        faval_->addRow(adjust_.propwid);
        adjust_.propwid->show();
        faval_->refreshLayout();
    }

    void mConnectorBind::clearInputs()
    {
        if (_masterPickController)
        {
            delete _masterPickController;
            _masterPickController = nullptr;
        }
        if (_slavePickController)
        {
            delete _slavePickController;
            _slavePickController = nullptr;
        }
        typewid.clear();
        if (switchbtn)
        {
            delete switchbtn;
            switchbtn = nullptr;
        }
        if (vlayout)
        {
            delete vlayout;
            vlayout = nullptr;
        }
        if (hlayout)
        {
            delete hlayout;
            hlayout = nullptr;
        }
        if (widinside)
        {
            delete widinside;
            widinside = nullptr;
        }
        if (widoutside)
        {
            delete widoutside;
            widoutside = nullptr;
        }
    }

    void mConnectorBind::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBind>(connectorData);

        loadBindDataToPickWidget(MapRelation_Master, _masterPickController);
        loadBindDataToPickWidget(MapRelation_Slave, _slavePickController);

        // 默认激活主面控件（与接触面板一致）
        if (_masterPickController && !_masterPickController->isPickActive() &&
            (!_slavePickController || !_slavePickController->isPickActive()))
        {
            _masterPickController->setPickActive(true);
        }

        typewid.combobox->blockSignals(true);
        typewid.combobox->setCurrentIndex(connectorPtr->type_);
        typewid.combobox->blockSignals(false);

        allowrancetype_.combobox->blockSignals(true);
        allowrancetype_.combobox->setCurrentIndex(connectorPtr->allowrancetype_);
        allowrancetype_.combobox->blockSignals(false);

        if (connectorPtr->allowrancetype_ == 0)
        {
            QVariant param = connectorPtr->allowrance_;
            allowrance_.lineeditvalue->setExpression(MxFunctions::mNumberToString(param.toDouble()));
        }

        adjust_.labelname->blockSignals(true);
        adjust_.labelname->setChecked(connectorPtr->adjust_);
        adjust_.labelname->blockSignals(false);
    }

    QVector<QWidget*> mConnectorBind::getPosInputs(FoldSection* fapos)
    {
        fapos_ = fapos;
        QVector<QWidget*>wds;
        wds.append(widoutside);
        return wds;
    }

    QVector<QWidget*> mConnectorBind::getValInputs(FoldSection* faval)
    {
        faval_ = faval;
        QVector<QWidget*>wds;
        wds.append(typewid.propwid);
        wds.append(allowrancetype_.propwid);
        if (allowrancetype_.combobox->currentIndex() == 0)
            wds.append(allowrance_.propwid);
        wds.append(adjust_.propwid);
        return wds;
    }

    void mConnectorBind::saveType()
    {
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->type_ = typewid.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorBind::saveAllowranceType()
    {
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->allowrancetype_ = allowrancetype_.combobox->currentIndex();
        if (connectorPtr->allowrancetype_ != 0)
        {
            // 非自定义容差时清空旧值，避免残留
            connectorPtr->allowrance_ = QVariant();
            allowrance_.lineeditvalue->setExpression("");
        }
        else
        {
            // 切回自定义容差时反显已有值
            QVariant param = connectorPtr->allowrance_;
            if (param.isValid() && !param.isNull())
            {
                allowrance_.lineeditvalue->setExpression(QString::number(param.toDouble()));
            }
            else
            {
                allowrance_.lineeditvalue->setExpression("");
            }
        }
        refreshValueSection();
        notifyConnectorChanged();
    }

    void mConnectorBind::saveAllowrance()
    {
        if (!connectorPtr)
        {
            return;
        }
        if (connectorPtr->allowrancetype_ == 0)
        {
            connectorPtr->allowrance_ = allowrance_.lineeditvalue->text().trimmed().toDouble();
        }
        notifyConnectorChanged();
    }

    void mConnectorBind::saveAdjust()
    {
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->adjust_ = adjust_.labelname->isChecked();
        notifyConnectorChanged();
    }

    /*************************************************刚性连接***************************************************************/
    mConnectorRigid::mConnectorRigid(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorRigid::~mConnectorRigid()
    {
        clearInputs();
    }

    void mConnectorRigid::initInputs()
    {
        //位置
        poswaywid.initWidget("方式");
        poswaywid.combobox->addItem("一对一");
        poswaywid.combobox->addItem("一对多");
        connect(poswaywid.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorRigid::slot_nodeWayChanged);

        // 通用拾取控件：主节点（一对一始终只能拾取一个）
        _mainPickController = mGeneralPickUp::createOnePickWidget(CBBlue);
        _mainPickController->setDefultTip("主节点");
        // 持久化策略在 setInputsValue 中根据当前 connectorData 重新绑定，
        // initInputs 时 _connectorData 尚为空，绑定空对象会导致后续提交无法写入。
        connect(_mainPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });

        connect(_mainPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteMasterConnNodes();
            appendMainConnNodes();
            updateConnectorSymbolRealtime();
            if (getWidgetDataAsPickIds().size() > 0)
                _subPickController->setPickActive(true);
            notifyConnectorChanged();
            });

        // 通用拾取控件：从节点（一对一在 applyPickMode 中切换为 PickOne）
        _subPickController = mGeneralPickUp::createMorePickWidget(CBRed);
        _subPickController->setDefultTip("从节点");
        // 持久化策略在 setInputsValue 中根据当前 connectorData 重新绑定。
        connect(_subPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_subPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteSubConnNodes();
            appendSubConnNodes();
            updateConnectorSymbolRealtime();
            if (poswaywid.combobox->currentIndex() == 0 && getWidgetDataAsPickPairs().size() > 0)
            {
                _mainPickController->setPickActive(true);
            }
            notifyConnectorChanged();
            });
        connect(_subPickController, &mPickWidgetController::calcMainNodeSig, this, &mConnectorRigid::slot_calMainNode);

        //参数
        names1_ = { "U1","U2","U3" };
        names2_ = { "UR1","UR2","UR3" };
        CheckBoxsWidget checks1;
        checks1.initWidget(names1_);
        checkswid.append(checks1);
        CheckBoxsWidget checks2;
        checks2.initWidget(names2_);
        checkswid.append(checks2);
        for (auto& check : checks1.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorRigid::saveDegreeOfFreedom);
        }
        for (auto& check : checks2.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorRigid::saveDegreeOfFreedom);
        }
    }

    void mConnectorRigid::slot_nodeWayChanged(int i)
    {
        Q_UNUSED(i)
            saveWay();

        deleteMasterConnNodes();
        deleteSubConnNodes();

        // 清空主从节点，避免切换方式后残留数据
        if (_mainPickController)
        {
            _mainPickController->replacePickedDataAndClearSets({});
        }
        if (_subPickController)
        {
            _subPickController->replacePickedDataAndClearSets({});
        }

        applyPickMode();
        applyAllowedPickTypes();

        notifyConnectorChanged();
    }

    void mConnectorRigid::saveWay()
    {
        std::shared_ptr<MechConnectorData::mConnectorRigid> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->way_ = poswaywid.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorRigid::applyPickMode()
    {
        if (!_mainPickController || !_subPickController) return;
        // 主节点始终一对一
        _mainPickController->setPickMode(PickOne);
        // 从节点：一对一只能拾取一个，一对多可拾取多个；
        // 切到一对一且已有多个从节点时，清空让用户重新选，避免遗留歧义。
        const int way = poswaywid.combobox->currentIndex();
        mPickMode subMode = (way == 0) ? PickOne : PickMore;
        _subPickController->setPickMode(subMode, way == 0);
    }

    void mConnectorRigid::applyAllowedPickTypes()
    {
        if (!_mainPickController || !_subPickController) return;

        //1对1时 主从节点都只可以拾取 几何点和节点
        //1对多时 主节点只可以拾取 几何点和节点 从节点可以拾取 几何点几何线几何面实体节点部件

        if (poswaywid.combobox->currentIndex() == 0)
        {
            _mainPickController->setAllowedTypes(&pickGeoPointAndNode);
            _subPickController->setAllowedTypes(&pickGeoPointAndNode);
        }
        else
        {
            _mainPickController->setAllowedTypes(&pickGeoPointAndNode);
            _subPickController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        }

        if (_mainPickController->isPickActive())
        {
            _mainPickController->setPickType();
        }

        if (_subPickController->isPickActive())
        {
            _subPickController->setPickType();
        }
    }

    void mConnectorRigid::saveDegreeOfFreedom()
    {
        std::shared_ptr<MechConnectorData::mConnectorRigid> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }

        connectorPtr->degreeoffreedom_main.clear();
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[0].labels[i - 1];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[1].labels[i - 4];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        notifyConnectorChanged();
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorRigid::getWidgetDataAsPickPairs() const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        if (!_subPickController) return result;
        const auto& allData = _subPickController->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    std::vector<std::pair<MxFunctions::PickDataType, int>> mConnectorRigid::getWidgetDataAsPickIds() const
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>> result;
        if (!_mainPickController) return result;
        const auto& allData = _mainPickController->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d.result_id);
            }
        }
        return result;
    }

    void mConnectorRigid::slot_calMainNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorRigid> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(_connectorData);
        if (!connectorPtr)
            return;

        auto subPairs = getWidgetDataAsPickPairs();
        if (subPairs.empty())
        {
            return;
        }

        int calnodeid = calculateMasterNode(subPairs);
        if (calnodeid == -1)
        {
            return;
        }

        PickWidgetData data;
        RenderPickResultData nodeData{ calnodeid, -1, -1 };
        data.pickedData[MxFunctions::PickDataType::Node].push_back(nodeData);
        _mainPickController->displayData(data, true);

        connectorPtr->mainnodes_ = { PickDataType::Node, nodeData };

        //添加新的主节点
        deleteMasterConnNodes();
        appendMainConnNodes();
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorRigid::clearInputs()
    {
        poswaywid.clear();
        choosewid.clear();
        if (_mainPickController)
        {
            delete _mainPickController;
            _mainPickController = nullptr;
        }
        if (_subPickController)
        {
            delete _subPickController;
            _subPickController = nullptr;
        }
        for (auto checks : checkswid)
        {
            checks.clear();
        }
        checkswid.clear();
    }

    void mConnectorRigid::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorRigid> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorRigid>(connectorData);
        if (!connectorPtr)
            return;

        // 更新当前数据对象，并重新绑定主/从拾取控件的持久化策略。
        // 该控件实例（mConnectorRigid）为单例，initInputs 时 _connectorData 为空，
        // 策略若提前绑定会捕获空 shared_ptr，导致 commit 时无法写入。此处每次反显都按
        // 当前 connectorData 重新生成 DataClassPersistenceStrategy，保证主/从节点
        // 拾取数据在切换面板或创建/编辑连接时正确保存到 mConnectorRigid 的 mainnodes_/subnodes_。
        _connectorData = connectorData;
        if (_mainPickController)
        {
            _mainPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorRigid::mainnodes_)));
        }
        if (_subPickController)
        {
            _subPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeTypedMapWriter(connectorPtr, &MechConnectorData::mConnectorRigid::subnodes_)));
        }

        poswaywid.combobox->blockSignals(true);
        poswaywid.combobox->setCurrentIndex(connectorPtr->way_);
        poswaywid.combobox->blockSignals(false);

        applyPickMode();
        applyAllowedPickTypes();

        master_node = -1;
        // 主节点反显（主节点始终 PickOne）
        if (_mainPickController)
        {
            PickWidgetData mainData;
            if (connectorPtr->mainnodes_.first != MxFunctions::PickDataType::None)
            {
                MxFunctions::PickDataType pickType = connectorPtr->mainnodes_.first;
                mainData.pickedData[pickType].push_back(connectorPtr->mainnodes_.second);
                if (pickType == MxFunctions::PickDataType::Node) master_node = connectorPtr->mainnodes_.second.result_id;
            }
            _mainPickController->displayData(mainData);
        }

        // 从节点反显（按当前方式决定 PickOne/PickMore）
        sub_node.clear();
        if (_subPickController)
        {
            PickWidgetData subData;
            for (auto it = connectorPtr->subnodes_.begin(); it != connectorPtr->subnodes_.end(); ++it)
            {
                MxFunctions::PickDataType pickType = it->first;
                subData.pickedData[pickType] = it->second;
                if (pickType == MxFunctions::PickDataType::Node)
                {
                    for (auto id : it->second)
                    {
                        sub_node.push_back(id.result_id);
                    }
                }
            }
            _subPickController->displayData(subData, true);
        }

        _mainPickController->setPickActive(true);

        // 反显后实时渲染连接符号，使切换回面板时立即可见。
        updateConnectorSymbolRealtime();

        // 反显自由度时阻断信号，避免 saveDegreeOfFreedom 在反显过程中被反复触发，
        // 导致数据被中间状态覆盖、界面勾选数量不一致。
        for (auto& checks : checkswid)
        {
            for (auto* check : checks.labels)
            {
                check->blockSignals(true);
            }
        }

        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[0].labels[i - 1];
            check->setChecked(connectorPtr->degreeoffreedom_main.find(i) != connectorPtr->degreeoffreedom_main.end());
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[1].labels[i - 4];
            check->setChecked(connectorPtr->degreeoffreedom_main.find(i) != connectorPtr->degreeoffreedom_main.end());
        }

        for (auto& checks : checkswid)
        {
            for (auto* check : checks.labels)
            {
                check->blockSignals(false);
            }
        }
    }

    QVector<QWidget*> mConnectorRigid::getPosInputs(FoldSection* fapos)
    {
        fapos_ = fapos;
        QVector<QWidget*>wds;
        wds.append(poswaywid.propwid);
        if (poswaywid.combobox->currentIndex() > 1)wds.append(choosewid.propwid);
        if (_mainPickController)
        {
            wds.append(_mainPickController->widget());
        }
        if (_subPickController)
        {
            wds.append(_subPickController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mConnectorRigid::getValInputs(FoldSection* faval)
    {
        faval_ = faval;
        QVector<QWidget*>wds;
        wds.append(checkswid[0].propwid);
        wds.append(checkswid[1].propwid);
        return wds;
    }

    void mConnectorRigid::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _mainPickController, _subPickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
    }

    void mConnectorRigid::deleteMasterConnNodes()
    {
        if (master_node == -1) return;

        vector<int> nodes{ master_node };
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(nodes);
        master_node = -1;
    }

    void mConnectorRigid::deleteSubConnNodes()
    {
        if (sub_node.empty()) return;
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(sub_node);
        sub_node.clear();
    }

    void mConnectorRigid::appendMainConnNodes()
    {
        std::vector<int> mainnodesid;
        auto mainIds = getWidgetDataAsPickIds();
        for (const auto& pair : mainIds)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second);
        }

        if (mainnodesid.size() > 0)  master_node = mainnodesid[0];
        getCVM->getModelData()->getMeshData()->appendConnectNodes(mainnodesid);
    }

    void mConnectorRigid::appendSubConnNodes()
    {
        std::vector<int> subnodesid;
        auto subPairs = getWidgetDataAsPickPairs();
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
        }
        sub_node = subnodesid;
        getCVM->getModelData()->getMeshData()->appendConnectNodes(subnodesid);
    }


    void mConnectorRigid::updateConnectorSymbolRealtime()
    {
        if (!_mainPickController || !_subPickController || !_connectorData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        if (!meshData || !geoManager)
            return;

        auto posOf = [&](MxFunctions::PickDataType type, int id) -> QVector3D {
            if (type == MxFunctions::PickDataType::Node)
            {
                return meshData->getVectorById(id);
            }
            else if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto points = geoManager->CalculateVertexAttr({ id });
                if (!points.empty())
                    return points[0].m_coor;
            }
            return QVector3D();
            };

        // 主节点：按模式应唯一，取第一个有效对象
        QVector3D masterPos;
        bool hasMaster = false;
        const auto mainData = _mainPickController->getAllData();
        for (const auto& pair : mainData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                masterPos = posOf(type, d.result_id);
                hasMaster = true;
                break;
            }
            if (hasMaster)
                break;
        }

        // 从节点：汇总所有有效对象坐标
        QVector<QVector3D> slavePoss;
        const auto subData = _subPickController->getAllData();
        for (const auto& pair : subData.pickedData)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        slavePoss.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) slavePoss.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            default:
                break;
            }
        }

        auto render = AllRenderSymbolManager::getInstance();
        if (hasMaster && !slavePoss.isEmpty())
        {
            render->assignConnector(_connectorData, masterPos, slavePoss, ConnectorType::Connector_Rigid);
        }
        else
        {
            // 数据不完整时清除实时符号，避免残留旧连线
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_Rigid);
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    /*************************************************柔性连接***************************************************************/
    mConnectorFlexible::mConnectorFlexible(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorFlexible::~mConnectorFlexible()
    {
        clearInputs();
    }

    void mConnectorFlexible::initInputs()
    {
        //位置
        poswaywid.initWidget("方式");
        poswaywid.combobox->addItem("一对一");
        poswaywid.combobox->addItem("一对多");
        connect(poswaywid.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorFlexible::slot_nodeWayChanged);

        // 通用拾取控件：从节点（一对一，几何点/节点）
        _mainPickController = mGeneralPickUp::createOnePickWidget(CBBlue);
        _mainPickController->setDefultTip("从节点");
        connect(_mainPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_mainPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteMainConnNodes();
            appendMainConnNodes();
            // 与刚性连接对齐：计算/选择的主节点直接写入业务数据，由通用拾取控件高亮渲染
            saveSubNode();
            updateConnectorSymbolRealtime();
            if (getWidgetDataAsPickIds(true).size() > 0)
                _subPickController->setPickActive(true);
            updateTableBtnUsing();
            notifyConnectorChanged();
            });

        // 通用拾取控件：主节点（一对一/一对多，几何点/节点）
        _subPickController = mGeneralPickUp::createMorePickWidget(CBRed);
        _subPickController->setDefultTip("主节点");
        connect(_subPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_subPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteSubConnNodes();
            appendSubConnNodes();
            updateConnectorSymbolRealtime();
            saveMainNode();
            if (poswaywid.combobox->currentIndex() == 0 && getWidgetDataAsPickPairs(false).size() > 0)
            {
                _mainPickController->setPickActive(true);
            }
            updateTableBtnUsing();
            if (nodeseting_.tablebtn->isChecked())
                slot_nodeSetting();
            notifyConnectorChanged();
            });
        connect(_subPickController, &mPickWidgetController::calcMainNodeSig, this, &mConnectorFlexible::slot_calMainNode);

        //主节点权重
        weights_.initWidget("权重值", false);
        connect(weights_.lineeditvalue, &mCalcLineEdit::editingFinished,
            this, &mConnectorFlexible::saveWeight);

        //主节点自由度
        names1_ = { "U1","U2","U3" };
        names2_ = { "UR1","UR2","UR3" };
        CheckBoxsWidget checks1;
        checks1.initWidget(names1_);
        for (auto* check : checks1.labels) check->setChecked(false);
        checkswid.append(checks1);
        CheckBoxsWidget checks2;
        checks2.initWidget(names2_);
        for (auto* check : checks2.labels) check->setChecked(false);
        checkswid.append(checks2);
        for (auto& check : checks1.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorFlexible::saveDegreeOfFreedom);
        }
        for (auto& check : checks2.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorFlexible::saveDegreeOfFreedom);
        }

        //从节点自由度
        CheckBoxsWidget checks3;
        checks3.initWidget(names1_);
        for (auto* check : checks3.labels) check->setChecked(false);
        checkswid.append(checks3);
        CheckBoxsWidget checks4;
        checks4.initWidget(names2_);
        for (auto* check : checks4.labels) check->setChecked(false);
        checkswid.append(checks4);
        for (auto& check : checks3.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorFlexible::saveDegreeOfFreedom);
        }
        for (auto& check : checks4.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorFlexible::saveDegreeOfFreedom);
        }

        //初始化表格
        nodeseting_.initWidget("对每个节点进行差异化设置");
        nodeseting_.tablebtn->setCheckable(true);
        nodeseting_.tablebtn->setChecked(false);
        connect(nodeseting_.tablebtn, &QPushButton::toggled, [=](bool checked) {
            if (checked) {
                slot_nodeSetting();
                setConnectTbaleSig(true);
                nodeseting_.tablebtn->setStyleSheet(checkedStyle);
            }
            else
            {
                setConnectTbaleSig(false);
                nodeseting_.tablebtn->setStyleSheet(defaultStyle);
            }
            saveTableMode();
            });
        initTable();
        setConnectTbaleSig(false);
        nodeseting_.tablebtn->setStyleSheet(
            defaultStyle
        );
    }

    void mConnectorFlexible::slot_nodeSetting()
    {
        QVector<QVector<QString>> datas;

        // 保留表格中已有节点的权重值和自由度状态，避免每次点击按钮都重置
        QHash<int, double> existingWeights;
        QHash<int, QVector<QString>> existingDofs;
        int rowCount = table_->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* nodeItem = table_->item(i, 0);
            if (nodeItem)
            {
                bool ok;
                int nodeId = nodeItem->text().toInt(&ok);
                if (ok)
                {
                    QTableWidgetItem* weightItem = table_->item(i, 1);
                    if (weightItem)
                        existingWeights[nodeId] = weightItem->text().toDouble();

                    QVector<QString> dof;
                    for (int col = 2; col <= 7; ++col)
                    {
                        QTableWidgetItem* item = table_->item(i, col);
                        if (item)
                        {
                            bool checked = item->data(Qt::CheckStateRole).toInt() == Qt::Checked;
                            dof.push_back(checked ? "1" : "");
                        }
                        else
                        {
                            dof.push_back("");
                        }
                    }
                    existingDofs[nodeId] = dof;
                }
            }
        }

        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            int x = pair.second.result_id;
            double w = existingWeights.contains(x) ? existingWeights[x] : 1.0;
            QVector<QString> freedoms = existingDofs.contains(x) ? existingDofs[x] : QVector<QString>{ "", "", "", "", "", "" };
            QVector<QString>tmp;
            tmp.push_back(MxFunctions::mNumberToString(x));
            tmp.push_back(MxFunctions::mNumberToString(w));
            tmp.append(freedoms);
            datas.push_back(tmp);
        }

        table_->blockSignals(true);
        setValueToTable(datas);
        table_->blockSignals(false);

        saveTableData();

        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mConnectorFlexible::slot_nodeWayChanged(int i)
    {
        Q_UNUSED(i)
            saveWay();

        deleteMainConnNodes();
        deleteSubConnNodes();

        // 切换方式时清空主从节点，避免残留数据
        if (_mainPickController)
        {
            _mainPickController->replacePickedDataAndClearSets({});
        }
        if (_subPickController)
        {
            _subPickController->replacePickedDataAndClearSets({});
        }

        applyPickMode();
        applyAllowedPickTypes();

        notifyConnectorChanged();
    }

    void mConnectorFlexible::applyPickMode()
    {
        if (!_mainPickController || !_subPickController) return;
        // 主节点始终一对一
        _mainPickController->setPickMode(PickOne);
        // 从节点：一对一只能拾取一个，一对多可拾取多个；
        // 切到一对一且已有多个从节点时，清空让用户重新选，避免遗留歧义。
        const int way = poswaywid.combobox->currentIndex();
        mPickMode subMode = (way == 0) ? PickOne : PickMore;
        _subPickController->setPickMode(subMode, way == 0);
    }

    void mConnectorFlexible::applyAllowedPickTypes()
    {
        if (!_mainPickController || !_subPickController) return;

        //1对1时 主从节点都只可以拾取 几何点和节点
        //1对多时 主节点只可以拾取 几何点和节点 从节点可以拾取 几何点几何线几何面实体节点部件

        if (poswaywid.combobox->currentIndex() == 0)
        {
            _mainPickController->setAllowedTypes(&pickGeoPointAndNode);
            _subPickController->setAllowedTypes(&pickGeoPointAndNode);
        }
        else
        {
            _mainPickController->setAllowedTypes(&pickGeoPointAndNode);
            _subPickController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        }

        if (_mainPickController->isPickActive())
        {
            _mainPickController->setPickType();
        }

        if (_subPickController->isPickActive())
        {
            _subPickController->setPickType();
        }
    }

    void mConnectorFlexible::slot_checkDegreeOfFreedom(int state)
    {
        Q_UNUSED(state)
            saveDegreeOfFreedom();
    }

    void mConnectorFlexible::slot_tableEditFinished(int row, int col)
    {
        if (col == 1) // 权重列，支持多选批量修改
        {
            QTableWidgetItem* currentItem = table_->item(row, col);
            if (currentItem)
            {
                QString newValue = currentItem->text();
                QList<QTableWidgetItem*> selectedItems = table_->selectedItems();
                bool blocked = table_->signalsBlocked();
                table_->blockSignals(true);
                for (QTableWidgetItem* item : selectedItems)
                {
                    if (item->column() == 1 && item->row() != row)
                    {
                        item->setText(newValue);
                    }
                }
                table_->blockSignals(blocked);
            }
        }
        if (col >= 2 && col <= 7)
        {
            updateHeaderCheckBoxState();
        }
        saveTableData();
    }

    void mConnectorFlexible::slot_mainPickDataChanged()
    {
        saveSubNode();
        notifyConnectorChanged();
    }

    void mConnectorFlexible::slot_subPickDataChanged()
    {
        saveMainNode();
        if (nodeseting_.tablebtn->isChecked())
        {
            slot_nodeSetting();
        }
        notifyConnectorChanged();
    }

    void mConnectorFlexible::initTable()
    {
        table_ = new mConnectorTableWidget();
        table_->setRowCount(0);
        table_->setColumnCount(8);
        table_->verticalHeader()->setHidden(false);
        table_->resizeColumnsToContents();

        QStringList headerText;
        headerText << QString("节点ID") << QString("权重") << QString("U1") << QString("U2") << QString("U3")
            << QString("UR1") << QString("UR2") << QString("UR3");
        table_->setHorizontalHeaderLabels(headerText);

        // 创建自定义表头（在setHorizontalHeaderLabels之后）
        checkBoxHeader_ = new mCheckBoxHeader(Qt::Horizontal, table_);
        table_->setHorizontalHeader(checkBoxHeader_);

        // 3-8列（索引2-7）设置表头复选框
        for (int c = 2; c < 8; ++c)
        {
            checkBoxHeader_->setColumnCheckBox(c, true, headerText[c]);
            QTableWidgetItem* headerItem = table_->horizontalHeaderItem(c);
            if (headerItem)
                headerItem->setText("");
        }

        for (int c = 0; c < 8; ++c)
        {
            table_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }

        // 创建复选框委托并设置给checkbox列
        checkBoxDelegate_ = new mCheckBoxDelegate(table_);
        checkBoxDelegate_->setColumnText(2, QString("U1"));
        checkBoxDelegate_->setColumnText(3, QString("U2"));
        checkBoxDelegate_->setColumnText(4, QString("U3"));
        checkBoxDelegate_->setColumnText(5, QString("UR1"));
        checkBoxDelegate_->setColumnText(6, QString("UR2"));
        checkBoxDelegate_->setColumnText(7, QString("UR3"));
        for (int c = 2; c < 8; ++c)
        {
            table_->setItemDelegateForColumn(c, checkBoxDelegate_);
        }

        // 连接表头复选框信号
        connect(checkBoxHeader_, &mCheckBoxHeader::columnCheckStateChanged, this, &mConnectorFlexible::slot_headerCheckBoxStateChanged);
    }

    void mConnectorFlexible::setValueToTable(const QVector<QVector<QString>>& data)
    {
        table_->setRowCount(0);
        table_->setColumnCount(8);
        table_->verticalHeader()->setHidden(false);
        table_->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("节点ID") << QString("权重") << QString("U1") << QString("U2") << QString("U3")
            << QString("UR1") << QString("UR2") << QString("UR3");
        table_->setHorizontalHeaderLabels(headerText);
        for (int c = 2; c < 8; ++c)
        {
            QTableWidgetItem* headerItem = table_->horizontalHeaderItem(c);
            if (headerItem)
                headerItem->setText("");
        }
        for (int c = 0; c < 8; ++c)
        {
            table_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
        table_->setRowCount(data.size());
        for (int i = 0; i < data.size(); ++i)
        {
            //节点id
            QTableWidgetItem* item1 = new QTableWidgetItem(data[i][0]);
            item1->setTextAlignment(Qt::AlignCenter);
            table_->setItem(i, 0, item1);

            //权重
            QTableWidgetItem* item2 = new QTableWidgetItem(data[i][1]);
            item2->setTextAlignment(Qt::AlignCenter);
            table_->setItem(i, 1, item2);

            // 使用委托绘制复选框，通过CheckStateRole存储状态
            for (int col = 2; col < 8; ++col)
            {
                QTableWidgetItem* item = new QTableWidgetItem();
                item->setTextAlignment(Qt::AlignCenter);
                if (!data[i][col].isEmpty())
                {
                    item->setData(Qt::CheckStateRole, Qt::Checked);
                }
                else
                {
                    item->setData(Qt::CheckStateRole, Qt::Unchecked);
                }
                table_->setItem(i, col, item);
            }
        }

        // 更新表头复选框状态
        if (checkBoxHeader_)
        {
            updateHeaderCheckBoxState();
        }
    }

    void mConnectorFlexible::slot_headerCheckBoxStateChanged(int col, int state)
    {
        Q_UNUSED(col)
            if (state == Qt::Checked)
            {
                toggleColumnCheckBoxes(col, true);
            }
            else if (state == Qt::Unchecked)
            {
                toggleColumnCheckBoxes(col, false);
            }
        saveTableData();
    }

    void mConnectorFlexible::toggleColumnCheckBoxes(int col, bool checked)
    {
        int rowCount = table_->rowCount();
        Qt::CheckState state = checked ? Qt::Checked : Qt::Unchecked;
        bool blocked = table_->signalsBlocked();
        table_->blockSignals(true);
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* item = table_->item(i, col);
            if (!item)
            {
                item = new QTableWidgetItem();
                table_->setItem(i, col, item);
            }
            item->setData(Qt::CheckStateRole, state);
        }
        table_->blockSignals(blocked);
        updateHeaderCheckBoxState();
    }

    void mConnectorFlexible::updateHeaderCheckBoxState()
    {
        if (!checkBoxHeader_) return;

        for (int col = 2; col < 8; ++col)
        {
            int rowCount = table_->rowCount();
            int checkedCount = 0;
            int totalCount = 0;

            for (int i = 0; i < rowCount; ++i)
            {
                QTableWidgetItem* item = table_->item(i, col);
                if (item)
                {
                    totalCount++;
                    if (item->data(Qt::CheckStateRole).toInt() == Qt::Checked)
                        checkedCount++;
                }
            }

            if (totalCount == 0)
            {
                checkBoxHeader_->setColumnCheckState(col, Qt::Unchecked);
            }
            else if (checkedCount == totalCount)
            {
                checkBoxHeader_->setColumnCheckState(col, Qt::Checked);
            }
            else if (checkedCount == 0)
            {
                checkBoxHeader_->setColumnCheckState(col, Qt::Unchecked);
            }
            else
            {
                checkBoxHeader_->setColumnCheckState(col, Qt::PartiallyChecked);
            }
        }
    }

    void mConnectorFlexible::calMainNode()
    {
        auto mainPairs = getWidgetDataAsPickPairs(true);
        if (!mainPairs.empty())
        {
            return;
        }

        auto subPairs = getWidgetDataAsPickPairs(false);
        if (subPairs.empty())
        {
            return;
        }

        int calnodeid = calculateMasterNode(subPairs);
        if (calnodeid == -1)
        {
            return;
        }

        // 与刚性连接对齐：计算出的主节点直接写入业务数据，并交给通用拾取控件高亮渲染
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }

        RenderPickResultData nodeData{ calnodeid, -1, -1 };
        connectorPtr->subnodes_.first = PickDataType::Node;
        connectorPtr->subnodes_.second = nodeData;

        PickWidgetData data;
        data.pickedData[MxFunctions::PickDataType::Node].push_back(nodeData);
        if (_mainPickController)
        {
            _mainPickController->displayData(data, true);
        }
        //添加新的从节点
        deleteMainConnNodes();
        appendMainConnNodes();
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorFlexible::slot_calMainNode()
    {
        calMainNode();
    }

    void mConnectorFlexible::updateTableBtnUsing()
    {
        bool usingbtn = false;
        auto mainPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : mainPairs)
        {
            if (pair.first == PickDataType::Node)
            {
                usingbtn = true;
                break;
            }
        }
        if (!usingbtn)
        {
            auto subPairs = getWidgetDataAsPickPairs(false);
            for (const auto& pair : subPairs)
            {
                if (pair.first == PickDataType::Node)
                {
                    usingbtn = true;
                    break;
                }
            }
        }
        nodeseting_.tablebtn->setDisabled(!usingbtn);
    }

    QSet<int> mConnectorFlexible::calMain2Subs(
        std::vector<std::pair<MxFunctions::PickDataType, int>>& areas,
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& parts)
    {
        QSet<int>nodesinparts;
        if (areas.empty() || parts.empty())
        {
            return nodesinparts;
        }
        int idx = poswaywid.combobox->currentIndex();


        //区域中的节点
        std::vector<int>nodeIDs;
        for (int i = 0; i < areas.size(); ++i)
        {
            nodeIDs.push_back(areas[i].second);
        }
        return nodesinparts;
    }

    void mConnectorFlexible::clearInputs()
    {
        rendFreeNodes(false);
        auto render = AllRenderSymbolManager::getInstance();
        if (_connectorData)
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_Flexible);

        poswaywid.clear();
        choosewid.clear();
        if (_mainPickController)
        {
            delete _mainPickController;
            _mainPickController = nullptr;
        }
        if (_subPickController)
        {
            delete _subPickController;
            _subPickController = nullptr;
        }
        for (auto& checks : checkswid)
        {
            checks.clear();
        }
        checkswid.clear();
        if (table_)
        {
            delete table_;
            table_ = nullptr;
            checkBoxHeader_ = nullptr;
            checkBoxDelegate_ = nullptr;
        }
    }

    void mConnectorFlexible::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(connectorData);
        _connectorData = connectorData;

        //位置
        poswaywid.combobox->blockSignals(true);
        poswaywid.combobox->setCurrentIndex(connectorPtr->way_);
        poswaywid.combobox->blockSignals(false);

        applyPickMode();
        applyAllowedPickTypes();

        // 绑定通用拾取控件持久化策略
        if (_mainPickController)
        {
            _mainPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorFlexible::subnodes_), &resolveSetToTypedData));
            _mainPickController->setSetResolver(&resolveSetToTypedData);
            _mainPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }
        if (_subPickController)
        {
            _subPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeTypedMapWriter(connectorPtr, &MechConnectorData::mConnectorFlexible::mainnodes_), &resolveSetToTypedData));
            _subPickController->setSetResolver(&resolveSetToTypedData);
            _subPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }

        // 反显主节点（subnodes_ 在数据中代表主节点 UI）
        if (_mainPickController)
        {
            PickWidgetData mainData;
            if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
            {
                MxFunctions::PickDataType setType = connectorPtr->subnodes_.first;
                mainData.pickedData[setType].push_back(connectorPtr->subnodes_.second);
            }
            _mainPickController->displayData(mainData);
        }

        // 反显从节点（mainnodes_ 在数据中代表从节点 UI）
        if (_subPickController)
        {
            PickWidgetData subData;
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); ++it)
            {
                MxFunctions::PickDataType setType = it->first;
                subData.pickedData[setType] = it->second;
            }
            _subPickController->displayData(subData, true);
        }

        // 默认激活从节点拾取控件
        if (_mainPickController && !_mainPickController->isPickActive())
        {
            _mainPickController->setPickActive(true);
        }

        // 反显后实时渲染连接符号，使切换回面板时立即可见。
        updateConnectorSymbolRealtime();

        //自由度：反显前先 blockSignals，避免 setChecked 触发 saveDegreeOfFreedom 把新连接数据覆盖
        for (auto& checks : checkswid)
        {
            for (auto* check : checks.labels)
            {
                if (check) check->blockSignals(true);
            }
        }

        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[0].labels[i - 1];
            check->setChecked(connectorPtr->degreeoffreedom_main.find(i) != connectorPtr->degreeoffreedom_main.end());
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[1].labels[i - 4];
            check->setChecked(connectorPtr->degreeoffreedom_main.find(i) != connectorPtr->degreeoffreedom_main.end());
        }
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[2].labels[i - 1];
            check->setChecked(connectorPtr->degreeoffreedom_sub.find(i) != connectorPtr->degreeoffreedom_sub.end());
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[3].labels[i - 4];
            check->setChecked(connectorPtr->degreeoffreedom_sub.find(i) != connectorPtr->degreeoffreedom_sub.end());
        }

        for (auto& checks : checkswid)
        {
            for (auto* check : checks.labels)
            {
                if (check) check->blockSignals(false);
            }
        }

        //权重
        if (connectorPtr->singleweight_ != -1)
        {
            weights_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->singleweight_));
        }

        //表格
        table_->clear();
        nodeseting_.tablebtn->setChecked(false);
        if (!connectorPtr->w_.empty() && !connectorPtr->dof_.empty())
        {
            QVector<QVector<QString>> datas;
            auto itNode = connectorPtr->mainnodes_.find(PickDataType::Node);
            if (itNode != connectorPtr->mainnodes_.end())
            {
                for (int i = 0, w_dof_idx = 0; i < itNode->second.size(); ++i)
                {
                    if (w_dof_idx >= connectorPtr->w_.size() || w_dof_idx >= connectorPtr->dof_.size())
                    {
                        continue;
                    }
                    int x = itNode->second[i].result_id;
                    QVector<QString>tmp;
                    tmp.push_back(QString::number(x));
                    tmp.push_back(QString::number(connectorPtr->w_[w_dof_idx]));
                    QVector<QString>degree;
                    QSet<int>dof;
                    for (auto d : connectorPtr->dof_[w_dof_idx])
                        dof.insert(d);
                    for (int j = 1; j <= 6; ++j)
                    {
                        if (dof.contains(j))
                        {
                            degree.append("1");
                        }
                        else
                        {
                            degree.append("");
                        }
                    }
                    tmp.append(degree);
                    datas.push_back(tmp);
                    ++w_dof_idx;
                }
            }

            table_->blockSignals(true);
            nodeseting_.tablebtn->blockSignals(true);
            nodeseting_.tablebtn->setChecked(connectorPtr->isTableMode_);
            nodeseting_.tablebtn->blockSignals(false);
            setValueToTable(datas);
            table_->blockSignals(false);

            if (connectorPtr->isTableMode_)
            {
                setConnectTbaleSig(true);
                nodeseting_.tablebtn->setStyleSheet(checkedStyle);
            }
            else
            {
                setConnectTbaleSig(false);
                nodeseting_.tablebtn->setStyleSheet(defaultStyle);
            }
        }

        updateTableBtnUsing();
        _main2subs.clear();

        // 确保差异化设置按钮可用性基于当前拾取数据同步一次
        nodeseting_.tablebtn->setEnabled(true);
        updateTableBtnUsing();
    }

    QVector<QWidget*> mConnectorFlexible::getPosInputs(FoldSection* fapos)
    {
        fapos_ = fapos;
        QVector<QWidget*>wds;
        wds.append(poswaywid.propwid);
        if (poswaywid.combobox->currentIndex() > 1)wds.append(choosewid.propwid);
        if (_mainPickController)
        {
            wds.append(_mainPickController->widget());
        }
        if (_subPickController)
        {
            wds.append(_subPickController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mConnectorFlexible::getWightsAndValsInputs(FoldSection* faweight)
    {
        famainweight_ = faweight;
        QVector<QWidget*>wds;
        wds.append(weights_.propwid);
        wds.append(checkswid[0].propwid);
        wds.append(checkswid[1].propwid);
        wds.append(nodeseting_.propwid);
        return wds;
    }

    QVector<QWidget*> mConnectorFlexible::getMainValInputs(FoldSection* famainval)
    {
        famainval_ = famainval;
        QVector<QWidget*>wds;
        return wds;
    }

    QVector<QWidget*> mConnectorFlexible::getSubValInputs(FoldSection* fasubval)
    {
        fasubval_ = fasubval;
        QVector<QWidget*>wds;
        wds.append(checkswid[2].propwid);
        wds.append(checkswid[3].propwid);
        return wds;
    }

    void mConnectorFlexible::saveWay()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->way_ = poswaywid.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveMainNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes;
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            mainnodes[pair.first].push_back(pair.second);
        }
        for (auto it = mainnodes.begin(); it != mainnodes.end(); )
        {
            if (it->second.empty())
            {
                it = mainnodes.erase(it);
            }
            else
            {
                ++it;
            }
        }
        connectorPtr->mainnodes_ = mainnodes;
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveSubNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        auto mainPairs = getWidgetDataAsPickPairs(true);
        if (!mainPairs.empty())
        {
            connectorPtr->subnodes_.first = mainPairs[0].first;
            connectorPtr->subnodes_.second = mainPairs[0].second;
        }
        else
        {
            connectorPtr->subnodes_.first = MxFunctions::PickDataType::None;
            connectorPtr->subnodes_.second = RenderPickResultData();
        }
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveWeight()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->singleweight_ = weights_.lineeditvalue->text().toDouble();
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveDegreeOfFreedom()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->degreeoffreedom_main.clear();
        connectorPtr->degreeoffreedom_sub.clear();
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[0].labels[i - 1];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[1].labels[i - 4];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid[2].labels[i - 1];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_sub.insert(i);
            }
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid[3].labels[i - 4];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_sub.insert(i);
            }
        }
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveTableData()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->w_.clear();
        connectorPtr->dof_.clear();
        if (nodeseting_.tablebtn->isChecked())
        {
            int rowCount = table_->rowCount();
            for (int i = 0; i < rowCount; ++i)
            {
                QTableWidgetItem* weightItem = table_->item(i, 1);
                if (weightItem && !weightItem->text().isEmpty())
                {
                    bool ok;
                    double weight = weightItem->text().toDouble(&ok);
                    connectorPtr->w_.push_back(ok ? weight : 0.0);
                }
                else
                {
                    connectorPtr->w_.push_back(0.0);
                }

                std::set<int> rowCheckboxData;
                for (int col = 2; col <= 7; ++col)
                {
                    QTableWidgetItem* item = table_->item(i, col);
                    if (item)
                    {
                        bool checked = item->data(Qt::CheckStateRole).toInt() == Qt::Checked;
                        if (checked)
                        {
                            rowCheckboxData.insert(col - 1);
                        }
                    }
                }
                connectorPtr->dof_.push_back(rowCheckboxData);
            }
        }
        notifyConnectorChanged();
    }

    void mConnectorFlexible::saveTableMode()
    {
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->isTableMode_ = nodeseting_.tablebtn->isChecked();
        notifyConnectorChanged();
    }

    void mConnectorFlexible::setConnectTbaleSig(bool on)
    {
        if (on)
        {
            connect(table_, &QTableWidget::cellChanged, this, &mConnectorFlexible::slot_tableEditFinished);
            mTableDataWindow::getInstance()->setCustomWidget(table_);
        }
        else
        {
            disconnect(table_, &QTableWidget::cellChanged, this, &mConnectorFlexible::slot_tableEditFinished);
            mTableDataWindow::getInstance()->resetInitialTable();
        }
    }

    void mConnectorFlexible::rendFreeNodes(bool showhide)
    {
        std::vector<int> tempIds;
        auto mainPairs = getWidgetDataAsPickPairs(true);
        if (!mainPairs.empty() && mainPairs[0].first == PickDataType::Node)
        {
            auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ mainPairs[0].second.result_id });
            if (!tempnodes.empty() && tempnodes[0])
            {
                tempIds.push_back(mainPairs[0].second.result_id);
            }
        }
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            if (pair.first == PickDataType::Node)
            {
                auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ pair.second.result_id });
                if (!tempnodes.empty() && tempnodes[0])
                {
                    tempIds.push_back(pair.second.result_id);
                }
            }
        }
        if (tempIds.empty())
            return;
        mDisplayMenuFunction::getInstance()->slot_showFreeNode(tempIds, showhide);
    }

    void mConnectorFlexible::deleteMainConnNodes()
    {
        std::vector<int> mainnodesid;
        int nodeid = -1;
        auto mainPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : mainPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);
            nodeid = pair.second.result_id;
        }
        std::shared_ptr<MechConnectorData::mConnectorFlexible> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorFlexible>(_connectorData);
        if (nodeid == -1 && connectorPtr) {
            nodeid = connectorPtr->subnodes_.second.result_id;
            mainnodesid.push_back(nodeid);
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(mainnodesid);
        if (nodeid != -1 && !getCVM->getModelData()->getMeshData()->NodeIsHaveByID(nodeid))
        {
            mDisplayMenuFunction::getInstance()->slot_showFreeNode({ nodeid }, false);
        }
    }

    void mConnectorFlexible::deleteSubConnNodes()
    {
        std::vector<int> subnodesid;
        int nodeid = -1;
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
            nodeid = pair.second.result_id;
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(subnodesid);
        if (nodeid != -1 && !getCVM->getModelData()->getMeshData()->NodeIsHaveByID(nodeid))
        {
            mDisplayMenuFunction::getInstance()->slot_showFreeNode({ nodeid }, false);
        }

        // 一对多模式下，若从节点已全部清空，同步删除自动计算的临时主节点
        // 注：自动计算的主节点现在走通用拾取控件高亮，不再单独加入 connectNodes，
        // 因此无需在这里删除 connectNodes。dataChanged 中已同步清空 _mainPickController。
    }

    void mConnectorFlexible::appendMainConnNodes()
    {
        std::vector<int> mainnodesid;
        auto mainPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : mainPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(mainnodesid);
    }

    void mConnectorFlexible::appendSubConnNodes()
    {
        std::vector<int> subnodesid;
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(subnodesid);
    }

    void mConnectorFlexible::updateConnectorSymbolRealtime()
    {
        if (!_mainPickController || !_subPickController || !_connectorData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        if (!meshData || !geoManager)
            return;

        auto posOf = [&](MxFunctions::PickDataType type, int id) -> QVector3D {
            if (type == MxFunctions::PickDataType::Node)
            {
                return meshData->getVectorById(id);
            }
            else if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto points = geoManager->CalculateVertexAttr({ id });
                if (!points.empty())
                    return points[0].m_coor;
            }
            return QVector3D();
            };

        // 主节点：按模式应唯一，取第一个有效对象
        QVector3D masterPos;
        bool hasMaster = false;
        const auto mainData = _mainPickController->getAllData();
        for (const auto& pair : mainData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                masterPos = posOf(type, d.result_id);
                hasMaster = true;
                break;
            }
            if (hasMaster)
                break;
        }

        // 从节点：汇总所有有效对象坐标
        QVector<QVector3D> slavePoss;
        const auto subData = _subPickController->getAllData();
        for (const auto& pair : subData.pickedData)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        slavePoss.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) slavePoss.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) slavePoss.push_back(c);
                break;
            }
            default:
                break;
            }
        }

        auto render = AllRenderSymbolManager::getInstance();
        if (hasMaster && !slavePoss.isEmpty())
        {
            render->assignConnector(_connectorData, masterPos, slavePoss, ConnectorType::Connector_Flexible);
        }
        else
        {
            // 数据不完整时清除实时符号，避免残留旧连线
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_Flexible);
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorFlexible::getWidgetDataAsPickPairs(bool isMain) const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        mPickWidgetController* ctrl = isMain ? _mainPickController : _subPickController;
        if (!ctrl) return result;
        const auto& allData = ctrl->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    std::vector<std::pair<MxFunctions::PickDataType, int>> mConnectorFlexible::getWidgetDataAsPickIds(bool isMain) const
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>> result;
        mPickWidgetController* ctrl = isMain ? _mainPickController : _subPickController;
        if (!ctrl) return result;
        const auto& allData = ctrl->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d.result_id);
            }
        }
        return result;
    }

    void mConnectorFlexible::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _mainPickController, _subPickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    /*************************************************MPC约束***************************************************************/
    mConnectorMPC::mConnectorMPC(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorMPC::~mConnectorMPC()
    {
        clearInputs();
    }

    void mConnectorMPC::saveSubNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorMPC> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        auto mainPairs = getWidgetDataAsPickPairs(true);
        if (!mainPairs.empty())
        {
            connectorPtr->subnodes_.first = mainPairs[0].first;
            connectorPtr->subnodes_.second = mainPairs[0].second;
        }
        else
        {
            connectorPtr->subnodes_.first = MxFunctions::PickDataType::None;
            connectorPtr->subnodes_.second = RenderPickResultData();
        }
        notifyConnectorChanged();
    }

    void mConnectorMPC::saveMainNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorMPC> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes;
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            mainnodes[pair.first].push_back(pair.second);
        }
        for (auto it = mainnodes.begin(); it != mainnodes.end(); )
        {
            if (it->second.empty())
            {
                it = mainnodes.erase(it);
            }
            else
            {
                ++it;
            }
        }
        connectorPtr->mainnodes_ = mainnodes;
        notifyConnectorChanged();
    }

    void mConnectorMPC::initInputs()
    {
        // 通用拾取控件：从节点位置（单选，几何点/线/面/实体/节点/部件）
        _subPickController = mGeneralPickUp::createOnePickWidget(CBBlue);
        _subPickController->setDefultTip("从节点位置");
        _subPickController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        connect(_subPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_subPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteSubConnNodes();
            appendSubConnNodes();
            saveSubNode();
           // rendFreeNodes(true);
            updateConnectorSymbolRealtime();
            // 从节点位置为单节点拾取，拾取完成后自动切换到主节点拾取控件（与刚性连接一致）
            if (getWidgetDataAsPickPairs(true).size() > 0)
            {
                _mainPickController->setPickActive(true);
            }
            notifyConnectorChanged();
            });

        // 通用拾取控件：主节点位置（多选）
        _mainPickController = mGeneralPickUp::createMorePickWidget(CBRed);
        _mainPickController->setDefultTip("主节点位置");
        _mainPickController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        connect(_mainPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_mainPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteMainConnNodes();
            appendMainConnNodes();
            // 从节点为空且主节点非空时自动计算形心临时节点作为从节点
            //calMainNode();
            updateConnectorSymbolRealtime();
            saveMainNode();
            updateTableBtnUsing();
            if (nodeseting_.tablebtn->isChecked())
                slot_nodeSetting();
            notifyConnectorChanged();
            });
        connect(_mainPickController, &mPickWidgetController::calcMainNodeSig, this, [this]() {
            calMainNode();
            });

        //参数
        names_ = QStringList{ "U1","U2","U3" ,"UR1","UR2","UR3" };
        for (auto name : names_)
        {
            CheckWithInput inp;
            inp.initWidget(name);
            inp.lineeditvalue->setDisabled(true);
            connect(inp.labelname, &QCheckBox::stateChanged, this, [this, inp](int i) {
                inp.lineeditvalue->setDisabled(i == 0);
                saveDefaultWeights();
                });
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveDefaultWeights();
                });
            freedomandweights.append(inp);
        }
        //初始化表格
        nodeseting_.initWidget("对每个节点进行差异化设置");
        nodeseting_.tablebtn->setCheckable(true);
        nodeseting_.tablebtn->setChecked(false);
        //nodeseting_.tablebtn->setStyleSheet(
        //	"QPushButton {"
        //	"background-color: #ADD8E6;"  // 浅蓝色
        //	"border: 1px solid #4682B4;"  // 蓝色边框
        //	"color: black;"
        //	"padding: 4px;"
        //	"}"
        //	"QPushButton:pressed {"
        //	"background-color: #4682B4;"  // 深蓝色按下效果
        //	"}");

        connect(nodeseting_.tablebtn, &QPushButton::toggled, [=](bool checked) {
            for (auto& inp : freedomandweights)
            {
                inp.labelname->setDisabled(checked);
                inp.lineeditvalue->setDisabled(checked);
                if (!checked && !inp.labelname->isChecked())
                    inp.lineeditvalue->setDisabled(true);
            }
            if (checked) {
                slot_nodeSetting();
                setConnectTbaleSig(true);
                // 勾选状态：深蓝色
                nodeseting_.tablebtn->setStyleSheet(
                    checkedStyle);
            }
            else
            {
                setConnectTbaleSig(false);
                // 未勾选状态：淡蓝色
                nodeseting_.tablebtn->setStyleSheet(
                    defaultStyle);
            }
            });

        //表格连接
        initTable();
        setConnectTbaleSig(false);
        nodeseting_.tablebtn->setStyleSheet(
            defaultStyle);

        //从节点自由度
        combo_.initWidget("自由度");
        combo_.labelname->setFixedWidth(90);
        for (auto name : names_)
        {
            combo_.combobox->addItem(name);
        }
        combo_.combobox->setCurrentIndex(0);
        connect(combo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this]() {
                saveSubDof();
            });

        //从节点权重
        inputwid_.initWidget("权重", false);
        connect(inputwid_.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveSubWeight();
            });

        //coefficient
        coefficient_.initWidget("非齐次项的系数", false);
        connect(coefficient_.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveCoefficient();
            });
    }


    void mConnectorMPC::initTable()
    {
        table_ = new mConnectorTableWidget();
        table_->setRowCount(0);
        table_->setColumnCount(7);
        table_->verticalHeader()->setHidden(false);
        table_->resizeColumnsToContents();
        // 多选单元格，允许直接编辑
        table_->setSelectionBehavior(QAbstractItemView::SelectItems);
        table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
        table_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);
        QStringList headerText;
        headerText << QString("节点ID") << QString("权重U1") << QString("权重U2") << QString("权重U3")
            << QString("权重UR1") << QString("权重UR2") << QString("权重UR3");
        table_->setHorizontalHeaderLabels(headerText);
        for (int c = 0; c < 7; ++c)
        {
            table_->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
        // 多选下单击不取消已有选区，直接进入编辑
        connect(table_, &QTableWidget::cellClicked, this, &mConnectorMPC::slot_cellClickedKeepSelection);
    }

    void mConnectorMPC::setValueToTable(const QVector<QVector<double>>& data)
    {
        table_->setRowCount(data.size());
        for (int i = 0; i < data.size(); ++i)
        {
            //节点ID
            QTableWidgetItem* item1 = new QTableWidgetItem(MxFunctions::mNumberToString((int)data[i][0]));
            item1->setTextAlignment(Qt::AlignCenter);
            table_->setItem(i, 0, item1);
            for (int j = 1; j < data[i].size(); ++j)
            {
                //权重
                QTableWidgetItem* item2 = new QTableWidgetItem(data[i][j] == -1 ? "" : MxFunctions::mNumberToString(data[i][j]));
                item2->setTextAlignment(Qt::AlignCenter);
                table_->setItem(i, j, item2);
            }
        }
        nodeseting_.tablebtn->setProperty("weights", QVariant::fromValue(data));
    }


    void mConnectorMPC::calMainNode()
    {
        // 从节点位置已有数据时不自动计算
        auto subPairs = getWidgetDataAsPickPairs(true);
        if (!subPairs.empty())
        {
            return;
        }

        auto mainPairs = getWidgetDataAsPickPairs(false);
        if (mainPairs.empty())
        {
            return;
        }

        int calnodeid = calculateMasterNode(mainPairs);
        if (calnodeid == -1)
        {
            return;
        }

        // 计算出的形心临时节点直接写入业务数据 subnodes_，并交给通用拾取控件高亮渲染
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }

        RenderPickResultData nodeData{ calnodeid, -1, -1 };
        connectorPtr->subnodes_.first = PickDataType::Node;
        connectorPtr->subnodes_.second = nodeData;

        PickWidgetData data;
        data.pickedData[MxFunctions::PickDataType::Node].push_back(nodeData);
        if (_subPickController)
        {
            _subPickController->displayData(data, true);
        }

        deleteSubConnNodes();
        //rendFreeNodes(true);
        appendSubConnNodes();
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorMPC::clearInputs()
    {
        if (_subPickController)
        {
            delete _subPickController;
            _subPickController = nullptr;
        }
        if (_mainPickController)
        {
            delete _mainPickController;
            _mainPickController = nullptr;
        }
        for (auto dfwt : freedomandweights)
        {
            dfwt.clear();
        }
        nodeseting_.clear();
        combo_.clear();
        inputwid_.clear();
        coefficient_.clear();
    }

    void mConnectorMPC::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorMPC> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(connectorData);
        if (!connectorPtr)
            return;
        _connectorData = connectorData;

        nodeseting_.tablebtn->blockSignals(true);
        nodeseting_.tablebtn->setChecked(false);
        for (auto& inp : freedomandweights)
        {
            inp.labelname->setDisabled(false);
            inp.lineeditvalue->setDisabled(false);
        }
        setConnectTbaleSig(false);
        // 未勾选状态：禁用
        nodeseting_.tablebtn->setStyleSheet(
            defaultStyle);
        nodeseting_.tablebtn->blockSignals(false);

        // 绑定通用拾取控件持久化策略：
        // - 从节点位置 -> subnodes_（单值）
        // - 主节点位置 -> mainnodes_（按类型分组）
        if (_subPickController)
        {
            _subPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorMPC::subnodes_), &resolveSetToTypedData));
            _subPickController->setSetResolver(&resolveSetToTypedData);
            _subPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }
        if (_mainPickController)
        {
            _mainPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeTypedMapWriter(connectorPtr, &MechConnectorData::mConnectorMPC::mainnodes_), &resolveSetToTypedData));
            _mainPickController->setSetResolver(&resolveSetToTypedData);
            _mainPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }

        // 反显从节点位置（subnodes_）
        if (_subPickController)
        {
            PickWidgetData subData;
            if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
            {
                MxFunctions::PickDataType setType = connectorPtr->subnodes_.first;
                subData.pickedData[setType].push_back(connectorPtr->subnodes_.second);
            }
            _subPickController->displayData(subData, true);
        }

        // 反显主节点位置（mainnodes_）
        if (_mainPickController)
        {
            PickWidgetData mainData;
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); ++it)
            {
                MxFunctions::PickDataType setType = it->first;
                mainData.pickedData[setType] = it->second;
            }
            _mainPickController->displayData(mainData, true);
        }

        // 默认激活从节点拾取控件
        if (_subPickController && !_subPickController->isPickActive())
        {
            _subPickController->setPickActive(true);
        }

        // 反显后实时渲染连接符号
        updateConnectorSymbolRealtime();

        //从节点自由度
        combo_.combobox->blockSignals(true);
        combo_.combobox->setCurrentIndex(connectorPtr->subDof_);
        combo_.combobox->blockSignals(false);
        inputwid_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->subWeights_));

        //主节点默认自由度（反显前阻断信号，避免覆盖数据）
        for (auto& inp : freedomandweights)
        {
            inp.labelname->blockSignals(true);
            inp.lineeditvalue->blockSignals(true);
        }
        for (int i = 0; i < 6; ++i)
        {
            if (connectorPtr->defaultweights_.contains(i))
            {
                freedomandweights[i].labelname->setChecked(true);
                freedomandweights[i].lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->defaultweights_[i]));
            }
            else
            {
                freedomandweights[i].labelname->setChecked(false);
                freedomandweights[i].lineeditvalue->setExpression("");
            }
        }
        for (auto& inp : freedomandweights)
        {
            inp.labelname->blockSignals(false);
            inp.lineeditvalue->blockSignals(false);
        }
        if (!connectorPtr->mainweights_.empty())//bug: MPC1表格设置后 MPC2 不设置表格，勾选 点击MPC1后点击MPC2崩溃
        {
            for (auto& inp : freedomandweights)
            {
                inp.labelname->setDisabled(true);
                inp.lineeditvalue->setDisabled(true);
            }

            QVector<QVector<double>>ws;
            auto mainnodes = connectorPtr->mainnodes_[PickDataType::Node];
            for (int i = 0; i < connectorPtr->mainweights_.size(); ++i)
            {
                QVector<double>w;
                w.push_back(mainnodes[i].result_id);
                for (int j = 0; j < 6; ++j)
                {
                    if (!connectorPtr->mainweights_[i][j].isNull() && connectorPtr->mainweights_[i][j].isValid())
                    {
                        w.push_back(connectorPtr->mainweights_[i][j].toDouble());
                    }
                    else
                    {
                        w.push_back(-1);
                    }
                }
                ws.push_back(w);
            }
            nodeseting_.tablebtn->setProperty("weights", QVariant::fromValue(ws));
            table_->blockSignals(true);
            setValueToTable(ws);
            table_->blockSignals(false);
            nodeseting_.tablebtn->blockSignals(true);
            nodeseting_.tablebtn->setChecked(true);
            for (auto& inp : freedomandweights)
            {
                inp.labelname->setDisabled(true);
                inp.lineeditvalue->setDisabled(true);
            }
            setConnectTbaleSig(true);
            // 勾选状态：深蓝色
            nodeseting_.tablebtn->setStyleSheet(
                checkedStyle);
            nodeseting_.tablebtn->blockSignals(false);
            mDockLayoutManager::getInstance()->activateDock("Table");
        }
        updateTableBtnUsing();


        coefficient_.lineeditvalue->setExpression("");
        if (!connectorPtr->coefficient_.isNull() && connectorPtr->coefficient_.isValid())
        {
            coefficient_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->coefficient_.toDouble()));
        }
    }

    void mConnectorMPC::slot_nodeSetting()
    {
        QVector<QVector<double>> weights;

        QVector<double> V;
        for (int i = 0; i < freedomandweights.size(); ++i)
        {
            int w = -1;
            if (freedomandweights[i].labelname->isChecked())
            {
                QString v = freedomandweights[i].lineeditvalue->text().trimmed();
                if (!v.isEmpty())
                {
                    w = v.toDouble();
                }
            }
            V.push_back(w);
        }

        auto mainPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : mainPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            int x = pair.second.result_id;
            QVector<double>tmp;
            tmp.push_back(x);
            tmp.append(V);
            weights.push_back(tmp);
        }
        nodeseting_.tablebtn->setProperty("weights", QVariant::fromValue(weights));

        table_->blockSignals(true);
        setValueToTable(weights);
        table_->blockSignals(false);

        mDockLayoutManager::getInstance()->activateDock("Table");
        saveTableWeights();
    }

    void mConnectorMPC::slot_tableEditFinished(int row, int col)
    {
        if (col < 0 || col >= table_->columnCount() || row < 0 || row >= table_->rowCount())
            return;

        QTableWidgetItem* currentItem = table_->item(row, col);
        if (!currentItem)
            return;

        QString newText = currentItem->text().trimmed();
        bool ok = false;
        double newValue = newText.toDouble(&ok);
        if (!ok && !newText.isEmpty())
        {
            newText = "";
            newValue = -1;
        }

        QVariant value = nodeseting_.tablebtn->property("weights");
        auto weights = value.value<QVector<QVector<double>>>();

        // 多选批量修改：同步所有被选中的同列单元格
        bool blocked = table_->signalsBlocked();
        table_->blockSignals(true);
        QList<QTableWidgetItem*> selectedItems = table_->selectedItems();
        for (QTableWidgetItem* item : selectedItems)
        {
            if (item && item->column() == col && item->row() != row)
            {
                item->setText(newText);
            }
        }
        table_->blockSignals(blocked);

        if (!weights.isEmpty() && row < weights.size() && col < weights[row].size())
        {
            for (QTableWidgetItem* item : selectedItems)
            {
                if (item && item->column() == col)
                {
                    int r = item->row();
                    if (r >= 0 && r < weights.size() && col < weights[r].size())
                    {
                        weights[r][col] = newText.isEmpty() ? -1 : newValue;
                    }
                }
            }
            weights[row][col] = newText.isEmpty() ? -1 : newValue;
            nodeseting_.tablebtn->setProperty("weights", QVariant::fromValue(weights));
        }
        else if (weights.isEmpty())
        {
            // 备用：从表格重建weights
            int rowCount = table_->rowCount();
            QVector<QVector<double>> rebuilt;
            for (int i = 0; i < rowCount; ++i)
            {
                QVector<double>w;
                for (int j = 0; j < 7; ++j)
                {
                    QTableWidgetItem* weightItem = table_->item(i, j);
                    if (weightItem && !weightItem->text().isEmpty())
                    {
                        bool ok2;
                        double weight = weightItem->text().toDouble(&ok2);
                        w.push_back(ok2 ? weight : -1);
                    }
                    else
                    {
                        w.push_back(-1);
                    }
                }
                rebuilt.push_back(w);
            }
            nodeseting_.tablebtn->setProperty("weights", QVariant::fromValue(rebuilt));
        }

        saveTableWeights();
    }

    QVector<QWidget*> mConnectorMPC::getPosInputs(FoldSection* fapos)
    {
        fapos_ = fapos;
        QVector<QWidget*>wds;
        if (_subPickController)
        {
            wds.append(_subPickController->widget());
        }
        if (_mainPickController)
        {
            wds.append(_mainPickController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mConnectorMPC::getValInputs(FoldSection* faval)
    {
        faval_ = faval;
        QVector<QWidget*>wds;
        for (auto inp : freedomandweights)
        {
            wds.append(inp.propwid);
        }
        wds.append(nodeseting_.tablebtn);
        return wds;
    }

    QVector<QWidget*> mConnectorMPC::getValInputs2(FoldSection* faval)
    {
        faval2_ = faval;
        QVector<QWidget*>wds;
        wds.append(combo_.propwid);
        wds.append(inputwid_.propwid);
        return wds;
    }

    QVector<QWidget*> mConnectorMPC::getValInputs3(FoldSection* faval)
    {
        faval3_ = faval;
        QVector<QWidget*>wds;
        wds.append(coefficient_.propwid);
        return wds;
    }

    void mConnectorMPC::saveSubDof()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->subDof_ = combo_.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorMPC::saveSubWeight()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->subWeights_ = inputwid_.lineeditvalue->text().toDouble();
        notifyConnectorChanged();
    }

    void mConnectorMPC::saveDefaultWeights()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        QHash<int, double> defaultweights;
        for (int i = 0; i < 6; ++i)
        {
            auto inp = freedomandweights[i];
            if (inp.labelname->isChecked())
            {
                defaultweights[i] = inp.lineeditvalue->text().toDouble();
            }
        }
        connectorPtr->defaultweights_ = defaultweights;
        notifyConnectorChanged();
    }

    void mConnectorMPC::saveTableWeights()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->mainweights_.clear();
        if (nodeseting_.tablebtn->isChecked())
        {
            QVariant mainweights = nodeseting_.tablebtn->property("weights");
            auto weights = mainweights.value<QVector<QVector<double>>>();
            std::vector<std::vector<QVariant>>ws;
            for (int i = 0; i < weights.size(); ++i)
            {
                std::vector<QVariant>w;
                for (int j = 1; j <= 6; ++j)
                {
                    if (weights[i][j] == -1)
                    {
                        w.push_back(QVariant());
                    }
                    else
                    {
                        w.push_back(QVariant::fromValue<double>(weights[i][j]));
                    }
                }
                ws.push_back(w);
            }
            connectorPtr->mainweights_ = ws;
        }
        else
        {
            std::vector<std::vector<QVariant>>ws;
            std::vector<QVariant>w;
            auto mainNodes = connectorPtr->mainnodes_[PickDataType::Node];
            for (int i = 0; i < 6; ++i)
            {
                auto inp = freedomandweights[i];
                if (inp.labelname->isChecked())
                {
                    w.push_back(QVariant::fromValue<double>(inp.lineeditvalue->text().toDouble()));
                }
                else
                {
                    w.push_back(QVariant());
                }
            }
            for (int i = 0; i < mainNodes.size(); ++i)
            {
                ws.push_back(w);
            }
            connectorPtr->mainweights_ = ws;
        }
        notifyConnectorChanged();
    }

    void mConnectorMPC::saveCoefficient()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->coefficient_ = QVariant();
        if (!coefficient_.lineeditvalue->text().isEmpty())
        {
            connectorPtr->coefficient_ = coefficient_.lineeditvalue->text().toDouble();
        }
        notifyConnectorChanged();
    }

    void mConnectorMPC::setConnectTbaleSig(bool on)
    {
        if (on)
        {
            connect(table_, &QTableWidget::cellChanged, this, &mConnectorMPC::slot_tableEditFinished);
            mTableDataWindow::getInstance()->setCustomWidget(table_);
        }
        else
        {
            disconnect(table_, &QTableWidget::cellChanged, this, &mConnectorMPC::slot_tableEditFinished);
            mTableDataWindow::getInstance()->resetInitialTable();
        }
    }

    void mConnectorMPC::rendFreeNodes(bool showhide)
    {
        std::vector<int> tempIds;
        auto collectTemp = [&](const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pairs) {
            for (const auto& pair : pairs)
            {
                if (pair.first != PickDataType::Node)
                {
                    continue;
                }
                auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ pair.second.result_id });
                if (!tempnodes.empty() && tempnodes[0])
                {
                    tempIds.push_back(pair.second.result_id);
                }
            }
            };
        collectTemp(getWidgetDataAsPickPairs(true));
        collectTemp(getWidgetDataAsPickPairs(false));
        if (tempIds.empty())
            return;
        mDisplayMenuFunction::getInstance()->slot_showFreeNode(tempIds, showhide);
    }

    void mConnectorMPC::updateTableBtnUsing()
    {
        bool usingbtn = false;
        auto subPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : subPairs)
        {
            if (pair.first == PickDataType::Node)
            {
                usingbtn = true;
                break;
            }
        }
        if (!usingbtn)
        {
            auto mainPairs = getWidgetDataAsPickPairs(false);
            for (const auto& pair : mainPairs)
            {
                if (pair.first == PickDataType::Node)
                {
                    usingbtn = true;
                    break;
                }
            }
        }
        nodeseting_.tablebtn->setDisabled(!usingbtn);
    }

    void mConnectorMPC::deleteMainConnNodes()
    {
        std::vector<int>mainnodesid;
        int nodeid = -1;
        auto mainPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : mainPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);
            nodeid = pair.second.result_id;
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(mainnodesid);
        if (nodeid != -1 && !getCVM->getModelData()->getMeshData()->NodeIsHaveByID(nodeid))
        {
            mDisplayMenuFunction::getInstance()->slot_showFreeNode({ nodeid }, false);
        }
    }

    void mConnectorMPC::deleteSubConnNodes()
    {
        std::vector<int>subnodesid;
        int nodeid = -1;
        auto subPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
            nodeid = pair.second.result_id;
        }
        std::shared_ptr<MechConnectorData::mConnectorMPC> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorMPC>(_connectorData);
        if (nodeid == -1 && connectorPtr) {
            nodeid = connectorPtr->subnodes_.second.result_id;
            subnodesid.push_back(nodeid);
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(subnodesid);
        if (nodeid != -1 && !getCVM->getModelData()->getMeshData()->NodeIsHaveByID(nodeid))
        {
            mDisplayMenuFunction::getInstance()->slot_showFreeNode({ nodeid }, false);
        }
    }

    void mConnectorMPC::appendMainConnNodes()
    {
        std::vector<int>mainnodesid;
        auto mainPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : mainPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(mainnodesid);
    }

    void mConnectorMPC::appendSubConnNodes()
    {
        std::vector<int>subnodesid;
        auto subPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : subPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(subnodesid);
    }

    void mConnectorMPC::slot_cellClickedKeepSelection(int row, int col)
    {
        if (col < 1 || col >= table_->columnCount())
            return;
        QTableWidgetItem* item = table_->item(row, col);
        if (!item)
            return;
        // 已有其他单元格被选中时，单击不取消选区，直接打开编辑器
        if (table_->selectedItems().size() > 1)
        {
            table_->editItem(item);
        }
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorMPC::getWidgetDataAsPickPairs(bool isSub) const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        mPickWidgetController* ctrl = isSub ? _subPickController : _mainPickController;
        if (!ctrl) return result;
        const auto& allData = ctrl->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    void mConnectorMPC::updateConnectorSymbolRealtime()
    {
        if (!_subPickController || !_mainPickController || !_connectorData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        if (!meshData || !geoManager)
            return;

        auto posOf = [&](MxFunctions::PickDataType type, int id) -> QVector3D {
            if (type == MxFunctions::PickDataType::Node)
            {
                return meshData->getVectorById(id);
            }
            else if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto points = geoManager->CalculateVertexAttr({ id });
                if (!points.empty())
                    return points[0].m_coor;
            }
            return QVector3D();
            };

        // 从节点：按模式应唯一，取第一个有效对象
        QVector3D subPos;
        bool hasSub = false;
        const auto subData = _subPickController->getAllData();
        for (const auto& pair : subData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                subPos = posOf(type, d.result_id);
                hasSub = true;
                break;
            }
            if (hasSub)
                break;
        }

        // 主节点：汇总所有有效对象坐标
        QVector<QVector3D> mainPoss;
        const auto mainData = _mainPickController->getAllData();
        for (const auto& pair : mainData.pickedData)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        mainPoss.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) mainPoss.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) mainPoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) mainPoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) mainPoss.push_back(c);
                break;
            }
            default:
                break;
            }
        }

        auto render = AllRenderSymbolManager::getInstance();
        if (hasSub && !mainPoss.isEmpty())
        {
            render->assignConnector(_connectorData, subPos, mainPoss, ConnectorType::Connector_MPC);
        }
        else
        {
            // 数据不完整时清除实时符号，避免残留旧连线
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_MPC);
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mConnectorMPC::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _subPickController, _mainPickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    mConnectingDisperse::mConnectingDisperse(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectingDisperse::~mConnectingDisperse()
    {
        clearInputs();
    }

    void mConnectingDisperse::initInputs()
    {
        // 参考点：通用拾取控件，单点，仅允许几何点/节点
        _refPointPickController = mGeneralPickUp::createOneDataWidget(CBBlue);
        _refPointPickController->setDefultTip("参考点");
        _refPointPickController->setAllowedTypes(&pickGeoPointAndNode);
        connect(_refPointPickController, &mPickWidgetController::pickActiveChanged,
            this, &mConnectingDisperse::slot_refPointPickActiveChanged);
        connect(_refPointPickController, &mPickWidgetController::dataChanged,
            this, [this]() {
                notifyConnectorChanged();
            });

        // 刚体单元集合下拉框
        _cellSetCombo.initWidget("选择集合");
        connect(_cellSetCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectingDisperse::saveCellSet);
    }

    void mConnectingDisperse::clearInputs()
    {
        if (_refPointPickController)
        {
            delete _refPointPickController;
            _refPointPickController = nullptr;
        }
        _cellSetCombo.clear();
    }

    void mConnectingDisperse::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorDisperse> connectorPtr =
            std::dynamic_pointer_cast<MechConnectorData::mConnectorDisperse>(connectorData);
        if (!connectorPtr)
            return;

        _connectorData = connectorData;

        // 绑定参考点持久化策略：直接写入 mConnectorDisperse::_mainnodes_id
        if (_refPointPickController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(MxFunctions::Physical_Connector, connectorPtr->getConnectorId(), MapRelation_Slave);
            _refPointPickController->setPersistenceStrategy(strategy);

            _refPointPickController->setSetResolver(&resolveSetToTypedData);
            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::Node });
            setToType = getOneDataSet(setToType);
            _refPointPickController->setSelectSetList(setToType);
        }

        // 反显参考点
        //获取节点集合
        PickWidgetData displayData;
        int setID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Slave);
        if (setID != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
            if (set && set->getInternal())
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (mixset)
                    displayData.pickedData = mixset->getMixSetData();
            }
            else if (set)
            {
                displayData.selectedSetNames = { set->getSetName() };
            }
        }

        if (_refPointPickController)
        {
            _refPointPickController->displayData(displayData, true);
            _refPointPickController->setPickActive(true);
        }

        // 获取并刷新单元集合下拉框
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int masterSetId = mapload.getMapRelation<int>(
            { Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Master);
        QString currentSetName;
        if (masterSetId != -1)
        {
            currentSetName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(masterSetId);
        }

        _cellSetCombo.combobox->blockSignals(true);
        _cellSetCombo.combobox->clear();
        QStringList setlist = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set)
            .getAllSetNamesByTypes(MxFunctions::SetType::Element);
        for (const auto& setname : setlist)
        {
            _cellSetCombo.combobox->addItem(setname);
        }
        if (!currentSetName.isEmpty() && setlist.contains(currentSetName))
        {
            _cellSetCombo.combobox->setCurrentText(currentSetName);
        }
        else
        {
            saveCellSet();
        }
        _cellSetCombo.combobox->blockSignals(false);
    }

    QVector<QWidget*> mConnectingDisperse::getReferencePoint(FoldSection* value)
    {
        referencePoint_ = value;
        QVector<QWidget*> wds;
        if (_refPointPickController)
            wds.append(_refPointPickController->widget());
        return wds;
    }

    QVector<QWidget*> mConnectingDisperse::getRigidBody(FoldSection* value)
    {
        rigidBody_ = value;
        QVector<QWidget*> wds;
        wds.append(_cellSetCombo.propwid);
        return wds;
    }

    void mConnectingDisperse::saveCellSet()
    {
        std::shared_ptr<MechConnectorData::mConnectorDisperse> dispersePtr =
            std::dynamic_pointer_cast<MechConnectorData::mConnectorDisperse>(_connectorData);
        if (!dispersePtr)
            return;

        // 保存单元集合映射（Master）
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        QString setName = _cellSetCombo.combobox->currentText();
        if (!setName.isEmpty())
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setName);
            if (set)
            {
                set->setInternal(false);
                mapload.appendMapRelation(
                    { Physical_Connector, dispersePtr->getConnectorId() },
                    { Physical_Set, set->getSetId() },
                    MapRelation_Master);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(setName + QString("集合不存在！"));
            }
        }
        notifyConnectorChanged();
    }

    void mConnectingDisperse::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        if (_refPointPickController)
        {
            _refPointPickController->setKeepHighlightOnDeactivate(false);
            const bool wasActive = _refPointPickController->isPickActive();
            _refPointPickController->setPickActive(false);
            if (renderValid && !wasActive)
            {
                _refPointPickController->clearAllHighlight();
            }
            _refPointPickController->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    void mConnectingDisperse::slot_refPointPickActiveChanged(bool active)
    {
        if (active)
            emit mMechGlobalSignals::getInstance()->activePickSig(5);
    }

    void mConnectingDisperse::slot_cellSetChanged(int idx)
    {
        Q_UNUSED(idx)
            // 信号已直接连接 saveCellSet，此处保留为空槽以避免编译警告
    }

    /*************************************************解析刚体***************************************************************/
    mConnectorRigidbodyPanel::mConnectorRigidbodyPanel(QWidget* parent)
    {
        initInputs();
    }

    mConnectorRigidbodyPanel::~mConnectorRigidbodyPanel()
    {
        clearInputs();

    }

    void mConnectorRigidbodyPanel::initInputs()
    {
        // 参考点：通用拾取控件，单点，仅允许几何点/节点
        _refPointPickController = mGeneralPickUp::createOneDataWidget(CBBlue);
        _refPointPickController->setDefultTip("参考点");
        _refPointPickController->setAllowedTypes(&pickOnlyNode);
        connect(_refPointPickController, &mPickWidgetController::pickActiveChanged,
            this, &mConnectorRigidbodyPanel::slot_refPointPickActiveChanged);
        connect(_refPointPickController, &mPickWidgetController::dataChanged,
            this, [this]() {
                notifyConnectorChanged();
            });

        // 解析表面集合下拉框
        _cellSetCombo.initWidget("选择集合");
        connect(_cellSetCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorRigidbodyPanel::saveCellSet);
    }

    void mConnectorRigidbodyPanel::clearInputs()
    {
        if (_refPointPickController)
        {
            delete _refPointPickController;
            _refPointPickController = nullptr;
        }
        _cellSetCombo.clear();
    }

    void mConnectorRigidbodyPanel::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorRigidbody> connectorPtr =
            std::dynamic_pointer_cast<MechConnectorData::mConnectorRigidbody>(connectorData);
        if (!connectorPtr)
            return;

        _connectorData = connectorData;

        // 绑定参考点持久化策略：直接写入 mConnectorDisperse::_mainnodes_id
        if (_refPointPickController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(MxFunctions::Physical_Connector, connectorPtr->getConnectorId(), MapRelation_Slave);
            _refPointPickController->setPersistenceStrategy(strategy);

            _refPointPickController->setSetResolver(&resolveSetToTypedData);
            QHash<QString, QString> setToType = getSetNamesList({ SetType::Node }); 
            _refPointPickController->setSelectSetList(setToType);
        }

        // 反显参考点
        //获取节点集合
        PickWidgetData displayData;
        int setID = getCVM->getModelData()->getMapRelationManager().getMapRelation<int>({ Physical_Connector ,connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Slave);
        if (setID != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setID);
            if (set && set->getInternal())
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (mixset)
                    displayData.pickedData = mixset->getMixSetData();
            }
            else if (set)
            {
                displayData.selectedSetNames = { set->getSetName() };
            }
        }

        if (_refPointPickController)
        {
            _refPointPickController->displayData(displayData, true);
            _refPointPickController->setPickActive(true);
        }

        // 获取并刷新单元集合下拉框
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int masterSetId = mapload.getMapRelation<int>(
            { Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set, MapRelation_Master);
        QString currentSetName;
        if (masterSetId != -1)
        {
            currentSetName = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetNameByID(masterSetId);
        }

        _cellSetCombo.combobox->blockSignals(true);
        _cellSetCombo.combobox->clear();
        QStringList setlist = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set)
            .getAllSetNamesByTypes(MxFunctions::SetType::SurfaceAnalysis);
        for (const auto& setname : setlist)
        {
            _cellSetCombo.combobox->addItem(setname);
        }
        if (!currentSetName.isEmpty() && setlist.contains(currentSetName))
        {
            _cellSetCombo.combobox->setCurrentText(currentSetName);
        }
        else
        {
            saveCellSet();
        }
        _cellSetCombo.combobox->blockSignals(false);
    }

    QVector<QWidget*> mConnectorRigidbodyPanel::getReferencePoint(FoldSection* value)
    {
        referencePoint_ = value;
        QVector<QWidget*> wds;
        if (_refPointPickController)
            wds.append(_refPointPickController->widget());
        return wds;
    }

    QVector<QWidget*> mConnectorRigidbodyPanel::getSurfaceAnalysis(FoldSection* value)
    {
        surfaceBody_ = value;
        QVector<QWidget*> wds;
        wds.append(_cellSetCombo.propwid);
        return wds;
    }

    void mConnectorRigidbodyPanel::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        if (_refPointPickController)
        {
            _refPointPickController->setKeepHighlightOnDeactivate(false);
            const bool wasActive = _refPointPickController->isPickActive();
            _refPointPickController->setPickActive(false);
            if (renderValid && !wasActive)
            {
                _refPointPickController->clearAllHighlight();
            }
            _refPointPickController->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    void mConnectorRigidbodyPanel::slot_refPointPickActiveChanged(bool active)
    {
        if (active)
            emit mMechGlobalSignals::getInstance()->activePickSig(5);
    }

    void mConnectorRigidbodyPanel::slot_cellSetChanged(int idx)
    {
        Q_UNUSED(idx)
            // 信号已直接连接 saveCellSet，此处保留为空槽以避免编译警告
    }
    void mConnectorRigidbodyPanel::saveCellSet()
    {
        std::shared_ptr<MechConnectorData::mConnectorRigidbody> dispersePtr =
            std::dynamic_pointer_cast<MechConnectorData::mConnectorRigidbody>(_connectorData);
        if (!dispersePtr)
            return;

        // 保存单元集合映射（Master）
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        QString setName = _cellSetCombo.combobox->currentText();
        if (!setName.isEmpty())
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setName);
            if (set)
            {
                set->setInternal(false);
                mapload.appendMapRelation(
                    { Physical_Connector, dispersePtr->getConnectorId() },
                    { Physical_Set, set->getSetId() },
                    MapRelation_Master);
            }
            else
            {
                MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(setName + QString("集合不存在！"));
            }
        }
        notifyConnectorChanged();
    }
    /*************************************************螺栓约束***************************************************************/
    mConnectorBoltPanel::mConnectorBoltPanel(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorBoltPanel::~mConnectorBoltPanel()
    {
        clearInputs();
    }

    void mConnectorBoltPanel::initInputs()
    {
        // 类型
        _typeCombo.initWidget("类型");
        _typeCombo.combobox->addItems({ "一维螺栓", "三维螺栓" });
        connect(_typeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorBoltPanel::slot_typeDefineChanged);
        _typeCombo.combobox->setCurrentIndex(0);

        // 作用对象：通用拾取控件（支持集合与多选）
        _targetPickController = mGeneralPickUp::createPickWidget(CBBlue);
        _targetPickController->setDefultTip("选择对象");
        _targetPickController->setPickActive(true);
        connect(_targetPickController, &mPickWidgetController::pickActiveChanged,
            this, &mConnectorBoltPanel::slot_targetPickActiveChanged);
        connect(_targetPickController, &mPickWidgetController::dataChanged,
            this, &mConnectorBoltPanel::slot_targetDataChanged);
        // 一维螺栓：仅允许一维单元被拾取/显示
        _targetPickController->setPickFilter(QSet<int>{1});
        connect(_targetPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                if (_typeCombo.combobox->currentIndex() == 0)
                {
                    emit mMechGlobalSignals::getInstance()->activePickSig(8);
                }
                else
                {
                    emit mMechGlobalSignals::getInstance()->activePickSig(7);
                }
            }
            });

        // 预紧方向：折叠框标题自带勾选框控制启用（参考重力局部施加），
        // 内部统一使用方向控件 mDirectionSelectorWidget，懒创建于 ensureDirectionWidget
    }

    void mConnectorBoltPanel::clearInputs()
    {
        _typeCombo.clear();
        if (_targetPickController)
        {
            _targetPickController->setPickActive(false);
            delete _targetPickController;
            _targetPickController = nullptr;
        }
        if (_directionWidget)
        {
            _directionWidget->hide();
            delete _directionWidget;
            _directionWidget = nullptr;
        }
        _pickeddata.clear();//清空容器
    }

    void mConnectorBoltPanel::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(connectorData);
        if (!connectorPtr)
            return;

        _connectorData = connectorData;

        // 方向控件重置到默认状态（内部已阻断信号，不会触发保存）
        if (_directionWidget)
        {
            _directionWidget->resetToDefault();
        }

        // 反显预紧方向控件完整UI状态（首次进入面板时控件可能尚未创建，
        // getVectorDefine 创建后由 applyDirStateToWidget 再次反显）
        applyDirStateToWidget(connectorPtr);

        _dir = connectorPtr->dir_.value<QVector3D>();

        _typeCombo.combobox->blockSignals(true);
        _typeCombo.combobox->setCurrentIndex(connectorPtr->type_);
        _typeCombo.combobox->blockSignals(false);

        // 按螺栓类型配置拾取白名单与集合列表
        applyAllowedPickTypes(connectorPtr->type_);

        if (_targetPickController)
        {
            _targetPickController->setPersistenceStrategy(
                std::make_shared<MapRelationPersistenceStrategy>(
                    Physical_Connector, connectorPtr->getConnectorId(), MapRelation_None));
            _targetPickController->setSetResolver(&resolveSetToTypedData);

            // 反显集合/拾取数据
            PickWidgetData widgetData;
            auto& mapload = getCVM->getModelData()->getMapRelationManager();
            int setid = mapload.getMapRelation<int>(
                { Physical_Connector, connectorPtr->getConnectorId() }, Physical_Set);
            if (setid != -1)
            {
                mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
                if (set)
                {
                    if (set->getSetType() == Mix)
                    {
                        auto mixset = dynamic_cast<mMixSet*>(set);
                        if (mixset)
                        {
                            // 一维螺栓：过滤单元集合，仅保留一维单元
                            widgetData.pickedData = filterBoltMixSetData(
                                mixset->getMixSetData(), connectorPtr->type_, getCVM->getModelData()->getMeshData());
                        }
                    }
                    else if (set->getSetType() == GeoEdge || set->getSetType() == GeoFace ||
                        set->getSetType() == Element)
                    {
                        widgetData.selectedSetNames.insert(set->getSetName());
                    }
                    else if (set->getSetType() == ElementSurface)
                    {
                        widgetData.selectedSetNames.insert(set->getSetName());
                    }
                }
            }
            _targetPickController->displayData(widgetData, true);

            // 进入面板默认激活拾取控件
            if (!_targetPickController->isPickActive())
            {
                _targetPickController->setPickActive(true);
            }
        }
        updateConnectorSymbolRealtime();
    }

    void mConnectorBoltPanel::applyDirStateToWidget(std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr)
    {
        if (!connectorPtr)
            return;

        // 控件尚未创建时先懒创建（依赖 vectorDefine_ 已知父折叠块）
        ensureDirectionWidget();
        if (!_directionWidget)
            return;

        const auto& state = connectorPtr->dirPickState_;

        // 反显过程中阻断信号，避免触发保存
        bool oldSignalsBlocked = _directionWidget->blockSignals(true);

        _directionWidget->setDirectionMode(
            static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
        _directionWidget->setReversed(state.reversed);

        // 自定义坐标反显
        if (state.mode == mDirectionSelectorWidget::CustomCoordinate &&
            state.startPoint.isValid() && !state.startPoint.isNull() &&
            state.endPoint.isValid() && !state.endPoint.isNull())
        {
            _directionWidget->setCustomCoordinatePoints(
                state.startPoint.value<QVector3D>(),
                state.endPoint.value<QVector3D>());
        }
        // 选择对象反显
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData displayData;

            // 有效拾取结果判空：通过 result_id 判断（id>0 视为有效拾取）
            if (state.selectedObject.second.result_id > 0)
            {
                // 用 pair 的 first 作 key，second 作 value，还原 map 结构适配控件接口
                displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
            }

            // 调用控件统一反显接口
            _directionWidget->objectPickController()->displayData(displayData);
        }
        // 直接输入反显
        else if (state.mode == mDirectionSelectorWidget::DirectInput)
        {
            _directionWidget->setDirection(state.directInputDir);
        }
        // 坐标轴模式自动根据 dir_ 推断方向
        else if (connectorPtr->dir_.isValid() && !connectorPtr->dir_.isNull())
        {
            _directionWidget->setDirection(connectorPtr->dir_.value<QVector3D>());
        }

        _directionWidget->blockSignals(oldSignalsBlocked);
    }

    void mConnectorBoltPanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        // 必须在已知父折叠块时创建，避免无 parent 的独立窗口短暂闪现
        if (!vectorDefine_)
            return;

        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("方向"), false, QString(), false,
            QString(), false, vectorDefine_->contentWidget());
        _directionWidget->setVisible(false);

        // 使用控件默认的方向/坐标计算器（基于几何属性）
        _directionWidget->setDirectionCalculator(
            mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(
            mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
            if (!connectorPtr)
            {
                return;
            }
            connectorPtr->x_ = QVariant();
            connectorPtr->y_ = QVariant();
            connectorPtr->z_ = QVariant();
            connectorPtr->dir_ = QVariant();

            // 预紧方向未勾选时整个折叠框内容不生效，方向数据清空
            if (!connectorPtr->isOpen_)
            {
                connectorPtr->dirPickState_ = DirectionPickState();
                mPickWidgetController::commitActivePickWidget();
                return;
            }

            // 从方向控件保存方向矢量与完整UI状态
            applyDirectionWidgetToConnector(connectorPtr);

            if (_dir.isNull())
            {
                _dir.setX(1.0);
            }
            connectorPtr->dir_ = QVariant::fromValue<QVector3D>(_dir);
            });

        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (vectorDefine_)
            {
                vectorDefine_->refreshLayout();
            }
            });

        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            // 方向控件当前方式页无拾取控件时，激活作用对象拾取控件
            if (_targetPickController)
            {
                _targetPickController->setPickActive(true);
            }
            });
    }

    void mConnectorBoltPanel::applyDirectionWidgetToConnector(std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr)
    {
        if (!connectorPtr || !_directionWidget)
        {
            return;
        }

        // 存方向矢量
        QVariant dirVar = _directionWidget->getDirection();
        if (dirVar.isValid() && !dirVar.isNull())
        {
            QVector3D dir = dirVar.value<QVector3D>();
            if (!dir.isNull())
            {
                dir.normalize();
                this->_dir = dir;
                connectorPtr->x_ = dir.x();
                connectorPtr->y_ = dir.y();
                connectorPtr->z_ = dir.z();
            }
        }

        // 存方向控件完整状态到结构体
        auto& state = connectorPtr->dirPickState_;
        state.mode = static_cast<int>(_directionWidget->directionMode());
        state.reversed = _directionWidget->isReversed();

        if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
        {
            state.startPoint = QVariant::fromValue(_directionWidget->startPoint());
            state.endPoint = QVariant::fromValue(_directionWidget->endPoint());
        }
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData pickData = _directionWidget->objectPickController()->getAllData();
            if (!pickData.pickedData.empty())
            {
                auto it = pickData.pickedData.begin();
                state.selectedObject.first = it->first;
                if (!it->second.empty())
                {
                    state.selectedObject.second = it->second.front();
                }
            }
        }
        else if (state.mode == mDirectionSelectorWidget::DirectInput)
        {
            state.directInputDir = dirVar.value<QVector3D>();
        }
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorBoltPanel::getPickedData()
    {
        return getTargetPickPairs();
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorBoltPanel::getTargetPickPairs() const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        if (!_targetPickController) return result;
        const auto& allData = _targetPickController->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    QVector<QWidget*> mConnectorBoltPanel::getTypeDefine(FoldSection* value)
    {
        typeDefine_ = value;
        QVector<QWidget*>wds;
        wds.append(_typeCombo.propwid);
        return wds;
    }

    QVector<QWidget*> mConnectorBoltPanel::getTargetObject(FoldSection* value)
    {
        targetObject_ = value;
        QVector<QWidget*>wds;
        if (_targetPickController)
            wds.append(_targetPickController->widget());
        return wds;
    }

    QVector<QWidget*> mConnectorBoltPanel::getVectorDefine(FoldSection* value)
    {
        vectorDefine_ = value;
        QVector<QWidget*>wds;

        // 折叠框内部统一为方向控件（标题勾选框控制整体启用）
        ensureDirectionWidget();
        if (_directionWidget)
        {
            // 首次创建后立即反显当前连接数据（setInputsValue 先于本函数执行）
            auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
            if (connectorPtr)
            {
                applyDirStateToWidget(connectorPtr);
            }
            wds.append(_directionWidget);
        }
        return wds;
    }

    void mConnectorBoltPanel::slot_typeDefineChanged(int idx) //一维 三维切换
    {
        applyAllowedPickTypes(idx);

        if (_targetPickController)
        {
            _targetPickController->replacePickedDataAndClearSets({});
        }

        std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->type_ = _typeCombo.combobox->currentIndex();
        updateConnectorSymbolRealtime();
    }

    void mConnectorBoltPanel::applyAllowedPickTypes(int typeIndex)
    {
        if (!_targetPickController) return;

        if (typeIndex == 0)
        {
            _targetPickController->setAllowedTypes(&pickGeoEdgeFaceAndElement);
            _targetPickController->setSelectSetList(
                getSetNamesList({ SetType::GeoEdge, SetType::GeoFace, SetType::Element }));
        }
        else
        {
            _targetPickController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _targetPickController->setSelectSetList(
                getSetNamesList({ SetType::GeoFace, SetType::ElementSurface }));
        }

        // 切换类型后清空已选数据，避免旧类型数据残留在新过滤规则下
        _targetPickController->replacePickedDataAndClearSets({});
        _targetPickController->setPickType();

        if (_typeCombo.combobox->currentIndex() == 0)
        {
            emit mMechGlobalSignals::getInstance()->activePickSig(8);
        }
        else
        {
            emit mMechGlobalSignals::getInstance()->activePickSig(7);
        }
    }

    void mConnectorBoltPanel::updateConnectorSymbolRealtime()
    {

        if (!_targetPickController || !_connectorData)
            return;

        std::shared_ptr<MechConnectorData::mConnectorBolt> connectorPtr =
            std::dynamic_pointer_cast<MechConnectorData::mConnectorBolt>(_connectorData);
        if (!connectorPtr)
            return;

        const int boltType = _typeCombo.combobox->currentIndex();

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开)，合并为按类型分组的map
        const PickWidgetData allData = _targetPickController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;

        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        auto appendIds = [](MxFunctions::PickDataType type, const std::vector<int>& ids,
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst) {
                auto& vec = dst[type];
                for (int id : ids)
                    vec.emplace_back(id, -1, -1);
            };

        for (const QString& setName : allData.selectedSetNames)
        {
            mSetBase* setBase = setManager.getSetData(setName);
            if (!setBase) continue;

            // 混合集合：按螺栓类型过滤后加入
            if (auto mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase))
            {
                auto filtered = filterBoltMixSetData(mixSet->getMixSetData(), boltType,
                    getCVM->getModelData()->getMeshData());
                for (const auto& inner : filtered)
                {
                    auto& vec = merged[inner.first];
                    vec.insert(vec.end(), inner.second.begin(), inner.second.end());
                }
                continue;
            }

            MxFunctions::SetType setType = setBase->getSetType();
            // 按螺栓类型过滤允许的集合类型，与 applyAllowedPickTypes 规则一致
            bool typeAllowed = false;
            if (boltType == 0) // 一维螺栓：允许几何线、几何面、一维单元
            {
                typeAllowed = (setType == MxFunctions::GeoEdge || setType == MxFunctions::GeoFace || setType == MxFunctions::Element);
            }
            else // 三维螺栓：允许几何面、单元面
            {
                typeAllowed = (setType == MxFunctions::GeoFace || setType == MxFunctions::ElementSurface);
            }
            if (!typeAllowed)
                continue;

            switch (setType)
            {
            case MxFunctions::GeoEdge:
                if (auto s = dynamic_cast<mGeoSet*>(setBase))
                    appendIds(MxFunctions::PickDataType::GeoEdge, s->getGeoSetData(), merged);
                break;
            case MxFunctions::GeoFace:
                if (auto s = dynamic_cast<mGeoSet*>(setBase))
                    appendIds(MxFunctions::PickDataType::GeoFace, s->getGeoSetData(), merged);
                break;
            case MxFunctions::Element:
                if (auto s = dynamic_cast<MechSetData::mElementSet*>(setBase))
                {
                    auto& vec = merged[MxFunctions::PickDataType::Element];
                    const auto& eleData = s->getElementSetData();
                    auto meshData = getCVM->getModelData()->getMeshData();
                    for (const auto& ele : eleData)
                    {
                        // 一维螺栓仅保留一维单元，与过滤规则一致
                        if (boltType != 0 || (meshData && meshData->getDim(ele.result_id) == 1))
                            vec.push_back(ele);
                    }
                }
                break;
            case MxFunctions::ElementSurface:
                if (auto s = dynamic_cast<MechSetData::mElementSurface*>(setBase))
                {
                    auto& vec = merged[MxFunctions::PickDataType::ElementFace];
                    const auto& surfaceIds = s->getElementSurfaceData();
                    vec.insert(vec.end(), surfaceIds.begin(), surfaceIds.end());
                }
                break;
            default:
                break;
            }
        }

        // 2. 按类型计算作用对象坐标与方向，与 AllRenderSymbolManager::getConnectorBolt 逻辑完全对齐
        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        std::vector<std::pair<QVector3D, QVector3D>> pos_dir;

        for (const auto& pair : merged)
        {
            const MxFunctions::PickDataType type = pair.first;
            const auto& dataVec = pair.second;
            if (dataVec.empty())
                continue;

            switch (type)
            {
            case MxFunctions::PickDataType::GeoEdge:
            {
                for (const auto& d : dataVec)
                {
                    auto set = geoData->getPartSetByGeoLineID(d.result_id);
                    if (!set) continue;
//MIG                     TopoDS_Edge edge = set->getEdgeByTag(d.result_id);

//MIG                     TopTools_IndexedMapOfShape vertexMap;
//MIG                     TopExp::MapShapes(edge, TopAbs_VERTEX, vertexMap);
//MIG                     QVector<gp_Pnt> points;
//MIG                     for (Standard_Integer i = 1; i <= vertexMap.Extent(); ++i)
                    {
//MIG                         TopoDS_Vertex vertex = TopoDS::Vertex(vertexMap.FindKey(i));
//MIG                         points.push_back(BRep_Tool::Pnt(vertex));
                    }

//MIG                     if (points.size() >= 2)
                    {
//MIG                         QVector3D p0 = { (float)points[0].X(), (float)points[0].Y(), (float)points[0].Z() };
//MIG                         QVector3D p1 = { (float)points[1].X(), (float)points[1].Y(), (float)points[1].Z() };
//MIG                         pos_dir.emplace_back(p0, p1);
                    }
                }
            }
            break;

            case MxFunctions::PickDataType::GeoFace:
            {
                std::vector<int> ids;
                ids.reserve(dataVec.size());
                for (const auto& d : dataVec)
                    ids.push_back(d.result_id);
                auto faceAttrs = geoManager->CalculateFaceAttr(ids);

                if (boltType == 0) // 一维螺栓：取面上采样点 + 法向偏移
                {
                    for (int i = 0; i < ids.size(); ++i)
                    {
//MIG                         auto set = geoData->getPartSetByGeoFaceID(ids[i]);
//MIG                         if (!set) continue;

                        QVector<QVector3D> coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, { ids[i] });
                        QVector3D normal = faceAttrs[i].m_dir;
                        for (const auto& coord : coords)
                            pos_dir.emplace_back(coord, coord + normal);
                    }
                }
                else // 三维螺栓：取面质心 + 法向
                {
                    for (int i = 0; i < ids.size(); ++i)
                    {
//MIG                         auto set = geoData->getPartSetByGeoFaceID(ids[i]);
//MIG                         if (!set) continue;

//MIG                         TopoDS_Face face = set->getFaceByTag(ids[i]);
//MIG                         GProp_GProps props;
//MIG                         BRepGProp::SurfaceProperties(face, props);
//MIG                         gp_Pnt centerOfMass = props.CentreOfMass();
//MIG                         QVector3D center = { (float)centerOfMass.X(), (float)centerOfMass.Y(), (float)centerOfMass.Z() };
                        pos_dir.emplace_back(faceAttrs[i].m_coor, faceAttrs[i].m_dir); //【复活修正】质心取自引擎属性
                    }
                }
            }
            break;

            case MxFunctions::PickDataType::Element:
            {
                for (const auto& d : dataVec)
                {
                    if (!meshData || meshData->getDim(d.result_id) != 1)
                        continue;

                    std::vector<RenderPickResultData> nodes = meshData->getNodeIdsByEleIds({ d.result_id });
                    if (nodes.size() < 2)
                        continue;

                    QVector3D p0 = meshData->getVectorById(nodes[0].result_id);
                    QVector3D p1 = meshData->getVectorById(nodes[1].result_id);
                    pos_dir.emplace_back(p0, p1);
                }
            }
            break;

            case MxFunctions::PickDataType::ElementFace:
            {
                for (const auto& d : dataVec)
                {
                    QVector3D centerPos;
                    QVector3D normal;
                    meshData->GetOutVectorByMeshFaceID(d, normal, centerPos);
                    pos_dir.emplace_back(centerPos, normal);
                }
            }
            break;

            case MxFunctions::PickDataType::ElementEdge:
            {
                for (const auto& d : dataVec)
                {
                    QVector3D p0 = meshData->getVectorById(d.result_id);
                    QVector3D p1 = meshData->getVectorById(d.result_id2);
                    pos_dir.emplace_back(p0, p1);
                }
            }
            break;

            default:
                break;
            }
        }

        // 3. 更新螺栓渲染符号（与提交后渲染共用key，同名覆盖；pos_dir为空则清除残留符号）
        const bool isFaceMode = (boltType == 1);
        AllRenderSymbolManager::getInstance()->assignConnectorBolt(
            connectorPtr, pos_dir, "Connector_Bolt", QVector3D(255.0f, 255.0f, 0.0f), isFaceMode);

        // 刷新视图
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mConnectorBoltPanel::slot_targetPickActiveChanged(bool active)
    {
        if (active)
        {
            int typeIndex = _typeCombo.combobox->currentIndex();
            if (typeIndex == 0)
                emit mMechGlobalSignals::getInstance()->activePickSig(8); // 一维单元/几何线/面
            else
                emit mMechGlobalSignals::getInstance()->activePickSig(3); // 面
        }
    }

    void mConnectorBoltPanel::slot_targetDataChanged()
    {
        updateConnectorSymbolRealtime();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mConnectorBoltPanel::filterBoltMixSetData(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& mixData,
        int boltType, mMeshData* meshData) const
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> result;
        if (!meshData)
            return result;

        for (const auto& pair : mixData)
        {
            MxFunctions::PickDataType type = pair.first;
            const auto& data = pair.second;
            if (data.empty())
                continue;

            if (boltType == 0 && type == MxFunctions::PickDataType::Element)
            {
                for (const auto& d : data)
                {
                    if (meshData->getMeshTypeByEleId(d.result_id) == MxFunctions::MeshBeam)
                    {
                        result[type].push_back(d);
                    }
                }
            }
            else if (boltType == 1 && type == MxFunctions::PickDataType::ElementFace)
            {
                // 三维螺栓仅保留单元面（也可根据后续需求继续按维度过滤）
                result[type] = data;
            }
            else if (type == MxFunctions::PickDataType::GeoEdge || type == MxFunctions::PickDataType::GeoFace)
            {
                // 几何线/几何面直接保留
                result[type] = data;
            }
            else
            {
                // 其它类型按原样保留，避免数据丢失
                result[type] = data;
            }
        }
        return result;
    }

    void mConnectorBoltPanel::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        if (_targetPickController)
        {
            _targetPickController->setKeepHighlightOnDeactivate(false);
            const bool wasActive = _targetPickController->isPickActive();
            _targetPickController->setPickActive(false);
            if (renderValid && !wasActive)
            {
                _targetPickController->clearAllHighlight();
            }
            _targetPickController->setKeepHighlightOnDeactivate(true);
        }

        // 方向选择控件内部包含多个子拾取控件，切出面板时同样要失活并清理高亮
        if (_directionWidget)
        {
            auto deactivateChildPick = [](mPickWidgetController* ctrl) {
                if (!ctrl) return;
                const bool wasActive = ctrl->isPickActive();
                if (!wasActive) return;
                ctrl->setKeepHighlightOnDeactivate(false);
                ctrl->setPickActive(false);
                ctrl->setKeepHighlightOnDeactivate(true);
                };
            deactivateChildPick(_directionWidget->startPickController());
            deactivateChildPick(_directionWidget->endPickController());
            deactivateChildPick(_directionWidget->objectPickController());
            _directionWidget->clearAllHighlight();
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    /*************************************************分布式耦合连接***************************************************************/
    mConnectorDistributedCouplingPanel::mConnectorDistributedCouplingPanel(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorDistributedCouplingPanel::~mConnectorDistributedCouplingPanel()
    {
        clearInputs();
    }

    void mConnectorDistributedCouplingPanel::saveSubNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        auto mainPairs = getWidgetDataAsPickPairs(true);
        if (!mainPairs.empty())
        {
            connectorPtr->subnodes_.first = mainPairs[0].first;
            connectorPtr->subnodes_.second = mainPairs[0].second;
        }
        else
        {
            connectorPtr->subnodes_.first = MxFunctions::PickDataType::None;
            connectorPtr->subnodes_.second = RenderPickResultData();
        }
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::saveMainNode()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mainnodes;
        auto subPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : subPairs)
        {
            mainnodes[pair.first].push_back(pair.second);
        }
        for (auto it = mainnodes.begin(); it != mainnodes.end(); )
        {
            if (it->second.empty())
            {
                it = mainnodes.erase(it);
            }
            else
            {
                ++it;
            }
        }
        connectorPtr->mainnodes_ = mainnodes;
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::initInputs()
    {
        // 位置
        poswaywid.initWidget("方式");
        poswaywid.combobox->addItem("一对一");
        poswaywid.combobox->addItem("一对多");
        connect(poswaywid.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorDistributedCouplingPanel::slot_nodeWayChanged);

        // 通用拾取控件：参考点位置（单点，几何点/节点）
        _refPickController = mGeneralPickUp::createOnePickWidget(CBBlue);
        _refPickController->setDefultTip("参考点");
        // 持久化策略在 setInputsValue 中根据当前 connectorData 重新绑定。
        connect(_refPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_refPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteSubConnNodes();
            appendSubConnNodes();
            updateConnectorSymbolRealtime();
            saveSubNode();
            // 参考点为单节点拾取，拾取完成后自动切换到耦合点拾取控件（与刚性连接一致）
            if (getWidgetDataAsPickPairs(true).size() > 0)
            {
                _couplingPickController->setPickActive(true);
            }
            notifyConnectorChanged();
            });

        // 通用拾取控件：耦合点位置（一对一 PickOne / 一对多 PickMore）
        _couplingPickController = mGeneralPickUp::createMorePickWidget(CBRed);
        _couplingPickController->setDefultTip("耦合点");
        connect(_couplingPickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_couplingPickController, &mPickWidgetController::dataChanged, this, [this]() {
            deleteMainConnNodes();
            appendMainConnNodes();
            // 参考点为空且耦合点非空时自动计算参考点
            //calMainNode();
            updateConnectorSymbolRealtime();
            saveMainNode();
            updateNodeSettingButtonState();
            if (nodeseting_.tablebtn->isChecked())
            {
                slot_nodeSetting();
            }
            // 一对一模式下耦合点为单节点拾取，拾取完成后自动切换到参考点拾取控件（与刚性连接一致）
            if (poswaywid.combobox->currentIndex() == 0 && getWidgetDataAsPickPairs(false).size() > 0)
            {
                _refPickController->setPickActive(true);
            }
            notifyConnectorChanged();
            });
        connect(_couplingPickController, &mPickWidgetController::calcMainNodeSig, this, [this]() {
            calMainNode();
            });

        // 权重
        weightInput_.initWidget("权重值", false);
        connect(weightInput_.lineeditvalue, &mCalcLineEdit::editingFinished,
            this, &mConnectorDistributedCouplingPanel::saveWeight);

        // 自由度
        names1_ = { "U1","U2","U3" };
        names2_ = { "UR1","UR2","UR3" };
        CheckBoxsWidget checks1;
        checks1.initWidget(names1_);
        checkswid_.append(checks1);
        CheckBoxsWidget checks2;
        checks2.initWidget(names2_);
        checkswid_.append(checks2);
        for (auto& check : checks1.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorDistributedCouplingPanel::saveDegreeOfFreedom);
        }
        for (auto& check : checks2.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mConnectorDistributedCouplingPanel::saveDegreeOfFreedom);
        }

        // 默认勾选U1/U2/U3并置灰
        for (auto& check : checkswid_[0].labels)
        {
            check->setChecked(true);
            check->setEnabled(false);
        }

        // 差异化设置按钮，初始无节点时置灰
        nodeseting_.initWidget("对每个节点进行差异化设置");
        nodeseting_.tablebtn->setCheckable(true);
        nodeseting_.tablebtn->setChecked(false);
        nodeseting_.tablebtn->setEnabled(false);
        connect(nodeseting_.tablebtn, &QPushButton::toggled, [=](bool checked) {
            if (checked) {
                slot_nodeSetting();
                setConnectTbaleSig(true);
                nodeseting_.tablebtn->setStyleSheet(checkedStyle);
            }
            else {
                // U1/U2/U3 默认选择，保持置灰
                for (auto& check : checkswid_[0].labels)
                {
                    check->setDisabled(true);
                }
                setConnectTbaleSig(false);
                nodeseting_.tablebtn->setStyleSheet(defaultStyle);
            }
            saveTableMode();
            });
        initTable();
        setConnectTbaleSig(false);
        nodeseting_.tablebtn->setStyleSheet(defaultStyle);
    }

    void mConnectorDistributedCouplingPanel::applyPickMode()
    {
        if (!_refPickController || !_couplingPickController) return;
        // 参考点始终单选
        _refPickController->setPickMode(PickOne);
        // 耦合点：一对一单选，一对多多选；切到一对一且已有多个耦合点时清空重选
        const int way = poswaywid.combobox->currentIndex();
        mPickMode couplingMode = (way == 0) ? PickOne : PickMore;
        _couplingPickController->setPickMode(couplingMode, way == 0);
    }

    void mConnectorDistributedCouplingPanel::applyAllowedPickTypes()
    {
        if (!_refPickController || !_couplingPickController) return;

        // 参考点始终只能拾取几何点/节点
        _refPickController->setAllowedTypes(&pickGeoPointAndNode);
        // 一对一：耦合点只能拾取几何点/节点；一对多：可拾取几何点/线/面/实体/节点/部件
        if (poswaywid.combobox->currentIndex() == 0)
        {
            _couplingPickController->setAllowedTypes(&pickGeoPointAndNode);
        }
        else
        {
            _couplingPickController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        }

        if (_refPickController->isPickActive())
        {
            _refPickController->setPickType();
        }
        if (_couplingPickController->isPickActive())
        {
            _couplingPickController->setPickType();
        }
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorDistributedCouplingPanel::getWidgetDataAsPickPairs(bool isRef) const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        mPickWidgetController* ctrl = isRef ? _refPickController : _couplingPickController;
        if (!ctrl) return result;
        const auto& allData = ctrl->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    void mConnectorDistributedCouplingPanel::slot_nodeSetting()
    {
        QVector<QVector<QString>> datas;

        // 保留表格中已有节点的权重值，避免每次点击按钮都重置为1.0
        QHash<int, double> existingWeights;
        int rowCount = weightTable_->table()->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* nodeItem = weightTable_->table()->item(i, 0);
            QTableWidgetItem* weightItem = weightTable_->table()->item(i, 1);
            if (nodeItem && weightItem)
            {
                bool ok;
                int nodeId = nodeItem->text().toInt(&ok);
                double w = weightItem->text().toDouble(&ok);
                if (ok)
                {
                    existingWeights[nodeId] = w;
                }
            }
        }

        auto couplingPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : couplingPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            int x = pair.second.result_id;
            double w = existingWeights.contains(x) ? existingWeights[x] : 1.0;
            QVector<QString> tmp;
            tmp.push_back(MxFunctions::mNumberToString(x));
            tmp.push_back(MxFunctions::mNumberToString(w));
            datas.push_back(tmp);
        }
        weightTable_->table()->blockSignals(true);
        setValueToTable(datas);
        weightTable_->table()->blockSignals(false);

        mDockLayoutManager::getInstance()->activateDock("Table");
    }

    void mConnectorDistributedCouplingPanel::initTable()
    {
        weightTable_ = new WeightTableWidget();
        weightTable_->table()->setRowCount(0);
        weightTable_->table()->setColumnCount(2);
        weightTable_->table()->verticalHeader()->setHidden(false);
        weightTable_->table()->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("Node ID") << QString("Weight");
        weightTable_->table()->setHorizontalHeaderLabels(headerText);
        for (int c = 0; c < 2; ++c)
        {
            weightTable_->table()->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
    }

    void mConnectorDistributedCouplingPanel::setValueToTable(const QVector<QVector<QString>>& data)
    {
        weightTable_->table()->setRowCount(0);
        weightTable_->table()->setColumnCount(2);
        weightTable_->table()->verticalHeader()->setHidden(false);
        weightTable_->table()->resizeColumnsToContents();
        QStringList headerText;
        headerText << QString("节点ID") << QString("权重");
        weightTable_->table()->setHorizontalHeaderLabels(headerText);
        for (int c = 0; c < 2; ++c)
        {
            weightTable_->table()->horizontalHeader()->setSectionResizeMode(c, QHeaderView::Stretch);
        }
        weightTable_->table()->setRowCount(data.size());
        for (int i = 0; i < data.size(); ++i)
        {
            QTableWidgetItem* item1 = new QTableWidgetItem(data[i][0]);
            item1->setTextAlignment(Qt::AlignCenter);
            item1->setFlags(item1->flags() & ~Qt::ItemIsEditable);
            weightTable_->table()->setItem(i, 0, item1);

            QTableWidgetItem* item2 = new QTableWidgetItem(data[i][1]);
            item2->setTextAlignment(Qt::AlignCenter);
            weightTable_->table()->setItem(i, 1, item2);
        }
    }

    void mConnectorDistributedCouplingPanel::getTableData(QVector<double>& weight)
    {
        weight.clear();
        int rowCount = weightTable_->table()->rowCount();
        for (int i = 0; i < rowCount; ++i)
        {
            QTableWidgetItem* weightItem = weightTable_->table()->item(i, 1);
            if (weightItem && !weightItem->text().isEmpty())
            {
                bool ok;
                double w = weightItem->text().toDouble(&ok);
                weight.push_back(ok ? w : 0.0);
            }
            else
            {
                weight.push_back(0.0);
            }
        }
    }

    void mConnectorDistributedCouplingPanel::slot_setTablDataToConnector()
    {
        saveTableData();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::updateNodeSettingButtonState()
    {
        bool hasNodes = getWidgetDataAsPickPairs(false).size() > 0;
        nodeseting_.tablebtn->setEnabled(hasNodes);
    }

    void mConnectorDistributedCouplingPanel::slot_nodeWayChanged(int i)
    {
        Q_UNUSED(i)
            saveWay();

        deleteMainConnNodes();
        deleteSubConnNodes();

        // 切换方式时清空参考点/耦合点，避免残留数据
        if (_refPickController)
        {
            _refPickController->replacePickedDataAndClearSets({});
        }
        if (_couplingPickController)
        {
            _couplingPickController->replacePickedDataAndClearSets({});
        }

        applyPickMode();
        applyAllowedPickTypes();

        updateConnectorSymbolRealtime();
        updateNodeSettingButtonState();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::calMainNode()
    {
        auto refPairs = getWidgetDataAsPickPairs(true);
        if (!refPairs.empty())
        {
            return;
        }

        auto couplingPairs = getWidgetDataAsPickPairs(false);
        if (couplingPairs.empty())
        {
            return;
        }

        int calnodeid = calculateMasterNode(couplingPairs);
        if (calnodeid == -1)
        {
            return;
        }

        // 与刚性/柔性连接对齐：计算出的参考点直接写入业务数据，并交给通用拾取控件高亮渲染
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }

        RenderPickResultData nodeData{ calnodeid, -1, -1 };
        connectorPtr->subnodes_.first = PickDataType::Node;
        connectorPtr->subnodes_.second = nodeData;

        PickWidgetData data;
        data.pickedData[MxFunctions::PickDataType::Node].push_back(nodeData);
        if (_refPickController)
        {
            _refPickController->displayData(data, true);
        }

        deleteSubConnNodes();
        appendSubConnNodes();
        //rendFreeNodes(true);
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::updateConnectorSymbolRealtime()
    {
        if (!_refPickController || !_couplingPickController || !_connectorData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        if (!meshData || !geoManager)
            return;

        auto posOf = [&](MxFunctions::PickDataType type, int id) -> QVector3D {
            if (type == MxFunctions::PickDataType::Node)
            {
                return meshData->getVectorById(id);
            }
            else if (type == MxFunctions::PickDataType::GeoPoint)
            {
                auto points = geoManager->CalculateVertexAttr({ id });
                if (!points.empty())
                    return points[0].m_coor;
            }
            return QVector3D();
            };

        // 参考点：按模式应唯一，取第一个有效对象
        QVector3D refPos;
        bool hasRef = false;
        const auto refData = _refPickController->getAllData();
        for (const auto& pair : refData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                refPos = posOf(type, d.result_id);
                hasRef = true;
                break;
            }
            if (hasRef)
                break;
        }

        // 耦合点：汇总所有有效对象坐标
        QVector<QVector3D> couplingPoss;
        const auto couplingData = _couplingPickController->getAllData();
        for (const auto& pair : couplingData.pickedData)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        couplingPoss.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) couplingPoss.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) couplingPoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) couplingPoss.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) couplingPoss.push_back(c);
                break;
            }
            default:
                break;
            }
        }

        auto render = AllRenderSymbolManager::getInstance();
        if (hasRef && !couplingPoss.isEmpty())
        {
            render->assignConnector(_connectorData, refPos, couplingPoss, ConnectorType::Connector_DistributedCoupling);
        }
        else
        {
            // 数据不完整时清除实时符号，避免残留旧连线
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_DistributedCoupling);
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mConnectorDistributedCouplingPanel::deleteMainConnNodes()
    {
        std::vector<int> mainnodesid;
        auto refPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : refPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);    
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(mainnodesid);
    }

    void mConnectorDistributedCouplingPanel::deleteSubConnNodes()
    {
        std::vector<int> subnodesid;
        int nodeid = -1;
        auto couplingPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : couplingPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
            nodeid = pair.second.result_id;
        }
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (nodeid == -1 && connectorPtr) {
            nodeid = connectorPtr->subnodes_.second.result_id;
            subnodesid.push_back(nodeid);
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(subnodesid);
        if (nodeid != -1 && !getCVM->getModelData()->getMeshData()->NodeIsHaveByID(nodeid))
        {
            mDisplayMenuFunction::getInstance()->slot_showFreeNode({ nodeid }, false);
        }
    }

    void mConnectorDistributedCouplingPanel::appendMainConnNodes()
    {
        std::vector<int> mainnodesid;
        auto refPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : refPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            mainnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(mainnodesid);
    }

    void mConnectorDistributedCouplingPanel::appendSubConnNodes()
    {
        std::vector<int> subnodesid;
        auto couplingPairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : couplingPairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            subnodesid.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(subnodesid);
    }

    void mConnectorDistributedCouplingPanel::clearInputs()
    {
        poswaywid.clear();
        if (_refPickController)
        {
            delete _refPickController;
            _refPickController = nullptr;
        }
        if (_couplingPickController)
        {
            delete _couplingPickController;
            _couplingPickController = nullptr;
        }
        weightInput_.clear();
        for (auto checks : checkswid_)
        {
            checks.clear();
        }
        if (weightTable_)
        {
            delete weightTable_;
            weightTable_ = nullptr;
        }
    }

    void mConnectorDistributedCouplingPanel::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase> connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(connectorData);
        if (!connectorPtr)
        {
            return;
        }
        _connectorData = connectorData;

        // 位置
        poswaywid.combobox->blockSignals(true);
        poswaywid.combobox->setCurrentIndex(connectorPtr->way_);
        poswaywid.combobox->blockSignals(false);

        applyPickMode();
        applyAllowedPickTypes();

        // 绑定通用拾取控件持久化策略：
        // - 参考点 -> subnodes_（单值）
        // - 耦合点 -> mainnodes_（按类型分组）
        if (_refPickController)
        {
            _refPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorDistributedCoupling::subnodes_), &resolveSetToTypedData));
            _refPickController->setSetResolver(&resolveSetToTypedData);
            _refPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }
        if (_couplingPickController)
        {
            _couplingPickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeTypedMapWriter(connectorPtr, &MechConnectorData::mConnectorDistributedCoupling::mainnodes_), &resolveSetToTypedData));
            _couplingPickController->setSetResolver(&resolveSetToTypedData);
            _couplingPickController->setSelectSetList(getSetNamesList({ SetType::GeoPoint, SetType::Node }));
        }

        // 反显参考点（subnodes_ 在数据中代表参考点 UI）
        if (_refPickController)
        {
            PickWidgetData refData;
            if (connectorPtr->subnodes_.first != MxFunctions::PickDataType::None)
            {
                MxFunctions::PickDataType setType = connectorPtr->subnodes_.first;
                refData.pickedData[setType].push_back(connectorPtr->subnodes_.second);
            }
            _refPickController->displayData(refData);
        }

        // 反显耦合点（mainnodes_ 在数据中代表耦合点 UI）
        if (_couplingPickController)
        {
            PickWidgetData couplingData;
            for (auto it = connectorPtr->mainnodes_.begin(); it != connectorPtr->mainnodes_.end(); ++it)
            {
                MxFunctions::PickDataType setType = it->first;
                couplingData.pickedData[setType] = it->second;
            }
            _couplingPickController->displayData(couplingData, true);
        }

        // 默认激活参考点拾取控件
        if (_refPickController && !_refPickController->isPickActive())
        {
            _refPickController->setPickActive(true);
        }

        // 反显后实时渲染连接符号，使切换回面板时立即可见。
        updateConnectorSymbolRealtime();

        // 自由度：U1/U2/U3 始终默认勾选并置灰；反显前 blockSignals 避免覆盖数据
        for (auto& checks : checkswid_)
        {
            for (auto* check : checks.labels)
            {
                if (check) check->blockSignals(true);
            }
        }
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid_[0].labels[i - 1];
            check->setChecked(true);
            check->setDisabled(true);
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid_[1].labels[i - 4];
            check->setChecked(connectorPtr->degreeoffreedom_main.find(i) != connectorPtr->degreeoffreedom_main.end());
        }
        for (auto& checks : checkswid_)
        {
            for (auto* check : checks.labels)
            {
                if (check) check->blockSignals(false);
            }
        }

        // 权重
        if (connectorPtr->singleweight_ != -1)
        {
            weightInput_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->singleweight_));
        }

        // 表格
        weightTable_->table()->clear();
        nodeseting_.tablebtn->blockSignals(true);
        nodeseting_.tablebtn->setChecked(false);
        nodeseting_.tablebtn->blockSignals(false);
        if (!connectorPtr->w_.empty())
        {
            QVector<QVector<QString>> datas;
            auto itNode = connectorPtr->mainnodes_.find(PickDataType::Node);
            if (itNode != connectorPtr->mainnodes_.end())
            {
                for (int i = 0; i < itNode->second.size(); ++i)
                {
                    if (i >= connectorPtr->w_.size())
                    {
                        continue;
                    }
                    int x = itNode->second[i].result_id;
                    QVector<QString> tmp;
                    tmp.push_back(MxFunctions::mNumberToString(x));
                    tmp.push_back(MxFunctions::mNumberToString(connectorPtr->w_[i]));
                    datas.push_back(tmp);
                }
            }

            weightTable_->table()->blockSignals(true);
            nodeseting_.tablebtn->blockSignals(true);
            nodeseting_.tablebtn->setChecked(true);
            nodeseting_.tablebtn->blockSignals(false);
            setValueToTable(datas);
            weightTable_->table()->blockSignals(false);

            setConnectTbaleSig(true);
            nodeseting_.tablebtn->setStyleSheet(checkedStyle);

            mDockLayoutManager::getInstance()->activateDock("Table");
        }
        else
        {
            setConnectTbaleSig(false);
            nodeseting_.tablebtn->setStyleSheet(defaultStyle);
        }
        updateNodeSettingButtonState();

        _main2subs.clear();
    }

    QVector<QWidget*> mConnectorDistributedCouplingPanel::getPosInputs(FoldSection* fapos)
    {
        fapos_ = fapos;
        QVector<QWidget*> wds;
        wds.append(poswaywid.propwid);
        if (_refPickController)
        {
            wds.append(_refPickController->widget());
        }
        if (_couplingPickController)
        {
            wds.append(_couplingPickController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mConnectorDistributedCouplingPanel::getWeightAndDofInputs(FoldSection* fasec)
    {
        fasec_ = fasec;
        QVector<QWidget*> wds;
        wds.append(weightInput_.propwid);
        wds.append(nodeseting_.propwid);
        wds.append(checkswid_[0].propwid);
        wds.append(checkswid_[1].propwid);
        return wds;
    }

    void mConnectorDistributedCouplingPanel::saveWay()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->way_ = poswaywid.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::saveWeight()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->singleweight_ = weightInput_.lineeditvalue->text().toDouble();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::saveDegreeOfFreedom()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->degreeoffreedom_main.clear();
        for (int i = 1; i <= 3; ++i)
        {
            auto& check = checkswid_[0].labels[i - 1];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        for (int i = 4; i <= 6; ++i)
        {
            auto& check = checkswid_[1].labels[i - 4];
            if (check->isChecked())
            {
                connectorPtr->degreeoffreedom_main.insert(i);
            }
        }
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::saveTableData()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->w_.clear();
        if (nodeseting_.tablebtn->isChecked())
        {
            QVector<double> weightvals;
            getTableData(weightvals);
            connectorPtr->w_ = weightvals.toStdVector();
        }
        else
        {
            // 非差异化模式：所有节点使用统一权重
            auto itNode = connectorPtr->mainnodes_.find(MxFunctions::PickDataType::Node);
            if (itNode != connectorPtr->mainnodes_.end())
            {
                for (int i = 0; i < itNode->second.size(); ++i)
                {
                    connectorPtr->w_.push_back(connectorPtr->singleweight_);
                }
            }
        }
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::saveTableMode()
    {
        std::shared_ptr<MechConnectorData::mConnectorDistributedCoupling> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorDistributedCoupling>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        // 差异化模式状态由 isTableMode_ 兼容字段记录（复用柔性连接的持久化字段约定）
        connectorPtr->dof_.clear();
        notifyConnectorChanged();
    }

    void mConnectorDistributedCouplingPanel::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _refPickController, _couplingPickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(QVector<bool>(10, true));
    }

    void mConnectorDistributedCouplingPanel::setConnectTbaleSig(bool on)
    {
        if (on)
        {
            connect(weightTable_->table(), &QTableWidget::cellChanged, this, &mConnectorDistributedCouplingPanel::slot_setTablDataToConnector);
            mTableDataWindow::getInstance()->setCustomWidget(weightTable_);
            mDockLayoutManager::getInstance()->activateDock("Table");
        }
        else
        {
            disconnect(weightTable_->table(), &QTableWidget::cellChanged, this, &mConnectorDistributedCouplingPanel::slot_setTablDataToConnector);
            mTableDataWindow::getInstance()->resetInitialTable();
        }
    }

    void mConnectorDistributedCouplingPanel::rendFreeNodes(bool showhide)
    {
        std::vector<int> tempIds;
        auto refPairs = getWidgetDataAsPickPairs(true);
        if (!refPairs.empty() && refPairs[0].first == PickDataType::Node)
        {
            auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ refPairs[0].second.result_id });
            if (!tempnodes.empty() && tempnodes[0])
            {
                tempIds.push_back(refPairs[0].second.result_id);
            }
        }
        auto couplingPairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : couplingPairs)
        {
            if (pair.first == PickDataType::Node)
            {
                auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ pair.second.result_id });
                if (!tempnodes.empty() && tempnodes[0])
                {
                    tempIds.push_back(pair.second.result_id);
                }
            }
        }
        if (tempIds.empty())
            return;
        mDisplayMenuFunction::getInstance()->slot_showFreeNode(tempIds, showhide);
    }
    /*************************************************轴承单元***************************************************************/
    mConnectorBearingUnits::mConnectorBearingUnits(QWidget* parent)
        :mConnectorBase(parent)
    {
        initInputs();
    }

    mConnectorBearingUnits::~mConnectorBearingUnits()
    {
        clearInputs();
    }

    void mConnectorBearingUnits::initInputs()
    {
        //类型
        typeWid_.initWidget("类型");
        typeWid_.combobox->addItem("几何体-地面");
        typeWid_.combobox->addItem("几何体-几何体");
        connect(typeWid_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorBearingUnits::slot_typeChanged);

        // 通用拾取控件：节点1（单点，几何点/节点）
        _node1PickController = mGeneralPickUp::createOnePickWidget(CBBlue);
        _node1PickController->setDefultTip("节点1");
        _node1PickController->setAllowedTypes(&pickGeoPointAndNode);
        connect(_node1PickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_node1PickController, &mPickWidgetController::dataChanged, this, &mConnectorBearingUnits::slot_node1PickDataChanged);

        // 通用拾取控件：节点2（单点，几何点/节点；仅"几何体-几何体"时可见）
        _node2PickController = mGeneralPickUp::createOnePickWidget(CBRed);
        _node2PickController->setDefultTip("节点2");
        _node2PickController->setAllowedTypes(&pickGeoPointAndNode);
        connect(_node2PickController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_node2PickController, &mPickWidgetController::dataChanged, this, &mConnectorBearingUnits::slot_node2PickDataChanged);

        coordinateWid1_.initWidget("节点1坐标系");
        coordinateWid2_.initWidget("节点2坐标系");
        coordinateWid2_.propwid->setVisible(false);
        // 默认"几何体-地面"，节点2控件整体隐藏（隐藏态拒绝激活并清理高亮）
        _node2PickController->setPickWidgetVisible(false);
        connect(coordinateWid1_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this]() {
                saveCoordinate1();
            });
        connect(coordinateWid2_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this]() {
                saveCoordinate2();
            });

        rotateFaceWid_.initWidget("旋转平面");
        rotateFaceWid_.combobox->addItem("XOY");
        rotateFaceWid_.combobox->addItem("XOZ");
        rotateFaceWid_.combobox->addItem("YOZ");
        cbRotate_.initWidget("定义方式");
        cbRotate_.combobox->addItem("固定");
        cbRotate_.combobox->addItem("随转速变化");
        K11Input_.initWidget("K11", false);
        K22Input_.initWidget("K22", false);
        K12Input_.initWidget("K12", false);
        K21Input_.initWidget("K21", false);
        C11Input_.initWidget("C11", false);
        C22Input_.initWidget("C22", false);
        C12Input_.initWidget("C12", false);
        C21Input_.initWidget("C21", false);

        bearingHeaderText_ << QString("转速") << QString("K11") << QString("K22") << QString("K12")
            << QString("K21") << QString("C11") << QString("C22") << QString("C12") << QString("C21");
        connect(cbRotate_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mConnectorBearingUnits::slot_defModeChanged);
        connect(rotateFaceWid_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            [this]() {
                saveRotateFace();
            });
        connect(K11Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(K22Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(K12Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(K21Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(C11Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(C22Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(C12Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });
        connect(C21Input_.lineeditvalue, &mCalcLineEdit::editingFinished, [this]() { saveBearingPara(); });

        connect(mTableDataWindow::getInstance(), &mTableDataWindow::tableDataChangedSig,
            this, &mConnectorBearingUnits::slot_tableEditFinished);
    }

    void mConnectorBearingUnits::clearInputs()
    {
        if (_node1PickController)
        {
            delete _node1PickController;
            _node1PickController = nullptr;
        }
        if (_node2PickController)
        {
            delete _node2PickController;
            _node2PickController = nullptr;
        }
        typeWid_.clear();
        coordinateWid1_.clear();
        coordinateWid2_.clear();
        rotateFaceWid_.clear();
        K11Input_.clear();
        K22Input_.clear();
        K12Input_.clear();
        K21Input_.clear();
        C11Input_.clear();
        C22Input_.clear();
        C12Input_.clear();
        C21Input_.clear();
        cbRotate_.clear();
        if (bearingPara_)
        {
            delete bearingPara_;
            bearingPara_ = nullptr;
        }
        if (bearingUnits_)
        {
            delete bearingUnits_;
            bearingUnits_ = nullptr;
        }
    }

    void mConnectorBearingUnits::setInputsValue(std::shared_ptr<MechConnectorData::mConnectorBase>connectorData)
    {
        std::shared_ptr<MechConnectorData::mConnectorBearingUnits> connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(connectorData);
        if (!connectorPtr)
            return;
        _connectorData = connectorData;

        // 绑定通用拾取控件持久化策略（单值写入 node1_/node2_）
        if (_node1PickController)
        {
            _node1PickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorBearingUnits::node1_)));
        }
        if (_node2PickController)
        {
            _node2PickController->setPersistenceStrategy(std::make_shared<DataClassPersistenceStrategy>(
                makeSingleValueWriter(connectorPtr, &MechConnectorData::mConnectorBearingUnits::node2_)));
        }

        typeWid_.combobox->blockSignals(true);
        cbRotate_.combobox->blockSignals(true);
        typeWid_.combobox->setCurrentIndex(connectorPtr->type_);
        typeWid_.combobox->blockSignals(false);

        // 按类型刷新节点2显隐（隐藏态拒绝激活并清理高亮）
        applyNode2Visible();

        // 节点1反显
        if (_node1PickController)
        {
            PickWidgetData node1Data;
            if (connectorPtr->node1_.first != MxFunctions::PickDataType::None && connectorPtr->node1_.second.result_id > 0)
            {
                node1Data.pickedData[connectorPtr->node1_.first].push_back(connectorPtr->node1_.second);
            }
            _node1PickController->displayData(node1Data, true);
        }

        // 节点2反显
        if (_node2PickController)
        {
            PickWidgetData node2Data;
            if (connectorPtr->node2_.first != MxFunctions::PickDataType::None && connectorPtr->node2_.second.result_id > 0)
            {
                node2Data.pickedData[connectorPtr->node2_.first].push_back(connectorPtr->node2_.second);
            }
            _node2PickController->displayData(node2Data, true);
        }

        QStringList coordinateList = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate)
            .getAllCoordinateNames();
        setCoordinateOptions(coordinateList);
        int connectorID = connectorData->getConnectorId();
        auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
        int coor1 = mapRelation.getMapRelation<int>(
            { Physical_Connector, connectorID },
            Physical_Coordinate
        );
        int coor2 = mapRelation.getMapRelation<int>(
            { Physical_Connector, connectorID },
            Physical_Coordinate, MapRelation_Coord2
        );
        coordinateWid1_.combobox->blockSignals(true);
        coordinateWid2_.combobox->blockSignals(true);
        if (coor1 != -1)
        {
            QString name1 = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coor1);
            coordinateWid1_.combobox->setCurrentText(name1);
        }
        if (coor2 != -1)
        {
            QString name2 = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coor2);
            coordinateWid2_.combobox->setCurrentText(name2);
        }
        coordinateWid1_.combobox->blockSignals(false);
        coordinateWid2_.combobox->blockSignals(false);

        rotateFaceWid_.combobox->blockSignals(true);
        rotateFaceWid_.combobox->setCurrentIndex(connectorPtr->rotateFace_);
        rotateFaceWid_.combobox->blockSignals(false);
        if (connectorPtr->defMode_ == 0 && connectorPtr->bearingPara_.size() == 8)
        {
            K11Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[0]));
            K22Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[1]));
            K12Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[2]));
            K21Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[3]));
            C11Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[4]));
            C22Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[5]));
            C12Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[6]));
            C21Input_.lineeditvalue->setExpression(MxFunctions::mNumberToString(connectorPtr->bearingPara_[7]));
        }
        cbRotate_.combobox->blockSignals(true);
        cbRotate_.combobox->setCurrentIndex(connectorPtr->defMode_);
        cbRotate_.combobox->blockSignals(false);
        if (connectorPtr->defMode_ == 1 && connectorPtr->curveRotate_.size() != 0)
        {
            QVector<QVector<double>> curveRotate_;
            int rows = connectorPtr->curveRotate_.size();
            int cols = connectorPtr->curveRotate_[0].size();
            QVector<QVector<double>> newdata(cols, QVector<double>(rows));
            for (int i = 0; i < rows; ++i) {
                int currentCols = connectorPtr->curveRotate_[i].size();
                for (int j = 0; j < currentCols && j < cols; ++j) {
                    newdata[j][i] = connectorPtr->curveRotate_[i][j];
                }
            }
            mTableDataWindow::getInstance()->setTableData(newdata);
            mTableDataWindow::getInstance()->setUseCurve(false);
            mTableDataWindow::getInstance()->setHorizontalHeaders(bearingHeaderText_);
            mTableDataWindow::getInstance()->setUseCurve(true);
        }

        // 默认激活节点1拾取控件
        if (_node1PickController && !_node1PickController->isPickActive())
        {
            _node1PickController->setPickActive(true);
        }

        // 反显后实时渲染连接符号并显示临时节点
        updateConnectorSymbolRealtime();
        rendFreeNodes(true);
    }

    void mConnectorBearingUnits::setCoordinateOptions(const QStringList& options)
    {
        coordinateWid1_.combobox->clear();
        coordinateWid2_.combobox->clear();
        for (auto name : options)
        {
            coordinateWid1_.combobox->addItem(name);
            coordinateWid2_.combobox->addItem(name);
        }
    }

    void mConnectorBearingUnits::slot_defModeChanged()
    {
        bearingPara_->removeAll();
        bearingPara_->addRow(cbRotate_.propwid);
        cbRotate_.propwid->show();
        if (cbRotate_.combobox->currentIndex() == 0)
        {
            mTableDataWindow::getInstance()->clearAndReset(0, 0);
            bearingPara_->addRow(K11Input_.propwid);
            bearingPara_->addRow(K22Input_.propwid);
            bearingPara_->addRow(K12Input_.propwid);
            bearingPara_->addRow(K21Input_.propwid);
            bearingPara_->addRow(C11Input_.propwid);
            bearingPara_->addRow(C22Input_.propwid);
            bearingPara_->addRow(C12Input_.propwid);
            bearingPara_->addRow(C21Input_.propwid);
            K11Input_.propwid->show();
            K22Input_.propwid->show();
            K12Input_.propwid->show();
            K21Input_.propwid->show();
            C11Input_.propwid->show();
            C22Input_.propwid->show();
            C12Input_.propwid->show();
            C21Input_.propwid->show();
        }
        bearingPara_->refreshLayout();
        saveDefMode();
    }

    QVector<QWidget*> mConnectorBearingUnits::getBearingUnits(FoldSection* BearingUnits)
    {
        bearingUnits_ = BearingUnits;
        QVector<QWidget*>wds;
        wds.append(typeWid_.propwid);
        if (_node1PickController)
        {
            wds.append(_node1PickController->widget());
        }
        wds.append(coordinateWid1_.propwid);
        if (typeWid_.combobox->currentIndex() == 1)
        {
            if (_node2PickController)
            {
                wds.append(_node2PickController->widget());
            }
            wds.append(coordinateWid2_.propwid);
        }
        wds.append(rotateFaceWid_.propwid);
        return wds;
    }

    QVector<QWidget*> mConnectorBearingUnits::getBearingPara(FoldSection* BearingPara)
    {
        bearingPara_ = BearingPara;
        QVector<QWidget*>wds;
        wds.append(cbRotate_.propwid);
        if (cbRotate_.combobox->currentIndex() == 0)
        {
            wds.append(K11Input_.propwid);
            wds.append(K22Input_.propwid);
            wds.append(K12Input_.propwid);
            wds.append(K21Input_.propwid);
            wds.append(C11Input_.propwid);
            wds.append(C22Input_.propwid);
            wds.append(C12Input_.propwid);
            wds.append(C21Input_.propwid);
        }
        return wds;
    }

    void mConnectorBearingUnits::rendFreeNode(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id, bool showhide)
    {
        std::vector<int> tempIds;
        if (!nodes_id.empty())
        {
            if (nodes_id[0].first == PickDataType::Node)
            {
                auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ nodes_id[0].second });
                if (!tempnodes.empty() && tempnodes[0])
                {
                    tempIds.push_back(nodes_id[0].second);
                }
            }
        }
        if (tempIds.empty())
            return;
        mDisplayMenuFunction::getInstance()->slot_showFreeNode(tempIds, showhide);
    }

    void mConnectorBearingUnits::rendFreeNodes(bool showhide)
    {
        std::vector<int> tempIds;
        auto collectTemp = [&](const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pairs) {
            for (const auto& pair : pairs)
            {
                if (pair.first != PickDataType::Node)
                {
                    continue;
                }
                auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ pair.second.result_id });
                if (!tempnodes.empty() && tempnodes[0])
                {
                    tempIds.push_back(pair.second.result_id);
                }
            }
            };
        collectTemp(getWidgetDataAsPickPairs(true));
        collectTemp(getWidgetDataAsPickPairs(false));
        if (tempIds.empty())
            return;
        mDisplayMenuFunction::getInstance()->slot_showFreeNode(tempIds, showhide);
    }

    void mConnectorBearingUnits::deleteNode1()
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>> node1Ids;
        auto pairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : pairs)
        {
            node1Ids.push_back({ pair.first, pair.second.result_id });
        }
        rendFreeNode(node1Ids, false);
        std::vector<int> node1;
        for (const auto& id : node1Ids)
        {
            if (id.first == PickDataType::Node)
            {
                node1.push_back(id.second);
            }
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(node1);
    }

    void mConnectorBearingUnits::deleteNode2()
    {
        std::vector<std::pair<MxFunctions::PickDataType, int>> node2Ids;
        auto pairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : pairs)
        {
            node2Ids.push_back({ pair.first, pair.second.result_id });
        }
        rendFreeNode(node2Ids, false);
        std::vector<int> node2;
        for (const auto& id : node2Ids)
        {
            if (id.first == PickDataType::Node)
            {
                node2.push_back(id.second);
            }
        }
        getCVM->getModelData()->getMeshData()->deleteConnectNodes(node2);
    }

    void mConnectorBearingUnits::appendNode1()
    {
        std::vector<int> node1Id;
        auto pairs = getWidgetDataAsPickPairs(true);
        for (const auto& pair : pairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            node1Id.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(node1Id);
    }

    void mConnectorBearingUnits::appendNode2()
    {
        std::vector<int> node2Id;
        auto pairs = getWidgetDataAsPickPairs(false);
        for (const auto& pair : pairs)
        {
            if (pair.first != PickDataType::Node)
            {
                continue;
            }
            node2Id.push_back(pair.second.result_id);
        }
        getCVM->getModelData()->getMeshData()->appendConnectNodes(node2Id);
    }

    void mConnectorBearingUnits::slot_typeChanged()
    {
        saveType();

        // 切换类型时清空节点，避免残留数据
        if (_node1PickController)
        {
            _node1PickController->replacePickedDataAndClearSets({});
        }
        if (_node2PickController)
        {
            _node2PickController->replacePickedDataAndClearSets({});
        }

        // 先重建布局行，再刷新节点2显隐：
        // removeAll 会将控件脱离父窗口，若先对未入布局的无父控件
        // setVisible(true)，控件会作为独立顶层窗口弹出
        bearingUnits_->removeAll();
        bearingUnits_->addRow(typeWid_.propwid);
        typeWid_.propwid->show();
        if (_node1PickController)
        {
            bearingUnits_->addRow(_node1PickController->widget());
            _node1PickController->widget()->show();
        }
        bearingUnits_->addRow(coordinateWid1_.propwid);
        coordinateWid1_.propwid->show();
        if (typeWid_.combobox->currentIndex() == 1)
        {
            if (_node2PickController)
            {
                bearingUnits_->addRow(_node2PickController->widget());
                _node2PickController->widget()->show();
            }
            bearingUnits_->addRow(coordinateWid2_.propwid);
            coordinateWid2_.propwid->show();
        }
        bearingUnits_->addRow(rotateFaceWid_.propwid);
        rotateFaceWid_.propwid->show();
        bearingUnits_->refreshLayout();

        // 按新类型刷新节点2显隐（此时节点2控件已入布局）
        applyNode2Visible();

        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::slot_node1PickDataChanged()
    {
        // 先移除旧连接节点，再按新数据重建（含几何点→临时节点转换）
        deleteNode1();
        auto pairs = getWidgetDataAsPickPairs(true);
        if (!pairs.empty())
        {
            std::vector<std::pair<MxFunctions::PickDataType, int>> nodeIds{ { pairs[0].first, pairs[0].second.result_id } };
            transPickedToNode(nodeIds);
            if (!nodeIds.empty())
            {
                PickWidgetData data;
                data.pickedData[nodeIds[0].first].push_back(RenderPickResultData(nodeIds[0].second, -1, -1));
                _node1PickController->displayData(data, true);
            }
        }
        appendNode1();
        rendFreeNodes(true);
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::slot_node2PickDataChanged()
    {
        // 先移除旧连接节点，再按新数据重建（含几何点→临时节点转换）
        deleteNode2();
        auto pairs = getWidgetDataAsPickPairs(false);
        if (!pairs.empty())
        {
            std::vector<std::pair<MxFunctions::PickDataType, int>> nodeIds{ { pairs[0].first, pairs[0].second.result_id } };
            transPickedToNode(nodeIds);
            if (!nodeIds.empty())
            {
                PickWidgetData data;
                data.pickedData[nodeIds[0].first].push_back(RenderPickResultData(nodeIds[0].second, -1, -1));
                _node2PickController->displayData(data, true);
            }
        }
        appendNode2();
        rendFreeNodes(true);
        updateConnectorSymbolRealtime();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveType()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->type_ = typeWid_.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveCoordinate1()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        if (coordinateWid1_.combobox->currentText() != "全局坐标系")
        {
            int coordinateId = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coordinateWid1_.combobox->currentText());
            mapRelationManager.appendMapRelation({ Physical_Connector ,connectorPtr->getConnectorId() }, { Physical_Coordinate,coordinateId });
        }
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveCoordinate2()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
        if (coordinateWid2_.combobox->currentText() != "全局坐标系")
        {
            int coordinateId = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coordinateWid2_.combobox->currentText());
            mapRelationManager.appendMapRelation({ Physical_Connector ,connectorPtr->getConnectorId() }, { Physical_Coordinate,coordinateId }, MapRelation_Coord2);
        }
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveRotateFace()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->rotateFace_ = rotateFaceWid_.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveDefMode()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        connectorPtr->defMode_ = cbRotate_.combobox->currentIndex();
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveBearingPara()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        if (cbRotate_.combobox->currentIndex() == 0)
        {
            connectorPtr->bearingPara_.clear();
            connectorPtr->bearingPara_.push_back(K11Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(K22Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(K12Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(K21Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(C11Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(C22Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(C12Input_.lineeditvalue->text().toDouble());
            connectorPtr->bearingPara_.push_back(C21Input_.lineeditvalue->text().toDouble());
        }
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::saveCurveRotate()
    {
        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
        {
            return;
        }
        if (cbRotate_.combobox->currentIndex() != 1)
        {
            return;
        }
        auto table = mTableDataWindow::getInstance()->getTableWidget();
        int rows = table->rowCount();
        if (rows != 0)
        {
            connectorPtr->curveRotate_.clear();
            connectorPtr->curveRotate_.resize(rows);
            for (int i = 0; i < rows; ++i)
            {
                connectorPtr->curveRotate_[i].resize(9);
                for (int j = 0; j < 9; ++j)
                {
                    QTableWidgetItem* item = table->item(i, j);
                    if (item)
                    {
                        bool ok = false;
                        double val = item->text().toDouble(&ok);
                        connectorPtr->curveRotate_[i][j] = ok ? val : 0;
                    }
                    else
                    {
                        connectorPtr->curveRotate_[i][j] = 0;
                    }
                }
            }
        }
        notifyConnectorChanged();
    }

    void mConnectorBearingUnits::slot_tableEditFinished(QVector<QVector<double>> x, QVector<QVector<double>> y)
    {
        saveCurveRotate();
    }

    void mConnectorBearingUnits::updateConnectorSymbolRealtime()
    {
        if (!_node1PickController || !_node2PickController || !_connectorData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        auto meshData = getCVM->getModelData()->getMeshData();
        if (!meshData)
            return;

        auto connectorPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectorBearingUnits>(_connectorData);
        if (!connectorPtr)
            return;

        // 节点1：取第一个有效对象
        QVector3D node1Pos;
        bool hasNode1 = false;
        auto node1Pairs = getWidgetDataAsPickPairs(true);
        if (!node1Pairs.empty() && node1Pairs[0].first == PickDataType::Node)
        {
            int node1Id = node1Pairs[0].second.result_id;
            if (meshData->NodeIsHaveByID(node1Id))
            {
                node1Pos = meshData->getVectorById(node1Id);
                hasNode1 = true;
            }
        }

        // 节点2：仅"几何体-几何体"时参与渲染
        QVector3D node2Pos;
        bool hasNode2 = false;
        if (connectorPtr->type_ == 1)
        {
            auto node2Pairs = getWidgetDataAsPickPairs(false);
            if (!node2Pairs.empty() && node2Pairs[0].first == PickDataType::Node)
            {
                int node2Id = node2Pairs[0].second.result_id;
                if (meshData->NodeIsHaveByID(node2Id))
                {
                    node2Pos = meshData->getVectorById(node2Id);
                    hasNode2 = true;
                }
            }
        }

        auto render = AllRenderSymbolManager::getInstance();
        if (hasNode1 && hasNode2)
        {
            QVector<QVector3D> node2PosV;
            node2PosV.push_back(node2Pos);
            render->assignConnector(_connectorData, node1Pos, node2PosV, ConnectorType::Connector_BearingUnits);
        }
        else
        {
            // 数据不完整时清除实时符号，避免残留旧连线
            render->assignConnector(_connectorData, QVector3D(), QVector<QVector3D>(), ConnectorType::Connector_BearingUnits);
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController)
            symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> mConnectorBearingUnits::getWidgetDataAsPickPairs(bool isNode1) const
    {
        std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> result;
        mPickWidgetController* ctrl = isNode1 ? _node1PickController : _node2PickController;
        if (!ctrl) return result;
        const auto& allData = ctrl->getAllData();
        for (const auto& pair : allData.pickedData)
        {
            MxFunctions::PickDataType type = pair.first;
            for (const auto& d : pair.second)
            {
                result.emplace_back(type, d);
            }
        }
        return result;
    }

    void mConnectorBearingUnits::applyNode2Visible()
    {
        if (!_node2PickController)
            return;
        const bool visible = (typeWid_.combobox->currentIndex() == 1);
        // 控件尚未加入布局（无父窗口）时不直接显示：
        // 无父控件 setVisible(true) 会作为独立顶层窗口弹出，
        // 显示交由布局装配路径（getBearingUnits / slot_typeChanged 的 addRow+show）完成
        if (visible && !_node2PickController->widget()->parentWidget())
            return;
        _node2PickController->setPickWidgetVisible(visible);
        coordinateWid2_.propwid->setVisible(visible);
    }

    void mConnectorBearingUnits::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        for (auto* ctrl : { _node1PickController, _node2PickController })
        {
            if (!ctrl) continue;
            ctrl->setKeepHighlightOnDeactivate(false);
            const bool wasActive = ctrl->isPickActive();
            ctrl->setPickActive(false);
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            ctrl->setKeepHighlightOnDeactivate(true);
        }
    }
}