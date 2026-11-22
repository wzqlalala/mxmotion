# 原生视口：Motion 渲染与鼠标交互借鉴

日期：2026-11-22。

本次在 `mxmotion/lite-occ` 的原生 Qt 视口中接入曲面显示法向、遵守场景深度的 CAD 拾取反馈，以及可切换的 NX 鼠标导航。

参考仓库：[wzqlalala/Motion](https://github.com/wzqlalala/Motion)，固定提交 `b636535a45895b193aa581afd4df10a9004c9b6e`，分支 `codex/multi-solver`。
目标修改基线：`99a0a1a324cb0bf39d0532daee5cecec8eb36902`。

## 渲染改动

- CAD 圆柱使用放置坐标系中的解析径向法向；其他面使用仅在该 OCC 面内累加的双精度面积加权法向。锐边处不跨面平滑，面反向和镜像沿实际三角形绕序校准。
- `DisplayMesh` 在原有字段后增加可选 `normals`。顶点、三角形、边和拓扑编号沿用原顺序；旧显示快照缺少法向时保留回退路径。FEM 继续显示单元平面法向。
- CAD 面填色、边高亮、顶点与参考位置标记进入真实 OpenGL 深度测试，不再通过 QPainter 直接覆盖前方实体。参考位置仍与拾取控件回填值一致，例如圆边对应圆心。
- 单面/单边高亮按拓扑 ID 缓存，整部件复用已有显示缓冲。场景替换和 OpenGL 上下文释放时清理缓存；隐藏、抑制或已用变形网格显示的部件不重绘原始 CAD 高亮。
- 表面附着边使用当前 Qt FBO 深度附件精度计算的两个量化单位偏移，替代固定 NDC 偏移；轮廓与线宽继续采用屏幕像素口径。
- 普通选择模式增加鼠标悬停预选，仅改变视图显示，不发出部件选择信号。

参考实现主要是 `src/ui/cad/CadMeshRenderer.cpp::Upload()/Draw()` 和 `src/ui/render/ScreenStrokeRenderer.h`。当前目标已经有两面光照、屏幕空间线宽、轮廓和基础轨迹球，本次在这些基础上补齐差异。

CAD 反馈遵守实体遮挡；已有 FEM 节点贯穿框选及相应节点/连接提示继续保留。线框模式没有实体面遮挡，允许看到后侧线段。

## 鼠标操作

入口：**视图 → 鼠标导航 → 经典导航 / NX 导航**。默认仍为经典导航。

| 操作 | 经典导航 | NX 导航 |
| --- | --- | --- |
| 选择 | 左键单击 | 左键单击 |
| 旋转 | 左键拖动 | 中键拖动 |
| 平移 | 中键或右键拖动 | Shift＋中键拖动 |
| 缩放 | 滚轮，锚定光标位置 | 滚轮，或 Ctrl＋中键拖动 |
| 旋转中心 | 沿用当前视图中心 | 双击可见表面，保持当前投影不跳动 |
| 适合全部 | 原有按钮或 F | 原有按钮/F；双击空白或 Ctrl＋双击 |

NX 模式中，拖动时切换 Shift/Ctrl 会重新设置锚点，切换事件本身不移动相机；Ctrl 优先于 Shift。第二个不同鼠标按钮不会覆盖当前手势。焦点丢失、鼠标捕获丢失、窗口停用/隐藏/禁用会终止当前手势和框选预览，保留已接受的拾取值。

节点拾取中的 Ctrl＋左键贯穿框选、右键撤回、空闲时 Esc 结束拾取保持原语义；导航中的 Esc 先停止导航。活跃拾取期间双击不额外提交引用或改变旋转中心。

参考 `SceneViewportInteraction.cpp`、`SceneViewportSelection.cpp` 和 `navigation/ViewportCamera.cpp`，适配了目标现有 world-to-eye 四元数、毫米和正平移约定。表面双击设置中心是结合参考“保持屏幕位置的旋转参考”接口增加的适配操作，不是逐字照搬的参考快捷键。

## 已完成验证

实际环境：Linux、Qt 5.15.15、OCCT 7.8.1、桌面 OpenGL 3.3 Core、Mesa 软件渲染。没有几何、拾取或渲染替身。

| 验证项 | 结果 |
| --- | --- |
| 新增 DisplayNormals | 通过：解析圆柱、箱体锐边、放置/反向/镜像、0.001 mm 球体 |
| 既有 NativeTopology | 通过：真实 STEP、裁剪几何、全局路径、独立曲线、指纹重绑定、歧义拒绝 |
| 既有 Picking，DPR=1 / DPR=2 | 两次通过：CAD/FEM 真实编号、参考点/命中点、pose、节点框选、标量与拾取事务 |
| 新增 Navigation，DPR=1 / DPR=2 | 两次通过：经典/NX 手势、光标锚定、修饰键切换、中断、旋转中心、原拾取兼容 |
| 新增 RenderFeedback，DPR=1 / DPR=2 | 两次通过：被遮挡面/边/顶点不透出，可见反馈、pose、隐藏/显示、缓存更新及清理 |
| 修改的主窗口、Ribbon、main 和 MOC | 真实对象文件编译通过 |

桌面测试从实际原生桌面启动，读取真实 OpenGL framebuffer 断言像素行为。原始运行日志：[DPR=1](verification/native-render-navigation-dpr1.txt)、[DPR=2](verification/native-render-navigation-dpr2.txt)。

本轮编译链接了实际视口/CAD组件和测试程序，并编译了修改的主窗口、Ribbon及入口；没有重新链接完整 MotionEngine 应用或执行求解器全量回归。Windows 实机未验证。

## 正式回归入口

在完整依赖和已初始化子模块的工程中配置 `BUILD_TESTING=ON`、`MX_TEST_DESKTOP=ON` 后：

```bash
cmake --build build-occ --parallel 3
ctest --test-dir build-occ -R 'MotionOcc\.(DisplayNormals|NativeTopology|Picking|Navigation|RenderFeedback)$' --output-on-failure
```

桌面测试需要可用的桌面/OpenGL 会话。可分别执行：

```bash
build-occ/bin/MxMotion --picking-smoke build-occ/picking-smoke
build-occ/bin/MxMotion --navigation-smoke build-occ/navigation-smoke
build-occ/bin/MxMotion --render-feedback-smoke build-occ/render-feedback-smoke
```

Linux 高 DPI 检查可在同一可用桌面下为测试进程设置 `QT_SCALE_FACTOR=2`。该环境变量仅作用于对应进程，不修改用户系统缩放设置。

## 后续可继续借鉴的部分

参考仓库还包含透明物体排序、视口显示过滤、候选循环和三轴导航。这些需要与当前对象、显示选项和界面入口进一步适配，本次未把它们标为已迁入。特别是透明显示需要完整的不透明/透明绘制顺序、深度写入规则与排序，不能只给表面添加 alpha。
