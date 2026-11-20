#-----------------------------------------------------------------------------
# PostWindow 回归测试
#
# 首批覆盖 mPostTreeModel（后处理树数据模型，自前处理共享树模型裁剪独立）：
# 建树骨架、index/parent 导航、键-索引往返、增删节点与信号、分组生命周期、
# data/setData 各角色、图标资源映射。全部用 QCoreApplication 即可运行（无头）。
#
# 第二批覆盖 mPostDeformation 纯逻辑（变形变量列表过滤/默认位移变量判定/
# 三分量标量场组装节点矢量场），QCoreApplication 即可运行（无头），
# 不触视口/渲染/读取器（applyDeformVariableField 依赖渲染端对象，不在无头范围）。
#
# 一个 QtTest 类一个可执行文件（各带 QTEST_MAIN，不能合并进同一目标）。
#
# 不实例化 mPostBrowserWindow（私有构造单例 + 强 GUI/停靠依赖），
# 窗口级用例待后续接缝成熟再补。
#-----------------------------------------------------------------------------

# 与 src/PostWindow/CMakeLists.txt 的目录级 include 保持一致
# （那里用 include_directories 目录作用域，不随目标传播，此处需自备）
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/PostWindow)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/Common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/CommonWidgets)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/GUIFoundation)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/PreData)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/PostCommon)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/PostIOFile)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/PostCurve)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/IOFile)

# 显式携带 include 路径：本工程各模块 vcxproj 均未内嵌 Qt 头路径，
# 依赖外部 INCLUDE 环境变量（含 Core/Gui/Widgets 但未必含 Test），此处显式声明保证任何环境可编
# tst_mPostDeformation 经 mPostDeformation.h（QWidget 族）与 mPostGUIData.h（mPostDataBase.h）
# 需要 Widgets 与 BasicData 头路径
find_package(Qt5 CONFIG COMPONENTS Core Gui Widgets Test REQUIRED)

#-----------------------------------------------------------------------------
# conan 运行库全量路径（configure 期解析 conan 生成的 conanrunenv bat）：
# PostWindowd.dll 的传递依赖 OCC/CGM/qwt/ffmpeg/python 等都在 conan 包 bin，
# 仅注入 Basic+Qt 时测试进程 0xc0000135——此清单与 CMakePresets 的
# CONAN_RUNTIME_PATHS_DEBUG 同源，conan 依赖更新后重新配置即自动跟随
#-----------------------------------------------------------------------------
set(_conan_run_bat "${CMAKE_SOURCE_DIR}/conan_install/build/generators/conanrunenv-debug-x86_64.bat")
set(MxTest_CONAN_RUNTIME_DIRS "")
if(EXISTS "${_conan_run_bat}")
    file(READ "${_conan_run_bat}" _conanrun)
    string(REGEX MATCH "set \"PATH=([^\r\n]*)\"" _m "${_conanrun}")
    set(_dirs "${CMAKE_MATCH_1}")
    foreach(_d ${_dirs})
        # 跳过 %PATH% 尾串与空项；正斜杠目录（CGM code/bin）CMake 可直接处理
        if(_d AND NOT _d MATCHES "^%")
            list(APPEND MxTest_CONAN_RUNTIME_DIRS "${_d}")
        endif()
    endforeach()
    list(REMOVE_DUPLICATES MxTest_CONAN_RUNTIME_DIRS)
endif()
unset(_conanrun)
unset(_dirs)
unset(_d)
unset(_m)

