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
        if (!part || !bodyParts.contains(partId)) fail(QStringLiteral("%1 引用了不属于运动体 %2 的 CAD 部件。").arg(field).arg(bodyId));
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
    QJsonObject material = definition.value("material").toObject();
    if (material.isEmpty()) material = doc.entity("materials", definition.value("materialId").toInt(-1));
    if (material.isEmpty())
        fail(QStringLiteral("自动柔性体 %1 引用的材料未随原 .motion.json 导出。请补充 material:{youngModulus,poissonRatio,density}（N/mm²、无量纲、kg/mm³）后求解。").arg(out.id));
    out.youngModulus = material.value("youngModulus").toDouble() * 1000.0;
    out.poissonRatio = material.value("poissonRatio").toDouble();
    out.density = material.value("density").toDouble();
    if (!(out.youngModulus>0) || !(out.density>0) || out.poissonRatio<=-1 || out.poissonRatio>=0.5)
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
        for(int partId:partIds){const auto* part=cad.part(partId);if(!part)fail("柔性体 CAD 部件不存在。");builder.Add(compound,part->shape);}
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
    for(const auto& value:attachmentPositions){const auto p=vector(value);out.attachPositions.insert(out.attachPositions.end(),p.begin(),p.end());}
    if(!originalAttachments.isEmpty()&&out.attachGroups.empty()&&attachmentPositions.isEmpty()){
        const auto ids=mesh.value("nodeIds").toArray();
        if(out.nodes.empty()||ids.size()!=static_cast<int>(out.nodes.size()/3))
            fail(QStringLiteral("自动柔性体 %1 的 attachPoints 是原网格全局 ID，原文件没有对应节点坐标。请提供 mesh.nodeIds，或重新指定 attachGroups/attachPointPositions；不能忽略这些附着约束。").arg(out.id));
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
        if(!selected.empty())out.attachGroups.emplace_back(selected.begin(),selected.end());
    }
    for (const auto& value : doc.collection("joints")) {
        const auto joint=value.toObject();
        if (joint.value("actionBodyId").toInt(-1)==out.bodyId || joint.value("baseBodyId").toInt(-1)==out.bodyId) {
            const auto p=vector(joint.value("origin"));
            out.attachPositions.insert(out.attachPositions.end(),p.begin(),p.end());
        }
    }
    return out;
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
        next.geometry=std::move(geometry);
        input=std::move(next); error.clear(); return true;
    } catch (const Standard_Failure& e) { error=QStringLiteral("OCC 几何准备失败：%1").arg(e.GetMessageString()); }
      catch (const std::exception& e) { error=QString::fromUtf8(e.what()); }
      catch (...) { error=QStringLiteral("原生运动模型准备失败。"); }
    return false;
}

} // namespace MotionOcc
