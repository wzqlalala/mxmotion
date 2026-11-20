#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetHighlightService.h（前处理拾取控件栈，dev_base@8d278d784）
// 后处理适配（非逐字复制）——临时高亮 v1 语义：
//   前处理版"临时高亮"走独立颜色渲染层（getExtraPre*HighLightRender，按 colorKey 隔离，
//   不触碰共享选中态）；后处理渲染侧暂无同构的按 key 隔离临时高亮层，v1 以共享选中容器
//   呈现临时高亮（mPostSectionForce 同款惯用法），即失活态数据与激活态数据共用容器、
//   颜色不可独立设置。后续若 mxr 提供 REBUILD_PostTempHighLightRender 的对外接入，
//   仅需重写本类 highlightSet/clearOwnTempHighlight/setColor 三个入口。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（适配点除外），不得单侧漂移。
// =============================================================================

#include <QObject>
#include <QString>
#include <QColor>

#include <unordered_map>
#include <vector>

#include "mpostwindow_global.h"
#include "mPostPickWidgetData.h"
#include "mPostPickHighlightServiceBase.h"

namespace PostWindow
{
    /**
     * @brief 通用拾取控件高亮服务（全功能）
     * 继承基础高亮能力，扩展集合临时高亮功能（v1 经共享选中容器呈现，见文件头说明）
     */
    class MPOSTWINDOW_EXPORT mPostPickHighlightService : public mPostPickHighlightServiceBase
    {
        Q_OBJECT

    public:
        explicit mPostPickHighlightService(QObject* parent = nullptr);
        ~mPostPickHighlightService() override;

        // 重写：设置临时高亮颜色（v1 无独立颜色层，空实现）
        void setColor(const QColor& color) override;

        // 重写：对集合数据进行临时高亮（v1 经共享选中容器，同 highlightPicked）
        void highlightSet(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data, bool on) override;

        // 重写：仅清空当前 colorKey 的临时高亮（v1 共享容器即全部，等同 clearAllHighlight）
        void clearOwnTempHighlight() override;

        // 重写：清空临时高亮 + 共享选中态
        void clearAllHighlight() override;
    };
}
