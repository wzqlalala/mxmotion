#include "mPostSelectMenuFunction.h"
#include "mPostIdLineEdit.h"

//视口管理器/后处理GUI数据/拾取公共函数（PreData，前后处理共用）
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostGUIData.h"
#include "PickCommonFunction.h"

//全局信号
#include "mMechGlobalSignals.h"

//结果数据库（RebuildBasicData）
#include "mPostDataManager.h"
#include "mPostDataBase.h"
#include "mPostModelData.h"
#include "mPhysicalFieldEnum.h"

//渲染 SDK
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/data/SelectDataStruct.h"

#include <QHash>
#include <QSet>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

//与 mPostTreeSetOps.cpp 的引用环境保持一致（MeshSelectInfoData 在 mxr、PickDataType 在 MxFunctions）
using namespace MxFunctions;
using namespace mxr;

namespace PostWindow
{
	mPostSelectMenuFunction* mPostSelectMenuFunction::getInstance()
	{
		static mPostSelectMenuFunction instance;
		return &instance;
	}

	mPostSelectMenuFunction::mPostSelectMenuFunction(QObject* parent) : QObject(parent)
	{
		_nodeLineEdit = new mPostIdLineEdit();
		_eleLineEdit = new mPostIdLineEdit();
	}

	mPostSelectMenuFunction::~mPostSelectMenuFunction()
	{
	}

	bool mPostSelectMenuFunction::getPostModelData(mxr::REBUILD_PostRend*& postRend, PostData::mPostModelData*& modelData)
	{
		postRend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
		if (postRend == nullptr || postRend->getPostRender() == nullptr) return false;

		MechPreData::mPostGUIData* guidata = getCVM->getGUIDataByPostRender(postRend);
		if (guidata == nullptr) return false;

		QPointer<PostData::mPostDataBase> postDB = getCVM->getModelData()
			->getManager<PostData::mPostDataManager>(MxFunctions::Physical_Post)
			.getPostDatabase(postRend->getPostDataName());
		if (postDB == nullptr) return false;

		modelData = postDB->getModelData(guidata->_cloudFrame);
		if (modelData == nullptr) return false;

		return true;
	}

