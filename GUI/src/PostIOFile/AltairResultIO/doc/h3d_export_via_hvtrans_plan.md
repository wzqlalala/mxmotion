# H3D 结果导出方案（Mesh + Frame 数组 → Altair Generic ASCII → HvTrans → H3D）

> 模块：`PostIOFile/AltairResultIO`（PostIOFile 子目录）
> 目标：将后处理的网格与逐帧结果数组写出为 Altair Generic ASCII 文本格式，再用 HyperView 自带的 HvTrans 命令行工具转换为 H3D，从而支持导出云图（标量 & 张量）、矢量图、张量图、变形图以及变形动画、线性动画、模态动画、瞬态动画、多体动力学动画。
> 文档状态：已实现（P1/P3 完成，P0 格式实测待 HyperWorks 环境）
> 关联文档：`doc/report-export-image-animation-design.md`、`doc/animation_data_and_h3d_mapping.md`

---

## 1. 背景与总体思路

### 1.1 为什么走这条链路

- H3D 是 Altair 私有的结果数据格式（逐帧网格变形 + 场数据），无公开写库，进程内直写不可行（详见途径分析）；
- HyperView 自带的 **HvTrans** 命令行工具支持把多种求解器/通用格式转换为 H3D，其中包括 **Altair Generic ASCII**（明文文本格式）这类通用交换格式；
- 明文格式无需许可库即可写出，转换由官方工具完成，格式正确性有保障。这是不引入 Altair 私有库的前提下可行性最高的路线。

### 1.2 总体链路（已实现）

```text
+----------------------------+     +----------------------------+     +----------------+     +------+
| 后处理数据库                |     | PostIOFile/AltairResultIO  |     | HvTrans (官方)  |     | H3D  |
| mPostDataBase 的           | --> | AgaModelBuilder 抽数据      | --> | (QProcess 调用) | --> | 文件 |
| Mesh + Frame 数组           |     | AgaWriter 写 .ascii 文本    |     | 命令行转换       |     |      |
+----------------------------+     +----------------------------+     +----------------+     +------+
入口：mPostExportWordPanel::exportAnimationH3D()（报告面板动画格式选 H3D）
```

### 1.3 动画类型在 H3D 中的表达

H3D 的"动画"本质是**同一网格上的多个结果帧（simulation frames）**。各动画类型只是帧序列的语义不同，写出端不需要区分（类型信息放在帧命名/元数据里）：

| 动画类型 | 写出方式 | 现状 |
|----------|----------|------|
| 云图（标量） | 每帧一个节点/单元标量数据集（当前变量） | ✅ |
| 云图（张量） | 每帧 6 分量张量数据集 | ⏳ P2（按分量拼装） |
| 矢量图 | 每帧 3 分量矢量数据集 | ⏳ P2 |
| 变形图 / 变形动画 | 每帧 Displacement 矢量数据集 | ✅ |
| 线性 / 瞬态 / 多体动力学动画 | 连续时间帧序列 | ✅ |
| 模态动画 | 每阶一帧（频率元数据 P2 补充到帧名） | ✅ 基本可用 |

---

## 2. 格式：Altair Generic ASCII

### 2.1 关键事实与待确认项（P0 任务，待 HyperWorks 环境）

- Altair Generic ASCII 是 HyperWorks 定义的**明文结果交换格式**，HyperView/HvTrans 原生支持导入；
- 精确的块语法、关键字拼写、单元类型枚举名，以**本机安装的 HyperWorks 版本文档**为准：
  1. 《HyperWorks Reference Guide》HvTrans / Altair Generic ASCII 章节；
  2. `hvtrans -help` 与 HvTrans 安装目录下的示例；
  3. 用 HyperView 手工导出参考样例对照（推荐）。
- **P0**：手工最小样例（10 节点 + 壳/实体单元 + 1 标量帧 + 1 位移帧）→ HvTrans → HyperView 确认可读。
- **格式常量已集中**在 `aga_writer.cpp` 的 `AgaFormat` 结构 + `elemTypeKeyword()`，实测后只改这一处；hvtrans 命令行参数模板在 `mPostExportWord::exportAnimationH3D()` 与 `AltairResultIO/hvtrans_job.cpp` 各有一份。

当前实现的块结构（待实测对齐）：

