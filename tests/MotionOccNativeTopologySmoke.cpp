#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeTopologyBinding.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QTemporaryDir>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRep_Builder.hxx>
#include <STEPCAFControl_Writer.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <TopoDS_Compound.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Circ.hxx>

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

enum class ConstructionPath { OpenChain, ClosedCurve, ClosedCurveWithTail };

void importReferenceFixture(CadDocument& cad, const QString& path, QString& error,
                            ConstructionPath kind = ConstructionPath::OpenChain)
{
    Handle(TDocStd_Document) document = new TDocStd_Document("BinXCAF");
    const auto tool = XCAFDoc_DocumentTool::ShapeTool(document->Main());
    const auto add = [&](const TopoDS_Shape& shape, const char* name) {
        const auto label = tool->AddShape(shape, false);
        TDataStd_Name::Set(label, TCollection_ExtendedString(name));
    };
    add(BRepPrimAPI_MakeBox(gp_Pnt(0,0,0),10,20,30).Shape(), "support");
    add(BRepPrimAPI_MakeBox(gp_Pnt(50,0,0),3,4,5).Shape(), "follower");
    if (kind == ConstructionPath::OpenChain) {
        BRepBuilderAPI_MakeWire wire;
        wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(100,0,0),gp_Pnt(100,20,0)).Edge());
        wire.Add(BRepBuilderAPI_MakeEdge(gp_Pnt(100,20,0),gp_Pnt(110,30,0)).Edge());
        add(wire.Wire(), "construction path");
    } else {
        const gp_Circ circle(gp_Ax2(gp_Pnt(110,0,0),gp_Dir(0,0,1),gp_Dir(1,0,0)),10);
        add(BRepBuilderAPI_MakeEdge(circle).Edge(), "closed construction curve");
        if (kind == ConstructionPath::ClosedCurveWithTail)
            add(BRepBuilderAPI_MakeEdge(gp_Pnt(120,0,0),gp_Pnt(130,0,0)).Edge(), "construction tail");
    }
    STEPCAFControl_Writer writer;
    require(writer.Transfer(document, STEPControl_AsIs), "Cannot transfer reference fixture");
    require(writer.Write(path.toUtf8().constData()) == IFSelect_RetDone, "Cannot write reference fixture");
    std::vector<int> ids;
    require(cad.importFile(path,ids,error),error);
    // STEP may export a multi-edge wire as separate geometric-curve leaves.
    // The adapter must retain both actual edges, independent of part grouping.
    require(ids.size()>=3,"Reference fixture did not retain construction geometry");
    int constructionEdges=0;
    for(std::size_t i=2;i<cad.parts().size();++i){
        TopTools_IndexedMapOfShape edges,faces;TopExp::MapShapes(cad.parts()[i].shape,TopAbs_EDGE,edges);TopExp::MapShapes(cad.parts()[i].shape,TopAbs_FACE,faces);
        require(faces.IsEmpty(),"Construction fixture gained a surrogate solid");constructionEdges+=edges.Extent();
    }
    require(constructionEdges==(kind == ConstructionPath::ClosedCurve ? 1 : 2),"Construction fixture lost a real path edge");
}

