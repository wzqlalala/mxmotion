#include "mLoadFreqdomain.h"

namespace MechCaseData
{
	mLoadFreqdomain::mLoadFreqdomain(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		delay_ = QVariant();
		phase_ = QVariant();
	}

	mLoadFreqdomain::~mLoadFreqdomain()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadFreqdomain::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadFreqdomain> load = std::make_shared<mLoadFreqdomain>(id, getLoadBoundaryType(), name);
		load->delay_ = delay_;
		load->phase_ = phase_;
		load->way_ = way_;
		load->way2_ = way2_;

		return load;
	}

	QString mLoadFreqdomain::getSelectLoadType()
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

	MxFunctions::BrowserTreeStatus mLoadFreqdomain::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		//延迟时间与相位角为可选项，不需要提示
		/*QString tip = "警告";
		if (!delay_.isValid() || delay_.isNull())
		{
			info.append({ tip,"延迟时间未输入" });
		}

		if (!phase_.isValid() || phase_.isNull())
		{
			info.append({ tip,"相位未输入" });
		}
		*/
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}