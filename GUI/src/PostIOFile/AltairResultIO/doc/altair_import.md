# Altair H3D / .hwascii / .fem 导入链路 —— 完整参考

> **已退役（2026-10-14）**：本文描述的 Altair 导入链（`mIOAltairFile` /
> `hyperview_job` / `aga_reader` / `fem_reader` / `altair_post_data_builder`）
> 已随 MxNativeReaders 原生读取链路接入而删除；其 NastranVizIO 依赖
> （`cmake/FindNastranVizIO.cmake`）也已于 2026-10-15 随子模块下线移除。
> 本文仅作历史接口参考保留，§3 的 Tcl 脚本规范仍适用于 `tools/h3d_export.tcl`
> 手工调试。

> 导出链（`AgaWriter`/`FemWriter` → hvtrans → H3D，见 `altair_ascii_format_notes.md`）
> 的对称实现：把 Altair 系后处理数据导入本项目后处理数据库。
> 本文是本次任务的**接口规格 + Tcl 规范 + 流程 + 理解辅助**的汇总，面向后续维护者。

---

## 1. 文件清单与职责（全部在本目录）

| 文件 | Qt 依赖 | 职责 |
|---|---|---|
| `mIOAltairFile.{h,cpp}` | Qt（QThread） | **对外调用接口**。H3D 模式（线程内先转换）或 .fem+.hwascii 文本对模式 |
| `hyperview_job.{h,cpp}` | 无 | h3d → .hwascii + .fem 转换流程控制（定位 exe、ASCII staging、脚本生成、成功判定） |
| `aga_reader.{h,cpp}` | 无 | .hwascii 结果解析（与 `AgaWriter` 写出布局对称） |
| `fem_reader.{h,cpp}` | 无 | .fem 网格解析（复用 NastranVizIO `read_deck`）+ DeckModel→AgaModel 映射 |
| `altair_post_data_builder.{h,cpp}` | Qt | 结果帧/变量/分量注册入数据库 + 云图懒读 |
| `tools/h3d_export.tcl` | — | HyperView 批处理导出脚本（手工调试用；逻辑与 `hyperview_job.cpp` 内嵌模板**必须同步**） |
| `tools/h3d_export_mock_test.tcl` | — | 导出脚本端到端 mock 测试（stub hwi 句柄链，17 项断言；兼校验两处同步逐字一致） |
| `tests/reader_roundtrip_test.cpp` | 无 | 写→读回环冒烟测试（21 项断言，2026-10-08 全部通过） |

模块分层约定：`AltairResultIO` 内除 `mIOAltairFile` 外全部无 Qt（std::string +
UTF-8 路径约定，Windows 内部转宽字符打开，见 `utf8_file.h`），与 hvtrans 导出
那半边保持同一风格。

---

## 2. 对外接口规格

### 2.1 `MIOFile::mIOAltairFile`（QThread，应用层入口）

```cpp
// 模式一：H3D 自动转换（线程内经 HyperViewJob 转出 .hwascii/.fem，需 HyperWorks）
mIOAltairFile(const QString& h3dFile,
              QPointer<PostData::mPostDataBase> modeldb,
              QObject* parent = nullptr);

// 模式二：现成文本对（跳过转换，无 HyperWorks 依赖）
mIOAltairFile(const QString& femFile,
              const QString& hwasciiFile,
              QPointer<PostData::mPostDataBase> modeldb,
              QObject* parent = nullptr);
```

- 调用模式与 `mIOP2File` 完全同构：`start()` 起线程，结果走信号回 GUI。
- **数据库命名**：`modeldb` 由调用方以主文件路径命名（H3D 模式 = .h3d 路径，
  文本对模式 = .hwascii 路径）；线程内 `setFile(同路径)`，该路径同时是 GUI 懒读
  分发的注册键。
- 信号：
  - `readFileSuccessSig(postName)` — 成功；
  - `readAltairErrorSig(MIOFile::IOFileErrorType)` — 失败类型；
  - `finishedAltairFileSig(postName)` — 总结束，**失败时携带空串**（GUI 据此回滚
    界面状态并删库，与 OP2 收尾一致）。
