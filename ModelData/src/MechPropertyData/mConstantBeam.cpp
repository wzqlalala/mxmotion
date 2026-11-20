#include "mConstantBeam.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mConstantBeam::mConstantBeam(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{
		_angle = "0";
	}

	mConstantBeam::~mConstantBeam()
	{

	}
	mConstantBeam* MechPropertyData::mConstantBeam::copyProperty(const int& id, const QString& name)
	{
		mConstantBeam* caseData = new mConstantBeam(id,getPropertyType(), name);
		caseData->_angle = _angle;
		return caseData;
	}
	void mConstantBeam::setAngle(QString angle)
	{
		_angle = angle;
	}
	QString mConstantBeam::getAngle()
	{
		return _angle;
	}
}
