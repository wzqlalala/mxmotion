// Adapted from wzqlalala/Motion b636535a45895b193aa581afd4df10a9004c9b6e
// src/ui/navigation/ViewportOrientationTriad.h. Keep the source geometry and interaction contract.
#pragma once

#include "OrientationTriadGeometry.h"
#include "OrientationTriadCamera.h"
#include "OrientationTriadInteraction.h"

#include <QEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWidget>
#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <utility>
#include <vector>

namespace MotionOcc {
// A viewport-owned transparent HUD. Its companion input filter consumes only
// navigation handles, not the rectangular overlay's empty space.
class ViewportOrientationTriad final : public QWidget {
  public:
    ViewportOrientationTriad(OrientationTriadCamera& camera, QWidget* viewport,
                            std::function<TriadNavigationAvailability()> availability = {})
        : QWidget(viewport), m_camera(camera), m_orientation(camera.Orientation()) {
        setObjectName(QStringLiteral("viewportOrientationTriad"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setFocusPolicy(Qt::NoFocus);
        setAccessibleName(QStringLiteral("视图导航坐标 XYZ"));
        setAccessibleDescription(QStringLiteral("点立方体面切换视图，点 XYZ 输入旋转角度，中键绕轴旋转。Alt+0 打开导航菜单；不改变模型坐标。"));
        viewport->installEventFilter(this);
        m_interaction = new OrientationTriadInteraction(camera, *this, std::move(availability));
        Place();
    }
    ~ViewportOrientationTriad() override {
        parentWidget()->removeEventFilter(this);
        delete m_interaction;
        m_interaction = nullptr;
    }
    const OrientationTriadInteraction& Interaction() const { return *m_interaction; }
    void CancelNavigation() { m_interaction->CancelNavigation(); }
    void InterruptPointerOperation() { m_interaction->InterruptPointerOperation(); }

  protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == parentWidget()) {
            if (event->type() == QEvent::Resize || event->type() == QEvent::Show) Place();
            if (event->type() == QEvent::Paint || event->type() == QEvent::Show) {
                // Updating on every parent paint would make a transparent
                // child's repaint invalidate its parent in an endless loop.
                const auto orientation = m_camera.Orientation();
                if (orientation != m_orientation) { m_orientation = orientation; update(); }
            }
        }
        return QWidget::eventFilter(watched, event);
    }

