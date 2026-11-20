# RebuildGUI 老几何栈空窗项替换方案（mGeoPartManager / GN_Part 接线）

> 版本：v1.2（2026-10-15，G1-G6 全部实施完成，见 §6 实施记录）
> 前置文档：[GeoDataBase-GeoDataModelSet-替换核对报告.md](GeoDataBase-GeoDataModelSet-替换核对报告.md)（下称"核对报告"）
> 范围：核对报告 §5 所列 **6 处功能空窗** 的调用点级替换方案。老类按冻结决策不修改（Basic/RebuildGeometry/doc/GN_Part功能对齐与开发方案.md §3.4）；本方案只做 **GUI 侧改线 + 少量新 API 建议**，全部落在新轨（mGeoPartManager / GN_Part / MGeoNeutralQuery / 引擎服务）。

---

## 1. 既有文档替换方案核查结论

对 RebuildGeometry/doc 与 RebuildGUI/docs 全量检索（关键词：edgeTopoTypeMap / expandVirtualTopology / getGeoIsMask / TinySolid / 几何清理 / executeRepair / InfoCalculator / getSetByPartId / PickCommonFunction / setPreWidgetSelectPartID / MGeoNeutralQuery），逐份核对：

| 文档 | 覆盖内容 | 是否含 6 空窗的替换方案 |
|------|----------|------------------------|
| `Basic/RebuildGeometry/doc/GN_Part功能对齐与开发方案.md` | GN 侧 API 补齐（P0 编辑历史/P1 共享拓扑与查询/P2 小项，§3.5 记录 T2~T9 已完成）。M8 边类型=「采用既有等价 edgeTopoTypeMap 不新增 API」（**仅数据位，无 GUI 接线方案**）；M5 虚拓扑扩展=S1 决策归 Mesh 域，「随 Mesh 域需要时承载」（**无实现计划**）；§3.4 明确「双轨期不追求把老类调用方切到新 API」 | ❌ 不含 |
| `Basic/RebuildGeometry/doc/GN_Part功能对齐审查记录.md` | 上述方案的审查记录 | ❌ 不含 |
| `Basic/RebuildGeometry/doc/2026-10-15/01/02-*` | MeshGeoData OCC 摘除第二阶段（Mesh 域），与 GUI 空窗无交集 | ❌ 无关 |
| `2026-10-07/05-直模中性化与MainWindow摘链方案.md` | getGeoIsMask 仅作为「S9 空指针壳」**处置记录**（补前置声明保编译），无替换方案 | ❌ 不含 |
| `2026-10-07/01-全库OCC依赖审计.md` | D-3 一句「几何清理族迁移」去向标注，无方案 | ❌ 不含 |
| `mOccGeoAdapter拆解方案.md` / `OCC适配层接口文档.md` | mOccGeoInfoCalculator 存在性与保留原则；修复族（repairSearchOCC/repairOCC）是后来新增，文档未含"几何清理"整链 | ❌ 不含 |
| 全仓 md | **无任何文档**提及 PickCommonFunction / setPreWidgetSelectPartID / MGeoNeutralQuery（GUI 侧收口无文档先例） | ❌ 不含 |

**结论：6 处空窗均无现成替换方案文档**，需本文档补齐。下述方案中所有"新轨能力已就绪"的论断均经源码核实（引用 文件:行号）。

---

## 2. 空窗项依赖的新轨能力现状（已核实）

