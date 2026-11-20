#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QStringList>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//变截面属性
	struct CrossBeamData
	{
		QString _location;
		QStringList _params;
	};
	class MXPROPERTYDATA_EXPORT mCrossBeam : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mCrossBeam(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mCrossBeam() {}

		/*
		* 析构函数
		*/
		~mCrossBeam();

		/*
		* 初始化数据
		*/
		void initCrossBeam();

		/*
		* 拷贝分析工况
		*/
	    mCrossBeam* copyProperty(const int& id, const QString& name) override;

		friend QDataStream& operator>>(QDataStream& in, mCrossBeam& mgr);
		friend QDataStream& InStreammCrossBeamV3(QDataStream& in, mCrossBeam& obj);
		friend QDataStream& operator<<(QDataStream& out, mCrossBeam& mgr);

	public:
		//变截面梁位置参数
		QStringList _paraList;
	};
}
