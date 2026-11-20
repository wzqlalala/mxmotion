#include "mDirectPickWidgetView.h"

namespace MechMainWindow
{
    mDirectPickWidgetView::mDirectPickWidgetView(mColorBlock colorBlock,mPickMode pickMode, QWidget* parent)
        : mPickWidgetViewBase(pickMode, parent)
    {
        // 基类已完成全部UI初始化、事件过滤、交互逻辑
        // 这里只需按需设置默认提示、Tooltip等即可
        // 示例：setDefultTip("请点击拾取对象");
        // 示例：setRoundTooltipText("点击激活拾取");
        setColorBlock(colorBlock); // 调用基类的颜色块设置函数
    }

    mDirectPickWidgetView::~mDirectPickWidgetView() = default;
}