#include "mCaseBase.h"
#include "mCaseImplicitTransient.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitTransient::mCaseImplicitTransient(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
		_method = "模态法";
		_solveMethod = "瞬态newmark法";
		_isRemnantOpen = false;

		_timestepCount = "0";
		_timestepVal = "0";

		_modalSolver = "兰索斯";
		_minFrequency = "";
		_maxFrequency = "";
		_order = "";
		_tolerance = "";
		_ampffact = "";
		_normalization = "无";

		_dampingG = "";
		_dampingOmg3 = "";
		_dampingAlpha1 = "";
		_dampingAlpha2 = "";

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setPchOutputs(defaultData);
		this->setOutputs(defaultData);

	}

	mCaseImplicitTransient::~mCaseImplicitTransient()
	{

	}
	mCaseImplicitTransient* MechCaseData::mCaseImplicitTransient::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitTransient* caseData = new mCaseImplicitTransient(id, getCaseType(), name);
		caseData->_method = _method;
		caseData->_timestepCount = _timestepCount;
		caseData->_timestepVal = _timestepVal;
		caseData->_modalSolver = _modalSolver;
		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		caseData->_tolerance = _tolerance;
		caseData->_ampffact = _ampffact;
		caseData->_normalization = _normalization;
		caseData->_isRemnantOpen = _isRemnantOpen;
		caseData->_dampingG = _dampingG;
		caseData->_dampingAlpha1 = _dampingAlpha1;
		caseData->_dampingAlpha2 = _dampingAlpha2;
		caseData->_solveMethod = _solveMethod;
		copyBaseDataTo(caseData);
		return caseData;
	}
}
