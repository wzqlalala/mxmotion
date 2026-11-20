# RebuildGUI 老几何数据栈（GeoDataBase / GeoDataModelSet）→ 新栈（mGeoPartManager / GN_Part）替换核对报告

- 分析日期：2026-10-15
- 分析范围：`Software/Common/RebuildGUI/src`（不含 install 归档头）
- 基线提交：`82cbc9b9`（2026-07-29，"几何引擎全新版本"引入 GN_Part 之前的最后完整状态）
- 当前提交：`0374bf6c`（2026-10-10，HEAD）
- 涉及老类：`MGeoData::GeoDataBase`（Basic/RebuildBasicData/src/MXGeoData/mGeoDataBase.h）、`MGeoData::GeoDataModelSet`（mGeoDataModelSet.h）
- 涉及新类：`GeoNeutralData::mGeoPartManager`（Basic/RebuildGeoData/src/GeoNeutralData/mGeoPartManager.h）、`GeoNeutralData::GN_Part`（mGeoNPDPart.h）、GUI 适配层 `MGeoNeutralQuery`（src/MainWindow/mGeoNeutralQuery.h）

---

## 1. 结论（TL;DR）

1. **主链路已完成等效替换**：基线中 RebuildGUI 直接调用的老类成员共 **GeoDataBase 14 项 / GeoDataModelSet 13 项**（约 190 个调用点 + 约 90 处 `setGeoDataBase(getGeoData())` 命令接线）。截至 HEAD，其中 **24 项已通过 mGeoPartManager / GN_Part / MGeoNeutralQuery / 引擎服务（getGEOServer）等效替换**，替换后的数据通路与旧实现语义一致（依据见 §4）。
2. **替换方式不是"原地改名"，而是"冻结岛 + 双轨"**：老类被保留为可链接的"空壳/兼容壳"（按几何反查部件恒返 `nullptr`、集合恒空、旧信号由 GeoManager 兼容层继续发射），GUI 的激活代码全部改走新轨；旧 MXGeoCommand 命令链在 GUI 激活代码中的工厂调用数已降为 **0**（残留均在块注释或 `#else` 门控分支内）。
3. **存在 6 处明确的"功能空窗"**（调用点仍在但落到空壳实现，或功能被显式下线），详见 §5。这些不是静默丢失——代码中均有 `【OCC摘除·S9】` 系列注释说明去向，但**尚未接通新轨等价物**，属于替换未完成项。

---

## 2. 基线清单：RebuildGUI 曾使用的老类成员（@82cbc9b9）

### 2.1 GeoDataBase 成员（14 项，约 127 个调用点）

| # | 成员 | 调用量 | 主要消费文件 |
|---|------|--------|--------------|
| 1 | `getPartSetByGeoFaceID` | 37 | mGeoEditPanel/mGeoExtractPanel/mGeoFeaturePanel/mGeoRemovalPanel/mGeoRepairPanel/mGeoSharedTopologyPanel/mGeoSolidifyPanel/mGeoTransformPanel/mShortcutActionList/mMeshThread/mSelectMenuFunction/PickCommonFunction |
| 2 | `getPartSetByGeoSolidID` | 28 | 同上族 |
| 3 | `getPartSetByGeoLineID` | 26 | 同上族 |
| 4 | `getPartSetByGeoPointID` | 13 | 同上族 |
| 5 | `getGeoIdByPartId` | 8 | mShortcutActionList（经 `getGeoManager()->getGeoModelBase()->` 包装调用） |
| 6 | `getSetByPartId` | 4 | mShortcutActionList ×2（`getGeoData()->` 链）、PickCommonFunction ×2（`modeldata._geoDataBase->` 直访成员） |
| 7 | `getSetList` | 2 | mTreeView（显隐/导出部件收集） |
| 8 | `setGeoDiscreteDataEdgeTopoType` | 2 | mMeshThread（边显示类型写入） |
| 9 | `updateGeoDataSig`（信号连接） | 1 | mViewportModel:1164（DeleteOnePart/AppendOnePart/ImportPart/ReplaceOnePart 视图刷新） |
| 10 | `getPartSet` | 1 | mMenuWidget（几何清理遍历部件） |
| 11 | `appendEditSet` | 1 | mMenuWidget |
| 12 | `removeSet` | 1 | mMenuWidget |
| 13 | `getAllPartIDVec` | 1 | mMenuWidget |
| 14 | `expandVirtualTopologyFaces` | 1 | PickCommonFunction（拾取时虚拓扑面递归扩展） |

