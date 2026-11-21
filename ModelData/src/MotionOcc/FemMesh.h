#pragma once

#include <QString>
#include <QJsonObject>

#include <array>
#include <cstdint>
#include <vector>

namespace MotionOcc {

enum class MeshKind { Surface, Volume };

struct FemMesh {
    std::vector<std::array<double, 3>> nodes; // XYZ in millimetres, with no unit conversion.
    // Connectivity uses zero-based indices into nodes, not Netgen's one-based IDs.
    std::vector<std::array<std::uint32_t, 3>> surface;
    std::vector<std::array<std::uint32_t, 4>> tetrahedra;
};

// Reads Netgen ASCII .vol connectivity. Both modes require triangles; Volume also
// requires tetrahedra. Netgen tetrahedral winding is normalized for VTK.
// On failure, mesh is unchanged and error describes why.
bool readNetgenMesh(const QString& path, MeshKind kind, FemMesh& mesh, QString& error);

// The native AutoFlex JSON/project carries the same real mesh used by the
// viewport and solver. Missing surface triangles are recovered from boundary
// tetrahedron faces; no volume cells or node positions are invented.
QJsonObject femMeshJson(const FemMesh& mesh);
bool readFemMeshJson(const QJsonObject& object, FemMesh& mesh, QString& error);

// Writes a legacy ASCII VTK unstructured grid, using tetrahedra if present and
// triangles otherwise. An existing destination is left intact on failure.
bool exportMesh(const QString& path, const FemMesh& mesh, QString& error);

} // namespace MotionOcc
