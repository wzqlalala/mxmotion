// The Netgen .vol connectivity reader is adapted from ReadVolume in
// Motion/src/cad/AutomaticFlexMesher.cpp. This reads the interchange format;
// it does not link Netgen (LGPL 2.1) or import Motion's flexible-body machinery.
#include "FemMesh.h"

#include <QByteArray>
#include <QFile>
#include <QJsonArray>
#include <QList>
#include <QLocale>
#include <QSaveFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <set>
#include <type_traits>
#include <utility>

namespace MotionOcc {
namespace {

// Match the worker's output limits before allocating from file counts.
constexpr int kMaxPoints = 4000;
constexpr int kMaxElements = 20000;
constexpr qint64 kMaxVolBytes = 16 * 1024 * 1024;
constexpr qint64 kMaxLineBytes = 64 * 1024;

class VolRecords {
public:
    explicit VolRecords(QFile& file) : file_(file) {}

    QByteArray next()
    {
        while (!file_.atEnd()) {
            ++lineNumber;
            auto line = file_.readLine(kMaxLineBytes + 1);
            if (file_.error() != QFileDevice::NoError) {
                problem = file_.errorString();
                return {};
            }
            if (file_.pos() > kMaxVolBytes ||
                (line.size() == kMaxLineBytes && !line.endsWith('\n') && !file_.atEnd()) ||
                line.contains('\0')) {
                problem = QStringLiteral("Oversized or non-text .vol record.");
                return {};
            }
            if (lineNumber == 1 && line.startsWith("\xEF\xBB\xBF"))
                line.remove(0, 3);
            const auto comment = line.indexOf('#');
            if (comment >= 0)
                line.truncate(comment);
            line = line.trimmed();
            if (!line.isEmpty())
                return line;
        }
        return {};
    }

    qint64 lineNumber = 0;
    QString problem;

private:
    QFile& file_;
};

bool isCountedMetadata(const QByteArray& name)
{
    // Rows that carry no FEM connectivity. Keep their counts so surplus data
    // rows cannot be mistaken for the start of another mesh section.
    const char* const names[] = {
        "facedescriptors", "edgesegments", "edgesegmentsgi", "edgesegmentsgi2",
        "pointelements", "identifications", "identificationnames", "identificationtypes",
        "materials", "bcnames", "cd2names", "cd3names", "singular_points",
        "singular_edge_left", "singular_edge_right", "singular_face_inside",
        "singular_face_outside", "face_colours", "face_transparencies"
    };
    for (const auto* candidate : names)
        if (name == candidate)
            return true;
    return false;
}

bool isInteger(const QByteArray& value)
{
    bool ok = false;
    value.toInt(&ok);
    return ok;
}

bool readFinite(const QByteArray& value, double& number)
{
    bool ok = false;
    number = value.toDouble(&ok); // QByteArray numeric conversion uses the C locale.
    return ok && std::isfinite(number);
}

template <std::size_t N>
bool readIndices(const QList<QByteArray>& fields, int first, std::array<std::uint32_t, N>& cell)
{
    for (auto& index : cell) {
        bool ok = false;
        const int id = fields.at(first++).toInt(&ok);
        if (!ok || id < 1 || id > kMaxPoints)
            return false;
        index = static_cast<std::uint32_t>(id - 1);
    }
    return true;
}

template <std::size_t N>
bool validateCells(const std::vector<std::array<std::uint32_t, N>>& cells,
                   std::size_t pointCount, const QString& description, QString& error)
{
    std::set<std::array<std::uint32_t, N>> seen;
    for (std::size_t i = 0; i < cells.size(); ++i) {
        auto key = cells[i];
        std::sort(key.begin(), key.end()); // Ignore winding for duplicate detection only.
        const auto row = static_cast<qulonglong>(i + 1);
        if (key.back() >= pointCount || std::adjacent_find(key.begin(), key.end()) != key.end()) {
            error = QStringLiteral("%1 %2 has an out-of-range or repeated node index.").arg(description).arg(row);
            return false;
        }
        if (!seen.insert(key).second) {
            error = QStringLiteral("%1 %2 duplicates another element.").arg(description).arg(row);
            return false;
        }
    }
    return true;
}

void orientTetrahedra(FemMesh& mesh)
{
    // Netgen normally writes negative signed tetrahedra. VTK's base face uses
    // the opposite convention, so normalize valid tets to right-handed order.
    for (auto& tet : mesh.tetrahedra) {
        const auto& a = mesh.nodes[tet[0]];
        std::array<double, 3> b{}, c{}, d{};
        for (std::size_t i = 0; i < 3; ++i) {
            b[i] = mesh.nodes[tet[1]][i] - a[i];
            c[i] = mesh.nodes[tet[2]][i] - a[i];
            d[i] = mesh.nodes[tet[3]][i] - a[i];
        }
        const double determinant = b[0] * (c[1] * d[2] - c[2] * d[1]) -
                                   b[1] * (c[0] * d[2] - c[2] * d[0]) +
                                   b[2] * (c[0] * d[1] - c[1] * d[0]);
        if (std::isfinite(determinant) && determinant < 0.0)
            std::swap(tet[1], tet[2]);
    }
}

template <std::size_t N>
void writeCells(QTextStream& stream, const std::vector<std::array<std::uint32_t, N>>& cells,
                int vtkType)
{
    stream << "CELLS " << static_cast<qulonglong>(cells.size()) << ' '
           << static_cast<qulonglong>(cells.size()) * (N + 1) << '\n';
    for (const auto& cell : cells) {
        stream << static_cast<int>(N);
        for (auto index : cell)
            stream << ' ' << index;
        stream << '\n';
    }
    stream << "CELL_TYPES " << static_cast<qulonglong>(cells.size()) << '\n';
    for (std::size_t i = 0; i < cells.size(); ++i)
        stream << vtkType << '\n';
}

} // namespace

bool readNetgenMesh(const QString& path, MeshKind kind, FemMesh& mesh, QString& error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        error = file.errorString();
        return false;
    }
    if (file.size() > kMaxVolBytes) {
        error = QStringLiteral("Netgen mesh exceeds the 16 MiB limit.");
        return false;
    }

