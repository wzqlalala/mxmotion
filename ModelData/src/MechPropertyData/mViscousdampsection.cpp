#include "mViscousdampsection.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mViscousdampsection::mViscousdampsection(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{
		_GE = "";
	}

	mViscousdampsection::~mViscousdampsection()
	{

	}
	mViscousdampsection* MechPropertyData::mViscousdampsection::copyProperty(const int& id, const QString& name)
	{
		mViscousdampsection* caseData = new mViscousdampsection(id,getPropertyType(), name);
		caseData->_GE = _GE;
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mViscousdampsection::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		checkRequiredPositive(_GE, "阻尼系数", info, status);
		return status;
	}
}
