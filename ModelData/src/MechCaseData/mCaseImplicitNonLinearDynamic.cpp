#include "mCaseBase.h"
#include "mCaseImplicitNonLinearDynamic.h"

using namespace MxFunctions;
namespace MechCaseData
{
    mCaseImplicitNonLinearDynamic::mCaseImplicitNonLinearDynamic(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
        // 几何非线性
        _nonLinearity = "关";  // 默认关
        _cntnlsub = "关闭";
        _inertiaRelief = "关闭";

        //准静态分析
        _quasiStaticAnasys = "关";  // 默认关

        // 时间步类型
        _timeStepType = "自动";  // 默认自动

        // 时间步相关
        _initialStepValue = "1.0";
        _totolTimeValue = "1.0";
        _minTimeStep = "1e-5";
        _maxTimeStep = "0";

        QHash<QString, QStringList> defaultData;
        defaultData["基本结果"] = QStringList{ "位移" };

        this->setOutputs(defaultData);
	}

    mCaseImplicitNonLinearDynamic::~mCaseImplicitNonLinearDynamic()
	{

	}
    mCaseImplicitNonLinearDynamic* MechCaseData::mCaseImplicitNonLinearDynamic::copyCase(const int& id, const QString& name)
 {
        mCaseImplicitNonLinearDynamic* caseData = new mCaseImplicitNonLinearDynamic(id, getCaseType(), name);
  caseData->_nonLinearity = _nonLinearity;
  caseData->_cntnlsub = _cntnlsub;
        caseData->_inertiaRelief = _inertiaRelief;
        caseData->_timeStepType = _timeStepType;
        caseData->_totolTimeValue = _totolTimeValue;
        caseData->_initialStepValue = _initialStepValue;
        caseData->_minTimeStep = _minTimeStep;
        caseData->_maxTimeStep = _maxTimeStep;
        caseData->_quasiStaticAnasys = _quasiStaticAnasys;
  copyBaseDataTo(caseData);
  return caseData;
 }
}
