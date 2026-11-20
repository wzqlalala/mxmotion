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
	class MXCASEDATA_EXPORT mCaseImplicitNonLinearDynamic : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitNonLinearDynamic(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitNonLinearDynamic() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitNonLinearDynamic();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitNonLinearDynamic* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearDynamic& obj);
		friend QDataStream& InStreammCaseImplicitNonLinearDynamicV3(QDataStream& in, mCaseImplicitNonLinearDynamic& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearDynamic& obj);

	public:

		/*****************************最新数据********************************/
		//////////////////////////////////////////////几何非线性，开或者关 下拉框
		QString _nonLinearity;

		//接续时间步，开启或者关闭 下拉框
		QString _cntnlsub;

		//惯性释放： (关闭/手动开启/自动开启) 下拉框
		QString _inertiaRelief;

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
		//只有时间步长为自动的时候才有上述参数

		//准静态分析，开或者关 下拉框
		QString _quasiStaticAnasys;
	};
}
