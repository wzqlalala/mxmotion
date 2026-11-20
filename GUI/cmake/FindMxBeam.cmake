#-----------------------------------------------------------------------------
# 引入MxBeam基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(MXBEAM_ALREADY_INCLUDED)
	return()
endif()
set(MXBEAM_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(MxBeam_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/Beam/install" ABSOLUTE)

set(MxBeam_INC_DIR "${MxBeam_Dir}/include")
set(MxBeam_BIN_DIR "${MxBeam_Dir}/bin")
set(MxBeam_LIB_DIR "${MxBeam_Dir}/lib")

add_library(BeamWindow SHARED IMPORTED)
set_property(TARGET BeamWindow PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MxBeam_INC_DIR})
set_property(TARGET BeamWindow APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(BeamWindow PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${MxBeam_LIB_DIR}/BeamWindow.lib"
		IMPORTED_LOCATION_RELEASE "${MxBeam_BIN_DIR}/BeamWindow.dll"
		IMPORTED_IMPLIB_DEBUG "${MxBeam_LIB_DIR}/BeamWindowd.lib"
		IMPORTED_LOCATION_DEBUG "${MxBeam_BIN_DIR}/BeamWindowd.dll"
	)
else()
set_target_properties(BeamWindow PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MxBeam_LIB_DIR}/libBeamWindow.so"
		IMPORTED_LOCATION_DEBUG "${MxBeam_LIB_DIR}/libBeamWindowd.so"
		LINK_FLAGS "-Wl,-rpath,${MxBeam_LIB_DIR}/"		
	)
endif()