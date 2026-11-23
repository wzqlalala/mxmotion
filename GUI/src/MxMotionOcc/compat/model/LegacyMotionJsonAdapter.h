#pragma once
#include <QJsonObject>
namespace MechModelData { class mModelData; }
namespace MotionOcc {
// Reuses the original GUI's field and reference conversions. No temporary file
// or worker thread is involved in editing an in-memory model.
class LegacyMotionJsonAdapter {
public:
    static void load(const QJsonObject& motionData, MechModelData::mModelData& model);
    static QJsonObject save(MechModelData::mModelData& model);
};
}
