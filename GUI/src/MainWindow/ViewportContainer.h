#pragma once

#include <QWidget>
#include <QHash>
#include <QList>
#include <QSet>

namespace MechMainWindow {

class ViewportWidget;
class ViewportLayoutEngine;

/**
 * @brief 视口容器，管理视口部件的几何排列、显隐与激活
 *
 * 核心设计原则：
 * 1. 容器不持有页概念：页归属/页布局类型的权威数据在视图树，
 *    布局由树驱动（applyLayout 传入布局类型与可见视口列表）
 * 2. 激活完全手动控制，无自动行为
 * 3. resize 只重算几何：按 _currentLayoutType 与当前可见部件重放布局
 */
class ViewportContainer : public QWidget
{
    Q_OBJECT

public:
    explicit ViewportContainer(QWidget* parent = nullptr);
    ~ViewportContainer() override;

    // ========== 视口管理 ==========

    /**
     * @brief 添加视口（初始可见，显隐由后续布局驱动纠正）
     * @param viewportID 全局唯一ID
     * @param content 内容部件
     * @return 创建的视口部件
     */
    ViewportWidget* addViewport(int viewportID, QWidget* content);

    /**
     * @brief 移除视口（删除的是激活视口时仅复位激活状态，
     *        激活转移由视图树/管理器统一决定，容器不参与决策）
     */
    void removeViewport(int viewportID);

    /**
     * @brief 获取视口
     */
    ViewportWidget* viewport(int viewportID) const;
    ViewportWidget* viewportAt(const QPoint& pos) const;
    QList<ViewportWidget*> allViewports() const;

    // ========== 显示控制 ==========

    /**
     * @brief 设置视口可见性
     * @note 纯粹控制显示，不影响激活状态，不发出任何信号
     */
    void setViewportVisible(int viewportID, bool visible);

    // ========== 激活控制 ==========

    /**
     * @brief 设置激活视口
     * @param viewportID 目标视口ID，-1表示取消所有激活
     * @param emitSignal 是否发出激活信号
     * @param force 即使视口不可见也强制激活
     * @note 单视口布局（当页仅显示一个窗口）下激活未占格视口时换入最大化，
     *       不按旧几何浮显在占格视口上方
     */
    void setActiveViewport(int viewportID, bool emitSignal = true, bool force = false);

    /**
     * @brief 当前激活视口ID，-1表示无激活
     */
    int activeViewportID() const { return _activeViewportID; }

    /**
     * @brief 当前激活视口
     */
    ViewportWidget* activeViewport() const;

    // ========== 布局 ==========

    /**
     * @brief 应用布局（由视图树驱动，或激活换入时由容器调用）
     * @param layoutType 布局类型（viewLayoutTable 下标）
     * @param viewportIDs 可见视口列表（含溢出隐藏项）。顺序来源：
     *        - 树驱动布局（切页/切布局/新建）：按树节点顺序，激活视口超出格数时换入尾格
     *        - 激活换入（setActiveViewport）：替换当前激活格/尾格，其余格不动
     *        列表外视口全部隐藏
     * @note 容器记录 _currentLayoutType 与 _laidOutViewportIDs 供 resize 重放几何
     */
    void applyLayout(int layoutType, const QList<int>& viewportIDs);

    /**
     * @brief 是否在最近一次 applyLayout 的实际呈现集内
     * @note 呈现集 = 上次布局中被 setVisible(true) 的视口（占格项）。
     *       物化挂载链据此守卫显隐：不在呈现集的卡片（溢出隐藏/他页）
     *       不得挂载即显示，否则会以建卡初始几何浮于当前布局之上
     */
    bool isViewportLayoutShown(int viewportID) const
    {
        return _layoutShownViewportIDs.contains(viewportID);
    }

    /**
     * @brief 设置布局引擎
     */
    void setLayoutEngine(ViewportLayoutEngine* engine);
    ViewportLayoutEngine* layoutEngine() const { return _layoutEngine; }

    // ========== 堆叠顺序 ==========

    void raiseViewport(int viewportID);
    void lowerViewport(int viewportID);
    void moveViewportToFront(int viewportID);
    void moveViewportToBack(int viewportID);

    // ========== 查询 ==========

    /**
     * @brief 是否有可见视口
     */
    bool hasVisibleViewport() const;

    /**
     * @brief 获取可见视口列表
     */
    QList<ViewportWidget*> visibleViewports() const;

    /**
     * @brief 获取视口显示矩形（考虑布局）
     */
    QRect viewportGeometry(int viewportID) const;

signals:
    // 激活变化
    void sig_activeViewportChanged(int viewportID, int previousViewportID);

    // 视口从隐藏转为可见（切页/单格布局激活换入/窗口还原）。
    // 渲染部件隐藏期间丢失的重绘请求需在此时补帧，接收方应触发一次渲染
    void sig_viewportShown(int viewportID);

    // 用户点击/聚焦视口（含点击的已是当前激活视口——该情形setActiveViewport
    // 同ID早退不发激活信号）。供菜单模式等上下文消费方在同ID去重场景下
    // 仍能显式跟随，不参与激活状态维护
    void sig_viewportUserFocused(int viewportID);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void onViewportFocusIn(int viewportID);

private:
    void setupUI();

private:
    // 视口存储
    QHash<int, ViewportWidget*> _viewports;      // viewportID -> widget

    // 激活状态
    int _activeViewportID = -1;

    // 堆叠顺序（底层到顶层）
    QList<int> _zOrder;

    // 布局
    ViewportLayoutEngine* _layoutEngine = nullptr;
    int _currentLayoutType = 0;                  // 当前生效布局类型（树驱动布局时写入，resize 重放用）
    QList<int> _laidOutViewportIDs;              // 上次布局的视口顺序（含溢出隐藏项，resize 重放用）。
                                                 // 顺序来源：树驱动时按树节点顺序（激活换入时按替换规则调整）。
                                                 // 不能用 _zOrder 代替——激活会把视口移到栈顶，导致格子分配错乱
    QSet<int> _layoutShownViewportIDs;           // 上次布局实际呈现（setVisible(true)）的视口集：
                                                 // 布局是卡片显隐唯一权威写入口，物化挂载链据此守卫
                                                 // （不在呈现集的卡片挂载不显示，防初始几何浮于布局之上）

    // 视觉设置
    QColor _activeBorderColor = QColor(240, 163, 83);
    QColor _inactiveBorderColor = QColor(200, 200, 200);
    int _borderWidth = 2;
    int _spacing = 2;
};

} // namespace MechMainWindow
