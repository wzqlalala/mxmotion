#pragma once

#include "NativeMotionDocument.h"
#include "RigidMotion.h"

#include <memory>

namespace MotionOcc {

class CadDocument;
struct NativeMotionGeometry;

// Immutable solve snapshot. Preparing on the document thread resolves CAD
// bindings and samples topology; solving never touches the live CAD document.
// Native source quantities retain the original mm, kg, s convention.
struct NativeMotionInput {
    NativeMotionDocument document;
    std::vector<PreparedBody> bodies;
    RigidScenario scenario;
    std::shared_ptr<const NativeMotionGeometry> geometry;
};

bool prepareNativeMotion(const CadDocument& cad, const NativeMotionDocument& document,
                         int scenarioId, NativeMotionInput& input, QString& error);

SolveStatus solveNativeMotion(const NativeMotionInput& input, RigidResult& result,
                              QString& error, const SolverCallbacks& callbacks = {});

struct NativeJointViolation {
    int jointId = -1;
    double violation = 0.0;
};

struct NativeConstraintSummary {
    int velocityDofs = 0;
    int constraintEquations = 0;
    int jacobianRank = 0;
    int redundantEquations = 0;
    double maxPositionViolation = 0.0;
    std::vector<NativeJointViolation> jointViolations;
};

bool evaluateNativeConstraintSystem(const NativeMotionInput& input,
                                    NativeConstraintSummary& summary,
                                    QString& error);

// Computes the selected flexible body's actual basis from its mesh, material
// and attachment patches. Does not run a transient solve or mutate the model.
SolveStatus inspectNativeFlexModes(const NativeMotionInput& input, int flexId,
                                  RigidFlexResult& result, QString& error,
                                  const SolverCallbacks& callbacks = {});

} // namespace MotionOcc
