#-----------------------------------------------------------------------------
# 工程属性设置
#-----------------------------------------------------------------------------
# 指定所需cmake的最低版本
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

#设置当前的前处理模块
set(CurrentModel Mech)
#set(CurrentModel Mech CACHE STRING "选择当前的模块")
#set_property(CACHE CurrentModel PROPERTY STRINGS Pre Dyna Mech Multi Emag CFD  CFD1 Fatigue )

#【OCC摘除·S11】OCC 版本缓存选项移除（宏无源码引用，见下方已注释块）
#set(OCC_VERSION "OCC7.9.0" CACHE STRING "选择OCC版本")
#set_property(CACHE OCC_VERSION PROPERTY STRINGS "OCC7.9.0" "OCC7.5.0")

# 设置是否启用 CGM 商业 CAD 格式转换，以及使用哪个版本
# 宏为目录级定义，Debug/Release 两配置同享；非法取值直接报错，
# 避免静默进入"启用了 HAS_CGM_INTEROP 却无版本宏"的半开状态
set(CGM_VERSION "None" CACHE STRING "选择CGM版本 (CGMWIN7/CGMWIN10)，None 表示不启用")
set_property(CACHE CGM_VERSION PROPERTY STRINGS "None" "CGMWIN7" "CGMWIN10")
if(NOT CGM_VERSION STREQUAL "None")
	if(NOT CGM_VERSION MATCHES "^(CGMWIN7|CGMWIN10)$")
		message(FATAL_ERROR "CGM_VERSION='${CGM_VERSION}' 非法：合法取值为 None / CGMWIN7 / CGMWIN10")
	endif()
	add_compile_definitions(HAS_CGM_INTEROP)
	if(CGM_VERSION STREQUAL "CGMWIN10")
		add_compile_definitions(CGM_VERSION_WIN10)
	else()
		add_compile_definitions(CGM_VERSION_WIN7)
	endif()
endif()

# 多体产品形态宏与主程序输出名（MxMotion.exe）。暂时写死为 Motion，
# 合并回 dev_Mech 时删除下面两行即可恢复隐式形态默认与 MxMech.exe
add_compile_definitions(MX_RIBBON_FORM_MOTION)
set(MX_PRODUCT_EXE_NAME MxMotion)

#【OCC摘除·S11】OCC 版本选择块移除：全 GUI 零 OCC（根级 find_package 已注释，
# 宏无源码引用）
#add_definitions(-DOCC_VERSION="${OCC_VERSION}")
#add_definitions(-DOCC_VERSION_STR="${OCC_VERSION}")
#if(OCC_VERSION STREQUAL "OCC7.5.0")
#	add_definitions(-DOCC7_5_0)
#elseif(OCC_VERSION STREQUAL "OCC7.9.0")
#  add_definitions(-DOCC7_9_0)
#endif()

# 让 MSVC 使用 64 位编译器宿主：巨型 qrc 生成的 cpp(百万行级)用 32 位 cl.exe 会报 C1060 堆空间不足
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE x64)

# 让 MSVC 使用 64 位编译器宿主：巨型 qrc 生成的 cpp(百万行级)用 32 位 cl.exe 会报 C1060 堆空间不足
set(CMAKE_VS_PLATFORM_TOOLSET_HOST_ARCHITECTURE x64)

# 设置项目名称 语言
project(Mx${CurrentModel}
	VERSION 2.0
	LANGUAGES CXX
)

#-----------------------------------------------------------------
# 几何建模后端选择（需与 RebuildGeometry 的选项保持一致）
#-----------------------------------------------------------------
#【后端可切换】后端选择权归 RebuildGeometry（其 install 导出 mGeoEngineBackend.cmake，
# 由 cmake/FindGeometry.cmake include 并自动路由）。本工程一律链接中性入口 GeoAdapter；
# 消费方 CMake 禁止直接引用 OCCAdapter/CGMAdapter/PoissonAdapter 等具体后端目标名。

if(WIN32)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
endif()

