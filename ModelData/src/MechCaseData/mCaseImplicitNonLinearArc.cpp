#include "mCaseBase.h"
#include "mCaseImplicitNonLinearArc.h"

using namespace MxFunctions;
namespace MechCaseData
{
    mCaseImplicitNonLinearArc::mCaseImplicitNonLinearArc(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
        // 几何非线性
        _nonLinearity = "关";  // 默认关
        _cntnlsub = "关闭";
        _inertiaRelief = "关闭";

        // 时间步类型
        _timeStepType = "自动";  // 默认自动

        // 时间步相关
        _initialStepValue = "1.0";
        _totolTimeValue = "1.0";
        _minTimeStep = "1e-5";
        _maxTimeStep = "1e+36";

        _lMax = "1e+20";
        
        _nMax = "100";

        // 初始缺陷
        _initBug = false;
        _initBugType = MxFunctions::InitBugType::Buckling;
        _initBugFileName = "";
        _scaleValue = "";

        QHash<QString, QStringList> defaultData;
        defaultData["基本结果"] = QStringList{ "位移" };

        this->setOutputs(defaultData);
	}

    mCaseImplicitNonLinearArc::~mCaseImplicitNonLinearArc()
	{

	}
    mCaseImplicitNonLinearArc* MechCaseData::mCaseImplicitNonLinearArc::copyCase(const int& id, const QString& name)
 {
        mCaseImplicitNonLinearArc* caseData = new mCaseImplicitNonLinearArc(id, getCaseType(), name);
  caseData->_nonLinearity = _nonLinearity;
  caseData->_cntnlsub = _cntnlsub;
        caseData->_inertiaRelief = _inertiaRelief;
        caseData->_timeStepType = _timeStepType;
        caseData->_totolTimeValue = _totolTimeValue;
        caseData->_initialStepValue = _initialStepValue;
        caseData->_minTimeStep = _minTimeStep;
        caseData->_maxTimeStep = _maxTimeStep;
        caseData->_lMax = _lMax;
        caseData->_nMax = _nMax;
        caseData->_initBug = _initBug;
        caseData->_initBugType = _initBugType;
        caseData->_initBugFileName = _initBugFileName;
        caseData->_scaleValue = _scaleValue;
  copyBaseDataTo(caseData);
  return caseData;
 }
}
