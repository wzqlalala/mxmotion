#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

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
# 深思（Virbox）许可 SDK 仅提供 Windows 库：非 Windows 平台不参与编译、安装与链接
if(NOT WIN32)
	list(REMOVE_ITEM _source "${CMAKE_CURRENT_SOURCE_DIR}/LicenseShenSi.cpp")
	list(REMOVE_ITEM _header "${CMAKE_CURRENT_SOURCE_DIR}/LicenseShenSi.h")
endif()

qt5_wrap_ui(_interface ${_ui})

#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(${CurrentModel}Common
	${_resource}
	${_interface}
	${_header}
	${_source}
)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(${CurrentModel}Common PRIVATE "COMMON_LIB")

list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network MxFunctions MxRenderLib ssl crypto
)

# 深思（Virbox）许可相关库仅在 Windows 链接（Linux 未提供 SDK，链接会失败）
if(WIN32)
        list(APPEND _runtimes_libraries licenseshensi::licenseshensi)
endif()

add_definitions("-DUNICODE" "-D_UNICODE")
#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(${CurrentModel}Common PRIVATE
	${_runtimes_libraries}
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