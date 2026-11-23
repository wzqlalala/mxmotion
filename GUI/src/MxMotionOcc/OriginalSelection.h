#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVector>
#include <map>
#include <set>

namespace MotionOcc {
class MotionWindow;
class OccViewport;
class CadDocument;
struct FemMesh;
struct PickResult;

struct SelectionInfo {
    QString status;
    QStringList columns;
    QVector<QStringList> rows;
};

SelectionInfo describeSelection(const CadDocument& document,
                                const std::map<int, FemMesh>& meshes,
                                const QVector<PickResult>& picks);

// Original selection-menu semantics, operating on actual OCC topology and
// real FEM connectivity. Results are committed through the original picker
// controller, so its model, counter, highlights and callbacks stay in sync.
bool dispatchOriginalSelection(MotionWindow& window, OccViewport& viewport,
    const CadDocument& document, const std::map<int, FemMesh>& meshes,
    const std::set<int>& hidden, int selectedPart,
    const QString& action, const QVariant& argument, QString& message);
}
