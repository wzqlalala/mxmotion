set(common_dir "${CMAKE_CURRENT_SOURCE_DIR}/../Common")
set(window_dir "${CMAKE_CURRENT_SOURCE_DIR}/../MainWindow")
set(foundation_dir "${CMAKE_CURRENT_SOURCE_DIR}/../GUIFoundation")
set(gui_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")

add_executable(MxMotion WIN32
    main.cpp
    MotionWindow.cpp MotionWindow.h
    MotionRibbon.cpp
    RigidMotionUi.cpp
    NativeMotionUi.cpp
    NativeMotionPickWidgets.cpp NativeMotionPickWidgets.h
    NativeFunctionEditor.cpp NativeFunctionEditor.h
    NativeMotionSymbols.cpp NativeMotionSymbols.h
    NativeLoadExportUi.cpp
    RigidPlot.cpp RigidPlot.h
    RigidResultSeries.cpp RigidResultSeries.h
    OccViewport.cpp OccViewport.h
    compat/RibbonControls.cpp compat/mUiMenu.h
    "${common_dir}/mWindowTitleBar.cpp"
    "${common_dir}/mFramelessWindowHelper.cpp"
    "${common_dir}/mFramelessDialog.cpp"
    "${common_dir}/mRibbonWidget.cpp"
    "${common_dir}/MyTabBar.cpp"
    "${common_dir}/mUiStyle.cpp"
    "${common_dir}/ThemeManager.cpp"
    "${foundation_dir}/mFloatingDockFrameless.cpp"
    "${window_dir}/ViewportWidget.cpp"
    "${gui_root}/src/CommonWidgets/mFoldSection.cpp")
if(WIN32)
    target_sources(MxMotion PRIVATE "${gui_root}/src/MxMech/icon.rc")
endif()
set_target_properties(MxMotion PROPERTIES AUTOMOC ON)
target_include_directories(MxMotion PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}" "${common_dir}" "${window_dir}" "${foundation_dir}"
    "${gui_root}/src/MxMech" "${gui_root}/src/CommonWidgets" compat)

# Let Qt's Unicode-aware resource compiler handle the original asset paths.
# Passing them individually as inputs to Windows Ninja can fail on a non-UTF-8 system code page.
add_custom_target(MotionOccResources
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    COMMAND Qt5::rcc --binary --output "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mMainWidgets.rcc"
            "${gui_root}/resource/mMainWidgets.qrc"
    BYPRODUCTS "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mMainWidgets.rcc"
    VERBATIM)
add_dependencies(MxMotion MotionOccResources)
# The shared chrome is compiled in this executable, not imported from MechCommon.dll.
target_compile_definitions(MxMotion PRIVATE BUILD_STATIC NOMINMAX _HAS_STD_BYTE=0)
target_compile_options(MxMotion PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
target_link_libraries(MxMotion PRIVATE MotionOccRigid MotionEngine::Core Qt5::Widgets Qt5::Svg Qt5::PrintSupport
    OpenGL::GL ads::qtadvanceddocking-qt5 Qwt::Qwt)
if(WIN32)
    target_link_libraries(MxMotion PRIVATE dwmapi)
endif()
add_dependencies(MxMotion MxMotionMeshWorker MotionEngineRuntime)
if(WIN32)
    add_custom_command(TARGET MxMotion POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:ads::qtadvanceddocking-qt5>" "$<TARGET_FILE:Qwt::Qwt>"
                "$<TARGET_FILE:Qt5::PrintSupport>" "$<TARGET_FILE:Qt5::Concurrent>" "$<TARGET_FILE:Qt5::OpenGL>"
                "$<TARGET_FILE_DIR:MxMotion>"
        VERBATIM)
endif()

if(WIN32 AND MX_DEPLOY_QT)
    get_target_property(qmake Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(qt_bin "${qmake}" DIRECTORY)
    find_program(windeployqt NAMES windeployqt HINTS "${qt_bin}" NO_DEFAULT_PATH REQUIRED)
    add_custom_command(TARGET MxMotion POST_BUILD
        COMMAND "${windeployqt}" --release --no-compiler-runtime --no-translations --no-angle
                "$<TARGET_FILE:MxMotion>"
        VERBATIM)
endif()

if(BUILD_TESTING AND MX_TEST_DESKTOP)
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccNativeDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccPickingSmoke.cpp")
    target_compile_definitions(MxMotion PRIVATE MX_OCC_DESKTOP_SMOKE)
    set(desktop_output "${PROJECT_BINARY_DIR}/desktop-smoke")
    file(MAKE_DIRECTORY "${desktop_output}")
    add_test(NAME MotionOcc.Desktop COMMAND MxMotion --desktop-smoke "${desktop_output}")
    set_tests_properties(MotionOcc.Desktop PROPERTIES TIMEOUT 90 LABELS desktop)
    set(native_desktop_output "${PROJECT_BINARY_DIR}/native-desktop-smoke")
    file(MAKE_DIRECTORY "${native_desktop_output}")
    add_test(NAME MotionOcc.NativeDesktop COMMAND MxMotion --native-desktop-smoke "${native_desktop_output}")
    set_tests_properties(MotionOcc.NativeDesktop PROPERTIES TIMEOUT 180 LABELS desktop)
    add_test(NAME MotionOcc.Picking COMMAND MxMotion --picking-smoke "${PROJECT_BINARY_DIR}/picking-smoke")
    set_tests_properties(MotionOcc.Picking PROPERTIES TIMEOUT 90 LABELS desktop)
endif()
