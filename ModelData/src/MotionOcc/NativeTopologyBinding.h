#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QJsonObject>
#include <QSet>
#include <TopAbs_ShapeEnum.hxx>

namespace MotionOcc {

class CadDocument;
struct NativeMotionDocument;

struct NativeTopologyBindingIssue {
    int jointId = -1;
    QString selectorField;
    QString message;
};

// Store an OCC reference together with the exact placed geometry's fingerprint.
// The fingerprint allows a later import to recover identity after enumeration
// changes; partId/index alone are only valid inside their original CAD document.
QJsonObject nativeTopologyReference(const CadDocument& cad, int partId, int index,
                                    TopAbs_ShapeEnum kind);

// Serialize selected edge/face identity without using a kernel's enumeration.
// Resolve only unambiguous matches in the same project part on read. The root
// and referenced part set are updated only after all selections are processed.
bool encodeNativeProjectTopology(const CadDocument& cad, QJsonObject& root,
                                 QString& error, QSet<int>* referencedParts = nullptr);
bool decodeNativeProjectTopology(const CadDocument& cad, QJsonObject& root, QString& error);

// Recover legacy references from validated import topology or unique geometry.
// Curves may be standalone construction geometry, as in mMotionSolverThread;
// their sampled world geometry is attached to the indicated body's frame.
// Original source IDs are retained. Saved geometric fingerprints take priority
// over current import ordinals, and ambiguous matches are reported explicitly.
bool autoBindNativeMotionTopology(const CadDocument& cad, NativeMotionDocument& document,
                                  QVector<NativeTopologyBindingIssue>& unresolved,
                                  QStringList* bindingEvidence = nullptr);

} // namespace MotionOcc
