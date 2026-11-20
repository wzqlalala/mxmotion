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
	//热-结构耦合分析工况
	class MXCASEDATA_EXPORT mCaseImplicitStructuralCouplingHeat : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitStructuralCouplingHeat(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitStructuralCouplingHeat() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitStructuralCouplingHeat();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitStructuralCouplingHeat* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitStructuralCouplingHeat& obj);
		friend QDataStream& InStreammCaseImplicitStructuralCouplingHeatV3(QDataStream& in, mCaseImplicitStructuralCouplingHeat& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitStructuralCouplingHeat& obj);

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
