#include "CadDocument.h"
#include "FemMesh.h"

#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStringList>
#include <QTemporaryDir>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace MotionOcc;
using Point = std::array<double, 3>;

[[noreturn]] void fail(const QString& message, const QString& detail = {})
{
    const auto text = detail.isEmpty() ? message : message + QStringLiteral(": ") + detail;
    throw std::runtime_error(text.toStdString());
}

void require(bool condition, const QString& message)
{
    if (!condition)
        fail(message);
}

void writeFixture(const QString& path, const QByteArray& bytes)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        fail(QStringLiteral("Could not open fixture %1").arg(path), file.errorString());
    if (file.write(bytes) != bytes.size() || !file.flush())
        fail(QStringLiteral("Could not write fixture %1").arg(path), file.errorString());
}

struct Bounds {
    static constexpr double inf = std::numeric_limits<double>::infinity();
    Point lower{inf, inf, inf};
    Point upper{-inf, -inf, -inf};

    void include(const Point& point, const QString& context)
    {
        for (std::size_t axis = 0; axis < point.size(); ++axis) {
            if (!std::isfinite(point[axis]))
                fail(context + QStringLiteral(" has a non-finite coordinate"));
            lower[axis] = (std::min)(lower[axis], point[axis]);
            upper[axis] = (std::max)(upper[axis], point[axis]);
        }
    }
};

void checkBounds(const Bounds& actual, const Bounds& expected, double tolerance, const QString& context)
{
    for (std::size_t axis = 0; axis < actual.lower.size(); ++axis) {
        const bool matches = std::abs(actual.lower[axis] - expected.lower[axis]) <= tolerance &&
                             std::abs(actual.upper[axis] - expected.upper[axis]) <= tolerance;
        if (!matches) {
            fail(QStringLiteral("%1 axis %2 bounds [%3, %4], expected [%5, %6]")
                     .arg(context).arg(static_cast<int>(axis)).arg(actual.lower[axis]).arg(actual.upper[axis])
                     .arg(expected.lower[axis]).arg(expected.upper[axis]));
        }
    }
}

template <std::size_t N>
void checkConnectivity(const std::vector<std::array<std::uint32_t, N>>& cells,
                       std::size_t pointCount, const QString& context)
{
    for (std::size_t row = 0; row < cells.size(); ++row) {
        auto sorted = cells[row];
        std::sort(sorted.begin(), sorted.end());
        if (sorted.back() >= pointCount || std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end()) {
            fail(QStringLiteral("%1 cell %2 has invalid node indices").arg(context).arg(static_cast<qulonglong>(row)));
        }
    }
}

int countFaces(const TopoDS_Shape& shape)
{
    int count = 0;
    for (TopExp_Explorer faces(shape, TopAbs_FACE); faces.More(); faces.Next())
        ++count;
    return count;
}

Bounds shapeBounds(const TopoDS_Shape& shape)
{
    // Compute bounds from analytic geometry, without reusing the display triangulation.
    Bnd_Box box;
    BRepBndLib::AddOptimal(shape, box, false, false);
    require(!box.IsVoid() && !box.IsOpen(), QStringLiteral("CAD shape has no finite bounds"));
    Bounds bounds;
    box.Get(bounds.lower[0], bounds.lower[1], bounds.lower[2], bounds.upper[0], bounds.upper[1], bounds.upper[2]);
    return bounds;
}

Bounds checkDisplay(const DisplayMesh& display, const QString& context)
{
    require(!display.vertices.empty() && !display.triangles.empty(),
            context + QStringLiteral(" has no display tessellation"));
    checkConnectivity(display.triangles, display.vertices.size(), context + QStringLiteral(" display triangles"));
    checkConnectivity(display.edges, display.vertices.size(), context + QStringLiteral(" display edges"));
    Bounds bounds;
    for (const auto& vertex : display.vertices)
        bounds.include({vertex.x(), vertex.y(), vertex.z()}, context);
    return bounds;
}

int addPrimitive(CadDocument& document, Primitive kind, const Point& dimensions,
                 int expectedFaces, const QString& name)
{
    int id = -1;
    QString error;
    if (!document.addPrimitive(kind, dimensions[0], dimensions[1], dimensions[2], id, error))
        fail(QStringLiteral("Could not create %1").arg(name), error);
    const auto* part = document.part(id);
    require(part && part->id == id && !part->name.isEmpty() && !part->shape.IsNull(),
            QStringLiteral("Missing CAD part for %1").arg(name));
    const int faces = countFaces(part->shape);
    require(part->shape.ShapeType() == TopAbs_SOLID && faces == expectedFaces,
            QStringLiteral("Unexpected solid topology for %1 (%2 faces; expected %3)")
                .arg(name).arg(faces).arg(expectedFaces));
    checkDisplay(part->display, name);
    qInfo().noquote() << name << ":" << faces << "CAD faces,"
                      << part->display.triangles.size() << "display triangles,"
                      << part->display.edges.size() << "display edges";
    return id;
}

