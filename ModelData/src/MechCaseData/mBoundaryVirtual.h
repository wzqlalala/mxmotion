#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
//#include "mBoundaryBase.h"
#include "mLoadBoundaryBase.h"


namespace MechCaseData
{
	class MXCASEDATA_EXPORT mBoundaryVirtual :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryVirtual(int id,MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryVirtual() {}

		/*
		* 析构函数
		*/
		~mBoundaryVirtual();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryVirtual& obj);
		friend QDataStream& InStreammBoundaryVirtualV3(QDataStream& in, mBoundaryVirtual& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryVirtual& obj);
	public:
		bool _U1;
		bool _U2;
		bool _U3;
		bool _UR1;
		bool _UR2;
		bool _UR3;

	};

}
