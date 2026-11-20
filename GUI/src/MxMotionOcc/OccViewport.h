#pragma once

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
    void setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                  const std::set<int>& hidden = {});
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
    int pickPart(const QPointF& position) const;
    void drawAxes();
    void reportGraphicsError(const QString& message);
};

} // namespace MotionOcc
