#-----------------------------------------------------------------------------
# 引入 RebuildGeometry 中性适配层（GeoAdapter）
# 架构：消费方 → GeoAdapter →（由 RebuildGeometry 导出的后端选择路由）→ 具体适配器
#       （OCCAdapter/CGMAdapter/PoissonAdapter）。
# 铁律：消费方 CMake 禁止直接引用具体后端目标名（如 OCCAdapter）——切换后端时
#       只重装 RebuildGeometry，本模块自动路由，消费方零改动。
#-----------------------------------------------------------------------------

#防止重复引入
if(GEOMETRY_ALREADY_INCLUDED)
	return()
endif()
set(GEOMETRY_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(Geometry_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/RebuildGeometry/install" ABSOLUTE)

set(Geometry_INC_DIR "${Geometry_Dir}/include")
set(Geometry_BIN_DIR "${Geometry_Dir}/bin")
set(Geometry_LIB_DIR "${Geometry_Dir}/lib")

# 后端选择：优先采用 RebuildGeometry 安装导出的路由片段
# （install/lib/cmake/GeoEngine/mGeoEngineBackend.cmake）；缺失时回退 OCC。
set(GEOENGINE_SELECTED_BACKEND_KIND "OCC")
set(GEOENGINE_SELECTED_BACKEND_TARGET "OCCAdapter")
include("${Geometry_LIB_DIR}/cmake/GeoEngine/mGeoEngineBackend.cmake" OPTIONAL)

# GeoServer：中性模板门面库（句柄/服务契约头随之传递）
if(NOT TARGET GeoServer)
	add_library(GeoServer SHARED IMPORTED)
	set_property(TARGET GeoServer PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Geometry_INC_DIR})
	set_property(TARGET GeoServer APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	if(WIN32)
		set_target_properties(GeoServer PROPERTIES
			IMPORTED_IMPLIB_RELEASE "${Geometry_LIB_DIR}/GeoServer.lib"
			IMPORTED_IMPLIB_DEBUG "${Geometry_LIB_DIR}/GeoServerd.lib"
			IMPORTED_LOCATION_RELEASE "${Geometry_BIN_DIR}/GeoServer.dll"
			IMPORTED_LOCATION_DEBUG "${Geometry_BIN_DIR}/GeoServerd.dll"
		)
	else()
		set_target_properties(GeoServer PROPERTIES
			IMPORTED_LOCATION_RELEASE "${Geometry_LIB_DIR}/libGeoServer.so"
			IMPORTED_LOCATION_DEBUG "${Geometry_LIB_DIR}/libGeoServerd.so"
		)
	endif()
endif()

# 具体后端目标：仅作为 GeoAdapter 的路由落点（消费方不要直接链接它）
if(NOT TARGET ${GEOENGINE_SELECTED_BACKEND_TARGET})
	add_library(${GEOENGINE_SELECTED_BACKEND_TARGET} SHARED IMPORTED)
	set_property(TARGET ${GEOENGINE_SELECTED_BACKEND_TARGET} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Geometry_INC_DIR})
	set_property(TARGET ${GEOENGINE_SELECTED_BACKEND_TARGET} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	if(WIN32)
		set_target_properties(${GEOENGINE_SELECTED_BACKEND_TARGET} PROPERTIES
			IMPORTED_IMPLIB_RELEASE "${Geometry_LIB_DIR}/${GEOENGINE_SELECTED_BACKEND_TARGET}.lib"
			IMPORTED_IMPLIB_DEBUG "${Geometry_LIB_DIR}/${GEOENGINE_SELECTED_BACKEND_TARGET}d.lib"
			IMPORTED_LOCATION_RELEASE "${Geometry_BIN_DIR}/${GEOENGINE_SELECTED_BACKEND_TARGET}.dll"
			IMPORTED_LOCATION_DEBUG "${Geometry_BIN_DIR}/${GEOENGINE_SELECTED_BACKEND_TARGET}d.dll"
		)
	else()
		set_target_properties(${GEOENGINE_SELECTED_BACKEND_TARGET} PROPERTIES
			IMPORTED_LOCATION_RELEASE "${Geometry_LIB_DIR}/lib${GEOENGINE_SELECTED_BACKEND_TARGET}.so"
			IMPORTED_LOCATION_DEBUG "${Geometry_LIB_DIR}/lib${GEOENGINE_SELECTED_BACKEND_TARGET}d.so"
		)
	endif()
endif()

# GeoAdapter：消费方唯一允许链接的适配层入口。
# INTERFACE 指向当前所选后端——链接 GeoAdapter 即传递获得具体适配器的导入库
# 与运行时加载（后端切换零感知）。
if(NOT TARGET GeoAdapter)
	add_library(GeoAdapter SHARED IMPORTED)
	set_property(TARGET GeoAdapter PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Geometry_INC_DIR})
	set_property(TARGET GeoAdapter APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	if(WIN32)
		set_target_properties(GeoAdapter PROPERTIES
			IMPORTED_IMPLIB_RELEASE "${Geometry_LIB_DIR}/GeoAdapter.lib"
			IMPORTED_IMPLIB_DEBUG "${Geometry_LIB_DIR}/GeoAdapterd.lib"
			IMPORTED_LOCATION_RELEASE "${Geometry_BIN_DIR}/GeoAdapter.dll"
			IMPORTED_LOCATION_DEBUG "${Geometry_BIN_DIR}/GeoAdapterd.dll"
			INTERFACE_LINK_LIBRARIES "${GEOENGINE_SELECTED_BACKEND_TARGET}"
		)
	else()
		set_target_properties(GeoAdapter PROPERTIES
			IMPORTED_LOCATION_RELEASE "${Geometry_LIB_DIR}/libGeoAdapter.so"
			IMPORTED_LOCATION_DEBUG "${Geometry_LIB_DIR}/libGeoAdapterd.so"
			INTERFACE_LINK_LIBRARIES "${GEOENGINE_SELECTED_BACKEND_TARGET}"
		)
	endif()
endif()
