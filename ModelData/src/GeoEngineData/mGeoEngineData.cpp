#include "mGeoEngineData.h"

//【OCC摘除·S10/P0-1】组合工厂符号声明（定义于 RebuildGeometry/OCCAdapter，
// 声明见 mGeoEngineAttach.h）。此处本地声明而非 include 该头——它经
// DefaultAdapter 链携带 OCC 头，本仓（ModelData）必须零 OCC 编译单元。
namespace GeoServer {
    mGeoEngineHandle mGeoEngineHandleCreateApplication(void* meshData);
    const char* mGeoEngineBackendName();
}

namespace GeoEngineConfig {

class GeoEngineData::Impl {
public:
    // 应用引擎句柄（工厂在所选后端目标内创建完整运行时并包装）。
    GeoServer::mGeoEngineHandle handle;
};

GeoEngineData::GeoEngineData(mMeshData* meshdata)
    : m_impl(new Impl{ GeoServer::mGeoEngineHandleCreateApplication(meshdata) })
{
}

GeoEngineData::~GeoEngineData()
{
}

const char* GeoEngineData::backendName() {
    return GeoServer::mGeoEngineBackendName();
}

GeoServer::mGeoEngineHandle& GeoEngineData::engine() {
    return m_impl->handle;
}

} // namespace GeoEngineConfig
