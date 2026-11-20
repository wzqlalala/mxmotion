# 原生结果导入异步失败：空树回滚与 key 级失败通知 开发计划

- 日期：2026-10-23
- 状态：**已立项、待实施**（本文档为实施会话的自包含输入，所有文件:行号已核实）
- 背景：2026-10-22"服务接口 OP2 支持优化"（PostWindow.md 修改记录同日条目）明确遗留；
  本文覆盖其"明确不做"中的第一项
- 关联文档：`src/PostWindow/PostWindow.md`（§2.2 服务契约）、
  `doc/post-result-service-usage.md`（前处理侧说明，实施后需同步）、
  `docs/superpowers/plans/2026-10-10-post-result-service-interface-plan.md`（key/注册表契约）

## 1. 缺陷清单（现状，已逐点核实）

三条异步导入链在**解析失败**时的现状，共性问题后述：

| 链路 | 失败分支 | 现状行为 |
|---|---|---|
| native（op2/rst/rth/odb/h3d/d3plot） | `mPost3DResult.cpp:2849-2868` `slot_nativeThreadFinished`（postname 空） | 删结果库 → 移除 reader → 发**无参** `postResultImportFailedSig`（:2866）。**不回滚模型树** |
| mxdb 新格式 | `mPost3DResult.cpp:2757-2768` `slot_mxdbThreadFinished`（isReadSuccess false） | 删结果库 → **直接 return，连 postResultImportFailedSig 都不发**（隐藏缺陷 D-4） |
| mxdb 旧格式 | `:2875` → `slot_finishedImportFiles:2977-2984`（postname 空） | 发无参 `postResultImportFailedSig`（:2983）。**不回滚模型树** |

共性问题（对三条链全部成立）：

1. **空树残留**：`createAndRegisterResult`（`mPostBrowserWindow.cpp:1641`）同步段只等
   `import3DResult` 返回 true（native 链=读取线程**已启动**即真），解析失败发生在返回之后
   ——此时后处理模型/树根已建、服务 key 已发出，均无人回收。GUI 手动导入与服务接口同病。
2. **key 级失败通知缺失**：`postResultImportFailedSig()` 无参（`mMechGlobalSignals.h:276`），
   唯一消费者是会话恢复协调器（`mPostSessionCoordinator.cpp:48-50`，跳过队列条目）。
   服务调用方持有有效 key 却不知道实例已废。
3. **删库竞态**：失败分支用 `getCVM`（**完成时刻**的当前模型）删结果库；导入期间用户切
   模型即删错管理者（native :2859、新格式 :2765 同病）。恢复链靠导入前显式
   `setCurrentViewportModel`（`mPostSessionCoordinator.cpp:159`）规避，用户手动导入无保护。

**D-4（随立项修复）**：mxdb 新格式链失败不发任何信号——若发生在会话恢复期间，
`_importing` 永不复位，恢复收尾与保存/打开会话全部卡死（与 :46-47 注释描述的旧格式
卡死同型，当年只修了旧格式两条发射点 :2983/:2990）。

## 2. 已收敛的设计决策

