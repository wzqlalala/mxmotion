#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//剪切板截面属性
	class MXPROPERTYDATA_EXPORT mShearPlate : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mShearPlate(int id,MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mShearPlate() {}

		/*
		* 析构函数
		*/
		~mShearPlate();

		/*
		* 拷贝分析工况
		*/
	    mShearPlate* copyProperty(const int& id, const QString& name) override;

		/*
		* 设置厚度
		*/
		void setThickness(QString thickness) override;

		/*
		* 获取厚度
		*/
		QString getThickness() override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mShearPlate& mgr);
		friend QDataStream& InStreammShearPlateV3(QDataStream& in, mShearPlate& obj);
		friend QDataStream& operator<<(QDataStream& out, mShearPlate& mgr);
	public:

		//厚度
		QString _thickness;

		//拉伸刚度F1
		QString _valF1;

		//拉伸刚度F2
		QString _valF2;
	};
}
