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
	class MXGLOBALPARAMDATA_EXPORT mTPContriData
	{
	public:

		/*
		* 重载构造函数
		*/
		mTPContriData(int id, QString name = "");

		/*
		*
		*/
		mTPContriData() {}

		/*
		* 析构函数
		*/
		~mTPContriData();

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
		mTPContriData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mTPContriData& mgr);
		friend QDataStream& InStreammTPContriDataV3(QDataStream& in, mTPContriData& obj);
		friend QDataStream& operator<<(QDataStream& out, mTPContriData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************传递路径贡献量**************************************************/
		//响应类型下拉框 位移、速度、加速度
		QString _respType;
	};
}