| 决策点 | 结论 | 理由 |
|---|---|---|
| 归属捕获时机 | **导入启动时**捕获 `postModelID`（native：`importNativeFile` :1012 构造 reader 处；mxdb：importNewFile/importOldFile 构造线程处），面板内存 `QHash<QThread*, int>`（或改 _nativeReaders 值类型） | 导入启动时 getCVM 即宿主模型（创建漏斗/恢复链都先建模型再导入）；失败时按捕获 ID 操作，顺带修复删库竞态（问题3） |
| 回滚判据 | 失败时查注册表 `_resultRegistry.findByPostModelID(capturedID)`：**有记录 → 回滚整模型；无记录 → 维持现状只删库** | 有记录 = 创建漏斗/服务流程新建的实例（回滚安全）；无记录 = 按工况更新/会话恢复等向既有模型补导（回滚会误删用户模型）。恢复链的回滚由协调器自理（见下） |
| 回滚动作 | 复用 mtime 重建的标准删除序列（`mPostBrowserWindow.cpp:1768-1769`）：`emit removePostModelTreeSig(modelID)` → `mViewTreeFunction::deleteViewportModel(modelID)` | 树根移除链（`mPostTreeView::slot_removePostModelTree:1250` → `unregisterResultJump:504` → `removeByPostModelID` → `onKeyInvalidated` → `postResultRemovedSig(key)`）**自动作废 key 并逐 key 发既有失效信号**——零新增失效通道 |
| key 级失败通知 | 新增总线信号 `postResultCreateFailedSig(const QString& resultKey)`，在回滚槽内、执行删除序列**前**对 `removeByPostModelID` 将返回的每个 key 发射 | 与 `postResultRemovedSig`（"实例被删"）语义区分：前处理据此提示"结果导入失败"而非"结果已删除"。不扩展 `postResultImportFailedSig` 的语义，避免与恢复协调器的既有消费混淆 |
| postResultImportFailedSig 扩参 | `()` → `(int postModelID)`；三链发射点全部补上（含 D-4 缺失点） | Qt 允许信号参数多于槽：协调器既有无参槽连接零改动继续工作；协调器可后续用 modelID 自理恢复回滚 |
| 会话恢复路径 | 协调器 `onImportFailed` 扩为 `(int postModelID)`：跳过条目前先回滚该条新建的空模型（同一删除序列），再推进队列 | 恢复条目模型由编排方新建、无注册表记录，不走浏览器窗口的回滚槽 |
| 执行位置 | 回滚槽放 `mPostBrowserWindow`（连接 postResultImportFailedSig），与 showPostResultByKey 同类职责 | 同模块、已持有注册表与标准删除序列；面板只负责捕获与发射 |
| 作用域 | 三链全覆盖：Phase 1 native（OP2 报障主链）、Phase 2 mxdb 新旧（含 D-4） | 三链病根同源，签名/信号一次扩展到位 |

## 3. 实施步骤

### Phase 1：native 链（报障主链）

1. **mMechGlobalSignals.h**：`postResultImportFailedSig()` → `postResultImportFailedSig(int postModelID)`；
   新增 `postResultCreateFailedSig(const QString& resultKey)`。
2. **mPost3DResult.cpp**：
   - `importNativeFile`（:1012 构造 reader 处）：捕获 `getCVM` 的模型 ID 存入
     `QHash<MIOFile::mIONativeFile*, int> _nativeReaderOwner`（新成员）；
   - `slot_nativeThreadFinished` 失败分支（:2855-2868）：按**捕获 ID** 的模型管理者删库
     （替代 getCVM，修复竞态）；`postResultImportFailedSig(ownerID)` 带参发射；清 `_nativeReaderOwner`。
3. **mPostBrowserWindow.cpp**：构造处连接 `postResultImportFailedSig` → 新私有槽
   `slot_nativeImportFailedRollback(int postModelID)`：
   ```cpp
   //有登记=创建/服务流程新建的实例才回滚；无登记=按工况更新等补导，维持只删库现状
   if (_resultRegistry.findByPostModelID(postModelID) == nullptr) return;
   const QStringList keys = _resultRegistry.removeByPostModelID(postModelID); //先取key再触发失效回调
   for (const QString& k : keys)
       emit mMechGlobalSignals::postResultCreateFailedSig(k);   //失败语义先于删除链发出
   emit mMechGlobalSignals::removePostModelTreeSig(postModelID); //树根链（内部会再查表，已空表无副作用）
   mViewTreeFunction::deleteViewportModel(postModelID);          //激活自动回落前处理
   ```
   注意：`removeByPostModelID` 逐条触发 `onKeyInvalidated` → `postResultRemovedSig`，
   与 `postResultCreateFailedSig` 形成"失败+失效"双通知，顺序保证失败信号先到。
   （实施时核对 `removeByPostModelID` 返回值与回调触发次序，必要时先断开回调再手动补发，
   以 `PostResultRegistryTests` 锁行为。）
