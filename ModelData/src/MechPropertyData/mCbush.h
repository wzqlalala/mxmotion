#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//六向弹簧阻尼截面属性
	class MXPROPERTYDATA_EXPORT mCbush : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCbush(int id, MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCbush() {}

		/*
		* 析构函数
		*/
		~mCbush();

		/*
		* 拷贝分析工况
		*/
		mCbush* copyProperty(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCbush& mgr);
		friend QDataStream& InStreammCbushV3(QDataStream& in, mCbush& obj);
		friend QDataStream& operator<<(QDataStream& out, mCbush& mgr);

	public:

		//6行6列
		QVector<bool> _isCheck;
		QVector<QVector<double>> _value;
 	};
}