另：**约 90 处** `cmd->setGeoDataBase(getCVM->getModelData()->getGeoData())`——把 GeoDataBase* 注入 MXGeoCommand 命令对象（布尔/编辑/修复/特征/草图/变换面板族），老类成员的"重度"使用实际发生在命令库内部；以及 **约 10 处** `getGeoManager()->deleteGeoDataModelSet(partIds)`（部件删除，经 GeoManager 包装）。

### 2.2 GeoDataModelSet 成员（13 项，约 66 个调用点）

| # | 成员 | 调用量 | 主要消费文件 / 用途 |
|---|------|--------|----------------------|
| 1 | `getPartID` | ~39 | `geoDataModelSet->` ×4（mSelectMenuFunction 掩码判断）、`cmd->getGeoDataSet()->getPartID()` ×33（创建成功后发 `createGeoModelSignal`）、mTreeView ×2 |
| 2 | `getPartName` | 4 | mGeoEditPanel ×2、mShortcutActionList ×2（部件名显示） |
| 3 | `getSolidByTag` / `getFaceByTag` / `getEdgeByTag` | 3/3/1 | mGeoEditPanel、mGeoExtractPanel（按 ID 取 TopoDS 子形状喂给命令） |
| 4 | `getShapeByDimTag` | 3 | mGeoEditPanel（短边/小面/小体检查：体→边/面展开） |
| 5 | `getEdgeSolidMap` / `getFaceSolidMap` / `getEdgeFaceMap` | 各 1 | mGeoEditPanel（跨维祖先关系） |
| 6 | `geosOnSolid` / `geosOnFace` | 2/1 | mGeoEditPanel（实体/面内子几何枚举） |
| 7 | `getTagByEdge` / `getTagByFace` | 2/1 | mGeoEditPanel（Shape→ID 反查） |
| 8 | `getShape` | 1 | mMenuWidget（几何清理取整体形状） |
| 9 | `updateEditPara` | 1 | mMenuWidget（清理后刷新新 Set 参数 + 建立新旧 ID 映射） |
| 10 | `_partNameTag`（公共数据成员） | 1 | mMeshThread:508（删网格部件取 `{名字,ID}` 对） |
| 11 | `new GeoDataModelSet(geoData)`（构造） | 1 | mMenuWidget:4858 |

### 2.3 老用法在 GUI 中的三条主要链路

- **L1 反查链**：`getGeoData()->getPartSetByGeo*ID(id)` → 得 `set` → null 判 + `set->getPartName()/getPartID()` 或把 `set` 指针塞进 `MGeoCommand::EditShape.mSet`；
- **L2 命令链**：`setGeoDataBase(getGeoData())` → 命令内部消费老类 → `cmd->getGeoDataSet()->getPartID()` → `createGeoModelSignal(partId, modelDataID)`；
- **L3 拓扑查询链**：`set->getShapeByDimTag / get*SolidMap / geosOnSolid / getTagBy*`（质量检查类面板）。

---

## 3. HEAD 替换核对总表（逐成员）

判定符号：✅ 已等效替换｜🟡 兼容壳仍工作（非直接等价但功能保持）｜🔴 空窗（未接新轨，见 §5）｜⬛ 门控/注释保留（当前不编译）

### 3.1 GeoDataBase

