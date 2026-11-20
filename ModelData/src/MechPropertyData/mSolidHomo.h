#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//实体截面属性
	class MXPROPERTYDATA_EXPORT mSolidHomo : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mSolidHomo(int id ,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mSolidHomo() {}

		/*
		* 析构函数
		*/
		~mSolidHomo();

		/*
		* 拷贝分析工况
		*/
	    mSolidHomo* copyProperty(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mSolidHomo& mgr);
		friend QDataStream& InStreammSolidHomoV3(QDataStream& in, mSolidHomo& obj);
		friend QDataStream& operator<<(QDataStream& out, mSolidHomo& mgr);

	public:

	};
}
