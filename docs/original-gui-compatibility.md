# 原 GUI 恢复与兼容层说明

记录日期：2026-11-23。目标分支：`lite-occ`。

本轮以用户上传的 GUI、ModelData 源码作为界面与调用契约的基准，恢复原多体面板、拾取组合控件、菜单入口、模型树图标及结果界面，在缺失 SDK 的位置接入现有 OCC、FEM、MotionEngine 和 Qwt 后端。本文记录具体已接接口、独立测试证据、已通过的完整应用指定流程和剩余边界，**不表示整个原 SDK 或所有菜单功能已经完成**。第 8 节记录原 GUI 主流程及后续新增的原生曲线、真实模态、独立全 FEM 静力显示与原载荷导出回归；第 9 节保留尚待执行和补充的验证项。

收尾验证为 **30/30 通过：21 项非视口测试 + 9 项真实桌面测试**，见[非视口报告](verification/original-headless.txt)与[最终桌面报告](verification/original-desktop.txt)。独立 picker/viewport 的 DPR 1/2 证据另列，不能扩大为完整应用双 DPI 或 Windows 实机全流程通过。

## 1. 参考输入与固定版本

以下哈希对本轮实际下载的 ZIP 原文件计算，未对解压目录重新打包：

| 输入 | SHA-256 |
| --- | --- |
| `RebuildGUI-wzq_Mech.zip` | `aa3c4116f0acbdbd502c46127e9b41e51ddee2cd929562a0879894aa624675b9` |
| `RebuildModelData-wzq_Mech.zip` | `7fefce972bdc315d3f078b5b738c4d1cd9439b5ef8d22fcf324ed379f490e1ed` |

本轮原文件与解压参考目录位于 `/workspace/scratch/62821ae63f18/original-upload-reference/`。复核 ZIP 时使用：

```sh
sha256sum RebuildGUI-wzq_Mech.zip RebuildModelData-wzq_Mech.zip
```

