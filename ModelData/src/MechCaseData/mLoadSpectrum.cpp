#include "mLoadSpectrum.h"

namespace MechCaseData
{
	mLoadSpectrum::mLoadSpectrum(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		type_ = "";
	}

	mLoadSpectrum::~mLoadSpectrum()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadSpectrum::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadSpectrum> load = std::make_shared<mLoadSpectrum>(id, getLoadBoundaryType(), name);
		load->type_ = type_;
		load->dampValueList_ = dampValueList_;
		//load->spectrumCurveList_ = spectrumCurveList_;
		load->dampValueList_ = dampValueList_;

		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadSpectrum::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{

		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}