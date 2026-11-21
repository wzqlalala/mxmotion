#pragma once

#include "NativeMotionSolver.h"

#include <QByteArray>
#include <QStringList>

#include <map>

namespace MotionOcc {

struct NativeTriangleMesh {
    std::vector<double> nodes;
    std::vector<int> triangles;
    std::vector<int> boundaryEdges;
    std::vector<double> boundaryInward;
};

struct NativeCurveGeometry {
    std::vector<double> nodes;
    std::vector<double> samples; // p, unit tangent, curvature vector, ds/du
    bool closed = false;
};

struct NativeJointGeometry {
    NativeTriangleMesh surface;
    NativeCurveGeometry curveI;
    NativeCurveGeometry curveJ;
};

struct NativeFlexGeometry {
    int id = -1;
    int bodyId = -1;
    QJsonObject definition;
    double youngModulus = 0; // mN/mm^2
    double poissonRatio = 0;
    double density = 0; // kg/mm^3
    std::vector<double> nodes;
    std::vector<int> tetrahedra;
    std::vector<std::vector<int>> attachGroups;
    std::vector<double> attachPositions;
    QByteArray sourceBrep; // Snapshot for automatic tetra meshing in worker thread.
    QString meshWorkerPath;
};

struct NativeMotionGeometry {
    QStringList diagnostics;
    std::map<int, NativeTriangleMesh> bodyMeshes;
    std::map<int, NativeJointGeometry> joints;
    std::vector<NativeFlexGeometry> flexBodies;
};

} // namespace MotionOcc
