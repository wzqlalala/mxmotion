# 后处理面数据跨帧缓存复用实现计划

> **面向 AI 代理的工作者：** 必需子技能：使用 `superpowers:subagent-driven-development`（推荐）或 `superpowers:executing-plans` 逐任务实现此计划。步骤使用复选框（`- [ ]`）语法来跟踪进度。

**目标：** 将 `MeshFaceCache` 从每帧独立的 `mPostModelData` 迁移到 `mPostDataBase`，利用后处理文件中的 `sameFrame` 字段实现同模型帧之间的面拓扑缓存复用，减少动画生成阶段的重复重建耗时。

**架构：** 在 `PostFrameInfo` 中记录 `sameFrame`；在 `mPostDataBase` 上按“模型来源帧号”维护一个 `QHash<int, MeshFaceCachePtr>`；重建函数改为先查缓存，命中则仅按当前帧失效集合重新分类写入，未命中则完整重建并写入缓存。

**技术栈：** C++、Qt、CMake、MSBuild。

---

## 文件清单

| 文件 | 职责 |
|------|------|
| `Basic/RebuildBasicData/src/PostData/mPostResultDataStruct.h` | 在 `PostFrameInfo` 中新增 `sameFrame` 字段 |
| `Basic/RebuildBasicData/src/PostData/mPostDataBase.h` | 声明跨帧缓存接口与私有 `_meshFaceCacheMap` |
| `Basic/RebuildBasicData/src/PostData/mPostDataBase.cpp` | 实现上述接口 |
| `Software/Common/RebuildGUI/src/PostIOFile/MxdbReader/MxdbDataBuilder.cpp` | 构建结果帧时把 `frame.sameFrame` 写入 `frameInfo.sameFrame` |
| `Software/Common/RebuildGUI/src/MainWindow/mPost3DResult.cpp` | 改造 `rebuildFacesForModelDataWithCache` 从 `postDB` 取缓存；让 `processFailedElements` 和 `hideFailedElementsForFrame` 走缓存路径 |
| `Basic/RebuildBasicData/src/PostData/test_mesh_signature.cpp` | 补充 `mPostDataBase` 缓存查询单元测试 |

---

## 任务 1：在 `PostFrameInfo` 中记录 `sameFrame`

**文件：**
- 修改：`Basic/RebuildBasicData/src/PostData/mPostResultDataStruct.h`

- [ ] **步骤 1：添加字段**

在 `PostFrameInfo` 的 `_frameNum` 字段附近添加：

```cpp
// 当前模型与哪一帧一致（-1 新模型，0 初始帧，n 第 n 帧）
int sameFrame = -1;
```

- [ ] **步骤 2：Commit**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData
git add src/PostData/mPostResultDataStruct.h
git commit -m "feat(PostData): add sameFrame to PostFrameInfo"
```

---

## 任务 2：在 `mPostDataBase` 上实现跨帧缓存接口

**文件：**
- 修改：`Basic/RebuildBasicData/src/PostData/mPostDataBase.h`
- 修改：`Basic/RebuildBasicData/src/PostData/mPostDataBase.cpp`

- [ ] **步骤 1：在头文件中声明接口和成员**

在 `mPostDataBase.h` 的 `public` 区域添加：

```cpp
// 获取指定帧应复用的面数据缓存（key 由 sameFrame 决定）
std::shared_ptr<mPostModelData::MeshFaceCache> getMeshFaceCacheForFrame(int frame) const;

// 为指定帧设置面数据缓存（key 由 sameFrame 决定）
void setMeshFaceCacheForFrame(int frame, std::shared_ptr<mPostModelData::MeshFaceCache> cache);

