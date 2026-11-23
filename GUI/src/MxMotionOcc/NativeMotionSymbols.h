#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QString>
#include <QVector>

#include <array>
#include <map>
#include <vector>

namespace MotionOcc {
class CadDocument;
struct NativeMotionDocument;

// Display-only glyphs, separate from CAD topology and all solver inputs.
// Every pair of linePoints is a segment. The helper returns animated world
// coordinates, so partId remains -1 and no second pose is applied by the view.
struct MotionSymbol {
    QString collection;
    int id = -1;
    QString label;
    std::array<double, 3> origin{};
    std::vector<std::array<double, 3>> linePoints;
    QColor color;
    bool emphasized = false;
    int partId = -1;
    // Original GUI spring/arrow symbols contain both line and triangle indices.
    // Each three entries is a face in initial/animated world mm, like linePoints.
    std::vector<std::array<double, 3>> trianglePoints;
    QColor faceColor;
    std::vector<std::pair<QString, std::array<double, 3>>> texts;
    QColor textColor;
    bool drawOrigin = true;
    bool retainOpacity = false;
    bool lineDepthTest = false;
    bool faceDepthTest = false;
};

QVector<MotionSymbol> nativeMotionSymbols(const CadDocument& cad,
    const NativeMotionDocument& document,
    const std::map<int, QMatrix4x4>& partPoses = {},
    const QString& selectedCollection = {}, int selectedId = -1,
    const std::map<int, QMatrix4x4>& bodyPoses = {});

} // namespace MotionOcc
