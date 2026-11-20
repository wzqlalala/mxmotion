#include "ViewportLayoutEngine.h"
#include "ViewportContainer.h"
#include "mViewBrowserWindow.h"

namespace MechMainWindow {

ViewportLayoutEngine::ViewportLayoutEngine(ViewportContainer* container)
    : QObject(container)
    , _container(container)
{
}

ViewportLayoutEngine::LayoutResult ViewportLayoutEngine::calculateLayout(
    int layoutType, const QSize& containerSize, const QList<int>& viewportIDs)
{
    LayoutResult result;
    result.usedSlots = 0;
    result.totalSlots = 0;

    // 布局定义复用 viewLayoutTable（与视图树/视图显示栏一致）
    const auto& table = viewLayoutTable();
    if (layoutType < 0 || layoutType >= table.size()) {
        layoutType = 0;  // 默认单视口
    }
    const mViewLayoutDef& def = table[layoutType];

    result.totalSlots = viewLayoutSlotCount(def);
    for (int i = 0; i < result.totalSlots; ++i) {
        LayoutCell cell;
        cell.slotIndex = i;
        cell.geometry = viewLayoutCellRect(def, i, containerSize);
        cell.viewportID = (i < viewportIDs.size()) ? viewportIDs[i] : -1;
        if (cell.viewportID != -1) result.usedSlots++;
        result.cells.append(cell);
    }
    return result;
}

} // namespace MechMainWindow
