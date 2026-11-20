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
	class MXCASEDATA_EXPORT mCaseImplicitNonLinearStatic : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitNonLinearStatic(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitNonLinearStatic() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitNonLinearStatic();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitNonLinearStatic* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitNonLinearStatic& obj);
		friend QDataStream& InStreammCaseImplicitNonLinearStaticV3(QDataStream& in, mCaseImplicitNonLinearStatic& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitNonLinearStatic& obj);

	public:

		/*****************************最新数据********************************/
		//////////////////////////////////////////////几何非线性，开或者关 下拉框
		QString _nonLinearity;

		//接续时间步，开启或者关闭 下拉框
		QString _cntnlsub;

		//惯性释放： (关闭/手动开启/自动开启) 下拉框
		QString _inertiaRelief;

		//非对称求解：开启或者关闭 下拉框
		QString _Sparsematrix;

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

		//////////////////////////////////////////////粘性
		//粘性状态：bool 复选框（勾选则写*visco关键字，不勾选则写*static关键字）
		bool _visco;

		//蠕变容差
		QString _cetol;
	};
}
