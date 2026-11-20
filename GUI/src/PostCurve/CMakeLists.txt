#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCommon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CommonWidgets)   #折叠分组控件（mFoldSection.h，参数栏三方共用）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../GUIFoundation)   #树层级虚线导引线（mTreeDelegate.h，与视图树同款）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)   #自绘标题栏对话框基类（mDrawingOptions 改用 mFramelessDialog）

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 添加资源文件（绘图选项对话框使用 :/mMainWidgets/Icon/NVH/ 图标）
#-----------------------------------------------------------------------------
set(_qrc "${CMAKE_CURRENT_SOURCE_DIR}/../../resource/mMainWidgets.qrc")
qt5_add_resources(_resource ${_qrc})

#-----------------------------------------------------------------------------
# 源码扫描（递归子目录：CurveCore/CurveCommands/CurveTree/CurveView/CurveWidgets/CurvePages）
#-----------------------------------------------------------------------------
file(GLOB_RECURSE _header "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE _source "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(PostCurve
	${_resource}
	${_header}
	${_source}
)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(PostCurve PRIVATE "POSTCURVE_LIB")

list(APPEND _depend_library ${CurrentModel}PreData ${CurrentModel}Common)   #Common：mDrawingOptions 自绘标题栏基类 mFramelessDialog
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL MxFunctions PlotView Qwt MechModelData PostData PostCommon MxRenderLib  # ModelData->MechModelData：本工作区 Basic 仓 target 带 Mech 前缀
	#表达式求值（定义曲线-数学运算来源）已改用模块内自研 mCurveMathExpression/mCurveMathFunctions，无第三方依赖
	CommonWidgets   #参数栏折叠分组控件（前处理/后处理/曲线三方样式单一来源）
	GUIFoundation   #曲线树分支虚线（PaintDashBranches，与视图树/前处理树同源）
	Plot3D          #3D绘图引擎（"3D曲线"模块：Qt3DPlotWidget+Series+MapFileLoader，仅本模块链接）
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(PostCurve PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	PchReader::PchReader
)

#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(PostCurve ${_depend_library})

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
set(MxGeo_PostCurve_Runtimes_Libraries ${_runtimes_libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
	FILES ${_header}
	DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)
