#include "hvtrans_job.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace altairresultio {

namespace {

std::string systemTempDir()
{
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    if (GetTempPathA(MAX_PATH, buf) != 0)
        return buf;
    return "C:\\Windows\\Temp";
#else
    const char* tmp = getenv("TMPDIR");
    return tmp ? tmp : "/tmp";
#endif
}

// 引号包裹命令（Windows cmd 规则）
std::string quote(const std::string& path)
{
    return "\"" + path + "\"";
}

std::string readFileHead(const std::string& path, size_t maxBytes)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return {};
    std::string data(maxBytes, '\0');
    in.read(&data[0], static_cast<std::streamsize>(maxBytes));
    data.resize(static_cast<size_t>(in.gcount()));
    return data;
}

} // namespace

bool HvTransJob::available(const HvTransOptions& opt)
{
    if (opt.hvTransExe.empty())
        return false;
#ifdef _WIN32
    const DWORD attr = GetFileAttributesA(opt.hvTransExe.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    return access(opt.hvTransExe.c_str(), X_OK) == 0;
#endif
}

bool HvTransJob::convertToH3d(const std::string& asciiPath,
                              const std::string& h3dPath,
                              const HvTransOptions& opt,
                              std::string& error)
{
    if (!available(opt))
    {
        error = "hvtrans not found: " + opt.hvTransExe;
        return false;
    }

    const std::string workDir = !opt.workDir.empty() ? opt.workDir : systemTempDir();
    const std::string logPath = workDir + "/hvtrans_log.txt";

    // P1 命令行模板：hvtrans -ascii <in> -h3d <out>。
    // 实际参数名在 P0 阶段按本机 hvtrans -help 实测修正（见方案文档 §2.1/§8）。
    std::string cmd = quote(opt.hvTransExe)
                    + " -ascii " + quote(asciiPath)
                    + " -h3d " + quote(h3dPath)
                    + " > " + quote(logPath) + " 2>&1";

    const int rc = std::system(cmd.c_str());
    if (rc != 0)
    {
        std::ostringstream ess;
        ess << "hvtrans failed, exit code " << rc;
        const std::string log = readFileHead(logPath, 4096);
        if (!log.empty())
            ess << "\n" << log;
        error = ess.str();
        return false;
    }

    std::ifstream out(h3dPath, std::ios::binary);
    if (!out)
    {
        error = "hvtrans reported success but h3d file not found: " + h3dPath
              + "\nlog: " + readFileHead(logPath, 4096);
        return false;
    }
    return true;
}

} // namespace altairresultio
