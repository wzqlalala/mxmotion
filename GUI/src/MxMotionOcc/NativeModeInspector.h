#pragma once

#include <functional>
#include <vector>

class QDialog;
class QString;
class QWidget;

namespace MotionOcc {
class CadDocument;
struct NativeMotionDocument;
struct RigidFlexResult;

// Owns a snapshot. Opening/animating a basis cannot alter solver states, the
// model editor, or an already loaded time history.
QDialog* openNativeModeInspector(QWidget* parent,const CadDocument& cad,
                                 const NativeMotionDocument& document,
                                 const std::vector<RigidFlexResult>& results);
QDialog* openImportedMnfModeInspector(QWidget* parent, const RigidFlexResult& result,
                                      const QString& name, std::function<void()> bind = {});
}