    FemMesh parsed;
    VolRecords records(file);
    const auto fail = [&](const QString& reason) {
        error = QStringLiteral("Netgen .vol line %1: %2").arg(records.lineNumber)
                    .arg(records.problem.isEmpty() ? reason : records.problem);
        return false;
    };
    bool havePoints = false, haveSurface = false, haveVolume = false;
    int dimension = 3;
    for (auto name = records.next(); !name.isEmpty(); name = records.next()) {
        if (name == "endmesh") // A serialized OCC geometry archive may follow it.
            break;
        if (name == "mesh3d")
            continue;
        if (name == "dimension" || name == "geomtype") {
            bool ok = false;
            const int value = records.next().toInt(&ok);
            if (!ok || (name == "dimension" && value != 2 && value != 3))
                return fail(QStringLiteral("Invalid .vol header value."));
            if (name == "dimension")
                dimension = value;
            continue;
        }

        const bool points = name == "points";
        const bool volume = name == "volumeelements";
        const bool uv = name == "surfaceelementsuv";
        const bool gi = name == "surfaceelementsgi";
        const bool surface = uv || gi || name == "surfaceelements";
        if (!points && !volume && !surface && !isCountedMetadata(name))
            return fail(QStringLiteral("Unexpected section or surplus data row."));

        bool ok = false;
        const int count = records.next().toInt(&ok);
        const int limit = points ? kMaxPoints : (surface || volume ? kMaxElements : int(kMaxVolBytes));
        if (!ok || count < 0 || count > limit)
            return fail(QStringLiteral("Invalid or excessive section count."));
        if ((points && havePoints) || (surface && haveSurface) || (volume && haveVolume))
            return fail(QStringLiteral("Duplicate points or element section."));

        if (points) {
            havePoints = true;
            parsed.nodes.reserve(count);
        } else if (surface) {
            haveSurface = true;
            parsed.surface.reserve(count);
        } else if (volume) {
            haveVolume = true;
            parsed.tetrahedra.reserve(count);
        }

        for (int i = 0; i < count; ++i) {
            const auto record = records.next();
            if (record.isEmpty() || record == "endmesh")
                return fail(QStringLiteral("Truncated section."));
            const auto fields = record.simplified().split(' ');
            if (points) {
                std::array<double, 3> p{};
                if (fields.size() != 3 || !readFinite(fields.at(0), p[0]) ||
                    !readFinite(fields.at(1), p[1]) || !readFinite(fields.at(2), p[2]))
                    return fail(QStringLiteral("Invalid point coordinates."));
                parsed.nodes.push_back(p); // BREP was written in millimetres.
            } else if (volume) {
                std::array<std::uint32_t, 4> tetra{};
                if (fields.size() != 6 || !isInteger(fields.at(0)) || fields.at(1).toInt(&ok) != 4 ||
                    !ok || !readIndices(fields, 2, tetra))
                    return fail(QStringLiteral("Invalid first-order tetrahedron record."));
                parsed.tetrahedra.push_back(tetra);
            } else if (surface) {
                std::array<std::uint32_t, 3> triangle{};
                if (fields.size() != (uv ? 14 : (gi ? 11 : 8)) ||
                    !std::all_of(fields.cbegin(), fields.cbegin() + 4, isInteger) ||
                    fields.at(4).toInt(&ok) != 3 || !ok || !readIndices(fields, 5, triangle))
                    return fail(QStringLiteral("Invalid first-order triangle record."));
                for (int j = 8; j < fields.size(); ++j) {
                    double coordinate = 0.0;
                    if (uv ? !readFinite(fields.at(j), coordinate) : !isInteger(fields.at(j)))
                        return fail(QStringLiteral("Invalid triangle geometry metadata."));
                }
                parsed.surface.push_back(triangle);
            } else if (name == "identificationtypes") {
                // Netgen writes this table as a list of integers, not one row per type.
                if (fields.size() > count - i || !std::all_of(fields.cbegin(), fields.cend(), isInteger))
                    return fail(QStringLiteral("Invalid identification types."));
                i += fields.size() - 1;
            } else if (name != "identificationnames" && !isInteger(fields.front())) {
                return fail(QStringLiteral("Invalid or truncated metadata section."));
            }
        }
    }
    if (!records.problem.isEmpty())
        return fail(records.problem);
    if (parsed.nodes.empty() || parsed.surface.empty())
        return fail(QStringLiteral("Mesh is missing points or surface triangles."));
    if (kind == MeshKind::Volume && parsed.tetrahedra.empty())
        return fail(QStringLiteral("Mesh is missing volume tetrahedra."));
    if (dimension != 3 && !parsed.tetrahedra.empty())
        return fail(QStringLiteral("Volume elements require a three-dimensional mesh."));
    if (!validateCells(parsed.surface, parsed.nodes.size(), QStringLiteral("Triangle"), error) ||
        !validateCells(parsed.tetrahedra, parsed.nodes.size(), QStringLiteral("Tetrahedron"), error))
        return false;

