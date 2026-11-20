#-----------------------------------------------------------------------------
# 头文件搜索路径
#-----------------------------------------------------------------------------
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PostCommon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../APIServer)

#-----------------------------------------------------------------------------
# 自动添加include目录
#-----------------------------------------------------------------------------
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#-----------------------------------------------------------------------------
# 源码扫描
file(GLOB _header "*.h")
file(GLOB _source "*.cpp")
file(GLOB _mxdb_reader_header "MxdbReader/*.h")
file(GLOB _mxdb_reader_source "MxdbReader/*.cpp")
file(GLOB _mxdb_reader_test "MxdbReader/test_*.cpp")
list(REMOVE_ITEM _mxdb_reader_source ${_mxdb_reader_test})

# Altair Generic ASCII 读写 + HvTrans/HyperView 作业 + Altair 导入线程（子目录）
file(GLOB _altair_source "AltairResultIO/*.cpp")
file(GLOB _altair_header "AltairResultIO/*.h")
list(APPEND _source ${_altair_source})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/AltairResultIO)

# MxNativeReaders 原生读取链路（OP2/RST·RTH/d3plot/H3D/ODB，子目录）；测试另编独立目标
file(GLOB _native_source "NativeReadersIO/*.cpp")
file(GLOB _native_header "NativeReadersIO/*.h")
file(GLOB _native_test "NativeReadersIO/test_*.cpp")
list(REMOVE_ITEM _native_source ${_native_test})
list(APPEND _source ${_native_source})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/NativeReadersIO)

# VTK 后处理文件读取链路（.vtk/.vtp/.vtu/.vtm 及 parallel 聚合，子目录，走 vfr::vfr）
file(GLOB _vtk_source "VtkIO/*.cpp")
file(GLOB _vtk_header "VtkIO/*.h")
file(GLOB _vtk_test "VtkIO/test_*.cpp")
list(REMOVE_ITEM _vtk_source ${_vtk_test})
list(APPEND _source ${_vtk_source})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/VtkIO)

