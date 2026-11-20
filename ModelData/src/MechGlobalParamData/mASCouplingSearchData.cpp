#include "mASCouplingSearchData.h"

namespace MechGlobalParamData
{
	mASCouplingSearchData::mASCouplingSearchData()
	{
		_filePath.resize(1);
	}

	mASCouplingSearchData::~mASCouplingSearchData()
	{


	}
	MxFunctions::BrowserTreeStatus mASCouplingSearchData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		/*QString tip = "警告";
		if (_filePath.empty())
		{
			info.append({ tip,"未选择pch文件" });
			status = MxFunctions::BrowserTreeStatus::Tree_Failure;
		}*/

		return status;
	}
}