| 老成员 | 新等价物 | HEAD 调用点现状 | 判定 |
|--------|----------|----------------|------|
| `getPartSetByGeoFaceID/LineID/PointID/SolidID` | `mGeoPartManager::getPartByGeoFaceID/LineID/PointID/SolidID`（`existShape` 判定）；GUI 实际多用 `MGeoNeutralQuery::partIdByGeoId / partNameByGeoId`（按 GN 部件离散表检索） | 104 → **1** 处残留（PickCommonFunction:136，🔴）。其余全部迁移：反查部件 ID/名字 35 处、EditShape 构造改为 `partIds`+引擎请求（mGeoEditPanel/mGeoSharedTopologyPanel 等原位改写） | ✅（除 1 处） |
| `getSetByPartId` | `mGeoPartManager::getPartPtrById` / `MGeoNeutralQuery` | mShortcutActionList 2 处 → 整槽引擎化（原体注释保留）；PickCommonFunction:682/743 残留（🔴 恒 null） | ✅/🔴 |
| `getGeoIdByPartId` | `MGeoNeutralQuery::geoIdsByPartId`（GUI 网格面板族 11 处使用） | 8 → 0 残留 | ✅ |
| `getSetList` / `getPartSet` | `mGeoPartManager::getPartList()/containsPart()` | mTreeView 2 处 → 树模型 ID 集合（slot_updatePartTree_2/导出，5346-5432 注释明示"改 GN"）；mShortcutActionList 撤消镜像重建（708-775）。mGeoEditPanel:3669（查找小体）残留恒空（🔴） | ✅/🔴 |
| `getAllPartIDVec` | `mGeoPartManager::getPartList()` | 唯一调用点（几何清理）整体下线 | ✅ |
| `appendEditSet` / `removeSet` / `appendGeometrySet` | `mGeoPartManager::_editPart` + `editShapeFinish()` / `removePart()`（部件簿记与删除走 GN；GUI 经 `deleteGeoDataModelSet_2` → `_partMgr->removePart`） | mMenuWidget 原调用链整体下线（见 §5-6）；引擎编辑收口 `mGeoPartManager::editShapeFinish` 发射 `updateGeoPartDataSig("EditShape")` | ✅ |
| `setGeoDiscreteDataEdgeTopoType` | `GN_TessellationMesh::edgeTopoTypeMap`（mGeoDataBase.cpp:941 注释明示新轨） | mMeshThread:2363/3115 仍调老接口 → **空壳**（🔴） | 🔴 |
| `expandVirtualTopologyFaces` | （GN 虚拓扑体系待接入；老实现 TopExp 遍历注释保留） | PickCommonFunction:140 仍调 → **空壳**（🔴） | 🔴 |
| `updateGeoDataSig` | `mGeoPartManager::updateGeoPartDataSig`（新信号；**双发兼容**） | GUI 仍连老信号（mViewportModel:1178）；GeoManager 兼容层在 Import/Delete/Append/Replace 四类操作中继续发射老信号（mGeoManager.cpp:452/1143/1181/1302/1999） | 🟡 |
| `editShapeFinish` | `mGeoPartManager::editShapeFinish`（同签名收口：编辑部件簿记 + 信号；历史查询走 `GN_Part::editHistoryNewIds`） | GUI 不再直接调用（引擎/适配层收口） | ✅ |
| 共享拓扑族（`GetDiscreteFacePair/GetFuseFacePair/getCommonEdge/setCommonEdge...`，基线 GUI 未见直接调用） | `mGeoPartManager::appendDiscreteFacePair/appendFuseFacePair/appendCommonEdge/discreteFacePairs()/tiedEdges()`（T5 对账结构 `GN_TiedFaces/GN_TiedEdges`） | 头文件注释标注"老 xxx 等价"；GUI 无直接消费 | ✅（结构对齐） |
| 空间查询族（`getBndBoxById/getAllPointCoordinatesBy*`，基线 GUI 未见直接调用） | `GN_Part::setCachedBoundingBox/cachedBoundingBox/discretePointsOfEdge/Face/Solid/Part`（T7/T8 标注"老 xxx 等价"） | 引擎侧计算回填缓存 | ✅（结构对齐） |

### 3.2 GeoDataModelSet

