#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXStructOptionData_global.h"
#include <QString>
#include <QDataStream>

namespace MechStructOptionData
{	
	class MXSTRUCTOPTIONDATA_EXPORT mRunnerOptimizationGoalData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mRunnerOptimizationGoalData();

		/*
		* 析构函数
		*/
		~mRunnerOptimizationGoalData();

		friend QDataStream& operator>>(QDataStream& in, mRunnerOptimizationGoalData& mgr);
		friend QDataStream& InStreammRunnerOptimizationGoalDataV3(QDataStream& in, mRunnerOptimizationGoalData& obj);
		friend QDataStream& operator<<(QDataStream& out, mRunnerOptimizationGoalData& mgr);

	public:

		//优化方式(最小化/最大化)，下拉框
		QString _objectiveType;

		//优化响应（平均压降/流量均匀性），下拉框
		QString _responseType;

	};
}
