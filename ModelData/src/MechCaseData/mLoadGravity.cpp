#include "mLoadGravity.h"

namespace MechCaseData
{
	mLoadGravity::mLoadGravity(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
		, way_(0)
		, isCornerAddChecked(false)
	{
		V1 = QVariant();
		V2 = QVariant();
		V3 = QVariant();
		magnitude_ = QVariant();
		dir_ = QVariant();
	}

	mLoadGravity::~mLoadGravity()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadGravity::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadGravity> load = std::make_shared<mLoadGravity>(id, getLoadBoundaryType(), name);
		load->way_ = way_;
		load->dirPickState_ = dirPickState_;
		load->magnitude_ = magnitude_;
		load->dir_ = dir_;
		load->V1 = V1;
		load->V2 = V2;
		load->V3 = V3;
		load->isCornerAddChecked = isCornerAddChecked;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadGravity::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";
		QString inf = "方向未确定";
		bool hasDir = false;
		if (way_ == 0)
		{
			hasDir = (V1.isValid() && !V1.isNull()) ||
				(V2.isValid() && !V2.isNull()) ||
				(V3.isValid() && !V3.isNull());
		}
		else if (way_ == 1)
		{
			hasDir = dir_.isValid() && !dir_.isNull();
		}
		if (!hasDir)
		{
			info.append({ tip, inf });
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}