- 云图懒读：
  ```cpp
  bool readVarDatas(int frameIndex, const QString& variableName,
                    const QString& componentName, const QString& complexMode) const;
  ```
  从内存中的解析结果按需重发布指定分量（complexMode 当前忽略，文本结果只有实数）。
- 静态辅助：
  - `findCompanionHwascii(femFile)` — 找同名 `.hwascii`（候选：`X.hwascii`、
    `X.hwascii.txt`），找不到返回空串；
  - `tryGetAltairSelection(fileNames, &primary, &companion)` — 多选识别：
    | 选择内容 | 判定 | 返回 |
    |---|---|---|
    | 单个 .h3d | H3D 模式 | primary=.h3d，companion=空 |
    | 一个 .fem(/.bdf) + 一个 .hwascii | 文本对 | primary=.hwascii，companion=.fem |
    | 单个 .hwascii 且同目录同名 .fem 存在 | 文本对 | 同上 |
    | 其他任何组合 | 非 Altair，返回 false | 调用方继续原有分发 |

### 2.2 纯格式层接口（无 Qt，可脱离主程序使用）

```cpp
namespace altairresultio {
    // .hwascii → 结果结构（帧跨组聚合、实体 ID 原样保留）
    static bool AgaReader::read(const std::string& utf8Path,
                                AgaResultFile& out, std::string& error);

    // .fem → NastranVizIO DeckModel（数据库链路用）
    static bool FemReader::readDeck(const std::string& utf8Path,
                                    nastranvizio::deck::DeckModel& out, std::string& error);
    // .fem → AgaModel（纯格式层用；部件按 Property 分组，不支持的单元跳过）
    static bool FemReader::read(const std::string& utf8Path, AgaModel& out, std::string& error);

    // h3d → .hwascii + .fem
    static std::string HyperViewJob::locateHyperViewExe();  // 查找 hyperview.exe
    static bool HyperViewJob::available(const HyperViewOptions& opt);
    static bool HyperViewJob::convertH3d(const std::string& h3dPath,      // UTF-8
                                         const std::string& hwasciiPath,  // UTF-8
                                         const std::string& femPath,      // UTF-8
                                         const HyperViewOptions& opt,
                                         std::string& error);             // 失败时含日志摘要
}
```

`HyperViewOptions`：`hyperViewExe`（空则自动查找）、`workDir`（staging 父目录，
空则系统临时目录）、`timeoutMs`（P1 未生效，`std::system` 同步阻塞，预留）。

### 2.3 错误码（`mIOEnum.h` 新增，GUI 按码出双语提示）

| 码 | 名称 | 语义 |
|---|---|---|
| 0x2000 | `AltairConvertFailed` | H3D 转换失败（缺 HyperWorks / 许可证 / 脚本失败） |
| 0x4000 | `AltairCompanionNotFound` | 未找到配套 .fem/.hwascii |
| 0x8000 | `AltairParseFailed` | .hwascii 或 .fem 解析失败 |
| 0x10000 | `AltairUnsupportedData` | 无受支持的结果帧 |
| 0x20000 | `AltairModelMismatch` | 结果与网格模型不匹配（网格无节点/单元） |

### 2.4 GUI 分发点（`MainWindow/mPost3DResult.{h,cpp}`）

- `filterFiles`：`tryGetAltairSelection` 判定成功即走 `importAltairFile`
  （**排在 OP2 判定之前**，两类文件扩展名本就互斥）；
- `importAltairFile(primary, companion)`：建库 → 起 `mIOAltairFile` →
  `_altairReaders[主文件路径]` 登记 → 连接 `slot_altairThreadFinished`；
- `slot_altairThreadFinished`：失败（空串）删库摘除读取器、复位光标/进度条；
  成功发 `finishedReadPostFileSig`；
- `altairReaderFor(postFile)` / `isLazyOp2Result(postFile)`（后者现同时覆盖
  OP2 与 Altair）；三处云图懒读分支统一"Altair 优先、其次 OP2"；
