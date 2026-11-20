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
	class MXPROPERTYDATA_EXPORT mSglLayerPlate : public mLaminateBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mSglLayerPlate(int id, MxFunctions::LaminateType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mSglLayerPlate() {}

		/*
		* 析构函数
		*/
		~mSglLayerPlate();

		/*
		* 拷贝分析工况
		*/
		mSglLayerPlate* copyLaminate(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mSglLayerPlate& mgr);
		friend QDataStream& InStreammSglLayerPlateV3(QDataStream& in, mSglLayerPlate& obj);
		friend QDataStream& operator<<(QDataStream& out, mSglLayerPlate& mgr);

	public:

		//角度
		QString _theta;
		//厚度
		QString _thick;
	};
}
