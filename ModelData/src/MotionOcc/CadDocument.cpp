#include "CadDocument.h"

// Adapted from Motion/src/cad/StepImporter.cpp (located face triangulations)
// and Motion/src/cad/CadGeometryExchange.cpp (streamed BREP and atomic output).
// This document keeps native OCCT topology and placed coordinates in millimetres.

#include <BRepAdaptor_Curve.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <IGESControl_Controller.hxx>
#include <IGESControl_Reader.hxx>
#include <IGESControl_Writer.hxx>
#include <IGESData_IGESModel.hxx>
#include <Interface_Check.hxx>
#include <Interface_CheckIterator.hxx>
#include <Interface_Static.hxx>
#include <OSD_OpenFile.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <STEPControl_Reader.hxx>
#include <STEPControl_Writer.hxx>
#include <Standard_Failure.hxx>
#include <StepData_ConfParameters.hxx>
#include <StepData_StepModel.hxx>
#include <TopExp_Explorer.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <Transfer_FinderProcess.hxx>
#include <UnitsMethods.hxx>
#include <XSControl_TransferReader.hxx>
#include <XSControl_TransferWriter.hxx>
#include <XSControl_WorkSession.hxx>

#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <mutex>
#include <new>
#include <ostream>
#include <streambuf>
#include <utility>

namespace MotionOcc {
namespace {

constexpr qint64 kMaxFileBytes = 128ll * 1024 * 1024;
constexpr std::size_t kMaxParts = 20000;
constexpr std::size_t kMaxTopology = 500000;
constexpr unsigned int kMaxGroupDepth = 64;
constexpr std::size_t kMaxVertices = 3000000;
constexpr std::size_t kMaxTriangles = 1000000;
constexpr std::size_t kMaxSegments = 2000000;
constexpr int kMaxEdgeSamples = 65536;
constexpr double kMillimetre = 1.0; // OCCT exchange unit factors are measured in mm.

// OCCT exchange uses shared registries and the IGES parser has global state.
// This serializes exchange calls from this module, not from unrelated OCCT users.
std::mutex exchangeMutex;

struct CadError { QString message; };

[[noreturn]] void fail(const QString& message)
{
    throw CadError{message};
}

template <class Operation>
bool runCad(const QString& context, QString& error, const Operation& operation)
{
    try {
        operation();
        error.clear();
        return true;
    } catch (const CadError& failure) {
        error = context + QStringLiteral("：") + failure.message;
    } catch (const Standard_Failure& failure) {
        const QString detail = QString::fromUtf8(failure.GetMessageString());
        error = context + QStringLiteral("：OCCT 内核错误。 ") + detail;
    } catch (const std::bad_alloc&) {
        error = context + QStringLiteral("：内存不足，请先简化模型。");
    } catch (const std::exception& failure) {
        error = context + QStringLiteral("：") + QString::fromUtf8(failure.what());
    } catch (...) {
        error = context + QStringLiteral("：发生未知错误。");
    }
    return false;
}

enum class Format { Step, Iges, Brep };

Format formatFor(const QString& path)
{
    if (path.isEmpty() || path.contains(QChar::Null))
        fail(QStringLiteral("文件路径为空或无效。"));
    const QString suffix = QFileInfo(path).suffix().toLower();
    if (suffix == QStringLiteral("step") || suffix == QStringLiteral("stp")) return Format::Step;
    if (suffix == QStringLiteral("iges") || suffix == QStringLiteral("igs")) return Format::Iges;
    if (suffix == QStringLiteral("brep") || suffix == QStringLiteral("brp")) return Format::Brep;
    fail(QStringLiteral("仅支持 STEP/STP、IGES/IGS 和 BREP/BRP 文件。"));
}

class IgesSettings {
public:
    explicit IgesSettings(bool reading)
        : m_unit(UnitsMethods::GetCasCadeLengthUnit()), m_reading(reading)
    {
        IGESControl_Controller::Init();
        if (m_reading)
            m_visible = Interface_Static::IVal("read.iges.onlyvisible");
        UnitsMethods::SetCasCadeLengthUnit(kMillimetre);
    }