#-----------------------------------------------------------------------------
# 公共设定包装（两测试目标同构：源文件→目标→注册 ctest）
#-----------------------------------------------------------------------------
function(add_postwindow_test TARGET_NAME)
    #可选 RESULT_FILE：测试结果落盘（无控制台宿主下 QtTest 调试版直写控制台
    #句柄会异常退出——零输出+非零退出码，2026-11-12 实测；落盘后仍按退出码判定）
    cmake_parse_arguments(TST "" "RESULT_FILE" "" ${ARGN})
    add_executable(${TARGET_NAME} ${TST_UNPARSED_ARGUMENTS})

    target_compile_features(${TARGET_NAME} PRIVATE cxx_std_17)
    target_include_directories(${TARGET_NAME} PRIVATE
        ${Qt5Core_INCLUDE_DIRS}
        ${Qt5Gui_INCLUDE_DIRS}
        ${Qt5Widgets_INCLUDE_DIRS}
        ${MxFunctions_INC_DIR}
        ${BasicData_INC_DIR}
    )
    # conan Qt5::Test 目标的 INTERFACE_INCLUDE_DIRECTORIES 只含 include/ 父目录且包在
    # CONFIG genex 里（解析不了 #include <QtTest>），显式补 QtTest 模块目录；
    # 测试源码同时使用 <QtTest/QtTest> 风格（仅需父目录），双保险
    target_include_directories(${TARGET_NAME} PRIVATE "${Qt5_DIR}/../../../include/QtTest")

    target_link_libraries(${TARGET_NAME} PRIVATE
        PostWindow
        MxFunctions      # mPostTreeModel.h 传递包含 mModelEnum/mModelStruct/mTreeModel 所需
        MechPreData      # tst_mPostDeformation 用 mPostGUIData（构造/析构在 PreData 库）
        CommonWidgets    # tst_mPanelBaseViewportFollow 直接实例化 mPanelBase（批次3）
        Qt5::Core
        Qt5::Gui         # MxFunctions 头内静态 PartColorLoop 引用 QVector4D（QtGui）
        Qt5::Widgets     # tst_mPostDeformation 经 mPostDeformation.h 传递包含 QWidget 族
        Qt5::Test
        # conan Qt5::Test 目标缺 IMPORTED_LOCATION（Qt5Test(d).lib 不进链接输入），按配置显式链
        "$<$<CONFIG:Debug>:${Qt5_DIR}/../../../lib/Qt5Testd.lib>"
        "$<$<NOT:$<CONFIG:Debug>>:${Qt5_DIR}/../../../lib/Qt5Test.lib>"
        Dbghelp          # CRT 断言钩子的栈回溯（SymInitialize/SymFromAddr）
    )

    set_target_properties(${TARGET_NAME} PROPERTIES FOLDER Tests)

    if(TST_RESULT_FILE)
        add_test(NAME ${TARGET_NAME}
            COMMAND $<TARGET_FILE:${TARGET_NAME}> -o ${TST_RESULT_FILE},txt)
    else()
        add_test(NAME ${TARGET_NAME} COMMAND $<TARGET_FILE:${TARGET_NAME}>)
    endif()
    #测试进程的 DLL 搜索路径：Qt/OSG 等运行库在 Conan 缓存目录、自研库在 Basic/*/install/bin
    # （根目录烘培为 MxTest_RUNTIME_PATH）。
    #工作目录设为 exe 所在目录（Debug/）：实测（2026-10-13）该环境下测试进程的
    #exe 目录与 PATH 搜索均异常失效、仅 CWD 解析可靠——自研 DLL（PostWindowd 等）
    #与 exe 同目录，经 CWD 兜底解析；Qt/Basic 运行库仍走下方 PATH 注入
    set_tests_properties(${TARGET_NAME} PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET_NAME}>")
    # 注意： ENVIRONMENT 属性按分号拆分列表，PATH 值内的分号会被拆散导致子进程 PATH 残缺，
    # 因此用 ENVIRONMENT_MODIFICATION（CMake>=3.22）逐目录前置，且运行时继承 ctest 自身 PATH
    set(_mxtest_path_mod "")
    foreach(_dir ${MxTest_RUNTIME_PATH})
        list(APPEND _mxtest_path_mod "PATH=path_list_prepend:${_dir}")
    endforeach()
    # Qt 运行库：conan Qt5 目标的 IMPORTED_LOCATION 只指向 lib 的 .lib，DLL 在包内 bin/；
    # "继承 ctest 自身 PATH"在干净 shell/CI（无 conan 激活环境）拿不到 Qt DLL，测试进程
    # 全线 0xc0000135——显式前置包内 bin（与上方 QtTest include/lib 的显式声明同理；
    # 2026-10-13 regen 后裸 shell 跑 ctest 复现定位）。conan 清单解析失败时此条兜底
    get_filename_component(_qt_runtime_bin "${Qt5_DIR}/../../../bin" ABSOLUTE)
    list(APPEND _mxtest_path_mod "PATH=path_list_prepend:${_qt_runtime_bin}")
    # Qt 平台插件：链接 Widgets 的测试经 QTEST_MAIN 实例化 QApplication，启动需
    # platforms/qwindows.dll——无 QT_PLUGIN_PATH 时弹"no Qt platform plugin could
    # be initialized"运行时库对话框（2026-10-13 regen 后实测复现）。显式前置包内
    # plugins 目录（与 CMakePresets 的 QT_PLUGIN_PATH_DEBUG 同源）
    get_filename_component(_qt_plugins "${Qt5_DIR}/../../../plugins" ABSOLUTE)
    list(APPEND _mxtest_path_mod "QT_PLUGIN_PATH=path_list_prepend:${_qt_plugins}")
    # conan 全量运行库（OCC/CGM/qwt/ffmpeg/python 等 PostWindowd.dll 传递依赖）：
    # 上方 configure 期自 conanrunenv bat 解析，见文件头注释
    foreach(_dir ${MxTest_CONAN_RUNTIME_DIRS})
        list(APPEND _mxtest_path_mod "PATH=path_list_prepend:${_dir}")
    endforeach()
    set_tests_properties(${TARGET_NAME} PROPERTIES
        ENVIRONMENT_MODIFICATION "${_mxtest_path_mod}")
    unset(_mxtest_path_mod)
    unset(_qt_runtime_bin)
