#include "mGasket.h"
#include "mNodeValidor.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mGasket::mGasket(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{
		_thicknessDef = "自动计算";
		_initialThickness = "";
		_initialGap = "";
	}

	mGasket::~mGasket()
	{

	}

	mGasket* MechPropertyData::mGasket::copyProperty(const int& id, const QString& name)
	{
		mGasket* caseData = new mGasket(id, getPropertyType(), name);
		caseData->_thicknessDef = _thicknessDef;
		caseData->_initialThickness = _initialThickness;
		caseData->_initialGap = _initialGap;
		return caseData;
	}

	void mGasket::setThickness(QString thickness)
	{
		_initialThickness = thickness;
	}

	QString mGasket::getThickness()
	{
		return _initialThickness;
	}

	MxFunctions::BrowserTreeStatus mGasket::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		if (_thicknessDef == "指定厚度")
		{
			checkRequiredPositive(_initialThickness, "初始厚度", info, status);
		}
		checkRequiredPositive(_initialGap, "初始间隙", info, status);
		return status;
	}
}
