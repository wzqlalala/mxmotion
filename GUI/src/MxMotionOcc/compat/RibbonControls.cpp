#include "mRibbonButton.h"
#include "mUiMenu.h"

#include "ThemeManager.h"
#include "UiMetrics.h"
#include "mUiStyle.h"

#include <QAction>
#include <QActionGroup>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QStyleOption>
#include <QWidgetAction>

namespace MxFunctions {
namespace {

QPointer<QStyle> sharedMenuStyle;

QFont ribbonFont()
{
    QFont font(UiMetrics::uiFontFamily());
    font.setPixelSize(UiMetrics::buttonFontSize());
    return font;
}

void paintDownChevron(QPainter* painter, const QPointF& center, const QColor& color)
{
    constexpr qreal stroke = 1.2;
    constexpr qreal halfWidth = (kDropArrowGlyphWidth - stroke) / 2.0;
    constexpr qreal halfHeight = 1.4;
    const QPointF points[] = {
        center + QPointF(-halfWidth, -halfHeight),
        center + QPointF(0, halfHeight),
        center + QPointF(halfWidth, -halfHeight)
    };
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(color, stroke, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->drawPolyline(points, 3);
    painter->restore();
}

} // namespace

void drawDropArrow(QPainter* painter, const QRect& box, const QColor& color)
{
    paintDownChevron(painter, QRectF(box).center(), color);
}

void drawDropArrowBesideText(QPainter* painter, const QRect& text, int gap, const QColor& color)
{
    const QRectF line(text);
    paintDownChevron(painter,
        QPointF(line.right() + gap + kDropArrowGlyphWidth / 2.0, line.center().y()), color);
}

mRibbonButton::mRibbonButton(Mode mode, QWidget* parent)
    : QToolButton(parent)
{
    const bool big = mode == Mode::Big;
    setProperty("uiBigButton", big);
    setToolButtonStyle(big ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonTextBesideIcon);
    const int edge = big ? UiMetrics::menuLargeIconSize() : UiMetrics::menuIconSize();
    setIconSize(QSize(edge, edge));
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setPopupMode(QToolButton::InstantPopup);
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::TabFocus);
}

void mRibbonButton::setHasArrow(bool show)
{
    setProperty("mxHasMenu", show);
    setProperty("mxNoArrow", !show);
    updateGeometry();
    update();
}

bool mRibbonButton::isBig() const
{
    return property("uiBigButton").toBool();
}

bool mRibbonButton::showsDropArrow() const
{
    return toolButtonStyle() != Qt::ToolButtonIconOnly && !property("mxNoArrow").toBool()
        && (property("mxHasMenu").toBool() || menu() || (defaultAction() && defaultAction()->menu()));
}

QSize mRibbonButton::sizeHint() const
{
    if (toolButtonStyle() == Qt::ToolButtonIconOnly)
        return QSize(UiMetrics::quickButtonSize(), UiMetrics::quickButtonSize());

    // ThemeManager's width calculation only reads the button despite its non-const API.
    const int width = MechMainWindow::ThemeManager::ribbonButtonMinWidth(
        const_cast<mRibbonButton*>(this), showsDropArrow());
    const int height = objectName() == QLatin1String("FileButton") ? UiMetrics::tabBarHeight()
        : isBig() ? UiMetrics::bigButtonHeight() : UiMetrics::halfButtonHeight();
    return QSize(width, height);
}

QSize mRibbonButton::minimumSizeHint() const
{
    return sizeHint();
}

void mRibbonButton::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    QStyleOptionToolButton option;
    initStyleOption(&option);
    const bool enabled = option.state.testFlag(QStyle::State_Enabled);
    const bool hover = enabled && option.state.testFlag(QStyle::State_MouseOver);
    const bool pressed = enabled && option.state.testFlag(QStyle::State_Sunken);
    const bool iconOnly = toolButtonStyle() == Qt::ToolButtonIconOnly;

    const QString dropText = property("dropText").toString();
    const QString visibleText = dropText.isEmpty() ? text() : dropText;
    MechMainWindow::mUiStyle::drawRibbonButton(&painter, rect(), iconOnly ? QString() : visibleText,
        iconOnly ? QIcon() : icon(), iconSize(), !iconOnly && isBig(), showsDropArrow(),
        hover, pressed, isChecked(), enabled, ribbonFont(), this);
    if (iconOnly) {
        // The legacy text-button renderer left-aligns small icons and uses a fixed icon edge.
        // Quick buttons need the caller's icon size, centered without a popup indicator.
        const QSize edge = iconSize().boundedTo(size());
        QRect iconRect(QPoint(), edge);
        iconRect.moveCenter(rect().center());
        icon().paint(&painter, iconRect, Qt::AlignCenter,
            enabled ? QIcon::Normal : QIcon::Disabled, isChecked() ? QIcon::On : QIcon::Off);
    }
    if (enabled && (option.state & QStyle::State_HasFocus)
        && (option.state & QStyle::State_KeyboardFocusChange))
        MechMainWindow::mUiStyle::drawButtonState(&painter, rect(), false, false, false, true);
}

