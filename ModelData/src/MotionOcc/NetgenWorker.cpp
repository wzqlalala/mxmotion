// Adapted from Motion/tools/AutomaticFlexMesher.cpp. Keep Netgen/OCCT's mutable
// meshing state out of the Qt process. Netgen 6.2.2505 (OCC-enabled) is a separate
// LGPL 2.1 library; its license and corresponding source are in the donor SDK's
// share/motion-automatic-flex directory.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace nglib {
#include <nglib.h>
#include <nglib_occ.h>
}

namespace {

// Coordinates and element sizes are in millimetres. Match the Qt mesh-size
// control and the .vol reader's node, element and output-file limits.
constexpr double kMinH = 0.0001;
constexpr double kMaxH = 100000.0;
constexpr int kMaxPoints = 4000;
constexpr int kMaxElements = 20000;
constexpr std::uintmax_t kMaxVolBytes = 16 * 1024 * 1024;
constexpr std::uintmax_t kMaxBrepBytes = 16 * 1024 * 1024;

enum ExitCode { Usage = 2, InputError = 3, MeshError = 4, OutputError = 5, UnexpectedError = 6 };

int fail(ExitCode code, const char* message)
{
    std::cerr << "MxMotionMeshWorker: " << message << '\n';
    return code;
}

bool simpleFileName(std::string_view name, std::string_view suffix)
{
    // The caller sets a Unicode work directory via QProcess. Only these ASCII
    // basenames go through Netgen's narrow path API; no path resolution/encoding
    // of the working directory is needed here.
    if (name.size() <= suffix.size() || name.front() == '.' ||
        name.substr(name.size() - suffix.size()) != suffix)
        return false;
    return std::all_of(name.begin(), name.end(), [](char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
    });
}

bool parseMaxH(const char* text, double& h)
{
    std::istringstream input(text);
    input.imbue(std::locale::classic());
    return (input >> std::noskipws >> h) && input.eof() && std::isfinite(h) &&
           h >= kMinH && h <= kMaxH;
}

struct NetgenSession {
    NetgenSession() { nglib::Ng_Init(); }
    ~NetgenSession() { nglib::Ng_Exit(); }
};

struct MeshCounts {
    int points;
    int triangles;
    int tetrahedra;
};

MeshCounts meshCounts(nglib::Ng_Mesh* mesh)
{
    return {nglib::Ng_GetNP(mesh), nglib::Ng_GetNSE(mesh), nglib::Ng_GetNE(mesh)};
}

bool checkCounts(const MeshCounts& counts, bool requireSurface, bool requireVolume)
{
    if (counts.points > kMaxPoints || counts.triangles > kMaxElements || counts.tetrahedra > kMaxElements) {
        fail(MeshError, "Mesh exceeds 4000 nodes or 20000 elements of one type; increase maxh.");
        return false;
    }
    if (counts.points < 1 || counts.triangles < (requireSurface ? 1 : 0) ||
        (requireVolume ? counts.tetrahedra < 1 : counts.tetrahedra != 0)) {
        fail(MeshError, requireVolume ? "No tetrahedral mesh was generated; a closed supported solid is required."
                                     : "No usable boundary mesh was generated.");
        return false;
    }
    return true;
}

using Point = std::array<double, 3>;
using Triangle = std::array<int, 3>;
using Tetrahedron = std::array<int, 4>;

// Ng_GetVolumeElement can report NG_TET for an unrecognized arity in this SDK.
// Inspect its filled indices as well as the enum. The SDK's internal Element
// storage can hold 20 indices, more than nglib's advertised maximum.
template <std::size_t N, std::size_t Capacity>
bool readCell(const std::array<int, Capacity>& raw, int points, std::array<int, N>& nodes)
{
    if (raw[N] != -1 || !std::all_of(raw.begin(), raw.begin() + N, [points](int id) {
            return id > 0 && id <= points;
        }))
        return false;
    std::copy_n(raw.begin(), N, nodes.begin());
    std::sort(nodes.begin(), nodes.end());
    return std::adjacent_find(nodes.begin(), nodes.end()) == nodes.end();
}

double sixTimesVolume(const std::vector<Point>& points, const Tetrahedron& t)
{
    const auto& a = points[t[0] - 1];
    Point b{}, c{}, d{};
    for (std::size_t i = 0; i < 3; ++i) {
        b[i] = points[t[1] - 1][i] - a[i];
        c[i] = points[t[2] - 1][i] - a[i];
        d[i] = points[t[3] - 1][i] - a[i];
    }
    return b[0] * (c[1] * d[2] - c[2] * d[1]) -
           b[1] * (c[0] * d[2] - c[2] * d[0]) +
           b[2] * (c[0] * d[1] - c[1] * d[0]);
}

bool checkResult(nglib::Ng_Mesh* mesh, const MeshCounts& counts)
{
    std::vector<Point> points(counts.points);
    for (int i = 0; i < counts.points; ++i) {
        nglib::Ng_GetPoint(mesh, i + 1, points[i].data());
        if (!std::all_of(points[i].begin(), points[i].end(), [](double x) { return std::isfinite(x); }))
            return false;
    }
    std::set<Triangle> surface;
    for (int i = 1; i <= counts.triangles; ++i) {
        std::array<int, NG_SURFACE_ELEMENT_MAXPOINTS> raw;
        raw.fill(-1);
        Triangle face{};
        if (nglib::Ng_GetSurfaceElement(mesh, i, raw.data()) != nglib::NG_TRIG ||
            !readCell(raw, counts.points, face) || !surface.insert(face).second)
            return false;
    }
    if (counts.tetrahedra == 0)
        return true;

    std::set<Tetrahedron> volume;
    std::map<Triangle, int> tetFaces;
    for (int i = 1; i <= counts.tetrahedra; ++i) {
        std::array<int, 20> raw;
        raw.fill(-1);
        Tetrahedron tet{};
        if (nglib::Ng_GetVolumeElement(mesh, i, raw.data()) != nglib::NG_TET ||
            !readCell(raw, counts.points, tet) || !volume.insert(tet).second)
            return false;
        const double determinant = sixTimesVolume(points, tet);
        if (!std::isfinite(determinant) || determinant == 0.0)
            return false;
        for (std::size_t omit = 0; omit < tet.size(); ++omit) {
            Triangle face{};
            for (std::size_t j = 0, k = 0; j < tet.size(); ++j)
                if (j != omit)
                    face[k++] = tet[j];
            if (++tetFaces[face] > 2)
                return false;
        }
    }

    // Ng_GenerateVolumeMesh may return NG_OK even after giving up on an OCC
    // domain. Don't accept partial results: all surface triangles must be
    // represented and every exposed tet face must be a surface triangle.
    for (const auto& face : surface)
        if (tetFaces.count(face) == 0)
            return false;
    for (const auto& entry : tetFaces)
        if (entry.second == 1 && surface.count(entry.first) == 0)
            return false;
    return true;
}

bool checkSavedMesh(std::istream& file, const MeshCounts& counts, bool volume)
{
    // The OCC writer emits one face descriptor per CAD face, then tags each
    // triangle with that face's one-based boundary number. nglib can report
    // NG_OK even if a CAD face failed; reject missing faces and, for a volume
    // job, reject free faces that do not belong to any solid domain.
    std::string line;
    if (!(file >> std::ws))
        return false;
    const auto seekRecord = [&](const char* name) {
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            if (line == name)
                return true;
        }
        return false;
    };
    int faceCount = 0, triangleCount = 0;
    if (!seekRecord("facedescriptors") || !(file >> faceCount) ||
        faceCount < 1 || faceCount > counts.triangles ||
        !seekRecord("surfaceelementsuv") || !(file >> triangleCount) ||
        triangleCount != counts.triangles)
        return false;
    std::getline(file, line); // Rest of the triangle count line.
    std::vector<bool> represented(faceCount, false);
    for (int i = 0; i < triangleCount; ++i) {
        if (!std::getline(file, line))
            return false;
        std::istringstream row(line);
        int surface = 0, boundary = 0, inside = 0, outside = 0;
        if (!(row >> surface >> boundary >> inside >> outside) || boundary < 1 || boundary > faceCount ||
            (volume && inside < 1 && outside < 1))
            return false;
        represented[boundary - 1] = true;
    }
    return std::all_of(represented.begin(), represented.end(), [](bool present) { return present; }) &&
           seekRecord("endmesh");
}

