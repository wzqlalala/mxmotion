#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//阻尼截面属性
	class MXPROPERTYDATA_EXPORT mViscousdampsection : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mViscousdampsection(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mViscousdampsection() {}

		/*
		* 析构函数
		*/
		~mViscousdampsection();

		/*
		* 拷贝分析工况
		*/
		mViscousdampsection* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mViscousdampsection& mgr);
		friend QDataStream& InStreammViscousdampsectionV3(QDataStream& in, mViscousdampsection& obj);
		friend QDataStream& operator<<(QDataStream& out, mViscousdampsection& mgr);

	public:

		//阻尼系数
		QString _GE;
	};
}
