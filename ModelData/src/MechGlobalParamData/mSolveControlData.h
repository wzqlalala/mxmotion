#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QVector>
#include <QMap>
#include <QHash>
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mModelEnum.h"

namespace MechGlobalParamData
{
	class MXGLOBALPARAMDATA_EXPORT mSolveControlData
	{

	public:

		/*
		* 重载构造函数
		*/
		mSolveControlData();

		/*
		* 析构函数
		*/
		~mSolveControlData();

		/*
		* 判断节点数据状态
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);


		friend QDataStream& operator>>(QDataStream& in, mSolveControlData& mgr);
		friend QDataStream& InStreammSolveControlDataV3(QDataStream& in, mSolveControlData& obj);
		friend QDataStream& operator<<(QDataStream& out, mSolveControlData& mgr);


	public:

		/*
		* 设置CPU并行核数
		* @param value 正整数字符串
		*/
		void setCpuCoreCount(const QString& value);

		/*
		* 获取CPU并行核数
		* @return 返回正整数，若未设置或非法则返回1
		*/
		int getCpuCoreCount() const;

		/*
		* 获取CPU并行核数字符串
		*/
		QString getCpuCoreCountString() const;

	private:

		QString _cpuCoreCount;
	};
}
