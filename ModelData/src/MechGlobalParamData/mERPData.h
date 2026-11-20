#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector3D>
#include "mModelEnum.h"

namespace MechGlobalParamData
{
	class MXGLOBALPARAMDATA_EXPORT mERPData
	{
	public:

		/*
		* 重载构造函数
		*/
		mERPData(int id, QString name = "");

		/*
		*
		*/
		mERPData() {}

		/*
		* 析构函数
		*/
		~mERPData();

		/*
		* 设置ID
		*/
		void setId(int id);

		/*
		* 获取ID
		*/
		int getId();

		/*
		* 设置名
		*/
		void setName(const QString& name);

		/*
		* 获取名
		*/
		QString getName();

		/*
		* 拷贝
		*/
		mERPData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mERPData& mgr);
		friend QDataStream& InStreammERPDataV3(QDataStream& in, mERPData& obj);
		friend QDataStream& operator<<(QDataStream& out, mERPData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************等效辐射功率**************************************************/
		//介质密度
		QString _erprho;

		//声速
		QString _erpc;

		//缩放系数
		QString _rhocp;

		//辐射损耗密度
		QString _erprlf;

		//参考值
		QString _erprefdb;
	};
}
