#include "NativeTopologyBinding.h"
#include "NativeMotionDocument.h"
#include "CadDocument.h"
#include "OccTessellation.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepGProp.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <Poly_Triangulation.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_SupportType.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Circ.hxx>

#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QSet>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace MotionOcc {
namespace {

constexpr double MatchToleranceMm = 1e-4;

struct Selector {
    QString sourceField;
    QString occField;
    int bodyId = -1;
    TopAbs_ShapeEnum kind = TopAbs_EDGE;
};

struct Candidate {
    int partId = -1;
    int index = -1;
    double distance = std::numeric_limits<double>::infinity();
    QString detail;
};

QString coordinates(const gp_Pnt& point)
{
    return QStringLiteral("(%1,%2,%3)").arg(point.X(), 0, 'g', 10).arg(point.Y(), 0, 'g', 10).arg(point.Z(), 0, 'g', 10);
}

QString edgeDescription(const CadPart& part, const TopoDS_Shape& shape, int index)
{
    const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
    const QStringList types{"Line", "Circle", "Ellipse", "Hyperbola", "Parabola", "Bezier", "BSpline", "Offset", "Other"};
    const int type = static_cast<int>(curve.GetType());
    QString detail = QStringLiteral("part=%1 edge=%2 type=%3 length=%4 mm start=%5 end=%6")
        .arg(part.id).arg(index).arg(types.value(type, "Other"))
        .arg(GCPnts_AbscissaPoint::Length(curve), 0, 'g', 10)
        .arg(coordinates(curve.Value(curve.FirstParameter())), coordinates(curve.Value(curve.LastParameter())));
    if (curve.GetType() == GeomAbs_Circle) {
        const auto circle = curve.Circle();
        detail += QStringLiteral(" center=%1 radius=%2 mm").arg(coordinates(circle.Location())).arg(circle.Radius(), 0, 'g', 10);
    }
    TopTools_IndexedMapOfShape faces;
    TopExp::MapShapes(part.shape, TopAbs_FACE, faces);
    TopTools_IndexedDataMapOfShapeListOfShape ancestors;
    TopExp::MapShapesAndAncestors(part.shape, TopAbs_EDGE, TopAbs_FACE, ancestors);
    QStringList adjacent;
    if (ancestors.Contains(shape)) for (TopTools_ListIteratorOfListOfShape it(ancestors.FindFromKey(shape)); it.More(); it.Next()) {
        const auto face = TopoDS::Face(it.Value());
        const auto type = BRepAdaptor_Surface(face).GetType();
        const QStringList faceTypes{"Plane", "Cylinder", "Cone", "Sphere", "Torus", "Bezier", "BSpline", "Revolution", "Extrusion", "Offset", "Other"};
        const auto description = QStringLiteral("%1:%2").arg(faces.FindIndex(face)).arg(faceTypes.value(static_cast<int>(type), "Other"));
        if (!adjacent.contains(description)) adjacent.append(description);
    }
    return detail + QStringLiteral(" adjacentFaces=[%1]").arg(adjacent.join(","));
}

bool locatedOnShape(const TopoDS_Vertex& point, const TopoDS_Shape& shape, TopAbs_ShapeEnum kind,
                    double& minimumDistance, double tolerance = MatchToleranceMm)
{
    if (kind == TopAbs_EDGE) {
        const auto edge = TopoDS::Edge(shape);
        if (BRep_Tool::Degenerated(edge)) return false;
        const BRepAdaptor_Curve curve(edge);
        const double length = GCPnts_AbscissaPoint::Length(curve);
        if (!std::isfinite(length) || length <= Precision::Confusion()) return false;
    }
    BRepExtrema_DistShapeShape distance(point, shape);
    if (!distance.IsDone() || distance.NbSolution() < 1 || !std::isfinite(distance.Value())
        || distance.Value() > tolerance) return false;
    minimumDistance = distance.Value();
    if (kind == TopAbs_EDGE) return true;

    // A point on a shared boundary does not identify one surface. Require a
    // solution on the trimmed face interior, not merely on its infinite support
    // surface, an adjacent edge, or an outside projection.
    for (int solution = 1; solution <= distance.NbSolution(); ++solution) {
        if (distance.SupportTypeShape2(solution) != BRepExtrema_IsInFace) continue;
        double u = 0, v = 0;
        distance.ParOnFaceS2(solution, u, v);
        BRepClass_FaceClassifier classifier(TopoDS::Face(shape), gp_Pnt2d(u, v), Precision::Confusion());
        if (classifier.State() == TopAbs_IN) return true;
    }
    return false;
}

QJsonArray xyz(const gp_Pnt& p) { return {p.X(), p.Y(), p.Z()}; }

QJsonObject fingerprint(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind)
{
    QJsonObject result{{"version", 1}, {"kind", kind == TopAbs_EDGE ? "edge" : "face"}};
    Bnd_Box box; BRepBndLib::AddOptimal(shape, box, false, false);
    if (box.IsVoid() || box.IsOpen()) return {};
    double x0, y0, z0, x1, y1, z1; box.Get(x0, y0, z0, x1, y1, z1);
    result["bounds"] = QJsonArray{x0, y0, z0, x1, y1, z1};
    GProp_GProps properties;
    if (kind == TopAbs_EDGE) {
        if (BRep_Tool::Degenerated(TopoDS::Edge(shape))) return {};
        const BRepAdaptor_Curve curve(TopoDS::Edge(shape));
        const double length = GCPnts_AbscissaPoint::Length(curve);
        if (!std::isfinite(length) || length <= Precision::Confusion()) return {};
        result["type"] = int(curve.GetType()); result["measure"] = length;
        // Sort samples so reversing an edge's orientation does not change its
        // identity. The actual parameterization remains in the native BRep.
        std::vector<std::array<double, 3>> samples;
        for (int i = 0; i <= 8; ++i) {
            const auto p = curve.Value(curve.FirstParameter() + (curve.LastParameter() - curve.FirstParameter()) * i / 8.0);
            samples.push_back({p.X(), p.Y(), p.Z()});
        }
        std::sort(samples.begin(), samples.end()); QJsonArray points;
        for (const auto& p : samples) for (double x : p) points.append(x);
        result["samples"] = points;
        BRepGProp::LinearProperties(shape, properties);
    } else {
        result["type"] = int(BRepAdaptor_Surface(TopoDS::Face(shape)).GetType());
        BRepGProp::SurfaceProperties(shape, properties);
        result["measure"] = properties.Mass();
        TopTools_IndexedMapOfShape edges, wires;
        TopExp::MapShapes(shape, TopAbs_EDGE, edges); TopExp::MapShapes(shape, TopAbs_WIRE, wires);
        result["edgeCount"] = edges.Extent(); result["wireCount"] = wires.Extent();
    }
    result["center"] = xyz(properties.CentreOfMass());
    const auto inertia = properties.MatrixOfInertia();
    result["moments"] = QJsonArray{inertia(1,1), inertia(2,2), inertia(3,3), inertia(1,2), inertia(1,3), inertia(2,3)};
    return result;
}

bool sameFingerprint(const QJsonObject& a, const QJsonObject& b)
{
    if (a.isEmpty() || b.isEmpty() || a.value("version") != b.value("version")
        || a.value("kind") != b.value("kind") || a.value("type") != b.value("type")
        || a.value("edgeCount") != b.value("edgeCount") || a.value("wireCount") != b.value("wireCount")) return false;
    const auto close = [](double x, double y) {
        return std::isfinite(x) && std::isfinite(y) && std::abs(x-y) <= std::max(1e-5, std::max(std::abs(x), std::abs(y))*1e-8);
    };
    if (!close(a.value("measure").toDouble(NAN), b.value("measure").toDouble(NAN))) return false;
    for (const auto* field : {"bounds", "samples", "center", "moments"}) {
        const auto x = a.value(QLatin1String(field)).toArray(), y = b.value(QLatin1String(field)).toArray();
        if (x.size() != y.size()) return false;
        for (int i = 0; i < x.size(); ++i) if (!close(x[i].toDouble(NAN), y[i].toDouble(NAN))) return false;
    }
    return true;
}

bool eligible(const NativeMotionDocument& document, const Selector& selector, int partId)
{
    // The old point/curve and curve/curve panels pick global GeoEdge objects.
    // A construction wire need not be one of the body's physical solid parts.
    return selector.kind == TopAbs_EDGE || document.bodyPartBindings.value(selector.bodyId).contains(partId);
}

struct Catalog {
    bool calibrated = false;
    QMap<int, QVector<Candidate>> edges, faces;
};

Catalog importCatalog(const CadDocument& cad, const NativeMotionDocument& document)
{
    Catalog result;
    QMap<QString, int> occurrenceOrdinals;
    QMap<int, int> partOccurrences;
    QSet<QString> paths;
    for (const auto& part : cad.parts()) {
        // Only a complete, uniquely identified STEP occurrence sequence can
        // propose a legacy global ordinal. Primitives, repeated imports and
        // removed occurrences must never be treated as that original registry.
        if (part.instancePath.isEmpty() || paths.contains(part.instancePath)) return result;
        paths.insert(part.instancePath);
        const int separator = part.instancePath.lastIndexOf('/');
        if (separator <= 0) return result;
        const auto occurrence = part.instancePath.left(separator);
        if (!occurrenceOrdinals.contains(occurrence)) occurrenceOrdinals[occurrence] = occurrenceOrdinals.size()+1;
        partOccurrences[part.id] = occurrenceOrdinals.value(occurrence);
    }
    int witnesses = 0, offset = 0;
    QSet<int> witnessedOccurrences;
    for (const auto& value : document.collection("bodies")) {
        const auto body = value.toObject(); const auto ids = body.value("geoEntityIds").toArray();
        if (ids.size() != 1) continue;
        const auto parts = document.bodyPartBindings.value(body.value("id").toInt(-1));
        if (parts.isEmpty()) return result;
        const int occurrence = partOccurrences.value(parts.front(), -1);
        for (int part : parts) if (partOccurrences.value(part, -2) != occurrence) return result;
        const int candidateOffset = ids.first().toInt(-1000000) - occurrence;
        if (witnesses && candidateOffset != offset) return result;
        if (witnessedOccurrences.contains(occurrence)) return result;
        witnessedOccurrences.insert(occurrence); offset = candidateOffset; ++witnesses;
    }
    // Two independently matched physical bodies establish the import ordering;
    // the number is still only a candidate and is checked against real geometry
    // below. It is never interpreted as a per-part OCC index or blindly shifted.
    if (witnesses < 2) return result;
    TopTools_IndexedMapOfShape globalEdges, globalFaces;
    for (const auto& part : cad.parts()) for (const auto kind : {TopAbs_EDGE, TopAbs_FACE}) {
        auto& global = kind == TopAbs_EDGE ? globalEdges : globalFaces;
        auto& catalog = kind == TopAbs_EDGE ? result.edges : result.faces;
        TopTools_IndexedMapOfShape local; TopExp::MapShapes(part.shape, kind, local);
        for (int index = 1; index <= local.Extent(); ++index) {
            const int ordinal = global.Add(local(index));
            catalog[ordinal].append({part.id, index, 0, {}});
        }
    }
    result.calibrated = true;
    return result;
}

double faceAssemblyTolerance(const TopoDS_Face& original)
{
    const auto copy = TopoDS::Face(BRepBuilderAPI_Copy(original).Shape());
    QString error;
    if (!ensureOccFaceTriangulation(copy, 0.02, error)) return 0;
    TopLoc_Location location; const auto triangles = BRep_Tool::Triangulation(copy, location);
    if (triangles.IsNull()) return 0;
    double maximum = 0;
    for (int t = 1; t <= triangles->NbTriangles(); ++t) {
        int a,b,c; triangles->Triangle(t).Get(a,b,c); const int ids[] = {a,b,c};
        for (int e = 0; e < 3; ++e)
            maximum = std::max(maximum, triangles->Node(ids[e]).Distance(triangles->Node(ids[(e+1)%3])));
    }
    return std::max(.5, maximum*.5);
}

bool catalogSelection(const CadDocument& cad, const NativeMotionDocument& document, const Catalog& catalog,
                      const Selector& selector, const QJsonArray& ids, const gp_Pnt& origin,
                      QVector<Candidate>& selected)
{
    if (!catalog.calibrated || ids.isEmpty()) return false;
    const auto& entries = selector.kind == TopAbs_EDGE ? catalog.edges : catalog.faces;
    QSet<int> used; std::vector<TopoDS_Shape> shapes;
    double length = 0, tolerance = 0, distance = std::numeric_limits<double>::infinity();
    for (const auto& id : ids) {
        const int key = id.toInt(-1); if (key < 1 || used.contains(key)) return false; used.insert(key);
        QVector<Candidate> matches;
        for (const auto& candidate : entries.value(key))
            if (eligible(document, selector, candidate.partId)) matches.append(candidate);
        if (matches.size() != 1) return false;
        const auto candidate = matches.first(); TopTools_IndexedMapOfShape local;
        TopExp::MapShapes(cad.part(candidate.partId)->shape, selector.kind, local);
        const auto shape = local(candidate.index); shapes.push_back(shape);
        if (selector.kind == TopAbs_EDGE) {
            if (BRep_Tool::Degenerated(TopoDS::Edge(shape))) return false;
            BRepAdaptor_Curve curve(TopoDS::Edge(shape));
            const double l = GCPnts_AbscissaPoint::Length(curve); if (!std::isfinite(l) || l <= Precision::Confusion()) return false;
            length += l;
        } else tolerance = std::max(tolerance, faceAssemblyTolerance(TopoDS::Face(shape)));
        selected.append(candidate);
    }
    if (selector.kind == TopAbs_EDGE) {
        tolerance = std::max(.1, .002*length);
        // A multi-edge selection must form one unbranched chain. The converter
        // subsequently uses the same endpoint stitching for its real samples.
        std::vector<std::pair<gp_Pnt,gp_Pnt>> ends;
        for (const auto& shape : shapes) { BRepAdaptor_Curve c(TopoDS::Edge(shape)); ends.emplace_back(c.Value(c.FirstParameter()),c.Value(c.LastParameter())); }
        // Stitching alone also accepts a closed loop followed by a tail. Group
        // coincident endpoints first and reject junctions with more than two
        // incident ends. A closed edge contributes both ends to the same group.
        std::vector<gp_Pnt> points;
        for (const auto& end : ends) { points.push_back(end.first); points.push_back(end.second); }
        std::vector<int> groups(points.size()), degree(points.size(), 0);
        for (int i = 0; i < int(groups.size()); ++i) groups[i] = i;
        const auto group = [&](int i) {
            while (groups[i] != i) { groups[i] = groups[groups[i]]; i = groups[i]; }
            return i;
        };
        for (int i = 0; i < int(points.size()); ++i)
            for (int j = 0; j < i; ++j)
                if (points[i].Distance(points[j]) < MatchToleranceMm) groups[group(i)] = group(j);
        for (int i = 0; i < int(points.size()); ++i)
            if (++degree[group(i)] > 2) return false;
        gp_Pnt first=ends.front().first, last=ends.front().second; ends.erase(ends.begin());
        while (!ends.empty()) {
            bool found=false;
            for (auto it=ends.begin();it!=ends.end();++it) {
                if (last.Distance(it->first)<MatchToleranceMm) last=it->second;
                else if(last.Distance(it->second)<MatchToleranceMm) last=it->first;
                else if(first.Distance(it->second)<MatchToleranceMm) first=it->first;
                else if(first.Distance(it->first)<MatchToleranceMm) first=it->second;
                else continue;
                ends.erase(it);found=true;break;
            }
            if(!found)return false;
        }
    }
    const auto point = BRepBuilderAPI_MakeVertex(origin).Vertex();
    bool close = false;
    for (const auto& shape : shapes) {
        double d = 0;
        if (locatedOnShape(point, shape, selector.kind, d, tolerance)) { close = true; distance = std::min(distance,d); }
    }
    if (!close) return false;
    // A broad solver assembly tolerance is not topology identity. A nearby
    // opposite face, or one incident edge at a shared vertex, can also satisfy
    // that tolerance. Require independent, exclusive geometric evidence.
    bool constructionOnly = selector.kind == TopAbs_EDGE;
    for (const auto& candidate : selected) {
        TopTools_IndexedMapOfShape faces; TopExp::MapShapes(cad.part(candidate.partId)->shape,TopAbs_FACE,faces);
        if (!faces.IsEmpty()) constructionOnly=false;
    }
    double competingDistance=std::numeric_limits<double>::infinity();
    for (const auto& part : cad.parts()) {
        if (constructionOnly) {
            TopTools_IndexedMapOfShape faces; TopExp::MapShapes(part.shape,TopAbs_FACE,faces);
            if (!faces.IsEmpty()) continue;
        } else if (!document.bodyPartBindings.value(selector.bodyId).contains(part.id)) continue;
        TopTools_IndexedMapOfShape alternatives; TopExp::MapShapes(part.shape,selector.kind,alternatives);
        for (int i=1;i<=alternatives.Extent();++i) {
            bool chosen=false;for(const auto& candidate:selected)if(candidate.partId==part.id&&candidate.index==i)chosen=true;
            if(chosen)continue;
            if(selector.kind==TopAbs_EDGE&&BRep_Tool::Degenerated(TopoDS::Edge(alternatives(i))))continue;
            BRepExtrema_DistShapeShape separation(point,alternatives(i));
            if(separation.IsDone()&&separation.NbSolution()>0&&std::isfinite(separation.Value()))
                competingDistance=std::min(competingDistance,separation.Value());
        }
    }
    if (competingDistance <= std::max(distance*4.0,distance+MatchToleranceMm)) return false;
    for (auto& candidate : selected) candidate.distance = distance;
    return true;
}

} // namespace

