



#-----------------------------------------------------------
# 1. 头文件搜索路径（保持原样）
#-----------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------
# 2. 源码扫描（保持原样，但**之后**不要再手动改 GLOB）
#-----------------------------------------------------------
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")

#-----------------------------------------------------------
# 3. 生成动态库（**显式**把 .cpp 扔进去，确保重新 CMake 时一定编译）
#-----------------------------------------------------------
add_library(MechSectionShapeData
    ${_header}   # 头文件仍可扔进来，方便 IDE 浏览
    ${_source}   # 这里至少包含 mSectionShapeBase.cpp / mSectionShapeManager.cpp
)

#-----------------------------------------------------------
# 4. 宏控制导出（**只在库侧**定义，使用侧不要）
#-----------------------------------------------------------
target_compile_definitions(MechSectionShapeData PRIVATE "SECTIONSHAPE_LIB")

list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets MxFunctions  
              # <-- 漏了，加上

)
#-----------------------------------------------------------
# 5. 链接库（保持原样）
#-----------------------------------------------------------
target_link_libraries(MechSectionShapeData PRIVATE
   ${_runtimes_libraries}
	#【OCC迁移·P4 达成】原 3 库（TKernel/TKService/TKV3d）摘除：其传递闭包（Geometry 导出头
	# operator new→Standard_OutOfMemory、经 mGeoManager.h 链 A 的 AIS/PrsMgr 构造闭包）
	# 已随 2026-09-18 链 A 续清（mGeoAttrDef.h 独立头+前向声明化）消除，链接探测零未解析符号。
)
#set(SectionShapeData_Runtimes_Libraries ${_runtimes_libraries} PARENT_SCOPE)
#-----------------------------------------------------------
# 6. 安装（保持原样）
#-----------------------------------------------------------
install(FILES ${_header}
        DESTINATION ${INSTALL_INCDIR})