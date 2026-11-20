#include "mCaseBase.h"
#include "mCaseImplicitNonLinearStatic.h"

using namespace MxFunctions;
namespace MechCaseData
{
	mCaseImplicitNonLinearStatic::mCaseImplicitNonLinearStatic(int id, CaseType type, const QString& name) :mCaseBase(id, type, name)
	{
		// 几何非线性
		_nonLinearity = "关";  // 默认关
		_cntnlsub = "关闭";
		_inertiaRelief = "关闭";
		_Sparsematrix = "关闭";

		// 时间步类型
		_timeStepType = "自动";  // 默认自动

		// 时间步相关
		_initialStepValue = "1.0";
		_totolTimeValue = "1";
		_minTimeStep = "1e-6";
		_maxTimeStep = "1";

		//粘性
		_visco = false;
		_cetol = "0.0001";

		QHash<QString, QStringList> defaultData;
		defaultData["基本结果"] = QStringList{ "位移" };

		this->setOutputs(defaultData);
	}

	mCaseImplicitNonLinearStatic::~mCaseImplicitNonLinearStatic()
	{

	}
	mCaseImplicitNonLinearStatic* MechCaseData::mCaseImplicitNonLinearStatic::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitNonLinearStatic* caseData = new mCaseImplicitNonLinearStatic(id, getCaseType(), name);
		caseData->_nonLinearity = _nonLinearity;
		caseData->_cntnlsub = _cntnlsub;
		caseData->_inertiaRelief = _inertiaRelief;
		caseData->_Sparsematrix = _Sparsematrix;
		caseData->_timeStepType = _timeStepType;
		caseData->_totolTimeValue = _totolTimeValue;
		caseData->_initialStepValue = _initialStepValue;
		caseData->_minTimeStep = _minTimeStep;
		caseData->_maxTimeStep = _maxTimeStep;
		caseData->_visco = _visco;
		caseData->_cetol = _cetol;
		copyBaseDataTo(caseData);
		return caseData;
	}
}
