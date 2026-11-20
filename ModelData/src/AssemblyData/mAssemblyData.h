#pragma once
// 双 ModelData 树同源共享头：MxAI 工作区同时安装 RebuildModelData(dev_Mech) 与
// RebuildModelData_Dyna(dev_Dyna)，本文件两树逐字节同源；#pragma once 按路径去重
// 无法跨树去重（双份均被包含即 C2011 重定义），补同名 include guard 先含者胜。
#ifndef MASSEMBLYDATA_H
#define MASSEMBLYDATA_H
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXAssemblyData_global.h"
#include <QString>

namespace AssemblyData
{
	/*
	* 装配实体：纯层级组织实体，不持有几何/网格数据。
	* 网格、载荷、属性等仍按部件挂载，装配只表达"装配-子装配/部件"的从属关系；
	* 同一部件/子装配可被多个装配共享引用。
	*/
	struct MXASSEMBLYDATA_EXPORT mAssemblyData
	{
		//装配ID（模型内唯一，递增不复用）
		int _assemblyID = -1;

		//装配名称（模型内唯一，沿用 mModelData 命名机制）
		QString _name;
	};
}
#endif // MASSEMBLYDATA_H
