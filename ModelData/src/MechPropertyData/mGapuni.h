#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>

#include "mPropertyBase.h"

namespace MechPropertyData
{
	//点对点间隙单元截面属性
	class MXPROPERTYDATA_EXPORT mGapuni : public mPropertyBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mGapuni(int id, MxFunctions::PropertyType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mGapuni() {}

		/*
		* 析构函数
		*/
		~mGapuni();

		/*
		* 拷贝分析属性
		*/
		mGapuni* copyProperty(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info) override;

		friend QDataStream& operator>>(QDataStream& in, mGapuni& mgr);
		friend QDataStream& InStreammGapuniV3(QDataStream& in, mGapuni& obj);
		friend QDataStream& operator<<(QDataStream& out, mGapuni& mgr);

	public:

		//间隙距离
		QString _gapDistance;

		//截面积
		QString _area;

		//方向
		QString _direction;
	};
}
