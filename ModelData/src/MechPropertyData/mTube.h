#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//管截面属性
	class MXPROPERTYDATA_EXPORT mTube : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mTube(int id,MxFunctions::PropertyType type, const QString& name = "" );

		/*
		* 仅用于输入输出流
		*/
		mTube() {}

		/*
		* 析构函数
		*/
		~mTube();

		/*
		* 拷贝分析工况
		*/
	    mTube* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mTube& mgr);
		friend QDataStream& InStreammTubeV3(QDataStream& in, mTube& obj);
		friend QDataStream& operator<<(QDataStream& out, mTube& mgr);

	public:

		//管外径
		QString _od;

		//管壁厚
		QString _t;

		//单位长度非结构质量
		QString _nsm;

		//第二个网格点处的管子直径
		QString _od2;
	};
}