    orientTetrahedra(parsed);
    mesh = std::move(parsed);
    error.clear();
    return true;
}

QJsonObject femMeshJson(const FemMesh& mesh)
{
    QJsonArray nodes, tetrahedra, surface;
    for (const auto& p : mesh.nodes) nodes.append(QJsonArray{p[0], p[1], p[2]});
    for (const auto& t : mesh.tetrahedra) tetrahedra.append(QJsonArray{int(t[0]), int(t[1]), int(t[2]), int(t[3])});
    for (const auto& t : mesh.surface) surface.append(QJsonArray{int(t[0]), int(t[1]), int(t[2])});
    return {{"nodes", nodes}, {"tetrahedra", tetrahedra}, {"surfaceTriangles", surface}};
}

bool readFemMeshJson(const QJsonObject& object, FemMesh& mesh, QString& error)
{
    const auto fail = [&](const QString& message) { error = message; return false; };
    const auto points = object.value("nodes").toArray();
    const auto tets = object.value("tetrahedra").toArray();
    if (points.isEmpty() || tets.isEmpty()) return fail(QStringLiteral("体网格缺少节点或四面体。"));
    FemMesh parsed;
    parsed.nodes.reserve(points.size());
    for (const auto& value : points) {
        const auto a = value.toArray(); const auto p = value.toObject();
        const std::array<double,3> point = value.isArray() && a.size()==3
            ? std::array<double,3>{a[0].toDouble(NAN), a[1].toDouble(NAN), a[2].toDouble(NAN)}
            : std::array<double,3>{p.value("x").toDouble(NAN), p.value("y").toDouble(NAN), p.value("z").toDouble(NAN)};
        if (!std::all_of(point.begin(), point.end(), [](double x) { return std::isfinite(x); }))
            return fail(QStringLiteral("体网格节点坐标无效。"));
        parsed.nodes.push_back(point);
    }
    const auto cells = [&](const QJsonArray& source, auto& target) {
        using Cell = typename std::decay_t<decltype(target)>::value_type;
        for (const auto& value : source) {
            const auto indices = value.toArray(); Cell cell{};
            if (indices.size()!=int(cell.size())) return false;
            for (std::size_t i=0; i<cell.size(); ++i) {
                const double index=indices[int(i)].toDouble(NAN);
                if (!std::isfinite(index) || index<0 || index>=points.size() || std::trunc(index)!=index) return false;
                cell[i]=static_cast<std::uint32_t>(index);
            }
            target.push_back(cell);
        }
        return true;
    };
    if (!cells(tets, parsed.tetrahedra) || !cells(object.value("surfaceTriangles").toArray(), parsed.surface))
        return fail(QStringLiteral("体网格单元必须使用有效的零基节点索引（tet4 / tri3）。"));
    if (!validateCells(parsed.tetrahedra, parsed.nodes.size(), QStringLiteral("Tetrahedron"), error)
        || !validateCells(parsed.surface, parsed.nodes.size(), QStringLiteral("Triangle"), error)) return false;
    orientTetrahedra(parsed);
    if (parsed.surface.empty()) {
        struct Face { std::array<std::uint32_t,3> nodes{}; int count=0; };
        std::map<std::array<std::uint32_t,3>, Face> faces;
        for (const auto& t : parsed.tetrahedra)
            for (const auto& indices : std::array<std::array<int,3>,4>{{{0,2,1}, {0,1,3}, {0,3,2}, {1,2,3}}}) {
                const std::array<std::uint32_t,3> face{t[indices[0]], t[indices[1]], t[indices[2]]};
                auto key=face; std::sort(key.begin(), key.end());
                auto& entry=faces[key]; entry.nodes=face; ++entry.count;
            }
        for (const auto& item : faces) if (item.second.count==1) parsed.surface.push_back(item.second.nodes);
    }
    mesh=std::move(parsed); error.clear(); return true;
}

