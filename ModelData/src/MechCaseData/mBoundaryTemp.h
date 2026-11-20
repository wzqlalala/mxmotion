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
	class MXCASEDATA_EXPORT mBoundaryTemp :public mLoadBoundaryBase
	{
	public:
		/*
		* 构造函数
		*/
		mBoundaryTemp(int id ,MxFunctions::LoadBoundaryType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mBoundaryTemp() {}

		/*
		* 析构函数
		*/
		~mBoundaryTemp();

		/*
		* 拷贝
		*/
		std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mBoundaryTemp& obj);
		friend QDataStream& InStreammBoundaryTempV3(QDataStream& in, mBoundaryTemp& obj);
		friend QDataStream& operator<<(QDataStream& out, mBoundaryTemp& obj);
	public:
		double _temp;
		// 分析场方式：true 时 mx 写出 field 关键字，无作用对象/大小/缩放因子
		bool isAnalysisFieldMode_ = false;
	};

}
