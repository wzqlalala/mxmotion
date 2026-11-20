#include "mCaseBase.h"
#include "mCaseImplicitLinearStatic.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitLinearStatic::mCaseImplicitLinearStatic(int id, CaseType type, const QString& name):mCaseBase(id,type, name)
	{
		_equationSolver = "直接法求解器DSV";
		_inertiaRelief = "关闭";
		_autospc = "开启";

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setOutputs(defaultData);
	}

	mCaseImplicitLinearStatic::~mCaseImplicitLinearStatic()
	{

	}
	mCaseImplicitLinearStatic* MechCaseData::mCaseImplicitLinearStatic::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitLinearStatic* caseData = new mCaseImplicitLinearStatic(id,getCaseType(), name);
		caseData->_equationSolver = _equationSolver;
		caseData->_inertiaRelief = _inertiaRelief;
		caseData->_autospc = _autospc;
		copyBaseDataTo(caseData);
		return caseData;
	}
}