- 目录扫描过滤器增加 `*.h3d`、`*.hwascii`（`readPostResultFile`）。

---

## 3. `tools/h3d_export.tcl` 规范

### 3.1 运行方式

```bat
cd /d <脚本目录>
"E:\SoftwareInstall\HyperWorks2022\hwdesktop\hv\bin\win64\hyperview.exe" -b -tcl h3d_export.tcl
```

- `-b` 批处理无界面、跑完自动退出；启动即签出 HWU 许可证（与 hvtrans 同模式）；
- 全程纯 ASCII 路径（hvtrans 同族的 0xC0000409 崩溃风险）；
- 先改脚本头部 CONFIG 段的四个路径。

### 3.2 CONFIG 协议（产品调用方按此生成脚本）

```tcl
set H3D_IN    "<输入 h3d>"       # 纯 ASCII 路径（非 ASCII 由 HyperViewJob 预先搬入 staging）
set OUT_HWA   "<输出 .hwascii>"
set OUT_FEM   "<输出 .fem>"
set LOG_PATH  "<日志路径>"
```

`hyperview_job.cpp` 内嵌同一份脚本主体（CONFIG 之外的全部内容），运行时前缀
生成 CONFIG 写入 staging。⚠️ **同步的两个硬性要求**：

1. 修改脚本逻辑后必须两处同步（文件头部均有提醒注释）；
2. 同步脚本 `tools/h3d_export_mock_test.tcl` 会校验两处逐字一致；
3. **本机 MSVC（14.44，/utf-8）实测字符串字面量上限约 16KB（低于文档的
   64KB，C2026）**，因此内嵌主体按行切成 3 段相邻原始字面量 `R"TCLBODY(...)TCLBODY"`
   自动拼接。重新生成同步内容用 Python 按 `keepends` 行界切块（每块 <14000
   字节），切勿整段替换。

另外 C++ 生成 CONFIG 时会把路径统一改写为正斜杠：Tcl 双引号串会吞反斜杠
（`"C:\Temp\in.h3d"` → `"C:Tempin.h3d"`），而 `GetTempPathA` 返回的是反斜杠
路径——不归一化则产品内转换必挂（2026-10-08 复查发现并修复）。

### 3.3 脚本结构（六节）

1. **会话句柄链**：`hwi OpenStack → session → project → page → window →
   SetClientType "HyperView" → GetClientHandle client`（"HyperView" 失败时用
   "HV" 复用已建句柄重试，两种 client 类型拼写都兼容）；
2. **打开 H3D**：`client OpenFile <path>` → `client OpenFile "H3D" <path>` →
   `client AddModel <path>` 逐个尝试；成功后 `WaitForResults`；
3. **模型句柄**：官方形式 `client GetActiveModel` + `client GetModelHandle model $idx`
   （2022 帮助文档 SetQuery 示例确认）；
4. **写 .fem**（官方查询字段，poIQueryCtrl）：
   - `q SetQuery "node.id node.coords"` → GRID,id,,x,y,z（free-field）；
   - `q SetQuery "element.id element.config element.connectivity"` →
     CTETRA/CHEXA/CQUAD4/CTRIA3/CPENTA/CPYRAM/CROD/CBEAM（config 名小写
     通配映射；beam 第 3 节点为方向节点，文本卡改写默认方向矢量 0,0,1）；
   - 迭代统一 `GetIteratorHandle → Valid / GetDataList / Next`，行形态自适应
     嵌套列表 / 平铺 / 逗号串；不支持的构型计数跳过并记日志；
   - 末尾补占位属性卡 MAT1/PSHELL/PSOLID/PROD/PBEAM + ENDDATA（导入端按
     属性 id 分组用）；