endfunction()

add_postwindow_test(PostWindowTests tst_mPostTreeModel.cpp
    RESULT_FILE ${CMAKE_BINARY_DIR}/PostWindowTests-result.txt)
add_postwindow_test(PostWindowDeformationTests tst_mPostDeformation.cpp)
add_postwindow_test(PostResultRegistryTests tst_mPostResultRegistry.cpp)
add_postwindow_test(PostDerivedCaseStoreTests tst_mPostDerivedCaseStore.cpp)
# 批次A1：Basic 派生帧接口族无头回归（宿主在 GUI 测试工程——BasicData 无独立测试基建）；
# RESULT_FILE 落盘（无控制台宿主下 QtTest 调试版直写控制台句柄异常退出）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPostDerivedFrames.cpp")
    add_postwindow_test(PostDerivedFramesTests tst_mPostDerivedFrames.cpp
        RESULT_FILE ${CMAKE_BINARY_DIR}/PostDerivedFramesTests-result.txt)
    # 直链 BasicData：本套件直接 new mPostDataBase/mPostModelData（符号在 PostDatad.dll），
    # 函数公共链接表只经 PostWindow 传递、dllimport 直引不传递（同 MechPreData 先例注释）
    target_link_libraries(PostDerivedFramesTests PRIVATE PostData)
else()
    message(STATUS "tst_mPostDerivedFrames.cpp 未入库,跳过 PostDerivedFramesTests 注册")
endif()
# 批次B：包络归约纯函数（mPostDerivedCaseEngine 静态函数，符号在 PostWindowd.dll，
# 函数公共链接表的 PostWindow 即够，无需额外链接）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPostEnvelopeReduce.cpp")
    add_postwindow_test(PostEnvelopeReduceTests tst_mPostEnvelopeReduce.cpp
        RESULT_FILE ${CMAKE_BINARY_DIR}/PostEnvelopeReduceTests-result.txt)
else()
    message(STATUS "tst_mPostEnvelopeReduce.cpp 未入库,跳过 PostEnvelopeReduceTests 注册")
endif()
# 向量图接入：参数折算纯函数（mPostVectorPlot detail 族，符号在 PostWindowd.dll，
# 函数公共链接表的 PostWindow 即够；RESULT_FILE 落盘同上）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPostVectorPlotLogic.cpp")
    add_postwindow_test(PostVectorPlotLogicTests tst_mPostVectorPlotLogic.cpp
        RESULT_FILE ${CMAKE_BINARY_DIR}/PostVectorPlotLogicTests-result.txt)
else()
    message(STATUS "tst_mPostVectorPlotLogic.cpp 未入库,跳过 PostVectorPlotLogicTests 注册")
endif()
# 截面剖切接入：渲染装配/校验纯函数（mPostSectionCut detail 族，符号在 PostWindowd.dll，
# 函数公共链接表的 PostWindow 即够；RESULT_FILE 落盘同上）
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPostSectionCutLogic.cpp")
    add_postwindow_test(PostSectionCutLogicTests tst_mPostSectionCutLogic.cpp
        RESULT_FILE ${CMAKE_BINARY_DIR}/PostSectionCutLogicTests-result.txt)
else()
    message(STATUS "tst_mPostSectionCutLogic.cpp 未入库,跳过 PostSectionCutLogicTests 注册")
endif()
# 批次3两套件源文件尚未入库（2026-10-29 核实：全分支无历史）,源缺席时守卫跳过,
# 否则 add_executable 无源使 generate 整体失败、拖垮全部测试注册
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPostViewportNaming.cpp")
    add_postwindow_test(PostViewportNamingTests tst_mPostViewportNaming.cpp)
else()
    message(STATUS "tst_mPostViewportNaming.cpp 未入库,跳过 PostViewportNamingTests 注册")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tst_mPanelBaseViewportFollow.cpp")
    add_postwindow_test(PanelBaseViewportFollowTests tst_mPanelBaseViewportFollow.cpp)
else()
    message(STATUS "tst_mPanelBaseViewportFollow.cpp 未入库,跳过 PanelBaseViewportFollowTests 注册")
endif()

#-----------------------------------------------------------------------------
# PostWindow 与 MainWindow 编译隔离守卫（防倒退棘轮，脚本自 src 根定位）
#-----------------------------------------------------------------------------
find_program(MxBASH_EXECUTABLE bash)
if(MxBASH_EXECUTABLE)
    add_test(NAME PostWindowIsolation
        COMMAND ${MxBASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/check_postwindow_isolation.sh)
else()
    message(STATUS "未找到 bash，跳过 PostWindowIsolation 测试注册（Git for Windows 环境应自带）")
endif()
