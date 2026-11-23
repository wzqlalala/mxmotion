// Adapted from wzqlalala/Motion b636535a45895b193aa581afd4df10a9004c9b6e
// src/ui/navigation/OrientationTriadGeometry.h. Keep the source geometry and interaction contract.
#pragma once

#include <QPointF>
#include <QQuaternion>
#include <QPolygonF>
#include <QSize>
#include <QVector3D>
#include <array>
#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace MotionOcc {
struct OrientationTriadAxis {
    QPointF direction;
    float depth = 0;
};

// Camera orientation maps camera-local axes to world. Only its inverse
// rotation belongs in a screen-corner indicator: never translation or zoom.
inline std::array<OrientationTriadAxis, 3> OrientationTriadAxes(const QQuaternion& orientation) {
    const auto inverse = orientation.normalized().conjugated();
    std::array<OrientationTriadAxis, 3> axes;
    for (int index = 0; index < 3; ++index) {
        QVector3D world;
        world[index] = 1;
        const auto camera = inverse.rotatedVector(world);
        axes[index] = {{camera.x(), -camera.y()}, camera.z()};
    }
    return axes;
}

struct OrientationTriadHit {
    enum class Kind { None, Axis, Face };
    Kind kind = Kind::None;
    int axis = -1;
    int sign = 0;
    bool operator==(const OrientationTriadHit& other) const {
        return kind == other.kind && axis == other.axis && sign == other.sign;
    }
    bool operator!=(const OrientationTriadHit& other) const { return !(*this == other); }
};

inline QVector3D OrientationTriadUnit(int axis) {
    QVector3D result;
    if (axis >= 0 && axis < 3) result[axis] = 1;
    return result;
}
inline QPointF OrientationTriadProject(const QQuaternion& orientation, const QSize& size, const QVector3D& point) {
    const auto p = orientation.normalized().conjugated().rotatedVector(point);
    return QPointF(size.width() * .5, size.height() * .5) + QPointF(p.x(), -p.y()) * (size.width() * .28);
}
inline QPointF OrientationTriadLabelCenter(const OrientationTriadAxis& axis, const QSize& size, qreal text_height) {
    const QPointF origin(size.width() * .5, size.height() * .5);
    const qreal radius = size.width() * .28;
    const auto length = std::hypot(axis.direction.x(), axis.direction.y());
    return length < .08 ? origin + QPointF(-.45, .55) * radius :
        origin + axis.direction * (radius * 1.08) + axis.direction / length * (text_height * .5 + 4);
}
inline std::array<QPointF, 3> OrientationTriadLabelCenters(const std::array<OrientationTriadAxis, 3>& axes,
                                                         const QSize& size, qreal text_height) {
    std::array<QPointF, 3> centers;
    for (int axis = 0; axis < 3; ++axis) centers[axis] = OrientationTriadLabelCenter(axes[axis], size, text_height);
    const qreal spacing = std::max(18., text_height * 1.35);
    const qreal margin = std::max(8., text_height * .55) + 2;
    for (int iteration = 0; iteration < 8; ++iteration) {
        for (int a = 0; a < 3; ++a) for (int b = a + 1; b < 3; ++b) {
            const auto delta = centers[b] - centers[a];
            const qreal distance = std::hypot(delta.x(), delta.y());
            if (distance >= spacing) continue;
            const auto radial = (centers[a] + centers[b]) * .5 - QPointF(size.width() * .5, size.height() * .5);
            const auto radial_length = std::hypot(radial.x(), radial.y());
            auto direction = radial_length > .01 ? QPointF(-radial.y(), radial.x()) / radial_length : QPointF(0, 1);
            if (QPointF::dotProduct(direction, delta) < 0) direction = -direction;
            // Separate sideways, not back along the shaft where the shorter
            // axis's letter would become obscured by the other arrow.
            const auto shift = direction * (std::sqrt(spacing * spacing - distance * distance) * .5 + .1);
            centers[a] -= shift; centers[b] += shift;
        }
        for (auto& center : centers) center = {std::clamp(center.x(), margin, size.width() - margin),
                                               std::clamp(center.y(), margin, size.height() - margin)};
    }
    return centers;
}

// A face ID always retains its world axis and sign. Picking screen-space
// bounding boxes instead would select the wrong side after an arbitrary orbit.
struct OrientationTriadFace {
    OrientationTriadHit hit;
    QPolygonF polygon;
    float depth = 0;
};
inline std::vector<OrientationTriadFace> OrientationTriadFaces(const QQuaternion& orientation, const QSize& size) {
    std::vector<OrientationTriadFace> faces;
    const auto inverse = orientation.normalized().conjugated();
    for (int axis = 0; axis < 3; ++axis) for (int sign : {-1, 1}) {
        const auto normal = OrientationTriadUnit(axis) * sign;
        if (inverse.rotatedVector(normal).z() <= 1e-5f) continue;
        const auto center = normal * .28f;
        const auto u = OrientationTriadUnit((axis + 1) % 3) * .28f;
        const auto v = OrientationTriadUnit((axis + 2) % 3) * .28f;
        OrientationTriadFace face{{OrientationTriadHit::Kind::Face, axis, sign}, {}, inverse.rotatedVector(center).z()};
        for (const auto& p : {center - u - v, center + u - v, center + u + v, center - u + v})
            face.polygon << OrientationTriadProject(orientation, size, p);
        faces.push_back(std::move(face));
    }
    return faces;
}

inline OrientationTriadHit PickOrientationTriad(const QQuaternion& orientation, const QSize& size,
                                               qreal text_height, const QPointF& point) {
    if (!QRectF(QPointF(), QSizeF(size)).contains(point)) return {};
    const auto axes = OrientationTriadAxes(orientation);
    const auto labels = OrientationTriadLabelCenters(axes, size, text_height);
    // Letters remain selectable even for the hidden/along-view axis.
    for (int axis = 0; axis < 3; ++axis) {
        const auto center = labels[axis];
        const qreal half = std::max(8., text_height * .55);
        if (QRectF(center - QPointF(half, half), QSizeF(2 * half, 2 * half)).contains(point))
            return {OrientationTriadHit::Kind::Axis, axis, 1};
    }
    OrientationTriadHit face_hit;
    float depth = -2;
    for (const auto& face : OrientationTriadFaces(orientation, size))
        if (face.depth > depth && face.polygon.containsPoint(point, Qt::OddEvenFill)) {
            face_hit = face.hit; depth = face.depth;
        }
    if (face_hit.kind != OrientationTriadHit::Kind::None) return face_hit;
    double distance = 6.;
    int closest = -1;
    for (int axis = 0; axis < 3; ++axis) {
        const auto a = OrientationTriadProject(orientation, size, OrientationTriadUnit(axis) * .32f);
        const auto b = OrientationTriadProject(orientation, size, OrientationTriadUnit(axis) * 1.08f);
        const auto edge = b - a;
        const double length2 = QPointF::dotProduct(edge, edge);
        if (length2 < 4) continue; // end-on axes are accessed through their letter
        const auto t = std::clamp(QPointF::dotProduct(point - a, edge) / length2, 0., 1.);
        const auto offset = point - (a + edge * t);
        const double d = std::hypot(offset.x(), offset.y());
        if (d < distance) { distance = d; closest = axis; }
    }
    return closest >= 0 ? OrientationTriadHit{OrientationTriadHit::Kind::Axis, closest, 1} : OrientationTriadHit{};
}
} // namespace MotionOcc
