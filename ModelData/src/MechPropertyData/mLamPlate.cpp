#include "mLamPlate.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mLamPlate::mLamPlate(int id, LaminateType type, const QString& name)
		: mLaminateBase(id, type, name)
	{
		_type = 0;
	}

	mLamPlate::~mLamPlate()
	{

	}

	mLamPlate* MechPropertyData::mLamPlate::copyLaminate(const int& id, const QString& name)
	{
		mLamPlate* caseData = new mLamPlate(id, getLaminateType(), name);
		caseData->_type = _type;
		return caseData;
	}
	MxFunctions::BrowserTreeStatus mLamPlate::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		return  MxFunctions::BrowserTreeStatus::Tree_Yes;
	}
}
