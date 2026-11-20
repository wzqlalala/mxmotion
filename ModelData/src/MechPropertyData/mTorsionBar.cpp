#include "mTorsionBar.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mTorsionBar::mTorsionBar(int id,PropertyType type, const QString& name):mPropertyBase(id, type, name)
	{
		_J = "";
		_A = "";
		_nsm = "";
	}

	mTorsionBar::~mTorsionBar()
	{

	}
	mTorsionBar* MechPropertyData::mTorsionBar::copyProperty(const int& id, const QString& name)
	{
		mTorsionBar* caseData = new mTorsionBar(id,getPropertyType(), name);
		caseData->_J = _J;
		caseData->_A = _A;
		caseData->_nsm = _nsm;
		return caseData;
	}

	MxFunctions::BrowserTreeStatus mTorsionBar::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		MxFunctions::BrowserTreeStatus status = MxFunctions::BrowserTreeStatus::Tree_Yes;

		// _J 是实数要求，_A 是正实数要求
		checkRequiredReal(_J, "扭转矩", info, status);
		checkRequiredPositive(_A, "杆横截面积", info, status);

		// 非结构化质量是可选参数，但填了就必须是正实数
		checkOptionalPositive(_nsm, "非结构化质量", info, status);

		return status;
	}
}
