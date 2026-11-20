#include "mDeputy.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mDisplayMenuFunction.h"
#include "PickCommonFunction.h"
#include "mInfoWindow.h"
#include "mDockLayoutManager.h"
#include "AllRenderSymbolManager.h"
//Geo
#include "mGeoManager.h"
//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"
//MechModelData
#include "mMeshData.h"
#include "mDataElement.h"
#include "mSetBase.h"
#include "mMixSet.h"
#include "mSetManager.h"
#include "mNode.h"
#include "mElementSurface.h"
#include "mGeoSet.h"
#include "mCoordinateManager.h"
#include "mCoordinateBase.h"
#include <qmessagebox.h>
#include <QStringListModel>
#include <QCompleter>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
using namespace MechElementData;
using namespace mxr;
using namespace MechSetData;

namespace MechMainWindow
{

/*************************************************连接副面板管理器***************************************************************/
	mDeputyPanelManager::mDeputyPanelManager(QObject* parent)
		: QObject(parent)
	{
		m_DeputyConnector = new mDeputyConnectorPanel();
		m_DeputyFastener = new mDeputyFastenerPanel();
	}

	mDeputyPanelManager::~mDeputyPanelManager()
	{
		delete m_DeputyConnector;
		delete m_DeputyFastener;
	}

	mDeputyPanelManager* mDeputyPanelManager::_instance = nullptr;

	mDeputyPanelManager* mDeputyPanelManager::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mDeputyPanelManager;
		}
		return _instance;
	}

	mDeputyPanelBase* mDeputyPanelManager::getPanelByType(MxFunctions::DeputyType type)
	{
		switch (type)
		{
		case MxFunctions::Deputy_Connector:
			return m_DeputyConnector;
		case MxFunctions::Deputy_Fastener:
			return m_DeputyFastener;
		default:
			break;
		}
		return nullptr;
	}

/*************************************************连接副基类***************************************************************/
	mDeputyPanelBase::mDeputyPanelBase(QWidget* parent)
		: mPanelBase(parent)
	{}

	mDeputyPanelBase::~mDeputyPanelBase()
	{}

	void mDeputyPanelBase::slot_finishedPick()
	{
		int objNum = 0;
		SaveSelectMemoryData(getCVM->getPreRenderWidget(), getCVM->getCurrentPickDataType(objNum), _pickeddata);
		if (_pickeddata.empty())
		{
			auto partIds = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (int i = 0; i < partIds.size(); ++i)
			{
				RenderPickResultData pickeddata;
				pickeddata.part_id = partIds[i];
				pickeddata.result_id = partIds[i];
				_pickeddata.push_back({ PickDataType::Part,pickeddata });
			}
		}
		emit sg_connectorPickFinished();
	}

	void mDeputyPanelBase::connectPickSig()
	{
		if (getCVM == nullptr ||
			getCVM->getPreRenderWidget() == nullptr ||
			getCVM->getPreRenderWidget()->getSelectionContainer() == nullptr)
			return;
		connect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mDeputyPanelBase::slot_finishedPick);
	}

	void mDeputyPanelBase::disconnectPickSig()
	{
		if (getCVM == nullptr ||
			getCVM->getPreRenderWidget() == nullptr ||
			getCVM->getPreRenderWidget()->getSelectionContainer() == nullptr)
			return;
		disconnect(getCVM->getPreRenderWidget()->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mDeputyPanelBase::slot_finishedPick);
	}

	void mDeputyPanelBase::clearPick()
	{
		if (getCVM == nullptr ||
			getCVM->getPreRenderWidget() == nullptr)
			return;

		ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
		ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
		UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}

	bool mDeputyPanelBase::getPickVecData(QVector3D& vec)
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

	bool mDeputyPanelBase::getPickPointData(QVector3D& point)
	{
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto meshData = getCVM->getModelData()->getMeshData();
		auto nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
		if (nodes.size() == 1)
		{
			double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[nodes.at(0).data_id.node_id] * 3;
			point = QVector3D(vertex[0], vertex[1], vertex[2]);
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

	void mDeputyPanelBase::setDeputyData(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)
	{
		_deputyData = deputyData;
	}

	QString mDeputyPanelBase::PickDataEnum2Str(MxFunctions::PickDataType type)
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

	int mDeputyPanelBase::calculateMasterNode(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickedData)
	{
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
		int nodeId = getCVM->createFreeNode(coords);
		mGlobalSignals::getInstance()->UpDateView();
		return nodeId;
	}

	QVector<QVector3D> mDeputyPanelBase::getNodesCenter(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
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

	void mDeputyPanelBase::transPickedToNode(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
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

	QVector<QVector3D> mDeputyPanelBase::transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
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

	void mDeputyPanelBase::getNodesFromPicked(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id)
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
						trans_res.push_back({ PickDataType::Node ,e2n[i].result_id});
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
						trans_res.push_back({ PickDataType::Node ,part2n[i]});
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

	void mDeputyPanelBase::pickedObjFilter(PosPickWidget& pickwid)
	{
		auto pickeddata = pickwid.nodes_;
		auto pickedids = pickwid.nodes_id;
		auto srcpos = pickwid.renderpos;
		pickwid.renderpos.clear();
		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>newdata;
		std::vector<std::pair<MxFunctions::PickDataType, int>>newids;
		auto meshData = getCVM->getModelData()->getMeshData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDataBase = getCVM->getModelData()->getGeoData();
		auto PartIdsVec = meshData->getAllPartIds();
		QSet<int>allPartIds;
		for (int i = 0; i < PartIdsVec.size(); ++i)
			allPartIds.insert(PartIdsVec[i]);
		for (int i = 0; i < pickeddata.size(); ++i)
		{
			auto picked = pickeddata[i];
			auto pickedid = pickedids[i];
			auto pickedtype = picked.first;
			switch (pickedtype)
			{
			case PickDataType::GeoSolid:
			{
				//【OCC摘除·S10】旧栈 set 反查随冻结岛退役（恒空壳），实体→部件后续由 GN 反查承接（保档）
				int partId = -1;
				/*auto geoDataModelSet = geoDataBase->getPartSetByGeoSolidID(picked.second.result_id);
				int partId = -1;
				if (geoDataModelSet)
				{
					partId = geoDataModelSet->getPartID();
				}*/
				if (partId > 0 && allPartIds.contains(partId))
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				break;
			}
			case PickDataType::GeoFace:
			{
				auto partId = meshData->getPartIdByGeoFace(picked.second.result_id);
				if (partId > 0 && allPartIds.contains(partId))
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				break;
			}
			case PickDataType::GeoEdge:
			{
				auto partId = meshData->getPartIdByGeoEdge(picked.second.result_id);
				if (partId > 0 && allPartIds.contains(partId))
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				break;
			}
			case PickDataType::GeoPoint:
			{
				auto partId = meshData->getPartIdByGeoPoint(picked.second.result_id);
				if (partId > 0 && allPartIds.contains(partId))
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				break;
			}
			case PickDataType::Node:
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(picked.second.result_id);
				bool isok = true;
				for (auto partId : partIds)
				{
					if (partId <= 0 || !allPartIds.contains(partId))
					{
						isok = false;
					}
				}
				if (isok)
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				else
				{
					auto tempnodes = getCVM->getModelData()->getMeshData()->isTempNode({ picked.second.result_id });
					if (!tempnodes.empty() && tempnodes[0])
					{
						newdata.push_back(picked);
						newids.push_back(pickedid);
					}
				}
				break;
			}
			case PickDataType::Part:
			{
				if (allPartIds.contains(pickedid.second))
				{
					newdata.push_back(picked);
					newids.push_back(pickedid);
				}
				break;
			}
			case PickDataType::ElementEdge:
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(picked.second.result_id);
				for (auto partId : partIds)
				{
					if (partId > 0 && allPartIds.contains(partId))
					{
						newdata.push_back(picked);
						newids.push_back({ picked.first,picked.second.result_id });
					}
				}
				break;
			}
			case PickDataType::ElementFace:
			{
				newdata.push_back(picked);
				newids.push_back(pickedid);
				if (pickwid.renderpos.isEmpty())
					pickwid.renderpos = srcpos;
				break;
			}
			case PickDataType::Element:
			{
				break;
			}
			default:
				break;
			}
		}
		pickwid.nodes_ = newdata;
		pickwid.nodes_id = newids;
		pickwid.renderpos = transPickedToNodes(pickwid.nodes_id);
	}

	void mDeputyPanelBase::highlightPicked(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& nodes_, bool hl)
	{
		std::vector<int> Ids_GeoPoint;
		std::vector<int> Ids_GeoEdge;
		std::vector<int> Ids_GeoFace;
		std::vector<int> Ids_GeoSolid;
		std::vector<mxr::MeshSelectInfoData> Ids_ElementEdge;
		std::vector<mxr::MeshSelectInfoData> Ids_ElementFace;
		std::vector<mxr::MeshSelectInfoData> meshIds;
		for (int i = 0; i < nodes_.size(); ++i)
		{
			auto type = nodes_[i].first;
			switch (type)
			{
			case MxFunctions::PickDataType::GeoPoint:
				Ids_GeoPoint.push_back(nodes_[i].second.result_id);
				break;
			case MxFunctions::PickDataType::GeoEdge:
				Ids_GeoEdge.push_back(nodes_[i].second.result_id);
				break;
			case MxFunctions::PickDataType::GeoFace:
				Ids_GeoFace.push_back(nodes_[i].second.result_id);
				break;
			case MxFunctions::PickDataType::GeoSolid:
				Ids_GeoSolid.push_back(nodes_[i].second.result_id);
				break;
			/*
			* 单元边，面高亮暂时屏蔽，数据结构特殊，无法获取完整信息，会导致崩溃
			*/
			case MxFunctions::PickDataType::Node:
			{
				mxr::MeshSelectInfoData n;
				n.data_id.element_id = nodes_[i].second.result_id;
				meshIds.push_back(n);
			}
			break;
			default:
				break;
			}
		}

		SelectionOperate highlight = hl ? SelectionOperate::ADD : SelectionOperate::REDUCE;
		if (!Ids_GeoPoint.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoPoint, Ids_GeoPoint);
		}
		if (!Ids_GeoEdge.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoEdge, Ids_GeoEdge);
		}
		if (!Ids_GeoFace.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoFace, Ids_GeoFace);
		}
		if (!Ids_GeoSolid.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(highlight, SelectionInfoType::GeoSolid, Ids_GeoSolid);
		}
		if (!meshIds.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(highlight, SelectionInfoType::MeshNode, meshIds);
		}
		if (!Ids_ElementEdge.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(highlight, SelectionInfoType::MeshLine, Ids_ElementEdge);
		}
		if (!Ids_ElementFace.empty())
		{
			getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(highlight, SelectionInfoType::MeshFace, Ids_ElementFace);
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		getCVM->getPreRenderWidget()->updatePaint();
	}

