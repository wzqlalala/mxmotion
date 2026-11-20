#-----------------------------------------------------------------------------
# 引入 MxNativeReaders 基础模块（静态库：OP2/ANSYS RST·RTH/d3plot/H3D/ODB 原生读取）
# MODULE 模式查找：仅定位安装目录并创建 IMPORTED 目标，不要求库文件在
# configure 期已存在（generate 阶段可能先于依赖模块的 compile/install 执行，
# 目录骨架由 CI/CD 工具预创建，真实产物由 compile 阶段的 install 填充）
#-----------------------------------------------------------------------------

# 防止重复引入
if(MXNATIVEREADERS_ALREADY_INCLUDED)
	return()
endif()
set(MXNATIVEREADERS_ALREADY_INCLUDED 1)

# 设置路径（与其它 Find* 模块一致，使用相对路径定位模块安装目录）
get_filename_component(MxNativeReaders_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/MxNativeReaders/install" ABSOLUTE)

set(MxNativeReaders_INC_DIR "${MxNativeReaders_Dir}/include")
set(MxNativeReaders_LIB_DIR "${MxNativeReaders_Dir}/lib")

# 库自身 PUBLIC 链接 zlib（H3D 解压块），静态库符号须由消费方最终解析：
# 显式请求 CONFIG 模式以使用 conan 生成的 ZLIBConfig.cmake（版本与库的
# conanfile zlib/1.3.2 及全局 conan_packages.toml 保持一致）
find_package(ZLIB REQUIRED CONFIG)

#-----------------------------------------------------------------------------
# zlib 一致性校验（防跨 conan 图静默漂移）
# MxNativeReaders 经 install 树消费、不在本工程 conan 图内，其 zlib 传递依赖
# 只能在消费侧人工镜像（conanfile.py 的 requires + zlib/*:shared）。库的 install
# 树随附 zlib_manifest.txt（share/mx_native_readers/，记录其构建时实际链接的
# zlib 版本/形态），存在则以之为准；无清单的旧 install 树回退下方硬编码期望值。
# 升级 zlib 的固定顺序：两侧 conanfile 同步改版本 → 先重编安装 MxNativeReaders
# → 再 conan install 本工程；任何一侧漏改，这里 configure 期直接报错拦下。
#-----------------------------------------------------------------------------
set(MxNativeReaders_ZLIB_VERSION "1.3.2")
set(MxNativeReaders_ZLIB_SHARED "True")
set(_mxnr_manifest "${MxNativeReaders_Dir}/share/mx_native_readers/zlib_manifest.txt")
if(EXISTS "${_mxnr_manifest}")
	file(STRINGS "${_mxnr_manifest}" _mxnr_manifest_lines)
	foreach(_mxnr_line IN LISTS _mxnr_manifest_lines)
		if(_mxnr_line MATCHES "^zlib_version[ \t]*=[ \t]*(.+)$")
			string(STRIP "${CMAKE_MATCH_1}" MxNativeReaders_ZLIB_VERSION)
		elseif(_mxnr_line MATCHES "^zlib_shared[ \t]*=[ \t]*(.+)$")
			string(STRIP "${CMAKE_MATCH_1}" MxNativeReaders_ZLIB_SHARED)
		endif()
	endforeach()
	unset(_mxnr_line)
	unset(_mxnr_manifest_lines)
endif()

if(NOT ZLIB_VERSION_STRING)
	message(WARNING "FindMxNativeReaders: 未取得 conan zlib 版本（ZLIB_VERSION_STRING 为空），跳过一致性校验")
elseif(NOT ZLIB_VERSION_STRING VERSION_EQUAL MxNativeReaders_ZLIB_VERSION)
	message(FATAL_ERROR
		"zlib 版本不一致：本工程 conan 安装 ${ZLIB_VERSION_STRING}，MxNativeReaders 构建于 "
		"${MxNativeReaders_ZLIB_VERSION}（来源：${_mxnr_manifest}）。两侧必须同版本——"
		"同步修改 Basic/MxNativeReaders/conanfile.py 与本工程 conanfile.py 的 zlib 版本，"
		"并先以新 zlib 重编安装 MxNativeReaders，再重跑本工程 conan install。")
endif()

# 形态校验：唯一确定会断链的组合是"库按共享 zlib（ZLIB_DLL，引用 __imp_* 导入
# 符号）构建 + 消费方解析到静态 zlib"——届时 LNK2019/LNK1120。反向组合（库按
# 静态编译、消费方共享链接）合法：普通符号经导入库 thunk 绑定 DLL。
# 当前实际状态：两侧均按静态 zlib 链接（库侧 conan 图默认静态，本工程亦未设
# zlib/*:shared），zlib 直接编入 PostIOFile.dll，无 zlib1.dll 运行时部署。
# 形态判定分层：conan 数据变量 zlib_LIBRARY_TYPE_<CONFIG>（SHARED/STATIC，权威）
# → 真实导入目标的 TYPE（内置 FindZLIB 解析时）→ 库文件名（共享 zdll/静态
# zlibstatic）。三层都落空时跳过校验，避免误报阻塞构建。
set(_mxnr_consumer_zlib_shared "")
foreach(_cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
	if(NOT "${zlib_LIBRARY_TYPE_${_cfg}}" STREQUAL "")
		if("${zlib_LIBRARY_TYPE_${_cfg}}" MATCHES "SHARED")
			set(_mxnr_consumer_zlib_shared TRUE)
		else()
			set(_mxnr_consumer_zlib_shared FALSE)
		endif()
	endif()
endforeach()
if(_mxnr_consumer_zlib_shared STREQUAL "")
	get_target_property(_mxnr_zlib_type ZLIB::ZLIB TYPE)
	if(_mxnr_zlib_type STREQUAL "SHARED_LIBRARY")
		set(_mxnr_consumer_zlib_shared TRUE)
	elseif(_mxnr_zlib_type MATCHES "STATIC_LIBRARY|UNKNOWN_LIBRARY")
		set(_mxnr_consumer_zlib_shared FALSE)
	endif()
	unset(_mxnr_zlib_type)
endif()
if(_mxnr_consumer_zlib_shared STREQUAL "")
	set(_mxnr_consumer_zlib_shared TRUE)
	foreach(_mxnr_zlib_var ZLIB_LIBRARIES ZLIB_LIBRARY zlib_LIBRARIES_DEBUG zlib_LIBRARIES_RELEASE)
		if(NOT "${${_mxnr_zlib_var}}" STREQUAL "" AND "${${_mxnr_zlib_var}}" MATCHES "zlibstatic")
			set(_mxnr_consumer_zlib_shared FALSE)
		endif()
	endforeach()
	unset(_mxnr_zlib_var)
endif()

if(MxNativeReaders_ZLIB_SHARED AND _mxnr_consumer_zlib_shared STREQUAL FALSE)
	message(FATAL_ERROR
		"zlib 形态不一致：MxNativeReaders 按共享 zlib（ZLIB_DLL，引用 __imp_* 导入符号）"
		"构建，而本工程解析到静态 zlib，链接将出现 LNK2019/LNK1120 未解析符号。"
		"两种解法任选其一：本工程 conanfile.py 保持 zlib/*:shared=True；或把 "
		"Basic/MxNativeReaders 的 conan 安装改为静态 zlib（默认即是）并重编安装该库。")
endif()
unset(_mxnr_consumer_zlib_shared)
unset(_mxnr_manifest)

add_library(MxNativeReaders STATIC IMPORTED)
set_property(TARGET MxNativeReaders PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MxNativeReaders_INC_DIR})
set_property(TARGET MxNativeReaders PROPERTY INTERFACE_LINK_LIBRARIES ZLIB::ZLIB)
set_property(TARGET MxNativeReaders APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)

if(WIN32)
	set_target_properties(MxNativeReaders PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxNativeReaders_LIB_DIR}/mx_native_readers.lib"
		IMPORTED_LOCATION_DEBUG "${MxNativeReaders_LIB_DIR}/mx_native_readersd.lib"
	)
else()
	set_target_properties(MxNativeReaders PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxNativeReaders_LIB_DIR}/libmx_native_readers.a"
		IMPORTED_LOCATION_DEBUG "${MxNativeReaders_LIB_DIR}/libmx_native_readersd.a"
		LINK_FLAGS "-Wl,-rpath,${MxNativeReaders_LIB_DIR}/"
	)
endif()

# 注意：静态库必须与消费方编译配置严格匹配（/MD vs /MDd、_ITERATOR_DEBUG_LEVEL），
# 不允许 Debug 缺失时回退链接 Release 库——那会触发 LNK2038 运行时库不匹配。
# 若 Debug 链接报"无法打开 mx_native_readersd.lib"，请先将 MxNativeReaders 以
# Debug 配置编译安装（跑一次 --build-type Debug 的 compile 阶段）。
