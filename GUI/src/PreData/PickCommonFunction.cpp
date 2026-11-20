#include "PickCommonFunction.h"
#include "mxr/view/SelectionCaculator.h"
#include "mxr/view/PostSelectionCalculator.h"
#include "mxr/view/SymbolSelectionCalculator.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mxr/data/IPostRenderDataProxy.h"
#include "mxr/view/HighlightController.h"
//【镜像六件套摘除】mGeoModelData1.h/mGeoLineData1.h 已随 RebuildRender 删除（本文件零使用）
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/REBUILD_PostOneFrameRender.h"

#include "mModelData.h"
#include "mMeshData.h"
#include "mGeoDataBase.h"
#include <mGeoPartManager.h> //【空窗收口·G1/G4】GN 部件注册表与离散数据查询（零 OCC 头链）

namespace
{
	//【空窗收口·G4】虚拓扑相连面闭包扩展——老 GeoDataBase::expandVirtualTopologyFaces 空壳的 GN 等价：
	// 与种子面同部件、且经 edgeTopoTypeMap==2（虚拓扑边）的边连通的面全部并入返回集；
	// 邻接关系由 GN_Part::getEdgeFaceMap（E→F 祖先表）即时推导，部件内 BFS（同老递归的部件内闭环语义）
	std::set<int> expandVirtualTopologyFacesGN(GeoNeutralData::mGeoPartManager* partMgr,
		const std::set<int>& seedFaceIds)
	{
		std::set<int> expanded = seedFaceIds;
		if (partMgr == nullptr || seedFaceIds.empty())
		{
			return expanded;
		}
		for (int partId : partMgr->getPartList())
		{
			const GeoNeutralData::GN_Part part = partMgr->getPartById(partId);
			const GeoNeutralData::GN_TessellationMesh& disc = part.getGeoDiscreteData();
			// 种子面不属于本部件则跳过
			std::set<int> seeds;
			for (int f : seedFaceIds)
			{
				if (disc.facePoints.count(f) > 0)
				{
					seeds.insert(f);
				}
			}
			if (seeds.empty())
			{
				continue;
			}
			// 虚拓扑边（type==2）经 E→F 祖先表建"面-面相邻"关系
			const auto& edgeToFaces = part.getEdgeFaceMap();
			std::map<int, std::vector<int>> neighbors;
			for (const auto& kv : disc.edgeTopoTypeMap)
			{
				if (kv.second != 2)
				{
					continue;
				}
				auto it = edgeToFaces.find(kv.first);
				if (it == edgeToFaces.end())
				{
					continue;
				}
				for (int f1 : it->second)
				{
					for (int f2 : it->second)
					{
						if (f1 != f2)
						{
							neighbors[f1].push_back(f2);
						}
					}
				}
			}
			if (neighbors.empty())
			{
				continue;
			}
			// 自种子面 BFS 闭包
			std::set<int> visited(seeds);
			std::vector<int> queue(seeds.begin(), seeds.end());
			while (!queue.empty())
			{
				const int faceId = queue.back();
				queue.pop_back();
				auto it = neighbors.find(faceId);
				if (it == neighbors.end())
				{
					continue;
				}
				for (int nb : it->second)
				{
					if (visited.insert(nb).second)
					{
						queue.push_back(nb);
					}
				}
			}
			expanded.insert(visited.begin(), visited.end());
		}
		return expanded;
	}
}

namespace MechPreData
{
	void ProcessPickCaculateResult(mxr::FinishPickInfo info, mxr::mBaseRend* _widget_ptr, MxFunctions::ViewportType type)
	{
		if (nullptr == _widget_ptr)
			return;

		//如果是临时高亮则不需要向Container里存储数据,直接高亮即可
		if (info.pick_operate == PickOperate::PreView)
		{
			ProcessPickCaculateResult_PreView(info, _widget_ptr, type);
		}
		else if(info.pick_operate == PickOperate::SoloPick || info.pick_operate == PickOperate::MultiPick)
		{
			ProcessPickCaculateResult_Normal(info, _widget_ptr, type);
		}
		else if (((uint64_t)_widget_ptr->getPickFilter() & (uint64_t)PickFilter::PickSymbol) == (uint64_t)PickFilter::PickSymbol)
		{
			ProcessPickCaculateResult_Normal(info, _widget_ptr, type);
		}
	}

