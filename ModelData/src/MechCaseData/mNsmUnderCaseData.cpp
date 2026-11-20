#include "mNsmUnderCaseData.h"
#include <QVector>
#include <QPair>
#include <QString>

using namespace MxFunctions;
namespace MechCaseData
{
	

	mNsmUnderCaseData::mNsmUnderCaseData(int id)
		: _caseId(id)
	{
		
	}

	mNsmUnderCaseData::~mNsmUnderCaseData()
	{
	}

	mNsmUnderCaseData* mNsmUnderCaseData::copyNsmUnderCaseData(const int& id)
	{
		mNsmUnderCaseData* nsmUnderCaseData = new mNsmUnderCaseData(id);
		//caseData->_angle = _angle;
		//先不拷贝属性
		return nsmUnderCaseData;
	}


	MxFunctions::BrowserTreeStatus mNsmUnderCaseData::getNodeStatus(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";

		if (_nsmStrs.empty())
		{
			info.append({tip,"非结构质量为空"});
			return Tree_Failure;
		}
		return info.isEmpty() ? Tree_Yes : Tree_Failure;
	}


}
