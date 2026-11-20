#-----------------------------------------------------------------------------
# 引入PlotView基础模块
#-----------------------------------------------------------------------------

#防止重复引入
if(PLOTVIEW_ALREADY_INCLUDED)
	return()
endif()
set(PLOTVIEW_ALREADY_INCLUDED 1)

#设置路径
get_filename_component(PlotView_Dir "${CMAKE_SOURCE_DIR}/../../../Basic/PlotView/install" ABSOLUTE)

set(PlotView_INC_DIR "${PlotView_Dir}/include")
set(PlotView_BIN_DIR "${PlotView_Dir}/bin")
set(PlotView_LIB_DIR "${PlotView_Dir}/lib")

add_library(PlotView SHARED IMPORTED)
set_property(TARGET PlotView PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${PlotView_INC_DIR})
set_property(TARGET PlotView APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)

if(WIN32)
set_target_properties(PlotView PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${PlotView_LIB_DIR}/PlotView.lib"
		IMPORTED_IMPLIB_DEBUG "${PlotView_LIB_DIR}/PlotViewd.lib"
		IMPORTED_LOCATION_RELEASE "${PlotView_BIN_DIR}/PlotView.dll"
		IMPORTED_LOCATION_DEBUG "${PlotView_BIN_DIR}/PlotViewd.dll"
	)
else()
set_target_properties(PlotView PROPERTIES
		IMPORTED_LOCATION_RELEASE "${PlotView_LIB_DIR}/libPlotView.so"
		IMPORTED_LOCATION_DEBUG "${PlotView_LIB_DIR}/libPlotViewd.so"
		LINK_FLAGS "-Wl,-rpath,${PlotView_LIB_DIR}/"
	)
endif()