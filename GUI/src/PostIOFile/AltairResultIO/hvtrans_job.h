#pragma once
// HvTrans 转换作业：调用 HyperWorks 自带的 hvtrans 命令行工具，
// 将 Altair Generic ASCII 转换为 H3D。
//
// P1 实现说明：使用 std::system 同步执行 + 日志重定向（保持模块无 Qt 依赖）。
// 局限：无超时/取消能力；RebuildGUI 集成层如需进度与取消，用 QProcess 重写。

#include "miofile_global.h"

#include <string>

namespace altairresultio {

struct HvTransOptions
{
    std::string hvTransExe;            // hvtrans 可执行文件绝对路径
    int timeoutMs = 120000;            // P1 未生效（同步阻塞），预留
    std::string workDir;               // 工作目录（日志与临时文件），空则用系统临时目录
};

class MIOFILE_EXPORT HvTransJob
{
public:
    // 检查 hvtrans 可执行文件是否存在
    static bool available(const HvTransOptions& opt);

    // ascii -> h3d。失败返回 false，error 携带退出码与日志摘要。
    static bool convertToH3d(const std::string& asciiPath,
                             const std::string& h3dPath,
                             const HvTransOptions& opt,
                             std::string& error);
};

} // namespace altairresultio
