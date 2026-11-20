#include "mShearPlate.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mShearPlate::mShearPlate(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{
		_thickness = "";
		_valF1 = "";
		_valF2 = "";
	}

	mShearPlate::~mShearPlate()
	{

	}
	mShearPlate* MechPropertyData::mShearPlate::copyProperty(const int& id, const QString& name)
	{
		mShearPlate* caseData = new mShearPlate(id,getPropertyType(), name);
		caseData->_thickness = _thickness;
		caseData->_valF1 = _valF1;
		caseData->_valF2 = _valF2;
		return caseData;
	}
	void mShearPlate::setThickness(QString thickness)
	{
		_thickness = thickness;
	}
	QString mShearPlate::getThickness()
	{
		return _thickness;
	}

	MxFunctions::BrowserTreeStatus mShearPlate::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		checkRequiredPositive(_thickness, "厚度", info, status);
		checkRequiredPositive(_valF1, "拉伸刚度F1", info, status);
		checkRequiredPositive(_valF2, "拉伸刚度F2", info, status);
		return status;
	}
}
