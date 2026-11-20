#include "mLoadBolt.h"
namespace MechCaseData
{
	mLoadBolt::mLoadBolt(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		way_ = 0;
		value_ = QVariant();
	}

	mLoadBolt::~mLoadBolt()
	{
	}

	std::shared_ptr<mLoadBoundaryBase> mLoadBolt::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadBolt> load = std::make_shared<mLoadBolt>(id, getLoadBoundaryType(), name);
		load->way_ = way_;
		load->value_ = value_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadBolt::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		if (way_ == 0|| way_ == 1)
		{
			if (!value_.isValid() || value_.isNull())
			{
				QString inf = "值未输入";
				info.append({ tip,inf });
			}
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}