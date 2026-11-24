#pragma once

#include <QMatrix4x4>
#include <QColor>
#include <QOpenGLWidget>
#include <QPointer>
#include <QScopedValueRollback>
#include <QString>
#include <QVector>
#include <QMetaType>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <utility>
#include <vector>

class QMouseEvent;
class QPointF;
class QResizeEvent;
class QWheelEvent;
class QKeyEvent;
class QEvent;
class QFocusEvent;

namespace MotionOcc {

class CadDocument;
class PreparedCadScene;
struct FemMesh;
struct MotionSymbol;

enum class PickKind { Part, Vertex, Edge, Face, Node, MeshEdge, MeshFace, Element, Solid };

// Coordinates are initial global CAD coordinates in millimetres, including
// STEP placement. Topology indices follow TopExp::MapShapes (one based).
// Node index is zero based; nodeId preserves the supplied FEM numbering.
// FEM edge/face/element indices are one-based connectivity ordinals, with
// nodeIds retaining the mesh's actual endpoint/cell connectivity numbering.
struct PickResult {
    PickKind kind = PickKind::Part;
    int partId = -1;
    int index = -1;
    int nodeId = -1;
    QVector<int> nodeIds; // actual connectivity IDs for FEM edges, faces and elements
    std::array<double, 3> position{};
    std::array<double, 3> hitPosition{}; // actual cursor location on CAD, when applicable
    std::array<double, 3> direction{};
    bool hasDirection = false;
    bool isValid() const { return partId > 0; }
};

class OccViewport final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit OccViewport(QWidget* parent = nullptr);
    ~OccViewport() override;

    enum class DisplayMode { ShadedEdges, Shaded, Wireframe, Mesh };
    Q_ENUM(DisplayMode)
    enum class MouseNavigationMode { Classic, Nx };
    Q_ENUM(MouseNavigationMode)
    struct ViewCameraState {
        QVector3D eye, target, up;
        double ortho_zoom = 1.0; // vertical visible span in world millimetres
    };
    ViewCameraState queryCameraState() const;
    bool queryVisibleBounds(QVector3D& minimum,QVector3D& maximum) const;
    void copyCameraFrom(const OccViewport& source);
    void copyPresentationFrom(const OccViewport& source);
    void setPresentationPeers(const QList<OccViewport*>& viewports);

