#pragma once
#ifdef _WIN32
// MSVC Debug 构建规避：pybind11(detail/common.h 的 PYBIND11_DEBUG_MARKER 逻辑)在
// "链接 release Python 且定义了 _DEBUG"时会临时 #undef _DEBUG 包含 Python.h；
// 本工程 conan py37 的 configure 版 pyconfig.h 第 59 行会在该窗口内 #include <io.h>，
// 导致 corecrt.h 中 _invalid_parameter 的声明被跳过且因 include guard 无法补声明，
// 进而 STL(xmemory/atomic/xlocale)报 C2039/C3861。先于 pybind11 以完整 _DEBUG
// 状态包含 CRT 头即可锁定声明。
#include <io.h>
#endif
// Qt 未定义 QT_NO_KEYWORDS 时把 slots 定义为空宏，会让 Python.h 中
// PyType_Slot *slots 成员声明(object.h)语法报错；包含 Python 前后保存/恢复该宏，
// 保证本头文件在任何"先包含 Qt"的翻译单元中都能正常展开。
#pragma push_macro("slots")
#undef slots
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#pragma pop_macro("slots")

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <numeric>
#include <cctype>
#include <QCoreApplication>
#include "mAppConfiguration.h"
#include <QDir>
#ifdef _WIN32
#include <windows.h>
#endif

namespace py = pybind11;

class EmbeddedPython {
public:
    explicit EmbeddedPython(const std::string& runtime_dir);
    static std::string runtime_path();
private:                
    static std::string _abs_runtime;
};

class MxOptionTest {
    std::string runtime;
    std::string pythonLib;
public:
    MxOptionTest();
    ~MxOptionTest();
    static std::shared_ptr<MxOptionTest> getInstance();

    int OptiomTest(std::string configJson);
};
