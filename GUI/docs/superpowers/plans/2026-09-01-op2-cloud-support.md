# OP2 后处理云图支持实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法进行跟踪。

**目标：** 让 `RebuildGUI` 后处理直接导入 `NastranVizIO` 支持的 `.op2` 结果文件，并基于配套 `.bdf` 几何模型显示节点 / 单元云图。

**架构：** 在 `PostIOFile` 内新增 OP2 适配层，负责 BDF / OP2 配对、表索引、按需读取和 `PostData` 构建；`MainWindow` 只负责文件选择、线程调度和界面刷新。OP2 采用“几何一次性构建 + 结果表按需加载 + 帧按需解析”的策略，避免 730 MB 级文件一次性进入内存。

**技术栈：** C++17、Qt5、CMake、`NastranVizIO`、MSVC。

---

## 全局约束

- 不新建 `MxOp2Reader` 静态库；OP2 适配代码全部放在 `PostIOFile` 内。
- `RebuildGUI` 全局 C++ 标准保持 C++17。
- OP2 只提供结果数据；云图渲染所需几何来自配套 BDF。
- OP2 结果采用懒加载：首帧只加载当前表、当前帧，禁止一次性解析全部结果表。
- 不修改 `NastranVizIO` 公共 API，除非实现中发现明确缺失。
- 现有 `.mxdb` / `.mxdb0` 导入路径必须保持不变。

---

## 文件清单

| 文件 | 职责 |
|------|------|
| `CMakeLists.txt` | 引入 `NastranVizIO` 根路径和目标依赖 |
| `src/PostIOFile/CMakeLists.txt` | 让 `PostIOFile` 链接 `NastranVizIO::NastranVizIO` |
| `src/PostIOFile/mIOP2File.h/.cpp` | OP2 导入线程、文件配对、表索引和读取调度 |
| `src/PostIOFile/Op2PostDataBuilder.h/.cpp` | BDF 几何与 OP2 结果到 `PostData` 的转换 |
| `src/PostIOFile/mIOEnum.h` | 补充 OP2 相关错误码 |
| `src/MainWindow/mMenuWidget.cpp` | 文件过滤器加入 `.op2` |
| `src/MainWindow/mPost3DResult.h/.cpp` | 新增 OP2 分支并接入结果面板刷新 |
| `src/PostIOFile/MxdbReader/test_op2_import.cpp` | 文件配对、表索引和数据构建测试 |

---

## 任务 1：接入 `NastranVizIO` 依赖

**文件：**
- 修改：`CMakeLists.txt`
- 修改：`src/PostIOFile/CMakeLists.txt`

**产出：** `PostIOFile` 可以直接包含 `<nastranvizio/post.hpp>` 并链接库目标。

- [ ] **步骤 1：在根 CMake 中声明库路径**

```cmake
set(NASTRANVIZIO_ROOT
    "D:/MxSimProject/MxSim_DevBase/Basic/NastranVizIO"
    CACHE PATH "NastranVizIO source root")
```

优先 `find_package`；如无安装版包配置，再用：

```cmake
add_subdirectory("${NASTRANVIZIO_ROOT}" "${CMAKE_BINARY_DIR}/NastranVizIO" EXCLUDE_FROM_ALL)
```

二选一，不重复引入。

- [ ] **步骤 2：让 `PostIOFile` 链接目标**

```cmake
target_link_libraries(PostIOFile PRIVATE NastranVizIO::NastranVizIO)
```

- [ ] **步骤 3：编译验证**

```powershell
cmake -S D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI -B D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin
cmake --build D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin --config Release --target PostIOFile
```

**验收：**
- 配置成功。
- `PostIOFile` 编译成功。
- 未新增 GUI 侧静态库。

- [ ] **步骤 4：提交**

```powershell
git add CMakeLists.txt src/PostIOFile/CMakeLists.txt
git commit -m "build(PostIOFile): link NastranVizIO for OP2 import"
```

---

## 任务 2：实现 OP2 / BDF 配对和导入线程

**文件：**
- 新增：`src/PostIOFile/mIOP2File.h`
- 新增：`src/PostIOFile/mIOP2File.cpp`
- 修改：`src/PostIOFile/mIOEnum.h`
- 修改：`src/PostIOFile/CMakeLists.txt`

**接口：**

