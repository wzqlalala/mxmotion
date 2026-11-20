#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mViewportModelManager.h"

#include <set>

namespace MechMainWindow
{
	/*
	* 多体实体引用映射的调用方便捷封装（读写统一入口）。
	* 底层为 mMapRelationManager 的通用 Motion 引用存储：
	*   源类型(Physical_Motion*) + 源ID + 角色(目标类型, MapRelationType) -> 目标ID集合。
	* 一对一角色用 MapRelation_None；同目标类型多角色（动作体/基本体、I/J端等）
	* 用 MapRelation_Motion* 枚举区分。数据类不再直存任何实体引用。
	*/
	class MMAINWINDOW_EXPORT mMotionRefMap
	{
	public:
		//写一对一引用：清掉该角色旧值后写入（-1 视为清除）
		static void set(MxFunctions::PhysicalFieldType srcType, int srcId,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role, int dstId);

		//写一对多引用：清掉该角色旧值后批量写入（用于运动体-部件集合等）
		static void setSet(MxFunctions::PhysicalFieldType srcType, int srcId,
			MxFunctions::PhysicalFieldType dstType, const std::set<int>& dstIds);

		//读一对一引用（无映射返回-1）
		static int get(MxFunctions::PhysicalFieldType srcType, int srcId,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role = MxFunctions::MapRelation_None);

		//读一对多引用（无映射返回空集）
		static std::set<int> getSet(MxFunctions::PhysicalFieldType srcType, int srcId,
			MxFunctions::PhysicalFieldType dstType, MxFunctions::MapRelationType role = MxFunctions::MapRelation_None);
	};
}
