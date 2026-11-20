#include "mLoadAcousticFrequencyDomain.h"

namespace MechCaseData
{
	mLoadAcousticFrequencyDomain::mLoadAcousticFrequencyDomain(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		delay_ = QVariant();
		xita_ = QVariant();
		rho_ = QVariant();
		B_ = QVariant();
	}

	mLoadAcousticFrequencyDomain::~mLoadAcousticFrequencyDomain()
	{
	}

	std::shared_ptr<mLoadBoundaryBase> mLoadAcousticFrequencyDomain::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadAcousticFrequencyDomain> load = std::make_shared<mLoadAcousticFrequencyDomain>(id, getLoadBoundaryType(), name);
		load->delay_ = delay_;
		load->xita_ = xita_;
		load->rho_ = rho_;
		load->B_ = B_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadAcousticFrequencyDomain::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		//延迟时间和相位为可选，不需判断
		QString tip = "警告";
		QString inf = "";
		if ((!rho_.isValid() || rho_.isNull()))
		{
			inf = "声场密度未确定";
			info.append({ tip, inf });
		}

		if ((!B_.isValid() || B_.isNull()))
		{
			inf = "体积模量未确定";
			info.append({ tip, inf });

		}

		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}