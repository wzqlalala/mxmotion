#include "mMotionRefMap.h"
#include "mViewportModel.h"

namespace MechMainWindow
{
	void mMotionRefMap::set(MxFunctions::PhysicalFieldType srcType, int srcId,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstId)
	{
		if (!getCVM || !getCVM->getModelData())
			return;
		auto& mgr = getCVM->getModelData()->getMapRelationManager();
		//清旧：按角色清空后按需写入（一对一语义）
		mgr.clearMotionRole(srcType, srcId, dstType, role);
		if (dstId >= 0)
			mgr.appendMapRelation({ srcType, srcId }, { dstType, dstId }, role);
	}

	void mMotionRefMap::setSet(MxFunctions::PhysicalFieldType srcType, int srcId,
		MxFunctions::PhysicalFieldType dstType, const std::set<int>& dstIds)
	{
		if (!getCVM || !getCVM->getModelData())
			return;
		auto& mgr = getCVM->getModelData()->getMapRelationManager();
		mgr.clearMotionRole(srcType, srcId, dstType, MxFunctions::MapRelation_None);
		for (int id : dstIds)
			mgr.appendMapRelation({ srcType, srcId }, { dstType, id }, MxFunctions::MapRelation_None);
	}

	int mMotionRefMap::get(MxFunctions::PhysicalFieldType srcType, int srcId,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role)
	{
		if (!getCVM || !getCVM->getModelData())
			return -1;
		auto& mgr = getCVM->getModelData()->getMapRelationManager();
		return mgr.getMapRelation<int>({ srcType, srcId }, dstType, role);
	}

	std::set<int> mMotionRefMap::getSet(MxFunctions::PhysicalFieldType srcType, int srcId,
		MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role)
	{
		if (!getCVM || !getCVM->getModelData())
			return {};
		auto& mgr = getCVM->getModelData()->getMapRelationManager();
		return mgr.getMapRelation<std::set<int>>({ srcType, srcId }, dstType, role);
	}
}
