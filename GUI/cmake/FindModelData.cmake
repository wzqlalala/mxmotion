#-----------------------------------------------------------------------------
# 引入ModelData基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(MODELDATA_ALREADY_INCLUDED)
	return()
endif()
set(MODELDATA_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(ModelData_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/RebuildModelData/install" ABSOLUTE)

set(ModelData_INC_DIR "${ModelData_Dir}/include")
set(ModelData_BIN_DIR "${ModelData_Dir}/bin")
set(ModelData_LIB_DIR "${ModelData_Dir}/lib")

# 1. 获取指定目录下所有子目录
file(GLOB ALL "${CMAKE_SOURCE_DIR}/../../../Basic/RebuildModelData/src/*")
set(filtered_subdirs "")

foreach(entry ${ALL})
    if(IS_DIRECTORY ${entry})
        get_filename_component(dirname ${entry} NAME)
		if(NOT "${dirname}" STREQUAL "Main")
        	list(APPEND filtered_subdirs ${dirname})
		endif()
    endif()
endforeach()

if(WIN32)

foreach(_library ${filtered_subdirs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ModelData_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(${_library} PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${ModelData_LIB_DIR}/${_library}.lib"
		IMPORTED_LOCATION_RELEASE "${ModelData_BIN_DIR}/${_library}.dll"
		IMPORTED_IMPLIB_DEBUG "${ModelData_LIB_DIR}/${_library}d.lib"
		IMPORTED_LOCATION_DEBUG "${ModelData_BIN_DIR}/${_library}d.dll"
	)
endforeach()

else()

foreach(_library ${filtered_subdirs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${ModelData_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(${_library} PROPERTIES
		IMPORTED_LOCATION_RELEASE "${ModelData_LIB_DIR}/lib${_library}.so"
		IMPORTED_LOCATION_DEBUG "${ModelData_LIB_DIR}/lib${_library}d.so"
	)
endforeach()

endif()