#pragma once

#include "OccViewport.h"
#include "mPickWidgetData.h"
#include <QColor>
#include <QObject>
#include <QPointer>
#include <map>
#include <memory>
#include <set>

namespace MechMainWindow { class mPickWidgetController; }
namespace MotionOcc {
class CadDocument;
struct FemMesh;
using LegacyPickMap = std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>;

// One per original window/model context, shared by the unmodified original
// point/direction/property widgets. All values are real CAD/FEM references.
class LegacyPickContext final : public QObject {
    Q_OBJECT
public:
    LegacyPickContext(QWidget* owner, OccViewport* viewport, const CadDocument& document,
                      const std::map<int, FemMesh>* meshes = nullptr);
    ~LegacyPickContext() override;
    static LegacyPickContext* forObject(const QObject* object, bool useCurrentFallback = true);
    static LegacyPickContext* current();
    static LegacyPickContext* activeForViewport(const OccViewport& viewport);
    void setMeshes(const std::map<int, FemMesh>* meshes);
    void setNodeIds(int partId, const QVector<int>& ids);
    QVector<int> nodeIds(int partId) const;
    void refreshGeometry();
    OccViewport* viewport() const { return m_viewport; }
    const CadDocument& document() const { return m_document; }

    std::pair<MxFunctions::PickDataType, RenderPickResultData> toLegacy(const PickResult& pick,
                                                                     MxFunctions::PickDataType selectedType);
    PickResult resolve(MxFunctions::PickDataType type, const RenderPickResultData& value) const;
    PickResult resolveGeometry(MxFunctions::PickDataType type, int globalId, int partHint = -1) const;
    QVector<PickResult> resolve(const LegacyPickMap& data) const;
    QVector3D averageCoordinate(const LegacyPickMap& data) const;
    QVector3D direction(const LegacyPickMap& data) const;
    int globalTopologyId(const PickResult& pick) const;
    bool acceptsEdgeLoad(MxFunctions::PickDataType type, const RenderPickResultData& value) const;

    void setHighlightLayer(const QString& key, const QVector<PickResult>& picks, const QColor& color);
    void clearHighlightLayer(const QString& key);
    void setPreviewPoints(const QString& key, const QVector<QVector3D>& points, const QColor& color, float pointSize = 10.f);
    void clearPreviewPoints(const QString& key);
    void begin(MechMainWindow::mPickWidgetController* controller, MxFunctions::PickDataType type,
               bool multiple, const LegacyPickMap& initial, const std::set<int>& allowedParts = {});
    void end(MechMainWindow::mPickWidgetController* controller);
    void updateSelection(MechMainWindow::mPickWidgetController* controller, const LegacyPickMap& data);
    void setPickFeedbackVisible(MechMainWindow::mPickWidgetController* controller, bool visible);
    void setPickType(MxFunctions::PickDataType type);
    MxFunctions::PickDataType pickType() const { return m_type; }
    QSet<MxFunctions::PickDataType> allowedTypes() const;
    static PickKind toKind(MxFunctions::PickDataType type);
signals:
    void allowedTypesChanged();
    void pickTypeChanged(MxFunctions::PickDataType type);
    void selectionRejected(const QString& message);
private:
    void ensureGeometry() const;
    struct GeometryIndex;
    std::unique_ptr<GeometryIndex> m_index;
    QPointer<OccViewport> m_viewport;
    const CadDocument& m_document;
    const std::map<int, FemMesh>* m_meshes;
    std::map<int, QVector<int>> m_nodeIds;
    QPointer<MechMainWindow::mPickWidgetController> m_controller;
    MxFunctions::PickDataType m_type = MxFunctions::PickDataType::GeoPoint;
    std::set<QString> m_layers, m_previews;
    static QPointer<LegacyPickContext> s_current;
};
} // namespace MotionOcc
