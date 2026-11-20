#pragma once

#include "MxdbReaderCore/MxdbModelData.h"
#include <QPointer>
#include <vector>

namespace PostData {
    class mPostDataBase;
    class mPostModelData;
    class mPostPartData;
}

namespace MIOFile {

using namespace MxdbReader;

// 将解析后的 Mxdb 数据写入现有 PostData 数据库
class MxdbDataBuilder {
public:
    bool buildInitialFrame(const MxdbInitialFrame& frame, QPointer<PostData::mPostDataBase> db);
    bool buildResultFrame(const MxdbResultFrame& frame, QPointer<PostData::mPostDataBase> db, int frameIndex);

    // 将常规变量值填充到数据库中指定帧
    bool buildVariableValues(const MxdbVariable& var, const std::vector<MxdbVariableValue>& values,
                             QPointer<PostData::mPostDataBase> db, int frameIndex);

    // 指定具体 shape/layer/component 填充
    bool buildVariableValues(const MxdbVariable& var, const std::vector<MxdbVariableValue>& values,
                             int componentIndex, int layerIndex, int shapeIndex,
                             QPointer<PostData::mPostDataBase> db, int frameIndex);

private:
    bool buildNodeData(PostData::mPostModelData* modelData, const std::vector<MxdbNode>& nodes);
    bool buildPartData(PostData::mPostModelData* modelData, const MxdbPart& part, int partId);
    bool buildRigidWallData(PostData::mPostDataBase* db, const std::vector<MxdbRigidWall>& walls);
    bool buildLocalCoordinateData(PostData::mPostDataBase* db,
                                  const std::vector<MxdbLocalCoordinateSystem>& coords);

    static bool isSameNode(const std::vector<int>& ids, const std::vector<int>& poss);
    static void d3h20ToD3p15(const std::vector<int>& ids, std::vector<int>& nodeIds);

    // 根据变量名和分量名推断是否需要启用 Mag（用于旧版或未写 isMag 的文件）
    static bool inferMagForVariable(const MxdbVariable& var);
};

} // namespace MIOFile
