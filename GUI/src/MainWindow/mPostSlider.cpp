#include "mPostSlider.h"
#include <QStyle>
#include <QStyleOptionSlider>

namespace MechMainWindow
{
    mPostSlider::mPostSlider(QWidget* parent) : QSlider(parent) {}

    mPostSlider::mPostSlider(Qt::Orientation orientation, QWidget* parent) : QSlider(orientation, parent) {}

    void mPostSlider::mousePressEvent(QMouseEvent* event) {
        // 1. 先调用父类事件处理，保证基本的交互（如拖动）不受影响[reference:5]
        QSlider::mousePressEvent(event);

        // 2. 判断是否为鼠标左键点击
        if (event->button() == Qt::LeftButton) {
            if (event->button() == Qt::LeftButton) {
                // Get the option
                QStyleOptionSlider opt;
                initStyleOption(&opt);
                // Get the handle rect (optional but good to avoid jumping when clicking handle)
                QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);

                // If click is on handle, let base class handle it (drag)
                if (handleRect.contains(event->pos())) {
                    QSlider::mousePressEvent(event);
                    return;
                }

                // Calculate new value based on orientation
                int newVal;
                if (orientation() == Qt::Horizontal) {
                    newVal = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().x(), width());
                }
                else {
                    // Vertical: y goes from top(0) to bottom(height). Slider usually goes bottom(min) to top(max).
                    // QStyle::sliderValueFromPosition expects position from the start (top for vertical usually).
                    // Let's just use manual ratio for clarity in explanation, or use QStyle. QStyle handles it well if orientation is set.
                    // Actually, QStyle::sliderValueFromPosition works for both, but for vertical, position is y.
                    newVal = QStyle::sliderValueFromPosition(minimum(), maximum(), event->pos().y(), height());
                }
                setValue(newVal);
                // We don't call base class here to prevent default page step behavior, 
                // but we need to emit signals? setValue emits valueChanged.
                return;
            }
            QSlider::mousePressEvent(event);
        }
    }
}
