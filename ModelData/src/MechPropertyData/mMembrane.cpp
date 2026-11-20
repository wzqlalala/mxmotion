#include "mMembrane.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mMembrane::mMembrane(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{

	}

	mMembrane::~mMembrane()
	{

	}
	mMembrane* MechPropertyData::mMembrane::copyProperty(const int& id, const QString& name)
	{
		mMembrane* caseData = new mMembrane(id,getPropertyType(), name);
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mMembrane::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		return status;
	}
}