// 清除所有面数据缓存
void clearMeshFaceCache();
```

在 `private` 区域添加：

```cpp
// key: 模型来源帧号（sameFrame == -1 时，用当前帧号作为该模型标识）
QHash<int, std::shared_ptr<mPostModelData::MeshFaceCache>> _meshFaceCacheMap;
```

- [ ] **步骤 2：在 cpp 中实现接口**

在 `mPostDataBase.cpp` 的 `PostData` 命名空间中新增实现：

```cpp
namespace
{
    // 根据 sameFrame 规则计算缓存 key
    int computeMeshFaceCacheKey(const mPostDataBase* db, int frame)
    {
        if (db == nullptr)
            return frame;

        QPointer<mPostModelData> modelData = db->getModelData(frame);
        if (modelData == nullptr)
            return frame;

        const int sameFrame = modelData->_frameInfo.sameFrame;
        return sameFrame == -1 ? frame : sameFrame;
    }
}

std::shared_ptr<mPostModelData::MeshFaceCache> mPostDataBase::getMeshFaceCacheForFrame(int frame) const
{
    const int key = computeMeshFaceCacheKey(this, frame);
    return _meshFaceCacheMap.value(key);
}

void mPostDataBase::setMeshFaceCacheForFrame(int frame, std::shared_ptr<mPostModelData::MeshFaceCache> cache)
{
    const int key = computeMeshFaceCacheKey(this, frame);
    _meshFaceCacheMap[key] = cache;
}

void mPostDataBase::clearMeshFaceCache()
{
    _meshFaceCacheMap.clear();
}
```

- [ ] **步骤 3：编译验证**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData/bin
cmake --build . --target PostData --config Release --parallel 4
```

预期：无编译错误。

- [ ] **步骤 4：Commit**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData
git add src/PostData/mPostDataBase.h src/PostData/mPostDataBase.cpp
git commit -m "feat(PostData): add cross-frame mesh face cache APIs on mPostDataBase"
```

---

## 任务 3：在构建结果帧时写入 `sameFrame`

**文件：**
- 修改：`Software/Common/RebuildGUI/src/PostIOFile/MxdbReader/MxdbDataBuilder.cpp`

- [ ] **步骤 1：写入 sameFrame**

在 `buildResultFrame` 中，找到 `PostFrameInfo frameInfo;` 初始化处，添加：

```cpp
frameInfo.sameFrame = frame.sameFrame;
```

具体位置：

```cpp
PostData::PostFrameInfo frameInfo;
frameInfo._frameNum = frameIndex;
frameInfo.sameFrame = frame.sameFrame;  // 新增
frameInfo._frameCase = QString::fromStdString(frame.caseName);
frameInfo._frameName = QString::number(frameIndex);
```

- [ ] **步骤 2：编译验证**

```bash
cd D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin
cmake --build . --target PostIOFile --config Release --parallel 4
```

预期：无编译错误。

- [ ] **步骤 3：Commit**

```bash
cd D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI
git add src/PostIOFile/MxdbReader/MxdbDataBuilder.cpp
git commit -m "feat(PostIOFile): propagate sameFrame into PostFrameInfo"
```

---

## 任务 4：改造 `rebuildFacesForModelDataWithCache` 使用 `mPostDataBase` 缓存

**文件：**
- 修改：`Software/Common/RebuildGUI/src/MainWindow/mPost3DResult.cpp`

- [ ] **步骤 1：修改函数签名**

将匿名命名空间中的函数从：

```cpp
bool rebuildFacesForModelDataWithCache(
    QPointer<PostData::mPostModelData> modelData,
    const QString& viewportName,
    const std::set<int>& lastLoseEles)
```

改为：

```cpp
bool rebuildFacesForModelDataWithCache(
    QPointer<PostData::mPostDataBase> postDB,
    QPointer<PostData::mPostModelData> modelData,
    const QString& viewportName,
    int frame)
