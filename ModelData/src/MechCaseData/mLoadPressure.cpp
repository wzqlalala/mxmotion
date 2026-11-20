#include "mLoadPressure.h"

namespace MechCaseData
{
	mLoadPressure::mLoadPressure(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		pressureType_ = 0;
		pa_ = QVariant();
		way_ = 0;
		cellFaceMap_.clear();
		path_ = "";
		depthDirection_ = QVector3D(1, 0, 0);
		depthDirPickState_ = DirectionPickState();
		depthDirPickState_.mode = 1; // mDirectionSelectorWidget::XAxis
		zeroDepth_ = QVariant();
		refDepth_ = QVariant();
		refPressure_ = QVariant();
	}

	mLoadPressure::~mLoadPressure()
	{

	}

	std::shared_ptr<mLoadBoundaryBase> mLoadPressure::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadPressure> load = std::make_shared<mLoadPressure>(id, getLoadBoundaryType(), name);
		load->pressureType_ = pressureType_;
		load->pa_ = pa_;
		load->way_ = way_;
		load->isAnalysisFieldMode_ = isAnalysisFieldMode_;
		load->cellFaceMap_ = cellFaceMap_;
		load->path_ = path_;
		load->depthDirection_ = depthDirection_;
		load->depthDirPickState_ = depthDirPickState_;
		load->zeroDepth_ = zeroDepth_;
		load->refDepth_ = refDepth_;
		load->refPressure_ = refPressure_;
		return load;
	}

	MxFunctions::BrowserTreeStatus mLoadPressure::getNodeStatu(QVector<QPair<QString, QString>>& info)
	{
		QString tip = "警告";

		if (pressureType_ == 0)
		{
			// 常规压力
			if (isAnalysisFieldMode_)
			{
				// 分析场：无大小概念，无需校验
			}
			else if (way_ == 0)
			{
				if (!pa_.isValid() || pa_.isNull())
				{
					info.append({ tip,"压强值未输入" });
				}
			}
			else if (way_ == 1)
			{
				if (cellFaceMap_.empty())
				{
					info.append({ tip,"单元面独立赋值未填写" });
				}
			}
			else if (way_ == 2)
			{
				if (path_.isEmpty())
				{
					info.append({ tip,"文件赋值未选择文件" });
				}
			}
		}
		else
		{
			// 净水压力
			if (depthDirection_.isNull())
			{
				info.append({ tip,"深度方向未输入" });
			}
			if (!zeroDepth_.isValid() || zeroDepth_.isNull())
			{
				info.append({ tip,"零压强深度未输入" });
			}
			if (!refDepth_.isValid() || refDepth_.isNull())
			{
				info.append({ tip,"参考深度未输入" });
			}
			if (!refPressure_.isValid() || refPressure_.isNull())
			{
				info.append({ tip,"参考压强值未输入" });
			}
		}
		return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
	}
}
