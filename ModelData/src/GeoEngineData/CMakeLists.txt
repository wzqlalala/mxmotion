set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(GeoEngineData
	${_header}
	${_source}
)

#-----------------------------------------------------------------------------
#【OCC摘除·S10/P0-1】本目标 pimpl 化收口：组合根经 mGeoEngineHandleCreateApplication
# 工厂下沉至 RebuildGeometry/OCCAdapter（本仓零 OCC 编译单元）。原 OpenCASCADE
# include 目录与链接（模板实例化闭包所需）随之整体摘除——mModelData.h 的头网
# 传递（→后端头→NCollection）同步根除，IOFile/PostIOFile/MxMech/PreData 等
# 消费方不再被迫链接 OCC。
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# 临时添加 MxFunctions 头文件搜索路径。
# MeshData 在 RebuildBasicData 中以 PRIVATE 链接 MxFunctions，导致其安装后的导入目标
# 没有将 MxFunctions 的 include 路径传递给下游。在重新构建 RebuildBasicData 之前，
# 这里临时补上，保证 GeoEngineData 能解析 mMeshData.h 间接引入的 mModelStruct.h。
# 待 RebuildBasicData/MeshData 以 PUBLIC 链接 MxFunctions 并重新安装后，可移除本段。
#-----------------------------------------------------------------------------
target_include_directories(GeoEngineData PUBLIC
	${MxFunctions_INC_DIR}
)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(GeoEngineData PRIVATE "GEOENGINEDATA_LIB")

#【OCC摘除·S10/P0-1】OpenCASCADE 链接摘除（组合根已下沉 OCCAdapter，见上方注）
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets GeoNeutralData EventDispatcher GeoServer ${GeoEngineData_BACKEND_ADAPTER} MxFunctions MeshData
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(GeoEngineData PUBLIC
	${_runtimes_libraries}
	${_depend_library}
)

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
#set(GeoEngineData_Runtimes_Libraries ${_runtimes_libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
	FILES ${_header}
	DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)