    ~IgesSettings()
    {
        // Selecting IGES roots writes the visibility option back into the registry.
        if (m_reading)
            Interface_Static::SetIVal("read.iges.onlyvisible", m_visible);
        // IGES PrepareForTransfer() can change this from xstep.cascade.unit.
        UnitsMethods::SetCasCadeLengthUnit(m_unit);
    }

private:
    double m_unit;
    bool m_reading;
    int m_visible = 0;
};

TopoDS_Shape transferRoots(XSControl_Reader& reader)
{
    const Handle(Interface_Check)& header = reader.Model()->GlobalCheck();
    if ((!header.IsNull() && header->HasFailed()) ||
        !reader.WS()->ModelCheckList(Standard_False).IsEmpty(Standard_True))
        fail(QStringLiteral("文件的头部或实体记录无效。"));

    const int count = reader.NbRootsForTransfer();
    if (count <= 0)
        fail(QStringLiteral("文件没有可转换的几何根对象。"));
    if (std::size_t(count) > kMaxParts)
        fail(QStringLiteral("文件的几何根对象数量超过 20000。"));
    if (reader.TransferRoots() != count || !reader.WS()->TransferReader()->LastCheckList().IsEmpty(Standard_True))
        fail(QStringLiteral("至少一个几何对象转换失败，未导入部分结果。"));
    return reader.OneShape();
}

TopoDS_Shape readShape(const QFileInfo& file, Format format)
{
    const QByteArray utf8Path = file.absoluteFilePath().toUtf8();
    if (format == Format::Iges) {
        IgesSettings settings(true);
        IGESControl_Reader reader;
        reader.SetReadVisible(Standard_False);
        // In OCCT 7.8.1 ReadStream is not implemented by the IGES work library.
        // Its ReadFile forwards UTF-8 to IGESFile_Read -> OSD_OpenFile -> _wfopen
        // on Windows; neither a local-codepage conversion nor staging is needed.
        if (reader.ReadFile(utf8Path.constData()) != IFSelect_RetDone || reader.IGESModel().IsNull())
            fail(QStringLiteral("无法读取有效的 IGES 文件。"));
        auto& global = reader.IGESModel()->ChangeGlobalSection();
        global.SetCascadeUnit(kMillimetre);
        const double factor = global.UnitValue();
        if (!std::isfinite(factor) || factor <= 0.0 || global.UnitFlag() == 3)
            fail(QStringLiteral("IGES 文件的长度单位未知，无法转换为毫米。"));
        return transferRoots(reader);
    }

    std::ifstream input;
    OSD_OpenStream(input, utf8Path.constData(), std::ios::in | std::ios::binary);
    if (!input)
        fail(QStringLiteral("无法打开文件，请检查路径和读取权限。"));
    if (format == Format::Step) {
        STEPControl_Reader reader;
        StepData_ConfParameters parameters;
        parameters.ReadTessellated = StepData_ConfParameters::RWMode_Tessellated_Off;
        if (reader.ReadStream("model.step", parameters, input) != IFSelect_RetDone || reader.StepModel().IsNull())
            fail(QStringLiteral("无法读取有效的 STEP 文件。"));
        reader.SetSystemLengthUnit(kMillimetre);
        return transferRoots(reader);
    }

    TopoDS_Shape shape;
    BRepTools::Read(shape, input, BRep_Builder());
    if (!input || shape.IsNull())
        fail(QStringLiteral("无法读取有效的文本 BREP 文件。"));
    return shape; // BREP does not encode units; do not rescale it.
}

// Only organizational compounds are flattened. A compsolid, solid, sheet or
// wire remains a single part. TopoDS_Iterator composes location and orientation.
void collectParts(const TopoDS_Shape& shape, std::size_t available,
                  std::size_t& visited, std::vector<TopoDS_Shape>& parts, unsigned int depth = 0)
{
    if (depth > kMaxGroupDepth || ++visited > kMaxTopology)
        fail(QStringLiteral("几何分组过深或过大，请先简化模型。"));
    if (shape.IsNull())
        fail(QStringLiteral("文件包含空几何对象。"));
    if (shape.ShapeType() == TopAbs_COMPOUND) {
        TopoDS_Iterator child(shape);
        if (!child.More()) fail(QStringLiteral("文件包含没有几何的空分组。"));
        for (; child.More(); child.Next())
            collectParts(child.Value(), available, visited, parts, depth + 1);
    } else {
        if (parts.size() >= available)
            fail(QStringLiteral("零件数量超过 20000，请分批处理。"));
        parts.push_back(shape);
    }
}

struct DisplayBudget {
    std::size_t topology = 0;
    std::size_t vertices = 0;
    std::size_t triangles = 0;
    std::size_t segments = 0;
};

void charge(std::size_t& used, std::size_t count, std::size_t limit, const QString& message)
{
    if (count > limit - used) fail(message);
    used += count;
}

TopTools_IndexedMapOfShape displayTopology(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind, DisplayBudget& budget)
{
    TopTools_IndexedMapOfShape result;
    for (TopExp_Explorer it(shape, kind); it.More(); it.Next()) {
        // Count occurrences, not just unique edges/faces, to bound traversal work.
        if (++budget.topology > kMaxTopology)
            fail(QStringLiteral("面与边的拓扑数量过多，请先简化模型。"));
        result.Add(it.Current());
    }
    return result;
}

QVector3D displayPoint(const gp_Pnt& point)
{
    constexpr double maxFloat = (std::numeric_limits<float>::max)();
    for (double coordinate : {point.X(), point.Y(), point.Z()})
        if (!std::isfinite(coordinate) || std::abs(coordinate) > maxFloat)
            fail(QStringLiteral("几何坐标不是可显示的有限数值。"));
    return QVector3D(float(point.X()), float(point.Y()), float(point.Z()));
}

bool hasArea(const QVector3D& a, const QVector3D& b, const QVector3D& c)
{
    // Test the stored float vertices in double precision to avoid overflow and
    // to spot triangles collapsed by the conversion to the display coordinate type.
    const double ux = double(b.x()) - a.x(), uy = double(b.y()) - a.y(), uz = double(b.z()) - a.z();
    const double vx = double(c.x()) - a.x(), vy = double(c.y()) - a.y(), vz = double(c.z()) - a.z();
    return uy * vz != uz * vy || uz * vx != ux * vz || ux * vy != uy * vx;
}

void appendFace(const TopoDS_Face& face, DisplayMesh& display, DisplayBudget& budget)
{
    TopLoc_Location location;
    const auto& mesh = BRep_Tool::Triangulation(face, location);
    if (mesh.IsNull() || mesh->NbNodes() < 3 || mesh->NbTriangles() < 1)
        fail(QStringLiteral("至少一个曲面无法生成三角形，未导入不完整几何。"));
    charge(budget.vertices, std::size_t(mesh->NbNodes()), kMaxVertices, QStringLiteral("显示顶点超过 300 万个。"));
    charge(budget.triangles, std::size_t(mesh->NbTriangles()), kMaxTriangles, QStringLiteral("显示三角形超过 100 万个。"));
    const auto offset = std::uint32_t(display.vertices.size());
    const gp_Trsf& transform = location.Transformation();
    const bool reverse = (face.Orientation() == TopAbs_REVERSED) != transform.IsNegative();
    for (int i = 1; i <= mesh->NbNodes(); ++i)
        display.vertices.push_back(displayPoint(mesh->Node(i).Transformed(transform)));

    const auto before = display.triangles.size();
    for (int i = 1; i <= mesh->NbTriangles(); ++i) {
        int a, b, c;
        mesh->Triangle(i).Get(a, b, c);
        for (int node : {a, b, c})
            if (node < 1 || node > mesh->NbNodes())
                fail(QStringLiteral("曲面三角形索引无效。"));
        if (reverse) std::swap(b, c);
        const std::uint32_t ia = offset + std::uint32_t(a - 1);
        const std::uint32_t ib = offset + std::uint32_t(b - 1);
        const std::uint32_t ic = offset + std::uint32_t(c - 1);
        if (hasArea(display.vertices[ia], display.vertices[ib], display.vertices[ic]))
            display.triangles.push_back({{ia, ib, ic}});
    }
    if (display.triangles.size() == before)
        fail(QStringLiteral("曲面的三角形在显示精度下全部退化。"));
}

template <class PointAt>
bool appendPolyline(int count, const PointAt& pointAt, DisplayMesh& display, DisplayBudget& budget)
{
    if (count < 2) return false;
    if (count > kMaxEdgeSamples)
        fail(QStringLiteral("单条边的显示采样点过多，请先简化模型。"));
    charge(budget.vertices, std::size_t(count), kMaxVertices, QStringLiteral("显示顶点超过 300 万个。"));
    charge(budget.segments, std::size_t(count - 1), kMaxSegments, QStringLiteral("显示边线段超过 200 万个。"));
    const auto before = display.vertices.size();
    display.vertices.push_back(displayPoint(pointAt(1)));
    for (int i = 2; i <= count; ++i) {
        const QVector3D point = displayPoint(pointAt(i));
        if (point == display.vertices.back()) continue;
        const auto index = std::uint32_t(display.vertices.size());
        display.vertices.push_back(point);
        display.edges.push_back({{index - 1, index}});
    }
    if (display.vertices.size() > before + 1) return true;
    display.vertices.pop_back();
    return false;
}

void appendEdge(const TopoDS_Edge& edge, double deflection, DisplayMesh& display, DisplayBudget& budget)
{
    if (BRep_Tool::Degenerated(edge)) return; // e.g. a sphere pole, not a line.
    if (!BRep_Tool::IsGeometric(edge))
        fail(QStringLiteral("拓扑边缺少可交换的精确曲线。"));

    // Reuse an OCC boundary polygon where possible so the overlay follows the
    // meshed face boundary. This is not a walk over the triangle sides.
    TopLoc_Location location;
    Handle(Poly_PolygonOnTriangulation) polygon;
    Handle(Poly_Triangulation) mesh;
    BRep_Tool::PolygonOnTriangulation(edge, polygon, mesh, location);
    if (!polygon.IsNull() && !mesh.IsNull()) {
        const gp_Trsf& transform = location.Transformation();
        if (appendPolyline(polygon->NbNodes(), [&](int i) {
                const int node = polygon->Node(i);
                if (node < 1 || node > mesh->NbNodes()) fail(QStringLiteral("边界多段线索引无效。"));
                return mesh->Node(node).Transformed(transform);
            }, display, budget)) return;
    }

    const auto& polygon3d = BRep_Tool::Polygon3D(edge, location);
    if (!polygon3d.IsNull()) {
        const auto& nodes = polygon3d->Nodes();
        const gp_Trsf& transform = location.Transformation();
        if (appendPolyline(polygon3d->NbNodes(), [&](int i) {
                return nodes(nodes.Lower() + i - 1).Transformed(transform);
            }, display, budget)) return;
    }

    // Standalone wires usually have no cached polygon. BRepAdaptor_Curve
    // includes the OCC edge location and can also evaluate curves on a surface.
    const BRepAdaptor_Curve curve(edge);
    const double first = curve.FirstParameter(), last = curve.LastParameter();
    if (!std::isfinite(first) || !std::isfinite(last) || Precision::IsInfinite(first) ||
        Precision::IsInfinite(last) || last <= first)
        fail(QStringLiteral("拓扑边的曲线参数范围无效或无界。"));

    const bool linear = curve.GetType() == GeomAbs_Line;
    if (!linear) {
        GCPnts_QuasiUniformDeflection samples;
        try {
            samples.Initialize(curve, deflection, first, last, GeomAbs_C0);
        } catch (const Standard_Failure&) {
            // Use a bounded parameter sample if OCC cannot sample for deflection.
        }
        if (samples.IsDone() && appendPolyline(samples.NbPoints(), [&](int i) {
                return samples.Value(i);
            }, display, budget)) return;
    }
    const int count = linear ? 2 : 65;
    if (!appendPolyline(count, [&](int i) {
            return curve.Value(i == count ? last : first + (last - first) * double(i - 1) / (count - 1));
        }, display, budget))
        fail(QStringLiteral("拓扑边在显示精度下没有非零长度的线段。"));
}

DisplayMesh makeDisplay(const TopoDS_Shape& shape, DisplayBudget& budget)
{
    const auto faces = displayTopology(shape, TopAbs_FACE, budget);
    const auto edges = displayTopology(shape, TopAbs_EDGE, budget);
    if (faces.IsEmpty() && edges.IsEmpty())
        fail(QStringLiteral("几何不包含可显示的实体、曲面或线框；不支持孤立点或空拓扑。"));
    if (!BRepCheck_Analyzer(shape).IsValid())
        fail(QStringLiteral("BREP 拓扑或几何无效，请先修复源模型。"));
    for (int i = 1; i <= faces.Extent(); ++i)
        if (!BRep_Tool::IsGeometric(TopoDS::Face(faces(i))))
            fail(QStringLiteral("曲面仅有离散网格，没有可交换的精确曲面。"));

    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds, Standard_False);
    if (bounds.IsVoid() || bounds.IsOpen())
        fail(QStringLiteral("几何没有有效的有限包围盒。"));
    const double diagonal = std::sqrt(bounds.SquareExtent());
    if (!std::isfinite(diagonal) || diagonal <= 0.0)
        fail(QStringLiteral("几何尺寸无效。"));
    const double deflection = (std::max)(Precision::Confusion(), diagonal * 0.001);

