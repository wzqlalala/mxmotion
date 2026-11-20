# MxMotion — OCC / OpenGL / Netgen / MotionEngine

这是 `MxSim.Motion` 的 OCC 构建。它沿用原有 Qt Widgets 标题栏、Ribbon、图标、模型树分组和 Qt ADS 停靠布局。原来的 `GUI/`、`ModelData/` 仍保留；根目录的 CMake 只构建不使用 CGM 的配置，不会尝试加载原版私有 SDK。

## Windows x64 构建和运行

需要 Windows 10/11、MSVC x64（VS 2019/2022 Build Tools，包含 Windows SDK）、CMake 3.21+、Ninja、Qt **5.15** MSVC x64（Widgets、Svg），以及以下 **Release x64** SDK：

- **OpenCASCADE 7.8.1**：指定包含 `include/opencascade`、`lib`、`bin` 的根目录。其 `bin` 需包含 OCCT 的运行依赖 DLL。
- **Netgen 6.2.2505**：用 `USE_OCC=ON` 编译，需 `include/nglib_occ.h`、`lib/nglib.lib` 和 `bin/nglib.dll`（以及依赖 DLL）。网格进程单独运行，不使用 GUI 的 VTK/OSG 或 Chrono。
- **Qt Advanced Docking System 4.4.1**：CMake 默认从[官方仓库](https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System)拉取并锁定提交 `258340842056271ec207f9b112ac5efc45d9b5f5`，构建为独立 DLL；离线构建可把 `MX_QTADS_SOURCE_DIR` 指向该版本的源码目录。

在 **x64 Native Tools Command Prompt** 中运行下列命令。路径以开发机示例为准，可换成自己的 SDK 目录。没有打开 Native Tools 时，也可以设置 `MX_VCVARS64` 为 `vcvars64.bat` 的完整路径。

```bat
git submodule update --init --recursive
set "MX_QT_ROOT=D:\Qt\5.15.2\msvc2019_64"
set "MX_OCCT_ROOT=E:\codex\Motion\output\occt-7.8.1\Library"
set "MX_NETGEN_ROOT=E:\codex\Motion\output\netgen-sdk"
tools\build-occ.cmd
build-occ\bin\MxMotion.exe
```

脚本配置 Release，从锁定的子模块构建 MotionEngine，并下载、校验 Qwt 6.2.0 官方源码，以当前 Qt x64 工具链构建共享库。最终部署 Qt、ADS、OCCT、Netgen、MotionEngine 和 Qwt DLL，并运行 CTest。也可用 `MX_QWT_ROOT` 指定已编译的匹配 Qwt 6.2.0 SDK；用 `MX_MOTIONENGINE_BUILD_DIR` 指定同工具链、同子模块版本的 Release Ninja 构建目录。`MX_BUILD_DIR` 和 `MX_BUILD_JOBS` 可覆盖默认 `build-occ` 和并行度。运行时请保留整个 `bin` 目录（含 `mMainWidgets.rcc`、`platforms/qwindows.dll` 和 `MxMotionMeshWorker.exe`）；另一台机器需预装对应的 Microsoft Visual C++ x64 Redistributable，并有支持桌面 OpenGL 3.3 的显卡驱动。

默认测试不需要桌面。要验证 Qt/ADS、OpenGL、网格、求解与 Qwt，请在可用桌面的会话里设置 `MX_TEST_DESKTOP=ON` 再运行脚本（直接使用 CMake 时设 `-DMX_TEST_DESKTOP=ON`）。测试日志和图片保存在构建目录的 `desktop-smoke/`。不构建桌面版、只测试 CAD/网格/刚体后端可设 `-DMX_BUILD_GUI=OFF`。

## 操作

- **主页 → 导入几何模型**，或右击模型树的 **部件 → 导入**。支持 STEP、IGES、BREP；长度统一以毫米显示，BREP 文件本身不含单位，本构建按毫米解释。**打开／保存**支持 `.mxmotion`：保存精确 CAD 几何和刚体配置，网格和结果不入项目；不是原生 `.mxcae`。
- **几何 → 实体**：创建立方体（可分别输入三个尺寸）、圆柱体、球体。
- 视口左键拖动旋转、中键或右键平移、滚轮缩放；左键点选部件与模型树同步。**自适应视图**（`F`）用于适配模型。
- 右击树中的具体部件 → **生成网格**，在 **参数** 区选三角形面网格或四面体体网格、设置单元尺寸，然后点 **生成**。四面体需要适合体划分的闭合几何。单次网格上限为 4,000 节点、每种单元 20,000 个；超限时请增大单元尺寸。网格模式显示实际有限元网格；CAD 轮廓/线框使用单独的 OCC 展示数据。部件右键可导出 `.vtk`；体网格导出四面体，面网格导出三角形。网格不会随 CAD 文件保存。
- **求解 → 刚体示例 → 求解**可直接运行一个恒速铰链算例。自己的模型走 **体定义 → 运动体**（选择 CAD 实体，指定质量或密度）→ **运动副与约束**（旋转、移动或固定副）→ **驱动**（恒定速度或从零线性到达末时刻位移）→ **新建求解方案／求解**。几何与线位移用 mm，旋转用 °，时间用 s，重力用 m/s²。模型树的「动画／结果导出／运动曲线」分别打开播放控制、写出 CSV 或打开 **Qwt** 曲线；点击曲线可跳转到对应时刻。
- 不带 CGM／专有 CAD 转换、完整草图/特征系统、柔性体、接触、高级约束或原版后处理。原功能区页签仍保留，不可用的命令置灰。

## motionengine 子模块

```bat
git submodule update --init --recursive
```

子模块锁定原 `dev` 对应的提交 `fb0a9d76a9d3c40827928e8d9a9662bb77862579`。根目录的刚体求解链接并调用子模块构建的核心 DLL，不需要 Modal。完整 GUI 的既有接入点在 `GUI/cmake/FindMotionEngine.cmake`：用匹配的工具链将引擎核心及 Modal 模块构建并安装，再用 `-DMotionEngine_SDK_DIR=...` 指向安装目录。完整构建另需 Basic、RebuildGeometry、Render、Mesh 等 SDK 和组织内部的授权配置。含内嵌密钥/凭据的原版授权与登录源码不作为此快照发布物。

## 源码

- `GUI/src/MxMotionOcc/`：在原界面框架中挂接可用命令、渲染与网格进程；直接编译 `GUI/src/Common/` 的原公共组件，沿用 `GUI/resource/`。
- `ModelData/src/MotionOcc/`：OCC 几何、Netgen 网格与 VTK、刚体模型与 MotionEngine 适配、项目与 CSV。CAD／显示／网格实现沿用同机 `Motion` 项目的对应实现思路，未引入供体主窗口或 Chrono。
- `tests/MotionOccBackendSmoke.cpp`：几何读写、错误回退和实际网格的自动测试。
- `tests/MotionOccRigidSmoke.cpp` 和桌面测试：核对 MotionEngine 的真实运动时程以及播放与 Qwt 联动。
