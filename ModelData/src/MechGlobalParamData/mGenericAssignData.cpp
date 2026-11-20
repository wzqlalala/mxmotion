#include "mGenericAssignData.h"
#include <QVector>
#include <QPair>
#include <QString>

namespace MechGlobalParamData
{
	mGenericAssignData::mGenericAssignData()
	{
		_filePath.resize(1);
	}

	mGenericAssignData::~mGenericAssignData()
	{

	}

	MxFunctions::BrowserTreeStatus mGenericAssignData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";

		if (_filePath.empty())
		{
			info.append({ tip,"未选择pch文件" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		// 检查矩阵配置：如果勾选了但未输入名字，给出警告
		if (_stiffnessMatrixEnabled && _stiffnessMatrixNames.trimmed().isEmpty())
		{
			info.append({ tip, "刚度矩阵已勾选但未输入矩阵名字" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		if (_massMatrixEnabled && _massMatrixNames.trimmed().isEmpty())
		{
			info.append({ tip, "质量矩阵已勾选但未输入矩阵名字" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		if (_viscousDampingMatrixEnabled && _viscousDampingMatrixNames.trimmed().isEmpty())
		{
			info.append({ tip, "粘性阻尼矩阵已勾选但未输入矩阵名字" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		if (_materialMatrixEnabled && _materialMatrixNames.trimmed().isEmpty())
		{
			info.append({ tip, "材料矩阵已勾选但未输入矩阵名字" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		return status;
	}
}