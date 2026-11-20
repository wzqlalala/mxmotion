#include "mCaseBase.h"
#include "mCaseImplASCoupling.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplASCoupling::mCaseImplASCoupling(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
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
		_mNodeContriData = nullptr;
		_mPanelContriData = nullptr;
	}

	mCaseImplASCoupling::~mCaseImplASCoupling()
	{

	}
	mCaseImplASCoupling* MechCaseData::mCaseImplASCoupling::copyCase(const int& id, const QString& name)
	{
		mCaseImplASCoupling* caseData = new mCaseImplASCoupling(id, getCaseType(), name);
		caseData->_method = _method;
		caseData->_modalSolver = _modalSolver;
		caseData->_isRemnantOpen = _isRemnantOpen;

		caseData->_minFrequency = _minFrequency;
		caseData->_maxFrequency = _maxFrequency;
		caseData->_order = _order;
		caseData->_tolerance = _tolerance;
		caseData->_ampffact = _ampffact;
		caseData->_normalization = _normalization;

		caseData->_structMinFrequency = _structMinFrequency;
		caseData->_structMaxFrequency = _structMaxFrequency;
		caseData->_structOrder = _structOrder;
		caseData->_structTolerance = _structTolerance;
		caseData->_structAmpffact = _structAmpffact;
		caseData->_structNormalization = _structNormalization;
		
		caseData->_acousticsMinFrequency = _acousticsMinFrequency;
		caseData->_acousticsMaxFrequency = _acousticsMaxFrequency;
		caseData->_acousticsOrder = _acousticsOrder;
		caseData->_acousticsTolerance = _acousticsTolerance;
		caseData->_acousticsAmpffact = _acousticsAmpffact;
		caseData->_acousticsNormalization = _acousticsNormalization;

		caseData->_dampingG = _dampingG;
		caseData->_dampingAlpha1 = _dampingAlpha1;
		caseData->_dampingAlpha2 = _dampingAlpha2;

		caseData->_structDampingG = _structDampingG;
		caseData->_structDampingAlpha1 = _structDampingAlpha1;
		caseData->_structDampingAlpha2 = _structDampingAlpha2;

		caseData->_acousticsDampingG = _acousticsDampingG;
		caseData->_acousticsDampingAlpha1 = _acousticsDampingAlpha1;
		caseData->_acousticsDampingAlpha2 = _acousticsDampingAlpha2;

		caseData->checked = checked;

		if (_modalContriData)
			caseData->_modalContriData = _modalContriData->copyData(id, "模态贡献量");
		if (_mNodeContriData)
			caseData->_mNodeContriData = _mNodeContriData->copyData(id, "节点贡献量");
		if (_mPanelContriData)
			caseData->_mPanelContriData = _mPanelContriData->copyData(id, "面板贡献量");

		copyBaseDataTo(caseData);
		return caseData;
	}

	void mCaseImplASCoupling::deleteModalContriData()
	{
		if (_modalContriData)
		{
			delete _modalContriData;
			_modalContriData = nullptr;
		}
	}

	mModalContriData* mCaseImplASCoupling::createModalContriData()
	{
		if (!_modalContriData)
		{
			_modalContriData = new mModalContriData(getCaseId(), "模态贡献量");
		}
		return _modalContriData;
	}

	mModalContriData* mCaseImplASCoupling::getModalContriData()
	{
		return _modalContriData;
	}

	void mCaseImplASCoupling::deleteNodeContriData()
	{
		if (_mNodeContriData)
		{
			delete _mNodeContriData;
			_mNodeContriData = nullptr;
		}
	}

	mNodeContriData* mCaseImplASCoupling::createNodeContriData()
	{
		if (!_mNodeContriData)
		{
			_mNodeContriData = new mNodeContriData(getCaseId(), "节点贡献量");
		}
		return _mNodeContriData;
	}

	mNodeContriData* mCaseImplASCoupling::getNodeContriData()
	{
		return _mNodeContriData;
	}

	void mCaseImplASCoupling::deletePanelContriData()
	{
		if (_mPanelContriData)
		{
			delete _mPanelContriData;
			_mPanelContriData = nullptr;
		}
	}

	mPanelContriData* mCaseImplASCoupling::createPanelContriData()
	{
		if (!_mPanelContriData)
		{
			_mPanelContriData = new mPanelContriData(getCaseId(), "面板贡献量");
		}
		return _mPanelContriData;
	}

	mPanelContriData* mCaseImplASCoupling::getPanelContriData()
	{
		return _mPanelContriData;
	}

}