```

- [ ] **步骤 2：修改函数体**

函数开头改为：

```cpp
bool rebuildFacesForModelDataWithCache(
    QPointer<PostData::mPostDataBase> postDB,
    QPointer<PostData::mPostModelData> modelData,
    const QString& viewportName,
    int frame)
{
    PerfTimer _t("rebuildFacesForModelDataWithCache");
    QElapsedTimer totalTimer;
    totalTimer.start();

    if (postDB == nullptr || modelData == nullptr)
        return false;

    using namespace PostData;

    std::shared_ptr<mPostModelData::MeshFaceCache> cache = postDB->getMeshFaceCacheForFrame(frame);

    // 缓存存在时，直接复用拓扑并按当前帧失效集合重新分类
    if (cache != nullptr && !cache->partTopologies.empty())
    {
        QElapsedTimer incTimer;
        incTimer.start();

        modelData->setAllEleShow(viewportName);

        std::set<int> currentLoseEles = modelData->getLoseElems();
        for (auto eleId : currentLoseEles)
        {
            int partid = modelData->getPartIDByEleID(eleId);
            if (partid >= 0)
                modelData->setEleVisual(viewportName, partid, eleId, false);
        }

        int partIdx = 0;
        for (auto part : modelData->_partdata)
        {
            if (part == nullptr || partIdx >= static_cast<int>(cache->partTopologies.size()))
            {
                ++partIdx;
                continue;
            }
            mPostParseMesh::classifyAndWriteFaces(part, cache->partTopologies[partIdx]);
            ++partIdx;
        }

        qDebug() << "[Perf] rebuildFacesForModelDataWithCache cache hit, total=" << totalTimer.elapsed() << "ms (incremental=" << incTimer.elapsed() << "ms)";
        return true;
    }

    QElapsedTimer rebuildTimer;
    rebuildTimer.start();

    // 缓存未命中：完整重建并缓存拓扑
    modelData->setAllEleShow(viewportName);

    std::set<int> currentLoseEles = modelData->getLoseElems();
    for (auto eleId : currentLoseEles)
    {
        int partid = modelData->getPartIDByEleID(eleId);
        if (partid >= 0)
            modelData->setEleVisual(viewportName, partid, eleId, false);
    }

    mPostParseMesh parseMesh(nullptr);
    for (auto part : modelData->_partdata)
    {
        if (part == nullptr)
            continue;
        parseMesh.buildMeshFace(part);
    }

    std::shared_ptr<mPostModelData::MeshFaceCache> newCache = std::make_shared<mPostModelData::MeshFaceCache>();
    newCache->signature = mPostMeshSignature::computeFullSignature(modelData);
    newCache->failureSignature = mPostMeshSignature::computeFailureSignature(modelData);
    for (auto part : modelData->_partdata)
    {
        if (part == nullptr)
        {
            newCache->partTopologies.emplace_back();
            continue;
        }
        UniqueFaceTopology topology;
        mPostParseMesh::buildUniqueFaceTopology(part, topology);
        newCache->partTopologies.push_back(std::move(topology));
    }
    postDB->setMeshFaceCacheForFrame(frame, newCache);

    qDebug() << "[Perf] rebuildFacesForModelDataWithCache full rebuild, total=" << totalTimer.elapsed() << "ms (rebuild=" << rebuildTimer.elapsed() << "ms)";
    return false;
}
```

- [ ] **步骤 3：修改 `processFailedElements` 调用**

把 `processFailedElements` 中两处：

```cpp
rebuildFacesForModelData(modelData);
```

替换为：

```cpp
rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
```

- [ ] **步骤 4：修改 `hideFailedElementsForFrame` 调用**

把 `hideFailedElementsForFrame` 中两处：

```cpp
rebuildFacesForModelData(modelData);
```

替换为：

```cpp
rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
```

- [ ] **步骤 5：修改 `hideFailedElementsForFrameWithCache` 调用**

把函数体中的：

```cpp
rebuildFacesForModelDataWithCache(modelData, viewportName, lastLoseEles);
```

替换为：

```cpp
rebuildFacesForModelDataWithCache(modeldb, modelData, viewportName, fi);
```

- [ ] **步骤 6：编译验证**

```bash
cd D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin
cmake --build . --target MechMainWindow --config Release --parallel 4
```

预期：无编译错误。

- [ ] **步骤 7：Commit**

```bash
cd D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI
git add src/MainWindow/mPost3DResult.cpp
git commit -m "feat(MainWindow): use mPostDataBase cross-frame cache in rebuildFacesForModelDataWithCache"
```

---

## 任务 5：补充单元测试

**文件：**
- 修改：`Basic/RebuildBasicData/src/PostData/test_mesh_signature.cpp`

- [ ] **步骤 1：添加 `mPostDataBase` 缓存测试**

在测试文件末尾新增：

```cpp
static void test_mPostDataBase_cross_frame_cache()
{
    PostData::mPostDataBase db("test_db");

    auto cache0 = db.getMeshFaceCacheForFrame(1);
    assert(cache0 == nullptr);

    auto cache = std::make_shared<PostData::mPostModelData::MeshFaceCache>();
    db.setMeshFaceCacheForFrame(1, cache);

    auto cache1 = db.getMeshFaceCacheForFrame(1);
    assert(cache1 == cache);

    db.clearMeshFaceCache();
    assert(db.getMeshFaceCacheForFrame(1) == nullptr);

    qDebug() << "[PASS] mPostDataBase cross-frame cache basic APIs";
}
```

并在 `main` 中调用 `test_mPostDataBase_cross_frame_cache()`。

- [ ] **步骤 2：编译并运行测试**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData/bin
cmake --build . --target PostDataMeshSignatureTest --config Release --parallel 4
$env:PATH = "D:/MxSimProject/MxSim_DevBase/Basic/RebuildMesh/bin/Release;D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData/bin/Release;" + $env:PATH
../bin/Release/PostDataMeshSignatureTest.exe
```