	void ProcessPickCaculateResult_Normal(mxr::FinishPickInfo info, mxr::mBaseRend* _widget_ptr, MxFunctions::ViewportType type)
	{
        if (type == Viewport3D && ((uint64_t)_widget_ptr->getPickFilter() & (uint64_t)PickFilter::PickSymbol) != (uint64_t)PickFilter::PickSymbol)
		{
			CCSMRendWidget* widget_ptr = dynamic_cast<CCSMRendWidget*>(_widget_ptr);
			mxr::PickFuntion pickFunction = info.pick_function;

			bool selectionDataChange = false;

			//判断更新数据是增加还是减少
			SelectionOperate operate = SelectionOperate::NONE;
			if (pickFunction == PickFuntion::AddPick) {
				operate = SelectionOperate::ADD;
			}
			else {
				operate = SelectionOperate::REDUCE;
			}

			auto pickcaclulator = widget_ptr->getSelectionCaculator();
			auto geoMap = pickcaclulator->getGeoSelectInfo();
			auto meshMap = pickcaclulator->getMeshSelectComputeData();
			auto container = widget_ptr->getSelectionContainer();
			auto highlight_controller = widget_ptr->getHighlightController();

			SelectionInfoType min_type = SelectionInfoType::GeoTypeEnd;
			std::pair<int, float> part_depth = { -1,FLT_MAX };

			//单选需要判断深度那个是最小的(深度都是从小到大已经排好顺序的)
			if (info.pick_operate == PickOperate::SoloPick)
			{
				if (!geoMap.empty())
				{
					std::vector<int> final_ids = {};
					std::vector<mxr::GeoSelectInfoData> final_data = {};
					SelectionInfoType final_type = SelectionInfoType::GeoTypeEnd;
					float depth_min = std::numeric_limits<float>::max();

					for (SelectionInfoType enumType : {
						SelectionInfoType::GeoPoint,
							SelectionInfoType::GeoEdge,
							SelectionInfoType::GeoFace,
							SelectionInfoType::GeoSolid,
							SelectionInfoType::GeoPart
					})
					{
						if (geoMap.find(enumType) != geoMap.end())
						{
							auto temp_info_array = geoMap[enumType];
							if (temp_info_array.empty())
								continue;
							auto temp_info = temp_info_array[0];
							if (depth_min > temp_info.intersection_depth)
							{
								depth_min = temp_info.intersection_depth;
								final_ids.clear();

								// 判断拾取模式
								final_ids.push_back(temp_info.id);
								final_data.push_back(temp_info);
								final_type = enumType;
								// 保存部件id与深度，用于判断和网格的遮挡关系
								if (enumType == SelectionInfoType::GeoPart)
								{
									min_type = SelectionInfoType::GeoPart;
									part_depth = { temp_info.id, depth_min };
								}
							}
						}
						selectionDataChange = true;
					}
					if (pickcaclulator)
					{
						if (!geoMap.empty())
						{
							bool isEmpty = true;
							SelectionInfoType type = SelectionInfoType::GeoTypeEnd;
							auto old_selectdata_map = widget_ptr->getSelectionContainer()->getGeoSelectData()->getSelectDataMap();
							for (auto old_geo_pair : old_selectdata_map)
							{
								if (old_geo_pair.second.size() > 0 && final_type == old_geo_pair.first)
								{
									isEmpty = false;
									type = old_geo_pair.first;
									break;
								}
							}

                            //如果是面，检查是否有虚拓扑边
                            std::set<int> expandedFaceIds;
                            if (final_type == SelectionInfoType::GeoFace)
                            {
                                //【空窗收口·G4】老 set 反查/递归扩展均空壳，改 GN 离散表闭包扩展
                                const std::set<int> seedFaceIds(final_ids.begin(), final_ids.end());
                                expandedFaceIds = expandVirtualTopologyFacesGN(
                                    widget_ptr->getModelData().getGeoPartMgr(), seedFaceIds);

                                // 将递归扩展得到的面同步更新到选中数据
                                final_ids.insert(final_ids.begin(),expandedFaceIds.begin(), expandedFaceIds.end());
                            }


							if (isEmpty)
							{
								if (!final_ids.empty())
								{
									if (final_type != SelectionInfoType::GeoPart)
									{
										container->getGeoSelectData()->UpdatePickData(operate, final_type, final_ids);
										container->getGeoSelectData()->UpdateMorePickData(operate, final_type, final_data);
									}
								}
							}
							else
							{
								std::unordered_map<mxr::SelectionInfoType, std::vector<int>> geo_id;
								for (auto geo_pair : geoMap)
								{
									if (!geo_pair.second.empty())
									{
										geo_id[geo_pair.first].push_back(geo_pair.second[0].id);
									}
								}

								if (type == final_type)
								{
                                    if (final_type == SelectionInfoType::GeoFace)
                                    {
                                        for(const auto id : expandedFaceIds)
                                            geo_id[SelectionInfoType::GeoFace].push_back(id);
                                    }

									if (geo_id.find(type) != geo_id.end())
									{
										if (final_type != SelectionInfoType::GeoPart)
										{
											container->getGeoSelectData()->UpdatePickData(operate, type, geo_id[type]);
											container->getGeoSelectData()->UpdateMorePickData(operate, final_type, final_data);
										}
									}
								}
							}
						}
					}
				}

				if (!meshMap.empty())
				{
					std::vector<mxr::MeshSelectInfoData> final_ids;
					SelectionInfoType final_type = SelectionInfoType::MeshTypeEnd;
					float depth_min = std::numeric_limits<float>::max();

					// 网格剔除深度
					for (SelectionInfoType enumType : {
						SelectionInfoType::MeshNode,
							SelectionInfoType::MeshLine,
							SelectionInfoType::MeshFace,
							SelectionInfoType::MeshElement,
							SelectionInfoType::MeshPart,
					})
					{
						if (meshMap.find(enumType) != meshMap.end())
						{
							auto temp_info_array = meshMap[enumType];
							if (temp_info_array.empty())
								continue;
							auto temp_info = temp_info_array[0];
							if (depth_min > temp_info.depth)
							{
								depth_min = temp_info.depth;
								final_ids.clear();
								final_ids.push_back(temp_info);
								final_type = enumType;
								// 保存部件id与深度，用于判断和网格的遮挡关系
								if (enumType == SelectionInfoType::MeshPart)
								{
									if (part_depth.first > 0)
									{
										min_type = SelectionInfoType::MeshPart;
										part_depth = { temp_info.part_id , depth_min };
									}
									else if (part_depth.second > depth_min)
									{
										// 网格在几何前面
										min_type = SelectionInfoType::MeshPart;
										part_depth = { temp_info.part_id , depth_min };
									}
								}
							}
						}
					}
					if (!final_ids.empty())
					{
						if (final_type != SelectionInfoType::MeshPart)
						{
							container->getMeshSelectData()->UpdateSelectDataByType(operate, final_type, final_ids);
							selectionDataChange = true;
						}
					}
				}

				if (part_depth.first >= 0)
				{
					if (min_type == SelectionInfoType::GeoPart)
					{
						container->getGeoSelectData()->UpdatePickData(operate, min_type, { part_depth.first });
						selectionDataChange = true;
					}
					else
					{
						container->getMeshSelectData()->UpdateSelectDataByType(operate, min_type, { { INT_MIN,part_depth.first,INT_MIN,INT_MIN,(double)part_depth.second } });
						selectionDataChange = true;
					}
				}


			}
			else if (info.pick_operate == PickOperate::MultiPick)
			{
				if (!geoMap.empty())
				{
					for (auto geo_pair : geoMap)
					{
						std::vector<int> ids;
						for (auto geoPickInfo : geo_pair.second)
						{
							ids.push_back(geoPickInfo.id);
						}
						container->getGeoSelectData()->UpdatePickData(operate, geo_pair.first, ids);
					}
					selectionDataChange = true;
				}

				if (!meshMap.empty())
				{
					for (auto mesh_pair : meshMap)
					{
						container->getMeshSelectData()->UpdateSelectDataByType(operate, mesh_pair.first, mesh_pair.second);
					}
					selectionDataChange = true;
				}
			}

			//这里直接高亮，如果后续不想要高亮就自己改一下
            if (selectionDataChange)
			UpdatePreWidgetHighlight(widget_ptr);

			// 网格剖分选中数字，左+右-
			int model_selection_id = pickcaclulator->getBiasNumSelectInfo();
			if (model_selection_id >= 0)
			{
				//if (pickFunction == PickFuntion::AddPick) {
				//	emit widget_ptr->getStateMachineManager()->signal_finishPickingBiasNumSig(true, model_selection_id);
				//}
				//else {
				//	emit widget_ptr->getStateMachineManager()->signal_finishPickingBiasNumSig(false, model_selection_id);
				//}
			}

		}
		else if (type == ViewportPost3D)
		{
			REBUILD_PostRend* widget_ptr = dynamic_cast<REBUILD_PostRend*>(_widget_ptr);
			mxr::PickFuntion pickFunction = info.pick_function;

			bool selectionDataChange = false;

			//判断更新数据是增加还是减少
			SelectionOperate operate = SelectionOperate::NONE;
			if (pickFunction == PickFuntion::AddPick) {
				operate = SelectionOperate::ADD;
			}
			else {
				operate = SelectionOperate::REDUCE;
			}

			auto pickcaclulator = widget_ptr->getSelectionCaculator();
			auto meshMap = pickcaclulator->getMeshSelectComputeData();
			auto container = widget_ptr->getSelectionContainer();
			auto highlight_controller = widget_ptr->getHighlightController();

			SelectionInfoType min_type = SelectionInfoType::GeoTypeEnd;
			std::pair<int, float> part_depth = { -1,FLT_MAX };

			//单选需要判断深度那个是最小的(深度都是从小到大已经排好顺序的)
			if (info.pick_operate == PickOperate::SoloPick)
			{
				if (!meshMap.empty())
				{
					std::vector<mxr::MeshSelectInfoData> final_ids;
					SelectionInfoType final_type;
					float depth_min = std::numeric_limits<float>::max();

					// 网格剔除深度
					for (SelectionInfoType enumType : {
						SelectionInfoType::MeshNode,
							SelectionInfoType::MeshLine,
							SelectionInfoType::MeshFace,
							SelectionInfoType::MeshElement,
							SelectionInfoType::MeshPart,
					})
					{
						if (meshMap.find(enumType) != meshMap.end())
						{
							auto temp_info_array = meshMap[enumType];
							if (temp_info_array.empty())
								continue;
							auto temp_info = temp_info_array[0];
							if (depth_min > temp_info.depth)
							{
								depth_min = temp_info.depth;
								final_ids.clear();
								final_ids.push_back(temp_info);
								final_type = enumType;
								// 保存部件id与深度，用于判断和网格的遮挡关系
								if (enumType == SelectionInfoType::MeshPart)
								{
									if (part_depth.first < 0)
									{
										min_type = SelectionInfoType::MeshPart;
										part_depth = { temp_info.part_id , depth_min };
									}
									else if (part_depth.second > depth_min)
									{
										// 网格在几何前面
										min_type = SelectionInfoType::MeshPart;
										part_depth = { temp_info.part_id , depth_min };
									}
								}
							}
						}
					}
					if (!final_ids.empty())
					{
						if (final_type != SelectionInfoType::MeshPart)
						{
							container->getMeshSelectData()->UpdateSelectDataByType(operate, final_type, final_ids);
							selectionDataChange = true;
						}
					}
				}

				if (part_depth.first >= 0)
				{
					if (min_type == SelectionInfoType::GeoPart)
					{
						container->getGeoSelectData()->UpdatePickData(operate, min_type, { part_depth.first });
						selectionDataChange = true;
					}
					else
					{
						container->getMeshSelectData()->UpdateSelectDataByType(operate, min_type, { { INT_MIN,part_depth.first,INT_MIN,INT_MIN,(double)part_depth.second } });
						selectionDataChange = true;
					}
				}


			}
			else if (info.pick_operate == PickOperate::MultiPick)
			{
				if (!meshMap.empty())
				{
					for (auto mesh_pair : meshMap)
					{
						container->getMeshSelectData()->UpdateSelectDataByType(operate, mesh_pair.first, mesh_pair.second);
					}
					selectionDataChange = true;
				}
			}

			//这里直接高亮，如果后续不想要高亮就自己改一下
			//UpdatePostWidgetHighlight 内部已 emit ReadyToReadContainer；这里再发一次会让选择信息表整表刷新两遍
			UpdatePostWidgetHighlight(widget_ptr);
		}

		if (type == Viewport3D && ((uint64_t)_widget_ptr->getPickFilter() & (uint64_t)PickFilter::PickSymbol) == (uint64_t)PickFilter::PickSymbol)
		{
			CCSMRendWidget* widget_ptr = dynamic_cast<CCSMRendWidget*>(_widget_ptr);
			mxr::PickFuntion pickFunction = info.pick_function;

			bool selectionDataChange = false;

			//判断更新数据是增加还是减少
			SelectionOperate operate = SelectionOperate::NONE;
			if (pickFunction == PickFuntion::AddPick) {
				operate = SelectionOperate::ADD;
			}
			else {
				operate = SelectionOperate::REDUCE;
			}

			auto pickcaclulator = widget_ptr->GetSymbolCalculator();
			auto symbolMap = pickcaclulator->getSymbolSelectData();
			auto container = widget_ptr->getSelectionContainer();
			auto highlight_controller = widget_ptr->getHighlightController();

			if (info.pick_operate == PickOperate::SoloPick)
			{
                SymbolSelectInfoData picked; float max = -FLT_MAX;
				std::vector<mxr::SymbolSelectInfoData> final_ids;
				if (!symbolMap[mxr::SymbolSelectionInfoType::Symbol_All].empty())
				{
                    for (const auto&ele : symbolMap)
                    {
                        for (const auto& de : ele.second)
                        {
                            if (max<de.depth)
                            {
                                max = de.depth;
                                picked = de;
                            }
                       }
                    }
                    selectionDataChange = true;
					final_ids.push_back(picked);
				}
				container->getSymbolSelectData()->UpdatePickData(operate, mxr::SymbolSelectionInfoType::Symbol_All, final_ids);
				//这里直接高亮，如果后续不想要高亮就自己改一下
                if (selectionDataChange)
				UpdatePreWidgetHighlight(widget_ptr);
			}
		}
	}