```cpp
class mIOP2File : public QThread
{
    Q_OBJECT
public:
    mIOP2File(const QString& op2File,
              const QString& bdfFile,
              QPointer<PostData::mPostDataBase> modeldb,
              QObject* parent = nullptr);

    static QString findCompanionBdf(const QString& op2File);

protected:
    void run() override;

signals:
    void readFileSuccessSig(const QString& postName);
    void readOp2ErrorSig(MIOFile::IOFileErrorType type);
    void finishedOp2FileSig(const QString& postName);
};
```

**行为：**
- 构造时保存 OP2 / BDF 路径和目标 `mPostDataBase`。
- `findCompanionBdf()` 查找：
  - `同名.bdf`
  - `同名_MODEL.bdf`
  - 去掉 `_MODEL` 后缀后的同名 `.bdf`
- `run()` 顺序：
  1. 校验文件存在。
  2. 解析 BDF 几何。
  3. 扫描 OP2 表。
  4. 构建首帧云图数据。
  5. 发出成功 / 失败信号。

- [ ] **步骤 1：补充错误码**

```cpp
Op2FileNotExist,
Op2BdfNotFound,
Op2ParseFailed,
Op2UnsupportedTable,
Op2ModelMismatch
```

具体枚举值跟随现有风格，不改变已有值。

- [ ] **步骤 2：实现文件配对**

`findCompanionBdf()` 返回规则：
1. 优先精确同名。
2. 其次 `<basename>_MODEL.bdf`。
3. 最后尝试去掉 OP2 文件名中的后缀后再匹配。
4. 找不到返回空字符串。

- [ ] **步骤 3：实现表扫描缓存**

```cpp
std::vector<nastranvizio::op2::Op2ScannedTable> _tables;
```

只缓存表元数据和文件偏移，不缓存全部结果值。

- [ ] **步骤 4：实现导入主流程**

```cpp
const auto deck = nastranvizio::deck::read_deck(bdfPath);
const auto tables = nastranvizio::op2::scan_op2_tables(op2Path);
```

然后把 `deck`、`tables`、OP2 路径传给 `Op2PostDataBuilder`。

- [ ] **步骤 5：编译验证**

```powershell
cmake --build D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin --config Release --target PostIOFile
```

**验收：**
- `.op2` 能找到配套 `.bdf`。
- 线程能正常启动并退出。
- 失败时能发出明确错误码。

- [ ] **步骤 6：提交**

```powershell
git add src/PostIOFile/mIOP2File.h src/PostIOFile/mIOP2File.cpp src/PostIOFile/mIOEnum.h src/PostIOFile/CMakeLists.txt
git commit -m "feat(PostIOFile): add OP2 import thread"
```

---

## 任务 3：实现 BDF 几何到 `PostData` 的转换

**文件：**
- 新增：`src/PostIOFile/Op2PostDataBuilder.h`
- 新增：`src/PostIOFile/Op2PostDataBuilder.cpp`
- 修改：`src/PostIOFile/CMakeLists.txt`

**接口：**

```cpp
class Op2PostDataBuilder
{
public:
    bool buildGeometry(const nastranvizio::deck::DeckModel& deck,
                       QPointer<PostData::mPostDataBase> db);

    bool buildFirstResultFrame(const QString& op2File,
                               const std::vector<nastranvizio::op2::Op2ScannedTable>& tables,
                               QPointer<PostData::mPostDataBase> db);
};
```

**几何转换规则：**
- `deck::Grid` → `mPostModelData::_nodedata`
- `deck::Element` → `mPostPartData::_parteledata`
- 按 `property_id` 分组生成 Part
- Part 名称使用 `Property_<id>`，未知属性回退为 `Unknown`

**必须覆盖的单元类型：**
- `CROD`
- `CBAR`
- `CBEAM`
- `CTRIA3`
- `CQUAD4`
- `CTETRA`
- `CPENTA`
- `CHEXA`

- [ ] **步骤 1：映射节点**

填充：
- `nodeamount`
- `ids`
- `nodeposF`
- `nodeidmap`

使用单精度 `nodeposF` 即可，OP2 结果本身为 `float`。

- [ ] **步骤 2：映射单元**

按 NastranVizIO 的 `ElementType` 转换为现有 `MxFunctions::ElementType`，再写入：
- `cell_types`
- `cell_rid`
- `cell_node_counts`
- `cell_node_offsets`
- `cell_info`
- `cell_node_ids`

所有节点 ID 必须经过 `nodeidmap` 转成连续 ID。

- [ ] **步骤 3：生成初始帧**