| 能力 | 现状 | 证据 |
|------|------|------|
| 边类型数据位 | `GN_TessellationMesh::edgeTopoTypeMap` 已存在（`std::map<int,int>`，语义同老：2=虚拓扑边），且**已随撤消/重做快照序列化**；业务侧零写入者（唯一写入在老类注释体内） | mGeoNFDRenderMesh.h:35；mGeoNeutralSnapshotCodec.cpp:774/789 |
| 边类型渲染消费 | 渲染代理已从 **GN 部件离散数据**读取该表决定虚线/实线（==2→type2 虚线，否则实线）——**消费闭环已在 GN 侧**，只缺写入 | RebuildRender/src/mxrPrivate/data/CCSMVisualDataProxy.cpp:574,624-631 |
| 部件存在性 | `mGeoPartManager::containsPart(int)` | mGeoPartManager.h:52 |
| E→F 邻接关系 | `GN_Part::getEdgeFaceMap()`（跨级祖先表，`map<int,vector<int>>`）+ `commonEdgesOf(f1,f2)` | mGeoNPDPart.h:287,299 |
| 体属性查询 | 引擎 `geoCalculate().getSolidAttr(ids)` → `GeoSolidAttr{id, m_coor, m_volume}` | mGeoEngineServices.h:30；mGeoNTHDGeometricAttr.h:33-38 |
| 部件几何 ID 枚举 | `MGeoNeutralQuery::geoIdsByPartId(mgr, dim, partId)` | mGeoNeutralQuery.h:77 |
| 修复族 | `geoEdit().repairSearchOCC(req)` / `repairOCC(req)`，`GeoRepairType` 现有 RepairSplitPoint / RepairAdditionalEdge / RepairDuplicateFace 三类（**无整件"清理"类型**） | mGeoEngineServices.h:87-88；mGeoEditTypes.h:325-330 |
| 后台属性计算器 | `mOccGeoInfoCalculator`（Qt 后台逐部件算 GeometryProperties，`calculationCompleted` 信号）——存在但当前无 GUI 订阅接线 | mOccGeoInfoCalculator.h:33-85 |

---

## 3. 六项替换方案

### G1（核对报告 §5-3）PickCommonFunction 按部件选择的 geo 分支 —— 一行级改线，P0

**现状**：`src/PreData/PickCommonFunction.cpp:682/743`，`modeldata._geoDataBase->getSetByPartId(id) != nullptr` 恒 false → `setPreWidgetSelectPartID` 的 `geo.push_back(id)` 永不执行（按部件选择时几何侧选择数据不更新）。

**老语义**：该 partId 在几何库中存在 set（即该部件是几何部件而非纯网格部件）→ 加入 geo 选择集。

**替换方案**：等价判据是"该 partId 已注册于 GN 部件管理器"。

```cpp
// mGeoNeutralQuery.h 新增（与既有 helper 同风格）
// 部件是否存在于 GN 注册表（老 _geoDataBase->getSetByPartId(id)!=nullptr 等价）
inline bool containsPart(MGeoManager::GeoManager* mgr, int partId)
{
    GeoNeutralData::mGeoPartManager* pm = partMgr(mgr);
    return (pm != nullptr) && pm->containsPart(partId);
}
```

```cpp
// PickCommonFunction.cpp:682 / 743 两处同改
auto* geoMgr = modeldata.getGeoManager();            // mModelData::getGeoManager() 已存在（mModelData.h:154）
if (MGeoNeutralQuery::containsPart(geoMgr, id))
{
    geo.push_back(id);
}
```

**验证**：导入含几何部件的模型 → 树上按部件多选 → `getGeoSelectData()` 中出现该 partId（选择信息窗口/高亮行为与基线一致）；纯网格部件（无 GN 注册）不入 geo 集，与老判据一致。
**工作量**：≤0.5h（含自测）。**风险**：无（新判据集合语义 ⊆ 老判据——老栈还有"临时未入树 set"的边角，GN 注册表与树/镜像三方一致性由 `mShortcutActionList:708` 刷新链保证）。

---

### G2（§5-5）mSelectMenuFunction::getGeoIsMask —— 函数体重写，P0

**现状**：`src/MainWindow/mSelectMenuFunction.cpp:3255+`，空壳恒返 false（9 个调用点 2968-3194：选择枚举时按部件掩码过滤）。`geoDataModelSet` 恒 nullptr → 掩码过滤失效。

**老语义**：`geoId --(getPartSetByGeo*ID)--> set --> set->getPartID() --> meshData->getGeoPartMask(partId)` 为 true 则该几何被掩码（枚举跳过）。

**替换方案**（原位重写函数体，签名不动——参数 `geoDatabase` 保留但不再使用，可随后续头清理一并移除）：

```cpp
bool mSelectMenuFunction::getGeoIsMask(int id, int dim, mMeshData* meshData,
    mxr::mGeoModelData1* geoModelData, MGeoData::GeoDataBase* geoDatabase)
{
    Q_UNUSED(geoDatabase); Q_UNUSED(geoModelData);
    if (meshData == nullptr) { return false; }
    auto* mgr = (getCVM != nullptr && getCVM->getModelData() != nullptr)
        ? getCVM->getModelData()->getGeoManager() : nullptr;
    const int partId = MGeoNeutralQuery::partIdByGeoId(mgr, dim, id);   // 老反查的 GN 等价（核对报告 §3.1）
    if (partId < 0) { return false; }
    return meshData->getGeoPartMask(partId);                            // 掩码数据源不变，仍是 mMeshData
}
```

