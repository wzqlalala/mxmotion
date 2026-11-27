// Adapted from wzqlalala/Motion b636535a45895b193aa581afd4df10a9004c9b6e
// src/ui/navigation/OrientationTriadInteraction.h. Keep the source geometry and interaction contract.
#pragma once

#include "OrientationTriadGeometry.h"
#include "OrientationTriadCamera.h"
#include <QApplication>
#include <QContextMenuEvent>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QHelpEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMouseEvent>
#include <QPointer>
#include <QShortcut>
#include <QToolButton>
#include <QToolTip>
#include <functional>

namespace MotionOcc {
enum class TriadNavigationAvailability { Available, Busy, SketchLocked };

// Input is filtered on the viewport, not captured by the rectangular HUD.
// Only actual faces/axes, and gestures that START on them, consume events.
class OrientationTriadInteraction final : public QObject {
  public:
    OrientationTriadInteraction(OrientationTriadCamera& camera, QWidget& hud,
                                std::function<TriadNavigationAvailability()> availability)
        : QObject(&hud), m_camera(camera), m_hud(hud), m_viewport(hud.parentWidget()),
          m_availability(std::move(availability)), m_last_orientation(camera.Orientation()) {
        m_viewport->installEventFilter(this);
        m_shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_0), m_viewport);
        m_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(m_shortcut, &QShortcut::activated, this, [this] {
            if (Availability() == TriadNavigationAvailability::Available)
                ShowMenu(m_hud.mapToGlobal(QPoint(m_hud.width() / 2, m_hud.height() / 2)));
        });
    }
    ~OrientationTriadInteraction() override {
        m_viewport->removeEventFilter(this);
        // The viewport's camera member may already have been destroyed. Never
        // read/restore camera state while deleting viewport-owned children.
        m_dragging = false;
        if (QWidget::mouseGrabber() == m_viewport) m_viewport->releaseMouse();
        RestoreCursor();
        delete m_shortcut;
        delete m_menu.data();
        delete m_panel;
    }
    OrientationTriadHit Hover() const { return m_hover; }
    int SelectedAxis() const { return m_axis; }
    bool IsDragging() const { return m_dragging; }
    bool HasPointerOperation() const { return m_dragging || m_press_button != Qt::NoButton; }
    bool IsAvailable() const { return Availability() == TriadNavigationAvailability::Available; }
    void CancelNavigation() { Cancel(false); }
    void InterruptPointerOperation() {
        if (m_dragging) FinishDrag(false);
        m_pressed = {};
        m_press_button = Qt::NoButton;
    }
    void PlacePanel() {
        if (!m_panel) return;
        m_panel->adjustSize();
        m_panel->move(std::clamp(m_hud.x(), 0, std::max(0, m_viewport->width() - m_panel->width())),
            std::max(0, m_hud.y() - m_panel->height() - 4));
        m_panel->raise();
    }

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        // Keep the viewport/window's Esc shortcuts from finishing an active
        // model picker while Esc is cancelling this view-only operation.
        if (event->type() == QEvent::ShortcutOverride && (m_axis >= 0 || HasPointerOperation()) &&
            static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            event->accept(); return true;
        }
        if (event->type() == QEvent::KeyPress && (m_axis >= 0 || m_dragging) &&
            static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape) {
            Cancel(true); m_viewport->setFocus(Qt::OtherFocusReason); return true;
        }
        if (event->type() == QEvent::KeyPress && watched != m_viewport &&
            (watched == m_angle || qobject_cast<QLineEdit*>(watched)) &&
            (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Return ||
             static_cast<QKeyEvent*>(event)->key() == Qt::Key_Enter)) {
            ApplyAngle(); return true;
        }
        if (watched != m_viewport) return false;
        if (event->type() == QEvent::Paint || event->type() == QEvent::Show) {
            if (m_camera.Orientation() != m_last_orientation) Cancel(false);
            m_last_orientation = m_camera.Orientation();
        }
        if (event->type() == QEvent::Hide || event->type() == QEvent::WindowDeactivate ||
            (event->type() == QEvent::EnabledChange && !m_viewport->isEnabled()))
            InterruptPointerOperation();
        if (event->type() == QEvent::UngrabMouse || event->type() == QEvent::FocusOut)
            InterruptPointerOperation();
        if (event->type() == QEvent::Resize) PlacePanel();
        if (event->type() == QEvent::Leave && !m_dragging) SetHover({});
        const auto availability = Availability();
        if (availability != TriadNavigationAvailability::Available && !m_dragging) {
            if (m_axis >= 0) Cancel(false);
            SetHover({});
        }
        if (availability == TriadNavigationAvailability::Busy && !m_dragging) return false;
        if (event->type() == QEvent::ToolTip) {
            auto* help = static_cast<QHelpEvent*>(event);
            const auto hit = Hit(help->pos());
            if (hit.kind == OrientationTriadHit::Kind::None) return false;
            const QString text = availability == TriadNavigationAvailability::SketchLocked ?
                QStringLiteral("草图编辑中视角锁定；完成草图后可使用视图导航。") :
                hit.kind == OrientationTriadHit::Kind::Axis ?
                QStringLiteral("%1 轴：单击输入旋转角度；中键拖动绕轴旋转；Esc 退出。Alt+0 打开导航菜单。")
                    .arg(QChar('X' + hit.axis)) : FaceName(hit.axis, hit.sign) + QStringLiteral(" · 单击切换视图 · Alt+0 导航菜单");
            QToolTip::showText(help->globalPos(), text, m_viewport); return true;
        }
        if (event->type() == QEvent::MouseMove) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (m_dragging) {
                if (!(mouse->buttons() & Qt::MiddleButton)) FinishDrag(false);
                else DragTo(mouse->localPos());
                return true;
            }
            if (m_press_button != Qt::NoButton) {
                if (mouse->buttons().testFlag(m_press_button)) return true;
                InterruptPointerOperation();
            }
            if (mouse->buttons() != Qt::NoButton) return false;
            const auto hit = Hit(mouse->localPos());
            SetHover(availability == TriadNavigationAvailability::Available ? hit : OrientationTriadHit{});
            return hit.kind != OrientationTriadHit::Kind::None;
        }
        if (event->type() == QEvent::MouseButtonPress) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (m_dragging || m_press_button != Qt::NoButton) return true; // a second button cannot replace the gesture
            const auto hit = Hit(mouse->localPos());
            if (availability == TriadNavigationAvailability::SketchLocked)
                return hit.kind != OrientationTriadHit::Kind::None;
            SetHover(hit);
            if (mouse->button() == Qt::MiddleButton &&
                (m_axis >= 0 || hit.kind == OrientationTriadHit::Kind::Axis)) {
                SelectAxis(hit.kind == OrientationTriadHit::Kind::Axis ? hit.axis : m_axis, false);
                m_viewport->setFocus(Qt::MouseFocusReason);
                m_drag_start = mouse->localPos(); m_drag_rotation = m_camera.Orientation();
                const auto direction = OrientationTriadAxes(m_drag_rotation)[m_axis].direction;
                const auto length = std::hypot(direction.x(), direction.y());
                m_drag_tangent = length > .15 ? QPointF(-direction.y(), direction.x()) / length : QPointF(1, 0);
                m_dragging = true;
                // Test-owned hidden windows must never capture the user's
                // desktop mouse. A visible viewport uses Qt's normal grab.
                if (!m_viewport->testAttribute(Qt::WA_DontShowOnScreen) &&
                    !m_viewport->window()->testAttribute(Qt::WA_DontShowOnScreen))
                    m_viewport->grabMouse();
                return true;
            }
            if (mouse->button() == Qt::LeftButton || mouse->button() == Qt::RightButton) {
                if (hit.kind == OrientationTriadHit::Kind::None) { Cancel(false); return false; }
                m_pressed = hit; m_press_position = mouse->localPos(); m_press_button = mouse->button();
                return true;
            }
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* mouse = static_cast<QMouseEvent*>(event);
            if (m_dragging && mouse->button() == Qt::MiddleButton) {
                DragTo(mouse->localPos()); FinishDrag(false); return true;
            }
            if (m_press_button != Qt::NoButton && mouse->button() == m_press_button) {
                const auto pressed = m_pressed; const auto button = m_press_button;
                m_pressed = {}; m_press_button = Qt::NoButton;
                if (availability == TriadNavigationAvailability::Available && pressed == Hit(mouse->localPos()) &&
                    (mouse->localPos() - m_press_position).manhattanLength() < QApplication::startDragDistance()) {
                    if (button == Qt::RightButton) ShowMenu(mouse->globalPos());
                    else if (pressed.kind == OrientationTriadHit::Kind::Face) SnapFace(pressed.axis, pressed.sign);
                    else SelectAxis(pressed.axis, true);
                }
                return true;
            }
            if (availability == TriadNavigationAvailability::SketchLocked)
                return Hit(mouse->localPos()).kind != OrientationTriadHit::Kind::None;
        }
        if (event->type() == QEvent::MouseButtonDblClick || event->type() == QEvent::ContextMenu) {
            const auto point = event->type() == QEvent::MouseButtonDblClick ?
                static_cast<QMouseEvent*>(event)->localPos() :
                QPointF(static_cast<QContextMenuEvent*>(event)->pos());
            return Hit(point).kind != OrientationTriadHit::Kind::None;
        }
        if (event->type() == QEvent::Wheel && m_dragging) return true;
        return false;
    }

  private:
    TriadNavigationAvailability Availability() const {
        if (m_camera.IsOrbiting() && !m_dragging) return TriadNavigationAvailability::Busy;
        return m_availability ? m_availability() : TriadNavigationAvailability::Available;
    }
    OrientationTriadHit Hit(const QPointF& viewport_point) const {
        QFont font(MxFunctions::UiMetrics::uiFontFamily()); font.setStyleHint(QFont::SansSerif);
        if (m_hud.font().pointSizeF() > 0.0)
            font.setPointSizeF(std::max(7.5, m_hud.font().pointSizeF() * .85));
        else
            font.setPixelSize(std::max(10, int(std::lround(std::max(12, m_hud.font().pixelSize()) * .85))));
        return PickOrientationTriad(m_camera.Orientation(), m_hud.size(), QFontMetricsF(font).height(),
                                    viewport_point - m_hud.pos());
    }
    static QString FaceName(int axis, int sign) {
        static const char* names[] = {"左视图", "右视图", "前视图", "后视图", "底视图", "顶视图"};
        return QString::fromUtf8(names[axis * 2 + (sign > 0 ? 1 : 0)]);
    }
    void SetHover(OrientationTriadHit hit) {
        if (hit == m_hover) return;
        m_hover = hit;
        if (hit.kind != OrientationTriadHit::Kind::None) {
            if (!m_owns_cursor) {
                m_old_cursor = m_viewport->cursor();
                m_had_cursor = m_viewport->testAttribute(Qt::WA_SetCursor); m_owns_cursor = true;
            }
            m_viewport->setCursor(Qt::PointingHandCursor);
        } else RestoreCursor();
        m_hud.update();
    }
    void RestoreCursor() {
        if (!m_owns_cursor) return;
        if (m_viewport->cursor().shape() == Qt::PointingHandCursor) {
            if (m_had_cursor) m_viewport->setCursor(m_old_cursor); else m_viewport->unsetCursor();
        }
        m_owns_cursor = false;
    }
    void Changed() {
        m_last_orientation = m_camera.Orientation(); m_hud.update(); m_viewport->update();
    }
    void Cancel(bool restore_drag) {
        if (m_dragging) FinishDrag(restore_drag);
        m_axis = -1; m_pressed = {}; m_press_button = Qt::NoButton;
        if (m_panel) m_panel->hide();
        SetHover({}); m_hud.update();
    }
    void RestoreRotation() {
        m_camera.SetPlanarOrientation(m_drag_rotation.rotatedVector({0, 0, 1}),
                                     m_drag_rotation.rotatedVector({0, 1, 0}));
    }
    void DragTo(const QPointF& position) {
        RestoreRotation();
        m_camera.RotateAboutWorldAxis(OrientationTriadUnit(m_axis),
            QPointF::dotProduct(position - m_drag_start, m_drag_tangent) * .5);
        Changed();
    }
    void FinishDrag(bool restore) {
        m_dragging = false;
        if (restore) { RestoreRotation(); Changed(); }
        if (QWidget::mouseGrabber() == m_viewport) m_viewport->releaseMouse();
    }
    void SnapFace(int axis, int sign) {
        if (!IsAvailable()) return;
        Cancel(false);
        const auto up = axis == 2 ? QVector3D(0, float(sign), 0) : QVector3D(0, 0, 1);
        m_camera.SetPlanarOrientation(OrientationTriadUnit(axis) * sign, up); Changed();
        m_viewport->setFocus(Qt::OtherFocusReason);
    }
    void ApplyAngle() {
        // Intermediate text such as '-' must not silently fall back to the
        // previous accepted angle and rotate the view unexpectedly.
        if (m_axis < 0 || m_dragging || !IsAvailable() || !m_angle->hasAcceptableInput()) return;
        m_angle->interpretText();
        if (m_camera.RotateAboutWorldAxis(OrientationTriadUnit(m_axis), m_angle->value())) {
            Changed(); m_angle->selectAll();
        }
    }
    void SelectAxis(int axis, bool focus) {
        if (!IsAvailable()) return;
        if (!m_panel) {
            m_panel = new QFrame(m_viewport);
            m_panel->setObjectName(QStringLiteral("orientationTriadAnglePanel"));
            m_panel->setFrameShape(QFrame::StyledPanel);
            m_panel->setAutoFillBackground(true);
            QFont panel_font = m_hud.font(); panel_font.setFamily(MxFunctions::UiMetrics::uiFontFamily());
            panel_font.setStyleHint(QFont::SansSerif); panel_font.setItalic(false); panel_font.setBold(false);
            m_panel->setFont(panel_font);
            auto* layout = new QHBoxLayout(m_panel); layout->setContentsMargins(4, 3, 4, 3); layout->setSpacing(4);
            m_axis_label = new QLabel(m_panel); layout->addWidget(m_axis_label);
            m_angle = new QDoubleSpinBox(m_panel);
            m_angle->setObjectName(QStringLiteral("orientationTriadAngle"));
            m_angle->setRange(-360, 360); m_angle->setDecimals(2); m_angle->setSingleStep(15);
            m_angle->setSuffix(QStringLiteral("°")); m_angle->setKeyboardTracking(false);
            m_angle->setAccessibleName(QStringLiteral("视图相对旋转角度"));
            m_angle->setToolTip(QStringLiteral("绕选定世界轴相对旋转；Enter 应用，Esc 退出。"));
            layout->addWidget(m_angle);
            auto* apply = new QToolButton(m_panel);
            apply->setText(QStringLiteral("↵")); apply->setAutoRaise(true); apply->setMinimumSize(24, 24);
            apply->setAccessibleName(QStringLiteral("应用视图旋转")); apply->setToolTip(QStringLiteral("应用（Enter）"));
            layout->addWidget(apply);
            connect(apply, &QToolButton::clicked, this, [this] { ApplyAngle(); });
            for (auto* widget : {static_cast<QWidget*>(m_panel), static_cast<QWidget*>(m_angle),
                                 static_cast<QWidget*>(m_angle->findChild<QLineEdit*>()), static_cast<QWidget*>(apply)})
                widget->installEventFilter(this);
        }
        if (m_axis != axis) m_angle->setValue(0);
        m_axis = axis; m_axis_label->setText(QString(QChar('X' + axis)));
        m_panel->show(); PlacePanel(); m_hud.update();
        if (focus) { m_angle->setFocus(Qt::MouseFocusReason); m_angle->selectAll(); }
    }
    void ShowMenu(const QPoint& global_position) {
        if (!IsAvailable()) return;
        if (m_menu) delete m_menu.data();
        m_menu = new QMenu(m_viewport); m_menu->setObjectName(QStringLiteral("orientationTriadMenu"));
        QFont menu_font(MxFunctions::UiMetrics::uiFontFamily());
        menu_font.setPixelSize(MxFunctions::UiMetrics::bodyFontSize());
        m_menu->setFont(menu_font);
        for (int axis = 0; axis < 3; ++axis) for (int sign : {-1, 1})
            m_menu->addAction(FaceName(axis, sign), this, [this, axis, sign] { SnapFace(axis, sign); });
        m_menu->addAction(QStringLiteral("轴测视图"), this, [this] {
            if (!IsAvailable()) return;
            Cancel(false); m_camera.SetStandardView(OrientationTriadCamera::StandardView::Isometric); Changed();
        });
        m_menu->addSeparator();
        for (int axis = 0; axis < 3; ++axis)
            m_menu->addAction(QStringLiteral("绕 %1 轴旋转…").arg(QChar('X' + axis)), this,
                              [this, axis] { SelectAxis(axis, true); });
        m_menu->popup(global_position);
    }

    OrientationTriadCamera& m_camera;
    QWidget& m_hud;
    QWidget* m_viewport;
    std::function<TriadNavigationAvailability()> m_availability;
    QQuaternion m_last_orientation, m_drag_rotation;
    OrientationTriadHit m_hover, m_pressed;
    int m_axis = -1;
    Qt::MouseButton m_press_button = Qt::NoButton;
    QPointF m_press_position, m_drag_start, m_drag_tangent;
    bool m_dragging = false, m_owns_cursor = false, m_had_cursor = false;
    QCursor m_old_cursor;
    QFrame* m_panel = nullptr;
    QDoubleSpinBox* m_angle = nullptr;
    QLabel* m_axis_label = nullptr;
    QShortcut* m_shortcut = nullptr;
    QPointer<QMenu> m_menu;
};
} // namespace MotionOcc
