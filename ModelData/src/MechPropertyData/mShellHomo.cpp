#include "mShellHomo.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mShellHomo::mShellHomo(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{
		_thickness = "";
		_nsm = "";
		_referenceplane = "无";
		_z0 = "";
	}

	mShellHomo::~mShellHomo()
	{

	}
	mShellHomo* MechPropertyData::mShellHomo::copyProperty(const int& id, const QString& name)
	{
		mShellHomo* caseData = new mShellHomo(id,getPropertyType(), name);
		caseData->_thickness = _thickness;
		caseData->_nsm = _nsm;
		caseData->_referenceplane = _referenceplane;
		caseData->_z0 = _z0;
		return caseData;
	}

	void mShellHomo::setThickness(QString thickness)
	{
		_thickness = thickness;
	}
	QString mShellHomo::getThickness()
	{
		return _thickness;
	}

	MxFunctions::BrowserTreeStatus mShellHomo::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		QString tip = "警告";
		checkRequiredPositive(_thickness, "厚度", info, status);
		//checkOptionalPositive(_nsm, "非结构化质量", info, status);
		if (_referenceplane == "偏置距离")
		{
			checkRequiredPositive(_z0, "偏置距离", info, status);
		}
		return status;
	}
}