渲染与鼠标交互参考仓库为 [wzqlalala/Motion](https://github.com/wzqlalala/Motion)，固定 donor 提交为 `b636535a45895b193aa581afd4df10a9004c9b6e`，分支名为 `codex/multi-solver`（multi-solver）。其用途是参考视口渲染、导航、坐标系几何与交互；应用外壳、菜单和多体参数控件仍以原上传 GUI 为依据。坐标系适配源保留 donor 注记，见 [navigation](../GUI/src/MxMotionOcc/navigation/)。

## 2. 恢复原则

1. **复用原控件与原面板。** `mPointPickerWidget`、`mDirectionSelectorWidget`、`mGeneralPickUp`、原 picker views 和 `mMotion*Panel` 继续承担界面。点输入、方向模式、起点/终点、应用、反向、清空、数据详情及选择会话均沿用原组合控件。新引擎字段需要点/方向输入时也嵌入这些控件。
2. **在原接口下面适配。** `mModelData`、`mViewportModel`、`CCSMRendWidget`、选择容器、几何查询、网格编号及绘图接口通过 `compat` 转接。原源码中的必要条件编译、单位/数据问题修正与缺失 SDK 实现须明确记录，不能以可编译为由把原方法替换为空成功。
3. **保留原菜单的结构与入口。** Motion 模式的主页、几何、体定义、运动副与约束、驱动、连接与力元、传感与控制、求解、显示、选择页按原 `mMenuWidget` 恢复；模型树使用真实资源图标。保留入口与真正实现其后端动作是两项不同状态。
4. **使用真实几何、网格与结果。** 拾取返回真实 OCC 拓扑或 FEM 节点/连接编号；质量属性、相机、可见边界、模态与结果查询取实际数据。没有可用后端的流程应给出明确错误，不合成网格、模态、曲线或求解成功状态。
5. **保留原宿主生命周期。** 原 `newXxxPanel` 往往只创建数据并通知树，新节点选中后才调用 `editXxxPanel` 填充字段和显示相应分组。兼容宿主恢复“新建 → flush 并识别新 ID → 原 edit”顺序；不产生单个实体 ID 的批量/模板面板只执行 new。相关修复位于 [OriginalMotionUi.cpp](../GUI/src/MxMotionOcc/OriginalMotionUi.cpp)。

构建入口 [OriginalGui.cmake](../GUI/src/MxMotionOcc/OriginalGui.cmake) 列出实际编译的原 GUI、原 `MechMotionData` 类和兼容后端。它是接口恢复范围的依据，不能据此推断未列入的原结构、网格、后处理 SDK 已恢复。

## 3. 原接口与实际后端的对应关系

下表中的“已接”表示源代码路径已连接到所列真实后端；运行验证仅以第 6、8 节列出的具体测试为准。

| 原入口/契约 | 兼容实现与集成文件 | 实际后端及行为 |
| --- | --- | --- |
| 原多体面板、原数据类、编辑信号 | `OriginalMotionUi.cpp`；`compat/model/LegacyMotionContext.*`、`LegacyModelData.cpp`、`LegacyMotionJsonAdapter.*`、`LegacyMotionExtensionCodec.*` | 编译原 `MechMotionData` 类；在原 manager 与 `NativeMotionDocument` 间转换。字段增量合并保留未编辑的精度与扩展字段，原面板修改通过原信号提交。 |
| 原点/方向/多选 picker 与 controller | `compat/picking/OccPickController.*`、`LegacyPickContext.*`、`OccPickPersistence.h`；原 `mPointPickerWidget`、`mDirectionSelectorWidget`、`mGeneralPickUp` | 原 UI 发出选择会话，`OccViewport` 完成真实 CAD/FEM 拾取，controller 接收数据并使用原应用/清除/反向流程。生命周期与当前模型上下文由 QObject ownership 和 `QPointer` 管理。 |
| 原几何编号、点在线上副/点在面上副引用 | `LegacyPickContext.*`；`LegacyMotionContext::bindTopologyEdits`；`ModelData/src/MotionOcc/NativeTopologyBinding.*` | `TopExp::MapShapes` 建立真实顶点/边/面/实体映射；原全局编号与 `{partId, index}`、几何指纹之间转换。边的解析切向、圆/椭圆中心与轴、面的几何方向来自 OCC。显示三角形序号不能充当几何编号。 |
| `mGeoEngineData` / `GeoEngineHandle::calcCalculatePartMass` | `compat/mGeoEngineData.h`、`mGeoEngineHandle.h` | 调用真实 `nativeBodyMassProperties`，取得所选 CAD 部件的质量、质心和惯量；不是包围盒体积近似。 |
| 原网格节点容器与全局节点号 | `compat/mMeshData.h`；`compat/model/LegacyMeshData.cpp` | 读取实际 `FemMesh` 节点、表面三角形和四面体连接；根据网格指纹保存稳定全局节点 ID。网格改变时重新分配，不把旧编号自动解释为新节点。 |
| 原 AutoFlex 网格、预览、附着点与导出输入 | `compat/solver/LegacyMotionFlexBridge.cpp`；`compat/model/LegacyFlexAttachmentCodec.*`、`LegacyMaterialCodec.h` | 调用现有 Netgen 网格任务及实际四面体数据；验证材料、目标体、附着节点和连接组。未变网格可复用，失效附着明确拒绝。预览退出恢复进入前的显示模式。原 `.mx` 导出继续使用 `mMotionFlexMxExport`。 |
| `CCSMRendWidget`、高亮 controller、几何/网格选择容器 | `compat/render/LegacyRender.*`；`compat/mxr/...`；`compat/PickCommonFunction.h` | 映射到 `OccViewport` 的独立辅助高亮层、实际相机和可见边界。多个 inactive 控件可保留各自颜色层，不覆盖 active collector。 |
| 原符号、公共点/圆柱图元 | `LegacyRender.*`；`NativeMotionSymbols.h`；`OccViewport.cpp` | 将真实 `_vexter/_lineIndex/_faceIndex` 送入线与三角形绘制；保留原颜色、透明度、文本与已支持的深度设置。点大小真正传至 framebuffer；原预览与 native 符号独立，同一实体避免重复绘制。 |
| Motion 左下角坐标系、导航与 CAD 反馈 | `navigation/OrientationTriadGeometry.h`、`OrientationTriadInteraction.h`、`ViewportOrientationTriad.h`、`OrientationTriadCamera.h`；`OccViewport.*` | 适配固定 donor 的三轴几何、命中和操作，使用实际相机。经典/NX 导航、屏幕轴旋转、平移、缩放与历史作用于相机；CAD 面/边/点高亮遵守实际深度。 |
| 原 `mMotionSolverThread` 对面板暴露的方法和信号 | `compat/solver/LegacyMotionSolverThread.h`；`OriginalMotionUi.cpp`、`RigidMotionUi.cpp` | `doTransInput`、数据检查、start/stop、进度及结果同步接入现有真实异步 MotionEngine 求解任务；不会为原面板另起一个物理系统。已编译接口不等同于所有物理模型/求解器组合通过。 |
| 原动画、结果查询与曲线面板 | `OriginalMotionResults.cpp`、`OriginalResultUi.cpp`；原 `mMotionAnimationPanel`、`mMotionCurvePanel`、`mMotionResultQueryService` | 原动画调用真实 `seekRigidTime` 与接受帧；单个静力帧可查看平衡构型，但禁用时间播放。曲线查询使用求解快照、单位边界转换和实际通道；缺失、重复、截断或错误单位的结果不得当作零值或连续有效采样。原三个选择框与列表接入 MotionEngine 新通道，保留原/native 两种曲线 schema、数值分量、颜色格式与未知扩展字段。 |
| 原 `mCurveWindow` / 缺失 `mPlot*` SDK | 原 `GUI/src/GUIFoundation/mCurveWindow.cpp`；`compat/mPlotView.h`、`mPlotViewStyleWidget.h`、`compat/plot/OriginalPlot.cpp` | 使用实际 Qwt curves、axes、legend、marker、picker 和 zoomer。各曲线保留独立时间轴，NaN 分段断线；时间点击可驱动原播放接口。 |
| 原载荷导出面板 / 原载荷导入面板的保留状态 | `OriginalResultUi.cpp`；原 `mMotionLoadExportPanel`、`mMotionLoadImportPanel` | 多体侧导出接入真实 CSV/BDF/载荷 JSON 服务。载荷导入原面板参与构建，保留 `showOriginalMotionLoadImport` 私有 factory，但当前没有外壳菜单调用，完整用户入口未接通；结构静力生成服务也未接，见第 7 节。 |
| 原非运动菜单与选择菜单 | `OriginalUiActions.cpp`、`OriginalSelection.cpp`、`MotionRibbon.cpp` | 已接项目/几何/JSON 等文件入口、相机动作及实际拓扑/网格筛选、编号选择、转换、邻接和角度扩展；选择结果经原 controller 提交。未接动作保持明确提示。 |

`mBasicEnum.h`、`mModelStruct.h`、`mPhysicalFieldEnum.h`、`mxr/data/SelectDataStruct.h` 等兼容头提供本轮原多体代码需要的类型子集。保留同名调用契约不意味着缺失 SDK 的所有枚举、数据类别、格式和 ABI 已复刻。

## 4. 拾取、呈现与数据生命周期的约束

以下行为已进入原 picker 专项回归，不能在后续优化中简化掉：

- `displayData(data, false)` 只更新控件 model/UI，不抢占当前 collector，也不写高亮；显式 `true` 才同步选择与反馈。共享部件选择接口本身需要改变选择时，应显式使用 `true`。
- `setCloudChartMode(true)` 只隐藏活动 CAD、hover、节点/网格反馈；typed 数据保留且继续接受拾取，退出后恢复。结束会话重置呈现状态，不泄漏到下一个控件。
- `ClearPreWidgetSymbolPick` 清除符号选中状态，不能删除符号数据。独立原符号层与 native 符号层分别管理。
- `Solid` 是真实子实体，不能按整部件 `Part` 代替。FEM 节点号与连接号必须来自实际网格，不能用当前显示序号替代。
- 场景重建同时校验 OCC shape/subshape identity、实际索引与 FEM 连接。相同部件 ID、相同局部编号不保证还是同一几何；删除和重导入后不能让旧辅助点复活。普通显示重建仍保留有效引用。
- parentless 原 factory 的 controller 随其 field 进入面板并随 field 销毁；析构不得回调已部分销毁的 QWidget。明确绑定的模型上下文销毁后不能自动落到另一个窗口的全局上下文。
- 临时 viewport 引用失效处理不能擅自删除持久模型数据；工程引用修复继续由持久拓扑绑定流程处理。

## 5. 原界面保留与缺失 SDK 的区别

原 `mCurveWindow` 和多体曲线/动画/载荷面板直接复用。两个输入 ZIP 均不包含 `mPlot*` SDK 的实现，因此 Qwt 适配层及其样式属性编辑器属于缺失接口的补实现，不作“与未提供的原 SDK 界面逐像素相同”的承诺。

同样，菜单页与命令恢复不代表所有高级 CAD 操作已经实现。当前 Box/Cylinder/Sphere 的参数入口仍使用现有通用参数表单，尚未恢复原鼠标构造交互。外壳尺寸、停靠区宽度和截图一致性须作为完整应用视觉验证的一部分单独确认，不能由 picker 独立窗口通过替代。

## 6. 已完成的独立验证与证据路径

核心测试报告已随仓库保存在 `docs/verification/`，下表使用可复核的仓库相对链接。原始运行输出位于 `/workspace/scratch/62821ae63f18/build-support/`，仅作复现辅助；后续 CI 应为实际受测提交重新生成并归档结果。

环境日志记录为 Linux、Qt `5.15.15`、`xcb`。桌面用例使用实际 OCC/FEM 数据和 OpenGL framebuffer；没有用无渲染替身替代图像断言。

| 验证批次 | 结果与范围 | 日志路径 |
| --- | --- | --- |
| 原 picker 基础及追加契约，DPR 1 | 通过：原点/方向控件、六种方向模式、反向、起终点独立应用、两实体子拓扑、真实 FEM ID、撤回/Esc/清空、世界原点、数值校验、新字段复用与符号层；另含 `displayData(false/true)`、节点与 CAD 云图隐藏/恢复、隐藏期间继续拾取、失效引用及连接检查。 | [原 picker DPR 1](verification/original-picker-dpr1.txt) |
| 原 picker 基础及追加契约，DPR 2 | 同上，通过高 DPI 实际呈现与交互断言；实际日志记录 `DPR=2`。 | [原 picker DPR 2](verification/original-picker-dpr2.txt) |
| Viewport suite，DPR 1 | Picking / Navigation / Render feedback 全部通过。 | [Picking](verification/original-picking-dpr1.txt)、[Navigation](verification/original-navigation-dpr1.txt)、[Render feedback](verification/original-render-feedback-dpr1.txt) |
| Viewport suite，DPR 2 | Picking / Navigation / Render feedback 全部通过。 | [Picking](verification/original-picking-dpr2.txt)、[Navigation](verification/original-navigation-dpr2.txt)、[Render feedback](verification/original-render-feedback-dpr2.txt) |
| Headless / 非视口组合回归 | 21 项通过，包含几何拓扑、工程、真实求解、柔性结果、质量、载荷导出、动力学、静力/伺服、独立柔性接口、模态、网格、播放/曲线，以及 `OriginalBackend`、`OriginalPlot`。具体测试名以报告为准。 | [完整 CTest 报告](verification/original-headless.txt) |
| 最终完整构建桌面回归 | 9 项全部通过，包括原 GUI 主流程、原生曲线、原载荷导出、真实模态与全 FEM 静力显示，以及视口与拾取。退出码 0，整批耗时 30.05 秒。 | [最终桌面 CTest 报告](verification/original-desktop.txt) |

最终 `full-build` 收尾回归共 **30/30 通过**。九项真实桌面测试为 `Desktop`、`NativeDesktop`、`OriginalNativeCurves`、`OriginalLoadExport`、`Picking`、`OriginalPicker`、`Navigation`、`RenderFeedback`、`ModalDesktop`；这一整批已包含原结果树 `Export` 分派修复，三张原导出页及实际文件断言全部通过。21 项非视口测试与最终桌面测试采用同一套物理源码。批次原始日志位于 `build-support/original-final-headless-tests.log` 与 `build-support/original-final-desktop-tests.log`，可复核的报告已按上表归档。

已归档的界面证据包括[原组合拾取控件](images/original-picker-controls.png)和 [Motion 左下角坐标系](images/original-motion-triad.png)。测试源码分别见 [MotionOccOriginalPickerSmoke.cpp](../tests/MotionOccOriginalPickerSmoke.cpp)、[MotionOccPickingSmoke.cpp](../tests/MotionOccPickingSmoke.cpp)、[MotionOccNavigationSmoke.cpp](../tests/MotionOccNavigationSmoke.cpp) 和 [MotionOccRenderFeedbackSmoke.cpp](../tests/MotionOccRenderFeedbackSmoke.cpp)。

原始输出目录对应 `original-picker-dpr1/`、`original-picker-dpr2/`、`original-picker-contract-desktop/`、`original-picker-contract-dpr2/`、`original-viewport-dpr1/`、`original-viewport-dpr2/`。前两个为基础批次；contract 批次保留基础断言并增加接口契约断言，已归档的两份 picker 报告取自 contract 批次。

Viewport Navigation 的两份日志均明确记录 Motion triad 源几何、平移/缩放独立性、面导航、轴角度/拖动、Esc、picker 隔离和字体缩放；同时覆盖原显示菜单的屏幕轴旋转、百分比平移、缩放、相机历史和实际可见边界。Render feedback 覆盖遮挡、刚体 pose、隐藏/显示、缓存重建与结束选择清理。

**验证批次不能混用：** 基础 picker、viewport suite 以及后追加的云图/回填/失效引用断言均有上表对应的 DPR 1/2 独立结果。它们未在本文中扩大为最新完整应用的双 DPI 全量通过。

正式构建开启 `BUILD_TESTING=ON` 与 `MX_TEST_DESKTOP=ON` 后，可在具备桌面 OpenGL 的会话中使用现有入口：

```sh
build/bin/MxMotion --original-picker-smoke build/original-picker-smoke
build/bin/MxMotion --picking-smoke build/picking-smoke
build/bin/MxMotion --navigation-smoke build/navigation-smoke
build/bin/MxMotion --render-feedback-smoke build/render-feedback-smoke
```

Linux 的第二轮可对对应进程设置 `QT_SCALE_FACTOR=2`。本轮普通 exec 环境尝试启动 Xvfb 时不能建立监听 socket，EGL headless 也未建立可用窗口；这两次失败发生在测试初始化前，未作为功能通过证据。上表的通过来自实际桌面终端启动的独立测试。

## 7. 明确的兼容边界

| 项目 | 当前范围与限制 |
| --- | --- |
| 任意固定像素符号 | `_lineIsFixedPixel` / `_faceIsFixedPixel` 的通用固定像素缩放尚未实现。当前接回的原多体 symbol helpers 显式使用 `false`；这不代表该 SDK 选项已通用实现。 |
| 任意高亮 cover/depth | `setHighLightCover` 保留接口及参数，但任意 cover/depth 组合尚未对应独立渲染策略。CAD 反馈目前采用 Motion 深度测试规则；不能把它说明为原高亮 SDK 全行为一致。 |
| 高级 CAD 与原几何构造交互 | 复杂几何编辑、原鼠标建模流程仍有未接动作；基础体当前通用参数表单与原鼠标构造不是同一流程。面/边/单元级 mask、剖切、草图等也不能仅因菜单存在而算完成。 |
| 全网格/结构 SDK | 当前以实际表面三角形和四面体 `FemMesh` 为基础，未实现所有原单元类型、任意网格编辑和结构对象体系。自由节点删除涉及稳定引用重映射，不能按显示删除冒充模型删除。 |
| 载荷导入用户入口与结构静力模型自动生成 | 原载荷导入面板和拾取控件源码编译保留，但私有 `showOriginalMotionLoadImport` factory 当前没有外壳菜单调用，完整用户入口尚未接通。`generateMotionStaticCases` 后端也未接通；当前不会创建结构 case/load/set/RBE3/connector 数据，该结构侧载荷导入→建模→求解流程尚未完成。已验证的多体静力平衡、独立全 FEM 双接口静力显示与该自动结构建模流程分开验收。 |
| 载荷导出到柔性节点 | 刚体节点作用点使用实际 body binding、初始质心及逐帧 pose 处理力臂。缺少逐帧柔性节点 ID 映射的情形明确拒绝，不能用刚体变换替代真实柔性节点运动。 |
| 外部 MNF 工具 | 原 `mMotionFlexMxExport` 可生成 `.mx` 输入；后续 `mxlibi` / `MxSim.exe` 等外部工具未包含在云端依赖中。native 后端没有完整 MNF 写出器；真实模态求解不等同于 MNF 导出完成。 |
| 外部联合仿真与模板 | 电、液、RL、机构/传动等原面板的类与配置入口接回，不代表 Simulink/FMU/AMESim、外部训练器或原模板中未实现的逻辑已实际运行通过。 |
| 工程格式与单位系统 | `.mxmotion` V3 使用 JSON 和内嵌 STEP，以毫米保存零件位姿和几何引用，同时打包外部 MNF 和 AutoFlex 体网格。支持读取 V1 和 V2。运动 JSON 交换可通过相对路径加载外部 STEP。原 `.mxcae`、`.mxses` 读写源码保留，当前构建暂未接回原几何序列化 SDK。 |
| 其他原应用服务 | 部分偏好设置、许可、定制截图及其它未提供 SDK 服务仍有缺口。未知或未接命令应返回明确的未接通提示，不伪造已执行。 |
| 多模型 | 新建模型保留现有模型及其未保存修改。通过视口或视图树切换模型，保存作用于活动模型。删除有更改的模型时可选择保存、放弃或取消。 |
| 多视图 | 在「视图」树右键新建视图页或为活动模型添加视口，右上角选择布局预设。同页视口各自使用所属模型的几何和求解结果。点击视口或双击树上的视口节点会切换模型树和拾取上下文。删除模型会移除其视口。相机可独立操作或开启联动。`.mxses` 会话存取仍待接入。 |
| 平台与组合覆盖 | 独立 Linux 桌面测试不等于 Windows 实机全流程通过，也不保证任意材料、几何、网格、求解器、积分器、接触与控制组合均有效或稳定。 |

## 8. 完整应用指定流程的实际验证

**第二轮完整原 GUI 桌面回归已通过，进程退出码为 0；最新完整构建中的 `MotionOcc.NativeDesktop` 也已复测通过。** 本节是完整 `MxMotion` 应用的自动回归结果，范围限于所列用例。受测程序为本轮 `build-support/full-build/bin/MxMotion`，源代码基于 `dcbca02e047213449fe1aa8631db99df5b250d02` 加本轮 GUI 恢复改动；独立对象文件编译结果没有用来替代此处的运行结果。

第一轮在新建点在线上副的控件分组显示处失败。检查原 `mTreeView::slot_updateMotionJointTree` 后确认：原 new 方法创建对象，树随后选择对象并进入 edit；兼容宿主当时缺少后半段。修复宿主 new→edit 生命周期后，第二轮保留原拓扑断言，并增加当前副类型和 collector 上下文断言，已实际通过。没有通过隐藏控件查找或省略真实几何持久化检查来放宽验收。

核心证据为已归档的[完整原 GUI 桌面报告](verification/original-gui-desktop.txt)、[AutoFlex 报告](verification/original-autoflex-desktop.txt)和[原模型树报告](verification/original-tree-desktop.txt)。三份报告及对应截图已更新为最新 `build-support/full-build/native-desktop-smoke/` 输出；较早的第二轮原始输出保留在 `original-app-desktop/`：

| 验证流程 | 实际结果 | 证据与范围 |
| --- | --- | --- |
| 原运动副控件、树图标与独立副动作 | 通过 | [主报告](verification/original-gui-desktop.txt)记录 25 个树图标与 17 个独立运动副动作；不表示全部菜单动作后端已实现。 |
| 导入 STEP → 导入原 JSON → 原方案面板 → 真实 MotionEngine 求解 | 通过 | [主报告](verification/original-gui-desktop.txt)：11 个接受帧，90°/s，0.1 s 时为 9°。实际求解与原方案入口连通。 |
| 原结果查询、动画滑块/播放及 Qwt 曲线 | 通过 | [主报告](verification/original-gui-desktop.txt)：接受帧、角度与 N·mm 单位、连续角度、实际距离导数和不可用通道；动画映射实际 CAD pose，曲线使用同批结果。修改曲线显示选项保留已接受的物理结果。 |
| 新建点在线上/点在面上副及持久拓扑引用 | 通过 | [主报告](verification/original-gui-desktop.txt)：可见原 collector、正确上下文、第二部件全局编号与准确 OCC 指纹。 |
| 连续创建原面板与销毁 controller | 通过 | [主报告](verification/original-gui-desktop.txt)：14 个原面板入口连续创建与清理。仍不把外部电液/RL 模板的创建通过当作其外部求解通过。 |
| 原字段编辑 → 保存工程 → 重新打开 | 通过 | [主报告](verification/original-gui-desktop.txt)：原组合点控件值重现，未编辑精度与未知扩展字段保留。 |
| AutoFlex 实际完整链 | 通过 | [AutoFlex 报告](verification/original-autoflex-desktop.txt)：CAD + JSON、原材料/网格设置、真实 Netgen、实际视口节点命中原 field、手动端面节点到 native 附着组、禁用 worker 后复用已存网格进行真实柔性求解、保存重开。45 节点、77 四面体、8 附着节点、3 接受帧；包含 body ID 0 / material ID 0 用例。 |
| 原模型树分组、菜单、复制/重命名/删除与求解动作 | 通过 | [模型树报告](verification/original-tree-desktop.txt)：固定/动态分组、图标、root/group/entity/scenario 菜单结构；typed 复制、内联重命名、删除、扩展/引用保留及 live-panel 生命周期；Planar/PointLine 真实类型、未知类型拒绝、四种力元子类型与曲线源清理；原 `onStartSolve` started/finished 信号及真实引擎结果。 |

可直接审阅的归档截图包括[原运动副面板](images/original-joint-panel.png)、[原面拾取控件](images/original-surface-picker.png)、[原曲线面板与 Qwt 结果](images/original-curve-panel.png)、[AutoFlex 真实网格](images/original-autoflex-real-mesh.png)。逐像素风格对照、任意窗口尺寸下的裁切情况仍须依据截图和具体环境检查。

### 8.1 原界面中的 MotionEngine 新曲线通道

`MotionOcc.OriginalNativeCurves` 已在最新完整程序中通过，详见[原生曲线报告](verification/original-native-curves.txt)、[实际曲线截图](images/original-native-force-curves.png)和[测试源码](../tests/MotionOccOriginalNativeCurvesSmoke.cpp)。原三个下拉选择框、原曲线列表，以及添加、显隐、颜色、删除操作均保留。

用例读取 `examples/native-forces.mxmotion` 并执行真实求解，保留 **101 个接受帧**。原有四条保存请求通过原面板进入 Qwt，逐点与真实通道查询比较；驱动角度保持原单位，绳索载荷出现实际非零值，止挡接合状态同时包含接合和分离样本。随后用原控件增加止挡 `Coordinate` 与只拉绳索 `SignedForce`，验证颜色、显隐、删除操作和未知扩展字段保存；重新打开工程并再次真实求解后，六条曲线仍保留正确通道与采样。

兼容层保留原枚举序号，追加新 source 与 `NativeChannel` 路径，接受原字符串分量/对象颜色和 native 数值分量/字符串颜色两种 schema。既有原七类变量继续使用原 typed 查询；新增 source 根据实际结果通道提供可选量。高维柔性分量的 codec 往返也有断言，但本次六条曲线的完整桌面物理用例集中在驱动、止挡与绳索，不能据此扩大为每种新力元及所有高维模态通道的完整用户流程均已实测。

### 8.2 真实模态检查与独立全 FEM 静力显示

`MotionOcc.ModalDesktop` 已在最新完整程序中通过，详见[模态与独立 FEM 桌面报告](verification/original-modal-desktop.txt)和[测试源码](../tests/MotionOccModalDesktopSmoke.cpp)。

| 验证流程 | 实际断言与范围 | 界面证据 |
| --- | --- | --- |
| 保存的 CAD + native JSON → 独立模态求解 → 真实模态检查器 | 取得 4 个真实频率，显示固定接口及 4 个实际附着节点；切换模态、正负相位、附着层与播放确实改变渲染，CSV 保留真实基向量。关闭检查器后模型与瞬态结果资格不变。 | [真实模态检查器](images/original-modal-inspector.png) |
| 同一主窗口重开独立全 FEM 静力工程 → 真实静力求解 → 原动画面板 | 两个独立接口，取得 1 个平衡帧。原播放、暂停、时间滑块和播放计时器保持禁用；停止操作不错误启用时间播放。原结果含一个 FEM 体、两个接口，内部模态数为 0，不伪造 `psi` 或模态坐标。 | [原动画面板中的全 FEM 静力结果](images/original-independent-static.png) |
| 原变形参照/逐帧色标选项 → 实际 FEM 变形与云图 | 真实受力端发生位移，固定端位置保持；整体运动与弹性变形选择改变实际网格颜色和色标，切换显示参照不改变物理解。自适应选项真实传到呈现层。 | 同上 |

原始输出分别位于 `build-support/full-build/original-native-curves-smoke/` 和 `build-support/full-build/modal-desktop-smoke/`。模态 CSV 是基向量导出；该结果不代表外部 MNF 生成或第 7 节结构载荷导入流程已实现。

### 8.3 原载荷导出三页与实际载荷文件

`MotionOcc.OriginalLoadExport` 修复结果树入口后，单项复测与最终九项桌面整批均已通过。[原载荷导出桌面报告](verification/original-load-export-desktop.txt)采用最终整批输出，记录六项通过；测试源为 [MotionOccOriginalLoadExportSmoke.cpp](../tests/MotionOccOriginalLoadExportSmoke.cpp)。测试使用实际项目、Netgen 网格、原方案的队列求解入口、原结果树节点和原文件保存对话框，逐一操作三张原导出页。

| 原界面流程 | 实际验证结果 | 界面证据 |
| --- | --- | --- |
| 手动导出页：实际运动结果 → 视口拾取目标节点 → CSV | 101 个接受帧全部导出；保留原来源、端点、分量、缩放和名称。实际命中的全局节点为 5，逐帧使用运动体 pose 换算目标位置，校验 `M_target = M_marker + (p_marker - p_target) × F`，并核对秒、mm、N、N·mm 单位。 | [原手动载荷导出页](images/original-load-manual.png) |
| 特征工况页与同步工况页 | 两种模式分别导出 11 个实际峰值工况；校验所选关节/运动体、接受帧时间、初始体坐标轴、同步语义与 N/N·mm JSON，结果与真实后端逐项一致。 | 操作与文件断言见[专项报告](verification/original-load-export-desktop.txt)。 |
| 相同 CAD/Netgen 网格 → 导入原 Static JSON → 原方案静力求解 → CSV/BDF | 获得一个实际平衡帧；校验 1 kg 重力的 9.81 N 支撑力以及拾取节点处的偏心力矩。原 CSV 与 Nastran BDF 按钮均导出真实载荷，BDF 包含实际节点号、工况 SID、FORCE/MOMENT 载荷卡。 | [原静力 BDF 导出页](images/original-load-static-bdf.png) |

原始文件位于 `build-support/full-build/original-load-export-smoke/`。这里的“Static JSON 导入”是把原运动 JSON 导入当前多体模型并完成真实静力求解；两类峰值 JSON 与 BDF 是多体侧的导出产物。它们不等同于调用原结构侧 `mMotionLoadImportPanel`，也没有代替尚未接通的结构 case/load/set/connector 自动创建服务。

## 9. 后续验证与发布记录（待填写）

下列项目不在第 8 节已经通过的结论中，待实际执行后补入构建、日志和结果：

| 验证项 | 当前状态 | 待补证据 |
| --- | --- | --- |
| 最终发布提交与受测构建对应 | 30/30 测试与报告已完成，发布提交待填写 | 补充最终发布提交与本轮受测构建的对应关系；若发布前继续修改源代码，应根据改动范围复测，不沿用旧构建证据。 |
| 完整应用高 DPI 与 Windows 实机流程 | 待验证 | 独立 picker/viewport 的双 DPI 通过不能代替完整应用高 DPI；编译通过也不能代替 Windows 实机验证。 |
| 原界面风格、停靠区宽度及完整控件显示的视觉验收 | 待持续复核 | 原上传界面对照截图、窗口尺寸、缩放设置及实际裁切结果。 |
| 第 7 节外部工具、结构静力及高级几何等缺口 | 尚未完成 | 需先有真实后端实现，再记录相应完整流程；不得仅按菜单可见或面板构造标为通过。 |

填写“通过”时应同时给出实际受测构建、输入类型、日志路径和关键断言。若某条流程只走到中间阶段，应保留其停止位置及原因；未覆盖的外部工具和第 7 节边界继续单列。
