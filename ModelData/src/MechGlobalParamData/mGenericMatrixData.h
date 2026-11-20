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
	class MXGLOBALPARAMDATA_EXPORT mGenericMatrixData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mGenericMatrixData();

		/*
		* 析构函数
		*/
		~mGenericMatrixData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);


		friend QDataStream& operator>>(QDataStream& in, mGenericMatrixData& mgr);
		friend QDataStream& InStreammGenericMatrixDataV3(QDataStream& in, mGenericMatrixData& obj);
		friend QDataStream& operator<<(QDataStream& out, mGenericMatrixData& mgr);


	public:

		QString _matrixName;
		
	};
}