5. **写 .hwascii**（官方分组布局，与 AgaWriter 产物同构）：
   - `model GetResultCtrlHandle rc` → `GetSubcaseList`（1 起）→
     `SetCurrentSubcase` → `GetDataTypeList $sid`；
   - 每类型：`GetDataComponentList` → 列规范化（张量 XX YY ZZ XY YZ ZX /
     矢量 X Y Z / 标量 1 列，Mag/vonMises 等派生列跳过——导入端自行计算）；
     `GetDataTypeBinding`（node_on_element 跳过）；`GetNumberOfSimulations`（0 起）
     → `SetCurrentSimulation`；`GetSimulationLabel` 的末位数字作 $TIME；
   - 取值：`model GetContourCtrlHandle cc` → `SetResultType` + `SetComponent` →
     `q SetQuery "<node|element>.id contour.value"` 逐列收集；绑定未知时按
     行数自动在 node/element 前缀间切换；
   - 组写出：组头 `$BINDING/$COLUMN_INFO/$RESULT_TYPE`（每组重发 BINDING；
     节点矢量标注 Anim Source）+ 每步 `$TIME = <t> sec` + 实体行；
6. **成功判定**：.fem 有 GRID 且 .hwascii 有结果组才写 `EXPORT_OK`，
   否则 `EXPORT_INCOMPLETE` 并 `exit 1`。

### 3.4 跨版本兼容策略（try_forms / acquire）

官方 2022 文档已核实的调用直接写；签名存疑的操作（OpenFile/AddModel、
SetComponent、SetSimulation、GetSubcaseList 等）按"官方形式优先、变体兜底"
逐个尝试：

- `try_forms <label> {form1 form2 ...}`：命中即记日志（含命中的调用形式）；
- `acquire <label> {forms} outName`：出参句柄获取统一化——hwi 语义是
  `hwi GetSessionHandle sess` 之后 **sess 是句柄命令名**（官方示例全裸名），
  acquire 把句柄名镜像进同名变量后以 `$name` 调用，出参别名式与返回值式
  实现都兼容，并验证句柄命令真实存在。

首轮在 102 机器运行时，每个未命中形式的报错全文都在日志里——要么直接跑通，
要么留下精确的二轮定位证据，无需再靠方法表盲猜。

### 3.5 日志协议（调用方以日志为准，不信任进程退出码）

| 日志标记 | 含义 |
|---|---|
| `=== h3d export start: <path>` | 开始 |
| `<label> OK: <形式>` / `<label> FAILED (tried N forms)` | 每个多形式调用的命中/全败 |
| `first node/element row: <...>` | 首行原始数据（行形态解析自证） |
| `fem nodes/elements written=<> unparsed=<>` | 网格写出统计（含不支持构型清单） |
| `subcases: <>` / `subcase <> types: <>` | 结果枚举 |
| `SKIP '<type>': <原因>` | 复数/无分量/node_on_element/超大组等跳过原因 |
| `group '<type>': kind=<> binding=<> cols=<> steps=<>/<>` | 每组写出统计 |
| `EXPORT_OK fem=<> hwa=<> nodes=<> elems=<> groups=<>` | **成功标记** |
| `EXPORT_INCOMPLETE ...` | 走完流程但产物不完整 |
| `==== subcommands of <handle> ====` | 方法表（兜底诊断，保留） |

`HyperViewJob::convertH3d` 失败时返回的 error = 退出码 + 脚本日志尾部 40 行 +
控制台输出尾部 15 行摘要，GUI 会把前 400 字符透出给用户。注意控制台
（hyperview 自身 stdout）重定向到独立的 `hyperview_console.txt`——脚本日志
`LOG_PATH` 必须保持单写者，两路输出指向同一文件会互相覆盖（Windows 双句柄
偏移量独立，日志会搅乱）。

### 3.6 v1 已知限制（均记日志，待实测后放开）

- 复数结果（分量名含 M/P、R/I）整组跳过；
- node_on_element 绑定跳过；
- 张量/矢量分量不全的类型整组跳过（导入端按 (s)=1/(v)=3/(t)=6 列严格解析，
  部分列的组是坏组）；单元连接超出卡型预期节点数时截断；
- beam 单元方向矢量写默认值 0,0,1（截面轴信息 h3d 查询侧未取）；
- 单类型取值上限 MAXCELLS 与全程缓存总闸 MAXTOTAL = 8,000,000 格（步数×
  列数×实体数），超限跳过并提示；非数值取值按 0 写出并计 badVals；