```cpp
frameInfo._frameNum = 1;
frameInfo._frameName = "1";
frameInfo._frameCase = "OP2";
frameInfo._frameTime = 0.0;
frameInfo.type = 0;
```

并调用：

```cpp
db->appendModelData(1, modelData);
db->appendInitframe(modelData);
```

- [ ] **步骤 4：验证几何**

用测试 BDF 断言：
- 节点数量一致
- 单元数量一致
- Part 数量一致
- 节点 ID 映射无缺失

**验收：**
- 只导入几何也能生成可渲染模型。
- 模型节点 / 单元数量与 BDF 解析结果一致。

- [ ] **步骤 5：提交**

```powershell
git add src/PostIOFile/Op2PostDataBuilder.h src/PostIOFile/Op2PostDataBuilder.cpp src/PostIOFile/CMakeLists.txt
git commit -m "feat(PostIOFile): build PostData geometry from BDF"
```

---

## 任务 4：实现 OP2 结果到云图数据的转换

**文件：**
- 修改：`src/PostIOFile/Op2PostDataBuilder.h`
- 修改：`src/PostIOFile/Op2PostDataBuilder.cpp`

**目标表：**
- `OUG1` → Displacement
- `OVG1` → Velocity
- `OAG1` → Acceleration
- `OES1X1` → Stress

**变量映射：**

| OP2 表 | 变量名 | 分量 |
|---------|--------|------|
| `OUG1` | Displacement | `T1/T2/T3/R1/R2/R3/Magnitude` |
| `OVG1` | Velocity | `T1/T2/T3/R1/R2/R3/Magnitude` |
| `OAG1` | Acceleration | `T1/T2/T3/R1/R2/R3/Magnitude` |
| `OES1X1` | Stress | `XX/YY/ZZ/XY/YZ/ZX/vonMises` |

**复数结果：**
- 首期每个分量额外生成 `Real`、`Imag`、`Magnitude` 三个子分量。
- `vonMises` 只对实部结果计算；复数应力首期不计算等效应力，避免语义不明确。

**懒加载策略：**
- 只解析当前用户选择的表。
- 只解析当前帧。
- 其他表只保留元数据和偏移。
- 帧切换时按需调用 `read_node_result_at_table()` / `read_stress_result_at_table()`。

- [ ] **步骤 1：建立表选择器**

优先级：
1. `OUG1`
2. `OES1X1`
3. `OVG1`
4. `OAG1`

首帧默认选择 `OUG1`；如不存在则按顺序回退。

- [ ] **步骤 2：转换节点结果**

```cpp
auto table = nastranvizio::op2::read_node_result_at_table(op2Path, tableOffset);
```

把每个 `FrequencyResponseStep` 或 `RealNodeResultStep` 转成一帧。

写入：
- `PostFrameInfo::_frameTime = step.frequency`
- `PostVariableInfo::_varName`
- `PostVariableInfo::_compoNames`
- `PostVariableInfo::_varType`
- `mPostDataBase::setResultNodeDataByFrameID()`

- [ ] **步骤 3：转换单元应力**

```cpp
auto table = nastranvizio::op2::read_stress_result_at_table(op2Path, tableOffset);
```

使用类型化视图：
- `to_solid_stress()`
- `to_cquad4_stress()`
- `to_ctria3_stress()`

把每个应力分量写入单元结果容器。

- [ ] **步骤 4：计算云图值**

```cpp
db->distributeFrameIdAndResult(frameId, values);
db->distributeFrameIdAndResultType(frameId, resultType);
```

同时保证：
- 最大值 / 最小值正确
- 缺失节点或单元不参与计算
- ID 不匹配时记录 warning，不中断导入

- [ ] **步骤 5：验证结果**

断言：
- 变量名列表正确
- 帧数量与 OP2 步数一致
- 每帧非空
- 首帧云图最大值 / 最小值与 `NastranVizIO` 独立计算结果一致

**验收：**
- 结果面板能列出 OP2 变量。
- 云图能按分量切换。
- 帧切换时数值随频率变化。

- [ ] **步骤 6：提交**

```powershell
git add src/PostIOFile/Op2PostDataBuilder.h src/PostIOFile/Op2PostDataBuilder.cpp
git commit -m "feat(PostIOFile): convert OP2 results to contour data"
```

---

## 任务 5：接入后处理 GUI

**文件：**
- 修改：`src/MainWindow/mMenuWidget.cpp`
- 修改：`src/MainWindow/mPost3DResult.h`
- 修改：`src/MainWindow/mPost3DResult.cpp`

