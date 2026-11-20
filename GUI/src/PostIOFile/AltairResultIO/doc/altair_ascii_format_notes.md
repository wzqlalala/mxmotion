# Altair ASCII（Generic ASCII）官方格式考证笔记

> 📑 本目录的**导入链路参考文档**（接口规格 / Tcl 规范 / 流程 / 键空间约定）见
> [altair_import.md](altair_import.md)；H3D 导出计划见 h3d_export_via_hvtrans_plan.md。
> 本文聚焦格式本身的考证。
>
> 考证来源：Altair 官方在线文档 HyperWorks Desktop Reference Guide（2022 版，help.altair.com）——
> *Generic ASCII Reader*、*Keywords and Descriptions*、*Rules and Guidelines*、*Examples* 四节。
> 目标环境 HyperWorks 2022 / 2026 本机未安装，2026 版文档路径类似，实测仍在目标机进行（见文末 P0 清单）。
> 结论：**原 `$ALTAIR_GENERIC_ASCII`/`$NODES`/`$ELEMENTS` 结构与官方格式完全不符**。
> **2026-09-29 起已按本节官方关键字重写：`AgaWriter` 只写结果文件 `.hwascii`，网格改由 `FemWriter` 写 Nastran bulk `.fem`，两者按实体 ID 绑定（见 §3.2）。**

---

## 1. 官方格式的关键事实

1. **官方名称与头**：格式叫 **Altair ASCII format**，文件第一行必须包含 `ALTAIR ASCII FILE`（无 `$` 前缀）。推荐扩展名 **`.hwascii`**（HyperView 文件对话框只过滤 `*.hwascii`，其他扩展名需手动输入，能读）。
2. **只含结果，不含网格**：HyperView 是 results-only 后处理器，Generic ASCII Reader 是它的**结果文件**读取器——ASCII 文件里只有逐点结果数据，**节点/单元网格由单独的模型文件提供**（官方示例就是一对文件：`sample.fem` + `sample.hwascii`，演示目录 `<安装>\hwdesktop\demos\mv_hv_hg\animation\hwascii\`）。
   → 因此"网格 + 结果写进同一个 ASCII"的现行设计**在格式层面不成立**，转换链路必须改为两个输入。
3. **行语义**：
   - `#` 开头 = 注释；
   - `$` 开头 = 数据头（keyword 行）；
   - 其余行 = 数据块（数值行）。
4. **关键字**（大小写不敏感、顺序任意；一个数据头组的信息自动延续到下一个数据块，改了才生效）：

| 关键字 | 作用 | 默认值 |
|--------|------|--------|
| `$DELIMITER` | 数据分隔符（SPACE/TAB/,;: 等，**`#` 和 `$` 不可用**） | SPACE |
| `$TITLE` | 标题 | — |
| `$SUBCASE` | `= id "label"` 子工况 | 1 "Subcase 1" |
| `$BINDING` | `= NODE` 或 `ELEMENT` | ELEMENT |
| `$LAYER_INFO` | `= 层数 "层标签"` | — |
| `$ID_POOL` | ID 池名（取值取决于所用模型文件） | "Nodes" / "Elements" |
| `$SYS_ID` | 结果坐标系 | 0（全局系） |
| `$COLUMN_INFO` | `= ENTITY_ID GRID_ID`，声明数据列含义 | 缺 GRID_ID 时只认质心结果；`GRID_ID = 0` 表示质心值 |
| `$RESULT_TYPE` | `= 标签(v)` 矢量 / `(t)` 张量 / `(2t)` 2D 张量 / `(s)` 标量；可后缀 `(R/I)` 实虚部、`(M/P)` 幅值相位；节点矢量后加 **`Anim Source`** 表示它驱动变形动画 | — |
| `$FREQUENCY` | `= 1.00 Hz` 模态频率 | — |
| `$MODE` | `= 1` 模态阶次 | — |
| `$TIME` | `= 10.0 sec` 瞬态时间 | — |
| `$INCLUDE` | 内嵌外部数据文件路径 | — |