- 属性卡全部占位（PSHELL t=0.1 / PSOLID / MAT1 钢默认值）——真实材料/厚度
  不是 h3d 携带信息；
- $TIME 取模拟标签末位数字（"Time = 0.5 sec" 类正确解析；异形标签退化为
  步号，仅影响动画时间轴不影响数据）；
- 取值行数少于实体数时打 NOTE（值缺失/选择集不全的可疑信号；多出的行属正常
  ——被跳过构型的实体有值但不写出）；
- 复查修掉的坑（勿回退）：AddModel 兜底不走 acquire（成功时 model 句柄尚未
  注册，info commands 检查会误判失败）；绑定自动切换后每列重算 node/element
  前缀；迭代器用唯一名（iter1/iter2/…）规避同名重复注册；q 句柄 fem/results
  两阶段复用不重复获取；
- CPYRAM 卡能否被 NastranVizIO `read_deck` 解析未验证（P2 实测，金字塔单元
  目前少见）。

### 3.7 本地 mock 测试（无 HyperView 环境的回归防线）

`tools/h3d_export_mock_test.tcl`：按官方 hwi 语义 stub 全套句柄（Get*Handle
出参 = 句柄命令名，`interp alias` 注册），合成 4 节点 + tetra4 + rbe2（不支
持构型）+ 超长连接 hex8 + Displacement（节点矢量 2 步）/ Stress（单元张量）/
复数 / 空类型 / 部分分量矢量，加载脚本 CONFIG-END 之后的真实主体跑端到端，
24 项断言（GRID/CTETRA/CHEXA 截断卡、分组布局、取值逐位、$TIME 解析、SKIP
路径、EXPORT_OK）。运行：

```bat
tclsh86 tools\h3d_export_mock_test.tcl     （Git Bash 自带 tclsh86，退出码判定）
```

2026-10-09 24/24 通过。该测试在开发机即可回归脚本逻辑（含同步前校验），
102 机器只负责验证真实 API 形态。

---

## 4. 流程

### 4.1 H3D 导入全流程

```
GUI filterFiles
  └─ tryGetAltairSelection 命中 .h3d
       └─ importAltairFile(h3d, "")          # 建库 + 起 mIOAltairFile 线程
            └─ run():
                 ① HyperViewJob::convertH3d(h3d → %TEMP%/mx_altair_import/<名>.hwascii/.fem)
                      ├─ locateHyperViewExe：ALTAIR_HOME → %ProgramFiles%\Altair\<版本>
                      │   \hwdesktop\hv\bin\win64 → 盘根两级 Altair*/*HyperWorks* 扫描
                      ├─ ASCII staging：<staging>/mx_hv_stage/{in.h3d, h3d_export.tcl,
                      │   out.hwascii, out.fem, hyperview_log.txt}
                      │   （输入/输出路径含非 ASCII 才搬运；纯 ASCII 直接引用原路径）
                      ├─ std::system: hyperview -b -tcl h3d_export.tcl > log 2>&1
                      └─ 判定：日志含 EXPORT_OK 且两产物 >40B；失败 error=退出码+日志尾40行
                 ② FemReader::readDeck(.fem)            → DeckModel        （进度 55）
                 ③ AgaReader::read(.hwascii)            → AgaResultFile    （进度 65）
                 ④ setFile / setDataFlag(false)                            # 懒读分发键
                 ⑤ AltairPostDataBuilder::buildGeometry(deck, db)          # 复用 OP2 链路（进度 75）
                 ⑥ _builder.buildResultFrames(results, hwascii, db)        # 注册+首帧发布（进度 90）
                 ⑦ 成功信号
```

### 4.2 结果帧注册与云图懒读（与 OP2 链路同构的两段式）

