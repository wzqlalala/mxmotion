#pragma once

#include <QMetaType>

namespace MxFunctions {

// The compatibility boundary uses the original named types. Motion JSON is
// converted explicitly by name; these values are never substituted for an
// unverified external RebuildFunctions binary enum.
enum class PickDataType {
    None = 0, GeoPoint, GeoEdge, GeoFace, GeoSolid,
    Node, ElementEdge, ElementFace, Element, Part
};
enum SetType {
    Mix = 0, GeoPoint, GeoEdge, GeoFace, GeoSolid,
    Node, NodeSurface, Element, ElementSurface, ElementEdge, Part
};

inline uint qHash(PickDataType type, uint seed = 0) noexcept
{
    return ::qHash(static_cast<int>(type), seed);
}
} // namespace MxFunctions

Q_DECLARE_METATYPE(MxFunctions::PickDataType)