    DisplayMesh display;
    if (!faces.IsEmpty()) {
        BRepMesh_IncrementalMesh mesher(shape, deflection, Standard_False, 0.25, Standard_False);
        if (!mesher.IsDone()) fail(QStringLiteral("OCCT 曲面离散失败。"));
        for (int i = 1; i <= faces.Extent(); ++i)
            appendFace(TopoDS::Face(faces(i)), display, budget);
    }
    for (int i = 1; i <= edges.Extent(); ++i)
        appendEdge(TopoDS::Edge(edges(i)), deflection, display, budget);
    if (display.triangles.empty() && display.edges.empty())
        fail(QStringLiteral("几何没有可显示的曲面或非退化线框。"));
    return display;
}

// Stream into an atomic file or an in-memory BREP payload with a byte cap.
class SaveBuffer final : public std::streambuf {
public:
    explicit SaveBuffer(QIODevice& device, qint64 limit = kMaxFileBytes) : m_device(device), m_limit(limit) {}
    bool tooLarge() const { return m_tooLarge; }
    qint64 written() const { return m_written; }

protected:
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
        if (size <= 0) return 0;
        if (size > m_limit - m_written) {
            m_tooLarge = true;
            return 0;
        }
        const qint64 count = m_device.write(data, qint64(size));
        if (count > 0) m_written += count;
        return count > 0 ? std::streamsize(count) : 0;
    }

