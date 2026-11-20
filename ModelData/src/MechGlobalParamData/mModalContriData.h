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
	class MXGLOBALPARAMDATA_EXPORT mModalContriData
	{

	public:

		/*
		* 重载构造函数
		*/
		mModalContriData(int id, QString name = "");

		/*
		* 
		*/
		mModalContriData() {}

		/*
		* 析构函数
		*/
		~mModalContriData();

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
		mModalContriData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mModalContriData& mgr);
		friend QDataStream& InStreammModalContriDataV3(QDataStream& in, mModalContriData& obj);
		friend QDataStream& operator<<(QDataStream& out, mModalContriData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************模态贡献量**************************************************/
		//输出类型下拉框 structure，fluid
		QString _outPutType;
		//排序方式下拉框 mag，percentage
		QString _SortType;
		//贡献量结果类型 disp、vel、acc
		QString _ResultType;
		//过滤分数 默认0.001
		QString _fltScore;
		//过滤分数是否勾选输出
		bool _fltScoreChecked;
		//输出阶数 默认3
		QString _OutOrd;
		//输出阶数是否勾选输出
		bool _OutOrdChecked;
		//峰值筛选
		bool _checkBoxPkScr;
	};
}
