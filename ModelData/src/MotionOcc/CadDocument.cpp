#include "CadDocument.h"
#include "OccTessellation.h"

// Adapted from Motion/src/cad/StepImporter.cpp (located face triangulations)
// and Motion/src/cad/CadGeometryExchange.cpp (streamed BREP and atomic output).
// This document keeps native OCCT topology and placed coordinates in millimetres.

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakeCylinder.hxx>
#include <BRepPrimAPI_MakeSphere.hxx>
#include <BRepTools.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GProp_GProps.hxx>
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
#include <STEPCAFControl_Reader.hxx>
#include <ShapeFix_Shape.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <TDF_LabelSequence.hxx>
#include <TDF_Tool.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
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
#include <gp_Cylinder.hxx>

#include <QBuffer>
#include <QByteArray>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <exception>
#include <fstream>
#include <functional>
#include <istream>
#include <iterator>
#include <limits>
#include <locale>
#include <mutex>
#include <new>
#include <ostream>
#include <optional>
#include <streambuf>
#include <utility>
#include <set>

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
struct DisplayBudgetError { QString message; };

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
    } catch (const DisplayBudgetError& failure) {
        error = context + QStringLiteral("：") + failure.message;
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
                  std::size_t& visited, std::vector<TopoDS_Shape>& parts, unsigned int depth = 0, bool allowEmptyGroups = false)
{
    if (depth > kMaxGroupDepth || ++visited > kMaxTopology)
        fail(QStringLiteral("几何分组过深或过大，请先简化模型。"));
    if (shape.IsNull())
        fail(QStringLiteral("文件包含空几何对象。"));
    if (shape.ShapeType() == TopAbs_COMPOUND) {
        TopoDS_Iterator child(shape);
        if (!child.More() && !allowEmptyGroups) fail(QStringLiteral("文件包含没有几何的空分组。"));
        for (; child.More(); child.Next())
            collectParts(child.Value(), available, visited, parts, depth + 1, allowEmptyGroups);
    } else {
        if (parts.size() >= available)
            fail(QStringLiteral("零件数量超过 20000，请分批处理。"));
        parts.push_back(shape);
    }
}

struct NamedShape {
    TopoDS_Shape shape;
    QString sourceName;
    QString instancePath;
};

QString labelName(const TDF_Label& label)
{
    Handle(TDataStd_Name) attr;
    if (!label.FindAttribute(TDataStd_Name::GetID(), attr)) return {};
    const auto& value = attr->Get();
    QString result;
    result.reserve(value.Length());
    for (int i = 1; i <= value.Length(); ++i) result.append(QChar(value.Value(i)));
    return result.trimmed();
}

// XCAF retains product names and repeated assembly instances. Every leaf shape
// is placed in world coordinates exactly once, matching the original JSON COMs.
std::vector<NamedShape> readStepParts(const QFileInfo& file, std::size_t available)
{
    STEPCAFControl_Reader reader;
    reader.SetNameMode(true);
    reader.SetColorMode(false);
    reader.SetLayerMode(false);
    std::ifstream input;
    const auto path = file.absoluteFilePath().toUtf8();
    OSD_OpenStream(input, path.constData(), std::ios::in | std::ios::binary);
    StepData_ConfParameters parameters;
    parameters.ReadTessellated = StepData_ConfParameters::RWMode_Tessellated_Off;
    if (!input || reader.ChangeReader().ReadStream("model.step", parameters, input) != IFSelect_RetDone)
        fail(QStringLiteral("无法读取有效的 STEP 装配文件。"));
    reader.ChangeReader().SetSystemLengthUnit(kMillimetre);
    Handle(TDocStd_Document) document = new TDocStd_Document("BinXCAF");
    if (!reader.Transfer(document) ||
        !reader.Reader().WS()->TransferReader()->LastCheckList().IsEmpty(Standard_True))
        fail(QStringLiteral("STEP 装配转换失败，未导入部分结果。"));
    const auto tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    TDF_LabelSequence roots;
    tool->GetFreeShapes(roots);
    if (roots.IsEmpty()) fail(QStringLiteral("STEP 没有可用的装配实例。"));
    std::vector<NamedShape> result;
    std::size_t visited = 0;
    std::function<void(const TDF_Label&, const TopLoc_Location&, const QString&, unsigned)> visit;
    visit = [&](const TDF_Label& instance, const TopLoc_Location& parent, const QString& prefix, unsigned depth) {
        if (depth > kMaxGroupDepth || ++visited > kMaxTopology)
            fail(QStringLiteral("STEP 装配层级过深或包含循环引用。"));
        TDF_Label definition = instance;
        if (tool->IsReference(instance) && !tool->GetReferredShape(instance, definition))
            fail(QStringLiteral("STEP 装配包含无法解析的实例引用。"));
        TCollection_AsciiString entry;
        TDF_Tool::Entry(instance, entry);
        const QString instancePath = prefix + QLatin1Char('/') + QString::fromLatin1(entry.ToCString());
        QString name = labelName(instance);
        if (name.isEmpty()) name = labelName(definition);
        const auto shape = tool->GetShape(instance);
        if (shape.IsNull()) fail(QStringLiteral("STEP 实例包含空几何。"));
        if (tool->IsAssembly(definition)) {
            TDF_LabelSequence children;
            tool->GetComponents(definition, children, false);
            const auto placement = parent * shape.Location();
            for (int i = 1; i <= children.Length(); ++i)
                visit(children.Value(i), placement, instancePath, depth + 1);
        } else {
            std::vector<TopoDS_Shape> leaves;
            // STEP exporters may include empty organizational products alongside
            // real parts. Skip only these empty groups, never invalid solids.
            collectParts(shape.Moved(parent), available - result.size(), visited, leaves, depth, true);
            for (std::size_t i = 0; i < leaves.size(); ++i)
                result.push_back({leaves[i], name, instancePath + QStringLiteral("/%1").arg(i + 1)});
        }
    };
    for (int i = 1; i <= roots.Length(); ++i) visit(roots.Value(i), {}, {}, 0);
    if (result.empty()) fail(QStringLiteral("STEP 装配没有几何零件。"));
    return result;
}

