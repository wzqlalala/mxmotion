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
	class MXCASEDATA_EXPORT mBoundaryFixed :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryFixed(int id,MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryFixed() {}

		/*
		* 析构函数
		*/
		~mBoundaryFixed();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryFixed& obj);
		friend QDataStream& InStreammBoundaryFixedV3(QDataStream& in, mBoundaryFixed& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryFixed& obj);
	public:
		//固定类型
		int _fixedType;
	};

}