bool saveResult(nglib::Ng_Mesh* mesh, const MeshCounts& counts, bool volume, const char* path)
{
    std::error_code error;
    // Use a fresh output path in the job directory; Ng_SaveMesh has no status
    // return, so an existing file could otherwise be mistaken for our result.
    const std::filesystem::path output(path);
    const auto status = std::filesystem::symlink_status(output, error);
    if ((!error && std::filesystem::exists(status)) ||
        (error && error != std::errc::no_such_file_or_directory))
        return false;
    nglib::Ng_SaveMesh(mesh, path);
    const auto bytes = std::filesystem::file_size(output, error);
    if (!error && bytes > 0 && bytes <= kMaxVolBytes) {
        std::ifstream file(output, std::ios::binary);
        if (checkSavedMesh(file, counts, volume))
            return true;
    }
    std::filesystem::remove(output, error);
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 5 || (std::string_view(argv[1]) != "surface" && std::string_view(argv[1]) != "volume") ||
        !simpleFileName(argv[2], ".brep") || !simpleFileName(argv[3], ".vol"))
        return fail(Usage, "Usage: MxMotionMeshWorker.exe <surface|volume> <input.brep> <output.vol> <maxh> (ASCII basenames)");
    const bool volume = std::string_view(argv[1]) == "volume";

    try {
        double h = 0.0;
        if (!parseMaxH(argv[4], h))
            return fail(Usage, "maxh must be a finite number from 0.0001 to 100000 mm.");
        const std::filesystem::path source(argv[2]);
        std::error_code error;
        if (!std::filesystem::is_regular_file(source, error))
            return fail(InputError, "Input BREP is not a readable regular file.");
        const auto inputBytes = std::filesystem::file_size(source, error);
        if (error || inputBytes == 0 || inputBytes > kMaxBrepBytes)
            return fail(InputError, "Input BREP is empty, unreadable, or exceeds 16 MiB.");

        const NetgenSession session;
        nglib::Ng_OCC_Geometry* geometry = nullptr;
        try {
            geometry = nglib::Ng_OCC_Load_BREP(argv[2]);
        } catch (const std::exception& e) {
            return fail(InputError, e.what());
        } catch (...) {
            return fail(InputError, "OCC could not read the BREP.");
        }
        if (!geometry)
            return fail(InputError, "OCC could not read the BREP.");

        // One job per process. Netgen 6.2.2505's Ng_DeleteMesh can double-free
        // boundary-name storage, so leave the mesh and geometry to process exit.
        auto* mesh = nglib::Ng_NewMesh();
        if (!mesh)
            return fail(MeshError, "Netgen could not allocate a mesh.");
        nglib::Ng_Meshing_Parameters parameters;
        parameters.maxh = h;
        parameters.minh = h * 0.02;
        parameters.grading = 0.3;
        parameters.second_order = 0;
        parameters.quad_dominated = 0;
        parameters.optsteps_3d = 3;

        if (nglib::Ng_OCC_SetLocalMeshSize(geometry, mesh, &parameters) != nglib::NG_OK)
            return fail(MeshError, "Netgen could not set local mesh sizes.");
        if (nglib::Ng_OCC_GenerateEdgeMesh(geometry, mesh, &parameters) != nglib::NG_OK)
            return fail(MeshError, "Netgen could not mesh the OCC edges.");
        if (!checkCounts(meshCounts(mesh), false, false))
            return MeshError;

        const auto surfaceStatus = nglib::Ng_OCC_GenerateSurfaceMesh(geometry, mesh, &parameters);
        auto counts = meshCounts(mesh);
        // This nglib version also returns NG_ERROR when triangles use only the
        // existing edge nodes. In that case check actual connectivity and face coverage.
        if (!checkCounts(counts, true, false))
            return MeshError;
        if ((surfaceStatus != nglib::NG_OK && surfaceStatus != nglib::NG_ERROR) || !checkResult(mesh, counts))
            return fail(MeshError, "Netgen did not generate a valid first-order triangle mesh.");
        if (volume) {
            if (nglib::Ng_GenerateVolumeMesh(mesh, &parameters) != nglib::NG_OK)
                return fail(MeshError, "Netgen could not mesh the volume; check that the solid is closed.");
            counts = meshCounts(mesh);
            if (!checkCounts(counts, true, true) || !checkResult(mesh, counts))
                return fail(MeshError, "Netgen did not generate a complete first-order tetrahedral mesh.");
        }

        if (!saveResult(mesh, counts, volume, argv[3]))
            return fail(OutputError, "Could not save a complete OCC mesh (all faces must be meshed; use a fresh .vol path; limit 16 MiB).");
        std::cout << "nodes=" << counts.points << " triangles=" << counts.triangles
                  << " tetrahedra=" << counts.tetrahedra << '\n';
        return 0;
    } catch (const std::exception& e) {
        return fail(UnexpectedError, e.what());
    } catch (...) {
        return fail(UnexpectedError, "Unexpected Netgen/OCC failure.");
    }
}