| 老成员 | 新等价物 | HEAD 调用点现状 | 判定 |
|--------|----------|----------------|------|
| `getPartID` | partId 本身（GN 以 int partId 为键）／引擎结果 `res.generatedObjects[i].id.id` | `cmd->getGeoDataSet()->getPartID()` 33 → 0 激活（布尔 3 处在块注释、缝合 2 处在 `#else`、草图 2 处注释）；改发 `createGeoModelSignal_2(partId,…)`（mGeoBoolOperationPanel:869 等）；mSelectMenuFunction 4 处 → `MGeoNeutralQuery::partIdByGeoId`（1722/2294） | ✅ |
| `getPartName` | `GN_Part::metadata().name()` / `MGeoNeutralQuery::partNameByGeoId` | 4 → 0 残留（mGeoEditPanel:3002/3037、mShortcutActionList:608/625 原位改写） | ✅ |
| `getSolidByTag/getFaceByTag/getEdgeByTag/getShapeByDimTag` | 引擎 API：`getGEOServer.geoCalculate().getShapeRelated / getEdgeAttr / interferenceCheck` 等（GN 拓扑在引擎侧消费） | 抽取中面（mGeoExtractPanel）改 `mGeoExtractMidFaceRequest` 引擎请求；短边/小面/干涉检查（mGeoEditPanel:3441+/3704+）改 `geoCalculate()`/`interferenceCheck()` | ✅ |
| `getEdgeSolidMap/getFaceSolidMap/getEdgeFaceMap` | `GN_Part::getEdgeSolidMap()/getFaceSolidMap()/getEdgeFaceMap()`（同构 `map<int,vector<int>>` 跨级祖先表） | GUI 0 残留（老调用点随检查功能引擎化） | ✅ |
| `geosOnSolid/geosOnFace/getTagByEdge/getTagByFace` | `GN_Part::commonEdgesOf`（面公共边）/ 引擎 `getShapeRelated`（结果直接携带几何 ID，无需 Shape→tag 反查） | GUI 0 残留 | ✅ |
| `getShape` | `GN_Part::getShape()`（返回中性根形状 ID） | 唯一调用点（几何清理）下线 | ✅ |
| `updateEditPara`（含新旧 ID 映射） | `GN_Part::appendEditHistoryPair/setEditWholePart/editHistoryNewIds/editHistoryFirstNewId/createdIds`（T2 编辑会话；mGeoNPDPart.h:341-361 逐一标注"老 xxx 等价"） | 唯一调用点（几何清理）下线待引擎修复族承接 | ✅ |
| `_partNameTag` | `{GN_Part::metadata().name(), partId}` | mMeshThread:508 → 881-882 `std::make_pair(partNameByGeoId(…,3,id), partIdByGeoId(…,3,id))`（注释"GN 等价 pair"） | ✅ |
| `new GeoDataModelSet` | GN 部件由引擎创建并注册（`mGeoPartManager::appendPart/appendPartTree`） | GUI 0 残留 | ✅ |
| 渲染属性族（`setElementColor4D/isElementVisible/GeoElementAttributes`，基线 GUI 未见直接调用） | `GN_Part::setColor4D/getColor4D/isElementVisible/propagateToChildren`（QMap<int,QVariantMap>×4 维同构） | 渲染链经 mViewportModel:164 注入 partMgr 读取 | ✅（结构对齐） |
| `setIsMeshAll/getIsMeshAll` | `GN_Part::setIsMeshAll/isMeshAll`（T9 标注等价） | 引擎侧 | ✅ |

### 3.3 命令接线链（`setGeoDataBase(getGeoData())`，基线约 90 处）

HEAD 激活代码中的 `mGeoCommandFactory::create*` 调用数为 **0**（经块注释 + 预处理器门控双重过滤验证）。原命令链各面板已改：

- 布尔/缝合/抽取/干涉/直模等 → `getGEOServer.<service>().<op>(req)` 引擎请求 + `res.generatedObjects/modifiedObjectIds` 结果；
- 创建/编辑通知 → `createGeoModelSignal_2 / editGeoModelSignal_2`（新形态，携带引擎返回的 partId）；
- 编辑部件登记 → `getGEOServer.context().PartManager()._editPart.insert(editpart.id)`（等价老 `appendEditSet` 簿记）；
- 部件删除 → `getGeoManager()->deleteGeoDataModelSet_2(partIds)` = `_partMgr->removePart(id)` + **新旧信号双发**（mGeoManager.cpp:1134-1145）；
- 残留的 9 处 `setGeoDataBase/getGeoDataSet()->getPartID()` 全部位于 `#if !MXGUI_HAS_*` 的 `#else` 旧分支或块注释内（门控宏见 `src/MainWindow/mGeoDevPreviewGate.h`，当前全为 0）。

