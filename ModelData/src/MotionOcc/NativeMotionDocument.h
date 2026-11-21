#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace MotionOcc {

class CadDocument;

// The original .motion.json document is the common editing/solver model.
// Keep its complete root, including unrecognised extension fields; importing
// must not reduce richer joints, loads, sensors or controllers to RigidModel.
// Entity IDs are collection-local, nonnegative; -1 denotes ground in body
// references. Geometry IDs in the source are not OCC part IDs.
struct NativeMotionDocument {
    QJsonObject root;
    QMap<int, QVector<int>> bodyPartBindings;
    QString baseDirectory;

    QJsonArray collection(const QString& name) const;
    QJsonObject entity(const QString& collectionName, int id) const;
};

struct NativeMotionBindingIssue {
    int bodyId = -1;
    QString message;
    QVector<int> candidatePartIds;
};

// Validation never rewrites or substitutes a source value. Omitted legacy
// fields remain omitted and retain the original model's documented defaults.
bool validateNativeMotionDocument(const NativeMotionDocument& document, QStringList& issues);
bool validateNativeMotionDocument(const NativeMotionDocument& document, QString& error);

// Failed reads/parses leave the output unchanged. Writes are atomic. Standalone
// .motion.json export preserves the source root; OCC bindings belong to the
// enclosing .mxmotion project, where the native CAD instances are also stored.
bool parseNativeMotionDocument(const QByteArray& bytes, NativeMotionDocument& out, QString& error,
                               const QString& baseDirectory = {});
bool readNativeMotionDocument(const QString& path, NativeMotionDocument& out, QString& error);
bool writeNativeMotionDocument(const QString& path, const NativeMotionDocument& document, QString& error);

// Automatic binding uses unique original STEP occurrence identities or a
// verified centroid/inertia signature. Explicit source names allow custom mass
// properties to differ from geometry. Optional evidence describes each match.
// Existing explicit bindings are retained; unresolved/ambiguous bodies are
// reported for the UI's part selection rather than silently matched by ID.
bool autoBindNativeMotionBodies(const CadDocument& cad, NativeMotionDocument& document,
                                QVector<NativeMotionBindingIssue>& unresolved,
                                QMap<int, QString>* bindingEvidence = nullptr);
bool setNativeMotionBodyBinding(const CadDocument& cad, NativeMotionDocument& document,
                                int bodyId, const QVector<int>& partIds, QString& error);
bool validateNativeMotionBindings(const CadDocument& cad, const NativeMotionDocument& document, QString& error);

} // namespace MotionOcc
