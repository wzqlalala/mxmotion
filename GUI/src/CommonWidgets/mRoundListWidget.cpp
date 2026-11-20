#include "mRoundListWidget.h"

namespace MechMainWindow
{
    mRoundListWidget::mRoundListWidget(QWidget* parent)
        : QListWidget(parent)
    {
        // 不使用 WA_TranslucentBackground，避免背景透明问题
        // 背景由 paintEvent 中 QPainter 绘制

        // 移除 QListWidget 默认边框（直角系统边框），只保留自定义圆角边框
        setFrameShape(QFrame::NoFrame);
        setFrameShadow(QFrame::Plain);
    }

    void mRoundListWidget::setBorderRadius(int radius)
    {
        _borderRadius = radius;
        updateMask();
        update();
    }

    void mRoundListWidget::paintEvent(QPaintEvent* event)
    {
        // 先绘制圆角背景（在 QListWidget 默认绘制之前）
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing);

        QPainterPath path;
        path.addRoundedRect(rect(), _borderRadius, _borderRadius);
        painter.fillPath(path, QColor("#FFFFFF"));

        // 调用父类绘制列表内容
        QListWidget::paintEvent(event);
    }

    void mRoundListWidget::resizeEvent(QResizeEvent* event)
    {
        QListWidget::resizeEvent(event);
        updateMask();
    }

    void mRoundListWidget::showEvent(QShowEvent* event)
    {
        QListWidget::showEvent(event);
        updateMask();
    }

    bool mRoundListWidget::coversGlobalPos(const QPoint& globalPos) const
    {
        // 判定光标是否落在本弹窗矩形内（含被 mask 裁掉的四角）：
        // 弹窗可见时其矩形覆盖的区域都应视为"弹窗区域"，由上层据此拦截下层主控件气泡。
        if (!isVisible() || width() <= 0 || height() <= 0)
            return false;

        QPoint local = mapFromGlobal(globalPos);
        return rect().contains(local);
    }

    void mRoundListWidget::updateMask()
    {
        if (!isVisible() || width() <= 0 || height() <= 0)
            return;

        QPainterPath path;
        path.addRoundedRect(rect(), _borderRadius, _borderRadius);
        setMask(path.toFillPolygon().toPolygon());
    }
}