/*************************************************连接副(Deputy Connector)***************************************************************/
	mDeputyConnectorPanel::mDeputyConnectorPanel(QWidget* parent)
        :mDeputyPanelBase(parent)
	{
		initInputs();
	}

	mDeputyConnectorPanel::~mDeputyConnectorPanel()
	{
		clearInputs();
	}

	void mDeputyConnectorPanel::initInputs()
	{
		pickingRow_ = -1;
		pickingCol_ = -1;
		refNodeIds_.clear();
		movNodeIds_.clear();

		// 类型列表
		typeList_ = QStringList{ "球铰", "滑动", "铰接", "焊接", "圆柱", "平移" };
		subTypeList_ = QStringList{ "旋转", "对齐", "万向节" };

		// 表格
		table_ = new QTableWidget();
		table_->setColumnCount(3);
		table_->setHorizontalHeaderLabels(QStringList{ "序号", "参考位置", "移动位置" });
		table_->horizontalHeader()->setStretchLastSection(true);
		// 单击选中单元格，不进入编辑；双击进入编辑；键盘输入可直接输入
		table_->setSelectionBehavior(QAbstractItemView::SelectItems);
		table_->setSelectionMode(QAbstractItemView::SingleSelection);
		table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
		// 隐藏表格自带的垂直表头（行号），使用自定义表头序号列
		table_->verticalHeader()->setVisible(false);
		connect(table_, &QTableWidget::cellClicked, this, &mDeputyConnectorPanel::slot_cellClicked);
		connect(table_, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
			if (col == 1 || col == 2)
			{
				table_->editItem(table_->item(row, col));
			}
		});
		connect(table_, &QTableWidget::cellChanged, this, &mDeputyConnectorPanel::slot_movCellChanged);
		connect(this, &mDeputyConnectorPanel::sg_connectorPickFinished, this, &mDeputyConnectorPanel::slot_pickFinished);

		// 按钮
		btnAddRow_ = new QPushButton("新增");
		btnDelRow_ = new QPushButton("删除");
		btnConfirm_ = new QPushButton("确定");
		btnAddRow_->setFixedHeight(28);
		btnDelRow_->setFixedHeight(28);
		btnConfirm_->setFixedHeight(28);
		connect(btnAddRow_, &QPushButton::clicked, this, &mDeputyConnectorPanel::slot_addRow);
		connect(btnDelRow_, &QPushButton::clicked, this, &mDeputyConnectorPanel::slot_delRow);
		connect(btnConfirm_, &QPushButton::clicked, this, &mDeputyConnectorPanel::slot_confirmRow);

		// 参数区控件
		typeCombo_.initWidget("类型");
		typeCombo_.combobox->addItems(typeList_);
		connect(typeCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mDeputyConnectorPanel::slot_typeChanged);
		typeCombo_.combobox->blockSignals(true);
		typeCombo_.combobox->setCurrentIndex(0);
		typeCombo_.combobox->blockSignals(false);

		subTypeCombo_.initWidget("子类型");
		subTypeCombo_.combobox->addItems(subTypeList_);
		connect(subTypeCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mDeputyConnectorPanel::slot_subTypeChanged);
		subTypeCombo_.combobox->blockSignals(true);
		subTypeCombo_.combobox->setCurrentIndex(0);
		subTypeCombo_.combobox->blockSignals(false);

		combocoor_.initWidget("坐标系");
		combocoor_.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png"));
		combocoor_.pushbtn->setToolTip("创建坐标系");
		auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
		for (auto coorname : allCoor)
		{
			combocoor_.combobox->addItem(coorname);
		}
		connect(combocoor_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mDeputyConnectorPanel::slot_coorChanged);
		connect(combocoor_.pushbtn, &QPushButton::clicked, this, &mDeputyConnectorPanel::slot_createCoor);
		combocoor_.combobox->blockSignals(true);
		combocoor_.combobox->setCurrentIndex(0);
		combocoor_.combobox->blockSignals(false);

	}

	void mDeputyConnectorPanel::clearInputs()
	{
		pickingRow_ = -1;
		pickingCol_ = -1;
		refNodeIds_.clear();
		movNodeIds_.clear();
		clearPreview();
		if (_deputyData)
			AllRenderSymbolManager::getInstance()->clearDeputy(_deputyData);
		if (table_)
		{
			disconnect(table_, &QTableWidget::cellClicked, this, &mDeputyConnectorPanel::slot_cellClicked);
			disconnect(table_, &QTableWidget::cellChanged, this, &mDeputyConnectorPanel::slot_movCellChanged);
			delete table_;
			table_ = nullptr;
		}
		if (btnAddRow_) { delete btnAddRow_; btnAddRow_ = nullptr; }
		if (btnDelRow_) { delete btnDelRow_; btnDelRow_ = nullptr; }
		if (btnConfirm_) { delete btnConfirm_; btnConfirm_ = nullptr; }
		typeCombo_.clear();
		subTypeCombo_.clear();
		combocoor_.clear();
	}

	void mDeputyConnectorPanel::setInputsValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)
	{
		std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(deputyData);
		if (!deputyPtr)
			return;

		pickingRow_ = -1;
		pickingCol_ = -1;
		clearPreview();
		if (_deputyData)
			AllRenderSymbolManager::getInstance()->clearDeputy(_deputyData);
		clearPick();
		_pickeddata.clear();
		refNodeIds_ = deputyPtr->refNodeIds_;
		movNodeIds_ = deputyPtr->movNodeIds_;

		// 刷新表格
		table_->blockSignals(true);
		table_->setRowCount(refNodeIds_.size());
		for (int i = 0; i < refNodeIds_.size(); ++i)
		{
			QTableWidgetItem* itemIdx = new QTableWidgetItem(MxFunctions::mNumberToString(i + 1));
			itemIdx->setFlags(itemIdx->flags() & ~Qt::ItemIsEditable);
			itemIdx->setTextAlignment(Qt::AlignCenter);
			table_->setItem(i, 0, itemIdx);

			QTableWidgetItem* itemRef = new QTableWidgetItem(refNodeIds_[i] > 0 ? MxFunctions::mNumberToString(refNodeIds_[i]) : "");
			table_->setItem(i, 1, itemRef);

			QTableWidgetItem* itemMov = new QTableWidgetItem(movNodeIds_[i] > 0 ? MxFunctions::mNumberToString(movNodeIds_[i]) : "");
			table_->setItem(i, 2, itemMov);
		}
		table_->blockSignals(false);

		// 类型
		int typeIdx = typeList_.indexOf(deputyPtr->datatype_);
		if (typeIdx >= 0)
		{
			typeCombo_.combobox->blockSignals(true);
			typeCombo_.combobox->setCurrentIndex(typeIdx);
			typeCombo_.combobox->blockSignals(false);
		}

		// 子类型
		int subIdx = subTypeList_.indexOf(deputyPtr->subdatatype_);
		if (subIdx >= 0)
		{
			subTypeCombo_.combobox->blockSignals(true);
			subTypeCombo_.combobox->setCurrentIndex(subIdx);
			subTypeCombo_.combobox->blockSignals(false);
		}

		// 坐标系
		combocoor_.combobox->blockSignals(true);
		combocoor_.combobox->clear();
		auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
		for (auto coorname : allCoor)
		{
			combocoor_.combobox->addItem(coorname);
		}
		auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
		int coorid = mapRelationManager.getMapRelation<int>({ PhysicalFieldType::Physical_Deputy, deputyData->getDeputyId() }, Physical_Coordinate);
		if (coorid == -1)
		{
			combocoor_.combobox->setCurrentIndex(0);  // "全局坐标系" 在索引 0
		}
		else
		{
			QString coorname = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coorid);
			combocoor_.combobox->setCurrentText(coorname);
		}
		combocoor_.combobox->blockSignals(false);

		updateSubTypeVisible();
	}

    QVector<QWidget*> mDeputyConnectorPanel::getPosInputs(FoldSection* fapos)
    {
        if (!fapos)
        {
            return {};
        }

        fapos_ = fapos;
        QVector<QWidget*> widgets;

        widgets.append(table_);

        // 表格与按钮之间的分割线
        /*QFrame* line = new QFrame(fapos);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setLineWidth(1);
        line->setMidLineWidth(0);
        widgets.append(line);*/
        /*QWidget* spacer = new QWidget(fapos);
        spacer->setFixedHeight(0);
        widgets.append(spacer);*/

        QWidget* bottomButtonBar = new QWidget(fapos);
        QHBoxLayout* bottomLayout = new QHBoxLayout(bottomButtonBar);

        bottomLayout->setContentsMargins(0, 0, 0, 0);

        bottomLayout->addStretch();

        bottomLayout->addWidget(btnAddRow_);
        bottomLayout->addWidget(btnDelRow_);
        bottomLayout->addWidget(btnConfirm_);

        widgets.append(bottomButtonBar);

        return widgets;
    }


	QVector<QWidget*> mDeputyConnectorPanel::getValInputs(FoldSection* faval)
	{
		faval_ = faval;
		QVector<QWidget*> wds;

		wds.append(typeCombo_.propwid);
		wds.append(subTypeCombo_.propwid);
		wds.append(combocoor_.propwid);

		return wds;
	}

	void mDeputyConnectorPanel::storeDeputyValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)
	{
		std::shared_ptr<MechConnectorData::mConnectingDeputy> deputyPtr = std::dynamic_pointer_cast<MechConnectorData::mConnectingDeputy>(deputyData);
		if (!deputyPtr)
			return;

		// 从表格读取数据
		int rowCount = table_->rowCount();
		deputyPtr->refNodeIds_.clear();
		deputyPtr->movNodeIds_.clear();
		for (int i = 0; i < rowCount; ++i)
		{
			int refId = table_->item(i, 1) ? table_->item(i, 1)->text().toInt() : 0;
			int movId = table_->item(i, 2) ? table_->item(i, 2)->text().toInt() : 0;
			deputyPtr->refNodeIds_.append(refId);
			deputyPtr->movNodeIds_.append(movId);
		}

		// 类型
		deputyPtr->datatype_ = typeCombo_.combobox->currentText();

		// 子类型
		deputyPtr->subdatatype_ = subTypeCombo_.combobox->currentText();

		// 坐标系通过映射存储，不存到数据类
		QString coorname = combocoor_.combobox->currentText();
		int coorid = -1;  // 默认全局坐标系
		if (coorname == "全局坐标系")
		{
			coorid = -1;
		}
		else if (!coorname.isEmpty())
		{
			auto coorData = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateData(coorname);
			if (coorData)
				coorid = coorData->getCoordinateId();
		}
		auto& mapRelationManager = getCVM->getModelData()->getMapRelationManager();
		mapRelationManager.appendMapRelation(
			{ PhysicalFieldType::Physical_Deputy, deputyData->getDeputyId() },
			{ Physical_Coordinate, coorid });
	}

	void mDeputyConnectorPanel::slot_typeChanged(int i)
	{
		pickingRow_ = -1;
		clearPick();
		_pickeddata.clear();
		updateSubTypeVisible();
		emit sg_editFinished();
	}

	void mDeputyConnectorPanel::slot_subTypeChanged(int i)
	{
		pickingRow_ = -1;
		clearPick();
		_pickeddata.clear();
		emit sg_editFinished();
	}

	void mDeputyConnectorPanel::slot_coorChanged(int i)
	{
		pickingRow_ = -1;
		clearPick();
		_pickeddata.clear();
		if (_deputyData)
			storeDeputyValue(_deputyData);
		emit sg_editFinished();
	}

	void mDeputyConnectorPanel::slot_createCoor()
	{
		pickingRow_ = -1;
		clearPick();
		_pickeddata.clear();
		mInfoWindow::getInstance()->unEditInfoWindow();
		mInfoWindow::getInstance()->createCoordinate();

		combocoor_.combobox->blockSignals(true);
		combocoor_.combobox->clear();
		auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
		for (auto coorname : allCoor)
		{
			combocoor_.combobox->addItem(coorname);
		}
		combocoor_.combobox->setCurrentIndex(allCoor.size() - 1);
		combocoor_.combobox->blockSignals(false);
		storeDeputyValue(_deputyData);
	}

	void mDeputyConnectorPanel::slot_addRow()
	{
		int row = table_->rowCount();
		// 先同步内部缓存，避免 setItem 触发 cellChanged 时越界
		refNodeIds_.insert(row, 0);
		movNodeIds_.insert(row, 0);

		table_->insertRow(row);
		QTableWidgetItem* itemIdx = new QTableWidgetItem(MxFunctions::mNumberToString(row + 1));
		itemIdx->setFlags(itemIdx->flags() & ~Qt::ItemIsEditable);
		itemIdx->setTextAlignment(Qt::AlignCenter);
		table_->setItem(row, 0, itemIdx);

		QTableWidgetItem* itemRef = new QTableWidgetItem("");
		table_->setItem(row, 1, itemRef);

		QTableWidgetItem* itemMov = new QTableWidgetItem("");
		table_->setItem(row, 2, itemMov);

		refreshTableIndex();
		pickingRow_ = -1;
		pickingCol_ = -1;
		clearPreview();
		clearPick();
		_pickeddata.clear();
	}

	void mDeputyConnectorPanel::slot_delRow()
	{
		int row = table_->currentRow();
		if (row < 0 || row >= table_->rowCount())
			return;
		table_->removeRow(row);
		refNodeIds_.removeAt(row);
		movNodeIds_.removeAt(row);
		refreshTableIndex();
		pickingRow_ = -1;
		pickingCol_ = -1;
		clearPreview();
		clearPick();
		_pickeddata.clear();
		emit sg_editFinished();
	}

	void mDeputyConnectorPanel::slot_confirmRow()
	{
		bool anyRowProcessed = false;

		for (int row = 0; row < table_->rowCount(); ++row)
		{
			// 从表格读取参考位置（支持拾取或手动输入）
			QString refText = table_->item(row, 1) ? table_->item(row, 1)->text().trimmed() : "";
			if (refText.isEmpty())
				continue;

			int refNodeId = refText.toInt();
			if (!validateNodeId(refNodeId))
			{
				table_->item(row, 1)->setText("");
				if (row < refNodeIds_.size())
					refNodeIds_[row] = 0;
				continue;
			}

			if (row < refNodeIds_.size())
				refNodeIds_[row] = refNodeId;

			// 获取参考位置坐标
			auto meshData = getCVM->getModelData()->getMeshData();
			QVector3D refPos;
			double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[refNodeId] * 3;
			refPos = QVector3D(vertex[0], vertex[1], vertex[2]);

			// 处理移动位置
			QString movText = table_->item(row, 2) ? table_->item(row, 2)->text().trimmed() : "";
			if (movText.isEmpty())
			{
				// 移动位置空缺，生成临时自由节点（与参考位置同坐标）
				int movNodeId = getCVM->createFreeNode(refPos);
				table_->item(row, 2)->setText(MxFunctions::mNumberToString(movNodeId));
				if (row < movNodeIds_.size())
					movNodeIds_[row] = movNodeId;
			}
			else
			{
				// 移动位置不空缺，验证合法性
				int movNodeId = movText.toInt();
				if (!validateNodeId(movNodeId))
				{
					table_->item(row, 2)->setText("");
					if (row < movNodeIds_.size())
						movNodeIds_[row] = 0;
				}
				else
				{
					if (row < movNodeIds_.size())
						movNodeIds_[row] = movNodeId;
				}
			}

			anyRowProcessed = true;
		}

		// 先重置状态再 clearPick
		pickingRow_ = -1;
		pickingCol_ = -1;
		_pickeddata.clear();
		clearPick();
		clearPreview();

		// 将确认后的数据同步到数据模型，并通过 AllRenderSymbolManager 统一渲染
		if (_deputyData)
		{
			storeDeputyValue(_deputyData);
			AllRenderSymbolManager::getInstance()->rendDeputy(_deputyData, true);
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
		}

		if (anyRowProcessed)
			emit sg_editFinished();
	}

	void mDeputyConnectorPanel::slot_cellClicked(int row, int col)
	{
		// 点击表格任意位置都清空高亮与拾取状态
		clearPick();
		_pickeddata.clear();
		// 点击不同单元格时，显示当前行的两个不同色预览点
		showRowPreview(row);
		if (col == 1 || col == 2) // 参考位置列或移动位置列进入拾取模式
		{
			pickingRow_ = row;
			pickingCol_ = col;
		}
		else
		{
			pickingRow_ = -1;
			pickingCol_ = -1;
		}
	}

	void mDeputyConnectorPanel::slot_pickFinished()
	{
		// 如果当前没有拾取行/列，则返回
		if (pickingRow_ < 0 || pickingRow_ >= table_->rowCount())
			return;
		if (_pickeddata.empty())
			return;

		// 只支持节点拾取
		if (_pickeddata[0].first != PickDataType::Node)
		{
			_pickeddata.clear();
			clearPick();
			return;
		}

		// 复制拾取数据，随后立即重置状态
		auto pickedDataCopy = _pickeddata;
		int targetRow = pickingRow_;
		int targetCol = pickingCol_;
		pickingRow_ = -1;
		pickingCol_ = -1;
		_pickeddata.clear();
		clearPick();

		int nodeId = pickedDataCopy[0].second.result_id;
		if (targetCol == 1) // 参考位置列
		{
			if (targetRow < refNodeIds_.size())
				refNodeIds_[targetRow] = nodeId;
			table_->item(targetRow, 1)->setText(MxFunctions::mNumberToString(nodeId));
		}
		else if (targetCol == 2) // 移动位置列
		{
			if (targetRow < movNodeIds_.size())
				movNodeIds_[targetRow] = nodeId;
			table_->item(targetRow, 2)->setText(MxFunctions::mNumberToString(nodeId));
		}

		// 拾取完成后显示当前行的临时预览点，不触发正式渲染
		showRowPreview(targetRow);
	}

	void mDeputyConnectorPanel::slot_movCellChanged(int row, int col)
	{
		// 如果 _pickeddata 不为空，说明这是由 slot_pickFinished 中的 setText
		// 触发的 cellChanged。此时只同步数据，不清理视图高亮和拾取状态。
		if (!_pickeddata.empty())
		{
			if (col == 1 && row >= 0 && row < refNodeIds_.size())
			{
				QString text = table_->item(row, col) ? table_->item(row, col)->text().trimmed() : "";
				if (!text.isEmpty())
					refNodeIds_[row] = text.toInt();
				else
					refNodeIds_[row] = 0;
			}
			else if (col == 2 && row >= 0 && row < movNodeIds_.size())
			{
				QString text = table_->item(row, col) ? table_->item(row, col)->text().trimmed() : "";
				if (!text.isEmpty())
					movNodeIds_[row] = text.toInt();
				else
					movNodeIds_[row] = 0;
			}
			return;
		}

		// 手动编辑单元格时同样清空视图高亮与拾取状态
		pickingRow_ = -1;
		pickingCol_ = -1;
		clearPick();
		_pickeddata.clear();

		if (col == 1) // 参考位置列
		{
			// 防止新增行时缓存尚未同步导致越界
			if (row < 0 || row >= refNodeIds_.size())
				return;

			QString text = table_->item(row, col) ? table_->item(row, col)->text().trimmed() : "";
			if (!text.isEmpty())
			{
				// 点击确定后再进行合法性检查，此处只做同步
				refNodeIds_[row] = text.toInt();
			}
			else
			{
				refNodeIds_[row] = 0;
			}
		}
		else if (col == 2) // 移动位置列
		{
			// 防止新增行时缓存尚未同步导致越界
			if (row < 0 || row >= movNodeIds_.size())
				return;

			QString text = table_->item(row, col) ? table_->item(row, col)->text().trimmed() : "";
			if (!text.isEmpty())
			{
				// 点击确定后再进行合法性检查并填充，此处只做同步
				movNodeIds_[row] = text.toInt();
			}
			else
			{
				movNodeIds_[row] = 0;
			}
		}

	}

	void mDeputyConnectorPanel::updateSubTypeVisible()
	{
		QString currentType = typeCombo_.combobox->currentText();
		bool showSub = (currentType == "球铰" || currentType == "滑动");

		if (!faval_)
			return;

		faval_->removeAll();
		faval_->addRow(typeCombo_.propwid);
		typeCombo_.propwid->show();

		if (showSub)
		{
			faval_->addRow(subTypeCombo_.propwid);
			subTypeCombo_.propwid->show();
		}

		faval_->addRow(combocoor_.propwid);
		combocoor_.propwid->show();

		faval_->refreshLayout();
	}

	bool mDeputyConnectorPanel::validateNodeId(int nodeId)
	{
		if (nodeId <= 0)
			return false;
		auto meshData = getCVM->getModelData()->getMeshData();
		if (!meshData)
			return false;
		return meshData->NodeIsHaveByID(nodeId);
	}

	int mDeputyConnectorPanel::createTempFreeNodeForRow(int row)
	{
		if (row < 0 || row >= refNodeIds_.size() || refNodeIds_[row] <= 0)
			return -1;
		auto meshData = getCVM->getModelData()->getMeshData();
		if (!meshData)
			return -1;
		int refId = refNodeIds_[row];
		double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[refId] * 3;
		QVector3D pos(vertex[0], vertex[1], vertex[2]);
		return getCVM->createFreeNode(pos);
	}

	void mDeputyConnectorPanel::refreshTableIndex()
	{
		for (int i = 0; i < table_->rowCount(); ++i)
		{
			QTableWidgetItem* item = table_->item(i, 0);
			if (item)
				item->setText(MxFunctions::mNumberToString(i + 1));
		}
	}

	void mDeputyConnectorPanel::showRowPreview(int row)
	{
		clearPreview();
		if (row < 0 || row >= table_->rowCount())
			return;

		auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
		if (!render)
			return;

		QVector3D refPos, movPos;
		bool hasRef = false, hasMov = false;

		QString refText = table_->item(row, 1) ? table_->item(row, 1)->text().trimmed() : "";
		if (!refText.isEmpty())
		{
			int refId = refText.toInt();
			if (getNodePosition(refId, refPos))
				hasRef = true;
		}

		QString movText = table_->item(row, 2) ? table_->item(row, 2)->text().trimmed() : "";
		if (!movText.isEmpty())
		{
			int movId = movText.toInt();
			if (getNodePosition(movId, movPos))
				hasMov = true;
		}

		if (hasRef)
			render->appendPoint("deputy_ref_preview", { refPos }, QVector3D(54, 79, 107) / 255.0);
		if (hasMov)
			render->appendPoint("deputy_mov_preview", { movPos }, QVector3D(252, 81, 133) / 255.0);
		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mDeputyConnectorPanel::clearPreview()
	{
		auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
		if (render)
		{
			render->deletePoint("deputy_ref_preview");
			render->deletePoint("deputy_mov_preview");
		}
		getCVM->getPreRenderWidget()->updatePaint();
	}


	bool mDeputyConnectorPanel::getNodePosition(int nodeId, QVector3D& pos)
	{
		if (nodeId <= 0)
			return false;
		auto meshData = getCVM->getModelData()->getMeshData();
		if (!meshData)
			return false;
		if (!meshData->NodeIsHaveByID(nodeId))
			return false;
		double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[nodeId] * 3;
		pos = QVector3D(vertex[0], vertex[1], vertex[2]);
		return true;
	}

	void mDeputyConnectorPanel::rendFreeNodes(bool showhide)
	{
		auto render = getCVM->getPreRenderWidget()->getCommonShapeRender();
		if (!render)
			return;

		render->deletePoint("deputy_ref_preview");
		render->deletePoint("deputy_mov_preview");

		if (!showhide)
		{
			getCVM->getPreRenderWidget()->updatePaint();
			return;
		}

		for (int row = 0; row < table_->rowCount(); ++row)
		{
			QString refText = table_->item(row, 1) ? table_->item(row, 1)->text().trimmed() : "";
			QString movText = table_->item(row, 2) ? table_->item(row, 2)->text().trimmed() : "";
			QVector3D refPos, movPos;
			if (!refText.isEmpty() && getNodePosition(refText.toInt(), refPos))
				render->appendPoint("deputy_ref_preview", { refPos }, QVector3D(54, 79, 107) / 255.0);
			if (!movText.isEmpty() && getNodePosition(movText.toInt(), movPos))
				render->appendPoint("deputy_mov_preview", { movPos }, QVector3D(252, 81, 133) / 255.0);
		}
		getCVM->getPreRenderWidget()->updatePaint();
	}

/*************************************************焊接点（Fastener）***************************************************************/
	mDeputyFastenerPanel::mDeputyFastenerPanel(QWidget* parent)
		: mDeputyPanelBase(parent)
	{
		initInputs();
	}

	mDeputyFastenerPanel::~mDeputyFastenerPanel()
	{
		clearInputs();
	}

	void mDeputyFastenerPanel::initInputs()
	{
		// 焊接点定义
		defineTypeCombo_.initWidget("类型");
		defineTypeCombo_.combobox->addItems({ "节点定义", "连接器部件定义" });
		connect(defineTypeCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mDeputyFastenerPanel::slot_defineTypeChanged);

		nodePick_.initWidget("节点");
		nodePick_.btnadd->setVisible(false);
		connect(nodePick_.btnok, &QPushButton::clicked, this, &mDeputyFastenerPanel::slot_pickNode);
		connect(nodePick_.btnclear, &QPushButton::clicked, this, &mDeputyFastenerPanel::slot_clearNodePick);
		connect(nodePick_.labelpicked, &QCheckBox::stateChanged, [this](int state) {
			nodePick_.setCheckedState(state == Qt::Checked);
			highlightPicked(nodePick_.nodes_, state == Qt::Checked);
		});

		deputyCombo_.initWidget("连接器部件");
		connect(deputyCombo_.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
			this, &mDeputyFastenerPanel::slot_deputyChanged);

		partListWidget_ = new QListWidget();
		partListWidget_->setSelectionMode(QAbstractItemView::ExtendedSelection);
		partListWidget_->setMinimumHeight(200);
		partListWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		addPartBtn_ = new QToolButton();
		addPartBtn_->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
		addPartBtn_->setToolTip("增加");
		removePartBtn_ = new QToolButton();
		removePartBtn_->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
		removePartBtn_->setToolTip("减少");
		clearPartBtn_ = new QToolButton();
		clearPartBtn_->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
		clearPartBtn_->setToolTip("清空");

		partListContainer_ = new QWidget();
		QGridLayout* grid = new QGridLayout();
		grid->addWidget(partListWidget_, 0, 0, 8, 7);
		grid->addWidget(addPartBtn_, 0, 7);
		grid->addWidget(removePartBtn_, 1, 7);
		grid->addWidget(clearPartBtn_, 2, 7);
		partListContainer_->setLayout(grid);

		connect(addPartBtn_, &QPushButton::clicked, this, &mDeputyFastenerPanel::slot_addPart);
		connect(removePartBtn_, &QPushButton::clicked, this, &mDeputyFastenerPanel::slot_removePart);
		connect(clearPartBtn_, &QPushButton::clicked, this, &mDeputyFastenerPanel::slot_clearParts);

		// 属性
		radiusInput_.initWidget("影响半径", false);
		connect(radiusInput_.lineeditvalue, &mCalcLineEdit::editingFinished,
			this, &mDeputyFastenerPanel::slot_radiusChanged);
	}

	void mDeputyFastenerPanel::clearInputs()
	{
		defineTypeCombo_.combobox->setCurrentIndex(0);
		nodePick_.nodes_.clear();
		nodePick_.nodes_id.clear();
		nodePick_.setState(false);
		deputyCombo_.combobox->clear();
		partListWidget_->clear();
		radiusInput_.lineeditvalue->clear();
	}

	QVector<QWidget*> mDeputyFastenerPanel::getFastenerDefine(FoldSection* value)
	{
		fastenerDefine_ = value;
		QVector<QWidget*> wds;
		wds.append(defineTypeCombo_.propwid);
		int idx = defineTypeCombo_.combobox->currentIndex();
		if (idx == 0)
		{
			wds.append(nodePick_.propwid);
		}
		else
		{
			wds.append(deputyCombo_.propwid);
		}
		return wds;
	}

	QVector<QWidget*> mDeputyFastenerPanel::getConnectParts(FoldSection* value)
	{
		connectParts_ = value;
		QVector<QWidget*> wds;
		wds.append(partListContainer_);
		return wds;
	}

	QVector<QWidget*> mDeputyFastenerPanel::getProperties(FoldSection* value)
	{
		properties_ = value;
		QVector<QWidget*> wds;
		wds.append(radiusInput_.propwid);
		return wds;
	}

	void mDeputyFastenerPanel::setInputsValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)
	{
		std::shared_ptr<MechConnectorData::mConnectorFastener> deputyPtr =
			std::dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(deputyData);
		if (!deputyPtr) return;

		// 1. 焊接点定义
		defineTypeCombo_.combobox->setCurrentIndex(deputyPtr->defineType_);

		deputyCombo_.combobox->clear();
		auto& depMgr = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
		QStringList deputyNames = depMgr.getAllDeputyNamesByTypes(MxFunctions::Deputy_Connector);
		for (const QString& name : deputyNames)
		{
			auto deputyData = depMgr.getDeputyData(name);
			if (deputyData && deputyData->isSuppressed())
				continue;
			deputyCombo_.combobox->addItem(name);
		}

		// 从普通映射恢复连接器部件
		int deputyId = deputyData->getDeputyId();
		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		int connDeputyId = mapRelation.getMapRelation<int>(
			{ Physical_Deputy, deputyId }, Physical_Deputy, MapRelation_None
		);
		if (connDeputyId > 0)
		{
			QString dname = depMgr.getDeputyNameByID(connDeputyId);
			if (!dname.isEmpty()) deputyCombo_.combobox->setCurrentText(dname);
		}

		nodePick_.nodes_.clear();
		nodePick_.nodes_id.clear();
		if (deputyPtr->defineType_ == 0 && deputyPtr->nodeId_ > 0)
		{
			RenderPickResultData data;
			data.result_id = deputyPtr->nodeId_;
			nodePick_.nodes_.push_back({ PickDataType::Node, data });
			nodePick_.nodes_id.push_back({ PickDataType::Node, deputyPtr->nodeId_ });
			nodePick_.setState(true);
            if (nodePick_.labelpicked->isChecked())
            {
                highlightPicked(nodePick_.nodes_, true);
            }
		}
		else
		{
			nodePick_.setState(false);
		}

		// 2. 连接部件 - 从表格映射恢复
		partListWidget_->clear();
		QList<int> partIds = mapRelation.getMapRelation<QList<int>>(
			{ Physical_Deputy, deputyId }, Physical_Part
		);
		for (int partId : partIds)
		{
			if (partId > 0)
			{
				QString partName = getCVM->getModelData()->getNameByPhyID(Physical_Part, partId);
				if (!partName.isEmpty())
				{
					partListWidget_->addItem(partName);
				}
			}
		}

		// 3. 属性
		radiusInput_.lineeditvalue->setExpression(MxFunctions::mNumberToString(deputyPtr->influenceRadius_));
	}

	void mDeputyFastenerPanel::storeDeputyValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)
	{
		std::shared_ptr<MechConnectorData::mConnectorFastener> deputyPtr =
			std::dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(deputyData);
		if (!deputyPtr) return;

		deputyPtr->defineType_ = defineTypeCombo_.combobox->currentIndex();
		if (deputyPtr->defineType_ == 0)
		{
			if (!nodePick_.nodes_id.empty())
			{
				deputyPtr->nodeId_ = nodePick_.nodes_id[0].second;
			}
			else
			{
				deputyPtr->nodeId_ = -1;
				deputyPtr->tempNodeId_ = -1;
			}
		}
		else
		{
			deputyPtr->nodeId_ = -1;
			deputyPtr->tempNodeId_ = -1;
		}

		QString radiusText = radiusInput_.lineeditvalue->text().trimmed();
		if (!radiusText.isEmpty())
		{
			deputyPtr->influenceRadius_ = radiusText.toDouble();
		}
		else
		{
			deputyPtr->influenceRadius_ = 0.0;
		}

		// 保存映射关系
		int deputyId = deputyData->getDeputyId();
		auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
		auto& depMgr = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);

		// 1. 保存连接器部件普通映射
		mapRelation.deleteMapRelation({ Physical_Deputy, deputyId }, { Physical_Deputy, -1 });
		if (deputyPtr->defineType_ == 1)
		{
			QString deputyName = deputyCombo_.combobox->currentText();
			if (!deputyName.isEmpty())
			{
				int connDeputyId = depMgr.getIDByDeputyName(deputyName);
				if (connDeputyId > 0)
				{
					mapRelation.appendMapRelation(
						{ Physical_Deputy, deputyId },
						{ Physical_Deputy, connDeputyId },
						MapRelation_None
					);
				}
			}
		}

		// 2. 保存部件表格映射：先删除旧映射，再重建
		QList<int> oldPartIds = mapRelation.getMapRelation<QList<int>>(
			{ Physical_Deputy, deputyId }, Physical_Part
		);
		for (int i = oldPartIds.size() - 1; i >= 0; --i)
		{
			mapRelation.deleteTabelMapRelation(
				{ Physical_Deputy, deputyId },
				{ Physical_Part, -1 },
				i
			);
		}
		for (int row = 0; row < partListWidget_->count(); ++row)
		{
			QListWidgetItem* item = partListWidget_->item(row);
			if (!item) continue;
			QString partName = item->text();
			int partId = getCVM->getModelData()->getPhyIDByName(Physical_Part, partName);
			if (partId > 0)
			{
				mapRelation.appendTableMapRelation(
					{ Physical_Deputy, deputyId },
					{ Physical_Part, partId },
					row
				);
			}
		}
	}

	void mDeputyFastenerPanel::slot_defineTypeChanged(int idx)
	{
		if (!fastenerDefine_) return;
		fastenerDefine_->removeAll();
		fastenerDefine_->addRow(defineTypeCombo_.propwid);
		defineTypeCombo_.propwid->show();
		if (idx == 0)
		{
			fastenerDefine_->addRow(nodePick_.propwid);
			nodePick_.propwid->show();
			nodePick_.setState(nodePick_.nodes_.size() != 0);
		}
		else
		{
			fastenerDefine_->addRow(deputyCombo_.propwid);
			deputyCombo_.propwid->show();
			// 刷新连接副列表
			deputyCombo_.combobox->clear();
			auto& depMgr = getCVM->getModelData()->getManager<MechConnectorData::mDeputyManager>(Physical_Deputy);
			QStringList deputyNames = depMgr.getAllDeputyNamesByTypes(MxFunctions::Deputy_Connector);
			for (const QString& name : deputyNames)
			{
				auto deputyData = depMgr.getDeputyData(name);
				if (deputyData && deputyData->isSuppressed())
					continue;
				deputyCombo_.combobox->addItem(name);
			}
		}
		fastenerDefine_->refreshLayout();
		emit sg_editFinished();
	}

	bool mDeputyFastenerPanel::getNodePosition(int nodeId, QVector3D& pos)
	{
		if (nodeId <= 0)
			return false;
		auto meshData = getCVM->getModelData()->getMeshData();
		if (!meshData)
			return false;
		if (!meshData->NodeIsHaveByID(nodeId))
			return false;
		double* vertex = meshData->_nodedata.xyz + meshData->_nodedata.nodeidmap[nodeId] * 3;
		pos = QVector3D(vertex[0], vertex[1], vertex[2]);
		return true;
	}

	void mDeputyFastenerPanel::slot_pickNode()
	{
		if (_pickeddata.empty())
			return;
		if (_pickeddata[0].first != PickDataType::Node)
		{
			_pickeddata.clear();
			return;
		}
		nodePick_.nodes_.clear();
		nodePick_.nodes_id.clear();
		nodePick_.nodes_.emplace_back(_pickeddata[0]);
		nodePick_.nodes_id.push_back({ _pickeddata[0].first, _pickeddata[0].second.result_id });
		nodePick_.setState(true);

		// 创建与选中节点同坐标的临时自由节点
		std::shared_ptr<MechConnectorData::mConnectorFastener> deputyPtr =
			std::dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(_deputyData);
		if (deputyPtr)
		{
			int pickedNodeId = _pickeddata[0].second.result_id;
			QVector3D nodePos;
			if (getNodePosition(pickedNodeId, nodePos))
			{
				deputyPtr->tempNodeId_ = getCVM->createFreeNode(nodePos);
			}
		}

		_pickeddata.clear();
		clearPick();
		emit sg_editFinished();
	}

	void mDeputyFastenerPanel::slot_clearNodePick()
	{
		nodePick_.nodes_.clear();
		nodePick_.nodes_id.clear();
		nodePick_.setState(false);

		// 清空临时节点ID
		std::shared_ptr<MechConnectorData::mConnectorFastener> deputyPtr =
			std::dynamic_pointer_cast<MechConnectorData::mConnectorFastener>(_deputyData);
		if (deputyPtr)
		{
			deputyPtr->tempNodeId_ = -1;
		}

		_pickeddata.clear();
		clearPick();
		emit sg_editFinished();
	}

	void mDeputyFastenerPanel::slot_deputyChanged(int idx)
	{
		emit sg_editFinished();
	}

	void mDeputyFastenerPanel::slot_addPart()
	{
		QStringList candidateList = getCVM->getModelData()->getAllNames(Physical_Part);
		QStringListModel* tempModel = new QStringListModel(this);
		tempModel->setStringList(candidateList);

		QListWidgetItem* newItem = new QListWidgetItem();
		partListWidget_->addItem(newItem);

		QLineEdit* editor = new QLineEdit(partListWidget_);
		partListWidget_->setItemWidget(newItem, editor);
		editor->setFocus();

		QCompleter* completer = new QCompleter(tempModel, editor);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setFilterMode(Qt::MatchContains);
		completer->setCompletionMode(QCompleter::PopupCompletion);
		editor->setCompleter(completer);
		completer->complete();

		QPointer<QLineEdit> editorGuard(editor);
		auto finishFlag = std::make_shared<bool>(false);

		std::function<void()> finishEditing = [=]() mutable {
			if (*finishFlag) return;
			*finishFlag = true;
			auto cleanUp = [&]() {
				if (editorGuard) editorGuard->deleteLater();
				tempModel->deleteLater();
			};
			if (!editorGuard) {
				int r = partListWidget_->row(newItem);
				if (r >= 0) delete partListWidget_->takeItem(r);
				cleanUp();
				return;
			}
			int row = partListWidget_->row(newItem);
			if (row < 0) {
				cleanUp();
				return;
			}
			partListWidget_->removeItemWidget(newItem);
			QString text = editorGuard->text().trimmed();

			if (text.isEmpty()) {
				delete partListWidget_->takeItem(row);
				cleanUp();
				return;
			}
			if (!tempModel->stringList().contains(text)) {
				delete partListWidget_->takeItem(row);
				cleanUp();
				return;
			}
			bool exists = false;
			for (int i = 0; i < partListWidget_->count(); ++i) {
				QListWidgetItem* it = partListWidget_->item(i);
				if (it == newItem) continue;
				if (it && it->text() == text) {
					exists = true;
					break;
				}
			}
			if (exists) {
				delete partListWidget_->takeItem(row);
				cleanUp();
				return;
			}

			newItem->setText(text);

			// 添加表格映射
			if (_deputyData)
			{
				int deputyId = _deputyData->getDeputyId();
				int partId = getCVM->getModelData()->getPhyIDByName(Physical_Part, text);
				if (partId > 0)
				{
					getCVM->getModelData()->getMapRelationManager().appendTableMapRelation(
						{ Physical_Deputy, deputyId },
						{ Physical_Part, partId },
						row
					);
				}
			}

			cleanUp();
			emit sg_editFinished();
		};

		connect(completer, QOverload<const QString&>::of(&QCompleter::activated),
			editor, [=](const QString& choice) {
				if (editorGuard) {
					editorGuard->setText(choice);
					finishEditing();
				}
			}, Qt::UniqueConnection);
		connect(editor, &QLineEdit::editingFinished, editor, finishEditing, Qt::UniqueConnection);
	}

	void mDeputyFastenerPanel::slot_removePart()
	{
		auto selectedItems = partListWidget_->selectedItems();
		for (QListWidgetItem* item : selectedItems) {
			int row = partListWidget_->row(item);
			if (row >= 0) {
				// 删除表格映射
				if (_deputyData)
				{
					int deputyId = _deputyData->getDeputyId();
					getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation(
						{ Physical_Deputy, deputyId },
						{ Physical_Part, -1 },
						row
					);
				}
				delete partListWidget_->takeItem(row);
			}
		}
		emit sg_editFinished();
	}

	void mDeputyFastenerPanel::slot_clearParts()
	{
		// 删除所有表格映射
		if (_deputyData)
		{
			int deputyId = _deputyData->getDeputyId();
			auto& mapRelation = getCVM->getModelData()->getMapRelationManager();
			int totalRow = partListWidget_->count();
			for (int row = totalRow - 1; row >= 0; --row)
			{
				mapRelation.deleteTabelMapRelation(
					{ Physical_Deputy, deputyId },
					{ Physical_Part, -1 },
					row
				);
			}
		}
		partListWidget_->clear();
		emit sg_editFinished();
	}

	void mDeputyFastenerPanel::slot_partSelectionChanged()
	{
		// 预留
		int a = 0;
	}

	void mDeputyFastenerPanel::slot_radiusChanged()
	{
		emit sg_editFinished();
	}

} // namespace MechMainWindow
