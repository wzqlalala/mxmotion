#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mCaseBase.h"

namespace MechCaseData
{
	class MXCASEDATA_EXPORT mCaseImplicitLinearBuckling : public mCaseBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCaseImplicitLinearBuckling(int id, MxFunctions::CaseType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCaseImplicitLinearBuckling() {}

		/*
		* 析构函数
		*/
		~mCaseImplicitLinearBuckling();

		/*
		* 拷贝分析工况
		*/
		mCaseImplicitLinearBuckling* copyCase(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCaseImplicitLinearBuckling& obj);
		friend QDataStream& InStreammCaseImplicitLinearBucklingV3(QDataStream& in, mCaseImplicitLinearBuckling& obj);
		friend QDataStream& operator<<(QDataStream& out, mCaseImplicitLinearBuckling& obj);

	public:

		//最小频率（可以为空，因此使用字符串存储）
		QString _minFrequency;

		//最大频率（可以为空，因此使用字符串存储）
		QString _maxFrequency;

		//阶数（可以为空，因此使用字符串存储）
		QString _order;

		//容差（可以为空，因此使用字符串存储）
		QString _tolerance;
	};
}
