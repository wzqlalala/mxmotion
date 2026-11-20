#pragma once
#pragma execution_character_set("utf-8")

#include "mPickWidgetViewBase.h"
#include "mmainwindow_global.h"

namespace MechMainWindow
{
    /**
     * @brief 极简拾取控件：仅保留核心拾取展示+清空功能
     * 无颜色块、无集合选择、无详情弹窗、无计算按钮
     */
    class MMAINWINDOW_EXPORT mDirectPickWidgetView : public mPickWidgetViewBase
    {
        Q_OBJECT

    public:
        explicit mDirectPickWidgetView(
            mColorBlock colorBlock = CBNone,
            mPickMode pickMode = PickMore,
            QWidget* parent = nullptr);
        ~mDirectPickWidgetView() override;
    };
}