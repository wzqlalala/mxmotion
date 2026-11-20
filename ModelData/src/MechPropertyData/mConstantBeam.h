#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//常截面梁属性
	class MXPROPERTYDATA_EXPORT mConstantBeam : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mConstantBeam(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mConstantBeam() {}

		/*
		* 析构函数
		*/
		~mConstantBeam();

		/*
		* 拷贝分析工况
		*/
	    mConstantBeam* copyProperty(const int& id, const QString& name) override;

		/*
		* 设置方向角度
		*/
		void setAngle(QString angle) override;

		/*
		* 获取方向角度
		*/
		QString getAngle() override;

		friend QDataStream& operator>>(QDataStream& in, mConstantBeam& mgr);
		friend QDataStream& InStreammConstantBeamV3(QDataStream& in, mConstantBeam& obj);
		friend QDataStream& operator<<(QDataStream& out, mConstantBeam& mgr);

	public:
		//材料名称
		//QString _matName;

		//部件名称
		//QString _partName;

		//截面形状名称
		//QString _secName;

		//角度
		QString _angle;
	};
}
