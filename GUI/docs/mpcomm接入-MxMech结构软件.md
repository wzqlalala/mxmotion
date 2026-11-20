# MxMech（结构软件）mpcomm 平台接入文档

> 本文档记录结构软件（MxMech / RebuildGUI）作为"节点软件"接入多物理平台（mpcomm 协议）的实现、单元格映射、信号接线与已踩过的坑。
> **后期改动接入相关代码时，请同步更新本文档。**

---

## 1. 总体架构

```
平台 (PlatformSession)
   │  拉起 MechLauncher.exe + 4 个 --mpcomm-* 参数 + TCP 握手 + 心跳
   ▼
MechLauncher（透明启动器，按 Config/launcher.config 配 PATH，原样透传参数）
   ▼
MxMech.exe（SoftwareAdapter("MxMech")，main.cpp 接入）
   │  on_command 回调（专用命令线程）
   ▼
MxMech::MpcommBridge（src/MxMech/mMpcommBridge.{h,cpp}）
   │  QMetaObject::invokeMethod(QueuedConnection) 投递主线程
   ▼
mMenuWidget / mStartSolve（导入、建工况、求解）→ 经 mMechGlobalSignals 回报桥接
```

关键文件：

| 文件 | 作用 |
|---|---|
| `src/MxMech/mMpcommBridge.{h,cpp}` | 桥接核心：命令分发、context 解析、opened/committed/changed/cleared 三件套、revision 自增 |
| `src/MxMech/main.cpp` | main 里接 SoftwareAdapter + `_isPlatformMode` 标志 |
| `src/Common/mMechGlobalSignals.h` | mpcomm 全局信号组（回报/请求/清空/平台模式标志） |
| `src/MainWindow/mMenuWidget.{h,cpp}` | 平台触发导入槽、templateId 建工况、几何/计算文件导入回报、删模型清空、单模型拦截 |
| `src/MainWindow/mStartSolve.cpp` | 求解状态监控（started/finished/failed 埋点）+ 结果文件收集 |
| `src/MainWindow/mTreeView.cpp` | 模型树"清空"按钮 → 通知平台清空单元格 |
| 顶层 `CMakeLists.txt` / `src/MxMech/CMakeLists.txt` | mpcomm / FieldTransferCore / FieldTransferMapping 三个 Basic 包的接入与链接 |

## 2. CMake 接入

