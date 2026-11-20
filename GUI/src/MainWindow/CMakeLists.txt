#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PluginManager)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../GUIFoundation)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../IOFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostIOFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCommon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCurve)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostWindow)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../APIServer)

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 添加资源文件
#-----------------------------------------------------------------------------
set(_qrc "${CMAKE_CURRENT_SOURCE_DIR}/../../resource/mMainWidgets.qrc")
qt5_add_resources(_resource ${_qrc})

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _ui "*.ui")
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")
#【OCC摘除·引擎化·Motion】mMotionSurfaceBoundary（点在面上副边界墙算法）整体迁入
# RebuildGeometry 求值服务（evaluation().faceBoundaryWalls，方案 E-37），算法语义
# 逐行等价；本文件纯 OCC 直引（TopoDS/BRepTools/WireExplorer/GCPnts）不再有
# GUI 侧消费者，退役出构建清单（源文件保档，恢复=删掉本行）。
list(REMOVE_ITEM _source "${CMAKE_CURRENT_SOURCE_DIR}/mMotionSurfaceBoundary.cpp")
# 批量剖分 CLI 源文件
list(APPEND _source mMeshBatchCLI.cpp)
# 新加入的面板源文件，避免 GLOB 缓存未更新导致链接失败
list(APPEND _source mEdgeSuppressPanel.cpp)
#（dev_base 侧已删 mGeoCommandDirectStubs.cpp、mPostNotesPanel 迁 PostWindow，
# 对应的 REMOVE/APPEND 条目随其空块一并取消）
qt5_wrap_ui(_interface ${_ui})

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(${CurrentModel}MainWindow
	${_resource}
	${_interface}
	${_header}
	${_source}
)


#（单独设置 C++17）
target_compile_features(${CurrentModel}MainWindow PRIVATE cxx_std_17)
# 或者用下面这种写法：
# set_target_properties(${CurrentModel}MainWindow PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(${CurrentModel}MainWindow PRIVATE "MMAINWINDOW_LIB")
target_compile_definitions(${CurrentModel}MainWindow PRIVATE MXGEOCOMMAND_LOCAL=1)  #【OCC摘除·S9】直模头本地化（DLL 退役，类内 default 特殊成员内联）

list(APPEND _depend_library ${CurrentModel}Common ${CurrentModel}IOFile ${CurrentModel}PreData PostIOFile PostCommon PostCurve APIServer GUIFoundation CommonWidgets)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets  Qt5::Sql Qt5::Network Qt5::WebSockets Qt5::WebEngine Qt5::WebEngineWidgets MxFunctions PlotView
	MechModelData Qwt muparser AssemblyData MechMaterialData MeshData MxRenderLib LayoutSystem MXGeoManager Tasking MechSetData MechCoordinateData MechFunctionData MechCaseData MechPropertyData MechMapRelation ffmpeg  #【OCC摘除·S9】摘 MXGeoCommand（符号本地桩）与 MXGeoDataExchange（Tasking 中性模块替代）
	MechSectionShapeData MeshPanelData MechMapRelation MXGeoData MeshGeoData PostData MechElementData MeshEngine MeshGUIData MeMeshProcess MeshLib MechConnectorData CommonData MechContactData InteractionCore
    MechStructOptionData MechGlobalParamData MechDiscreteFieldData MechModelFixData MeSurfaceWrap MechSolveTaskData PostWindow
#（dev_base 侧此列表用无前缀旧 target 名：其构建链 Basic 仓 target 未带 Mech 前缀；
# 本工作区 pin 的 Basic 仓导出 Mech 前缀 target（add_library(MechModelData...)），取本侧）
    GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
	FieldTransferCore::FieldTransferCore
	FieldTransferMapping::FieldTransferMapping
	MechFrpBaseData
	MechMotionData
	MotionEngine::MotionEngine
	MotionEngine::Modal
)

target_link_libraries(MechMainWindow PRIVATE pybind11::headers MxPython)
#pybind11头文件路径显式补充：conan生成的pybind11目标把include包在$<CONFIG:Debug>生成器
#表达式里（pybind11-Target-debug.cmake），Release配置拿不到，导致MxOptim.h包含
#pybind11/embed.h失败；头文件本身与配置无关，直接取Debug变量里的实际路径无条件加入
if(pybind11_pybind11_headers_INCLUDE_DIRS_DEBUG)
	include_directories(${pybind11_pybind11_headers_INCLUDE_DIRS_DEBUG})
endif()
if(CGMInterOP_FOUND)
	list(APPEND _runtimes_libraries ${CGMInterOP_LIBRARIES})
	if(TARGET CGMDataExchange)
		list(APPEND _runtimes_libraries CGMDataExchange)
	endif()
endif()

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(${CurrentModel}MainWindow PRIVATE
	GUIFoundation             # 面板基座/dock基座/表格/曲线窗等共享UI基础设施
	${_runtimes_libraries}
	${_depend_library}
	#${OpenCASCADE_LIBRARIES}  #【OCC摘除·S10/P0-1 终局3·实验】
    PchReader::PchReader
)



#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(${CurrentModel}MainWindow ${_depend_library})

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
        DESTINATION ${INSTALL_INCDIR}                        #将头文件安装到install/include目录
)