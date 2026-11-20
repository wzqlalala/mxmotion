#include "mMacroEquivalent.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mMacroEquivalent::mMacroEquivalent(int id, PropertyType type, const QString& name) :mPropertyBase(id, type, name)
	{

	}

	mMacroEquivalent::~mMacroEquivalent()
	{

	}

	mMacroEquivalent* MechPropertyData::mMacroEquivalent::copyProperty(const int& id, const QString& name)
	{
		mMacroEquivalent* caseData = new mMacroEquivalent(id, getPropertyType(), name);
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mMacroEquivalent::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;
		return status;
	}
}
