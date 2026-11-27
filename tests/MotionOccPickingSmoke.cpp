#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTextStream>
#include <QTimer>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
using namespace MotionOcc;
void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}
bool waitFor(const std::function<bool()>& predicate, int timeoutMs)
{
    if (predicate()) return true;
    QEventLoop loop; QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] { if (predicate()) loop.quit(); });
    timer.start(15); QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit); loop.exec();
    return predicate();
}
void mouse(OccViewport& view, QEvent::Type type, const QPointF& at, Qt::MouseButton button,
           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QMouseEvent event(type, at, view.mapToGlobal(at.toPoint()), button, buttons, modifiers);
    QApplication::sendEvent(&view, &event);
}
void click(OccViewport& view, const QPointF& at, Qt::MouseButton button = Qt::LeftButton)
{
    mouse(view, QEvent::MouseButtonPress, at, button, button);
    mouse(view, QEvent::MouseButtonRelease, at, button, Qt::NoButton);
}
bool pointsNear(const std::array<double,3>& a, const std::array<double,3>& b, double tolerance = 1e-7)
{
    for (int i = 0; i < 3; ++i) if (std::abs(a[i]-b[i]) > tolerance) return false;
    return true;
}
}

int runMotionOccPickingSmoke(QApplication& app, const QString& outputDirectory)
{
    QDir output(outputDirectory); QDir().mkpath(output.absolutePath());
    QFile report(output.filePath(QStringLiteral("picking-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return 2;
    QTextStream log(&report);
    try {
        CadDocument cad; QString error; int partId = -1;
        require(cad.addPrimitive(Primitive::Box, 40, 30, 20, partId, error), error);
        OccViewport view; view.resize(960, 700); view.show();
        QString graphicsError;
        QObject::connect(&view, &OccViewport::graphicsError, &view, [&](const QString& text) { graphicsError = text; });
        require(waitFor([&] { return view.glReady() || !graphicsError.isEmpty(); }, 10000) && view.glReady(), graphicsError);
        view.setScene(cad, {}); view.viewTop(); view.fitAll(); app.processEvents();
        require(!view.grabFramebuffer().isNull(), "Viewport produced no framebuffer");
        int partSignals = 0, changed = 0, cancelled = 0;
        QObject::connect(&view, &OccViewport::partPicked, &view, [&](int) { ++partSignals; });
        QObject::connect(&view, &OccViewport::pickSelectionChanged, &view, [&](const auto&) { ++changed; });
        QObject::connect(&view, &OccViewport::pickCancelled, &view, [&] { ++cancelled; });

        view.beginPick(PickKind::Vertex, false, {partId});
        click(view, view.projectPoint({0,0,20}, partId));
        require(view.pickSelection().size() == 1 && partSignals == 0, "Vertex pick did not stay in active field session");
        auto picked = view.pickSelection().front();
        require(pointsNear(picked.position, {0,0,20}), "Vertex coordinate is not exact CAD world mm");
        TopTools_IndexedMapOfShape vertices; TopExp::MapShapes(cad.part(partId)->shape, TopAbs_VERTEX, vertices);
        require(picked.index > 0 && picked.index <= vertices.Extent(), "Vertex index is not OCC topology numbering");
        const auto vertex = BRep_Tool::Pnt(TopoDS::Vertex(vertices(picked.index)));
        require(pointsNear(picked.position, {vertex.X(),vertex.Y(),vertex.Z()}), "Vertex index does not identify picked geometry");

        view.beginPick(PickKind::Edge, false, {partId});
        click(view, view.projectPoint({20,0,20}, partId));
        require(view.pickSelection().size() == 1, "Could not select real top edge");
        picked = view.pickSelection().front();
        require(pointsNear(picked.position, {20,0,20}) && picked.hasDirection && std::abs(picked.direction[0]) > .999, "Line representative midpoint/tangent incorrect");
        TopTools_IndexedMapOfShape edges; TopExp::MapShapes(cad.part(partId)->shape, TopAbs_EDGE, edges);
        require(picked.index > 0 && picked.index <= edges.Extent() && BRepAdaptor_Curve(TopoDS::Edge(edges(picked.index))).GetType() == GeomAbs_Line, "Edge index does not map to real edge");

        view.beginPick(PickKind::Face, false, {partId});
        click(view, view.projectPoint({13,11,20}, partId));
        require(view.pickSelection().size() == 1, "Could not select real top face");
        picked = view.pickSelection().front();
        require(pointsNear(picked.position, {20,15,20}) && pointsNear(picked.hitPosition, {13,11,20}, 1e-5), "Face center and actual cursor hit conflated");
        require(picked.hasDirection && picked.direction[2] > .999, "Face normal did not follow oriented CAD topology");
        TopTools_IndexedMapOfShape faces; TopExp::MapShapes(cad.part(partId)->shape, TopAbs_FACE, faces);
        require(picked.index > 0 && picked.index <= faces.Extent() && BRepAdaptor_Surface(TopoDS::Face(faces(picked.index))).GetType() == GeomAbs_Plane, "Face index is not an OCC face");
        require(view.grabFramebuffer().save(output.filePath("face-selection.png")), "Failed to save face selection evidence");

        view.beginPick(PickKind::Part, true, {partId});
        click(view, view.projectPoint({20,15,20}, partId));
        require(view.pickSelection().size() == 1 && pointsNear(view.pickSelection().front().position, {20,15,10}), "Part pick centroid incorrect");
        click(view, view.projectPoint({20,15,20}, partId), Qt::RightButton);
        require(view.pickSelection().isEmpty(), "Right click did not subtract latest selection");
        view.beginPick(PickKind::Face, false, {99999});
        require(!view.pickAt(view.projectPoint({20,15,20}, partId)).isValid(), "Allowed-body filter ignored");
        log << "OCC vertex, edge, face, part IDs; representative coordinates, face hit, axis, selection filter and right-click undo passed\n";

        FemMesh cloud; cloud.nodes = {{2,2,20},{10,5,20},{25,15,20},{30,25,20}};
        view.setPickNodes(partId, cloud, {101,205,500,902});
        view.beginPick(PickKind::Node, true, {partId});
        click(view, view.projectPoint(cloud.nodes[0], partId));
        click(view, view.projectPoint(cloud.nodes[1], partId));
        require(view.pickSelection().size() == 2 && view.pickSelection()[0].nodeId == 101 && view.pickSelection()[1].nodeId == 205, "Node pick lost actual node IDs");
        const auto initial = view.pickSelection();
        view.endPick(); view.beginPick(PickKind::Node, true, {partId}, initial);
        require(view.pickSelection().size() == 2, "Re-entered session did not restore node picks");
        view.setPickSelection({});
        const auto a = view.projectPoint({0,0,20}, partId), b = view.projectPoint({15,10,20}, partId);
        // Shift + 左键拖动 = 框选加
        mouse(view, QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, b, Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.pickSelection().size() == 2 && view.pickSelection()[0].nodeId == 101 && view.pickSelection()[1].nodeId == 205,
                "Shift-left rectangle did not box-add actual nodes inside rectangle");
        // Ctrl + 左键单选加
        mouse(view, QEvent::MouseButtonPress, view.projectPoint(cloud.nodes[2], partId), Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseButtonRelease, view.projectPoint(cloud.nodes[2], partId), Qt::LeftButton, Qt::NoButton, Qt::ControlModifier);
        require(view.pickSelection().size() == 3 && view.pickSelection()[2].nodeId == 500,
                "Ctrl-left click did not single-add node");
        // Ctrl + 右键单选减
        mouse(view, QEvent::MouseButtonPress, view.projectPoint(cloud.nodes[2], partId), Qt::RightButton, Qt::RightButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseButtonRelease, view.projectPoint(cloud.nodes[2], partId), Qt::RightButton, Qt::NoButton, Qt::ControlModifier);
        require(view.pickSelection().size() == 2 && view.pickSelection()[0].nodeId == 101 && view.pickSelection()[1].nodeId == 205,
                "Ctrl-right click did not single-subtract targeted node");
        // Shift + 右键拖动 = 框选减
        const auto subA = view.projectPoint({0,0,20}, partId), subB = view.projectPoint({6,6,20}, partId);
        mouse(view, QEvent::MouseButtonPress, subA, Qt::RightButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, subB, Qt::NoButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, subB, Qt::RightButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.pickSelection().size() == 1 && view.pickSelection()[0].nodeId == 205,
                "Shift-right rectangle did not box-subtract targeted node");
        const auto beforeHover = view.grabFramebuffer();
        mouse(view, QEvent::MouseMove, view.projectPoint(cloud.nodes[2], partId), Qt::NoButton, Qt::NoButton);
        require(view.grabFramebuffer() != beforeHover, "Hover highlight did not update rendered image");
        require(view.grabFramebuffer().save(output.filePath("node-selection.png")), "Failed to save node selection evidence");
        QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier); QApplication::sendEvent(&view, &escape);
        require(!view.isPicking() && view.pickSelection().isEmpty() && cancelled == 1, "Esc did not close active pick session");
        require(partSignals == 0, "Typed selection leaked into whole-part/tree selection");
        click(view, view.projectPoint({20,15,20}, partId));
        require(partSignals == 1, "Original whole-part click no longer works outside a session");
        log << "Actual-node IDs, multi-pick, session restore, Ctrl rectangle, hover, Esc and whole-part signal isolation passed\n";

        QMatrix4x4 pose; pose.translate(75,-20,10); pose.rotate(25, 0, 0, 1);
        view.setPartPoses({{partId,pose}}); view.fitAll();
        view.beginPick(PickKind::Node, false, {partId});
        click(view, view.projectPoint(cloud.nodes[2], partId));
        require(view.pickSelection().size() == 1 && view.pickSelection().front().nodeId == 500 && pointsNear(view.pickSelection().front().position, cloud.nodes[2]), "Posed picking corrupted initial world node coordinates");
        view.endPick(); view.clearPartPoses(); view.fitAll();
        log << "Posed geometry picking retained initial world CAD coordinates passed\n";

        FemMesh cells;
        cells.nodes = {{0,0,20},{40,0,20},{0,30,20},{0,0,0}};
        cells.surface = {{0,1,2},{0,3,1},{0,2,3},{1,3,2}};
        cells.tetrahedra = {{0,2,1,3}};
        view.setPickNodes(partId,cells,{101,205,500,902});
        view.beginPick(PickKind::MeshEdge,false,{partId});
        click(view,view.projectPoint({20,0,20},partId));
        require(view.pickSelection().size()==1,"Real FEM edge was not picked");
        picked=view.pickSelection().front();
        require(picked.index==1 && picked.nodeIds==QVector<int>{101,205} && pointsNear(picked.position,{20,0,20}) && picked.hasDirection && picked.direction[0]>.999,
                "FEM edge lost actual connectivity, midpoint or tangent");
        view.beginPick(PickKind::MeshFace,false,{partId});
        click(view,view.projectPoint({5,5,20},partId));
        require(view.pickSelection().size()==1,"Real FEM face was not picked");
        picked=view.pickSelection().front();
        require(picked.index==1 && picked.nodeIds==QVector<int>{101,205,500} && pointsNear(picked.position,{40.0/3,10,20}) && picked.hasDirection && picked.direction[2]>.999,
                "FEM face lost actual connectivity, centroid or oriented normal");
        view.beginPick(PickKind::Element,false,{partId});
        click(view,view.projectPoint({5,5,20},partId));
        require(view.pickSelection().size()==1,"Real tetrahedron was not picked");
        picked=view.pickSelection().front();
        require(picked.index==1 && picked.nodeIds==QVector<int>{101,500,205,902} && pointsNear(picked.position,{10,7.5,15}) && !picked.hasDirection,
                "FEM element must retain tetra connectivity and centroid without inventing a direction");
        require(view.grabFramebuffer().save(output.filePath("element-selection.png")),"Failed to save FEM selection evidence");
        view.endPick();
        log << "Real FEM edge, surface face and volume-element connectivity IDs, centroid and direction passed\n";

        FemMesh mesh; mesh.nodes = {{0,0,20},{40,0,20},{40,30,20},{0,30,20}}; mesh.surface = {{0,1,2},{0,2,3}};
        view.setScene(cad, {{partId,mesh}}); view.setDisplayMode(OccViewport::DisplayMode::Mesh); view.viewTop(); view.fitAll();
        const auto uncolored = view.grabFramebuffer();
        view.setMeshScalars({{partId,{0,.25,1,.75}}}, 0, 1, QStringLiteral("弹性位移 / mm"));
        const auto colored = view.grabFramebuffer();
        require(colored != uncolored, "Per-node scalar colors were not rendered");
        const double dpr=view.devicePixelRatioF();
        const int legendX=qRound((view.width()-177+12+9)*dpr);
        const int legendHeight=std::min(220,view.height()-100);
        const auto legendHigh=colored.pixelColor(legendX,qRound((78+legendHeight*.1)*dpr));
        const auto legendLow=colored.pixelColor(legendX,qRound((78+legendHeight*.9)*dpr));
        require(legendHigh.red()>legendHigh.blue()*2 && legendLow.blue()>legendLow.red()*2,
                "Colorbar gradient was obscured or does not match scalar ramp");
        require(colored.save(output.filePath("mesh-contour.png")), "Failed to save scalar evidence");
        view.clearMeshScalars(); require(view.grabFramebuffer() == uncolored, "Clearing contours did not restore original mesh colors");
        log << "Per-node scalar interpolation, color legend and restoration passed\n";

        cad.clear(); int cylinder = -1;
        require(cad.addPrimitive(Primitive::Cylinder,10,30,0,cylinder,error), error);
        view.setScene(cad, {}); view.setDisplayMode(OccViewport::DisplayMode::ShadedEdges); view.viewTop(); view.fitAll();
        view.beginPick(PickKind::Edge, false, {cylinder});
        click(view, view.projectPoint({10,0,30}, cylinder));
        require(view.pickSelection().size() == 1, "Circular edge could not be picked");
        picked = view.pickSelection().front();
        require(pointsNear(picked.position, {0,0,30}) && picked.hasDirection && std::abs(picked.direction[2]) > .999, "Circle pick did not provide center and rotation axis");
        view.viewFront(); view.fitAll(); view.beginPick(PickKind::Face, false, {cylinder});
        click(view, view.projectPoint({0,-10,15}, cylinder));
        require(view.pickSelection().size() == 1, "Cylindrical face could not be picked");
        picked = view.pickSelection().front();
        require(picked.hasDirection && std::abs(picked.direction[2]) > .999, "Cylindrical face direction should be its real axis");
        require(graphicsError.isEmpty(), graphicsError);
        log << "Circular-edge center/axis and cylindrical-face axis passed\n";

        CadDocument appended;
        int temporaryId = -1;
        require(appended.addPrimitive(Primitive::Box, 40, 30, 100, temporaryId, error), error);
        const auto prepared = OccViewport::prepareCadScene(appended);
        std::vector<int> ids;
        require(cad.appendImported(std::move(appended), output.filePath("prepared.step"), ids, error), error);
        require(ids.size() == 1 && ids.front() != temporaryId, "Append did not exercise CAD scene ID remapping");
        view.setScene(cad, {}, {}, prepared, ids);
        require(view.pickSelection().size() == 1 && view.pickSelection().front().partId == cylinder,
                "Appending prepared CAD geometry invalidated a pick on unchanged topology");
        view.viewTop(); view.fitAll();
        view.beginPick(PickKind::Vertex, false, {ids.front()});
        click(view, view.projectPoint({40,30,100}, ids.front()));
        require(view.pickSelection().size() == 1, "Remapped prepared part does not accept geometry picks");
        picked = view.pickSelection().front();
        require(picked.partId == ids.front() && pointsNear(picked.position, {40,30,100}),
                "Remapped prepared part changed its ID or vertex location");
        log << "Prepared scene append preserves existing picks and remaps new geometry IDs\n";
        view.endPick();

        // 验证视口交互拾取模式下（部件 / 几何面）Ctrl+左键单选加、Ctrl+右键单选减、Shift+左键框选加、Shift+右键框选减，以及大规模网格拾取流畅度
        cad.clear();
        int boxA = -1, boxB = -1;
        require(cad.addPrimitive(Primitive::Box, 20, 20, 20, boxA, error), error);
        require(cad.addPrimitive(Primitive::Box, 20, 20, 20, boxB, error), error);
        QMatrix4x4 poseB;
        poseB.translate(80, 0, 0);
        view.setScene(cad, {});
        view.setPartPoses({{boxB, poseB}});
        view.setSelectionFilter(PickKind::Part);
        view.viewTop();
        view.fitAll();
        app.processEvents();
        const QPointF ptA = view.projectPoint({10, 10, 20}, boxA);
        const QPointF ptB = view.projectPoint({10, 10, 20}, boxB);
        click(view, ptA);
        mouse(view, QEvent::MouseButtonPress, ptB, Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseButtonRelease, ptB, Qt::LeftButton, Qt::NoButton, Qt::ControlModifier);
        require(view.viewportSelection().size() == 2, "Part mode Ctrl+left did not single-add second part");
        mouse(view, QEvent::MouseButtonPress, ptA, Qt::RightButton, Qt::RightButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseButtonRelease, ptA, Qt::RightButton, Qt::NoButton, Qt::ControlModifier);
        require(view.viewportSelection().size() == 1 && view.viewportSelection().front().partId == boxB,
                "Part mode Ctrl+right did not single-subtract first part");
        const double splitX = (ptA.x() + ptB.x()) * 0.5;
        const QPointF allTopLeft(10, 10);
        const QPointF allBottomRight(view.width() - 10, view.height() - 10);
        mouse(view, QEvent::MouseButtonPress, allTopLeft, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, allBottomRight, Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, allBottomRight, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.viewportSelection().size() == 2, "Part mode Shift+left did not box-add both parts");
        const QPointF subPartB0(splitX, 10), subPartB1(view.width() - 10, view.height() - 10);
        mouse(view, QEvent::MouseButtonPress, subPartB0, Qt::RightButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, subPartB1, Qt::NoButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, subPartB1, Qt::RightButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.viewportSelection().size() == 1 && view.viewportSelection().front().partId == boxA,
                "Part mode Shift+right did not box-subtract second part");

        view.setSelectionFilter(PickKind::Face);
        mouse(view, QEvent::MouseButtonPress, allTopLeft, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, allBottomRight, Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, allBottomRight, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.viewportSelection().size() == 12, "Face mode Shift+left did not box-add all 12 faces of 2 boxes");
        mouse(view, QEvent::MouseButtonPress, subPartB0, Qt::RightButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, subPartB1, Qt::NoButton, Qt::RightButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, subPartB1, Qt::RightButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.viewportSelection().size() == 6, "Face mode Shift+right did not box-subtract 6 faces of second box");

        FemMesh dense;
        QVector<int> denseIds;
        constexpr int nx = 100, ny = 150;
        dense.nodes.reserve(nx * ny);
        denseIds.reserve(nx * ny);
        for (int iy = 0; iy < ny; ++iy) {
            for (int ix = 0; ix < nx; ++ix) {
                dense.nodes.push_back({20.0 * ix / (nx - 1), 20.0 * iy / (ny - 1), 20.0});
                denseIds.append(int(dense.nodes.size()));
            }
        }
        QElapsedTimer timer;
        timer.start();
        view.setPickNodes(boxA, dense, denseIds);
        view.setPickNodes(boxA, dense, denseIds); // 重复调用应命中 O(1) 缓存短路
        view.beginPick(PickKind::Node, true, {boxA});
        for (int step = 0; step < 10; ++step)
            mouse(view, QEvent::MouseMove, QPointF(ptA.x() - 10 + step * 2, ptA.y()), Qt::NoButton, Qt::NoButton);
        mouse(view, QEvent::MouseButtonPress, QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, QPointF(splitX, view.height() - 10), Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseButtonRelease, QPointF(splitX, view.height() - 10), Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
        QVector<QVector3D> auxPts;
        auxPts.reserve(5000);
        for (int i = 0; i < 5000; ++i) auxPts.append(QVector3D(float(i % 100) * 0.2f, float(i / 100) * 0.4f, 20.0f));
        view.setAuxiliaryPoints(QStringLiteral("bench_pts"), auxPts, QColor(24, 144, 255), 8.0f);
        const auto denseImg = view.grabFramebuffer(); // 含 15000 候选+已选节点与 5000 个三维辅助点的整帧渲染
        view.clearAuxiliaryPoints(QStringLiteral("bench_pts"));
        const qint64 elapsedMs = timer.elapsed();
        require(!denseImg.isNull() && view.pickSelection().size() == nx * ny, "Dense 15k-node Shift+left box-add missed nodes");
        require(elapsedMs < 350, QStringLiteral("Dense 15k-node pick + 5k 3D point highlight frame too slow (%1 ms)").arg(elapsedMs));
        log << "Part/Face Ctrl+Left/Right & Shift+Left/Right and 15k-node fast pick (" << elapsedMs << " ms) passed\n";

        log << "PASS; selection changes=" << changed << "\n"; log.flush();
        view.close(); return 0;
    } catch (const std::exception& failure) {
        log << "FAIL: " << failure.what() << "\n"; log.flush(); return 1;
    }
}
