#pragma once

#include <QLabel>
#include <QPainter>

namespace MechMainWindow {

class CurveNameLabel final : public QLabel
{
public:
    explicit CurveNameLabel(const QString& text, QWidget* parent) : QLabel(text, parent)
    {
        setObjectName(QStringLiteral("motionCurveRowTitle"));
        setToolTip(text);
        setTextFormat(Qt::PlainText);
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setMinimumWidth(0);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setPen(palette().color(foregroundRole()));
        const auto displayed = fontMetrics().elidedText(text(), Qt::ElideRight, contentsRect().width());
        painter.drawText(contentsRect(), alignment() | Qt::TextSingleLine, displayed);
    }
};

} // namespace MechMainWindow
