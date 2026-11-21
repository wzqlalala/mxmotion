#pragma once

#include <QString>
#include <TopoDS_Face.hxx>

namespace MotionOcc {

// Add triangulation without replacing the face, its analytic surface, or its
// boundary topology. Call on a private display/solver copy of the CAD shape.
bool ensureOccFaceTriangulation(const TopoDS_Face& face, double deflection, QString& error);

} // namespace MotionOcc
