#pragma once

#include <QString>

namespace MotionOcc {

class CadDocument;

// 仅供已按 SHA-256 核对原始 STEP 的导入流程使用。缓存缺失时可以直接继续导入。
class VerifiedStepCache {
public:
    static bool hasEntry(const QString& sha256);
    static bool read(const QString& sha256, CadDocument& cad);
    static void write(const QString& sha256, const CadDocument& cad);
};

} // namespace MotionOcc
