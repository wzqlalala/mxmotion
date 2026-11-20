#include "mPostDirectPickWidgetView.h"

// =============================================================================
// 复制自 src/MainWindow/mDirectPickWidgetView.cpp（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：类名/命名空间替换，其余保持一致。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

namespace PostWindow
{
    mPostDirectPickWidgetView::mPostDirectPickWidgetView(mColorBlock colorBlock, mPickMode pickMode, QWidget* parent)
        : mPostPickWidgetViewBase(pickMode, parent)
    {
        // 基类已完成全部UI初始化、事件过滤、交互逻辑
        // 这里只需按需设置默认提示、Tooltip等即可
        // 示例：setDefultTip("请点击拾取对象");
        // 示例：setRoundTooltipText("点击激活拾取");
        setColorBlock(colorBlock); // 调用基类的颜色块设置函数
    }

    mPostDirectPickWidgetView::~mPostDirectPickWidgetView() = default;
}
