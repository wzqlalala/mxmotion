#include "mCaseBase.h"
#include "mCaseImplicitStructuralCouplingHeat.h"

using namespace MxFunctions;
namespace MechCaseData
{
    mCaseImplicitStructuralCouplingHeat::mCaseImplicitStructuralCouplingHeat(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
        _cntnlsub = "关闭";  // 默认关

        // 时间步类型
        _timeStepType = "自动";  // 默认自动

        _initialStepValue = "0";
        _totolTimeValue = "0";
        _minTimeStep = "0";
        _maxTimeStep = "0";

        QHash<QString, QStringList> defaultData;
        defaultData["基本结果"] = QStringList{ "位移" };
        defaultData["热学"] = QStringList{ "温度" };

        this->setOutputs(defaultData);
	}

    mCaseImplicitStructuralCouplingHeat::~mCaseImplicitStructuralCouplingHeat()
	{

	}
    mCaseImplicitStructuralCouplingHeat* MechCaseData::mCaseImplicitStructuralCouplingHeat::copyCase(const int& id, const QString& name)
 {
        mCaseImplicitStructuralCouplingHeat* caseData = new mCaseImplicitStructuralCouplingHeat(id, getCaseType(), name);
        caseData->_cntnlsub = _cntnlsub;
        caseData->_timeStepType = _timeStepType;
        caseData->_totolTimeValue = _totolTimeValue;
        caseData->_initialStepValue = _initialStepValue;
        caseData->_minTimeStep = _minTimeStep;
        caseData->_maxTimeStep = _maxTimeStep;
  copyBaseDataTo(caseData);
  return caseData;
 }
}
