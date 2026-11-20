#include "ViewportWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>

namespace MechMainWindow {

ViewportWidget::ViewportWidget(int viewportID, QWidget* content, QWidget* parent)
    : QFrame(parent)
    , _viewportID(viewportID)
    , _content(content)
{
    setupUI();
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFrameStyle(QFrame::NoFrame);
}

ViewportWidget::~ViewportWidget() = default;

void ViewportWidget::setupUI()
{
    // 主布局
    // 四周留 2px 边距作为边框绘制区：子部件（标题栏/渲染内容）铺满时会盖住
    // paintEvent 画在最外圈的边框，导致激活高亮边框完全不可见
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    mainLayout->setSpacing(0);

    // 标题栏
    _titleBar = new QWidget(this);
    _titleBar->setFixedHeight(24);
    _titleBar->setObjectName("viewportTitleBar");
    auto* titleLayout = new QHBoxLayout(_titleBar);
    titleLayout->setContentsMargins(4, 0, 4, 0);
    titleLayout->setSpacing(2);

    _titleLabel = new QLabel(_titleBar);
    _titleLabel->setObjectName("viewportTitleLabel");
    //视口名称显示在标题栏右上角
    _titleLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    titleLayout->addStretch();
    titleLayout->addWidget(_titleLabel);

    mainLayout->addWidget(_titleBar);

    // 内容区域
    if (_content) {
        _content->setParent(this);
        mainLayout->addWidget(_content);
        //拦截内容部件及其子部件（OSG 输入表面会吃掉鼠标事件，见 eventFilter）
        installContentEventFilter(_content);
        //内容部件生命周期归视口模型侧：删除视口时内容先于本包裹部件被销毁
        //（容器侧 removeViewport 走 deleteLater 延迟析构），销毁时同步清空指针，
        //使 setState/显隐等处的 if(_content) 判空对悬空指针真正生效
        connect(_content, &QObject::destroyed, this, [this]() { _content = nullptr; });
    }

    // 初始样式
    updateTitleBar();
}

void ViewportWidget::setTitle(const QString& title)
{
    if (_title == title) return;
    _title = title;
    _titleLabel->setText(title);
}

void ViewportWidget::setActive(bool active)
{
    if (_active == active) return;
    _active = active;
    updateTitleBar();
    update();  // 重绘边框
}

void ViewportWidget::setState(State state)
{
    if (_state == state) return;

    State oldState = _state;
    _state = state;

    switch (state) {
    case State::Normal:
        if (oldState == State::Maximized || oldState == State::Minimized) {
            setGeometry(_normalGeometry);
        }
        _titleBar->setVisible(true);
        if (_content) _content->setVisible(true);
        break;

    case State::Maximized:
        if (oldState == State::Normal) {
            _normalGeometry = geometry();
        }
        //单视口布局（Maximized）无需标题栏：视口填满中央区，标题无辨识意义
        _titleBar->setVisible(false);
        if (_content) _content->setVisible(true);
        // 几何由容器设置
        break;

    case State::Minimized:
        if (oldState == State::Normal) {
            _normalGeometry = geometry();
        }
        _titleBar->setVisible(true);
        if (_content) _content->setVisible(false);
        setFixedHeight(24);
        break;

    case State::Hidden:
        setVisible(false);
        break;
    }
}

void ViewportWidget::setContentWidget(QWidget* widget)
{
    if (_content == widget) return;

    if (_content) {
        removeContentEventFilter(_content);
        _content->setParent(nullptr);
        _content->hide();
    }

    _content = widget;
    if (_content) {
        _content->setParent(this);
        auto* layout = qobject_cast<QVBoxLayout*>(this->layout());
        if (layout) {
            layout->addWidget(_content);
        }
        _content->setVisible(_state != State::Minimized && _state != State::Hidden);
        //拦截内容部件及其子部件点击（渲染窗口会吃掉鼠标事件，见 eventFilter）
        installContentEventFilter(_content);
        //同构造：内容先于包裹部件销毁时清空指针，防悬空（见 setupUI 注释）
        connect(_content, &QObject::destroyed, this, [this]() { _content = nullptr; });
    }
}

void ViewportWidget::installContentEventFilter(QWidget* content)
{
    //渲染输入表面（OSG 部件）是内容部件的子孙且自己消费鼠标事件，
    //须递归挂到所有子孙部件才能拦截到点击
    if (!content) return;
    content->installEventFilter(this);
    for (auto* child : content->findChildren<QWidget*>()) {
        child->installEventFilter(this);
    }
}

void ViewportWidget::removeContentEventFilter(QWidget* content)
{
    if (!content) return;
    content->removeEventFilter(this);
    for (auto* child : content->findChildren<QWidget*>()) {
        child->removeEventFilter(this);
    }
}

void ViewportWidget::updateTitleBar()
{
    QString style = QString(
        "QWidget#viewportTitleBar {"
        "   background-color: %1;"
        "   border-bottom: 1px solid #dcdcdc;"
        "}"
        "QLabel#viewportTitleLabel {"
        "   color: %2;"
        "   font-size: 12px;"
        "   font-weight: %3;"
        "}"
    ).arg(_active ? "#e6f0fa" : "#f0f0f0")
     .arg(_active ? "#0066cc" : "#333333")
     .arg(_active ? "bold" : "normal");

    _titleBar->setStyleSheet(style);
}

void ViewportWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制边框（布局已留 2px 边距，边框区不被子部件遮盖）
    QColor borderColor = _active ? QColor(240, 163, 83) : QColor(200, 200, 200);

    painter.setPen(QPen(borderColor, 1));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
    if (_active) {
        // 激活边框 2px：内圈再补一条
        painter.drawRect(rect().adjusted(1, 1, -2, -2));
    }
}

void ViewportWidget::showEvent(QShowEvent* event)
{
    QFrame::showEvent(event);

    // 渲染部件（QOpenGLWidget）隐藏期间收到的重绘请求全部被 Qt 丢弃
    //（update() 对隐藏部件无效），重新显示时直接合成的是隐藏前的旧帧。
    // 通知外部补一次渲染，覆盖切页、单格布局激活换入、窗口还原等路径
    emit shownRequested(_viewportID);
}

void ViewportWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        // 点击请求激活
        emit focusInRequested(_viewportID);
    }

    QFrame::mousePressEvent(event);
}

bool ViewportWidget::eventFilter(QObject* watched, QEvent* event)
{
    //内容部件（OSG 渲染窗口）会吃掉鼠标事件，点击视口内容区域不会
    //冒泡到本部件的 mousePressEvent——用事件过滤器拦截子部件的点击，
    //实现"点击视口任意位置即激活"
    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            emit focusInRequested(_viewportID);
        }
    }
    return QFrame::eventFilter(watched, event);
}

} // namespace MechMainWindow
