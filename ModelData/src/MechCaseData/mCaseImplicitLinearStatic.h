#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitLinearStatic : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitLinearStatic(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitLinearStatic() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitLinearStatic();

		/*
		* 拷贝分析工况
		*/
	    mCaseImplicitLinearStatic* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitLinearStatic& obj);
		friend QDataStream& InStreammCaseImplicitLinearStaticV3(QDataStream& in, mCaseImplicitLinearStatic& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitLinearStatic& obj);

	public:
		//方程求解器：直接法求解器DSV/迭代法求解器Pariters 下拉框
		QString _equationSolver;

		//惯性释放： (关闭/手动开启/自动开启) 下拉框
		QString _inertiaRelief;

		////预应力状态  复选框
		//bool _statusPreload;

		//自动单点约束 (关闭/开启) 下拉框
		QString _autospc;
	};
}
