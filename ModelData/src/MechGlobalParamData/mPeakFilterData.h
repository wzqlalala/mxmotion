#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector3D>
#include "mModelEnum.h"
#include <QVector>

namespace MechGlobalParamData
{
	class MXGLOBALPARAMDATA_EXPORT mPeakFilterData
	{
	public:

		/*
		* 重载构造函数
		*/
		mPeakFilterData(int id, QString name = "");

		/*
		*
		*/
		mPeakFilterData() {}

		/*
		* 析构函数
		*/
		~mPeakFilterData();

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
		mPeakFilterData* copyData(const int& id, const QString& name);

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mPeakFilterData& mgr);
		friend QDataStream& InStreammPeakFilterDataV3(QDataStream& in, mPeakFilterData& obj);
		friend QDataStream& operator<<(QDataStream& out, mPeakFilterData& mgr);

	private:

		//ID
		int _id;

		//名字
		QString _name;

	public:
		/**************************************峰值筛选**************************************************/
		// 响应类型 (disp, vel, acc)
		QString _respType;

		// 识别类型 (db, dba, non)
		QString _identType;

		// 提取个数
		QString _extractCount;

		// 最小间距
		QString _minSpacing;

		// 最大间距
		QString _maxSpacing;

		// 起始频率
		QString _startFreq;

		// 终止频率
		QString _endFreq;

		// 应用到全局
		bool _applyToGlobal;

		// 表格数据：响应点, 自由度, 截断值
		QVector<QVector<double>> _tableData;
	};
}
