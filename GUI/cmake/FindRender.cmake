#-----------------------------------------------------------------------------
# 引入Render基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(Render_ALREADY_INCLUDED)
	return()
endif()
set(Render_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(Render_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/RebuildRender/install" ABSOLUTE)

set(Render_INC_DIR "${Render_Dir}/include")
set(Render_BIN_DIR "${Render_Dir}/bin")
set(Render_LIB_DIR "${Render_Dir}/lib")

list(APPEND RenderLibs MxRenderLib InteractionCore)

foreach(_library ${RenderLibs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Render_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
	set_target_properties(${_library} PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${Render_LIB_DIR}/${_library}.lib"
		IMPORTED_LOCATION_RELEASE "${Render_BIN_DIR}/${_library}.dll"
		IMPORTED_IMPLIB_DEBUG "${Render_LIB_DIR}/${_library}d.lib"
		IMPORTED_LOCATION_DEBUG "${Render_BIN_DIR}/${_library}d.dll"
	)
else()
	set_target_properties(${_library} PROPERTIES
		IMPORTED_LOCATION_RELEASE "${Render_LIB_DIR}/lib${_library}.so"
		IMPORTED_LOCATION_DEBUG "${Render_LIB_DIR}/lib${_library}d.so"
		LINK_FLAGS "-Wl,-rpath,${Render_LIB_DIR}/"
	)
endif()
endforeach()