4. **mPostSessionCoordinator.cpp**：`onImportFailed` 扩 `(int postModelID)`（无参旧连接兼容）：
   跳过条目前对 `entry.modelID` 执行同一删除序列（恢复条目模型无注册表记录，直接删）。

### Phase 2：mxdb 新旧链

5. `importNewFile`/`importOldFile` 构造线程处同样捕获宿主模型 ID（同款 hash，key 类型 `QThread*`）。
6. `slot_mxdbThreadFinished` 失败分支（:2763-2768）：补 `postResultImportFailedSig(ownerID)`
   ——同时修复 **D-4 恢复卡死**；删库改按捕获 ID。
7. 旧格式链发射点 :2983/:2990 补带 `ownerID`。

### Phase 3：文档与契约同步

8. `doc/post-result-service-usage.md` §6/§9：补"异步解析失败会收到
   `postResultCreateFailedSig(key)` + `postResultRemovedSig(key)`，key 作废"的契约。
9. `src/PostWindow/PostWindow.md` §2.1 信号表补 `postResultCreateFailedSig` 行、
   §2.2 补失败语义；文末登记修改记录（隔离检查门要求）。

## 4. 风险与对策

| 风险 | 对策 |
|---|---|
| 失败瞬间用户正操作该模型（删除时激活漂移） | 与 mtime 重建同款时序（先树信号后删模型，:1768-1769 注释：激活转移已由删除链处理）；回滚槽仅按**捕获 ID** 操作，不碰当前模型 |
| mtime"删旧重建"重放（reuseKey）期间异步失败 | 重建成功段已把旧 key 重登记（:1708-1709）；异步失败回滚 → `removeByPostModelID` 自动作废该 key → 前处理收双信号作废入口，语义自洽（重放失败=结果文件坏了） |
| 恢复队列回滚与推进交错 | 协调器先回滚后 `_importing=false` + `removeFirst` + `startHeadImport()`，顺序与现状跳过路径一致（:162-170） |
| 双信号（CreateFailed+Removed）消费者混乱 | 契约写明：CreateFailed 仅"异步导入失败"场景、先到；收到任一即作废 key。`PostResultRemovedSig` 既有消费者（置灰入口）行为不变 |
| 同一模型多文件并发导入部分失败（旧格式多文件组合） | 旧格式链失败=整组失败（单线程逐文件）；native 链单文件单模型。无部分成功态，回滚安全 |

## 5. 验证计划

1. **单测**：`tests/PostWindow/tst_mPostResultRegistry.cpp` 补用例——回滚槽依赖的
   `removeByPostModelID` 返回值/回调次序契约（headless，纯注册表行为）。
2. **构建门**：全量 Debug/Release 0 error（信号签名变更触发 moc 重生成，下游全重编）；
   `check_postwindow_isolation.sh` PASS。
3. **默认回归**：ctest 6/6。
4. **端到端**（GUI 冒烟，样本用改坏扩展名的 op2/拷断的 mxdb0）：
   - 服务路径：`createPostResult` 拿 key → 等异步失败 → 断言收到
     `postResultCreateFailedSig(key)` 与 `postResultRemovedSig(key)`、树根消失、
     `showPostResult(key)` 返回 `InvalidKey`；
   - GUI 路径：手动导入损坏文件 → 空树不再残留；
   - 按工况更新路径：向既有模型导入损坏文件 → **模型仍在**（仅库删除，现状保持）；
   - 恢复路径：会话含损坏结果文件 → 恢复不卡死、空模型被清理、后续条目继续。
5. **NativeImportTest**：双配置跑通（无签名耦合，预期零改动）。

## 6. 不做的事（防蔓延）

- 不改 `IPostResultService` 虚签名（失败通知走总线信号，接口保持请求-应答语义）；
- 不做"失败自动重试/自动重建"（重新导入仍是用户显式动作，与既有失效哲学一致）；
- 不把 mxdb 旧格式多文件组合拆成逐文件部分成功；
- 不动导入成功路径的任何行为。
