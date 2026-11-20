#include "mLoadCombined.h"

namespace MechCaseData
{
	mLoadCombined::mLoadCombined(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		ktotal_ = QVariant();
	}

	mLoadCombined::~mLoadCombined()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadCombined::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadCombined> load = std::make_shared<mLoadCombined>(id, getLoadBoundaryType(), name);
		load->ktotal_ = ktotal_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadCombined::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (!ktotal_.isValid() || ktotal_.isNull())
		{
			info.append({ tip,"总比例系数未输入" });
		}

		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}