# 条件编译 MxdbReader 测试
if(MXDB_READER_ENABLE_TEST)
    add_executable(MxdbReaderTest
        MxdbReader/test_mxdb_reader.cpp
        MxdbReader/MxdbDataBuilder.cpp
    )
    target_include_directories(MxdbReaderTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/MxdbReader
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
    )
    target_link_libraries(MxdbReaderTest PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        PostData
        MechModelData  # ModelData->MechModelData：本工作区 Basic 仓 target 带 Mech 前缀
        MxFunctions
        MxdbReaderCore
    )
    if(WIN32)
        target_link_libraries(MxdbReaderTest PRIVATE Dbghelp)
    else()
        target_link_options(MxdbReaderTest PRIVATE -rdynamic)
    endif()

    add_executable(MxdbThreadVerify
        MxdbReader/test_mxdbthread_verify.cpp
    )
    target_include_directories(MxdbThreadVerify PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/MxdbReader
        ${CMAKE_CURRENT_SOURCE_DIR}/../PreData
        ${CMAKE_CURRENT_SOURCE_DIR}/../PostCommon
        ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../Basic/RebuildRender/include
        #【OCC摘除·S11】OCC 头路径引用移除：根级 find_package 已注释，变量为空的
        # include 无效果（GeoEngineData pimpl 化后头网已根除）
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
    )
    target_link_libraries(MxdbThreadVerify PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        #【后端可切换·P0-6 甄别修正】非死链（导出头网内联闭包需 TKernel 符号；消根归 P0-1/P1 头链卫生）
        #【后端可切换·头链卫生·探测结论】OCC 链接暂留：门面前向声明化需与 P0-1（getGEOServer 318 处切 handle）同批，届时一并摘除
        #【OCC摘除·S10/P0-1】头网已根除（GeoEngineData pimpl 化），链接摘除
        #${OpenCASCADE_LIBRARIES}
        PostIOFile
        PostData
        MechModelData  # ModelData->MechModelData：本工作区 Basic 仓 target 带 Mech 前缀
        MxFunctions
        MxdbReaderCore
        ${CurrentModel}PreData
    )
    if(WIN32)
        target_link_libraries(MxdbThreadVerify PRIVATE Dbghelp)
    endif()
    add_dependencies(MxdbThreadVerify PostIOFile)
    add_executable(NativeImportTest
        NativeReadersIO/test_native_import.cpp
    )
    target_include_directories(NativeImportTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/NativeReadersIO
    )
    target_link_libraries(NativeImportTest PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        PostIOFile
        PostData
        MxFunctions
    )
    add_dependencies(NativeImportTest PostIOFile)
    # VTK 导入回归（自包含合成数据；提供样例文件时额外跑 vfr::read_file+builder 全链路）
    add_executable(VtkImportTest
        VtkIO/test_vtk_import.cpp
    )
    target_include_directories(VtkImportTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/VtkIO
    )
    target_link_libraries(VtkImportTest PRIVATE
        Qt5::Core
        Qt5::Gui
        Qt5::Widgets
        PostIOFile
        PostData
        MxFunctions
    )
    add_dependencies(VtkImportTest PostIOFile)

    #---- 收编进 ctest（需根目录 MxSim_ENABLE_TESTS=ON 提供 enable_testing）----
    if(MxSim_ENABLE_TESTS)
        # NativeImportTest 无参数时仅验证 tryGetNativeSelection 分发规则，可直接注册；
        # 提供样例结果文件时额外跑完整导入+懒读（OP2/RST/RTH/ODB/H3D/d3plot 任一）
        add_test(NAME NativeImportTest COMMAND $<TARGET_FILE:NativeImportTest>)
        add_test(NAME VtkImportTest COMMAND $<TARGET_FILE:VtkImportTest>)
        if(MXDB_TEST_SAMPLE_FILE)
            add_test(NAME NativeImportTestSample COMMAND $<TARGET_FILE:NativeImportTest> "${MXDB_TEST_SAMPLE_FILE}")
        endif()
        # MxdbReaderTest / MxdbThreadVerify 需要样例结果文件参数，仅在提供时注册：
        #   cmake --preset conan-default -DMXDB_READER_ENABLE_TEST=ON -DMXDB_TEST_SAMPLE_FILE=D:/path/sample.mxdb
        if(MXDB_TEST_SAMPLE_FILE)
            add_test(NAME MxdbReaderTest COMMAND $<TARGET_FILE:MxdbReaderTest> "${MXDB_TEST_SAMPLE_FILE}")
            add_test(NAME MxdbThreadVerify COMMAND $<TARGET_FILE:MxdbThreadVerify> "${MXDB_TEST_SAMPLE_FILE}")
        endif()
        set(_native_ctest_list NativeImportTest VtkImportTest)
        if(MXDB_TEST_SAMPLE_FILE)
            list(APPEND _native_ctest_list NativeImportTestSample MxdbReaderTest MxdbThreadVerify)
        endif()
        foreach(_t ${_native_ctest_list})
            # ENVIRONMENT 按分号拆列表，PATH 值内的分号会拆散，改用逐目录前置（CMake>=3.22）
            set(_mxdb_path_mod "")
            foreach(_dir ${MxTest_RUNTIME_PATH})
                list(APPEND _mxdb_path_mod "PATH=path_list_prepend:${_dir}")
            endforeach()
            set_tests_properties(${_t} PROPERTIES ENVIRONMENT_MODIFICATION "${_mxdb_path_mod}")
        endforeach()
        unset(_mxdb_path_mod)
        unset(_native_ctest_list)
    endif()
endif()


#-----------------------------------------------------------------------------
# 添加动态库目标
#-----------------------------------------------------------------------------
add_library(PostIOFile
	${_header}
	${_source}
	${_altair_header}
	${_mxdb_reader_header}
	${_mxdb_reader_source}
)

