#pragma once
//【OCC摘除·S9】旧栈 set 反查的 GN 替代查询：按几何 ID/部件名在 GN 部件离散数据
//（GN_TessellationMesh 各表）中检索，零 OCC 内联（旧栈 set 已退役，旧查询恒失败）。
#include "mGeoManager.h"
#include <QString>
#include <map>
#include <set>
#include <vector>

namespace MGeoNeutralQuery
{
	inline GeoNeutralData::mGeoPartManager* partMgr(MGeoManager::GeoManager* mgr)
	{
		return (mgr != nullptr) ? mgr->getGeoPartMgr() : nullptr;
	}

	// 按 (维度, 几何ID) 反查所属部件名；未命中返回空串
	inline QString partNameByGeoId(MGeoManager::GeoManager* mgr, int dim, int geoId)
	{
		GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
		if (pm == nullptr) { return QString(); }
		for (int partId : pm->getPartList())
		{
			const GeoNeutralData::GN_Part part = pm->getPartById(partId);
			const GeoNeutralData::GN_TessellationMesh& disc = part.getGeoDiscreteData();
			bool hit = false;
			if (dim == 2) { hit = disc.facePoints.find(geoId) != disc.facePoints.end(); }
			else if (dim == 3) { hit = disc.solidfaces.find(geoId) != disc.solidfaces.end(); }
			else if (dim == 1) { hit = disc.edgePoints.find(geoId) != disc.edgePoints.end(); }
			else if (dim == 0) { hit = disc.geoPointIndices.find(geoId) != disc.geoPointIndices.end(); }
			if (hit) { return part.metadata().name(); }
		}
		return QString();
	}

	// 按部件名取该部件指定维度的全部几何 ID
	inline std::vector<int> geoIdsByPartName(MGeoManager::GeoManager* mgr, int dim, const QString& partName)
	{
		std::vector<int> out;
		GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
		if (pm == nullptr) { return out; }
		for (int partId : pm->getPartList())
		{
			const GeoNeutralData::GN_Part part = pm->getPartById(partId);
			if (part.metadata().name() != partName) { continue; }
			const GeoNeutralData::GN_TessellationMesh& disc = part.getGeoDiscreteData();
			if (dim == 2) { for (const auto& kv : disc.facePoints) { out.push_back(kv.first); } }
			else if (dim == 3) { for (const auto& kv : disc.solidfaces) { out.push_back(kv.first); } }
			else if (dim == 1) { for (const auto& kv : disc.edgePoints) { out.push_back(kv.first); } }
			else if (dim == 0) { for (const auto& kv : disc.geoPointIndices) { out.push_back(kv.first); } }
		}
		return out;
	}

	// 按 (维度, 几何ID) 反查所属部件 ID；未命中返回 -1
	inline int partIdByGeoId(MGeoManager::GeoManager* mgr, int dim, int geoId)
	{
		GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
		if (pm == nullptr) { return -1; }
		for (int partId : pm->getPartList())
		{
			const GeoNeutralData::GN_Part part = pm->getPartById(partId);
			const GeoNeutralData::GN_TessellationMesh& disc = part.getGeoDiscreteData();
			bool hit = false;
			if (dim == 2) { hit = disc.facePoints.find(geoId) != disc.facePoints.end(); }
			else if (dim == 3) { hit = disc.solidfaces.find(geoId) != disc.solidfaces.end(); }
			else if (dim == 1) { hit = disc.edgePoints.find(geoId) != disc.edgePoints.end(); }
			else if (dim == 0) { hit = disc.geoPointIndices.find(geoId) != disc.geoPointIndices.end(); }
			if (hit) { return partId; }
		}
		return -1;
	}

	// 按部件 ID 取该部件指定维度的全部几何 ID（旧 getAllGeoIDByPart 等价）
	inline std::vector<int> geoIdsByPartId(MGeoManager::GeoManager* mgr, int dim, int partId)
	{
		std::vector<int> out;
		GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
		if (pm == nullptr || !pm->getPartList().contains(partId)) { return out; }
		//【崩溃修复】getPartById 按值返回临时部件，其 getGeoDiscreteData() 引用随临时
		// 析构悬空；改指针取部件（map 节点地址稳定），引用始终有效。
		const GeoNeutralData::GN_Part* part = pm->getPartPtrById(partId);
		if (part == nullptr) { return out; }
		const GeoNeutralData::GN_TessellationMesh& disc = part->getGeoDiscreteData();
		if (dim == 2) { for (const auto& kv : disc.facePoints) { out.push_back(kv.first); } }
		else if (dim == 3) { for (const auto& kv : disc.solidfaces) { out.push_back(kv.first); } }
		else if (dim == 1) { for (const auto& kv : disc.edgePoints) { out.push_back(kv.first); } }
		else if (dim == 0) { for (const auto& kv : disc.geoPointIndices) { out.push_back(kv.first); } }
		return out;
	}

	// 全部部件名（旧 getAllPartNameVec 等价）
	inline std::set<QString> allPartNames(MGeoManager::GeoManager* mgr)
	{
		std::set<QString> out;
		GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
		if (pm == nullptr) { return out; }
		for (int partId : pm->getPartList())
		{
			out.insert(pm->getPartById(partId).metadata().name());
		}
		return out;
	}
}
