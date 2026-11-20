#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mDirectPickWidgetView.h（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：类名 mDirectPickWidgetView→mPostDirectPickWidgetView；导出宏/命名空间替换；
// 其余保持一致。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

#include "mPostPickWidgetViewBase.h"
#include "mpostwindow_global.h"

namespace PostWindow
{
    /**
     * @brief 极简拾取控件：仅保留核心拾取展示+清空功能
     * 无颜色块、无集合选择、无详情弹窗、无计算按钮
     */
    class MPOSTWINDOW_EXPORT mPostDirectPickWidgetView : public mPostPickWidgetViewBase
    {
        Q_OBJECT

    public:
        explicit mPostDirectPickWidgetView(
            mColorBlock colorBlock = CBNone,
            mPickMode pickMode = PickMore,
            QWidget* parent = nullptr);
        ~mPostDirectPickWidgetView() override;
    };
}
