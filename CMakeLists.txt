cmake_minimum_required(VERSION 3.21)
project(MxMotionOcc VERSION 1.0 LANGUAGES CXX)

# The original, full SDK builds remain in GUI/ and ModelData/. The OCC profile
# can also run on Linux, including a software OpenGL desktop for cloud checks.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8 OR (WIN32 AND NOT MSVC))
    message(FATAL_ERROR "The OCC profile requires a 64-bit compiler (MSVC on Windows).")
endif()
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING "Release SDK configurations" FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
elseif(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "Use Release with the Windows OCCT and Netgen SDKs.")
endif()
if(WIN32)
    enable_language(RC)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
include(CTest)
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui)
option(MX_BUILD_GUI "Build the MxMotion desktop application" ON)
option(MX_TEST_DESKTOP "Exercise the real Qt/ADS and OpenGL window (requires an interactive desktop)" OFF)

set(MX_OCCT_ROOT "" CACHE PATH "OCCT 7.8.x SDK prefix (optional with system Linux packages)")
set(MX_NETGEN_ROOT "" CACHE PATH "OCC-enabled Netgen SDK prefix (optional with system Linux packages)")
set(MX_QTADS_SOURCE_DIR "" CACHE PATH "Optional local checkout of Qt Advanced Docking System 4.4.1")
if(WIN32)
    set(MX_QWT_DEFAULT "${PROJECT_SOURCE_DIR}/.local/qwt-sdk")
else()
    set(MX_QWT_DEFAULT "")
endif()
set(MX_QWT_ROOT "${MX_QWT_DEFAULT}" CACHE PATH "Qwt SDK prefix (optional with system Linux packages)")
set(MX_MOTIONENGINE_BUILD_DIR "" CACHE PATH "Optional matching Release Ninja build of the pinned motionengine")
set(MX_MOTIONENGINE_SOURCE_DIR "" CACHE PATH "Optional MotionEngine checkout matching the supplied build")
set(MX_ENGINE_BUILD_JOBS "4" CACHE STRING "Parallel jobs for the isolated MotionEngine build")
include(cmake/MotionOccDependencies.cmake)
option(MX_DEPLOY_QT "Deploy the Windows Qt runtime beside MxMotion.exe" ${WIN32})

if(WIN32)
    if(NOT EXISTS "${MX_OCCT_ROOT}/include/opencascade/STEPControl_Reader.hxx" OR
       NOT EXISTS "${MX_OCCT_ROOT}/bin/TKernel.dll")
        message(FATAL_ERROR "Set MX_OCCT_ROOT to the OCCT SDK's Library directory.")
    endif()
    if(NOT EXISTS "${MX_NETGEN_ROOT}/include/nglib_occ.h" OR
       NOT EXISTS "${MX_NETGEN_ROOT}/bin/nglib.dll")
        message(FATAL_ERROR "Set MX_NETGEN_ROOT to an OCC-enabled Netgen SDK with headers, libraries and DLLs.")
    endif()
endif()
find_path(MX_OCCT_INCLUDE_DIR STEPControl_Reader.hxx
    HINTS "${MX_OCCT_ROOT}/include" PATH_SUFFIXES opencascade REQUIRED)
find_path(MX_NETGEN_INCLUDE_DIR nglib.h
    HINTS "${MX_NETGEN_ROOT}/include" PATH_SUFFIXES netgen REQUIRED)

