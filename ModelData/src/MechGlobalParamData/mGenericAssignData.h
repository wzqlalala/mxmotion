#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXGlobalParamData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QDataStream>
#include <QList>
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mModelEnum.h"
#include <vector>

namespace MechGlobalParamData
{
	class MXGLOBALPARAMDATA_EXPORT mGenericAssignData
	{

	public:

		/*
		* 重载构造函数
		*/
		mGenericAssignData();

		/*
		* 析构函数
		*/
		~mGenericAssignData();

		/*
		* 判断数据完整性
		*/
		MxFunctions::BrowserTreeStatus getNodeStatus(QVector<QPair<QString, QString>>& info);

		friend QDataStream& operator>>(QDataStream& in, mGenericAssignData& mgr);
		friend QDataStream& InStreammGenericAssignDataV3(QDataStream& in, mGenericAssignData& obj);
		friend QDataStream& operator<<(QDataStream& out, const mGenericAssignData& mgr);

	public:
		std::vector<QString> _filePath; // 超单元文件路径

		// 矩阵配置（固定4个，不随超单元个数变化）
		bool _stiffnessMatrixEnabled = false;   // 刚度矩阵勾选状态
		QString _stiffnessMatrixNames;          // 刚度矩阵名字(逗号分隔，数量=超单元个数)

		bool _massMatrixEnabled = false;        // 质量矩阵勾选状态
		QString _massMatrixNames;               // 质量矩阵名字(逗号分隔，数量=超单元个数)

		bool _viscousDampingMatrixEnabled = false; // 粘性阻尼矩阵勾选状态
		QString _viscousDampingMatrixNames;        // 粘性阻尼矩阵名字(逗号分隔，数量=超单元个数)

		bool _materialMatrixEnabled = false;    // 材料矩阵勾选状态
		QString _materialMatrixNames;           // 材料矩阵名字(逗号分隔，数量=超单元个数)
	};
}