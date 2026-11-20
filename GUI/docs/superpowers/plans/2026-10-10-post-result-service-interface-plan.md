# 后处理结果服务接口（前处理 ⇄ 后处理跳转）开发计划

- 日期：2026-10-10
- 状态：已实施——阶段0/1/2/3/5 完成并各自提交（79994aec、a17c424c、7bdc9c2e 及文档提交）；
  阶段4（前处理接入）暂缓：MainWindow/MxMech 侧尚无 solveFinishedSig 发射方与求解记录/结果查看
  入口，接入范式已固化于 src/PostWindow/PostWindow.md §2.2，待前处理入口落地照抄接入
- 关联文档：`src/PostWindow/PostWindow.md`（隔离规则与回归要求）、`doc/AGENTS.md`（命名/提交规约）

## 1. 背景与目标

前处理需要：

1. 提供结果文件路径，请求创建后处理实例（模型 + 树根 + 导入结果）；
2. 从后处理拿到一个 key；
3. 之后任意时刻通过 key + 接口，切换到该实例对应的后处理模型树（视口激活自动联动 Ribbon/dock）。

已收敛的设计决策（讨论定稿，不再重议）：

| 决策点 | 结论 |
|---|---|
| key 语义 | 后处理侧自铸造的不透明字符串（`post-N` 单调计数），**与"第几次创建"绑定而非与文件路径绑定**；同一路径多次导入 → 多个 key、多棵树；永不复用；进程内有效、不持久化 |
| key 不用路径 | 前处理存在多次导入同一路径的场景，路径无法区分实例 |
| key 不用 modelDataID | `getModelData()->getModelDataID()` 分配本身可靠（`Basic/RebuildModelData` 静态计数器，单调不复用、不序列化），但 `mViewportModel::clearViewportModel()` 会就地重建 `_modelData` 导致 ID 悄悄漂移且无通知；对外契约不押在它身上 |
| 接口位置 | `src/PostCommon`（MainWindow 与 PostWindow 均已链接该库，语义即"后处理公共契约库"） |
| 实现/注册 | `mPostBrowserWindow` 实现接口，构造函数自注册全局引用（仿 `mPostDockProvider.h` 自注册模式） |
| 通道纪律 | 前处理侧只依赖 PostCommon 接口头 + 判空调用，不 include PostWindow 头（`check_postwindow_isolation.sh` 保证方向） |
| 实例删除通知 | 全局总线新增 `postResultRemovedSig(QString resultKey)`，后处理在 key 失效时发射，前处理据此作废旧 key（见 §3.1） |
| key 失效行为 | `showPostResult` 返回 `PostResultError::InvalidKey` **显式报错，不自动重建**；重新导入仅由用户显式发起（见 §3） |

## 2. 扩展性设计（为后续对外接口预留）

本次不是"加一个接口"，而是**固化一套后处理对外契约的扩展模式**，后续新增接口零争议：

1. **一能力域一接口**：每个对外能力独立头文件 `src/PostCommon/mPost<X>ServiceProvider.h`，内含 `I<X>Service` 纯虚接口 + `COMMON_EXPORT <x>ServiceRef()` 全局引用。本次 `IPostResultService` 是第一个实例；后续如导出、动画控制、曲线跳转各自新增头文件，**不向已有接口塞方法**，避免签名膨胀和职责混杂。
2. **参数对象预留**：创建接口用选项结构体而非裸参数列表，后续加选项不改签名：
   ```cpp
   struct PostResultCreateOptions
   {
       bool activateNow = true;   //false=仅创建不切换，留待后续 showPostResult
       //预留：后续新增选项在此追加，带默认值，老调用方零改动
   };
   ```
