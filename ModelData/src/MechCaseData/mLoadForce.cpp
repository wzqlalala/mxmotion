#include "mLoadForce.h"

namespace MechCaseData
{
	mLoadForce::mLoadForce(int id, MxFunctions::LoadBoundaryType type, const QString& name)
		:mLoadBoundaryBase(id, type, name)
	{
		way_ = 0;
		dirPickState_ = DirectionPickState();
		force_ = QVariant();
		force_x = QVariant();
		force_y = QVariant();
		force_z = QVariant();
		dir_ = QVariant();
	}

	mLoadForce::~mLoadForce() = default;

	std::shared_ptr<mLoadBoundaryBase> mLoadForce::copyLoadBoundary(const int& id, const QString& name)
	{
		std::shared_ptr<mLoadForce> load = std::make_shared<mLoadForce>(id, getLoadBoundaryType(), name);
		load->way_ = way_;
		load->isAnalysisFieldMode_ = isAnalysisFieldMode_;
		load->dirPickState_ = dirPickState_;
		load->force_ = force_;
		load->force_x = force_x;
		load->force_y = force_y;
		load->force_z = force_z;
		load->dir_ = dir_;
		load->nodeForceMap_ = nodeForceMap_;
		load->path_ = path_;
		return load;
	}

    MxFunctions::BrowserTreeStatus mLoadForce::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        bool isForce = (getLoadBoundaryType() == LOAD_IM_FORCE);
        int fileWay = isForce ? 3 : 2;
        int independentWay = isForce ? 2 : 1;
        int dirAndMagWay = 1; // 仅集中力有方向与大小


        QString tip = "警告";

        if (way_ == 0) // 各向分量
        {
            if ((!force_x.isValid() || force_x.isNull()) &&
                (!force_y.isValid() || force_y.isNull()) &&
                (!force_z.isValid() || force_z.isNull()))
            {
                info.append({ tip, "至少需要输入一个方向的分量" });
            }
        }


        if (isForce && way_ == dirAndMagWay) // 方向与大小
        {
            if (!force_.isValid() || force_.isNull())
            {
                info.append({ tip, "载荷大小未设置" });
            }
            if (!dir_.isValid() || dir_.isNull())
            {
                info.append({ tip, "载荷方向未选择" });
            }
        }

        if (way_ == independentWay) // 独立赋值
        {
            if (nodeForceMap_.empty())
            {
                info.append({ tip, "未设置任何节点载荷数据" });
            }
        }

        if (isAnalysisFieldMode_) // 分析场
        {
            return MxFunctions::BrowserTreeStatus::Tree_Yes;
        }

        if (way_ == fileWay) // 文件
        {
            if (path_.isEmpty())
            {
                info.append({ tip, "文件路径未设置" });
                return MxFunctions::BrowserTreeStatus::Tree_Failure;
            }
            return MxFunctions::BrowserTreeStatus::Tree_Yes;
        }
        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }
}