struct DisplayBudget {
    std::size_t topology = 0;
    std::size_t vertices = 0;
    std::size_t triangles = 0;
    std::size_t segments = 0;
};

TopoDS_Shape repairImportedShape(const TopoDS_Shape& shape, bool& repaired)
{
    repaired = false;
    if (BRepCheck_Analyzer(shape).IsValid()) return shape;
    // Heal interchange topology on a private copy. Accept it only if the
    // actual surface/solid measures and exact geometric bounds are unchanged.
    // This repairs STEP sewing/tolerance defects without changing dimensions.
    const bool solid = TopExp_Explorer(shape, TopAbs_SOLID).More();
    const auto signature = [solid](const TopoDS_Shape& value) {
        std::array<double, 16> out{};
        Bnd_Box bounds;
        BRepBndLib::AddOptimal(value, bounds, Standard_False, Standard_False);
        if (bounds.IsVoid() || bounds.IsOpen()) fail(QStringLiteral("无法验证 STEP 修复前后的几何边界。"));
        bounds.Get(out[0], out[1], out[2], out[3], out[4], out[5]);
        GProp_GProps properties;
        if (solid) BRepGProp::VolumeProperties(value, properties);
        else BRepGProp::SurfaceProperties(value, properties);
        out[6] = properties.Mass();
        const auto center = properties.CentreOfMass();
        out[7] = center.X(); out[8] = center.Y(); out[9] = center.Z();
        const auto inertia = properties.MatrixOfInertia();
        out[10] = inertia(1, 1); out[11] = inertia(2, 2); out[12] = inertia(3, 3);
        out[13] = inertia(1, 2); out[14] = inertia(1, 3); out[15] = inertia(2, 3);
        for (double number : out) if (!std::isfinite(number)) fail(QStringLiteral("STEP 修复的几何验证出现非有限数值。"));
        return out;
    };
    const auto before = signature(shape);
    BRepBuilderAPI_Copy copy(shape, Standard_True, Standard_False);
    ShapeFix_Shape fix(copy.Shape());
    fix.SetPrecision(Precision::Confusion());
    fix.SetMaxTolerance(0.001); // mm; never turn a real gap into a large-tolerance seam.
    fix.Perform();
    const auto fixed = fix.Shape();
    if (fixed.IsNull() || !BRepCheck_Analyzer(fixed).IsValid())
        fail(QStringLiteral("STEP 拓扑无法在保持几何尺寸的条件下修复。"));
    const auto after = signature(fixed);
    const double span = std::hypot(before[3] - before[0], before[4] - before[1], before[5] - before[2]);
    for (std::size_t i = 0; i < before.size(); ++i) {
        const double tolerance = i < 6 || (i >= 7 && i < 10)
            ? (std::max)(1e-6, span * 1e-8) : (std::max)(1e-8, std::abs(before[i]) * 1e-7);
        if (std::abs(before[i] - after[i]) > tolerance)
            fail(QStringLiteral("STEP 修复会改变几何尺寸、体积或惯量，请先核对源模型。"));
    }
    repaired = true;
    return fixed;
}

