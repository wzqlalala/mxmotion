#-----------------------------------------------------------------------------
# 引入GeoData基础模块（GeoNeutralData、EventDispatcher）
# 这些模块已从 RebuildBasicData 迁移到独立的 RebuildGeoData 工程，
# 本工程通过预编译导入库的方式依赖它们。
#-----------------------------------------------------------------------------

#防止重复引入
if(GEODATA_ALREADY_INCLUDED)
	return()
endif()
set(GEODATA_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(GeoData_Dir "${CMAKE_SOURCE_DIR}/../RebuildGeoData/install" ABSOLUTE)

set(GeoData_INC_DIR "${GeoData_Dir}/include")
set(GeoData_BIN_DIR "${GeoData_Dir}/bin")
set(GeoData_LIB_DIR "${GeoData_Dir}/lib")

add_library(GeoNeutralData SHARED IMPORTED)
set_property(TARGET GeoNeutralData PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GeoData_INC_DIR})
set_property(TARGET GeoNeutralData APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

add_library(EventDispatcher SHARED IMPORTED)
set_property(TARGET EventDispatcher PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GeoData_INC_DIR})
set_property(TARGET EventDispatcher APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(GeoNeutralData PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${GeoData_LIB_DIR}/GeoNeutralData.lib"
		IMPORTED_LOCATION_RELEASE "${GeoData_BIN_DIR}/GeoNeutralData.dll"
		IMPORTED_IMPLIB_DEBUG "${GeoData_LIB_DIR}/GeoNeutralDatad.lib"
		IMPORTED_LOCATION_DEBUG "${GeoData_BIN_DIR}/GeoNeutralDatad.dll"
	)
set_target_properties(EventDispatcher PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${GeoData_LIB_DIR}/EventDispatcher.lib"
		IMPORTED_LOCATION_RELEASE "${GeoData_BIN_DIR}/EventDispatcher.dll"
		IMPORTED_IMPLIB_DEBUG "${GeoData_LIB_DIR}/EventDispatcherd.lib"
		IMPORTED_LOCATION_DEBUG "${GeoData_BIN_DIR}/EventDispatcherd.dll"
	)
else()
set_target_properties(GeoNeutralData PROPERTIES
		IMPORTED_LOCATION_RELEASE "${GeoData_LIB_DIR}/libGeoNeutralData.so"
		IMPORTED_LOCATION_DEBUG "${GeoData_LIB_DIR}/libGeoNeutralDatad.so"
		LINK_FLAGS "-Wl,-rpath,${GeoData_LIB_DIR}/"
	)
set_target_properties(EventDispatcher PROPERTIES
		IMPORTED_LOCATION_RELEASE "${GeoData_LIB_DIR}/libEventDispatcher.so"
		IMPORTED_LOCATION_DEBUG "${GeoData_LIB_DIR}/libEventDispatcherd.so"
		LINK_FLAGS "-Wl,-rpath,${GeoData_LIB_DIR}/"
	)
endif()