要点：`partIdByGeoId` 未命中返 -1 → 不掩码，与老"set 为 null 时不判掩码"分支一致；掩码数据本身（`mMeshData::getGeoPartMask`）从未迁移，直接沿用。

**验证**：对某部件设置掩码（隐藏不参与选择）→ 基线行为：该部件的点/线/面/体不出现在选择枚举；重写后同。手测 4 维度各一例。
**工作量**：≤0.5h。**风险**：无。

---

### G3（§5-1）mMeshThread 边类型写入（抑制边/恢复边显示）—— P1

**现状**：`src/MainWindow/mMeshThread.cpp:2363/3115`，`getGeoData()->setGeoDiscreteDataEdgeTopoType(typeOfEdge)` 空壳 → 抑制边（typeOfEdge=2）/恢复（=1）后渲染不区分虚线。两处上文已经用 `MGeoNeutralQuery::partIdByGeoId` 收集了 `pickPardID`（2352/3104），即**所属部件 ID 现成可得**，只差写表。

**替换方案**（首选 B，零新 API；A 作为后续整洁项）：

- **B（GUI 侧 read-copy-write，推荐先行）**：

```cpp
// mMeshThread.cpp 两处，替换 setGeoDiscreteDataEdgeTopoType(typeOfEdge) 调用：
auto* partMgr = modelData->getGeoManager()->getGeoPartMgr();
for (int partId : pickPardID)                       // 受影响部件逐一写入
{
    auto* part = partMgr ? partMgr->getPartPtrById(partId) : nullptr;
    if (part == nullptr) { continue; }
    GeoNeutralData::GN_TessellationMesh disc = part->getGeoDiscreteData();   // 值拷贝
    for (const auto& p : typeOfEdge) { disc.edgeTopoTypeMap[p.first] = p.second; }
    part->setGeoDiscreteData(disc);                 // 整表回写（GN_Part 既有 API）
}
// 其后既有的 getCVM->updateAllModelOperate(ReplaceOnePart, pickPardID) 不变——
// 渲染代理重建时读 gnPart.getGeoDiscreteData().edgeTopoTypeMap（CCSMVisualDataProxy.cpp:624），
// 值 2 → 虚线，其余实线：与老链显示语义一致。
```

- **A（GN 小 setter，整洁化，可与 B 二选一或后续替换 B）**：`GN_Part` 增加 `void setEdgeTopoType(int edgeId, int type) { m_geoDiscreteData.edgeTopoTypeMap[edgeId] = type; }`（两行，非虚拓扑设施，不违反 S1 决策；对齐方案 M8 的"不新增 API"针对计数族，写入器建议补）。

**验证**：① 抑制某边 → 渲染变虚线；恢复 → 实线（基线同款操作对照）。② 撤消/重做后边类型保持（快照 codec 已序列化该表，撤消恢复链理论成立，实测一遍）。③ 部件被引擎操作重建后表清空（老语义"部件更新时清空"，GN 重建路径自然满足——顺手验证）。
**工作量**：B ≈ 1h（含手测）；A 另 +0.5h（GN 库 + 重装）。**风险**：低——`getPartPtrById` 返回管理器存储内指针，写后立即被同一 GUI 线程读取，无并发面。

---

### G4（§5-2）PickCommonFunction 虚拓扑面扩展拾取 —— P1

**现状**：`src/PreData/PickCommonFunction.cpp:136-140`，`getPartSetByGeoFaceID` 恒 null + `expandVirtualTopologyFaces` 空壳 → 点选虚拓扑相连面组时不再自动扩展。

**老语义**（mGeoDataBase.cpp:970-1030 注释体）：从选中面出发，凡被 `edgeTopoTypeMap==2` 的边相连的面归入同组，递归到闭包。

**替换方案**：在 `MGeoNeutralQuery` 新增纯组合 helper——**只读** GN 既有数据（`GN_Part::getEdgeFaceMap()` + `GN_TessellationMesh::edgeTopoTypeMap`），不新增任何 GN 结构，与 S1 决策（PVT 设施归 Mesh 域）不冲突：

