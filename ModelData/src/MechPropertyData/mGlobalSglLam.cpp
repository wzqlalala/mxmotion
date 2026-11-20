#include "mGlobalSglLam.h"

using namespace MxFunctions;
namespace MechPropertyData
{
	mGlobalSglLam::mGlobalSglLam(int id, LaminateType type, const QString& name)
		: mLaminateBase(id, type, name)
	{
		_theta = "0";
		_thick = "";
	}

	mGlobalSglLam::~mGlobalSglLam()
	{

	}

	mGlobalSglLam* MechPropertyData::mGlobalSglLam::copyLaminate(const int& id, const QString& name)
	{
		mGlobalSglLam* caseData = new mGlobalSglLam(id, getLaminateType(), name);
		caseData->_theta = _theta;
		caseData->_thick = _thick;
		return caseData;
	}
	MxFunctions::BrowserTreeStatus mGlobalSglLam::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (_thick.isEmpty())
		{
			info.append({ tip,"未填写厚度或厚度值无效" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
