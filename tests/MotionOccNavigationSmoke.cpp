#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QTextStream>
#include <QTimer>
#include <QWheelEvent>
#include <array>
#include <cmath>
#include <functional>
#include <stdexcept>

namespace {
using namespace MotionOcc;
using Point = std::array<double, 3>;
using Projection = std::array<QPointF, 3>;

void require(bool condition, const QString& message)
{
    if (!condition) throw std::runtime_error(message.toStdString());
}

bool waitFor(const std::function<bool()>& predicate, int timeoutMs)
{
    if (predicate()) return true;
    QEventLoop loop;
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, &loop, [&] { if (predicate()) loop.quit(); });
    timer.start(15);
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
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

void key(OccViewport& view, QEvent::Type type, int code, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    QKeyEvent event(type, code, modifiers);
    QApplication::sendEvent(&view, &event);
}

void drag(OccViewport& view, const QPointF& from, const QPointF& to, Qt::MouseButton button,
          Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    mouse(view, QEvent::MouseButtonPress, from, button, button, modifiers);
    mouse(view, QEvent::MouseMove, to, Qt::NoButton, button, modifiers);
    mouse(view, QEvent::MouseButtonRelease, to, button, Qt::NoButton, modifiers);
}

Projection projected(const OccViewport& view, int partId)
{
    return {view.projectPoint({4, 5, 20}, partId), view.projectPoint({33, 7, 3}, partId),
            view.projectPoint({17, 26, 15}, partId)};
}

bool pointsNear(const QPointF& a, const QPointF& b, double tolerance = 0.002)
{
    return std::hypot(a.x()-b.x(), a.y()-b.y()) < tolerance;
}

bool sameProjection(const Projection& a, const Projection& b)
{
    for (std::size_t i = 0; i < a.size(); ++i) if (!pointsNear(a[i], b[i])) return false;
    return true;
}

void translated(const Projection& before, const Projection& after, const QPointF& delta, const QString& reason)
{
    for (std::size_t i = 0; i < before.size(); ++i) require(pointsNear(after[i], before[i]+delta), reason);
}
}