    // Snapshot of visible geometry; does not fit, recenter or reset the view.
    // Retains poses for IDs still in the document, including hidden parts.
    // 纯 CPU 快照可在后台构建。追加时按源文档部件顺序传入最终编号。
    static std::shared_ptr<const PreparedCadScene> prepareCadScene(const CadDocument& document);
    void setScene(const CadDocument& document, const std::map<int, FemMesh>& meshes,
                  const std::set<int>& hidden = {}, const std::shared_ptr<const PreparedCadScene>& prepared = {},
                  const std::vector<int>& preparedPartIds = {});
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
    DisplayMode displayMode() const;
    // Navigation is view-only; changing it keeps the active typed-pick session.
    void setMouseNavigationMode(MouseNavigationMode mode);
    MouseNavigationMode mouseNavigationMode() const;
    QString mouseNavigationHint() const;
    void setPickEnabled(bool enabled);
    // An empty filter means smart CAD picking. Editor sessions temporarily take precedence.
    void setSelectionFilter(std::optional<PickKind> kind);
    std::optional<PickKind> selectionFilter() const;
    const QVector<PickResult>& viewportSelection() const;
    void clearViewportSelection();
    enum class PartPickResolution { ExactCenter, IdOnly };
    void beginPick(PickKind kind, bool multiple = false,
                   const std::set<int>& allowedParts = {}, const QVector<PickResult>& initial = {},
                   PartPickResolution partResolution = PartPickResolution::ExactCenter);
    void endPick();
    bool isPicking() const;
    QVector<PickResult> pickSelection() const;
    void setPickSelection(const QVector<PickResult>& selection,
                          PartPickResolution partResolution = PartPickResolution::ExactCenter);
    void setPickHighlightColor(const QColor& color);
    // Presentation only: cloud plots retain their live typed selection while
    // suppressing active selection, hover and selectable-node feedback.
    void setPickFeedbackVisible(bool visible);
    // Original picker services keep independent inactive highlights and point
    // previews. These layers do not replace the active pick session or symbols.
    void setAuxiliaryPickHighlights(const QString& key, const QVector<PickResult>& picks, const QColor& color);
    void clearAuxiliaryPickHighlights(const QString& key);
    void setAuxiliaryPoints(const QString& key, const QVector<QVector3D>& points, const QColor& color, float pointSize = 10.f);
    void clearAuxiliaryPoints(const QString& key);
    // Replaces this part's selectable node cloud without modifying the FEM mesh.
    void setPickNodes(int partId, const FemMesh& mesh, const QVector<int>& nodeIds = {});
    void setMeshScalars(const std::map<int, std::vector<double>>& values,
                        double minimum, double maximum, const QString& title);
    std::optional<std::pair<double, double>> meshScalarRange() const;
    void clearMeshScalars();
    void setMotionSymbols(const QVector<MotionSymbol>& symbols);
    void setAuxiliaryMotionSymbols(const QString& key, const QVector<MotionSymbol>& symbols);
    void clearAuxiliaryMotionSymbols(const QString& key);
    PickResult pickAt(const QPointF& position) const;
    QPointF projectPoint(const std::array<double, 3>& position, int partId = -1) const;
    void fitAll();
    void fitAllIncluding(const std::optional<std::array<double, 3>>& point);
    void viewIsometric();
    void viewFront();
    void viewBack();
    void viewTop();
    void viewBottom();
    void viewRight();
    void viewLeft();
    // Original display-menu controls use screen axes Sx/Sy/Sz. They never
    // change the document's world coordinates or the active pick collector.
    void rotateView(const QString& screenAxis, double degrees);
    void panView(const QString& direction, double percent);
    void zoomView(bool zoomIn, double factor);
    void stepViewHistory(bool forward);
    void setAutoRotation(const QString& screenAxis, bool enabled);
    bool glReady() const;

signals:
    void cameraChanged();
    void partPicked(int id);
    void graphicsError(const QString& message);
    void pickSelectionChanged(const QVector<MotionOcc::PickResult>& selection);
    void viewportSelectionChanged();
    void pickSessionChanged(bool active);
    void pickCancelled();

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    bool event(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void applyPreparedScene(const std::shared_ptr<const PreparedCadScene>& scene,
                            const std::map<int, FemMesh>& meshes, const std::set<int>& hidden);
    struct State;
    std::unique_ptr<State> m_state;
    QList<QPointer<OccViewport>> m_peers;
    bool m_applyingPeerUpdate = false;
    template<class Operation> void updatePeers(const Operation& apply)
    {
        if (m_applyingPeerUpdate) return;
        for (const auto& peer : m_peers) {
            if (!peer) continue;
            const QScopedValueRollback<bool> updating(peer->m_applyingPeerUpdate, true);
            apply(*peer);
        }
    }

    void cleanupGL();
    void releaseSceneBuffers();
    bool uploadScene();
    bool uploadChangedMeshes();
    int pickPart(const QPointF& position) const;
    PickResult pickAt(const QPointF& position, PickKind kind, const std::set<int>& allowedParts) const;
    void setViewOrientation(const QVector3D& direction, const QVector3D& up);
    void updateCamera();
    void drawCadPickFeedback();
    void drawPickOverlay();
    void drawScalarLegend();
    void drawMotionSymbols();
    void reportGraphicsError(const QString& message);
    void interruptPointerOperation();
    bool setRotationCenterAt(const QPointF& position);
};

} // namespace MotionOcc

Q_DECLARE_METATYPE(MotionOcc::PickResult)
Q_DECLARE_METATYPE(QVector<MotionOcc::PickResult>)
