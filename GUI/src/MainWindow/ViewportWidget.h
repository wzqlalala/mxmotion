#pragma once

#include <QFrame>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QShowEvent>
#include <QStyleOption>

namespace MechMainWindow {

class ViewportContainer;

/**
 * @brief 自研视口组件，替代 QMdiSubWindow
 *
 * 核心特性：
 * - 完全自绘边框/标题栏，无系统依赖
 * - 显示/隐藏不触发任何激活逻辑
 * - 激活状态仅通过 setActive() 显式设置
 */
class ViewportWidget : public QFrame
{
    Q_OBJECT

public:
    enum class State {
        Normal,      // 正常
        Maximized,   // 最大化（占据整个容器）
        Minimized,   // 最小化（仅显示标题栏）
        Hidden       // 隐藏（不可见）
    };
    Q_ENUM(State)

    explicit ViewportWidget(int viewportID, QWidget* content, QWidget* parent = nullptr);
    ~ViewportWidget() override;

    // 基本属性
    int viewportID() const { return _viewportID; }
    QString title() const { return _title; }
    void setTitle(const QString& title);

    // 激活状态（仅视觉，不触发信号）
    bool isActive() const { return _active; }
    void setActive(bool active);

    // 状态控制
    State state() const { return _state; }
    void setState(State state);

    // 内容区域
    QWidget* contentWidget() const { return _content; }
    void setContentWidget(QWidget* widget);

    // 布局信息
    QRect normalGeometry() const { return _normalGeometry; }
    void setNormalGeometry(const QRect& rect) { _normalGeometry = rect; }

signals:
    void focusInRequested(int viewportID);  // 点击请求激活
    void shownRequested(int viewportID);    // 从隐藏转为可见（切页/单格换入等）

protected:
    // 自绘
    void paintEvent(QPaintEvent* event) override;

    // 显隐变化（见 shownRequested）
    void showEvent(QShowEvent* event) override;

    // 事件处理
    void mousePressEvent(QMouseEvent* event) override;
    // 拦截内容部件（渲染窗口）的点击以实现"点击视口任意位置即激活"
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void setupUI();
    void updateTitleBar();
    //递归给内容部件及其子孙挂/摘事件过滤器（渲染输入表面是子孙部件）
    void installContentEventFilter(QWidget* content);
    void removeContentEventFilter(QWidget* content);

private:
    int _viewportID;
    QString _title;
    bool _active = false;
    State _state = State::Normal;

    // UI 组件
    QWidget* _content = nullptr;
    QWidget* _titleBar = nullptr;
    QLabel* _titleLabel = nullptr;

    // 几何记忆
    QRect _normalGeometry;
};

} // namespace MechMainWindow
