#include "mLoadEdge.h"

namespace MechCaseData
{
    mLoadEdge::mLoadEdge(int id, MxFunctions::LoadBoundaryType type, const QString& name)
        :mLoadBoundaryBase(id, type, name)
    {
        distributionType_ = "均匀分布";
        defineWay_ = "统一赋值";
        directionMode_ = "面内法向";
        size_ = "";
        dirPickState_ = DirectionPickState();
        dir_ = QVariant();
    }

    mLoadEdge::~mLoadEdge() = default;

    std::shared_ptr<mLoadBoundaryBase> mLoadEdge::copyLoadBoundary(const int& id, const QString& name)
    {
        std::shared_ptr<mLoadEdge> load = std::make_shared<mLoadEdge>(id, getLoadBoundaryType(), name);
        load->distributionType_ = distributionType_;
        load->defineWay_ = defineWay_;
        load->directionMode_ = directionMode_;
        load->size_ = size_;
        load->dirPickState_ = dirPickState_;
        load->dir_ = dir_;
        return load;
    }

    MxFunctions::BrowserTreeStatus mLoadEdge::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        QString tip = "警告";

        if (size_.isEmpty())
        {
            info.append({ tip, "载荷大小未设置" });
        }

        // 通用模式校验方向是否设置
        if (directionMode_ == "通用")
        {
            if (!dir_.isValid() || dir_.isNull())
            {
                info.append({ tip, "通用方向未设置" });
            }
        }

        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }

    QString mLoadEdge::getDirectionModeEng()
    {
        if (directionMode_ == "面内法向")
        {
            return "ednor";
        }
        else if (directionMode_ == "面外法向")
        {
            return "edtra";
        }
        else if (directionMode_ == "边切向")
        {
            return "edshr";
        }
        else if (directionMode_ == "通用")
        {
            return "edld";
        }
        else
        {
            return QString();
        }
    }
}