```cpp
// mGeoNeutralQuery.h 新增
// 虚拓扑相连面闭包扩展（老 GeoDataBase::expandVirtualTopologyFaces 的 GUI 等价）：
// 与 seed 面同部件、且经 edgeTopoTypeMap==2 的边连通的面全部并入返回集
inline std::set<int> expandVirtualTopologyFaces(MGeoManager::GeoManager* mgr,
                                                const std::set<int>& seedFaceIds)
{
    std::set<int> expanded(seedFaceIds);
    if (mgr == nullptr || seedFaceIds.empty()) { return expanded; }
    auto* pm = partMgr(mgr);
    if (pm == nullptr) { return expanded; }

    for (int partId : pm->getPartList())
    {
        const auto disc = pm->getPartById(partId).getGeoDiscreteData();
        // 1) 收集本部件全部虚拓扑边（type==2）
        std::set<int> vtEdges;
        for (const auto& kv : disc.edgeTopoTypeMap)
            if (kv.second == 2) { vtEdges.insert(kv.first); }
        if (vtEdges.empty()) { continue; }
        // 2) 由 E→F 祖先表建"面-面经 VT 边相邻"关系（面并查集）
        const auto& e2f = pm->getPartById(partId).getEdgeFaceMap();
        std::map<int, std::vector<int>> vtNeighbors;   // face -> 经 VT 边相邻的 faces
        for (int e : vtEdges)
        {
            auto it = e2f.find(e);
            if (it == e2f.end()) { continue; }
            for (int f1 : it->second)
                for (int f2 : it->second)
                    if (f1 != f2) { vtNeighbors[f1].push_back(f2); }
        }
        // 3) 自 seed 做 BFS 闭包（seed 命中任一面即从该面起扩散）
        std::set<int> seeds;
        for (int f : expanded)
            if (!vtNeighbors.count(f) == false || true) { }   // 见下：先判 seed 是否在本部件
        // —— 实现时以 disc.facePoints.count(f) 判定 f 属于本部件，再 BFS；
        //    未命中任何 seed 的部件直接跳过（老实现按 set 归属同样隔离在单部件内）
    }
    return expanded;
}
```

> 注：上面第 3 步以伪代码示意；落地实现为「判定 seed 面属于本部件（`disc.facePoints.count(faceId)`）→ 队列 BFS 扩散 `vtNeighbors`」。BFS 闭包与老递归 `expandVirtualTopologyFaces` 等价（老实现同为沿 VT 边递归、部件内闭环）。

```cpp
// PickCommonFunction.cpp:129-142 改写（原 getPartSetByGeoFaceID + expandVirtualTopologyFaces 两调用）：
if (final_type == SelectionInfoType::GeoFace)
{
    std::set<int> seed(final_ids.begin(), final_ids.end());
    const auto expanded = MGeoNeutralQuery::expandVirtualTopologyFaces(
        widget_ptr->getModelData().getGeoManager(), seed);
    final_ids.insert(final_ids.begin(), expanded.begin(), expanded.end());
}
```

**验证**：先用 G3 的写入路径把两条边标为 VT 边（或导入含 VT 边的模型）→ 点选其一相邻面 → 同组面全部进入选中集（与基线行为对照）；普通面不受影响（无 type==2 边时扩展集 == 种子集）。
**工作量**：2-3h（helper + 接线 + 手测）。**风险**：低；性能为 O(部件×VT边×邻接面)，远小于老实现（老实现每次拾取逐面递归走 TopExp 遍历）。

---

### G5（§5-4）mGeoEditPanel::slot_checkTinySolid（查找小体）—— P1

**现状**：`src/MainWindow/mGeoEditPanel.cpp:3669`，`geoData->getPartSet()` 恒空 + 内部用空属性结构体（注释：待 mOccGeoInfoCalculator 接入恢复）。

**老语义**：遍历部件 → 体积缓存（mGeoInfoCalculate）→ 过滤 `0 < volume ≤ maxVolume` → 高亮。

**替换方案**：不需要后台计算器缓存——引擎已有同步属性查询，一次调用即得（老链的缓存是为大模型后台算力兜底，首版先同步，卡顿再切 `mOccGeoInfoCalculator` 异步订阅）：

