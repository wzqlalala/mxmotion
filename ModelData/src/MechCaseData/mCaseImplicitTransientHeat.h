#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QStringList>

#include "mCaseBase.h"

namespace MechCaseData
{	
	//瞬态传热工况
	class MXCASEDATA_EXPORT mCaseImplicitTransientHeat : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitTransientHeat(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitTransientHeat() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitTransientHeat();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitTransientHeat* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitTransientHeat& obj);
		friend QDataStream& InStreammCaseImplicitTransientHeatV3(QDataStream& in, mCaseImplicitTransientHeat& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitTransientHeat& obj);

	public:

		/*****************************最新数据********************************/
		//////////////////////////////////////////////接续时间步，开启或者关闭 下拉框
		QString _cntnlsub;

		//////////////////////////////////////////////时间步
		//时间步长类型   分为自动或者固定 下拉框
		QString _timeStepType;

		//初始步长 int
		QString _initialStepValue;

		//总时间 double
		QString _totolTimeValue;

		//最小步长 double(只有时间步长为自动的时候)
		QString _minTimeStep;

		//最大步长 double(只有时间步长为自动的时候)
		QString _maxTimeStep;
	};
}
