#include "mCaseBase.h"
#include "mCaseImplicitResponseSpectrum.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitResponseSpectrum::mCaseImplicitResponseSpectrum(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
		_modalSolver = "兰索斯";
		_minFrequency = "";
		_maxFrequency = "";
		_order = "";
		_tolerance = "";
		_ampffact = "";
		_normalization = "无";

		_dampingG = "";
		_dampingAlpha1 = "";
		_dampingAlpha2 = "";

		_dirction = "";
		_modal = "";
		_modalFrequencyClose = "";


		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setOutputs(defaultData);

	}

	mCaseImplicitResponseSpectrum::~mCaseImplicitResponseSpectrum()
	{

	}
	mCaseImplicitResponseSpectrum* MechCaseData::mCaseImplicitResponseSpectrum::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitResponseSpectrum* caseData = new mCaseImplicitResponseSpectrum(id, getCaseType(), name);
		caseData->_modalSolver = _modalSolver;

		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		caseData->_tolerance = _tolerance;
		caseData->_ampffact = _ampffact;
		caseData->_normalization = _normalization;

		caseData->_dampingG = _dampingG;
		caseData->_dampingAlpha1 = _dampingAlpha1;
		caseData->_dampingAlpha2 = _dampingAlpha2;

		caseData->_dirction = _dirction;
		caseData->_modal = _modal;
		caseData->_modalFrequencyClose = _modalFrequencyClose;
		caseData->_tableData = _tableData;
		copyBaseDataTo(caseData);
		return caseData;
	}
}