	void ProcessPickCaculateResult_PreView(mxr::FinishPickInfo info, mxr::mBaseRend* _widget_ptr, MxFunctions::ViewportType type)
	{
		if (type == Viewport3D)
		{
			CCSMRendWidget* widget_ptr = dynamic_cast<CCSMRendWidget*>(_widget_ptr);
			mxr::PickFuntion pickFunction = info.pick_function;
			auto pickcaclulator = widget_ptr->getSelectionCaculator();
			auto geoMap = pickcaclulator->getGeoSelectInfo();
			auto highlight_controller = widget_ptr->getHighlightController();
			//不处理网格数据并且临时高亮
			std::shared_ptr<mxr::mGeoSelectData> geodata = std::make_shared<mxr::mGeoSelectData>();
			if (!geoMap.empty())
			{
				std::vector<int> final_ids;
				SelectionInfoType final_type = SelectionInfoType::GeoTypeEnd;
				float depth_min = std::numeric_limits<float>::max();

				for (SelectionInfoType enumType : {
					SelectionInfoType::GeoPoint,
						SelectionInfoType::GeoEdge,
						SelectionInfoType::GeoFace,
						SelectionInfoType::GeoSolid,
						SelectionInfoType::GeoPart
				})
				{
					if (geoMap.find(enumType) != geoMap.end())
					{
						auto temp_info_array = geoMap[enumType];
						if (temp_info_array.empty())
							continue;
						auto temp_info = temp_info_array[0];
						if (depth_min > temp_info.intersection_depth)
						{
							depth_min = temp_info.intersection_depth;
							final_ids.clear();
							final_ids.push_back(temp_info.id);
							final_type = enumType;
						}
					}
				}

				if (pickcaclulator)
				{
					if (!geoMap.empty())
					{
						bool isEmpty = true;
						SelectionInfoType type = SelectionInfoType::GeoTypeEnd;
						auto old_selectdata_map = widget_ptr->getSelectionContainer()->getGeoSelectData()->getSelectDataMap();
						for (auto old_geo_pair : old_selectdata_map)
						{
							if (old_geo_pair.second.size() > 0)
							{
								isEmpty = false;
								type = old_geo_pair.first;
								break;
							}
						}

						if (isEmpty)
						{
							if (!final_ids.empty())
							{
								if (final_type != SelectionInfoType::GeoPart)
								{
									geodata->UpdatePickData(SelectionOperate::ADD, final_type, final_ids);
								}
							}
						}
						else
						{
							std::unordered_map<mxr::SelectionInfoType, std::vector<int>> geo_id;
							for (auto geo_pair : geoMap)
							{
								if (!geo_pair.second.empty())
								{
									geo_id[geo_pair.first].push_back(geo_pair.second[0].id);
								}
							}

							if (type == final_type)
							{
								if (geo_id.find(type) != geo_id.end())
								{
									if (final_type != SelectionInfoType::GeoPart)
									{
										geodata->UpdatePickData(SelectionOperate::ADD, type, geo_id[type]);
									}
								}
							}
						}
					}
				}
			}
			highlight_controller->updateTempHighLight(geodata.get());
		}
		else if (type == ViewportPost3D)
		{

		}
	}

