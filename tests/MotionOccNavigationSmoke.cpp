#include "OccViewport.h"
#include "CadDocument.h"
#include "FemMesh.h"
#include "navigation/ViewportOrientationTriad.h"

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
#include <QDoubleSpinBox>
#include <QImage>
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

        // Original Display-tab controls act in screen axes and must retain
        // exact camera history, even after a back/forward/branch sequence.
        auto originalProjection=projected(view,partId);
        view.rotateView("+Sz",90);
        const auto rotatedProjection=projected(view,partId);
        for(std::size_t i=0;i<originalProjection.size();++i) {
            const auto delta=originalProjection[i]-center;
            require(pointsNear(rotatedProjection[i],center+QPointF(delta.y(),-delta.x()),0.01),
                    "Original Sz rotation was not around the screen normal");
        }
        view.panView("right",20);
        const auto pannedProjection=projected(view,partId);
        translated(rotatedProjection,pannedProjection,{view.width()*0.1,0},
                   "Original pan percentage does not match the uploaded control");
        view.stepViewHistory(false);
        require(sameProjection(rotatedProjection,projected(view,partId)),"View-history back did not restore the camera");
        view.stepViewHistory(true);
        require(sameProjection(pannedProjection,projected(view,partId)),"View-history forward did not restore the camera");
        view.stepViewHistory(false);
        const auto camera=view.queryCameraState();
        view.zoomView(true,2);
        require(std::abs(view.queryCameraState().ortho_zoom-camera.ortho_zoom/2)<1e-5,
                "Original zoom factor did not update the actual orthographic camera");
        const auto branchProjection=projected(view,partId);
        view.stepViewHistory(true);
        require(sameProjection(branchProjection,projected(view,partId)),"Camera history retained an invalid redo branch");
        QVector3D boundsMin,boundsMax;
        require(view.queryVisibleBounds(boundsMin,boundsMax) && boundsMin.length()<1e-4
                && (boundsMax-QVector3D(40,30,20)).length()<1e-4,"Visible bounds did not describe actual CAD");
        log<<"Original screen-axis rotation, percentage pan, zoom, camera history and visible bounds passed\n";
        reset();

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

        // Verify default model bounding-box rotation center, view center, custom interactive pick,
        // and Classic left-button OrbitSpace / OrbitPlane / Pan modes.
        reset();
        const auto pointNear3D = [](const Point& a, const Point& b, double tol = 1e-3) {
            return std::hypot(std::hypot(a[0] - b[0], a[1] - b[1]), a[2] - b[2]) < tol;
        };
        require(view.rotationCenterMode() == OccViewport::RotationCenterMode::ModelCenter &&
                pointNear3D(view.rotationCenter(), {20.0, 15.0, 10.0}),
                "Default rotation center must equal the model bounding-box center");
        const QPointF modelCenterBeforeOrbit = view.projectPoint({20.0, 15.0, 10.0});
        drag(view, center + QPointF(-60, -25), center + QPointF(55, 42), Qt::LeftButton);
        require(pointsNear(modelCenterBeforeOrbit, view.projectPoint({20.0, 15.0, 10.0}), 0.01),
                "Classic orbit did not pivot around the model bounding-box center");

        // Adding a translated part updates the default rotation center to the combined model AABB center
        // automatically on setScene without jumping the existing view.
        {
            CadDocument twoPartCad;
            int firstId = -1, secondId = -1;
            require(twoPartCad.addPrimitive(Primitive::Box, 40, 30, 20, firstId, error) &&
                    twoPartCad.addPrimitive(Primitive::Box, 40, 30, 20, secondId, error), error);
            QMatrix4x4 secondPose;
            secondPose.translate(60.0f, 0.0f, 0.0f);
            reset();
            const auto projBeforeTwoPart = projected(view, partId);
            view.setScene(twoPartCad, {});
            view.setPartPoses({{secondId, secondPose}});
            require(view.rotationCenterMode() == OccViewport::RotationCenterMode::ModelCenter &&
                    pointNear3D(view.rotationCenter(), {50.0, 15.0, 10.0}),
                    "Default rotation center did not follow the whole-model bounding-box center");
            require(sameProjection(projBeforeTwoPart, projected(view, firstId)),
                    "Updating default model-bounds pivot jumped the displayed scene");
            const QPointF combinedPivotScreen = view.projectPoint({50.0, 15.0, 10.0});
            drag(view, center, center + QPointF(64, 38), Qt::LeftButton);
            require(pointsNear(combinedPivotScreen, view.projectPoint({50.0, 15.0, 10.0}), 0.01),
                    "Orbit did not use the updated whole-model bounding-box center");
            view.clearPartPoses();
            view.setScene(cad, {});
            reset();
        }

        // View-center rotation pivot: pan off-center, set pivot to view center without scene jump, then orbit.
        drag(view, center, center + QPointF(92, -48), Qt::MiddleButton);
        before = projected(view, partId);
        view.setRotationCenterToView();
        require(view.rotationCenterMode() == OccViewport::RotationCenterMode::ViewCenter &&
                sameProjection(before, projected(view, partId)) &&
                pointsNear(view.projectPoint(view.rotationCenter()), center, 0.02),
                "Setting view rotation center jumped the scene or missed viewport center");
        drag(view, center + QPointF(70, 10), center + QPointF(-20, 65), Qt::LeftButton);
        require(pointsNear(view.projectPoint(view.rotationCenter()), center, 0.02) &&
                !sameProjection(before, projected(view, partId)),
                "Orbit after setRotationCenterToView did not pivot around the viewport center");

        // Custom interactive rotation-center pick mode + Esc isolation from typed picker.
        reset();
        view.beginPick(PickKind::Face, false, {partId});
        view.beginPickRotationCenter();
        require(view.isPickingRotationCenter() && view.isPicking(),
                "Custom rotation-center pick unexpectedly cancelled an active typed picker");
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(!view.isPickingRotationCenter() && view.isPicking() && cancelled == 0,
                "Esc during custom rotation-center pick did not preserve the active typed picker");
        view.endPick();

        const Point cornerVertex{40.0, 30.0, 20.0};
        const QPointF cornerScreen = view.projectPoint(cornerVertex, partId);
        before = projected(view, partId);
        view.beginPickRotationCenter();
        mouse(view, QEvent::MouseMove, cornerScreen, Qt::NoButton, Qt::NoButton);
        require(!view.grabFramebuffer().isNull(), "Rotation-center hover overlay failed to render");
        click(view, cornerScreen, Qt::LeftButton);
        require(!view.isPickingRotationCenter() && selected == 0 &&
                view.rotationCenterMode() == OccViewport::RotationCenterMode::CustomPoint &&
                pointNear3D(view.rotationCenter(), cornerVertex) &&
                sameProjection(before, projected(view, partId)),
                "Interactive custom rotation-center pick did not snap cleanly without scene jump");
        drag(view, center, center + QPointF(-75, 48), Qt::LeftButton);
        require(pointsNear(cornerScreen, view.projectPoint(cornerVertex, partId), 0.02) &&
                !sameProjection(before, projected(view, partId)),
                "Classic left-drag orbit did not pivot around the picked custom vertex");

        // Restore model bounding-box rotation center without jumping the current camera view.
        before = projected(view, partId);
        view.setRotationCenterToModel();
        require(view.rotationCenterMode() == OccViewport::RotationCenterMode::ModelCenter &&
                pointNear3D(view.rotationCenter(), {20.0, 15.0, 10.0}) &&
                sameProjection(before, projected(view, partId)),
                "Restoring model rotation center jumped the scene or missed bounds center");

        // Left-button OrbitPlane (screen-plane roll around pivot) and Pan modes.
        reset();
        view.setLeftButtonMode(OccViewport::LeftButtonMode::OrbitPlane);
        before = projected(view, partId);
        drag(view, center + QPointF(120, 0), center + QPointF(0, -120), Qt::LeftButton);
        const auto rolled = projected(view, partId);
        for (std::size_t i = 0; i < before.size(); ++i) {
            const QPointF d = before[i] - center;
            require(pointsNear(rolled[i], center + QPointF(d.y(), -d.x()), 0.5),
                    "Left-button OrbitPlane did not roll in the screen plane around the pivot");
        }
        before = projected(view, partId);
        drag(view, center, center + QPointF(44, -28), Qt::MiddleButton);
        translated(before, projected(view, partId), {44, -28}, "Middle-button pan failed in OrbitPlane mode");

        reset();
        view.setLeftButtonMode(OccViewport::LeftButtonMode::Pan);
        before = projected(view, partId);
        drag(view, center, center + QPointF(-38, 29), Qt::LeftButton);
        translated(before, projected(view, partId), {-38, 29}, "Left-button Pan mode did not translate cleanly");
        require(selected == 0, "Left-button Pan drag unexpectedly selected a part");
        view.setLeftButtonMode(OccViewport::LeftButtonMode::OrbitSpace);
        reset();
        log << "Model/view/custom rotation centers and left-button OrbitSpace/OrbitPlane/Pan modes passed\n";

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
        drag(view, first, last, Qt::LeftButton, Qt::ShiftModifier);
        require(view.pickSelection().size() == 2 && view.pickSelection()[0].nodeId == 101 && view.pickSelection()[1].nodeId == 205,
                "NX navigation broke Shift-left rectangle node IDs");
        require(sameProjection(before, projected(view, partId)), "Node rectangle moved the camera");
        mouse(view, QEvent::MouseButtonPress, view.projectPoint(cloud.nodes.front(), partId), Qt::RightButton, Qt::RightButton, Qt::ControlModifier);
        mouse(view, QEvent::MouseButtonRelease, view.projectPoint(cloud.nodes.front(), partId), Qt::RightButton, Qt::NoButton, Qt::ControlModifier);
        require(view.pickSelection().size() == 1 && view.pickSelection().front().nodeId == 205,
                "Ctrl-right click did not remove the targeted node");
        mouse(view, QEvent::MouseButtonPress, center, Qt::MiddleButton, Qt::MiddleButton);
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(view.isPicking() && view.pickSelection().size() == 1 && cancelled == 0,
                "Stopping navigation unexpectedly cancelled the typed picker");
        mouse(view, QEvent::MouseButtonRelease, center+QPointF(80,60), Qt::MiddleButton, Qt::NoButton);
        require(sameProjection(before, projected(view, partId)), "Esc left a camera gesture active");
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(!view.isPicking() && cancelled == 1, "Idle Esc no longer cancels the pick session");
        view.beginPick(PickKind::Node, true, {partId});
        mouse(view, QEvent::MouseButtonPress, first, Qt::LeftButton, Qt::LeftButton, Qt::ShiftModifier);
        mouse(view, QEvent::MouseMove, last, Qt::NoButton, Qt::LeftButton, Qt::ShiftModifier);
        QFocusEvent lostFocus(QEvent::FocusOut); QApplication::sendEvent(&view, &lostFocus);
        mouse(view, QEvent::MouseButtonRelease, last, Qt::LeftButton, Qt::NoButton, Qt::ShiftModifier);
        require(view.isPicking() && view.pickSelection().isEmpty(), "Interrupted rectangle committed stale nodes or ended the collector");
        view.setMouseNavigationMode(OccViewport::MouseNavigationMode::Classic);
        require(view.isPicking(), "Changing mouse mode cancelled an active typed picker");
        view.endPick();
        require(selected == 0, "Camera or typed picking leaked whole-part selection signals");
        log << "NX typed-picker isolation, actual node numbering, right subtract, Esc and interrupted rectangle passed\n";

        view.beginPick(PickKind::Face, false, {partId});
        click(view, view.projectPoint(anchorWorld, partId));
        require(view.pickSelection().size() == 1 && view.pickSelection().front().partId == partId &&
                view.pickSelection().front().index > 0, "Navigation changes broke CAD face binding");
        const auto& hit = view.pickSelection().front().hitPosition;
        for (int k = 0; k < 3; ++k) require(std::abs(hit[k]-anchorWorld[k]) < 1e-5,
                "Typed CAD coordinates no longer use the initial global millimetre frame");
        click(view, view.projectPoint(anchorWorld, partId), Qt::RightButton);
        require(view.pickSelection().isEmpty(), "Single-face picker no longer clears on right click");
        view.endPick();
        click(view, view.projectPoint({20,15,20}, partId));
        require(selected == 1, "Whole-part click stopped working after navigation and picker transitions");
        require(!view.grabFramebuffer().isNull(), "Navigation viewport did not render a real framebuffer");
        log << "CAD face IDs, initial global mm and original whole-part selection passed\n";

        // Exercise the actual Motion HUD and its input filter, through public
        // viewport events. It must never masquerade as a model reference.
        auto* hud = dynamic_cast<ViewportOrientationTriad*>(view.findChild<QWidget*>(QStringLiteral("viewportOrientationTriad")));
        require(hud && hud->width() >= 120 && hud->x() == 12 &&
                hud->y()+hud->height()+12 == view.height() &&
                hud->testAttribute(Qt::WA_TransparentForMouseEvents),
                "Motion's transparent, DPI-aware lower-left triad was not installed");
        QFont triadFont(MxFunctions::UiMetrics::uiFontFamily()); triadFont.setStyleHint(QFont::SansSerif);
        if (hud->font().pointSizeF() > 0.0)
            triadFont.setPointSizeF(std::max(7.5, hud->font().pointSizeF()*.85));
        else
            triadFont.setPixelSize(std::max(10, int(std::lround(std::max(12, hud->font().pixelSize())*.85))));
        const auto textHeight = QFontMetricsF(triadFont).height();
        view.viewIsometric(); app.processEvents();
        require(view.grab().save(output.filePath(QStringLiteral("motion-triad-isometric.png"))),
                "Could not capture the real Motion triad");
        const auto imageBeforePan = hud->grab().toImage();
        drag(view, center, center+QPointF(40,-30), Qt::MiddleButton);
        app.processEvents();
        require(imageBeforePan == hud->grab().toImage(), "Triad changed under camera pan");
        QWheelEvent hudWheel(center, view.mapToGlobal(center.toPoint()), {}, {0,120},
                             Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(&view, &hudWheel); app.processEvents();
        require(imageBeforePan == hud->grab().toImage(), "Triad changed under camera zoom");

        view.viewTop(); app.processEvents();
        const auto expectedTop = projected(view, partId);
        view.viewIsometric(); app.processEvents();
        QMatrix4x4 isoMatrix; isoMatrix.lookAt({1,-1,1}, {}, {0,0,1});
        const auto iso = QQuaternion::fromRotationMatrix(isoMatrix.normalMatrix()).conjugated().normalized();
        const auto faces = OrientationTriadFaces(iso, hud->size());
        const auto topFace = std::find_if(faces.begin(), faces.end(), [](const auto& face) {
            return face.hit.axis == 2 && face.hit.sign == 1;
        });
        require(topFace != faces.end(), "Isometric top face not available for navigation");
        QPointF faceCenter;
        for (const auto& point : topFace->polygon) faceCenter += point;
        faceCenter /= topFace->polygon.size();
        const int selectionsBeforeHud = selected;
        click(view, QPointF(hud->pos())+faceCenter); app.processEvents();
        require(sameProjection(expectedTop, projected(view, partId)) && selected == selectionsBeforeHud,
                "Triad cube-face click did not set the correct camera view, or selected model geometry");

        const auto labels = OrientationTriadLabelCenters(OrientationTriadAxes(QQuaternion()), hud->size(), textHeight);
        const auto xHandle = QPointF(hud->pos()) + labels[0];
        click(view, xHandle); app.processEvents();
        auto* angle = view.findChild<QDoubleSpinBox*>(QStringLiteral("orientationTriadAngle"));
        require(angle && angle->isVisible() && hud->Interaction().SelectedAxis() == 0,
                "Motion XYZ handle did not open its existing relative-angle editor");
        angle->setValue(45);
        before = projected(view, partId);
        QKeyEvent applyAngle(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(angle, &applyAngle); app.processEvents();
        require(!sameProjection(before, projected(view, partId)) && selected == selectionsBeforeHud,
                "Triad relative angle did not rotate the view only");
        key(view, QEvent::KeyPress, Qt::Key_Escape); app.processEvents();
        require(!angle->isVisible(), "Esc did not close the triad angle editor");

        view.viewTop(); app.processEvents(); before = projected(view, partId);
        mouse(view, QEvent::MouseButtonPress, xHandle, Qt::MiddleButton, Qt::MiddleButton);
        mouse(view, QEvent::MouseMove, xHandle+QPointF(0,70), Qt::NoButton, Qt::MiddleButton);
        require(hud->Interaction().IsDragging() && !sameProjection(before, projected(view, partId)),
                "Motion middle-button axis drag did not rotate about its world axis");
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(sameProjection(before, projected(view, partId)) && !hud->Interaction().IsDragging(),
                "Esc did not restore the start orientation of the triad axis drag");
        mouse(view, QEvent::MouseButtonRelease, xHandle+QPointF(0,70), Qt::MiddleButton, Qt::NoButton);
        require(sameProjection(before, projected(view, partId)), "Cancelled triad release changed camera orientation");

        view.beginPick(PickKind::Node, true, {partId});
        PickResult saved; saved.kind=PickKind::Node; saved.partId=partId; saved.index=0; saved.nodeId=101;
        saved.position=saved.hitPosition={2,2,20};
        view.setPickSelection({saved});
        const int cancellationsBeforeHud = cancelled;
        click(view, xHandle); app.processEvents();
        QKeyEvent overrideEscape(QEvent::ShortcutOverride, Qt::Key_Escape, Qt::NoModifier);
        QApplication::sendEvent(&view, &overrideEscape);
        require(overrideEscape.isAccepted(), "Triad Esc escaped into the enclosing window shortcut");
        key(view, QEvent::KeyPress, Qt::Key_Escape);
        require(view.isPicking() && view.pickSelection().size()==1 &&
                view.pickSelection().front().nodeId==101 && cancelled==cancellationsBeforeHud,
                "Triad interaction changed/cancelled the accepted typed pick session");
        view.endPick();
        const auto emptyHudCorner = QPointF(hud->pos()) + QPointF(2, 2);
        click(view, emptyHudCorner);
        require(selected == selectionsBeforeHud, "An empty viewport click emitted a whole-part pick");
        before = projected(view, partId);
        drag(view, emptyHudCorner, emptyHudCorner + QPointF(42, -24), Qt::MiddleButton);
        require(!sameProjection(before, projected(view, partId)), "Empty triad rectangle swallowed viewport navigation");
        view.viewIsometric(); app.processEvents();
        const int oldSide = hud->width();
        auto enlarged = hud->font(); enlarged.setPointSizeF(20); hud->setFont(enlarged);
        app.processEvents();
        require(hud->width() > oldSide && hud->y()+hud->height()+12 == view.height(),
                "Triad failed to follow larger UI fonts at the lower-left corner");
        log << "Motion triad source geometry, pan/zoom independence, face navigation, axis angle/drag, Esc, picker isolation and font scaling passed; DPR="
            << view.devicePixelRatioF() << '\n';
        log.flush();
        return 0;
    } catch (const std::exception& exception) {
        log << "FAIL: " << exception.what() << '\n';
        log.flush();
        return 1;
    }
}
