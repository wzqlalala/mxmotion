#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPoint>

/**
 * @brief 可拖动的信息标签
 *
 * 带黑色边框、白色背景、紫色文字，默认支持鼠标拖动，
 * 用于在曲线图上叠加显示图例/统计信息。
 */
class mDraggableInfoLabel : public QLabel
{
    Q_OBJECT

public:
    explicit mDraggableInfoLabel(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    bool _dragging;
    QPoint _dragPosition;
};
