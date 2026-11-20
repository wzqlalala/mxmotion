#-----------------------------------------------------------------------------
# 引入Shortcut基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(SHORTCUT_ALREADY_INCLUDED)
	return()
endif()
set(SHORTCUT_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(Shortcut_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/Shortcut/install" ABSOLUTE)

set(Shortcut_INC_DIR "${Shortcut_Dir}/include")
set(Shortcut_BIN_DIR "${Shortcut_Dir}/bin")
set(Shortcut_LIB_DIR "${Shortcut_Dir}/lib")

add_library(KeySignals SHARED IMPORTED)
set_property(TARGET KeySignals PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Shortcut_INC_DIR})
set_property(TARGET KeySignals APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

add_library(KeyCustom SHARED IMPORTED)
set_property(TARGET KeyCustom PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Shortcut_INC_DIR})
set_property(TARGET KeyCustom APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(KeySignals PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${Shortcut_LIB_DIR}/KeySignals.lib"
		IMPORTED_LOCATION_RELEASE "${Shortcut_BIN_DIR}/KeySignals.dll"
		IMPORTED_IMPLIB_DEBUG "${Shortcut_LIB_DIR}/KeySignalsd.lib"
		IMPORTED_LOCATION_DEBUG "${Shortcut_BIN_DIR}/KeySignalsd.dll"
	)
set_target_properties(KeyCustom PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${Shortcut_LIB_DIR}/KeyCustom.lib"
		IMPORTED_LOCATION_RELEASE "${Shortcut_BIN_DIR}/KeyCustom.dll"
		IMPORTED_IMPLIB_DEBUG "${Shortcut_LIB_DIR}/KeyCustomd.lib"
		IMPORTED_LOCATION_DEBUG "${Shortcut_BIN_DIR}/KeyCustomd.dll"
	)
else()
set_target_properties(KeySignals PROPERTIES
		IMPORTED_LOCATION_RELEASE "${Shortcut_LIB_DIR}/libKeySignals.so"
		IMPORTED_LOCATION_DEBUG "${Shortcut_LIB_DIR}/libKeySignalsd.so"
		LINK_FLAGS "-Wl,-rpath,${Shortcut_LIB_DIR}/"
	)
set_target_properties(KeyCustom PROPERTIES
		IMPORTED_LOCATION_RELEASE "${Shortcut_LIB_DIR}/libKeyCustom.so"
		IMPORTED_LOCATION_DEBUG "${Shortcut_LIB_DIR}/libKeyCustomd.so"
		LINK_FLAGS "-Wl,-rpath,${Shortcut_LIB_DIR}/"
	)
endif()