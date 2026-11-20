#include "mBoundarySuperelement.h"

namespace MechCaseData
{
	mBoundarySuperelement::mBoundarySuperelement(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		_ConstraintType = "ASET";
		_DOF = { false,false ,false ,false ,false ,false };
	}

	mBoundarySuperelement::~mBoundarySuperelement()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mBoundarySuperelement::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mBoundarySuperelement> boundary = std::make_shared<mBoundarySuperelement>(id, getLoadBoundaryType(), name);
		boundary->_DOF = _DOF;
		boundary->_ConstraintType = _ConstraintType;
		return boundary;
	}

	MxFunctions::BrowserTreeStatus mBoundarySuperelement::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (!_DOF.contains(true))
		{
			info.append({ tip,"请勾选自由度" });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}