#include "mRoundTooltip.h"

#include <QScreen>
#include <QGuiApplication>

namespace MechMainWindow
{
    mRoundTooltip::mRoundTooltip(QWidget* parent)
        : QWidget(parent, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setMouseTracking(true);

        initUI();

        // 自动隐藏定时器（鼠标离开后延迟隐藏，避免闪烁）
        _hideTimer = new QTimer(this);
        _hideTimer->setSingleShot(true);
        _hideTimer->setInterval(300);
        connect(_hideTimer, &QTimer::timeout, this, &mRoundTooltip::hide);
    }

    void mRoundTooltip::initUI()
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(_paddingH, _paddingV, _paddingH, _paddingV);

        _label = new QLabel(this);
        _label->setStyleSheet("color: #333333; font-size: 12px;");
        layout->addWidget(_label);

        setLayout(layout);
    }

    void mRoundTooltip::setText(const QString& text)
    {
        _label->setText(text);
        adjustSize();
    }

    QString mRoundTooltip::text() const
    {
        return _label->text();
    }

    void mRoundTooltip::showAt(const QPoint& globalPos)
    {
        adjustPosition(globalPos);
        show();
        raise();
    }

    void mRoundTooltip::hideTooltip()
    {
        _hideTimer->stop();
        hide();
    }

    void mRoundTooltip::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制圆角背景
        QPainterPath path;
        path.addRoundedRect(rect(), _borderRadius, _borderRadius);
        painter.fillPath(path, QColor("#FFFFFF"));

        // 绘制边框
        painter.setPen(QPen(QColor("#DDDDDD"), 1));
        painter.drawPath(path);
    }

    void mRoundTooltip::enterEvent(QEvent* event)
    {
        // 鼠标进入 tooltip 时停止隐藏定时器，允许用户阅读内容
        _hideTimer->stop();
        QWidget::enterEvent(event);
    }

    void mRoundTooltip::leaveEvent(QEvent* event)
    {
        // 鼠标离开后延迟隐藏
        _hideTimer->start();
        QWidget::leaveEvent(event);
    }

    void mRoundTooltip::mousePressEvent(QMouseEvent* event)
    {
        // 点击 tooltip 时立即隐藏
        hideTooltip();
        QWidget::mousePressEvent(event);
    }

    void mRoundTooltip::adjustPosition(const QPoint& globalPos)
    {
        // 默认显示在鼠标右下方
        QPoint pos = globalPos + QPoint(12, 16);

        // 获取屏幕几何，避免 tooltip 超出屏幕
        QScreen* screen = QGuiApplication::screenAt(globalPos);
        if (!screen)
        {
            screen = QGuiApplication::primaryScreen();
        }
        QRect screenRect = screen->availableGeometry();

        // 右边界检查
        if (pos.x() + width() > screenRect.right())
        {
            pos.setX(globalPos.x() - width() - 12);
        }
        // 下边界检查
        if (pos.y() + height() > screenRect.bottom())
        {
            pos.setY(globalPos.y() - height() - 8);
        }
        // 上边界检查
        if (pos.y() < screenRect.top())
        {
            pos.setY(screenRect.top());
        }
        // 左边界检查
        if (pos.x() < screenRect.left())
        {
            pos.setX(screenRect.left());
        }

        move(pos);
    }
}