bool exportMesh(const QString& path, const FemMesh& mesh, QString& error)
{
    const bool volume = !mesh.tetrahedra.empty();
    if (mesh.nodes.empty() || (!volume && mesh.surface.empty())) {
        error = QStringLiteral("There is no mesh to export.");
        return false;
    }
    // Legacy VTK's CELLS total is the number of indices plus one size per cell.
    constexpr auto maxVtkInt = (std::numeric_limits<std::int32_t>::max)();
    if (mesh.nodes.size() > std::size_t(maxVtkInt) ||
        (volume ? mesh.tetrahedra.size() > std::size_t(maxVtkInt / 5)
                : mesh.surface.size() > std::size_t(maxVtkInt / 4))) {
        error = QStringLiteral("Mesh exceeds the limits of legacy VTK.");
        return false;
    }
    for (const auto& point : mesh.nodes) {
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]) || !std::isfinite(point[2])) {
            error = QStringLiteral("Mesh contains non-finite point coordinates.");
            return false;
        }
    }
    if (!validateCells(mesh.surface, mesh.nodes.size(), QStringLiteral("Triangle"), error) ||
        !validateCells(mesh.tetrahedra, mesh.nodes.size(), QStringLiteral("Tetrahedron"), error))
        return false;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        error = file.errorString();
        return false;
    }
    {
        QTextStream stream(&file);
        stream.setLocale(QLocale::c());
        stream.setRealNumberPrecision(std::numeric_limits<double>::max_digits10);
        stream << "# vtk DataFile Version 3.0\nMxMotion FEM mesh (mm)\nASCII\nDATASET UNSTRUCTURED_GRID\n"
               << "POINTS " << static_cast<qulonglong>(mesh.nodes.size()) << " double\n";
        for (const auto& p : mesh.nodes)
            stream << p[0] << ' ' << p[1] << ' ' << p[2] << '\n';
        if (volume)
            writeCells(stream, mesh.tetrahedra, 10); // VTK_TETRA
        else
            writeCells(stream, mesh.surface, 5); // VTK_TRIANGLE
        stream.flush();
        if (stream.status() != QTextStream::Ok || file.error() != QFileDevice::NoError) {
            error = file.error() == QFileDevice::NoError
                ? QStringLiteral("Could not write VTK mesh.") : file.errorString();
            file.cancelWriting();
            return false;
        }
    }
    if (!file.commit()) {
        error = file.errorString();
        return false;
    }
    error.clear();
    return true;
}

} // namespace MotionOcc
