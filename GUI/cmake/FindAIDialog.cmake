#-----------------------------------------------------------------------------
# 引入AIDialog基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(AIDialog_ALREADY_INCLUDED)
	return()
endif()
set(AIDialog_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(AIDialog_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/AIDialog/install" ABSOLUTE)

set(AIDialog_INC_DIR "${AIDialog_Dir}/include")
set(AIDialog_BIN_DIR "${AIDialog_Dir}/bin")
set(AIDialog_LIB_DIR "${AIDialog_Dir}/lib")

add_library(AIDialog SHARED IMPORTED)
set_property(TARGET AIDialog PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${AIDialog_INC_DIR})
set_property(TARGET AIDialog APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)


if(WIN32)
set_target_properties(AIDialog PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${AIDialog_LIB_DIR}/AIPlugin.lib"
		IMPORTED_LOCATION_RELEASE "${AIDialog_BIN_DIR}/AIPlugin.dll"
		IMPORTED_IMPLIB_DEBUG "${AIDialog_LIB_DIR}/AIPlugind.lib"
		IMPORTED_LOCATION_DEBUG "${AIDialog_BIN_DIR}/AIPlugind.dll"
	)
else()
set_target_properties(AIDialog PROPERTIES
		IMPORTED_LOCATION_RELEASE "${AIDialog_LIB_DIR}/libAIDialog.so"
		IMPORTED_LOCATION_DEBUG "${AIDialog_LIB_DIR}/libAIDialogd.so"
		LINK_FLAGS "-Wl,-rpath,${AIDialog_LIB_DIR}/"
	)
endif()