    int_type overflow(int_type value) override
    {
        if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
        const char character = traits_type::to_char_type(value);
        return xsputn(&character, 1) == 1 ? value : traits_type::eof();
    }

private:
    QIODevice& m_device;
    qint64 m_limit;
    qint64 m_written = 0;
    bool m_tooLarge = false;
};

class BrepInputBuffer final : public std::streambuf {
public:
    explicit BrepInputBuffer(const QByteArray& bytes)
    {
        // std::streambuf's get area requires char*, but the input is never modified.
        auto* begin = const_cast<char*>(bytes.constData());
        setg(begin, begin, begin + bytes.size());
    }
};

// The project accepts only the v3, exact-geometry stream that it writes. Check
// the native framing, sizes and token lengths before handing the bytes to OCCT.
void checkProjectBrep(const QByteArray& bytes)
{
    static const QByteArray header("\nCASCADE Topology V3, (c) Open Cascade\n");
    if (!bytes.startsWith(header)) fail(QStringLiteral("不是受支持的 OCCT V3 BREP 数据。"));

    struct Table { const char* label; bool mesh; };
    const Table tables[] = {{"Locations", false}, {"Curve2ds", false}, {"Curves", false},
        {"Polygon3D", true}, {"PolygonOnTriangulations", true}, {"Surfaces", false},
        {"Triangulations", true}, {"TShapes", false}};
    int nextTable = 0;
    int tokenLength = 0;
    int lineStart = header.size();
    const auto* text = bytes.constData();
    for (int i = lineStart; i < bytes.size(); ++i) {
        const unsigned char c = text[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            tokenLength = 0;
        } else if (c < 0x21 || c > 0x7e || ++tokenLength > 128) {
            fail(QStringLiteral("BREP 含有无效或超长的文本字段。"));
        }
        if (c != '\n') continue;
        if (nextTable < int(std::size(tables))) {
            const auto& table = tables[nextTable];
            const int labelSize = int(std::char_traits<char>::length(table.label));
            if (i - lineStart > labelSize && text[lineStart + labelSize] == ' ' &&
                std::char_traits<char>::compare(text + lineStart, table.label, labelSize) == 0) {
                bool ok = false;
                const auto count = QByteArray::fromRawData(text + lineStart + labelSize + 1,
                    i - lineStart - labelSize - 1).toInt(&ok);
                if (!ok || count < 0 || count > int(kMaxTopology) || (table.mesh && count != 0) ||
                    (nextTable == int(std::size(tables)) - 1 && count == 0))
                    fail(QStringLiteral("BREP 表计数无效、超出上限或包含网格。"));
                ++nextTable;
            }
        }
        lineStart = i + 1;
    }
    if (nextTable != int(std::size(tables))) fail(QStringLiteral("BREP 缺少必需的数据表。"));
}

