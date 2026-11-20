#include "mGenericPartData.h"

namespace MechGlobalParamData
{
	mGenericPartData::mGenericPartData()
	{

	}

	mGenericPartData::~mGenericPartData()
	{


	}
	MxFunctions::BrowserTreeStatus mGenericPartData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;

		return status;
	}
}