3. **注册表独立成类**：key 铸造/登记/校验逻辑提取为无 UI 依赖的普通类 `mPostResultRegistry`（PostWindow 内部），可 headless 单测；未来其他服务可复制此模式。
4. **key 内部表示可替换**：契约只承诺"不透明字符串 + 唯一 + 永不复用"，内部从 `post-N` 计数换 UUID 不影响调用方。
5. **失效通知走既有总线**：实例删除对前处理的被动通知复用 `mMechGlobalSignals` 全局信号（key 为参数），与接口通道（请求-应答）互补，不引入第三种跨模块通道。
6. **不做的事（防止过度设计）**：不引入通用服务注册中心（字符串查服务、类型不安全，不符合现有 provider 模式）；不做 key 跨会话持久化；不替换内部 modelDataID 身份体系。

## 3. 接口契约（定稿签名）

```cpp
// src/PostCommon/mPostResultServiceProvider.h（新增）
// namespace Common，导出沿用 mcommon_global.h 的 COMMON_EXPORT
namespace Common
{
    struct PostResultCreateOptions { bool activateNow = true; };

    //showPostResult 错误码（不抛C++异常：跨DLL边界抛异常不符Qt惯例，用显式枚举返回；
    //枚举后续追加新值不影响已编译调用方）
    enum class PostResultError
    {
        Ok,          //切换成功
        InvalidKey,  //key未登记或已失效（模型被删/ID漂移，条目已移除）——报错，不重建
        //预留：后续错误类型在此追加
    };

    class IPostResultService
    {
    public:
        virtual ~IPostResultService() = default;

        //创建后处理实例（新建模型+树根+导入结果）。
        //成功返回实例唯一不透明key（同一文件多次调用返回不同key）；失败返回空串。
        //仅限UI线程调用
        virtual QString createPostResult(const QStringList& resultFilePaths,
                                         const PostResultCreateOptions& options = {}) = 0;

        //按key幂等切换到对应后处理模型树/视口。
        //InvalidKey → 调用方提示用户"该结果已失效或被删除"并作废入口，不自动重建；
        //            重新导入是用户的显式动作（再次createPostResult拿新key）
        //key命中但结果文件mtime变化 → 后处理内部"删旧重建该key实例"后切换
        //（key仍有效，与失效无关；同文件其他实例不受影响）
        virtual PostResultError showPostResult(const QString& resultKey) = 0;
    };

    //全局服务引用（PostWindow侧构造时自注册；注册前为空，调用方必须判空）
    COMMON_EXPORT IPostResultService*& postResultServiceRef();
}
```

调用方使用范式（写进接口头注释）：

```cpp
if (auto* svc = Common::postResultServiceRef())
    key = svc->createPostResult(paths);
...
if (svc->showPostResult(key) == Common::PostResultError::InvalidKey)
    //报错不重建：提示用户"该结果已失效或被删除"并作废入口；
    //用户仍需查看时由其显式重新发起导入（createPostResult拿新key）
```

### 3.1 失效通知信号（全局总线）

接口是"请求→应答"通道；实例被删除是被动事件，前处理需要总线通知。复用全局总线，不新增通道类型：

```cpp
// src/Common/mMechGlobalSignals.h（MechCommon::mMechGlobalSignals 新增）
//后处理实例（key）已被删除/失效：前处理收到后应作废保存的同值key
//（入口置灰/标记失效）。UI线程发射，Auto连接
void postResultRemovedSig(const QString& resultKey);
```

命名不叫 `postModelRemovedSig`：与既有 `removePostModelTreeSig(int modelDataID)`（前处理→后处理的删除请求）方向相反、参数不同，按 key 语义命名避免混淆，两者不合并。

**触发边界（阶段 2 单测锁死）：**

| 场景 | 是否发射 |
|---|---|
| 模型被删（树根删除 / `deleteViewportModel` 路径，经 `removeByPostModelID`） | 发射 |
| `showPostResult` 校验发现模型已删 / ID 漂移而移除条目 | 发射 |
| mtime 变化"删旧重建该 key 实例"（key 保留，仅更新记录） | **不发射** |
| 程序退出 | 不发射（key 契约本就进程内） |

## 4. 阶段划分