int runMotionOccNavigationSmoke(QApplication& app, const QString& outputDirectory)
{
    QDir output(outputDirectory);
    QDir().mkpath(output.absolutePath());
    QFile report(output.filePath(QStringLiteral("navigation-smoke.txt")));
    if (!report.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return 2;
    QTextStream log(&report);
    try {
        CadDocument cad;
        QString error;
        int partId = -1;
        require(cad.addPrimitive(Primitive::Box, 40, 30, 20, partId, error), error);
        OccViewport view;
        view.resize(960, 700);
        QString graphicsError;
        QObject::connect(&view, &OccViewport::graphicsError, &view, [&](const QString& text) { graphicsError = text; });
        view.show();
        require(waitFor([&] { return view.glReady() || !graphicsError.isEmpty(); }, 10000) && view.glReady(),
                QStringLiteral("Navigation OpenGL context: ")+graphicsError);
        view.setScene(cad, {});
        view.viewTop();
        view.fitAll();
        app.processEvents();
        int selected = 0, cancelled = 0;
        QObject::connect(&view, &OccViewport::partPicked, &view, [&](int) { ++selected; });
        QObject::connect(&view, &OccViewport::pickCancelled, &view, [&] { ++cancelled; });
        const auto reset = [&] { view.viewTop(); view.fitAll(); };
        const QPointF center(view.width()*0.5, view.height()*0.5);

        require(view.mouseNavigationMode() == OccViewport::MouseNavigationMode::Classic,
                "Classic navigation must remain the default");
        auto before = projected(view, partId);
        drag(view, center, center+QPointF(83, 41), Qt::LeftButton);
        require(!sameProjection(before, projected(view, partId)) && selected == 0,
                "Classic left drag must orbit without selecting a part");
        for (auto button : {Qt::MiddleButton, Qt::RightButton}) {
            reset(); before = projected(view, partId);
            drag(view, center, center+QPointF(52, -31), button);
            translated(before, projected(view, partId), {52, -31}, "Classic pan direction or scale changed");
        }
        reset();
        const Point anchorWorld{13, 11, 20};
        auto anchor = view.projectPoint(anchorWorld, partId);
        before = projected(view, partId);
        QWheelEvent wheel(anchor, view.mapToGlobal(anchor.toPoint()), {}, {0,120},
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&view, &wheel);
        require(pointsNear(anchor, view.projectPoint(anchorWorld, partId)) &&
                !sameProjection(before, projected(view, partId)), "Wheel zoom did not retain its cursor anchor");
        log << "Classic orbit/pan, click isolation and cursor-anchored wheel zoom passed\n";

        view.setMouseNavigationMode(OccViewport::MouseNavigationMode::Nx);
        require(view.mouseNavigationHint().contains("Shift+中键") && view.mouseNavigationHint().contains("Ctrl+中键"),
                "NX navigation help does not describe its modifiers");
        reset(); before = projected(view, partId);
        drag(view, center, center+QPointF(83, 41), Qt::LeftButton);
        require(sameProjection(before, projected(view, partId)) && selected == 0,
                "NX left drag unexpectedly moved the camera or selected a part");
        drag(view, center, center+QPointF(83, 41), Qt::MiddleButton);
        require(!sameProjection(before, projected(view, partId)), "NX middle drag did not orbit");
        reset(); before = projected(view, partId);
        drag(view, center, center+QPointF(52, -31), Qt::MiddleButton, Qt::ShiftModifier);
        translated(before, projected(view, partId), {52, -31}, "NX Shift-middle pan is incorrect");
        reset(); anchor = view.projectPoint(anchorWorld, partId); before = projected(view, partId);
        drag(view, anchor, anchor+QPointF(0, 65), Qt::MiddleButton, Qt::ControlModifier);
        require(pointsNear(anchor, view.projectPoint(anchorWorld, partId)) &&
                !sameProjection(before, projected(view, partId)), "NX Ctrl-middle zoom lost its press-point anchor");
        log << "NX left selection, middle orbit, Shift-middle pan and Ctrl-middle zoom passed\n";

        reset();
        QPointF pointer = center+QPointF(36, 19);
        mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton);
        before = projected(view, partId);
        key(view, QEvent::KeyPress, Qt::Key_Shift, Qt::ShiftModifier);
        require(sameProjection(before, projected(view, partId)), "Pressing Shift jumped the camera");
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton, Qt::ShiftModifier);
        require(sameProjection(before, projected(view, partId)), "Modifier-transition move jumped the camera");
        pointer += QPointF(40, -20);
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton, Qt::ShiftModifier);
        translated(before, projected(view, partId), {40, -20}, "Switching orbit to pan used a stale anchor");
        before = projected(view, partId);
        key(view, QEvent::KeyPress, Qt::Key_Control, Qt::ShiftModifier|Qt::ControlModifier);
        require(sameProjection(before, projected(view, partId)), "Pressing Ctrl jumped the camera");
        pointer += QPointF(0, 35);
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton, Qt::ShiftModifier|Qt::ControlModifier);
        const auto zoomed = projected(view, partId);
        require(std::abs(QLineF(before[0],before[1]).length()-QLineF(zoomed[0],zoomed[1]).length()) > 1,
                "Ctrl must take precedence over Shift during NX navigation");
        key(view, QEvent::KeyRelease, Qt::Key_Control, Qt::ShiftModifier);
        require(sameProjection(zoomed, projected(view, partId)), "Releasing Ctrl jumped the camera");
        pointer += QPointF(25,-10);
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton, Qt::ShiftModifier);
        translated(zoomed, projected(view, partId), {25,-10}, "Returning from zoom to pan used a stale anchor");
        before = projected(view, partId);
        key(view, QEvent::KeyRelease, Qt::Key_Shift);
        require(sameProjection(before, projected(view, partId)), "Returning to orbit jumped the camera");
        mouse(view, QEvent::MouseButtonPress, pointer, Qt::RightButton, Qt::MiddleButton|Qt::RightButton);
        require(sameProjection(before, projected(view, partId)), "A second button reset the active gesture");
        pointer += QPointF(34,24);
        mouse(view, QEvent::MouseMove, pointer, Qt::NoButton, Qt::MiddleButton|Qt::RightButton);
        require(!sameProjection(before, projected(view, partId)), "Orbit did not continue after modifier transition");
        mouse(view, QEvent::MouseButtonRelease, pointer, Qt::RightButton, Qt::MiddleButton);
        mouse(view, QEvent::MouseButtonRelease, pointer, Qt::MiddleButton, Qt::NoButton);
        log << "Held-button modifier transitions, Ctrl precedence and second-button isolation passed\n";

        for (const auto type : {QEvent::UngrabMouse, QEvent::WindowDeactivate, QEvent::FocusOut}) {
            reset();
            mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
            mouse(view, QEvent::MouseMove, center+QPointF(40,20), Qt::NoButton, Qt::MiddleButton);
            before = projected(view, partId);
            if (type == QEvent::FocusOut) {
                QFocusEvent event(type); QApplication::sendEvent(&view, &event);
            } else {
                QEvent event(type); QApplication::sendEvent(&view, &event);
            }
            mouse(view, QEvent::MouseButtonRelease, center+QPointF(130,90), Qt::MiddleButton, Qt::NoButton);
            require(sameProjection(before, projected(view, partId)), "Lost capture/focus allowed a stale release to move the view");
        }
        reset();
        mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
        before = projected(view, partId);
        view.setEnabled(false); view.setEnabled(true);
        mouse(view, QEvent::MouseButtonRelease, center+QPointF(130,90), Qt::MiddleButton, Qt::NoButton);
        require(sameProjection(before, projected(view, partId)), "Disabled viewport retained an active gesture");
        mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
        view.hide(); view.show(); app.processEvents();
        before = projected(view, partId);
        mouse(view, QEvent::MouseButtonRelease, center+QPointF(130,90), Qt::MiddleButton, Qt::NoButton);
        require(sameProjection(before, projected(view, partId)), "Hidden viewport retained an active gesture");
        log << "Lost capture, focus, deactivate, disable and hide interruption passed\n";

        reset(); before = projected(view, partId); anchor = view.projectPoint(anchorWorld, partId);
        mouse(view, QEvent::MouseButtonDblClick, anchor, Qt::LeftButton, Qt::LeftButton);
        require(sameProjection(before, projected(view, partId)), "Changing rotation center jumped the scene");
        drag(view, anchor+QPointF(60,0), anchor+QPointF(110,45), Qt::MiddleButton);
        require(pointsNear(anchor, view.projectPoint(anchorWorld, partId)), "Orbit did not use the chosen surface center");
        require(!sameProjection(before, projected(view, partId)), "Orbit about the chosen center did not rotate");
        QMatrix4x4 pose; pose.translate(75,-20,10); pose.rotate(25,0,0,1);
        view.setPartPoses({{partId,pose}}); reset();
        before = projected(view, partId); anchor = view.projectPoint(anchorWorld, partId);
        mouse(view, QEvent::MouseButtonDblClick, anchor, Qt::LeftButton, Qt::LeftButton);
        require(sameProjection(before, projected(view, partId)), "Posed rotation-center selection jumped the scene");
        drag(view, anchor+QPointF(60,0), anchor+QPointF(110,45), Qt::MiddleButton);
        require(pointsNear(anchor, view.projectPoint(anchorWorld, partId)), "Posed rotation center used rest rather than displayed coordinates");
        view.clearPartPoses(); reset();
        log << "Surface rotation center preserved screen position and followed displayed rigid poses\n";

        FemMesh cloud; cloud.nodes = {{2,2,20},{10,5,20},{30,25,20}};
        view.setPickNodes(partId, cloud, {101,205,902});
        view.beginPick(PickKind::Node, true, {partId});
        const auto first = view.projectPoint({0,0,20},partId), last = view.projectPoint({15,10,20},partId);
        before = projected(view, partId);
        drag(view, first, last, Qt::LeftButton, Qt::ControlModifier);
        require(view.pickSelection().size() == 2 && view.pickSelection()[0].nodeId == 101 && view.pickSelection()[1].nodeId == 205,
                "NX navigation broke Ctrl-left rectangle node IDs");
        require(sameProjection(before, projected(view, partId)), "Node rectangle moved the camera");
        click(view, last, Qt::RightButton);
        require(view.pickSelection().size() == 1 && view.pickSelection().front().nodeId == 101,
                "NX right click no longer removes the latest typed pick");
        mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(view.isPicking() && view.pickSelection().size() == 1 && cancelled == 0,
                "Stopping navigation unexpectedly cancelled the typed picker");
        mouse(view, QEvent::MouseButtonRelease, center+QPointF(80,60), Qt::MiddleButton, Qt::NoButton);
        require(sameProjection(before, projected(view, partId)), "Esc left a camera gesture active");
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(!view.isPicking() && cancelled == 1, "Idle Esc no longer cancels the pick session");
        view.beginPick(PickKind::Node, true, {partId});
        mouse(view, QEvent::MouseButtonPress, first, Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseMove, last, Qt::NoButton, Qt::LeftButton, Qt::ControlModifier);
        QFocusEvent lostFocus(QEvent::FocusOut); QApplication::sendEvent(&view, &lostFocus);
        mouse(view, QEvent::MouseButtonRelease, last, Qt::LeftButton, Qt::NoButton, Qt::ControlModifier);
        require(view.isPicking() && view.pickSelection().isEmpty(), "Interrupted rectangle committed stale nodes or ended the collector");
        view.setMouseNavigationMode(OccViewport::MouseNavigationMode::Classic);
        require(view.isPicking(), "Changing mouse mode cancelled an active typed picker");
        view.endPick();
        require(selected == 0, "Camera or typed picking leaked whole-part selection signals");
        log << "NX typed-picker isolation, actual node numbering, right undo, Esc and interrupted rectangle passed\n";

        view.beginPick(PickKind::Face, false, {partId});
        click(view, view.projectPoint(anchorWorld, partId));
        require(view.pickSelection().size() == 1 && view.pickSelection().front().partId == partId &&
                view.pickSelection().front().index > 0, "Navigation changes broke CAD face binding");
        const auto& hit = view.pickSelection().front().hitPosition;
        for (int k = 0; k < 3; ++k) require(std::abs(hit[k]-anchorWorld[k]) < 1e-5,
                "Typed CAD coordinates no longer use the initial global millimetre frame");
        view.endPick();
        click(view, view.projectPoint({20,15,20}, partId));
        require(selected == 1, "Whole-part click stopped working after navigation and picker transitions");
        require(!view.grabFramebuffer().isNull(), "Navigation viewport did not render a real framebuffer");
        log << "CAD face IDs, initial global mm and original whole-part selection passed\n";
        log.flush();
        return 0;
    } catch (const std::exception& exception) {
        log << "FAIL: " << exception.what() << '\n';
        log.flush();
        return 1;
    }
}