三个包都来自 `..\Basic\` 的 install 树，与其它 Basic 基础模块同构（install/{include,lib,bin} + cmake 包配置）：

```cmake
# 顶层 CMakeLists.txt（CMakeLists.txt:184-209）
# 独立开发环境：自动探测相对路径；自动构建系统已通过 CMAKE_PREFIX_PATH 注入。
if(NOT DEFINED mpcomm_INSTALL_DIR)
    set(mpcomm_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/mpcomm/install")
endif()
if(EXISTS "${mpcomm_INSTALL_DIR}" AND NOT "${mpcomm_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${mpcomm_INSTALL_DIR}")
endif()
find_package(mpcomm 1.0.0 EXACT CONFIG REQUIRED)

# FieldTransferCore / FieldTransferMapping（场数据传递，Basic 基础模块）
if(NOT DEFINED fieldtransfercore_INSTALL_DIR)
    set(fieldtransfercore_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/fieldtransfercore/install")
endif()
if(EXISTS "${fieldtransfercore_INSTALL_DIR}" AND NOT "${fieldtransfercore_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${fieldtransfercore_INSTALL_DIR}")
endif()
find_package(FieldTransferCore 0.1.0 EXACT CONFIG REQUIRED)

if(NOT DEFINED fieldtransfermapping_INSTALL_DIR)
    set(fieldtransfermapping_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/fieldtransfermapping/install")
endif()
if(EXISTS "${fieldtransfermapping_INSTALL_DIR}" AND NOT "${fieldtransfermapping_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${fieldtransfermapping_INSTALL_DIR}")
endif()
find_package(FieldTransferMapping 0.1.0 EXACT CONFIG REQUIRED)
```

每个包都是同一模式：`INSTALL_DIR` 缓存变量（preset/自动构建按 `<pkg>_INSTALL_DIR` 约定注入）优先，缺省时回退到相对路径 `../../../Basic/<模块>/install`，追加进 `CMAKE_PREFIX_PATH` 后 `find_package(... CONFIG REQUIRED)`。三个都是静态库，`ws2_32`/`Threads` 由包配置传递。

`src/MxMech/CMakeLists.txt` 的 `_runtimes_libraries` 增加链接：

```cmake
list(APPEND _runtimes_libraries
    Qt5::Core Qt5::Gui Qt5::Widgets  MxFunctions MxPython
    Qt5::Network Qt5::WebSockets Qt5::Sql  Qt5::WebEngine Qt5::WebEngineWidgets ssl crypto
    mpcomm::mpcomm
    FieldTransferCore::FieldTransferCore
    FieldTransferMapping::FieldTransferMapping
    ${OpenCASCADE_LIBRARIES}
)
```

## 3. main.cpp 接入（src/MxMech/main.cpp）

`QApplication app(argc, argv)` 之后立即：

```cpp
MxMech::MpcommBridge mpcommBridge;
mpcomm::simple::SoftwareAdapter mpcommAdapter("MxMech");
mpcommAdapter.on_command(&MxMech::handleMpcommCommand, &mpcommBridge);
const bool mpcommStarted = mpcommAdapter.start(argc, argv);
if (!mpcommStarted && mpcommAdapter.platform_mode()) {
    return 2;  // 平台模式下手失败必须退出
}
mpcommBridge.setAdapter(&mpcommAdapter);
MechCommon::mMechGlobalSignals::getInstance()->_isPlatformMode =
    mpcommAdapter.platform_mode();
```

独立启动（无 `--mpcomm-*` 参数）时 `start()` 为 no-op，不影响正常使用。

## 4. 单元格（cell）映射

| 角色 | 结构软件语义 | changes 字段 |
|---|---|---|
| `geometry` | 几何文件（step/stp/iges/stl…） | `files: [{path}]` |
| `mesh` | **计算文件（.mx）** | `path: "<字符串>"`（单 path，非数组） |
| `setup` | **同一个 .mx**（与 mesh 同文件挂两个角色） | `projectFiles: [{path}]` |
| `solution` | 求解结果文件 | `resultFiles: [{id, path}]` |
| `results` | 后处理结果文件（同 solution） | `resultFiles: [{id, path}]` |

**mpif（上游场数据，输入角色）**：其他场（热/流体/电磁）共享关联过来的场数据文件，经 `connectionInputs`/`data.files` 里 `role=mpif` 传入。**只收不报**——它不在 editableCells 五个角色里，平台 schema 也没有它的 changes 字段；结构侧接收后只做处理，不发 committed（`results.mpifFiles` 是**写出**方向，后续耦合时才有）。`readContext` 收进 `mpifPaths_`（可多个），OPEN_NODE 挂 `mpcomm_pending_mpif`；REFRESH 时**不依赖 `changed_cells` 标记**，只要 context 里解析到了 mpif 文件就重导（连线目标是"设置"单元格，平台 `changed_cells` 只会给 `setup`），处理动作是 `mMenuWidget::runMpifMappingDemo`（见第 6 节信号表）。

**两条平台 schema 硬约束（都被拒过）**：
- mesh 只接受 `path` 字符串，用 `files` 数组会被拒；
- `resultFiles` **含多个文件时每项必须提供唯一 `id`**（`changes.solution.resultFiles 含多个文件时每项都必须提供唯一 id`）。桥接按 `file-0/file-1/...` 生成（`onSolverStatusChanged` 和 `markSharedInputsCommitted` 两处都要带）。

## 5. 分析类型判定（templateId → CaseType）

OPEN_NODE 的 `data.context.node.templateId` 判定分析类型。桥接 `readContext` 解析存入 `templateId_`，`onOpenNode` 挂到 `qApp->setProperty("mpcomm_pending_templateId")`；`mMenuWidget` 构造后取出，经 `slot_mpcommCreateCaseByTemplateId` 映射建工况：

| templateId | CaseType |
|---|---|
| `analysis.static_structural` | `ImplicitLinearStatic` |
| `analysis.transient_structural` | `ImplicitTransient` |
| `analysis.modal` | `ImplicitModal` |
| `analysis.harmonic_response` | `ImplicitFrequency` |
| `analysis.response_spectrum` | `ImplicitResponseSpectrum` |
| `analysis.random_vibration` | `ImplicitRandomVibration` |
| `analysis.buckling` | `ImplicitLinearBuckling` |

## 6. 信号接线（mMechGlobalSignals）

```
桥接 → 软件（请求）：
  mpcommImportGeometryRequested(path)   → mMenuWidget::slot_mpcommImportGeometry
  mpcommImportCalcFileRequested(path)   → mMenuWidget::slot_mpcommImportCalcFile
  mpcommImportMpifRequested(path)       → mMenuWidget::slot_mpcommImportMpif
                                          → runMpifMappingDemo（点到点映射示例）
  mpcommStartSolveRequested()           → mMenuWidget::slot_startSolver（= 菜单"提交计算"同槽）

软件 → 桥接（回报）：
  mpcommImportGeometryFinished(path)          → committed(geometry)
  mpcommImportCalcFileFinished(ok,path)      → committed(mesh+setup)
  mpcommExportCalcFileFinished(ok,path)      → committed(mesh+setup)
                                              （两个来源：手动"导出计算模型"经
                                                writeSolveFinishedSig；求解流程
                                                自动写出经 mStartSolve::
                                                slot_outThreadFinished 写出成功
                                                后 emit。后者保证用户改参后求解
                                                的最新 mx 回传平台，否则第二次
                                                打开会回到导入时的旧文件。）
  mpcommExportGeometryFinished(path)          → committed(geometry)
                                              （手动"导出几何模型" stp/step/iges）
  mpcommSolverStatusChanged(state, outputs)   → started→changed(solution)；
                                                finished→committed(solution+results)；
                                                failed→changed(solution)
  mpcommModelCleared()                        → cleared(全部角色)

标志：
  _isPlatformMode   平台模式下禁用"新建"（slot_New 拦截）、跳过快照/恢复弹窗
```

## 7. 各命令处理要点

### OPEN_NODE
1. `readContext` 解析 exchangeId/contextRevision/node.id/**node.templateId**/editableCells/**data.files[] + connectionInputs[].data.files[]**（两处都读，后者优先）；
2. templateId / geometry / calcfile / results / mpif 五个待处理项挂 `qApp` 属性（OPEN_NODE 先于界面构造，信号会丢）；
3. **先发 `opened`**（状态机 start→opened，直接 committed 会被平台拒）；
4. `markSharedInputsCommitted(sharedRoles)` 把 context 带入的角色标记就绪。

`mMenuWidget` 构造函数取回 `mpcomm_pending_*` 属性，`QTimer::singleShot(500ms)` 延时执行（模型管理器就绪）；建工况 → 导几何 → 导计算文件。**results（后处理结果）特殊**：不在此处直接导入，而是挂在 `readSolveFinishedSig` / `importGeoModelRenderFinished` 两个导入完成信号上，待几何/计算文件导入完成、工况已建好后触发。**三个要点**：
1. **多个结果文件全部传入**——`readContext` 把 context 里 role=result/results 的文件收进 `resultPaths_` 列表（求解结果可能有多个），OPEN_NODE 挂 `mpcomm_pending_results`（QStringList），界面侧 `import3DResult(resultPaths, -1)` 全量导入；
2. **必须 caseid=-1**——走"直接导入结果文件"分支（`filterFiles` 按 mxdb 魔数识别直接读文件）；传具体 caseID 会改走 `caseBase->getSolveDir()` 扫描本机求解目录，而平台场景结果文件来自上游共享、本机求解目录为空，会报"未找到结果文件路径"；
3. **导入成功后要补建后处理视口 + 切换树选中项**——树节点/视口创建在 `mPost3DResultPanel::slot_finishedImportFiles`，依赖 `_curPostRend` 已由 `slot_create3DResultViewport` 设置（手动创建云图时由模型树信号触发），且 `slot_updateResultTree` 有"当前必须选中且类型是 ResultParentItem"的守卫。自动导入路径需在 `import3DResult` 前 emit `create3DResultViewport(Post_<modelID>_Cloud<resultID>)` 补建视口，并调 `mTreeView::selectPostResultParentItem()`（后处理根不存在时先创建，然后把选中项切到后处理"结果"父节点）——后处理单独建树（不挂工况下），与手动"打开结果文件"一致。

### REFRESH_INPUTS
1. 校验 `baseContextRevision == contextRevision_`（不匹配 `REFRESH_REJECTED`）；
2. `readContext` 更新本地 + `markSharedInputsCommitted` 立即上报；
3. `changed_cells` 含 geometry → 重导几何；含 mesh/setup → 重导计算文件；
4. **mpif 不依赖 `changed_cells` 标记**：只要本次 refresh 的 context 解析到了 `role=mpif` 文件（来自 `connectionInputs[].data.files`，上游结果连线到下游"设置"时平台 `changed_cells` 给的是 `setup`、不含 `mpif`），就触发 `mpcommImportMpifRequested` 重导；
5. **导入完成后照常回报 committed（不做回声抑制，见第 8.4 节）**。

### mpif 上游场数据（点到点映射示例）
OPEN_NODE/REFRESH 带来 `role=mpif` 文件时，经 `mpcommImportMpifRequested` → `slot_mpcommImportMpif` → `runMpifMappingDemo`（mMenuWidget，引路用示例，具体集成与映射规则由后续实现）：
1. `ftc::read_mpif_file` 解析，经 `outputMessageSig` 在软件信息窗口打印全部 set（id/name/节点/面/单元数）及 **NODE/FACE/CELL 三类**变量清单（名/分量数）——打包软件看不到 qDebug，统一走信息窗口（普通消息 `outputMessageSig(QString)`，警告 `outputMessageSig(1, QString)`）；
2. 选源 set：按 NODE → FACE → CELL 顺序找第一个"有变量且有记录"的实体（流体边界 set 的变量常挂在 FACE section 上，无脑取第一个 set 的 NODE 变量会扑空）；用源 set 节点坐标包围盒造 6 个目标点（域内 3 / 域外 2 / 精确命中 1），`TargetLocationSet::create(kind=Point)`；
3. `ftm::MappingPlan::prepare + apply_field`（Nearest 点到点，`outside_domain_policy=Nearest`，`MappingOptions::source_kind` 同步设为选中的实体类型），信息窗口打印每个目标点映射值与状态（Mapped/NearestFallback/OutsideDomain）+ 覆盖率统计。

注意：`MainWindow/CMakeLists.txt` 需链接 `FieldTransferCore::FieldTransferCore` + `FieldTransferMapping::FieldTransferMapping`（示例在 mMenuWidget 编译单元里用这两个库）；`MxMech/CMakeLists.txt` 桥接侧原有链接保留。

### EXECUTE_ACTION（solve）
校验 operation=solve / cell=solution / hasSession / contextRevision（不匹配 `STALE_CONTEXT`）→ emit `mpcommStartSolveRequested` → `Reply::pending`。
`slot_startSolver` 即菜单"提交计算"槽：取 `caseIDs[0]` → `mStartSolve::solver(caseID)`，全程无弹窗。

### CLOSE
先发 `closed` 再 `qApp->quit()`。两个来源都进 `MpcommBridge::onClose`（重入安全，`hasSession_` 已关闭时不再重复发）：
1. 平台下发 CLOSE 命令（`handleCommand` → 队列投递 `onClose`）；
2. **用户主动关软件**（main.cpp 里 `QCoreApplication::aboutToQuit` → `onClose`）——覆盖不经平台 CLOSE、用户直接关窗口的场景，保证平台能收到 `closed` 事件。

## 8. 已踩过的坑（重要）

### 8.1 计算文件重导后"设置无数据"
**现象**：REFRESH/OPEN_NODE 重导 .mx 后，网格有值但设置树/面板是空的。
**根因**：流程是"按 templateId 建空工况 → 再导 .mx"；而 `mIMxFile::ReadCaseData` 遇 `*subcase` 会**再建一个工况**且不发模型树信号。结果数据层两个工况并存（前空后实），面板/求解取 `caseIDs[0]`（空的）。
**修复**（mMenuWidget.cpp）：
- `slot_mpcommImportCalcFile`：重导前删除全部既有工况（`mInfoWindow::deleteCase` + `mCaseManager::deleteCase`），让 .mx 的工况成为唯一工况；
- `slot_readSolveFinished`：导入成功后对所有工况补发 `createUpdateCaseSig(true, caseID)` 建模型树节点（文件导入路径从来不发这个信号）。

### 8.2 导入完成后进程崩溃（peer closed the connection）
**现象**：OPEN_NODE 导入完成后约 1 秒进程消失。
**根因**：`slot_readSolveFinished`/`slot_readMeshFinished` 里 2 秒延时的**模型快照截图**（`modelFastCapture`/`FastCapture`）在后台运行环境下崩溃。
**修复**：平台模式（`_isPlatformMode`）下跳过快照；`checkAutoSaveRecovery` 的恢复询问弹窗同理跳过（平台模式无人点弹窗会卡死）。

### 8.3 几何导入了但没上报平台
**现象**：手动"导入几何模型"后平台 geometry 行不更新。
**根因**：手动菜单走的是 `mGeoPythonAPIRegistrar::onImportGeoFile` 注册器链路，不经过 `slotImportGeoFiles` 调用点，回报挂漏了。
**修复**：统一回报点 `reportMpcommGeometryOnRenderFinished(filePath)`（挂在 `importGeoModelRenderFinished` 信号上，先 disconnect 指向 this 的旧连接再 connect 防重复），在 `mMenuWidget` 全部 **7 个**几何导入路径挂接（菜单导入、最近打开×2、继承几何、STL、平台触发、注册器链路）。
**未覆盖**（如需要再补）：`mTreeView.cpp:9737`（模型树右键导入）、`mRecentDataBase.cpp:78`。

### 8.4 回声抑制（已去除）
曾按多体做法做回声抑制（`fromPlatform=true` 的导入不回传 committed），导致 REFRESH 重导后平台行不更新。**已全部去掉**：所有导入完成（含平台触发的）都回报 committed，平台对同值重复提交幂等。配套的 `fromPlatform` 参数 / `_mpcommImportFromPlatform` 标记已一并删除。

### 8.5 求解 committed 被平台拒绝
**现象**：`changes.solution.resultFiles 含多个文件时每项都必须提供唯一 id`。
**修复**：resultFiles 每项生成 `{id:"file-N", path:...}`（见第 4 节）。

### 8.6 resultFiles 必须是具体文件
`mStartSolve::slot_processFinished` 扫描求解目录 `_caldir` 下的 `*.mxdb/*.mxdb0/*.op2`（与后处理 `readPostResultFile` 规则一致），收集为 `outputs["resultFiles"]` 字符串列表上报；目录路径不行。

## 9. 本地联调

模拟器：`E:\mpcomm_test\main_mech.cpp` → `build-mech\Release\mpcomm_mech_test.exe`

```
mpcomm_mech_test.exe <exe或launcher路径> [mx计算文件] [templateId] [几何文件]
# 例：
mpcomm_mech_test.exe E:/MxSim/test/Version20260921-RebuildMech/MechLauncher.exe E:/WorkingDir/7-3.mx analysis.static_structural E:/WorkingDir/11.stp
```

流程：拉起 → OPEN_NODE（templateId + 计算文件 + 可选几何）→ REFRESH_INPUTS(mesh+setup, r1→r2) → EXECUTE_ACTION solve → CLOSE，打印全部回包与事件，检查 opened / REFRESH 后 committed(mesh+setup) / geometry committed / closed。

**环境要求**：被测 exe 必须是**带完整运行时依赖的**（打包目录经 MechLauncher 拉起，或直接配好 PATH 的 bin/Release）；裸 bin/Release 的 exe 缺 dll 会静默退出，平台侧报 `timed out waiting for child process connection`。`E:\mpcomm_test\run_mech_test.bat` 里有完整的 PATH 配置参考。

## 10. 平台模式行为差异汇总

| 行为 | 独立模式 | 平台模式（`_isPlatformMode`） |
|---|---|---|
| 新建模型 | 允许 | 禁止（slot_New 拦截提示） |
| 模型快照截图 | 导入后 2s 截图 | 跳过（后台截图会崩溃） |
| 求解进度/完成弹窗 | 正常 | 无弹窗（提交计算入口本身无弹窗） |

## 11. 维护清单（改动时对照检查）

- [ ] 新增导入入口（几何/计算文件）→ 挂 `reportMpcommGeometryOnRenderFinished` 或 emit `mpcommImportCalcFileFinished`
- [ ] committed 涉及 resultFiles → 多项时带唯一 id
- [ ] 平台模式新增的界面操作 → 检查是否弹窗/截图类（跳过）
- [ ] 删/清模型的新入口 → emit `mpcommModelCleared`
- [ ] 求解结果文件类型变化 → 同步 `mStartSolve` 的扫描后缀与后处理规则
- [ ] 修改本文件涉及的行为 → 更新本文档

---

## 12. 消息模板（软件 → 平台，当前实现完整对照）

所有业务消息都发在 topic `mxsim.node`；日志发在 topic `application.log`。`revision` 为软件本地自增整数（每条 mxsim.node 消息取 `revision_+1`，发送成功后赋值回去），与平台下发的 `contextRevision`（字符串）是两个不同的量。下文 `<N>` 表示自增后的 revision 值。

### 12.1 节点事件（opened / failed / closed）

用途：状态机流转。`sendNodeEvent(event, text, level)`。

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "MxMech node opened",
  "data": {
    "event": "opened",
    "revision": <N>
  }
}
```

| event | 触发时机 | level |
|---|---|---|
| `opened` | OPEN_NODE 处理成功（**必须先于任何 committed/changed**，否则平台拒绝 `start -> committed`） | info |
| `failed` | OPEN_NODE 的 readContext 解析失败 | error |
| `closed` | CLOSE 命令或用户关软件（`aboutToQuit`），退出前发送 | info |

### 12.2 changed（某单元格开始变化）

用途：求解开始/失败等"进行中/异常"状态。`sendChanged(cells, text)`。**cells 过滤 `editableCells`**，越权角色剔除；全部被过滤则整包不发。

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "solver started",
  "data": {
    "event": "changed",
    "revision": <N>,
    "changed_cells": ["solution"]
  }
}
```

| text | changed_cells | 触发时机 |
|---|---|---|
| `solver started` | `["solution"]` | 求解器进程启动成功（`mStartSolve::startSolve`） |
| `solver failed` | `["solution"]` | 求解进程异常退出 / 计算文件写出失败 |

### 12.3 committed（某单元格有了结果）

用途：导入完成、求解完成、共享输入就绪。`sendCommitted(cells, changes, text)`。**cells 过滤 `editableCells`**；changes 非空才携带。

**几何导入完成**（含 OPEN_NODE 平台触发的，不做回声抑制）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "geometry committed",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["geometry"],
    "changes": {
      "geometry": { "files": [{ "path": "<几何文件绝对路径>" }] }
    }
  }
}
```

**计算文件导入完成**（.mx 同时挂 mesh+setup；mesh 用单 `path` 字符串不是数组）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "calc file committed",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["mesh", "setup"],
    "changes": {
      "mesh":  { "path": "<计算文件绝对路径>" },
      "setup": { "projectFiles": [{ "path": "<计算文件绝对路径>" }] }
    }
  }
}
```

**手动导出计算文件完成**（结构同导入，text 不同；**求解流程的自动导出不上报**，见第 6 节）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "calc file exported",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["mesh", "setup"],
    "changes": {
      "mesh":  { "path": "<导出文件绝对路径>" },
      "setup": { "projectFiles": [{ "path": "<导出文件绝对路径>" }] }
    }
  }
}
```

**手动导出几何完成**（stp/step/iges，结构与几何导入相同）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "geometry exported",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["geometry"],
    "changes": {
      "geometry": { "files": [{ "path": "<导出文件绝对路径>" }] }
    }
  }
}
```

**求解完成**（resultFiles 多项时**每项必须带唯一 id**；results 可附加 mpifFiles，后续耦合用）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "solver finished",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["solution", "results"],
    "changes": {
      "solution": { "resultFiles": [
        { "id": "file-0", "path": "<结果文件1绝对路径>" },
        { "id": "file-1", "path": "<结果文件2绝对路径>" }
      ]},
      "results": { "resultFiles": [
        { "id": "file-0", "path": "<结果文件1绝对路径>" },
        { "id": "file-1", "path": "<结果文件2绝对路径>" }
      ]}
    }
  }
}
```

**共享输入就绪**（OPEN_NODE/REFRESH 后 `markSharedInputsCommitted`；按实际收到的角色子集组装，results 兜底用 context 带入的列表）：

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "shared inputs applied",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": ["geometry", "mesh", "setup"],
    "changes": {
      "geometry": { "files": [{ "path": "<几何文件绝对路径>" }] },
      "mesh":  { "path": "<计算文件绝对路径>" },
      "setup": { "projectFiles": [{ "path": "<计算文件绝对路径>" }] }
    }
  }
}
```

### 12.4 清空单元格（删模型/清空按钮）

用途：删除模型/模型树"清空"按钮，通知平台清空全部单元格。`sendCleared(cells, text)`——**event 仍是 `committed`**，但 `committed_cells` 为空数组、`cleared_cells` 带要清的角色（过滤 `editableCells`）。

```json
{
  "topic": "mxsim.node",
  "level": "info",
  "text": "model cleared",
  "data": {
    "event": "committed",
    "revision": <N>,
    "committed_cells": [],
    "cleared_cells": ["geometry", "mesh", "setup", "solution", "results"]
  }
}
```

### 12.5 应用日志

用途：诊断/过程信息，不进节点状态机。`sendLog(text, level)`，topic 固定 `application.log`，data 为 null。**默认不发送**：桥接里所有 `sendLog` 调用都经 `MPCOMM_SEND_LOG` 宏包了一层（mMpcommBridge.cpp 顶部），`MPCOMM_ENABLE_DEBUG_LOG` 未定义时宏展开为空操作——排障时在编译定义里加 `MPCOMM_ENABLE_DEBUG_LOG`（或临时取消该处注释掉的 `#define`）才打开。

```json
{
  "topic": "application.log",
  "level": "info",
  "text": "import calc file from context: E:/WorkingDir/1-3.mx",
  "data": null
}
```

### 12.6 字段速查（写错会被平台拒绝）

| 角色 | changes 字段 | 错误示范 |
|---|---|---|
| geometry | `files: [{path}]` | — |
| mesh | `path: "..."`（单字符串） | `files` 数组（`changes.mesh 含不支持字段：files`） |
| setup | `projectFiles: [{path}]` | `files` |
| solution | `resultFiles: [{id, path}]` | `files`；空提交（`提交 solution 必须提供新的 resultFiles 或 mpifFiles 作为完成证明`）；多项缺 id（`resultFiles 含多个文件时每项都必须提供唯一 id`） |
| results | `resultFiles: [{id, path}]` + 可选 `mpifFiles` | 同上 |
