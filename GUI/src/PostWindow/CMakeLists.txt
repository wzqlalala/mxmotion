#-----------------------------------------------------------------------------
# PostWindow：后处理UI模块（自 MainWindow 增量迁入，与 PostCurve/PostCommon 平级）
# 链接方向（断链完成）：PostWindow -> Common/PreData/PostCommon/PostIOFile/PostCurve/GUIFoundation/CommonWidgets，
# 不再链接 MainWindow（共享UI基础设施居GUIFoundation，轻量圆角控件居CommonWidgets；Info dock经IPanelHost接口；
# 共享树模型经Common提供者）——隔离守卫见 scripts/check_postwindow_isolation.sh
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../GUIFoundation)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCommon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostIOFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCurve)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../IOFile)
#后处理自动化测试 API 注册器（mPostAutoTestAPIRegistrar）需 APIServer 调度器接口
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../APIServer)

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 添加资源文件（菜单图标 :/mMainWidgets/Icon/post/** 等）
#-----------------------------------------------------------------------------
set(_qrc "${CMAKE_CURRENT_SOURCE_DIR}/../../resource/mMainWidgets.qrc")
qt5_add_resources(_resource ${_qrc})

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _ui "*.ui")
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")
#派生工况公共数据层子目录（2026-11-12 批次A0；GLOB 不递归须单列）
file(GLOB _derived_header "DerivedCase/*.h")
file(GLOB _derived_source "DerivedCase/*.cpp")
list(APPEND _header ${_derived_header})
list(APPEND _source ${_derived_source})
qt5_wrap_ui(_interface ${_ui})

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(PostWindow
	${_resource}
	${_interface}
	${_header}
	${_source}
)

target_compile_features(PostWindow PRIVATE cxx_std_17)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(PostWindow PRIVATE "MPOSTWINDOW_LIB")

list(APPEND _depend_library ${CurrentModel}Common ${CurrentModel}PreData ${CurrentModel}IOFile PostCommon PostIOFile PostCurve GUIFoundation CommonWidgets)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::WebEngineWidgets MxFunctions MechModelData CommonData MeshData PostData MxRenderLib PlotView Qwt ffmpeg  # ModelData->MechModelData：本工作区 Basic 仓 target 带 Mech 前缀
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(PostWindow PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	#【OCC摘除·集成】头网根除后 OCC 链接摘除；mPostMeasureTool 测量已切引擎句柄
	#GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
	GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
	PchReader::PchReader     # PostCurve公共头（mOpenCurveDlg.h等）引用PchReader头
	InteractionCore          # 标注族面板（mPostArrow/Line/Log/Text）用 mxr::Interaction 注释工具
	MXGeoManager             # mPostMeasureTool 用 MGeoManager::GeoManager 测量（体积/面积）

	LayoutSystem             # v1.3 实时动画停靠页签（mPost3DResult）直用 ads API（QtAds::QtAds 别名，GUIFoundation 同款）
	APIServer                # mPostAutoTestAPIRegistrar 向调度器注册测试数据抽取 API（目标生成期前向解析，无环）

)

#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(PostWindow ${_depend_library})

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
list(APPEND MxPost_Runtimes_Libraries ${_runtimes_libraries})
set(MxPost_Runtimes_Libraries ${MxPost_Runtimes_Libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
        FILES ${_header}
        DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)
