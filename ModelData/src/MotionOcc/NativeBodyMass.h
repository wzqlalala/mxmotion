#pragma once

#include <QJsonObject>
#include <QString>
#include <QVector>

#include <array>
#include <vector>

namespace MotionOcc {

class CadDocument;

// The original automatic body panel uses steel when no density is supplied.
constexpr double kDefaultNativeBodyDensityKgMm3 = 7.85e-6;

struct NativeBodyMassProperties {
    double volumeMm3 = 0;
    double massKg = 0;
    std::array<double, 3> centerMm{}; // Initial global CAD coordinates.
    // Centroid tensor entries, in global CAD axes: xx, yy, zz, xy, xz, yz.
    std::array<double, 6> inertiaKgMm2{};
};

// Integrates the exact, placed OCC solids. Part IDs are treated as a set.
// On failure neither properties nor the document is changed.
bool nativeBodyMassProperties(const CadDocument& cad, const std::vector<int>& partIds,
                              double densityKgMm3, NativeBodyMassProperties& properties,
                              QString& error);

// A custom record is left byte-for-byte unchanged. For automatic records only
// mass, centerOfMass, inertia, inertiaConvention and densityKgMm3 are updated.
// Geometry references, names, user extensions and initial conditions are kept.
// Callers explicitly set isCustomMassProps=false when switching to automatic.
bool updateNativeBodyMass(const CadDocument& cad, const std::vector<int>& partIds,
                          QJsonObject& body, double densityKgMm3, QString& error);

inline bool updateNativeBodyMass(const CadDocument& cad, const QVector<int>& partIds,
                                QJsonObject& body, double densityKgMm3, QString& error)
{
    return updateNativeBodyMass(cad, std::vector<int>(partIds.cbegin(), partIds.cend()),
                                body, densityKgMm3, error);
}

} // namespace MotionOcc
