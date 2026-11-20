

#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)


#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(${CurrentModel}PreData
	${_header}
	${_source}
)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(${CurrentModel}PreData PRIVATE "PREDATA_LIB")

list(APPEND _depend_library  ${CurrentModel}Common OpenThreads)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets MxFunctions PlotView MXGeoManager Qwt #【产品集成】2D 视口(mViewport2DModel)经 mPlotView 拉 qwt 头
	MxPython Qt5::Network Qt5::WebSockets MechModelData MeshData MechMaterialData MechPropertyData MechCaseData MechConnectorData
	MxRenderLib  MXGeoData MechSectionShapeData MechMapRelation CommonData InteractionCore PostData MechCoordinateData MechStructOptionData
    GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(${CurrentModel}PreData PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	#${OpenCASCADE_LIBRARIES}  #【OCC摘除·S10】GeoWidget 出列+截面形状窗口下线后摘除；
	#若报缺 NCollection.hxx：系 mModelData.h 头网（P0-1）传递所致，修 P0-1 而非恢复本链接
)

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
list(APPEND MxPre_Runtimes_Libraries ${_runtimes_libraries})
set(MxPre_Runtimes_Libraries ${MxPre_Runtimes_Libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
	FILES ${_header}
	DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录

)