QJsonObject nativeTopologyReference(const CadDocument& cad, int partId, int index, TopAbs_ShapeEnum kind)
{
    QJsonObject result{{"partId", partId}, {"index", index}};
    const auto* part = cad.part(partId); if (!part) return result;
    TopTools_IndexedMapOfShape shapes; TopExp::MapShapes(part->shape, kind, shapes);
    if (index < 1 || index > shapes.Extent()) return result;
    const auto signature = fingerprint(shapes(index), kind);
    if (!signature.isEmpty()) result["geometry"] = signature;
    return result;
}

bool autoBindNativeMotionTopology(const CadDocument& cad, NativeMotionDocument& document,
                                  QVector<NativeTopologyBindingIssue>& unresolved, QStringList* bindingEvidence)
{
    unresolved.clear();
    if (bindingEvidence) bindingEvidence->clear();
    auto data = document.root.value("motionData").toObject();
    auto joints = data.value("joints").toArray();
    const auto catalog = importCatalog(cad, document);
    for (int row = 0; row < joints.size(); ++row) {
        auto joint = joints[row].toObject();
        const auto type = joint.value("jointType").toString();
        QVector<Selector> selectors;
        const int action = joint.value("actionBodyId").toInt(-1), base = joint.value("baseBodyId").toInt(-1);
        if (type == "PointOnCurve") selectors.append({"curveEdgeIds", "occCurveEdges", base, TopAbs_EDGE});
        else if (type == "PointOnSurface") selectors.append({"surfaceFaceIds", "occSurfaceFaces", base, TopAbs_FACE});
        else if (type == "CurveOnCurve") {
            selectors.append({"curveIEdgeIds", "occCurveIEdges", action, TopAbs_EDGE});
            selectors.append({"curveJEdgeIds", "occCurveJEdges", base, TopAbs_EDGE});
        } else continue;

        const int jointId = joint.value("id").toInt(-1);
        const auto origin = joint.value("origin").toObject();
        const double x = origin.value("x").toDouble(NAN), y = origin.value("y").toDouble(NAN), z = origin.value("z").toDouble(NAN);
        const bool validOrigin = std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
        for (const auto& selector : selectors) {
            const auto issue = [&](const QString& reason) {
                unresolved.append({jointId, selector.occField,
                    QStringLiteral("运动副 %1（%2）%3：%4").arg(jointId).arg(joint.value("name").toString(), selector.occField, reason)});
            };
            if (!joint.value(selector.occField).toArray().isEmpty()) {
                auto references = joint.value(selector.occField).toArray();
                bool valid = true, changed = false, rebound = false;
                try {
                    for (int n = 0; n < references.size(); ++n) {
                        auto reference = references[n].toObject();
                        const auto saved = reference.value("geometry").toObject();
                        const int partId = reference.value("partId").toInt(-1), index = reference.value("index").toInt(-1);
                        const auto current = nativeTopologyReference(cad, partId, index, selector.kind);
                        if (saved.isEmpty()) {
                            // Old projects own the stored exact BRep/index. Add
                            // identity evidence when that valid reference is used
                            // again, so its next standalone export can be rebound.
                            if (!document.ownsTopologyCad) {
                                valid = false; issue(QStringLiteral("独立 JSON 仅保存了 OCC 序号，缺少几何特征；无法证明它对应本次导入的边/面。请重新选择一次并保存。")); break;
                            }
                            if (!eligible(document, selector, partId) || current.value("geometry").toObject().isEmpty()) {
                                valid = false; issue(QStringLiteral("已指定的部件/拓扑序号无效，请重新选择约束几何。")); break;
                            }
                            reference["geometry"] = current.value("geometry"); references[n] = reference; changed = true; continue;
                        }
                        if (eligible(document, selector, partId) && sameFingerprint(saved, current.value("geometry").toObject())) continue;
                        QVector<QJsonObject> matches;
                        for (const auto& part : cad.parts()) {
                            if (!eligible(document, selector, part.id)) continue;
                            TopTools_IndexedMapOfShape shapes; TopExp::MapShapes(part.shape, selector.kind, shapes);
                            for (int i = 1; i <= shapes.Extent(); ++i) {
                                const auto candidate = nativeTopologyReference(cad, part.id, i, selector.kind);
                                if (sameFingerprint(saved, candidate.value("geometry").toObject())) matches.append(candidate);
                            }
                        }
                        if (matches.size() != 1) {
                            valid = false;
                            issue(QStringLiteral("保存的几何特征匹配到 %1 个候选，不能沿用已变化的部件/拓扑序号。").arg(matches.size()));
                            break;
                        }
                        reference["partId"] = matches.first().value("partId"); reference["index"] = matches.first().value("index");
                        references[n] = reference; changed = true; rebound = true;
                    }
                } catch (const Standard_Failure& failure) {
                    valid = false; issue(QStringLiteral("验证保存的几何特征失败：%1").arg(QString::fromUtf8(failure.GetMessageString())));
                }
                if (valid && changed) joint[selector.occField] = references;
                if (valid && bindingEvidence) bindingEvidence->append(QStringLiteral("运动副 %1 %2：%3")
                    .arg(jointId).arg(selector.occField).arg(rebound
                        ? QStringLiteral("按保存的真实几何特征恢复部件及拓扑编号。") : QStringLiteral("保留已明确指定的拓扑绑定；已有几何特征已验证。")));
                continue;
            }
            // With no selected faces the original point-on-surface definition
            // uses the complete base-body surface; no new selector is required.
            if (selector.kind == TopAbs_FACE && joint.value(selector.sourceField).toArray().isEmpty()) continue;
            if (!validOrigin) { issue(QStringLiteral("缺少有效的初始约束点，无法验证拓扑身份。")); continue; }
            const auto boundParts = document.bodyPartBindings.value(selector.bodyId);
            if (selector.bodyId < 0 || boundParts.isEmpty()) {
                issue(QStringLiteral("所引用的运动体 %1 尚未绑定 CAD 部件。").arg(selector.bodyId));
                continue;
            }
            QVector<Candidate> candidates;
            QString geometryError;
            try {
                QVector<Candidate> indexed;
                if (catalogSelection(cad, document, catalog, selector, joint.value(selector.sourceField).toArray(), gp_Pnt(x,y,z), indexed)) {
                    QJsonArray references; const auto ids = joint.value(selector.sourceField).toArray();
                    for (int i = 0; i < indexed.size(); ++i) {
                        auto reference = nativeTopologyReference(cad, indexed[i].partId, indexed[i].index, selector.kind);
                        reference["sourceId"] = ids[i]; references.append(reference);
                    }
                    joint[selector.occField] = references;
                    if (bindingEvidence) bindingEvidence->append(QStringLiteral("运动副 %1 %2：原几何 ID 作为 STEP 全局拓扑候选，经独立部件顺序核验、真实%3及原求解器装配容差验证；恢复 %4 个引用，初始距离 %5 mm，已保存几何特征。")
                        .arg(jointId).arg(selector.occField).arg(selector.kind == TopAbs_EDGE ? QStringLiteral("曲线连续性") : QStringLiteral("裁剪面内部"))
                        .arg(references.size()).arg(indexed.first().distance,0,'g',10));
                    continue;
                }
                if (joint.value(selector.sourceField).toArray().size() != 1) {
                    issue(QStringLiteral("无法验证原多边/多面集合的完整拓扑对应，请明确选择完整约束几何。"));
                    continue;
                }
                const auto point = BRepBuilderAPI_MakeVertex(gp_Pnt(x, y, z)).Vertex();
                for (int partId : boundParts) {
                    const auto* part = cad.part(partId);
                    if (!part) { geometryError = QStringLiteral("CAD 部件 %1 不存在。").arg(partId); break; }
                    TopTools_IndexedMapOfShape shapes;
                    TopExp::MapShapes(part->shape, selector.kind, shapes);
                    for (int index = 1; index <= shapes.Extent(); ++index) {
                        double distance = 0;
                        if (locatedOnShape(point, shapes(index), selector.kind, distance)) {
                            const auto detail = selector.kind == TopAbs_EDGE ? edgeDescription(*part, shapes(index), index)
                                : QStringLiteral("part=%1 face=%2").arg(partId).arg(index);
                            candidates.append({partId, index, distance, detail});
                        }
                    }
                }
            } catch (const Standard_Failure& error) {
                geometryError = QStringLiteral("OCC 无法可靠验证裁剪拓扑：%1").arg(QString::fromUtf8(error.GetMessageString()));
            }
            if (!geometryError.isEmpty()) { issue(geometryError); continue; }
            if (candidates.size() != 1) {
                QStringList details;
                for (const auto& candidate : candidates) details.append(candidate.detail);
                issue(candidates.isEmpty()
                    ? QStringLiteral("初始约束点没有唯一落在对应裁剪边/面内部（容差 0.0001 mm），请明确选择。")
                    : QStringLiteral("初始约束点同时匹配 %1 个拓扑候选，请明确选择。\n%2").arg(candidates.size()).arg(details.join(QLatin1Char('\n'))));
                continue;
            }
            const auto& selected = candidates.front();
            joint[selector.occField] = QJsonArray{nativeTopologyReference(cad, selected.partId, selected.index, selector.kind)};
            if (bindingEvidence) bindingEvidence->append(QStringLiteral("运动副 %1 %2：初始约束点在运动体 %3 的裁剪%4上唯一匹配；部件 %5，拓扑序号 %6，距离 %7 mm。")
                .arg(jointId).arg(selector.occField).arg(selector.bodyId)
                .arg(selector.kind == TopAbs_EDGE ? QStringLiteral("边") : QStringLiteral("面内部"))
                .arg(selected.partId).arg(selected.index).arg(selected.distance, 0, 'g', 8) + QLatin1Char('\n') + selected.detail);
        }
        joints[row] = joint;
    }
    // Do not manufacture an empty collection in legacy documents without joints.
    if (data.contains("joints")) {
        data["joints"] = joints;
        document.root["motionData"] = data;
    }
    return unresolved.isEmpty();
}

} // namespace MotionOcc