	void UpdatePreWidgetHighlight(mxr::CCSMRendWidget* widget_ptr)
	{
		if (nullptr == widget_ptr)
			return;

		auto highlight_controller = widget_ptr->getHighlightController();
		auto container = widget_ptr->getSelectionContainer();

		highlight_controller->updateHighlightGeoModel(container->getGeoSelectData());
		highlight_controller->updateHighlightMeshData(container->getMeshSelectData());
		//highlight_controller->updateExtraHighLight(container->getGeoSelectData());
		highlight_controller->updateHighlightSymbolData(widget_ptr->GetSymbolController(), container->getSymbolSelectData(), {});

		//发送信号通知界面数据改变
		emit widget_ptr->getSelectionContainer()->ReadyToReadContainer();
	}

    void UpdatePreWidgetHighlightWithoutRTRC(mxr::CCSMRendWidget* widget_ptr)
    {
        if (nullptr == widget_ptr)
            return;

        auto highlight_controller = widget_ptr->getHighlightController();
        auto container = widget_ptr->getSelectionContainer();

        highlight_controller->updateHighlightGeoModel(container->getGeoSelectData());
        highlight_controller->updateHighlightMeshData(container->getMeshSelectData());
        //highlight_controller->updateExtraHighLight(container->getGeoSelectData());
        highlight_controller->updateHighlightSymbolData(widget_ptr->GetSymbolController(), container->getSymbolSelectData(), {});
    }

