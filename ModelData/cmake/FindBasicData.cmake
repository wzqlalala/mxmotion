#-----------------------------------------------------------------------------
# 引入BasicData基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(BASICDATA_ALREADY_INCLUDED)
	return()
endif()
set(BASICDATA_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(BasicData_Dir "${CMAKE_SOURCE_DIR}/../RebuildBasicData/install" ABSOLUTE)

set(BasicData_INC_DIR "${BasicData_Dir}/include")
set(BasicData_BIN_DIR "${BasicData_Dir}/bin")
set(BasicData_LIB_DIR "${BasicData_Dir}/lib")

# 1. 获取指定目录下所有子目录
file(GLOB ALL "${CMAKE_SOURCE_DIR}/../RebuildBasicData/src/*")
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
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${BasicData_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(${_library} PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${BasicData_LIB_DIR}/${_library}.lib"
		IMPORTED_LOCATION_RELEASE "${BasicData_BIN_DIR}/${_library}.dll"
		IMPORTED_IMPLIB_DEBUG "${BasicData_LIB_DIR}/${_library}d.lib"
		IMPORTED_LOCATION_DEBUG "${BasicData_BIN_DIR}/${_library}d.dll"
	)
endforeach()

else()

foreach(_library ${filtered_subdirs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${BasicData_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	set_target_properties(${_library} PROPERTIES
		IMPORTED_LOCATION_RELEASE "${BasicData_LIB_DIR}/lib${_library}.so"
		IMPORTED_LOCATION_DEBUG "${BasicData_LIB_DIR}/lib${_library}d.so"
	)
endforeach()

endif()