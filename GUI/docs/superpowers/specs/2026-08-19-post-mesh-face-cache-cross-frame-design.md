# 后处理面数据跨帧缓存复用设计

## 背景

当前后处理动画生成阶段，每帧都会调用 `rebuildFacesForModelDataWithCache` 重建面数据。虽然引入了 `MeshFaceCache`，但缓存挂在每帧独立的 `mPostModelData` 上，导致动画每一帧都缓存未命中（full rebuild），反而比无缓存版本更慢：

- 第 1 帧：`rebuildFacesForModelDataWithCache` 7453 ms
- 第 2 帧：6541 ms
- 第 3 帧：6818 ms
- 第 4 帧：6747 ms

根本原因是：`MeshFaceCache` 没有跨帧共享能力。

## 目标

实现后处理面数据缓存的跨帧共享：

1. 如果当前帧的网格模型与之前某帧相同，则复用该帧已建立的面拓扑缓存。
2. 如果当前帧是全新模型（`sameFrame == -1`），则独立重建并建立新缓存。
3. 云图导入时建立的缓存，动画生成时也能直接复用。

## 关键约束

- 使用后处理文件中的 `sameFrame` 字段判断模型是否一致，不再依赖签名哈希。
  - `-1`：新模型
  - `0`：与初始帧一致
  - `n`：与第 `n` 帧一致
- 缓存命中时只根据当前帧失效集合重新分类写入面，不再重建拓扑。
- 不改动 `MeshFaceCache` 结构本身。

## 方案

### 1. 记录 `sameFrame` 到 `PostFrameInfo`

在 `mPostResultDataStruct.h` 的 `PostFrameInfo` 中新增字段：

```cpp
// 当前模型与哪一帧一致（-1 新模型，0 初始帧，n 第 n 帧）
int sameFrame = -1;
```

在 `MxdbDataBuilder::buildResultFrame` 读取结果帧时，把 `frame.sameFrame` 写入 `frameInfo.sameFrame`：

```cpp
PostData::PostFrameInfo frameInfo;
frameInfo._frameNum = frameIndex;
frameInfo.sameFrame = frame.sameFrame;
// ... 其他字段保持不变
```

### 2. 在 `mPostDataBase` 上管理跨帧缓存

在 `mPostDataBase.h` 中新增：

```cpp
// key: 模型来源帧号（sameFrame == -1 时，用当前帧号作为该模型的缓存 key）
QHash<int, std::shared_ptr<mPostModelData::MeshFaceCache>> _meshFaceCacheMap;

// 获取指定帧应复用的面数据缓存
std::shared_ptr<mPostModelData::MeshFaceCache> getMeshFaceCacheForFrame(int frame) const;

// 为指定帧设置面数据缓存
void setMeshFaceCacheForFrame(int frame, std::shared_ptr<mPostModelData::MeshFaceCache> cache);

// 清除所有面数据缓存
void clearMeshFaceCache();
```

`getMeshFaceCacheForFrame` 实现逻辑：

1. 取 `modelData = getModelData(frame)`，若为空返回 `nullptr`。
2. 取 `frameInfo = modelData->_frameInfo`。
3. 若 `frameInfo.sameFrame == -1`，缓存 key 为 `frame`；否则 key 为 `frameInfo.sameFrame`。
4. 返回 `_meshFaceCacheMap.value(key)`。

`setMeshFaceCacheForFrame` 实现逻辑：

1. 同上述步骤 1~3 得到 cache key。
2. `_meshFaceCacheMap[key] = cache`。

### 3. 改造 `rebuildFacesForModelDataWithCache`

把函数签名改为接收 `postDB` 和当前帧号：

```cpp
bool rebuildFacesForModelDataWithCache(
    QPointer<PostData::mPostDataBase> postDB,
    QPointer<PostData::mPostModelData> modelData,
    const QString& viewportName,
    int frame);
```

逻辑：

1. 调用 `postDB->getMeshFaceCacheForFrame(frame)` 查询缓存。
2. 缓存存在且 `partTopologies` 非空：
   - 将当前视图所有单元设为显示。
   - 隐藏当前帧失效单元。
   - 遍历 `modelData->_partdata`，用缓存中的 `partTopologies` 调用 `mPostParseMesh::classifyAndWriteFaces`。
   - 输出 `[Perf] rebuildFacesForModelDataWithCache cache hit`。
3. 缓存不存在：
   - 将当前视图所有单元设为显示。
   - 隐藏当前帧失效单元。
   - 调用 `mPostParseMesh::buildMeshFace` 完整重建每 part 面数据。
   - 为每个 part 调用 `mPostParseMesh::buildUniqueFaceTopology` 生成拓扑，存入新的 `MeshFaceCache`。
   - 调用 `postDB->setMeshFaceCacheForFrame(frame, newCache)` 写入缓存。
   - 输出 `[Perf] rebuildFacesForModelDataWithCache full rebuild`。

### 4. 统一云图与动画的重建路径

- `processFailedElements` 从直接调用 `rebuildFacesForModelData` 改为调用 `rebuildFacesForModelDataWithCache(postDB, modelData, viewportName, fi)`。
- `hideFailedElementsForFrame` 同样改为调用 `rebuildFacesForModelDataWithCache`。
- `hideFailedElementsForFrameWithCache` 保持接口不变，内部把 `postDB` 和 `frame` 传给 `rebuildFacesForModelDataWithCache`。

这样云图导入时建立的缓存，动画生成时可以直接命中。

### 5. 保留兼容接口

`mPostModelData` 上的 `getMeshFaceCache/setMeshFaceCache/clearMeshFaceCache` 保留但不再被主逻辑使用，避免破坏其他可能调用这些接口的代码。后续可考虑废弃。

## 测试计划

1. 编译 `RebuildBasicData`（`PostData`）和 `RebuildGUI`（`MechMainWindow`）通过。
2. 运行 `PostDataMeshSignatureTest.exe`，确认单元测试通过。
3. 使用模型 `F:\Test_Model_Data\数据重构测试文件\PostV3.0测试文件\有bug\HTYY-xiaolv-04` 进行手动验证：
   - 导入后处理模型，记录 `rebuildFacesForModelDataWithCache` 日志。
   - 一键生成动画 2~4 帧，确认第 2 帧及以后输出 `cache hit`。
   - 对比优化前后的动画生成总时间。

## 预期效果

- 第 1 帧（云图导入或动画第 1 帧）：1 次完整重建，约 4.7~7.5 s。
- 第 2 帧及以后（模型相同）：跳过拓扑重建，仅重新分类写入，预计降至数百毫秒以内。
- 动画生成总时间从约 50 s 显著下降。
