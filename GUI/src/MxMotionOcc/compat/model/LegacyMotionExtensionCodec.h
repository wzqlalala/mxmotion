#pragma once
#include <QJsonObject>
namespace MechModelData { class mModelData; }
namespace MotionOcc {
// Original GUI modules that were persisted in the old session format but
// omitted by its .motion.json exporter. Use named, inspectable JSON fields.
void loadLegacyMotionExtensions(const QJsonObject&,MechModelData::mModelData&);
void saveLegacyMotionExtensions(QJsonObject&,MechModelData::mModelData&);
}
