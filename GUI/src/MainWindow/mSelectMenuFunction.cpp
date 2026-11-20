#include "mSelectMenuFunction.h"
namespace MGeoData { class GeoDataModelSet; } //【OCC摘除·S10/P0-1 终局3】全局前置声明（S9 下线壳仅存空指针用法）
//【OCC摘除·S9】旧栈 set 反查改 GN
#include "mGeoNeutralQuery.h"
#include "mGlobalSignals.h"
#include "mMechGlobalSignals.h"
#include "mTitleBar.h"
#include "mCustomTooltip.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QMenu>
#include <QApplication>
#include <QClipboard>
#include <qmath.h>

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"


//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
//【镜像六件套摘除】mGeo*Data1.h 已随 RebuildRender 删除（本文件仅剩死 include）


//Geo
#include "mGeoManager.h"
//#include <mGeoInfoCalculate.h> //【OCC摘除·S10/P0-1 终局2】死 include（零使用）
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1 终局3】死 include（零使用；60+ 冻结岛头深链入口）
//【OCC摘除·S10/P0-1 终局3】清扫乱行重写：//#include "mGeoCommandSolidOfConcaveOCC.h" //#include "mGeoCommandSolidOfConvexOCC.h" //#include "mGeoCommandFactory.h"
//#include "mGeoCommandFactory.h" //【OCC摘除·S10/P0-1 终局3】重复入口（60+ 冻结岛头深链）


//BasicData
#include "mModelData.h"
#include "mMeshData.h"

#define ep 1e-3
using namespace mxr;
using namespace MxFunctions;
using namespace MechPreData;
//using namespace MGeoCommand; //【OCC摘除·S10/P0-1 终局2】随死 include 下线

namespace std {
    template<> struct hash<std::pair<int, int>> {
        size_t operator() (std::pair<int, int> const& v) const {
            return (hash<int>()(v.first)) ^ (hash<int>()(v.second) << 1);
        }
    };
}

namespace MechMainWindow
{
	mSelectMenuFunction* mSelectMenuFunction::_instance = nullptr;
	mSelectMenuFunction* mSelectMenuFunction::getInstance()
	{
		if (_instance == nullptr)
		{
			_instance = new mSelectMenuFunction;
			//_instance->setTitleBarWidget(new TitleBarWidget(_instance, "选择信息"));
			_instance->setObjectName("SelectInfoDock");
		}
		return _instance;
	}

	std::vector<int> mSelectMenuFunction::subtractSetVector(const std::set<int>& s, std::vector<int> v)
	{
		std::sort(v.begin(), v.end()); // 确保有序
		std::vector<int> result;

		std::set_difference(s.begin(), s.end(),
			v.begin(), v.end(),
			std::back_inserter(result));
		return result;

	}

	mSelectMenuFunction::mSelectMenuFunction(QWidget* parent) :QObject(parent)
	{
		_nodeLineEdit = new mInputIDLineEdit();
		_eleLineEdit = new mInputIDLineEdit();
		//切换界面语言
		connect(MechPreData::mViewportModelManager::getInstance(), &MechPreData::mViewportModelManager::currentViewportChanged, this, &mSelectMenuFunction::slot_subWindowActivated);

	}

	void mSelectMenuFunction::clearHighlight()
	{
		if (!getCVM)
		{
			return;
		}
		ClearPreWidgetGeoPick(getCVM->getPreRenderWidget());
		ClearPreWidgetMeshPick(getCVM->getPreRenderWidget());
		getCVM->updateRender();
	}

