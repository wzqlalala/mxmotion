#pragma once

#include <QByteArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector3D>
#include <TopoDS_Shape.hxx>

#include <array>
#include <cstdint>
#include <utility>
#include <vector>

namespace MotionOcc {

struct RigidModel;

enum class Primitive { Box, Cylinder, Sphere };

struct DisplayMesh {
    // Millimetres in the shape's placed coordinate system; indices are zero-based.
    std::vector<QVector3D> vertices;
    std::vector<std::array<std::uint32_t, 3>> triangles;
    std::vector<std::array<std::uint32_t, 2>> edges;
    // Display-only unit normals, aligned with vertices. CAD faces retain
    // separate vertices at shared topological edges; line-only vertices use
    // zero normals. Empty is valid for older/custom display snapshots.
    std::vector<QVector3D> normals;
};

struct CadPart {
    int id;
    QString name;
    TopoDS_Shape shape;
    DisplayMesh display;
    QString sourceName;   // STEP product/instance name, without generated suffixes.
    QString instancePath; // Stable XCAF instance path within its source document.
};

// Single-threaded document API. IDs are positive and are not reused, even after clear().
// A pointer returned by part() may be invalidated by a successful mutation.
class CadDocument {
public:
    const std::vector<CadPart>& parts() const;
    const CadPart* part(int id) const;
    const QStringList& importNotes() const { return m_importNotes; }
    void clear();
    bool remove(int id);

    // Imports STEP, IGES or text BREP; unitless BREP coordinates are assumed to be mm.
    // On failure the document and newIds are unchanged.
    bool importFile(const QString& path, std::vector<int>& newIds, QString& error);
    // Box: x/y/z dimensions. Cylinder: x radius, y height. Sphere: x radius. All in mm.
    // Unused dimensions are ignored; id is written only on success.
    bool addPrimitive(Primitive primitive, double x, double y, double z, int& id, QString& error);
    // A positive ID exports that part; -1 exports all parts as a compound.
    bool exportFile(const QString& path, int partId, QString& error) const;
    bool exportNamedStep(const QString& path, const QMap<int, QString>& partNames, QString& error) const;
    // Restore logical IDs and display names after importing a named exchange assembly.
    bool restoreNamedStepParts(const QMap<QString, std::pair<int, QString>>& parts, QString& error);

private:
    // Project-only access preserves the part IDs and local coordinate frames.
    friend bool saveRigidProject(const QString&, const CadDocument&, const RigidModel&, QString&);
    friend bool loadRigidProject(const QString&, CadDocument&, RigidModel&, QString&);

    enum class ProjectPartEncoding { LegacyBrep, Step };
    struct ProjectPart {
        int id;
        QString name;
        QByteArray geometry;
        QString sourceName;
        QString instancePath;
        ProjectPartEncoding encoding = ProjectPartEncoding::LegacyBrep;
        std::array<double, 12> placement{};
    };
    static bool encodeProjectStep(const CadPart&, QByteArray& geometry, std::array<double, 12>& placement,
                                  int maxBytes, QString& error);
    bool restoreProjectParts(const std::vector<ProjectPart>& parts, std::uint64_t nextId, QString& error,
                             bool buildDisplay = true);

    std::vector<CadPart> m_parts;
    std::uint64_t m_nextId = 1;
    QStringList m_importNotes;
};

} // namespace MotionOcc
