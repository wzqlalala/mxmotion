#include "mLoadHeatFlux.h"

namespace MechCaseData
{
    mLoadHeatFlux::mLoadHeatFlux(int id, MxFunctions::LoadBoundaryType type, const QString& name)
        :mLoadBoundaryBase(id, type, name)
    {
        pa_ = QVariant();
        way_ = 0;
        defineType_ = 0;
        cellFaceData_.clear();
    }

    mLoadHeatFlux::~mLoadHeatFlux()
    {

    }

    std::shared_ptr<mLoadBoundaryBase> mLoadHeatFlux::copyLoadBoundary(const int& id, const QString& name)
    {
        std::shared_ptr<mLoadHeatFlux> load = std::make_shared<mLoadHeatFlux>(id, getLoadBoundaryType(), name);
        load->pa_ = pa_;
        load->way_ = way_;
        load->defineType_ = defineType_;
        load->cellFaceData_ = cellFaceData_;
        return load;
    }

    MxFunctions::BrowserTreeStatus mLoadHeatFlux::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        QString tip = "警告";
        if (defineType_ == 0)
        {
            if (!pa_.isValid() || pa_.isNull())
            {
                info.append({ "警告","热流密度大小未输入" });
            }
        }
        else
        {
            if (cellFaceData_.empty())
            {
                info.append({ "警告","热流密度独立赋值未填写" });
            }
        }
        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }
}