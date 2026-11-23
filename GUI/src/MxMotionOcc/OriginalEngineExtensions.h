#pragma once
#include <QString>
class QWidget;
namespace MotionOcc {
class LegacyMotionContext;
void appendOriginalEngineExtensions(QWidget*,LegacyMotionContext*,const QString& collection,int id);
}
