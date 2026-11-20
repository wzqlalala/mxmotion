#include "mLoadTransient.h"

namespace MechCaseData
{
	mLoadTransient::mLoadTransient(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		pa_ = QVariant();
	}

	mLoadTransient::~mLoadTransient()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadTransient::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadTransient> load = std::make_shared<mLoadTransient>(id, getLoadBoundaryType(), name);
		load->pa_ = pa_;
		load->way_ = way_;

		return load;
	}

	QString mLoadTransient::getSelectLoadType()
	{
		if (way_ == 0)
		{
			return QString("load");
		}
		else if (way_ == 1)
		{
			return QString("disp");
		}		
		else if (way_ == 2)
		{
			return QString("vel");
		}		
		else if (way_ == 3)
		{
			return QString("acc");
		}
	}

	MxFunctions::BrowserTreeStatus mLoadTransient::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		//时域载荷延迟时间也为非必须
		/*QString tip = "警告";
		if (!pa_.isValid() || pa_.isNull())
		{
			info.append({ tip,"延迟时间未输入" });
		}*/
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}