预期：所有测试通过。

- [ ] **步骤 3：Commit**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData
git add src/PostData/test_mesh_signature.cpp
git commit -m "test(PostData): add cross-frame cache API tests for mPostDataBase"
```

---

## 任务 6：端到端手动验证

**文件：** 无需修改，仅验证。

- [ ] **步骤 1：编译完整 Release 版本**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData/bin
cmake --build . --target PostData --config Release --parallel 4
cmake --build . --target PostDataMeshSignatureTest --config Release --parallel 4

cd D:/MxSimProject/MxSim_DevBase/Software/Common/RebuildGUI/bin
cmake --build . --target MechMainWindow --config Release --parallel 4
```

- [ ] **步骤 2：运行单元测试**

```bash
cd D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData
$env:PATH = "D:/MxSimProject/MxSim_DevBase/Basic/RebuildMesh/bin/Release;D:/MxSimProject/MxSim_DevBase/Basic/RebuildBasicData/bin/Release;" + $env:PATH
./bin/Release/PostDataMeshSignatureTest.exe
```

预期：全部通过。

- [ ] **步骤 3：用指定模型手动测试**

模型路径：`F:\Test_Model_Data\数据重构测试文件\PostV3.0测试文件\有bug\HTYY-xiaolv-04`

操作：
1. 导入后处理模型。
2. 一键生成动画（建议 2~4 帧）。
3. 观察 Qt 调试输出：
   - 第 1 帧应为 `full rebuild`
   - 第 2 帧及以后应为 `cache hit`
   - `applyHideFailedElementForFrame` 耗时从约 7~11s 降至数百毫秒
4. 验证视图自动-fit 是否正常。

---

## 自检

- **规格覆盖度：** 规格中的 `sameFrame` 记录、`mPostDataBase` 缓存接口、重建函数改造、单元测试、端到端验证均已对应任务。
- **占位符扫描：** 无 TODO/待定/模糊描述，每个步骤包含具体代码或命令。
- **类型一致性：** `MeshFaceCache` 仍使用 `mPostModelData::MeshFaceCache`；`getMeshFaceCacheForFrame` 返回 `std::shared_ptr<...>`；`rebuildFacesForModelDataWithCache` 签名与调用处一致。
