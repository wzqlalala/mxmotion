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
	class MXGLOBALPARAMDATA_EXPORT mGenericPartData
	{
		
	public:

		/*
		* 重载构造函数
		*/
		mGenericPartData();

		/*
		* 析构函数
		*/
		~mGenericPartData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);


		friend QDataStream& operator>>(QDataStream& in, mGenericPartData& mgr);
		friend QDataStream& InStreammGenericPartDataV3(QDataStream& in, mGenericPartData& obj);
		friend QDataStream& operator<<(QDataStream& out, mGenericPartData& mgr);


	public:

		
	};
}