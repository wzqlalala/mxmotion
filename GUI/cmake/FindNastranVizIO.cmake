#-----------------------------------------------------------------------------
# 引入 NastranVizIO 模块（静态库）
# MODULE 模式查找：仅定位安装目录并创建 IMPORTED 目标，不要求库文件在
# configure 期已存在（compile 阶段逐模块"configure→build→install"，
# 下游 configure 时上游产物必然已生成，目录骨架由 CI/CD 工具预创建）
#-----------------------------------------------------------------------------

# 防止重复引入
if(NASTRANVIZIO_ALREADY_INCLUDED)
	return()
endif()
set(NASTRANVIZIO_ALREADY_INCLUDED 1)

# 设置路径（消费兄弟模块 install 树）
get_filename_component(NastranVizIO_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/NastranVizIO/install" ABSOLUTE)

set(NastranVizIO_INC_DIR "${NastranVizIO_Dir}/include")
set(NastranVizIO_LIB_DIR "${NastranVizIO_Dir}/lib")

# 消费方链接名带命名空间（对应 install(EXPORT NAMESPACE NastranVizIO::)）
add_library(NastranVizIO::NastranVizIO STATIC IMPORTED)
set_property(TARGET NastranVizIO::NastranVizIO PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${NastranVizIO_INC_DIR})
set_property(TARGET NastranVizIO::NastranVizIO APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)

if(WIN32)
	set_target_properties(NastranVizIO::NastranVizIO PROPERTIES
		IMPORTED_LOCATION_RELEASE "${NastranVizIO_LIB_DIR}/nastranvizio.lib"
		IMPORTED_LOCATION_DEBUG "${NastranVizIO_LIB_DIR}/nastranviziod.lib"
	)
else()
	set_target_properties(NastranVizIO::NastranVizIO PROPERTIES
		IMPORTED_LOCATION_RELEASE "${NastranVizIO_LIB_DIR}/libnastranvizio.a"
		IMPORTED_LOCATION_DEBUG "${NastranVizIO_LIB_DIR}/libnastranviziod.a"
	)
endif()

# 注意：静态库必须与消费方编译配置严格匹配（/MD vs /MDd、_ITERATOR_DEBUG_LEVEL），
# 不做 Debug 缺失回退 Release 的兜底——那会触发 LNK2038 运行时库不匹配。
# 若链接报"无法打开 nastranviziod.lib"，请先将 NastranVizIO 以 Debug 配置编译安装。
