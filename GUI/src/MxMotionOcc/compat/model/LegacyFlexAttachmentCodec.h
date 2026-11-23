#pragma once
#include "NativeMotionDocument.h"
#include "mMeshData.h"
#include <QJsonArray>
#include <QSet>

namespace MotionOcc {
QJsonObject legacyFlexMeshSnapshot(const mMeshData& mesh,const QVector<int>& parts);
QJsonArray legacyFlexGuiAttachmentIds(const QJsonObject& flex,const mMeshData& mesh,const QVector<int>& parts);
// Synchronizes data after the original panel's typed-object merge. Explicit
// reselection is required to acknowledge IDs from a changed/unknown mesh.
void synchronizeLegacyFlexAttachments(const NativeMotionDocument& previous,const NativeMotionDocument& current,
    QJsonObject& data,const QJsonObject& before,const QJsonObject& after,const mMeshData& mesh,
    const QSet<int>& committed,const QSet<int>& confirmed);
}
