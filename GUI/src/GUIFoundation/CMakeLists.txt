#-----------------------------------------------------------------------------
# GUIFoundation：共享GUI基础设施（自 MainWindow 析出，前后处理共用）
# 内容：面板基座(mPanelBase)/dock基座(mDockBase)/表格(mTableDataWindow)/
#       曲线窗(mCurveWindow)/绘图件(BarChartScaleDraw/PlotDataCache)/
#       提示(mCustomTooltip)/标题栏(mTitleBar)/可移动标签(mMoveableLabel)
# 链接方向：GUIFoundation -> Common/PreData/Basic；MainWindow 与 PostWindow 均链接本库
# （文件名保持不变，消费方 #include "mPanelBase.h" 等无需改动）
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../CommonWidgets)   #mPanelBase.h 转发包含折叠分组控件（mFoldSection.h）

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _ui "*.ui")
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")
qt5_wrap_ui(_interface ${_ui})

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(GUIFoundation
	${_interface}
	${_header}
	${_source}
)

target_compile_features(GUIFoundation PRIVATE cxx_std_17)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(GUIFoundation PRIVATE "GUIFOUNDATION_LIB")

list(APPEND _depend_library ${CurrentModel}Common ${CurrentModel}PreData)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets MxFunctions PlotView Qwt MxRenderLib MXGeoManager MechModelData MechCaseData MechMapRelation LayoutSystem MeshData
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(GUIFoundation PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	#【OCC摘除·集成】mPanelBase Calculate 族已切 engine().geoCalculate()，OCC 链接
	# 换中性五件套（句柄实现编入后端目标、组合根 engine() 在 GeoEngineData）
	GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
)

#折叠分组控件（FoldWidget/FoldSection）已下沉 CommonWidgets；PUBLIC 链接把
#其头文件路径传递给 MainWindow/PostWindow 等下游（它们经 mPanelBase.h 间接包含）
target_link_libraries(GUIFoundation PUBLIC CommonWidgets)

#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(GUIFoundation ${_depend_library})

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
list(APPEND MxGUI_Runtimes_Libraries ${_runtimes_libraries})
set(MxGUI_Runtimes_Libraries ${MxGUI_Runtimes_Libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
        FILES ${_header}
        DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)
