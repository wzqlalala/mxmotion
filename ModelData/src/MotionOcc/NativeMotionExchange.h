#pragma once

#include <QString>

namespace MotionOcc {

class CadDocument;
struct NativeMotionDocument;

// Resolve a companion STEP after reading a .motion.json document. Files without
// an explicit STEP reference retain the legacy import path. A failed read does
// not replace the supplied CAD or motion document.
bool readNativeMotionExchange(const QString& motionPath, NativeMotionDocument& document,
                              CadDocument& cad, bool& hasGeometry, QString& error);

// Export motion data and a named STEP assembly that can be moved together.
bool writeNativeMotionExchange(const QString& motionPath, const CadDocument& cad,
                               const NativeMotionDocument& document, QString& error);

} // namespace MotionOcc
