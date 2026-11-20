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

## Linux / 云端构建与验证

同一套 OCC 界面现在支持 Linux x64；使用仓库锁定的 MotionEngine，并链接真实 OCCT、Netgen 和 Qwt。Windows/MSVC 构建方式保持不变。已验证的 Linux 依赖组合为 Debian 13、GCC 14、Qt 5.15.15、OCCT 7.8.1、Netgen 6.2.2501、Qwt 6.1.4 与锁定版本的 Qt ADS。

Debian 13 安装依赖后执行：

```sh
sudo apt-get install cmake ninja-build g++ qtbase5-dev qtbase5-private-dev libqt5svg5-dev \
    libqt5opengl5-dev libocct-data-exchange-dev libnglib-dev libqwt-qt5-dev \
    libxcb1-dev libx11-dev xvfb xauth libgl1-mesa-dri fonts-noto-cjk
git submodule update --init --recursive
MX_TEST_DESKTOP=ON tools/build-occ.sh
build-occ/bin/MxMotion
```

`motionengine` 子模块需要当前 GitHub 账号对该仓库有读取权限。缺少子模块时构建会报错，不会换成示例数据或虚拟求解器。非系统 SDK 可通过 `CMAKE_PREFIX_PATH` 或 `-DMX_OCCT_ROOT=...`、`-DMX_NETGEN_ROOT=...`、`-DMX_QWT_ROOT=...` 指定。

无桌面的服务器设置 `MX_TEST_DESKTOP=ON` 时，脚本通过 Xvfb 和 Mesa 软件 OpenGL 运行真实窗口测试；测试截图和报告在 `build-occ/desktop-smoke/`。这用于验证界面及动画、曲线交互，不代表服务器软件渲染的性能等同于本机显卡。`MX_BUILD_DIR` 与 `MX_BUILD_JOBS` 可覆盖构建目录和并行度。

Linux 构建仅在构建目录内复制并适配锁定引擎的编译兼容性（MSVC 编译参数、非 Windows 计时器的标准库名称等），具体变更见 `cmake/PrepareMotionEngine.cmake`；不修改子模块源码，也不改变求解模型或算法。

## 操作

- **主页 → 导入几何模型**，或右击模型树的 **部件 → 导入**。支持 STEP、IGES、BREP；长度统一以毫米显示，BREP 文件本身不含单位，本构建按毫米解释。**打开／保存**支持 `.mxmotion`：保存精确 CAD 几何和刚体配置，网格和结果不入项目；不是原生 `.mxcae`。
- **几何 → 实体**：创建立方体（可分别输入三个尺寸）、圆柱体、球体。
- 视口左键拖动旋转、中键或右键平移、滚轮缩放；左键点选部件与模型树同步。**自适应视图**（`F`）用于适配模型。
- 右击树中的具体部件 → **生成网格**，在 **参数** 区选三角形面网格或四面体体网格、设置单元尺寸，然后点 **生成**。四面体需要适合体划分的闭合几何。单次网格上限为 4,000 节点、每种单元 20,000 个；超限时请增大单元尺寸。网格模式显示实际有限元网格；CAD 轮廓/线框使用单独的 OCC 展示数据。部件右键可导出 `.vtk`；体网格导出四面体，面网格导出三角形。网格不会随 CAD 文件保存。
- **求解 → 刚体示例 → 求解**可直接运行一个恒速铰链算例。自己的模型走 **体定义 → 运动体**（选择 CAD 实体，指定质量或密度）→ **运动副与约束**（旋转、移动或固定副）→ **驱动**（恒定速度或从零线性到达末时刻位移）→ **新建求解方案／求解**。几何与线位移用 mm，旋转用 °，时间用 s，重力用 m/s²。求解完成后，由用户选择方案下的「动画／结果导出／运动曲线」。
- **运动曲线**：在参数栏选择源类型、对象、变量分量及名称，点击 **添加曲线**。支持多曲线、颜色、显隐、删除、时间范围与可见曲线 CSV 导出；配置按方案随项目保存。质心位置是世界坐标，位移增量相对第一帧；关节标量明确区分相对转角/行程及其速度。没有结果的通道不会用演示数值代替。
- **动画**：在参数列中使用独立的播放、暂停、停止、倍速与循环控制，滑条对应真实采样帧。停止回到第一帧；关闭动画、选择其他建模内容或切回参数标签时，恢复 CAD 初始位置。只有进入动画并启用对应开关时，曲线时间标记及点击曲线跳转才生效；两个开关可分别控制。
- **可打开的验证项目**：[examples/rigid_results.mxmotion](examples/rigid_results.mxmotion) 包含偏置斜轴旋转与三条已保存曲线请求，操作步骤和应有结果见 [examples/README.md](examples/README.md)。
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
- `tests/MotionOccPlaybackSmoke.cpp`、`MotionOccCurveSmoke.cpp`、`MotionOccPlotSmoke.cpp`：动画坐标和编号、曲线数值语义，以及按方案配置曲线的实际交互回归。
- [原界面逻辑对照与云端验证](docs/lite-occ-restoration-review.md)：修复依据、行为差异、测试范围及完整 SDK 的剩余边界。
