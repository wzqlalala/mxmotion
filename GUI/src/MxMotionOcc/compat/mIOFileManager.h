#pragma once
#include <QString>
namespace MechIOFile {
class mIOFileManager {
public:
    static mIOFileManager* getInstance() { static mIOFileManager value;return &value; }
    void slot_exportMotionJsonFile(const QString& path,bool fromPlatform=false);
};
}
