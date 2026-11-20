cmake_minimum_required(VERSION 3.16)
project(MechMainWindow)

# ---------------------------------------------------------------------------
# ① 让 MSVC 全程按 UTF-8 编译（解决 C2001/C3688/C2018/MSB8084）
# ---------------------------------------------------------------------------
if(WIN32)
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
else()
    add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
endif()

# ---------------------------------------------------------------------------
# 平台线程库
# ---------------------------------------------------------------------------
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# ② 头文件搜索路径（保持你原来）
# ---------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../MainWindow)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../IOFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../APIServer)
include_directories(${Render_INC_DIR})
#include_directories(${OpenCASCADE_INCLUDE_DIRS}) #【OCC摘除·S10/P0-1 终局】
# GeoCommand 头文件（mDirectModelingController.h 等引用）已迁入 RebuildBasicData
# （MXGeoCommand_INC_DIR 为旧架构遗留空变量），改用 BasicData 安装头目录
include_directories(${BasicData_INC_DIR})
# ModelData 头文件（mSolveTaskCenter.h 引用 mSolveTaskData.h 等），由 FindModelData
# 定位到 Basic/RebuildModelData/install/include；MainWindow 对各 Mech* 库的
# 链接为私有传播，MxMech 需显式加入该 include 目录
include_directories(${ModelData_INC_DIR})
# PostWindow/Common 头（组合根装配：mPostMenuContributor/IRibbonHost）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostWindow)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)

# ---------------------------------------------------------------------------
# ③ 自动包含当前目录 & 扫描源码
# ---------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")

# ---------------------------------------------------------------------------
# ④ 生成可执行目标
# ---------------------------------------------------------------------------
add_executable(${CMAKE_PROJECT_NAME}
    WIN32        # 关闭控制台窗口
    ${_header}
    ${_source}
    icon.rc
)
# 多体产品主程序输出名（根 CMakeLists 的 MX_PRODUCT_EXE_NAME）；未定义时保持 MxMech.exe
if(MX_PRODUCT_EXE_NAME)
    set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${MX_PRODUCT_EXE_NAME})
endif()

# ---------------------------------------------------------------------------
# ⑤ 设为 VS 启动项
# ---------------------------------------------------------------------------
set_property(DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${CMAKE_PROJECT_NAME})

# ---------------------------------------------------------------------------
# ⑥ 依赖库列表（保持你原来）
# ---------------------------------------------------------------------------
list(APPEND _depend_library
    ${CurrentModel}MainWindow
    ${CurrentModel}IOFile
    ${CurrentModel}Common
    ${CurrentModel}PreData
    PostWindow
    APIServer
)
list(APPEND _runtimes_libraries
    Qt5::Core Qt5::Gui Qt5::Widgets  MxFunctions MxPython
    Qt5::Network Qt5::WebSockets Qt5::Sql  Qt5::WebEngine Qt5::WebEngineWidgets ssl crypto
    mpcomm::mpcomm
    FieldTransferCore::FieldTransferCore
    FieldTransferMapping::FieldTransferMapping
    #【OCC摘除·S11】${OpenCASCADE_LIBRARIES} 引用移除：根级 find_package 已注释
    # （S10 终局），变量为空的链接无效果；二进制零 TK 已由 dumpbin 验证
)

if(MxPre_LINUX)
# Conan2查找
#find_package(OpenCASCADE 7.9.0 CONFIG REQUIRED) #【OCC摘除·S10/P0-1 终局】

list(APPEND _linux_rpath_libraries
	 osg osgUtil osgViewer osgGA osgDB osgFX osgText Threads::Threads
	 #【OCC摘除·S11】${OpenCASCADE_LIBRARIES} 引用移除（同上，变量为空的链接无效果）
    )
