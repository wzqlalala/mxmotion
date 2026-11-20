# 项目动画数据获取 与 H3D 数据对照笔记

> 整理自 H3D 导出功能（`AltairResultIO/` + `mPostAgaModelBuilder` + `mPostExportWord::exportAnimationH3D`）开发过程中对项目动画数据链路的调研，供后续扩展（张量/矢量导出、OP2 备选路线、模态元数据）参考。
> 相关代码：`Basic/RebuildBasicData/src/PostData/mPostDataBase.h`、`mPostModelData.h`、`Basic/RebuildRender/src/mxr/view/ZsEngine_AnimationWindow.cpp`、`src/PostIOFile/mPostAgaModelBuilder.cpp`、`src/MainWindow/mPostExportWord.cpp`

---

## 1. 项目里"动画数据"的两条链路

项目中的动画导出存在两条**相互独立**的链路，本质区别是"录屏幕"还是"取数据"：

### 1.1 屏幕录制链路（AVI/MP4/GIF，已上线）

```text
动画面板(mPostAnimation)设置动画类型/变量
        │  驱动视口播放（变形/模态/瞬态/多体...）
        v
mPostExportWordPanel::slot_exportAnimation()
        │  RecordAnimation(fNo, sNo, 背景色, 文件名, 目录, .后缀)
        v
ZsEngine_AnimationWindow（渲染引擎，离屏相机逐帧渲染）
        │  逐帧 PNG 落到 <目录>/<文件名>/ 子目录
        v
EndAnimation → AnimationSystem::createAnimationFile → ffmpeg 合成视频
```

关键事实（均从源码实测/读码确认）：

- **录制入口**：`REBUILD_PostRend::getAnimationWindow()->RecordAnimation(int start, int end, const glm::vec3& bgcolor, const std::string& fileName, const std::string& filePath, const std::string& suffix)`，suffix 原生支持 `.avi/.gif/.mp4/.mpeg`（`ZsEngine_AnimationWindow.h:19`）；
- **首帧陷阱**：引擎绘制回调里，第一帧记录的是"开始录制那一刻视口正在显示的动画帧"，录制前必须先把视口定位到起始帧（`postRend->getPostRender()->toAnimation(fNo)`），否则序列起始位置错误；
- **离线缓冲背景**：离线相机清屏色曾经硬编码（`AnimationSystem.cpp` 的 `initAniamtionCamera`），`bgcolor` 参数只作用于主视口渐变背景；现已改为 `beginRecord` 时跟随 bgcolor（白底 → 深色图例文字可见）；
- **裁剪区域**：`setAnimationRect` 的离线缓冲就是整个视口内容，必须传 `(0, 0, W, H)`，传 MDI 子窗口坐标会导致画面错位/宽高比错误；
- **ffmpeg 依赖**：编码用 `<程序目录>/ffmpeg/ffmpeg.exe`（组件默认值 `ZsEngine_Components.h:217`），目录结构必须平铺；视频编码是 `mpeg4`（MPEG-4 Part 2），浏览器 `<video>` 不能直接播，HTML 报告里已做 `h264_mf` 转码；
- **分辨率**：`setAnimationSize(W, H, scale)`，scale 可放大离线缓冲（上限 8192），宽高需为 2 的倍数；
- **帧范围**：来自 `mInfoWindow::slot_aniStartFrame()/slot_aniEndFrame()`（动画面板当前设置的起止帧）。

**这条链路只有像素，没有结果数据——所以它永远产出不了 H3D。**

### 1.2 结果数据链路（本次为 H3D 新建）

动画的"数据本体"一直在后处理数据库里：每帧的位移场 + 各类结果数组。云图/矢量/变形/模态/瞬态/多体动力学等动画，都是渲染端拿到"当前帧的数据"后在视口呈现的。数据导出就是把数据按帧写出来，与渲染无关。

---

## 2. 动画数据获取（数据库 API）

### 2.1 帧管理