void checkDocumentUnchanged(const CadDocument& document, const std::vector<CadPart>& original,
                            const QString& context)
{
    require(document.parts().size() == original.size(), context + QStringLiteral(" changed the part count"));
    for (std::size_t i = 0; i < original.size(); ++i) {
        const auto& before = original[i];
        const auto& after = document.parts()[i];
        require(before.id == after.id && before.name == after.name && before.shape.IsEqual(after.shape) &&
                    before.display.vertices == after.display.vertices &&
                    before.display.triangles == after.display.triangles && before.display.edges == after.display.edges,
                QStringLiteral("%1 changed CAD geometry or display data for part %2").arg(context).arg(before.id));
    }
}

QString checkCadRoundTrips(const CadDocument& document, int boxId, const Bounds& expected, const QDir& root)
{
    QString brepPath;
    const QStringList formats{QStringLiteral("step"), QStringLiteral("brep"), QStringLiteral("iges")};
    for (const auto& format : formats) {
        const QString path = root.filePath(QStringLiteral("箱体_échange.%1").arg(format));
        QString error;
        if (!document.exportFile(path, boxId, error))
            fail(QStringLiteral("CAD export failed: %1").arg(path), error);

        CadDocument imported;
        std::vector<int> ids;
        if (!imported.importFile(path, ids, error))
            fail(QStringLiteral("CAD import failed: %1").arg(path), error);
        require(ids.size() == 1 && imported.parts().size() == 1,
                format + QStringLiteral(" round trip did not contain exactly one part"));
        const auto* part = imported.part(ids.front());
        require(part && !part->shape.IsNull() && countFaces(part->shape) == 6,
                format + QStringLiteral(" round trip lost the box faces"));
        // IGES may represent the faces as a shell/compound instead of a solid.
        checkBounds(shapeBounds(part->shape), expected, 1e-4, format + QStringLiteral(" CAD"));
        checkBounds(checkDisplay(part->display, format), expected, 1e-4, format + QStringLiteral(" display"));
        qInfo().noquote() << format << "round trip: 6 CAD faces," << part->display.triangles.size()
                          << "display triangles";
        if (format == QStringLiteral("brep"))
            brepPath = path;
    }
    return brepPath;
}

QString runWorker(const QString& workerPath, const QDir& root, const QString& brepPath, const QString& mode)
{
    // Keep the actual worker arguments ASCII and relative, even in a Unicode job directory.
    const QString job = QStringLiteral("网格_%1").arg(mode);
    require(root.mkpath(job), QStringLiteral("Could not create the %1 worker directory").arg(mode));
    const QDir work(root.filePath(job));
    const QString sourceName = QStringLiteral("source.brep");
    const QString resultName = QStringLiteral("result.vol");
    QFile source(brepPath);
    if (!source.copy(work.filePath(sourceName)))
        fail(QStringLiteral("Could not stage the %1 worker BREP").arg(mode), source.errorString());

    QProcess process;
    process.setWorkingDirectory(work.absolutePath());
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(workerPath, QStringList{mode, sourceName, resultName, QStringLiteral("5")});
    if (!process.waitForStarted(5000))
        fail(QStringLiteral("Could not start the %1 Netgen worker").arg(mode), process.errorString());
    if (process.state() != QProcess::NotRunning && !process.waitForFinished(30000)) {
        const auto problem = process.errorString();
        process.kill();
        process.waitForFinished(3000);
        fail(QStringLiteral("%1 Netgen worker did not finish: %2").arg(mode, problem),
             QString::fromLocal8Bit(process.readAll().right(4096)));
    }
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        fail(QStringLiteral("%1 Netgen worker failed (exit code %2)").arg(mode).arg(process.exitCode()),
             QString::fromLocal8Bit(process.readAll().right(4096)));
    }
    return work.filePath(resultName);
}