endif()

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
# Visual Studio中调试依赖的独立环境变量
#set_property(TARGET ${PROJECT_NAME}	 PROPERTY VS_DEBUGGER_ENVIRONMENT "PATH=C:/Qt/Qt5.12.3/5.12.3/msvc2017_64/bin")
 
 
# 如果上面写法还是不行，那就试试这样写（保持原来默认的环境不变的情况下添加Qt的运行环境变量）
set(Qt5_PATH ${Qt5_DIR}/../../../bin)

# Conan 2 第三方库运行时路径。
# 数据来源优先级：
#   1. preset 注入的 $ENV{CONAN_RUNTIME_PATHS_*}（conan-install/compile 阶段经
#      CMakePresets.json environment 传入，与本机构建状态一致）
#   2. 兜底：磁盘聚合文件 ThirdLib/conan_install/conan_runtime_paths_<cfg>.cmake
#      （VS sln 的 ZERO_CHECK 重新 cmake 时不带 preset 环境，$ENV{} 为空时启用）
get_filename_component(_conan_rt_dir "${CMAKE_SOURCE_DIR}/../../../ThirdLib/conan_install" ABSOLUTE)

set(CONAN_RUNTIME_PATHS_DEBUG "")
set(CONAN_RUNTIME_PATHS_RELEASE "")
set(CONAN_RUNTIME_PATHS_RELWITHDEBINFO "")
set(QT_PLUGIN_PATH_DEBUG "")
set(QT_PLUGIN_PATH_RELEASE "")
set(QT_PLUGIN_PATH_RELWITHDEBINFO "")

# 1) preset 环境变量优先
set(_conan_rt_debug_raw "$ENV{CONAN_RUNTIME_PATHS_DEBUG}")
set(_conan_rt_release_raw "$ENV{CONAN_RUNTIME_PATHS_RELEASE}")
set(_conan_rt_relwithdebinfo_raw "$ENV{CONAN_RUNTIME_PATHS_RELWITHDEBINFO}")
set(_qt_plugin_debug_raw "$ENV{QT_PLUGIN_PATH_DEBUG}")
set(_qt_plugin_release_raw "$ENV{QT_PLUGIN_PATH_RELEASE}")
set(_qt_plugin_relwithdebinfo_raw "$ENV{QT_PLUGIN_PATH_RELWITHDEBINFO}")

if(_conan_rt_debug_raw)
    list(APPEND CONAN_RUNTIME_PATHS_DEBUG ${_conan_rt_debug_raw})
endif()
if(_conan_rt_release_raw)
    list(APPEND CONAN_RUNTIME_PATHS_RELEASE ${_conan_rt_release_raw})
endif()
if(_conan_rt_relwithdebinfo_raw)
    list(APPEND CONAN_RUNTIME_PATHS_RELWITHDEBINFO ${_conan_rt_relwithdebinfo_raw})
endif()
if(_qt_plugin_debug_raw)
    list(APPEND QT_PLUGIN_PATH_DEBUG ${_qt_plugin_debug_raw})
endif()
if(_qt_plugin_release_raw)
    list(APPEND QT_PLUGIN_PATH_RELEASE ${_qt_plugin_release_raw})
endif()
if(_qt_plugin_relwithdebinfo_raw)
    list(APPEND QT_PLUGIN_PATH_RELWITHDEBINFO ${_qt_plugin_relwithdebinfo_raw})
endif()

# 2) 兜底：环境变量缺失的配置，从磁盘聚合文件读取
foreach(_cfg debug release relwithdebinfo)
    string(TOUPPER "${_cfg}" _cfg_u)
    if(NOT CONAN_RUNTIME_PATHS_${_cfg_u})
        set(_rt_file "${_conan_rt_dir}/conan_runtime_paths_${_cfg}.cmake")
        if(EXISTS "${_rt_file}")
            unset(CONAN_RUNTIME_PATHS)
            unset(QT_PLUGIN_PATH)
            include("${_rt_file}")
            set(CONAN_RUNTIME_PATHS_${_cfg_u} ${CONAN_RUNTIME_PATHS})
            set(QT_PLUGIN_PATH_${_cfg_u} ${QT_PLUGIN_PATH})
        endif()
    endif()