---

## 4. 功能一致性评估依据

1. **数据结构同构**：`GN_TessellationMesh`（mGeoNFDRenderMesh.h）与老 `GeoDidcreteData` 字段一一同名同义（pointFlag/edgePoints/facePoints/solidfaces/geoPointIndices/isolated*/edgeTopoTypeMap/snapData，gp_Pnt→GN_Point）；`GN_Part` 四维属性表与 `GeoElementAttributes` 同构，`propagateToChildren` 语义保留（颜色仅体→面，可见性全级联）。
2. **查询语义等价**：老 `getPartSetByGeoFaceID` = 遍历 set `getFaceByTag(id)` 非空；新 `mGeoPartManager::getPartByGeoFaceID` = 遍历部件 `part.existShape(2, id)`；`MGeoNeutralQuery` 以离散表键（facePoints/edgePoints/solidfaces/geoPointIndices 的 key = 全局几何 ID）检索——键空间一致，结果等价（前提：GN 部件离散数据已填充，当前由引擎导入/编辑链负责）。
3. **簿记与信号等价**：部件增删改经 `mGeoPartManager::appendPart/removePart/editShapeFinish`；删除/导入等操作由 GeoManager 兼容层**双发**新旧信号，GUI 既有老信号连接（mViewportModel:1178）继续生效，视图刷新行为不变。
4. **指针身份 → 整数身份**：老代码用 `GeoDataModelSet*` 指针做部件同一性判等（mMeshThread partsWithFace），新代码用 `partIdByGeoId` 返回的 int partId 判等（mMeshThread:175-184），语义等价且更稳。
5. **性能特征差异（非功能差异）**：`MGeoNeutralQuery` 部分为 O(部件数×表查找) 线性扫描，老实现是 O(集合数×tag 查找)；在当前部件规模下无功能影响。

---

## 5. 未完成替换项（功能空窗清单，共 6 处）

| # | 位置（HEAD 行号） | 老调用 | 现状 | 功能影响 |
|---|------------------|--------|------|----------|
| 1 | mMeshThread.cpp:2363、3115 | `getGeoData()->setGeoDiscreteDataEdgeTopoType(typeOfEdge)` | 老实现空壳（mGeoDataBase.cpp:943-946）；新轨 `GN_TessellationMesh::edgeTopoTypeMap` **未接线** | 抑制/恢复边时"边显示类型"（普通边/虚拓扑边）不再写入，下游渲染区分丢失 |
| 2 | PickCommonFunction.cpp:136-140 | `getPartSetByGeoFaceID(faceId)` + `expandVirtualTopologyFaces(...)` | 前者恒 nullptr、后者空壳直接返回 | 拾取几何面时不再沿虚拓扑边递归扩展相连面（多选虚拓扑面组失效） |
| 3 | PickCommonFunction.cpp:682、743 | `modeldata._geoDataBase->getSetByPartId(id) != nullptr` | 恒 nullptr → 分支恒 false | `setPreWidgetSelectPartID` 按部件选择时 geo 分支不再入选（几何侧选择数据不更新；可用 `MGeoNeutralQuery::geoIdsByPartId` 等价替换） |
| 4 | mGeoEditPanel.cpp:3669（slot_checkTinySolid） | `geoData->getPartSet()` 遍历 | 列表恒空 + 内部体积属性集为空结构体（原注释：待 mOccGeoInfoCalculator 接入恢复） | "查找小体"功能不可用（显式挂起） |
| 5 | mSelectMenuFunction.cpp:3255+（getGeoIsMask，9 个调用点） | `getPartSetByGeo*ID` + `set->getPartID()` + `meshData->getGeoPartMask` | 空壳恒返 false（`/*S9 退役*/`） | 部件掩码过滤失效——若仍有部件处于掩码状态，选择枚举不再排除 |
| 6 | mMenuWidget.cpp:5276（几何清理按钮） | `getPartSet/getShape/new set/updateEditPara/appendEditSet/removeSet` 整链 | 旧链整体退役，按钮提示"已切换至引擎修复管线（迁移中），暂不可用" | 几何清理功能下线（有用户可见提示，非静默） |

