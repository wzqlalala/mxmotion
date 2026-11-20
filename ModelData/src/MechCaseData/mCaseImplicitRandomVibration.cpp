#include "mCaseBase.h"
#include "mCaseImplicitRandomVibration.h"

using namespace MxFunctions; 
namespace MechCaseData
{
	mCaseImplicitRandomVibration::mCaseImplicitRandomVibration(int id, CaseType type, const QString& name):mCaseBase(id,type,name)
	{
		_time0 = "0.0";
		_timeMax = "0.0";
		_nTime = "0";

		QHash<QString, QStringList> defaultData;
		defaultData["PSD谱分析"] = QStringList{ "节点位移功率谱" };
		defaultData["RMS谱分析"] = QStringList{ "节点位移1σ均方根" };
		QHash<QString, QStringList> defaultPchData;
		defaultPchData["基本结果"] = QStringList{ "位移" };
		this->setPchOutputs(defaultPchData);
		this->setOutputs(defaultData);

	}

	mCaseImplicitRandomVibration::~mCaseImplicitRandomVibration()
	{

	}
	mCaseImplicitRandomVibration* MechCaseData::mCaseImplicitRandomVibration::copyCase(const int& id, const QString& name)
	{
		mCaseImplicitRandomVibration* caseData = new mCaseImplicitRandomVibration(id, getCaseType(), name);
		caseData->_time0 = _time0;
		caseData->_timeMax = _timeMax;
		caseData->_nTime = _nTime;
		caseData->_tableXYData = _tableXYData;
		caseData->m_outputData = m_outputData;
		copyBaseDataTo(caseData);

		return caseData;
	}
}
