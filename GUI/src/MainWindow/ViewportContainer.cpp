#include "ViewportContainer.h"
#include "ViewportWidget.h"
#include "ViewportLayoutEngine.h"
#include "ViewportLayouts.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

namespace MechMainWindow {

ViewportContainer::ViewportContainer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    setMouseTracking(true);

    // 布局引擎（与视图树共用 viewLayoutTable）
    setLayoutEngine(new ViewportLayoutEngine(this));
}

ViewportContainer::~ViewportContainer()
{
    // 清理所有视口。
    // 激活信号槽（slot_viewportActivated）会访问已销毁的渲染部件导致崩溃
    blockSignals(true);
    QList<int> ids = _viewports.keys();
    for (int id : ids) {
        removeViewport(id);
    }
    blockSignals(false);
}

void ViewportContainer::setupUI()
{
    // 设置背景
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(240, 240, 240));
    setPalette(pal);
}

ViewportWidget* ViewportContainer::addViewport(int viewportID, QWidget* content)
{
    if (_viewports.contains(viewportID)) {
        qWarning() << "ViewportContainer: viewport already exists:" << viewportID;
        return _viewports[viewportID];
    }

    // 创建视口
    auto* viewport = new ViewportWidget(viewportID, content, this);
    viewport->setTitle(QString::fromUtf8("视口-%1").arg(viewportID));

    // 连接信号
    connect(viewport, &ViewportWidget::focusInRequested,
            this, &ViewportContainer::onViewportFocusIn);
    // 视口从隐藏转为可见时中继：隐藏期间 update() 均被丢弃，需补一次渲染
    connect(viewport, &ViewportWidget::shownRequested,
            this, &ViewportContainer::sig_viewportShown);

    // 存储
    _viewports[viewportID] = viewport;
    _zOrder.append(viewportID);

    // 初始几何仅在容器已有有效尺寸时设置（运行中新建视口居中放置）；
    // 启动首个视口创建时容器尚未拉伸（width/height 为默认小值），
    // 居中计算会得到过小矩形，此时跳过由树随后驱动布局设置
    if (width() > 0 && height() > 0)
    {
        const int w = width() * 2 / 3;
        const int h = height() * 2 / 3;
        viewport->setGeometry((width() - w) / 2, (height() - h) / 2, w, h);
    }
    viewport->setVisible(true);

    return viewport;
}

void ViewportContainer::removeViewport(int viewportID)
{
    auto* viewport = _viewports.value(viewportID, nullptr);
    if (!viewport) return;

    // 删除的是激活视口时仅复位激活状态：激活转移到哪个视口不是容器的职责，
    // 由视图树/管理器在删除完成后统一决定并恢复
    if (_activeViewportID == viewportID) {
        _activeViewportID = -1;
    }

    // 从存储中移除
    _viewports.remove(viewportID);
    _zOrder.removeAll(viewportID);

    // 删除视口部件
    viewport->deleteLater();
}

ViewportWidget* ViewportContainer::viewport(int viewportID) const
{
    return _viewports.value(viewportID, nullptr);
}

ViewportWidget* ViewportContainer::viewportAt(const QPoint& pos) const
{
    // 按堆叠顺序从上到下查找
    for (int i = _zOrder.size() - 1; i >= 0; --i) {
        auto* viewport = _viewports.value(_zOrder[i]);
        if (viewport && viewport->isVisible() && viewport->geometry().contains(pos)) {
            return viewport;
        }
    }
    return nullptr;
}

QList<ViewportWidget*> ViewportContainer::allViewports() const
{
    return _viewports.values();
}

void ViewportContainer::setViewportVisible(int viewportID, bool visible)
{
    auto* viewport = _viewports.value(viewportID, nullptr);
    if (!viewport) return;

    // 纯粹设置可见性，不触发任何激活逻辑（与用户明确需求一致：
    // 隐藏视口不改变激活状态，由调用方/用户显式决定激活切换）
    viewport->setVisible(visible);
}

