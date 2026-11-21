#include "NativeTopologyBinding.h"
#include "NativeMotionDocument.h"
#include "CadDocument.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
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
                    double& minimumDistance)
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
        || distance.Value() > MatchToleranceMm) return false;
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

} // namespace

bool autoBindNativeMotionTopology(const CadDocument& cad, NativeMotionDocument& document,
                                  QVector<NativeTopologyBindingIssue>& unresolved, QStringList* bindingEvidence)
{
    unresolved.clear();
    if (bindingEvidence) bindingEvidence->clear();
    auto data = document.root.value("motionData").toObject();
    auto joints = data.value("joints").toArray();
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
                if (bindingEvidence) bindingEvidence->append(QStringLiteral("运动副 %1 %2：保留已明确指定的 OCC 拓扑绑定。")
                    .arg(jointId).arg(selector.occField));
                continue;
            }
            // With no selected faces the original point-on-surface definition
            // uses the complete base-body surface; no new selector is required.
            if (selector.kind == TopAbs_FACE && joint.value(selector.sourceField).toArray().isEmpty()) continue;
            if (joint.value(selector.sourceField).toArray().size() != 1) {
                issue(QStringLiteral("原设置未唯一指定一条边/一个面，请明确选择完整约束几何。"));
                continue;
            }
            if (!validOrigin) { issue(QStringLiteral("缺少有效的初始约束点，无法验证拓扑身份。")); continue; }
            const auto boundParts = document.bodyPartBindings.value(selector.bodyId);
            if (selector.bodyId < 0 || boundParts.isEmpty()) {
                issue(QStringLiteral("所引用的运动体 %1 尚未绑定 CAD 部件。").arg(selector.bodyId));
                continue;
            }
            QVector<Candidate> candidates;
            QString geometryError;
            try {
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
            joint[selector.occField] = QJsonArray{QJsonObject{{"partId", selected.partId}, {"index", selected.index}}};
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
