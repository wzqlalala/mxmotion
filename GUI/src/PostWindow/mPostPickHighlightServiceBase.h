#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetHighlightServiceBase.h（前处理拾取控件栈，dev_base@8d278d784）
// 后处理适配（非逐字复制）：
//   1. 渲染窗口类型 CCSMRendWidget*→REBUILD_PostRend*（两者为 mxr::mBaseRend 兄弟类，
//      后处理视口必须用 getPostRenderWidget()）；
//   2. 后处理无几何渲染链路：删除几何高亮分支（clearGeoHighlight/splitPartPicks 几何侧），
//      Part 类型在后处理全部为网格部件，直接走 MeshPart；
//   3. 高亮数据写入/清空走共享选中容器的 MeshSelectMemoryData（mPostSectionForce 等
//      后处理面板同款惯用法），刷新经 UpdatePostWidgetHighlightWithoutRTRC。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（适配点除外），不得单侧漂移。
// =============================================================================

#include "mpostwindow_global.h"

#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>

#include <unordered_map>
#include <vector>

#include "mPostPickWidgetData.h"

// Render
// post 侧增补（豁免项）：REBUILD_PostRend 仅作指针声明，改前向声明——本头经
// mPostVectorPlot.h 进入无 RebuildRender include 集的无头测试目标会 C1083；
// 完整类型由本类 .cpp 与解引用方（如 mPostPickWidgetController.h）自行包含
namespace mxr { class REBUILD_PostRend; }
#include "mxr/data/SelectDataStruct.h"
// PreData（后处理拾取公共函数，PostWindow 已链接 PreData）
#include "PickCommonFunction.h"

namespace PostWindow
{
    /**
     * @brief 后处理拾取高亮服务基类
     * 封装公共核心能力：渲染窗口管理、共享选中态高亮、刷新防抖、通用工具函数
     * 临时高亮相关接口提供默认空实现，派生类按需重写扩展
     */
    class MPOSTWINDOW_EXPORT mPostPickHighlightServiceBase : public QObject
    {
        Q_OBJECT

    public:
        explicit mPostPickHighlightServiceBase(QObject* parent = nullptr);
        ~mPostPickHighlightServiceBase() override = default;

        // 设置目标渲染窗口
        void setRenderWidget(mxr::REBUILD_PostRend* widget);

        // 设置颜色标识（基类统一存值，派生类按需使用）
        void setColorKey(const QString& colorKey);

        // 设置临时高亮颜色（基类空实现，需临时高亮的派生类重写）
        virtual void setColor(const QColor& color);

        // 对集合数据进行临时高亮（基类空实现，需临时高亮的派生类重写）
        virtual void highlightSet(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data, bool on);

        // 对拾取数据进行正常高亮（更新共享 SelectionContainer 选中态）
        virtual void highlightPicked(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& picks, bool on);

        // 仅清空自身临时高亮（基类空实现，需临时高亮的派生类重写）
        virtual void clearOwnTempHighlight();

        // 清空所有高亮（含共享选中态，切出面板通用）
        virtual void clearAllHighlight();

    protected:
        // 确保渲染窗口指针有效
        void ensureRenderWidget();

        // PickDataType 转渲染层 SelectionInfoType
        // 注意：Part 类型不在此映射，由 highlightPicked 单独走 MeshPart；
        // 几何类型（GeoPoint 等）后处理不存在，统一返回 MeshTypeEnd 跳过
        mxr::SelectionInfoType setTypeToSelectionInfoType(MxFunctions::PickDataType type) const;

        // 请求刷新高亮（防抖合并：短时间多次请求只执行一次）
        void requestUpdateHighlight();
        // 实际执行高亮刷新（由防抖定时器触发）
        void doUpdateHighlight();

public:
        // 立即执行一次待处理的防抖刷新（停表后同步执行 doUpdateHighlight）。
        // 供视口/模型切换清理后同步重画旧视口：防抖定时器触发时当前模型已切换，
        // doUpdateHighlight 的"非当前视口跳过"守卫会跳过旧视口，
        // 旧视口渲染层不按已清空的容器重建、不重画，残留切换前画面
        void flushHighlightNow();

    protected:
        mxr::REBUILD_PostRend* _widget = nullptr;
        QString _colorKey = "CBDefault";   // 颜色标识，基类统一存储
        QTimer* _updateTimer = nullptr;    // 高亮刷新防抖定时器
        bool _isTempHighlight = true;   //是否需要临时高亮
    };
}