void ViewportContainer::setActiveViewport(int viewportID, bool emitSignal, bool force)
{
    // 检查目标视口是否存在
    auto* target = _viewports.value(viewportID, nullptr);
    if (!target && viewportID != -1) return;

    // 检查是否已激活
    if (_activeViewportID == viewportID) return;

    // 激活超出当前布局格数的视口：用新视口替换当前激活视口所在格，
    // 其余格不动；当前无激活视口（或激活视口不在可见格内）时替换尾格。
    // 关键：判断"新视口是否在可见格内"必须看位置 < slotCount，不能仅看 contains——
    // 因为 _laidOutViewportIDs 可能含超出格数的溢出项（隐藏），仅 contains 会把
    // 溢出视口误判为"已在布局"而跳过重排。
    // 例：slotCount=3，laidOut=[1,2,3,4]、激活 2，激活 4 → laidOut=[1,4,3]。
    const auto& layoutTable = viewLayoutTable();
    int layoutIdx = _currentLayoutType;
    if (layoutIdx < 0 || layoutIdx >= layoutTable.size()) layoutIdx = 0;
    const int slotCount = viewLayoutSlotCount(layoutTable[layoutIdx]);
    const int posInLayout = _laidOutViewportIDs.indexOf(viewportID);
    const bool inVisibleSlot = (posInLayout >= 0 && posInLayout < slotCount);
    if (target && !inVisibleSlot) {
        // 新激活视口不在可见格内：找替换位置
        int replaceIdx = _laidOutViewportIDs.indexOf(_activeViewportID);
        // 激活视口若不在可见格内（比如也是溢出位），退化为替换最后一个可见格
        if (replaceIdx < 0 || replaceIdx >= slotCount) replaceIdx = slotCount - 1;
        // 先移除新视口的旧位置（避免重复），再写入替换位置
        _laidOutViewportIDs.removeAll(viewportID);
        if (replaceIdx < 0 || replaceIdx >= _laidOutViewportIDs.size())
            replaceIdx = _laidOutViewportIDs.size() - 1;
        if (replaceIdx >= 0) {
            _laidOutViewportIDs[replaceIdx] = viewportID;
        } else {
            _laidOutViewportIDs = { viewportID };
        }
        applyLayout(_currentLayoutType, _laidOutViewportIDs);
    }

    int previousID = _activeViewportID;

    // 取消旧激活（仅视觉）
    if (auto* old = _viewports.value(_activeViewportID)) {
        old->setActive(false);
    }

    // 更新状态
    _activeViewportID = viewportID;

    // 设置新激活
    if (target) {
        target->setActive(true);
        target->raise();  // 确保在最前

        // 更新堆叠顺序
        _zOrder.removeAll(viewportID);
        _zOrder.append(viewportID);
    }

    // 发出信号
    if (emitSignal) {
        emit sig_activeViewportChanged(viewportID, previousID);
    }
}

ViewportWidget* ViewportContainer::activeViewport() const
{
    return _viewports.value(_activeViewportID, nullptr);
}

void ViewportContainer::setLayoutEngine(ViewportLayoutEngine* engine)
{
    if (_layoutEngine) {
        _layoutEngine->deleteLater();
    }
    _layoutEngine = engine;
    if (_layoutEngine) {
        _layoutEngine->setParent(this);
    }
}

void ViewportContainer::applyLayout(int layoutType, const QList<int>& viewportIDs)
{
    if (!_layoutEngine) return;

    // 记录当前生效布局类型与视口顺序：resize 时按此重放几何。
    // 顺序来源：树驱动布局时按树节点顺序（激活超格换尾格）；
    // 激活换入时按替换规则调整。不能用堆叠顺序 _zOrder——
    // 激活会把视口移到栈顶，导致 resize 重放时格子分配错乱
    _currentLayoutType = layoutType;
    _laidOutViewportIDs = viewportIDs;

    // 计算布局（显式给定顺序）
    auto result = _layoutEngine->calculateLayout(layoutType, size(), viewportIDs);

    // 应用布局：列表内视口按格子排列并显示，列表外视口全部隐藏
    // 格子分配严格按 viewportIDs 顺序；超出格数的溢出项隐藏
    QSet<int> visibleSet;
    for (int id : viewportIDs) visibleSet.insert(id);

    // 呈现集（本次实际 setVisible(true) 的视口）：物化挂载链按此守卫显隐，
    // 不在呈现集的卡片挂载即显示会以建卡初始几何浮于当前布局之上
    QSet<int> shownSet;
    for (auto it = _viewports.begin(); it != _viewports.end(); ++it) {
        const int viewportID = it.key();
        auto* viewport = it.value();
        if (!viewport) continue;

        if (!visibleSet.contains(viewportID)) {
            viewport->setVisible(false);
            continue;
        }

        // 查找该视口所在格子
        const ViewportLayoutEngine::LayoutCell* target = nullptr;
        for (const auto& cell : result.cells) {
            if (cell.viewportID == viewportID) { target = &cell; break; }
        }
        if (!target) {
            // 超出格子数：隐藏
            viewport->setVisible(false);
            continue;
        }

        if (result.totalSlots == 1) {
            // 单视口：最大化占据整个容器
            viewport->setGeometry(0, 0, width(), height());
            viewport->setState(ViewportWidget::State::Maximized);
        } else {
            // 先复位到 Normal 让 setState(Maximized→Normal) 恢复 _normalGeometry，
            // 再设置新几何：否则前一次单视口布局留下的 Maximized 状态会让
            // setState(Normal) 用旧的 _normalGeometry 覆盖新算出的格子矩形
            if (viewport->state() != ViewportWidget::State::Normal) {
                viewport->setState(ViewportWidget::State::Normal);
            }
            viewport->setNormalGeometry(target->geometry);
            viewport->setGeometry(target->geometry);
        }
        viewport->setVisible(true);
        shownSet.insert(viewportID);
    }
    _layoutShownViewportIDs = shownSet;
}