> 规约：每阶段 = 一个工作单元 = 一个 commit（Conventional Commits，`feat(post): ...` 风格）。
> 每阶段合入前硬性回归：`ctest --preset conan-debug` 全 PASS + `bash scripts/check_postwindow_isolation.sh` PASS（PostWindow.md §5）。

### 阶段 0：基线确认（0.5 人日）
- 在干净工作区跑通全量 ctest 与隔离脚本，记录基线，确保后续失败可归因。

### 阶段 1：契约层：PostCommon 接口头 + Common 总线信号（1 人日）
- 新增 `src/PostCommon/mPostResultServiceProvider.h`（见 §3）。
- `mMechGlobalSignals.h` 新增 `postResultRemovedSig(const QString& resultKey)` 声明，注释写明 §3.1 触发边界。
- **重跑 `cmake .`**（src/ 为 GLOB 收源文件）。
- 验收：全仓编译通过；隔离脚本通过（新增文件不引入违规依赖方向）。

### 阶段 2：注册表类 + 单测（1.5 人日）
- PostWindow 内新增 `mPostResultRegistry`（普通 C++ 类，无 QObject/无 UI 依赖）：
  - `QString mintKey()`（单调计数，删除不回退）
  - `insert/takeByKey/removeByPostModelID`（按 postModelID 反查删条目，供模型删除路径）
  - `const PostJumpRecord* find(key)`；`PostJumpRecord{QStringList filePaths; int postModelID; int mainViewportID; qint64 fileMtime;}`
  - 失效回调 `std::function<void(const QString&)> onKeyInvalidated`：条目因删除/漂移被移除时触发（纯回调不依赖 QObject，PostWindow 侧接线到总线信号；单测直接用回调断言）
- 新增 `tests/PostWindow/tst_mPostResultRegistry.cpp`（QtTest headless，仿 `tst_mPostTreeModel.cpp`；tests/ 本地目录不进 git，按 `tests/Tests.md` 重建）。
- 用例：铸造唯一性；同路径多实例共存互不影响；removeByPostModelID 后旧 key 查询为空；mtime 变化仅标记对应实例；`onKeyInvalidated` 在删除/漂移移除时触发、在 mtime 重建（key 保留）不触发。
- 验收：新单测全过 + 阶段回归项全过。

### 阶段 3：实现与自注册（2 人日，核心阶段）
- `mPostBrowserWindow` 继承实现 `IPostResultService`，构造函数 `Common::postResultServiceRef() = this;`（单例进程级存活，无需反注册）。
- **创建漏斗统一**：`openPostResultFiles` 核心逻辑提取为 `createAndRegisterResult(paths, options)`，三条创建路径（手动对话框 / `solveFinishedSig` 槽 / 接口 `createPostResult`）全部汇聚于此，失败回滚逻辑保持；`openPostResultFiles` 保留原签名作薄封装（手动路径零改动）。
- `_resultJumpTable` 成员替换为 `mPostResultRegistry`；`unregisterResultJump(postModelID)` 改走 `removeByPostModelID`（现有模型删除路径调用点不变）。
- **失效通知接线（单点发射）**：`mPostResultRegistry::onKeyInvalidated` → emit `postResultRemovedSig(key)`。模型删除路径与 `showPostResult` 漂移移除路径都经注册表移除条目，天然只在回调处发射一次；mtime 重建不条目移除、不发信号。
- **切换实现** `showPostResultByKey(key)`：
  1. 注册表未命中 → 返回 `PostResultError::InvalidKey`；
  2. 命中但 `getCVM->getViewportModel(rec.postModelID)` 为空（模型被删 / `clearViewportModel` 就地换 ID 漂移）→ 移除条目，返回 `InvalidKey`；
  3. mtime 变化 → 走现有"删旧重建该实例"流程后切换（key 保留，返回 `Ok`）；
  4. 正常 → `activateViewportByID(rec.mainViewportID)`；入口视口已删 → 现有 `setCurrentViewportModel(postModelID) + activatePostViewport()` 兜底。
