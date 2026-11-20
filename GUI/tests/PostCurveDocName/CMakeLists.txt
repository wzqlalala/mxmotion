# PostCurve 曲线允许重名（R23-T3）无头自测（纯控制台断言，退出码=失败数）
#
# 覆盖（20261109-R23-T3 测试任务单新增用例要求）：
#   1. 模型层：同图表同名创建普通/复数/柱状曲线成功且节点独立（数据/样式互不
#      覆盖，series.name 相同但为两个节点）；kName 重命名为既有名成功；空名
#      守卫仍在；撤销/重做/增删后 id 与名稳定
#   2. 粘贴 " (N)" 自动后缀回归（同名加后缀/剪切原位粘贴=原名回归/跨图表原名）
#   3. 视图层键规则镜像（mCurveChartView::ensureEngineCurveKeys 文档口径的独立
#      复算：无碰撞键=名/碰撞 名#2/#3/字面 "名#2" 共存不串键/改名后重算/双射
#      不变式）+ 图例文本优先级镜像（标签文本>模型名>键名）+ 模板叠加 " (N)"
#      后缀镜像——真实视图接线（QWidget/mPlotView 链）无法无头链接，列入手动
#      验收清单
#   4. pch/复数/柱状无碰撞抽样回归（复数 a/b 截齐、XY→柱状提取转换名保留、
#      同名曲线整批转柱状名各自保留；pch 装配静态路径由 tst_mCurveAscii 覆盖）
#
# 直接编译 src/PostCurve 模型层源文件（BUILD_STATIC 剥导出宏），不链 PostCurve
# DLL；mCurveDocController.cpp 的 report* 引用 MxFunctions 两符号（getInstance/
# outputMessageSig），测试内给空实现（BUILD_STATIC 下 MXFUNCTIONS_EXPORT 为空，
# 成员可本地定义；不链 MxFunctions DLL）。依赖 Qt5 Core/Gui/Widgets（Gui：
# CurveDocument/QColor；Widgets：mGlobalSignals.h 传递 QDockWidget 等头）。
find_package(Qt5 CONFIG COMPONENTS Core Gui Widgets REQUIRED)

set(_postcurve_src "${CMAKE_CURRENT_SOURCE_DIR}/../../src/PostCurve")

add_executable(tst_mCurveDocName
    tst_mCurveDocName.cpp
    "${_postcurve_src}/CurveCore/mCurveDocController.cpp"
    "${_postcurve_src}/CurveCore/CurveDocument.cpp"
    "${_postcurve_src}/CurveCore/ChartTypeProfile.cpp"
    "${_postcurve_src}/CurveCommands/CurveCommands.cpp"
    "${_postcurve_src}/CurveBarExtract.cpp"
    "${_postcurve_src}/mCurveAsciiReader.cpp"
    "${_postcurve_src}/mCurveDataSourceStore.cpp"
)

target_compile_features(tst_mCurveDocName PRIVATE cxx_std_17)
# BUILD_STATIC：PostCurveGlobal.h 的 POSTCURVE_EXPORT 与 mxfunctions_global.h 的
# MXFUNCTIONS_EXPORT 均退化为空（本地编译不导入 DLL 符号，MxFunctions 桩可定义）
target_compile_definitions(tst_mCurveDocName PRIVATE BUILD_STATIC)
target_include_directories(tst_mCurveDocName PRIVATE
    "${_postcurve_src}"
    "${MxFunctions_INC_DIR}"   #mGlobalSignals.h/mOutputMessage.h（report* 桩用）
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Gui_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
)
target_link_libraries(tst_mCurveDocName PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets PchReader::PchReader)

set_target_properties(tst_mCurveDocName PROPERTIES FOLDER Tests)

add_test(NAME tst_mCurveDocName COMMAND $<TARGET_FILE:tst_mCurveDocName>)
set_tests_properties(tst_mCurveDocName PROPERTIES
    WORKING_DIRECTORY "$<TARGET_FILE_DIR:tst_mCurveDocName>")
# Qt5Core.dll 运行库：conan 包 bin 逐目录前置（同 tests/PostCurveAscii 机制；
# MxTest_RUNTIME_PATH 由根 CMakeLists 烘焙：conan 运行库 + Basic install/bin）
set(_mxtest_path_mod "")
foreach(_dir ${MxTest_RUNTIME_PATH})
    list(APPEND _mxtest_path_mod "PATH=path_list_prepend:${_dir}")
endforeach()
get_filename_component(_qt_runtime_bin "${Qt5_DIR}/../../../bin" ABSOLUTE)
list(APPEND _mxtest_path_mod "PATH=path_list_prepend:${_qt_runtime_bin}")
set_tests_properties(tst_mCurveDocName PROPERTIES
    ENVIRONMENT_MODIFICATION "${_mxtest_path_mod}")