```cpp
void mGeoEditPanel::slot_checkTinySolid()
{
    const double maxVolume = tinySolidMaxVolume->text().toDouble();
    if (maxVolume <= 0) { /* 原提示保留 */ return; }

    auto* mgr = getCVM->getModelData()->getGeoManager();
    std::vector<int> solidIds;
    for (int partId : MGeoNeutralQuery::/*新增*/partList(mgr))          // 或直接 mgr->getGeoPartMgr()->getPartList()
        for (int sid : MGeoNeutralQuery::geoIdsByPartId(mgr, 3, partId))
            solidIds.push_back(sid);
    if (solidIds.empty()) { /* 未找到提示 */ return; }

    const auto attrs = getGEOServer.geoCalculate().getSolidAttr(solidIds);   // GeoSolidAttr{id, m_volume}
    std::vector<int> highLightSolids;
    for (const auto& a : attrs)
        if (a.m_volume > 0 && a.m_volume <= maxVolume) { highLightSolids.push_back(a.id); }

    /* 其后高亮代码不变（UpdatePickData GeoSolid + UpdatePreWidgetHighlight + updatePaint） */
}
```

删除 `getPartSet()` 遍历与空 `props` 占位结构。`MGeoNeutralQuery` 顺手补 `partList(mgr)` 一行 helper（返回 `pm->getPartList()`，空安全）。

**验证**：含小体的装配模型（如 temp_model.stp 的立方体旁放小圆柱）→ 阈值内只高亮小体；阈值 0/负数提示不变。体积数值可与 mGeoInfoPanel（GN_Part 信息窗口，已用 GN 链）显示交叉对拍。
**工作量**：1-2h。**风险**：低；大模型同步查询耗时→ 若实测 >1s，二期改 `mOccGeoInfoCalculator::queuePartsCalculation + calculationCompleted` 异步（能力已在，mOccGeoInfoCalculator.h:85）。

---

### G6（§5-6）mMenuWidget 几何清理 —— 依赖引擎新修复类型，P2

**现状**：按钮提示"已切换至引擎修复管线（迁移中），暂不可用"（mMenuWidget.cpp:5276-5280）。老链语义（基线 4849-4870）：逐部件 `doGeoClean(shape)`（ShapeFix 级整件修复）→ 新 set 替换 + `updateEditPara` 旧→新 ID 映射 → `editGeoModelSignal`。

**缺口**：引擎 `GeoRepairType` 只有三个定点类型（分割点/多余边/重复面），**无整件清理**；整件 heal + 编辑历史迁移需引擎/适配层配合（mOccGeoAdapter 的 editRepair 族扩展）。

**替换方案**（两步）：

1. **引擎侧（前置依赖，Basic/RebuildGeometry）**：`mGeoEditTypes.h` 的 `GeoRepairType` 增加枚举 `RepairGeometryClean /*整件几何清理*/`；`mGeoEditService::editRepair` 分发新增分支 → OCCAdapter 实现（`mOccPartOperations` 邻域，`ShapeFix_Shape` + 既有点焊合，输出走**已有** `mOccUpdateTranslator::updateFromNative` 重识别链——编辑历史/存活映射/createdIds 自动落 GN_Part，对齐方案 §3.5 T3 接线即为此设计）。
2. **GUI 侧（引擎合入后）**：

```cpp
connect(geoCleanBtn, &QToolButton::clicked, this, [this]() {
    try {
        auto* mgr = getCVM->getModelData()->getGeoManager();
        std::set<int> partIds(mgr->getGeoPartMgr()->getPartList().begin(),
                              mgr->getGeoPartMgr()->getPartList().end());
        bool allOk = true;
        for (int pid : partIds) {
            EventDispatcher::mGeoRepairRequest req;
            req.repairType = EventDispatcher::RepairGeometryClean;
            auto res = getGEOServer.geoEdit().repairOCC(req);
            for (auto& o : res.modifiedObjectIds)
                getGEOServer.context().PartManager()._editPart.insert(o.id);   // 编辑簿记（对齐 appendEditSet）
            allOk = allOk && res.succeeded();
        }
        MxFunctions::mGlobalSignals::getInstance()->editGeoModelSignal_2(
            getCVM->getModelData()->getModelDataID());
        /* 成功/失败提示，风格同基线 */
    } catch (...) { /* 失败提示 */ }
});
```

> 老链的"逐 set 替换 + updateEditPara + appendEditSet/removeSet"在引擎路径中的等价物 = 适配层重识别写 GN 编辑历史（T3 已接线）+ `_editPart` 簿记 + `editGeoModelSignal_2` 视图刷新（核对报告 §3.3 已验证的布尔/编辑同款收口），无需 GUI 手工建 set。