	void UpdatePostWidgetHighlight(mxr::REBUILD_PostRend* widget_ptr)
	{
		UpdatePostWidgetHighlightWithoutRTRC(widget_ptr);
		if (nullptr == widget_ptr)
			return;

		emit widget_ptr->getSelectionContainer()->ReadyToReadContainer();
	}

	void UpdatePostWidgetHighlightWithoutRTRC(mxr::REBUILD_PostRend* widget_ptr)
	{
		if (nullptr == widget_ptr)
			return;

		auto postRender = widget_ptr->getPostRender();
		//无结果帧时 frameProxy 为空（后处理尚未打开结果），跳过避免空指针解引用
		if (!postRender || !postRender->frameProxy(postRender->currentFrame()))
			return;

		auto highlight_controller = widget_ptr->getHighlightController();
		auto container = widget_ptr->getSelectionContainer();

		highlight_controller->updateHighlightPostMeshData(
			postRender->getHighLightRender(),
			postRender->frameProxy(postRender->currentFrame())->getModelRenderDataProxy(),   // S5：getCurFrameDataProxy→frameProxy(currentFrame())（F8 现取）
			postRender->frameProxy(postRender->currentFrame())->getHighlightDataProxy(),   // S5：getPostFrameDataProxy 同源=帧数据代理，frameProxy 承载
			container->getMeshSelectData(), {});
	}