```
buildResultFrames（导入时一次）：
  帧 1 复用 initframe 几何，帧 >1 用 Op2PostDataBuilder::cloneGeometry 克隆
  每帧：PostFrameInfo（帧号/时间/模态/ALTAIR 工况名/_frameFile）+ 逐数据集
        注册 PostVariableInfo（变量名/类型/分量表）
  最后：setCaseFrameName + 发布首帧首个数据集的首分量

readVarDatas(帧, 变量, 分量)（云图切换时按需）：
  在内存 _results 中定位数据集 → 实体 ID→连续 ID → 逐行取分量
  → publishComponentValues（setComponeValues + setTempData +
    distributeFrameIdAndResult[带名字] + distributeFrameIdAndResultType +
    setResultMaxMin）——与 Op2PostDataBuilder 发布语义逐项一致
```

### 4.3 GUI 分发决策顺序

```
多选文件 → tryGetAltairSelection？ ─是→ importAltairFile
        → tryGetOp2Selection？    ─是→ importOp2File
        → mxdb 魔数嗅探           ─→ importNewFile / importOldFile

云图取值（isLazyOp2Result 命中后）：
  altairReaderFor？ ─是→ mIOAltairFile::readVarDatas（内存重发布）
  否则               → mIOP2File::readVarDatas（OP2 文件懒读）
```

---

## 5. 理解辅助

### 5.1 导出 / 导入对偶表

| 导出端 | 导入端 | 说明 |
|---|---|---|
| `FemWriter`（AgaModel→.fem） | `FemReader`（.fem→DeckModel/AgaModel） | 同一份 Nastran bulk 语法 |
| `AgaWriter`（AgaModel→.hwascii，按类型分组 V7） | `AgaReader`（.hwascii→AgaResultFile，按帧还原） | 读端把"按类型分组"重排回"按帧" |
| `HvTransJob`（→h3d） | `HyperViewJob`（h3d→文本） | 方向相反；成功判定都以产物+标记为准 |
| `mPostAgaModelBuilder`（库→AgaModel，rid→cid） | `AltairPostDataBuilder`（AgaResultFile→库，rid→cid） | **同一键空间约定，见 5.2** |
| `mIOP2File`（OP2 导入线程） | `mIOAltairFile` | 信号/懒读/收尾全部同构 |

### 5.2 键空间约定（重要，踩过坑）

- 实体 ID 有两套：**rid**（求解器原始 ID，.fem/.hwascii 里的 ENTITY_ID）与
  **cid**（连续 ID，渲染管线/结果缓存 `_resultDatas` 的键）。
- 映射：节点 `modelData->_nodedata.nodeidmap`（rid→cid），单元
  `partData->_parteledata.cell_rid_to_cid`（全部部件合并）。
- **入库前必须完成 rid→cid 换算**（导出端 2026-09-29 因漏换算导致 HyperView
  与 MxMech 极值位置错位，导入端实现时直接内置）。

### 5.3 数据集 → 变量/分量映射

| 数据集（$RESULT_TYPE） | 变量 | 分量 | 备注 |
|---|---|---|---|
| `Displacement(v)`（节点矢量） | Displacement | X/Y/Z/Magnitude | 另写变形动画（DisplacementValuesR + setDisplacementDataByFrameID） |
| 其他 `(v)` | 原名 | X/Y/Z/Magnitude | Magnitude 就地计算 |
| `(s)` | 原名 | Value | — |
| `(t)` | 原名 | XX/YY/ZZ/XY/YZ/ZX/vonMises | vonMises 实体公式；`(2t)`（3 值）按 XX/YY/XY + 壳公式 |
| `(R/I)/(M/P)` 等复数 | 跳过 | — | 记入 `AgaResultFile::warnings`（上限 20 条） |

名称细节：`$RESULT_TYPE = Stress(N) Min Principal(s)` → 变量名剥掉末尾类型
后缀 = `Stress(N) Min Principal`；行尾 `Anim Source` 标注在解析时去除。

### 5.4 .hwascii 解析容差

- 首个非空行必须含 `ALTAIR ASCII FILE`；`#` 注释与空行跳过；
- 未知 `$` 头（`$TITLE`/`$SIMULATION`/`$DATATYPE`…）跳过不报错；
- 帧键：`$TIME`（瞬态，容差 1e-9）/ `$MODE`（模态）/ 都无（静态）；跨组同键合并；
- 数据行实体 ID 不要求连续、不要求与网格同序，坏的行跳过并记警告；
- 零帧才判定失败（纯网格 .fem + 空结果文件不是合法组合）。