void mRibbonButton::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || popupMode() != QToolButton::MenuButtonPopup
        || !(menu() || (defaultAction() && defaultAction()->menu()))) {
        QToolButton::mousePressEvent(event);
        return;
    }

    // Qt's split hit target occupies a right gutter, but the shared painter places the
    // arrow beside the label. On a full-height button the label row is the menu zone.
    bool inMenuZone = false;
    if (showsDropArrow() && rect().contains(event->pos())) {
        if (isBig()) {
            const qreal labelTop = height() - UiMetrics::bigButtonTextBottomPaddingF()
                - UiMetrics::menuLineHeight() - UiMetrics::menuContentGap();
            inMenuZone = event->pos().y() >= labelTop;
        } else {
            const int iconWidth = icon().isNull() ? 0 : UiMetrics::menuIconSize() + UiMetrics::menuContentGap();
            const QString dropText = property("dropText").toString();
            const int labelWidth = QFontMetrics(ribbonFont()).horizontalAdvance(
                dropText.isEmpty() ? text() : dropText);
            inMenuZone = event->pos().x() >= UiMetrics::menuInset() + iconWidth + labelWidth;
        }
    }
    if (inMenuZone) {
        event->accept();
        showMenu();
        return;
    }
    QAbstractButton::mousePressEvent(event);
}

mUiMenu::mUiMenu(QWidget* parent)
    : QMenu(parent)
{
    init();
}

mUiMenu::mUiMenu(const QString& title, QWidget* parent)
    : QMenu(title, parent)
{
    init();
}

void mUiMenu::setSharedStyle(QStyle* style)
{
    sharedMenuStyle = style;
}

void mUiMenu::init()
{
    setProperty("mxUiMenu", true);
    // Set this before the popup is made native, retaining Qt's popup/drop-shadow flags.
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover);
    connect(this, &QMenu::aboutToShow, this, [this] {
        applyMetrics();
        for (const auto* action : actions()) {
            if (QMenu* child = action->menu(); child && !child->property("mxUiMenu").toBool())
                MechMainWindow::mUiStyle::installMenu(child);
        }
    });
    applyMetrics();
}

void mUiMenu::applyMetrics()
{
    if (!sharedMenuStyle)
        MechMainWindow::mUiStyle::instance();
    if (m_appliedStyle != sharedMenuStyle) {
        m_appliedStyle = sharedMenuStyle;
        setStyle(sharedMenuStyle.data());
    }

    const int level = UiMetrics::level();
    if (m_metricsLevel != level) {
        m_metricsLevel = level;
        QFont menuFont(UiMetrics::uiFontFamily());
        menuFont.setPixelSize(UiMetrics::bodyFontSize());
        // QMenu handles FontChange by invalidating its cached action geometry.
        setFont(menuFont);
        updateGeometry();
    }
    update();
}

void mUiMenu::paintEvent(QPaintEvent* event)
{
    // Keep Qt's action rectangles and active-action state. Call the shared renderer
    // directly so Fusion cannot add a square PE_FrameMenu/CE_MenuEmptyArea over the panel.
    if (!sharedMenuStyle || isTearOffEnabled()
        || style()->styleHint(QStyle::SH_Menu_Scrollable, nullptr, this)
        || columnCount() > 1) {
        QMenu::paintEvent(event);
        return;
    }

    QPainter painter(this);
    // Clear damaged transparent corners as well as the opaque center on repaints.
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(event->rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    QStyleOptionMenuItem panel;
    panel.initFrom(this);
    sharedMenuStyle->drawPrimitive(QStyle::PE_PanelMenu, &panel, &painter, this);

    for (QAction* action : actions()) {
        if (qobject_cast<const QWidgetAction*>(action))
            continue; // Qt positions and paints the embedded widget separately.
        const QRect row = actionGeometry(action);
        if (!event->region().intersects(row))
            continue;
        QStyleOptionMenuItem item;
        initStyleOption(&item, action);
        item.rect = row;
        painter.save();
        painter.setClipRect(row);
        sharedMenuStyle->drawControl(QStyle::CE_MenuItem, &item, &painter, this);
        painter.restore();
    }
}

void mUiMenu::mouseReleaseEvent(QMouseEvent* event)
{
    QAction* action = actionAt(event->pos());
    // The original resident-menu checklists stay open for consecutive toggles.
    // Exclusive selections, ordinary commands and submenu navigation retain
    // QMenu's normal dismissal and activation semantics.
    if (event->button() == Qt::LeftButton && action && action->isEnabled()
        && action->isCheckable() && !action->menu()
        && (!action->actionGroup() || !action->actionGroup()->isExclusive())) {
        action->trigger();
        update(actionGeometry(action));
        event->accept();
        return;
    }
    QMenu::mouseReleaseEvent(event);
}

} // namespace MxFunctions
