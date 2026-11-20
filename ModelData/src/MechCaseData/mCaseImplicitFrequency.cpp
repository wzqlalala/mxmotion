#include "mCaseBase.h"
#include "mCaseImplicitFrequency.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitFrequency::mCaseImplicitFrequency(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
		_method = "模态法";
		_isRemnantOpen = false;

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

		checked = false;

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setPchOutputs(defaultData);
		this->setOutputs(defaultData);
		_modalContriData = nullptr;
	}

	mCaseImplicitFrequency::~mCaseImplicitFrequency()
	{

	}
	mCaseImplicitFrequency* MechCaseData::mCaseImplicitFrequency::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitFrequency* caseData = new mCaseImplicitFrequency(id, getCaseType(), name);
		caseData->_method = _method;
		caseData->_modalSolver = _modalSolver;
		caseData->_isRemnantOpen = _isRemnantOpen;

		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		caseData->_tolerance = _tolerance;
		caseData->_ampffact = _ampffact;
		caseData->_normalization = _normalization;

		caseData->_dampingG = _dampingG;
		caseData->_dampingAlpha1 = _dampingAlpha1;
		caseData->_dampingAlpha2 = _dampingAlpha2;

		caseData->checked = checked;

		if (_modalContriData)
			caseData->_modalContriData = _modalContriData->copyData(id, "模态贡献量");

		copyBaseDataTo(caseData);
		return caseData;
	}

	void mCaseImplicitFrequency::deleteModalContriData()
	{
		if (_modalContriData)
		{
			delete _modalContriData;
			_modalContriData = nullptr;
		}
	}
	mModalContriData* mCaseImplicitFrequency::createModalContriData()
	{
		if (!_modalContriData)
		{
			_modalContriData = new mModalContriData(getCaseId(), "模态贡献量");
		}
		return _modalContriData;
	}
	mModalContriData* mCaseImplicitFrequency::getModalContriData()
	{
		return _modalContriData;
	}
}