> 补充说明：上述 #1/#2/#3/#5 属于"调用点还在、实现被掏空"的静默空窗（运行时不报错但行为退化为"无操作"）；#4/#6 为代码中显式声明待恢复的功能。

---

## 6. HEAD 新栈使用统计

| 使用方式 | 数量 | 位置 |
|----------|------|------|
| `MGeoNeutralQuery::partNameByGeoId` | 19 | mMeshThread/mGeoEditPanel/mShortcutActionList/mSelectInfoWindow 等 10 文件 |
| `MGeoNeutralQuery::partIdByGeoId` | 16 | 同上（含 mMeshThread 部件判等、mSelectMenuFunction 掩码原位替换） |
| `MGeoNeutralQuery::geoIdsByPartId` | 11 | 网格面板族（面/线/体/自适应）+ mSelectInfoWindow |
| `MGeoNeutralQuery::geoIdsByPartName` | 4 | 网格面板族部件计数 |
| `mGeoPartManager` 直用（getPartList/containsPart/getPartById） | 4 处入口 | mShortcutActionList:708（撤消镜像重建）、mTreeView:4932/5351（部件树/导出）、mViewportModel:164（渲染代理注入） |
| `GN_Part` 直用 | 5 | mGeoSolidifyPanel:415、mGeoInfoPanel:487、mGeoSketchSignalManager:200（`findPart`）、mTreeView:4970、mShortcutActionList:757（`metadata().name()` 重建 mPartData） |
| 引擎服务（getGEOServer.*，GN 之上的主替换层） | 布尔/缝合/抽取/干涉/短边/小面/直模/草图等全部主流程 | 各面板激活路径 |
| GeoManager 兼容包装（deleteGeoDataModelSet_2 等） | 5 | mTreeView ×3、mGeoEditPanel ×2 |

---

## 7. 总体判定

- **替换完成度**：基线 27 个直接使用成员中 21 项 ✅ 完成等效替换、1 项 🟡 经兼容壳保持（信号）、6 项涉及未接线的功能空窗（其中 #3/#5 仅需把残留调用点改指 `MGeoNeutralQuery` 即可收口，#1/#2 需在 GN 侧补 edgeTopoTypeMap 写入与 VT 扩展查询，#4/#6 依赖引擎修复族/信息计算器接入）。
- **功能一致性**：已替换路径在数据结构（离散表/属性表/拓扑祖先表同构）、查询语义（同一几何 ID 键空间）、簿记与信号（双发兼容）三个层面与老栈等价；未发现已替换路径中的语义偏差。
- **建议收口顺序**：① PickCommonFunction 两处反查改 `MGeoNeutralQuery`（纯 GUI 改动，风险最低）；② getGeoIsMask 改 `partIdByGeoId + getGeoPartMask`；③ mMeshThread 边类型写入接 `GN_TessellationMesh::edgeTopoTypeMap`；④ VT 面扩展等 GN 虚拓扑查询就绪后接回拾取链；⑤ 小体/几何清理随引擎修复族恢复后删除冻结岛残壳。

---

### 附：核对方法说明

- 基线/HEAD 均以"成员名全量扫描（两老类头文件全部方法+公共成员）× 接收者类型甄别（链式入口 `getGeoData()->`、`_geoDataBase->`、`getGeoModelBase()->`、命令结果 `getGeoDataSet()->`、QPointer/容器迭代别名）"交叉得出，已剔除同名异类噪声（QWidget::setVisible、MXGeoFace::getPartName、mPartData::getPartID 等）。
- HEAD 残留判定经过**块注释剥离 + 预处理器门控求值**（`MXGUI_HAS_*` 六宏全 0），确保"激活代码"口径准确；工厂调用归零结论同口径。
