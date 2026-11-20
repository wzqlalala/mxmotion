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
	class MXGLOBALPARAMDATA_EXPORT mNodeContriData
	{
	public:

		/*
		* 重载构造函数
		*/
		mNodeContriData(int id, QString name = "");

		/*
		*
		*/
		mNodeContriData() {}

		/*
		* 析构函数
		*/
		~mNodeContriData();

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
		mNodeContriData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mNodeContriData& mgr);
		friend QDataStream& InStreammNodeContriDataV3(QDataStream& in, mNodeContriData& obj);
		friend QDataStream& operator<<(QDataStream& out, mNodeContriData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************模态贡献量**************************************************/
		//响应截断值类型("帕斯卡"/"分贝计权")
		QString _rpTruncType;

		//响应截断值
		QString _rpTruncValue;

		//声腔网格投影值输出
		bool _checkBoxCavityProj;

		//峰值筛选
		bool _checkBoxPkScr;
	};
}
