#-----------------------------------------------------------------------------
# 引入MxFunctions基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(MXFUNCTIONS_ALREADY_INCLUDED)
	return()
endif()
set(MXFUNCTIONS_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(MxFunctions_Dir "${CMAKE_SOURCE_DIR}/../RebuildFunctions/install" ABSOLUTE)

set(MxFunctions_INC_DIR "${MxFunctions_Dir}/include")
set(MxFunctions_BIN_DIR "${MxFunctions_Dir}/bin")
set(MxFunctions_LIB_DIR "${MxFunctions_Dir}/lib")

add_library(MxFunctions SHARED IMPORTED)
set_property(TARGET MxFunctions PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MxFunctions_INC_DIR})
set_property(TARGET MxFunctions APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(MxFunctions PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${MxFunctions_LIB_DIR}/MxFunctions.lib"
		IMPORTED_IMPLIB_DEBUG "${MxFunctions_LIB_DIR}/MxFunctionsd.lib"
		IMPORTED_LOCATION_RELEASE "${MxFunctions_BIN_DIR}/MxFunctions.dll"
		IMPORTED_LOCATION_DEBUG "${MxFunctions_BIN_DIR}/MxFunctionsd.dll"
	)
else()
set_target_properties(MxFunctions PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxFunctions_LIB_DIR}/libMxFunctions.so"
		IMPORTED_LOCATION_DEBUG "${MxFunctions_LIB_DIR}/libMxFunctionsd.so"
		LINK_FLAGS "-Wl,-rpath,${MxFunctions_LIB_DIR}/"
	)
endif()