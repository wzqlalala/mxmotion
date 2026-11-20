#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXPropertyData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QVector>

#include "mLaminateBase.h"

namespace MechPropertyData
{
	class MXPROPERTYDATA_EXPORT mLamPlate : public mLaminateBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mLamPlate(int id, MxFunctions::LaminateType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mLamPlate() {}

		/*
		* 析构函数
		*/
		~mLamPlate();

		/*
		* 拷贝分析工况
		*/
		mLamPlate* copyLaminate(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mLamPlate& mgr);
		friend QDataStream& InStreammLamPlateV3(QDataStream& in, mLamPlate& obj);
		friend QDataStream& operator<<(QDataStream& out, mLamPlate& mgr);

	public:

		//单层板or全局单层板
		int _type;   //0单层板 1全局单层板
	};
}