bool writeShape(const TopoDS_Shape& shape, Format format, std::ostream& output)
{
    switch (format) {
    case Format::Step: {
        STEPControl_Writer writer;
        const auto model = writer.Model();
        model->SetLocalLengthUnit(kMillimetre);
        model->SetWriteLengthUnit(kMillimetre);
        StepData_ConfParameters parameters = model->InternalParameters;
        parameters.WriteUnit = UnitsMethods_LengthUnit_Millimeter;
        parameters.WriteAssembly = StepData_ConfParameters::WriteMode_Assembly_On;
        parameters.WriteTessellated = StepData_ConfParameters::RWMode_Tessellated_Off;
        if (writer.Transfer(shape, STEPControl_AsIs, parameters) != IFSelect_RetDone ||
            !writer.WS()->TransferWriter()->FinderProcess()->CheckList(Standard_True).IsEmpty(Standard_True))
            fail(QStringLiteral("STEP 几何转换失败。"));
        return writer.WriteStream(output) == IFSelect_RetDone;
    }
    case Format::Iges: {
        IgesSettings settings(false);
        IGESControl_Writer writer("MM", 1); // Exact BREP mode (not face-only mode).
        writer.Model()->ChangeGlobalSection().SetCascadeUnit(kMillimetre);
        if (!writer.AddShape(shape) || !writer.TransferProcess()->CheckList(Standard_True).IsEmpty(Standard_True))
            fail(QStringLiteral("IGES 几何转换失败。"));
        return writer.Write(output);
    }
    case Format::Brep:
        BRepTools::Write(shape, output, Standard_False, Standard_False, TopTools_FormatVersion_CURRENT);
        return bool(output);
    }
    return false;
}

