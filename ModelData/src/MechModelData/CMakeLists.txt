#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
# 获取上一级目录
get_filename_component(PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)

# GLOB出所有内容（仅名称）
file(GLOB entries RELATIVE "${PARENT_DIR}" "${PARENT_DIR}/*")

set(_depend_library "")
foreach(entry ${entries})
    if(IS_DIRECTORY "${PARENT_DIR}/${entry}")
#		message(STATUS ${CMAKE_CURRENT_SOURCE_DIR}/../${entry})
		include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../${entry})
		if(NOT "${entry}" STREQUAL "Main" AND NOT "${entry}" STREQUAL "MechModelData")
			list(APPEND _depend_library "${entry}")
		endif()
    endif()
endforeach()

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
add_library(MechModelData
	${_header}
	${_source}
)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(MechModelData PRIVATE "MODELDATA_LIB")

list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets MxFunctions CommonData MeshData MeshGeoData MeshPanelData MXGeoData MXGeoManager PostData  #【OCC摘除·S9】MXGeoDataExchange 已退役（lib 已清理，ModelData 不再链接）
	
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
target_link_libraries(MechModelData PUBLIC
	${_runtimes_libraries}
	${_depend_library}
	GeoAdapter  #【后端可切换】中性适配层入口（原直引 OCCAdapter 违规；源码零 OCC 判据不变）
)

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
#set(ModelData_Runtimes_Libraries ${_runtimes_libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
	FILES ${_header}
	DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)