#-----------------------------------------------------------------------------
# 编译系统设置
#-----------------------------------------------------------------------------
# c++语言标准
# 需 C++17：依赖的 RebuildRender postalg 头文件使用了嵌套命名空间定义（C2429）
set(CMAKE_CXX_STANDARD 17)
# 指定是否使用编译器特定的扩展
set(CMAKE_CXX_EXTENSIONS OFF)
# 是否强制要求C++标准为${CMAKE_CXX_STANDARD},如果为OFF则可能使用旧的标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Qt5.13 WebEngineCore 的 .dynsym 存在缺陷，GNU ld 会拒绝链接；使用 gold 解决
if(UNIX)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=gold")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=gold")
endif()
# 默认编译动态库
set(BUILD_SHARED_LIBS ON)
# Debug添加后缀d
set(CMAKE_DEBUG_POSTFIX d)
# 禁用 std::byte，避免 Windows SDK 头文件中的 byte 宏与 C++17 std::byte 冲突
add_compile_definitions(_HAS_STD_BYTE=0)



#-----------------------------------------------------------------------------
# 检测操作系统
#-----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(MxPre_LINUX True)

elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(MxPre_WIN True)
else()
  message(SEND_ERROR "No Support System!")
endif()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/install" CACHE PATH "${PROJECT_NAME}的安装路径" FORCE)
endif()
######################使用多处理器编译########################
IF (WIN32)
		if(MSVC)
				OPTION(USE_MP "use multiple" ON)
				OPTION(ProjectConfig_Global_COMPILE_FLAGS_WITH_MP 
					"Set The Global Option COMPILE_FLAGS /MP to target." ON)
				if(ProjectConfig_Global_COMPILE_FLAGS_WITH_MP OR USE_MP)
					set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
					set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
				endif()
				# 启用C++异常 
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHa") 
				set(VS_STARTUP_PROJECT ${PROJECT_NAME})
		ENDIF(MSVC)
ENDIF()
#-----------------------------------------------------------------------------
# 编译选项设置
#-----------------------------------------------------------------------------
option(MxSim_ENABLE_DEV "ON:开启代码调试，OFF:仅安装程序" ON)

#-----------------------------------------------------------------------------
# 指定编译选项
#-----------------------------------------------------------------------------
# 仅对单配制编译器(Makefile Generators or Ninja)生效
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif (POLICY CMP0091)

#添加预定义
if(MSVC)
	add_definitions(-D_SCL_SECURE_NO_DEPRECATE)
endif()

#-----------------------------------------------------------------------------
# 从系统查找Qt模块,开启Qt中间文件的自动生成
#-----------------------------------------------------------------------------
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

list(APPEND QtNeededModules
	Core
	Widgets
	Gui
	Help
	Network
	WebSockets
	Sql
	WebEngine
	WebEngineWidgets
	OpenGL   #QOpenGLWidget（Qt5 中属 QtOpenGL 模块）：PostCurve 接入 Plot3D 的 Qt3DPlotWidget 头所需
)
if(MxPre_LINUX)
	#list(APPEND QtNeededModules XcbQpa)
	# Qt5XcbQpa is a private platform plugin; only add it to find_package
	# if its CMake config is actually present in the active Qt installation.
	find_package(Qt5XcbQpa QUIET)
	if(Qt5XcbQpa_FOUND)
		list(APPEND QtNeededModules XcbQpa)
		message(STATUS "Qt5XcbQpa found, adding to QtNeededModules")
	else()
		message(STATUS "Qt5XcbQpa not found, skipping XcbQpa component")
	endif()
endif()