### 5.5 帧语义（PostFrameInfo 映射）

| 文件帧头 | _frameTime | _isFrequency | _frequency._modalFreq |
|---|---|---|---|
| `$TIME = t sec` | t | false | — |
| `$MODE = n` [+ `$FREQUENCY = f`] | f，无则 n | true | 同左 |
| 无（静态） | 0 | false | — |

### 5.6 部署前提与限制（P1 现状）

- H3D 模式需要部署机装 HyperWorks 且许可证可用；转换同步阻塞（无超时/取消，
  GUI 侧在子线程跑故不卡界面）；**不要把转换挂到高频自动加载路径**——许可证
  消耗随频次线性增长，产品形态建议做成显式"导入 Altair H3D…"动作。
- 转换仅结果侧可靠（Tcl 取值腿待 2022 实测，见 3.5）；**网格建议优先从源头
  .fem 走**（交付约定"h3d + 源网格"），转换只兜底"客户只有 h3d"的场景。
- 文本对模式（.fem + .hwascii）无任何 HyperWorks 依赖，当前即可全量使用。
- 高阶单元（Tet10/Hex20）节点序按 Nastran bulk 官方序直传（导出端同约定）；
  Wedge15/Tri6/Quad8 等暂不支持，读入时跳过。

### 5.7 测试

`tests/reader_roundtrip_test.cpp`（无 Qt，独立编译，编译命令见文件头注释）：
构造 2 帧 ×（节点矢量 + 节点标量 + 单元标量）的小模型 → AgaWriter/FemWriter
写出 → AgaReader/FemReader 读回 → 21 项断言（帧聚合、实体 ID 100/107/114/121
保留、绑定切换、数值逐位一致、网格读回）。2026-10-08 全部通过。
预期输出 `ALL PASS (0 failure(s))`。

`tests/tcl_output_reader_test.cpp`（无 Qt，独立编译）：**导出端↔导入端契约
测试**——先跑 `tools/h3d_export_mock_test.tcl` 产出 mock 产物，再把
`out.fem`/`out.hwascii` 喂给真实的 FemReader/AgaReader，20 项断言（free-field
卡解析、rbe2 不出现、CHEXA 截断连接、$TIME 帧聚合、取值逐位、复数/部分分量
类型缺席）。2026-10-09 全部通过。至此"mock hwi → Tcl 导出 → 真实读取器"
全链在开发机闭环，仅剩真实 HyperView API 形态待 102 机器确认。
（编译注意：MSVC 开关用短横线形式 + 必带 `-utf-8` + `tests/qt_shim` 提供
无 Qt 导出宏替身，完整命令见文件头。）

### 5.8 验证状态一览（2026-10-09）

| 环节 | 状态 |
|---|---|
| .hwascii 解析 + .fem 解析 + 数据库构建 | ✅ 回环测试验证 21/21（5.7） |
| Tcl 导出腿逻辑（.fem 遍历 + .hwascii 分组写出 + 取值） | ✅ mock 端到端 24/24（3.7）；官方 API 调用链按 2022 文档核实，存疑签名 try_forms 兜底 |
| 导出端↔导入端格式契约 | ✅ mock 产物喂真实读取器 20/20（5.7），开发机全链闭环 |
| PostIOFiled.dll（含内嵌导出脚本） | ✅ Debug 编译链接通过（bin 树，10-09 09:14） |
| mPost3DResult.cpp GUI 分发 | ✅ 编译通过（obj 已生成） |
| Tcl 导出腿真实 API 实测 | ⏳ 102 机器首跑（预期要么 EXPORT_OK、要么日志给出精确修正点） |
| MechMainWindowd.dll 链接 | ⛔ 被 dev_Mech 分支既有 WIP 阻塞（IOFile 缺 mFrpManager.h 等，与本次改动无关），分支可建后与 PostIOFiled.dll 一起同步 install/ |
| ELEMENT 绑定真实用例 / 高阶单元实测 | ⏳ P2（与导出端共享待办） |
