#include "mCaseBase.h"
#include "mCaseImplicitGeneric.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitGeneric::mCaseImplicitGeneric(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
	}

	mCaseImplicitGeneric::~mCaseImplicitGeneric()
	{
	}

	mCaseImplicitGeneric* MechCaseData::mCaseImplicitGeneric::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitGeneric* caseData = new mCaseImplicitGeneric(id, getCaseType(), name);
		copyBaseDataTo(caseData);
		return caseData;
	}
}
