#pragma once
// 双 ModelData 树同源共享头：MxAI 工作区同时安装 RebuildModelData(dev_Mech) 与
// RebuildModelData_Dyna(dev_Dyna)，本文件两树逐字节同源；#pragma once 按路径去重
// 无法跨树去重（双份均被包含即 C2011 重定义），补同名 include guard 先含者胜。
#ifndef MGEOENGINEDATA_H
#define MGEOENGINEDATA_H

// GeoEngineData 是几何引擎联调用的轻量运行时对象。
//【OCC摘除·S10/P0-1】pimpl 化收口：组合根（server 模板实例化）经
// mGeoEngineHandleCreateApplication 工厂下沉到 RebuildGeometry/OCCAdapter，
// 本头不再暴露 DefaultAdapter/GeoServerType/server()——由此切断
// mModelData.h → mGeoEngineData.h → 后端头 → OCC 头的头网传递
// （IOFile/PostIOFile/MxMech/PreData 等消费方被迫链接 OCC 的根因）。
// 调用方一律经 engine()（零 OCC 句柄）+ 同语法服务视图（mGeoEngineServices.h）访问。

#include "MXGeometryEngineData_global.h"
#include "mGeoEngineHandle.h"

#include <memory>

class mMeshData;

namespace GeoEngineConfig {

// 几何引擎运行时数据对象（唯一职责：创建并持有应用引擎句柄，保证生命周期）。
class GEOENGINEDATA_EXPORT GeoEngineData {
public:
    // 构造应用引擎运行时（组合在所选后端目标内完成；meshData 绑定部件管理器）。
    GeoEngineData(mMeshData* meshdata);

    // 析构运行时对象（Impl 完整类型仅存在于本库 cpp）。
    ~GeoEngineData();

    // 禁止拷贝；句柄视图内部引用运行时对象地址，拷贝后会产生悬空引用风险。
    GeoEngineData(const GeoEngineData&) = delete;

    // 禁止拷贝赋值。
    GeoEngineData& operator=(const GeoEngineData&) = delete;

    // 禁止移动；移动后句柄视图内部引用不会自动重新绑定。
    GeoEngineData(GeoEngineData&&) = delete;

    // 禁止移动赋值。
    GeoEngineData& operator=(GeoEngineData&&) = delete;

    // 获取当前 CMake 选择的后端名称。
    static const char* backendName();

    // 唯一对外出口：零 OCC 引擎句柄（服务访问语法与旧 server() 一致，
    // 见 mGeoEngineServices.h 的 geoCalculate()/features()/geoEdit() 等视图）。
    GeoServer::mGeoEngineHandle& engine();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace GeoEngineConfig
#endif // MGEOENGINEDATA_H
