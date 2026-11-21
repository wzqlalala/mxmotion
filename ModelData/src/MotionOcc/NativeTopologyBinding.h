#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace MotionOcc {

class CadDocument;
struct NativeMotionDocument;

struct NativeTopologyBindingIssue {
    int jointId = -1;
    QString selectorField;
    QString message;
};

// Recover only a unique, exact geometric association for an original single
// edge/face constraint. Existing explicit OCC selectors are never overwritten.
// The original topology IDs remain intact; this adds separate OCC references
// to document.root and reports any ambiguous or unverifiable selector.
bool autoBindNativeMotionTopology(const CadDocument& cad, NativeMotionDocument& document,
                                  QVector<NativeTopologyBindingIssue>& unresolved,
                                  QStringList* bindingEvidence = nullptr);

} // namespace MotionOcc