void charge(std::size_t& used, std::size_t count, std::size_t limit, const QString& message)
{
    if (count > limit - used) throw DisplayBudgetError{message};
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

QVector3D unitDisplayNormal(const std::array<double, 3>& value)
{
    // Normalize in double precision: area vectors of small but valid faces
    // must not trip QVector3D's fuzzy-null threshold or overflow a float sum.
    const double length = std::hypot(value[0], value[1], value[2]);
    if (!std::isfinite(length) || length <= 0.0) return {};
    return QVector3D(float(value[0] / length), float(value[1] / length), float(value[2] / length));
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
    display.normals.resize(display.vertices.size());

    // Adapted from wzqlalala/Motion b636535a45895b193aa581afd4df10a9004c9b6e,
    // src/ui/cad/CadMeshRenderer.cpp, Upload(): analytic cylinder normals and
    // face-local, area-weighted normals for the other surfaces. This metadata
    // changes lighting only; vertex/triangle order and OCCT topology stay intact.
    std::vector<std::array<double, 3>> normalSums(std::size_t(mesh->NbNodes()));
    std::optional<gp_Cylinder> cylinder;
    try {
        const BRepAdaptor_Surface surface(face);
        if (surface.GetType() == GeomAbs_Cylinder) cylinder = surface.Cylinder();
    } catch (const Standard_Failure&) {
        // Analytic classification is optional for display. Keep the valid
        // tessellation and use its oriented face-local normals on failure.
    }

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
        if (hasArea(display.vertices[ia], display.vertices[ib], display.vertices[ic])) {
            display.triangles.push_back({{ia, ib, ic}});
            const gp_Pnt pa = mesh->Node(a).Transformed(transform);
            const gp_Pnt pb = mesh->Node(b).Transformed(transform);
            const gp_Pnt pc = mesh->Node(c).Transformed(transform);
            const double ux = pb.X() - pa.X(), uy = pb.Y() - pa.Y(), uz = pb.Z() - pa.Z();
            const double vx = pc.X() - pa.X(), vy = pc.Y() - pa.Y(), vz = pc.Z() - pa.Z();
            const std::array<double, 3> area{{uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx}};
            const QVector3D fallback = unitDisplayNormal(area);
            for (int node : {a, b, c}) {
                auto& sum = normalSums[std::size_t(node - 1)];
                for (int axis = 0; axis < 3; ++axis) sum[axis] += area[axis];
                auto& normal = display.normals[offset + std::uint32_t(node - 1)];
                if (normal.isNull()) normal = fallback;
            }
        }
    }
    if (display.triangles.size() == before)
        fail(QStringLiteral("曲面的三角形在显示精度下全部退化。"));

    for (int i = 1; i <= mesh->NbNodes(); ++i) {
        const auto& sum = normalSums[std::size_t(i - 1)];
        QVector3D normal = unitDisplayNormal(sum);
        if (cylinder) {
            // BRepAdaptor_Surface includes the face location. Compute radial
            // directions from the same placed double coordinates as triangles,
            // then orient them with the already-corrected triangle winding.
            const gp_Pnt point = mesh->Node(i).Transformed(transform);
            const gp_Pnt origin = cylinder->Location();
            const gp_Dir axis = cylinder->Axis().Direction();
            const std::array<double, 3> direction{{axis.X(), axis.Y(), axis.Z()}};
            std::array<double, 3> radial{{point.X() - origin.X(), point.Y() - origin.Y(), point.Z() - origin.Z()}};
            double axial = 0.0;
            for (int k = 0; k < 3; ++k) axial += radial[k] * direction[k];
            for (int k = 0; k < 3; ++k) radial[k] -= axial * direction[k];
            const double orientation = radial[0] * sum[0] + radial[1] * sum[1] + radial[2] * sum[2];
            if (orientation < 0.0) for (double& value : radial) value = -value;
            const QVector3D analytic = unitDisplayNormal(radial);
            if (!analytic.isNull() && orientation != 0.0) normal = analytic;
        }
        if (!normal.isNull()) display.normals[offset + std::uint32_t(i - 1)] = normal;
    }
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

DisplayMesh makeDisplay(const TopoDS_Shape& shape, DisplayBudget& budget, double displayScale = 1.0)
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
    const double deflection = (std::max)(Precision::Confusion(), diagonal * 0.001 * displayScale);

    DisplayMesh display;
    if (!faces.IsEmpty()) {
        const double angle = (std::min)(0.8, 0.25 * std::sqrt(displayScale));
        BRepMesh_IncrementalMesh mesher(shape, deflection, Standard_False, angle, Standard_False);
        for (int i = 1; i <= faces.Extent(); ++i) {
            const auto face = TopoDS::Face(faces(i));
            QString meshError;
            if (!ensureOccFaceTriangulation(face, deflection, meshError))
                fail(QStringLiteral("第 %1 个曲面离散失败：%2").arg(i).arg(meshError));
            appendFace(face, display, budget);
        }
    }
    for (int i = 1; i <= edges.Extent(); ++i)
        appendEdge(TopoDS::Edge(edges(i)), deflection, display, budget);
    display.normals.resize(display.vertices.size()); // Standalone edge vertices are unlit.
    if (display.triangles.empty() && display.edges.empty())
        fail(QStringLiteral("几何没有可显示的曲面或非退化线框。"));
    return display;
}