#-----------------------------------------------------------------------------
# MxdbReader 核心库链接
# MxdbReaderCore 已是独立基础库，通过 find_package 引入；
# 其 include 路径由 IMPORTED target 自动传播。
#-----------------------------------------------------------------------------
target_link_libraries(PostIOFile PUBLIC MxdbReaderCore)
target_link_libraries(PostIOFile PUBLIC ucp::ucp)   # 云图-过滤 导出STL（UnstructureCellPost 等值面拼接，header-only）
# MxNativeReaders：五种求解器结果格式的原生读取（静态库，zlib 由其 INTERFACE 传播，
# 按默认静态链接直接编入 PostIOFile.dll，无 zlib1.dll 运行时部署）
target_link_libraries(PostIOFile PUBLIC MxNativeReaders)
# VTKFileReader：VTK 系后处理文件原生读取（纯 C++17 静态库，无运行时部署）
target_link_libraries(PostIOFile PUBLIC vfr::vfr)

#-----------------------------------------------------------------------------
# 添加接口声明宏
#-----------------------------------------------------------------------------
target_compile_definitions(PostIOFile PRIVATE "MIOFILE_LIB")

list(APPEND _depend_library PostCommon ${CurrentModel}PreData APIServer)
list(APPEND _runtimes_libraries
	Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network MxFunctions MechModelData PostData MxRenderLib MeshPanelData
    GeoNeutralData EventDispatcher GeoServer GeoAdapter GeoEngineData
)

#-----------------------------------------------------------------------------
# 链接依赖库
#-----------------------------------------------------------------------------
# 注意：本模块源码不直接使用 OCC，但历史上经其它模块公共头的内联实现
# （缝合/增量分配器闭包）在链接期需要 OCC 库；【OCC摘除·S10/P0-1】头网根除后已摘除。
target_link_libraries(PostIOFile PRIVATE
	${_runtimes_libraries}
	${_depend_library}
	#【后端可切换·P0-6 甄别修正】非死链（同上：导出头网内联闭包）
	#【OCC摘除·S10/P0-1】头网已根除（GeoEngineData pimpl 化），链接摘除
	#${OpenCASCADE_LIBRARIES}
)
if(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "/MD /Zi /Ob0 /Od /RTC1")
endif(MSVC)

#-----------------------------------------------------------------------------
# 添加依赖关系
#-----------------------------------------------------------------------------
add_dependencies(PostIOFile ${_depend_library})

#-----------------------------------------------------------------------------
# 查找编译器OpenMP支持
#-----------------------------------------------------------------------------
# 在Release/Debug/RelWithDebInfo解决方案模式下启用OpenMP支持
find_package(OpenMP REQUIRED)
target_compile_options(PostIOFile PRIVATE $<$<CONFIG:Release>:${OpenMP_CXX_FLAGS}>)
target_link_libraries(PostIOFile PRIVATE $<$<CONFIG:Release>:OpenMP::OpenMP_CXX>)
target_compile_options(PostIOFile PRIVATE $<$<CONFIG:Debug>:${OpenMP_CXX_FLAGS}>)
target_link_libraries(PostIOFile PRIVATE $<$<CONFIG:Debug>:OpenMP::OpenMP_CXX>)
target_compile_options(PostIOFile PRIVATE $<$<CONFIG:RelWithDebInfo>:${OpenMP_CXX_FLAGS}>)
target_link_libraries(PostIOFile PRIVATE $<$<CONFIG:RelWithDebInfo>:OpenMP::OpenMP_CXX>)

#-----------------------------------------------------------------------------
# 添加运行时依赖关系
#-----------------------------------------------------------------------------
set(MxGeo_PostIOFile_Runtimes_Libraries ${_runtimes_libraries} PARENT_SCOPE)

#-----------------------------------------------------------------------------
# 安装头文件
#-----------------------------------------------------------------------------
install(
	FILES ${_header}
	DESTINATION ${INSTALL_INCDIR}			#将头文件安装到install/include目录
)

# Altair 子目录头保持相对布局安装（外部以 #include "AltairResultIO/xxx.h" 引用）
install(
	FILES ${_altair_header}
	DESTINATION ${INSTALL_INCDIR}/AltairResultIO
)