**验证**：构造带缝隙/坏面的模型 → 清理 → ①提示成功且树/渲染刷新；②旧 ID 引用（网格组、选择集）经 `GN_Part::editHistoryNewIds` 可查存活映射（对齐方案 §6-2 口径：比对"存活集合"）；③撤消可回退。
**工作量**：引擎侧 1-2 天（ShapeFix 管线 + 重识别回归）；GUI 侧 0.5 天。**依赖**：引擎修复族排期（本方案仅登记，不阻塞 G1-G5）。**过渡态**：维持现状提示，不算静默丢失。

---

## 4. 实施顺序与总账

| 顺位 | 项 | 改动面 | 新增 API | 工作量 | 依赖 |
|------|----|--------|----------|--------|------|
| 1 | G1 部件选择 geo 分支 | GUI（mGeoNeutralQuery + PickCommonFunction） | `containsPart` 内联 | ≤0.5h | 无 |
| 2 | G2 getGeoIsMask | GUI（mSelectMenuFunction） | 无 | ≤0.5h | 无 |
| 3 | G3 边类型写入 | GUI（mMeshThread ×2）（可选 GN setter） | 无（B）/ `setEdgeTopoType`（A） | 1-1.5h | 无 |
| 4 | G4 VT 面扩展 | GUI（mGeoNeutralQuery + PickCommonFunction） | `expandVirtualTopologyFaces` 内联 | 2-3h | G3（需先有 type==2 写入途径便于构造测试场景） |
| 5 | G5 查找小体 | GUI（mGeoEditPanel） | `partList` 内联 | 1-2h | 无 |
| 6 | G6 几何清理 | 引擎（RebuildGeometry）+ GUI（mMenuWidget） | `RepairGeometryClean` 枚举 + editRepair 分支 | 引擎 1-2d + GUI 0.5d | 引擎修复族排期 |

共性原则：

1. **不动冻结岛**——老类零修改；所有改线只发生在 GUI 调用点与 `MGeoNeutralQuery`（零 OCC 内联头文件）。
2. **G1/G2/G5 是纯 GUI 改线**（现成 API），可先行合入；G3/G4 组成"边类型显示"功能闭环（写入+消费+扩展），建议同一批交付并共用验证场景；G6 单独跟引擎排期。
3. 完成后可将核对报告 §5 的 6 项逐一销项，并把 §7 建议收口顺序表更新为完成态；届时 GUI 激活代码中对老类的成员调用将归零（仅剩类型前置声明与 `mViewportModel:1178` 的兼容信号连接——后者由 GeoManager 双发机制供养，属设计内保留）。

## 5. 回归验证清单（全批合一）

1. 按部件多选 → 几何选择集更新（G1）；
2. 部件掩码后选择枚举排除该部件 4 维度几何（G2）；
3. 抑制/恢复边 → 虚线/实线渲染 + 撤消重做保持 + 部件重建后清空（G3）；
4. 虚拓扑边组内点选单面 → 同组面全选（G4）；
5. 小体阈值查找 → 命中集高亮与 GN 信息窗口体积一致（G5）；
6. （G6 合入后）脏模型清理成功、旧 ID 存活映射可查、可撤消。

---

## 6. 实施记录（2026-10-15，v1.1）

### 6.1 代码变更（G1-G5 已全部落地，改动 4 文件）