	void mPostSelectMenuFunction::slot_nodeIDSelect()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostModelData(postRend, modelData))
			return;

		//拾取工具栏切换到"拾取节点"模式（模式索引见mCenterMainWindow拾取组：5=节点；
		//前处理同款顺序：先校验视口上下文，再切模式，防空场景误切拾取模式）
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(5);

		auto nodeIds = _nodeLineEdit->getIds();
		if (nodeIds.empty()) return;

		//过滤掉结果数据中不存在的节点
		std::set<int> allNodes = modelData->getAllNodeIDs();
		std::vector<MeshSelectInfoData> nodes;
		nodes.reserve(nodeIds.size());
		for (int node : nodeIds)
		{
			if (allNodes.count(node) == 0)
				continue;
			MeshSelectInfoData n;
			n.data_id.element_id = node;
			nodes.push_back(n);
		}
		if (nodes.empty()) return;

		getCVM->clearPick();
		postRend->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
			mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, nodes);

		MechPreData::UpdatePostWidgetHighlight(postRend);
		postRend->updatePaint();
	}

	void mPostSelectMenuFunction::slot_eleIDSelect()
	{
		mxr::REBUILD_PostRend* postRend = nullptr;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostModelData(postRend, modelData))
			return;

		//拾取工具栏切换到"拾取单元"模式（模式索引见mCenterMainWindow拾取组：8=单元）
		MechCommon::mMechGlobalSignals::getInstance()->activePickSig(8);

		auto eleIds = _eleLineEdit->getIds();
		if (eleIds.empty()) return;

		//经 rid→cid 映射反查单元所属部件，不存在的单元过滤
		//（注意：不能用 getAllMeshIDAndPartID——它把 cell_rid 里的真实ID误当
		// cell_info 紧凑下标用，rid 与 cid 不一致的数据（如 op2 导入）上会越界崩溃）
		std::vector<MeshSelectInfoData> eles;
		eles.reserve(eleIds.size());
		for (int ele : eleIds)
		{
			const int partId = modelData->getPartIDByEleID(ele);
			if (partId < 0)
				continue;
			MeshSelectInfoData n;
			n.data_id.element_id = ele;
			n.part_id = partId;
			eles.push_back(n);
		}
		if (eles.empty()) return;

		getCVM->clearPick();
		postRend->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
			mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshElement, eles);

		MechPreData::UpdatePostWidgetHighlight(postRend);
		postRend->updatePaint();
	}

	void mPostSelectMenuFunction::slot_reverseSelect()
	{
		if (!getCVM)
			return;

		mxr::REBUILD_PostRend* postRend = nullptr;
		PostData::mPostModelData* modelData = nullptr;
		if (!getPostModelData(postRend, modelData))
			return;

		int pickNum = 0;
		PickDataType pickType = getCVM->getPostPickDataType(pickNum);
		if (pickNum == 0) return;

		if (pickType == PickDataType::Element)
		{
			//当前拾取集（部件ID+单元ID二元组）
			std::vector<MeshSelectInfoData> res = postRend->getSelectionContainer()->getMeshSelectData()
				->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
			std::set<std::pair<int, int>> picked;
			for (const auto& r : res)
				picked.insert({ r.part_id, r.data_id.element_id });

			//补集（可见单元全集减当前拾取）。全集逐部件按紧凑下标 cid 遍历，
			//cell_rid[cid] 取真实ID（同 getAllMeshIDs 的正确索引方式；
			// getAllMeshIDAndPartID 把 rid 误当 cid 用会越界，勿用）
			std::vector<MeshSelectInfoData> eles;
			for (int partId : modelData->getAllPartIDs())
			{
				QPointer<PostData::mPostPartData> partData = modelData->getPartData(partId);
				if (partData == nullptr || !partData->getPartVisual())
					continue;

				const auto& ped = partData->_parteledata;
				const int cellCount = (int)std::min(ped.cell_rid.size(), ped.cell_info.size());
				eles.reserve(eles.size() + cellCount);
				for (int cid = 0; cid < cellCount; ++cid)
				{
					if (!ped.cell_info[cid].is_show)
						continue;

					MeshSelectInfoData d;
					d.part_id = partId;
					d.data_id.element_id = ped.cell_rid[cid];
					if (picked.count({ d.part_id, d.data_id.element_id }) > 0)
						continue;
					eles.push_back(d);
				}
			}

			getCVM->clearPick();
			postRend->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
				mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshElement, eles);
		}
		else if (pickType == PickDataType::Node)
		{
			std::vector<MeshSelectInfoData> res = postRend->getSelectionContainer()->getMeshSelectData()
				->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
			std::set<int> picked;
			for (const auto& r : res)
				picked.insert(r.data_id.element_id);

			std::vector<MeshSelectInfoData> nodes;
			const std::set<int> allNodes = modelData->getAllNodeIDs();
			nodes.reserve(allNodes.size());
			for (int nodeId : allNodes)
			{
				if (picked.count(nodeId) > 0)
					continue;
				MeshSelectInfoData d;
				d.data_id.element_id = nodeId;
				nodes.push_back(d);
			}

			getCVM->clearPick();
			postRend->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(
				mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshNode, nodes);
		}
		else
		{
			return;
		}

		MechPreData::UpdatePostWidgetHighlight(postRend);
		postRend->updatePaint();
	}

	void mPostSelectMenuFunction::slot_clearPick()
	{
		if (!getCVM)
		{
			return;
		}
		//clearPick 内部完成拾取数据清空+高亮刷新+ReadyToReadContainer广播+重绘
		getCVM->clearPick();
	}
}