FemMesh readWorkerMesh(const QString& path, MeshKind kind, const Bounds& expected, const QString& mode)
{
    FemMesh mesh;
    QString error;
    if (!readNetgenMesh(path, kind, mesh, error))
        fail(QStringLiteral("Could not read the %1 Netgen mesh").arg(mode), error);
    qInfo().noquote() << mode << "Netgen mesh:" << mesh.nodes.size() << "nodes," << mesh.surface.size()
                      << "triangles," << mesh.tetrahedra.size() << "tetrahedra";
    require(mesh.nodes.size() >= 8 && mesh.surface.size() >= 12,
            mode + QStringLiteral(" did not produce a triangulated box"));
    require(kind == MeshKind::Volume ? !mesh.tetrahedra.empty() : mesh.tetrahedra.empty(),
            mode + QStringLiteral(" has an unexpected tetrahedron count"));
    checkConnectivity(mesh.surface, mesh.nodes.size(), mode + QStringLiteral(" FE triangles"));
    checkConnectivity(mesh.tetrahedra, mesh.nodes.size(), mode + QStringLiteral(" FE tetrahedra"));
    Bounds bounds;
    for (const auto& node : mesh.nodes)
        bounds.include(node, mode);
    checkBounds(bounds, expected, 1e-3, mode + QStringLiteral(" FE mesh"));
    return mesh;
}

void checkInvalidVol(const QDir& root, const FemMesh& validMesh)
{
    const QString path = root.filePath(QStringLiteral("无效.vol"));
    // Netgen uses one-based node IDs. The final tetrahedron references missing node 5.
    writeFixture(path, "mesh3d\ndimension\n3\npoints\n4\n0 0 0\n1 0 0\n0 1 0\n0 0 1\n"
                       "surfaceelements\n4\n1 1 1 0 3 1 2 3\n1 1 1 0 3 1 4 2\n"
                       "1 1 1 0 3 1 3 4\n1 1 1 0 3 2 4 3\nvolumeelements\n1\n1 4 1 2 3 5\nendmesh\n");
    QString error;
    FemMesh retained = validMesh;
    require(!readNetgenMesh(path, MeshKind::Volume, retained, error), QStringLiteral("Accepted an invalid VOL"));
    require(retained.nodes == validMesh.nodes && retained.surface == validMesh.surface &&
                retained.tetrahedra == validMesh.tetrahedra,
            QStringLiteral("Rejected VOL replaced the existing FEM mesh"));
}

template <std::size_t N>
void checkVtk(const QString& path, const FemMesh& mesh,
              const std::vector<std::array<std::uint32_t, N>>& cells, int expectedType)
{
    QString error;
    if (!exportMesh(path, mesh, error))
        fail(QStringLiteral("VTK export failed: %1").arg(path), error);
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        fail(QStringLiteral("Could not open VTK %1").arg(path), file.errorString());
    std::istringstream in(file.readAll().toStdString());
    in.imbue(std::locale::classic());
    std::string line;
    require(bool(std::getline(in, line)) && line.rfind("# vtk DataFile Version ", 0) == 0,
            QStringLiteral("Missing VTK header: %1").arg(path));
    std::getline(in, line); // Title.
    require(bool(std::getline(in, line)) && line == "ASCII", QStringLiteral("Expected ASCII VTK: %1").arg(path));

    std::string keyword, value;
    std::size_t count = 0, total = 0;
    require(bool(in >> keyword >> value) && keyword == "DATASET" && value == "UNSTRUCTURED_GRID",
            QStringLiteral("Expected a VTK unstructured grid: %1").arg(path));
    require(bool(in >> keyword >> count >> value) && keyword == "POINTS" && count == mesh.nodes.size() &&
                (value == "double" || value == "float"),
            QStringLiteral("Invalid VTK POINTS header: %1").arg(path));
    const auto badPoint = QStringLiteral("VTK point coordinates differ from Netgen: %1").arg(path);
    for (const auto& expected : mesh.nodes) {
        Point point{};
        require(bool(in >> point[0] >> point[1] >> point[2]) &&
                    std::abs(point[0] - expected[0]) <= 1e-6 && std::abs(point[1] - expected[1]) <= 1e-6 &&
                    std::abs(point[2] - expected[2]) <= 1e-6, badPoint);
    }

    require(bool(in >> keyword >> count >> total) && keyword == "CELLS" && count == cells.size() &&
                total == cells.size() * (N + 1),
            QStringLiteral("Invalid VTK CELLS header: %1").arg(path));
    const auto badCell = QStringLiteral("VTK connectivity differs from Netgen: %1").arg(path);
    for (const auto& cell : cells) {
        std::size_t arity = 0;
        require(bool(in >> arity) && arity == N, badCell);
        for (auto expectedIndex : cell) {
            std::uint32_t index = 0;
            require(bool(in >> index) && index == expectedIndex, badCell);
        }
    }

    require(bool(in >> keyword >> count) && keyword == "CELL_TYPES" && count == cells.size(),
            QStringLiteral("Invalid VTK CELL_TYPES header: %1").arg(path));
    const auto badType = QStringLiteral("Expected VTK cell type %1: %2").arg(expectedType).arg(path);
    for (std::size_t i = 0; i < count; ++i) {
        int type = 0;
        require(bool(in >> type) && type == expectedType, badType);
    }
    qInfo().noquote() << QFileInfo(path).fileName() << ":" << count << "VTK cells of type" << expectedType;
}

