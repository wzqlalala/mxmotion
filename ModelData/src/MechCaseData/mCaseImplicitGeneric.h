#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitGeneric : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitGeneric(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitGeneric() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitGeneric();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitGeneric* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitGeneric& obj);
		friend QDataStream& InStreammCaseImplicitGenericV3(QDataStream& in, mCaseImplicitGeneric& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitGeneric& obj);
	};
}