    void changeEvent(QEvent* event) override {
        QWidget::changeEvent(event);
        if (event->type() == QEvent::FontChange || event->type() == QEvent::ApplicationFontChange ||
            event->type() == QEvent::StyleChange) { Place(); update(); }
    }

    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);
        const auto axes = OrientationTriadAxes(m_orientation);
        const std::array<QColor, 3> colors{{QColor(196, 48, 53), QColor(37, 132, 59), QColor(47, 77, 194)}};
        const auto hover = m_interaction->Hover();
        const int selected_axis = m_interaction->SelectedAxis();
        if (!m_interaction->IsAvailable()) painter.setOpacity(.65);
        const QPointF origin(width() * .5, height() * .5);
        const qreal radius = width() * .28;
        const auto rotation = m_orientation.normalized().conjugated();
        const auto project = [&](const QVector3D& world) {
            const auto p = rotation.rotatedVector(world);
            return origin + QPointF(p.x(), -p.y()) * radius;
        };
        // Small orthographically projected meshes, painted into the transparent
        // HUD. No legacy GL, offscreen context, or scene/solver resources.
        struct Face { QPolygonF polygon; QColor color; float depth; bool cube; bool highlighted; };
        std::vector<Face> faces;
        faces.reserve(160);
        const QVector3D light = QVector3D(-.4f, .65f, 1).normalized();
        const auto add_face = [&](std::initializer_list<QVector3D> vertices,
                                  const QVector3D& normal, const QColor& color, bool cube = false, bool highlighted = false) {
            const auto camera_normal = rotation.rotatedVector(normal);
            if (camera_normal.z() <= 0) return; // only the outward, visible surface
            Face face{{}, color, 0, cube, highlighted};
            for (const auto& vertex : vertices) {
                face.polygon << project(vertex);
                face.depth += rotation.rotatedVector(vertex).z();
            }
            face.depth /= static_cast<float>(vertices.size());
            if (!cube) {
                const float shade = .66f + .44f * std::max(0.f, QVector3D::dotProduct(camera_normal, light));
                face.color = QColor::fromRgbF(std::min(1., color.redF() * shade),
                    std::min(1., color.greenF() * shade), std::min(1., color.blueF() * shade));
            }
            faces.push_back(std::move(face));
        };
        constexpr float half = .28f;
        const std::array<QVector3D, 3> unit{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
        // Faint rear edges preserve the small wireframe cube, without a HUD card.
        painter.setPen(QPen(QColor(130, 137, 149, 70), .7));
        for (int index = 0; index < 3; ++index) {
            const auto u = unit[(index + 1) % 3], v = unit[(index + 2) % 3];
            for (int a : {-1, 1}) for (int b : {-1, 1}) {
                const auto corner = (u * a + v * b) * half;
                painter.drawLine(project(corner - unit[index] * half), project(corner + unit[index] * half));
            }
            for (int sign : {-1, 1}) {
                const auto center = unit[index] * (half * sign);
                const bool highlighted = hover == OrientationTriadHit{OrientationTriadHit::Kind::Face, index, sign};
                add_face({center - u * half - v * half, center + u * half - v * half,
                          center + u * half + v * half, center - u * half + v * half},
                         unit[index] * sign, highlighted ? QColor(69, 155, 218, 115) : QColor(248, 249, 252, 72),
                         true, highlighted);
            }
            // Round shafts and conical heads keep the same 3D appearance in
            // every view, rather than rotating a flat screen-space arrow.
            constexpr int segments = 16;
            constexpr float pi = 3.14159265358979323846f;
            const auto shaft_start = unit[index] * half;
            const auto shoulder = unit[index] * .83f;
            const auto tip = unit[index] * 1.08f;
            const bool highlighted = (hover.kind == OrientationTriadHit::Kind::Axis && hover.axis == index) || selected_axis == index;
            const auto color = highlighted ? colors[index].lighter(135) : colors[index];
            for (int segment = 0; segment < segments; ++segment) {
                const float a = 2 * pi * segment / segments, b = 2 * pi * (segment + 1) / segments;
                const auto r0 = u * std::cos(a) + v * std::sin(a);
                const auto r1 = u * std::cos(b) + v * std::sin(b);
                const auto radial = (r0 + r1).normalized();
                add_face({shaft_start + r0 * .032f, shaft_start + r1 * .032f,
                          shoulder + r1 * .032f, shoulder + r0 * .032f}, radial, color, false, highlighted);
                add_face({shoulder + r0 * .098f, shoulder + r1 * .098f, tip},
                         (radial * .25f + unit[index] * .098f).normalized(), color, false, highlighted);
                add_face({shoulder + r0 * .032f, shoulder + r1 * .032f,
                          shoulder + r1 * .098f, shoulder + r0 * .098f}, -unit[index], color, false, highlighted);
            }
        }
        std::stable_sort(faces.begin(), faces.end(), [](const Face& a, const Face& b) { return a.depth < b.depth; });
        for (const auto& face : faces) {
            painter.setPen(face.cube ? (face.highlighted ? QPen(QColor(36, 115, 182), 1.3) :
                QPen(QColor(116, 124, 140, 170), .8)) :
                (face.highlighted ? QPen(face.color, .4) : QPen(Qt::NoPen)));
            painter.setBrush(face.color);
            painter.drawPolygon(face.polygon);
        }

        // Explicit sans-serif avoids the serif XYZ fallback of the old HUD.
        QFont label_font(QStringLiteral("Segoe UI"));
        label_font.setStyleHint(QFont::SansSerif);
        label_font.setPointSizeF(std::max(7.5, font().pointSizeF() * .85));
        const QFontMetricsF metrics(label_font);
        const auto labels = OrientationTriadLabelCenters(axes, size(), metrics.height());
        for (int index = 0; index < 3; ++index) {
            const auto direction = axes[index].direction;
            const qreal length = std::hypot(direction.x(), direction.y());
            const bool end_on = length < .08;
            const QString label(QChar('X' + index));
            const QPointF label_center = labels[index];
            label_font.setBold(selected_axis == index || (hover.kind == OrientationTriadHit::Kind::Axis && hover.axis == index));
            QPainterPath text;
            text.addText(label_center + QPointF(-metrics.horizontalAdvance(label) * .5,
                (metrics.ascent() - metrics.descent()) * .5), label_font, label);
            painter.strokePath(text, QPen(QColor(255, 255, 255, 200), 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.fillPath(text, colors[index]);
            // A compact dot/cross still distinguishes towards/away when an
            // axis is viewed end-on and its shaft projects to zero length.
            if (end_on) {
                painter.setBrush(QColor(250, 251, 253, 230)); painter.setPen(QPen(colors[index], 1));
                painter.drawEllipse(origin, 3.3, 3.3);
                if (axes[index].depth >= 0) {
                    painter.setPen(Qt::NoPen); painter.setBrush(colors[index]); painter.drawEllipse(origin, 1.4, 1.4);
                } else {
                    painter.drawLine(origin + QPointF(-1.6, -1.6), origin + QPointF(1.6, 1.6));
                    painter.drawLine(origin + QPointF(-1.6, 1.6), origin + QPointF(1.6, -1.6));
                }
            }
        }
    }

  private:
    void Place() {
        if (!parentWidget()) return;
        // Logical pixels: Qt supplies device-pixel scaling. Font enlargement
        // also expands the indicator so labels are not clipped at high DPI.
        // Keep the existing design, with a 20% larger projection and hit map.
        const int side = std::max(120, fontMetrics().height() * 6 + 24);
        resize(side, side);
        move(12, std::max(0, parentWidget()->height() - side - 12));
        raise();
        if (m_interaction) m_interaction->PlacePanel();
    }

    const OrientationTriadCamera& m_camera;
    QQuaternion m_orientation;
    OrientationTriadInteraction* m_interaction = nullptr;
};
} // namespace MotionOcc
