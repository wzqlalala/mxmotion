#-----------------------------------------------------------------------------
# 引入 PchReader 基础模块（静态库）
# MODULE 模式查找：仅定位安装目录并创建 IMPORTED 目标，不要求库文件在
# configure 期已存在（generate 阶段可能先于依赖模块的 compile/install 执行，
# 目录骨架由 CI/CD 工具预创建，真实产物由 compile 阶段的 install 填充）
#-----------------------------------------------------------------------------

# 防止重复引入
if(PCHREADER_ALREADY_INCLUDED)
	return()
endif()
set(PCHREADER_ALREADY_INCLUDED 1)

# 设置路径（与其它 Find* 模块一致，使用相对路径定位模块安装目录）
get_filename_component(PchReader_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/PchReader/install" ABSOLUTE)

set(PchReader_INC_DIR "${PchReader_Dir}/include")
set(PchReader_LIB_DIR "${PchReader_Dir}/lib")

# 消费方链接名带命名空间（对应 install(EXPORT NAMESPACE PchReader::)）
add_library(PchReader::PchReader STATIC IMPORTED)
set_property(TARGET PchReader::PchReader PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PchReader_INC_DIR})
set_property(TARGET PchReader::PchReader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)

if(WIN32)
	set_target_properties(PchReader::PchReader PROPERTIES
		IMPORTED_LOCATION_RELEASE "${PchReader_LIB_DIR}/PchReader.lib"
		IMPORTED_LOCATION_DEBUG "${PchReader_LIB_DIR}/PchReaderd.lib"
	)
else()
	set_target_properties(PchReader::PchReader PROPERTIES
		IMPORTED_LOCATION_RELEASE "${PchReader_LIB_DIR}/libPchReader.a"
		IMPORTED_LOCATION_DEBUG "${PchReader_LIB_DIR}/libPchReaderd.a"
	)
endif()

# 注意：静态库必须与消费方编译配置严格匹配（/MD vs /MDd、_ITERATOR_DEBUG_LEVEL），
# 不做 Debug 缺失回退 Release 的兜底——那会触发 LNK2038 运行时库不匹配。
# 若链接报"无法打开 PchReaderd.lib"，请先将 PchReader 以 Debug 配置编译安装。
