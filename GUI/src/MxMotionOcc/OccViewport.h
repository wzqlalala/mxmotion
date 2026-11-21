#pragma once

#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QString>
#include <map>
#include <memory>
#include <set>

class QMouseEvent;
class QPointF;
class QResizeEvent;
class QWheelEvent;

namespace MotionOcc {

class CadDocument;
struct FemMesh;

class OccViewport final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OccViewport(QWidget* parent = nullptr);
    ~OccViewport() override;

    enum class DisplayMode { ShadedEdges, Shaded, Wireframe, Mesh };
    Q_ENUM(DisplayMode)

    // Snapshot of visible geometry; does not fit, recenter or reset the view.
    // Retains poses for IDs still in the document, including hidden parts.
    void setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                  const std::set<int>& hidden = {});
    // Replaces the transient poses for the current document. Rigid matrices map
    // rest CAD world to animated world in mm. Missing or invalid poses mean
    // identity; unknown part IDs are ignored. Does not refit or modify geometry.
    void setPartPoses(const std::map<int, QMatrix4x4>& poses);
    // Real modal node positions in global mm replace the bound CAD only while
    // animating. Other bound parts are suppressed, not removed from the model.
    void setDeformedMeshes(const std::map<int, FemMesh>& meshes, const std::set<int>& suppressed);
    void clearPartPoses();
    void selectPart(int id);
    void setDisplayMode(DisplayMode mode);
    void setPickEnabled(bool enabled);
    void fitAll();
    void viewIsometric();
    void viewFront();
    void viewTop();
    void viewRight();
    bool glReady() const;

signals:
    void partPicked(int id); // -1 when nothing was hit
    void graphicsError(const QString& message);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    struct State;
    std::unique_ptr<State> m_state;

    void cleanupGL();
    void releaseSceneBuffers();
    bool uploadScene();
    bool uploadChangedMeshes();
    int pickPart(const QPointF& position) const;
    void drawAxes();
    void reportGraphicsError(const QString& message);
};

} // namespace MotionOcc