| API | 说明 |
|-----|------|
| `mPostDataBase::getFrameIds()` | 全部帧号列表。**注意：基于 QHash，键顺序不确定**——使用前必须 `std::sort` 升序排序（H3D 导出已修） |
| `mPostDataBase::getModelDataByFrameID(int)` | 取该帧的 `mPostModelData`（网格 + 该帧派生数据） |
| `getVariableByFrameID(int)` | 该帧可用的变量名列表 |
| `getVariableType(var)` | 变量数据位置：0=节点数据、1=单元数据 |
| `getVariableCompos(var)` | 变量的分量名列表（张量分量 Sxx/Syy/... 从这里取） |

动画帧范围（动画面板）与数据库帧号的关系：`mInfoWindow::slot_aniStartFrame()/slot_aniEndFrame()` 给出的是**起止帧序号**，对应 `getFrameIds()` 列表的下标区间，导出时按下标截取子集（见 `exportAnimationH3D()`，排序后截取）。

### 2.2 逐帧数组

| API | 返回 | 键 | 用途 |
|-----|------|----|------|
| `getDisplacementDataByFrameID(frame)` | `QHash<int, QVector3D>` | **节点真实 ID** | 位移场（变形图 / 变形动画的本体） |
| `getResultDataByFrameID(frame, flag&)` | `QHash<int, double>` | 节点/单元真实 ID（取决于当前变量类型，`getVariableType` 0=节点 1=单元） | 当前云图变量的数值场 |
| `getEleNodeResultDataByFrameID(frame)` | `QHash<int, QHash<int, double>>` | 单元 ID → 节点 ID → 值 | 单元积分点/节点插值结果 |

注意：`getResultDataByFrameID` 返回的是**当前生效变量**的场（由界面云图状态决定），一次只有一个变量；要导出多个变量需按变量切换或扩展数据库接口（P2 待办）。

### 2.3 网格（`mPostModelData`）

| API | 说明 |
|-----|------|
| `getAllPartsIDs()` | 全部部件 ID |
| `getNodeRealIds(partId)` | 部件内节点**真实 ID** 列表 |
| `getVectorById(realId)` | 节点真实 ID → 坐标 `QVector3D`（基准/未变形坐标） |
| `getElementIdsByPartId(partId)` | 部件内单元真实 ID |
| `getNodeIdsByEleIdWithoutBuild(partId, eleRid, dim&)` | 单元连接表（**节点真实 ID**）+ 维度 flag（0=点 1=线 2=面 3=体） |
| `getEleTypeByEleId(eleRid)` | `MxFunctions::ElementType`（枚举巨大，含显式单元） |
| `getAllVariableNames()` / `getComponentNamesByVariableName(var)` | 变量与分量 |

### 2.4 ID 体系（易踩坑）

- 项目里存在**两套节点 ID**：真实 ID（persistent，结果/位移哈希的键）与连续 ID（部件内紧凑化）；
- 结果哈希、`getVectorById` 用真实 ID；`getElementNodeId` 等部分接口返回连续 ID——**混用会静默取错/取空**；
- `mPostAgaModelBuilder` 统一使用**真实 ID** 作为 H3D 节点号，单元连接用 `getNodeIdsByEleIdWithoutBuild`（真实 ID）。

### 2.5 单元类型映射

`MxFunctions::ElementType` 枚举极大（显式安全带/传感器等几十种），按"维度 flag + 顶点数"归类映射到 Generic ASCII 单元类型（`mPostAgaModelBuilder.cpp` 的 `mapElementType`）：

| 维度 | 顶点数 | 映射 | 说明 |
|------|--------|------|------|
| 2 | 3 / 6 | TRIA3 | 6 节点暂按顶点导出（P2 精确 TRI6） |
| 2 | 4 / 8 | QUAD4 | 同上 |
| 3 | 4 / 10 | TETRA4 / TETRA10 | |
| 3 | 8 / 20 | HEXA8 / HEXA20 | |
| 3 | 6 | — | 五面体 P2 扩展 |
| 1 | 2 | ROD | |
| 0 | — | 跳过 | 质量点/传感器等无网格单元 |

---

## 3. H3D 数据模型与项目数据对照

H3D 的组织：**模型（网格）+ 多个子工况（simulation），每个子工况含多个结果帧，帧内是数据集（标量/矢量/张量 × 节点/单元）**。与项目数据一一对应：

