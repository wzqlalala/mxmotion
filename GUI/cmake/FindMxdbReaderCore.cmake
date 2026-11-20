#-----------------------------------------------------------------------------
# 引入 MxdbReaderCore 基础模块（静态库）
# MODULE 模式查找：仅定位安装目录并创建 IMPORTED 目标，不要求库文件在
# configure 期已存在（generate 阶段可能先于依赖模块的 compile/install 执行，
# 目录骨架由 CI/CD 工具预创建，真实产物由 compile 阶段的 install 填充）
#-----------------------------------------------------------------------------

# 防止重复引入
if(MXDBREADERCORE_ALREADY_INCLUDED)
	return()
endif()
set(MXDBREADERCORE_ALREADY_INCLUDED 1)

# 设置路径（与其它 Find* 模块一致，使用相对路径定位模块安装目录）
get_filename_component(MxdbReaderCore_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/MxdbReaderCore/install" ABSOLUTE)

set(MxdbReaderCore_INC_DIR "${MxdbReaderCore_Dir}/include")
set(MxdbReaderCore_LIB_DIR "${MxdbReaderCore_Dir}/lib")

add_library(MxdbReaderCore STATIC IMPORTED)
set_property(TARGET MxdbReaderCore PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MxdbReaderCore_INC_DIR})
# 静态库消费者需要该公开编译定义（与库自身 add_library 时的 PUBLIC 定义保持一致）
set_property(TARGET MxdbReaderCore PROPERTY INTERFACE_COMPILE_DEFINITIONS "MXDB_READER_CORE_STATIC")
set_property(TARGET MxdbReaderCore APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)

if(WIN32)
	set_target_properties(MxdbReaderCore PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxdbReaderCore_LIB_DIR}/MxdbReaderCore.lib"
		IMPORTED_LOCATION_DEBUG "${MxdbReaderCore_LIB_DIR}/MxdbReaderCored.lib"
	)
else()
	set_target_properties(MxdbReaderCore PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxdbReaderCore_LIB_DIR}/libMxdbReaderCore.a"
		IMPORTED_LOCATION_DEBUG "${MxdbReaderCore_LIB_DIR}/libMxdbReaderCored.a"
		LINK_FLAGS "-Wl,-rpath,${MxdbReaderCore_LIB_DIR}/"
	)
endif()

# 注意：静态库必须与消费方编译配置严格匹配（/MD vs /MDd、_ITERATOR_DEBUG_LEVEL），
# 不允许 Debug 缺失时回退链接 Release 库——那会触发 LNK2038 运行时库不匹配。
# 若 Debug 链接报"无法打开 MxdbReaderCored.lib"，请先将 MxdbReaderCore 以 Debug
# 配置编译安装（跑一次 --build-type Debug 的 compile 阶段）。
