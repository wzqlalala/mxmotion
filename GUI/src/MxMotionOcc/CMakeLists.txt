set(common_dir "${CMAKE_CURRENT_SOURCE_DIR}/../Common")
set(window_dir "${CMAKE_CURRENT_SOURCE_DIR}/../MainWindow")
set(foundation_dir "${CMAKE_CURRENT_SOURCE_DIR}/../GUIFoundation")
set(gui_root "${CMAKE_CURRENT_SOURCE_DIR}/../..")
include(OriginalGui.cmake)

add_executable(MxMotion WIN32
    main.cpp
    MotionWorkspace.cpp MotionWorkspace.h
    MotionWindow.cpp MotionWindow.h
    MotionViewPages.cpp MotionViewPages.h MotionViewPageControls.cpp
    MotionRibbon.cpp
    OriginalMotionUi.cpp OriginalMotionResults.cpp OriginalResultUi.cpp OriginalUiActions.cpp OriginalSelection.cpp OriginalSelection.h
    OriginalEngineExtensions.cpp OriginalEngineExtensions.h
    RigidMotionUi.cpp
    NativeMotionUi.cpp
    NativeMotionTypedPanels.cpp NativeMotionTypedPanels.h
    NativeModeInspector.cpp NativeModeInspector.h NativeModeInspectorFlow.cpp NativeMnfImportUi.cpp
    NativeMnfBindingView.cpp NativeMnfBindingView.h
    NativeMotionPickWidgets.cpp NativeMotionPickWidgets.h
    NativeFunctionEditor.cpp NativeFunctionEditor.h
    NativeMotionSymbols.cpp NativeMotionSymbols.h
    MultiphysicsSchematicDialog.cpp MultiphysicsSchematicDialog.h
    NativeLoadExportUi.cpp
    RigidPlot.cpp RigidPlot.h
    RigidResultSeries.cpp RigidResultSeries.h
    OccViewport.cpp OccViewport.h
    compat/RibbonControls.cpp compat/mUiMenu.h
    "${common_dir}/mWindowTitleBar.cpp"
    "${common_dir}/mFramelessWindowHelper.cpp"
    "${common_dir}/mFramelessDialog.cpp"
    "${common_dir}/mFramelessMessageBox.cpp" "${common_dir}/mFramelessMessageBox.h"
    "${common_dir}/mRibbonWidget.cpp"
    "${common_dir}/MyTabBar.cpp"
    "${common_dir}/mUiStyle.cpp"
    "${common_dir}/ThemeManager.cpp"
    "${foundation_dir}/mFloatingDockFrameless.cpp"
    "${window_dir}/mChineseTextEdit.h"
    "${window_dir}/ViewportWidget.cpp"
    "${window_dir}/ViewportContainer.cpp" "${window_dir}/ViewportLayoutEngine.cpp"
    "${gui_root}/src/CommonWidgets/mFoldSection.cpp")
if(WIN32)
    target_sources(MxMotion PRIVATE "${gui_root}/src/MxMech/icon.rc")
