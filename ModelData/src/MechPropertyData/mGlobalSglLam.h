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
	class MXPROPERTYDATA_EXPORT mGlobalSglLam : public mLaminateBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mGlobalSglLam(int id, MxFunctions::LaminateType type, const QString& name = "");

		/*
		* 仅用于输入输出流
		*/
		mGlobalSglLam() {}

		/*
		* 析构函数
		*/
		~mGlobalSglLam();

		/*
		* 拷贝分析工况
		*/
		mGlobalSglLam* copyLaminate(const int& id, const QString& name) override;

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mGlobalSglLam& mgr);
		friend QDataStream& InStreammGlobalSglLamV3(QDataStream& in, mGlobalSglLam& obj);
		friend QDataStream& operator<<(QDataStream& out, mGlobalSglLam& mgr);

	public:

		//角度
		QString _theta;
		//厚度
		QString _thick;
	};
}
