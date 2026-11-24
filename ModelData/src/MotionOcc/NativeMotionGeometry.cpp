#include "NativeMotionGeometry.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "OccTessellation.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRep_Builder.hxx>
#include <BRepTools.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>

#include <QDir>
#include <QCoreApplication>
#include <QJsonArray>
#include <QStringList>

#include <Eigen/Eigenvalues>

#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <set>
#include <stdexcept>
#include <sstream>

namespace MotionOcc {
namespace {

[[noreturn]] void fail(const QString& message) { throw std::runtime_error(message.toStdString()); }

Vector3d vector(const QJsonValue& value, Vector3d fallback = {})
{
    if (value.isUndefined() || value.isNull()) return fallback;
    Vector3d result{};
    if (value.isArray()) {
        const auto a = value.toArray();
        if (a.size() != 3) fail(QStringLiteral("三维向量必须包含三个分量。"));
        for (int d = 0; d != 3; ++d) result[d] = a[d].toDouble(NAN);
    } else {
        const auto o = value.toObject();
        result = {o.value("x").toDouble(NAN), o.value("y").toDouble(NAN), o.value("z").toDouble(NAN)};
    }
    for (double x : result) if (!std::isfinite(x)) fail(QStringLiteral("三维向量含无效数值。"));
    return result;
}

void append(std::vector<double>& values, const gp_Pnt& p, const Vector3d& origin)
{
    values.insert(values.end(), {p.X() - origin[0], p.Y() - origin[1], p.Z() - origin[2]});
}

NativeTriangleMesh meshFaces(const std::vector<TopoDS_Face>& faces, const Vector3d& origin)
{
    NativeTriangleMesh out;
    // A coordinate key also welds the duplicated boundary vertices of adjacent
    // OCC faces, allowing the engine to distinguish seams from surface borders.
    std::map<std::array<long long, 3>, int> welded;
    const auto vertex = [&](const gp_Pnt& p) {
        std::array<long long, 3> key{};
        for (int d = 0; d != 3; ++d) key[d] = std::llround((p.Coord(d + 1) - origin[d]) * 1e7);
        const auto inserted = welded.emplace(key, static_cast<int>(welded.size()));
        if (inserted.second) append(out.nodes, p, origin);
        return inserted.first->second;
    };
    for (const auto& original : faces) {
        const auto copy = TopoDS::Face(BRepBuilderAPI_Copy(original).Shape());
        QString meshError;
        if(!ensureOccFaceTriangulation(copy,0.02,meshError))fail(meshError);
        TopLoc_Location location;
        const auto triangles = BRep_Tool::Triangulation(copy, location);
        if (triangles.IsNull()) fail(QStringLiteral("OCC 曲面离散失败。"));
        for (int i = 1; i <= triangles->NbTriangles(); ++i) {
            int a, b, c;
            triangles->Triangle(i).Get(a, b, c);
            if (copy.Orientation() == TopAbs_REVERSED) std::swap(b, c);
            const auto pa = triangles->Node(a).Transformed(location.Transformation());
            const auto pb = triangles->Node(b).Transformed(location.Transformation());
            const auto pc = triangles->Node(c).Transformed(location.Transformation());
            if (gp_Vec(pa, pb).Crossed(gp_Vec(pa, pc)).SquareMagnitude() < 1e-24) continue;
            const int ia = vertex(pa), ib = vertex(pb), ic = vertex(pc);
            if (ia != ib && ib != ic && ic != ia) out.triangles.insert(out.triangles.end(), {ia, ib, ic});
        }
    }
    if (out.triangles.empty()) fail(QStringLiteral("选定几何没有可用于求解的三角网格。"));
    return out;
}

std::vector<TopoDS_Face> bodyFaces(const CadDocument& cad, const QVector<int>& parts)
{
    std::vector<TopoDS_Face> out;
    for (int id : parts) {
        const auto* part = cad.part(id);
        if (!part) fail(QStringLiteral("绑定的 CAD 部件 %1 不存在。").arg(id));
        if (!part->metreDisplaySource.IsNull())
            fail(QStringLiteral("零件“%1”的毫米拓扑未通过校验，无法用作精确接触几何。请先修复源 STEP。").arg(part->name));
        for (TopExp_Explorer it(part->shape, TopAbs_FACE); it.More(); it.Next())
            out.push_back(TopoDS::Face(it.Current()));
    }
    return out;
}

std::vector<TopoDS_Shape> selectedShapes(const CadDocument& cad, const NativeMotionDocument& doc,
                                       const QJsonObject& joint, const QString& field,
                                       int bodyId, TopAbs_ShapeEnum kind)
{
    const auto refs = joint.value(field).toArray();
    if (refs.isEmpty())
        fail(QStringLiteral("运动副 %1 的原几何拓扑 ID 无法从 STEP/IGES 自动恢复。请在 %2 中重新绑定 {partId,index}（OCC 索引从 1 开始）。")
                 .arg(joint.value("id").toInt()).arg(field));
    const auto bodyParts = doc.bodyPartBindings.value(bodyId);
    std::vector<TopoDS_Shape> out;
    for (const auto& value : refs) {
        const auto ref = value.toObject();
        const int partId = ref.value("partId").toInt(-1);
        const int index = ref.value("index").toInt(-1);
        const auto* part = cad.part(partId);
        // Original GeoEdge references are global and may point to a separate
        // construction wire. Its world samples become local to this body below.
        // Faces retain the original selected base-body surface semantics.
        if (!part || (kind == TopAbs_FACE && !bodyParts.contains(partId)))
            fail(QStringLiteral("%1 引用了不存在或不属于运动体 %2 的 CAD 部件。").arg(field).arg(bodyId));
        if (!part->metreDisplaySource.IsNull())
            fail(QStringLiteral("零件“%1”的毫米拓扑未通过校验，无法用于曲线或曲面约束求解。请先修复源 STEP。").arg(part->name));
        TopTools_IndexedMapOfShape shapes;
        TopExp::MapShapes(part->shape, kind, shapes);
        if (index < 1 || index > shapes.Extent()) fail(QStringLiteral("%1 的 OCC 拓扑索引 %2 超出范围。").arg(field).arg(index));
        out.push_back(shapes(index));
    }
    return out;
}

NativeCurveGeometry sampleCurve(const std::vector<TopoDS_Shape>& shapes, const Vector3d& origin)
{
    using Sample = std::array<double, 10>;
    std::vector<std::vector<Sample>> chains;
    for (const auto& shape : shapes) {
        BRepAdaptor_Curve curve(TopoDS::Edge(shape));
        const double length = GCPnts_AbscissaPoint::Length(curve);
        if (!std::isfinite(length) || length <= 1e-9) fail(QStringLiteral("约束曲线含退化边。"));
        const int count = std::clamp(static_cast<int>(std::ceil(length / 0.02)), 32, 4096);
        GCPnts_UniformAbscissa sampler(curve, count);
        if (!sampler.IsDone()) fail(QStringLiteral("OCC 曲线弧长采样失败。"));
        std::vector<Sample> samples;
        for (int i = 1; i <= sampler.NbPoints(); ++i) {
            gp_Pnt p; gp_Vec d1, d2;
            curve.D2(sampler.Parameter(i), p, d1, d2);
            const double speed = d1.Magnitude();
            if (speed < 1e-12) fail(QStringLiteral("约束曲线包含奇异切向，无法建立连续曲线副。"));
            const auto tangent = d1 / speed;
            const auto curvature = (d2 - tangent * d2.Dot(tangent)) / (speed * speed);
            samples.push_back({p.X() - origin[0], p.Y() - origin[1], p.Z() - origin[2],
                               tangent.X(), tangent.Y(), tangent.Z(),
                               curvature.X(), curvature.Y(), curvature.Z(), 1.0});
        }
        if (shape.Orientation() == TopAbs_REVERSED) {
            std::reverse(samples.begin(), samples.end());
            for (auto& s : samples) for (int d = 3; d < 6; ++d) s[d] = -s[d];
        }
        chains.push_back(std::move(samples));
    }
    if (chains.empty()) fail(QStringLiteral("约束曲线为空。"));
    const auto distance = [](const Sample& a, const Sample& b) { return std::hypot(a[0]-b[0], a[1]-b[1], a[2]-b[2]); };
    std::vector<Sample> merged = std::move(chains.front());
    chains.erase(chains.begin());
    while (!chains.empty()) {
        bool found = false;
        for (auto it = chains.begin(); it != chains.end(); ++it) {
            bool atFront = false, reverse = false;
            if (distance(merged.back(), it->front()) < 1e-4) {}
            else if (distance(merged.back(), it->back()) < 1e-4) reverse = true;
            else if (distance(merged.front(), it->back()) < 1e-4) atFront = true;
            else if (distance(merged.front(), it->front()) < 1e-4) { atFront = true; reverse = true; }
            else continue;
            if (reverse) { std::reverse(it->begin(), it->end()); for (auto& s : *it) for (int d=3;d<6;++d) s[d]=-s[d]; }
            if (atFront) merged.insert(merged.begin(), it->begin(), it->end()-1);
            else merged.insert(merged.end(), it->begin()+1, it->end());
            chains.erase(it); found = true; break;
        }
        if (!found) fail(QStringLiteral("约束边不能连接成一条连续曲线。"));
    }
    NativeCurveGeometry out;
    out.closed = distance(merged.front(), merged.back()) < 1e-4;
    for (const auto& sample : merged) {
        out.nodes.insert(out.nodes.end(), sample.begin(), sample.begin()+3);
        out.samples.insert(out.samples.end(), sample.begin(), sample.end());
    }
    return out;
}

TopoDS_Shape shapeSet(const std::vector<TopoDS_Shape>& shapes)
{
    BRep_Builder builder;TopoDS_Compound compound;builder.MakeCompound(compound);
    for(const auto& shape:shapes)builder.Add(compound,shape);
    return compound;
}

double exactDistance(const TopoDS_Shape& a,const TopoDS_Shape& b)
{
    BRepExtrema_DistShapeShape distance(a,b);
    if(!distance.IsDone()||distance.NbSolution()<1||!std::isfinite(distance.Value()))
        fail(QStringLiteral("无法验证几何运动副的初始贴合距离。"));
    return distance.Value();
}

void checkInitialDistance(int id,const QString& type,double distance,double tolerance)
{
    if(distance>tolerance)
        fail(QStringLiteral("%1副 %2 的初始贴合距离为 %3 mm，超过容差 %4 mm。请先使应用点/两条曲线贴合；不能用初始装配把错误几何瞬间拉到约束位置。")
            .arg(type).arg(id).arg(distance,0,'g',12).arg(tolerance,0,'g',12));
}

BodyProperties prepareProperties(const CadDocument& cad, const NativeMotionDocument& doc, const QJsonObject& source,QString& diagnostic)
{
    BodyProperties props;
    const int id = source.value("id").toInt(-1);
    props.massKg = source.value("mass").toDouble();
    props.centerMm = vector(source.value("centerOfMass"));
    const auto inertia = source.value("inertia").toArray();
    bool needGeometry = inertia.size() != 6;
    for (int i = 0; i < inertia.size() && i < 6; ++i) props.inertiaKgM2[i] = inertia[i].toDouble() * 1e-6;
    if (!needGeometry) needGeometry = props.inertiaKgM2[0] <= 0 || props.inertiaKgM2[1] <= 0 || props.inertiaKgM2[2] <= 0;
    const bool fixed = source.value("isFixed").toBool();
    GProp_GProps aggregate;bool haveAggregate=false,completeAggregate=true;
    const auto aggregateCad=[&](){
        if(haveAggregate)return;haveAggregate=true;
        for (int partId : doc.bodyPartBindings.value(id)) {
            const auto* part = cad.part(partId);
            if (!part) fail(QStringLiteral("运动体 %1 缺少 CAD 几何绑定。").arg(id));
            if (!part->metreDisplaySource.IsNull()) {
                if (needGeometry && !fixed)
                    fail(QStringLiteral("零件“%1”的毫米拓扑未通过校验，请为运动体 %2 提供完整质量属性。").arg(part->name).arg(id));
                completeAggregate = false;
                continue;
            }
            bool haveSolid=false;
            for (TopExp_Explorer it(part->shape, TopAbs_SOLID); it.More(); it.Next()) {
                haveSolid=true;
                GProp_GProps solid;
                BRepGProp::VolumeProperties(it.Current(), solid);
                if (solid.Mass() <= 0 || !std::isfinite(solid.Mass())) fail(QStringLiteral("运动体 %1 的实体体积无效。").arg(id));
                aggregate.Add(solid);
            }
            if(!haveSolid)completeAggregate=false;
        }
    };
    if (needGeometry && !fixed) {
        aggregateCad();
        if (aggregate.Mass() <= 0) fail(QStringLiteral("运动体 %1 没有有效惯量；请提供封闭 CAD 实体或完整质量属性。").arg(id));
        if (props.massKg <= 0) props.massKg = aggregate.Mass() * 7.85e-6;
        const auto center = aggregate.CentreOfMass();
        props.centerMm = {center.X(), center.Y(), center.Z()};
        const auto matrix = aggregate.MatrixOfInertia();
        const double scale = props.massKg / aggregate.Mass() * 1e-6;
        props.inertiaKgM2 = {matrix(1,1)*scale,matrix(2,2)*scale,matrix(3,3)*scale,
                             matrix(1,2)*scale,matrix(1,3)*scale,matrix(2,3)*scale};
    }else if(!needGeometry){
        const QString convention=source.value("inertiaConvention").toString("auto");
        if(convention!="auto"&&convention!="tensor"&&convention!="products")fail(QStringLiteral("运动体 %1 的惯量积约定无效。").arg(id));
        bool products=convention=="products",provedByCad=false,declaredByDocument=false;
        const bool hasProducts=props.inertiaKgM2[3]!=0||props.inertiaKgM2[4]!=0||props.inertiaKgM2[5]!=0;
        if(convention=="auto"){
            const QString declaration=doc.root.value("generatorNotes").toObject().value("coordinateAndInertiaConvention").toObject().value("inertia").toString();
            QString formula;for(QChar c:declaration)if(!c.isSpace())formula+=c;
            declaredByDocument=formula.contains(QStringLiteral("[Ixx,Iyy,Izz,-CAD_tensor_xy,-CAD_tensor_xz,-CAD_tensor_yz]"));
            if(declaredByDocument)products=true;
        }
        if(convention=="auto"&&!declaredByDocument&&hasProducts&&props.massKg>0&&std::isfinite(props.massKg)){
            aggregateCad();
            if(completeAggregate&&aggregate.Mass()>0){
                const auto center=aggregate.CentreOfMass();const auto matrix=aggregate.MatrixOfInertia();
                const double densityScale=props.massKg/aggregate.Mass()*1e-6;
                const std::array<double,6> expected{{matrix(1,1)*densityScale,matrix(2,2)*densityScale,matrix(3,3)*densityScale,
                    matrix(1,2)*densityScale,matrix(1,3)*densityScale,matrix(2,3)*densityScale}};
                double inertiaScale=0;for(double value:expected)inertiaScale=std::max(inertiaScale,std::abs(value));
                const double inertiaTolerance=std::max(1e-18,inertiaScale*1e-6);
                const double lengthScale=std::sqrt(std::max({expected[0],expected[1],expected[2]})*1e6/props.massKg);
                const double locationScale=std::max({1.0,lengthScale,std::abs(center.X()),std::abs(center.Y()),std::abs(center.Z())});
                const double centerError=std::hypot(props.centerMm[0]-center.X(),props.centerMm[1]-center.Y(),props.centerMm[2]-center.Z());
                bool tensorMatch=true,productsMatch=centerError<=2e-7*locationScale;
                for(int i=0;i<6;++i){
                    tensorMatch=tensorMatch&&std::abs(props.inertiaKgM2[i]-expected[i])<=inertiaTolerance;
                    productsMatch=productsMatch&&std::abs((i<3?props.inertiaKgM2[i]:-props.inertiaKgM2[i])-expected[i])<=inertiaTolerance;
                }
                // A failed physicality check alone never authorizes a sign
                // change. All bound solids, the source COM, and all six tensor
                // components at the source mass must identify the old +∫xy
                // convention; an ambiguous near-zero product is left alone.
                products=productsMatch&&!tensorMatch;provedByCad=products;
            }
        }
        if(products){
            for(int i=3;i<6;++i)props.inertiaKgM2[i]=-props.inertiaKgM2[i];
            if(hasProducts){
                if(declaredByDocument)diagnostic=QStringLiteral("运动体 %1 按原文件 generatorNotes.coordinateAndInertiaConvention.inertia 中的明确六分量公式，将 +∫xy 惯量积转换为矩阵交叉项；原数值和声明均保留。").arg(id);
                else if(provedByCad)diagnostic=QStringLiteral("运动体 %1 的全部绑定 CAD、质心及六分量质量惯量指纹确认了旧 +∫xy 惯量积约定，求解时已转换为惯量矩阵交叉项；原 JSON 数值未改写。").arg(id);
                else diagnostic=QStringLiteral("运动体 %1 按显式 inertiaConvention=products 将惯量积转换为矩阵交叉项；原 JSON 数值未改写。").arg(id);
            }
        }
    }
    if (!fixed) {
        if (!std::isfinite(props.massKg) || props.massKg <= 0) fail(QStringLiteral("运动体 %1 的质量必须为正数。").arg(id));
        const auto& a = props.inertiaKgM2;
        Eigen::Matrix3d tensor;
        tensor << a[0],a[3],a[4], a[3],a[1],a[5], a[4],a[5],a[2];
        Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> eig(tensor);
        const auto& moments = eig.eigenvalues();
        if (!tensor.allFinite() || eig.info()!=Eigen::Success || moments[0] <= 0 || moments[2] > moments[0]+moments[1]+1e-8*std::abs(moments[2]))
            fail(QStringLiteral("运动体 %1 的惯量张量不满足正定性或主惯量三角不等式（原单位 kg·mm²）。请核对自定义质量属性及 inertiaConvention=tensor/products；没有明确声明或 CAD 六分量证据时不会猜测交叉项符号。").arg(id));
    }
    return props;
}

NativeFlexGeometry prepareFlex(const CadDocument& cad, const NativeMotionDocument& doc, const QJsonObject& definition)
{
    NativeFlexGeometry out;
    out.id = definition.value("id").toInt();
    out.bodyId = definition.value("targetBodyId").toInt(-1);
    out.definition = definition;
    if (definition.value("attachmentMeshChanged").toBool())
        fail(QStringLiteral("自动柔性体 %1 的体网格或目标几何已改变，旧附着节点编号不能直接复用。请在当前网格上重新选择节点，或明确改为按运动副自动附着。").arg(out.id));
    QJsonObject material = definition.value("material").toObject();
    if (material.isEmpty()) material = doc.entity("materials", definition.value("materialId").toInt(-1));
    if (material.isEmpty())
        fail(QStringLiteral("自动柔性体 %1 引用的材料未随原 .motion.json 导出。请打开该自动柔性体，选择或填写材料并保存后求解（弹性模量 MPa、泊松比、密度 kg/mm³）。").arg(out.id));
    out.youngModulus = material.value("youngModulus").toDouble(NAN) * 1000.0;
    out.poissonRatio = material.value("poissonRatio").toDouble(NAN);
    out.density = material.value("density").toDouble(NAN);
    if (!std::isfinite(out.youngModulus) || !std::isfinite(out.density) || !std::isfinite(out.poissonRatio)
        || !(out.youngModulus>0) || !(out.density>0) || out.poissonRatio<=-1 || out.poissonRatio>=0.5)
        fail(QStringLiteral("自动柔性体 %1 的材料参数无效。").arg(out.id));
    const auto mesh = definition.value("mesh").toObject();
    if (!definition.value("meshFile").toString().isEmpty()) {
        FemMesh parsed; QString error;
        const QString path = QDir(doc.baseDirectory).absoluteFilePath(definition.value("meshFile").toString());
        if (!readNetgenMesh(path, MeshKind::Volume, parsed, error)) fail(error);
        for (const auto& node : parsed.nodes) out.nodes.insert(out.nodes.end(), node.begin(),node.end());
        for (const auto& tet : parsed.tetrahedra) for (auto i : tet) out.tetrahedra.push_back(static_cast<int>(i));
    } else {
        for (const auto& node : mesh.value("nodes").toArray()) {
            const auto p = vector(node); out.nodes.insert(out.nodes.end(), p.begin(),p.end());
        }
        for (const auto& tet : mesh.value("tetrahedra").toArray()) {
            const auto indices = tet.toArray();
            if (indices.size()!=4) fail(QStringLiteral("柔性体体网格必须使用四节点四面体。"));
            for (const auto& i : indices) out.tetrahedra.push_back(i.toInt(-1));
        }
    }
    if (out.nodes.empty() || out.tetrahedra.empty()) {
        if (!out.nodes.empty() || !out.tetrahedra.empty()) fail(QStringLiteral("柔性体内嵌网格不完整。"));
        BRep_Builder builder; TopoDS_Compound compound; builder.MakeCompound(compound);
        const auto partIds=doc.bodyPartBindings.value(out.bodyId);
        if(partIds.isEmpty())fail(QStringLiteral("自动柔性体 %1 没有可用于自动剖分的 CAD 实体。").arg(out.id));
        for (int partId : partIds) {
            const auto* part = cad.part(partId);
            if (!part) fail("柔性体 CAD 部件不存在。");
            if (!part->metreDisplaySource.IsNull())
                fail(QStringLiteral("零件“%1”的毫米拓扑未通过校验，无法直接生成柔性体网格。").arg(part->name));
            builder.Add(compound, part->shape);
        }
        std::ostringstream serialized;BRepTools::Write(compound,serialized);
        out.sourceBrep=QByteArray::fromStdString(serialized.str());
#ifdef Q_OS_WIN
        const auto workerName=QStringLiteral("MxMotionMeshWorker.exe");
#else
        const auto workerName=QStringLiteral("MxMotionMeshWorker");
#endif
        out.meshWorkerPath=qEnvironmentVariable("MXMOTION_MESH_WORKER",QDir(QCoreApplication::applicationDirPath()).filePath(workerName));
    }
    for (int i : out.tetrahedra) if (i<0 || static_cast<std::size_t>(i)>=out.nodes.size()/3) fail(QStringLiteral("柔性体网格节点索引无效。"));
    for (const auto& entry : definition.value("attachGroups").toArray()) {
        std::vector<int> group;
        for (const auto& index : entry.toArray()) {
            const int i=index.toInt(-1);
            if (i<0 || (!out.nodes.empty() && static_cast<std::size_t>(i)>=out.nodes.size()/3)) fail(QStringLiteral("柔性体附着节点索引无效。"));
            group.push_back(i);
        }
        if (!group.empty()) out.attachGroups.push_back(std::move(group));
    }
    const auto originalAttachments=definition.value("attachPoints").toArray();
    const auto attachmentPositions=definition.value("attachPointPositions").toArray();
    if (out.attachGroups.empty()) for(const auto& value:attachmentPositions){const auto p=vector(value);out.attachPositions.insert(out.attachPositions.end(),p.begin(),p.end());}
    if(!originalAttachments.isEmpty()&&out.attachGroups.empty()&&attachmentPositions.isEmpty()){
        const auto ids=mesh.value("nodeIds").toArray();
        if(out.nodes.empty()||ids.size()!=static_cast<int>(out.nodes.size()/3))
            fail(QStringLiteral("自动柔性体 %1 的 attachPoints 是原网格全局 ID，原文件没有对应节点坐标。请编辑该自动柔性体，在当前体网格上重新选择附着节点，或提供原 mesh.nodeIds/节点坐标；不能忽略这些附着约束。").arg(out.id));
        std::set<int> selected;
        for(const auto& id:originalAttachments){
            int found=-1;for(int i=0;i<ids.size();++i)if(ids[i].toInt(-1)==id.toInt(-2)){found=i;break;}
            if(found<0)fail(QStringLiteral("自动柔性体 %1 的附着节点 %2 在提供的网格中不存在。").arg(out.id).arg(id.toInt()));
            selected.insert(found);
        }
        const double radius=definition.value("connectionRadius").toDouble();
        if(radius>0){const auto centers=selected;for(int center:centers)for(int i=0;i<ids.size();++i){
            const double distance=std::hypot(out.nodes[3*i]-out.nodes[3*center],out.nodes[3*i+1]-out.nodes[3*center+1],out.nodes[3*i+2]-out.nodes[3*center+2]);
            if(distance<=radius)selected.insert(i);
        }}
        // The original bridge groups selected nodes by mesh connectivity. Two
        // separate interface patches must not become one large rigid weld.
        std::map<int, std::set<int>> neighbours;
        for (std::size_t t=0; t+3<out.tetrahedra.size(); t+=4)
            for (int a=0; a<4; ++a) for (int b=a+1; b<4; ++b) {
                const int lhs=out.tetrahedra[t+a], rhs=out.tetrahedra[t+b];
                if (selected.count(lhs) && selected.count(rhs)) {
                    neighbours[lhs].insert(rhs); neighbours[rhs].insert(lhs);
                }
            }
        while (!selected.empty()) {
            std::vector<int> group{*selected.begin()}; selected.erase(group.front());
            for (std::size_t i=0; i<group.size(); ++i)
                for (int adjacent : neighbours[group[i]])
                    if (selected.erase(adjacent)) group.push_back(adjacent);
            std::sort(group.begin(), group.end());
            out.attachGroups.push_back(std::move(group));
        }
    }
    // Explicit user patches/positions take precedence, exactly as in the
    // original mMotionFlexBridge. Adding joint patches as well overconstrains
    // a manually attached body and changes its flexible modes.
    if (out.attachGroups.empty() && out.attachPositions.empty()) for (const auto& value : doc.collection("joints")) {
        const auto joint=value.toObject();
        if (joint.value("actionBodyId").toInt(-1)==out.bodyId || joint.value("baseBodyId").toInt(-1)==out.bodyId) {
            const auto p=vector(joint.value("origin"));
            out.attachPositions.insert(out.attachPositions.end(),p.begin(),p.end());
        }
    }
    return out;
}

// Expand independent interfaces in the solver snapshot only. Original body,
// joint and load IDs in the saved design remain untouched.
void prepareIndependentInterfaces(NativeMotionInput& input,NativeMotionGeometry& geometry)
{
    auto data=input.document.root.value("motionData").toObject();
    int nextBodyId=0;
    for(const auto& body:input.bodies){
        if(body.body.id>=std::numeric_limits<int>::max()-10000)
            fail("运动体 ID 过大，无法分配独立柔性接口 ID。");
        nextBodyId=std::max(nextBodyId,body.body.id+1);
    }
    std::set<int> targets;
    const auto pointObject=[](const Vector3d& p){return QJsonObject{{"x",p[0]},{"y",p[1]},{"z",p[2]}};};
    const auto distance=[](const Vector3d& a,const Vector3d& b){return std::hypot(a[0]-b[0],a[1]-b[1],a[2]-b[2]);};
    for(auto& flex:geometry.flexBodies){
        if(!targets.insert(flex.bodyId).second)fail("一个运动体不能被重复转换为多个自动柔性体。");
        const auto mode=flex.definition.value("interfaceMode").toString("SharedBody");
        if(mode!="SharedBody"&&mode!="Independent")fail("自动柔性体接口模式无效。");
        if(mode=="SharedBody")continue;
        flex.independentInterfaces=true;
        const auto originalIt=std::find_if(input.bodies.begin(),input.bodies.end(),[&](const PreparedBody& b){return b.body.id==flex.bodyId;});
        if(originalIt==input.bodies.end())fail("独立接口的原运动体不存在。");
        const PreparedBody original=*originalIt;
        const auto originalDefinition=input.document.entity("bodies",flex.bodyId);
        auto joints=data.value("joints").toArray();
        std::map<int,QJsonObject> bodyJoints;
        for(const auto& value:joints){
            const auto joint=value.toObject();
            const bool action=joint.value("actionBodyId").toInt(-1)==flex.bodyId;
            const bool base=joint.value("baseBodyId").toInt(-1)==flex.bodyId;
            if(!action&&!base)continue;
            const auto type=joint.value("jointType").toString();
            if((base&&(type=="PointOnCurve"||type=="PointOnSurface"))||type=="CurveOnCurve")
                fail(QStringLiteral("柔性体 %1 的 CAD 曲线/曲面几何副尚未采用变形网格，不可作为独立接口的约束面。请将几何导轨设为独立刚体。").arg(flex.id));
            bodyJoints.emplace(joint.value("id").toInt(),joint);
        }
        for(const auto& value:data.value("contacts").toArray()){
            const auto contact=value.toObject();
            if(contact.value("enabled").toBool(true)&&(contact.value("actionBodyId").toInt(-1)==flex.bodyId||contact.value("baseBodyId").toInt(-1)==flex.bodyId))
                fail(QStringLiteral("柔性体 %1 的独立接口全有限元目前不支持将原刚体 CAD 接触壳当作变形接触面。请移除该接触或保留刚体接触部件。").arg(flex.id));
        }
        std::vector<Vector3d> centers;
        std::map<int,int> jointPort;
        const auto explicitGroups=flex.definition.value("interfaceJointGroups").toArray();
        if(!explicitGroups.isEmpty()){
            for(const auto& value:explicitGroups){
                if(!value.isArray()||value.toArray().isEmpty())fail("interfaceJointGroups 每组需要至少一个有效运动副 ID。");
                std::vector<int> group;Vector3d center{};
                for(const auto& item:value.toArray()){
                    const double numeric=item.toDouble(-1);const int id=item.toInt(-1);
                    if(numeric!=id||!bodyJoints.count(id)||jointPort.count(id))fail("独立接口副分组包含无效、重复或不属于目标运动体的 ID。");
                    jointPort[id]=static_cast<int>(centers.size());group.push_back(id);
                    const auto p=vector(bodyJoints.at(id).value("origin"));for(int d=0;d<3;++d)center[d]+=p[d];
                }
                for(double& v:center)v/=group.size();
                centers.push_back(center);flex.interfaceJointGroups.push_back(std::move(group));
            }
            if(jointPort.size()!=bodyJoints.size())fail("interfaceJointGroups 必须覆盖目标运动体的全部运动副端点。");
            if(!flex.attachGroups.empty()&&flex.attachGroups.size()!=centers.size())fail("显式附着节点组数量必须与独立接口副分组数量一致。");
            if(flex.attachGroups.empty()){
                flex.attachPositions.clear();for(const auto& p:centers)flex.attachPositions.insert(flex.attachPositions.end(),p.begin(),p.end());
            }
        }else if(!flex.attachGroups.empty()){
            if(flex.nodes.empty())fail("独立接口使用显式节点组时必须提供当前网格，以便核对接口中心。");
            for(const auto& group:flex.attachGroups){
                Vector3d center{};
                for(int node:group)for(int d=0;d<3;++d)center[d]+=flex.nodes[3*node+d];
                for(double& v:center)v/=group.size();centers.push_back(center);
            }
        }else{
            // Repeated joints at a common pin share one port; distinct pins
            // retain relative deformation instead of being welded to one body.
            for(std::size_t i=0;i+2<flex.attachPositions.size();i+=3){
                const Vector3d p{flex.attachPositions[i],flex.attachPositions[i+1],flex.attachPositions[i+2]};
                if(std::none_of(centers.begin(),centers.end(),[&](const Vector3d& c){return distance(c,p)<1e-8;}))centers.push_back(p);
            }
            flex.attachPositions.clear();for(const auto& p:centers)flex.attachPositions.insert(flex.attachPositions.end(),p.begin(),p.end());
        }
        if(centers.size()<2)fail(QStringLiteral("自动柔性体 %1 的独立接口模式至少需要两个不同接口；请设置两个运动副作用点或两个附着节点组。").arg(flex.id));
        const auto nearest=[&](const Vector3d& p,const QJsonObject& object,const QString& field){
            const QString overrideKey=field+"InterfaceIndex";
            const auto override=object.contains(overrideKey)?object.value(overrideKey):object.value("flexInterfaceIndex");
            if(!override.isUndefined()){
                const int index=override.toInt(-1);
                if(!override.isDouble()||override.toDouble()!=index||index<0||index>=static_cast<int>(centers.size()))
                    fail("独立柔性接口索引无效（从 0 开始）。");
                return index;
            }
            int best=-1;double first=std::numeric_limits<double>::infinity(),second=first;
            for(std::size_t i=0;i<centers.size();++i){const double d=distance(centers[i],p);if(d<first){second=first;first=d;best=static_cast<int>(i);}else second=std::min(second,d);}
            if(std::abs(first-second)<=1e-9*std::max(1.0,first))
                fail(QStringLiteral("柔性体 %1 的 %2 位于两个接口等距位置，不能猜测绑定；请设置 %3 或 flexInterfaceIndex。").arg(flex.id).arg(field,overrideKey));
            return best;
        };
        if(explicitGroups.isEmpty()){
            flex.interfaceJointGroups.resize(centers.size());
            for(const auto& item:bodyJoints){
                const int port=nearest(vector(item.second.value("origin")),item.second,"joint");
                jointPort[item.first]=port;flex.interfaceJointGroups[port].push_back(item.first);
            }
        }
        auto bodyDefinitions=data.value("bodies").toArray();
        const auto velocity=vector(originalDefinition.value("initialVelocity"));
        const auto angular=vector(originalDefinition.value("initialAngularVelocity"));
        const Eigen::Vector3d omega(angular[0]*3.14159265358979323846/180,angular[1]*3.14159265358979323846/180,angular[2]*3.14159265358979323846/180);
        for(std::size_t i=0;i<centers.size();++i){
            if(i==0){flex.interfaceBodyIds.push_back(flex.bodyId);continue;}
            PreparedBody port=original;port.body.id=nextBodyId++;port.body.fixed=false;port.body.partId=-1;
            port.body.name=QStringLiteral("%1 / 接口 %2").arg(original.body.name).arg(i+1);
            port.properties.centerMm=centers[i];
            // Atomic port mass is cleared by BuildFlexBody before any solve.
            // This placeholder is only for the parser's body construction.
            input.bodies.push_back(port);flex.interfaceBodyIds.push_back(port.body.id);
            auto source=originalDefinition;source["id"]=port.body.id;source["name"]=port.body.name;source["isFixed"]=false;
            source["geoEntityIds"]=QJsonArray{};source["geoEntityNames"]=QJsonArray{};source["centerOfMass"]=pointObject(centers[i]);
            const Eigen::Vector3d r(centers[i][0]-original.properties.centerMm[0],centers[i][1]-original.properties.centerMm[1],centers[i][2]-original.properties.centerMm[2]);
            const auto induced=omega.cross(r);
            source["initialVelocity"]=pointObject({velocity[0]+induced[0],velocity[1]+induced[1],velocity[2]+induced[2]});
            source["internalFlexPortOf"]=flex.id;source["internalFlexPortIndex"]=static_cast<int>(i);
            bodyDefinitions.append(source);
        }
        data["bodies"]=bodyDefinitions;
        for(int i=0;i<joints.size();++i){auto o=joints[i].toObject();const auto found=jointPort.find(o.value("id").toInt(-1));if(found==jointPort.end())continue;
            for(const auto* key:{"actionBodyId","baseBodyId"})if(o.value(key).toInt(-1)==flex.bodyId)o[key]=flex.interfaceBodyIds[found->second];
            joints[i]=o;
        }
        data["joints"]=joints;
        const std::map<QString,std::vector<std::pair<QString,QString>>> endpoints{
            {"markers",{{"bodyId","localPosition"}}}, {"vectorForces",{{"bodyId","actionPoint"},{"baseBodyId","actionPoint"}}},
            {"springs",{{"bodyIId","localI"},{"bodyJId","localJ"}}},
            {"scalarForces",{{"bodyIId","localI"},{"bodyJId","localJ"}}},{"scalarTorques",{{"bodyIId","localI"},{"bodyJId","localJ"}}},
            {"tensionCables",{{"bodyIId","localI"},{"bodyJId","localJ"}}},{"softStops",{{"bodyIId","localI"},{"bodyJId","localJ"}}},
            {"threePointForces",{{"bodyIId","localI"},{"originBodyId","localOrigin"},{"targetBodyId","localTarget"}}},
            {"threePointTorques",{{"bodyIId","localI"},{"originBodyId","localOrigin"},{"targetBodyId","localTarget"}}}
        };
        for(const auto& collection:endpoints){
            auto values=data.value(collection.first).toArray();
            for(int i=0;i<values.size();++i){auto o=values[i].toObject();if(!o.value("enabled").toBool(true))continue;
                for(const auto& field:collection.second)if(o.value(field.first).toInt(-1)==flex.bodyId){
                    const int port=nearest(vector(o.value(field.second)),o,field.first);o[field.first]=flex.interfaceBodyIds[port];
                }
                values[i]=o;
            }
            if(data.contains(collection.first))data[collection.first]=values;
        }
        // Point sensor coordinates are COM-local, unlike force/marker origins.
        // Preserve their initial world point while changing a port reference.
        auto sensors=data.value("sensors").toArray();
        for(int i=0;i<sensors.size();++i){auto sensor=sensors[i].toObject();
            const auto routePoint=[&](QJsonObject& object,const QString& bodyField,const QString& pointField){
                if(object.value(bodyField).toInt(-1)!=flex.bodyId)return;
                auto p=vector(object.value(pointField));for(int d=0;d<3;++d)p[d]+=original.properties.centerMm[d];
                const int port=nearest(p,object,bodyField);
                object[bodyField]=flex.interfaceBodyIds[port];
                const auto& center=port==0?original.properties.centerMm:centers[port];
                for(int d=0;d<3;++d)p[d]-=center[d];object[pointField]=pointObject(p);
            };
            const auto type=sensor.value("sensorType").toString();
            if(type=="Distance")for(const auto* key:{"pointA","pointB"}){auto point=sensor.value(key).toObject();routePoint(point,"bodyId","localPos");sensor[key]=point;}
            if(type=="BodyDisplacement"||type=="BodyVelocity"||type=="BodyAcceleration")routePoint(sensor,"targetBodyId","referencePoint");
            // An orientation reference has no material point. Its documented
            // default is the first interface; explicit indices select another.
            for(const auto* field:{"referenceBodyId","bodyAId","bodyBId","axisBodyId"})if(sensor.value(field).toInt(-1)==flex.bodyId){
                const QString indexKey=QString::fromLatin1(field)+"InterfaceIndex";
                const int port=sensor.value(indexKey).toInt(0);
                if(port<0||port>=static_cast<int>(centers.size()))fail("柔性体传感器参考接口索引越界。");
                sensor[field]=flex.interfaceBodyIds[port];
            }
            sensors[i]=sensor;
        }
        if(data.contains("sensors"))data["sensors"]=sensors;
        geometry.diagnostics.append(QStringLiteral("自动柔性体 %1 使用 %2 个独立接口和真实共回转 tetra4 全有限元；原刚体质量由网格质量替换。接口首项保留原体参考系；点载荷按施力位置绑定接口，模态基仅用于检查。").arg(flex.id).arg(centers.size()));
        input.document.root["motionData"]=data;
    }
}

} // namespace

bool prepareNativeMotion(const CadDocument& cad, const NativeMotionDocument& document,
                         int scenarioId, NativeMotionInput& input, QString& error)
{
    try {
        QStringList issues;
        if (!validateNativeMotionDocument(document, issues)) fail(issues.join('\n'));
        if (!validateNativeMotionBindings(cad, document, error)) fail(error);
        const auto sourceScenario = document.entity("scenarios", scenarioId);
        if (sourceScenario.isEmpty()) fail(QStringLiteral("仿真方案 %1 不存在。").arg(scenarioId));
        NativeMotionInput next;
        next.document = document;
        next.scenario.id = scenarioId;
        next.scenario.name = sourceScenario.value("name").toString();
        next.scenario.duration = sourceScenario.value("endTime").toDouble(1);
        next.scenario.step = sourceScenario.value("stepSize").toDouble(0.01);
        auto gravity = vector(sourceScenario.value("gravity"), {0,0,-9810});
        for (int i=0;i<3;++i) next.scenario.gravityMS2[i]=gravity[i]*0.001;
        if (!(next.scenario.duration>0) || !(next.scenario.step>0) ||
            std::ceil(next.scenario.duration/next.scenario.step)>1000000)
            fail(QStringLiteral("仿真时长、步长无效或超过 1000000 步。"));
        auto geometry=std::make_shared<NativeMotionGeometry>();
        std::map<int,Vector3d> centers;
        for (const auto& value : document.collection("bodies")) {
            const auto source=value.toObject();
            PreparedBody body;
            body.body.id=source.value("id").toInt(-1);
            body.body.name=source.value("name").toString();
            body.body.fixed=source.value("isFixed").toBool();
            body.body.partId=document.bodyPartBindings.value(body.body.id).value(0,-1);
            QString diagnostic;
            body.properties=prepareProperties(cad,document,source,diagnostic);
            if(!diagnostic.isEmpty())geometry->diagnostics.append(diagnostic);
            body.body.massKg=body.properties.massKg;
            centers[body.body.id]=body.properties.centerMm;
            next.bodies.push_back(body);
        }
        const auto ensureMesh=[&](int bodyId) -> const NativeTriangleMesh& {
            const auto existing=geometry->bodyMeshes.find(bodyId);
            if (existing!=geometry->bodyMeshes.end()) return existing->second;
            const auto center=centers.find(bodyId);
            if (center==centers.end()) fail(QStringLiteral("几何约束引用的运动体 %1 不存在。").arg(bodyId));
            return geometry->bodyMeshes.emplace(bodyId,meshFaces(bodyFaces(cad,document.bodyPartBindings.value(bodyId)),center->second)).first->second;
        };
        for (const auto& value : document.collection("contacts")) {
            const auto c=value.toObject(); if (!c.value("enabled").toBool(true)) continue;
            ensureMesh(c.value("actionBodyId").toInt(-1)); ensureMesh(c.value("baseBodyId").toInt(-1));
        }
        for (const auto& value : document.collection("joints")) {
            const auto joint=value.toObject(); const auto type=joint.value("jointType").toString();
            const int id=joint.value("id").toInt(), a=joint.value("actionBodyId").toInt(-1), b=joint.value("baseBodyId").toInt(-1);
            if (type=="PointOnSurface") {
                auto& prepared=geometry->joints[id];
                std::vector<TopoDS_Shape> selected;
                if (joint.value("surfaceFaceIds").toArray().isEmpty() && joint.value("occSurfaceFaces").toArray().isEmpty()){
                    prepared.surface=ensureMesh(b);
                    for(const auto& face:bodyFaces(cad,document.bodyPartBindings.value(b)))selected.push_back(face);
                }
                else {
                    std::vector<TopoDS_Face> faces;
                    selected=selectedShapes(cad,document,joint,"occSurfaceFaces",b,TopAbs_FACE);
                    for (const auto& shape : selected) faces.push_back(TopoDS::Face(shape));
                    prepared.surface=meshFaces(faces,centers.at(b));
                }
                double maxEdge=0;
                const auto& mesh=prepared.surface;
                for(std::size_t t=0;t<mesh.triangles.size();t+=3)for(int e=0;e<3;++e){
                    const int i=3*mesh.triangles[t+e],j=3*mesh.triangles[t+(e+1)%3];
                    maxEdge=std::max(maxEdge,std::hypot(mesh.nodes[i]-mesh.nodes[j],mesh.nodes[i+1]-mesh.nodes[j+1],mesh.nodes[i+2]-mesh.nodes[j+2]));
                }
                const auto p=vector(joint.value("origin"));
                checkInitialDistance(id,QStringLiteral("点在面上"),exactDistance(BRepBuilderAPI_MakeVertex(gp_Pnt(p[0],p[1],p[2])).Shape(),shapeSet(selected)),std::max(.5,.5*maxEdge));
            } else if (type=="PointOnCurve") {
                const auto selected=selectedShapes(cad,document,joint,"occCurveEdges",b,TopAbs_EDGE);
                geometry->joints[id].curveJ=sampleCurve(selected,centers.at(b));
                double length=0;for(const auto& shape:selected){BRepAdaptor_Curve curve(TopoDS::Edge(shape));length+=GCPnts_AbscissaPoint::Length(curve);}
                const auto p=vector(joint.value("origin"));
                checkInitialDistance(id,QStringLiteral("点在线上"),exactDistance(BRepBuilderAPI_MakeVertex(gp_Pnt(p[0],p[1],p[2])).Shape(),shapeSet(selected)),std::max(.1,.002*length));
            } else if (type=="CurveOnCurve") {
                auto& prepared=geometry->joints[id];
                const auto selectedI=selectedShapes(cad,document,joint,"occCurveIEdges",a,TopAbs_EDGE),selectedJ=selectedShapes(cad,document,joint,"occCurveJEdges",b,TopAbs_EDGE);
                prepared.curveI=sampleCurve(selectedI,centers.at(a));
                prepared.curveJ=sampleCurve(selectedJ,centers.at(b));
                checkInitialDistance(id,QStringLiteral("线在线上"),exactDistance(shapeSet(selectedI),shapeSet(selectedJ)),.2);
            }
        }
        for (const auto& value : document.collection("autoFlexBodies")) {
            const auto definition=value.toObject();
            if (definition.value("enabled").toBool(true)) geometry->flexBodies.push_back(prepareFlex(cad,document,definition));
        }
        prepareIndependentInterfaces(next,*geometry);
        next.geometry=std::move(geometry);
        input=std::move(next); error.clear(); return true;
    } catch (const Standard_Failure& e) { error=QStringLiteral("OCC 几何准备失败：%1").arg(e.GetMessageString()); }
      catch (const std::exception& e) { error=QString::fromUtf8(e.what()); }
      catch (...) { error=QStringLiteral("原生运动模型准备失败。"); }
    return false;
}

} // namespace MotionOcc