**改动点：**
- 文件过滤器加入 `.op2`
- `filterFiles()` 识别 `.op2`
- 新增 `importOp2File()`
- 创建 `mPostDataBase` 并启动 `mIOP2File`
- 连接完成信号，刷新三维结果面板

- [ ] **步骤 1：修改文件过滤器**

```cpp
QString("Results File(*.mxdb0 *.mxdb *.op2)")
```

- [ ] **步骤 2：新增导入函数**

```cpp
bool importOp2File(const QString& filename);
```

流程：
1. 调用 `mIOP2File::findCompanionBdf()`
2. 未找到 BDF 时报错并返回 `false`
3. 创建 `mPostDataBase`
4. 加入 `Physical_Post`
5. 启动 `mIOP2File`
6. 返回 `true`

- [ ] **步骤 3：连接完成信号**

```cpp
connect(op2Thread, &mIOP2File::finishedOp2FileSig,
        this, &mPost3DResultPanel::slot_op2FileFinished);
```

`slot_op2FileFinished()` 负责：
- 更新当前视口
- 刷新变量 / 分量 / 帧下拉框
- 触发云图渲染

- [ ] **步骤 4：回归原有路径**

分别测试：
- `.mxdb`
- `.mxdb0`
- `.op2`

确保 `.mxdb` / `.mxdb0` 行为不变。

**验收：**
- 菜单可选 `.op2`
- 选择后自动找到 BDF
- 导入完成后模型显示
- 变量、分量、帧可切换
- 云图正常渲染

- [ ] **步骤 5：提交**

```powershell
git add src/MainWindow/mMenuWidget.cpp src/MainWindow/mPost3DResult.h src/MainWindow/mPost3DResult.cpp
git commit -m "feat(MainWindow): enable OP2 result import"
```

---

## 任务 6：补充测试与验收

**文件：**
- 新增：`src/PostIOFile/MxdbReader/test_op2_import.cpp`
- 修改：`src/PostIOFile/CMakeLists.txt`

**测试范围：**
1. BDF 自动配对
2. OP2 表扫描
3. 节点数 / 单元数
4. 变量名
5. 帧数量
6. 首帧云图非空
7. 最大值 / 最小值

- [ ] **步骤 1：编写文件配对测试**

用临时目录构造：
- `model.op2`
- `model.bdf`
- `model_MODEL.op2`
- `model_MODEL.bdf`

断言 `findCompanionBdf()` 返回正确路径。

- [ ] **步骤 2：编写表扫描测试**

使用真实测试文件：

```text
F:/Test_Model_Data/数据重构测试文件/PostV3.0测试文件/OP2/A19_SUV_hood_FREQ.op2
```

断言扫描结果包含：
- `OUG1`
- `OVG1`
- `OAG1`
- `OES1X1`

- [ ] **步骤 3：编写数据构建测试**

断言：
- BDF 节点数一致
- BDF 单元数一致
- OP2 帧数一致
- 变量列表包含 `Displacement`
- 首帧结果非空

- [ ] **步骤 4：运行全部验证**

```powershell
ctest --test-dir D:/MxSimProject/MxSim_DevBase/Basic/NastranVizIO/build -C Release --output-on-failure
cmake --build D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin --config Release --target PostIOFile
```

**验收：**
- NastranVizIO 测试全部通过
- `PostIOFile` 编译通过
- OP2 导入测试全部通过
- 原有结果文件导入不受影响

- [ ] **步骤 5：提交**

```powershell
git add src/PostIOFile/MxdbReader/test_op2_import.cpp src/PostIOFile/CMakeLists.txt
git commit -m "test(PostIOFile): cover OP2 import and contour conversion"
```

---

## 交付标准

1. 选择 `.op2` 后能自动匹配同目录 BDF。
2. 模型几何正常显示。
3. 变量面板出现 `Displacement` / `Velocity` / `Acceleration` / `Stress`。
4. 分量面板出现对应分量。
5. 帧面板出现频率步。
6. 云图随分量和帧切换刷新。
7. 导入 730 MB OP2 时首帧可交互，内存峰值可控。
8. `.mxdb` / `.mxdb0` 原有功能无回归。

---

## 非目标

- 不支持只有 OP2、没有 BDF 的渲染。
- 不在首期实现复数应力的等效应力。
- 不新增独立 OP2 静态库。
- 不重构现有 `MxdbReader`。
- 不修改渲染引擎。

