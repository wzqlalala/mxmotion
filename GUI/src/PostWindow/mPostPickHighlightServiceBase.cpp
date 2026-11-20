#include "mPostPickHighlightServiceBase.h"

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetHighlightServiceBase.cpp（前处理拾取控件栈，dev_base@8d278d784）
// 后处理适配（非逐字复制）：渲染窗口换 getPostRenderWidget()；拾取数据经
// PickData2SelectData 统一转换后写入共享选中容器 MeshSelectMemoryData；几何高亮分支
// 删除（后处理仅网格链路）；刷新走 UpdatePostWidgetHighlightWithoutRTRC（避免自身刷新
// 触发 ReadyToReadContainer 回读环路）。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（适配点除外），不得单侧漂移。
// =============================================================================

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
#include "mxr/view/REBUILD_PostRend.h"   // 头文件改前向声明后完整类型由本 cpp 自含

using namespace MechPreData;
namespace PostWindow
{
    mPostPickHighlightServiceBase::mPostPickHighlightServiceBase(QObject* parent)
        : QObject(parent)
    {
        // 0ms 单次定时器：合并同一事件循环内多次高亮请求，避免批量操作卡顿
        _updateTimer = new QTimer(this);
        _updateTimer->setSingleShot(true);
        _updateTimer->setInterval(0);
        connect(_updateTimer, &QTimer::timeout, this, &mPostPickHighlightServiceBase::doUpdateHighlight);
    }

    void mPostPickHighlightServiceBase::setRenderWidget(mxr::REBUILD_PostRend* widget)
    {
        _widget = widget;
    }

    void mPostPickHighlightServiceBase::setColorKey(const QString& colorKey)
    {
        _colorKey = colorKey;
    }

    void mPostPickHighlightServiceBase::setColor(const QColor& color)
    {
        // 基类无临时高亮渲染器，空实现
        Q_UNUSED(color);
    }

    void mPostPickHighlightServiceBase::ensureRenderWidget()
    {
        //模型全删后 getCVM 为空：置空 _widget，后续接口按空渲染窗口兜底
        _widget = getCVM ? getCVM->getPostRenderWidget() : nullptr;
    }

    mxr::SelectionInfoType mPostPickHighlightServiceBase::setTypeToSelectionInfoType(MxFunctions::PickDataType type) const
    {
        switch (type)
        {
        case PickDataType::Node:            return mxr::SelectionInfoType::MeshNode;
        case PickDataType::Element:         return mxr::SelectionInfoType::MeshElement;
        case PickDataType::ElementFace:     return mxr::SelectionInfoType::MeshFace;
        case PickDataType::ElementEdge:     return mxr::SelectionInfoType::MeshLine;
        // Part 由 highlightPicked 单独走 MeshPart（后处理部件全部为网格部件）
        // Geo* 后处理不存在渲染链路，返回 MeshTypeEnd 跳过
        default:                           return mxr::SelectionInfoType::MeshTypeEnd;
        }
    }

    void mPostPickHighlightServiceBase::highlightSet(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        bool on)
    {
        // 基类无临时高亮能力，空实现
        Q_UNUSED(data);
        Q_UNUSED(on);
    }

    void mPostPickHighlightServiceBase::highlightPicked(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks,
        bool on)
    {
        ensureRenderWidget();
        if (!_widget || !_widget->getSelectionContainer()) return;

        // 拾取数据写入共享选中态容器，与视口交互拾取表现完全一致
        auto* meshSelectData = _widget->getSelectionContainer()->getMeshSelectData();
        const auto highlightOp = on ? mxr::SelectionOperate::ADD : mxr::SelectionOperate::REDUCE;
        for (const auto& pair : picks)
        {
            // 后处理 Part 全部为网格部件，直接写 MeshPart
            const mxr::SelectionInfoType infoType = (pair.first == MxFunctions::PickDataType::Part)
                ? mxr::SelectionInfoType::MeshPart
                : setTypeToSelectionInfoType(pair.first);
            if (infoType == mxr::SelectionInfoType::MeshTypeEnd)
                continue;

            std::vector<mxr::MeshSelectInfoData> meshIds;
            meshIds.reserve(pair.second.size());
            for (const auto& d : pair.second)
            {
                meshIds.push_back(MechPreData::PickData2SelectData(infoType, d));
            }
            meshSelectData->UpdateSelectDataByType(highlightOp, infoType, meshIds);
        }

        requestUpdateHighlight();
    }

    void mPostPickHighlightServiceBase::clearOwnTempHighlight()
    {
        // 基类无独立临时高亮渲染器，空实现
    }

    void mPostPickHighlightServiceBase::clearAllHighlight()
    {
        ensureRenderWidget();
        if (!_widget || !_widget->getSelectionContainer()) return;

        // 清空共享选中态容器
        MechPreData::ClearPostWidgetMeshPick(_widget);
        requestUpdateHighlight();
    }

    void mPostPickHighlightServiceBase::requestUpdateHighlight()
    {
        // 定时器已激活则跳过，合并为一次刷新
        if (_updateTimer && !_updateTimer->isActive())
        {
            _updateTimer->start();
        }
    }

    void mPostPickHighlightServiceBase::flushHighlightNow()
    {
        if (_updateTimer && _updateTimer->isActive())
        {
            _updateTimer->stop();
        }
        doUpdateHighlight();
    }

    void mPostPickHighlightServiceBase::doUpdateHighlight()
    {
        if (!_widget) return;
        // _widget 均来源于 getPostRenderWidget()（见 ensureRenderWidget/setRenderWidget）。
        // 定时器触发时后处理视口可能已被删除，_widget 悬空；与当前视口不一致则跳过。
        // 模型全删后 getCVM 为空，同样跳过（避免空指针解引用）。
        if (!getCVM || _widget != getCVM->getPostRenderWidget()) return;
        // 不发 ReadyToReadContainer：自身刷新发出的通知会回到控制器回读槽，
        // 形成"刷新→回读→置数→刷新"环路（前处理侧同款理由，见 UpdatePreWidgetHighlightWithoutRTRC）
        MechPreData::UpdatePostWidgetHighlightWithoutRTRC(_widget);
        _widget->updatePaint();
    }
}