void runSmokeTest(const QString& workerPath, const QDir& root)
{
    const Point boxSize{20.0, 15.0, 10.0};
    CadDocument document;
    const int boxId = addPrimitive(document, Primitive::Box, boxSize, 6, QStringLiteral("Box"));
    const int cylinderId = addPrimitive(document, Primitive::Cylinder, {5.0, 12.0, 0.0}, 3,
                                        QStringLiteral("Cylinder"));
    const int sphereId = addPrimitive(document, Primitive::Sphere, {5.0, 0.0, 0.0}, 1,
                                      QStringLiteral("Sphere"));
    require(document.parts().size() == 3 && boxId != cylinderId && boxId != sphereId && cylinderId != sphereId,
            QStringLiteral("Primitives did not create three distinct parts"));
    const auto* box = document.part(boxId);
    require(box && !box->display.edges.empty(), QStringLiteral("Box has no display edges"));
    const auto displayTriangleCount = box->display.triangles.size();
    const Bounds expected{{0.0, 0.0, 0.0}, boxSize};
    checkBounds(shapeBounds(box->shape), expected, 1e-6, QStringLiteral("Box CAD"));
    checkBounds(checkDisplay(box->display, QStringLiteral("Box")), expected, 1e-4, QStringLiteral("Box display"));
    const auto originalParts = document.parts();
    const QString brepPath = checkCadRoundTrips(document, boxId, expected, root);

    const QString badCad = root.filePath(QStringLiteral("损坏.brep"));
    writeFixture(badCad, "not a BREP file\n");
    std::vector<int> ids{boxId};
    QString error;
    require(!document.importFile(badCad, ids, error), QStringLiteral("Accepted a corrupt CAD file"));
    require(ids == std::vector<int>{boxId}, QStringLiteral("Corrupt CAD import replaced the caller's IDs"));
    checkDocumentUnchanged(document, originalParts, QStringLiteral("Corrupt CAD import"));

    const auto surfaceMode = QStringLiteral("surface");
    const auto volumeMode = QStringLiteral("volume");
    const auto surface = readWorkerMesh(runWorker(workerPath, root, brepPath, surfaceMode), MeshKind::Surface,
                                        expected, surfaceMode);
    const auto volume = readWorkerMesh(runWorker(workerPath, root, brepPath, volumeMode), MeshKind::Volume,
                                       expected, volumeMode);
    // The 5 mm FE size subdivides these planar box faces independently of OCC's display tessellation.
    require(surface.surface.size() > displayTriangleCount,
            QStringLiteral("Netgen FE mesh did not subdivide the box's display faces independently"));
    checkInvalidVol(root, volume);
    checkVtk(root.filePath(QStringLiteral("表面.vtk")), surface, surface.surface, 5); // VTK_TRIANGLE
    checkVtk(root.filePath(QStringLiteral("体积.vtk")), volume, volume.tetrahedra, 10); // VTK_TETRA
    checkDocumentUnchanged(document, originalParts, QStringLiteral("FE meshing and export"));
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() != 2 || !QFileInfo(args[1]).isAbsolute() || !QFileInfo(args[1]).isFile()) {
        qCritical().noquote() << "Usage: MotionOccBackendSmoke <absolute path to the Netgen worker executable>";
        return 2;
    }

    try {
        QTemporaryDir temporary;
        if (!temporary.isValid())
            fail(QStringLiteral("Could not create test directory"), temporary.errorString());
        runSmokeTest(args[1], QDir(temporary.path()));
        qInfo().noquote() << "MotionOcc backend smoke test passed";
        return 0;
    } catch (const Standard_Failure& failure) {
        qCritical() << "MotionOcc smoke test hit an OCCT exception:" << failure.GetMessageString();
    } catch (const std::exception& failure) {
        qCritical().noquote() << "MotionOcc smoke test failed:" << QString::fromUtf8(failure.what());
    }
    return 1;
}