5. **数据行**：按 `$COLUMN_INFO` 声明，第一列 `ENTITY_ID`（节点/单元 ID，与模型文件一致），后接该点结果值——每点列数固定：矢量 3 列（X,Y,Z）、张量 6 列（XX,YY,ZZ,XY,YZ,ZX）、2D 张量 3 列、标量 1 列。数值自由格式，分隔符分隔。
6. **动画帧的组织**：一帧 = 一个数据头组（`$SUBCASE` + `$BINDING`/`$COLUMN_INFO`/`$RESULT_TYPE`… + `$TIME` 或 `$MODE`/`$FREQUENCY`）+ 一个数据块；多帧就是重复多组。瞬态用 `$TIME`，模态用 `$MODE`+`$FREQUENCY`。
7. **变形动画源**：节点矢量结果类型后写 `Anim Source`（如 `$RESULT_TYPE = Displacement(v) Anim Source`）；不写时第一个矢量类型默认作为动画源。

## 2. 与当前生成文件的逐项对照

| 项目 | 官方格式 | 当前生成文件 | 符合? |
|------|----------|--------------|-------|
| 文件头 | 第一行 `ALTAIR ASCII FILE` | `$ALTAIR_GENERIC_ASCII` | ❌ |
| 网格 | 不含，外部模型文件提供 | `$NODES`/`$ELEMENTS` 块 | ❌（格式中不存在） |
| 帧组织 | 重复 `$SUBCASE…$TIME/$MODE` 头组 + 数据块 | `$FRAMES`/`$FRAME`/`$END_FRAME` | ❌（关键字不存在） |
| 数据集声明 | `$RESULT_TYPE = 名(v/t/2t/s)` | `$DATASET 名 SCALAR/VECTOR/TENSOR NODE/ELEMENT` | ❌ |
| 列含义声明 | `$COLUMN_INFO = ENTITY_ID …` | 无（数据不带 ID 列，按顺序隐式对应） | ❌ |
| 数据行 | `ENTITY_ID 值…` | 纯数值（标量 8 个/行，矢量 3 个/行） | ❌ |
| 注释/结束 | `#`；无显式结束关键字 | `$END` | ❌ |
| 扩展名 | 推荐 `.hwascii` | `.ascii` | ⚠️ 可读但不规范 |

此外两个数据层面的问题（已随重写一并处理）：

- **同帧数据集重名**：原实现一帧里矢量（位移分量场）与标量（当前云图变量）都被命名为 `Displacement`。官方格式里结果类型标签即 HyperView 结果列表条目，同帧重名会互相覆盖——现在标量重名时改名 `Displacement Magnitude`（`mPostAgaModelBuilder.cpp`）。
- **帧时间语义**：官方格式没有帧名/帧号概念，帧由 `$TIME`（瞬态）或 `$MODE`+`$FREQUENCY`（模态）区分。当前数据库没有真实时间/频率元数据（P2 待办），先按瞬态处理、用帧号占位 `$TIME = <帧号> sec` 保证多步动画成立；接真实元数据后改 `AgaFrame` 的 `timeSeconds/modeNumber/frequencyHz` 即可。

## 3. 链路（已按此实现，2026-09-29）

```text
后处理数据库 ─┬─► FemWriter → 网格文件（Nastran bulk .fem：GRID/CHEXA/CTETRA/CQUAD4/CTRIA3/CROD，纯文本）
              └─► AgaWriter → 结果文件（.hwascii：$SUBCASE/$RESULT_TYPE/$TIME… + ENTITY_ID 数据行，ID 与 .fem 对齐）
                          │
                          ▼
              hvtrans（两输入合转一个 H3D；命令行参数待目标机实测，见 §4）
```

### 3.1 fem / hwascii / h3d 三者的分工（官方 Reader 页原文佐证）

