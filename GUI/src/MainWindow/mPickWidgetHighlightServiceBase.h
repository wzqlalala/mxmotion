#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QObject>
#include <QString>
#include <QColor>
#include <QTimer>

#include <unordered_map>
#include <vector>

#include "mPickWidgetData.h"

// Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/mPreGeoHighLightRender.h"
#include "mxr/view/CCSMMeshHighLightRender.h"
#include "PickCommonFunction.h"

namespace MechMainWindow
{
    /**
     * @brief 高亮服务基类
     * 封装公共核心能力：渲染窗口管理、共享选中态高亮、刷新防抖、通用工具函数
     * 临时高亮相关接口提供默认空实现，派生类按需重写扩展
     */
    class MMAINWINDOW_EXPORT mPickWidgetHighlightServiceBase : public QObject
    {
        Q_OBJECT

    public:
        explicit mPickWidgetHighlightServiceBase(QObject* parent = nullptr);
        ~mPickWidgetHighlightServiceBase() override = default;

        // 设置目标渲染窗口
        void setRenderWidget(mxr::CCSMRendWidget* widget);

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

        // 高亮抑制开关（云图模式）：开启后防抖刷新不再从共享选中容器重画高亮，
        // 拾取数据本身不受影响，由调用方负责清空已有高亮显示
        void setHighlightSuppressed(bool suppress) { _highlightSuppressed = suppress; }
        bool isHighlightSuppressed() const { return _highlightSuppressed; }

    protected:
        // 确保渲染窗口指针有效
        void ensureRenderWidget();

        // PickDataType 转渲染层 SelectionInfoType
        // 注意：Part 类型不在此映射，需结合 part_id 区分几何/网格，由调用处走 splitPartPicks
        mxr::SelectionInfoType setTypeToSelectionInfoType(MxFunctions::PickDataType type) const;

        // Part 类型拾取按模型数据区分几何 part 与网格 part（判断逻辑同 PickCommonFunction::addPreWidgetSelectPartID）
        void splitPartPicks(const std::vector<RenderPickResultData>& parts,
            std::vector<int>& geoPartIds,
            std::vector<mxr::MeshSelectInfoData>& meshPartInfos) const;

        // 通用清空工具：清空几何高亮渲染器
        void clearGeoHighlight(mxr::mPreGeoHighLightRender* geoRender);
        // 通用清空工具：清空网格高亮渲染器
        void clearMeshHighlight(mxr::CCSMMeshHighLightRender* meshRender);

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
        mxr::CCSMRendWidget* _widget = nullptr;
        QString _colorKey = "CBDefault";   // 颜色标识，基类统一存储
        QTimer* _updateTimer = nullptr;    // 高亮刷新防抖定时器
        bool _isTempHighlight = true;   //是否需要临时高亮
        bool _highlightSuppressed = false; // 高亮抑制（云图模式）：抑制从共享选中容器重画高亮
    };
}