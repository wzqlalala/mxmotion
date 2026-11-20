#pragma once

#include <QObject>
#include <QRect>
#include <QVector>
#include <QList>

namespace MechMainWindow {

class ViewportContainer;

/**
 * @brief 视口布局引擎，负责计算视口几何位置
 *
 * 布局定义复用 mViewBrowserWindow.h 中的 viewLayoutTable()（条带模型），
 * 与视图显示栏/视图树共用同一份布局表，保证格子几何一致。
 * 引擎本身无状态：布局类型与视口列表均由调用方传入。
 */
class ViewportLayoutEngine : public QObject
{
    Q_OBJECT

public:
    struct LayoutCell {
        int slotIndex;      // 格子序号
        QRect geometry;     // 几何矩形
        int viewportID;     // 分配的视口ID，-1为空
    };

    struct LayoutResult {
        QVector<LayoutCell> cells;
        int usedSlots;      // 已使用格子数
        int totalSlots;     // 总格子数
    };

    explicit ViewportLayoutEngine(ViewportContainer* container);

    // 布局计算
    // layoutType：布局类型（viewLayoutTable 下标，越界按单视口处理）
    // viewportIDs：可见视口列表，按顺序填入格子
    LayoutResult calculateLayout(int layoutType, const QSize& containerSize,
                                  const QList<int>& viewportIDs);

private:
    ViewportContainer* _container;
};

} // namespace MechMainWindow