| 项 | 文件 | 变更 |
|----|------|------|
| G1 | `src/PreData/PickCommonFunction.cpp` | 新增 `#include <mGeoPartManager.h>`；`setPreWidgetSelectPartID`（:683）与 `addPreWidgetSelectPartID`（:745）两处 `modeldata._geoDataBase->getSetByPartId(id)!=nullptr` → `modeldata.getGeoPartMgr()->containsPart(id)`（经 ZsModelData 直存的部件管理器，空指针保护）。**与方案的差异**：未加 `MGeoNeutralQuery::containsPart` helper——ZsModelData 本身直存 `_geoPartMgr`（ZsModelData.h:27/48），PreData 库直接用其 const 访问器更短，且避免了跨库 include MainWindow 头 |
| G2 | `src/MainWindow/mSelectMenuFunction.cpp` | `getGeoIsMask` 函数体整体重写（:3255）：`MGeoNeutralQuery::partIdByGeoId(mgr, dim, id)` 反查部件 → `meshData->getGeoPartMask(partId)`；未命中（-1）不判掩码，同老 null 分支。旧空壳体（switch×4 + 恒假判断 + 注释块）删除 |
| G3 | `src/MainWindow/mMeshThread.cpp` | 文件级匿名命名空间新增 `setEdgeTopoTypeToNeutralData(partMgr, partIds, edgeTypeMap)`（GN_Part 离散表 read-copy-write）；:2381 与 :3133 两处老 `setGeoDiscreteDataEdgeTopoType` 空壳调用替换为该 helper（部件集合复用既有 `pickPardID`），其后 `updateAllModelOperate(ReplaceOnePart,…)` 不变（渲染代理按 CCSSMVisualDataProxy.cpp:624 读该表区分虚线/实线） |
| G4 | `src/PreData/PickCommonFunction.cpp` | 匿名命名空间新增 `expandVirtualTopologyFacesGN(partMgr, seedFaceIds)`：按部件过滤种子面（`facePoints` 键命中）→ 虚拓扑边（`edgeTopoTypeMap==2`）经 `GN_Part::getEdgeFaceMap()` 建"面-面相邻"→ BFS 闭包；:131-141 老两段调用（恒 null 反查 + 空壳递归）替换为该 helper 一次调用 |
| G5 | `src/MainWindow/mGeoEditPanel.cpp` | `slot_checkTinySolid` 重写（:3665）：GN 部件枚举（`getPartList()` + `MGeoNeutralQuery::geoIdsByPartId(mgr,3,partId)`）收集实体 ID → 引擎 `getGEOServer.geoCalculate().getSolidAttr(ids)` 过滤 `0<m_volume<=maxVolume` → 高亮尾段不变。老 `getPartSet()` 空遍历 + 空 props 占位结构删除。**未引入 `MGeoNeutralQuery::partList` helper**——`getGeoPartMgr()->getPartList()` 就地调用更直接 |
| G6 | 引擎 + GUI | **已实施（v1.2）**，共 4 处改动：① `RebuildGeoData/EventDispatcher/mGeoEditTypes.h`：`GeoRepairType` 新增 `RepairGeometryClean`（整件清理，忽略 ID 入参）；② `RebuildGeometry/OCCAdapter/mOccGeoEdit.h/.cpp`：新增静态 `executeGeometryClean(srcCompounds, msg)`——老 `mGeoCommonFunction::doGeoClean`（ShapeFix_Shape 全模式、精度 1e-6 默认）等价移植，仅返回拓扑实际变化（非 `IsSame`）的部件，单件异常跳过不中断批次；③ `mOccRepairAdapter.cpp::executeRepair` 新增分支：全部件源 Compound 收集（同 executeDuplicateLineFix 模式）→ 清理 → 共用收口 `updatePart_EditShape(nullptr history)`（重识别链自动写 GN 编辑历史，等价老 updateEditPara）；**全部件无可清理项返回无操作成功**（对齐老链"清理成功"提示语义，不走共用收口的空表失败分支）；④ `RebuildGUI/mMenuWidget.cpp` 几何清理按钮：`repairOCC(RepairGeometryClean)` 单请求全量清理 → 失败透出引擎消息；`modifiedObjectIds` 逐一登记 `PartManager()._editPart`（对齐老 appendEditSet）→ `editGeoModelSignal_2` 刷新 + 成功提示。**与方案的差异**：GUI 侧由"逐 part 循环请求"简化为单请求（适配器本就多部件批量处理，语义相同、往返更少） |

### 6.2 构建验证

- **G1-G5（首轮）**：`build.py --stage build --only RebuildGUI --skip-git --skip-conan` —— 4 个改动翻译单元全部重编（`PickCommonFunction.obj` 等），`MechPreDatad.dll`、`MechMainWindowd.dll`、`MxMech.exe` 产出新二进制。
- **G6（第二轮，含引擎依赖链）**：同命令重跑 —— `mOccGeoEdit.obj`/`mOccRepairAdapter.obj` 18:51 重编，`OCCAdapterd.dll` 18:52 重链，`mMenuWidget.obj` 18:54 重编（`RepairGeometryClean` 枚举经 install 头链传递成功），`MechMainWindowd.dll`/`MxMech.exe` 18:55 产出。RebuildGeoData（枚举所在，纯头消费）随依赖链重建。
- 两轮 `RebuildMesh` 模块均构建失败，**与全部改动无关**：失败点为构建后步骤 `cmake -E copy Qt5Cored.dll → Basic/RebuildMesh/bin/Debug`（MSB3073，目标文件被占用/环境问题），非编译错误，且未改动该模块任何源码。

### 6.3 探针级运行验证（G6 引擎链，2026-10-15）