TopoDS_Shape copyForExport(const TopoDS_Shape& shape)
{
    // Compound insertion freezes the child's TShape, and neutral transfer can
    // process topology in place. Keep both effects off the document's shapes.
    BRepBuilderAPI_Copy copy(shape, Standard_True, Standard_False);
    if (!copy.IsDone()) fail(QStringLiteral("无法复制导出所需的精确几何。"));
    const auto& result = copy.Shape();
    if (result.IsNull()) fail(QStringLiteral("导出用几何副本为空。"));
    return result;
}

auto findPart(const std::vector<CadPart>& parts, int id) -> std::vector<CadPart>::const_iterator
{
    return std::lower_bound(parts.begin(), parts.end(), id,
        [](const CadPart& part, int key) { return part.id < key; });
}

int checkedId(std::uint64_t next)
{
    if (next > std::uint64_t((std::numeric_limits<int>::max)()))
        fail(QStringLiteral("零件编号已用尽，请创建新文档。"));
    return static_cast<int>(next);
}

bool validDimension(double value)
{
    return std::isfinite(value) && value > 0.0;
}

} // namespace

const std::vector<CadPart>& CadDocument::parts() const { return m_parts; }

const CadPart* CadDocument::part(int id) const
{
    const auto it = findPart(m_parts, id);
    return it != m_parts.end() && it->id == id ? &*it : nullptr;
}

void CadDocument::clear() { m_parts.clear(); }

bool CadDocument::remove(int id)
{
    const auto it = findPart(m_parts, id);
    if (it == m_parts.end() || it->id != id) return false;
    m_parts.erase(it);
    return true;
}

