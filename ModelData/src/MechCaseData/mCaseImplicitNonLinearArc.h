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
	class MXCASEDATA_EXPORT mCaseImplicitNonLinearArc : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitNonLinearArc(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitNonLinearArc() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitNonLinearArc();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitNonLinearArc* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearArc& obj);
		friend QDataStream& InStreammCaseImplicitNonLinearArcV3(QDataStream& in, mCaseImplicitNonLinearArc& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearArc& obj);

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

		//初始载荷因子 int
		QString _initialStepValue;

		//总载荷 double
		QString _totolTimeValue;

		//最小弧长增量 double(只有时间步长为自动的时候)
		QString _minTimeStep;

		//最大弧长增量 double(只有时间步长为自动的时候)
		QString _maxTimeStep;

		//最大载荷因子
		QString _lMax;

		//最大载荷步数
		QString _nMax;

		//初始缺陷
		bool _initBug = false;
		MxFunctions::InitBugType _initBugType = MxFunctions::InitBugType::Buckling;
		QString _initBugFileName;
		QString _scaleValue;
	};
}