endforeach()

# RelWithDebInfo 与 Release 同为 /MD 运行库：聚合文件/环境变量缺失时回退 Release
# （策略 B 下磁盘聚合文件为 release 命名，无 relwithdebinfo 变体）
if(NOT CONAN_RUNTIME_PATHS_RELWITHDEBINFO)
    set(CONAN_RUNTIME_PATHS_RELWITHDEBINFO ${CONAN_RUNTIME_PATHS_RELEASE})
endif()
if(NOT QT_PLUGIN_PATH_RELWITHDEBINFO)
    set(QT_PLUGIN_PATH_RELWITHDEBINFO ${QT_PLUGIN_PATH_RELEASE})
endif()

# 兜底：若某一配置缺失，使用另一配置
# if(NOT CONAN_RUNTIME_PATHS_DEBUG)
#     set(CONAN_RUNTIME_PATHS_DEBUG ${CONAN_RUNTIME_PATHS_RELEASE})
# endif()
# if(NOT CONAN_RUNTIME_PATHS_RELEASE)
#     set(CONAN_RUNTIME_PATHS_RELEASE ${CONAN_RUNTIME_PATHS_DEBUG})
# endif()

# 顺序约束：属主模块的 install\bin 必须排在 RebuildMesh 之前——RebuildMesh 的
# install\bin 里带有其他模块 DLL 的搭车副本（GeoServerd/OCCAdapterd/MXGeoManagerd/
# MeshGeoDatad），若 Mesh 在前会用旧副本遮蔽属主模块的新版本，导致"无法找到入口"。
set(_my_path_common ${TBB_BIN_DIR} ${BasicData_BIN_DIR} ${GeoData_BIN_DIR} ${ModelData_BIN_DIR} ${MxFunctions_BIN_DIR} ${Geometry_BIN_DIR} ${Mesh_BIN_DIR} ${PlotView_BIN_DIR} ${Plot3D_BIN_DIR}
${Render_BIN_DIR} ${Qt5_PATH} ${MotionEngine_BIN_DIR})
set(MY_PATH_RELEASE ${CONAN_RUNTIME_PATHS_RELEASE} ${_my_path_common})
set(MY_PATH_DEBUG ${CONAN_RUNTIME_PATHS_DEBUG} ${_my_path_common})
set(MY_PATH_RELWITHDEBINFO ${CONAN_RUNTIME_PATHS_RELWITHDEBINFO} ${_my_path_common})

# Qt 插件路径：优先使用系统 Qt 的 plugins 目录，否则使用 environment 传入的 QT_PLUGIN_PATH
if(Qt5_PATH AND EXISTS "${Qt5_PATH}/../plugins")
    set(_qt_qpa_plugin_path "${Qt5_PATH}/../plugins")
else()
    set(_qt_plugin_release "")
    set(_qt_plugin_debug "")
    set(_qt_plugin_relwithdebinfo "")
    if(QT_PLUGIN_PATH_RELEASE)
        list(GET QT_PLUGIN_PATH_RELEASE 0 _qt_plugin_release)
    endif()
    if(QT_PLUGIN_PATH_DEBUG)
        list(GET QT_PLUGIN_PATH_DEBUG 0 _qt_plugin_debug)
    endif()
    if(QT_PLUGIN_PATH_RELWITHDEBINFO)
        list(GET QT_PLUGIN_PATH_RELWITHDEBINFO 0 _qt_plugin_relwithdebinfo)
    endif()
    if(NOT _qt_plugin_release)
        set(_qt_plugin_release "${_qt_plugin_debug}")
    endif()
    if(NOT _qt_plugin_relwithdebinfo)
        set(_qt_plugin_relwithdebinfo "${_qt_plugin_release}")
    endif()
    set(_qt_qpa_plugin_path "$<IF:$<CONFIG:Debug>,${_qt_plugin_debug},$<IF:$<CONFIG:RelWithDebInfo>,${_qt_plugin_relwithdebinfo},${_qt_plugin_release}>>")