- **.fem（OptiStruct deck）**：只存**网格**（GRID 节点 + CHEXA/CTETRA/CQUAD4/CTRIA3/CROD 等单元卡），是 HyperView 打开结果前必须先有的"模型文件"；
- **.hwascii**：只存**逐帧结果数值**（每帧每节点/单元 ID 的位移矢量/标量/张量），按 ID 与 .fem 网格绑定；"动画"即这些结果帧的序列（`$TIME`/`$MODE` 头组重复 N 次），变形动画由 `Displacement(v) Anim Source` 矢量驱动；
- **.h3d**：**网格 + 全部结果帧的自包含合并容器**（Altair 二进制）。单个 h3d 在 HyperView 里即可播放变形动画/画云图，无需再带 .fem。
- hvtrans 能做此合并的官方依据（Generic ASCII Reader 页原文）：
  > "This generic ASCII reader is also available in HvTrans; therefore an H3D file can easily be generated for any of the custom results in the ASCII file."
  即 hvtrans 内置同一 Generic ASCII 读取器，可将 hwascii 中的自定义结果生成 H3D；模型来源（.fem）如何传入命令行 → 目标机 `hvtrans -help` 实测（本机网络拿不到 Translators 章节）。
- 对项目链路的含义：.fem 是**中间产物**（可放临时目录，转换成功后清理），最终交付/入报告的是自包含的 .h3d。

### 3.2 .fem（Nastran Bulk Data）格式要点与 FemWriter 设计

**格式性质**：`.fem` 是 OptiStruct 求解器 deck 格式 = **Nastran Bulk Data 的事实公开文本格式**——不是受控标准但无许可门槛：语法逐卡公开于《MSC Nastran Quick Reference Guide》与《OptiStruct Reference Guide》（help.altair.com），pyNastran、meshio、gmsh 等开源实现可作参照。可视化用途只需极小子集：`BEGIN BULK` / `GRID` / 单元卡 / 最少属性卡 / `ENDDATA`，该子集数十年稳定，2022/2026 读取无兼容性顾虑。

**FemWriter 设计**（`AltairResultIO/fem_writer.h/.cpp`，输入复用 AgaModel，抽数层不动）：

- **free-field 逗号格式**（`GRID,1,,0.0,0.0,0.0`），不用固定 8 列格式——避免大 ID 与高精度坐标被 8 字符截断；
- **最少属性卡**：`MAT1` 一张；按（部件 × 单元类）各发一张 `PSHELL`（壳）/`PSOLID`（体）/`PROD`（杆）——HyperView 的 component 归组来自属性卡；
- **续行规则**：每行最多 9 个数据字段，续行标记 `+E<单元ID>` 作第 10 字段（续行首字段），跨 MSC/OptiStruct 读取器均安全；
- 节点用真实 ID，与 .hwascii 的 `ENTITY_ID` 天然对齐。

| AgaElemType | Nastran 卡 | 属性卡 | 备注 |
|---|---|---|---|
| Hex8 | `CHEXA`（G1-G8，续行） | PSOLID | |
| Hex20 | `CHEXA`（G1-G20，双续行） | PSOLID | 中节点顺序需按 Nastran 棱边序核对（P2/P4） |
| Wedge6 | `CPENTA`（G1-G3 底面，G4-G6 顶面） | PSOLID | 2026-09-29 补：实测模型存在五面体被跳过导致缺块 |
| Pyramid5 | `CPYRAM`（G1-G4 底面，G5 顶点） | PSOLID | 2026-09-29 补 |
| Tet4 | `CTETRA` | PSOLID | 单行 |
| Tet10 | `CTETRA`（G5-G10 续行） | PSOLID | 同上 |
| Quad4 | `CQUAD4` | PSHELL | 单行 |
| Tri3 | `CTRIA3` | PSHELL | 单行 |
| Rod / Beam | `CROD` | PROD | `CBEAM` 需方向矢量，先按杆降级（P2） |