独立探针 `temp/temp_test_g6cleangeo.cpp`（T3b' 独立句柄路径：`defaultInstance → importStepShapeAsPart → editRepair(RepairGeometryClean) ×2 → getTopologyByDimension/editHistoryNewIds 核对`），4 个 STEP 模型全部 **PROBE PASS**：

| 模型 | 导入 | clean#1 | 清理后核对 | clean#2（幂等） |
|------|------|---------|-----------|----------------|
| torus_model.stp（1 面/2 边） | ok | 无操作成功（modified=0） | 拓扑不变、部件完好 | 无操作成功 |
| m12333.step（36 面/72 边） | ok | **modified=1（实际清理）** | 拓扑数量不变；**36 面全部获得编辑历史存活映射（0→36，重识别链写入，等价老 updateEditPara 旧→新 ID 语义）** | 无操作成功 |
| sat1.stp（84 面/202 边） | ok | 无操作成功 | 拓扑不变 | 无操作成功 |
| step_roundtrip.step（87 面/198 边） | ok | 无操作成功 | 拓扑不变 | 无操作成功 |

探针抓到并已修复一处实现缺陷：适配器 `executeRepair` 的 `error` 初值 `"failed to execute"` 非空，导致"全部件无可清理"被误报为失败——修复为调用 `executeGeometryClean` 前 `error.clear()`（空 msg = 无操作的契约由此成立）；修复后 RebuildGeometry 3/3 重编通过。验证结论：**变化检测（IsSame）、修改路径（ShapeFix→updatePart_EditShape→存活映射）、幂等性、无操作成功语义全部符合设计**。

### 6.4 崩溃修复（2026-10-16）：进入线/面/体网格面板访问冲突

**现象**：进入线网格/面网格/体网格面板即异常（读取访问权限冲突，`this->_Ptr = 0xFFFFFFFFFFFFFFFE`，MechMainWindow.dll），三面板同一根因。

**根因**（S9 期遗留，非本次 G 批引入）：`mGeoNeutralQuery.h` 的 `geoIdsByPartId` 原写法

```cpp
const GN_TessellationMesh& disc = pm->getPartById(partId).getGeoDiscreteData();
```

`getPartById` 按值返回**临时** GN_Part，`.getGeoDiscreteData()` 返回指向该临时的引用——语句结束临时即析构（"经函数 const 引用返回的间接绑定"不触发生命周期延长），`disc` 悬空；随后 `for (kv : disc.facePoints)` 遍历已析构的 std::map，读到的正是 MSVC Debug 堆释放内存的 0xFE 填充 → `0xFFFFFFFFFFFFFFFE`。三面板入口 `updatePanel→refreshGeoComboBox→geoIdsByPartId`（每个部件调用一次）共用此 helper。

**修复**：改 `getPartPtrById` 指针取部件（免整部件拷贝，unordered_map 节点地址稳定、引用始终有效），附空指针保护。全库扫描 `getPartById(...).get*(...)` 链式用法：**仅此一处**把链式结果绑到引用/用于遍历（其余均在语句内即时消费；G3/G4 的实现用命名局部/指针，均安全；`allPartNames` 即时消费安全）。该修复同时保护 G5 `slot_checkTinySolid`（同 helper 消费方）。

**验证**：RebuildGUI 重编 11/11 绿（首次失败系用户挂死的 MxMech 进程锁 bin/Debug，终止后通过）；mLineMeshPanel/mFaceMeshPanel/mSolidMeshPanel obj 全部重编，MechMainWindowd.dll 新产出。待用户 GUI 复测三面板入口确认。

### 6.5 残余事项

1. §5 回归验证清单 1-5 及 6 的 GUI 交互面（按钮点击、树/渲染刷新）仍需**人工在 GUI 环境执行**后销项——G6 引擎链已由 §6.3 探针验证，GUI 侧仅需点按按钮确认提示与刷新；
2. G6 引擎侧 `executeGeometryClean` 的 ShapeFix 模式集为老 doGeoClean 逐项等价移植（含 `FixTailMode` 等），行为对齐依赖手测确认；若清理后渲染/剖分异常，优先排查 `IsSame` 变化判定与重识别链的映射；
3. 手测通过后，核对报告 §5 的 6 项空窗可全部标记收口，GUI 激活代码中对老类的成员调用归零（仅剩设计内保留项：类型前置声明与 mViewportModel 的兼容信号连接）。