bool CadDocument::importFile(const QString& path, std::vector<int>& newIds, QString& error)
{
    return runCad(QStringLiteral("CAD 导入失败"), error, [&] {
        const Format format = formatFor(path);
        const QFileInfo file(path);
        if (!file.isFile() || file.size() <= 0)
            fail(QStringLiteral("文件不存在、不是普通文件或为空。"));
        if (file.size() > kMaxFileBytes)
            fail(QStringLiteral("文件超过 128 MiB 导入上限。"));

        TopoDS_Shape root;
        {
            const std::lock_guard<std::mutex> lock(exchangeMutex);
            root = readShape(file, format);
        }
        std::vector<TopoDS_Shape> shapes;
        std::size_t visited = 0;
        collectParts(root, kMaxParts - m_parts.size(), visited, shapes);
        checkedId(m_nextId + shapes.size() - 1);

        QString name = file.completeBaseName();
        if (name.trimmed().isEmpty()) name = QStringLiteral("导入零件");
        DisplayBudget budget;
        std::vector<CadPart> staged;
        std::vector<int> ids;
        staged.reserve(shapes.size());
        ids.reserve(shapes.size());
        for (std::size_t i = 0; i < shapes.size(); ++i) {
            const int id = checkedId(m_nextId + i);
            const QString partName = shapes.size() == 1 ? name : QStringLiteral("%1 (%2)").arg(name).arg(qulonglong(i + 1));
            try {
                staged.push_back({id, partName, shapes[i], makeDisplay(shapes[i], budget)});
            } catch (const CadError& failure) {
                fail(QStringLiteral("零件“%1”：%2").arg(partName, failure.message));
            }
            ids.push_back(id);
        }

        const std::size_t oldSize = m_parts.size();
        // Append with vector's geometric growth rather than reserve(size+1)
        // on repeated small imports. Roll back appends if committing throws.
        try {
            for (auto& item : staged) m_parts.push_back(std::move(item));
        } catch (...) {
            while (m_parts.size() > oldSize) m_parts.pop_back();
            throw;
        }
        m_nextId += ids.size();
        newIds.swap(ids);
    });
}

bool CadDocument::addPrimitive(Primitive primitive, double x, double y, double z, int& id, QString& error)
{
    return runCad(QStringLiteral("创建 CAD 基本体失败"), error, [&] {
        if (m_parts.size() >= kMaxParts) fail(QStringLiteral("文档零件数量已达 20000。"));
        const int next = checkedId(m_nextId);
        QString name;
        TopoDS_Shape shape;
        switch (primitive) {
        case Primitive::Box:
            if (!validDimension(x) || !validDimension(y) || !validDimension(z))
                fail(QStringLiteral("长方体的长、宽、高必须为有限正数。"));
            name = QStringLiteral("长方体");
            shape = BRepPrimAPI_MakeBox(x, y, z).Shape();
            break;
        case Primitive::Cylinder:
            if (!validDimension(x) || !validDimension(y))
                fail(QStringLiteral("圆柱的半径和高度必须为有限正数。"));
            name = QStringLiteral("圆柱");
            shape = BRepPrimAPI_MakeCylinder(x, y).Shape();
            break;
        case Primitive::Sphere:
            if (!validDimension(x)) fail(QStringLiteral("球体的半径必须为有限正数。"));
            name = QStringLiteral("球体");
            shape = BRepPrimAPI_MakeSphere(x).Shape();
            break;
        default:
            fail(QStringLiteral("未知的基本体类型。"));
        }
        if (shape.IsNull()) fail(QStringLiteral("OCCT 未生成有效的基本体。"));
        DisplayBudget budget;
        m_parts.push_back({next, QStringLiteral("%1 %2").arg(name).arg(next), shape, makeDisplay(shape, budget)});
        ++m_nextId;
        id = next;
    });
}

