#include "mGenericMatrixData.h"

namespace MechGlobalParamData
{
	mGenericMatrixData::mGenericMatrixData()
	{

	}

	mGenericMatrixData::~mGenericMatrixData()
	{


	}
	MxFunctions::BrowserTreeStatus mGenericMatrixData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		if (_matrixName.isEmpty())
		{
			info.append({ tip,"矩阵未定义名称" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}

		return status;
	}
}

