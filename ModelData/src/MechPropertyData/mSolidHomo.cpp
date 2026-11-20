#include "mSolidHomo.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mSolidHomo::mSolidHomo(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{

	}

	mSolidHomo::~mSolidHomo()
	{

	}
	mSolidHomo* MechPropertyData::mSolidHomo::copyProperty(const int& id, const QString& name)
	{
		mSolidHomo* caseData = new mSolidHomo(id,getPropertyType(), name);
		return caseData;
	}
}
