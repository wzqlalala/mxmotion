#pragma once
#pragma execution_character_set("utf-8")
#include "MXCaseData_global.h"
#include "mPhysicalFieldEnum.h"
#include "mLoadBoundaryBase.h"
#include "mModelStruct.h"

#include <QString>
#include <QVector3D>
#include <QVariant>
#include <QVector>
#include <map>

namespace MechCaseData
{
    class MXCASEDATA_EXPORT mLoadSurface : public mLoadBoundaryBase
    {
    public:
        mLoadSurface(int id, MxFunctions::LoadBoundaryType type, const QString& name = "");
        mLoadSurface() {}
        ~mLoadSurface();

        std::shared_ptr<mLoadBoundaryBase> copyLoadBoundary(const int& id, const QString& name) override;
        MxFunctions::BrowserTreeStatus getNodeStatu(QVector<QPair<QString, QString>>& info) override;

        friend QDataStream& operator>>(QDataStream& in, mLoadSurface& obj);
        friend QDataStream& InStreammLoadSurfaceV3(QDataStream& in, mLoadSurface& obj);
        friend QDataStream& operator<<(QDataStream& out,mLoadSurface& obj);

    public:
        // 分布类型：0=均匀分布
        int distributeType_;

        // 定义方式：0=统一赋值, 1=独立赋值
        int way_;

        // 方向模式：0=表面切向, 1=通用
        int dirMode_;

        // 统一赋值：载荷值（Pa，内部存储为国际单位 Pa）
        QVariant loadValue_;

        // 统一赋值：方向控件完整 UI 状态（面板反显用）
        DirectionPickState dirPickState_;

        // 独立赋值表格数据
        // key: 单元面拾取结构体（单元ID+面ID唯一标识）
        // value: [压强值, v1, v2, v3]，无值存空 QVariant
        std::map<RenderPickResultData, QVector<QVariant>> surfaceForceMap_;
    };
}