#pragma once
// 内部工具头：UTF-8 路径安全打开（不对外导出）。
//
// 调用方约定路径为 UTF-8（QString::toStdString）。Windows 窄字符文件 API 按 ANSI
// 代码页解释这些字节，中文路径会落成乱码文件名；MSVC/MinGW 的 ofstream 支持
// wchar_t* 打开，这里统一转宽字符。非 Windows 平台直接用窄路径。
#include <fstream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace altairresultio {
namespace detail {

inline std::wstring utf8ToWide(const std::string& utf8)
{
    if (utf8.empty())
        return {};
    const int count = MultiByteToWideChar(CP_UTF8, 0,
        utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring wide(count, L'\0');
    if (count > 0)
        MultiByteToWideChar(CP_UTF8, 0,
            utf8.data(), static_cast<int>(utf8.size()), &wide[0], count);
    return wide;
}

inline std::ofstream openBinaryWrite(const std::string& utf8Path)
{
    return std::ofstream(utf8ToWide(utf8Path), std::ios::binary | std::ios::trunc);
}

inline std::ifstream openBinaryRead(const std::string& utf8Path)
{
    return std::ifstream(utf8ToWide(utf8Path), std::ios::binary);
}

// 二进制整拷贝（UTF-8 路径安全），用于 ASCII staging 的进出搬运
inline bool copyBinaryFile(const std::string& utf8From, const std::string& utf8To)
{
    std::ifstream in = openBinaryRead(utf8From);
    if (!in)
        return false;
    std::ofstream out = openBinaryWrite(utf8To);
    if (!out)
        return false;
    out << in.rdbuf();
    return static_cast<bool>(out);
}

} // namespace detail
} // namespace altairresultio
#else
namespace altairresultio {
namespace detail {

inline std::ofstream openBinaryWrite(const std::string& utf8Path)
{
    return std::ofstream(utf8Path, std::ios::binary | std::ios::trunc);
}

inline std::ifstream openBinaryRead(const std::string& utf8Path)
{
    return std::ifstream(utf8Path, std::ios::binary);
}

// 二进制整拷贝，用于 ASCII staging 的进出搬运（行为与 Windows 分支一致）
inline bool copyBinaryFile(const std::string& utf8From, const std::string& utf8To)
{
    std::ifstream in = openBinaryRead(utf8From);
    if (!in)
        return false;
    std::ofstream out = openBinaryWrite(utf8To);
    if (!out)
        return false;
    out << in.rdbuf();
    return static_cast<bool>(out);
}

} // namespace detail
} // namespace altairresultio
#endif
