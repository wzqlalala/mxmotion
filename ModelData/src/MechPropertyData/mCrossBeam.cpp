#include "mCrossBeam.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mCrossBeam::mCrossBeam(int id,PropertyType type, const QString& name):mPropertyBase(id,type, name)
	{
		initCrossBeam();
	}

	mCrossBeam::~mCrossBeam()
	{

	}
	void mCrossBeam::initCrossBeam()
	{
		_paraList = QStringList{ "0", "1" };
	}
	mCrossBeam* MechPropertyData::mCrossBeam::copyProperty(const int& id, const QString& name)
	{
		mCrossBeam* caseData = new mCrossBeam(id,getPropertyType(), name);
		caseData->_paraList = _paraList;
		return caseData;
	}
}
