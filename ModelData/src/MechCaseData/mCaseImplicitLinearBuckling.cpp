#include "mCaseBase.h"
#include "mCaseImplicitLinearBuckling.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitLinearBuckling::mCaseImplicitLinearBuckling(int id, CaseType type, const QString& name) :mCaseBase(id, type, name)
	{
		_minFrequency = "";
		_maxFrequency = "";
		_order = "";
		_tolerance = "0.0001";

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setOutputs(defaultData);
	}

	mCaseImplicitLinearBuckling::~mCaseImplicitLinearBuckling()
	{

	}
	mCaseImplicitLinearBuckling* MechCaseData::mCaseImplicitLinearBuckling::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitLinearBuckling* caseData = new mCaseImplicitLinearBuckling(id, getCaseType(), name);
		//caseData->_modalSolver = _modalSolver;
		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		//caseData->_preloadName = _preloadName;
		caseData->_tolerance = _tolerance;
		copyBaseDataTo(caseData);
		return caseData;
	}
}