NativeMotionDocument catalogFixture(const CadDocument& cad)
{
    auto doc=fixture(cad.parts()[0].id,cad.parts()[1].id,"PointOnCurve",point(100,.01,0));
    auto data=doc.root.value("motionData").toObject(), joint=doc.entity("joints",91);
    auto bodies=data.value("bodies").toArray();
    for(int i=0;i<bodies.size();++i){auto b=bodies[i].toObject();b["geoEntityIds"]=QJsonArray{i+2};bodies[i]=b;}
    data["bodies"]=bodies;doc.root["motionData"]=data;
    // Two boxes contribute 24 edges; these are global import candidates, not
    // per-part indices. The two actual wire edges must survive as one path.
    joint["curveEdgeIds"]=QJsonArray{25,26};editJoint(doc,joint);return doc;
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

        QTemporaryDir temporary;require(temporary.isValid(),"No temporary directory");
        CadDocument imported;
        const auto path=temporary.filePath("construction.step");
        importReferenceFixture(imported,path,error);
        auto legacy=catalogFixture(imported);
        require(autoBindNativeMotionTopology(imported,legacy,issues,&evidence),issues.isEmpty()?"Construction path not restored":issues.first().message);
        const auto restoredPath=legacy.entity("joints",91).value("occCurveEdges").toArray();
        require(restoredPath.size()==2,"A multi-edge path lost one of its global references");
        for(const auto& v:restoredPath){const auto ref=v.toObject();require(ref.value("partId").toInt()>=imported.parts()[2].id
            && !ref.value("geometry").toObject().isEmpty(),"Standalone curve was replaced by a physical body edge or lost its geometry identity");}
        require(validateNativeMotionBindings(imported,legacy,error),error);

        CadDocument loop;
        importReferenceFixture(loop,temporary.filePath("closed-curve.step"),error,ConstructionPath::ClosedCurve);
        auto closed=catalogFixture(loop);auto closedJoint=closed.entity("joints",91);
        closedJoint["origin"]=point(100,0,0);closedJoint["curveEdgeIds"]=QJsonArray{25};editJoint(closed,closedJoint);
        require(autoBindNativeMotionTopology(loop,closed,issues)
            && closed.entity("joints",91).value("occCurveEdges").toArray().size()==1,
            "A valid closed edge was rejected by automatic path recovery");
        CadDocument loopWithTail;
        importReferenceFixture(loopWithTail,temporary.filePath("closed-curve-with-tail.step"),error,ConstructionPath::ClosedCurveWithTail);
        auto branched=catalogFixture(loopWithTail);auto branchedJoint=branched.entity("joints",91);
        branchedJoint["origin"]=point(100,0,0);editJoint(branched,branchedJoint);
        const auto branchedRoot=branched.root;
        require(!autoBindNativeMotionTopology(loopWithTail,branched,issues) && issues.size()==1
            && branched.root==branchedRoot && branched.entity("joints",91).value("occCurveEdges").toArray().isEmpty(),
            "A closed curve plus a tail was automatically accepted as an unbranched path");

        auto nearFace=catalogFixture(imported);auto faceJoint=nearFace.entity("joints",91);
        faceJoint["jointType"]="PointOnSurface";faceJoint["origin"]=point(-.2,10,15);faceJoint["surfaceFaceIds"]=QJsonArray{1};editJoint(nearFace,faceJoint);
        require(autoBindNativeMotionTopology(imported,nearFace,issues),"A verified face was rejected before the original assembly tolerance was applied");
        require(nearFace.entity("joints",91).value("occSurfaceFaces").toArray().first().toObject().value("partId").toInt()==imported.parts()[0].id,
            "Global face mapping lost base-body ownership");
        auto opposite=catalogFixture(imported);faceJoint["surfaceFaceIds"]=QJsonArray{2};editJoint(opposite,faceJoint);
        require(!autoBindNativeMotionTopology(imported,opposite,issues)
            && opposite.entity("joints",91).value("occSurfaceFaces").toArray().isEmpty(),
            "Wide assembly tolerance incorrectly proved the identity of a farther opposite face");

        auto bare=legacy;auto bareJoint=bare.entity("joints",91);auto bareRefs=bareJoint.value("occCurveEdges").toArray();
        for(int i=0;i<bareRefs.size();++i){auto ref=bareRefs[i].toObject();ref.remove("geometry");bareRefs[i]=ref;}
        bareJoint["occCurveEdges"]=bareRefs;editJoint(bare,bareJoint);
        require(!autoBindNativeMotionTopology(imported,bare,issues),"Bare external JSON indices were silently treated as saved CAD identities");
        bare.ownsTopologyCad=true;
        require(autoBindNativeMotionTopology(imported,bare,issues)
            && !bare.entity("joints",91).value("occCurveEdges").toArray().first().toObject().value("geometry").toObject().isEmpty(),
            "An original project could not capture fingerprints from its own stored BReps");

        // Reimport into a nonempty project changes every part ID. The stored
        // geometry, rather than the now-stale numbers, must restore the path.
        CadDocument shifted;int dummy=-1;
        require(shifted.addPrimitive(Primitive::Sphere,2,0,0,dummy,error),error);
        std::vector<int> shiftedIds;require(shifted.importFile(path,shiftedIds,error),error);
        auto remapped=legacy;remapped.bodyPartBindings[0]={shiftedIds[0]};remapped.bodyPartBindings[7]={shiftedIds[1]};
        require(autoBindNativeMotionTopology(shifted,remapped,issues,&evidence),"Stored curve identity did not survive part reallocation");
        const auto remappedPath=remapped.entity("joints",91).value("occCurveEdges").toArray();
        for(int i=0;i<remappedPath.size();++i)
            require(remappedPath[i].toObject().value("partId").toInt()==restoredPath[i].toObject().value("partId").toInt()+1,"Reimport retained a stale construction part number");
        require(remapped.entity("joints",91).value("curveEdgeIds").toArray()==QJsonArray{25,26},"Original global IDs were overwritten");

        auto missing=remapped;for(std::size_t i=2;i<shiftedIds.size();++i)require(shifted.remove(shiftedIds[i]),"Cannot remove fixture curve");
        const auto missingRoot=missing.root;
        require(!autoBindNativeMotionTopology(shifted,missing,issues)&&missing.root==missingRoot,
            "A missing construction path was silently replaced with another edge");

        auto duplicateSignature=curve;auto duplicateJoint=duplicateSignature.entity("joints",91);
        auto duplicateRefs=duplicateJoint.value("occCurveEdges").toArray();auto stale=duplicateRefs[0].toObject();stale["partId"]=99999;
        duplicateRefs[0]=stale;duplicateJoint["occCurveEdges"]=duplicateRefs;editJoint(duplicateSignature,duplicateJoint);
        require(!autoBindNativeMotionTopology(cad,duplicateSignature,issues),"Two identical topology fingerprints were silently resolved by order");
        output << "Native topology: trimmed geometry, verified legacy global paths, standalone curves, original assembly tolerance, saved fingerprint rebind and ambiguity rejection passed\n";
        return 0;
    } catch (const std::exception& error) {
        errors << error.what() << '\n'; return 1;
    }
}