| H3D 概念 | 项目对应 | 获取 API |
|----------|----------|----------|
| 模型网格（节点/单元） | `mPostModelData` 基准网格 | `getNodeRealIds` + `getVectorById` + `getNodeIdsByEleIdWithoutBuild` |
| 仿真帧（animation frame） | 数据库帧（`getFrameIds()` 的一项） | 逐帧取数组 |
| Displacement 矢量数据集 | `getDisplacementDataByFrameID` 位移场 | → **变形图 / 变形动画** |
| 标量数据集 | `getResultDataByFrameID` 当前变量场 | → **云图（标量）** |
| 张量数据集（6 分量） | 当前张量变量的各分量（`getVariableCompos`） | → **云图（张量）**（P2：需按分量逐个取值拼装） |
| 矢量数据集 | 矢量变量（3 分量） | → **矢量图**（P2） |
| 帧名 | 动画类型语义：瞬态/多体 `T=0.05s`；模态 `Mode 1 - 23.5 Hz`；线性 `Frame N` | 写帧时放入 annotation |

### 3.1 动画类型 → 导出数据对照

| 动画类型 | H3D 里需要什么 | 项目数据来源 | 现状 |
|----------|----------------|--------------|------|
| 变形图 / 变形动画 | 每帧 Displacement 矢量 | `getDisplacementDataByFrameID` | ✅ 已实现 |
| 云图（标量） | 每帧标量数据集 | `getResultDataByFrameID` | ✅ 已实现（当前变量，节点/单元自动区分） |
| 云图（张量） | 每帧 6 分量张量 | 按分量名逐个取（`getVariableCompos`） | ⏳ P2 |
| 矢量图 | 每帧 3 分量矢量 | 矢量变量 | ⏳ P2 |
| 线性动画 | 多帧（线性叠加步） | 同瞬态，帧序列 | ✅（按帧导出即可） |
| 模态动画 | 每阶一帧，名称带频率 | 帧号即模态阶次（频率元数据 P2 从帧名/结果补） | ✅ 基本可用 |
| 瞬态动画 | 时间帧序列 | 同上 | ✅ |
| 多体动力学动画 | 时间帧序列 | 同上 | ✅ |

**核心结论：写帧端不区分动画类型，"帧 + 数据集"即一切；类型语义只体现在帧命名上。**

---

## 4. H3D 导出链路（本次实现）

```text
mPostExportWordPanel::exportAnimationH3D()          [MainWindow]
  │ getFrameIds() 升序排序后截取动画面板帧范围
  v
MxPost::buildAgaModelFromPost()                     [PostIOFile/mPostAgaModelBuilder.cpp]
  │ 网格(真实ID) + 每帧 Displacement/Scalar → altairresultio::AgaModel
  v
altairresultio::AgaWriter::write()                  [PostIOFile/AltairResultIO/aga_writer.cpp]
  │ Altair Generic ASCII 明文（格式常量集中在 AgaFormat 表，P0 实测后冻结）
  v
hvtrans.exe（HyperWorks 自带，findHvTransExe 检测）
  │  -ascii in.ascii -h3d out.h3d（参数名待 P0 实测修正）
  v
报告动画<时间戳>.h3d
```

与屏幕录制链路的关系：**互补而非替代**——AVI/MP4/GIF 给报告直观展示；H3D 给分析工程师在 HyperView 里交互查看（可旋转、改云图）。两者可同时导出（H3D 走 `exportAnimationH3D` 分支，不影响录制链路）。

---

## 5. 已知限制与后续工作

1. **格式冻结**：Generic ASCII 块语法常量（`aga_writer.cpp` 的 `AgaFormat`）与 hvtrans 命令行参数模板需在安装 HyperWorks 后实测确认（方案文档 P0）；
2. **单变量限制**：`getResultDataByFrameID` 只给当前变量；多变量/张量导出需按变量循环取分量拼装（P2）；
3. **流式写出**：大模型多帧时 `AgaModel` 整体驻留内存，P2 提供 `AgaWriter` 流式接口边取边写；
4. **模态元数据**：频率/阻尼目前不在帧名里，需从结果数据库补充（P2）；
5. **高阶单元**：6/8 节点高阶壳暂按顶点数降级导出，精确类型映射 P2。