**目检项（P4，HyperView 中）**：Nastran 约定 HEXA 的 G1-G4 为底面逆时针、G5-G8 顶面对应；TRIA3/CQUAD4 法向 = 节点顺序右手定则。项目连接表顺序若与约定不一致，单元会显示扭曲/翻面——需在目标机目检一次，必要时在 FemWriter 里做顺序调整。

## 4. 目标机 P0 实测清单（本机无 HyperWorks，2022/2026 在目标环境）

1. 打开安装自带演示 `<安装>\hwdesktop\demos\mv_hv_hg\animation\hwascii\sample.fem + sample.hwascii`，确认 HyperView 读取与官方文档一致；
2. `hvtrans -help` 输出存档到本目录，确定 generic→h3d 的确切参数名与"模型文件 + 结果文件"双输入写法；同时确认是否存在 **H3D → Altair ASCII 的反向输出选项**（社区见过的用法形如 `hvtrans -aaf xxx.h3d`，可把二进制 H3D 落成 .hwascii 文本）——若存在，可直接用它做我们 H3D 导出的 golden 数值对比；
3. 用最小自造样例（10 节点 .fem + 1 帧标量 + 1 帧矢量 .hwascii）走通 hvtrans → HyperView；
4. **确认 hvtrans 转换是否消耗 Altair 许可（HWU）**——hvtrans 是 HyperWorks 组成部分，技术上不可单独再分发（依赖安装树内私有 DLL），许可上也不允许随第三方软件分发；若转换消耗浮动许可，"有安装无许可"的机器上转换会失败，需在导出提示里说明；
5. 确认 2022 与 2026 两版对 .hwascii 的兼容性差异（如有）；
6. 高阶单元（10/20 节点）在 .fem 中的关键字与节点顺序（TETRA10/HEXA20 需按 OptiStruct 顺序）。

## 5. HyperView 2022 读入器实测约束（2026-09-29，192.168.8.102 逐变体验证）

用同一份数据做变体二分实测，结论已固化进 AgaWriter：

| 变体 | 与 V1 差异 | 结果 |
| --- | --- | --- |
| V1 | （基线，同旧版写出器）CRLF | ❌ `$COLUMN_INFO/$RESULT_TYPE header is mandatory` |
| V2 | 去 `#`/`$TITLE`/标签，`$SUBCASE = 1` 光杆 | ❌ `Format Error in $SUBCASE header` |
| V3 | 去 `#` 注释 + `$TITLE`，SUBCASE 带 `"标签"` | ✅ |
| V4 | V3 + `$TIME` 移到 `$RESULT_TYPE` 之后 | ✅ |
| V5 | V3 + 去掉 ` Anim Source` 后缀 | ✅ 导入 |
| V6 | V3 + `$TIME = 2`（无 sec 单位） | ✅ 导入 |
| V7 | 按结果类型分组：每种类型头只声明一次，其后连续 `$TIME`+数据 | ✅ 导入 + **全部帧云图有效** |

- **`#` 注释行与 `$TITLE` 是 V1 失败根因**（V3 相对 V1 仅删这两者即通过；2022 读入器吃到后把同组 `$BINDING/$COLUMN_INFO/$RESULT_TYPE` 判为缺失——文档宣称支持注释，实测不行，2026 待验）。
- **`$SUBCASE = <id> "<label>"` 标签必填**（V2 光杆形式报 Format Error）。
- `$TIME` 位置（组头顺序内任意处）、`sec` 单位、` Anim Source` 后缀均不影响解析；官方示例中标签可含空格（`Total Ply Thickness(s)`）。
- **多帧云图必须用 V7 分组布局**：V3~V6 的"逐帧重复 `$RESULT_TYPE`"虽能导入，但第 2 个时间步起云图为 "No Result"（同名义类型被当成新实例，未挂到既有时间步）。
- 播放一轮后回到第 1 帧颜色"变暗"不是数据问题：图例从单帧最大值（1.407E-01）自动扩成全动画全局最大值（7.034E-01 = 末帧最大，5×=时间比 10/2），花纹位置不变；动画期望固定色标，可在 Contour 的 Legend 页固定 Min/Max。
- 由此写出器最终结构 = `ALTAIR ASCII FILE` 首行 + `$SUBCASE = 1 "MxSim Animation"` 一次 + 每个结果类型一组（`$BINDING → $COLUMN_INFO → $RESULT_TYPE` 一次，其后每帧 `$TIME/$MODE` + 数据行），CRLF，无注释无 `$TITLE`。
- 另实测：Displacement Magnitude 标量集同样按 NODE 绑定逐节点给出（64 行/帧），不是单元数据。