- 信号迁移：`slot_showPostResult` 改走同一实现；`mMechGlobalSignals.h` 中 `requestShowPostResultSig` 注释更新（resultKey 不再是路径，当前 PostWindow 外无发射者，零破坏）；`solveFinishedSig` 槽内部改调漏斗函数（行为不变）。
- 验收：手动回归清单——同文件导入两次得两棵树且各自按键跳转；删除其一后其 key 返回 `InvalidKey` 且发射 `postResultRemovedSig`（携带被删 key）、另一 key 不受影响；重算覆盖文件后跳转触发重建（key 保留、不发信号、返回 `Ok`）；阶段回归项全过；PostWindow.md 追加修改记录。

### 阶段 4：前处理接入（1–2 人日，依赖前处理侧入口排期）
- 调用点位于 MainWindow/MxMech 层（如求解记录/结果查看入口），将返回 key 与该入口绑定保存。
- 跳转时 `showPostResult`：返回 `InvalidKey` 则弹错误提示（"该后处理结果已失效或被删除"）并作废入口，**不自动重建**；重新导入是用户显式动作。
- 连接 `postResultRemovedSig`：作废对应入口保存的 key（置灰/标记失效），把失效在用户点击前就暴露；`InvalidKey` 错误返回兜住"信号到达与用户点击之间被删"的竞态。
- 服务判空兜底：`postResultServiceRef()` 为空（后处理未注册）时入口置灰或回落到原文件打开对话框。
- 验收：端到端场景——求解完成→创建拿 key→留在前处理→按 key 跳回对应树；重启后旧 key 跳转得到 `InvalidKey` 错误提示、由用户决定是否重新导入；后处理内删除模型→前处理入口经信号同步作废（置灰）。

### 阶段 5：文档与扩展规约固化（0.5 人日）
- PostWindow.md 新增"对外服务接口"一节：接口契约、key 规则（不透明/唯一/永不复用/进程内）、失效语义（`InvalidKey` 报错不自动重建）、UI 线程约束、**新增对外接口的扩展规约**（§2 第 1 条的命名与自注册/判空纪律）；§2.1 信号表补 `postResultRemovedSig` 条目。
- 修改记录追加；如涉及布局改动补 `tests/layout-repro` 流程。

## 5. 风险与对策

| 风险 | 对策 |
|---|---|
| `mViewportModel::clearViewportModel()` 就地换 `_modelData` 导致 postModelID 漂移（`mMeshBatchCLI.cpp:328` 在用） | `showPostResult` 存在性校验（阶段 3 第 2 步）+ false 走重建兜底；不在本计划内修该函数（顺带发现其旧 `_modelData` 未 delete 的泄漏，另立 issue） |
| 非UI线程调用接口 | 契约注明仅 UI 线程；求解完成路径保留信号 Auto 连接的队列行为 |
| 失效信号漏发/多发（触发边界模糊） | 发射点收敛于注册表移除回调单点（删除与漂移移除都经它）；触发/不触发边界由阶段 2 单测锁死（mtime 重建不发） |
| "报错不重建"与前处理旧期望不一致 | 契约显式写明失效即 `InvalidKey`、重建是用户显式动作；`PostResultError` 枚举预留后续细分（如文件丢失）|
| `COMMON_EXPORT` 宏与 Common 模块同名 | 照抄 PostCommon 现状（其 CMake `PRIVATE "COMMON_LIB"` 定义隔离），不新造宏 |
| 新增文件未被编译（GLOB） | 阶段 1/2 的验收步骤明确重跑 `cmake .` |
| 前处理侧调用点下沉到 PreData/IOFile 层（不链 PostCommon） | 位置选择跟调用方走：届时接口头迁至 `src/Common`（仅动 include 路径，实现零改动），已在扩展规约中注明 |

## 6. 工期汇总

约 **5.5–7 人日**：阶段 0（0.5）→ 1（1）→ 2（1.5）→ 3（2）→ 4（1–2）→ 5（0.5）。阶段 1–3 为后处理侧独立可交付；阶段 4 可与前处理排期解耦并行。