function(mx_add_docking)
    set(BUILD_EXAMPLES OFF)
    set(BUILD_STATIC OFF)
    set(ADS_VERSION 4.4.1)
    set(QT_VERSION_MAJOR 5)
    if(MX_QTADS_SOURCE_DIR)
        add_subdirectory("${MX_QTADS_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/qtads" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(mx_qtads
            GIT_REPOSITORY https://github.com/githubuser0xFFFF/Qt-Advanced-Docking-System.git
            GIT_TAG 258340842056271ec207f9b112ac5efc45d9b5f5)
        FetchContent_MakeAvailable(mx_qtads)
    endif()
    if(UNIX AND NOT APPLE)
        # ADS 4.4.1 uses QPlatformNativeInterface in its Qt5 X11 backend.
        if(NOT TARGET Qt5::GuiPrivate)
            message(FATAL_ERROR "Qt ADS on Linux requires the Qt5 private headers (qtbase5-private-dev).")
        endif()
        target_link_libraries(qtadvanceddocking-qt5 PRIVATE Qt5::GuiPrivate)
        include("${PROJECT_SOURCE_DIR}/cmake/PatchQtAdsLifetime.cmake")
        mx_patch_qtads_lifetime(qtadvanceddocking-qt5)
    endif()
endfunction()

set(model_dir "${CMAKE_CURRENT_SOURCE_DIR}/ModelData/src/MotionOcc")
add_library(MotionOccData STATIC
    "${model_dir}/CadDocument.cpp"
    "${model_dir}/VerifiedStepCache.cpp"
    "${model_dir}/NativeMotionDocument.cpp"
    "${model_dir}/NativeMotionExchange.cpp"
    "${model_dir}/NativeTopologyBinding.cpp"
    "${model_dir}/NativeBodyMass.cpp"
    "${model_dir}/OccTessellation.cpp"
    "${model_dir}/FemMesh.cpp")
target_include_directories(MotionOccData PUBLIC "${model_dir}")
target_include_directories(MotionOccData SYSTEM PUBLIC "${MX_OCCT_INCLUDE_DIR}")
target_link_libraries(MotionOccData PUBLIC Qt5::Gui)
target_compile_options(MotionOccData PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
# Link directly, avoiding relocated OCCT package exports that contain packager paths.
foreach(lib IN ITEMS TKernel TKMath TKG2d TKG3d TKGeomBase TKBRep TKGeomAlgo
        TKTopAlgo TKPrim TKBO TKBool TKMesh TKShHealing TKXSBase TKDE TKDESTEP TKDEIGES
        TKCDF TKLCAF TKCAF TKXCAF)
    if(WIN32)
        set(occt_lib "${MX_OCCT_ROOT}/lib/${lib}.lib")
        if(NOT EXISTS "${occt_lib}")
            message(FATAL_ERROR "Missing OCCT import library: ${occt_lib}")
        endif()
    else()
        find_library(occt_lib NAMES "${lib}" HINTS "${MX_OCCT_ROOT}/lib" NO_CACHE REQUIRED)
    endif()
    target_link_libraries(MotionOccData PRIVATE "${occt_lib}")
    unset(occt_lib)
endforeach()

add_executable(MxMotionMeshWorker "${model_dir}/NetgenWorker.cpp")
target_include_directories(MxMotionMeshWorker SYSTEM PRIVATE "${MX_NETGEN_INCLUDE_DIR}")
find_library(MX_NGLIB NAMES nglib libnglib HINTS "${MX_NETGEN_ROOT}/lib" PATH_SUFFIXES netgen REQUIRED)
target_link_libraries(MxMotionMeshWorker PRIVATE "${MX_NGLIB}")
target_compile_definitions(MxMotionMeshWorker PRIVATE $<$<PLATFORM_ID:Windows>:WIN32> NOMINMAX)
target_compile_options(MxMotionMeshWorker PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

if(WIN32)
    file(GLOB occ_dlls CONFIGURE_DEPENDS "${MX_OCCT_ROOT}/bin/*.dll")
    file(GLOB netgen_dlls CONFIGURE_DEPENDS "${MX_NETGEN_ROOT}/bin/*.dll")
    add_custom_target(MotionOccRuntime
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different ${occ_dlls} ${netgen_dlls}
                "$<TARGET_FILE:Qt5::Core>" "$<TARGET_FILE:Qt5::Gui>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        VERBATIM)
else()
    add_custom_target(MotionOccRuntime)
endif()
add_dependencies(MxMotionMeshWorker MotionOccRuntime)
mx_import_motionengine()
add_library(MotionOccRigid STATIC "${model_dir}/RigidMotion.cpp" "${model_dir}/RigidProject.cpp"
    "${model_dir}/NativeMotionSolver.cpp" "${model_dir}/NativeMotionGeometry.cpp"
    "${model_dir}/NativeMotionPid.cpp" "${model_dir}/NativeMotionSensors.cpp"
    "${model_dir}/NativeMotionProjection.cpp"
    "${model_dir}/NativeMotionResults.cpp" "${model_dir}/NativeMotionFlex.cpp"
    "${model_dir}/FlexResultScalars.cpp" "${model_dir}/FlexModalBasis.cpp"
    "${model_dir}/NativeMnfPreview.cpp"
    "${model_dir}/NativeMotionActuators.cpp" "${model_dir}/NativeMotionStatics.cpp")
target_sources(MotionOccRigid PRIVATE "${model_dir}/NativeMotionLoadExport.cpp" "${model_dir}/NativeMotionDiagnostics.cpp")
target_link_libraries(MotionOccRigid PUBLIC MotionOccData PRIVATE MotionEngine::Core MotionEngine::Modal)
target_compile_options(MotionOccRigid PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(MotionOccRigid PRIVATE _HAS_STD_BYTE=0)
if(TARGET MotionEngineBuild)
    add_dependencies(MotionOccRigid MotionEngineBuild)
endif()
add_custom_target(MotionEngineRuntime
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:MotionEngine::Core>" "$<TARGET_FILE:MotionEngine::Modal>" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
    VERBATIM)
if(TARGET MotionEngineBuild)
    add_dependencies(MotionEngineRuntime MotionEngineBuild)
endif()
if(MX_BUILD_GUI)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Svg PrintSupport Concurrent OpenGL)
    find_package(OpenGL REQUIRED)
    mx_add_docking()
    mx_import_qwt()
    add_subdirectory(GUI/src/MxMotionOcc)
endif()

if(BUILD_TESTING)
    add_executable(MotionOccDisplayNormalsSmoke tests/MotionOccDisplayNormalsSmoke.cpp)
    target_link_libraries(MotionOccDisplayNormalsSmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccDisplayNormalsSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccDisplayNormalsSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.DisplayNormals COMMAND MotionOccDisplayNormalsSmoke)

    add_executable(MotionOccNativeTopologySmoke tests/MotionOccNativeTopologySmoke.cpp)
    target_link_libraries(MotionOccNativeTopologySmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccNativeTopologySmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeTopologySmoke MotionOccRuntime)
    add_test(NAME MotionOcc.NativeTopology COMMAND MotionOccNativeTopologySmoke)

    add_executable(MotionOccNativeProjectSmoke tests/MotionOccNativeProjectSmoke.cpp)
    target_link_libraries(MotionOccNativeProjectSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccNativeProjectSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeProjectSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.NativeProject COMMAND MotionOccNativeProjectSmoke)

    add_executable(MotionOccNativeSmoke tests/MotionOccNativeSmoke.cpp)
    target_link_libraries(MotionOccNativeSmoke PRIVATE MotionOccRigid MotionEngine::Core MotionEngine::Modal)
    target_compile_options(MotionOccNativeSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeSmoke MxMotionMeshWorker MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.Native COMMAND MotionOccNativeSmoke)
    add_executable(MotionOccFlexResultsSmoke tests/MotionOccFlexResultsSmoke.cpp)
    target_link_libraries(MotionOccFlexResultsSmoke PRIVATE MotionOccRigid MotionEngine::Core MotionEngine::Modal)
    target_compile_options(MotionOccFlexResultsSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccFlexResultsSmoke MxMotionMeshWorker MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.FlexResults COMMAND MotionOccFlexResultsSmoke)
    add_executable(MotionOccNativeMassSmoke tests/MotionOccNativeMassSmoke.cpp)
    target_link_libraries(MotionOccNativeMassSmoke PRIVATE MotionOccData)
    add_dependencies(MotionOccNativeMassSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.NativeMass COMMAND MotionOccNativeMassSmoke)
    add_executable(MotionOccFunctionEditorSmoke tests/MotionOccFunctionEditorSmoke.cpp
        GUI/src/MxMotionOcc/NativeFunctionEditor.cpp)
    target_include_directories(MotionOccFunctionEditorSmoke PRIVATE GUI/src/MxMotionOcc)
    target_link_libraries(MotionOccFunctionEditorSmoke PRIVATE Qt5::Widgets Qwt::Qwt MotionEngine::Core)
    add_dependencies(MotionOccFunctionEditorSmoke MotionEngineRuntime)
    add_test(NAME MotionOcc.FunctionEditor COMMAND MotionOccFunctionEditorSmoke)
    set_tests_properties(MotionOcc.FunctionEditor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)
    add_executable(MotionOccSymbolsSmoke tests/MotionOccSymbolsSmoke.cpp GUI/src/MxMotionOcc/NativeMotionSymbols.cpp)
    target_include_directories(MotionOccSymbolsSmoke PRIVATE GUI/src/MxMotionOcc)
    target_link_libraries(MotionOccSymbolsSmoke PRIVATE MotionOccData)
    add_dependencies(MotionOccSymbolsSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.Symbols COMMAND MotionOccSymbolsSmoke)
    add_executable(MotionOccLoadExportSmoke tests/MotionOccLoadExportSmoke.cpp)
    target_link_libraries(MotionOccLoadExportSmoke PRIVATE MotionOccRigid)
    add_dependencies(MotionOccLoadExportSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.LoadExport COMMAND MotionOccLoadExportSmoke)
    add_executable(MotionOccNativeDynamicsSmoke tests/MotionOccNativeDynamicsSmoke.cpp)
    target_link_libraries(MotionOccNativeDynamicsSmoke PRIVATE MotionOccRigid)
    add_dependencies(MotionOccNativeDynamicsSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.NativeDynamics COMMAND MotionOccNativeDynamicsSmoke)
    set_tests_properties(MotionOcc.NativeDynamics PROPERTIES TIMEOUT 120)
    add_executable(MotionOccStaticServoSmoke tests/MotionOccStaticServoSmoke.cpp)
    target_link_libraries(MotionOccStaticServoSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccStaticServoSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccStaticServoSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.StaticServo COMMAND MotionOccStaticServoSmoke)
    set_tests_properties(MotionOcc.StaticServo PROPERTIES TIMEOUT 120)
    add_executable(MotionOccHydraulicSmoke tests/MotionOccHydraulicSmoke.cpp)
    target_link_libraries(MotionOccHydraulicSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccHydraulicSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccHydraulicSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.Hydraulic COMMAND MotionOccHydraulicSmoke)
    set_tests_properties(MotionOcc.Hydraulic PROPERTIES TIMEOUT 120)
    add_executable(MotionOccIndependentFlexSmoke tests/MotionOccIndependentFlexSmoke.cpp)
    target_link_libraries(MotionOccIndependentFlexSmoke PRIVATE MotionOccRigid MotionEngine::Core MotionEngine::Modal)
    target_compile_options(MotionOccIndependentFlexSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccIndependentFlexSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.IndependentFlex COMMAND MotionOccIndependentFlexSmoke)
    set_tests_properties(MotionOcc.IndependentFlex PROPERTIES TIMEOUT 180)
    add_executable(MotionOccModesSmoke tests/MotionOccModesSmoke.cpp)
    target_link_libraries(MotionOccModesSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccModesSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccModesSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.Modes COMMAND MotionOccModesSmoke)
    set_tests_properties(MotionOcc.Modes PROPERTIES TIMEOUT 120)
    foreach(target IN ITEMS MotionOccNativeMassSmoke MotionOccFunctionEditorSmoke MotionOccSymbolsSmoke MotionOccLoadExportSmoke MotionOccNativeDynamicsSmoke)
        target_compile_options(${target} PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    endforeach()
    set_tests_properties(MotionOcc.Native PROPERTIES TIMEOUT 180)

    add_executable(MotionOccNativeBindings tests/MotionOccNativeCases.cpp)
    target_link_libraries(MotionOccNativeBindings PRIVATE MotionOccData)
    target_compile_options(MotionOccNativeBindings PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeBindings MotionOccRuntime)

    add_executable(MotionOccNativeCases tests/MotionOccNativeCases.cpp)
    target_compile_definitions(MotionOccNativeCases PRIVATE MX_NATIVE_SOLVE_CASES)
    target_link_libraries(MotionOccNativeCases PRIVATE MotionOccRigid)
    target_compile_options(MotionOccNativeCases PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeCases MotionOccRuntime MotionEngineRuntime)

    add_executable(MotionOccNativeDocumentSmoke tests/MotionOccNativeDocumentSmoke.cpp)
    target_link_libraries(MotionOccNativeDocumentSmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccNativeDocumentSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccNativeDocumentSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.NativeDocument COMMAND MotionOccNativeDocumentSmoke)

    add_executable(MotionOccBackendSmoke tests/MotionOccBackendSmoke.cpp)
    target_link_libraries(MotionOccBackendSmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccBackendSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccBackendSmoke MxMotionMeshWorker)
    add_test(NAME MotionOcc.CadMesh COMMAND MotionOccBackendSmoke "$<TARGET_FILE:MxMotionMeshWorker>")
    set_tests_properties(MotionOcc.CadMesh PROPERTIES TIMEOUT 120)

    add_executable(MotionOccRigidSmoke tests/MotionOccRigidSmoke.cpp)
    target_link_libraries(MotionOccRigidSmoke PRIVATE MotionOccRigid)
    target_compile_options(MotionOccRigidSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccRigidSmoke MotionOccRuntime MotionEngineRuntime)
    add_test(NAME MotionOcc.Rigid COMMAND MotionOccRigidSmoke)
    set_tests_properties(MotionOcc.Rigid PROPERTIES TIMEOUT 120)

    add_executable(MotionOccPlaybackSmoke tests/MotionOccPlaybackSmoke.cpp)
    target_include_directories(MotionOccPlaybackSmoke PRIVATE GUI/src/MxMotionOcc "${model_dir}")
    target_link_libraries(MotionOccPlaybackSmoke PRIVATE MotionOccData)
    target_compile_options(MotionOccPlaybackSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccPlaybackSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.Playback COMMAND MotionOccPlaybackSmoke)

    add_executable(MotionOccCurveSmoke tests/MotionOccCurveSmoke.cpp GUI/src/MxMotionOcc/RigidResultSeries.cpp)
    target_include_directories(MotionOccCurveSmoke PRIVATE GUI/src/MxMotionOcc "${model_dir}")
    target_link_libraries(MotionOccCurveSmoke PRIVATE Qt5::Gui)
    target_compile_options(MotionOccCurveSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
    add_dependencies(MotionOccCurveSmoke MotionOccRuntime)
    add_test(NAME MotionOcc.Curves COMMAND MotionOccCurveSmoke)

    if(MX_BUILD_GUI)
        add_executable(MotionOccPlotSmoke tests/MotionOccPlotSmoke.cpp
            GUI/src/MxMotionOcc/RigidPlot.cpp GUI/src/MxMotionOcc/RigidPlot.h
            GUI/src/MxMotionOcc/RigidResultSeries.cpp
            GUI/src/CommonWidgets/mFoldSection.cpp GUI/src/CommonWidgets/mFoldSection.h)
        set_target_properties(MotionOccPlotSmoke PROPERTIES AUTOMOC ON)
        target_include_directories(MotionOccPlotSmoke PRIVATE GUI/src/MxMotionOcc
            GUI/src/MxMotionOcc/compat GUI/src/CommonWidgets "${model_dir}")
        target_compile_definitions(MotionOccPlotSmoke PRIVATE BUILD_STATIC)
        target_compile_options(MotionOccPlotSmoke PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
        target_link_libraries(MotionOccPlotSmoke PRIVATE Qt5::Widgets Qwt::Qwt)
        if(WIN32)
            add_dependencies(MotionOccPlotSmoke MxMotion)
        endif()
        add_test(NAME MotionOcc.Plot COMMAND MotionOccPlotSmoke)
        set_tests_properties(MotionOcc.Plot PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
    endif()
endif()

# Windows offscreen tests otherwise start without any font directory, which
# breaks text metrics and fails width/layout assertions that pass on Linux;
# qoffscreen is also not deployed next to the desktop executable.
if(WIN32 AND BUILD_TESTING AND TARGET Qt5::qmake)
    get_target_property(qmake_path Qt5::qmake IMPORTED_LOCATION)
    execute_process(COMMAND "${qmake_path}" -query QT_INSTALL_PLUGINS
        OUTPUT_VARIABLE qt_plugins OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY)
    foreach(mx_offscreen_test IN ITEMS MotionOcc.Plot MotionOcc.FunctionEditor)
        set_property(TEST ${mx_offscreen_test} APPEND PROPERTY ENVIRONMENT
            "QT_QPA_PLATFORM_PLUGIN_PATH=${qt_plugins}/platforms"
            "QT_QPA_FONTDIR=C:/Windows/Fonts")
    endforeach()

endif()
