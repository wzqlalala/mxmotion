#pragma once

#include <QToolButton>

class QColor;
class QMouseEvent;
class QPainter;

namespace MxFunctions {

inline constexpr int kDropArrowGlyphWidth = 6;
void drawDropArrow(QPainter* painter, const QRect& box, const QColor& color);
void drawDropArrowBesideText(QPainter* painter, const QRect& text, int gap, const QColor& color);

class mRibbonButton : public QToolButton
{
public:
    enum class Mode { Small, Big };

    explicit mRibbonButton(Mode mode, QWidget* parent = nullptr);
    void setHasArrow(bool show);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    bool isBig() const;
    bool showsDropArrow() const;
};

} // namespace MxFunctions