	void mSelectMenuFunction::slot_adjacentFaceLine(bool isRecursive)
	{
		if (!getCVM)
		{
			return;
		}
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		//if (pickNum != 1)
		//{
		//	return;
		//}
		if (pickDataType == PickDataType::GeoFace)
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			std::set<int> seen;
			for (auto id : res)
			{
				auto faceIds = getGEOServer.geoCalculate().getAdjacentFaceByFaceId(id, 150, 0, isRecursive);
				for (auto id : faceIds)
				{
					seen.insert(id);
				}
			}
			res = vector<int>(seen.begin(), seen.end());
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
		}
		else if (pickDataType == PickDataType::GeoEdge)
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			std::set<int> seen;
			for (auto id : res)
			{
				auto edgeIds = getGEOServer.geoCalculate().getAdjacentEdgeByEdgeId(id, 150, 0, isRecursive);
				for (auto id : edgeIds)
				{
					seen.insert(id);
				}
			}
			res = vector<int>(seen.begin(), seen.end());
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_extendConnectFace()
	{
		if (!getCVM)
		{
			return;
		}
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();

		if (pickDataType != PickDataType::GeoFace)
		{
			return;
		}

		auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
		auto faceIds = getGEOServer.geoCalculate().getAllFaceFaceID(res);
		res = std::vector<int>{ faceIds.begin(), faceIds.end() };
		clearHighlight();
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);


		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();

	}

	void mSelectMenuFunction::slot_cylinderFaceSelect()
	{
		if (!getCVM)
		{
			return;
		}
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);

		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto cylinderIds = getGEOServer.geoCalculate().getAllCylinderFaceID();

		std::vector<int> res{ cylinderIds.begin(), cylinderIds.end() };

		clearHighlight();
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_faceCommonEdge()
	{
		if (!getCVM)
		{
			return;
		}
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();

		if (pickDataType != PickDataType::GeoFace)
		{
			return;
		}

		auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
		auto edgetIds = getGEOServer.geoCalculate().getFaceFaceCommonEdgeIds(res);
		res = std::vector<int>{ edgetIds.begin(), edgetIds.end() };
		clearHighlight();
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_sunkenRaisedFaceSelect(bool sunken)
{
    //【OCC摘除·S9→引擎化恢复】旧 mGeoCommandSolidOfConcave/Convex 命令链退役后，
    // 迁移至引擎 geoCalculate().getSunkenRaisedFaceIds：从选中面沿凹边（凹陷）或
    // 凸边（凸起）及光顺过渡边广度优先收集连通面（OCC/CGM 双后端同语义）。
    if (!getCVM || getCVM->getPreRenderWidget() == nullptr)
    {
        return;
    }
    int pickNum = 0;
    if (getCVM->getCurrentPickDataType(pickNum) != PickDataType::GeoFace)
    {
        return;
    }
    std::vector<int> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
    if (res.empty())
    {
        return;
    }
    MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
    auto faceIds = getGEOServer.geoCalculate().getSunkenRaisedFaceIds(res, sunken);
    res = std::vector<int>{ faceIds.begin(), faceIds.end() };
    clearHighlight();
    getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);

    MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

    getCVM->getPreRenderWidget()->updatePaint();
}

	void mSelectMenuFunction::slot_nodeIDSelect()
	{
		if (!getCVM || getCVM->getPreRenderWidget() == nullptr)
			return;
        auto meshData = getCVM->getModelData()->getMeshData();
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
		auto nodeIds = _nodeLineEdit->getIds();
		std::vector<MeshSelectInfoData> nodes;
		for (auto node : nodeIds)
		{
            if (!meshData->NodeIsHaveByID(node))
            {
                continue;
            }
			MeshSelectInfoData n;
			n.data_id.element_id = node;
			//n.part_id = -1;
			nodes.push_back(n);
		}
		clearHighlight();
		getCVM->getPreRenderWidget()->getSelectionContainer()
            ->getMeshSelectData()
            ->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());



		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_eleIDSelect()
	{
		if (!getCVM || getCVM->getPreRenderWidget() == nullptr)
			return;
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
		auto meshData = getCVM->getModelData()->getMeshData();
		auto eleIds = _eleLineEdit->getIds();
		std::vector<MeshSelectInfoData> eles;
		for (auto ele : eleIds)
		{
			MeshSelectInfoData n;
			n.data_id.element_id = ele;
			auto partId = meshData->getPartIdByEleId(ele);
			if (partId < 0)
			{
				continue;
			}
			n.part_id = meshData->getPartIdByEleId(ele);
			eles.push_back(n);
		}
		clearHighlight();
		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());


		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_reverse()
	{
		if (!getCVM || getCVM->getPreRenderWidget() == nullptr)
			return;
		auto meshData = getCVM->getModelData()->getMeshData();
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(3);
			res = subtractSetVector(allIds, res);
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, res);
			break;
		}
		case PickDataType::GeoFace:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(2);
			res = subtractSetVector(allIds, res);
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
			break;
		}
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(1);
			res = subtractSetVector(allIds, res);
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
			break;
		}
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(0);
			res = subtractSetVector(allIds, res);
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, res);
			break;
		}
		case PickDataType::Element:
		{
			std::vector<MeshSelectInfoData> eles;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			//按照部件分类
			QHash<int, std::unordered_set<int>> partEles;
			for (auto mesh : res)
			{
				partEles[mesh.part_id].insert(mesh.data_id.element_id);
			}
			//去重
			for (auto partId : meshData->getAllPartIds())
			{
                auto partData = meshData->getPartData(partId);
                if (partData == nullptr || partData->getMeshPartMask())
                {
                    continue;
                }
				auto selectMeshIds = partEles[partId];
				auto meshIds = meshData->getAllMeshIDsInPart(partData->getPartName());

				for (auto meshId : meshIds) {
					if (!selectMeshIds.count(meshId.result_id))
					{
                        MeshSelectInfoData d = MechPreData::PickData2SelectData(PickDataType::Element, meshId);
						eles.push_back(d);
					}

				}
			}

			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);

			break;
		}
		case PickDataType::ElementFace:
		{
			std::vector<MeshSelectInfoData> eles;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			//按照部件分类
			QHash<int, QSet<RenderPickResultData>> partEles;
			for (auto mesh : res)
			{
				RenderPickResultData data = MechPreData::SelectData2ResultData(PickDataType::ElementFace, mesh);
				partEles[mesh.part_id].insert(data);
			}
			//去重
			for (auto partId : meshData->getAllPartIds())
			{
                auto partData = meshData->getPartData(partId);
                if (partData == nullptr || partData->getMeshPartMask())
                {
                    continue;
                }
				QSet<RenderPickResultData> selectMeshIds = partEles[partId];
				auto meshIds = meshData->getAllMeshFaceIDsInPartByRPRD(partData->getPartName());
				for (auto meshId : meshIds) {
					if (selectMeshIds.constFind(meshId) == selectMeshIds.cend())
					{
						MeshSelectInfoData d = MechPreData::PickData2SelectData(PickDataType::ElementFace, meshId);
						eles.push_back(d);
					}
				}
			}

			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, eles);

			break;
		}
        case PickDataType::ElementEdge:
        {
            std::vector<MeshSelectInfoData> eles;
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
            //按照部件分类
            QHash<int, QSet<RenderPickResultData>> partEles;
            for (auto mesh : res)
            {
                RenderPickResultData data = MechPreData::SelectData2ResultData(PickDataType::ElementEdge, mesh);
                partEles[mesh.part_id].insert(data);
            }
            //去重
            for (auto partId : meshData->getAllPartIds())
            {
                auto partData = meshData->getPartData(partId);
                if (partData == nullptr || partData->getMeshPartMask())
                {
                    continue;
                }
                QSet<RenderPickResultData> selectMeshIds = partEles[partId];
                auto meshIds = meshData->getAllMeshLineIDsInPart(partData->getPartName());
                for (auto meshId : meshIds) {
                    if (selectMeshIds.constFind(meshId) == selectMeshIds.cend())
                    {
                        MeshSelectInfoData d = MechPreData::PickData2SelectData(PickDataType::ElementEdge, meshId);
                        eles.push_back(d);
                    }
                }
            }

            clearHighlight();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshLine, eles);

            break;
        }
        case PickDataType::Node:
        {
            std::vector<MeshSelectInfoData> nodes;
            auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);

            std::unordered_set<int> selectNodes;
            for (auto mesh : res)
            {
                selectNodes.insert(mesh.data_id.element_id);
            }
                //去重
            auto nodeIds = meshData->getAllShowNodesID();

            for (int nodeId : nodeIds) {
                if (!selectNodes.count(nodeId))
                {
                    MeshSelectInfoData d;
                    d.data_id.element_id = nodeId;
                    nodes.push_back(d);
                }

            }


            clearHighlight();
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);

            break;
        }
		default:
			break;
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();

	}

	void mSelectMenuFunction::slot_growEles()
	{
		//获取当前单元的共节点的单元，递进一层即可
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		std::vector<MeshSelectInfoData> eles;

		if (pickDataType != PickDataType::Element)
		{
			return;
		}
		QHash<int, QSet<int>> partEles;
		std::vector<MeshSelectInfoData> res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
		for (auto ele : res)
		{
			auto eleNodes = meshData->getBFaceNodeIdsByEleId(ele.part_id, ele.data_id.element_id);
			auto partData = meshData->getPartData(ele.part_id);
            for (auto node : eleNodes)
            {
                auto nodeEles = partData->_parteledata.nodeelements.value(node);
                for (int eleId : nodeEles)
                {
                    partEles[ele.part_id].insert(eleId);
                }
            }
		}

		QHashIterator<int, QSet<int>> iter(partEles);
		while (iter.hasNext())
		{
			iter.next();
			int partId = iter.key();

			for (auto ele : iter.value())
			{
				MeshSelectInfoData d;
				d.data_id.element_id = ele;
				d.part_id = partId;
				eles.push_back(d);
			}
		}

		clearHighlight();
		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

    void mSelectMenuFunction::slot_lineAngleSelect(float angleThresholdDeg)
    {
        if (!getCVM)
        {
            return;
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        int pickNum = 0;
        PickDataType pickType = getCVM->getCurrentPickDataType(pickNum);

        if (pickType != PickDataType::Node || pickNum < 1)
        {
            return;
        }

        // 1. 获取最后拾取的节点（作为种子点），使用真实 ID
        std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
        int seedRealNodeId = nodes.back().data_id.node_id;

        /*
        // 2. 寻找起始边
        RenderPickResultData seedLineID;
        bool foundSeed = false;

        for (auto part : meshData->_partdata)
        {
            for (auto iter : part->_partedgedata.nodesurfaceids)//std::map<std::pair<int, int>, std::vector<BoundaryFace>> nodesurfaceids(//连续节点id和边界面绑定);
            {
                int readid1 = meshData->_nodedata.realids[iter.first.first];
                int readid2 = meshData->_nodedata.realids[iter.first.second];
                if (seedRealNodeId == readid1 || seedRealNodeId == readid2)//该节点处于这个边界上
                {
                    seedLineID.part_id = part->getPartID();
                    seedLineID.result_id = readid1;//线的第一个真实节点(到底存真实还是连续就看你了)
                    seedLineID.result_id2 = readid2;//线的第二个真实节点
                    foundSeed = true;
                    break;
                }
            }

        }
        */
        // 2. 寻找起始边，并同时构建该 Part 的边界邻接表
        bool foundSeed = false;
        int seedContinuous1 = -1;
        int seedContinuous2 = -1;
        mPartData* targetPart = nullptr;
        // 邻接表：记录某个连续节点 ID 相连的所有连续节点 ID
        std::map<int, std::vector<int>> adjEdges;

        for (auto part : meshData->_partdata)
        {
            adjEdges.clear(); // 清空上一个 part 的临时数据

            for (auto iter : part->_partedgedata.nodesurfaceids)
            {
                int c1 = iter.first.first; // 连续ID 1
                int c2 = iter.first.second; // 连续ID 2

                // 构建无向图邻接表
                adjEdges[c1].push_back(c2);
                adjEdges[c2].push_back(c1);

                int realId1 = meshData->_nodedata.realids[c1];
                int realId2 = meshData->_nodedata.realids[c2];

                // 寻找种子边
                if (!foundSeed && (seedRealNodeId == realId1 || seedRealNodeId == realId2))
                {
                    seedContinuous1 = c1;
                    seedContinuous2 = c2;
                    targetPart = part;
                    foundSeed = true;
                }
            }

            // 既然在这个部件里找到了种子点，且邻接表已构建完毕，就无需再遍历其他部件了
            if (foundSeed)
            {
                break;
            }
        }

        if (!foundSeed)
        {
            return;
        }

        // =========================================================
        // 3. 基于线夹角的 BFS 双向扩展逻辑
        // =========================================================
        // 队列中存储的是有向边 {上一节点连续ID, 当前节点连续ID}
        std::queue<std::pair<int, int>> edgeQueue;
        // 用于记录已访问的无向边，防止死循环 (存入时保证 first < second)
        std::set<std::pair<int, int>> visitedEdges;
        // 收集满足条件的所有连续节点
        std::set<int> resultContinuousNodes;

        // 辅助 Lambda：标记边为已访问
        auto markVisited = [&](int u, int v) {
            visitedEdges.insert({ std::min(u, v), std::max(u, v) });
            };

        // 辅助 Lambda：判断边是否已访问
        auto isVisited = [&](int u, int v) {
            return visitedEdges.count({ std::min(u, v), std::max(u, v) });
            };

        // 阈值计算：将夹角容差转为 Cosine 值
        float cosThreshold = std::cos(angleThresholdDeg * M_PI / 180.0f);

        // 将初始边沿着两个方向推入队列进行双向扩散
        edgeQueue.push({ seedContinuous1, seedContinuous2 });
        edgeQueue.push({ seedContinuous2, seedContinuous1 });
        markVisited(seedContinuous1, seedContinuous2);
        resultContinuousNodes.insert(seedContinuous1);
        resultContinuousNodes.insert(seedContinuous2);

        while (!edgeQueue.empty())
        {
            auto currentEdge = edgeQueue.front();
            edgeQueue.pop();

            int prevCId = currentEdge.first;
            int currCId = currentEdge.second;

            // 获取前驱向量 (A -> B)
            QVector3D pPrev = meshData->getVectorById(meshData->_nodedata.realids[prevCId]);
            QVector3D pCurr = meshData->getVectorById(meshData->_nodedata.realids[currCId]);
            QVector3D vecIn = (pCurr - pPrev).normalized();

            // 遍历当前节点的所有相连边
            for (int nextCId : adjEdges[currCId])
            {
                // 不走回头路
                if (nextCId == prevCId) continue;
                // 防止成环死循环
                if (isVisited(currCId, nextCId)) continue;

                // 获取后续向量 (B -> C)
                QVector3D pNext = meshData->getVectorById(meshData->_nodedata.realids[nextCId]);
                QVector3D vecOut = (pNext - pCurr).normalized();

                // 计算两条线段的夹角 Cosine 值
                float dot = QVector3D::dotProduct(vecIn, vecOut);
                // 规避精度误差导致的崩溃
                if (dot > 1.0f) dot = 1.0f;
                if (dot < -1.0f) dot = -1.0f;

                // dot 越大，说明越接近 0 度（直线）。只要 dot >= 阈值，就说明夹角在容差内
                if (dot >= cosThreshold)
                {
                    markVisited(currCId, nextCId);
                    resultContinuousNodes.insert(nextCId);
                    edgeQueue.push({ currCId, nextCId });
                }
            }
        }

        // =========================================================
        // 4. 结果组装与 UI 更新
        // =========================================================
        std::vector<MeshSelectInfoData> res;
        // 将算法得出的连续节点ID全部转换为真实ID
        for (int cId : resultContinuousNodes)
        {
            MeshSelectInfoData info;
            info.data_id.node_id = meshData->_nodedata.realids[cId];
            res.push_back(info);
        }

        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, res);
        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();

    }

    void mSelectMenuFunction::slot_faceAngleSelect(float angleThresholdDeg)
    {
        if (!getCVM)
        {
            return;
        }

        int pickNum = 0;
        PickDataType pickType = getCVM->getCurrentPickDataType(pickNum);
        auto meshData = getCVM->getModelData()->getMeshData();
        // 几何面模式：以最后拾取的几何面为种子，按二面角扩散选中相邻的几何面
        if (pickType == PickDataType::GeoFace && pickNum >= 1)
        {
            auto geoDatabase = getCVM->getModelData()->getGeoData();
            auto faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
            if (faces.empty())
            {
                return;
            }
            int seedFaceId = faces.back();

            // 二面角(平缓连续为180°)与法向夹角互补，转换阈值保持与网格模式语义一致
            double dihedralThresholdDeg = 180.0 - angleThresholdDeg;
            //【OCC摘除·集成】旧静态族（实参版）→ 引擎视图（无参版，4 参重载）
            auto faceIds = getGEOServer.geoCalculate().getAdjacentFaceByFaceId(seedFaceId, dihedralThresholdDeg, 0, true);

            std::vector<int> res{ faceIds.begin(), faceIds.end() };
            getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);

            MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
            getCVM->getPreRenderWidget()->updatePaint();
            return;
        }

        if (pickNum < 1 ||
            (pickType != PickDataType::Node &&
             pickType != PickDataType::ElementFace &&
             pickType != PickDataType::Element))
        {
            return;
        }

        // =========================================================
        // 1. 根据拾取类型构建种子面列表
        // =========================================================
        std::vector<RenderPickResultData> seedFaces;

        if (pickType == PickDataType::Node)
        {
            std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
            int seedRealNodeId = nodes.back().data_id.node_id;

            if (!meshData->_nodedata.nodeidmap.contains(seedRealNodeId))
            {
                return;
            }
            int seedContinuousId = meshData->_nodedata.nodeidmap.value(seedRealNodeId);

            bool foundSeed = false;
            for (auto part : meshData->_partdata)
            {
                QSet<BoundaryFace> nodeFaces = part->_partfacedata.nodeFaces.value(seedContinuousId);
                if (!nodeFaces.empty())
                {
                    const BoundaryFace& face = *nodeFaces.begin();
                    RenderPickResultData seedFaceID;
                    seedFaceID.part_id = part->getPartID();
                    seedFaceID.result_id = part->_parteledata.realids[face.elementID];
                    seedFaceID.result_id2 = face.index;
                    seedFaces.push_back(seedFaceID);
                    foundSeed = true;
                    break;
                }
            }
            if (!foundSeed)
            {
                return;
            }
        }
        else if (pickType == PickDataType::ElementFace)
        {
            std::vector<MeshSelectInfoData> faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
            if (faces.empty())
            {
                return;
            }
            RenderPickResultData seed = SelectData2ResultData(PickDataType::ElementFace, faces.back());
            seedFaces.push_back(seed);
        }
        else if (pickType == PickDataType::Element)
        {
            std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
            if (eles.empty())
            {
                return;
            }
            const MeshSelectInfoData& seedEle = eles.back();
            int partID = seedEle.part_id;
            int realEleID = seedEle.data_id.element_id;

            const auto& faceIDs = meshData->getSurfaceByElementID(partID, realEleID);
            if (faceIDs.empty())
            {
                return;
            }

            for (const auto& faceID : faceIDs)
            {
                RenderPickResultData seed;
                seed.part_id = faceID.part_id;
                seed.result_id = faceID.result_id;
                seed.result_id2 = faceID.result_id2;
                seedFaces.push_back(seed);
            }
        }

        if (seedFaces.empty())
        {
            return;
        }

        // =========================================================
        // 2. 基于面夹角的 BFS 扩散核心逻辑
        // =========================================================
        std::queue<RenderPickResultData> faceQueue;
        // 使用 std::tuple 唯一标识一个面 (part_id, real_element_id, face_index)
        std::set<std::tuple<int, int, int>> visitedFaces;

        for (const auto& seed : seedFaces)
        {
            faceQueue.push(seed);
            visitedFaces.insert({ seed.part_id, seed.result_id, seed.result_id2 });
        }

        float cosThreshold = std::cos(angleThresholdDeg * M_PI / 180.0f);

        // 内部 Lambda：利用实际接口计算面的法向量
        auto calcFaceNormal = [&](const RenderPickResultData& faceID) -> QVector3D {
            QVector<int> nIdx = meshData->getNodeIndexBySurface(faceID);
            if (nIdx.size() < 3) return QVector3D();
            QVector3D p0 = meshData->getVectorById(nIdx[0]);
            QVector3D p1 = meshData->getVectorById(nIdx[1]);
            QVector3D p2 = meshData->getVectorById(nIdx[2]);
            return QVector3D::crossProduct(p1 - p0, p2 - p0).normalized();
            };

        while (!faceQueue.empty())
        {
            RenderPickResultData currFace = faceQueue.front();
            faceQueue.pop();

            QVector3D currNormal = calcFaceNormal(currFace);
            QVector<int> currNodes = meshData->getNodeIndexBySurface(currFace);

            // 利用实际的 nodeFaces 逻辑寻找相邻面
            mPartData* currPart = meshData->getPartData(currFace.part_id);
            if (!currPart)
            {
                continue;
            }
            for (int nId : currNodes)
            {
                int continuousNId = meshData->_nodedata.nodeidmap.value(nId);
                if (continuousNId == -1)
                {
                    continue;
                }

                QSet<BoundaryFace> nodeFaces = currPart->_partfacedata.nodeFaces.value(continuousNId);
                if (nodeFaces.empty())
                {
                    continue;
                }

                for (const auto& face : nodeFaces)
                {
                    RenderPickResultData adjFaceID;
                    adjFaceID.part_id = currPart->getPartID();
                    adjFaceID.result_id = currPart->_parteledata.realids[face.elementID];
                    adjFaceID.result_id2 = face.index;

                    auto faceKey = std::make_tuple(adjFaceID.part_id, adjFaceID.result_id, adjFaceID.result_id2);

                    // 已经访问过则跳过
                    if (visitedFaces.count(faceKey))
                    {
                        continue;
                    }

                    // 检测相邻面是否共边 (提取实际节点判断交集)
                    QVector<int> adjNodes = meshData->getNodeIndexBySurface(adjFaceID);
                    int sharedNodesCount = 0;
                    for (int cn : currNodes)
                    {
                        for (int an : adjNodes)
                        {
                            if (cn == an) sharedNodesCount++;
                        }
                    }
                    // 只有共享 2 个及以上的节点才算是共边相邻，排除仅共顶点的面
                    if (sharedNodesCount < 2)
                    {
                        continue;
                    }

                    // 计算法向夹角
                    QVector3D adjNormal = calcFaceNormal(adjFaceID);
                    float dot = QVector3D::dotProduct(currNormal, adjNormal);
                    // 防御性编程：消除浮点误差导致 dot 略大于 1 的情况
                    if (dot > 1.0f) dot = 1.0f;
                    if (dot < -1.0f) dot = -1.0f;

                    if (dot >= cosThreshold)
                    {
                        visitedFaces.insert(faceKey);
                        faceQueue.push(adjFaceID);
                    }
                }
            }
        }

        // =========================================================
        // 3. 结果组装与 UI 更新
        // =========================================================
        std::vector<MeshSelectInfoData> res;

        if (pickType == PickDataType::Node)
        {
            std::set<int> resultRealNodes;
            for (const auto& face : visitedFaces)
            {
                RenderPickResultData pick;
                pick.part_id = std::get<0>(face);
                pick.result_id = std::get<1>(face);
                pick.result_id2 = std::get<2>(face);
                QVector<int> nodeIds = meshData->getAllNodeIndexBySurface(pick);
                for (int nId : nodeIds)
                {
                    resultRealNodes.insert(nId);
                }
            }
            for (int realNodeId : resultRealNodes)
            {
                MeshSelectInfoData info;
                info.data_id.node_id = realNodeId;
                res.push_back(info);
            }
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, res);
        }
        else if (pickType == PickDataType::ElementFace)
        {
            for (const auto& face : visitedFaces)
            {
                MeshSelectInfoData info;
                info.part_id = std::get<0>(face);
                info.data_id.element_id = std::get<1>(face);
                info.data_index.face_index = std::get<2>(face);
                res.push_back(info);
            }
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, res);
        }
        else if (pickType == PickDataType::Element)
        {
            std::set<std::pair<int, int>> resultElements; // (part_id, element_id)
            for (const auto& face : visitedFaces)
            {
                resultElements.insert({ std::get<0>(face), std::get<1>(face) });
            }
            for (const auto& ele : resultElements)
            {
                MeshSelectInfoData info;
                info.part_id = ele.first;
                info.data_id.element_id = ele.second;
                res.push_back(info);
            }
            getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, res);
        }

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mSelectMenuFunction::slot_pathSelect()
    {
        struct AStarNode {
            int id;
            float f_score; // 综合得分 f(n) = g(n) + h(n)
            // 重载大于号，使得 priority_queue 成为小顶堆（f_score 最小的排在前面优先探索）
            bool operator>(const AStarNode& other) const {
                return f_score > other.f_score;
            }
        };

        if (!getCVM)
        {
            return;
        }
        auto meshData = getCVM->getModelData()->getMeshData();
        int pickNum = 0;
        PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
        auto geoManager = getCVM->getModelData()->getGeoManager();
        std::vector<MeshSelectInfoData> eles;
        if (pickDataType != PickDataType::Node)
        {
            return;
        }
        if (pickNum < 2)
        {
            return;
        }
        std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
        /*
        vector<MeshSelectInfoData> res;
        //通过最近拾取的两个节点连成最短路径
        //取最后两个节点连接路径
        std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
        for (auto node : nodes)
        {
            if (!meshData->_nodedata.nodeidmap.contains(node.data_id.node_id))
            {
                continue;
            }
            int id = meshData->_nodedata.nodeidmap.value(node.data_id.node_id);//获取节点连续索引ID
            for (auto part : meshData->_partdata)
            {
                QSet<BoundaryFace> nodeFaces = part->_partfacedata.nodeFaces.value(id);//BoundaryFace结构体内部为elementID(连续id)和第几个面索引
                if (nodeFaces.empty())
                {
                    continue;
                }

                //找到节点所在的单元面
                RenderPickResultData  meshFaceID;
                meshFaceID.part_id = part->getPartID();
                meshFaceID.result_id =part->_parteledata.realids[nodeFaces.begin()->elementID];
                meshFaceID.result_id2 = nodeFaces.begin()->index;
                QVector<int> nodeIndex = meshData->getNodeIndexBySurface(meshFaceID);//找到了节点node.data_id.node_id相邻的节点
            }
        }
        */

        // 2. 取最后拾取的两个节点作为路径的起点和终点
        int startId = nodes[nodes.size() - 2].data_id.node_id;
        int endId = nodes[nodes.size() - 1].data_id.node_id;

        if (startId == endId)
        {
            return; // 起点和终点相同，无需寻路
        }

        // =========================================================
        // 3. A* (A-Star) 几何最短路径搜索核心逻辑
        // =========================================================
        QVector3D endPos = meshData->getVectorById(endId);

        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> pq;
        std::map<int, float> gScore; // g(n): 记录从起点到各节点的实际已走最短距离
        std::map<int, int> parentMap; // 记录路径的来源节点，用于最终反向回溯
        std::set<int> visited; // 记录已经完全探索过的节点，避免重复计算

        // 初始化起点
        gScore[startId] = 0.0f;
        QVector3D startPos = meshData->getVectorById(startId);
        float startHeuristic = (startPos - endPos).length(); // h(n): 起点到终点的预测直线距离
        pq.push({ startId, startHeuristic }); // f(n) = 0 + h(n)

        bool pathFound = false;

        // 开始定向搜索
        while (!pq.empty())
        {
            AStarNode current = pq.top();
            pq.pop();

            int currId = current.id;

            // 当前弹出的节点是终点，说明找到了最短路径
            if (currId == endId)
            {
                pathFound = true;
                break;
            }

            // 如果该节点已经被探索过，直接跳过
            if (visited.count(currId))
            {
                continue;
            }
            visited.insert(currId);

            QVector3D currPos = meshData->getVectorById(currId);
            std::set<int> neighbors;

            // 3.1 拓展当前节点的相邻节点 (通过面来找点)
            int id = meshData->_nodedata.nodeidmap.value(currId);
            if (id == -1)
            {
                continue;
            }
            for (auto part : meshData->_partdata)
            {
                QSet<BoundaryFace> nodeFaces = part->_partfacedata.nodeFaces.value(id);
                if (nodeFaces.empty())
                {
                    continue;
                }

                for (const auto& face : nodeFaces)
                {
                    RenderPickResultData meshFaceID;
                    meshFaceID.part_id = part->getPartID();
                    meshFaceID.result_id = part->_parteledata.realids[face.elementID];
                     meshFaceID.result_id2 = face.index; 

                    QVector<int> nodeIndex = meshData->getNodeIndexBySurface(meshFaceID);
                    for (int nId : nodeIndex)
                    {
                        if (nId != currId)
                        {
                            neighbors.insert(nId);
                        }
                    }
                }
            }

            // 3.2 遍历相邻节点，计算代价并更新优先队列
            for (int neighborId : neighbors)
            {
                // 已在 Closed Set 中的节点不再处理
                if (visited.count(neighborId))
                {
                    continue;
                }

                QVector3D neighborPos = meshData->getVectorById(neighborId);
                // 计算实际代价 g(n) = 当前实际总路程 + 当前点到邻居的距离
                float weight = (neighborPos - currPos).length();
                float tentative_gScore = gScore[currId] + weight;

                // 如果发现了到达该邻居更短的实际路径，则更新数据
                if (gScore.find(neighborId) == gScore.end() || tentative_gScore < gScore[neighborId])
                {
                    gScore[neighborId] = tentative_gScore;
                    parentMap[neighborId] = currId;
                    // 计算启发式预测代价 h(n) = 邻居到终点的直线距离
                    float hScore = (neighborPos - endPos).length();
                    // 综合得分 f(n) = g(n) + h(n)
                    float fScore = tentative_gScore + hScore;
                    pq.push({ neighborId, fScore });
                }
            }
        }

        // =========================================================
        // 4. 路径回溯与数据组装
        // =========================================================
        std::vector<MeshSelectInfoData> res;

        if (pathFound)
        {
            std::vector<int> pathRealIds;
            int temp = endId;
            while (temp != startId)
            {
                pathRealIds.push_back(temp);
                temp = parentMap[temp];
            }
            pathRealIds.push_back(startId);
            // 反转数组，使其顺序变为正向 (起点 -> 终点)
            std::reverse(pathRealIds.begin(), pathRealIds.end());

            // 直接构造结果数据
            for (int realId : pathRealIds)
            {
                MeshSelectInfoData info;
                info.data_id.node_id = realId;
                res.push_back(info);
            }
        }


        getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, res);

        MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mSelectMenuFunction::setPickAssistMode(PickAssistMode mode)
    {
        _pickAssistMode = mode;
        if (mode != PickAssistMode::None)
        {
            ensurePickAssistConnection();
        }
    }

    void mSelectMenuFunction::setPickAssistLineAngle(float angle)
    {
        _pickAssistLineAngle = angle;
    }

    void mSelectMenuFunction::setPickAssistFaceAngle(float angle)
    {
        _pickAssistFaceAngle = angle;
    }

    void mSelectMenuFunction::applyPickAssist()
    {
        if (_pickAssistBusy || _pickAssistMode == PickAssistMode::None)
        {
            return;
        }
        if (!getCVM || !getCVM->getPreRenderWidget())
        {
            return;
        }
        //执行过程中的 UpdatePreWidgetHighlight 会再次发出 ReadyToReadContainer，用 _pickAssistBusy 防止重入
        _pickAssistBusy = true;
        switch (_pickAssistMode)
        {
        case PickAssistMode::LineAngle:
            slot_lineAngleSelect(_pickAssistLineAngle);
            break;
        case PickAssistMode::FaceAngle:
            slot_faceAngleSelect(_pickAssistFaceAngle);
            break;
        case PickAssistMode::Path:
            slot_pathSelect();
            break;
        default:
            break;
        }
        _pickAssistBusy = false;
    }

    void mSelectMenuFunction::slot_pickAssistOnPick()
    {
        applyPickAssist();
    }

    void mSelectMenuFunction::ensurePickAssistConnection()
    {
        if (conn_geo)
        {
            disconnect(conn_geo);
            conn_geo = QMetaObject::Connection();
        }
        if (!getCVM)
        {
            return;
        }
        auto widget = getCVM->getPreRenderWidget();
        if (!widget || !widget->getSelectionContainer())
        {
            return;
        }
        conn_geo = connect(widget->getSelectionContainer(), &mxr::SelectionDataContainer::ReadyToReadContainer, this, &mSelectMenuFunction::slot_pickAssistOnPick);
    }
    

	void mSelectMenuFunction::slot_mappingGeoPoint(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		std::vector<int> points;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		case PickDataType::GeoFace:
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType(int(pickDataType)));
			points = this->getShapeRelated(res, pickDataType, 0);
			break;
		}
		case PickDataType::Node:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			for (auto r : res)
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(r.data_id.element_id);
				for (auto partId : partIds)
				{
					mPartData* part = meshData->getPartData(partId);
					if (part == nullptr)
						continue;

					QHashIterator<int, int> iter(part->_partGeoToMeshdata.pointToNode);
					while (iter.hasNext())
					{
						iter.next();
						if (iter.value() == r.data_id.element_id)
						{
							points.push_back(iter.key());
						}
					}
				}
			}
			break;
		}
		case PickDataType::ElementEdge:
		case PickDataType::ElementFace:
		case PickDataType::Element:
		{
			slot_mappingNode(isHighlight);
			slot_mappingGeoPoint(isHighlight);
			return;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				auto ids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 0);
				if (!ids.empty())
				{
					points.reserve(points.size() + ids.size());
					points.insert(points.end(), ids.begin(), ids.end());
				}
			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(1);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, points);
		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		//MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingGeoEdge(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		std::vector<int> edges;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		case PickDataType::GeoFace:
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType(int(pickDataType)));
			edges = this->getShapeRelated(res, pickDataType, 1);
			break;
		}
		case PickDataType::ElementEdge:
		{
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);
			std::unordered_set<int> seen;
			for (auto ele : eles)
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(ele.data_id.element_id);
				bool isBreak = false;
				for (auto partId : partIds)
				{
					auto partData = meshData->getPartData(partId);
					if (partData == nullptr)
					{
						continue;
					}
					QHashIterator<int, std::vector<int>> iter(partData->_partGeoToMeshdata.edgeToNode);
					while (iter.hasNext())
					{
						iter.next();
						if (find(iter.value().begin(), iter.value().end(), ele.data_id.element_id) != iter.value().end() && find(iter.value().begin(), iter.value().end(), ele.data_index.line_index) != iter.value().end())//存在
						{
							if (seen.insert(iter.key()).second)
							{
								edges.push_back(iter.key()); isBreak = true;
								break;
							}
						}
					}
					if (isBreak)
					{
						break;
					}
				}

			}

			break;
		}
		case PickDataType::Node:
		case PickDataType::ElementFace:
		case PickDataType::Element:
		{
			slot_mappingMeshLine(isHighlight);
			slot_mappingGeoEdge(isHighlight);
			return;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				auto ids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 1);
				if (!ids.empty())
				{
					edges.reserve(edges.size() + ids.size());
					edges.insert(edges.end(), ids.begin(), ids.end());
				}


			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, edges);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingGeoFace(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		std::vector<int> faces;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		case PickDataType::GeoEdge:
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType(int(pickDataType)));
			faces = this->getShapeRelated(res, pickDataType, 2);
			break;
		}
        case PickDataType::ElementFace:
        {
            auto eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
            std::unordered_set<int> seen;

            for (const auto& ele : eles)
            {
                auto partData = meshData->getPartData(ele.part_id);
                if (partData == nullptr)
                {
                    continue;
                }

                RenderPickResultData data = MechPreData::SelectData2ResultData(PickDataType::ElementFace, ele);
                int eleid = data.result_id;

                // 先验证1个eleid是否存在，再验证3个节点是否存在
                for (auto it = partData->_partGeoToMeshdata.faceToMesh.constBegin();
                    it != partData->_partGeoToMeshdata.faceToMesh.constEnd(); ++it)
                {
                    int faceId = it.key();
                    const std::vector<int>& meshList = it.value();

                    if (std::find(meshList.begin(), meshList.end(), eleid) == meshList.end())
                    {
                        continue; // 不存在，直接跳过
                    }

                    // 如果单元 ID 存在，再获取对应的节点列表进行验证
                    const std::vector<int>& nodeList = partData->_partGeoToMeshdata.faceToNode.value(faceId);

                    auto eleNodes = meshData->getNodeIndexBySurface(data);

                    // 节点数小于 3 直接跳过 
                    if (eleNodes.size() < 3)
                    {
                        continue;
                    }

                    if (std::find(nodeList.begin(), nodeList.end(), eleNodes[0]) != nodeList.end() &&
                        std::find(nodeList.begin(), nodeList.end(), eleNodes[1]) != nodeList.end() &&
                        std::find(nodeList.begin(), nodeList.end(), eleNodes[2]) != nodeList.end())
                    {
                        // 全部满足，插入到结果中
                        if (seen.insert(faceId).second)
                        {
                            faces.push_back(faceId);
                        }
                        break; // 已经找到对应的几何面，跳出遍历
                    }
                }
            }
            break;
        }
		case PickDataType::Node:
		case PickDataType::ElementEdge:
		case PickDataType::Element:
		{
			slot_mappingMeshFace(isHighlight);
			slot_mappingGeoFace(isHighlight);
			return;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				auto ids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 2);
				if (!ids.empty())
				{
					faces.reserve(faces.size() + ids.size());
					faces.insert(faces.end(), ids.begin(), ids.end());
				}


			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, faces);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingGeoSolid(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		std::vector<int> solids;
		switch (pickDataType)
		{
		case PickDataType::GeoFace:
		case PickDataType::GeoEdge:
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType(int(pickDataType)));
			solids = this->getShapeRelated(res, pickDataType, 3);
			break;
		}
		case PickDataType::Element:
		{
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			std::unordered_set<int> seen;
			for (auto ele : eles)
			{
				auto partData = meshData->getPartData(ele.part_id);
				if (partData == nullptr)
				{
					continue;
				}
				QHashIterator<int, std::vector<int>> iter(partData->_partGeoToMeshdata.solidToMesh);
				while (iter.hasNext())
				{
					iter.next();
					if (find(iter.value().begin(), iter.value().end(), ele.data_id.element_id) != iter.value().end())//存在
					{
						if (seen.insert(iter.key()).second)
						{
							solids.push_back(iter.key());
							break;
						}
					}
				}
			}

			break;
		}
		case PickDataType::Node:
		case PickDataType::ElementEdge:
		case PickDataType::ElementFace:
		{
			slot_mappingEle(isHighlight);
			slot_mappingGeoSolid(isHighlight);
			return;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				auto ids = getGEOServer.geoCalculate().getAllGeoIDByPart(partId, 3);
				if (!ids.empty())
				{
					solids.reserve(solids.size() + ids.size());
					solids.insert(solids.end(), ids.begin(), ids.end());
				}


			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, solids);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingNode(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDataBase = getCVM->getModelData()->getGeoData();
		std::vector<MeshSelectInfoData> nodes;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			for (auto id : res)
			{
				//【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
				int partId = MGeoNeutralQuery::partIdByGeoId(
					(getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getGeoManager() : nullptr, 3, id);
				auto partData = meshData->getPartData(partId);
				if (!partData)
				{
					continue;
				}
				if (!partData->_partGeoToMeshdata.solidToNode.contains(id))
				{
					continue;
				}
				auto solidNodes = partData->_partGeoToMeshdata.solidToNode.value(id);
				for (auto nid : solidNodes)
				{
					if (partData->_parteledata.nodeelements.constFind(nid) != partData->_parteledata.nodeelements.cend())
					{
						if (seen.insert(nid).second)  // 如果没见过
						{
							MeshSelectInfoData d;
							d.data_id.node_id = nid;
							nodes.push_back(d);
						}
					}
				}
			}
			break;
		}
		case PickDataType::GeoFace:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			for (auto id : res)
			{
				auto faceNodes = meshData->getNodeByGeoFace(id);
				for (auto nid : faceNodes)
				{
					if (nid.result_id == -1)
					{
						continue;
					}
					if (seen.insert(nid.result_id).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.node_id = nid.result_id;
						nodes.push_back(d);
					}
				}
			}
			break;
		}
		case PickDataType::GeoEdge:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			for (auto id : res)
			{
				auto edgeNodes = meshData->getNodeByGeoEdge(id);
				for (auto nid : edgeNodes)
				{
					if (nid.result_id == -1)
					{
						continue;
					}
					if (seen.insert(nid.result_id).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.node_id = nid.result_id;
						nodes.push_back(d);
					}
				}
			}
			break;
		}
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			for (auto r : res)
			{
				auto point = meshData->getNodeByGeoPoint(r);
				if (point.result_id == -1)
				{
					continue;
				}
				MeshSelectInfoData d;
				d.data_id.node_id = point.result_id;
				nodes.push_back(d);
			}
			break;
		}
		case PickDataType::ElementEdge:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);

			for (auto ele : eles)
			{
				if (seen.insert(ele.data_id.node_id).second)  // 如果没见过
				{
					MeshSelectInfoData d;
					d.data_id.node_id = ele.data_id.node_id;
					nodes.push_back(d);
				}
				if (seen.insert(ele.data_index.line_index).second)  // 如果没见过
				{
					MeshSelectInfoData d;
					d.data_id.node_id = ele.data_index.line_index;
					nodes.push_back(d);
				}

			}

			break;
		}
		case PickDataType::ElementFace:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			for (auto ele : eles)
			{
				RenderPickResultData data = MechPreData::SelectData2ResultData(PickDataType::ElementFace, ele);
				auto eleNodes = meshData->getNodeIndexBySurface(data);
				for (int nid : eleNodes)
				{
					if (seen.insert(nid).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.node_id = nid;
						nodes.push_back(d);
					}
				}
			}

			break;
		}
		case PickDataType::Element:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			for (auto ele : eles)
			{
				auto eleNodes = meshData->getBFaceNodeIdsByEleId(ele.part_id, ele.data_id.element_id);
				for (int nid : eleNodes)
				{
                    if (nid == -1)
                    {
                        continue;
                    }
					if (seen.insert(nid).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.node_id = nid;
						nodes.push_back(d);
					}
				}
			}

			break;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				mPartData* part = meshData->getPartData(partId);
				if (part == nullptr)
					continue;
				auto res = part->_parteledata.nodeelements.keys();
				for (int nid : res)
				{
                    if (nid == -1)
                    {
                        continue;
                    }
					MeshSelectInfoData d;
					d.data_id.node_id = nid;
					nodes.push_back(d);
				}
			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshNode, nodes);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingMeshLine(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		std::vector<MeshSelectInfoData> eleLines;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		case PickDataType::GeoFace:
		case PickDataType::GeoEdge:
		{
			slot_mappingEle(isHighlight);//先映射为单元
			slot_mappingMeshLine(isHighlight);//再通过单元映射为单元线
			return;
		}
		case PickDataType::GeoPoint:
		{
			slot_mappingNode(isHighlight);
			slot_mappingMeshLine(isHighlight);
			return;
		}
		case PickDataType::Node:
		{
			QSet<int> nodeset;
			std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			//int lianxuid = -1;
			for (auto node : nodes)
			{
				nodeset.insert(node.data_id.element_id);
				//lianxuid = meshData->_nodedata.nodeidmap.value(node.data_id.element_id);
			}
			for (auto partId : meshData->getAllPartIds())
			{
				auto part = meshData->getPartData(partId);
				if (part == nullptr)
				{
					continue;
				}
				std::unordered_set<std::pair<int, int>> lines;
				for (auto iter : part->_partedgedata.nodesurfaceids) {
					//转为真实id
					int readid1 = meshData->_nodedata.realids[iter.first.first];
					int readid2 = meshData->_nodedata.realids[iter.first.second];
					if (nodeset.contains(readid1) || nodeset.contains(readid2))
					{
						if (lines.insert({ std::min(readid1, readid2), std::max(readid1, readid2) }).second)
						{
							MeshSelectInfoData d;
							d.data_id.element_id = readid1;
							d.data_index.line_index = readid2;
							d.part_id = partId;
							eleLines.push_back(d);
						}

					}
				}
			}

			break;
		}
		case PickDataType::ElementFace:
		{
			std::vector<MeshSelectInfoData> eleFaces = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			vector<RenderPickResultData> surfaceIDs;
			for (auto eleFace : eleFaces)
			{
				surfaceIDs.push_back(MechPreData::SelectData2ResultData(SelectionInfoType::MeshFace, eleFace));
			}

			QHash<int, QSet<std::pair<int, int>>> partLines = meshData->getLineBySurfaceID(surfaceIDs);

			QHashIterator<int, QSet<std::pair<int, int>>> iter(partLines);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.node_id = meshData->_nodedata.realids[ele.first];
					d.data_index.line_index = meshData->_nodedata.realids[ele.second];
					d.part_id = partId;
					eleLines.push_back(d);
				}
			}

			break;
		}
		case PickDataType::Element:
		{
			slot_mappingMeshFace(isHighlight);
			slot_mappingMeshLine(isHighlight);
			return;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				auto res = meshData->getMeshLineByPart(partId);

				for (auto id : res)
				{
					MeshSelectInfoData d;
					d.data_id.element_id = id.first;
					d.data_index.line_index = id.second;
					d.part_id = partId;
					eleLines.push_back(d);
				}
			}

			break;
		}
		}

		getCVM->clearPick();
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(6);

		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshLine, eleLines);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingMeshFace(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		std::vector<MeshSelectInfoData> eleFaces;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			slot_mappingEle(isHighlight);//先映射为单元
			slot_mappingMeshFace(isHighlight);//再通过单元映射为单元面
			return;
		}
		case PickDataType::GeoFace:
		{
			std::vector<int> faces = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			std::set<int> faceSets{ faces.begin(), faces.end() };
			auto faceNodes = meshData->getNodeByGeoFace(faceSets);

			slot_mappingEle();//先映射为单元
			slot_mappingMeshFace();//再通过单元映射为单元面
			//TODO删掉三个节点不完全处于几何面的单元面
			std::vector<MeshSelectInfoData> elefaces = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			for (auto eleFace : elefaces)
			{
				auto partData = meshData->getPartData(eleFace.part_id);
				if (partData == nullptr)
				{
					continue;
				}
				RenderPickResultData data = MechPreData::SelectData2ResultData(PickDataType::ElementFace, eleFace);
				auto eleNodes = meshData->getNodeIndexBySurface(data);
				if (eleNodes.size() < 3)
				{
					continue;
				}
				//int face1 = partData->_partGeoToMeshdata.nodeToFace.value(eleNodes.at(0));
				//int face2 = partData->_partGeoToMeshdata.nodeToFace.value(eleNodes.at(1));
				//int face3 = partData->_partGeoToMeshdata.nodeToFace.value(eleNodes.at(2));
				//partData->_partGeoToMeshdata.faceToNode.value(face1);
				//partData->_partGeoToMeshdata.faceToNode.value(face2);
				//partData->_partGeoToMeshdata.faceToNode.value(face3);
				if (faceNodes.find(eleNodes.at(0)) != faceNodes.end() && faceNodes.find(eleNodes.at(1)) != faceNodes.end() && faceNodes.find(eleNodes.at(2)) != faceNodes.end())
				{
					eleFaces.push_back(eleFace);
				}
			}
			break;
		}
		case PickDataType::GeoEdge:
		{
			slot_mappingEle(isHighlight);//先映射为单元
			slot_mappingMeshFace(isHighlight);//再通过单元映射为单元面
			return;
		}
		case PickDataType::GeoPoint:
		{
			slot_mappingNode(isHighlight);
			slot_mappingMeshFace(isHighlight);
			return;
		}
		case PickDataType::Node:
		{
			QHash<int, QSet<BoundaryFace>> partFaces;
			std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			for (auto node : nodes)
			{	
				int id = meshData->_nodedata.nodeidmap.value(node.data_id.element_id);//获取连续索引ID
				if (id == -1)
				{
					continue;
				}
				
				for (auto part : meshData->_partdata)
				{
					auto nodeFaces = part->_partfacedata.nodeFaces.value(id);
					if (nodeFaces.size() == 0)
					{
						continue;
					}
					partFaces[part->getPartID()].unite(nodeFaces);
				}
			}

			QHashIterator<int, QSet<BoundaryFace>> iter(partFaces);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();
				mPartData* part = meshData->getPartData(partId);

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.element_id = part->_parteledata.realids[ele.elementID];
					d.part_id = partId;
					d.data_index.face_index = ele.index;
					eleFaces.push_back(d);
				}
			}

			break;
		}
		case PickDataType::ElementEdge:
		{
			QHash<int, QSet<BoundaryFace>> partEles;
			std::vector<MeshSelectInfoData> eleLines = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);

			for (auto eleLine : eleLines)
			{
				if (!meshData->_nodedata.nodeidmap.contains(eleLine.data_id.node_id) ||
				    !meshData->_nodedata.nodeidmap.contains(eleLine.data_index.line_index))
				{
					continue;
				}
				int nodeId1 = meshData->_nodedata.nodeidmap.value(eleLine.data_id.node_id);//转换为连续Id
				int nodeId2 = meshData->_nodedata.nodeidmap.value(eleLine.data_index.line_index);//转换为连续Id
				mPartData* part = meshData->getPartData(eleLine.part_id);
				if (part == nullptr)
					continue;

				auto faces = part->_partedgedata.nodesurfaceids[std::pair<int, int>{ nodeId1, nodeId2 }];
				for (auto id : faces) {
					partEles[eleLine.part_id].insert(id);
				}
			}

			QHashIterator<int, QSet<BoundaryFace>> iter(partEles);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();
				mPartData* part = meshData->getPartData(partId);
				if (!part)
					continue;
				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.element_id = part->_parteledata.realids[ele.elementID];//转换为真实id
					d.part_id = partId;
					d.data_index.face_index = ele.index;
					eleFaces.push_back(d);
				}
			}

			break;
		}
		case PickDataType::Element:
		{
			QHash<int, QSet<RenderPickResultData>> partEles;
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			for (auto ele : eles)
			{
				auto meshFaceIds = meshData->getSurfaceByElementID(ele.part_id, ele.data_id.element_id);
				for (auto& id : meshFaceIds) {
					partEles[ele.part_id].insert(id);
				}
			}

			QHashIterator<int, QSet<RenderPickResultData>> iter(partEles);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d = MechPreData::PickData2SelectData(PickDataType::ElementFace, ele);
					eleFaces.push_back(d);
				}
			}

			break;
		}
		case PickDataType::Part:
		{
			auto parts = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto partId : parts)
			{
				const auto& bFace = meshData->getAll3DMeshFaceIDsInPartByRPRD(partId);

				for (const auto& face : bFace)
				{
					MeshSelectInfoData d;
					d.data_id.element_id = face.result_id;
					d.part_id = partId;
					d.data_index.face_index = face.result_id2;
					eleFaces.push_back(d);
				}
			}

			break;
		}
		default:
			break;
		}

		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(7);

		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshFace, eleFaces);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingEle(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDataBase = getCVM->getModelData()->getGeoData();
		std::vector<MeshSelectInfoData> eles;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			for (auto id : res)
			{
				//【OCC摘除·S9】旧栈 set 反查退役，改 GN 部件 ID
				int partId = MGeoNeutralQuery::partIdByGeoId(
					(getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getGeoManager() : nullptr, 3, id);
				auto partData = meshData->getPartData(partId);
				if (!partData)
				{
					continue;
				}
				if (!partData->_partGeoToMeshdata.solidToMesh.contains(id))
				{
					continue;
				}
				auto solidEles = partData->_partGeoToMeshdata.solidToMesh.value(id);
				for (auto nid : solidEles)
				{
					if (partData->_parteledata.elementidmap.constFind(nid) != partData->_parteledata.elementidmap.cend())
					{
						if (seen.insert(nid).second)  // 如果没见过
						{
							MeshSelectInfoData d;
							d.data_id.element_id = nid;
							d.part_id = partId;
							eles.push_back(d);
						}
					}
				}
			}
			break;
		}
		case PickDataType::GeoFace:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			for (auto id : res)
			{
				auto faceEles = meshData->getElementByGeoFace(id);
				for (auto nid : faceEles)
				{
					if (seen.insert(nid.result_id).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.element_id = nid.result_id;
						d.part_id = nid.part_id;
						eles.push_back(d);
					}
				}
			}
			break;
		}
		case PickDataType::GeoEdge:
		{
			std::unordered_set<int> seen;
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			for (auto id : res)
			{
				auto edgeEles = meshData->getElementByGeoEdge(id);
				for (auto nid : edgeEles)
				{
					if (seen.insert(nid.result_id).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.element_id = nid.result_id;
						d.part_id = nid.part_id;
						eles.push_back(d);
					}
				}
			}
			break;
		}
		case PickDataType::GeoPoint:
		{
			slot_mappingNode(isHighlight);
			slot_mappingMeshFace(isHighlight);
			return;
		}
		case PickDataType::Node:
		{
			QHash<int, QSet<int>> partEles;
			std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			for (auto node : nodes)
			{
				for (auto part : meshData->_partdata)
				{
					auto nodeEles = part->_parteledata.nodeelements.value(node.data_id.element_id);
					if (nodeEles.size() == 0)
					{
						continue;
					}
                    for (int eleId : nodeEles)
                    {
                        partEles[part->getPartID()].insert(eleId);
                    }
				}
			}

			QHashIterator<int, QSet<int>> iter(partEles);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.element_id = ele;
					d.part_id = partId;
					eles.push_back(d);
				}
			}

			break;
		}
		case PickDataType::ElementEdge:
		{
			QHash<int, QSet<int>> partEles;
			std::vector<MeshSelectInfoData> eleLines = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);

			for (auto eleLine : eleLines)
			{
				int nodeId1 = eleLine.data_id.element_id;
				int nodeId2 = eleLine.data_index.line_index;
				mPartData* part = meshData->getPartData(eleLine.part_id);
				if (part == nullptr)
					continue;

				auto eles1 = part->_parteledata.nodeelements.value(nodeId1);
				auto eles2 = part->_parteledata.nodeelements.value(nodeId2);
                QSet<int> eleSet1;
                for (int e : eles1)
                {
                    eleSet1.insert(e);
                }
                QSet<int> eleSet2;
                for (int e : eles2)
                {
                    eleSet2.insert(e);
                }
                QSet<int> intersectEles = eleSet1 & eleSet2;
				if (intersectEles.size() > 0)
				{
					partEles[part->getPartID()].unite(intersectEles);
				}
			}

			QHashIterator<int, QSet<int>> iter(partEles);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.element_id = ele;
					d.part_id = partId;
					eles.push_back(d);
				}
			}

			break;
		}
		case PickDataType::ElementFace:
		{
			QHash<int, QSet<int>> partEles;
			std::vector<MeshSelectInfoData> eleFaces = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			for (auto eleFace : eleFaces)
			{
				mPartData* part = meshData->getPartData(eleFace.part_id);
				if (part == nullptr)
					continue;
				partEles[part->getPartID()].insert(eleFace.data_id.element_id);
			}

			QHashIterator<int, QSet<int>> iter(partEles);
			while (iter.hasNext())
			{
				iter.next();
				int partId = iter.key();

				for (auto ele : iter.value())
				{
					MeshSelectInfoData d;
					d.data_id.element_id = ele;
					d.part_id = partId;
					eles.push_back(d);
				}
			}

			break;
		}
		case PickDataType::Part:
		{
			std::unordered_set<int> seen;
			auto res = getPreWidgetSelectPartID(getCVM->getPreRenderWidget());
			for (auto id : res)
			{
				auto faceEles = meshData->getEleIdsByPartId(id);
				for (auto nid : faceEles)
				{
					if (seen.insert(nid).second)  // 如果没见过
					{
						MeshSelectInfoData d;
						d.data_id.element_id = nid;
						d.part_id = id;
						eles.push_back(d);
					}
				}
			}
			break;
		}
		default:
			break;
		}


		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);
		getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::ADD, SelectionInfoType::MeshElement, eles);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_mappingPart(bool isHighlight)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoManager = getCVM->getModelData()->getGeoManager();
		auto geoDataBase = getCVM->getModelData()->getGeoData();
		std::vector<int> parts;
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			std::unordered_set<int> seen;
			for (auto id : res)
			{
				auto geoDataModelSet = /*S9 退役*/ nullptr; Q_UNUSED(id);
				if (!geoDataModelSet)
					continue;
				int partId = -1; /*S9 退役*/
				if (partId == -1)
				{
					continue;
				}
				
				if (seen.insert(partId).second)  // 如果没见过
				{
					parts.push_back(partId);
				}
			}

			break;
		}
		case PickDataType::GeoFace:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			std::unordered_set<int> seen;
			for (auto id : res)
			{
				auto partId = meshData->getPartIdByGeoFace(id);
				if (seen.insert(partId).second)  // 如果没见过
				{
					parts.push_back(partId);
				}
			}

			break;
		}
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			std::unordered_set<int> seen;
			for (auto id : res)
			{
				auto partId = meshData->getPartIdByGeoEdge(id);
				if (seen.insert(partId).second)  // 如果没见过
				{
					parts.push_back(partId);
				}
			}

			break;
		}
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			std::unordered_set<int> seen;
			for (auto id : res)
			{
				auto partId = meshData->getPartIdByGeoPoint(id);
				if (seen.insert(partId).second)  // 如果没见过
				{
					parts.push_back(partId);
				}
			}

			break;
		}
		case PickDataType::Node:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> nodes = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshNode);
			for (auto node : nodes)
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(node.data_id.element_id);
				for (auto partId : partIds)
				{
					if (seen.insert(partId).second)  // 如果没见过
					{
						parts.push_back(partId);
					}
				}
			}

			break;
		}
		case PickDataType::ElementEdge:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eleLines = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshLine);

			for (auto eleLine : eleLines)
			{
				QVector<int> partIds = meshData->getPartIdsByNodeid(eleLine.data_id.element_id);
				for (auto partId : partIds)
				{
					if (seen.insert(partId).second)  // 如果没见过
					{
						parts.push_back(partId);
					}
				}
			}

			break;
		}
		case PickDataType::ElementFace:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eleFaces = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshFace);
			for (auto eleFace : eleFaces)
			{
				if (seen.insert(eleFace.part_id).second)  // 如果没见过
				{
					parts.push_back(eleFace.part_id);
				}
			}

			break;
		}
		case PickDataType::Element:
		{
			std::unordered_set<int> seen;
			std::vector<MeshSelectInfoData> eles = getCVM->getPreRenderWidget()->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(SelectionInfoType::MeshElement);
			for (auto ele : eles)
			{
				if (seen.insert(ele.part_id).second)  // 如果没见过
				{
					parts.push_back(ele.part_id);
				}
			}

			break;
		}
		default:
			break;
		}


		clearHighlight();

		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(9);
		addPreWidgetSelectPartID(getCVM->getPreRenderWidget(), parts);

		if (isHighlight)
		{
			UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());
		}


		getCVM->getPreRenderWidget()->updatePaint();
	}


	void mSelectMenuFunction::slot_sameLocationGeoSelect(int xyz)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			if (res.size() != 1)
			{
				return;
			}
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(3);
			std::vector<GeoNeutralData::GeoSolidAttr> vec = getGEOServer.geoCalculate().getSolidAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getSolidAttr(std::vector<int>(allIds.begin(), allIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, res);
			break;
		}
		case PickDataType::GeoFace:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			if (res.size() != 1)
			{
				return;
			}
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(2);
			std::vector<GeoNeutralData::GeoFaceAttr> vec = getGEOServer.geoCalculate().getFaceAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getFaceAttr(std::vector<int>(allIds.begin(), allIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
			break;
		}
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			if (res.size() != 1)
			{
				return;
			}
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(1);
			std::vector<GeoNeutralData::GeoEdgeAttr> vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>(allIds.begin(), allIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
			break;
		}
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			if (res.size() != 1)
			{
				return;
			}
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(0);
			std::vector<GeoNeutralData::GeoVertexAttr> vec = getGEOServer.geoCalculate().getVertexAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getVertexAttr(std::vector<int>(allIds.begin(), allIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, res);
			break;
		}
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_sameLocationFaceGeoSelect(int xyz)
	{
		if (!getCVM)
		{
			return;
		}
		auto meshData = getCVM->getModelData()->getMeshData();
		int pickNum = 0;
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			break;
		}
		case PickDataType::GeoFace:
		{
			break;
		}
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			auto faceId = getShapeRelated(res, PickDataType::GeoEdge, 2);
			auto edgeIds = getShapeRelated(faceId, PickDataType::GeoFace, 1);
			if (res.size() != 1)
			{
				return;
			}
			std::vector<GeoNeutralData::GeoEdgeAttr> vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>(edgeIds.begin(), edgeIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
			break;
		}
		case PickDataType::GeoPoint:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoPoint);
			auto faceId = getShapeRelated(res, PickDataType::GeoPoint, 2);
			auto pointIds = getShapeRelated(faceId, PickDataType::GeoFace, 0);
			if (res.size() != 1)
			{
				return;
			}
			std::vector<GeoNeutralData::GeoVertexAttr> vec = getGEOServer.geoCalculate().getVertexAttr(std::vector<int>{res.at(0)});
			if (vec.size() != 1)
			{
				return;
			}
			auto basePos = vec.at(0).m_coor;
			vec = getGEOServer.geoCalculate().getVertexAttr(std::vector<int>(pointIds.begin(), pointIds.end()));
			res.clear();
			for (auto v : vec)
			{
				if (fabs(basePos[xyz] - v.m_coor[xyz]) < ep)
				{
					res.push_back(v.id);
				}
			}
			clearHighlight();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoPoint, res);
			break;
		}
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_sizeGeoSelect(int compare)
	{
		std::function<bool(double, double)> Compare;  // 可存放任意比较器

		if (compare == 0) {
			Compare = std::greater<double>();
		}
		else if (compare == 1) {
			Compare = [=](double a, double b)
				{
					return std::fabs(a - b) < b * 0.001;//保留百分之1的误差
				};
		}
		else if (compare == 2) {
			Compare = std::less<double>();
		}

		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		//【镜像六件套摘除】旧 auto geoModelData（渲染镜像）已删，掩码判断不再需要
		int pickNum = 0;
		auto meshData = getCVM->getModelData()->getMeshData();
		//根据当前的拾取模式来判断高亮模式，然后执行反向
		PickDataType pickDataType = getCVM->getCurrentPickDataType(pickNum);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto geoManager = getCVM->getModelData()->getGeoManager();
		if (pickNum < 1)
		{
			return;
		}
		switch (pickDataType)
		{
		case PickDataType::GeoSolid:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoSolid);
			if (res.size() < 1)
			{
				return;
			}
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(3);
			auto vec = getGEOServer.geoCalculate().getSolidAttr(std::vector<int>(allIds.begin(), allIds.end()));
			auto basevecs = getGEOServer.geoCalculate().getSolidAttr(res);
			std::vector<double> baseSizes;
			res.clear();
			for (int i = 0; i < basevecs.size(); i++)
			{
				auto basePos = basevecs.at(i).m_volume;
				bool alreadyProcessed = false;
				for (double b : baseSizes) {
					if (std::fabs(basePos - b) < b * 0.001) {
						alreadyProcessed = true;
						break;
					}
				}
				if (alreadyProcessed) {
					continue;
				}
				baseSizes.push_back(basePos);
				for (auto v : vec)
				{
					if (Compare(v.m_volume, basePos))
					{
						if (!getGeoIsMask(v.id, 3, meshData, geoDatabase))
						{

							res.push_back(v.id);

						}
					}
				}
			}

			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, res);
			break;
		}
		case PickDataType::GeoFace:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoFace);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(2);
			auto basevecs = getGEOServer.geoCalculate().getFaceAttr(res);
			auto vec = getGEOServer.geoCalculate().getFaceAttr(std::vector<int>(allIds.begin(), allIds.end()));
			std::vector<double> baseSizes;
			res.clear();
			for (int i = 0; i < basevecs.size(); i++)
			{
				auto basePos = basevecs.at(i).m_area;
				bool alreadyProcessed = false;
				for (double b : baseSizes) {
					if (std::fabs(basePos - b) < b * 0.001) {
						alreadyProcessed = true;
						break;
					}
				}
				if (alreadyProcessed) {
					continue;
				}
				baseSizes.push_back(basePos);
				for (auto v : vec)
				{
					if (Compare(v.m_area, basePos))
					{
						if (!getGeoIsMask(v.id, 2, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
			break;
		}
		case PickDataType::GeoEdge:
		{
			auto res = getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(SelectionInfoType::GeoEdge);
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(1);
			auto basevecs = getGEOServer.geoCalculate().getEdgeAttr(res);
			auto vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>(allIds.begin(), allIds.end()));
			std::vector<double> baseSizes;
			res.clear();
			for (int i = 0; i < basevecs.size(); i++)
			{
				auto basePos = basevecs.at(i).m_length;
				bool alreadyProcessed = false;
				for (double b : baseSizes) {
					if (std::fabs(basePos - b) < b * 0.001) {
						alreadyProcessed = true;
						break;
					}
				}
				if (alreadyProcessed) {
					continue;
				}
				baseSizes.push_back(basePos);
				for (auto v : vec)
				{
					if (Compare(v.m_length, basePos))
					{
						if (!getGeoIsMask(v.id, 1, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
			break;
		}
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();
	}

	void mSelectMenuFunction::slot_sizeGeoSelect(QString geoType, int compare, double size1, double size2)
	{
		std::function<bool(double, double)> Compare;  // 可存放任意比较器

		if (compare == 0) {
			Compare = std::greater<double>();
		}
		else if (compare == 1) {
			Compare = [=](double a, double b)
				{
					return std::fabs(a - b) < b * 0.001;//保留百分之1的误差
				};
		}
		else if (compare == 2) {
			Compare = std::less<double>();
		}

		if (!getCVM)
		{
			return;
		}
		if (!getCVM->getPreRenderWidget())
		{
			return;
		}
		//【镜像六件套摘除】旧 auto geoModelData（渲染镜像）已删，掩码判断不再需要
		auto meshData = getCVM->getModelData()->getMeshData();
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		auto geoManager = getCVM->getModelData()->getGeoManager();

		if (geoType == "几何体")
		{
			vector<int> res;
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(3);
			std::vector<GeoNeutralData::GeoSolidAttr> vec = getGEOServer.geoCalculate().getSolidAttr(std::vector<int>(allIds.begin(), allIds.end()));
			if (compare != 3)
			{
				for (auto v : vec)
				{
					if (Compare(v.m_volume, size2))
					{
						if (!getGeoIsMask(v.id, 3, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			else
			{
				for (auto v : vec)
				{
					if ((v.m_volume <= size2) && (v.m_volume >= size1))
					{
						if (!getGeoIsMask(v.id, 3, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(4);
			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoSolid, res);
		}
		else if (geoType == "几何面")
		{
			vector<int> res;
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(2);
			auto vec = getGEOServer.geoCalculate().getFaceAttr(std::vector<int>(allIds.begin(), allIds.end()));
			if (compare != 3)
			{
				for (auto v : vec)
				{
					if (Compare(v.m_area, size2))
					{
						if (!getGeoIsMask(v.id, 2, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			else
			{
				for (auto v : vec)
				{
					if ((v.m_area <= size2) && (v.m_area >= size1))
					{
						if (!getGeoIsMask(v.id, 2, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(3);
			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoFace, res);
		}
		else if (geoType == "几何线")
		{
			vector<int> res;
			auto allIds = getGEOServer.geoCalculate().getAllGeoID(1);
			std::vector<GeoNeutralData::GeoEdgeAttr> vec = getGEOServer.geoCalculate().getEdgeAttr(std::vector<int>(allIds.begin(), allIds.end()));
			if (compare != 3)
			{
				for (auto v : vec)
				{
					if (Compare(v.m_length, size2))
					{
						if (!getGeoIsMask(v.id, 1, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			else
			{
				for (auto v : vec)
				{
					if ((v.m_length <= size2) && (v.m_length >= size1))
					{
						if (!getGeoIsMask(v.id, 1, meshData, geoDatabase))
						{

							res.push_back(v.id);
						}
					}
				}
			}
			MechCommon::mMechGlobalSignals::getInstance()->activePickSig(2);
			getCVM->clearPick();
			getCVM->getPreRenderWidget()->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::ADD, SelectionInfoType::GeoEdge, res);
		}

		MechPreData::UpdatePreWidgetHighlight(getCVM->getPreRenderWidget());

		getCVM->getPreRenderWidget()->updatePaint();


	}

	void mSelectMenuFunction::slot_subWindowActivated(int viewportID)
    {
        Q_UNUSED(viewportID);
		if (conn_geo)
			disconnect(conn_geo);
		if (conn_mesh)
			disconnect(conn_mesh);
		//拾取辅助联动：conn_geo 由 ensurePickAssistConnection 统一管理，随视口切换重连
		ensurePickAssistConnection();
	}

	std::vector<int> mSelectMenuFunction::getShapeRelated(std::vector<int> ids, PickDataType pickDataType, int dim)
	{
		int dimResource = int(pickDataType);
		auto geoDatabase = getCVM->getModelData()->getGeoData();
		std::set<int> seen;
		for (auto id : ids)
		{
			auto res = getGEOServer.geoCalculate().getShapeRelated(id, dimResource);
			std::vector<int> shapeIds;
			if (dim == 0)
			{
				shapeIds = res.Vertices;
			}
			else if (dim == 1)
			{
				shapeIds = res.Edges;
			}
			else if (dim == 2)
			{
				shapeIds = res.Faces;
			}
			else if (dim == 3)
			{
				shapeIds = res.Solids;
			}
			for (auto sid : shapeIds)
			{
				seen.insert(sid);
			}
		}
		return std::vector<int>(seen.begin(), seen.end());
	}

	bool mSelectMenuFunction::getGeoIsMask(int id, int dim, mMeshData* meshData, MGeoData::GeoDataBase* geoDatabase)
	{
        //【空窗收口·G2】老链（getPartSetByGeo*ID→set->getPartID→getGeoPartMask）随 S9 冻结岛空壳化，
        // 改 GN 反查部件 ID；掩码数据源不变（mMeshData::getGeoPartMask），未命中部件不判掩码（同老 null 分支）
        //【镜像六件套摘除】死参 geoModelData（渲染镜像）已删；geoDatabase 亦为死参，仅保留指针形态
        Q_UNUSED(geoDatabase);
        if (meshData == nullptr || dim < 0 || dim > 3)
        {
            return false;
        }
        int partId = MGeoNeutralQuery::partIdByGeoId(
            (getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getGeoManager() : nullptr, dim, id);
        if (partId < 0)
        {
            return false;
        }
        return meshData->getGeoPartMask(partId);
	}

	void mSelectMenuFunction::slot_changeLanguage(int language)
	{
		//if (language == 0)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("选择信息"));
		//}
		//else if (language == 1)
		//{
		//	//设置Dock名称
		//	this->setDockWindowTitle(QString("SelectInfo"));
		//}
	}

	mSelectMenuFunction::~mSelectMenuFunction()
	{

	}

	mInputIDLineEdit::mInputIDLineEdit()
	{
		this->setValidator(new IDValidator(this));
		//this->setToolTip("1 或者 3-6 或者 4, 5-10, 60:100:2");
		//this->installEventFilter(new TooltipInstaller("输入ID",
		//QPixmap(""),
		//	"1 或者 3-6 或者 4, 5-10, 60:100:2",
		//	200, this));
		//this->setPlaceholderText("1 或者 3-6 或者 4, 5-10, 60:100:2");

		setProperty("ttTitle", "输入ID");
		setProperty("ttBody", "1 或者 3:6 或者 4, 5:10, 60:100:2");
		InputIDLineEditTipInstaller* installer = new InputIDLineEditTipInstaller(this);
		// 输入完成时解析
		connect(this, &QLineEdit::editingFinished, this, [this]() {
			slot_parseIdString(this->text());
		});

		// 实时输入时检查有效性并高亮颜色
		connect(this, &QLineEdit::textChanged, this, &mInputIDLineEdit::slot_textChanged);
	}

	mInputIDLineEdit::~mInputIDLineEdit()
	{

	}

	std::vector<int> mInputIDLineEdit::getIds()
	{
		return _ids;
	}

	void mInputIDLineEdit::slot_parseIdString(const QString& text)
	{
		QSet<int> idSet; // 去重
		QStringList parts = text.split(",", QString::SkipEmptyParts);

		for (const QString& part : parts) {
			QString trimmed = part.trimmed();

			// 匹配范围（纯 : 范围/步长）
            QRegularExpression reColon(R"((\d+)\s*:\s*(\d+)(?:\s*:\s*(\d+))?)");
			QRegularExpressionMatch match = reColon.match(trimmed);

			if (match.hasMatch()) {
				int start = match.captured(1).toInt();
				int end = match.captured(2).toInt();
				int step = match.captured(3).isEmpty() ? 1 : match.captured(3).toInt();

				// 业务校验：最大值不能小于最小值
				if (start > end)
					continue;

				// 业务校验：步长必须 >= 1 且 <= (end - start)
				if (step < 1 || step > (end - start))
					continue;

				for (int i = start; i <= end; i += step) {
					idSet.insert(i);
				}
			}
			else if (!trimmed.isEmpty()) {
				// 单个数字
				bool ok = false;
				int num = trimmed.toInt(&ok);
				if (ok) {
					idSet.insert(num);
				}
			}
		}

		// 转成有序 QVector
		_ids.clear();
		_ids.reserve(idSet.size());
		for (int v : idSet) {
			_ids.push_back(v);
		}

		std::sort(_ids.begin(), _ids.end());
	}

	void mInputIDLineEdit::setInputState(InputState state)
	{
		QPalette pal = palette();
		pal.setColor(QPalette::Base,
			state == InputState::Invalid ? QColor("#FFB6C1")
			: state == InputState::Incomplete ? QColor("#FFFACD")
			: QColor(Qt::white));
		// 校验只更新调色板，不清空/替换主题 QSS；文本和光标几何不随输入变化。
		setPalette(pal);
		update();
	}

	void mInputIDLineEdit::slot_textChanged(const QString& text)
	{
		if (text.isEmpty()) {
			setInputState(InputState::Normal);
			return;
		}

		IDValidator idValidator;

		QString input = text;
		int pos = cursorPosition();
		QValidator::State state = idValidator.validate(input, pos);

		if (state == QValidator::Acceptable) {
			// 格式完整合法，再做业务校验（最大值不能小于最小值、步长范围等）
			// 业务不合法也标红，但不阻断输入（让用户可以继续修改）
			static const QRegularExpression rangeRe(
				R"(^\s*(\d+)\s*:\s*(\d+)(?:\s*:\s*(\d+))?\s*$)"
			);
			bool businessValid = true;
			for (const QString& part : text.split(',')) {
				QString seg = part.trimmed();
				if (seg.isEmpty())
					continue;

				QRegularExpressionMatch m = rangeRe.match(seg);
				if (m.hasMatch()) {
					int start = m.captured(1).toInt();
					int end = m.captured(2).toInt();
					int step = m.captured(3).isEmpty() ? 1 : m.captured(3).toInt();

					if (start > end) {
						businessValid = false;
						break;
					}
					if (step < 1 || step > (end - start)) {
						businessValid = false;
						break;
					}
				}
			}

			if (businessValid) {
				setInputState(InputState::Normal);
			}
			else {
				setInputState(InputState::Invalid);
			}
		}
		else if (state == QValidator::Invalid) {
			// 非法字符/格式：红色背景
			setInputState(InputState::Invalid);
		}
		else {
			// Intermediate（如 "1:2:" 这种未完成输入）：浅黄色背景提示
			setInputState(InputState::Incomplete);
		}
	}


}
