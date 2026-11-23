#pragma once

#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector3D>
#include <cmath>
#include <functional>

namespace MotionOcc {

// Thin view-only adapter for Motion's orientation HUD. The donor camera stores
// camera-to-world rotation; OccViewport stores the inverse. The boundary below
// performs that conversion without changing the existing viewport or picker API.
// No model, geometry, mesh, pose, pan, zoom or solver state is owned here.
struct OrientationTriadCamera {
    enum class StandardView { Isometric };
    std::function<QQuaternion()> orientation;
    std::function<void(const QQuaternion&)> applyOrientation;
    std::function<bool()> isOrbiting;
    std::function<void()> isometric;

    QQuaternion Orientation() const { return orientation(); }
    bool IsOrbiting() const { return isOrbiting && isOrbiting(); }

    bool SetPlanarOrientation(const QVector3D& back, const QVector3D& up) {
        const auto finite = [](const QVector3D& v) {
            return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z());
        };
        if (!finite(back) || !finite(up) || back.lengthSquared() < 1e-12f || up.lengthSquared() < 1e-12f)
            return false;
        const auto normalizedBack = back.normalized();
        const auto projectedUp = up - QVector3D::dotProduct(up, normalizedBack) * normalizedBack;
        if (projectedUp.lengthSquared() < 1e-12f) return false;
        QMatrix4x4 view;
        view.lookAt(normalizedBack, {}, projectedUp.normalized());
        const auto next = QQuaternion::fromRotationMatrix(view.normalMatrix()).conjugated().normalized();
        if (next.isNull()) return false;
        applyOrientation(next);
        return true;
    }

    bool RotateAboutWorldAxis(const QVector3D& axis, double degrees) {
        if (!std::isfinite(axis.x()) || !std::isfinite(axis.y()) || !std::isfinite(axis.z()) ||
            axis.lengthSquared() < 1e-12f || !std::isfinite(degrees)) return false;
        // Retain Motion's right-handed displayed-geometry rotation convention.
        const auto delta = QQuaternion::fromAxisAndAngle(axis.normalized(), float(-std::remainder(degrees, 360.0)));
        applyOrientation((delta * Orientation()).normalized());
        return true;
    }

    void SetStandardView(StandardView) { if (isometric) isometric(); }
};

} // namespace MotionOcc
