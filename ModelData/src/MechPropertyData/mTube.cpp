#include "mTube.h"
#include "mNodeValidor.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mTube::mTube(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{
		_od = "";
		_t = "";
		_nsm = "";
		_od2 = "";
	}

	mTube::~mTube()
	{

	}

	mTube* MechPropertyData::mTube::copyProperty(const int& id, const QString& name)
	{
		mTube* caseData = new mTube(id,getPropertyType(), name);
		caseData->_od = _od;
		caseData->_t = _t;
		caseData->_nsm = _nsm;
		caseData->_od2 = _od2;
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mTube::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		checkRequiredPositive(_od, "外径", info, status);
		checkRequiredPositive(_t, "壁厚", info, status);
		checkRequiredPositive(_nsm, "非结构化质量", info, status);
		checkRequiredPositive(_od2, "内径", info, status);
		return status;
	}
}