	void ClearPreWidgetGeoPick(mxr::CCSMRendWidget* widget_ptr)
	{
		widget_ptr->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoPoint, vector<int>{});
		widget_ptr->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoEdge, vector<int>{});
		widget_ptr->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoFace, vector<int>{});
		widget_ptr->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoSolid, vector<int>{});
		widget_ptr->getSelectionContainer()->getGeoSelectData()->UpdatePickData(SelectionOperate::CLEAR, SelectionInfoType::GeoPart, vector<int>{});
	}

	void ClearPreWidgetMeshPick(mxr::CCSMRendWidget* widget_ptr)
	{
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode , {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine , {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace , {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshElement, {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshPart, {});
	}

	void ClearPostWidgetMeshPick(mxr::REBUILD_PostRend* widget_ptr)
	{
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshNode, {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshLine, {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshFace, {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshElement, {});
		widget_ptr->getSelectionContainer()->getMeshSelectData()->UpdateSelectDataByType(SelectionOperate::CLEAR, SelectionInfoType::MeshPart, {});
	}

    void ClearPreWidgetSymbolPick(mxr::CCSMRendWidget* widget_ptr)
    {
        widget_ptr->getSelectionContainer()->getSymbolSelectData()->UpdatePickData(SelectionOperate::CLEAR, SymbolSelectionInfoType::Symbol_All, {});
    }

    void ClearPreWidgetPickHighlightKeepData(mxr::CCSMRendWidget* widget_ptr)
    {
        if (nullptr == widget_ptr)
            return;
        auto highlight_controller = widget_ptr->getHighlightController();
        if (nullptr == highlight_controller)
            return;
        //仅清高亮渲染层（几何+网格），选择容器中的拾取数据保持不变
        highlight_controller->clearHighlightGeoModel();
        highlight_controller->clearHighlightMesh();
        widget_ptr->updatePaint();
    }

    std::vector<int> getPreWidgetSelectPartID(mxr::CCSMRendWidget* w)
    {
		std::set<int> rel;
		auto container = w->getSelectionContainer();
		// 获取网格部件
		const auto& ids_mesh = container->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshPart);
		for (const auto& id : ids_mesh)
			rel.insert(id.part_id);
		// 获取几何部件
		const auto& ids_geo = container->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPart);
		for (const auto& id : ids_geo)
			rel.insert(id);
		return {rel.begin(),rel.end()}; 
    }

    void setPreWidgetSelectPartID(mxr::CCSMRendWidget* w, const std::vector<int>& ids)
    {
		auto container = w->getSelectionContainer();
		const auto& modeldata = w->getModelData();
		std::vector<mxr::MeshSelectInfoData> mesh;
		std::vector<int> geo;
		for (const auto& id : ids)
		{
			auto* meshdata = mxr::refData<mMeshData>(modeldata._meshData);
			if (meshdata->havePart(meshdata->getPartNameByPartId(id)))
			{
				MeshSelectInfoData data;
				data.part_id = id;
				mesh.push_back(data);
			}
			
			//【空窗收口·G1】老 _geoDataBase->getSetByPartId 恒空壳，改判 GN 部件注册表
			if (modeldata.getGeoPartMgr() != nullptr && modeldata.getGeoPartMgr()->containsPart(id))
			{
				geo.push_back(id);
			}
		}

		if (!mesh.empty())
		{
			container->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoPart, {});
			container->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoPart, mesh);
		}

		if (!geo.empty())
		{
			container->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::CLEAR, mxr::SelectionInfoType::GeoPart, {});
			container->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoPart, geo);
		}

    }

	void reducePreWidgetSelectPartID(mxr::CCSMRendWidget* w, const std::vector<int>&ids)
	{
		auto container = w->getSelectionContainer();
		const auto& modeldata = w->getModelData();
		std::vector<mxr::MeshSelectInfoData> mesh;
		std::vector<int> geo;
		for (const auto& id : ids)
		{
			MeshSelectInfoData data;
			data.part_id = id;
			mesh.push_back(data);
			geo.push_back(id);
		}

		if (!mesh.empty())
		{
			container->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::REDUCE, mxr::SelectionInfoType::MeshPart, mesh);
		}

		if (!geo.empty())
		{
			container->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::REDUCE, mxr::SelectionInfoType::GeoPart, geo);
		}
	}

	void addPreWidgetSelectPartID(mxr::CCSMRendWidget* w, const std::vector<int>& ids)
	{
		auto container = w->getSelectionContainer();
		const auto& modeldata = w->getModelData();
		std::vector<mxr::MeshSelectInfoData> mesh;
		std::vector<int> geo;
		for (const auto& id : ids)
		{
			auto* meshdata = mxr::refData<mMeshData>(modeldata._meshData);
			if (meshdata->returnPartType(meshdata->getPartNameByPartId(id)) > 1)
			{
				MeshSelectInfoData data;
				data.part_id = id;
				mesh.push_back(data);
			}

			//【空窗收口·G1】老 _geoDataBase->getSetByPartId 恒空壳，改判 GN 部件注册表
			if (modeldata.getGeoPartMgr() != nullptr && modeldata.getGeoPartMgr()->containsPart(id))
			{
				geo.push_back(id);
			}
		}

		if (!mesh.empty())
		{
			container->getMeshSelectData()->UpdateSelectDataByType(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::MeshPart, mesh);
		}

		if (!geo.empty())
		{
			container->getGeoSelectData()->UpdatePickData(mxr::SelectionOperate::ADD, mxr::SelectionInfoType::GeoPart, geo);
		}
	}

    void clearPreWidgetSelectPartID(mxr::CCSMRendWidget* w)
    {
		auto container = w->getSelectionContainer();
		container->getMeshSelectData()->ClearAll();
		container->getGeoSelectData()->ClearAll();

    }

    void SaveSelectMemoryData(mxr::CCSMRendWidget* rend, MxFunctions::PickDataType pickFilter, std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& dst)
	{
		//存储拾取结果
		std::vector<int>res;
		std::vector<mxr::MeshSelectInfoData> res2;
		switch (pickFilter)
		{
		case MxFunctions::PickDataType::None:
			return;
		case MxFunctions::PickDataType::GeoPoint:
		{
			res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPoint);
		}
		break;
		case MxFunctions::PickDataType::GeoEdge:
		{
			res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
		}
		break;
		case MxFunctions::PickDataType::GeoFace:
		{
			res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
		}
		break;
		case MxFunctions::PickDataType::GeoSolid:
		{
			res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoSolid);
		}
		break;
		case MxFunctions::PickDataType::Node:
		{
			res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
		}
		break;
		case MxFunctions::PickDataType::ElementEdge:
		{
			res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshLine);
		}
		break;
		case MxFunctions::PickDataType::ElementFace:
		{
			res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshFace);
		}
		break;
		case MxFunctions::PickDataType::Element:
		{
			res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
		}
		break;
		case MxFunctions::PickDataType::Part:
		{
			res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);

		}
		break;
		default:
			break;
		}
		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>datas;
		for (auto id : res)
		{
			RenderPickResultData data;
			data.result_id = id;
			datas.push_back({ pickFilter,data });
		}
		for (auto meshelem : res2)
		{
			datas.push_back({ pickFilter, SelectData2ResultData(pickFilter, meshelem) });
		}
		dst = datas;

	}

    void SaveSelectMemoryData(mxr::CCSMRendWidget* rend, std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst)
    {
        //存储拾取结果
        std::vector<int>res;
        std::vector<mxr::MeshSelectInfoData> res2;
        MxFunctions::SetType settype;

        std::vector<RenderPickResultData> datas;
        res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoPoint);
        for (auto id : res)
        {
            RenderPickResultData data;
            data.result_id = id;
            datas.push_back(data);
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::GeoPoint] = datas;

        datas.clear();
        res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoEdge);
        for (auto id : res)
        {
            RenderPickResultData data;
            data.result_id = id;
            datas.push_back(data);
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::GeoEdge] = datas;

        datas.clear();
        res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoFace);
        for (auto id : res)
        {
            RenderPickResultData data;
            data.result_id = id;
            datas.push_back(data);
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::GeoFace] = datas;

        datas.clear();
        res = rend->getSelectionContainer()->getGeoSelectData()->getSelectDataByType(mxr::SelectionInfoType::GeoSolid);
        for (auto id : res)
        {
            RenderPickResultData data;
            data.result_id = id;
            datas.push_back(data);
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::GeoSolid] = datas;

        datas.clear();
        res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshNode);
        for (auto meshelem : res2)
        {
            datas.push_back(SelectData2ResultData(PickDataType::Node, meshelem));
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::Node] = datas;


        datas.clear();
        res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshLine);
        for (auto meshelem : res2)
        {
            datas.push_back(SelectData2ResultData(PickDataType::ElementEdge, meshelem));
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::ElementEdge] = datas;

        datas.clear();
        res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshFace);
        for (auto meshelem : res2)
        {
            datas.push_back(SelectData2ResultData(PickDataType::ElementFace, meshelem));
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::ElementFace] = datas;

        datas.clear();
        res2 = rend->getSelectionContainer()->getMeshSelectData()->getSelectDataByType(mxr::SelectionInfoType::MeshElement);
        for (auto meshelem : res2)
        {
            datas.push_back(SelectData2ResultData(PickDataType::Element, meshelem));
        }
        if (!datas.empty())  dst[MxFunctions::PickDataType::Element] = datas;

    }

	MeshSelectInfoData PickData2SelectData(mxr::SelectionInfoType type, const RenderPickResultData& rel)
	{
		MeshSelectInfoData data;
		data.part_id = rel.part_id;

		switch (type)
		{
		case mxr::SelectionInfoType::MeshNode:
			data.data_id.node_id = rel.result_id;
			break;
		case mxr::SelectionInfoType::MeshLine:
			data.data_id.node_id = rel.result_id;
			data.data_index.line_index = rel.result_id2;
			break;
		case mxr::SelectionInfoType::MeshFace:
			data.data_id.element_id = rel.result_id;
			data.data_index.face_index = rel.result_id2;
			break;
		case mxr::SelectionInfoType::MeshElement:
			data.data_id.element_id = rel.result_id;
			break;
		case mxr::SelectionInfoType::MeshPart:
			data.part_id = rel.result_id;
			break;
		default:
			break;
		}

		return std::move(data);
	}

	MeshSelectInfoData PickData2SelectData(MxFunctions::PickDataType type, const RenderPickResultData& rel)
	{
		MeshSelectInfoData data;
		data.part_id = rel.part_id;

		switch (type)
		{
		case MxFunctions::PickDataType::Node:
			data.data_id.node_id = rel.result_id;
			break;
		case MxFunctions::PickDataType::ElementEdge:
			data.data_id.node_id = rel.result_id;
			data.data_index.line_index = rel.result_id2;
			break;
		case MxFunctions::PickDataType::ElementFace:
			data.data_id.element_id = rel.result_id;
			data.data_index.face_index = rel.result_id2;
			break;
		case MxFunctions::PickDataType::Element:
			data.data_id.element_id = rel.result_id;
			break;
		case MxFunctions::PickDataType::Part:
			data.part_id = rel.result_id;
			break;
		default:
			break;
		}
		
		return std::move(data);
	}

	RenderPickResultData SelectData2ResultData(mxr::SelectionInfoType type, const MeshSelectInfoData& data)
	{
		RenderPickResultData rel;
		rel.part_id = data.part_id;

		switch (type)
		{
		case mxr::SelectionInfoType::MeshNode:
			rel.result_id = data.data_id.node_id;
			break;
		case mxr::SelectionInfoType::MeshLine:
			rel.result_id = data.data_id.node_id;
			rel.result_id2 = data.data_index.line_index;
			break;
		case mxr::SelectionInfoType::MeshFace:
			rel.result_id = data.data_id.element_id;
			rel.result_id2 = data.data_index.face_index;
			break;
		case mxr::SelectionInfoType::MeshElement:
			rel.result_id = data.data_id.element_id;
			break;
		case mxr::SelectionInfoType::MeshPart:
			rel.result_id = data.part_id;
			break;
		default:
			break;
		}

		return std::move(rel);
	}

	RenderPickResultData SelectData2ResultData(MxFunctions::PickDataType type, const MeshSelectInfoData& data)
	{
		RenderPickResultData rel;
		rel.part_id = data.part_id;

		switch (type)
		{
		case MxFunctions::PickDataType::Node:
			rel.result_id = data.data_id.node_id;
			break;
		case MxFunctions::PickDataType::ElementEdge:
			rel.result_id = data.data_id.node_id;
			rel.result_id2 = data.data_index.line_index;
			break;
		case MxFunctions::PickDataType::ElementFace:
			rel.result_id = data.data_id.element_id;
			rel.result_id2 = data.data_index.face_index;
			break;
		case MxFunctions::PickDataType::Element:
			rel.result_id = data.data_id.element_id;
			break;
		case MxFunctions::PickDataType::Part:
			rel.result_id = data.part_id;
			break;
		default:
			break;
		}

		return std::move(rel);
	}

}