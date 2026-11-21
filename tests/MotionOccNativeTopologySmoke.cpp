#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeTopologyBinding.h"

#include <QCoreApplication>
#include <QTextStream>

#include <stdexcept>

namespace {
using namespace MotionOcc;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

QJsonObject point(double x, double y, double z) { return {{"x", x}, {"y", y}, {"z", z}}; }

NativeMotionDocument fixture(int first, int second, const QString& type, const QJsonObject& origin)
{
    QJsonObject joint{{"id", 91}, {"name", "source constraint"}, {"jointType", type}, {"actionBodyId", 7},
        {"baseBodyId", 0}, {"origin", origin}, {"axis", point(0, 0, 1)}, {"curveEdgeIds", QJsonArray{98765}},
        {"surfaceFaceIds", QJsonArray{87654}}, {"curveIEdgeIds", QJsonArray{76543}}, {"curveJEdgeIds", QJsonArray{65432}}};
    NativeMotionDocument doc;
    doc.root = {{"formatVersion", 1}, {"motionData", QJsonObject{
        {"bodies", QJsonArray{QJsonObject{{"id", 0}, {"geoEntityIds", QJsonArray{400}}},
            QJsonObject{{"id", 7}, {"geoEntityIds", QJsonArray{900}}}}}, {"joints", QJsonArray{joint}}}}};
    doc.bodyPartBindings[0] = {first}; doc.bodyPartBindings[7] = {second};
    return doc;
}

void editJoint(NativeMotionDocument& document, const QJsonObject& joint)
{
    auto data = document.root.value("motionData").toObject();
    data["joints"] = QJsonArray{joint}; document.root["motionData"] = data;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTextStream output(stdout), errors(stderr);
    try {
        CadDocument cad; int first = -1, second = -1; QString error;
        require(cad.addPrimitive(Primitive::Box, 10, 20, 30, first, error), error);
        require(cad.addPrimitive(Primitive::Box, 10, 20, 30, second, error), error);
        QVector<NativeTopologyBindingIssue> issues; QStringList evidence;

        auto curve = fixture(first, second, "PointOnCurve", point(0, 10, 0));
        require(autoBindNativeMotionTopology(cad, curve, issues, &evidence), issues.isEmpty() ? "Unique edge failed" : issues.first().message);
        const auto restored = curve.entity("joints", 91);
        const auto edge = restored.value("occCurveEdges").toArray().first().toObject();
        require(edge.value("partId").toInt() == first && edge.value("index").toInt() >= 1 && edge.value("index").toInt() <= 12,
            "Point-on-curve did not bind the base body's unique actual OCC edge");
        require(restored.value("curveEdgeIds").toArray() == QJsonArray{98765} && !evidence.isEmpty(),
            "Original topology IDs were rewritten or binding evidence was omitted");
        auto explicitJoint = restored; explicitJoint["origin"] = point(900, 800, 700); editJoint(curve, explicitJoint);
        const auto explicitRoot = curve.root;
        require(autoBindNativeMotionTopology(cad, curve, issues) && curve.root == explicitRoot,
            "An explicit topology selection was overwritten by automatic recovery");

        auto ambiguous = fixture(first, second, "PointOnCurve", point(0, 0, 0));
        const auto ambiguousRoot = ambiguous.root;
        require(!autoBindNativeMotionTopology(cad, ambiguous, issues) && ambiguous.root == ambiguousRoot && issues.size() == 1,
            "A shared vertex silently chose one of several incident edges");
        auto duplicate = fixture(first, second, "PointOnCurve", point(0, 10, 0));
        duplicate.bodyPartBindings[0] = {first, second};
        require(!autoBindNativeMotionTopology(cad, duplicate, issues)
            && duplicate.entity("joints", 91).value("occCurveEdges").toArray().isEmpty(),
            "Two coincident geometry candidates were silently resolved by CAD/source IDs");
        auto outside = fixture(first, second, "PointOnCurve", point(0, 25, 0));
        require(!autoBindNativeMotionTopology(cad, outside, issues), "An infinite support-line match ignored the trimmed edge endpoints");
        auto multiple = fixture(first, second, "PointOnCurve", point(0, 10, 0));
        auto multipleJoint = multiple.entity("joints", 91); multipleJoint["curveEdgeIds"] = QJsonArray{1, 2}; editJoint(multiple, multipleJoint);
        require(!autoBindNativeMotionTopology(cad, multiple, issues), "An original multi-edge path was reduced to a single nearby edge");

        auto surface = fixture(first, second, "PointOnSurface", point(5, 10, 0));
        require(autoBindNativeMotionTopology(cad, surface, issues), "Unique trimmed face interior was not restored");
        require(surface.entity("joints", 91).value("occSurfaceFaces").toArray().first().toObject().value("partId").toInt() == first,
            "Point-on-surface used the action body's geometry instead of the base body");
        auto boundary = fixture(first, second, "PointOnSurface", point(0, 10, 0));
        require(!autoBindNativeMotionTopology(cad, boundary, issues), "A face boundary was incorrectly treated as an interior identity");
        auto plane = fixture(first, second, "PointOnSurface", point(100, 200, 300));
        auto planeJoint = plane.entity("joints", 91); planeJoint["surfaceFaceIds"] = QJsonArray{}; editJoint(plane, planeJoint);
        const auto planeRoot = plane.root;
        require(autoBindNativeMotionTopology(cad, plane, issues) && plane.root == planeRoot,
            "A valid whole-base-body surface definition incorrectly demanded an explicit face selection");

        auto pair = fixture(first, second, "CurveOnCurve", point(0, 10, 0));
        require(autoBindNativeMotionTopology(cad, pair, issues), "Curve-on-curve did not identify each body's owned edge separately");
        const auto joined = pair.entity("joints", 91);
        require(joined.value("occCurveIEdges").toArray().first().toObject().value("partId").toInt() == second
            && joined.value("occCurveJEdges").toArray().first().toObject().value("partId").toInt() == first,
                "Curve-on-curve swapped action/base ownership");
        output << "Native topology: unique trimmed edge/face recovery, body ownership, ambiguity rejection and explicit binding preservation passed\n";
        return 0;
    } catch (const std::exception& error) {
        errors << error.what() << '\n'; return 1;
    }
}
