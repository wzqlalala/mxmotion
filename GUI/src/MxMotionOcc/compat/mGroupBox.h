#pragma once

#include "UiMetrics.h"
#include "UiPalette.h"
#include <QGroupBox>
#include <QGridLayout>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

namespace MxFunctions {

// RebuildFunctions' group API used by the uploaded point/direction widgets.
// Its content and grid ownership follow the original contract; the original
// widgets continue to create and arrange every control themselves.
class mGroupBox : public QGroupBox {
public:
    explicit mGroupBox(const QString& title = {}, const QPixmap& = {},
                       const QPixmap& = {}, QWidget* parent = nullptr)
        : QGroupBox(title, parent), m_content(new QWidget(this)),
          m_grid(new QGridLayout(m_content))
    {
        auto* outer = new QVBoxLayout(this);
        outer->setContentsMargins(UiMetrics::baseG(), UiMetrics::bodyFontSize() + UiMetrics::rowGap(),
                                  UiMetrics::baseG(), UiMetrics::rowGap());
        outer->setSpacing(0);
        outer->addWidget(m_content);
        m_grid->setContentsMargins(0, 0, 0, 0);
        m_grid->setHorizontalSpacing(UiMetrics::columnGap());
        m_grid->setVerticalSpacing(UiMetrics::rowGap());
    }
    QGridLayout* grid() const { return m_grid; }
    QWidget* contentWidget() const { return m_content; }
protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::TextAntialiasing);
        QFont caption(UiMetrics::uiFontFamily());
        caption.setPixelSize(UiMetrics::bodyFontSize());
        painter.setFont(caption);
        const QFontMetrics metrics(caption);
        const qreal y = metrics.height() * .5;
        const qreal x = UiMetrics::fieldInset();
        const qreal labelEnd = x + metrics.horizontalAdvance(title());
        painter.setPen(UiPalette::border());
        painter.drawLine(QPointF(.5, y), QPointF(.5, height() - .5));
        painter.drawLine(QPointF(width() - .5, y), QPointF(width() - .5, height() - .5));
        painter.drawLine(QPointF(.5, height() - .5), QPointF(width() - .5, height() - .5));
        if (title().isEmpty()) painter.drawLine(QPointF(.5, y), QPointF(width() - .5, y));
        else {
            painter.drawLine(QPointF(.5, y), QPointF(x - UiMetrics::choiceGap(), y));
            painter.drawLine(QPointF(labelEnd + UiMetrics::choiceGap(), y), QPointF(width() - .5, y));
            painter.setPen(UiPalette::text());
            painter.drawText(QPointF(x, metrics.ascent()), title());
        }
    }
private:
    QWidget* m_content;
    QGridLayout* m_grid;
};
} // namespace MxFunctions
