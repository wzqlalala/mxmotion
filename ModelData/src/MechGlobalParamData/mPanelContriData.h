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
	class MXGLOBALPARAMDATA_EXPORT mPanelContriData
	{
	public:

		/*
		* 重载构造函数
		*/
		mPanelContriData(int id, QString name = "");

		/*
		*
		*/
		mPanelContriData() {}

		/*
		* 析构函数
		*/
		~mPanelContriData();

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
		mPanelContriData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mPanelContriData& mgr);
		friend QDataStream& InStreammPanelContriDataV3(QDataStream& in, mPanelContriData& obj);
		friend QDataStream& operator<<(QDataStream& out, mPanelContriData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************面板贡献量**************************************************/
		//输出类型下拉框 幅值、实部、虚部
		QString _outPutType;

		//峰值筛选
		bool _checkBoxPkScr;
	};
}
