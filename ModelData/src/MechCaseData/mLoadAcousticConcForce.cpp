#include "mLoadAcousticConcForce.h"

namespace MechCaseData
{
	mLoadAcousticConcForce::mLoadAcousticConcForce(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		value_ = QVariant();
	}

	mLoadAcousticConcForce::~mLoadAcousticConcForce()
	{
	}

	std::shared_ptr<mLoadBoundaryBase> mLoadAcousticConcForce::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadAcousticConcForce> load = std::make_shared<mLoadAcousticConcForce>(id, getLoadBoundaryType(), name);
		load->value_ = value_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadAcousticConcForce::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		QString inf = "";
		if ((!value_.isValid() || value_.isNull()))
		{
			inf = "大小未确定";
			info.append({ tip, inf });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}