void ViewportContainer::raiseViewport(int viewportID)
{
    auto* viewport = _viewports.value(viewportID);
    if (!viewport) return;

    viewport->raise();
    _zOrder.removeAll(viewportID);
    _zOrder.append(viewportID);
}

void ViewportContainer::lowerViewport(int viewportID)
{
    auto* viewport = _viewports.value(viewportID);
    if (!viewport) return;

    viewport->lower();
    _zOrder.removeAll(viewportID);
    _zOrder.prepend(viewportID);
}

void ViewportContainer::moveViewportToFront(int viewportID)
{
    raiseViewport(viewportID);
}

void ViewportContainer::moveViewportToBack(int viewportID)
{
    lowerViewport(viewportID);
}

bool ViewportContainer::hasVisibleViewport() const
{
    for (auto* viewport : _viewports) {
        if (viewport && viewport->isVisible()) {
            return true;
        }
    }
    return false;
}

QList<ViewportWidget*> ViewportContainer::visibleViewports() const
{
    QList<ViewportWidget*> result;
    for (auto* viewport : _viewports) {
        if (viewport && viewport->isVisible()) {
            result.append(viewport);
        }
    }
    return result;
}

QRect ViewportContainer::viewportGeometry(int viewportID) const
{
    auto* viewport = _viewports.value(viewportID);
    return viewport ? viewport->geometry() : QRect();
}

void ViewportContainer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), QColor(240, 240, 240));
}

void ViewportContainer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // resize 只重算几何：可见组/布局类型/页归属均不变。
    // 用 applyLayout 记录的布局顺序（含激活换入后的调整），不用堆叠顺序——
    // 激活会把视口移到 _zOrder 栈顶，resize 时按堆叠顺序会导致格子分配错乱
    if (!_layoutEngine) return;

    QList<int> visibleIDs;
    for (int id : _laidOutViewportIDs) {
        //防御性过滤：布局后若有视口被删除（树会立即驱动新布局刷新，此处仅兜底）
        // The layout still owns this slot when an ancestor is hidden, for
        // example while sizing a newly opened page or an inactive project.
        if (_viewports.contains(id) && _layoutShownViewportIDs.contains(id)) {
            visibleIDs.append(id);
        }
    }
    if (visibleIDs.isEmpty()) return;

    auto result = _layoutEngine->calculateLayout(_currentLayoutType, size(), visibleIDs);
    for (const auto& cell : result.cells) {
        if (cell.viewportID == -1) continue;
        auto* viewport = _viewports.value(cell.viewportID);
        if (!viewport) continue;

        if (result.totalSlots == 1) {
            viewport->setGeometry(0, 0, width(), height());
        } else {
            viewport->setNormalGeometry(cell.geometry);
            viewport->setGeometry(cell.geometry);
        }
    }
}

void ViewportContainer::mousePressEvent(QMouseEvent* event)
{
    //点击空白区域不取消激活：视口应用要求始终有一个激活视口，
    //取消激活会导致面板 getCVM 失效、当前模型/视口丢失
    QWidget::mousePressEvent(event);
}

void ViewportContainer::onViewportFocusIn(int viewportID)
{
    //先走正常激活（变化时全链联动）；随后总是广播用户聚焦——同ID早退的
    //点击没有激活信号，菜单模式等上下文消费方靠它显式跟随（同模式下幂等）
    setActiveViewport(viewportID);
    emit sig_viewportUserFocused(viewportID);
}

} // namespace MechMainWindow