endif()

# 多配置支持：VS_DEBUGGER_ENVIRONMENT 支持生成器表达式，按 $<CONFIG> 分别求值。
# 生成器表达式内部的分号必须用 $<SEMICOLON> 转义，否则会被当作参数分隔符。
string(REPLACE ";" "$<SEMICOLON>" _my_path_release_ge "${MY_PATH_RELEASE}")
string(REPLACE ";" "$<SEMICOLON>" _my_path_debug_ge "${MY_PATH_DEBUG}")
string(REPLACE ";" "$<SEMICOLON>" _my_path_relwithdebinfo_ge "${MY_PATH_RELWITHDEBINFO}")
set(_vs_path "$<IF:$<CONFIG:Debug>,${_my_path_debug_ge},$<IF:$<CONFIG:RelWithDebInfo>,${_my_path_relwithdebinfo_ge},${_my_path_release_ge}>>")

# VS_DEBUGGER_ENVIRONMENT 使用分号分隔多个环境变量，但 PATH 值本身也包含分号。
# 使用换行符分隔多个环境变量，避免分号冲突。
set(_vs_env "PATH=${_vs_path}$<SEMICOLON>%PATH%\nQT_QPA_PLATFORM_PLUGIN_PATH=${_qt_qpa_plugin_path}")
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY VS_DEBUGGER_ENVIRONMENT "${_vs_env}")
# ---------------------------------------------------------------------------
# ⑧ 链接库
# ---------------------------------------------------------------------------
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    ${_runtimes_libraries}
    ${_depend_library}
	${_linux_rpath_libraries}
    
	$<$<PLATFORM_ID:Windows>:Dbghelp>
	$<$<PLATFORM_ID:Windows>:Shell32>
	$<$<PLATFORM_ID:Windows>:Propsys>
	$<$<PLATFORM_ID:Windows>:licenseshensi::licenseshensi>
    $<$<PLATFORM_ID:Linux>:xcb>
)

# ---------------------------------------------------------------------------
# ⑧.1 任务栏 AUMID 按产品线参数化（CurrentModel = Mech/Pre/Dyna/CFD...）：
# 不同产品共机部署时任务栏身份互不干扰；同一产品各版本保持稳定不丢固定项。
# 与安装器开始菜单/桌面快捷方式写入的 AUMID 保持一致。
# ---------------------------------------------------------------------------
set(MX_APP_AUMID "Hnmx.MxSim.${CurrentModel}")
set(MX_APP_DISPLAY_NAME "MxSim-${CurrentModel}")
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    "MX_APP_AUMID=L\"${MX_APP_AUMID}\""
    "MX_APP_DISPLAY_NAME=L\"${MX_APP_DISPLAY_NAME}\"")

# ---------------------------------------------------------------------------
# ⑨ 依赖 & 运行时拷贝（保持你原来）
# ---------------------------------------------------------------------------
add_dependencies(${CMAKE_PROJECT_NAME} ${_depend_library})

list(APPEND MxPre_Runtimes_Libraries ${_runtimes_libraries})
set(MxPre_Runtimes_Libraries ${MxPre_Runtimes_Libraries} PARENT_SCOPE)

# ---------------------------------------------------------------------------
# ⑩ Qt 插件安装（保持你原来）
# ---------------------------------------------------------------------------
get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
execute_process(COMMAND "${_qmake_executable}" -query QT_INSTALL_ARCHDATA
    OUTPUT_VARIABLE _qt_archdata_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE)

if(MxSim_ENABLE_DEV)
    list(REMOVE_DUPLICATES MxPre_Runtimes_Libraries)
    if(WIN32)
        include(InitRuntime)
    endif()
endif()

install(DIRECTORY
    ${_qt_archdata_dir}/plugins/imageformats
    ${_qt_archdata_dir}/plugins/platforms
    DESTINATION ${INSTALL_BINDIR}
)
