#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"

#include <BRep_Tool.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <TopExp.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
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
QImage frame(OccViewport& view)
{
    const auto image = view.grabFramebuffer().convertToFormat(QImage::Format_RGB32);
    require(!image.isNull(), "No actual OpenGL framebuffer");
    return image;
}
int changed(const QImage& a, const QImage& b, const QRectF& logical, const OccViewport& view)
{
    require(a.size() == b.size(), "Framebuffer size changed during comparison");
    const double sx = double(a.width()) / view.width(), sy = double(a.height()) / view.height();
    const auto region = QRectF(logical.x()*sx, logical.y()*sy, logical.width()*sx, logical.height()*sy)
        .toAlignedRect().intersected(a.rect());
    require(!region.isEmpty(), "Pixel assertion is outside the viewport");
    int count = 0;
    for (int y = region.top(); y <= region.bottom(); ++y) for (int x = region.left(); x <= region.right(); ++x) {
        const QRgb p = a.pixel(x,y), q = b.pixel(x,y);
        if (std::abs(qRed(p)-qRed(q)) + std::abs(qGreen(p)-qGreen(q)) + std::abs(qBlue(p)-qBlue(q)) > 6) ++count;
    }
    return count;
}
}

int runMotionOccRenderFeedbackSmoke(QApplication&, const QString& outputDirectory)
{
    QDir output(outputDirectory); QDir().mkpath(output.absolutePath());
    QFile report(output.filePath("render-feedback-smoke.txt"));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return 2;
    QTextStream log(&report);
    try {
        // A small rear box lies entirely behind the front box in top view.
        // This is a visibility fixture, not a physical contact model.
        CadDocument cad; QString error; int front = -1, rear = -1;
        require(cad.addPrimitive(Primitive::Box, 40,30,20,front,error), error);
        require(cad.addPrimitive(Primitive::Box, 10,10,5,rear,error), error);
        TopTools_IndexedMapOfShape faces, edges, vertices;
        TopExp::MapShapes(cad.part(rear)->shape, TopAbs_FACE, faces);
        TopExp::MapShapes(cad.part(rear)->shape, TopAbs_EDGE, edges);
        TopExp::MapShapes(cad.part(rear)->shape, TopAbs_VERTEX, vertices);
        int topFace = -1, topEdge = -1, topVertex = -1;
        std::array<double,3> edgePoint{}, vertexPoint{};
        for (int i=1; i<=faces.Extent(); ++i) {
            GProp_GProps properties; BRepGProp::SurfaceProperties(faces(i), properties);
            if (std::abs(properties.CentreOfMass().Z()-5) < 1e-8) topFace=i;
        }
        for (int i=1; i<=edges.Extent(); ++i) {
            BRepAdaptor_Curve curve(TopoDS::Edge(edges(i)));
            const auto p=curve.Value((curve.FirstParameter()+curve.LastParameter())*.5);
            if (std::abs(p.Z()-5)<1e-8 && p.X()>1 && p.Y()>1) {
                topEdge=i; edgePoint={p.X(),p.Y(),p.Z()}; break;
            }
        }
        for (int i=1; i<=vertices.Extent(); ++i) {
            const auto p=BRep_Tool::Pnt(TopoDS::Vertex(vertices(i)));
            if (std::abs(p.Z()-5)<1e-8 && p.X()>1 && p.Y()>1) {
                topVertex=i; vertexPoint={p.X(),p.Y(),p.Z()}; break;
            }
        }
        require(topFace>0 && topEdge>0 && topVertex>0, "Missing real OCC fixture topology");
        OccViewport view; view.resize(960,700); view.show();
        QString graphicsError;
        QObject::connect(&view,&OccViewport::graphicsError,&view,[&](const QString& text){graphicsError=text;});
        require(waitFor([&]{return view.glReady()||!graphicsError.isEmpty();},10000)&&view.glReady(),graphicsError);
        view.setScene(cad,{}); view.viewTop(); view.fitAll();
        const auto sample=view.projectPoint({5,5,5},rear);
        const QRectF faceRegion(sample-QPointF(12,12),QSizeF(24,24));
        const auto setPick=[&](PickKind kind,int index) {
            PickResult p; p.kind=kind; p.partId=rear; p.index=index;
            view.setPickSelection({p});
            require(view.pickSelection().size()==1,"Real topology selection was rejected");
        };
        view.beginPick(PickKind::Face,false,{rear});
        const auto hiddenFaceBase=frame(view);
        setPick(PickKind::Face,topFace);
        require(changed(hiddenFaceBase,frame(view),faceRegion,view)==0,"Hidden CAD face leaked through the front solid");
        log << "Hidden face fill obeys the scene depth buffer\n";

        view.beginPick(PickKind::Edge,false,{rear});
        const auto hiddenEdgeBase=frame(view);
        setPick(PickKind::Edge,topEdge);
        const auto edgeScreen=view.projectPoint(edgePoint,rear);
        require(changed(hiddenEdgeBase,frame(view),QRectF(edgeScreen-QPointF(10,10),QSizeF(20,20)),view)==0,
                "Hidden CAD edge leaked through the front solid");
        log << "Hidden CAD edge stroke obeys the scene depth buffer\n";

        view.beginPick(PickKind::Vertex,false,{rear});
        const auto hiddenVertexBase=frame(view);
        setPick(PickKind::Vertex,topVertex);
        const auto vertexScreen=view.projectPoint(vertexPoint,rear);
        const QRectF markerRegion(vertexScreen-QPointF(12,12),QSizeF(24,24));
        require(changed(hiddenVertexBase,frame(view),markerRegion,view)==0,"Hidden vertex marker leaked through the front solid");

        QMatrix4x4 moved; moved.translate(50,0,0);
        view.setPartPoses({{front,moved}});
        view.setPickSelection({}); const auto visibleVertexBase=frame(view);
        setPick(PickKind::Vertex,topVertex);
        require(changed(visibleVertexBase,frame(view),markerRegion,view)>10,"Visible CAD vertex marker was not rendered");
        view.beginPick(PickKind::Face,false,{rear});
        const auto visibleFaceBase=frame(view);
        setPick(PickKind::Face,topFace);
        const auto visibleFace=frame(view);
        require(changed(visibleFaceBase,visibleFace,faceRegion,view)>100,"Visible face selection did not tint the surface");
        require((view.projectPoint({5,5,5},rear)-sample).manhattanLength()<1e-5,
                "Pose updates changed the viewport framing");
        visibleFace.save(output.filePath("visible-face-feedback.png"));
        view.clearPartPoses();
        require(changed(hiddenFaceBase,frame(view),faceRegion,view)==0,"Cached face highlight ignored restored occlusion");
        log << "Visible face/vertex feedback and animated occlusion restoration passed\n";

        view.setScene(cad,{}, {rear});
        const auto hiddenPart=frame(view);
        view.setPickSelection({});
        require(changed(hiddenPart,frame(view),faceRegion,view)==0,"Hidden part left stale cached CAD feedback");
        view.setScene(cad,{}, {front});
        view.beginPick(PickKind::Face,false,{rear});
        const auto revealedBase=frame(view);
        setPick(PickKind::Face,topFace);
        require(changed(revealedBase,frame(view),faceRegion,view)>100,"Scene replacement did not rebuild CAD feedback");
        view.endPick();
        require(changed(revealedBase,frame(view),faceRegion,view)==0,"Ending selection left stale face feedback");
        log << "Hide/show, scene replacement and selection cleanup passed\n";
        view.makeCurrent();
        require(view.context()->functions()->glGetError()==GL_NO_ERROR,"OpenGL reported an error after feedback rendering");
        view.doneCurrent();
        require(graphicsError.isEmpty(),graphicsError);
        log << "PASS; Qt DPR=" << view.devicePixelRatioF() << "\n"; log.flush();
        view.close(); return 0;
    } catch(const std::exception& failure) {
        log << "FAIL: " << failure.what() << "\n"; log.flush(); return 1;
    }
}
