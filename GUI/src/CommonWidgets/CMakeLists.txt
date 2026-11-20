#-----------------------------------------------------------------------------
# CommonWidgets：跨模块共享轻量控件库（仅依赖 Qt Widgets）
# 内容：参数栏折叠分组（FoldWidget/FoldSection/PanelLabel/drawArrowIcon，
#       自 GUIFoundation/mPanelBase 下沉，并合入原 PostCurve/mCurveFoldSection
#       的行级显隐/置灰/追加块能力）、圆角 Tooltip/圆角列表
# 链接方向：GUIFoundation 与 PostCurve 链接本库，MainWindow/PostWindow 显式
#       链接——前处理/后处理/曲线三方共用轻量控件与参数栏样式单一来源
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 源码扫描
#-----------------------------------------------------------------------------
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(CommonWidgets
	${_header}
	${_source}
)

target_compile_features(CommonWidgets PRIVATE cxx_std_17)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(CommonWidgets PRIVATE "COMMONWIDGETS_LIB")

#-----------------------------------------------------------------------------
# 头文件路径向链接方传递（GUIFoundation PUBLIC 链接后，MainWindow/PostWindow
# 包含 mPanelBase.h -> mFoldSection.h 无需各自加 include 目录）
#-----------------------------------------------------------------------------
target_include_directories(CommonWidgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#-----------------------------------------------------------------------------
# 链接依赖库
# MxFunctions PUBLIC：mFoldSection.h 引用 UiMetrics/UiPalette（五档尺寸/三主题
# 语义色的单一来源，RebuildFunctions 库），头文件路径随链接方传递
#-----------------------------------------------------------------------------
target_link_libraries(CommonWidgets PUBLIC MxFunctions PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
        FILES ${_header}
        DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)