endif()
set_target_properties(MxMotion PROPERTIES AUTOMOC ON)
target_include_directories(MxMotion PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${CMAKE_CURRENT_SOURCE_DIR}" "${common_dir}" "${window_dir}" "${foundation_dir}"
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
target_link_libraries(MxMotion PRIVATE MotionOccOriginalGui MotionOccRigid MotionEngine::Core Qt5::Widgets Qt5::Svg Qt5::PrintSupport Qt5::Concurrent
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
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccWorkspaceSmoke.cpp")
    add_test(NAME MotionOcc.Workspace COMMAND MxMotion --workspace-smoke "${PROJECT_BINARY_DIR}/workspace-smoke")
    set_tests_properties(MotionOcc.Workspace PROPERTIES TIMEOUT 90 LABELS desktop)
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccViewPagesSmoke.cpp")
    add_test(NAME MotionOcc.ViewPages COMMAND MxMotion --view-pages-smoke "${PROJECT_BINARY_DIR}/view-pages-smoke")
    set_tests_properties(MotionOcc.ViewPages PROPERTIES TIMEOUT 120 LABELS desktop)
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccMixedViewSmoke.cpp")
    add_test(NAME MotionOcc.MixedViews COMMAND MxMotion --mixed-view-smoke "${PROJECT_BINARY_DIR}/mixed-view-smoke")
    set_tests_properties(MotionOcc.MixedViews PROPERTIES TIMEOUT 120 LABELS desktop)
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccNativeDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccPortableCaseDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccCadImportSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccTreeSelectionSmoke.cpp")
    add_test(NAME MotionOcc.TreeSelection COMMAND MxMotion --tree-selection-smoke "${PROJECT_BINARY_DIR}/tree-selection-smoke")
    set_tests_properties(MotionOcc.TreeSelection PROPERTIES TIMEOUT 90 LABELS desktop)
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalGuiSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalAutoFlexSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalTreeSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalNativeCurvesSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalLoadExportSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalPickerSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalBackendSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalPlotSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccPickingSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccModalDesktopSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccNavigationSmoke.cpp")
    target_sources(MxMotion PRIVATE "${PROJECT_SOURCE_DIR}/tests/MotionOccRenderFeedbackSmoke.cpp")
    set_property(SOURCE main.cpp
        "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalBackendSmoke.cpp"
        "${PROJECT_SOURCE_DIR}/tests/MotionOccOriginalPlotSmoke.cpp"
        APPEND PROPERTY COMPILE_DEFINITIONS MX_OCC_DESKTOP_SMOKE)
    set(desktop_output "${PROJECT_BINARY_DIR}/desktop-smoke")
    file(MAKE_DIRECTORY "${desktop_output}")
    add_test(NAME MotionOcc.Desktop COMMAND MxMotion --desktop-smoke "${desktop_output}")
    set_tests_properties(MotionOcc.Desktop PROPERTIES TIMEOUT 90 LABELS desktop)
    set(native_desktop_output "${PROJECT_BINARY_DIR}/native-desktop-smoke")
    file(MAKE_DIRECTORY "${native_desktop_output}")
    add_test(NAME MotionOcc.NativeDesktop COMMAND MxMotion --native-desktop-smoke "${native_desktop_output}")
    set_tests_properties(MotionOcc.NativeDesktop PROPERTIES TIMEOUT 180 LABELS desktop)
    add_test(NAME MotionOcc.ForceEditorDesktop COMMAND MxMotion --force-editor-smoke "${PROJECT_BINARY_DIR}/force-panel-check")
    set_tests_properties(MotionOcc.ForceEditorDesktop PROPERTIES TIMEOUT 90 LABELS desktop)
    get_target_property(engine_features MotionEngine::Core INTERFACE_COMPILE_DEFINITIONS)
    if("MX_HAS_BODY_DRIVES=1" IN_LIST engine_features)
        add_test(NAME MotionOcc.BodyDriveDesktop COMMAND MxMotion --body-drive-desktop-smoke "${PROJECT_BINARY_DIR}/body-drive-desktop-smoke")
        set_tests_properties(MotionOcc.BodyDriveDesktop PROPERTIES TIMEOUT 90 LABELS desktop)
    endif()
    add_test(NAME MotionOcc.OriginalNativeCurves COMMAND MxMotion --original-native-curves-smoke "${PROJECT_BINARY_DIR}/original-native-curves-smoke")
    set_tests_properties(MotionOcc.OriginalNativeCurves PROPERTIES TIMEOUT 120 LABELS desktop)
    add_test(NAME MotionOcc.OriginalLoadExport COMMAND MxMotion --original-load-export-smoke "${PROJECT_BINARY_DIR}/original-load-export-smoke")
    set_tests_properties(MotionOcc.OriginalLoadExport PROPERTIES TIMEOUT 120 LABELS desktop)
    add_test(NAME MotionOcc.Picking COMMAND MxMotion --picking-smoke "${PROJECT_BINARY_DIR}/picking-smoke")
    set_tests_properties(MotionOcc.Picking PROPERTIES TIMEOUT 90 LABELS desktop)
    add_test(NAME MotionOcc.OriginalPicker COMMAND MxMotion --original-picker-smoke "${PROJECT_BINARY_DIR}/original-picker-smoke")
    set_tests_properties(MotionOcc.OriginalPicker PROPERTIES TIMEOUT 90 LABELS desktop)
    add_test(NAME MotionOcc.OriginalBackend COMMAND MxMotion --original-backend-smoke)
    set_tests_properties(MotionOcc.OriginalBackend PROPERTIES TIMEOUT 90 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    add_test(NAME MotionOcc.OriginalPlot COMMAND MxMotion --original-plot-smoke)
    set_tests_properties(MotionOcc.OriginalPlot PROPERTIES TIMEOUT 90 ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    if(WIN32)
        get_target_property(original_qmake Qt5::qmake IMPORTED_LOCATION)
        execute_process(COMMAND "${original_qmake}" -query QT_INSTALL_PLUGINS
            OUTPUT_VARIABLE original_qt_plugins OUTPUT_STRIP_TRAILING_WHITESPACE
            COMMAND_ERROR_IS_FATAL ANY)
        foreach(original_offscreen_test IN ITEMS MotionOcc.OriginalBackend MotionOcc.OriginalPlot)
            set_property(TEST ${original_offscreen_test} APPEND PROPERTY ENVIRONMENT
                "QT_QPA_PLATFORM_PLUGIN_PATH=${original_qt_plugins}/platforms"
                "QT_QPA_FONTDIR=C:/Windows/Fonts")
        endforeach()
    endif()
    add_test(NAME MotionOcc.Navigation COMMAND MxMotion --navigation-smoke "${PROJECT_BINARY_DIR}/navigation-smoke")
    set_tests_properties(MotionOcc.Navigation PROPERTIES TIMEOUT 90 LABELS desktop)
    add_test(NAME MotionOcc.RenderFeedback COMMAND MxMotion --render-feedback-smoke "${PROJECT_BINARY_DIR}/render-feedback-smoke")
    set_tests_properties(MotionOcc.RenderFeedback PROPERTIES TIMEOUT 90 LABELS desktop)
    add_test(NAME MotionOcc.ModalDesktop COMMAND MxMotion --modal-desktop-smoke "${PROJECT_BINARY_DIR}/modal-desktop-smoke")
    set_tests_properties(MotionOcc.ModalDesktop PROPERTIES TIMEOUT 120 LABELS desktop)
endif()
