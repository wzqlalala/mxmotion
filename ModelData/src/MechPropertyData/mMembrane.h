#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//膜截面属性
	class MXPROPERTYDATA_EXPORT mMembrane : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mMembrane(int id,MxFunctions::PropertyType type, const QString& name = "" );

		/*
		* 仅用于输入输出流
		*/
		mMembrane() {}

		/*
		* 析构函数
		*/
		~mMembrane();

		/*
		* 拷贝分析工况
		*/
	    mMembrane* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mMembrane& mgr);
		friend QDataStream& InStreammMembraneV3(QDataStream& in, mMembrane& obj);
		friend QDataStream& operator<<(QDataStream& out, mMembrane& mgr);

	public:

	};
}