## 6. 官方规则明文（2022 help "Rules and Guidelines"/"Keywords" 页，2026-09-29 抓取核对）

面向后续结果类型（单元/张量/模态）接入的权威依据：

- **张量分量顺序：`XX, YY, ZZ, XY, YZ, ZX`**——每点 6 值（与 `aga_data.h` 中 Tensor 注释一致，无需改码）；2D 张量 `(2t)` 每点 3 值；各类型值数：`(v)`=3、`(t)`=6、`(2t)`=3、`(s)`=1。
- **复数结果**：后缀 `(R/I)`（实部/虚部）或 `(M/P)`（幅值/相位，相位单位为度）；行内先写**所有分量的实部（或幅值），再跟所有分量的虚部（或相位）**——值数翻倍：`(v)`=6、`(t)`=12、`(2t)`=6、`(s)`=2。当前未实现，频响类结果接入时按此格式。
- **模态**：`$SUBCASE` + 每步 `$MODE = n`、`$FREQUENCY = f [单位]`（单位空格分隔可省），替代 `$TIME`；写法与 AgaWriter 现有分支一致。无 `$SUBCASE` 且无 `$TIME` → 静态单子工况。
- **绑定默认值 = ELEMENT**：省略 `$BINDING` 时按单元结果处理（AgaWriter 始终显式写出，不受默认影响）。
- **单元结果**：`$COLUMN_INFO = ENTITY_ID` 行首为 .fem 单元号，质心数据每单元每类型 1 组值；行顺序无关（按 ID 键合）。角点数据用 `$COLUMN_INFO = GRID_ID`，角点顺序必须与单元连接顺序一致、`GRID_ID = 0` 表示质心值。
- **头部延续规则（V7 分组布局的官方依据）**：*"If the header block does not contain any information about the Subcase, Simulation, and Data Type, then the same information is taken from last header read."* —— 后续数据块缺省头信息时继承上一个头块；因此组头声明一次后连续 `$TIME`+数据是官方预期写法，逐块重复 `$RESULT_TYPE` 反而是"重新声明"（实测生成新实例 → 第 2 步起 No Result）。
- **⚠️ BINDING 不在延续范围内（2026-09-29 实测补充）**：延续规则明文只列 Subcase/Simulation/DataType 三项。第二组不写 `$BINDING` 时读入器按**默认值 ELEMENT** 处理（实测：NODE 绑定的节点应力被当成单元结果，行 ID 恰好 1~27 对上 27 个单元）。**每个组都必须显式重发 `$BINDING/$COLUMN_INFO`**（官方示例即每块完整重复），AgaWriter 已照此实现。
- **重复记录**：同一 loadstep 内同实体出现重复行时只取最后一个有效值。
- **空行忽略**（V7 组间空行无害）；关键词大小写不敏感；组头内关键词顺序任意。
- **多动画源允许**（多个节点矢量都标注 Anim Source 合法）；`$SYS_ID`：0=全局系（默认），-1=单元坐标系。
- `#` 注释：官方文档支持，但 2022 实测破坏后续头部解析（见 §5），维持禁用。