bool CadDocument::exportFile(const QString& path, int partId, QString& error) const
{
    return runCad(QStringLiteral("CAD 导出失败"), error, [&] {
        const Format format = formatFor(path);
        TopoDS_Shape shape;
        if (partId == -1) {
            if (m_parts.empty()) fail(QStringLiteral("文档没有可导出的零件。"));
            TopoDS_Compound compound;
            BRep_Builder builder;
            builder.MakeCompound(compound);
            for (const auto& item : m_parts) builder.Add(compound, copyForExport(item.shape));
            shape = compound;
        } else {
            const CadPart* item = part(partId);
            if (!item) fail(QStringLiteral("找不到要导出的零件。"));
            shape = copyForExport(item->shape);
        }

        QSaveFile file(path);
        file.setDirectWriteFallback(false);
        if (!file.open(QIODevice::WriteOnly)) fail(file.errorString());
        SaveBuffer buffer(file);
        std::ostream output(&buffer);
        bool written;
        {
            const std::lock_guard<std::mutex> lock(exchangeMutex);
            written = writeShape(shape, format, output);
        }
        output.flush();
        if (buffer.tooLarge()) fail(QStringLiteral("文件超过 128 MiB 导出上限，请分批导出。"));
        if (file.error() != QFileDevice::NoError) fail(file.errorString());
        if (!written || !output || buffer.written() == 0)
            fail(QStringLiteral("未能写入完整的几何文件。"));
        if (!file.commit()) fail(file.errorString());
    });
}

bool CadDocument::encodeProjectPart(const CadPart& part, QByteArray& brep, int maxBytes, QString& error)
{
    return runCad(QStringLiteral("编码 CAD 零件 %1 失败").arg(part.id), error, [&] {
        if (part.shape.IsNull()) fail(QStringLiteral("零件没有精确几何。"));
        if (maxBytes <= 0) fail(QStringLiteral("项目已达到 BREP 大小上限。"));
        QByteArray bytes;
        QBuffer device(&bytes);
        if (!device.open(QIODevice::WriteOnly)) fail(device.errorString());
        SaveBuffer buffer(device, maxBytes);
        std::ostream output(&buffer);
        output.imbue(std::locale::classic());
        bool written;
        {
            const std::lock_guard<std::mutex> lock(exchangeMutex);
            // Standalone stream preserves the location and orientation of the part.
            written = writeShape(part.shape, Format::Brep, output);
        }
        if (buffer.tooLarge()) fail(QStringLiteral("BREP 超过项目大小上限。"));
        if (!written || bytes.isEmpty()) fail(QStringLiteral("未能写入完整的 BREP 数据。"));
        checkProjectBrep(bytes);
        brep.swap(bytes);
    });
}

bool CadDocument::restoreProjectParts(const std::vector<ProjectPart>& parts, std::uint64_t nextId, QString& error)
{
    return runCad(QStringLiteral("恢复项目 CAD 几何失败"), error, [&] {
        if (parts.size() > kMaxParts || nextId == 0 || nextId > std::uint64_t((std::numeric_limits<int>::max)()) + 1)
            fail(QStringLiteral("项目零件数量或下一编号无效。"));

        std::vector<const ProjectPart*> sorted;
        sorted.reserve(parts.size());
        for (const auto& part : parts) sorted.push_back(&part);
        std::sort(sorted.begin(), sorted.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
        int lastId = 0;
        for (const auto* part : sorted) {
            if (part->id <= lastId || std::uint64_t(part->id) >= nextId)
                fail(QStringLiteral("无效或重复的项目零件编号：%1。" ).arg(part->id));
            lastId = part->id;
        }

        DisplayBudget budget;
        std::vector<CadPart> staged;
        staged.reserve(parts.size());
        for (const auto* part : sorted) {
            try {
                checkProjectBrep(part->brep);
                BrepInputBuffer buffer(part->brep);
                std::istream input(&buffer);
                input.imbue(std::locale::classic());
                input.exceptions(std::ios::failbit | std::ios::badbit);
                TopoDS_Shape shape;
                {
                    const std::lock_guard<std::mutex> lock(exchangeMutex);
                    BRepTools::Read(shape, input, BRep_Builder());
                }
                if (shape.IsNull() || (!input.eof() && !(input >> std::ws).eof()))
                    fail(QStringLiteral("无法读取单个完整的 BREP 形状。"));
                // A single budget covers the entire project, as in CAD import.
                staged.push_back({part->id, part->name, shape, makeDisplay(shape, budget)});
            } catch (const CadError& failure) {
                fail(QStringLiteral("零件 %1：%2").arg(part->id).arg(failure.message));
            }
        }
        m_parts.swap(staged);
        m_nextId = nextId;
    });
}

} // namespace MotionOcc
