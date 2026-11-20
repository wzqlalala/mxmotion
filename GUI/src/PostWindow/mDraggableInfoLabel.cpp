#include "mDraggableInfoLabel.h"

mDraggableInfoLabel::mDraggableInfoLabel(QWidget* parent)
    : QLabel(parent)
    , _dragging(false)
{
    setStyleSheet("QLabel { border: 1px solid black; background-color: transparent; color: purple; padding: 4px; }");
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setTextFormat(Qt::RichText);
}

void mDraggableInfoLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _dragging = true;
        _dragPosition = event->globalPos() - frameGeometry().topLeft();
        event->accept();
    }
}

void mDraggableInfoLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (_dragging)
    {
        move(event->globalPos() - _dragPosition);
        event->accept();
    }
}

void mDraggableInfoLabel::mouseReleaseEvent(QMouseEvent* event)
{
    _dragging = false;
    event->accept();
}
