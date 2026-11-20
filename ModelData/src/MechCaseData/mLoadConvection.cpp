#include "mLoadConvection.h"
#include <QDataStream>

namespace MechCaseData
{
	mLoadConvection::mLoadConvection(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		V1 = QVariant();
		V2 = QVariant();
		defineType_ = 0;
		cellFaceData_.clear();
	}

	mLoadConvection ::~mLoadConvection()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadConvection::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadConvection > load = std::make_shared<mLoadConvection >(id, getLoadBoundaryType(), name);
		load->V1 = V1;
		load->V2 = V2;
		load->defineType_ = defineType_;
		load->cellFaceData_ = cellFaceData_;
		load->path_ = path_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadConvection::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		if (defineType_ == 0)
		{
			if ((!V1.isValid() || V1.isNull()))
			{
				info.append({ "警告", "环境温度未确定" });
			}

			if ((!V2.isValid() || V2.isNull()))
			{
				info.append({ "警告", "对流换热系数未确定" });
			}
		}
		else if (defineType_ == 1)
		{
			if (cellFaceData_.empty())
			{
				info.append({ "警告","对流换热独立赋值未填写" });
			}
		}
		else
		{
			if (path_.isEmpty())
			{
				info.append({ "警告","对流换热文件定义未选择文件" });
			}
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}