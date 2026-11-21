#pragma once

#include <QString>

class QJsonObject;
namespace MotionOcc {
class NativeMotionSystem;
struct SolverCallbacks;
struct NativeStaticCancelled {};
struct NativeStaticReport {
    int iterations=0;
    double residual=0,constraintResidual=0;
    QString description() const;
};

// Equilibrate the actual engine state at t=0. Rest lengths, FEA reference
// positions, modal bases and accumulated elastic deformation are never reset.
NativeStaticReport equilibrateNativeMotion(NativeMotionSystem& system,const QJsonObject& scenario,
                                          const SolverCallbacks& callbacks);
}