```text
$ALTAIR_GENERIC_ASCII
$MODEL_NAME = MxSim
$NODES
<id> <x> <y> <z>
$ELEMENTS
<id> TRIA3|QUAD4|TETRA4|HEXA8|TETRA10|HEXA20|ROD|BEAM <n1> <n2> ...
$FRAMES
$FRAME <index> <name>
$DATASET <name> SCALAR|VECTOR|TENSOR NODE|ELEMENT ["annotation"]
<values...>
$END_FRAME
$END
```

---

## 3. 代码结构（PostIOFile/AltairResultIO）

```text
src/PostIOFile/
├── AltairResultIO/
│   ├── aga_data.h        # AgaModel/Node/Element/Frame/Dataset + 枚举（无 Qt 依赖）
│   ├── aga_writer.h/.cpp # AgaWriter::write/validate + AgaFormat 格式常量表
│   ├── hvtrans_job.h/.cpp# HvTransJob::available/convertToH3d（std::system 版）
│   ├── doc/              # 本方案 + 数据对照笔记
│   └── tests/            # 预留
├── mPostAgaModelBuilder.h/cpp  # 数据源适配器 + findHvTransExe()（Qt 侧）
└── CMakeLists.txt        # GLOB 已含 AltairResultIO/*.cpp 与 include 路径
```

导出宏约定：类/函数用 `MIOFILE_EXPORT`（PostIOFile 模块宏），因为源码直接编入 PostIOFile DLL。

### 3.1 适配器（mPostAgaModelBuilder）

- `buildAgaModelFromPost(postDB, frameIds, out, error, skipped)`：
  - 网格：第一帧 `getModelDataByFrameID` → `getAllPartsIDs` → `getNodeRealIds` + `getVectorById` + `getElementIdsByPartId` + `getNodeIdsByEleIdWithoutBuild`（返回真实 ID 连接表 + 维度 flag）；
  - 位移：`getDisplacementDataByFrameID(frame)` → Displacement 矢量数据集；
  - 标量：`getResultDataByFrameID(frame, flag)` + `getVariableType`（0=节点 1=单元，自动选 Nodal/Elemental 位置并按对应实体顺序取值）；
  - 单元类型映射：维度 flag + 顶点数 → TRIA3/QUAD4/TETRA4/TETRA10/HEXA8/HEXA20/ROD，不支持的跳过并统计；
- `findHvTransExe()`：`%ALTAIR_HOME%\hwdesktop\hv\bin\win64\hvtrans.exe` → Program Files 下 Altair 版本目录倒序扫描。

### 3.2 导出入口（exportAnimationH3D）

`mPostExportWord.cpp`：帧范围（`mInfoWindow` 起止帧 → `getFrameIds()` **升序排序后**按下标截取，QHash 键序不确定必须排序）→ 抽数据 → 写 ASCII → QProcess 调 hvtrans → `报告动画<时间戳>.h3d`。hvtrans 缺失时保留 .ascii 并提示可手动转换。

---

## 4. 分阶段状态

| 阶段 | 内容 | 状态 |
|------|------|------|
| P1 | 模块骨架 + 网格/标量/位移写出 + 单测 | ✅ 代码完成（golden 对比随 P0 一起做） |
| P2 | 多帧动画（已含）+ 矢量 + 张量 + 高阶单元 + 流式写出 | ⏳ 张量/矢量待做 |
| P3 | RebuildGUI 集成（适配器 + H3D 导出项 + hvtrans 检测/调用） | ✅ 代码完成 |
| P0 | 格式实测（需 HyperWorks 环境） | ⏳ **阻塞项，环境就绪后优先做** |
| P4 | 真实模型回归 | ⏳ |

## 5. 风险与对策

| 风险 | 对策 |
|------|------|
| Generic ASCII 语法与假设不符 | P0 实测（原 OP2 备选路线随 NastranVizIO 子模块下线移除） |
| hvtrans 依赖 HyperWorks 安装/许可 | 导出前检测 + 明确提示；.ascii 保留可手动转换 |
| 大模型 ASCII 体积/耗时 | 精度选项；P2 流式写出 |
| 高阶/特殊单元 | 按维度+顶点数降级映射；不支持项统计报告 |
| 模态/瞬态在 H3D 中的语义 | P0/P2 用 HyperView 实测帧命名与子工况组织 |

## 6. 待确认清单

1. 本机 HyperWorks 版本与 hvtrans.exe 路径、`hvtrans -help` 输出存档；
2. Altair Generic ASCII 官方格式文档章节存档到 doc/；
3. 目标模型规模与可接受导出耗时；
4. 单元类型清单（按现有测试模型统计）；
5. 模态频率/阻尼元数据是否需要写入帧名；
6. 多结果类型分文件还是合文件。
