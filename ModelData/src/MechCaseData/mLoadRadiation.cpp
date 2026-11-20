#include "mLoadRadiation.h"

namespace MechCaseData
{
    mLoadRadiation::mLoadRadiation(int id, MxFunctions::LoadBoundaryType type, const QString& name)
        :mLoadBoundaryBase(id, type, name)
    {
        V1 = QVariant();
        V2 = QVariant();
        defineType_ = 0;
        cellFaceData_.clear();
    }

    mLoadRadiation::~mLoadRadiation()
    {

    }

    std::shared_ptr<mLoadBoundaryBase> mLoadRadiation::copyLoadBoundary(const int& id, const QString& name)
    {
        std::shared_ptr<mLoadRadiation> load = std::make_shared<mLoadRadiation>(id, getLoadBoundaryType(), name);
        load->V1 = V1;
        load->V2 = V2;
        load->defineType_ = defineType_;
        load->cellFaceData_ = cellFaceData_;
        return load;
    }

    MxFunctions::BrowserTreeStatus mLoadRadiation::getNodeStatu(QVector<QPair<QString, QString>>& info)
    {
        QString tip = "警告";
        if (defineType_ == 0)
        {
            if ((!V1.isValid() || V1.isNull()))
            {
                info.append({ tip, "环境温度未确定" });
            }
            if ((!V2.isValid() || V2.isNull()))
            {
                info.append({ tip, "热辐射系数未确定" });
            }
        }
        else
        {
            if (cellFaceData_.empty())
            {
                info.append({ tip, "独立赋值热辐射数据未填写" });
            }
        }
        return info.isEmpty() ? MxFunctions::BrowserTreeStatus::Tree_Yes : MxFunctions::BrowserTreeStatus::Tree_Failure;
    }
}