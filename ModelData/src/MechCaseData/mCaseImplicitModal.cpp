#include "mCaseBase.h"
#include "mCaseImplicitModal.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitModal::mCaseImplicitModal(int id, CaseType type, const QString& name) :mCaseBase(id, type, name)
	{
		_modalType = "模态";
		_modalSolver = "兰索斯";
		_nlgeo = false;
		_complexMethod = "无";
		_minFrequency = "";
		_maxFrequency = "";
		_order = "";
		_tolerance = "";
		_ampffact = "";
		_normalization = "无";
		_rootNum = "";
		_dampingG = "";

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setOutputs(defaultData);
	}

	mCaseImplicitModal::~mCaseImplicitModal()
	{

	}
	mCaseImplicitModal* MechCaseData::mCaseImplicitModal::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitModal* caseData = new mCaseImplicitModal(id, getCaseType(), name);
		caseData->_modalType = _modalType;
		caseData->_modalSolver = _modalSolver;
		caseData->_nlgeo = _nlgeo;
		caseData->_complexMethod = _complexMethod;
		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		caseData->_tolerance = _tolerance;
		caseData->_ampffact = _ampffact;
		caseData->_normalization = _normalization;
		caseData->_rootNum = _rootNum;
		caseData->_dampingG = _dampingG;
		caseData->_standCoor = _standCoor;
		caseData->_picChecked = _picChecked;
		caseData->_picValue = _picValue;
		caseData->_rotationalChecked = _rotationalChecked;
		copyBaseDataTo(caseData);

		return caseData;
	}
}
