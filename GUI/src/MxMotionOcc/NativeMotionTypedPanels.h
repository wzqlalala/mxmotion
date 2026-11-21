#pragma once

#include "NativeMotionDocument.h"

#include <QJsonValue>
#include <QSet>
#include <functional>

class QFormLayout;
class QWidget;

namespace MotionOcc {

using NativeFieldReader = std::function<QJsonValue()>;
using NativeFieldFactory = std::function<NativeFieldReader(QFormLayout*, const QString&, const QJsonValue&)>;

// Complete missing editable fields and canonicalise the ignored legacy Angle
// variable. Other existing values and extension keys survive panel edits.
void completeNativeTypedRecord(QJsonObject& row, const QString& collection,
                               const NativeMotionDocument& document);

// The supplied factory is the same one used by the native geometric pickers;
// specialised panels do not introduce a second coordinate or reference model.
QSet<QString> addNativeTypedFields(QWidget* panel, QFormLayout* form,
                                  const QJsonObject& row, const QString& collection,
                                  const NativeMotionDocument& document,
                                  const NativeFieldFactory& createField,
                                  QMap<QString, NativeFieldReader>& fields);

QStringList nativeSensorVariables(const QString& type);
QString nativeSensorQuantityUnit(const QJsonObject& sensor, const NativeMotionDocument& document);
QString nativeTypedRecordIssue(const QJsonObject& row, const QString& collection,
                               const NativeMotionDocument& document);
QString nativeDriveCommandUnit(const QJsonObject& drive);

// Keep nested forms at their actual wrapped height in the original narrow dock.
void fitNativeFormToWidth(QWidget* panel);

} // namespace MotionOcc
