#pragma once
#pragma execution_character_set("utf-8")

#ifdef MX_MOTION_OCC_COMPAT
#include "picking/OccPickController.h"
#else

#include <QObject>
#include <QString>
#include <QColor>

#include <unordered_map>
#include <vector>

#include "mmainwindow_global.h"
#include "mPickWidgetData.h"
#include "mPickWidgetHighlightServiceBase.h"

namespace MechMainWindow
{
    /**
     * @brief 通用拾取控件高亮服务（全功能）
     * 继承基础高亮能力，扩展集合临时高亮功能，支持独立颜色标识的临时高亮渲染
     */
    class MMAINWINDOW_EXPORT mPickWidgetHighlightService : public mPickWidgetHighlightServiceBase
    {
        Q_OBJECT

    public:
        explicit mPickWidgetHighlightService(QObject* parent = nullptr);
        ~mPickWidgetHighlightService() override;

        // 重写：设置临时高亮颜色（RGB）
        void setColor(const QColor& color) override;

        // 重写：对集合数据进行临时高亮（不写入共享选中态）
        void highlightSet(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data, bool on) override;

        // 重写：仅清空当前 colorKey 的临时高亮，不触碰共享选中态
        void clearOwnTempHighlight() override;

        // 重写：清空临时高亮 + 共享选中态
        void clearAllHighlight() override;

        QColor _color;
    };
}
#endif // MX_MOTION_OCC_COMPAT
