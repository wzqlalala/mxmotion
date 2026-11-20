#include "mLoadSurface.h"

namespace MechCaseData
{
    mLoadSurface::mLoadSurface(int id, MxFunctions::LoadBoundaryType type, const QString& name)
        : mLoadBoundaryBase(id, type, name)
    {
        distributeType_ = 0;
        way_ = 0;
        dirMode_ = 0;
        loadValue_ = QVariant();
        dirPickState_ = DirectionPickState();
        surfaceForceMap_.clear();
    }

    mLoadSurface::~mLoadSurface()
    {
    }

    std::shared_ptr<mLoadBoundaryBase> mLoadSurface::copyLoadBoundary(const int& id, const QString& name)
    {
        std::shared_ptr<mLoadSurface> load = std::make_shared<mLoadSurface>(id, getLoadBoundaryType(), name);
        load->distributeType_ = distributeType_;
        load->way_ = way_;
        load->dirMode_ = dirMode_;
        load->loadValue_ = loadValue_;
        load->dirPickState_ = dirPickState_;
        load->surfaceForceMap_ = surfaceForceMap_; // 直接整体复制
        return load;
    }

    MxFunctions::BrowserTreeStatus mLoadSurface::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        QString tip = "警告";

        if (way_ == 0) // 统一赋值
        {
            if (!loadValue_.isValid() || loadValue_.isNull())
            {
                info.append({ tip, "载荷值未设置" });
            }
        }
        else if (way_ == 1) // 独立赋值
        {
            if (surfaceForceMap_.empty())
            {
                info.append({ tip, "未设置任何单元面载荷数据" });
            }
        }
        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }
}