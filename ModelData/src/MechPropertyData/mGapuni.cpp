#include "mGapuni.h"
#include "mNodeValidor.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mGapuni::mGapuni(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{
		_gapDistance = "";
		_area = "1.0";
		_direction = "";
	}

	mGapuni::~mGapuni()
	{

	}

	mGapuni* MechPropertyData::mGapuni::copyProperty(const int& id, const QString& name)
	{
		mGapuni* caseData = new mGapuni(id, getPropertyType(), name);
		caseData->_gapDistance = _gapDistance;
		caseData->_area = _area;
		caseData->_direction = _direction;
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mGapuni::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		checkOptionalPositive(_gapDistance, "间隙距离", info, status);
		checkOptionalPositive(_area, "截面积", info, status);
		checkOptionalVector(_direction, "方向", info, status);
		return status;
	}
}
