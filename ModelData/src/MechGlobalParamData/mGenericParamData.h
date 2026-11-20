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
	class MXGLOBALPARAMDATA_EXPORT mGenericParamData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mGenericParamData(int id, QString name = "");

		/*
		* 析构函数
		*/
		~mGenericParamData();

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
		mGenericParamData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mGenericParamData& mgr);
		friend QDataStream& InStreammGenericParamDataV3(QDataStream& in, mGenericParamData& obj);
		friend QDataStream& operator<<(QDataStream& out, mGenericParamData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		
		//缩聚方法(必选) GUYAN/CBN/GM
		QString _method;

		//模态频率值上限(double,非必选)
		QString _upFrequency;

		//模态阶数(int，非必选)
		QString _order;

		//起始点编号(int,GUYAN非必选)
		QString _startID;
	};
}
