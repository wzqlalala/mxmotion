#include "mLoadConcentration.h"

namespace MechCaseData
{
	mLoadConcentration::mLoadConcentration(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		F1_ = 0;
		F2_ = 0;
		F3_ = 0;
	}

	mLoadConcentration::~mLoadConcentration()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadConcentration::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadConcentration> load = std::make_shared<mLoadConcentration>(id, getLoadBoundaryType(), name);
		load->F1_ = F1_;
		load->F2_ = F2_;
		load->F3_ = F3_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadConcentration::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		return MxFunctions::BrowserTreeStatus::Tree_Yes;
	}

}