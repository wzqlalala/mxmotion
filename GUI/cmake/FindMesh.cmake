#-----------------------------------------------------------------------------
# 引入Mesh基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(Mesh_ALREADY_INCLUDED)
	return()
endif()
set(Mesh_ALREADY_INCLUDED 1)

#设置路径
set(Mesh_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/RebuildMesh/install")

set(Mesh_INC_DIR "${Mesh_Dir}/include")
set(Mesh_BIN_DIR "${Mesh_Dir}/bin")
set(Mesh_LIB_DIR "${Mesh_Dir}/lib")

	
list(APPEND MeshDespLibs MeshEngine MeshGUIData MeMeshProcess MeshLib MeSurfaceWrap)


if(WIN32)
foreach(_library ${MeshDespLibs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Mesh_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	
	set_target_properties(${_library} PROPERTIES
	IMPORTED_IMPLIB_RELEASE "${Mesh_LIB_DIR}/${_library}.lib"
	IMPORTED_LOCATION_RELEASE "${Mesh_BIN_DIR}/${_library}.dll"
	IMPORTED_IMPLIB_DEBUG "${Mesh_LIB_DIR}/${_library}d.lib"
	IMPORTED_LOCATION_DEBUG "${Mesh_BIN_DIR}/${_library}d.dll"
)
endforeach()
else()
foreach(_library ${MeshDespLibs})
	add_library(${_library} SHARED IMPORTED)
	set_property(TARGET ${_library} PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Mesh_INC_DIR})
	set_property(TARGET ${_library} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
	
	set_target_properties(${_library} PROPERTIES
	IMPORTED_IMPLIB_RELEASE "${Mesh_LIB_DIR}/lib${_library}.so"
	IMPORTED_LOCATION_RELEASE "${Mesh_LIB_DIR}/lib${_library}.so"
	IMPORTED_IMPLIB_DEBUG "${Mesh_LIB_DIR}/lib${_library}d.so"
	IMPORTED_LOCATION_DEBUG "${Mesh_LIB_DIR}/lib${_library}d.so"
	LINK_FLAGS "-Wl,-rpath,${Mesh_LIB_DIR}/"
)
endforeach()
endif()