#查找外部库
#Qt5为查找的外部库名称
#COMPONENTS：其后跟需要查找的组件名称，任何一个没有找到视为查找失败
#REQUIRED：当没有查找到外部库时，给出错误提示
find_package(Qt5 CONFIG COMPONENTS ${QtNeededModules} REQUIRED)
#变量Qt5_FOUND表示查找包Qt5时是否找到，形如<NAME>_FOUND
if(Qt5_FOUND)
  message(STATUS "Found Qt5: ${Qt5_DIR}")
  if(Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  endif()
else()
  set(Qt5_DIR "Qt5_DIR-NOTFOUND" CACHE PATH "Qt5_DIR")
endif()

#-----------------------------------------------------------------------------
# 引入依赖库
#-----------------------------------------------------------------------------
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")



# MxFunctions
find_package(MxFunctions REQUIRED)

# MxdbReaderCore
# 独立开发环境：自动探测相对路径；自动构建系统已通过 CMAKE_PREFIX_PATH 注入。
set(_MxdbReaderCore_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/MxdbReaderCore/install")
if(EXISTS "${_MxdbReaderCore_INSTALL_DIR}" AND NOT "${_MxdbReaderCore_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${_MxdbReaderCore_INSTALL_DIR}")
endif()
unset(_MxdbReaderCore_INSTALL_DIR)
find_package(MxdbReaderCore REQUIRED)

# UnstructureCellPost (ucp)
# 云图-过滤 导出STL 使用的等值面拼接算法库（header-only）
set(_UCP_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/UnstructureCellPost/install")
if(EXISTS "${_UCP_INSTALL_DIR}" AND NOT "${_UCP_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${_UCP_INSTALL_DIR}")
endif()
unset(_UCP_INSTALL_DIR)
find_package(ucp 0.1.7 REQUIRED)

# mpcomm（平台通信 SDK，Basic 基础模块，与 MxdbReaderCore 同样的探测方式）
# 独立开发环境：自动探测相对路径；自动构建系统已通过 CMAKE_PREFIX_PATH 注入。
if(NOT DEFINED mpcomm_INSTALL_DIR)
    set(mpcomm_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/mpcomm/install")
endif()
if(EXISTS "${mpcomm_INSTALL_DIR}" AND NOT "${mpcomm_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${mpcomm_INSTALL_DIR}")
endif()
find_package(mpcomm 1.0.0 EXACT CONFIG REQUIRED)

# FieldTransferCore / FieldTransferMapping（场数据传递，Basic 基础模块）
if(NOT DEFINED fieldtransfercore_INSTALL_DIR)
    set(fieldtransfercore_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/fieldtransfercore/install")
endif()
if(EXISTS "${fieldtransfercore_INSTALL_DIR}" AND NOT "${fieldtransfercore_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${fieldtransfercore_INSTALL_DIR}")
endif()
find_package(FieldTransferCore 0.1.0 EXACT CONFIG REQUIRED)

if(NOT DEFINED fieldtransfermapping_INSTALL_DIR)
    set(fieldtransfermapping_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/fieldtransfermapping/install")
endif()
if(EXISTS "${fieldtransfermapping_INSTALL_DIR}" AND NOT "${fieldtransfermapping_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${fieldtransfermapping_INSTALL_DIR}")
endif()
find_package(FieldTransferMapping 0.1.0 EXACT CONFIG REQUIRED)
# PchReader
set(PchReader_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/PchReader/install")
if(EXISTS "${PchReader_INSTALL_DIR}" AND NOT "${PchReader_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${PchReader_INSTALL_DIR}")
endif()
unset(PchReader_INSTALL_DIR)
find_package(PchReader REQUIRED)


# Render
find_package(Render REQUIRED)

# OpenSceneGraph (Conan 2)
find_package(OpenSceneGraph REQUIRED CONFIG)

# Conan 2 CMakeDeps 目标兼容映射：openscenegraph::osg -> osg
if(NOT TARGET osg)
    add_library(osg ALIAS openscenegraph::osg)
endif()
if(NOT TARGET osgDB)
    add_library(osgDB ALIAS openscenegraph::osgDB)
endif()
if(NOT TARGET osgGA)
    add_library(osgGA ALIAS openscenegraph::osgGA)
endif()
if(NOT TARGET osgUtil)
    add_library(osgUtil ALIAS openscenegraph::osgUtil)
endif()
if(NOT TARGET osgViewer)
    add_library(osgViewer ALIAS openscenegraph::osgViewer)
endif()
if(NOT TARGET osgFX)
    add_library(osgFX ALIAS openscenegraph::osgFX)
endif()
if(NOT TARGET osgText)
    add_library(osgText ALIAS openscenegraph::osgText)
endif()
if(NOT TARGET OpenThreads AND TARGET OpenThreads::OpenThreads)
    add_library(OpenThreads ALIAS OpenThreads::OpenThreads)
endif()

# OpenSSL (Conan 2)
find_package(OpenSSL REQUIRED CONFIG)

# 兼容：Conan 2 的 openssl 组件 -> 传统 ssl / crypto 目标名
if(TARGET OpenSSL::SSL AND NOT TARGET ssl)
    add_library(ssl ALIAS OpenSSL::SSL)
endif()
if(TARGET OpenSSL::Crypto AND NOT TARGET crypto)
    add_library(crypto ALIAS OpenSSL::Crypto)
endif()

# OSG 3.6 中部分 API 被标记为 deprecated，需启用以兼容旧代码
add_compile_definitions(OSG_USE_DEPRECATED_API)

# OpenCASCADE (Conan 2)
#find_package(OpenCASCADE REQUIRED)  #【OCC摘除·S10/P0-1 终局】全 GUI 零 OCC（PreData/IOFile/PostIOFile/MainWindow/MxMech 均已摘链）
#【OCC摘除·S10/P0-1 终局】链接已全摘（约束以二进制为准）；以下仅为编译期 include：
# mGeoPanelBase/mGeoDataModelSet 等 BasicData 旧导出头仍携带 OCC 头（头卫生渐进项，见 05 方案 §五）
#【OCC摘除·S10/P0-1 终局4】编译期 OCC 清零：全 GUI TU 已无 .hxx 可达
#（直模控制器/桩/GeoWidget 均在 #if 0 或出列构建中），include 目录整体摘除。
#find_package(OpenCASCADE REQUIRED)
#include_directories(${OpenCASCADE_INCLUDE_DIRS})

# Qwt (Conan 2)
find_package(qwt REQUIRED)
if(TARGET qwt::qwt AND NOT TARGET Qwt)
	add_library(Qwt ALIAS qwt::qwt)
endif()

find_package(ffmpeg REQUIRED)
if(TARGET ffmpeg::ffmpeg AND NOT TARGET ffmpeg)
	add_library(ffmpeg ALIAS ffmpeg::ffmpeg)
endif()

# PlotView
find_package(PlotView REQUIRED)

# Plot3D（3D绘图引擎：Qt3DPlotWidget+Series+MapFileLoader，消费 Basic/Plot3D install 树，
# 本仓库不重建；仅 PostCurve 链接，头面不出模块）
find_package(Plot3D REQUIRED)

# Mesh
find_package(Mesh REQUIRED)

# muparser (Conan 2)
find_package(muparser REQUIRED)
if(TARGET muparser::muparser AND NOT TARGET muparser)
	add_library(muparser ALIAS muparser::muparser)
endif()


# Shortcut
find_package(Shortcut REQUIRED)

# BasicData
find_package(BasicData REQUIRED)

#-----------------------------------------------------------------------------
# robustpredicates + rtree conan 包（2026-11-13 消费侧接线，缺陷修复：downstream-wiring）
# RebuildBasicData/RebuildMesh 切 conan 后两模块 install/include 不再携带
# robustPredicates.h/rtree.h（meshgeo-3rdparty-conan FR-04 零残留），GUI 经传递
# 头链消费二者：①BasicData 五头链（MeshEntity.h:8→MXGeoSolid.h:14、MXGeoEntity.h/
# MXMeshFacet.h/MXMeshLine.h/MXMeshVertex.h→MXMeshParameters.h:5→robustPredicates.h）
# ②Mesh 安装头链（DiscreteSurface.h:9→rtree.h，被 MXMesh.h/meshFace.h 等 7 头传递；
# meshFace.h:7→MXGeoPVTFace.h:8→rtree.h）——MechMainWindow 24 TU C1083 "rtree.h"
# 编译实证；GUI 零 rp/rtree 符号实例化（仅头路径）。
# 落点理由：两包的 CMakeDeps target 为 INTERFACE IMPORTED 且非 GLOBAL（目录级
# 作用域）；GUI 全目标逐个 target_link_libraries 须改动他人进行中的脏文件
#（src/PostWindow/CMakeLists.txt 等），故把两包链接叠加在 FindBasicData / FindMesh
# 创建的模块导入目标（其 INTERFACE_INCLUDE_DIRECTORIES = 各自 install/include，
# 正是上述传递链的宿主）接口面上——凡既有链接链覆盖者零改动自动获得头路径。
# 注意 IMPORTED 目标只接受 INTERFACE 关键字（PUBLIC/PRIVATE 对 imported 目标报
# "IMPORTED library can only be used with the INTERFACE keyword"）
# （find_package 置于 add_subdirectory(src) 之前，两包 target 对全部子目录可见）
#-----------------------------------------------------------------------------
find_package(RobustPredicates REQUIRED)	# robustpredicates/2.0@mxsim/stable → RobustPredicates::RobustPredicates（动态库）
find_package(RTree REQUIRED)			# rtree/20261111@mxsim/stable → RTree::rtree（仅头）
foreach(_mod_imported
		CommonData MXGeoCommand MXGeoDataExchange MXGeoData MXGeoManager
		MeshData MeshGeoData MeshPanelData PostData Tasking	# FindBasicData.cmake 导入目标（src 子目录）
		MeshEngine MeshGUIData MeMeshProcess MeshLib MeSurfaceWrap)	# FindMesh.cmake 导入目标（MeshDespLibs）
	if(TARGET ${_mod_imported})
		target_link_libraries(${_mod_imported} INTERFACE
			RobustPredicates::RobustPredicates RTree::rtree)
	endif()
endforeach()

# GeoData
find_package(GeoData REQUIRED)

# ModelData
find_package(ModelData REQUIRED)

# MotionEngine（多体求解引擎，预编译 SDK：deps/MotionEngine/install）
find_package(MotionEngine REQUIRED)

# Python (Conan 2)
find_package(python REQUIRED CONFIG)
if(NOT TARGET MxPython AND TARGET python::python)
    add_library(MxPython ALIAS python::python)
endif()

find_package(pybind11 REQUIRED)
if(TARGET pybind11::pybind11 AND NOT TARGET pybind11)
	add_library(pybind11 ALIAS pybind11::pybind11)
endif()

# Debug 构建定义 Py_DEBUG：配合源码编译的 Debug Python 包（python37_d.lib / python37_d.dll）。
# 不定义时 pybind11（detail/common.h 的 PYBIND11_DEBUG_MARKER 逻辑）会在 Debug 下临时
# #undef _DEBUG 包含 Python.h，使 pyconfig.h 注入 /DEFAULTLIB:"python37.lib"，
# 而 Debug Python 包中没有该库，链接报 LNK1104；定义后 pybind11 跳过 undef，
# pyconfig.h 走 _DEBUG 分支（python37_d.lib + Py_DEBUG 调试 ABI），全工程 Python ABI 一致。
add_compile_definitions($<$<CONFIG:Debug>:Py_DEBUG>)

# QtADS (Qt Advanced Docking System, Conan 2)
# 配方导出 cmake_file_name=QtAds，导出 target 为 QtAds::QtAds
find_package(QtAds REQUIRED)
if(TARGET QtAds::QtAds AND NOT TARGET LayoutSystem)
	add_library(LayoutSystem ALIAS QtAds::QtAds)
endif()

# Geometry（GeoServer + 几何后端；MXGeoCommand 已迁入 BasicData，由 FindBasicData 自动提供）
find_package(Geometry REQUIRED)
#【后端感知·格式清单】按 RebuildGeometry install 导出的后端路由结果
# （GEOENGINE_SELECTED_BACKEND_KIND，来自其 CMake 的 GEOENGINE_GEOMETRY_BACKEND
# 选项）发放编译宏：MXGEO_BACKEND_CGM=1 / MXGEO_BACKEND_OCC=1。导入几何模型
# 的文件格式清单随后端切换（OCC：STEP/IGES/BREP/IDF；CGM：九种 CAD 直读，
# 引擎端见 mCgmDataExchangeAdapter）。路由片段缺失或后端为 POISSON 时按 OCC
# 清单回退（与 FindGeometry 的回退语义一致）。
#【OCC+商软导入】OCC 后端下 CGM_VERSION≠None（HAS_CGM_INTEROP，见文件头选项
# 块）时清单追加九种商用 CAD——convertToStepIfNeeded 经 CGMDataExchange
# （CAD→STEP）转换后交 OCC 导入；需 RebuildGeometry 以相同 CGM_VERSION 构建
# （install 树须含 CGMDataExchange 库，见下方 CGMInterOP + CGMExchange 块）。
if(GEOENGINE_SELECTED_BACKEND_KIND STREQUAL "CGM")
	add_compile_definitions(MXGEO_BACKEND_CGM=1)
	message(STATUS "Geometry backend: CGM —— 导入格式清单启用商用 CAD 直读（STEP/IGES/CATIA V5/Inventor/SolidWorks/Parasolid/SolidEdge/NX/Creo）")
else()
	add_compile_definitions(MXGEO_BACKEND_OCC=1)
	if(NOT CGM_VERSION STREQUAL "None")
		message(STATUS "Geometry backend: ${GEOENGINE_SELECTED_BACKEND_KIND} —— 导入格式清单为 OCC 能力（STEP/IGES/BREP/IDF）+ CGM 商软转换（CGM_VERSION=${CGM_VERSION}：CATIA V5/Inventor/SolidWorks/Parasolid/SolidEdge/NX/Creo 经 CGMDataExchange 转 STEP 后导入）")
	else()
		message(STATUS "Geometry backend: ${GEOENGINE_SELECTED_BACKEND_KIND} —— 导入格式清单按 OCC 能力（STEP/IGES/BREP/IDF）")
	endif()
endif()
#【OCC摘除·S10/P0-1】引擎中性头（句柄/服务视图/契约）全目标可见——mViewportModelManager.h
# 的 getGEOServer 宏随宏发放 mGeoEngineServices.h，未直链 GeoServer 的目标（如 PostCurve）
# 也需解析；纯中性头（零 OCC），不影响摘链门禁。
include_directories(${Geometry_INC_DIR} ${GeoData_INC_DIR})

# PchReader（静态库，消费兄弟模块 install 树，走 Find 模块）
unset(PchReader_DIR CACHE)
find_package(PchReader REQUIRED)

# MxNativeReaders（静态库：OP2/ANSYS RST·RTH/d3plot/H3D/ODB 原生读取，
# 消费兄弟模块 install 树，走 Find 模块；其 PUBLIC 依赖 zlib 由 Find 模块内解析）
unset(MxNativeReaders_DIR CACHE)
find_package(MxNativeReaders REQUIRED)

# VTKFileReader（静态库 vfr::vfr：VTK legacy/XML/parallel 后处理文件读取，
# 消费兄弟模块 install 树；库自带 CONFIG 包（lib/cmake/vfr），无第三方依赖）
set(_VTKFileReader_INSTALL_DIR "${CMAKE_SOURCE_DIR}/../../../Basic/VTKFileReader/install")
if(EXISTS "${_VTKFileReader_INSTALL_DIR}" AND NOT "${_VTKFileReader_INSTALL_DIR}" IN_LIST CMAKE_PREFIX_PATH)
    list(APPEND CMAKE_PREFIX_PATH "${_VTKFileReader_INSTALL_DIR}")
endif()
unset(_VTKFileReader_INSTALL_DIR)
unset(vfr_DIR CACHE)
find_package(vfr CONFIG REQUIRED)

# TBB (Conan 2)
find_package(TBB REQUIRED)

# CGMInterOP + CGMExchange（【CGM迁移·20261019】交换库现居 RebuildGeometry install，
# 此处查找 cgminterop 配置包并以导入目标消费 Geometry 产出的 CGMDataExchange 库；
# 头文件经 Geometry_INC_DIR 全局包含路径解析，公共头零 CGM SDK 依赖）
#【Release 支持】Debug（CGMDataExchanged.lib）/Release（CGMDataExchange.lib）导入库
# 按 install 树实际存在的文件分别登记：任一存在即建目标，只登记存在的配置，
# 缺失配置由 CMake 回退到已登记变体（IMPORTED_CONFIGURATIONS）。
# 此前仅凭 Debug 库存在才建目标、且无条件写全四配置路径，Release-only
# install 树下目标缺失（LNK2019）、或指向不存在的 CGMDataExchange.lib（LNK1104）。
if(NOT CGM_VERSION STREQUAL "None")
	find_package(cgminterop REQUIRED)
	if(cgminterop_FOUND)
		set(CGMInterOP_FOUND TRUE)
		get_target_property(_CGM_INCLUDE_DIRS cgminterop::cgminterop INTERFACE_INCLUDE_DIRECTORIES)
		if(_CGM_INCLUDE_DIRS)
			set(CGMInterOP_INCLUDE_DIRS ${_CGM_INCLUDE_DIRS})
		endif()
		set(CGMInterOP_LIBRARIES cgminterop::cgminterop)
		set(_CGMExchange_DBG_LIB "${Geometry_LIB_DIR}/CGMDataExchanged.lib")
		set(_CGMExchange_REL_LIB "${Geometry_LIB_DIR}/CGMDataExchange.lib")
		if(NOT TARGET CGMDataExchange AND (EXISTS "${_CGMExchange_DBG_LIB}" OR EXISTS "${_CGMExchange_REL_LIB}"))
			add_library(CGMDataExchange UNKNOWN IMPORTED GLOBAL)
			set_target_properties(CGMDataExchange PROPERTIES
				INTERFACE_INCLUDE_DIRECTORIES "${Geometry_INC_DIR}")
			set(_CGMExchange_CFGS "")
			if(EXISTS "${_CGMExchange_DBG_LIB}")
				list(APPEND _CGMExchange_CFGS DEBUG)
				#【注意】UNKNOWN IMPORTED 目标链接用的是 IMPORTED_LOCATION（IMPLIB 不被采纳），
				# 故 LOCATION 必须指导入库 .lib；指向 .dll 会把 DLL 送上链接线（LNK1107）
				set_target_properties(CGMDataExchange PROPERTIES
					IMPORTED_LOCATION_DEBUG "${_CGMExchange_DBG_LIB}"
					IMPORTED_IMPLIB_DEBUG   "${_CGMExchange_DBG_LIB}")
			endif()
			if(EXISTS "${_CGMExchange_REL_LIB}")
				list(APPEND _CGMExchange_CFGS RELEASE RELWITHDEBINFO MINSIZEREL)
				set_target_properties(CGMDataExchange PROPERTIES
					IMPORTED_LOCATION_RELEASE        "${_CGMExchange_REL_LIB}"
					IMPORTED_IMPLIB_RELEASE          "${_CGMExchange_REL_LIB}"
					IMPORTED_LOCATION_RELWITHDEBINFO "${_CGMExchange_REL_LIB}"
					IMPORTED_IMPLIB_RELWITHDEBINFO   "${_CGMExchange_REL_LIB}"
					IMPORTED_LOCATION_MINSIZEREL     "${_CGMExchange_REL_LIB}"
					IMPORTED_IMPLIB_MINSIZEREL       "${_CGMExchange_REL_LIB}")
			endif()
			set_target_properties(CGMDataExchange PROPERTIES
				IMPORTED_CONFIGURATIONS "${_CGMExchange_CFGS}")
			message(STATUS "CGMExchange: 自 RebuildGeometry install 导入 CGMDataExchange（可用配置：${_CGMExchange_CFGS}，缺失配置回退已有变体）")
			unset(_CGMExchange_CFGS)
		elseif(NOT TARGET CGMDataExchange)
			message(WARNING "CGMExchange: ${Geometry_LIB_DIR} 下无 CGMDataExchanged.lib/CGMDataExchange.lib——请先以 CGM_VERSION=${CGM_VERSION} 构建 RebuildGeometry（Debug/Release 任一）")
		endif()
		unset(_CGMExchange_DBG_LIB)
		unset(_CGMExchange_REL_LIB)
	endif()
endif()

# 深思数盾授权 SDK（Conan 2，仅 Windows；Win7/Win10 通用一套预编译二进制）
if(WIN32)
	find_package(licenseshensi REQUIRED)
	if(TARGET licenseshensi::licenseshensi AND NOT TARGET licenseshensi)
		add_library(licenseshensi ALIAS licenseshensi::licenseshensi)
	endif()
endif()
#-----------------------------------------------------------------------------
# 开启项目分组
#-----------------------------------------------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#-----------------------------------------------------------------------------
# 定义GNU标准的安装目录（GNUInstallDirs.cmake）
# 提供GNU编码标准定义的安装目录变量
#-----------------------------------------------------------------------------
include(GNUInstallDirs)
set(CMAKE_INSTALL_LIBDIR "lib")

#-----------------------------------------------------------------------------
# 定义项目构建中间文件的生成目录
#-----------------------------------------------------------------------------
if(MxPre_WIN)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/RelWithDebInfo)
	if(MxSim_ENABLE_DEV)
		set(DEVRUNTIME_BINDIR_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
		set(DEVRUNTIME_BINDIR_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
		set(DEVRUNTIME_BINDIR_RELWITHDEBINFO ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO})
	endif()
elseif(MxPre_LINUX)  
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})
endif()



#-----------------------------------------------------------------------------
# 定义变量作为项目的安装路径
#-----------------------------------------------------------------------------
set(INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(INSTALL_INCDIR ${CMAKE_INSTALL_BINDIR}/../include)

#-----------------------------------------------------------------------------
# 编译位置无关的代码
#-----------------------------------------------------------------------------
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#-----------------------------------------------------------------------------
# 回归测试（ctest）
#   配置：cmake --preset conan-default（MxSim_ENABLE_TESTS=OFF 可裁掉测试目标）
#   构建：cmake --build --preset conan-debug
#   运行：ctest --preset conan-debug
#-----------------------------------------------------------------------------
option(MxSim_ENABLE_TESTS "ON:构建回归测试目标并注册 ctest" ON)
if(MxSim_ENABLE_TESTS)
    enable_testing()
    # ctest 启动测试进程时的 DLL 搜索路径：
    #   1) Qt/OSG 等 Conan 运行库（preset 的 CONAN_RUNTIME_PATHS_DEBUG/RELEASE，
    #      configure 期烘焙；Release 列表含 Release-only FAT 包（qwt/opens 等），
    #      缺它则 Release 配置跑测试报 0xc0000135 缺 DLL）
    #   2) 自研基础库 DLL（Basic/*/install/bin，Release/Debug 同目录并存）
    set(_mxtest_path_list "")
    if(DEFINED ENV{CONAN_RUNTIME_PATHS_DEBUG})
        list(APPEND _mxtest_path_list "$ENV{CONAN_RUNTIME_PATHS_DEBUG}")
    endif()
    if(DEFINED ENV{CONAN_RUNTIME_PATHS_RELEASE})
        list(APPEND _mxtest_path_list "$ENV{CONAN_RUNTIME_PATHS_RELEASE}")
    endif()
    file(GLOB _basic_install_bins LIST_DIRECTORIES true "${CMAKE_SOURCE_DIR}/../../../Basic/*/install/bin")
    foreach(_bin ${_basic_install_bins})
        if(IS_DIRECTORY "${_bin}")
            list(APPEND _mxtest_path_list "${_bin}")
        endif()
    endforeach()
    list(JOIN _mxtest_path_list ";" MxTest_RUNTIME_PATH)
    set(MxTest_RUNTIME_PATH "${MxTest_RUNTIME_PATH}" CACHE INTERNAL
        "ctest 测试进程的 PATH 前缀（Conan 运行库 + 自研基础库目录）")
    unset(_mxtest_path_list)
    unset(_basic_install_bins)
endif()

# 定义源码路径
add_subdirectory(src)

# VS 解决方案默认启动项目=主程序（打开 bin/MxMech.sln 后 F5/本地 Windows 调试器
# 直接可跑；调试环境 PATH 与 Qt 插件路径已由 src/MxMech 的 VS_DEBUGGER_ENVIRONMENT
# 烘焙，bin/Debug 为调试产物目录——用户实机验证经 VS 调试器时无需先同步 install/bin）
if(CMAKE_GENERATOR MATCHES "Visual Studio" AND TARGET MxMech)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT MxMech)
endif()

# 回归测试目录在 src 之后（测试目标链接各模块库）
if(MxSim_ENABLE_TESTS)
    if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
        add_subdirectory(tests)
    else()
        message(STATUS "MxSim_ENABLE_TESTS=ON 但 tests/ 目录不存在，跳过回归测试目标")
    endif()
endif()
