#pragma once

#include "FemMesh.h"

#include <QMatrix4x4>
#include <QPointF>
#include <QString>
#include <QWidget>

#include <array>
#include <functional>
#include <map>
#include <optional>

namespace MotionOcc {

class OccViewport;
struct RigidFlexResult;

// Dialog-local preview. All positions are in mm and node IDs are from the MNF.
class NativeMnfBindingView final : public QWidget {
public:
    explicit NativeMnfBindingView(const RigidFlexResult& preview, QWidget* parent = nullptr);

    void setPlacement(const QMatrix4x4& mnfLocalToWorld);
    void setActiveJoint(const QString& name, std::optional<std::array<double, 3>> worldPivot,
                        int selectedNodeId = -1);
    void setNodePickedCallback(std::function<void(int)> callback);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    using Point = std::array<double, 3>;

    Point worldPoint(const Point& local) const;
    void updateSymbols();
    void pickNode(const QPointF& point);

    OccViewport* m_viewport = nullptr;
    int m_partId = -1;
    double m_markerSize = 1.0;
    std::map<int, FemMesh> m_restMeshes;
    std::map<int, Point> m_interfaceNodes;
    QMatrix4x4 m_localToWorld;
    QString m_jointName;
    std::optional<Point> m_pivot;
    int m_selectedNodeId = -1;
    std::optional<QPointF> m_clickStart;
    std::function<void(int)> m_nodePicked;
};

} // namespace MotionOcc
