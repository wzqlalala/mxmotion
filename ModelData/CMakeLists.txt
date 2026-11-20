#-----------------------------------------------------------------------------
# 工程属性设置
#-----------------------------------------------------------------------------
# 指定所需cmake的最低版本
cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
# 设置项目名称 语言
project(ModelData
	VERSION 2.0
	LANGUAGES CXX
)
#【OCC摘除·S11】OCC 版本选择块移除：本仓零 OCC（GeoEngineData pimpl 化收口，
# 宏无源码引用）
#set(OCC_VERSION "OCC7.9.0" CACHE STRING "选择OCC版本")
#set_property(CACHE OCC_VERSION PROPERTY STRINGS "OCC7.9.0" "OCC7.5.0")
#add_definitions(-DOCC_VERSION="${OCC_VERSION}")
#add_definitions(-DOCC_VERSION_STR="${OCC_VERSION}")
#if(OCC_VERSION STREQUAL "OCC7.5.0")
#	add_definitions(-DOCC7_5_0)
#elseif(OCC_VERSION STREQUAL "OCC7.9.0")
#  add_definitions(-DOCC7_9_0)
#endif()
#-----------------------------------------------------------------------------
# 编译系统设置
#-----------------------------------------------------------------------------
# c++语言标准
set(CMAKE_CXX_STANDARD 11)
# 指定是否使用编译器特定的扩展
set(CMAKE_CXX_EXTENSIONS OFF)
# 是否强制要求C++标准为${CMAKE_CXX_STANDARD},如果为OFF则可能使用旧的标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 默认编译动态库
set(BUILD_SHARED_LIBS ON)
# Debug添加后缀d
set(CMAKE_DEBUG_POSTFIX d)

#-----------------------------------------------------------------------------
# 检测操作系统
#-----------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(ModelData_LINUX True)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
	set(ModelData_WIN True)
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
				set(VS_STARTUP_PROJECT ${PROJECT_NAME})
		ENDIF(MSVC)
ENDIF()

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
)
if(ModelData_LINUX)
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
find_package(Qt5 COMPONENTS ${QtNeededModules} REQUIRED)
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


#【OCC摘除·S11】OpenCASCADE 残留依赖移除：GeoEngineData pimpl 化（P0-1）与
# SectionShapeData（P4）链接摘除后，本仓已无任何目标引用 OCC
#find_package(OpenCASCADE REQUIRED)


# MxFunctions
find_package(MxFunctions REQUIRED)

# BasicData
find_package(BasicData REQUIRED)

# GeoData（GeoNeutralData、EventDispatcher 已迁移至 RebuildGeoData）
find_package(GeoData REQUIRED)

# Geometry
find_package(Geometry REQUIRED)

#-----------------------------------------------------------------------------
# 几何引擎后端选择（需与 RebuildGeometry 安装时选择的后端保持一致）
#-----------------------------------------------------------------------------
#【后端可切换】后端选择权归 RebuildGeometry（其 install 导出 mGeoEngineBackend.cmake，
# 由各工程 FindGeometry.cmake include 并自动路由）。本工程一律链接中性入口 GeoAdapter；
# 消费方 CMake 禁止直接引用 OCCAdapter/CGMAdapter/PoissonAdapter 等具体后端目标名。
set(GeoEngineData_BACKEND_ADAPTER GeoAdapter)
message(STATUS "ModelData GeoEngineData 适配层入口: ${GeoEngineData_BACKEND_ADAPTER}（后端由 RebuildGeometry 路由）")

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
if(ModelData_WIN)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/Debug)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/Release)
elseif(ModelData_LINUX)  
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

# 定义源码路径
add_subdirectory(src)