void makeAssemblyDisplays(std::vector<CadPart>& parts, QStringList& notes)
{
    // Keep the complete exact assembly while bounding the viewport's mesh.
    // Only a display budget overflow retries at a coarser tessellation; invalid
    // geometry, missing faces and excessive topology remain explicit errors.
    for (int attempt = 0; attempt <= 6; ++attempt) {
        const double scale = double(std::uint64_t(1) << attempt);
        DisplayBudget budget;
        for (auto& part : parts) part.display = {};
        if (attempt) for (auto& part : parts) BRepTools::Clean(part.shape);
        try {
            for (auto& part : parts) {
                try { part.display = makeDisplay(part.shape, budget, scale); }
                catch (const DisplayBudgetError& failure) {
                    throw DisplayBudgetError{QStringLiteral("零件“%1”：%2").arg(part.name, failure.message)};
                } catch (const CadError& failure) {
                    fail(QStringLiteral("零件“%1”：%2").arg(part.name, failure.message));
                }
            }
            if (attempt) notes.append(QStringLiteral("大型装配采用 %1 倍显示离散公差，共 %2 个显示三角形；全部 %3 个零件及精确 CAD 曲面、边界和质量属性保留。")
                .arg(scale, 0, 'g', 3).arg(qulonglong(budget.triangles)).arg(qulonglong(parts.size())));
            return;
        } catch (const DisplayBudgetError& failure) {
            if (attempt == 6) fail(failure.message + QStringLiteral(" 已尝试调整显示精度，仍超过完整装配的显示预算；请分批导入。"));
        }
    }
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

void CadDocument::clear() { m_parts.clear(); m_importNotes.clear(); }

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

        std::vector<NamedShape> named;
        {
            const std::lock_guard<std::mutex> lock(exchangeMutex);
            if (format == Format::Step) named = readStepParts(file, kMaxParts - m_parts.size());
            else {
                const auto root = readShape(file, format);
                std::vector<TopoDS_Shape> shapes;
                std::size_t visited = 0;
                collectParts(root, kMaxParts - m_parts.size(), visited, shapes);
                for (const auto& shape : shapes) named.push_back({shape, {}, {}});
            }
        }
        std::vector<TopoDS_Shape> shapes;
        for (const auto& item : named) shapes.push_back(item.shape);
        checkedId(m_nextId + shapes.size() - 1);

        QString name = file.completeBaseName();
        if (name.trimmed().isEmpty()) name = QStringLiteral("导入零件");
        std::vector<CadPart> staged;
        QStringList notes;
        std::vector<int> ids;
        staged.reserve(shapes.size());
        ids.reserve(shapes.size());
        std::set<QString> usedNames;
        for (const auto& existing : m_parts) usedNames.insert(existing.name);
        for (std::size_t i = 0; i < shapes.size(); ++i) {
            const int id = checkedId(m_nextId + i);
            const QString baseName = named[i].sourceName.isEmpty() ? name : named[i].sourceName;
            QString partName = baseName;
            for (int n = 2; usedNames.count(partName); ++n) partName = QStringLiteral("%1 (%2)").arg(baseName).arg(n);
            usedNames.insert(partName);
            try {
                if (format == Format::Step) {
                    bool repaired = false;
                    shapes[i] = repairImportedShape(shapes[i], repaired);
                    if (repaired) notes.append(QStringLiteral("已修复“%1”的 STEP 拓扑；几何边界、质量积分和惯量验证保持一致。").arg(partName));
                }
                staged.push_back({id, partName, shapes[i], {}, named[i].sourceName, named[i].instancePath});
            } catch (const CadError& failure) {
                fail(QStringLiteral("零件“%1”：%2").arg(partName, failure.message));
            }
            ids.push_back(id);
        }
        makeAssemblyDisplays(staged, notes);

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
        m_importNotes = std::move(notes);
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

        std::vector<CadPart> staged;
        QStringList notes;
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
                staged.push_back({part->id, part->name, shape, {}, part->sourceName, part->instancePath});
            } catch (const CadError& failure) {
                fail(QStringLiteral("零件 %1：%2").arg(part->id).arg(failure.message));
            }
        }
        makeAssemblyDisplays(staged, notes);
        m_parts.swap(staged);
        m_importNotes = std::move(notes);
        m_nextId = nextId;
    });
}

} // namespace MotionOcc
