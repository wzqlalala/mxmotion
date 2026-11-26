#pragma once

#include "NativeMotionDocument.h"
#include "NativeMotionSolver.h"

#include <QString>
#include <QVector>

namespace MotionOcc {

class CadDocument;

struct MotionDiagnosticItem {
    enum class Severity { Pass, Warning, Error } severity = Severity::Pass;
    QString category;   // "模型" / "质量惯量" / "几何拓扑" / "约束与自由度"
    QString collection; // Optional model tree collection ("bodies", "joints", etc.)
    int entityId = -1;
    QString entityName;
    QString message;
};

struct MotionDiagnosticReport {
    int totalBodies = 0;
    int movingBodies = 0;
    int fixedBodies = 0;
    int flexBodies = 0;
    int activeJoints = 0;
    int activeDrives = 0;
    int activeForces = 0;

    bool hasJacobianAnalysis = false;
    int velocityDofs = 0;
    int constraintEquations = 0;
    int jacobianRank = 0;
    int topologicalDofs = 0;
    int effectiveDofs = 0;
    int redundantEquations = 0;
    double maxInitialViolation = 0.0;

    int errorCount = 0;
    int warningCount = 0;
    QVector<MotionDiagnosticItem> items;
};

MotionDiagnosticReport diagnoseNativeMotion(const CadDocument& cad,
                                            const NativeMotionDocument& document,
                                            int scenarioId = -1);

} // namespace MotionOcc
