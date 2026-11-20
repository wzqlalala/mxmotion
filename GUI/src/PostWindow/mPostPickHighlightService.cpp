#include "mPostPickHighlightService.h"

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetHighlightService.cpp（前处理拾取控件栈，dev_base@8d278d784）
// 后处理适配（非逐字复制）：临时高亮 v1 经共享选中容器呈现（详见头文件说明），
// 不再持有独立临时高亮渲染层。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（适配点除外），不得单侧漂移。
// =============================================================================

namespace PostWindow
{
    mPostPickHighlightService::mPostPickHighlightService(QObject* parent)
        : mPostPickHighlightServiceBase(parent)
    {
    }

    mPostPickHighlightService::~mPostPickHighlightService() = default;

    void mPostPickHighlightService::setColor(const QColor& color)
    {
        // v1 无独立临时高亮颜色层：临时高亮与正式高亮共用共享选中容器的呈现，
        // 控件色块颜色仅用于 UI 侧标识，不影响视口高亮颜色
        Q_UNUSED(color);
    }

    void mPostPickHighlightService::highlightSet(
        const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data,
        bool on)
    {
        // v1：临时高亮与正式高亮共用共享选中容器写入路径
        highlightPicked(data, on);
    }

    void mPostPickHighlightService::clearOwnTempHighlight()
    {
        // v1：临时高亮即共享容器数据，清空等同清空全部
        mPostPickHighlightServiceBase::clearAllHighlight();
    }

    void mPostPickHighlightService::clearAllHighlight()
    {
        // v1 临时高亮与共享选中态同源，一次清空即可
        mPostPickHighlightServiceBase::clearAllHighlight();
    }
}
