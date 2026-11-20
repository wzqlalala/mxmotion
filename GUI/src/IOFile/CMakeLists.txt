#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)

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
add_library(${CurrentModel}IOFile
	${_header}
	${_source}
)
target_compile_options(${CurrentModel}IOFile PRIVATE 
$<$<PLATFORM_ID:Windows>:/bigobj>
)
#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------s
target_compile_definitions(${CurrentModel}IOFile PRIVATE "MIOFILE_LIB")

list(APPEND _depend_library ${CurrentModel}Common ${CurrentModel}PreData MeshPanelData)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets MxFunctions CommonData MeshData MxRenderLib MXGeoData MeshPanelData MeshGUIData MeshGeoData #【产品集成】GN 面边镜像反查（mOMxFile 拾取展开）
	MechModelData MeshData MechElementData MechMapRelation MechMaterialData MechSetData
    MechFunctionData MechCoordinateData MechCaseData MechPropertyData MechContactData
    MechConnectorData MechSectionShapeData MechStructOptionData MechGlobalParamData MechMotionData
    MechDiscreteFieldData MechModelFixData MechFrpBaseData
    GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
)
target_compile_options(${CurrentModel}IOFile PRIVATE
$<$<PLATFORM_ID:Windows>:/bigobj>
)
#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
# 注意：本模块源码不直接使用 OCC，但 BasicData 公共头 mGeoDataBase.h 的
# QDataStream 流操作符在头文件内联调用 BinTools/TopExp，链接期需要 OCC 库。
target_link_libraries(${CurrentModel}IOFile PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	#【OCC摘除·S10/P0-1】头网已根除（GeoEngineData pimpl 化），链接摘除
	#${OpenCASCADE_LIBRARIES}
	# 构造闭包，链接期需要 TKernel 族符号。消根路径=P0-1/P1 头链卫生（组合头不得被普通导出头拉入），届时此处一并摘除。
	#【后端可切换·头链卫生·探测结论】OCC 链接暂留：门面前向声明化需与 P0-1（getGEOServer 318 处切 handle）同批，届时一并摘除
	#【OCC摘除·S10/P0-1】头网已根除（GeoEngineData pimpl 化），链接摘除
	#${OpenCASCADE_LIBRARIES}
)

#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(${CurrentModel}IOFile ${_depend_library})

# 在Release/RelWithDebInfo解决方案模式下启用OpenMP支持
find_package(OpenMP REQUIRED)
target_compile_options(${CurrentModel}IOFile PRIVATE $<$<CONFIG:Release>:${OpenMP_CXX_FLAGS}>)
target_link_libraries(${CurrentModel}IOFile PRIVATE $<$<CONFIG:Release>:OpenMP::OpenMP_CXX>)
target_compile_options(${CurrentModel}IOFile PRIVATE $<$<CONFIG:RelWithDebInfo>:${OpenMP_CXX_FLAGS}>)
target_link_libraries(${CurrentModel}IOFile PRIVATE $<$<CONFIG:RelWithDebInfo>:OpenMP::OpenMP_CXX>)

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

#【OCC摘除·S11】OCC 头路径引用移除：根级 find_package 已注释（S10 终局4），
# 变量为空的 include 无效果；全 GUI TU 已无 .hxx 可达
#include_directories(${OpenCASCADE_INCLUDE_DIRS})
