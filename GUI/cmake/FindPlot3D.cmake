#-----------------------------------------------------------------------------
# 引入Plot3D基础模块（3D绘图引擎：Qt3DPlotWidget/Series/MapFileLoader）
#-----------------------------------------------------------------------------

#防止重复引入
if(PLOT3D_ALREADY_INCLUDED)
	return()
endif()
set(PLOT3D_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(Plot3D_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/Plot3D/install" ABSOLUTE)

#头文件安装在 include/opengl 子目录：保持 "#include <Qt3DPlotWidget.h>" 写法
set(Plot3D_INC_DIR "${Plot3D_Dir}/include/opengl")
set(Plot3D_BIN_DIR "${Plot3D_Dir}/bin")
set(Plot3D_LIB_DIR "${Plot3D_Dir}/lib")

add_library(Plot3D SHARED IMPORTED)
set_property(TARGET Plot3D PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${Plot3D_INC_DIR})
set_property(TARGET Plot3D APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(Plot3D PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${Plot3D_LIB_DIR}/Plot3D.lib"
		IMPORTED_IMPLIB_DEBUG "${Plot3D_LIB_DIR}/Plot3Dd.lib"
		IMPORTED_LOCATION_RELEASE "${Plot3D_BIN_DIR}/Plot3D.dll"
		IMPORTED_LOCATION_DEBUG "${Plot3D_BIN_DIR}/Plot3Dd.dll"
	)
else()
set_target_properties(Plot3D PROPERTIES
		IMPORTED_LOCATION_RELEASE "${Plot3D_LIB_DIR}/libPlot3D.so"
		IMPORTED_LOCATION_DEBUG "${Plot3D_LIB_DIR}/libPlot3Dd.so"
		LINK_FLAGS "-Wl,-rpath,${Plot3D_LIB_DIR}/"
	)
endif()
